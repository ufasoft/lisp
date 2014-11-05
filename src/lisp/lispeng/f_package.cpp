#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

#if UCFG_LISP_SPLIT_SYM

CSymbolValue *CSymMap::Find(RCString name) {
	iterator i = find(name);
	return i != end() ? i->second : 0;
}

bool CSymMap::FindCP(RCString name, CP& p) {
	p = 0;
	iterator i = find(name);
	if (i == end())
		return false;
	if (size_t idx = i->second-Lisp().m_symbolMan.Base)
		p = CP(idx<<TYPE_BITS) | TS_SYMBOL;
	return true;
}

#else

CSymbolValue *CSymMap::Find(RCString name) {	
	iterator i = find((CSymbolValue*)((byte*)&name-offsetof(CSymbolValue, m_s)));
	return i != end() ? i->first : 0;

/*!!!R
	CSymbolValue sv;
	sv.m_s = name;
	iterator i = find(&sv);
	return i != end() ? i->first : 0;
	*/
}

bool CSymMap::FindCP(RCString name, CP& p, byte& flags) {
	p = 0;
	CSymbolValue sv;
	sv.m_s = name;
	iterator i = find(&sv);
	if (i == end())
		return false;
	p = Lisp().FromSValue(i->first);
	flags = i->second;
	return true;
}

#endif

CPackage::~CPackage() {
}

void CPackage::Connect() {
	CLispEng& lisp = Lisp();
	CP pack = lisp.FromSValue(this);
	lisp.m_setPackage.insert(pack);
	lisp.m_mapPackage[m_name] = pack;
	for (size_t i=0; i<m_arNick.size(); ++i)
		lisp.m_mapPackage[m_arNick[i]] = pack;
}

void CPackage::Disconnect() {
	CLispEng& lisp = Lisp();
	lisp.m_setPackage.erase(lisp.FromSValue(this));
	lisp.m_mapPackage.erase(m_name);
	for (size_t i=0; i<m_arNick.size(); ++i)
		lisp.m_mapPackage.erase(m_arNick[i]);
	m_name = "";
	m_arNick.clear();
}

void CPackage::AdjustSymbols(ssize_t offset) {
	for (CSymMap::iterator i=m_mapSym.begin(); i!=m_mapSym.end(); ++i)
		(byte*&)*i += offset;
}

void CLispEng::MakePresent(CP sym, CPackage *pack, bool bCheckSymMacro) {
	CP package = FromSValue(pack);
	CSymbolValue *sv = AsSymbol(sym);
	byte flags = 0;
	if (package == m_packKeyword) {
		if (sv->HomePackage == 0) {
			if (bCheckSymMacro && SymMacroP(sym))
				E_Error();
			sv->m_fun |= SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL;
			sv->m_dynValue = sym;
			sv->HomePackage = package;
			flags |= PACKAGE_SYM_EXTERNAL;
		}
	} else if (sv->HomePackage == 0)
		sv->HomePackage = package;
#if UCFG_LISP_SPLIT_SYM
	m_mapSym.insert(make_pair(name, sv));
#else
	pack->m_mapSym.insert(make_pair(sv, flags));
#endif
}

bool CLispEng::FindInherited(RCString name, CPackage *pack, CP& r) {
	for (CP packs=pack->m_useList, car; SplitPair(packs, car);) {
		byte flags;
		if (ToPackage(car)->m_mapSym.FindCP(name, r, flags) && (flags & PACKAGE_SYM_EXTERNAL))
			return true;
	}
	return false;
}

CP CPackage::GetSymbol(CLispEng& lisp, const CLString& name, bool bFindInherited) {
	if (CSymbolValue *sv = m_mapSym.Find(name))
		return lisp.FromSValue(sv);
	CP r;
	if (bFindInherited && lisp.FindInherited(name, this, r))
		return r;
	r = lisp.FromSValue(lisp.CreateSymbol(name));
	lisp.MakePresent(r, this, false);
	return r;
}

void CLispEng::Unpresent(CP sym, CP package) {
	CPackage *pack = ToPackage(package);
	CSymbolValue *sv = ToSymbol(sym);

	CSymMap::iterator i = pack->m_mapSym.find(sv);
	if (i!=pack->m_mapSym.end() && i->first==sv) {
		pack->m_mapSym.erase(i);
		if (sv->HomePackage == package)
			sv->HomePackage = 0;
	}
}

