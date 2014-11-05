#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_If() {
	LISP_TAIL_REC_DISABLE_1;

	CP form = Eval(SV2) ? SV1 : SV;
	SkipStack(3);
	if (form == V_U) {
		m_r = 0;
		m_cVal = 1; //!!! ClearResult();
	} else {
		LISP_TAIL_REC_RESTORE;

		m_cVal = 1;
		m_r = Eval(form);
	}
}

bool CLispEng::CheckSetqBody(CP p) {
	for (CP body=SV, n; SplitPair(body, n); Inc(body)) {
		if (SymMacroP(n)) {
			CP form = Cons(p, Pop());
			m_cVal = 1;
			m_r = Eval(form);
			return true;
		}
	}
	return false;
}

void CLispEng::F_Setq() {
	LISP_TAIL_REC_DISABLE_1;

	if (CheckSetqBody(S(L_SETF)))
		return;
	for (CP p=SV, sym, form; SplitPair(p, sym);)
		if (SplitPair(p, form))
			Setq(sym, m_r = Eval(form));
		else
			E_ProgramErr();
	SkipStack(1);
	m_cVal = 1;  
}

void CLispEng::F_Quote() {
	m_r = Pop();
}

void __fastcall CLispEng::Prog() {
	LISP_TAIL_REC_DISABLE_1;

	for (CP car; SplitPair(SV, car);) {
#if UCFG_LISP_TAIL_REC
		if (!SV) {
			LISP_TAIL_REC_RESTORE;
		}
#endif
		m_cVal = 1;
		m_r = Eval(car);
	}
	SkipStack(1);
}

void CLispEng::F_Progn() {
	Progn(Pop());
}

void __fastcall CLispEng::PrognNoRec(CP p) {
	if (p) {
		Push(p);
		ProgNoRec();
	} else
		ClearResult();
}

void CLispEng::F_Block() {
	CP body = Pop(),
		name = Pop();
	ToSymbol(name);

#ifdef X_DEBUG//!!!D
	if (g_b) {
		cerr << "(BLOCK ";
		Print(name);
		cerr << "  " << m_pSP << endl;
	}
#endif

	CIBlockFrame blockFrame(_self, name);
	CEnv1BFrame b1Frame(_self);
	m_env.m_blockEnv = CreateFramePtr(blockFrame.GetStackP(_self));

	LISP_TRY(blockFrame) {
		Progn(body);
	} LISP_CATCH(blockFrame) {
		m_pSP = m_pSPTop-blockFrame.m_spOff;
	} LISP_CATCH_END

#ifdef X_DEBUG//!!!D
	if (g_b) {
		cerr << "(AFTER BLOCK ";
		Print(name);
		cerr << "  " << m_pSP << endl;
	}
#endif
}

void CLispEng::UnwindTo(CP *frame) {
	for (CJmpBufBase *p = m_pJmpBuf; p; p=p->m_pNext) {
		if (p->GetStackP(_self) == frame) {
			m_bUnwinding = true;
			LISP_THROW(p);
		}
	}
	E_Error();
	//!!!    throw HRESULT(HR_UNWIND | (m_pStackTop-frame));
}

void CLispEng::F_ReturnFrom() {
	LISP_TAIL_REC_DISABLE_1;

	CP name = SV1,
		env = m_env.m_blockEnv;

	ToSymbol(name);
	CP *frame,
		car;

#ifdef X_DEBUG//!!!D
	if (g_b) {
		cerr << "(Return-From ";
		Print(name);
		cerr << endl;
	}
#endif

	while (Type(env) == TS_FRAME_PTR) {
		frame = AsFrame(env);
		env = frame[FRAME_NEXT_ENV];
		if (IsFrameNested(frame[0]))
			break;
		if (frame[FRAME_NAME] == name)
			goto LAB_FOUND;
	}
	while (SplitPair(env, car)) {
		if (Car(car) == name)
			if ((env = Cdr(car)) == V_D)
				E_ControlErr(IDS_E_BlockHasLeft, name);
			else {
				frame = ToFrame(env); //!!!
				goto LAB_FOUND;
			}
	}
	E_ProgramErr(IDS_E_NoSuchBlock, name);
LAB_FOUND:
	CP p = Pop();
	SkipStack(1);
	if (p != V_U) {
		m_cVal = 1;
		m_r = Eval(p);
	}
	UnwindTo(frame); 
}

