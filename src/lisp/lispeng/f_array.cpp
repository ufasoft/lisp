#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

static void GSetElement(uintptr_t *pData, byte elType, ssize_t i, CP v) {
	switch (elType) {
	case ELTYPE_T:
		pData[i] = v;
		break;
	case ELTYPE_BIT:
		(*((byte*)pData+i/8) &= ~(1 << (i % 8))) |= (AsBit(v) << (i % 8));
		break;
	case ELTYPE_CHARACTER:
		*((WORD*)pData+i) = AsChar(v);
		break;
	case ELTYPE_BASECHAR:
		*((byte*)pData+i) = (char)AsChar(v); //!!!
		break;
	case ELTYPE_BYTE:
		*((byte*)pData+i) = (byte)AsNumber(v);
		break;
	default:
		Lisp().E_SeriousCondition();
	}
}

CMultiIterator::CMultiIterator(uintptr_t *pData, byte elType, CP dims)
	:	m_pData(pData)
	,	m_elType(elType)
{
	CLispEng& lisp = Lisp();
	m_dims = CArrayValue::GetDims(dims);
	for (size_t i=0; i<m_dims.size(); ++i)
		m_idxs.push_back(0);
}

void CMultiIterator::Inc(ssize_t i) {
	if (m_bEnd = i<0)
		return;
	int& n = m_idxs[i];
	n++;
	if (n == m_dims[i]) {
		n = 0;
		Inc(i-1);
	}
}

void CMultiIterator::SetElement(CP v) {
	int r = 0;
	if (int dims = m_idxs.size()) {
		r = m_idxs[0];
		for (int i=1; i<dims; ++i)
			r = r*m_dims[i]+m_idxs[i];
	}
	GSetElement(m_pData, m_elType, r, v);
}

void CArrayValue::Dispose() {
#ifdef _X_DEBUG//!!!D
	if (m_pData == (void*)0x0160fd08)
		cerr << "deleted m_pData == 0x0160fd08" << endl;
#endif
	if (!(m_flags & FLAG_Displaced))
		free(m_pData);
}

CArrayValue& CArrayValue::operator=(CArrayValue& av) {
	memcpy(this, &av, sizeof(CArrayValue));
	av.m_pData = 0;
	return _self;
}

size_t CArrayValue::GetRank() {
	return Type(m_dims)==TS_FIXNUM ? 1 : Lisp().Length(m_dims);
}

size_t CArrayValue::TotalSize(CP p) {
	if (Type(p) == TS_FIXNUM)
		return AsNumber(p);
	CLispEng& lisp = Lisp();
	size_t size = 1;
	for (CP car; lisp.SplitPair(p, car);)
		size *= AsNumber(car);
	return size;
}

size_t CArrayValue::ElementBitSize(byte elType) {
	size_t r = sizeof(CP);
	switch (elType) {
	case ELTYPE_T: r = sizeof(CP)*8; break;
	case ELTYPE_BIT: r = 1; break;
	case ELTYPE_CHARACTER: r = sizeof(String::value_type)*8; break;
	case ELTYPE_BYTE:
	case ELTYPE_BASECHAR: r = 8;
		break;
	default:
		Lisp().E_SeriousCondition();
	}
	return r;
}

vector<int> CArrayValue::GetDims(CP dims) {
	CLispEng& lisp = Lisp();
	vector<int> r;
	if (Type(dims) != TS_CONS)
		r.push_back(AsNumber(dims));
	else
		for (CP car; lisp.SplitPair(dims, car);)
			r.push_back(AsNumber(car));
	return r;
}

size_t CArrayValue::CalculateRowIndex(CP *pRest, size_t nArgs) {
	if (GetRank() != nArgs)
		Lisp().E_SeriousCondition();
	size_t r = 0;
	if (nArgs == 1) {
		r = AsPositive(pRest[-1]);
		if (r >= AsNumber(m_dims))
			Lisp().E_RangeErr(pRest[-1], m_dims);
	} else {
		CLispEng& lisp = Lisp();
		for (CP p=m_dims, car; lisp.SplitPair(p, car);) {
			size_t idx = AsPositive(*--pRest);
			size_t dim = AsNumber(car);
			if (idx >= dim)
				lisp.E_RangeErr(*pRest, car);
			r = r*dim+idx;
		}
	}
	return r;
}

