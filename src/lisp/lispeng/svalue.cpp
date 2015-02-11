#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

const byte CLispEng::s_ts2numberP[32] = {
#	define LTYPE(ts, isnum, man) isnum,
#	define LMAN(typ, ts, man, init)
#	include "typedef.h"
#	undef LTYPE
#	undef LMAN
};

CP __fastcall CLispEng::FromSValueT(CSValue *pv, CTypeSpec ts) {
	size_t idx;
	switch (ts) {
	case TS_CONS:
	case TS_RATIO:
	case TS_COMPLEX:
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	case TS_RANDOMSTATE:
#endif
	case TS_MACRO:
	case TS_SYMBOLMACRO:
	case TS_GLOBALSYMBOLMACRO:
	case TS_FUNCTION_MACRO:
	case TS_READLABEL:
		idx = (CConsValue*)pv-m_consMan.Base;
		break;
	case TS_SYMBOL:
		idx = (CSymbolValue*)pv-m_symbolMan.Base;
		break;
	case TS_PACKAGE:
		idx = (CPackage*)pv-m_packageMan.Base;
		break;
	case TS_STREAM:
		idx = (CStreamValue*)pv-m_streamMan.Base;
		break;
	case TS_INTFUNC:
		idx = (CIntFuncValue*)pv-m_intFuncMan.Base;
		break;
	case TS_FLONUM:
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		idx = (CConsValue*)pv-m_consMan.Base;
#else
		idx = (CFloat*)pv-m_floatMan.Base;
#endif
		break;
	case TS_BIGNUM:
		idx = (CBignum*)pv-m_bignumMan.Base;
		break;
	case TS_ARRAY:
	case TS_STRUCT:
	case TS_OBJECT:
	case TS_CCLOSURE:
		idx = (CArrayValue*)pv-m_arrayMan.Base;
		break;
		/*!!!D
		idx = (CObjectValue*)pv-m_objectMan.Base;
		break;*/
	case TS_HASHTABLE:
		idx = (CHashTable*)pv-m_hashTableMan.Base;
		break;
	case TS_READTABLE:
		idx = (CReadtable*)pv-m_readtableMan.Base;
		break;
	case TS_PATHNAME:
		idx = (CPathname*)pv-m_pathnameMan.Base;
		break;
	case TS_WEAKPOINTER:
		idx = (CWeakPointer*)pv-m_weakPointerMan.Base;
		break;
#if UCFG_LISP_FFI
	case TS_FF_PTR:
		idx = (CConsValue*)pv-m_consMan.Base;
		break;
#endif
	default:
		E_ProgramErr();
	}
	return (CP(idx)<<8)|ts;
}

CP CLispEng::FromFunctionDesignator(CP p) {
	CSPtr q;
	switch (Type(p)) {
	case TS_INTFUNC:
	case TS_SUBR:
	case TS_CCLOSURE:
		return p;
	case TS_SYMBOL:
		q =  ToSymbol(p)->GetFun();
		break;
	case TS_CONS:
		if (FunnameP(p)) {
			q = ToSymbol(GetSymProp(Car(Cdr(p)), S(L_SETF_FUNCTION)))->GetFun();
			break;
		}
	default:
		E_TypeErr(p, 0, IDS_E_IsNotAFunctionName, GetSubrName(m_subrSelf), p);
	}
	switch (Type(q)) {
	case TS_INTFUNC:
	case TS_SUBR:
	case TS_CCLOSURE:
		return q;
	default:
#ifdef X_TEST //!!!D
		Print(p);
		Disassemble(cerr, CurClosure);
#endif
		E_UndefinedFunction(p);
	}
}

/*!!!
CP __fastcall CLispEng::FromSValue(CArrayValue *pv, CTypeSpec ts)
{
return FromValue(((CP*)pv-m_alloc.m_base), ts);
}*/

CIntFuncValue *CLispEng::ToIntFunc(CP p) {
	if (Type(p) == TS_INTFUNC)
		return AsIntFunc(p);
	Error(E_LISP_BadArgumentType, p);
}

CArrayValue*  __fastcall CLispEng::ToArray(CP p) {
	if (Type(p) == TS_ARRAY)
		return AsArray(p);
	E_TypeErr(p, S(L_ARRAY));
	//!!!D  E_TypeError(E_LISP_IsNotArray, p);
}