void CLispEng::F_Function() {
	CP name = SV,
		funname = SV1;
	if (name != V_U) {
		name = SV1;
		if (!FunnameP(name))
			E_Error();
		funname = SV;
	} else {
		if (FunnameP(funname)) {
			m_r = GetSymFunction(funname, m_env.m_funEnv);
			switch (Type(m_r)) {
			case TS_FUNCTION_MACRO:
				m_r = ToFunctionMacro(m_r)->m_function; //!!!O
			case TS_SUBR:
			case TS_INTFUNC:
			case TS_CCLOSURE:
				break;
			default:
				Print(funname);
				E_Error();
			}
			SkipStack(2);
			return;
		} else
			name = S(L_K_LAMBDA);
	}
	if (!(Type(funname) == TS_CONS && Car(funname) == S(L_LAMBDA)))
		E_Error();
#ifdef _DEBUG
	if (Cdr(funname) == 0x0036c700)
		Print(Cdr(funname));
#endif 
	m_r = GetClosure(Cdr(funname), name, false, m_env);//!!!
	m_cVal = 1;
	SkipStack(2);
}

CP CLispEng::SkipDeclarations(CP body) {
	for (; Type(body)==TS_CONS && body; Inc(body)) {
		CP car = Car(body);
		if (Type(car) != TS_CONS || Car(car) != S(L_DECLARE))
			break;
	}
	return body;
}

void CLispEng::FinishFLet(CP *pTop, CP body) {
	{  
		CFunFrame funFrame(pTop);
		CEnv1FFrame f1Frame;
		m_env.m_funEnv = CreateFramePtr(funFrame.GetStackP(_self));
		PrognNoRec(SkipDeclarations(body));
	}
	SkipStack(2);
}

void CLispEng::F_Flet() {
	CP *pTop = m_pStack;
	for (CP car; SplitPair(pTop[1], car);) {
		ToCons(car);
		CP name = Car(car),
			lambdaBody = Cdr(car);
		if (!FunnameP(name))
			E_Error();
		ToCons(lambdaBody);
		Push(0, name);
		SV1 = GetClosure(lambdaBody, name, true, m_env);
	}
	FinishFLet(pTop, pTop[0]);
}

void CLispEng::F_FunctionMacroFunction() {
	m_r = ToFunctionMacro(Pop())->m_function;
}

void CLispEng::F_FunctionMacroExpander() {
	m_r = ToFunctionMacro(Pop())->m_expander;
}

void CLispEng::F_MacroExpander() {
	m_r = ToMacro(Pop())->m_expander;
}

void CLispEng::F_MacroLambdaList() {
	m_r = ToMacro(Pop())->m_lambdaList;
}

void CLispEng::F_MakeFunctionMacro() {
	m_r = FromSValueT(CreateFunctionMacro(SV1, SV), TS_FUNCTION_MACRO);
	SkipStack(2);
}

void CLispEng::F_FunctionMacroLet() {
	LISP_TAIL_REC_DISABLE_1;

	CP *pTop = m_pStack;
	for (CP p=SV1, car; SplitPair(p, car);) {
		CP name, fun, macro;
		if (!SplitPair(car, name) || !SplitPair(car, fun) || !SplitPair(car, macro))
			E_Error();
		ToSymbol(name);
		ToCons(fun);
		ToCons(macro);
		Push(GetClosure(fun, name, false, m_env));
		Call(S(L_MAKE_FUNMACRO_EXPANDER), name, macro);
		Push(m_r);
		F_MacroExpander();
		Push(m_r);
		F_MakeFunctionMacro();
		Push(m_r, name);
	}
	FinishFLet(pTop, pTop[0]);
}