uintptr_t *CArrayValue::Alloc(size_t size) {
	return (uintptr_t*)Malloc(size * sizeof(uintptr_t));
}

void CArrayValue::Fill(uintptr_t *pData, byte elType, size_t beg, size_t end, CP initEl) {
	byte by = 0;
	switch (elType) {
	case ELTYPE_T:
		std::fill(pData+beg, pData+end, initEl);
		break;
	case ELTYPE_BIT:
		{
			by = initEl && AsBit(initEl) ? 0xFF : 0;
			byte *p = (byte*)pData;
			byte mask = beg % 8 ? (byte(0xFF) >> (8-(beg % 8))) : 0;
			p[beg/8] = p[beg/8] & mask | by & ~mask;
			if (int rest = int((end+7)/8) - int(beg/8) -1)
				memset(p+beg/8+1, by, rest);
		}
		break;
	case ELTYPE_BYTE:
		by = initEl ? (byte)AsNumber(initEl) : 0;
		memset((byte*)pData+beg, by, end-beg);
		break;
	case ELTYPE_CHARACTER:
		std::fill((String::value_type*)pData+beg, (String::value_type*)pData+end, String::value_type(initEl ? AsChar(initEl) : 0));
		break;
	case ELTYPE_BASECHAR:
		by = initEl ? (byte)AsChar(initEl) : 0;
		memset((byte*)pData+beg, by, end-beg);
		break;
	default:
		Lisp().E_SeriousCondition();
	}
}

uintptr_t *CArrayValue::CreateData(byte elType, CP dims, CP initEl) {
	size_t totalSize = TotalSize(dims);
	uintptr_t *pData = Alloc((totalSize*CArrayValue::ElementBitSize(elType)+LISP_BITS_IN_CP-1)/LISP_BITS_IN_CP);
	if (totalSize)
		Fill(pData, elType, 0, totalSize, initEl);
	return pData;
}

CP CArrayValue::GetElement(size_t i) {
	CLispEng& lisp = Lisp();
	CArrayValue *av = this;
	while (av->m_flags & FLAG_Displaced) {
		i += av->m_nDisplaceIndex;
		av = lisp.ToArray(av->m_displace);
	}
	if (i >= av->TotalSize())
		Lisp().E_RangeErr(CreateFixnum((int)i), CreateFixnum((int)av->TotalSize()));
	switch (m_elType) {
	case ELTYPE_T:
		return av->m_pData[i];
	case ELTYPE_BIT:
		return CreateFixnum((*((byte*)av->m_pData+i/8)>>(i % 8)) & 1);
	case ELTYPE_CHARACTER:
		return CreateChar(*((WORD*)av->m_pData+i));
	case ELTYPE_BASECHAR:
		return CreateChar(*((byte*)av->m_pData+i));
	case ELTYPE_BYTE:
		return CreateFixnum(*((byte*)av->m_pData+i));
	default:
		Lisp().E_SeriousCondition();
	}
}

void CArrayValue::SetElement(size_t i, CP p) {
	CLispEng& lisp = Lisp();
	CArrayValue *av = this;
	while (av->m_flags & FLAG_Displaced) {
		i += av->m_nDisplaceIndex;
		av = lisp.ToArray(av->m_displace);
	}
	GSetElement(av->m_pData, m_elType, i, p);
}

CP CArrayValue::GetByIterator(const vector<int>& vdims, const CMultiIterator& mi) {
	CLispEng& lisp = Lisp();
	int r = 0;
	for (int i=0, dims=mi.m_idxs.size(); i<dims; ++i) {
		int idx = mi.m_idxs[i],
			dim = vdims[i];
		if (idx >= dim)
			return V_U;
		r = i ? r*dim+idx : idx;
	}
	return GetElement(r);
}