CStructInst* __fastcall CLispEng::ToStruct(CP p) {
	if (Type(p) == TS_STRUCT)
		return (CStructInst*)AsArray(p);
	E_TypeErr(p, S(L_STRUCTURE_OBJECT));
}

bool CLispEng::VectorP(CP p) {
	if (Type(p) != TS_ARRAY)
		return false;
	return Type(AsArray(p)->m_dims) == TS_FIXNUM;
}

bool CLispEng::StringP(CP p) {
	if (!VectorP(p))
		return false;
	switch (AsArray(p)->GetElementType()) {
	case ELTYPE_CHARACTER:
	case ELTYPE_BASECHAR:
		return true;
	}
	return false;
}


CArrayValue *CLispEng::ToVector(CP p) {
	if (Type(p) == TS_ARRAY) {
		CArrayValue *av = AsArray(p);
		if (Type(av->m_dims) == TS_FIXNUM)
			return av;
	}
	E_TypeErr(p, S(L_VECTOR));
}

CReadtable *CLispEng::ToReadtable(CP p) {
	if (Type(p) == TS_READTABLE)
		return AsReadtable(p);
	E_TypeErr(p, S(L_READTABLE));
}

CHashTable *CLispEng::ToHashTable(CP p) {
	if (Type(p) == TS_HASHTABLE)
		return AsHashTable(p);
	E_TypeErr(p, S(L_HASH_TABLE));
}

CStreamValue *CLispEng::ToStream(CP p) {
	if (Type(p) == TS_STREAM)
		return AsStream(p);
	E_TypeErr(p, S(L_STREAM));
}

#if UCFG_LISP_BUILTIN_RANDOM_STATE

CRandomState *CLispEng::ToRandomState(CP p) {
	if (Type(p) == TS_RANDOMSTATE)
		return AsRandomState(p);
	E_TypeErr(p, S(L_RANDOM_STATE));
}
#endif

CSymbolMacro *CLispEng::ToSymbolMacro(CP p) {
	if (Type(p) == TS_SYMBOLMACRO)
		return (CSymbolMacro*)AsCons(p);
	E_TypeErr(p, S(L_SYMBOL_MACRO));
}

CSymbolMacro *CLispEng::ToGlobalSymbolMacro(CP p) {
	if (Type(p) == TS_GLOBALSYMBOLMACRO)
		return (CSymbolMacro*)AsCons(p);
	E_TypeErr(p, S(L_GLOBAL_SYMBOL_MACRO));
}

CFunctionMacro *CLispEng::ToFunctionMacro(CP p) {
	if (Type(p) == TS_FUNCTION_MACRO)
		return (CFunctionMacro*)AsCons(p);
	E_TypeErr(p, S(L_FUNCTION_MACRO));
}



/*!!!D
CLispEng::CLispSubr* _fastcall CLispEng::ToSpecialOperator(CP p)
{
if (Type(p) == TS_SPECIALOPERATOR)
return m_arSO+(AsIndex(p) & 0x3FF);
Error(E_LISP_BadArgumentType, p);
}
*/

CPackage *ToPackage(CP p) {
	if (Type(p) == TS_PACKAGE)
		return Lisp().AsPackage(p);
	Lisp().E_TypeErr(p, S(L_PACKAGE));
}

CPathname * __stdcall ToPathname(CP p) {
	if (Type(p) == TS_PATHNAME)
		return Lisp().AsPathname(p);
	Lisp().E_TypeErr(p, S(L_PATHNAME));
}

CWeakPointer *ToWeakPointer(CP p) {
	if (Type(p) == TS_WEAKPOINTER)
		return Lisp().AsWeakPointer(p);
	Lisp().E_TypeErr(p, S(L_WEAK_POINTER));
}

CArrayValue *ToObject(CP p) {
	if (Type(p) == TS_OBJECT)
		return Lisp().AsArray(p);
	Lisp().E_TypeErr(p, S(L_STANDARD_OBJECT));
}

CClosure *ToCClosure(CP p) {
	if (Type(p) == TS_CCLOSURE)
		return &Lisp().TheClosure(p);
	Lisp().Error(E_LISP_BadArgumentType, p);
}

CMacro *ToMacro(CP p) {
	if (Type(p) == TS_MACRO)
		return Lisp().AsMacro(p);
	Lisp().Error(E_LISP_BadArgumentType, p);
}

