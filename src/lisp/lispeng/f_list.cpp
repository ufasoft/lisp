#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_Cons() {
	m_r = Cons(SV1, SV);
	SkipStack(2);
}

void CLispEng::F_Car() {
	m_r = Car(Pop());
}

void CLispEng::F_Cdr() {
	m_r = Cdr(Pop());
}

void CLispEng::F_Null() {	// NULL/NOT
	m_r = FromBool(!Pop());
}

void CLispEng::F_Atom() {
	m_r = FromBool(!ConsP(Pop()));
}

CP CLispEng::GetSymProp(CP sym, CP name) { // returns NIL if not fount
	for (CP p=ToSymbol(sym)->GetPropList(), car; SplitPair(p, car); Inc(p))
		if (car == name)
			return Car(p);
	return 0;
}

CP CLispEng::Get(CP sym, CP name) { // returns V_U if not fount
	for (CP p=ToSymbol(sym)->GetPropList(), car; SplitPair(p, car); Inc(p))
		if (car == name)
			return Car(p);
	return V_U;
}

void CLispEng::F_Get() {
	if ((m_r=Get(SV2, SV1)) == V_U)
		m_r = ToOptionalNIL(SV);
	SkipStack(3);
}


/*!!!R
void CLispEng::Putf(CSPtr& plist, CP p, CP v)
{
for (CP q=plist, car; SplitPair(q, car); Inc(q))
if (car == p)
{
AsCons(q)->m_car = v;
return;
}
Push(p, v, plist);
plist = ListofEx(3);
}
*/

CP& CLispEng::PlistFind(CP& p, CP k) {
	CP *q = &p;
	for (; ConsP(*q) && Car(*q)!=k ; q=&AsCons(*q)->m_cdr)
		if (!ConsP(*(q=&AsCons(*q)->m_cdr)))
			E_Error();
	return *q;
}

void CLispEng::F_Getf() {
	if (CP tail = PlistFind(SV2, SV1))
		m_r = ToCons(Cdr(tail))->m_car;
	else
		m_r = ToOptionalNIL(SV);
	//!!!R	m_r = Getf(SV2, SV1, SV==V_U ? 0 : SV);
	SkipStack(3);
}

void CLispEng::F_PRemf() {
	if (CP& tail = PlistFind(SV1, SV)) {
		CP p = Cdr(tail);
		if (!ConsP(p))
			E_Error();
		Inc(p);
		if (ConsP(p))
			*AsCons(tail) = *AsCons(p);
		else
			tail = p;
		m_arVal[1] = V_T;
	} else
		m_arVal[1] = 0;
	m_r = SV1;
	m_cVal = 2;
	SkipStack(2);
}

void CLispEng::F_PPutf() {
	if (CP tail = PlistFind(SV2, SV1))
		ToCons(Cdr(tail))->m_car = SV;
	else {
		CP cons1 = Cons(0, Cons(0, 0));
		CConsValue *pcons2 = AsCons(Cdr(cons1));
		if (ConsP(SV2)) {
			pcons2->m_car = Car(SV2);
			pcons2->m_cdr = Cdr(SV2);
			AsCons(SV2)->m_car = SV1;
			AsCons(SV2)->m_cdr = cons1;
			AsCons(cons1)->m_car = SV;
		} else {
			pcons2->m_car = SV;
			pcons2->m_cdr = SV2;
			AsCons(m_r=cons1)->m_car = SV1;
		}
	}
	SkipStack(3);
}

void CLispEng::F_PPut() {
	CP plist = ToSymbol(SV2)->GetPropList();
	if (CP tail = PlistFind(plist, SV1))
		ToCons(Cdr(tail))->m_car = SV;
	else
		AsSymbol(SV2)->SetPropList(Cons(SV1, Cons(SV, plist)));
	m_r = SV;
	SkipStack(3);
}

void CLispEng::F_Memq() {
	CP q = Pop(),
		p = Pop(),
		car = 0;
	do {
		m_r = q;
	} while (SplitPair(q, car) && car!=p);
}