void CArrayValue::Write(BlsWriter& wr) {
	wr << m_fillPointer << m_displace << m_dims;
	(BinaryWriter&)wr << m_flags;
	if (m_flags & FLAG_Displaced)
		(BinaryWriter&)wr << m_nDisplaceIndex;
	else {
		(BinaryWriter&)wr << m_elType;
		size_t totalSize = TotalSize();
		switch (m_elType) {
		case ELTYPE_T:
			{
				for (size_t i=0; i<totalSize; ++i)
					wr << *(CSPtr*)(m_pData+i);
			}
			break;
		case ELTYPE_BIT:
			wr.Write(m_pData, (totalSize+7)/8);
			break;
		case ELTYPE_CHARACTER:
			wr << String((const String::value_type*)m_pData, totalSize);
			break;
		case ELTYPE_BYTE:
		case ELTYPE_BASECHAR:
			((BinaryWriter&)wr).Write(m_pData, totalSize);
			break;
		}
	}
}

void CArrayValue::Read(const BlsReader& rd) {
	rd >> m_fillPointer >> m_displace >> m_dims;
	(BinaryReader&)rd >> m_flags;
	if (m_flags & FLAG_Displaced)
		(BinaryReader&)rd >> m_nDisplaceIndex;
	else {
		(BinaryReader&)rd >> m_elType;
		m_pData = CreateData(m_elType, m_dims, 0);
		size_t totalSize = TotalSize();
		switch (m_elType) {
		case ELTYPE_T:
			{
				for (size_t i=0; i<totalSize; ++i)
					rd >> *(CSPtr*)(m_pData+i);
			}
			break;
		case ELTYPE_BIT:
			rd.Read(m_pData, (totalSize+7)/8);
			break;
		case ELTYPE_CHARACTER:
			{
				String s = rd.ReadString();
				if (s.length() != totalSize)
					Lisp().E_SeriousCondition();
				memcpy(m_pData, (const String::value_type*)s, s.length()*sizeof(String::value_type));   		//!!! ASSERT(sizeof(wchat_t)==2)
			}
			break;
		case ELTYPE_BYTE:
		case ELTYPE_BASECHAR:
			rd.Read(m_pData, totalSize);
			break;
		}
	}
}

byte CLispEng::EltypeCode(CP p) {
	switch (p) {
	case V_T:				return ELTYPE_T;
	case S(L_BIT):			return ELTYPE_BIT;
	case S(L_CHARACTER):	return ELTYPE_CHARACTER;
	case 0: Throw(E_FAIL); //!!!
	}
	Push(p);
	Call(S(L_SUBTYPEP), p, 0);
	if (m_r) {
		SkipStack(1);
		Throw(E_FAIL); //!!!
	}
	Call(S(L_SUBTYPE_INTEGER), p);
	SkipStack(1);
	if (m_cVal>1 && IntegerP(m_r) && AsNumber(m_r)>=0 && IntegerP(m_arVal[1])) {	//!!! may be BIGNUM 
		Push(m_arVal[1]);
		F_IntegerLength();
		int len = AsNumber(m_r);
		if (len <= 1)
			return ELTYPE_BIT;
		else if (len <= 8)
			return ELTYPE_BYTE;
	}
	Call(S(L_SUBTYPEP), p, S(L_CHARACTER));
	return m_r ? ELTYPE_CHARACTER : ELTYPE_T;
}

void CLispEng::F_ArrayP() {
	m_r = FromBool(Type(Pop()) == TS_ARRAY);
}

void CLispEng::F_VectorP() {
	m_r = FromBool(VectorP(Pop()));
}

void CLispEng::F_StringP() {
	m_r = FromBool(StringP(Pop()));
}

void CLispEng::TestIndex(CArrayValue*& av, size_t& idx) {
	idx = AsPositive(SV);
	av = ToArray(SV1);
	size_t total = av->TotalSize();
	if (idx >= total)
		E_TypeErr(SV, List(S(L_INTEGER), V_0, CreateFixnum((int)total)));
	SkipStack(2);
}

void CLispEng::F_SimpleVectorP() {
	CP p = Pop();
	m_r = FromBool(VectorP(p) && AsArray(p)->SimpleP());
}

void CLispEng::F_SimpleArrayP() {
	CP p = Pop();
	m_r = FromBool(Type(p)==TS_ARRAY && AsArray(p)->SimpleP());
}

