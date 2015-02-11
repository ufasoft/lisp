#include <el/ext.h>

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#include "lispeng.h"

namespace Lisp {

CObMap::CObMap()
	:	m_arP(Lisp().m_arValueMan.size())
{
	//!!!D    memset(m_arP, 0, 4*MAN_COUNT);
}


struct CListElement {
	CP m_p;
	size_t m_len;

	CListElement(CP p)
		:	m_p(p)
		,	m_len(Lisp().Length(p))
	{}

	bool operator<(const CListElement& le) const { return m_len>le.m_len; }
};

struct CLenLists {
	size_t m_len;
	vector<CP> m_ar;
};

int __cdecl CompareListElement(const void *v1, const void *v2) {
	CListElement &p1 = *(CListElement*)v1,
		&p2 = *(CListElement*)v2;
	return p2.m_len - p1.m_len;
}

CBlsHeader::CBlsHeader(bool bVerifyVersion)
	:	m_sig(BINLISP_SIG)
{
	if (bVerifyVersion) {
#if UCFG_USE_POSIX
		uint64_t ar[4] = { VER_PRODUCTVERSION };
		m_ver = (ar[0]<<48)|(ar[1]<<32);
#else
		Version ver = FileVersionInfo().GetProductVersionN();		
		m_ver = (uint64_t(ver.Major)<<48)|(uint64_t(ver.Minor)<<32);
#endif
	}
	//!!!  m_verLastCompatible = m_ver
}

BlsBase::BlsBase()
	:	m_arCount(Lisp().m_arValueMan.size())
{}


void BlsWriter::WriteLens() {
	for (size_t i=0; i<m_arCount.size(); ++i)
		WriteSize(m_arCount[i]);
}

void BlsReader::ReadLens() {
	for (size_t i=0; i<m_arCount.size(); ++i)
		m_arCount[i] = ReadSize();
}

const int g_arTS[32] = {
#	define LTYPE(ts, isnum, man) ENUM_##man, 
#	define LMAN(typ, ts, man, init)
#	include "typedef.h"
#	undef LTYPE
};

void BlsWriter::WriteVal(byte ts, uintptr_t idx) {
#if LISP_DYN_IDX_SAVE
	int n = 7;
	if (idx < 256)
		n = 1;
	else if (idx < 65536) {
		n = 2;
		ts += 64;
	} else if (idx < (1<<24)) {
		n = 3;
		ts += 128;
	} else
		ts += 128+64;
#else
	int n = sizeof(CP)-1;
#endif
	WriteBytes(1, ts);
	WriteBytes(n, idx);
}

void BlsWriter::WriteSignedVal(byte ts, intptr_t idx) {
#if LISP_DYN_IDX_SAVE
	int n = 7;
	if (idx < 128 && idx >= -128)
		n = 1;
	else if (idx < 32768 && idx >= -32768) {
		n = 2;
		ts += 64;
	} else if (idx < (1<<23) && idx >= -(1<<23)) {
		n = 3;
		ts += 128;
	}
	else
		ts += 128+64;
#else
	int n = sizeof(CP)-1;
#endif
	WriteBytes(1, ts);
	WriteBytes(n, idx);
}

const byte TS_NIL = TS_READLABEL;  // TS_READ_LABEL cannot be in image, so use this number as NIL tag

void BlsWriter::WriteCP(const CSPtr& p) {
	switch (byte ts = (byte)Type(p)) {
	case TS_CHARACTER:
		{
			wchar_t ch = AsChar(p);
			if (ch < 256) {
				WriteBytes(1, ts|0x80);
				WriteBytes(1, ch);
			} else {
				WriteBytes(1, ts);
				WriteBytes(2, ch);
			}
		}
		break;
	case TS_FIXNUM:
		WriteSignedVal(ts, LONG_PTR(p)>>VALUE_SHIFT);
		break;		
	case TS_FRAME_PTR:
		WriteVal(ts, AsIndex(p));
		break;
	case TS_SPECIALOPERATOR:		
	case TS_SUBR:
		WriteBytes(2, ((AsIndex(p) & 0x3FF)<<6) | ts);
		break;
	case TS_READLABEL:
		E_Error();
	default:
		if (!p)
			WriteBytes(1, TS_NIL);
		else {
			int idx = g_arTS[ts];
			if (idx == -1)
				Throw(E_FAIL);
			WriteVal(ts, m_obMap.m_arP[idx][AsIndex(p)]);
		}
	}
}

pair<byte, uintptr_t> BlsReader::ReadByteVal() const {
	byte b = (byte)ReadBytes(1);

	int c = b >> 6;
	b &= 0x1F;
	uintptr_t r = 0;
	switch (c) {
#if LISP_DYN_IDX_SAVE
	case 0:
		r = ReadBytes(1);
		break;
	case 1: 
		r = ReadBytes(2);
		break;
	case 2: 
		r = ReadBytes(3);
		break;
	default:
		r = ReadBytes(7);
		break;
#else
	default:
		r = ReadBytes(sizeof(CP)-1);
		break;
#endif
	}
	return pair<byte, DWORD_PTR>(b, r);
}

CP BlsReader::ReadVal(byte ts) const {
	int c = ts >> 6;
	ts &= 0x1F;
	intptr_t r;
	switch (c) {
#if LISP_DYN_IDX_SAVE
	case 0:
		if (ts == TS_FIXNUM)
			r = (signed char)ReadBytes(1);
		else
			r = ReadBytes(1);
		break;
	case 1: 
		if (ts == TS_FIXNUM)
			r = (int16_t)ReadBytes(2);
		else
			r = ReadBytes(2);
		break;
	case 2: 
		r = (int32_t)(ReadBytes(3) << 8) >> 8;
		break;
#endif
	default: 
		r = ReadBytes(sizeof(CP)-1);
	}
	return FromValue(r, CTypeSpec(ts));	
}

const BlsReader& operator>>(const BlsReader& rd, CSPtr& p) {
	byte ts = rd.ReadBytes(1);
	switch (ts & 0x1F) {
	case TS_NIL:
		p = 0;
		break;
	case TS_CHARACTER:
		p = FromValue(rd.ReadBytes((ts & 0x80) ? 1 : 2), TS_CHARACTER);
		break;
	case TS_FIXNUM:
		p = rd.ReadVal(ts);
		break;
	case TS_FRAME_PTR:
		p = rd.ReadVal(ts);
		break;
	case TS_SPECIALOPERATOR:
		p = CLispEng::CreateSpecialOperator((rd.ReadBytes(1)<<2) | (ts>>6));
		break;
	case TS_SUBR:
		p = CLispEng::CreateSubr((rd.ReadBytes(1)<<2) | (ts>>6));
		break;
	default:
		int idx = g_arTS[ts & 0x1F];
		if (idx == -1) {
			Throw(E_FAIL);
		}
		p = rd.ReadVal(ts);
	}
	return rd;
}

vector<String> CLispEng::ReadBinHeader(const BinaryReader& rd) {
	CBlsHeader header(m_bVerifyVersion);
	rd.ReadStruct(header);
	if (header.m_sig != BINLISP_SIG)
		Throw(E_EXT_InvalidFileHeader);
#if UCFG_USE_POSIX
		uint64_t arv[4] = { VER_PRODUCTVERSION };
		uint64_t ver = (arv[0]<<48)|(arv[1]<<32);
#else
		Version v = FileVersionInfo().GetProductVersionN();		
		uint64_t ver = (uint64_t(v.Major)<<48)|(uint64_t(v.Minor)<<32);
#endif
	if (m_bVerifyVersion && header.m_ver != ver)
		Throw(E_EXT_IncompatibleFileVersion);
	CStringVector ar;
	rd >> ar;
	return ar;
}

void CLispEng::LoadMem(Stream& bstm) {
	BlsReader rd(bstm);

	uint32_t nNonCons = 0;
	rd.ReadLens();
#if UCFG_LISP_GC_USE_ONLY_BITMAP
	size_t nFloat = rd.ReadSize();
	nNonCons += nFloat;
#endif

#if UCFG_LISP_FFI
	size_t nForeignPointer = rd.ReadSize();
	nNonCons += nForeignPointer;
#endif

	size_t i;
	for (i=0; i<m_arValueMan.size(); ++i)
		m_arValueMan[i]->PrepareType(rd.m_arCount[i]);
	for (i=0; i<RESERVED_CONS; ++i) {
		m_consMan.get_Base()[i].m_car = 0;
		m_consMan.get_Base()[i].m_cdr = 0;
	}
	size_t count = rd.ReadSize();
	CConsValue *pCons = m_consMan.Base+RESERVED_CONS;
	for (i=0; i<count; ++i) {
		size_t len = rd.ReadSize(),
			n = rd.ReadSize();
		for (size_t j=0; j<n; j++)
			for (size_t k=0; k<len; ++k) {
				rd >> pCons->m_car;
				if (k == len-1)
					pCons->m_cdr = 0;
				else
					pCons->m_cdr = FromSValue(pCons+1);
				pCons++;
			}
	}

	for (CConsValue *pEnd=m_consMan.Base+rd.m_arCount[0]-nNonCons; pCons != pEnd; ++pCons)
		pCons->Read(rd);

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	for (size_t i=0; i<nFloat; ++i, ++pCons)
		((CFloat*)pCons)->Read(rd);
#endif

#if UCFG_LISP_FFI
	LoadFFI(rd);
	for (size_t i=0; i<nForeignPointer; ++i, ++pCons)
		((CForeignPointer*)pCons)->Read(rd);
#endif

	for (i=1; i<m_arValueMan.size(); i++)
		m_arValueMan[i]->ReadType(rd);
}

bool CLispEng::IsProperList(CP p) {
	if (!p)
		return false;
	for (CP q=p; Type(p)==TS_CONS; Inc(q)) {
		CP a = Cdr(p);
		if (Type(a) != TS_CONS)
			return false;
		if (!a)
			return true;
		if (a == q)
			return false;
		p = Cdr(a);
	}
	return false;
}

void CLispEng::SaveMem(Stream& bstm) {
	m_cVal = 0;

	m_pBitmapCons = new BASEWORD[m_consMan.m_size/BASEWORD_BITS];
	memset(m_pBitmapCons, 0, m_consMan.m_size/8);
//!!!R	m_mfnCollectDeferred = &CLispEng::CollectDeferredAndMapConsImp;
	m_mfnApplyCheckEx = &CLispGC::ApplyCheckExAndMapConsImp;
	Collect();	
//!!!R	m_mfnCollectDeferred = &CLispEng::CollectDeferredImp;
	m_mfnApplyCheckEx = &CLispGC::ApplyCheckExImp;	

	CBlsHeader header(true);
	BlsWriter wr(bstm);
	wr.WriteStruct(header);
	(BinaryWriter&)wr << vector<String>(); //!!!for secret m_arModule;

	size_t i;
	for (i=0; i<m_arValueMan.size(); ++i)
		wr.m_arCount[i] = m_arValueMan[i]->ScanType(wr.m_obMap);

	//!!!Verify(wr.m_obMap);//!!!

	//!!!        nArray  = m_alloc.ScanType(m);
	wr.WriteLens();

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	uint32_t nFloat = 0;
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j)
		if (BitOps::BitTest(m_consMan.m_pBitmap, j) && !BitOps::BitTest(m_pBitmapCons, j))
			++nFloat;
	wr.WriteSize(nFloat);
#endif


#if UCFG_LISP_FFI
	uint32_t nForeignPointer = 0;
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j) {
		if (BitOps::BitTest(m_pBitmapCons, j) && ((CSValueEx*)(m_consMan.Base+j))->m_type==TS_EX_FF_PTR)
			++nForeignPointer;
	}
	wr.WriteSize(nForeignPointer);
