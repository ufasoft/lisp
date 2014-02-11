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

void CLispEng::F_NReverse() {
	m_r = Pop();
	switch (Type(m_r))
	{
	case TS_CONS:
		if (m_r)
			m_r = NReverse(m_r);
		break;
	case TS_ARRAY:
		{
			CArrayValue *av = ToVector(m_r);
			for (ssize_t i=0, j=av->GetVectorLength(); i<--j; i++) {
				CP tmp = av->GetElement(i);
				av->SetElement(i, av->GetElement(j));
				av->SetElement(j, tmp);
			}
		}
		break;
	default:
		E_TypeErr(m_r, S(L_SEQUENCE));
	}
}

void CLispEng::F_Vector(size_t nArgs) {
	CArrayValue *vec = CreateVector(nArgs);
	while (nArgs--)
		vec->m_pData[nArgs] = Pop();
	m_r = FromSValue(vec);
}

void CLispEng::F_Elt() {
	switch (Type(SV1))
	{
	case TS_CONS:
		swap(SV, SV1);
		F_NthCdr();
		m_r = Car(m_r);
		break;
	case TS_ARRAY:
		m_r = ToVector(SV1)->GetElement(AsPositive(SV));
		SkipStack(2);
		break;
	default:
		E_TypeErr(SV1, S(L_SEQUENCE));
	}
}

void CLispEng::F_SetfElt() {
	switch (Type(SV1))
	{
	case TS_CONS:
		swap(SV, SV1);
		F_NthCdr();
		m_r = ToCons(m_r)->m_car = Pop();
		break;
	case TS_ARRAY:
		ToVector(SV1)->SetElement(AsPositive(SV), m_r=SV2);
		SkipStack(3);
		break;
	default:
		E_TypeErr(SV1, S(L_SEQUENCE));
	}
}

void CLispEng::F_FindSeqType() {
	for (CP n=Pop(), p = Spec(L_S_SEQ_TYPES); SplitPair(p, m_r);)
		if (n == SeqType(m_r))
			return;
	m_r = 0;
}

void CLispEng::F_GetSeqType() {
	CP p = Pop();
	switch (Type(p))
	{
	case TS_CONS:
		Push(S(L_LIST));
		break;
	case TS_ARRAY:
		if (StringP(p))
			Push(S(L_STRING));
		else if (VectorP(p))
			Push(S(L_VECTOR));
		else
			return;
		break;
	default:
		return;
	}
	F_FindSeqType();
}

void CLispEng::F_GetValidSeqType() {
	Push(SV);
	F_GetSeqType();
	if (!m_r) {
		E_TypeErr(SV, S(L_SEQUENCE));
	}
	SkipStack(1);
}

void CLispEng::F_GetTdCheckIndex() {
	size_t i = AsPositive(Pop());
#ifdef _X_DEBUG //!!!D
	cerr << "\n------F_GetTdCheckIndex-----------\n";
	PrintForm(cerr, SV);
#endif
	if (VectorP(SV) && i>=AsArray(SV)->GetVectorLength())
		E_TypeErr(SV, S(L_VECTOR));
	F_GetValidSeqType();
}

size_t CLispEng::SeqLength(CP seq) {
	switch (Type(seq))
	{
	case TS_ARRAY: return ToVector(seq)->GetVectorLength();
	case TS_CONS: return Length(seq);
	default:
		E_TypeErr(seq, S(L_SEQUENCE));
	}
}

pair<size_t, size_t> CLispEng::PopSeqBoundingIndex(CP seq, size_t& len) {
	CP end = ToOptionalNIL(Pop());
	size_t nStart = AsPositive(ToOptional(Pop(), V_0));
	len = SeqLength(seq);
	size_t nEnd = end ? AsPositive(end) : len;
	if (nStart>nEnd || nEnd>len)
		E_Error(S(L_ERROR));
	return make_pair(nStart, nEnd);
}

