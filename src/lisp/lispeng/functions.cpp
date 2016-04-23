/*######   Copyright (c) 2002-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_Eq() {
	m_r = FromBool(SV == SV1);
	SkipStack(2);
}

/*!!!D
static bool FloatsEqual(double a, double b)  {		// not-precise compare
	if (a == b)
		return true;
	return fabs((a-b)/max(a, b)) < 0.001;
}
*/

bool __fastcall CLispEng::Eql(CP p, CP q) {
	if (p == q)
		return true;
	if (Type(p) != Type(q))
		return false;
	switch (Type(p)) {
	case TS_BIGNUM:
		return ToBigInteger(p)==ToBigInteger(q);
	case TS_FLONUM:
#ifdef X_DEBUG
		return FloatsEqual(AsFloatVal(p), AsFloatVal(q));
#else
		return AsFloatVal(p) == AsFloatVal(q);
#endif
	case TS_RATIO:
		{
			CRatio *r1 = AsRatio(p),
				*r2 = AsRatio(q);
			return Eql(r1->m_numerator, r2->m_numerator) && Eql(r1->m_denominator, r2->m_denominator);
		}
		break;
	case TS_COMPLEX:
		{
			CComplex *c1 = AsComplex(p),
				*c2 = AsComplex(q);
			return Eql(c1->m_real, c2->m_real) && Eql(c1->m_imag, c2->m_imag);
		}
		break;
#if UCFG_LISP_FFI
	case TS_FF_PTR:
		return ToPointer(p)==ToPointer(q);
#endif
	default:
		return false;
	}
}

void CLispEng::F_Eql() {
	m_r = FromBool(Eql(Pop(), Pop()));
}

bool CLispEng::Equal(CP p, CP q) {
	if (p == q)
		return true;
	if (Type(p) != Type(q))
		return false;
	switch (Type(p)) {
	case TS_BIGNUM:
	case TS_FLONUM:
	case TS_RATIO:
	case TS_COMPLEX:
		return Eql(p, q);
	case TS_CONS:
		if (p && q) {
			CConsValue *xc = AsCons(p), *yc = AsCons(q);
			return Equal(xc->m_car, yc->m_car) && Equal(xc->m_cdr, yc->m_cdr);
		}
		break;
	case TS_ARRAY:
		{
			CArrayValue *avP = AsArray(p),
				*avQ = AsArray(q);
			if (VectorP(p) && VectorP(q) && avP->GetVectorLength()==avQ->GetVectorLength() &&
				(avP->GetElementType()==ELTYPE_BIT && avQ->GetElementType()==ELTYPE_BIT || (StringP(p) && StringP(q)))) {
				for (size_t i=avP->GetVectorLength(); i--;)
					if (avP->GetElement(i) != avQ->GetElement(i))
						return false;
				return true;
			}
		}
		break;
	case TS_PATHNAME:
		{
			CPathname *x = AsPathname(p), *y = AsPathname(q);
			if (x->LogicalP == y->LogicalP) {


				bool (CLispEng::*pfn)(CP, CP);
	#ifdef _WIN32
				pfn = &CLispEng::EqualP;			// case-insensitive
	#else
				pfn = &CLispEng::Equal;
	#endif
				return (this->*pfn)(x->m_host, y->m_host) &&
							(x->LogicalP || (this->*pfn)(x->m_dev, y->m_dev)) &&		// m_dev not used in Logical-Path
							(this->*pfn)(x->m_dir, y->m_dir) &&
							(this->*pfn)(x->m_name, y->m_name) &&
							(this->*pfn)(x->m_type, y->m_type) &&
							(this->*pfn)(x->m_ver, y->m_ver);
			}
		}
		break;
	}
	return false;
}

void CLispEng::F_Equal() {
	m_r = FromBool(Equal(Pop(), Pop()));
}