#endif

	//!!!m_consMan     .WriteType(wr);

	BASEWORD *bm = new BASEWORD[m_consMan.m_size/BASEWORD_BITS];
	memset(bm, 0, m_consMan.m_size/8);
	
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j) {
		CConsValue *cons = m_consMan.Base+j;
		if (!BitOps::BitTest(bm, j) && BitOps::BitTest(m_pBitmapCons, j)
#if UCFG_LISP_FFI
			&& ((CSValueEx*)(m_consMan.Base+j))->m_type!=TS_EX_FF_PTR
#endif
			) {
			CP p = FromSValue(cons);
			if (IsProperList(p)) {
				while (Inc(p), p)
					BitOps::BitTestAndSet(bm, AsIndex(p));
				continue;
			}				
		}
		BitOps::BitTestAndSet(bm, j);
	}

	vector<CListElement> arLE;
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j) {
		if (!BitOps::BitTest(bm, j) && BitOps::BitTest(m_pBitmapCons, j)
#if UCFG_LISP_FFI
			&& ((CSValueEx*)(m_consMan.Base+j))->m_type!=TS_EX_FF_PTR
#endif
			)
			arLE.push_back(CListElement(FromSValue(m_consMan.Base+j)));
	}


/*!!!	vector<CListElement*> arP;
	for (i=0; i<arLE.size(); i++)
		arP.push_back(&arLE[i]); */

	qsort(&arLE[0], arLE.size(), sizeof(CListElement), CompareListElement);
	DWORD n = RESERVED_CONS;
	CObMap::ContType& deq = wr.m_obMap.m_arP[0];
	deq.resize(m_consMan.m_size);
	fill(deq.begin(), deq.end(), size_t(-1));
	for (i=0; i<RESERVED_CONS; i++)
		deq[i] = i;
	deq[0] = 0;					//!!!?
	vector<CLenLists> ar;
	for (i=0; i<arLE.size(); ++i) {
		CListElement& le = arLE[i];
		CP p = le.m_p;
		for (CP q=p; q; Inc(q))
			if (deq[AsIndex(q)] != -1)
				goto LAB_OUT;
		if (ar.empty() || ar.back().m_len != le.m_len) {
			CLenLists ll;
			ll.m_len = le.m_len;
			ar.push_back(ll);
		}
		ar.back().m_ar.push_back(p);
		for (; p; Inc(p))
			deq[AsIndex(p)] = n++;			
LAB_OUT:
		;
	}
	DWORD nMaxList = n;
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j) {
		if (BitOps::BitTest(m_pBitmapCons, j) && BitOps::BitTest(m_pBitmapCons, j) 
#if UCFG_LISP_FFI
			&& ((CSValueEx*)(m_consMan.Base+j))->m_type!=TS_EX_FF_PTR
#endif
			&& deq[j] == -1) //!!!Dublicate
			deq[j] = n++; 
	}

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j)
		if (BitOps::BitTest(m_consMan.m_pBitmap, j) && !BitOps::BitTest(m_pBitmapCons, j))
			deq[j] = n++; 