CP CLispEng::FindSymbol(RCString name, CP package, CP& sym, byte& flags) {
	CPackage *pack = ToPackage(package);
	flags = 0;
	if (pack->m_mapSym.FindCP(name, sym, flags))
		return flags & PACKAGE_SYM_EXTERNAL ? S(L_K_EXTERNAL) : S(L_K_INTERNAL);
	else if (FindInherited(name, pack, sym))
		return S(L_K_INHERITED);
	return 0;
}

void CLispEng::F_FindSymbol() {
	CP package = FromSValue(FromPackageDesignator(Pop()));
	byte flags;
	if (!(m_arVal[1] = FindSymbol(AsTrueString(Pop()), package, m_r, flags)))
		m_r = 0;
	m_cVal = 2;
}

struct CWriteCallback {
	CLispEng *lisp;
	BlsWriter *pwr;
	CP m_packThis;
	map<uintptr_t, byte> m_m;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		byte flags = pp.second;
		if (pp.first->HomePackage == m_packThis)
			flags |= PACKAGE_SYM_OWNS;
		m_m.insert(make_pair(uintptr_t(pwr->m_obMap.m_arP[1][pp.first - lisp->m_symbolMan.Base]), flags));
	}
};

void CPackage::Write(BlsWriter& wr) {
	static_cast<BinaryWriter&>(wr) << m_name << m_arNick;
	wr << m_useList << m_docString;

	wr.WriteSize(m_mapSym.size());
	CWriteCallback c = { &Lisp(), &wr, Lisp().FromSValue(this) };
	m_mapSym.ForEachSymbolValue(c);

	uintptr_t prev = 0;
	for (map<uintptr_t, byte>::iterator i=c.m_m.begin(); i!=c.m_m.end(); ++i) {
		wr.WriteVal(i->second, i->first-prev);
		prev = i->first;
	}
}

void CPackage::Read(const BlsReader& rd) {
	CLispEng& lisp = Lisp();
	CP packThis = lisp.FromSValue(this);

	rd >> m_name >> m_arNick;
	rd >> m_useList >> m_docString;
	size_t count = rd.ReadSize();
	uintptr_t prev = 0;
	for (size_t i=0; i<count; ++i) {
		pair<byte, uintptr_t> pp = rd.ReadByteVal();
		byte flags = pp.first & ~PACKAGE_SYM_OWNS;
		prev += pp.second;

		CSymbolValue *sv = lisp.m_symbolMan.Base+prev;
		if (pp.first & PACKAGE_SYM_OWNS)
			sv->HomePackage = packThis;
#if UCFG_LISP_SPLIT_SYM
		m_mapSym.insert(make_pair(lisp.SymNameByIdx(prev), sv));
#else
		m_mapSym.insert(make_pair(sv, flags));
#endif
	}

	Connect();
}

CPackage *CLispEng::FromPackageDesignator(CP p) {
	if (p == V_U)
		p = Spec(L_S_PACKAGE);
	switch (Type(p)) {
	case TS_PACKAGE:
		break;
	case TS_CHARACTER:
	case TS_SYMBOL:
	case TS_ARRAY:
		{
			CP q;
			String s = FromStringDesignator(p);
			if (!Lookup(m_mapPackage, s, q))
				E_PackageErr(p);
			p = q;
		}
	}
	return ToPackage(p);
}

void CLispEng::MapSym(CP fun, CObjectSet& syms) {
	CP f = FromFunctionDesignator(fun);
	for (CObjectSet::iterator i(syms.begin()), e(syms.end()); i!=e; ++i) {
		Push(*i);
		Apply(f, 1);
	}
}

struct CMapAllCallback {
	CLispEng *lisp;
	CLispEng::CObjectSet *psyms;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		psyms->insert(lisp->FromSValue(pp.first));
	}
};

void CLispEng::F_MapAllSymbols() {
	CObjectSet syms;
	for (CPackageSet::iterator i=m_setPackage.begin(); i!=m_setPackage.end(); ++i) {
		CMapAllCallback c = { this, &syms };
		ToPackage(*i)->m_mapSym.ForEachSymbolValue(c);
	}
	MapSym(SV, syms);
	SkipStack(1);
}