bool CLispEng::EqualP(CP x, CP y) {
	if (Equal(x, y))
		return true;
	switch (Type(x)) {
	case TS_CHARACTER:
		if (Type(y) != TS_CHARACTER)
			return false;
		{
			Push(x, y);
			F_CharUpcase();
			CP ux = m_r;
			F_CharUpcase();
			return m_r == ux;
		}
	case TS_CONS:
		if (Type(y)==TS_CONS && x && y) {
			CConsValue *xc = AsCons(x), *yc = AsCons(y);
			return EqualP(xc->m_car, yc->m_car) && EqualP(xc->m_cdr, yc->m_cdr);
		}
		break;
	case TS_BIGNUM:
	case TS_FLONUM:
	case TS_RATIO:
	case TS_COMPLEX:
		return EqualpNum(CoerceToComplex(x), CoerceToComplex(y));
	case TS_ARRAY:
		if (Type(y) != TS_ARRAY)
			return false;
		{
			CArrayValue *xa = AsArray(x), *ya = AsArray(y);
			if (Equal(xa->m_dims, ya->m_dims)) {
				for (size_t size=xa->TotalSize(), i=0; i<size; ++i)
					if (!EqualP(xa->GetElement(i), ya->GetElement(i)))
						return false;
				return true;
			}
		}
		break;
	case TS_OBJECT:
	case TS_STRUCT:
	case TS_CCLOSURE:
		if (Type(x) != Type(y))
			return false;
		{
			CArrayValue *ax = AsArray(x),
				*ay = AsArray(y);
			size_t len = ax->DataLength;
			if (len == ay->DataLength) {
				for (size_t i=0; i<len; ++i)
					if (!EqualP(AsArray(x)->GetElement(i), AsArray(y)->GetElement(i)))
						return false;
				return true;
			}
		}
		break;
	case TS_HASHTABLE:
		if (Type(y) != TS_HASHTABLE)
			return false;
		{
			CHashMap &hx = *AsHashTable(x)->m_pMap,
				     &hy = *AsHashTable(y)->m_pMap;
			if (hx.m_func==hy.m_func && hx.size()==hy.size()) {
				for (CHashMap::iterator i=hx.begin(), e=hx.end(); i!=e; ++i) {
					CHashMap::iterator j = hy.find(i->first);
					if (j == hy.end() || !EqualP(i->second, j->second))
						return false;
				}
				return true;
			}
		}
		break;
	}
	return false;
}

void CLispEng::F_EqualP() {
	m_r = FromBool(EqualP(Pop(), Pop()));
}

void CLispEng::F_SymbolP() {
	CP p = Pop();
	m_r = FromBool(!p || Type(p)==TS_SYMBOL);
}

void CLispEng::F_CharacterP() {
	m_r = FromBool(Type(Pop()) == TS_CHARACTER);
}

void CLispEng::F_ListP() {
	m_r = FromBool(Type(Pop()) == TS_CONS);
}

void CLispEng::F_SpecialOperatorP() {
	m_r = FromBool(Type(ToSymbol(Pop())->GetFun())==TS_SPECIALOPERATOR);
}

void CLispEng::F_ConsP() {
	m_r = FromBool(ConsP(Pop()));
}

void CLispEng::F_Funcall(size_t nArgs) {
	Funcall(m_pStack[nArgs], nArgs);
	SkipStack(1);
}

void CLispEng::F_FunctionNameP() {
	m_r = FromBool(FunnameP(Pop()));
}

CP CLispEng::GetSubrName(CP p) {
	CSymbolValue *sv=(CSymbolValue*)m_symbolMan.m_pBase;
	for (size_t i=0; i<m_symbolMan.m_size; ++i, ++sv)
		if (sv->GetFun() == p)
			return FromSValue(sv);
	return 0;
}

void CLispEng::F_SubrInfo() {
	CP p = Pop();
	if (Type(p) == TS_SYMBOL)
		p = AsSymbol(p)->GetFun();
	if (Type(p) != TS_SUBR)
		return;
	Push(CreateInteger(int(AsIndex(p) & 0x1FF)));
	F_FunTabRef();
	CReqOptRest ror = AsReqOptRest(p);
	m_arVal[1] = CreateInteger(ror.m_nReq);
	m_arVal[2] = CreateInteger(ror.m_nOpt);
	m_arVal[3] = FromBool(WithRestP(p));
	m_arVal[4] = 0;
	m_arVal[5] = 0;
	m_cVal = 6;
	uintptr_t idx = AsIndex(p) & 0x3FF;
	if (idx < SUBR_FUNCS) {
		const CLispFunc& lf = s_stFuncInfo[idx];
		if (lf.m_keywords) {
			pair<size_t, bool> pp = KeywordsLen(lf.m_keywords);
			m_arVal[5] = FromBool(pp.second);
			for (size_t n=pp.first; n--;) {
				m_arVal[4] = Cons(get_Sym(CLispSymbol(lf.m_keywords[n])), m_arVal[4]);
				//!!!R	m_arVal[4] = Cons(get_Sym(CLispSymbol(byte(lf.m_keywords>>(8*n)))), m_arVal[4]);
			}
		}
	}
}

void CLispEng::F_PRplacA() {
	m_r = Pop();
	ToCons(Pop())->m_car = m_r;
}

void CLispEng::F_PRplacD() {
	m_r = Pop();
	ToCons(Pop())->m_cdr = m_r;
}

void CLispEng::F_RplacA() {
	ToCons(m_r=SV1)->m_car = SV;
	SkipStack(2);
}

void CLispEng::F_RplacD() {
	ToCons(m_r=SV1)->m_cdr = SV;
	SkipStack(2);
}

void CLispEng::F_SetfMacroFunction() {
	CP macro = CreateMacro(m_r=Pop(), 0);
	ToSymbol(Pop())->SetFun(macro);
}

void CLispEng::F_Boundp() {
#ifdef _X_DEBUG //!!!D
	cerr << "\n.......F_Boundp()............\n";
	PrintForm(cerr, SV);
	cerr << "\n";
	PrintForm(cerr, ToSymbol(SV)->m_dynValue);
#endif
	m_r = FromBool(ToSymbol(Pop())->m_dynValue != V_U);
}