double AsFloatVal(CP p) {
	if (Type(p) == TS_FLONUM)
		return Lisp().AsFloat(p)->m_d;
	Lisp().E_TypeErr(p, S(L_FLOAT));
}

CFrameType AsFrameType(CP p) {
	if (Type(p) == TS_FRAMEINFO)
		return CFrameType(AsIndex(p) & FRAME_TYPE_MASK);
	Lisp().Error(E_LISP_BadArgumentType, p);
}

uintptr_t AsFrameTop(CP p) {
	if (Type(p) == TS_FRAMEINFO)
		return AsIndex(p) >> 8;
	Lisp().Error(E_LISP_BadArgumentType, p);
}

CP CLispEng::Cons(CP a, CP b) {
	CConsValue *cons;
	if (m_consMan.m_pHeap) {
		cons = (CConsValue*)exchange(m_consMan.m_pHeap, (CConsValue*)((uintptr_t*)m_consMan.m_pHeap)[1]);
		cons->m_car = a;
		cons->m_cdr = b;
	} else {
		Push(a, b);
		cons = CreateCons();
		cons->m_cdr = Pop();
		cons->m_car = Pop();
	}
	return FromSValue(cons);
}

CP CLispEng::CreateRatio(CP num, CP den) {
	Push(num, den);
	CRatio *pRatio = (CRatio*)m_consMan.CreateInstance();
	pRatio->m_denominator = Pop();
	pRatio->m_numerator = Pop();
	return FromSValueT(pRatio, TS_RATIO);
}

#if UCFG_LISP_BUILTIN_RANDOM_STATE
CRandomState *CLispEng::CreateRandomState(CP seed) {
	if (!seed)
		seed = CreateInteger(Ext::Random().m_seed);
	Push(seed);
	CRandomState *pRS = (CRandomState*)m_consMan.CreateInstance();
	pRS->m_rnd = Pop();
	pRS->m_stub = CSPtr();
	return pRS;
}
#endif

CP CLispEng::CreateMacro(CP expander, CP lambdaList) {
	Push(expander, lambdaList);
	CMacro *pSM = (CMacro*)m_consMan.CreateInstance();
	pSM->m_lambdaList = Pop();
	pSM->m_expander = Pop();
	return FromSValueT(pSM, TS_MACRO);
}

CSymbolMacro *CLispEng::CreateSymbolMacro(CP p) {
	Push(p);
	CSymbolMacro *pSM = (CSymbolMacro*)m_consMan.CreateInstance();
	pSM->m_macro = Pop();
	pSM->m_stub = 0;
	return pSM;
}

CFunctionMacro *CLispEng::CreateFunctionMacro(CP f, CP e) {
	Push(f, e);
	CFunctionMacro *pFM = (CFunctionMacro*)m_consMan.CreateInstance();
	pFM->m_expander = Pop();
	pFM->m_function = Pop();
	return pFM;
}

CP CLispEng::CreateReadLabel(CP lab) {
	Push(lab);
	CConsValue *cons = m_consMan.CreateInstance();
	cons->m_car = Pop();
	cons->m_cdr = 0;
	return FromSValueT(cons, TS_READLABEL);
}

CP CLispEng::CreateComplex(CP real, CP imag) {
	Push(real, imag);
	CComplex *pComplex = (CComplex*)m_consMan.CreateInstance();
	pComplex->m_imag = Pop();
	pComplex->m_real = Pop();
	return FromSValueT(pComplex, TS_COMPLEX);
}

CFloat *CLispEng::CreateFloat(double d) {
#if UCFG_LISP_GC_USE_ONLY_BITMAP
	CFloat *f = (CFloat*)m_consMan.CreateInstance();
#else
	CFloat *f = m_floatMan.CreateInstance();
#endif
	f->m_d = d;
	return f;
}

CBignum *CLispEng::CreateBignum(const BigInteger& i) {
#ifdef _X_DEBUG //!!!D
	static int count = 0;
	if (++count==4)
		g_b = true;
	cout << i << endl;
#endif
	return m_bignumMan.CreateInstance(i);
}

CP CLispEng::CreateKeyword() {
	CSymbolValue *sv = m_symbolMan.CreateInstance();
	sv->m_fun |= SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL;
	return sv->m_dynValue = FromSValue(sv);
}

CClosure *CLispEng::CreateClosure(size_t len) {
	return (CClosure*)CreateVector(len);
}