void CLispEng::CallBindedFormsEx(FPBindVar pfn, CP forms, CP *pBind, int nBinds) {
	LISP_TAIL_REC_DISABLE_1;

	Push(forms);
	int i;
	for (i=0; i<nBinds; i++) {
		CP *p = pBind-(1+i)*2;
		CP val = (this->*pfn)(i, p[0], p[1]);
#ifdef X_DEBUG //!!!D
		if (val == V_U)
			val = val;
#endif
		p[1] = SwapIfDynSymVal(p[0], val);
	}
	for (i=0; i<nBinds; i++)
		pBind[-(1+i)*2] |= FLAG_ACTIVE;

#if UCFG_LISP_TAIL_REC == 1
	LISP_TAIL_REC_RESTORE
#endif
	Progn(Pop());
}

CP CLispEng::AugmentDeclEnv(CP declSpec, CP env) {
	CP typ = Car(declSpec);
	if (Type(typ) == TS_SYMBOL)
		for (CP p=env, spec; SplitPair(p, spec);)
			if (Car(spec) == S(L_DECLARATION))
				for (CP list=Cdr(spec), car; SplitPair(list, car);)
					if (car == typ)
						return Cons(declSpec, env);
	return env;
}

void CLispEng::CallBindedForms(FPBindVar pfn, CP caller, CP varSpecs, CP declars, CP forms) {
	CVarFrame varFrame(m_pStack);
	CP *pBind;
	int nBinds;
	CP *pSpec = m_pStack;
	int nSpec = 0,
		nVar = 0;
	CP spec;
	for (CP declSpecs=declars; SplitPair(declSpecs, spec);) {
		if (Type(spec) == TS_CONS && Car(spec) == S(L_SPECIAL)) {
			for (Inc(spec); spec; ++nSpec) {
				CP sym = 0;
				SplitPair(spec, sym);
				if (Type(sym) != TS_SYMBOL)
					E_Error();
				Push(V_SPEC, sym | FLAG_ACTIVE);
			}
		}
	}
	pBind = m_pStack;
	for (; SplitPair(varSpecs, spec); nVar++) {
		CP sym = 0,
			init = 0;
		if (Type(spec) == TS_SYMBOL && caller != S(L_SYMBOL_MACROLET)) {
			sym = spec;
			init = V_U;
		} else if (ConsP(spec)) {			//!!!
			SplitPair(spec, sym);
			if (Type(sym) == TS_SYMBOL)
				SplitPair(spec, init);  //!!! Check more conditions
			else
				E_Error();
		} else
			E_Error();
		Push(init, sym);
		CP *p = pSpec;
		CP toCompare = sym | FLAG_ACTIVE;
		bool bSpecDecl = false;
		for (int i=0; i<nSpec; i++) {
			--p;
			if (*--p == toCompare) {
				bSpecDecl = true;
				break;
			}
		}
		if (caller == S(L_SYMBOL_MACROLET)) {
			if (bSpecDecl || (ToSymbol(sym)->m_fun & SYMBOL_FLAG_SPECIAL)) 
				E_Error();
		}
		else if ((ToVariableSymbol(sym)->m_fun & SYMBOL_FLAG_SPECIAL) || bSpecDecl) //!!!? FLAG_CONSTANT
			SV |= FLAG_DYNAMIC;
	}
	nBinds = nVar;
	nVar += nSpec;
	Push(m_env.m_varEnv, CreateFixnum(nVar));
	varFrame.Finish(_self, pSpec);
	varFrame.m_pVars -= nBinds*2;//!!!
	CP pVarFrame = CreateFramePtr(m_pStack);
	Push(forms, declars);
	CP denv = m_env.m_declEnv;
	for (; SplitPair(SV, spec);)
		if (ConsP(spec) && Car(spec) != S(L_SPECIAL)) //!!!
			denv = AugmentDeclEnv(spec, denv);
	SkipStack(2);
	if (denv == m_env.m_declEnv) {
		CEnv1VFrame vFrame;
		m_env.m_varEnv = pVarFrame;
		CallBindedFormsEx(pfn, forms, pBind, nBinds);
	} else {
		CEnv2VDFrame vdFrame;
		m_env.m_declEnv = denv;
		m_env.m_varEnv = pVarFrame;
		CallBindedFormsEx(pfn, forms, pBind, nBinds);
	}
}