void CLispEng::F_SymbolPList() {
	m_r = ToSymbol(Pop())->GetPropList();
}

void CLispEng::F_PPutPlist() {
	m_r = Pop();
	ToSymbol(Pop())->SetPropList(m_r);
}

void CLispEng::F_List(size_t nArgs) {
	m_r = Listof(nArgs);
}

void CLispEng::F_ListEx(size_t nArgs) {
	m_r = ListofEx(nArgs+1);
}

void CLispEng::F_NthCdr() {
	CP p = Pop();
	for (size_t n=AsPositive(Pop()); n && p; n--)
		p = Cdr(p);
	m_r = p;
}

pair<CP, CP> CLispEng::ListLength(CP p) {
	int n = 0;
	for (CP slow=p; ConsP(p); p=Cdr(p), slow=Cdr(slow), n++) {
		p = Cdr(p);
		n++;
		if (!ConsP(p))
			break;
		if (p == slow)
			return pair<CP, CP>(0, 0);
	}
	Push(p);
	CP len = CreateInteger(n);
	return pair<CP, CP>(len, Pop());
}

void CLispEng::F_ListLength() {
	pair<CP, CP> pp = ListLength(Pop());
	if (pp.second)
		E_Error();
	m_r = pp.first;
}

void CLispEng::F_ListLengthDotted() {
	pair<CP, CP> pp = ListLength(Pop());
	if (pp.first) {
		m_r = pp.first;
		m_arVal[1] = pp.second;
		m_cVal = 2;
	}
}

void CLispEng::F_CopyList() {
	m_r = CopyList(Pop());
}

void CLispEng::F_PProperList() {
	pair<CP, CP> pp = ListLength(Pop());
	m_r = pp.first;
	m_arVal[1] = pp.second;
	m_cVal = 2;
}

void CLispEng::F_Append(size_t nArgs) {
	if (!nArgs)
		return;
	int count = 0;
	CP *pStack = m_pStack;
	for (size_t i=nArgs-1; i>0; i--)
		for (CP p=pStack[i], car; SplitPair(p, car); count++)
			Push(car);
	Push(pStack[0]);
	m_r = ListofEx(count+1);
	SkipStack(nArgs);
}

void CLispEng::F_Nconc(size_t nArgs) {
	if (!nArgs)
		return;
	m_r = Pop();
	while (--nArgs) {
		if (CP p = Pop()) {
			if (!ConsP(p))
				E_TypeErr(p, S(L_LIST));
			CP r = p;
			for (CP q; ConsP(q=Cdr(r));)
				r = q;
			AsCons(r)->m_cdr = exchange(m_r, p);
		}
	}
}

void CLispEng::F_NReconc() {
	m_r = Pop();
	for (CP p=Pop(); p;)
		m_r = exchange(p, exchange(ToCons(p)->m_cdr, m_r));
}

void CLispEng::Map(CMapCB& mapCB, CP *pStack, size_t nArgs, bool bCar) {
	CP fun = pStack[nArgs] = FromFunctionDesignator(pStack[nArgs]);
	while (true) {
		for (size_t i=nArgs; i--;) {
			if (CP p = pStack[i]) {
				if (bCar)
					Push(Car(p));
				else
					Push(p);
				pStack[i] = Cdr(p);
			} else
				return;
		}
		Apply(fun, nArgs);
		mapCB.OnResult(m_r);
	}
}

class CMapCarCB : public CLispEng::CMapCB {
public:
	CListConstructor m_lc;

	~CMapCarCB() {
		Lisp().m_r = m_lc;
	}

	void OnResult(CP x) {
		m_lc.Add(x);
	}
};

void CLispEng::F_MapCar(size_t nArgs) {
	CP *pStack = m_pStack;
	{
		CMapCarCB cb;
		Map(cb, pStack, nArgs+1, true);
	}
	SkipStack(nArgs+2);
}

void CLispEng::F_MapList(size_t nArgs) {
	CP *pStack = m_pStack;
	{
		CMapCarCB cb;
		Map(cb, pStack, nArgs+1, false);
	}
	SkipStack(nArgs+2);
}