void CLispEng::F_Makunbound() {
	ToSymbol(m_r = Pop());
	SetSymVal(m_r, V_U);
}

bool __fastcall CLispEng::FunnameP(CP pfun) {
	switch (Type(pfun)) {
	case TS_SYMBOL:
		return true;
	case TS_CONS:
		{
			if (!pfun)
				return true;
			CConsValue *cons = AsCons(pfun);
			CP car = cons->m_car,
				p = cons->m_cdr;
			if (car == S(L_SETF) && Type(p) == TS_CONS && p) {
				SplitPair(p, car);
				return !p && Type(car) == TS_SYMBOL;
			}
		}
	default:
		return false;
	}
}

CP CLispEng::FunnameToSymbol(CP p) {
	if (!FunnameP(p)) {
		Push(S(L_CONS));
		Push(List(S(L_EQL), S(L_SETF)));
		Push(List(S(L_CONS), S(L_SYMBOL), S(L_NULL)));
		E_TypeErr(p, List(S(L_OR), S(L_SYMBOL), Listof(3)));
	}
	return Type(p)==TS_SYMBOL ? p : Get(Car(Cdr(p)), S(L_SETF_FUNCTION));
}

void CLispEng::F_FBoundp() {
	CP p = FunnameToSymbol(Pop());
	m_r = FromBool(Type(p)==TS_SYMBOL && AsSymbol(p)->GetFun());
}

void CLispEng::F_FDefinition() {
	if (!(m_r=ToSymbol(FunnameToSymbol(SV))->GetFun()))
		E_UndefinedFunction(SV);
	SkipStack(1);
}

void CLispEng::F_SymbolFunction() {
	if (!(m_r = ToSymbol(SV)->GetFun()))
		E_UndefinedFunction(SV);
	SkipStack(1);
}

void CLispEng::F_FMakunbound() {
	CP p = FunnameToSymbol(m_r=Pop());
	if (Type(p) == TS_SYMBOL)
		ToSymbol(p)->SetFun(0);
	//!!!  sv->m_fun &= ~SYMBOL_FLAG_MACRO;
}

void CLispEng::F_SetfFuncName() {
	CSPtr val = Pop();
	ToIntFunc(Pop())->m_name = val;
}

void CLispEng::F_GenSym() {
	CSPtr prefix;
	BigInteger counter = ToBigInteger(Spec(L_S_GENSYM_COUNTER));
	CP x = Pop();
	if (x == V_U) {
		CP n = FromCInteger(counter+1);
		Spec(L_S_GENSYM_COUNTER) = n;
	} else {
		switch (Type(x)) {
		case TS_ARRAY:
			prefix = x;
			break;
		case TS_FIXNUM:
		case TS_BIGNUM:
			if ((counter=ToBigInteger(x)) >= 0)
				break;
		default:
			E_TypeErr(x, List(S(L_OR), S(L_STRING), List(S(L_INTEGER), V_0, S(L_ASTERISK))));
		}
	}
	CSymbolValue *sv;
	S_BASEWORD counterBW;
	if (prefix || !counter.AsBaseWord(counterBW)) {
		sv = CreateSymbol((prefix ? AsTrueString(prefix) : "G")+counter.ToString());
	} else {
#if UCFG_USE_POSIX
		sv = CreateSymbol("G"+Convert::ToString(counterBW));
#else
		wchar_t buf[32] = L"G";
#	if UCFG_64
		_i64tow(counterBW, buf+1, 10);
#	else
		_itow(counterBW, buf+1, 10);
#	endif
		sv = CreateSymbol(buf);
#endif
	}	
	m_r = FromSValue(sv);
}

void CLispEng::F_SymbolName() {  
	CP p = Pop();
	CSymbolValue *sv = ToSymbol(p);
#if UCFG_LISP_SPLIT_SYM
	String name = SymNameByIdx(AsIndex(p));
#else
	String name = sv->m_s;
#endif
	m_r = CreateString(name);
}

void CLispEng::CheckSymbolValue(CP sym) {
	while ((m_r=ToSymbol(sym)->m_dynValue) == V_U) {
		E_UnboundVariableErr(sym, IDS_E_VariableHasNoValue, sym);
		//!!! shoul be continuable
	}
}

CP CLispEng::SymbolValue(CP sym) {
	CSymbolValue *sv = ToSymbol(sym);
	CP val = sv->m_dynValue;
	if (val == V_U)
		E_CellErr(sym);
	return val;
}

void CLispEng::F_SymbolValue() {
	CheckSymbolValue(SV);
	SkipStack(1);
	/*!!!R
	if (Type(m_r) == TS_SYMBOLMACRO) {
		m_cVal = 1;
		m_r = Eval(ToSymbolMacro(m_r)->m_macro);
	}
	*/
}

void CLispEng::F_Set() {
	m_r = Pop();
	CP sym = Pop();
	ToVariableSymbol(sym);
	SetSymVal(sym, m_r);
}