CP CLispEng::BindLet(size_t n, CP& sym, CP form) {
	return form==V_U ? 0 : Eval(form);
}

CP CLispEng::BindLetA(size_t n, CP& sym, CP form) {
	CP r = form==V_U ? 0 : Eval(form);
	sym |= FLAG_ACTIVE;
	return r;
}

CEnvironment *CLispEng::NestEnv(CEnvironment& penv) {
	CEnvironment *env5 = (CEnvironment*)(m_pStack -= 5);
	*env5 = penv; //!!!  was m_env
	CP env = env5->m_goEnv;
	CP *fr;
	int depth = 0;
	while (Type(env) == TS_FRAME_PTR) {
		fr = AsFrame(env);
		if (!IsFrameNested(fr[0])) {
			Push(env);
			//!!!D      env = fr[FRAME_TAG_NEXT_ENV];
			depth++;
		}
		env = fr[FRAME_TAG_NEXT_ENV];
	}
	while (depth--) {
		CP frame = SV;
		fr = AsFrame(frame);
		SV = env;
		CP *tags = fr+FRAME_TAG_BINDINGS,
			*top = AsFrameTop(fr);
		ssize_t count = (top-tags)/2;
		CArrayValue *vv = CreateVector(count);
		for (int i=0; i<count; i++)
			vv->m_pData[i] = tags[i*2];
		fr[FRAME_TAG_NEXT_ENV] = env = Cons(Cons(FromSValue(vv), frame), SV);
		SkipStack(1);
		fr[0] = fr[0] | (FLAG_NESTED << VALUE_SHIFT);
	}
	env5->m_goEnv = env;
	env = env5->m_blockEnv;
	depth = 0;
	while (Type(env) == TS_FRAME_PTR) {
		fr = AsFrame(env);
		if (!IsFrameNested(fr[0])) {
			Push(env);
			//!!!D      env = fr[FRAME_NEXT_ENV];
			depth++;
		}
		env = fr[FRAME_NEXT_ENV];
	}
	while (depth--) {
		CP frame = SV;
		fr = AsFrame(frame);
		fr[FRAME_NEXT_ENV] = env = Cons(Cons(fr[FRAME_NAME], frame), env);
		SkipStack(1);
		fr[0] = fr[0] | (FLAG_NESTED << VALUE_SHIFT);
	}
	env5->m_blockEnv = env;
	env5->m_funEnv = NestFun(env5->m_funEnv);
	env5->m_varEnv = NestVar(env5->m_varEnv);
	return env5;
}

void CLispEng::PushNestEnvAsArray(CEnvironment& env) {
	NestEnv(env);
	CArrayValue *av = CreateVector(5);
	memcpy(av->m_pData, m_pStack, 5*sizeof(CP));
	SkipStack(5);
	Push(FromSValue(av));
}

void CLispEng::PushNestEnv(CEnvironment& env) {
	CEnvironment *senv = NestEnv(env);
	swap(senv->m_declEnv, senv->m_varEnv);
	swap(senv->m_goEnv, senv->m_funEnv);
}

bool CLispEng::ParseCompileEvalForm(size_t skip) {
	CP compileName = ParseDD(SV, false);
	if (compileName == V_0)
		return false;
	SkipStack(skip);
	Push(SV1); //!!! (was SV1  - form in EVAL frame)
	PushNestEnv(m_env);
	int nArgs = 6;
	if (compileName != V_U) {
		Push(compileName);
		++nArgs;
	}
	Apply(S(L_COMPILE_FORM), nArgs, 0);
	Call(m_r);
	return true;
}