void CLispEng::F_MapCan(size_t nArgs) {
	CP *pStack = m_pStack;
	{
		CMapCarCB cb;
		Map(cb, pStack, nArgs+1, true);
	}
	SkipStack(nArgs+2);
	size_t count = 0;
	for (CP car; SplitPair(m_r, car); count++)
		Push(car);
	ClearResult();
	F_Nconc(count);
}

void CLispEng::F_MapCon(size_t nArgs) {
	CP *pStack = m_pStack;
	{
		CMapCarCB cb;
		Map(cb, pStack, nArgs+1, false);
	}
	SkipStack(nArgs+2);
	DWORD count = 0;
	for (CP car; SplitPair(m_r, car); count++)
		Push(car);
	ClearResult();
	F_Nconc(count);
}

void CLispEng::F_MapC(size_t nArgs) {
	Push(m_pStack[nArgs]);
	CP *pStack = m_pStack;
	{
		CMapCB cb;
		Map(cb, m_pStack+1, nArgs+1, true);
	}
	m_pStack = pStack; //!!!
	m_r = Pop();
	SkipStack(nArgs+2);
}

void CLispEng::F_MapL(size_t nArgs) {
	Push(m_pStack[nArgs]);
	CP *pStack = m_pStack;
	{
		CMapCB cb;
		Map(cb, m_pStack+1, nArgs+1, false);
	}
	m_pStack = pStack; //!!!
	m_r = Pop();
	SkipStack(nArgs+2);
}

void CLispEng::F_MakeList() {
	CP el = ToOptionalNIL(SV);
	size_t len = AsPositive(SV1);
	m_r = 0;
	while (len--)
		m_r = Cons(el, m_r);
	SkipStack(2);
}

void CLispEng::MemberAssoc(bool bMember) {
#ifdef _X_DEBUG//!!!D
	static int count = 0;
	if (!(++count % 100)) {
		cerr << "\n";
		Print(SV3);
		cerr << "\n";
	}

#endif
	bool bTestNot;
	CP key = ToOptionalNIL(SV2),
		tf = FromFunctionDesignator((bTestNot = ToOptionalNIL(SV)) ? SV : ToOptional(SV1, g_fnEql));
	if (key)
		key = FromFunctionDesignator(key);
	for (CP car, prev; SplitPair(prev=SV3, car); SV3=prev) {
		if (bMember)
			m_r = car;
		else if (!car)
			continue;
		else
			m_r = Car(car);
		if (key) {
			Push(m_r);
			Apply(key, 1);
		}
		bool b;
		if (tf == g_fnEql)
			b = Eql(SV4, m_r);
		else if (tf == g_fnEq)
			b = SV4==m_r;
		else if (tf == g_fnEqual)
			b = Equal(SV4, m_r);
		else {
			Push(SV4, m_r);
			Apply(tf, 2);
			b = m_r;
		}
		if (b ^= bTestNot) {
			if (!bMember)
				SV3 = car; // result
			break;
		}
	}
	m_r = SV3;
	m_cVal = 1;
	SkipStack(5);
}

// (MEMBER item list &key key test test-not)
void CLispEng::F_Member() {
	MemberAssoc(true);
}

// (ASSOC item list &key key test test-not)
void CLispEng::F_Assoc() {
	MemberAssoc(false);
}

void CLispEng::F_NSublis() {
	m_r = SV3;
	if (CP key = ToOptionalNIL(SV2))
		Call(key, m_r);
	Push(m_r, SV4, V_U, SV1, SV);
	F_Assoc();
	if (m_r)
		m_r = Cdr(m_r);
	else {
		if (ConsP(SV3)) {
			Push(SV4, Car(SV3), SV2, SV1, SV);
			F_NSublis();
			AsCons(SV3)->m_car = m_r;
			Push(SV4, Cdr(SV3), SV2, SV1, SV);
			F_NSublis();
			AsCons(SV3)->m_cdr = m_r;
		}
		m_r = SV3;
	}
	SkipStack(5);
}


} // Lisp::

