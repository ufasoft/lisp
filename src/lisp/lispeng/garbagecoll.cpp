/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

#if UCFG_LISP_PROFILE

CProfInfo *g_arProfBegin[LISP_SP_SIZE],
	**g_arProfEnd = g_arProfBegin;

#endif


CValueManBase::CValueManBase(size_t defaultSize, size_t szof, CTypeSpec ts)
	:	//!!!m_gc(gc),
#ifdef _DEBUG//!!!
	m_defaultSize((defaultSize+63) & ~(63))
#else
	m_defaultSize((defaultSize+BASEWORD_BITS-1) & ~(BASEWORD_BITS-1))
#endif
	,	m_sizeof(szof)
	,	m_pBase(0)
	,	m_pBitmap(0)
	,	m_ts(ts)
{
	Lisp().m_arValueMan.push_back(this);
}

void CValueManBase::Destroy() {
	byte *p = (byte*)m_pBase;

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	for (byte *pHeap=(byte*)m_pHeap; ;) {
		byte *next = pHeap ? pHeap : (byte*)m_pBase+m_size*m_sizeof;
	
		for (; p<next; p+=m_sizeof)
			Delete(p);
		p += m_sizeof;
		if (!pHeap)
			break;
		pHeap = ((byte**)next)[1];
	}
#else
	for (int i=0; i<m_size; i++, p+=m_sizeof)
		if (*p!=0xFF)
			Delete(p);
#endif
	FreeBitmap();
	AlignedFree(m_pBase);
}

void CValueManBase::FillEmpty(int i) {
	byte *q = (byte*)m_pBase+i*m_sizeof;
	for (; i<m_size; i++) {
		void **p = (void**)q;
#if !UCFG_LISP_GC_USE_ONLY_BITMAP
		*(byte*)p = 0xFF;
#endif
		q += m_sizeof;
		p[1] = q;
	}
	((void**)((byte*)m_pBase+(m_size-1)*m_sizeof))[1] = 0;
}

void CValueManBase::Allocate(size_t size) {
	m_size = size;
	m_pHeap = m_pBase = AlignedMalloc(m_size*m_sizeof+1, LISP_ALIGN); // one byte more need! see Fill
//!!!R	m_pBitmap = new BASEWORD[m_size/BASEWORD_BITS];
	FillEmpty(0);
}

size_t CValueManBase::ScanType(CObMap& obMap) {
	vector<CValueManBase*>& ar= Lisp().m_arValueMan;
	size_t idx = find(ar.begin(), ar.end(), this)-ar.begin();
	obMap.m_arP[idx].resize(m_size);
	//!!!    memset(obMap.m_arP[idx], 0xFF, m_size*4);//!!!
	size_t count = 0;
	byte *p = (byte*)m_pBase;
	for (int i=0; i<m_size; i++, p+=m_sizeof) {
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		if (BitOps::BitTest(m_pBitmap, i))
			obMap.m_arP[idx][i] = count++;
#else
		if (*p != 0xFF)
			obMap.m_arP[idx][i] = count++; //!!! m.SetAt(FromSValue(p, ts), count++);
#endif
	}
	return count;
}