void CLispEng::GeneralLet(FPBindVar pfn) {
	if (ParseCompileEvalForm(2))
		return;
	SkipStack(1);
	CallBindedForms(pfn, S(L_LET), Pop(), m_arVal[1], m_r);
}

void CLispEng::F_Let() {
	GeneralLet(&CLispEng::BindLet);
}

void CLispEng::F_LetA() {
	GeneralLet(&CLispEng::BindLetA);
}

void CLispEng::F_CompilerLet() {
	CP *pStack = m_pStack;
	size_t count = 0;
	for (CP p=pStack[1], car; SplitPair(p, car); count++) {
		if (ConsP(car)) {
			CP v;
			SplitPair(car, v);
			ToVariableSymbol(v);
			if (!SplitPair(car, v))
				v = 0;
			if (car)
				E_ProgramErr();
			Push(Eval(v));
		} else {
			ToVariableSymbol(car);
			Push(0);
		}
	}
	{
		CDynBindFrame dynBind;
		int i = 0;
		for (CP p=pStack[1], car; SplitPair(p, car); i++)
			dynBind.Bind(_self, ConsP(car) ? Car(car) : car, m_pStack[-i-1]);
		dynBind.Finish(_self, count);
		Progn(pStack[0]);
	}
	m_pStack = pStack+2;
}

void CLispEng::F_Catch() {
	SV1 = Eval(SV1);
	CP body = Pop();
	CCatchFrame catchFrame(_self, Pop());
	LISP_TRY(catchFrame) {
		Progn(body);
	} LISP_CATCH(catchFrame) {
	} LISP_CATCH_END;
#if UCFG_WIN32
	if (StackOverflowAddress) {
		DWORD old;
		Win32Check(::VirtualProtect(StackOverflowAddress, 1, PAGE_READWRITE|PAGE_GUARD, &old));
		StackOverflowAddress = 0;
	}
#endif
}

void CLispEng::ThrowTo(CP tag) {
	const DWORD testVal = (FT_CATCH<<TYPE_BITS)|TS_FRAMEINFO,
		mask = (FRAME_TYPE_MASK<<TYPE_BITS) | 0x1F;
	for (CP *p=m_pStack; p!=m_pStackTop; p++)
		if ((*p & mask)==testVal && p[FRAME_TAG]==tag)
			UnwindTo(p);
	E_ControlErr(IDS_E_NoSuchThrow, tag);
}

void CLispEng::F_EvalWhen() {
	for (CP p=SV1, sit; SplitPair(p, sit);) {
		if (sit==S(L_EVAL) || sit==S(L_K_EXECUTE))
			goto found;
		if (Type(sit)==TS_CONS && Car(sit)==S(L_NOT)) {
			sit = Cdr(sit);
			if (sit==S(L_COMPILE) || sit==S(L_K_COMPILE_TOPLEVEL))
				goto found;
		}
	}
	SkipStack(2);
	return;
found:
	F_Progn(); //!!!
	SkipStack(1);
}

void CLispEng::F_Macrolet() {
	LISP_TAIL_REC_DISABLE_1;

	CP *pTop = m_pStack;
	for (CP p=pTop[1], car; SplitPair(p, car);) {
		CP name = Car(car);
		ToSymbol(name);
		ToCons(Cdr(car));

		Push(car, 0);
		PushNestEnvAsArray(m_env);
		CEnvironment *penv = (CEnvironment*)AsArray(SV)->m_pData;
		penv->m_varEnv = Cons(S(L_MACROLET), penv->m_varEnv);
		penv->m_funEnv = Cons(S(L_MACROLET), penv->m_funEnv);
		Funcall(S(L_MAKE_MACRO_EXPANDER), 3);
		Push(m_r, name);
	}
	FinishFLet(pTop, pTop[0]);
}