struct CMapExternalCallback {
	CLispEng *lisp;
	CLispEng::CObjectSet *psyms;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		if (pp.second & PACKAGE_SYM_EXTERNAL)
			psyms->insert(lisp->FromSValue(pp.first));
	}
};

void CLispEng::F_MapExternalSymbols() {
	CObjectSet syms;
	CMapExternalCallback c = { this, &syms };
	FromPackageDesignator(Pop())->m_mapSym.ForEachSymbolValue(c);
	MapSym(SV, syms);
	SkipStack(1);
}

struct CMapSymbolsCallback {
	CLispEng *lisp;
	CLispEng::CObjectSet *psyms;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		psyms->insert(lisp->FromSValue(pp.first));
	}
};

struct CMapSymbols2Callback {
	CLispEng *lisp;
	CLispEng::CObjectSet *psyms;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		if (pp.second & PACKAGE_SYM_EXTERNAL)
			psyms->insert(lisp->FromSValue(pp.first));
	}
};

void CLispEng::F_MapSymbols() {
	CPackage *package = FromPackageDesignator(Pop());
	CObjectSet syms;	
	CMapSymbolsCallback c = { this, &syms };
	package->m_mapSym.ForEachSymbolValue(c);	

	for (CP p=package->m_useList, car; SplitPair(p, car);) {		
		CMapSymbols2Callback c = { this, &syms };
		ToPackage(car)->m_mapSym.ForEachSymbolValue(c);
	}

	MapSym(SV, syms);
	SkipStack(1);
}

void CLispEng::F_FindPackage() {
	CP p = Pop();
	if (Type(p) == TS_PACKAGE)
		m_r = p;
	else
		Lookup(m_mapPackage, FromStringDesignator(p), m_r);
}

void CLispEng::F_Intern() {
	CP pack = FromSValue(FromPackageDesignator(Pop()));
	String s = AsTrueString(Pop());
	m_cVal = 2;
	byte flags;
	if (!(m_arVal[1] = FindSymbol(s, pack, m_r, flags)))
		m_r = GetSymbol(s, pack);
}

void CLispEng::F_Unintern() {
	CPackage *package = FromPackageDesignator(SV);
	CP sym = SV1, pack = FromSValue(package), otherSym, symToShadowImport = 0;
	CSymbolValue *sv = ToSymbol(sym);
	String name = sv->m_s;
	byte flags;

	if (package->m_mapSym.FindCP(name, otherSym, flags) && sym==otherSym) {
		if (flags & PACKAGE_SYM_SHADOWING) {
			typedef map<CP, CP> CSym2Pack;
			CSym2Pack sym2pack;

			for (CP list=package->m_useList, car; SplitPair(list, car);) {
				if (AsPackage(car)->m_mapSym.FindCP(name, otherSym, flags) && (flags & PACKAGE_SYM_EXTERNAL) && sym2pack.find(otherSym)==sym2pack.end())
					sym2pack[otherSym] = car;
			}
			if (sym2pack.size() > 1) {
				CListConstructor lc;

				for (CSym2Pack::iterator i=sym2pack.begin(), e=sym2pack.end(); i!=e; ++i) {
					Call(S(L_FORMAT), 0, CreateString(AfxLoadString(IDS_SYM_FROM_PACK_WILL_SHADOW)), i->first, i->second);
					lc.Add(ListEx(CreateString(AsPackage(i->second)->m_name), m_r, i->first));
				}

				Push(lc);
				Push(S(L_PACKAGE_ERROR));
				Push(CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, IDS_E_UninterNameConflict)));
				Push(CreateString(AfxLoadString(IDS_E_UninterNameConflict)));
				Push(sym);
				Push(pack);
				symToShadowImport = CorrectableError(6);
			}
		}
		Unpresent(sym, pack);
		if (symToShadowImport)
			ShadowingImport(symToShadowImport, pack);
		m_r = V_T;
	}

	SkipStack(2);
}