void CValueManBase::Resize(size_t n) {
	//	TRC(0, "");

	if (n > (INT_PTR(1)<<VAL_BITS)) {
		Lisp().m_bOutOfMem = true;
		n = INT_PTR(1)<<VAL_BITS;
	}
	byte *pNew = (byte*)AlignedMalloc(n*m_sizeof+1, LISP_ALIGN); // one byte more need! see Fill
	
	int i, j;
	m_pHeap = 0;
	void **pPrev = &m_pHeap;
	for (i=0, j=0; j<m_size; i++, j++) {
		byte *p = pNew+i*m_sizeof,
			*q = (byte*)m_pBase+j*m_sizeof;
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		if (!BitOps::BitTest(m_pBitmap, j)) {
#else
		if (*q == 0xFF) {
#endif
			*pPrev = p;
#if !UCFG_LISP_GC_USE_ONLY_BITMAP
			*(byte*)p = 0xFF;
#endif
			pPrev = (void**)(p+sizeof(CP));
		} else {
			Copy(p, q);
			Delete(q);
		}
	}

	AlignedFree(m_pBase);
	delete[] (BASEWORD*)SwapRet(m_pBitmap, new BASEWORD[n/BASEWORD_BITS]);
	ssize_t offset = pNew-(byte*)m_pBase;
	switch (m_ts)
	{
	case TS_SYMBOL:
		Lisp().AdjustSymbols(offset);
		break;
#if UCFG_LISP_PROFILE
	case TS_INTFUNC:
	case TS_ARRAY:  //!!! was CLOSURE:
		byte *pEnd = (byte*)m_pBase+m_size*m_sizeof;
		for (CProfInfo **p=g_arProfBegin; p!=g_arProfEnd; p++) {
			byte *&q = (byte*&)*p;
			if (q>=(byte*)m_pBase && q<pEnd)
				q += offset;
		}
		break;
#endif
	}

	//!!!D  if ((void*)this == &Lisp().m_symbolMan)
	m_pBase = pNew;
	m_size = n;
	FillEmpty(i);
	*pPrev =  (byte*)m_pBase+i*m_sizeof;


//!!!R	void **end = (void**)((byte*)m_pBase+(m_size-1)*m_sizeof);
//!!!R	*(end+1) = SwapRet(m_pHeap, (byte*)m_pBase+i*m_sizeof);


	//!!!D *(end+1) = SwapRet(m_pHeap, end);
}

void CValueManBase::AfterApplyCheck(bool bSimple) {
	void *pHeapOriginal = m_pHeap;
	byte *p = (byte*)m_pBase,
		*pHeap = (byte*)m_pHeap;
	int countDel = 0; //!!!D
#if UCFG_LISP_GC_USE_ONLY_BITMAP
	int i = 0;
	void **pPrev = &m_pHeap;
	while (true) {
		byte *next = pHeap ? pHeap : (byte*)m_pBase+m_size*m_sizeof;
		
		int prevCountDel = countDel;
		for (; p<next; ++i, p+=m_sizeof) {
			if (!BitOps::BitTest(m_pBitmap, i)) {
				countDel++;
				if (!bSimple)
					Delete(p);				
//!!!R				*p = 0xFF;
				*pPrev = p;
				pPrev = (void**)(p+sizeof(CP));
			}
		}
		++i;
		p += m_sizeof;
		if (countDel != prevCountDel)
			*pPrev = pHeap;
		if (!pHeap)
			break;
		pPrev = (void**)(next+sizeof(CP));
		pHeap = ((byte**)next)[1];
	}
#else
	for (int i=0; i<m_size; ++i, p+=m_sizeof) {
		if (!BitOps::BitTest(m_pBitmap, i) && *p!=0xFF) {
			countDel++;
			if (!bSimple)
				Delete(p);
			*p = 0xFF;
			*(void**)(p+sizeof(CP)) = SwapRet(pHeap, p);
		}
	}
	m_pHeap = pHeap;
#endif
	if (!pHeapOriginal && countDel < m_size/2)
		Resize(m_size*2);
}

CLispGC::CLispGC()
	:	m_lispThreadKeeper((CLispEng*)this)
	,	m_mfnApplyCheckEx(&CLispGC::ApplyCheckExImp)

#	define LTYPE(ts, isnum, man)
#	define LMAN(typ, ts, man, init) , man(init)
#	include "typedef.h"
#	undef LTYPE
#	undef LMAN
{
}

CLispGC::~CLispGC() {
}

void CLispGC::InitValueMans() {
	for (int i=0; i<m_arValueMan.size(); i++)
		m_arValueMan[i]->Allocate();
}

CSValue *CLispGC::ToSValue(CP p) {
	size_t idx = AsIndex(p);
	switch (Type(p))
	{
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
		return m_consMan.Base+idx;
	case TS_SYMBOL:				return m_symbolMan.Base+idx;
		//!!!R  case TS_CCLOSURE:			return m_closureMan.Base+idx;
	case TS_PACKAGE:		return m_packageMan.Base+idx;
	case TS_STREAM:			return m_streamMan.Base+idx;
	case TS_INTFUNC:		return m_intFuncMan.Base+idx;

	case TS_FLONUM:
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		return m_consMan.Base+idx;
#else		
		return m_floatMan.Base+idx;
#endif

	case TS_BIGNUM:			return m_bignumMan.Base+idx;
	case TS_OBJECT:
	case TS_STRUCT:
	case TS_CCLOSURE:
	case TS_ARRAY:			return m_arrayMan.Base+idx;
	case TS_HASHTABLE:		return m_hashTableMan.Base+idx;
	case TS_READTABLE:		return m_readtableMan.Base+idx;
	case TS_PATHNAME:		return m_pathnameMan.Base+idx;
	case TS_WEAKPOINTER:	return m_weakPointerMan.Base+idx;
//!!!?	case TS_FF_PTR:			return m_consMan.Base+idx;
	default:
		Lisp().E_SeriousCondition();
	}
}

void CLispEng::CheckGarbageCollection() {
	E_Error(); //!!!
	/*!!!
	if (!m_bLockGC && m_nAllocCount >= ALLOCS_MAX)
	{
	m_nAllocCount = 0;
	Collect();
	}*/
}

void CLispGC::ApplyCheckArray(size_t idx) {
	CArrayValue *av = m_arrayMan.TryApplyCheck(idx);
	ApplyCheck(WITHOUT_FLAGS(av->m_fillPointer));
	ApplyCheck(av->m_displace);
	ApplyCheck(av->m_dims);
	if (!(av->m_flags & FLAG_Displaced) && av->m_elType==ELTYPE_T) {
		size_t size = 1;
		if (Type(av->m_dims) == TS_FIXNUM)
			size = AsNumber(av->m_dims);
		else {
			for (CP p=av->m_dims; p;) {
				CConsValue *cons = AsCons(p);
				size *= AsNumber(WITHOUT_FLAGS(cons->m_car));
				p = cons->m_cdr;
			}
		}
		for (int i=0; i<size; i++)
			ApplyCheck(av->m_pData[i]); 
	}
}

void CLispGC::ApplyCheckReadtable(size_t idx) {
	CReadtable *rt = m_readtableMan.TryApplyCheck(idx);
	ApplyCheck(WITHOUT_FLAGS(rt->m_case));  //!!!
	for (int i=0; i<_countof(rt->m_ar); i++) {
		CCharType& ct = rt->m_ar[i];
		ApplyCheck(ct.m_macro);
		ApplyCheck(ct.m_disp);
	}
	for (CReadtable::CCharMap::iterator it=rt->m_map.begin(); it!=rt->m_map.end(); ++it) {
		CCharType& ct = it->second;
		ApplyCheck(ct.m_macro);
		ApplyCheck(ct.m_disp);
	}
}

void CLispGC::ApplyCheckHashTable(size_t idx) {
	CHashTable *ht = m_hashTableMan.TryApplyCheck(idx);
	ApplyCheck(ht->m_rehashSize);
	ApplyCheck(ht->m_rehashThreshold);
	ApplyCheck(ht->m_pMap->m_func);
	for (CHashMap::iterator i=ht->m_pMap->begin(); i!=ht->m_pMap->end(); ++i) {
		ApplyCheck(i->first);
		ApplyCheck(i->second);
	}
}

void CLispGC::ApplyCheckIntFunc(size_t idx) {
	CIntFuncValue *ifv = m_intFuncMan.TryApplyCheck(idx);
	ApplyCheck(WITHOUT_FLAGS(ifv->m_form));  //!!!
	ApplyCheck(ifv->m_docstring);
	ApplyCheck(ifv->m_name);
	ApplyCheck(ifv->m_body);
	ApplyCheck(ifv->m_vars);
	ApplyCheck(ifv->m_optInits);
	ApplyCheck(ifv->m_keywords);
	ApplyCheck(ifv->m_keyInits);
	ApplyCheck(ifv->m_auxInits);
	ApplyCheck(ifv->m_env.m_varEnv);
	ApplyCheck(ifv->m_env.m_funEnv);
	ApplyCheck(ifv->m_env.m_blockEnv);
	ApplyCheck(ifv->m_env.m_goEnv);
	ApplyCheck(ifv->m_env.m_declEnv);
}

void __fastcall CLispGC::ApplyCheckSValue(CP p) {
	while (true) {
		DWORD_PTR idx = AsIndex(p);
		switch (Type(p))
		{
		case TS_CONS:          // Special case without m_type field
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
			{
				CConsValue *cons = m_consMan.TryApplyCheck(idx);
				ApplyCheck(cons->m_cdr);
				ApplyCheck(WITHOUT_FLAGS(cons->m_car));  //!!!
			}
			break;
		case TS_SYMBOL:        // Special case without m_type field
			{
				CSymbolValue *sv = m_symbolMan.TryApplyCheck(idx);
				ApplyCheck(sv->GetPropList());
				ApplyCheck(sv->m_dynValue);
				ApplyCheck(sv->GetFun());
			}
			break;
		case TS_PACKAGE:
			ApplyCheckPackage(idx);
			break;
		case TS_OBJECT:
		case TS_STRUCT:
		case TS_ARRAY:
		case TS_CCLOSURE:
			ApplyCheckArray(idx);
			break;
			/*!!!R
			{
			CClosure *cl = m_closureMan.TryApplyCheck(idx);
			ApplyCheck(WITHOUT_FLAGS(cl->NameOrClassVersion));  //!!!
			ApplyCheck(cl->CodeVec);
			ApplyCheck(cl->m_consts);
			}
			break;
			*/
		case TS_HASHTABLE:
			ApplyCheckHashTable(idx);
			break;
		case TS_READTABLE:
			ApplyCheckReadtable(idx);
			break;
		case TS_PATHNAME:
			{
				CPathname *pn = m_pathnameMan.TryApplyCheck(idx);
				ApplyCheck(WITHOUT_FLAGS(pn->m_host));  //!!!
				ApplyCheck(pn->m_dev);
				ApplyCheck(pn->m_dir);
				ApplyCheck(pn->m_name);
				ApplyCheck(pn->m_type);
				ApplyCheck(pn->m_ver);
			}
			break;
		case TS_STREAM:
			{
				CStreamValue *stm = m_streamMan.TryApplyCheck(idx);
				ApplyCheck(stm->m_in);
				ApplyCheck(stm->m_out);
				ApplyCheck(stm->m_reader);
				ApplyCheck(stm->m_writer);
				ApplyCheck(stm->m_elementType);
				ApplyCheck(stm->m_pathname);
			}
			break;

			/*!!!
			case TS_BIGNUM:
			m_bignumMan.TryApplyCheck(idx);
			break;
			case TS_FLONUM:
			m_floatMan.TryApplyCheck(idx);
			break;
			case TS_WEAKPOINTER:
			m_weakPointerMan.TryApplyCheck(idx);
			break;

			*/
		case TS_INTFUNC:
			ApplyCheckIntFunc(idx);
			break;
		default:
			Throw(E_FAIL);//!!!
			E_Error();
		}
		break;
	}
}

void CLispGC::ApplyCheckExImp(CP p) {
	int ts = Type(p);
	int tidx = g_arTS[ts];
	CValueManBase *vman = m_arValueMan[tidx];
	size_t i = AsIndex(p);
	//	if (!ImpBTS(vman->m_pBitmap, i)) //!!!
	if (!BitOps::BitTest(vman->m_pBitmap, i)) {		//!!!
		BitOps::BitTestAndSet(vman->m_pBitmap, (int)i);
		if (ts < TS_BIGNUM)
			m_arDeferred.push_back(p);
	}
}

void CLispGC::ApplyCheckExAndMapConsImp(CP p) {
	switch (Type(p))
	{
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
#if UCFG_LISP_FFI
	case TS_FF_PTR:
#endif
		BitOps::BitTestAndSet(m_pBitmapCons, AsIndex(p));
	}

	ApplyCheckExImp(p);
}

void CLispGC::ApplyCheckEx(CSymbolValue *sv) {
	int i = sv-m_symbolMan.Base;
	if (!BitOps::BitTest(m_symbolMan.m_pBitmap, i)) {		//!!!
		BitOps::BitTestAndSet(m_symbolMan.m_pBitmap, i);
		m_arDeferred.push_back(IDX_TS(i, TS_SYMBOL));
	}
}

void CLispGC::ApplyCheckPackage(size_t idx) {
	CPackage *pack = m_packageMan.TryApplyCheck(idx);
	ApplyCheck(WITHOUT_FLAGS(pack->m_useList));  //!!!
	ApplyCheck(pack->m_docString);
	for (CSymMap::iterator i=pack->m_mapSym.begin(); i!=pack->m_mapSym.end(); ++i) {
#if UCFG_LISP_SPLIT_SYM
		CSymbolValue *sv = i->second;
#else
		CSymbolValue *sv = i->first;
#endif
		ApplyCheckEx(sv);
	}
}

void CLispEng::ClearBitmaps() {
	for (int i=0; i<m_arValueMan.size(); i++)
		m_arValueMan[i]->GetCleanBitmap();
}

void CLispEng::FreeReservedBuffers() {
	{
		vector<CP> vEmpty;
		m_arDeferred.swap(vEmpty);
	}
	for (int i=0; i<m_arValueMan.size(); i++)
		m_arValueMan[i]->FreeBitmap();
}

#if !UCFG_WCE && UCFG_EXTENDED
class CGcBeeper {
public:
	CGcBeeper() {
		Console::Beep(1000, 50);
	}

	~CGcBeeper() {
		Console::Beep(500, 50);
	}
};
#endif


void CLispEng::CollectDeferred() {
	while (!m_arDeferred.empty()) {
		CP p = m_arDeferred.back();
		m_arDeferred.pop_back();
		ApplyCheckSValue(p);
	}
}

/*!!!R
void CLispEng::CollectDeferredAndMapConsImp() {
	while (!m_arDeferred.empty()) {
		CP p = m_arDeferred.back();
		m_arDeferred.pop_back();

		switch (Type(p))
		{
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
#if UCFG_LISP_FFI
		case TS_FF_PTR:
#endif
			BitOps::BitTestAndSet(m_pBitmapCons, AsIndex(p));
		}

		ApplyCheckSValue(p);
	}
}
*/

void CLispEng::CollectEx() {
	++m_nGC;

	DateTime now = DateTime::UtcNow();

#ifdef X_DEBUG
	CGcBeeper _gcBeeper;
#endif

	ClearBitmaps();

	m_arDeferred.clear();
	m_arDeferred.reserve(10000); //!!! param

	for (int i=0; i<m_cVal; i++)
		ApplyCheck(m_arVal[i]);

#if UCFG_LISP_TAIL_REC
	ApplyCheck(m_tailedFun);
#endif

	for (CPackageSet::iterator it=m_setPackage.begin(), e=m_setPackage.end(); it!=e; ++it)
		ApplyCheck(*it);

	ApplyCheck(m_env.m_varEnv);
	ApplyCheck(m_env.m_funEnv);
	ApplyCheck(m_env.m_blockEnv);
	ApplyCheck(m_env.m_goEnv);
	ApplyCheck(m_env.m_declEnv);

	ApplyCheck(m_specDot);//!!!
	//!!!  ApplyCheck(m_specRPar);
	ApplyCheck(V_EOF);
	ApplyCheck(V_U);
	ApplyCheck(V_D);
	ApplyCheck(V_SPEC);

	for (CP *p = m_pStack; p != m_pStackTop; p++)
		ApplyCheck(*p & MASK_WITHOUT_FLAGS);

	for (CTracedFuns::iterator i(m_tracedFuns.begin()), e(m_tracedFuns.end()); i!=e; ++i) {
		ApplyCheck(i->first);
		ApplyCheck(i->second);
	}

	ApplyCheck(0);//!!! *(byte*)m_consMan.Base |= 0x80; //!!! NIL

	CollectDeferred();

	//!!!Q	CollectFreachable();
	for (CMapFinalizable::iterator itf=m_mapFinalizable.begin(); itf!=m_mapFinalizable.end();) {
		ApplyCheck(itf->second);
		if (!MarkedP(itf->first)) {
			AddToFreachable(*itf);
			m_mapFinalizable.erase(itf++);
		} else
			++itf;
	}
	CollectFreachable();

	CollectDeferred();


	for (int i=0; i<m_weakPointerMan.m_size; i++) {
		if (BitOps::BitTest(m_weakPointerMan.m_pBitmap, i)) {
			CWeakPointer *wp = m_weakPointerMan.Base+i;
			if (CP q = WITHOUT_FLAGS(wp->m_p))
				if (!MarkedP(q))
					wp->m_p = V_U; //!!! ((DWORD&)wp->m_p & ~MASK_WITHOUT_FLAGS);
		}
	}

	m_consMan.AfterApplyCheck(true);
	for (int i=1, count=m_arValueMan.size(); i<count; i++)
		m_arValueMan[i]->AfterApplyCheck(false); //!!! may be need special check for arrays

		//!!!  m_alloc       .AfterApplyCheck();
	m_spanGC += DateTime::UtcNow()-now;
}

void CLispEng::F_GC() {
	CollectEx();
}

void CLispEng::F_Finalize() {
	m_mapFinalizable[SV1] = SV;
	SkipStack(2);
}

void CLispEng::F_MakeWeakPointer() {
	m_r = FromSValueT(CreateWeakPointer(Pop()), TS_WEAKPOINTER);
}

void CLispEng::F_WeakPointerValue() {
	CWeakPointer *wp = ToWeakPointer(Pop());
	if (m_arVal[1] = FromBool(wp->m_p != V_U))
		m_r = wp->m_p;
	m_cVal = 2;
}

void CLispEng::F_SetfWeakPointerValue() {
	m_r = Pop();
	ToWeakPointer(Pop())->m_p = m_r;
}

//!!!D__int64 g_ticksLastGC,
//!!!D        g_ticksGC;

void CLispEng::Collect() {
//		TRC(0, "\n--------------" );
	//!!!D	g_ticksGC = 0;
#if UCFG_LISP_PROFILE
	unsigned __int64 prev = __rdtsc();
#endif
	{
		PROF_POINT(AsProfiled(ToSymbol(S(L_S_GC))->GetFun()))
		F_GC();
	}
#if UCFG_LISP_PROFILE
	unsigned __int64 ticks = __rdtsc()-prev;
	for (CProfInfo **p=g_arProfBegin; p!=g_arProfEnd; p++)
		if (CProfInfo *pi = *p)
			pi->m_nTicks -= ticks;
#endif
	//!!!D	g_ticksGC = (g_ticksLastGC=RdTSC())-prev;
	if (m_bOutOfMem)
		Throw(E_OUTOFMEMORY); //!!!
}

} // Lisp::