void CLispEng::F_SVRef() {
#ifdef C_LISP_QUICK_MACRO
	if (Type(SV1) == TS_FRAME_PTR) {
		m_r = ToFrame(SV1)[AsPositive(SV)]; //!!!
		SkipStack(2);
		return;
	}
#endif
	CArrayValue *av = ToVector(SV1);
	if (!av->SimpleP()) // simple-vector-p ?
		E_TypeErr(SV1, S(L_SIMPLE_VECTOR));
	m_r = av->GetElement(AsPositive(SV));
	SkipStack(2);
}

void CLispEng::F_ArrayRowMajorIndex(size_t nArgs) {
	m_r = CreateFixnum(ToArray(m_pStack[nArgs])->CalculateRowIndex(m_pStack+nArgs, nArgs));
	SkipStack(nArgs+1);
}

void CLispEng::F_RowMajorAref() {
	CArrayValue *av;
	size_t idx;
	TestIndex(av, idx);
	m_r = av->GetElement(idx);
}

void CLispEng::F_SetfRowMajorAref() {
	m_r = Pop();
	CArrayValue *av;
	size_t idx;
	TestIndex(av, idx);
	av->SetElement(idx, m_r);
}

void CLispEng::F_Aref(size_t nArgs) {
	CP a = m_pStack[nArgs];		 
	F_ArrayRowMajorIndex(nArgs);
	Push(a, m_r);
	F_RowMajorAref();
}

void CLispEng::F_Store(size_t nArgs) {
	CP v = Pop(), // GC cannot be because index mustbe FIXNUM
		a = m_pStack[nArgs];		 
	F_ArrayRowMajorIndex(nArgs);
	Push(a, m_r, v);
	F_SetfRowMajorAref();
}

byte CArrayValue::GetElementType() {
	CLispEng& lisp = Lisp();
	CArrayValue *av = this;
	while (av->m_flags & FLAG_Displaced)
		av = lisp.ToArray(av->m_displace);
	return av->m_elType;
}

void CLispEng::F_ArrayElementType() {
	switch (ToArray(Pop())->GetElementType()) {
	case ELTYPE_T:			m_r = V_T; break;
	case ELTYPE_BIT:		m_r = S(L_BIT); break;
	case ELTYPE_CHARACTER:	m_r = S(L_CHARACTER); break;
	case ELTYPE_BASECHAR:	m_r = S(L_BASE_CHAR); break;
	case ELTYPE_BYTE:		m_r = List(S(L_UNSIGNED_BYTE), V_8); break;
	default:
		Throw(E_FAIL); //!!!NOTREACH
	}
}

void CLispEng::F_ArrayRank() {
	m_r = CreateFixnum(ToArray(Pop())->GetRank());
}

void CLispEng::F_ArrayDimensions() {
	CP dims = ToArray(Pop())->m_dims;
	m_r = Type(dims)==TS_FIXNUM ? List(dims) : dims;
}

void CLispEng::F_ArrayDisplacement() {
	CArrayValue *av = ToArray(Pop());
	if (av->m_flags & FLAG_Displaced) {
		m_r = av->m_displace;
		m_arVal[1] = CreateInteger(av->m_nDisplaceIndex);
	}
	else
		m_arVal[1] = V_0;
	m_cVal = 2;
}

void CLispEng::F_ArrayHasFillPointerP() {
	m_r = ToArray(Pop())->m_fillPointer;
}

void CLispEng::F_FillPointer() {
	CP p = Pop();
	CArrayValue *av = ToVector(p);
	if (!(m_r=av->m_fillPointer))
		Error(E_LISP_NoFillPointer, p);
}

void CLispEng::F_SetFillPointer() {
	AsPositive(m_r = Pop());
	CP p = Pop();
	CArrayValue *av = ToVector(p);
	if (!av->m_fillPointer)
		Error(E_LISP_NoFillPointer, p);
	av->m_fillPointer = m_r;
}

bool CLispEng::DimsEqual(CP p, CP q) {
	for (; p!=q; Inc(p), Inc(q))
		if (!Eql(Car(p), Car(q)))
			return false;
	return true;
}