void CLispEng::F_MakePackage() {
	CSPtr use = SV;
	int count = 0;
	if (use == V_U) {
		Push(m_packCL);
		Push(m_packEXT);
		count = 2;
	} else {
		for (CSPtr car; SplitPair(use, car); ++count) {
			Push(car);
			F_FindPackage();
			if (!m_r)
				E_PackageErr(car);
			Push(m_r);
		}
	}
	CP uses = Listof(count);
	SV = uses;					// SV = Listof() is not-determined
	vector<String> nicks;
	for (CSPtr nicknames=ToOptionalNIL(SV1), car; SplitPair(nicknames, car);) {
		Push(car);
		F_String();
		nicks.push_back(AsString(m_r));
	}
	
	Push(SV2);
	F_String();
	CPackage *package = CreatePackage(AsString(m_r), nicks);

	package->m_useList = SV;
	m_r = FromSValue(package);
	SkipStack(3);
}

void CLispEng::F_RenamePackage() {
	SV = ToOptionalNIL(SV);
	FromPackageDesignator(SV2)->Disconnect();
	for (CP car; SplitPair(SV, car);)
		AsPackage(SV2)->m_arNick.push_back(FromStringDesignator(car));
	SkipStack(1);
	String name = FromStringDesignator(Pop());
	CPackage *pack = AsPackage(Pop());
	pack->m_name = name;
	pack->Connect();
}

void CLispEng::F_ListAllPackages() {
	for (CPackageSet::iterator i=m_setPackage.begin(), e=m_setPackage.end(); i!=e; ++i)
		m_r = Cons(*i, m_r);
}

void CLispEng::F_PackageUseList() {
	m_r = FromPackageDesignator(Pop())->m_useList;
}

void CLispEng::F_PackageUsedByList() {
	m_r = 0;
	CP pack = FromSValue(FromPackageDesignator(Pop()));
	for (CPackageSet::iterator i=m_setPackage.begin(), e=m_setPackage.end(); i!=e; ++i)
		if (Memq(pack, AsPackage(*i)->m_useList))
			m_r = Cons(*i, m_r);
}

void CLispEng::F_DeletePackage() {
	switch (Type(SV)) {
	case TS_ARRAY:
	case TS_SYMBOL:
	case TS_CHARACTER:
		{
			String s = FromStringDesignator(SV);
			CP q;
			if (!Lookup(m_mapPackage, s, q)) {
				Push(CreateString("Ignore."), S(L_PACKAGE_ERROR), S(L_K_PACKAGE), SV);
				Push(CreateString("~S: There is no package with name ~S."), GetSubrName(m_subrSelf), SV4);
				Apply(S(L_CERROR_OF_TYPE), 7);
				m_r = 0;
				break;
			}
			SV = q;
		}
	case TS_PACKAGE:
		if (m_setPackage.find(SV) == m_setPackage.end())
			break;
		Push(SV);
		F_PackageUsedByList();
		if (m_r) {
			Push(CreateString("~*Delete ~S anyway."), S(L_PACKAGE_ERROR), S(L_K_PACKAGE), SV);
			Push(CreateString("~S: ~S is used by ~{~S~^, ~}."), GetSubrName(m_subrSelf), SV4, m_r);
			Apply(S(L_CERROR_OF_TYPE), 8);

			Push(SV);
			F_PackageUsedByList();
			if (m_r) {
				Push(m_r);
				for (CP car; SplitPair(SV, car);) {
					Push(SV1, car);
					F_UnusePackage();
				}
				SkipStack(1);
			}
		}
		{
			CPackage *package = AsPackage(SV);
			package->m_useList = 0;
			while (!package->m_mapSym.empty())
				Unpresent(FromSValue(package->m_mapSym.begin()->first), SV);
			package->Disconnect();
			m_r = V_T;
		}
		break;
	default:
		E_TypeErr(SV, List(S(L_OR), S(L_PACKAGE), S(L_STRING), S(L_SYMBOL), S(L_CHARACTER)));
	}

	SkipStack(1);
}

void CLispEng::F_PackageName() {
	CPackage *pack = FromPackageDesignator(Pop());
	if (m_setPackage.find(FromSValue(pack)) != m_setPackage.end())
		m_r = CreateString(pack->m_name);
}

void CLispEng::F_PackageNicknames() {
	vector<String> ar = FromPackageDesignator(Pop())->m_arNick;
	for (size_t i=ar.size(); i--;)
		m_r = Cons(CreateString(ar[i]), m_r);
}

