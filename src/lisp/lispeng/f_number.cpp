#include <el/ext.h>

using namespace rel_ops;

#include "lispeng.h"

namespace Lisp {

BigInteger CLispEng::ToBigInteger(CP p) {
	switch (Type(p)) {
	case TS_FIXNUM:
		return BigInteger(AsFixnum(p));
	case TS_BIGNUM:
		return AsBignum(p)->m_int;
	default:
		E_TypeErr(p, S(L_INTEGER));
	}
}

CP CLispEng::FromCInteger(const BigInteger& v) {
	S_BASEWORD n;
	if (v.AsBaseWord(n) && n < FIXNUM_LIMIT && n >= -FIXNUM_LIMIT)
		return CreateInteger(n);
	return FromSValue(CreateBignum(v));
}

CP CLispEng::BignumFrom(int64_t n) {
	return FromSValue(CreateBignum(BigInteger(n)));
}

CP CLispEng::CreateInteger64(int64_t n) {
	if (n < FIXNUM_LIMIT && n >= -FIXNUM_LIMIT)
		return CreateFixnum((INT_PTR)n);
	return BignumFrom(n);
}

CP CLispEng::CreateIntegerU64(uint64_t n) {
	if (n < FIXNUM_LIMIT)
		return CreateFixnum((INT_PTR)n);
	return BignumFrom(n);
}

void CLispEng::F_LogNAND() {
	m_r = FromCInteger(~(ToBigInteger(SV1) & ToBigInteger(SV)));
	SkipStack(2);
}

void CLispEng::F_LogNOR() {
	m_r = FromCInteger(~(ToBigInteger(SV1) | ToBigInteger(SV)));
	SkipStack(2);
}

void CLispEng::F_LogANDC1() {
	m_r = FromCInteger(~ToBigInteger(SV1) & ToBigInteger(SV));
	SkipStack(2);
}

void CLispEng::F_LogANDC2() {
	m_r = FromCInteger(ToBigInteger(SV1) & ~ToBigInteger(SV));
	SkipStack(2);
}

void CLispEng::F_LogORC1() {
	m_r = FromCInteger(~ToBigInteger(SV1) | ToBigInteger(SV));
	SkipStack(2);
}

void CLispEng::F_LogORC2() {
	m_r = FromCInteger(ToBigInteger(SV1) | ~ToBigInteger(SV));
	SkipStack(2);
}

void CLispEng::F_LogNOT() {
	m_r = FromCInteger(~ToBigInteger(Pop()));
}

void CLispEng::F_LogTest() {
	m_r = FromBool(!!(ToBigInteger(Pop()) & ToBigInteger(Pop())));
}

void CLispEng::F_LogCount() {
	BigInteger v = ToBigInteger(Pop());
	int count = 0;
	int xorMask = Sign(v) < 0 ? 1 : 0;
	for (int i=0, len=v.Length; i<len; ++i)
		count += (int(v.TestBit(i)) ^ xorMask);
	m_r = CreateFixnum(count);
}

void CLispEng::F_LogIOR(size_t nArgs) {
	BigInteger v;
	while (nArgs--)
		v |= ToBigInteger(Pop());
	m_r = FromCInteger(v);
}

void CLispEng::F_LogXOR(size_t nArgs) {
	if (nArgs) {
		BigInteger v = ToBigInteger(Pop());
		while (--nArgs)
			v ^= ToBigInteger(Pop());
		m_r = FromCInteger(v);
	} else
		m_r = V_0;
}

void CLispEng::F_LogAND(size_t nArgs) {
	BigInteger v(-1);
	while (nArgs--)
		v &= ToBigInteger(Pop());
	m_r = FromCInteger(v);
}

void CLispEng::F_LogEQV(size_t nArgs) {
	BigInteger v(-1);
	while (nArgs--)
		v = ~(v^ToBigInteger(Pop()));
	m_r = FromCInteger(v);
}

void CLispEng::F_Numerator() {
	switch (Type(m_r=Pop())) {
	case TS_FIXNUM:
	case TS_BIGNUM:
		break;
	case TS_RATIO:
		m_r = AsRatio(m_r)->m_numerator;
		break;
	default:
		E_TypeErr(m_r, S(L_RATIONAL));
	}
}

void CLispEng::F_Denominator() {
	switch (Type(m_r=Pop())) {
	case TS_FIXNUM:
	case TS_BIGNUM:
		m_r = V_1;
		break;
	case TS_RATIO:
		m_r = AsRatio(m_r)->m_denominator;
		break;
	default:
		E_TypeErr(m_r, S(L_RATIONAL));
	}
}

void CLispEng::F_EvenP() {
	CP p = Pop();
	switch (Type(p)) {
	case TS_FIXNUM:
		m_r = FromBool((p & (1 << VALUE_SHIFT)) == 0);
		break;
	case TS_BIGNUM:
		m_r = FromBool(!ToBigInteger(p).TestBit(0));
		break;
	default:
		E_TypeErr(p, S(L_INTEGER));
	}
}

double CLispEng::ToFloatValue(CP x) {
	switch (Type(x)) {
	case TS_FLONUM:
		return AsFloatVal(x);
	case TS_FIXNUM:
	case TS_BIGNUM:
		return ToBigInteger(x).AsDouble();
	case TS_RATIO:
		CRatio *rat = AsRatio(x);
		BigInteger n = ToBigInteger(rat->m_numerator),
			       d = ToBigInteger(rat->m_denominator);
		int nlen = n.Length,
			dlen = d.Length,
			exp = nlen - (dlen+DBL_MANT_DIG);
		if (exp > 0) {
			d <<= exp;
		} else {
			n <<= -exp;
		}
		BigInteger r = n/d;
		return ::ldexp(r.AsDouble() , exp);
	}
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::F_Float() {
	if (SV != V_U && Type(SV)!=TS_FLONUM)
		E_TypeErr(SV, S(L_FLOAT));
	CP p = SV1;
	switch (Type(p)) {
	case TS_FLONUM:
		m_r = p;
		break;
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
		FloatResult(ToFloatValue(p));
		break;
	default:
		E_TypeErr(p, S(L_REAL));
	}
	SkipStack(2);
}

CP CLispEng::CoerceTo(CP x, CTypeSpec ts) {
	Push(x);
	switch (ts) {
	case TS_COMPLEX:
		if (Type(x) != TS_COMPLEX)
			x = CreateComplex(x, V_0); //!!!
		break;
	case TS_FLONUM:
		Push(x, V_U);
		F_Float();
		x = m_r;
		break;
	case TS_RATIO:
		if (Type(x) != TS_RATIO)
			x = CreateRatio(x, V_1);
	case TS_FIXNUM:
	case TS_BIGNUM:
		break;
	default:
		E_SeriousCondition();
	}
	SkipStack(1);
	return x;
}

void CLispEng::F_NumberP() {
	m_r = FromBool(NumberP(Pop()));
}

void CLispEng::CheckNumbers(size_t nArgs) {
	while (nArgs--) {
		CP p = m_pStack[nArgs];
		if (!NumberP(p))
			E_TypeErr(p, S(L_NUMBER));
	}
}

void CLispEng::CheckIntegers(size_t nArgs) {
	while (nArgs--)
		CheckInteger(m_pStack[nArgs]);
}

void CLispEng::Add(CP n1, CP n2) {
	switch (Type(n1)) {
	case TS_FIXNUM:
		if (Type(n2) == TS_FIXNUM) {
			m_r = CreateInteger(AsFixnum(n1)+AsFixnum(n2));
			break;
		}
	case TS_BIGNUM:
		m_r = FromCInteger(ToBigInteger(n1)+ToBigInteger(n2));
		break;
	case TS_FLONUM:
		FloatResult(AsFloatVal(n1)+AsFloatVal(n2));
		break;
	default:
		E_TypeErr(n1, List(S(L_OR), S(L_INTEGER), S(L_FLOAT)));
	}
}

void CLispEng::Sub(CP n1, CP n2) {
	switch (Type(n1)) {
	case TS_FIXNUM:
		if (Type(n2) == TS_FIXNUM) {
			m_r = CreateInteger(AsFixnum(n1)-AsFixnum(n2));
			break;
		}
	case TS_BIGNUM:
		m_r = FromCInteger(ToBigInteger(n1)-ToBigInteger(n2));
		break;
	case TS_FLONUM:
		{
			double dx = ToFloatValue(n1), dy = ToFloatValue(n2);
			FloatResult(dx-dy);
		}
		break;
	default:
		E_TypeErr(n1, List(S(L_OR), S(L_INTEGER), S(L_FLOAT)));
	}
}

void CLispEng::NormPair() {
	CP x = SV1,
		y = SV;
	if (IntegerP(x) || Type(y)==TS_COMPLEX)
		x = CoerceTo(x, Type(y));
	else if (IntegerP(y) || Type(x)==TS_COMPLEX)
		y = CoerceTo(y, Type(x));
	else if (Type(x) == TS_FLONUM)
		y = CoerceTo(y, Type(x));
	else
		x = CoerceTo(x, Type(y));
	m_r = x;
	m_arVal[1] = y;
	m_cVal = 2;
	SkipStack(2);
}

void CLispEng::AddSub(FPAddSub pfn, size_t nArgs, bool bMinus) {
	CheckNumbers(nArgs);
	ssize_t i = nArgs;
	m_r = i>int(bMinus) ? m_pStack[--i] : V_0;
	while (i--) {
		Push(m_r, m_pStack[i]);
		NormPair();
		CP x = m_r,
			y = m_arVal[1];
		Push(x, y);
		switch (Type(x)) {
		case TS_COMPLEX:
			{
				CComplex *cx = AsComplex(x),
					*cy = AsComplex(y);
				Push(cx->m_real, cy->m_real);
				AddSub(pfn, 2, bMinus);
				Push(m_r, cx->m_imag, cy->m_imag);
				AddSub(pfn, 2, bMinus);
				Push(m_r);
				F_Complex();
			}
			break;
		case TS_RATIO:
			{
				CRatio *rx = AsRatio(x),
					*ry = AsRatio(y);
				BigInteger nx = ToBigInteger(rx->m_numerator),
					dx = ToBigInteger(rx->m_denominator),
					ny = ToBigInteger(ry->m_numerator),
					dy = ToBigInteger(ry->m_denominator);
				Push(FromCInteger(nx*dy));
				(this->*pfn)(SV, FromCInteger(ny*dx));
				SkipStack(1);
				Push(m_r, FromCInteger(dy*dx));
				F_Divide(1);
			}
			break;
		default:
			(this->*pfn)(x, y);
		}
		SkipStack(2);
	}
	SkipStack(nArgs);
	m_cVal = 1;
}

void CLispEng::F_Plus(size_t nArgs) {
	AddSub(&CLispEng::Add, nArgs, false);
}

void CLispEng::F_Minus(size_t nArgs) {
	AddSub(&CLispEng::Sub, nArgs+1, true);
}

void CLispEng::F_Multiply(size_t nArgs) {
	CheckNumbers(nArgs);
	if (nArgs == 0)
		m_r = V_1;
	else {
		while (--nArgs > 0) {
			CP x = SV, y = SV1;
			if (Type(x)==TS_FIXNUM && Type(y)==TS_FIXNUM) {
				SV1 = CreateInteger64(int64_t(AsFixnum(x)) * int64_t(AsFixnum(y)));
				SkipStack(1);
			} else if (Type(x)==TS_COMPLEX || Type(y)==TS_COMPLEX)
				Mul(CoerceToComplex(x), CoerceToComplex(y));
			else if (Type(x)==TS_FLONUM || Type(y)==TS_FLONUM) {
				FloatResult(ToFloatValue(x) * ToFloatValue(y));
				SkipStack(1);
				SV = m_r;
			} else if (Type(x)==TS_RATIO || Type(y)==TS_RATIO)
				Mul(CoerceToRatio(x), CoerceToRatio(y));
			else  {
				SV1 = FromCInteger(ToBigInteger(x) * ToBigInteger(y));
				SkipStack(1);
			}
		}
		m_r = Pop();
	}
}

void CLispEng::Mul(const CComplex& x, const CComplex& y) {	//(complex (- (* a c) (* b d)) (+ (* c b) (* a d)))
	Push(x.m_real, y.m_real);
	F_Multiply(2);
	Push(m_r);
	Push(x.m_imag, y.m_imag);
	F_Multiply(2);
	Push(m_r);
	F_Minus(1);
	Push(m_r);

	Push(x.m_imag, y.m_real);
	F_Multiply(2);
	Push(m_r);
	Push(x.m_real, y.m_imag);
	F_Multiply(2);
	Push(m_r);
	F_Plus(2);
	Push(m_r);

	F_Complex();
	SkipStack(1);
	SV = m_r;
}

void CLispEng::F_Abs() {
	CP x = SV;
	if (!NumberP(x))
		E_TypeErr(x, S(L_NUMBER));
	switch (Type(x)) {
	case TS_COMPLEX:
		{
			CComplex *c = AsComplex(x);

			Push(c->m_real, c->m_real);
			F_Multiply(2);
			Push(m_r);

			Push(c->m_imag, c->m_imag);
			F_Multiply(2);
			Push(m_r);

			F_Plus(2);
			Call(S(L_SQRT), m_r);
		}
		break;
	default:
		Push(x);
		F_MinusP();
		if (m_r) {
			Push(x);
			F_Minus(0);
		} else
			m_r = x;
	}

	SkipStack(1);
}

void CLispEng::F_Truncate() {
	CP d = ToOptional(SV, V_1),
		n = SV1;
	CheckReal(d);
	CheckReal(n);
	
	Push(d);
	F_ZeroP();
	if (m_r)
		E_DivisionByZero(n, d);

	if (IntegerP(n) && IntegerP(d)) {
		if (d == V_1) {
			m_r = n;
			m_arVal[1] = V_0;
		} else {
			pair<BigInteger, BigInteger> p = div(ToBigInteger(n), ToBigInteger(d));
			m_r = FromCInteger(p.first);
			m_arVal[1] = FromCInteger(p.second);
		}
	} else if (FloatP(n) || FloatP(d)) {
		Push(n, V_U);
		F_Float();
		double dn = AsFloatVal(m_r);

		Push(d, V_U);
		F_Float();
		double dd = AsFloatVal(m_r);

		double r = dn/dd;
		Push(FromFloat(r));
		F_IntegerDecodeFloat();
		bool bMinus = m_arVal[2] != V_1;
		Push(m_r, m_arVal[1]);
		F_ASH();
		if (bMinus) {
			Push(m_r);
			F_Minus(0);
		}
		Push(m_r, m_r, V_U);
		F_Float();
		m_arVal[1] = FromFloat(dn-AsFloatVal(m_r)*dd);
		m_r = Pop();
	} else { 			// only rationals can be here
		Push(n, d);
		F_Divide(1);
		Push(0, m_r, m_r);
		F_Numerator();
		SV1 = m_r;
		F_Denominator();
		Push(m_r);
		F_Truncate();
		Push(m_r);	          	// q
		Push(m_r, d);
		F_Multiply(2);			// (values q (- n (* q d)))
		Push(n, m_r);
		F_Minus(1);
		m_arVal[1] = m_r;
		m_r = Pop();
	}
	m_cVal = 2;
	SkipStack(2);
}

void CLispEng::F_GCD(size_t nArgs) {
	CheckIntegers(nArgs);
	if (nArgs == 0)
		m_r = V_0;
	else {
		F_Abs();
		Push(m_r);
		while (--nArgs > 0) {
			Push(SV1);
			F_Abs();
			SV1 = m_r;			

			while (SV != V_0) {								// Euclid algorithm
				Push(SV1, SV);
				F_Truncate();
				SV1 = SV;
				SV = m_arVal[1];
			}
			SkipStack(1);
		}
		m_r = Pop();
		m_cVal = 1;
	}
}

void CLispEng::MakeRatio() {		// Contract: n, d are Integers
	CP n = SV1, d = SV;
	if (d == V_0)
		E_DivisionByZero(n, d);

	Push(n, d);
	F_GCD(2);
	
	Push(n, m_r);
	Push(d, m_r);
	F_Truncate();
	d = SV2 = m_r;
	F_Truncate();
	n = SV1 = m_r;

	Push(d);
	F_MinusP();
	if (m_r) {
		n = SV1 = FromCInteger(-ToBigInteger(n));
		d = SV = FromCInteger(-ToBigInteger(d));
	}

	m_r = d==V_1 ? n : CreateRatio(n, d);
	m_cVal = 1;
	SkipStack(2);
}

void CLispEng::Mul(const CRatio& x, const CRatio& y) {
	Push(x.m_numerator, y.m_numerator);
	F_Multiply(2);
	Push(m_r);

	Push(x.m_denominator, y.m_denominator);
	F_Multiply(2);
	Push(m_r);

	MakeRatio();

	SkipStack(1);
	SV = m_r;
}

/*!!!!R
void CLispEng::F_Mul() {
	CP n2 = Pop(),
		n1 = Pop();
	switch (Type(n1)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
		m_r = FromCInteger(ToBigInteger(n1)*ToBigInteger(n2));
		break;
	case TS_FLONUM:
		m_r = FromFloat(AsFloatVal(n1)*AsFloatVal(n2));
		break;
	default:
		E_TypeErr(n1, List(S(L_OR), S(L_INTEGER), S(L_FLOAT)));
	}
}
*/

/*!!!R
void CLispEng::F_Div() {
	CP n2 = Pop(),
		n1 = Pop();
	switch (Type(n1)) {
		//!!!R  case TS_FIXNUM:
		//!!!Rcase TS_BIGNUM:
		//!!!Rm_r = DivBignums(n1, n2);
		//!!!Rbreak;
	case TS_FLONUM:
		m_r = FromFloat(AsFloatVal(n1) / AsFloatVal(n2));
		break;
	default:
		E_TypeErr(n1, S(L_FLOAT));
	}
}
*/

void CLispEng::F_Divide(size_t nArgs) {
	CheckNumbers(nArgs+1);
	if (nArgs == 0) {
		Push(SV);
		SV1 = V_1;
	} else if (nArgs>1) {
		F_Multiply(nArgs);				//  (/ a b c ...) === (/ a (* b c ...))
		Push(m_r);	
	}
	CP x = SV1, y = SV;
	Push(y);
	F_ZeroP();
	if (m_r)
		E_DivisionByZero(x, y);

	if (IntegerP(x) && IntegerP(y))
		MakeRatio();
	else if (Type(x)==TS_COMPLEX || Type(y)==TS_COMPLEX) {
		CComplex cx = CoerceToComplex(x), cy = CoerceToComplex(y);

		Push(cy.m_real, cy.m_real);
		F_Multiply(2);
		Push(m_r);
		Push(cy.m_imag, cy.m_imag);
		F_Multiply(2);
		Push(m_r);
		F_Plus(2);
		CP m = m_r;
		Push(m);						// m = c^2+d^2

		Push(cx.m_real, cy.m_real);
		F_Multiply(2);
		Push(m_r);
		Push(cx.m_imag, cy.m_imag);
		F_Multiply(2);
		Push(m_r);
		F_Plus(2);
		Push(m_r, m);
		F_Divide(1);					//  (ac+bd)/m
		Push(m_r);

		Push(cx.m_imag, cy.m_real);
		F_Multiply(2);
		Push(m_r);
		Push(cx.m_real, cy.m_imag);
		F_Multiply(2);
		Push(m_r);
		F_Minus(1);
		Push(m_r, m);
		F_Divide(1);					//  (bc-ad)/m
		Push(m_r);

		F_Complex();
		SkipStack(3);
	} else if (Type(x)==TS_FLONUM || Type(y)==TS_FLONUM) {
		double dx = ToFloatValue(x), dy = ToFloatValue(y);
		FloatResult(dx / dy);
		SkipStack(2);
	} else {		// Rational
		CRatio rx = CoerceToRatio(x), ry = CoerceToRatio(y);

		Push(rx.m_numerator, ry.m_denominator);
		F_Multiply(2);
		Push(m_r);

		Push(ry.m_numerator, rx.m_denominator);
		F_Multiply(2);
		Push(m_r);

		MakeRatio();
		SkipStack(2);
	}
}

void CLispEng::Lesser(CP x, CP y) {
	switch (Type(x)) {
	case TS_FIXNUM:
		if (Type(y) == TS_FIXNUM) {
			m_r = FromBool(intptr_t(x) < intptr_t(y));
			break;
		}
	case TS_BIGNUM:
		m_r = FromBool(ToBigInteger(x) < ToBigInteger(y));
		break;
	case TS_FLONUM:
		m_r = FromBool(AsFloatVal(x) < AsFloatVal(y));
		break;
	default:
		E_TypeErr(x, S(L_REAL));
	case TS_RATIO:
		CRatio *rx = AsRatio(x),
			*ry = AsRatio(y);
		m_r = FromBool(ToBigInteger(rx->m_numerator)*ToBigInteger(ry->m_denominator) < ToBigInteger(ry->m_numerator)*ToBigInteger(rx->m_denominator));
		break;
	}
}

void CLispEng::Greater(CP x, CP y) {
	Lesser(y, x);
}

void CLispEng::GreaterOrEqual(CP x, CP y) {
	Lesser(x, y);
	m_r = FromBool(!m_r);
}

void CLispEng::LesserOrEqual(CP x, CP y) {
	Lesser(y, x);
	m_r = FromBool(!m_r);
}

void CLispEng::BinCompare(FPAddSub pfn, size_t nArgs) {
	m_r = V_T;
	for (size_t i=nArgs-1; i--;) {
		Push(m_pStack[i+1], m_pStack[i]);
		NormPair();
		(this->*pfn)(m_r, m_arVal[1]);
		if (!m_r)
			break;
	}
	SkipStack(nArgs);
	m_cVal = 1;
}

void CLispEng::F_Lesser(size_t nArgs) {
	CheckReals(nArgs+1);
	BinCompare(&CLispEng::Lesser, nArgs+1);
}

void CLispEng::F_Greater(size_t nArgs) {
	CheckReals(nArgs+1);
	BinCompare(&CLispEng::Greater, nArgs+1);
}

void CLispEng::F_GreaterOrEqual(size_t nArgs) {
	if (nArgs==1) {
		CP x = SV1, y = SV;
		if (Type(x)==TS_FIXNUM && Type(y)==TS_FIXNUM) {
			m_r = FromBool(AsFixnum(x) >= AsFixnum(y));
			SkipStack(2);
			return;
		}
	}
	CheckReals(nArgs+1);
	BinCompare(&CLispEng::GreaterOrEqual, nArgs+1);
}

void CLispEng::F_LesserOrEqual(size_t nArgs) {
	if (nArgs==1 && Type(SV)==TS_FIXNUM && Type(SV1)==TS_FIXNUM) {
		m_r = FromBool(AsFixnum(SV1) <= AsFixnum(SV));
		SkipStack(2);
	} else {
		CheckReals(nArgs+1);
		BinCompare(&CLispEng::LesserOrEqual, nArgs+1);
	}
}

/*!!!R
void CLispEng::EqNum(CP x, CP y) {
	switch (Type(x)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
		m_r = FromBool(ToBigInteger(x)==ToBigInteger(y));
		break;
	case TS_FLONUM:
		m_r = FromBool(AsFloatVal(x)==AsFloatVal(y));
		break;
	case TS_COMPLEX:
		{
			Push(x, y);
			CComplex *cx = AsComplex(x),
				*cy = AsComplex(y);
			Push(cx->m_real, cy->m_real);
			F_EqNum(1);
			if (m_r) {
				Push(cx->m_imag, cy->m_imag);
				F_EqNum(1);
			}
			SkipStack(2);
		}
		break;
	case TS_RATIO:
		{
			CRatio *rx = AsRatio(x),
				*ry = AsRatio(y);
			m_r = FromBool(ToBigInteger(rx->m_numerator)==ToBigInteger(ry->m_numerator) && ToBigInteger(rx->m_denominator)==ToBigInteger(ry->m_denominator));
		}
		break;
	default:
		E_Error();
	}
}*/

bool CLispEng::EqualpReal(CP x, CP y) {
	if (Type(x)==TS_FLONUM) {
		double xf = AsFloatVal(x);
		if (Type(y)==TS_FLONUM)
			return xf == AsFloatVal(y);
		return xf == ToFloatValue(y);
	}
	if (Type(y)==TS_FLONUM)
		return AsFloatVal(y) == ToFloatValue(y);
	return Eql(x, y);
}

bool CLispEng::EqualpNum(const CComplex& x, const CComplex& y) {
	return EqualpReal(x.m_real, y.m_real) && EqualpReal(x.m_imag, y.m_imag);
}

void CLispEng::EqualpNum(CP x, CP y) {
	m_r = FromBool(EqualpNum(CoerceToComplex(x), CoerceToComplex(y)));
}

void CLispEng::F_EqNum(size_t nArgs) {
	CheckNumbers(nArgs+1);
	BinCompare(&CLispEng::EqualpNum, nArgs+1);
}

void CLispEng::F_AllDifferent(size_t nArgs) {
	CP *pStack = m_pStack+nArgs+1;
	for (; nArgs; nArgs--, SkipStack(1)) {
		for (size_t i=nArgs; i--;) {
			Push(m_pStack[i+1], SV);
			Apply(pStack[0], 2);
			if (m_r) {
				m_r = 0;
				goto LAB_RET;
			}
		}
	}
	m_r = V_T;
LAB_RET:
	m_pStack = pStack+1;
}


void CLispEng::F_NotEqNum(size_t nArgs) {
	CheckNumbers(nArgs+1);
	Push(m_pStack[nArgs]);
	m_pStack[nArgs+1] = g_fnEqNum;
	F_AllDifferent(nArgs);
}

void CLispEng::F_Inc() {
	CP x = Pop();
	if (Type(x) == TS_FIXNUM)
		m_r = CreateInteger(AsFixnum(x)+1);
	else {
		Push(x, V_1);
		F_Plus(2);
	}
}

void CLispEng::F_Dec() {
	Push(V_1);
	F_Minus(1);
}

void CLispEng::F_IntegerP() {
	m_r = FromBool(IntegerP(Pop()));
}

void CLispEng::F_FloatP() {
	m_r = FromBool(FloatP(Pop()));
}

bool CLispEng::RationalP(CP p) {
	switch (Type(p)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
		return true;
	}
	return false;
}

bool CLispEng::RealP(CP p) {
	switch (Type(p)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
	case TS_FLONUM:
		return true;
	}
	return false;
}

void CLispEng::F_RationalP() {
	m_r = FromBool(RationalP(Pop()));
}

void CLispEng::F_RealP() {
	switch (Type(Pop())) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
	case TS_FLONUM:
		m_r = FromBool(true);
	}
}

void CLispEng::CheckReal(CP x) {
	switch (Type(x)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
	case TS_FLONUM:
		break;
	default:
		E_TypeErr(x, S(L_REAL));		
	}
}

void CLispEng::CheckReals(size_t nArgs) {
	while (nArgs--)
		CheckReal(m_pStack[nArgs]);
}

/*!!!R
void CLispEng::F_Rational() {
	m_r = Pop();
	switch (Type(m_r)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
		break;
	case TS_FLONUM:
		{
			Push(m_r);
			F_IntegerDecodeFloat();
			Push(m_r, m_arVal[2]);
			Call(S(L_EXPT), CreateInteger(2), m_arVal[1]);
			Push(m_r);
			Funcall(S(L_ASTERISK), 3);
		}
		break;
	default:
		E_Error();//!!!
	}
}
*/

/*!!!
void CLispEng::F_Rational()
{
double dbl = AsFloatVal(Pop());
bool bMinus = dbl < 0;
dbl = fabs(dbl);
const DWORD d = 0x100000;
DWORD n = DWORD(dbl * d);
Apply("MAKE-INSTANCE", GetSymbol("RATIONAL"));
CSPtr rat = m_r; //!!!
CObjectValue *ov = ToObject(rat);
ov->SetSlotValue(GetSymbol("M_SIGN"), FromBool(bMinus));
ov->SetSlotValue(GetSymbol("M_NUM"), CreateInteger((short)n));
ov->SetSlotValue(GetSymbol("M_DIV"), CreateInteger((short)d));
m_r = CSPtr(rat);
//!!!  m_r = CreateRatio(n, d, bMinus); 
}

void CLispEng::F_AddInt()
{
int n1 = AsNumber(Pop()),
n2 = AsNumber(Pop());
__int64 r = __int64(n1)+__int64(n2);
m_r = List(FromBool(r & 0x80000000), FromBool(r & 0x100000000), CreateInteger((short)r));
}*/

double CLispEng::CheckFinite(double d) {
	switch (fpclassify(d)) {
	case FP_INFINITE:
		CerrorOverflow();
		break;
	case FP_SUBNORMAL:
		CerrorUnderflow();
		break;
	case FP_NAN:
//!!!R		E_Error(); //!!!D
		E_ArithmeticErr(IDS_E_NaN, GetSubrName(m_subrSelf));
	}
	return d;
}

void CLispEng::FloatResult(double d) {
	m_r = FromFloat(CheckFinite(d));
}

void CLispEng::F_Sqrt() {
	FloatResult(sqrt(AsFloatVal(Pop())));
}

void CLispEng::F_Exp() {
	FloatResult(exp(AsFloatVal(Pop())));
}

void CLispEng::F_Log() {
	FloatResult(log(AsFloatVal(Pop())));
}

void CLispEng::F_Sin() {
	FloatResult(sin(AsFloatVal(Pop())));
}

void CLispEng::F_Cos() {
	FloatResult(cos(AsFloatVal(Pop())));
}

void CLispEng::F_ASin() {
	FloatResult(asin(AsFloatVal(Pop())));
}

void CLispEng::F_ACos() {
	FloatResult(acos(AsFloatVal(Pop())));
}

void CLispEng::F_ATan() {
	FloatResult(atan(AsFloatVal(Pop())));
}

void CLispEng::F_ATan2() {
	CP b = Pop(),
		a = Pop();
	FloatResult(::atan2(AsFloatVal(a), AsFloatVal(b)));
}

void CLispEng::F_Phase() {
	CP p = Pop();
	switch (Type(p)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
	case TS_FLONUM:
		Push(p);
		F_MinusP();
		m_r = m_r ? Spec(L_PI) : V_0;
		break;
	case TS_COMPLEX:
		{
			CComplex *x = AsComplex(p);
			FloatResult(::atan2(ToFloatValue(x->m_imag), ToFloatValue(x->m_real)));
		}
		break;
	default:
		E_TypeErr(p, S(L_NUMBER));	
	}
}

static int fsign(double d) {
	return d>=0 ? 1 : -1;
}

void CLispEng::F_Signum() {
	CP p = Pop();
	int n = 0;
LAB_START:
	switch (Type(p)) {
	case TS_RATIO:
		p = AsRatio(p)->m_numerator;
		goto LAB_START;
	case TS_FIXNUM:
		n = AsNumber(p);
		break;
	case TS_BIGNUM:
		n = Sign(ToBigInteger(p));
		break;
	case TS_FLONUM:
		FloatResult(fsign(AsFloatVal(p)));
		return;
	case TS_COMPLEX:
		Push(p);
		F_Phase();
		Push(0, m_r, V_U, m_r, V_U);
		F_Float();
		Push(m_r);
		F_Cos();
		m_pStack[2] = m_r;
		F_Float();
		Push(m_r);
		F_Sin();
		Push(m_r);
		F_Complex();
		return;
	default:
		E_TypeErr(p, S(L_NUMBER));	
	}
	if (n == 0)
		m_r = V_0;
	else
		m_r = n<0 ? V_M1 : V_1;
}

class CFloat8 {
	union {
		double m_d;
		int64_t m_i;
	};