void CLispEng::F_Subseq() {
	size_t len;
	pair<size_t, size_t> pp = PopSeqBoundingIndex(SV2, len);
	len = pp.second-pp.first;
	switch (Type(SV))
	{
	case TS_ARRAY:
		{
			Push(SV);
			F_GetValidSeqType();
			Call(SeqMake(m_r), CreateFixnum((int)len));
			CArrayValue *sar = AsArray(SV),
				*dar = AsArray(m_r);
			for (int i=0; i<len; i++)
				dar->SetElement(i, sar->GetElement(pp.first+i));
		}
		break;
	case TS_CONS:
		CP seq = SV;
		while (pp.first--)
			seq = Cdr(seq);
		CListConstructor lc;
		for (CP car; len--;) {
			SplitPair(seq, car);
			lc.Add(car);
		}
		m_r = lc;
	}
	SkipStack(1);
}

void CLispEng::SetSeqElt(CP rseq, CP& p, CP el) {
	switch (Type(p))
	{
	case TS_FIXNUM:
		{
			size_t idx = AsIndex(p);
			AsArray(rseq)->SetElement(idx, el);
			p = CreateFixnum(idx+1);
		}
		break;
	case TS_CONS:
		if (!p)
			E_Error();
		CConsValue *cons = AsCons(p);
		cons->m_car = el;
		p = cons->m_cdr;
	}
}

void CLispEng::F_ExpandDeftype() {
	bool bOnce = ToOptionalNIL(Pop());
	m_r = SV;
	do {
		CP sym = m_r;
		switch (Type(m_r))
		{
		case TS_SYMBOL:
			break;
		case TS_CONS:
			if (Type(sym=Car(m_r)) == TS_SYMBOL)
				break;
		default:
			goto LAB_RET;
		}
		if (CP e = GetSymProp(sym, S(L_DEFTYPE_EXPANDER)))
			Call(e, Type(m_r)==TS_SYMBOL ? Cons(m_r, 0) : m_r);
		else
			break;
	} while (!bOnce);
LAB_RET:
	m_arVal[1] = FromBool(Pop() != m_r);
	m_cVal = 2;
}