void CLispEng::F_PackageShadowingSymbols() {
	CPackage *pack = FromPackageDesignator(SV);
	{
		CListConstructor lc;
		for (CSymMap::iterator i=pack->m_mapSym.begin(), e=pack->m_mapSym.end(); i!=e; ++i)
			if (i->second & PACKAGE_SYM_SHADOWING)
				lc.Add(FromSValue(i->first));
		m_r = lc;
	}
	SkipStack(1);
}

void CLispEng::CheckStringDesignator(CP name) {
	if (Type(name)==TS_CHARACTER || Type(name)==TS_SYMBOL || !name || StringP(name))
		return;
	E_TypeErr(name, List(S(L_OR), S(L_STRING), S(L_CHARACTER), S(L_SYMBOL)));
}

void CLispEng::CheckSymbol(CP name) {
	if (Type(name)==TS_SYMBOL || !name)
		return;
	E_TypeErr(name, S(L_SYMBOL));
}

void CLispEng::ApplySymbols(PFN_PackageFun packageFun, PFN_PackageCheckFun checkFun) {
	CPackage *pack = FromPackageDesignator(SV);
	CP package = FromSValue(pack);
	if (Type(SV1) != TS_CONS)
		(this->*checkFun)(SV1);
	else
		for (CP p=SV1, car; SplitPair(p, car);)
			(this->*checkFun)(car);

	if (Type(SV1) != TS_CONS)
		(this->*packageFun)(SV1, package);
	else
		for (CP p=SV1, car; SplitPair(p, car);)
			(this->*packageFun)(car, package);

	SkipStack(2);
	m_r = V_T;
}

void CLispEng::Shadow(CP name, CP package) {
	CPackage *pack = AsPackage(package);
	pack->m_mapSym.find(AsSymbol(pack->GetSymbol(_self, FromStringDesignator(name), false)))->second |= PACKAGE_SYM_SHADOWING;
}

void CLispEng::F_Shadow() {
	ApplySymbols(&CLispEng::Shadow, &CLispEng::CheckStringDesignator);
}

void CLispEng::ShadowingImport(CP sym, CP package) {
	CPackage *pack = AsPackage(package);

	do {
		CP p;
		byte flags;
		if (pack->m_mapSym.FindCP(AsSymbol(sym)->m_s, p, flags)) {
			if (p == sym)
				break;
			else
				Unpresent(sym, package);
		}
		MakePresent(sym, pack);
	} while (false);
	pack->m_mapSym.find(AsSymbol(sym))->second |= PACKAGE_SYM_SHADOWING;
}

void CLispEng::F_ShadowingImport() {
	ApplySymbols(&CLispEng::ShadowingImport, &CLispEng::CheckSymbol);
}

void CLispEng::Import(CP sym, CP package) {
	CPackage *pack = AsPackage(package);
	String name = AsSymbol(sym)->m_s;
	CP otherSym, otherSym2;
	byte flags;

	if (CP fr = FindSymbol(name, package, otherSym, flags)) {
		if (sym == otherSym) {
			if (fr == S(L_K_INHERITED))
				MakePresent(sym, pack);
			return;
		}

		CListConstructor lc;

		int ignCode = fr==S(L_K_INHERITED) ? IDS_IMPORT_AND_SHADOW : FindInherited(name, pack, otherSym2) ? IDS_IMPORT_AND_UNINTERN_AND_SHADOW : IDS_IMPORT_AND_UNINTERN;
		Push(S(L_IMPORT), CreateString(AfxLoadString(ignCode)), V_T);
		lc.Add(ListofEx(3));
		Push(S(L_IGNORE), CreateString(AfxLoadString(IDS_DO_NOTHING)), 0);
		lc.Add(ListofEx(3));

		Push(lc);
		Push(S(L_PACKAGE_ERROR));
		Push(CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, IDS_E_ImportConflict)));
		Push(CreateString(AfxLoadString(IDS_E_ImportConflict)));
		Push(sym);
		Push(package);
		Push(otherSym);
		if (CorrectableError(7)) {
			if (fr == S(L_K_INHERITED) || (flags & PACKAGE_SYM_SHADOWING))
				ShadowingImport(sym, package);
			else {
				Unpresent(sym, package);
				MakePresent(sym, pack);
			}
		}
	} else
		MakePresent(sym, pack);
}