void CLispEng::F_Proclaim() {
	CP p = SV, spec;
	if (!SplitPair(p, spec))
		E_Error();
	switch (spec) {
	case S(L_SPECIAL):
		for (CP car; SplitPair(p, car);) {
			CSymbolValue *sv = ToSymbol(car);
			sv->m_fun = (sv->m_fun & ~(SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL)) | SYMBOL_FLAG_SPECIAL;
		}
		break;
	case S(L_NOTSPECIAL):
		for (CP car; SplitPair(p, car);) {
			CSymbolValue *sv = ToSymbol(car);
			sv->m_fun = sv->m_fun & ~SYMBOL_FLAG_SPECIAL;
			if (sv->HomePackage != m_packKeyword)
				sv->m_fun = sv->m_fun & ~SYMBOL_FLAG_CONSTANT;
		}
		break;
	case S(L_INLINE):
	case S(L_NOTINLINE):
		for (CP car; SplitPair(p, car);) {
			Call(S(L_GET_FUNNAME_SYMBOL), car);
			Push(m_r, S(L_INLINABLE), spec);
			F_PPut();
		}
		break;
	}
	SkipStack(1);
}

void CLispEng::F_ProclaimConstant() {
	CP val = Pop();
	CSymbolValue *sv = ToSymbol(m_r=Pop());
	if (sv->SymMacroP)
		E_ProgramErr();
	sv->m_fun |= SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL;
	sv->m_dynValue = val;
}

void CLispEng::F_ProclaimSymbolMacro() {
	CSymbolValue *sv = ToSymbol(m_r=Pop());
	if (sv->SpecialP)
		E_ProgramErr();
	sv->SymMacroP = true;
}

void CLispEng::F_MakeSymbol() {
	m_r = FromSValue(CreateSymbol(AsTrueString(Pop())));
}

/*!!!void CLispEng::F_MakeString()
{
m_r = CreateString(String(char(' '), AsNumber(Pop())));
}*/

String CLispEng::AsTrueString(CP p) {
	if (!StringP(p))
		E_TypeErr(p, S(L_STRING));
	CArrayValue *av = AsArray(p);
	size_t size = av->GetVectorLength();
	String r(' ', size);
	for (size_t i=0; i<size; ++i)
		r.SetAt(i, (String::value_type)AsChar(av->GetElement(i)));
	return r;
}

String AsString(CP p) {
	return Type(p)==TS_SYMBOL ? 
#if UCFG_LISP_SPLIT_SYM
		Lisp().SymNameByIdx(AsIndex(p))
#else
		Lisp().AsSymbol(p)->m_s
#endif
		: Lisp().AsTrueString(p);
}

String CLispEng::FromStringDesignator(CP p) {
	switch (Type(p)) {
	case TS_CONS:
		if (p)
			goto LAB_ERR;
	case TS_SYMBOL:
		return AsSymbol(p)->m_s;
	case TS_CHARACTER:
		return String((String::value_type)AsChar(p));
	case TS_ARRAY:
		if (StringP(p))
			return AsTrueString(p);
	default:
LAB_ERR:
		E_TypeErr(SV, List(S(L_OR), S(L_STRING), S(L_SYMBOL), S(L_CHARACTER)));
	}
}

void CLispEng::F_String() {
	if (!StringP(m_r = Pop()))
		m_r = CreateString(FromStringDesignator(m_r));
}

void CLispEng::F_StringUpcase() {
	Push(SV2);
	F_String();
	pair<size_t, size_t> pp = PopStringBoundingIndex(m_r);
	String s = AsTrueString(m_r).substr(pp.first, pp.second-pp.first);
	m_r = CreateString(s.ToUpper());
	SkipStack(1);
}

void CLispEng::F_StringDowncase() {
	Push(SV2);
	F_String();
	pair<size_t, size_t> pp = PopStringBoundingIndex(m_r);
	String s = AsTrueString(m_r).substr(pp.first, pp.second-pp.first);
	m_r = CreateString(s.ToLower());
	SkipStack(1);
}

/*!!!D
void CLispEng::F_StringEqual() {
m_r = FromBool(AsString(Pop()) == AsString(Pop())); //!!!
}*/

void CLispEng::F_Values(size_t nArgs) {
	StackToMv(nArgs);
}

void CLispEng::F_ValuesList() {
	int count = 0;
	for (CP list=Pop(), p=list, car; ; Push(car), ++count) {
		if (Type(p) != TS_CONS)
			E_TypeErr(list, S(L_LIST));  //!!! should be Proper-list
		if (!SplitPair(p, car))
			break;
	}
	StackToMv(count);
}

void CLispEng::F_Apply(size_t nArgs) {
	CP args = Pop();
	Apply(m_pStack[nArgs], nArgs, args);
	SkipStack(1);
}

/*!!!R
void CLispEng::F_StringConcat(size_t nArgs) {
	String s;
	while (nArgs--)
		s = AsString(Pop())+s;
	m_r = CreateString(s);
}*/