void CLispEng::BitUp(CBitOp bitOp) {
	CArrayValue *av1 = ToArray(SV2),
		*av2 = ToArray(SV1);

	if (av1->GetElementType() != ELTYPE_BIT)
		E_TypeErr(SV2, S(L_BIT_VECTOR));
	if (av2->GetElementType() != ELTYPE_BIT)
		E_TypeErr(SV1, S(L_BIT_VECTOR));
	if (!DimsEqual(av1->m_dims, av2->m_dims))
		E_TypeErr(SV2, S(L_BIT_VECTOR));
	CP arg = ToOptionalNIL(SV);
	CArrayValue *avr;
	size_t totalSize = av1->TotalSize();
	if (!arg) {
		avr = CreateArray();
		avr->m_elType = ELTYPE_BIT;
		avr->m_dims = av1->m_dims;
		avr->m_pData = CArrayValue::Alloc((totalSize+LISP_BITS_IN_CP-1)/LISP_BITS_IN_CP);
		av1 = AsArray(SV2);
		av2 = ToArray(SV1);
	} else if (arg == V_T)
		avr = av1;
	else {
		avr = ToArray(arg);
		if (avr->GetElementType() != ELTYPE_BIT || !DimsEqual(avr->m_dims, av2->m_dims))
			E_TypeErr(arg, S(L_BIT_VECTOR));
	} for (size_t i=0; i<totalSize; ++i) {
		uintptr_t n1 = AsNumber(av1->GetElement(i)),
			n2 = AsNumber(av2->GetElement(i)),
			n;
		switch (bitOp) {
		case BITOP_AND:
			n = n1 & n2;
			break;
		case BITOP_IOR:
			n = n1 | n2;
			break;
		case BITOP_XOR:
			n = n1 ^ n2;
			break;
		case BITOP_EQV:
			n = ~(n1 ^ n2);
			break;
		case BITOP_NAND:
			n = ~(n1 & n2);
			break;
		case BITOP_NOR:
			n = ~(n1 | n2);
			break;
		case BITOP_ANDC1:
			n = ~n1 & n2;
			break;
		case BITOP_ANDC2:
			n = n1 & ~n2;
			break;
		case BITOP_ORC1:
			n = ~n1 | n2;
			break;
		case BITOP_ORC2:
			n = n1 | ~n2;
			break;
		}
		n &= 1;
		avr->SetElement(i, CreateInteger(n));
	}
	m_r = FromSValue(avr);
	SkipStack(3);
}

void CLispEng::F_BitAND() {
	BitUp(BITOP_AND);
}

void CLispEng::F_BitIOR() {
	BitUp(BITOP_IOR);
}

void CLispEng::F_BitXOR() {
	BitUp(BITOP_XOR);
}

void CLispEng::F_BitEQV() {
	BitUp(BITOP_EQV);
}

void CLispEng::F_BitNAND() {
	BitUp(BITOP_NAND);
}

void CLispEng::F_BitNOR() {
	BitUp(BITOP_NOR);
}

void CLispEng::F_BitANDC1() {
	BitUp(BITOP_ANDC1);
}

void CLispEng::F_BitANDC2() {
	BitUp(BITOP_ANDC2);
}

void CLispEng::F_BitORC1() {
	BitUp(BITOP_ORC1);
}

void CLispEng::F_BitORC2() {
	BitUp(BITOP_ORC2);
}

void CLispEng::F_BitNOT() {
	CArrayValue *av = ToArray(SV1);
	if (av->GetElementType() != ELTYPE_BIT)
		E_TypeErr(SV1, List(S(L_ARRAY), S(L_BIT)));
	CP arg = SV;
	CArrayValue *avr;
	size_t totalSize = av->TotalSize();
	if (!arg || arg==V_U) {
		avr = CreateArray();
		avr->m_elType = ELTYPE_BIT;
		avr->m_dims = av->m_dims;
		avr->m_pData = CArrayValue::Alloc((totalSize+LISP_BITS_IN_CP-1)/LISP_BITS_IN_CP);
		av = AsArray(SV1);
	} else if (arg == V_T)
		avr = av;
	else {
		avr = ToArray(arg);
		if (avr->GetElementType() != ELTYPE_BIT || !DimsEqual(avr->m_dims, av->m_dims))
			E_TypeErr(arg, S(L_BIT_VECTOR));
	}
	for (size_t i=0; i<totalSize; ++i)
		avr->SetElement(i, !AsNumber(av->GetElement(i)));
	m_r = FromSValue(avr);
	SkipStack(2);
}