void CLispEng::F_Import() {
	ApplySymbols(&CLispEng::Import, &CLispEng::CheckSymbol);
}

void CLispEng::Export(CP sym, CP package) {
	CP fr, otherSym;
	String name = AsSymbol(sym)->m_s;
	byte flags;

	if ((fr = FindSymbol(name, package, otherSym, flags)) && sym == otherSym) {
		if (fr == S(L_K_EXTERNAL))
			return;
	} else {
		CListConstructor lc;
		lc.Add(ListEx(S(L_IMPORT), CreateString(AfxLoadString(IDS_IMPORT_FIRSTLY)), V_T));
		lc.Add(ListEx(S(L_IGNORE), CreateString(AfxLoadString(IDS_DO_NOTHING)), 0));
		
		Push(lc);
		Push(S(L_PACKAGE_ERROR));			
		Push(CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, IDS_E_ImportBeforeExport)));
		Push(CreateString(AfxLoadString(IDS_E_ImportBeforeExport)));
		Push(GetSubrName(m_subrSelf));
		Push(sym);
		Push(package);
		if (!CorrectableError(7))
			return;
	}

	vector<pair<CP, CP> > resolvers;	
	Push(package);
	F_PackageUsedByList();
	Push(m_r);
	for (CP car; SplitPair(SV, car);) {
		if (FindSymbol(name, car, otherSym, flags) && otherSym!=sym) {

			CListConstructor lc;

			Push(CreateString(AsPackage(package)->m_name));
			Call(S(L_FORMAT), 0, CreateString(AfxLoadString(IDS_SYMBOL_TO_EXPORT)), sym);
			Push(m_r, sym);
			lc.Add(ListofEx(3));

			Push(CreateString(AsPackage(car)->m_name));
			Call(S(L_FORMAT), 0, CreateString(AfxLoadString(IDS_OLD_SYMBOL)), otherSym);
			Push(m_r, otherSym);
			lc.Add(ListofEx(3));		
			
			Push(lc);
			Push(S(L_PACKAGE_ERROR));			
			Push(CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, IDS_E_ExportConflict)));
			Push(CreateString(AfxLoadString(IDS_E_ExportConflict)));
			Push(sym);
			Push(package);
			Push(otherSym);
			Push(car);
			Push(package);
			resolvers.push_back(make_pair(CorrectableError(9), car));
		}
	}
	SkipStack(1);

	if (fr != S(L_K_INTERNAL))
		Import(sym, package);
	for (size_t i=0; i<resolvers.size(); ++i)
		ShadowingImport(resolvers[i].first, resolvers[i].second);
	AsPackage(package)->m_mapSym.find(AsSymbol(sym))->second |= PACKAGE_SYM_EXTERNAL;
}

void CLispEng::F_Export() {
	ApplySymbols(&CLispEng::Export, &CLispEng::CheckSymbol);
}

/*!!!R
void CLispEng::F_SysExport() {
	CPackage *pack = FromPackageDesignator(V_U);
	for (CSPtr syms=Pop(), sym; SplitPair(syms, sym);) {
#if UCFG_LISP_SPLIT_SYM
		CSymbolValue *sv = ToSymbol(sym);
		pack->m_mapExternalSym.insert(make_pair(SymNameByIdx(AsIndex(sym)), sv));
#else
		pack->m_mapExternalSym.insert(ToSymbol(sym));
#endif
	}
}
*/

void CLispEng::Unexport(CP sym, CP package) {
	CPackage *pack = AsPackage(package);

	CSymbolValue *sv = AsSymbol(sym);
	CSymMap::iterator it = pack->m_mapSym.find(sv);
	if (it != pack->m_mapSym.end()) {
		if (it->second & PACKAGE_SYM_EXTERNAL) {
			if (package == m_packKeyword)
				E_PackageErr(package);
			it->second &= ~PACKAGE_SYM_EXTERNAL;
		}
	} else {
		for (CP packs=pack->m_useList, car; SplitPair(packs, car);)
			if (ToPackage(car)->m_mapSym.find(sv) != AsPackage(car)->m_mapSym.end())
				return;
		E_PackageErr(package);
	}
}

void CLispEng::F_Unexport() {
	ApplySymbols(&CLispEng::Unexport, &CLispEng::CheckSymbol);
}