void CLispEng::F_Eval() {
#ifdef _DEBUG
	//!!!PrintForm(cerr, SV);
#endif
	Eval5Env(SV, CEnvironment());
	SkipStack(1);
}

void CLispEng::F_Evalhook() {
	CEnvironment env = m_env;
	CP p = Pop();
	if (p != V_U) {
		CArrayValue *av = ToArray(p);
		if (av->GetVectorLength() != 5)
			E_SeriousCondition();
		memcpy(&env, av->m_pData, 5*sizeof(CP));
	}
	CP ahook = Pop(),
		ehook = Pop(),
		form = Pop();
#ifdef _X_DEBUG
	cerr << "\n-----------Evalhook----------\n";
	PrintForm(cerr, form); //!!!D
#endif
	CDynBindFrame bind;
	bind.Bind(_self, S(L_S_EVALHOOK), ehook);
	bind.Bind(_self, S(L_S_APPLYHOOK), ahook);
	bind.Finish(_self, 2);
	Eval5Env(form, env);
}

void CLispEng::F_SymbolPackage() {
	m_r = ToSymbol(Pop())->HomePackage;
}

void CLispEng::F_ConstantP() {
	Pop(); // Env not used
	CP p = Pop();
	switch (Type(p)) {
	case TS_SYMBOL:
		m_r = FromBool(!(~ToSymbol(p)->m_fun & (SYMBOL_FLAG_SPECIAL|SYMBOL_FLAG_CONSTANT)));
		break;  
	case TS_CONS:
		if (p && Car(p)!=S(L_QUOTE))
			break;
	default:
		m_r = V_T;
	}
}


/*!!!R
void CLispEng::F_PutdClosure()
{
CSymbolValue *sv = ToSymbol(SV2);
size_t len = ToVector(SV)->GetVectorLength()+CCV_START_NONKEY;
CArrayValue *codevec = CreateVector(len, ELTYPE_BYTE);
byte *pb = (byte*)codevec->m_pData+CCV_START_NONKEY;
Push(FromSValue(codevec));
CClosure *c = CreateClosure(0);
c->CodeVec = Pop();
c->NameOrClassVersion = SV2;
c->NumReq = (WORD)AsNumber(SV1);
memcpy(pb, ToVector(SV)->m_pData, ToVector(SV)->GetVectorLength());
sv->SetFun(FromSValueT(c, TS_CCLOSURE));
SkipStack(3);
}
*/

void CLispEng::F_Putd() {
	CSymbolValue *sv = ToSymbol(SV1);
	//!!!  sv->m_fun &= ~SYMBOL_FLAG_MACRO;
	sv->SetFun(m_r=Pop());
	SkipStack(1);
}

void CLispEng::F_GetClosure() {
	m_r = GetClosure(SV2, SV1, SV, m_env); //!!! env
	SkipStack(3);
}

void CLispEng::F_MakeMacro() {
	m_r = CreateMacro(SV1, SV);
	SkipStack(2);
}

/*!!!void CLispEng::F_MakeReadtable()
{
m_r = FromSValue(CreateReadTable());
}*/


/*!!!R
void CLispEng::F_ConcS(size_t nArgs) {
	String s;
	while (nArgs--)
		s = AsString(Pop())+s;
	m_r = CreateString(s);
}*/

CSPtr CLispEng::ReadClosure(CP stm, int n) {
	Throw(E_FAIL);
	/*!!!
	CSPtr car;
	if (n == -1)
	{
	Push(ReadSValue(stm));
	CClosure *pClosure = CreateClosure();
	CSPtr p = Pop();
	Push(FromSValue(pClosure));
	SplitPair(p, pClosure->m_name);
	SplitPair(p, car);		
	CArrayValue *bv = ToArray(car);
	pClosure->m_code = new byte[pClosure->m_codeSize=AsNumber(Car(bv->m_dims))/8];
	int i;
	for (i=0; i<AsNumber(Car(bv->m_dims))/8; i++)
	pClosure->m_code[i] = *((byte*)bv->m_pData+i);
	CArrayValue *vv = CreateVector(Length(p));
	CSPtr closure = Pop();
	AsClosure(closure)->m_consts = FromSValue(vv);
	for (i=0; i<vv->TotalSize(); i++)
	{
	SplitPair(p, car);
	vv->m_pData[i] = car;
	}
	return closure;
	}
	else if (n > 0)
	{
	CChangeRadix changeRadix(16);
	Push(ReadSValue(stm));
	CArrayValue *av = CreateArray();
	av->m_elType = ELTYPE_BIT;
	Push(FromSValue(av)); //!!!
	av->m_dims = Cons(CreateInteger(n*8), 0);
	CP r = Pop();
	av->m_pData = av->CreateData(ELTYPE_BIT, av->m_dims, 0);
	CSPtr p = Pop();
	for (int i=0; i<n; i++)
	{
	SplitPair(p, car);
	*((byte*)av->m_pData+i) = (byte)AsNumber(car);
	}
	return r;
	}
	else
	{
	ReadSValue(stm);
	return 0;
	}*/
}