void CLispEng::F_Labels() {
	LISP_TAIL_REC_DISABLE_1;

	Push(NestFun(m_env.m_funEnv));
	int veclen = 1;
	CP funspecs = SV2,
		car = 0;
	for (; SplitPair(funspecs, car); veclen+=2) {
		CP name;
		if (!SplitPair(car, name) || !FunnameP(name) || (Type(car) != TS_CONS || !car)) //!!!
			E_Error();
	}
	CArrayValue *vv = CreateVector(veclen);
	CP *p = vv->m_pData;
	funspecs = SV2;
	for (; SplitPair(funspecs, car); p+=2)
		*p = Car(car);
	*p = Pop();
	CP body = Pop();
	funspecs = Pop();
	CEnv1FFrame f1Frame;
	m_env.m_funEnv = FromSValue(vv);
	Push(body, m_env.m_funEnv);
	int index = 1;
	for (; SplitPair(funspecs, car); index+=2) {
		Push(funspecs);
		CP fun = GetClosure(Cdr(car), Car(car), true, m_env);
		funspecs = Pop();
		ToArray(SV)->m_pData[index] = fun;
	}
	SkipStack(1);
	Progn(SkipDeclarations(Pop()));
}

void CLispEng::F_LoadTimeValue() {
	Eval5Env(SV1, CEnvironment());
	SkipStack(2);
	m_cVal = 1;
}

void CLispEng::F_Locally() {
	LISP_TAIL_REC_KEEPER(false);

	if (ParseCompileEvalForm(1))
		return;
	CallBindedForms(0, 0, 0, m_arVal[1], m_r);
	//!!! Check SP
}

void CLispEng::F_MultipleValueCall() {
	LISP_TAIL_REC_DISABLE_1;

	CP fun = SV1 = Eval(SV1);
	size_t count = 0;
	for (CP forms=SV, car; SplitPair(forms, car);) {
		m_cVal = 1;
		m_r = Eval(car);
		count += m_cVal;
		MvToStack();
	}
	Apply(fun, count, 0);
	SkipStack(2);
}

void CLispEng::F_MultipleValueProg1() {
	LISP_TAIL_REC_DISABLE_1;

	m_cVal = 1;
	m_r = Eval(SV1);
	CP body = Pop();
	SkipStack(1);
	size_t cVal = m_cVal;
	MvToStack();
	Push(body);
	ProgNoRec();
	StackToMv(cVal);
}

void CLispEng::F_Progv() {
	LISP_TAIL_REC_DISABLE_1;

	SV2 = Eval(SV2);
	m_cVal = 1;
	m_r = Eval(SV1);
	CP body = Pop();
	SkipStack(1);//!!!
	CDynBindFrame frame(Pop(), m_r);
	PrognNoRec(body);
}

CP CLispEng::BindSymbolMacrolet(size_t n, CP& sym, CP form) {
	return FromSValueT(CreateSymbolMacro(form), TS_SYMBOLMACRO);
}

void CLispEng::F_SymbolMacrolet() {
	LISP_TAIL_REC_DISABLE_1;

	if (ParseCompileEvalForm(2))
		return;
	SkipStack(1);
	CallBindedForms(&CLispEng::BindSymbolMacrolet, S(L_SYMBOL_MACROLET), Pop(), m_arVal[1], m_r);
}

class CITagbodyFrame : public CJmpBuf {
public:
	CITagbodyFrame(CP *pTop) {
		CLispEng& lisp = Lisp();
		lisp.Push(lisp.m_env.m_goEnv);
		Finish(lisp, FT_ITAGBODY, pTop);
	}
};