CClosure *CLispEng::CopyClosure(CP oldClos) {
	CClosure *oc = ToCClosure(oldClos);
	size_t len = AsArray(oldClos)->DataLength;
	CClosure *nc = CreateClosure(len);
	nc->NameOrClassVersion = oc->NameOrClassVersion;
	nc->CodeVec = oc->CodeVec;
	memcpy(nc->Consts, oc->Consts, len*sizeof(CP));
	return nc;
}

CStreamValue *CLispEng::CreateStream(int subtype, CP in, CP out, byte flags) {
	Push(in, out);
	CStreamValue *stm = m_streamMan.CreateInstance();
	stm->m_elementType = S(L_CHARACTER);
	stm->m_subtype = (byte)subtype;
	stm->m_out = Pop();
	stm->m_in = Pop();
	stm->m_flags = flags;
	//!!!  stm->m_pStm = pStm;
	//!!!	stm->m_reader = AsSymbol(GetSymbol("%READ-CHAR", m_packSYS))->GetFun();
	//!!!	stm->m_writer = AsSymbol(GetSymbol("%WRITE-CHAR", m_packSYS))->GetFun();
	return stm;
}

CP CLispEng::CreateSynonymStream(CP p) {
	return FromSValue(CreateStream(STS_SYNONYM_STREAM, p));
}

CPackage *CLispEng::CreatePackage(RCString name, const vector<String>& nicks) {
	CPackage *pack = m_packageMan.CreateInstance();
	pack->m_name = name;
	pack->m_arNick = nicks;
	pack->Connect();
	return pack;
}

CWeakPointer *CLispEng::CreateWeakPointer(CP p) {
	Push(p);
	CWeakPointer *wp = m_weakPointerMan.CreateInstance();
	wp->m_p = Pop();
	return wp;
}

CArrayValue *CLispEng::CreateArray() {
	return m_arrayMan.CreateInstance();
}

CArrayValue *CLispEng::CreateVector(size_t size, byte elType) {
	CArrayValue *av = CreateArray();
	av->m_pData = av->CreateData(av->m_elType=elType, av->m_dims=CreateFixnum((INT_PTR)size), 0);
	return av;

	/*!!!  Push(FromSValue(av));
	av->m_pData = av->CreateData(ELTYPE_T, av->m_dims=FromSValue(Cons(CreateInteger(size), 0)), 0);
	SkipStack(1);
	return av;*/
}

CP CLispEng::CopyVector(CP p) {
	Push(p);
	CArrayValue *oav = AsArray(p);
	size_t size = oav->DataLength;
	CArrayValue *av = CreateVector(size, oav->GetElementType());
	oav = AsArray(Pop());
	memcpy(av->m_pData, oav->m_pData, oav->GetByteLength());
	return FromSValue(av);
}

CP CLispEng::CreateString(RCString s) {
	size_t size = s.length();
	CArrayValue *av = CreateVector(size, ELTYPE_CHARACTER);
	memcpy(av->m_pData, (const String::value_type*)s, size*sizeof(String::value_type));
	return FromSValue(av);
}

CReadtable *CLispEng::CreateReadtable() {
	return m_readtableMan.CreateInstance();
}

CHashTable *CLispEng::CreateHashTable() {
	return m_hashTableMan.CreateInstance();
}

void CLispEng::AdjustSymbols(ssize_t offset) {
	byte *p = (byte*)m_packageMan.m_pBase;

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	for (byte *pHeap=(byte*)m_packageMan.m_pHeap; ;) {
		byte *next = pHeap ? pHeap : (byte*)m_packageMan.m_pBase+m_packageMan.m_size*m_packageMan.m_sizeof;
	
		for (; p<next; p+=m_packageMan.m_sizeof)
			((CPackage*)p)->AdjustSymbols(offset);
		p += m_packageMan.m_sizeof;
		if (!pHeap)
			break;
		pHeap = ((byte**)next)[1];
	}
#else
	for (int i=0; i<m_packageMan.m_size; i++, p+=m_packageMan.m_sizeof)
		if (*p!=0xFF)
			((CPackage*)p)->AdjustSymbols(offset);
#endif
}

CIntFuncValue::CIntFuncValue(const CIntFuncValue& ifv) {
	operator=((CIntFuncValue&)ifv);
}