/*!!!R
void CLispEng::F_ReadClosure() {	//!!!D
	CSPtr p = Pop();
	int n = p ? AsNumber(p) : -1;
	SkipStack(1);
	m_r = ReadClosure(Pop(), n);
	m_cVal = 1;
}*/

/*!!!R
void CLispEng::F_RevListToString() {
	size_t size = Length(SV);
	CArrayValue *av = CreateVector(size, ELTYPE_BASECHAR);
	for (CP p=Pop(), car; size--;) {
		SplitPair(p, car);
		*((char*)av->m_pData+size) = (char)AsChar(car); //!!! 
	}
	m_r = FromSValue(av);
}*/

/*!!!R
void CLispEng::F_ChainsEx() {
	CP *pCase = m_pStack,
		*pStr = m_pStack+2,
		end = GetStack(1),
		x = GetStack(2),
		car;  
	int n = 0;
	for (; x!=end; n++) {
		SplitPair(x, car);
		Push(Car(car));
	}
	*pStr = CreateString(String(char(' '), n));
	CP cas = *pCase;
	CArrayValue *av = ToArray(*pStr);
	while (n--) {
		if (cas == S(L_K_UPCASE))
			F_CharUpcase();
		else if (cas == S(L_K_DOWNCASE))
			F_CharDowncase();
		else if (cas == S(L_K_PRESERVE))
			m_r = Pop();
		else if (cas == S(L_K_INVERT)) {
			Push(GetStack(0));
			F_UpperCaseP();
			if (m_r)
				F_CharDowncase();
			else
				F_CharUpcase();
		} else
			E_Error();
		av->SetElement(n, m_r);
	}
	SkipStack(2);
	m_r = Pop();
}
*/

path CLispEng::FindInLISPINC(const path& filename, const path& defaultDir) {
	vector<String> ar;
#if !UCFG_WCE
	String lispinc = Environment::GetEnvironmentVariable("LISPINC");
	if (!!lispinc)
		ar = lispinc.Split(String(Path::PathSeparator));
#endif
	if (m_bVarsInited) {
		for (CP lp=AsSymbol(GetSymbol("*LOAD-PATHS*", m_packCustom))->m_dynValue, car; SplitPair(lp, car);)
			ar.push_back(AsPathname(car)->ToString(true));
	} else
		ar.insert(ar.end(), LoadPaths.begin(), LoadPaths.end());

	ar.insert(ar.begin(), defaultDir);
	path r;
	for (size_t i=0; i<ar.size(); ++i)
		if (exists(r = ar[i] / filename))
			return r;
	return path();
}

path CLispEng::SearchFile(const path& name) {
	vector<path> ar;
	path filename = name;
	if (!name.has_extension())
		filename += ".lisp";
	path s = FindInLISPINC(filename, AsPathname(Spec(L_S_DEFAULT_PATHNAME_DEFAULTS))->ToString(true));
	if (!s.empty())
		ar.push_back(s);
	filename = name;
	if (!name.has_extension())
		filename += ".fas";
	s = FindInLISPINC(filename, AsPathname(Spec(L_S_DEFAULT_PATHNAME_DEFAULTS))->ToString(true));
	if (!s.empty())
		ar.push_back(s);
	if (ar.size()==2 && last_write_time(ar[1]) > last_write_time(ar[0]))
		return ar[1];
	else if (ar.size() > 0)
		return ar[0];
	else
		return path();
}

void CLispEng::F_FindInLispenv() {
	F_Pathname();
	path p = ToPathname(m_r)->ToString(); //!!!
	path npath = SearchFile(p);
	if (npath.empty())
		E_Error();
	m_r = CreatePathname(npath);
}

void CLispEng::F_Load() {
	String name = AsString(Pop());
	path p = SearchFile(name);

	TRC(1, "Found file: " << p)

	if (p.empty())
		E_Error();
	/*!!!	if (!Path::HasExtension(name))
	name += ".lisp";
	String path = name;
	if (!Path::IsPathRooted(name) &&
	(path=FindInLISPINC(name, AsPathname(Spec(L_S_DEFAULT_PATHNAME_DEFAULTS))->ToString(true))) == "")
	E_Error();*/
	if (m_bInit)
		m_arModule.push_back(name);
	if (SymbolValue(GetSymbol("*LOAD-VERBOSE*", m_packCL)))
		cout << "Loading " << p << " ..." << endl;
//!!!R	CDynBindFrame bindDef(S(L_S_DEFAULT_PATHNAME_DEFAULTS), CreatePathname(AddDirSeparator(Path::GetDirectoryName(path))), true); //!!!

	Push(CreatePathname(p));
	Push(S(L_K_INPUT), S(L_CHARACTER), 0);
	Push(S(L_K_ERROR), S(L_K_DEFAULT));
	F_Open();

	Push(m_r, 0);
	LoadFromStream(m_r);
	F_Close();
}