#endif

#if UCFG_LISP_FFI
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j)
		if (BitOps::BitTest(m_pBitmapCons, j) &&  ((CSValueEx*)(m_consMan.Base+j))->m_type==TS_EX_FF_PTR)
			deq[j] = n++; 
#endif

	wr.WriteSize(ar.size());

	//!!!  (BinaryWriter&)wr << nMaxList;
	for (i=0; i<ar.size(); ++i) {
		CLenLists& ll = ar[i];
		wr.WriteSize(ll.m_len);
		wr.WriteSize(ll.m_ar.size());
		for (size_t j=0; j<ll.m_ar.size(); ++j) {
#ifdef _X_DEBUG //!!!D
			for (CP p=ll.m_ar[j], car; SplitPair(p, car);)
				if (!IntegerP(car))
					goto LAB_OUT_INT;
			Print(ll.m_ar[j]);
LAB_OUT_INT:
#endif
			for (CP p=ll.m_ar[j], car; SplitPair(p, car);)
				wr.WriteCP(car);				//!!! should be <<
		}
	}

	for (size_t j=1; j<m_consMan.m_size; ++j) {		//!!! may be from reserved
		CConsValue *p = m_consMan.Base+j;
		if (BitOps::BitTest(m_pBitmapCons, j) && BitOps::BitTest(m_pBitmapCons, j) && deq[j] >= nMaxList
#if UCFG_LISP_FFI
			&& ((CSValueEx*)(m_consMan.Base+j))->m_type!=TS_EX_FF_PTR			
#endif		
			)
			p->Write(wr);
	}

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j)
		if (BitOps::BitTest(m_consMan.m_pBitmap, j) && !BitOps::BitTest(m_pBitmapCons, j))
			((CFloat*)m_consMan.get_Base()+j)->Write(wr);
#endif

#if UCFG_LISP_FFI
	SaveFFI(wr);
	for (size_t j=RESERVED_CONS; j<m_consMan.m_size; ++j)
		if (BitOps::BitTest(m_pBitmapCons, j) &&  ((CSValueEx*)(m_consMan.Base+j))->m_type==TS_EX_FF_PTR)
			((CForeignPointer*)m_consMan.Base+j)->Write(wr);
#endif


	delete bm;
	delete[] (BASEWORD*)exchange(m_pBitmapCons, (void*)0);
	FreeReservedBuffers();

	for (i=1; i<m_arValueMan.size(); ++i)
		m_arValueMan[i]->WriteType(wr);

	//!!!  (BinaryWriter&)wr << nArray;
	//!!!  m_alloc.WriteType(wr, m);
}



} // Lisp::