void CLispEng::F_Tagbody() {
	LISP_TAIL_REC_DISABLE_1;

	CP body = Pop();
	CEnv1GFrame goFrame;
	int count = 0;
	for (CP bodyrest=body, car; SplitPair(bodyrest, car);) {
		switch (Type(car)) {
		case TS_SYMBOL:
		case TS_FIXNUM:
			Push(bodyrest, car);
			count++;
		case TS_CONS:
			if (!car) //!!!
				break;
			break;
		default:
			E_Error();
		}
	}
	if (count) {
		CITagbodyFrame tagbodyFrame(goFrame.GetStackP(_self));
		m_env.m_goEnv = CreateFramePtr(m_pStack);

		while (true) {
			LISP_TRY(tagbodyFrame) {
				Push(body);
				for (CP car; SplitPair(SV, car);)
					if (ConsP(car)) {
						m_cVal = 1;
						m_r = Eval(car);
					}
				SkipStack(1);
				break;
			} LISP_CATCH(tagbodyFrame) {
				m_pStack = tagbodyFrame.GetStackP(_self);
				body = m_r;
			} LISP_CATCH_END
		}
	} else {
		Push(body); //!!! possible to free ENV1G
		ProgNoRec();
	}
	ClearResult();
}

void CLispEng::F_Go() {
	CP *frame,
		tag = Pop();
	switch (Type(tag)) {
	case TS_SYMBOL:
	case TS_FIXNUM:
		break;
	default:
		E_Error();
	}
	CP env = m_env.m_goEnv,
		car = 0;
	while (Type(env) == TS_FRAME_PTR) {
		frame = AsFrame(env);
		env = frame[FRAME_TAG_NEXT_ENV];
		if (IsFrameNested(frame[0]))
			break;
		CP *top = AsFrameTop(frame);
		for (CP *binds=frame+FRAME_TAG_BINDINGS; binds!=top; binds+=2)
			if (Eql(*binds, tag)) {
				m_r = binds[1];
				goto LAB_FOUND;
			}
	}
	while (SplitPair(env, car)) {
		CArrayValue *av = ToArray(Car(car));
		for (size_t i=0; i<av->TotalSize(); ++i)
			if (av->m_pData[i] == tag) {		 //!!! must be EQL
				env = Cdr(car);
				if (env == V_D)
					E_ProgramErr();
				frame = ToFrame(env);//!!!
				m_r = frame[FRAME_TAG_BINDINGS+1+2*i];
				goto LAB_FOUND;
			}
	}
	E_ProgramErr();
LAB_FOUND:
	UnwindTo(frame);
}

void CLispEng::F_The() {
	m_cVal = 1;
	m_r = Eval(Pop());
	MvToList();
	CP typ = SV;
	Push(m_r, m_r);
	Call(S(L_TYPE_FOR_DISCRIMINATION), typ);
	Push(m_r);
	Funcall(S(L_PTHE), 2);
	if (!m_r)
		E_TypeErr(SV, typ);
	ListToMv(Pop());
	SkipStack(1);
}

void CLispEng::F_Throw() {
	LISP_TAIL_REC_DISABLE_1;

	SV1 = Eval(SV1);
	m_cVal = 1;
	m_r = Eval(Pop());
	ThrowTo(Pop());
}

class CUnwindProtectFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CUnwindProtectFrame(CP *pTop) {
		Finish(Lisp(), FT_UNWINDPROTECT, pTop);
	}

	~CUnwindProtectFrame() {
		CLispEng& lisp = Lisp();
		lisp.m_pStack = GetStackP(lisp);
		CP cleanup = lisp.SV1; //!!! optimize Stack
		size_t cVal = lisp.m_cVal;
		lisp.MvToStack();
		lisp.Push(cleanup);
		lisp.Prog();
		lisp.StackToMv(cVal);
		base::ReleaseStack(lisp);
	}
};

void CLispEng::F_UnwindProtect() {
	LISP_TAIL_REC_DISABLE_1;

	CP cleanup = Pop(),
		form = Pop();
	Push(cleanup);
	CUnwindProtectFrame unwindProtectFrame(m_pStack+1);
#ifdef _X_DEBUG //!!!D
	cerr << endl;
	Print(form);
#endif
	m_cVal = 1;
	m_r = Eval(form);
}

void CLispEng::F_Declare() {
	SkipStack(1);
	//!!!
}


} // Lisp::