void CLispEng::F_Length() {
	CP p = Pop();
	switch (Type(p)) {
	case TS_CONS:
		m_r = CreateInteger(Length(p));
		break;
	case TS_ARRAY:
		m_r = CreateInteger(int(ToVector(p)->GetVectorLength()));
		break;
	default:
		E_TypeErr(p, S(L_SEQUENCE));
	}
}

void CLispEng::F_PrintFloat() {
	char buf[30];
	sprintf(buf, "%1.8g", AsFloatVal(SV1));
	if (!strchr(buf, '.'))
		strcat(buf, ".0");
	CStm sk(SV);
	for (const char *p = buf; *p; p++)
		WriteChar(*p);
	SkipStack(1);
	m_r = Pop();
}

bool CLispEng::RtCaseDifferent(CP rtCase, CP pch) {
	Push(pch);
	F_BothCaseP();
	if (m_r) {
		if (rtCase == S(L_K_UPCASE)) {
			Push(pch);
			F_LowerCaseP();
		} else if (rtCase == S(L_K_DOWNCASE)) {
			Push(pch);
			F_UpperCaseP();
		}
		else
			m_r = 0;
	}
	return m_r;
}

void CLispEng::F_PrintSymbolName() {
	if (!StringP(SV1))
		E_TypeErr(SV1, S(L_STRING));
	vector<wchar_t> v;
	CArrayValue *av = AsArray(SV1);
	bool bReadable = Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_ESCAPE);
	Push(Spec(L_S_READTABLE));
	F_ReadtableCase();
	CP rtCase = m_r;
	bool bSingleCase = true;
	CP prevCase = 0;
	size_t len = av->GetVectorLength();
	CTokenVec vec;
	size_t i;
	for (i=0; i<len; ++i) {
		CP pch = av->GetElement(i);
		wchar_t ch = AsChar(pch);
		CCharType ct = GetCharType(ch);
		vec.push_back(CCharWithAttrs(ch, true, ct.m_traits));
	}
	bool bMescape = OnlyDotsOrEmpty(vec) || PotentialNumberP(vec, Spec(L_S_PRINT_BASE));
	for (i=0; i<len; ++i) {
		CP pch = av->GetElement(i);
		if (bSingleCase && rtCase==S(L_K_INVERT)) {
			Push(pch);
			F_BothCaseP();
			if (m_r) {
				Push(pch);
				F_UpperCaseP();
				CP newCase = m_r ? S(L_K_UPCASE) : S(L_K_DOWNCASE);
				if (!prevCase)
					prevCase = newCase;
				else
					bSingleCase = prevCase==newCase;
			}
		}
		Push(pch, V_U);
		F_CharType();
		if (m_r!=V_CONSTITUENT || RtCaseDifferent(rtCase, pch)) {
			bMescape = true;
			break;
		}
	}
	if (bMescape)
		v.push_back('|');
	bool bPrevAlphanumeric = false;
	CP prCase = Spec(L_S_PRINT_CASE);
	for (i=0; i<len; ++i) {
		CP pch = av->GetElement(i);
		if (bMescape) {
			wchar_t ch = AsChar(pch);
			switch (ch) {
			case '\\':
			case '|':
				v.push_back('\\');
			}
			v.push_back(ch);
		} else {
			if (rtCase != S(L_K_PRESERVE))
				;
			else if (rtCase == S(L_K_INVERT)) {
				if (bSingleCase && prevCase) {
					Push(pch);
					if (prevCase == S(L_K_UPCASE))
						F_CharUpcase();
					else
						F_CharDowncase();
					pch = m_r;
				}
			} else {
				if (prCase == S(L_K_CAPITALIZE)) {
					if (!bPrevAlphanumeric) {
						Push(pch);
						F_CharUpcase();
						pch = m_r;
					}
					Push(pch, V_U);
					F_AlphanumericP();
					bPrevAlphanumeric = m_r;
				} else if (!RtCaseDifferent(rtCase, pch)) {
					Push(pch);
					if (prCase == S(L_K_UPCASE))
						F_CharUpcase();
					else
						F_CharDowncase();
					pch = m_r;
				}
			}
			v.push_back(AsChar(pch));
		}
	}
	if (bMescape)
		v.push_back('|');
	CStm sk(SV);
	for (i=0; i<v.size(); ++i)
		WriteChar(v[i]);
	SkipStack(2);
}

#ifdef _DEBUG//!!!D
bool g_print;
#endif

void CLispEng::F_Pr(size_t nArgs) {
	cerr << "_Pr ";
	for (int i=0; i<(int)nArgs; ++i) {
		CP p = m_pStack[nArgs-i-1];
		m_r = p;		//	result is last arg
		if (Type(p) == TS_ARRAY && !StringP(p)) {
			CArrayValue *av = AsArray(p);
			av = av;
		}
		Print(p);
		cerr << " ";
		if (p == CreateFixnum(255)) {
			Disassemble(cerr, CurClosure);
		}
	}
	cerr << endl;
	SkipStack(nArgs);
#ifdef X_DEBUG //!!!D	
	static int s_i = 0;
	++s_i;
	cerr << " " << s_i << " " << endl;
	if (s_i >= 593) {
		++g_b;
	}
#endif
	}