byte CLispEng::ToElType(CP p) {
	if (p == S(L_BIT))
		return ELTYPE_BIT;
	else if (p == S(L_CHARACTER))
		return ELTYPE_CHARACTER;
	else if (p == S(L_BASE_CHAR)) //!!!
		return ELTYPE_BASECHAR;
	else
		return ELTYPE_T;
}

size_t CLispEng::CheckDims(CP& dims) {
	if (Type(dims) == TS_FIXNUM) {
		if (AsNumber(dims) >= 0)
			return 1;
	} else {
		size_t rank = 0;
		uint64_t total = 1;
		for (CP q=dims, p; SplitPair(q, p); rank++) {
			intptr_t n;
			switch (Type(p)) {
			case TS_FIXNUM:
				if ((n=AsNumber(p)) >= 0)
					break;
			default:
				goto LAB_ERROR;
			}
			if ((total *= n) > FIXNUM_LIMIT)
				goto LAB_ERROR;
		}
		if (rank == 1)
			dims = Car(dims);
		return rank;
	}
LAB_ERROR:
	E_TypeErr(dims, List(S(L_INTEGER), V_0, CreateFixnum(FIXNUM_LIMIT-1)));
}

void CLispEng::F_MakeArray() {
	CP dims = GetStack(7);
	CP displ = ToOptionalNIL(SV1),
		fillPointer = ToOptionalNIL(SV2);

	CP &elementType = *(m_pStack+6),
		&initEl = *(m_pStack+5),
		&displIndex = *m_pStack;
	CP initContent = GetStack(4);

	initEl = ToOptionalNIL(initEl);

	elementType = ToOptional(elementType, V_T);
	if (AsSymbol(S(L_UPGRADED_ARRAY_ELEMENT_TYPE))->GetFun()) {		// not defined during boot
		Call(S(L_UPGRADED_ARRAY_ELEMENT_TYPE), elementType);
		elementType = m_r;
	}
	byte elType = ToElType(elementType);

	displIndex = ToOptional(displIndex, V_0);

	if (displ) {
#ifdef _DEBUG //!!!D
		CArrayValue *av = ToArray(displ);
#endif
		if (ToArray(displ)->m_elType != elType)
			E_TypeErr(displ, S(L_ARRAY));
	}
	CheckDims(dims);
	CArrayValue *av = CreateArray();
	av->m_dims = dims;
	av->m_elType = elType;
	SV = FromSValue(av);
	if (av->m_fillPointer = fillPointer) {
		ToVector(SV);
		if (av->m_fillPointer == V_T)
			av->m_fillPointer = dims;
	} if (av->m_displace = displ) {
		av->m_flags |= FLAG_Displaced;
		av->m_nDisplaceIndex = AsNumber(displIndex);
	} else
		av->m_pData = CArrayValue::CreateData(av->m_elType, av->m_dims, initEl);
	if (initContent != V_U)
		Call(S(L_FILL_ARRAY_CONTENTS), SV, initContent);
	m_r = SV;
	SkipStack(8);
}

void CLispEng::CopyArray(CArrayValue *avSrc, CArrayValue *avDst, CP subs, int nArgs) {
	CP car;
	if (SplitPair(subs, car)) {
		nArgs++;
		DWORD n = AsNumber(car);
		for (int i=0; i<n; i++) {
			Push(CreateInteger(i));
			CopyArray(avSrc, avDst, subs, nArgs);
		}
	} else {
		avDst->SetElement(avDst->CalculateRowIndex(m_pStack+nArgs, nArgs), avSrc->GetElement(avSrc->CalculateRowIndex(m_pStack+nArgs, nArgs)));
	}
}

/*!!!R
void CLispEng::F_CopyArray() {
	CP subs = Pop();
	CArrayValue *avDst = ToArray(Pop()),
		*avSrc = ToArray(Pop());              
	CopyArray(avSrc, avDst, subs);
}
*/