	int ExpPart() {
		return int(m_i >> (DBL_MANT_DIG-1)) & 0x7FF;
	}
public:

	CFloat8(double d) {
		m_d = d;
	}

	int64_t Mantissa() {
		int64_t r = m_i & 0xFFFFFFFFFFFFFLL;
		if (!r && !ExpPart())
			return 0;
		return r | 0x10000000000000LL;
	}

	int Exponent() {
		return ExpPart()-DBL_MAX_EXP-DBL_MANT_DIG+2;
	}

	int Sign() {
		return m_i < 0 ? -1 : 1;
	}
};

void CLispEng::F_DecodeFloat() {
	int exp;
	double r = frexp(AsFloatVal(Pop()), &exp);
	CFloat8 f(r);
	if (f.Sign() == -1)
		r = -r;
	m_r = FromFloat(r);
	m_arVal[1] = CreateInteger(exp);
	m_arVal[2] = FromFloat(f.Sign());
	m_cVal = 3;
}

void CLispEng::F_FloatDigits() {
	AsFloatVal(Pop());
	m_r = CreateFixnum(DBL_MANT_DIG);
}

void CLispEng::F_ScaleFloat() {
	int exp = AsNumber(Pop());
	FloatResult(ldexp(AsFloatVal(Pop()), exp));
}

void CLispEng::F_FloatSign() {
	double val = 1;
	CP p = Pop();
	if (p != V_U)
		val = AsFloatVal(p);
	m_r = FromFloat(copysign(val, AsFloatVal(Pop())));
}

void CLispEng::F_IntegerDecodeFloat() {
	CFloat8 f = AsFloatVal(Pop());
#ifdef _DEBUG //!!!D
	int64_t m = f.Mantissa();
	int e = f.Exponent();
	int s = f.Sign();
#endif
	m_r = CreateInteger64(f.Mantissa());
	m_arVal[1] = CreateInteger(f.Exponent());
	m_arVal[2] = CreateInteger(f.Sign());
	m_cVal = 3;
}

void CLispEng::F_ZeroP() {
	CP p = Pop();
	switch (Type(p)) {
	case TS_FIXNUM:
		m_r = FromBool(p==V_0);
		break;
	case TS_BIGNUM:
	case TS_RATIO:
		m_r = 0;
		break;
	case TS_FLONUM:
		m_r = FromBool(AsFloatVal(p) == 0);
		break;
	case TS_COMPLEX:
		{
			CComplex *c = AsComplex(p);
			Push(c->m_real, c->m_imag);
			F_ZeroP();
			if (!m_r)
				SkipStack(1);
			else
				F_ZeroP();
		}
		break;
	default:
		E_TypeErr(p, S(L_NUMBER));
	}
}

void CLispEng::F_MinusP() {
	CSPtr p = Pop();
	switch (Type(p)) {
	case TS_FIXNUM:
		m_r = FromBool(AsFixnum(p) < 0);
		break;
	case TS_BIGNUM:
		m_r = FromBool(Sign(ToBigInteger(p)) < 0);
		break;
	case TS_FLONUM:
		m_r = FromBool(AsFloatVal(p) < 0);
		break;
	case TS_RATIO:
		Push(AsRatio(p)->m_numerator);
		F_MinusP();
		break;
	default:
		E_TypeErr(p, S(L_REAL));
	}
}

void CLispEng::F_PlusP() {
	CSPtr p = Pop();
	switch (Type(p)) {
	case TS_FIXNUM:
		m_r = FromBool(AsFixnum(p) > 0);
		break;
	case TS_BIGNUM:
		m_r = FromBool(Sign(ToBigInteger(p)) > 0);
		break;
	case TS_FLONUM:
		m_r = FromBool(AsFloatVal(p) > 0);
		break;
	case TS_RATIO:
		Push(AsRatio(p)->m_numerator);
		F_PlusP();
		break;
	default:
		E_TypeErr(p, S(L_REAL));
	}
}




/*!!!D
void CLispEng::F_Floor1()
{
m_r = Pop();
switch (Type(m_r)) {
case TS_FLONUM:
m_r = CreateInteger64((LONGLONG)CheckFinite(floor(AsFloatVal(m_r))));
break;
case TS_RATIO:
{
CRatio *ra = AsRatio(m_r);
m_r = FromCInteger(BigInteger(ra->m_numerator)/BigInteger(ra->m_denominator));
}
case TS_FIXNUM:
case TS_BIGNUM:
break;
default:
E_Error();
}  
}*/

void CLispEng::F_Complex() {
	CP imag = Pop();
	if (!RealP(SV))
		E_TypeErr(SV, S(L_REAL));
	if (imag == V_U) {
		if (Type(SV) == TS_FLONUM)
			imag = FromFloat(0);
		else
			imag = V_0;
	} else if (!RealP(imag))
		E_TypeErr(imag, S(L_REAL));
	if (RationalP(SV) && imag==V_0)
		m_r = SV;
	else
		m_r = CreateComplex(Pop(), imag);
}

void CLispEng::F_RealPart() {
	m_r = Pop();
	switch (Type(m_r)) {
	case TS_COMPLEX:
		m_r = AsComplex(m_r)->m_real;
		break;
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_FLONUM:
	case TS_RATIO:
		break;
	default:
		E_TypeErr(m_r, S(L_NUMBER));
	}
}

void CLispEng::F_ImagPart() {
	m_r = Pop();
	switch (Type(m_r)) {
	case TS_COMPLEX:
		m_r = AsComplex(m_r)->m_imag;
		break;
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
		m_r = V_0;
		break;
	case TS_FLONUM:
		m_r = FromFloat(0);
		break;
	default:
		E_TypeErr(m_r, S(L_NUMBER));
	}
}

void CLispEng::F_IntegerLength() {
	m_r = CreateInteger((int)ToBigInteger(Pop()).Length);
}

void CLispEng::F_LogBitP() {
	BigInteger i = ToBigInteger(Pop());
	m_r = FromBool(i.TestBit(AsPositive(Pop())));
}

void CLispEng::F_Ldb() {
	BigInteger i = ToBigInteger(Pop());
	size_t pos = AsPositive(Cdr(SV)),
		size = AsPositive(Car(Pop()));
	m_r = FromCInteger((i >> pos) & ((BigInteger(1)<<size)-1));
#ifdef X_DEBUG //!!!D
	static int s_cnt;
	if (++s_cnt == 0x15 || m_r == 0x7E19 || m_r == 0x7419)  //!!! 0xBC 0x83
		pos = pos;
#endif
}

void CLispEng::F_MaskField() {
	BigInteger i = ToBigInteger(Pop());
	size_t pos = AsPositive(Cdr(SV)),
		size = AsPositive(Car(Pop()));
	m_r = FromCInteger(i & (((BigInteger(1)<<size)-1) << pos));
}

void CLispEng::F_DepositField() {
	BigInteger i = ToBigInteger(Pop());
	size_t pos = AsPositive(Cdr(SV)),
		size = AsPositive(Car(Pop()));
	BigInteger n = ToBigInteger(Pop());
	BigInteger mask = ((BigInteger(1)<<size)-1) << pos;
	m_r = FromCInteger(i & ~mask | n & mask);
}

void CLispEng::F_ASH() {
	int n = AsNumber(Pop());
	BigInteger v = ToBigInteger(Pop());
	m_r = FromCInteger(n<0 ? v >> -n : v << n);
}

#if UCFG_LISP_BUILTIN_RANDOM_STATE
void CLispEng::F_RandomStateP() {
	m_r = FromBool(Type(Pop()) == TS_RANDOMSTATE);
}
#endif

CP CLispEng::CpFromRandomState(CP rs) {
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	return ToRandomState(rs)->m_rnd;
#else
	Push(S(L_RANDOM_STATE), rs, V_1);
	return GetStructRef();
#endif
}

void CLispEng::FromRandomState(CP rs, default_random_engine& rngeng) {
	istringstream is(AsTrueString(CpFromRandomState(rs)).c_str());
	is >> rngeng;
}

CP CLispEng::SerializedRandomeEngine(const default_random_engine& rneng) {
	ostringstream os;
	os << rneng;
	return CreateString(os.str());
}

void CLispEng::ModifyRandomState(CP rs, CP p) {
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	AsRandomState(rs)->m_rnd = p;
#else
	Push(S(L_RANDOM_STATE), rs, V_1);
	GetStructRef() = p;
#endif
}

void CLispEng::ModifyRandomState(CP rs, const default_random_engine& rneng) {
	ModifyRandomState(rs, SerializedRandomeEngine(rneng));
}

void CLispEng::F_Random() {
	Push(SV1);
	F_PlusP();
	if (!m_r)
		E_TypeErr(SV1, List(S(L_REAL), V_0, S(L_ASTERISK)));
	CP pRS = SV==V_U ? Spec(L_S_RANDOM_STATE) : SV;

	default_random_engine rngeng;
	FromRandomState(pRS, rngeng);
	switch (Type(SV1)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
		m_r = FromCInteger(BigInteger::Random(ToBigInteger(SV1), rngeng));
		break;
	case TS_FLONUM:
		m_r = FromFloat(uniform_real_distribution<double>(0, AsFloatVal(SV1)) (rngeng));
		break;
	default:
		E_TypeErr(SV1, List(S(L_OR), S(L_INTEGER), S(L_FLOAT)));
	}
	ModifyRandomState(pRS, rngeng);
	SkipStack(2);
}

void CLispEng::F_MakeRandomState() {
	CP p = Pop();
	CP rs;
	switch (p) {
	case V_T: 
		rs  = SerializedRandomeEngine(default_random_engine());
		break;
	case 0:
	case V_U:
		p = Spec(L_S_RANDOM_STATE);
	default:
		rs = CpFromRandomState(p);
	}
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	m_r = FromSValueT(CreateRandomState(seed), TS_RANDOMSTATE);
#else
	if (AsSymbol(S(L_SYS_MAKE_RANDOM_STATE))->GetFun())
		Call(S(L_SYS_MAKE_RANDOM_STATE), S(L_K_DATA), rs);
	else {
		Push(rs);
		m_r = FromSValueT(CreateVector(2), TS_STRUCT);					// we need random-state before defining this type
		TheStruct(m_r).Types = List(S(L_RANDOM_STATE));
		ModifyRandomState(m_r, Pop());
	}
#endif
}


} // Lisp::