void CLispEng::F_Disassemble() {
	Disassemble(cerr, SV);
}

static int m_nObfuscate;

void CLispEng::F_Obfuscate() {
	m_mapObfuscated[Pop()] = "obf_"+Convert::ToString(++m_nObfuscate);
}

void CLispEng::F_Last() {
	CP num = Pop();
	m_r = Pop();
	CP q = m_r;
	for (size_t count=num!=V_U ? AsPositive(num) : 1; count--;) {
		switch (Type(q)) {
		case TS_CONS:
			if (q) {
				q = Cdr(q);
				continue;
			}
		default:
			return;
		}
	}
	while (q && Type(q) == TS_CONS) {
		q = Cdr(q);
		m_r = Cdr(m_r);
	}
}

void CLispEng::F_Endp() {
	switch (Type(m_r=Pop())) {
	case TS_CONS:
		m_r = FromBool(!m_r);
		break;
	default:
		E_TypeErr(m_r, S(L_LIST));
	}
}

void CLispEng::F_Identity() {
	m_r = Pop();
}

void CLispEng::F_Exit() {
#ifdef X_DEBUG//!!!D
	E_Error();
#endif
	CP r = ToOptional(Pop(), V_0);	
	if (IntegerP(r)) {
		BigInteger bi = ToBigInteger(r);
		int64_t n;
		if (bi.AsInt64(n))
			m_exitCode = (uint32_t)n;
		else
			m_exitCode = 1;
	} else
		m_exitCode = 1;
	Environment::ExitCode = m_exitCode;
	ShowInfo();
	if (m_pSink)
		m_pSink->OnExit();
	Throw(ExtErr::NormalExit); //!!!
}

void CLispEng::F_SaveMem() {
	class CMemImageStream : public Stream {
	public:
		StandardStream *m_ts;

		void WriteBuffer(const void *buf, size_t count) {
			for (byte *pb=(byte*)buf; count--;)
				m_ts->WriteByte(*pb++);
		}
	} stm;
	stm.m_ts = GetOutputTextStream(ToStream(SV));
	SaveMem(stm);
	SkipStack(1);
}

void CLispEng::F_RenameFile() {
	Push(SV);
	F_Pathname();
	SV = m_r;

	Push(SV1);
	F_Pathname();
	SV1 = m_r;

	Call(S(L_MERGE_PATHNAMES), SV, SV1);
	path defNewName = AsString(m_r),
		from = AsString(SV1);
	error_code ec;
	sys::rename(from, defNewName, ec);
	if (ec)
		E_FileErr(m_r);

	Push(m_r);		// defNewName

	Push(CreateString(from));
	F_Pathname();
	SV2 = m_r;

	Push(SV1);		//!!!?
	F_Pathname();
	m_arVal[2] = m_r;
	m_arVal[1] = SV2;
	m_r = Pop();	
	m_cVal = 3;
	SkipStack(2);
}


/*!!!D
void CLispEng::F_FloatTokenP()
{
CP p = Pop();
if (!p)
return;
ostringstream os;
CP car = Car(p);
char ch;
uint32_t traits;
if (AsNumber(Cdr(car)) & TRAIT_SIGN)
{
os << (char)AsChar(Car(car)); //!!!
Inc(p);
}
while (true)
{
if (!p)
return;
car = Car(p);
ch = (char)AsChar(Car(car));//!!!
traits = AsNumber(Cdr(car));
if (!(traits & TRAIT_ALPHADIGIT) || !isdigit(ch))
break;
os << ch;
Inc(p);
}
if (traits & TRAIT_POINT)
{
os << ch;
Inc(p);
while (true)
{
if (!p)
goto out;
car = Car(p);
ch = (char)AsChar(Car(car));//!!!
traits = AsNumber(Cdr(car));
if (!(traits & TRAIT_ALPHADIGIT) || !isdigit(ch))
break;
os << ch;
Inc(p);
}
}
if (!(traits & TRAIT_EXPONENT_MARKER))
return;
os << 'E';
Inc(p);
if (!p)
return;
if (traits & TRAIT_SIGN)
{
os << ch;
Inc(p);
if (!p)
return;
}
while (p)
{
car = Car(p);
ch = (char)AsChar(Car(car));//!!!
traits = AsNumber(Cdr(car));
if (!(traits & TRAIT_ALPHADIGIT) || !isdigit(ch))
return;
os << ch;
Inc(p);
}
out:
m_r = FromFloat(atof(os.str()));
}
*/

/*!!!R
void CLispEng::F_Empty() {
	E_Error();
}

void CLispEng::F_EmptyR(size_t nArgs) {
	E_Error();
}*/

} // Lisp::