CP CIntFuncValue::GetField(size_t idx) {		//!!!CLISP
	switch (idx) {
	case 0: return m_name;
	case 1: return m_form;
	case 2: return m_docstring;
	case 3: return m_body;
	case 4: return m_env.m_varEnv;
	case 5: return m_env.m_funEnv;
	case 6: return m_env.m_blockEnv;
	case 7: return m_env.m_goEnv;
	case 8: return m_env.m_declEnv;
	case 9: return m_vars;
	case 10: return m_optInits;
	case 11: return m_keyInits;
	case 12: return CreateFixnum(m_nReq);
	case 13: return CreateFixnum(m_nOpt);
	case 14: return m_auxInits;
	case 16: return m_keywords;
	case 18: return FromBool(m_bAllowFlag);
	case 19: return FromBool(m_bRestFlag);
	default:
		if (idx < 12)
			return ((CP*)this)[idx];
		else
			Lisp().E_ProgramErr();
	}
}

void CIntFuncValue::Write(BlsWriter& wr) {
	wr << m_name << m_docstring << m_form << m_body
		<< m_env.m_varEnv << m_env.m_funEnv << m_env.m_blockEnv << m_env.m_goEnv << m_env.m_declEnv
		<< m_vars
		<< m_optInits << m_keywords << m_keyInits << m_auxInits;
	(BinaryWriter&)wr << m_nReq << m_nOpt << m_nSpec << m_nKey << m_nAux << m_bAllowFlag << m_bRestFlag;
	if (m_varFlags) {
		uint16_t count = uint16_t(Lisp().ToArray(m_vars)->TotalSize()-m_nSpec);
		(BinaryWriter&)wr << count;
		for (int i=0; i<count; i++)
			(BinaryWriter&)wr << m_varFlags[i];
	} else
		(BinaryWriter&)wr << uint16_t(0);
}

void CIntFuncValue::Read(const BlsReader& rd) {
	rd >> m_name >> m_docstring >> m_form >> m_body
		>> m_env.m_varEnv >> m_env.m_funEnv >> m_env.m_blockEnv >> m_env.m_goEnv >> m_env.m_declEnv
		>> m_vars
		>> m_optInits >> m_keywords >> m_keyInits >> m_auxInits;
	(BinaryReader&)rd >> m_nReq >> m_nOpt >> m_nSpec >> m_nKey >> m_nAux >> m_bAllowFlag >> m_bRestFlag;
	uint16_t count = rd.ReadUInt16();
	if (count) {
		m_varFlags = new byte[count];
		for (int i=0; i<count; i++)
			(BinaryReader&)rd >> m_varFlags[i];
	}
}

CP CClosure::get_VEnv() {
	return Consts[0];
}

void CClosure::put_VEnv(CP venv) {
	Consts[0] = venv;
}

/*!!!
void CClosure::Read(CBlsStream& stm)
{
stm >> NameOrClassVersion >> CodeVec >> m_consts;
}

void CClosure::Write(CBlsStream& stm)
{
stm << NameOrClassVersion << CodeVec << m_consts;
}
*/

CStreamValue::CStreamValue()
:	m_nStandard(-1)
,	m_nLine(1)
,	m_nPos(0)
,	m_bUnread(false)
,	m_bEchoed(false)
,	m_bMultiLine(false)
,	m_bClosed(false)
,	m_bSigned(false)
,	m_nCur(0)
,	m_nEnd(0)
,	m_elementBits(8)
,	m_curOctet(-1)
,	m_mode(FileMode::Open)
{
	m_type = TS_STREAM;
	m_subtype = STS_STREAM;
}

void CStreamValue::Write(BlsWriter& wr) {
	CLispEng& lisp = Lisp();
	(BinaryWriter&)wr << m_subtype << m_nStandard << m_nLine << m_nPos << m_nCur << m_nEnd << m_flags;
	wr << m_in << m_out << m_reader << m_writer << m_elementType << m_pathname;
	/*!!!
	byte i;
	for (i=0; i<lisp.m_arStream.size(); i++)
	if (lisp.m_arStream[i] == m_pStm)
	{
	(Stream&)stm << i;
	return;
	}
	E_Error();*/
}

void CStreamValue::Read(const BlsReader& rd) {
	(BinaryReader&)rd >> m_subtype >> m_nStandard >> m_nLine >> m_nPos >> m_nCur >> m_nEnd >> m_flags;
	rd >> m_in >> m_out >> m_reader >> m_writer >> m_elementType >> m_pathname;
	/*!!!
	byte n;
	(Stream&)stm >> n;
	m_pStm = Lisp().m_arStream[n];*/
}