void CLispEng::F_ValidType1() {
	Push(SV, 0);
	F_ExpandDeftype();
	CP name = m_r;
	CP c = 0;
	switch (Type(name))
	{
	case TS_SYMBOL:
		switch (name)
		{
		case S(L_NULL):
			c = V_0;
			name = S(L_LIST);
			break;
		case S(L_CONS):
			c = CreateFixnum(-1);
			name = S(L_LIST);
			break;
		case S(L_SIMPLE_VECTOR):
		case S(L_ARRAY):
		case S(L_SIMPLE_ARRAY):
			name = S(L_VECTOR);
			break;
		case S(L_BIT_VECTOR):
		case S(L_SIMPLE_BIT_VECTOR):
			name = V_1;
			break;
		case S(L_SIMPLE_STRING):
		case S(L_BASE_STRING):
		case S(L_SIMPLE_BASE_STRING):
			name = S(L_STRING);
		}
		break;
	case TS_CONS:
		{
			CP name1 = Car(name);
			if (Type(name1) == TS_SYMBOL) {
				CP name2 = Cdr(name);
				if (!name2 || ConsP(name2) && !(Cdr(name2)))
				{
					switch (name1)
					{
					case S(L_SIMPLE_VECTOR):
						name = S(L_VECTOR);
						break;
					case S(L_STRING):
					case S(L_SIMPLE_STRING):
					case S(L_BASE_STRING):
					case S(L_SIMPLE_BASE_STRING):
						name = S(L_STRING);
					case S(L_BIT_VECTOR):
					case S(L_SIMPLE_BIT_VECTOR):
						name = V_1;
						break;
					default:
						goto LAB_OUT;
					}
					if (ConsP(name2) && IntegerP(Car(name2)))
						c = Car(name2);
					break;
				} else {
					CP name3 = 0;
					if (!name2)
						name2 = name3 = S(L_ASTERISK);
					else if (ConsP(name2)) {
						name3 = Cdr(name2);
						name2 = Car(name2);
						if (!name3)
							name3 = S(L_ASTERISK);
						else if (ConsP(name3) && !Cdr(name3))
							name3 = Car(name3);
						else
							goto LAB_OUT;
					}
					else
						goto LAB_OUT;
					if (name1==S(L_VECTOR) ||
						(name1==S(L_ARRAY) || name1==S(L_SIMPLE_ARRAY)) && 
						(name3==S(L_ASTERISK) || name3==V_1 || ConsP(name3)&&!Cdr(name3)))
					{
						if (name1 == S(L_VECTOR)) {
							if (ConsP(name3))
								name3 = Car(name3);
							if (IntegerP(name3))
								c = name3;
						}
						byte elType = name2==S(L_ASTERISK) ? ELTYPE_T : EltypeCode(name2);
						switch (elType)
						{
						case ELTYPE_T:					name = S(L_VECTOR); break;
						case ELTYPE_BIT:				name = V_1; break;
						case ELTYPE_CHARACTER:	name = S(L_STRING); break;
						}
						break;
					}
				}
			}
		}
		goto LAB_OUT;
	case TS_OBJECT:
		if (DefinedClassP(name)) {
			if (name == GetSymProp(S(L_LIST), S(L_CLOSCLASS)))
				name = S(L_LIST);
			else if (name == GetSymProp(S(L_NULL), S(L_CLOSCLASS))) {
				c = V_0;
				name = S(L_LIST);
			} else if (name == GetSymProp(S(L_CONS), S(L_CLOSCLASS))) {
				c = CreateFixnum(-1);
				name = S(L_LIST);
			}
			else if (name==GetSymProp(S(L_VECTOR), S(L_CLOSCLASS)) || name==GetSymProp(S(L_ARRAY), S(L_CLOSCLASS)))
				name = S(L_VECTOR);
			else if (name == GetSymProp(S(L_STRING), S(L_CLOSCLASS)))
				name = S(L_STRING);
			else if (name == GetSymProp(S(L_BIT_VECTOR), S(L_CLOSCLASS)))
				name = V_1;
			else
				name = TheClass(name).Classname;
			break;
		}
LAB_OUT:
		Push(c=name);
		Call(S(L_SUBTYPE_SEQUENCE), name);
		SkipStack(1);
		if (!m_r || m_r==S(L_SEQUENCE)) {
			m_r = 0;
			goto LAB_RET;
		}
		name = m_r;
	}
	Push(name);
	F_FindSeqType();
LAB_RET:
	m_cVal = 2;
	m_arVal[1] = c;
	SkipStack(1);
}

void CLispEng::F_ValidType() {
	Push(SV);
	F_ValidType1();
	if (!m_r)
		E_TypeErr(SV, 0);//!!!
	SkipStack(1);
}

void CLispEng::F_MakeSequence() {
	size_t size = AsPositive(SV1);
	if (SV2 == S(L_STRING)) {		// special case need during Boot
		m_r = CreateString(String(' ', size));
		if (SV != V_U) {
			CArrayValue *av = ToArray(m_r);
			for (int i=0; i<size; i++)
				av->SetElement(i, SV);
		}
		SkipStack(3);
	} else {
		Push(SV2);
		F_ValidType();
		CP td = SV2 = m_r;
		CP seqType = SeqType(td);
		if (seqType == S(L_LIST)) {
			F_MakeList();
			SkipStack(1);
		} else {
			Call(AsArray(td)->GetElement(11), SV1);
			if (SV != V_U) {
				CArrayValue *av = ToArray(m_r);
				for (int i=0; i<size; i++)
					av->SetElement(i, SV);
			}
			SkipStack(3);
		}
	}
}