typedef unordered_map<String, map<CP, CP>> CName2Packages;

struct CUsePackageCallback {
	CLispEng *lisp;
	CName2Packages *pConflict;
	set<CP> *pPacksToUse;
	CP usingPack, pack;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		if (pp.second & PACKAGE_SYM_EXTERNAL) {
			CP sym = lisp->FromSValue(pp.first);
			String name = lisp->AsSymbol(sym)->m_s;
			if (pConflict->find(name) == pConflict->end()) {
				byte flags;
				CP symOther;
				if (lisp->FindSymbol(name, usingPack, symOther, flags)) {
					if (flags & PACKAGE_SYM_SHADOWING)
						return;
					(*pConflict)[name][symOther] = usingPack;
				}

				for (set<CP>::iterator j=pPacksToUse->begin(), je=pPacksToUse->end(); j!=je; ++j) {
					CP packToUse = *j;
					if (lisp->AsPackage(packToUse)->m_mapSym.FindCP(name, symOther, flags) && (flags & PACKAGE_SYM_EXTERNAL))
						(*pConflict)[name][symOther] = packToUse;
				}
				CName2Packages::iterator it = pConflict->find(name);
				if (it == pConflict->end() || it->second.size() < 2)
					pConflict->erase(it);
			}					
		}
	}
};

void CLispEng::F_UsePackage() {
	CPackage *package = FromPackageDesignator(SV);
	CP pack = FromSValue(package);
	set<CP> packsToUse;

	for (CP p=SV1, list=(Type(p)==TS_CONS ? p : 0); SplitPair(list, p) || p; p=0) {     // Listify
		CP q = FromSValue(FromPackageDesignator(p));
		if (!Memq(q, package->m_useList))
			packsToUse.insert(q);
	}

	CName2Packages conflicts;

	for (set<CP>::iterator i=packsToUse.begin(), e=packsToUse.end(); i!=e; ++i) {
		CP p = *i;

		CUsePackageCallback c = { this, &conflicts, &packsToUse, pack, p };
		AsPackage(p)->m_mapSym.ForEachSymbolValue(c);
	}

	int remain = conflicts.size();
	set<CP> resolvers;
	for (CName2Packages::iterator i=conflicts.begin(), e=conflicts.end(); i!=e; ++i, --remain) {
		String s = i->first; //!!!D
		CListConstructor lc;		
		for (map<CP, CP>::iterator j=i->second.begin(), je=i->second.end(); j!=je; ++j) {
			Push(CreateString(AsPackage(j->second)->m_name), j->second);
			Push(j->first);
			lc.Add(ListofEx(3));	
		}
		Push(lc, S(L_PACKAGE_ERROR),  CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, IDS_USE_PACKAGE_NAME_CONFLICT)));
		Push(CreateString(AfxLoadString(IDS_USE_PACKAGE_NAME_CONFLICT)), CreateFixnum(remain));
		Push(CreateString(i->first), pack);
		resolvers.insert(CorrectableError(7));	
	}

	for (set<CP>::iterator i=resolvers.begin(), e=resolvers.end(); i!=e; ++i)
		ShadowingImport(*i, pack);	

	for (set<CP>::iterator i=packsToUse.begin(), e=packsToUse.end(); i!=e; ++i)
		Push(*i);
	Push(package->m_useList);
	package->m_useList = ListofEx(packsToUse.size()+1);

	SkipStack(2);
	m_r = V_T;
}

void CLispEng::F_UnusePackage() {
	CPackage *package = FromPackageDesignator(Pop());
	for (CP p=Pop(), list=(Type(p)==TS_CONS ? p : 0); SplitPair(list, p) || p; p=0)     // Listify
		package->m_useList = Remq(FromSValue(FromPackageDesignator(p)), package->m_useList);
	m_r = V_T;
}


void CLispEng::F_PackageDocumentation() {
	m_r = FromPackageDesignator(Pop())->m_docString;
}

void CLispEng::F_SetfPackageDocumentation() {
	if (SV1 && !StringP(SV1))
		E_TypeErr(SV1, S(L_STRING)); //!!! must be CheckString
	FromPackageDesignator(SV)->m_docString = m_r = SV1;
	SkipStack(2);
}

} // Lisp::