int64_t& CStreamValue::LinePosRef() {
	if (m_stream)
		return m_stream->m_nPos;
	if (m_nStandard != -1)
		return Lisp().m_streams[m_nStandard]->m_nPos;
	return m_nPos;
}

void CHashTable::Write(BlsWriter& wr) {
	m_pMap->Write(wr << m_rehashSize << m_rehashThreshold);
}

void CHashTable::Read(const BlsReader& rd) {
	m_pMap->Read(rd >> m_rehashSize >> m_rehashThreshold);
}

void CSymbolValue::Write(BlsWriter& wr) {
	CLispEng& lisp = Lisp();
	
	BinaryWriter& bwr = wr;
	
#if UCFG_LISP_SPLIT_SYM
	String name = lisp.SymNameByIdx(this-lisp.m_symbolMan.Base);
#else
	String name = m_s;
#endif
	map<CP, String>::iterator i = lisp.m_mapObfuscated.find(lisp.FromSValue(this));
	if (i != lisp.m_mapObfuscated.end())
		name = i->second;
	byte flags = byte((m_fun & ~MASK_WITHOUT_FLAGS)|((m_fun>>30)&3));
	if (name == lisp.GetStaticSymbolNamePack(this-lisp.m_symbolMan.Base).first)
		flags |= STREAM_FLAG_STANDARD_NAME;
	bwr << flags;

	if (!(flags & STREAM_FLAG_STANDARD_NAME))
		wr << name;
	wr << GetFun() << m_dynValue << GetPropList();
}

void CSymbolValue::Read(const BlsReader& rd) {
	CSPtr fun, propList;

	const BinaryReader& brd = rd;

	byte flags;
	brd >> flags;
	String name;
	if (flags & STREAM_FLAG_STANDARD_NAME)
		name = Lisp().GetStaticSymbolNamePack(this-Lisp().m_symbolMan.Base).first;
	else
		rd >> name;
#if UCFG_LISP_SPLIT_SYM
		Lisp().SymNameByIdx(this-Lisp().m_symbolMan.Base) = name;
#else
		m_s = name;
#endif		
	rd >> fun >> m_dynValue >> propList;
	m_fun = DWORD(flags & ~MASK_WITHOUT_FLAGS) | (DWORD(flags & 3)<<30);
	SetFun(fun);
	SetPropList(propList);
}

void CBignum::Read(const BlsReader& rd) {
	rd >> m_int;
#ifdef _X_DEBUG //!!!D
	static int count;
	cout << ++count << endl;
#endif
}

void CBignum::Write(BlsWriter& wr) {
	wr << m_int;
}

BlsWriter& operator<<(BlsWriter& wr, const CCharType& ct) {
	(BinaryWriter&)wr << (byte)ct.m_syntax << (uint16_t)ct.m_traits << (byte)ct.m_bTerminating;
	wr << ct.m_macro << ct.m_disp;
	return wr;
}

const BlsReader& operator>>(const BlsReader& rd, CCharType& ct) {
	byte syntax, bTerminating;
	uint16_t traits;
	rd >> syntax >> traits >> bTerminating;
	rd >> ct.m_macro >> ct.m_disp;
	ct.m_syntax = (CSyntaxType)syntax;
	ct.m_traits = traits;
	ct.m_bTerminating = bTerminating;
	return rd;
}

void CReadtable::Write(BlsWriter& wr) {
	wr << m_case;
	for (int i=0; i<size(m_ar); i++)
		wr << m_ar[i];
	wr.WriteSize(m_map.size());
	for (CCharMap::iterator it=m_map.begin(); it!=m_map.end(); ++it) {
		(BinaryWriter&)wr << it->first;
		wr << it->second;
	}
}

void CReadtable::Read(const BlsReader& rd) {
	rd >> m_case;
	for (int i=0; i<size(m_ar); i++)
		rd >> m_ar[i];
	size_t num = rd.ReadSize();
	while (num--) {
		Char16 ch = rd.ReadUInt16();
		rd >> m_map[ch];
	}
}

void CPathname::Write(BlsWriter& wr) {
	wr << m_host << m_dev << m_dir << m_name << m_type << m_ver;
}

void CPathname::Read(const BlsReader& rd) {
	rd >> m_host >> m_dev >> m_dir >> m_name >> m_type >> m_ver;
}





} // Lisp::