void CLispEng::F_Concatenate(size_t nArgs) {
	size_t len = 0;
	for (int i=0; i<nArgs; ++i) {
		Push(m_pStack[i]);
		F_Length();
		len += AsNumber(m_r);
	}
	Push(m_pStack[nArgs], CreateFixnum(len), V_U);
	F_MakeSequence();

	CP p = (Type(m_r)==TS_ARRAY) ? V_0 : m_r;
	for (int i=nArgs; i--;) {
		CP seq = m_pStack[i];
		switch (Type(seq))
		{
		case TS_CONS:
			for (CP car; SplitPair(seq, car);)
				SetSeqElt(m_r, p, car);
			break;
		case TS_ARRAY:
			{
				CArrayValue *av = AsArray(seq);
				for (int i=0; i<av->GetVectorLength(); i++)
					SetSeqElt(m_r, p, av->GetElement(i));
			}	
			break;
		default:
			E_TypeErr(seq, S(L_SEQUENCE));
		}
	}
	SkipStack(1+nArgs);
}

void CLispEng::F_MapInto(size_t nArgs) {
	CP *pStack = m_pStack+nArgs;
	CP seq = pStack[1];
	pStack[0] = FromFunctionDesignator(pStack[0]);
	size_t len;
	switch (Type(seq))
	{
	case TS_ARRAY:
		len = ToVector(seq)->DataLength;
		break;
	case TS_CONS:
		len = Length(seq);
		break;
	default:
		E_TypeErr(seq, S(L_SEQUENCE));
	}
	for (int i=1; i<=nArgs; i++) {
		CP p = pStack[-i];
		len = min(len, SeqLength(p));
		Push(Type(p)==TS_ARRAY ? V_0 : p);
	}
	CP dst = Type(seq)==TS_ARRAY ? V_0 : seq;
	for (size_t j=0; j<len; j++) {
		for (int i=1; i<=nArgs; i++) {
			CP& src = pStack[-(int)nArgs-i];
			CP val;
			if (Type(src) == TS_FIXNUM) {
				size_t idx = AsIndex(src);
				val = AsArray(pStack[-i])->GetElement(idx++);
				src = CreateFixnum(idx);
			}
			else
				SplitPair(src, val);
			Push(val);
		}
		Apply(pStack[0], nArgs);
		if (Type(dst) == TS_FIXNUM) {
			size_t idx = AsIndex(dst);
			AsArray(seq)->SetElement(idx++, m_r);
			dst = CreateFixnum(idx);
		} else {
			CConsValue *cons = AsCons(dst);
			cons->m_car = m_r;
			dst = cons->m_cdr;
		}
	}
	if (Type(seq)==TS_ARRAY) {
		CArrayValue *av = AsArray(seq);
		if (av->m_fillPointer)
			av->m_fillPointer = CreateFixnum(len);
	}
	m_r = seq;
	m_pStack = pStack+2;
}

CP CLispEng::DoSeqAccess(CP seq, CP ptr) {
	switch (Type(seq))
	{
	case TS_CONS: return Car(ptr);
	case TS_ARRAY: return AsArray(seq)->GetElement(AsIndex(ptr));
	default:
		E_Error();
	}
}

void CLispEng::DoSeqSetAccess(CP seq, CP ptr, CP v) {
	switch (Type(seq))
	{
	case TS_CONS:
		if (!ptr)
			E_Error();
		ToCons(ptr)->m_car = v;
		break;		
	case TS_ARRAY:
		AsArray(seq)->SetElement(AsIndex(ptr), v);		
		break;
	default:
		E_Error();
	}
}

CP CLispEng::DoListUpdate(CP ptr) {
	return Cdr(ptr);
}

CP CLispEng::DoVectorUpdate(CP ptr) {
	return CreateFixnum(AsIndex(ptr)+1);
}

CP CLispEng::DoVectorFeUpdate(CP ptr) {
	return CreateFixnum(AsIndex(ptr)-1);
}

CLispEng::FPSeqUpdate CLispEng::GetSeqUpdate(CP seq) {
	switch (Type(seq))
	{
	case TS_CONS: return &CLispEng::DoListUpdate;
	case TS_ARRAY: return &CLispEng::DoVectorUpdate;
	default:
		E_Error();
	}
}