void CLispEng::F_AdjustArray() {
	CP &elementType = *(m_pStack+5);
	if (elementType == V_U) {
		Push(GetStack(7));
		F_ArrayElementType();
		elementType = m_r;
	} else  if (AsSymbol(S(L_UPGRADED_ARRAY_ELEMENT_TYPE))->GetFun()) {		// not defined during boot
		Call(S(L_UPGRADED_ARRAY_ELEMENT_TYPE), elementType);
		elementType = m_r;
	}
	byte elType = ToElType(elementType);

	CArrayValue *av = ToArray(m_r = GetStack(7));

	CP initCont = *(m_pStack+3);

	CP &dims = *(m_pStack+6),
		&initEl = *(m_pStack+4),
		fillPointer = ToOptionalNIL(SV2),
		&displ = *(m_pStack+1),
		&displIndex = *m_pStack,
		v;
	initEl = ToOptionalNIL(initEl);
	displ = ToOptionalNIL(displ);
	displIndex = ToOptional(displIndex, V_0);
	size_t rank = av->GetRank();
	if (rank != CheckDims(dims))
		E_TypeErr(FromSValue(av), S(L_ARRAY));
	uintptr_t *pData = 0;
	if (!displ) {
		if (1==rank && elType==av->m_elType && !(av->m_flags & FLAG_Displaced)) {
			size_t prevDim = AsNumber(Type(av->m_dims)==TS_CONS ? Car(av->m_dims) : CP(av->m_dims));
			size_t newDim = AsNumber(Type(dims)==TS_CONS ? Car(dims) : dims);
			pData = (uintptr_t*)Realloc(av->m_pData, (newDim*CArrayValue::ElementBitSize(elType)+LISP_BITS_IN_CP-1) / LISP_BITS_IN_CP * sizeof(uintptr_t));
			if (newDim > prevDim)
				CArrayValue::Fill(pData, elType, prevDim, newDim, initEl);
			goto LAB_END;
		} else {
			vector<int> vdims = CArrayValue::GetDims(av->m_dims);
			for (CMultiIterator mi(pData=CArrayValue::CreateData(elType, dims, initEl), elType, dims); mi; mi.Inc())
				if ((v=av->GetByIterator(vdims, mi)) != V_U)
					mi.SetElement(v);
		}
	}
	av->Dispose();
LAB_END:
	av->m_dims = dims;
	if (av->m_displace = displ) {
		av->m_flags |= FLAG_Displaced;
		av->m_nDisplaceIndex = AsNumber(displIndex);
	} else
		av->m_pData = pData;
	if (fillPointer)
		av->m_fillPointer = fillPointer==V_T ? av->TotalSize() : fillPointer;
	if (initCont != V_U) {
		Call(S(L_FILL_ARRAY_CONTENTS), m_r, initCont);		//!!!O
		m_r = GetStack(7);
	}
	SkipStack(8);
}

void CLispEng::F_VectorPushExtend() {
	CP pext = Pop();
	CArrayValue *av = ToVector(SV);
	size_t ext;
	if (pext == V_U)
		ext = 1024 / CArrayValue::ElementBitSize(av->GetElementType());
	else if (!(ext = AsPositive(pext)))
		E_TypeErr(pext, List(S(L_INTEGER), V_0, S(L_ASTERISK)));

	CP pfp = av->m_fillPointer;
	if (!pfp)
		Error(E_LISP_NoFillPointer, SV);
	size_t fp = AsPositive(pfp);
	if (fp >= av->DataLength) {
		Push(SV);
		F_ArrayElementType();		
		Push(SV, CreateFixnum(fp+ext), m_r, 0);
		Push(V_U, 0, 0, 0);
		F_AdjustArray();
		av = AsArray(m_r);
	}
	av->m_fillPointer = CreateFixnum(fp+1);
	av->SetElement(fp, SV1);
	m_r = pfp;
	SkipStack(2);
}

void CLispEng::F_AdjustableArrayP() {
	ToArray(Pop());
	m_r = V_T;
}



} // Lisp::