CLispEng::FPSeqUpdate CLispEng::GetSeqFeUpdate(CP seq) {
	switch (Type(seq))
	{
	case TS_CONS: return &CLispEng::DoListUpdate;
	case TS_ARRAY: return &CLispEng::DoVectorFeUpdate;
	default:
		E_Error();
	}
}

void CLispEng::ProcessSeq(CP seq, CP fromEnd, CP start, CP end, CP count, CP key, ISeqHandler& sh, bool bCreateBV) {
	Push(sh.m_seq = seq);
	F_GetValidSeqType();
	sh.m_td = m_r;
	Push(start, end);
	size_t len;
	pair<size_t, size_t> pp = PopSeqBoundingIndex(seq, len);
	CP bv = bCreateBV ? FromSValue(CreateVector(len, ELTYPE_BIT)) : 0;
	Push(bv);
	size_t cnt = numeric_limits<size_t>::max();
	switch (Type(count = ToOptionalNIL(count))) {
	case TS_BIGNUM:
		{
			BigInteger big = ToBigInteger(count); //!!!? why value not used
			if (big < 0)
				cnt = 0;
		}
		break;
	case TS_FIXNUM:
		cnt = max(LONG_PTR(0), AsNumber(count));  //!!!O
		break;
	case TS_CONS:
		if (!count)
			break;
	default:
		E_TypeErr(count, 0);//!!!
	}
	bool bFromEnd = ToOptionalNIL(fromEnd);
	key = ToOptionalNIL(key);

	FPSeqUpdate pfnUpd;
	int i,
		dir;
	if (bFromEnd) {
		pfnUpd = GetSeqFeUpdate(seq);
		i = int(pp.second-1);
		dir = -1;
		Call(SeqFeInitEnd(sh.m_td), seq, CreateFixnum(pp.second));
	} else {
		pfnUpd = GetSeqUpdate(seq);
		i = int(pp.first);
		dir = 1;
		Call(SeqInitStart(sh.m_td), seq, CreateFixnum(pp.first));
	}
	CP ptr = m_r;
	Push(ptr);
	//!!!	u = FromFunctionDesignator(u);
	for (size_t n=pp.second-pp.first; cnt && n--; i+=dir) {
		Push(m_r=DoSeqAccess(seq, ptr));
		if (key)
			Call(key, m_r);
		if (sh.OnItem(Pop(), m_r, CreateFixnum(i))) {
			cnt--;
			if (bCreateBV)
				AsArray(bv)->SetElement(i, V_1);
		}
		ptr = (this->*pfnUpd)(ptr);
	}
	if (bv)
		sh.Fun(bv);
	SkipStack(2);
	m_cVal = 1; // help to callers
}


/*!!!
CP seq = SV4;
//!!!		 g = SV5 = FromFunctionDesignator(SV5);
Push(seq);
F_GetValidSeqType();
CP td = m_r;
CP key = Pop();
size_t len;
pair<size_t, size_t> pp = PopSeqBoundingIndex(seq, len);

m_r = seq;
SkipStack(6);*/

// (_seq-iterate (g seq from-end start end key)
void CLispEng::F_SeqIterate() {
	struct CIterateHandler : ISeqHandler {
		CP m_g;

		bool OnItem(CP x, CP z, CP i) {
			CLispEng& lisp = LISP_GET_LISPREF;
			lisp.Push(x, z, i);
			lisp.Apply(m_g, 3);
			return true;
		}
	} sh;
	sh.m_g = SV5 = FromFunctionDesignator(SV5);
	ProcessSeq(SV4, SV3, SV2, SV1, 0, SV, sh);
	m_r = SV4;
	SkipStack(6);
}

struct CLispEng::CSeqTestHandler : CLispEng::ISeqHandler {
	CP m_item,
		m_test;
	bool m_bMask;

	CSeqTestHandler()
		:	m_item(V_U)
		,	m_test(g_fnEql)
	{}

	virtual bool OnResult(CP x, CP z, CP i, bool br) { return br; }

	bool OnItem(CP x, CP z, CP i) {
		CLispEng& lisp = LISP_GET_LISPREF;
		if (m_item == V_U) {
			lisp.Push(z);
			lisp.Apply(m_test, 1);
		} else {
			lisp.Push(m_item, z);
			lisp.Apply(m_test, 2);
		}
		return OnResult(x, z, i, m_bMask ^ bool(lisp.m_r));
	}
};

void CLispEng::ProcessSeqTest(CP seq, CP fromEnd, CP start, CP end, CP count, CP key, CSeqTestHandler& sh, bool bCreateBV, CP item, CP test, CP testNot) {
	if ((sh.m_item=item) != V_U) {
		if (test!=V_U && testNot != V_U)
			E_Error();
		if (!(sh.m_bMask=sh.m_test=ToOptionalNIL(testNot)))
			sh.m_test = ToOptional(test, g_fnEql);
	} else {
		sh.m_test = test;
		sh.m_bMask = testNot;
	}
	sh.m_test = FromFunctionDesignator(sh.m_test);
	ProcessSeq(seq, fromEnd, start, end, count, key, sh, bCreateBV);
}

// (_seq-test (f g seq from-end start end key)
void CLispEng::F_SeqTest() {
	struct CTestHandler : CSeqTestHandler {
		CP m_f;

		bool OnResult(CP x, CP z, CP i, bool br) {
			if (br) {
				CLispEng& lisp = LISP_GET_LISPREF;
				lisp.Push(i);
				lisp.Apply(m_f, 1);
			}
			return br;
		}
	} sh;
	sh.m_f = SV6;
	ProcessSeqTest(SV4, SV3, SV2, SV1, 0, SV, sh, false, V_U, SV5, 0);
	m_r = 0;
	SkipStack(7);
}

struct CPositionHandler : CLispEng::CSeqTestHandler {
	CP m_i;

	CPositionHandler()
		:	m_i(0)
	{}

	bool OnResult(CP x, CP z, CP i, bool br) {
		if (br)
			m_i = i;
		return br;
	}
};

// K_FROM_END K_TEST K_TEST_NOT K_START K_END K_KEY
void CLispEng::F_Position() {
	CPositionHandler sh;
	ProcessSeqTest(SV6, SV5, SV2, SV1, V_1, SV, sh, false, SV7, SV4, SV3);
	m_r = sh.m_i;
	SkipStack(8);
}

struct CRemoveHandler : CLispEng::CSeqTestHandler {
	void Fun(CP bv) {
		CLispEng& lisp = LISP_GET_LISPREF;
		CArrayValue *avBv = lisp.AsArray(bv);
		int rlen = 0;
		size_t len = avBv->DataLength;
		for (size_t i=len; i--;)
			rlen += avBv->GetElement(i) != V_1;
		lisp.Call(lisp.SeqMake(m_td), CreateFixnum(rlen));
		CP result = lisp.m_r;
		lisp.Push(result);
		lisp.Call(lisp.SeqInit(m_td), m_seq);
		CP ptrSrc = lisp.m_r;
		lisp.Call(lisp.SeqInit(m_td), result);
		CP ptrDst = lisp.m_r;
		CLispEng::FPSeqUpdate pfnUpd = lisp.GetSeqUpdate(m_seq);
		for (size_t i=0; i<len; i++) {
			if (lisp.AsArray(bv)->GetElement(i) != V_1) {			//!!!O
				lisp.DoSeqSetAccess(result, ptrDst, lisp.DoSeqAccess(m_seq, ptrSrc));
				ptrDst = (lisp.*pfnUpd)(ptrDst);
			}
			ptrSrc = (lisp.*pfnUpd)(ptrSrc);
		}
		lisp.m_r = lisp.Pop();
	}
};

// test seq K_FROM_END K_START K_END K_COUNT K_KEY
void CLispEng::F_RemoveIf() {
#ifdef X_DEBUG//!!!D
	Print(SV5);
#endif

	CRemoveHandler sh;
	ProcessSeqTest(SV5, SV4, SV3, SV2, SV1, SV, sh, true, V_U, SV6, 0);
	SkipStack(7);
	//	Print(m_r);//!!!D
}


} // Lisp::

