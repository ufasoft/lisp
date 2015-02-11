#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

class CForEveryKeyword {
protected:
	virtual void Start() =0;
public:
};

class CClosureForEveryKeyword : public CForEveryKeyword {
protected:
	void Start();
public:
	CSPtr *m_pP;
	uint16_t m_nKey;

	CClosureForEveryKeyword(CSPtr *pP, uint16_t nKey)
		:	m_pP(pP)
		,	m_nKey(nKey)
	{}
};

void CClosureForEveryKeyword::Start() {
	for (int i=0; i<m_nKey; i++)
		Lisp().Push(m_pP[i]);
}

CP CLispEng::ParseDD(CP body, bool bAllowDoc) {			//!!! delete env!
	Push(body, 0, 0);
	CP compile = V_0;
	CP prev;
	for (CP form; prev=body, SplitPair(body, form);)
		if (Type(form) == TS_CONS && Car(form) == S(L_DECLARE))
			for (CP decls=Cdr(form), declSpec; SplitPair(decls, declSpec);) {
				if (Type(declSpec) == TS_CONS && Car(declSpec) == S(L_COMPILE)) {
					CP specRest = declSpec, word;
					SplitPair(specRest, word);
					if (!specRest)
						compile = V_U;
					else if (!ConsP(specRest) || !SplitPair(specRest, compile) || !FunnameP(compile))
						E_ProgramErr();
				}
				SV = Cons(declSpec, SV);
			}
		else if (StringP(form) && body) {
			if (SV1)
				E_ProgramErr();
			SV1 = form;
		} else {
			if (form != Car(prev)) //!!!
				prev = Cons(form, body);
			break;
		}
	m_r = prev;
	m_arVal[1] = NReverse(Pop());
	m_arVal[2] = Pop();
	SkipStack(1);
	if (!bAllowDoc && m_arVal[2])
		E_ProgramErr();
	m_cVal = 3;
	return compile;
}

void CLispEng::F_ParseBody() {
	bool bAllowDoc = ToOptionalNIL(Pop());
	m_arVal[3] = ParseDD(Pop(), bAllowDoc);
	if (m_arVal[3] == V_U)
		m_arVal[3] = V_1;		
	m_cVal = 4;
}

bool CLispEng::GetParamType(CP& p, CP& car, CParamType& pt) {
	pt = PT_NONE;
	if (!SplitPair(p, car))
		pt = PT_END;
	else if (Type(car) != TS_SYMBOL)
		return false;
	else {
		switch (AsIndex(car)) {
		case ENUM_L_OPTIONAL:			pt = PT_OPTIONAL; break;
		case ENUM_L_REST:				pt = PT_REST; break;
		case ENUM_L_KEY:				pt = PT_KEY; break;
		case ENUM_L_ALLOW_OTHER_KEYS:	pt = PT_ALLOW_OTHER_KEYS; break;
		case ENUM_L_AUX:				pt = PT_AUX; break;
		case ENUM_L_BODY:				pt = PT_BODY; break;
		default:
			return false;
		}
	}
	return true;
}

CP CLispEng::LambdabodySource(CP lb) {
	CP body = Cdr(lb);
	if (!ConsP(body))
		return V_U;
	CP form = Car(body);
	if (!ConsP(form) || Car(form)!=S(L_DECLARE))
		return V_U;
	CP ds = Cdr(form);
	if (!ConsP(ds))
		return V_U;
	CP d = Car(ds);
	if (!ConsP(d) || Car(d)!=S(L_SOURCE))
		return V_U;
	CP r = Cdr(d);
	return ConsP(r) ? Car(r) : V_U;
}

CP CLispEng::FunnameBlockname(CP p) {
	return ConsP(p) ? Car(Cdr(p)) : p;
}

void CLispEng::AddImplicitBlock() {
	Push(m_arVal[1], m_arVal[2]);
	CP p = Cons(Cons(S(L_BLOCK), Cons(FunnameBlockname(SV3), m_r)), 0);
	if (CP doc = Pop())
		p = Cons(doc, p);
	if (CP decls = Pop()) {
		Push(p);
		p = Cons(S(L_DECLARE), decls);
		p = Cons(p, Pop());
	}
	SV = Cons(Car(SV), p);
}

CP CLispEng::GetClosure(CP lambdaBody, CP name, bool bBlock, CEnvironment& env) {
	if (!ConsP(lambdaBody))
		E_ProgramErr();
	Push(Car(lambdaBody));
	CP& lambdaList = *m_pStack;
	Push(name, lambdaBody);
	CP compileName = ParseDD(Cdr(lambdaBody), true);
	if (compileName != V_0) {
		if (compileName!=V_U && SV1==S(L_K_LAMBDA))
			SV1 = compileName;

		CP source = LambdabodySource(SV);		// replace Lambdabody with its source (because some Macros can be compiled more efficiently than their Macro-Expansion)
		if (source != V_U)
			SV = source;
		else if (bBlock)
			AddImplicitBlock();
		PushNestEnv(env);
		Push(V_T);
		Funcall(S(L_COMPILE_LAMBDA), 8);
		SkipStack(1);
		return m_r;
	}
	CP source = LambdabodySource(SV);
#ifdef _DEBUG
	bool bPrintAfter = false;
#endif
	if (source == V_U) {
		if (bBlock)
			AddImplicitBlock();
		if (m_bExpandingLambda)
			lambdaBody = SV;
		else {
#if UCFG_LISP_LAZY_EXPAND
			lambdaBody = SV;
#else
			Push(SV, NestVar(m_env.m_varEnv));
			Push(NestFun(m_env.m_funEnv));
			CBoolKeeper keeper(m_bExpandingLambda, true);
			Funcall(S(L_EXPAND_LAMBDABODY_MAIN), 3);
			lambdaBody = m_r;
#endif // UCFG_LISP_LAZY_EXPAND

#ifdef _DEBUG
			if (bPrintAfter) {
				cerr << "\n-----------------L_EXPAND_LAMBDABODY_MAIN-------------------\n";
				Print(name);
				cerr << "\n";
				Print(m_r);
			}
#endif
		}
	}
	else {
		lambdaBody = exchange(SV, source);
	}
	lambdaList = Car(lambdaBody);
	ParseDD(Cdr(lambdaBody), true);
	CEnvironment *stackEnv = NestEnv(env);
	CIntFuncValue *ifv = CreateIntFunc();
	ifv->m_env = *stackEnv;
	SkipStack(5);
	ifv->m_docstring = m_arVal[2];
#ifdef _DEBUG //!!!D
	if (!m_r) {
		Print(SV);
		cerr << endl;
	}
#endif
	ifv->m_body = m_r;
	ifv->m_form = Pop();
	ifv->m_name = SV;
	CP clos = SV = FromSValue(ifv);
	int nSpec = 0,
		nReq = 0,
		nOpt = 0,
		nKey = 0,
		nAux = 0,
		nVar = 0;
	for (CP decl, car; SplitPair(m_arVal[1], decl);) {
		if (!SplitPair(decl, car))
			E_ProgramErr();
		if (car == S(L_SPECIAL))
			while (SplitPair(decl, car))
				if (Type(car) != TS_SYMBOL)
					E_ProgramErr();
				else {
					Push(car);
					nSpec++;
					nVar++;
				}		
				CP denv = AugmentDeclEnv(decl, AsIntFunc(clos)->m_env.m_declEnv);
				AsIntFunc(clos)->m_env.m_declEnv = denv;
	}
	CP item = 0,
		initForm = 0;
	CParamType pt;
	while (!GetParamType(lambdaList, item, pt)) {
		ToVariableSymbol(item);
		Push(item, V_0);
		nReq++;
		nVar++;
	}
	if (pt == PT_OPTIONAL) {
		while (!GetParamType(lambdaList, item, pt)) {
			nOpt++;
			nVar++;
			initForm = 0;
			if (Type(item) == TS_SYMBOL)
			{
				ToVariableSymbol(item);
				Push(item, V_0);
			}
			else
			{
				CP car = 0;
				SplitPair(item, car);
				ToVariableSymbol(car);
				Push(car, V_0);
				if (SplitPair(item, initForm) && Type(item) == TS_CONS && item) { //!!!
					if (Cdr(item))
						E_ProgramErr();
					item = Car(item);
					ToVariableSymbol(item);
					SV = CreateInteger(short(AsNumber(SV)|FLAG_SP));
					Push(item, V_0);
					nVar++;
				}
			}
			CP cons = Cons(initForm, AsIntFunc(clos)->m_optInits);
			AsIntFunc(clos)->m_optInits = cons;
		}
	}
	if (pt == PT_REST) {
		if (GetParamType(lambdaList, item, pt))
			E_ProgramErr();
		ToVariableSymbol(item);
		Push(item, V_0);
		nVar++;
		AsIntFunc(clos)->m_bRestFlag = true;
		GetParamType(lambdaList, item, pt);
	}
	if (pt == PT_KEY) {
		while (!GetParamType(lambdaList, item, pt)) {
			nKey++;
			nVar++;
			initForm = 0;
			CSPtr keyword;
			if (Type(item) == TS_SYMBOL)
			{
				ToVariableSymbol(item);
				Push(item, V_0);
				keyword = GetSymbol(AsString(item), m_packKeyword);
			}
			else {
				CP car = 0;
				SplitPair(item, car);
				if (Type(car) == TS_SYMBOL) {
					ToVariableSymbol(car);
					Push(car, V_0, item);
					keyword = GetSymbol(AsString(car), m_packKeyword);
					item = Pop();
				} else {
					keyword = Car(car);
					if (Type(keyword) != TS_SYMBOL)
						E_ProgramErr();
					//!!! need additional check
					car = Car(Cdr(car));
					ToVariableSymbol(car);
					Push(car, V_0);
				}
				if (SplitPair(item, initForm) && item) {
					SplitPair(item, car);
					if (item)
						E_ProgramErr();
					ToVariableSymbol(car);
					SV = CreateInteger(short(AsNumber(SV)|FLAG_SP));
					Push(car, V_0);
					nVar++;  
				}
			}
			Push(initForm);
			CP cons = Cons(keyword, AsIntFunc(clos)->m_keywords);
			AsIntFunc(clos)->m_keywords = cons;
			cons = Cons(Pop(), AsIntFunc(clos)->m_keyInits);
			AsIntFunc(clos)->m_keyInits = cons;
		}
	}
	if (pt == PT_ALLOW_OTHER_KEYS) {
		AsIntFunc(clos)->m_bAllowFlag = true;
		GetParamType(lambdaList, item, pt);
	}
	if (pt == PT_AUX) {
		while (!GetParamType(lambdaList, item, pt)) {
			nAux++;
			nVar++;
			initForm = 0;
			if (Type(item) != TS_SYMBOL) {
				CP r = exchange(item, 0);
				if (SplitPair(r, item) && SplitPair(r, initForm) && r)
					E_ProgramErr();
			}
			ToVariableSymbol(item);
			Push(item, V_0);
			CP cons = Cons(initForm, AsIntFunc(clos)->m_auxInits);
			AsIntFunc(clos)->m_auxInits = cons;
		} 
	}
	if (pt != PT_END)
		E_ProgramErr();
	CArrayValue *vv = CreateVector(nVar);
	ifv = AsIntFunc(clos);
	byte *q = 0; //!!!
	int k = 0;
	if (nSpec)
		goto foundFlag;
	for (; k<nVar-nSpec; k++)
		if (m_pStack[k*2] != V_0) {
foundFlag:
			ifv->m_varFlags = new byte[nVar-nSpec];
			q = ifv->m_varFlags+nVar-nSpec;
			for (int i=0; i<nVar-nSpec; i++)
				*--q = (byte)AsNumber(m_pStack[i*2]);
		}
		CP *p = vv->m_pData+nVar;
		int i;
		for (i=0; i<nVar-nSpec; i++) {
			SkipStack(1);
			*--p = Pop();
		}
		for (i=0; i<nSpec; i++)
			*--p = Pop();
		if (nSpec) {
			for (i=nSpec; i<nVar; i++) {
				for (int j=nSpec; j--;) {
					if (p[j] == p[i]) {
						q[i-nSpec] |= FLAG_DYNAMIC;
						break;
					}
				}
			}
		}
		ifv->m_vars = FromSValue(vv);
		ifv->m_nSpec = (byte)nSpec;
		ifv->m_nReq = (byte)nReq;
		ifv->m_nOpt = (uint16_t)nOpt;
		ifv->m_nKey = (byte)nKey;
		ifv->m_nAux = (byte)nAux;
		NReverse(ifv->m_optInits);
		NReverse(ifv->m_keywords);
		NReverse(ifv->m_keyInits);
		NReverse(ifv->m_auxInits);
		SkipStack(2); //!!!
		return clos;
}

CP CLispEng::NestFun(CP env) {
	int depth = 0;
	for (; Type(env)==TS_FRAME_PTR; depth++) {
		Push(env);
		env = AsFrame(env)[2];
	}
	while (depth--) {
		CP *frame = AsFrame(Pop());
		if (int count = AsNumber(frame[FRAME_COUNT])) {
			Push(env);
			CArrayValue *vv = CreateVector(count*2+1);
			for (int i=0; i<count; i++) {
				vv->m_pData[i*2] = frame[FRAME_BINDINGS+i*2];
				vv->m_pData[i*2+1] = frame[FRAME_BINDINGS+1+i*2];
			}
			vv->m_pData[count*2] = Pop();
			env = FromSValue(vv);
			frame[FRAME_NEXT_ENV] = env;
			frame[FRAME_COUNT] = V_0;
		}
	}
	return env;
}

CP CLispEng::NestVar(CP env) {
	int depth = 0;
	for (; Type(env)==TS_FRAME_PTR; depth++) {
		Push(env);
		env = AsFrame(env)[FRAME_NEXT_ENV];
	}
	while (depth--) {
		CP *frame = AsFrame(Pop());
		int nBinds = AsNumber(frame[FRAME_COUNT]); 
		int count = nBinds;
		CP *binds = frame+FRAME_BINDINGS;
		for (; count && !(binds[0] & FLAG_ACTIVE); count--)
			binds+=2;
		if (count) {
			nBinds -= count;
			Push(env);
			CArrayValue *vv = CreateVector(count*2+1);
			for (int i=0; i<count; i++, binds+=2) {
				if (binds[0] & FLAG_DYNAMIC) {
					vv->m_pData[i*2] = binds[0] & MASK_WITHOUT_FLAGS;
					vv->m_pData[i*2+1] = V_SPEC;
				} else {
					binds[0] = binds[0] & ~FLAG_ACTIVE;
					vv->m_pData[i*2] = binds[0];
					vv->m_pData[i*2+1] = binds[1];
				}
			}
			vv->m_pData[count*2] = Pop();
			env = FromSValue(vv);
			frame[FRAME_NEXT_ENV] = env;
			frame[FRAME_COUNT] = CreateFixnum(nBinds);
		}
	}
	return env;
}

/*!!!
bool CLispEng::IsMacro(CSymbolValue *sv)
{
return Type(GetSymRef(sv, m_env.m_varEnv)) == TS_SYMBOLMACRO;
}*/

void CLispEng::F_MacroP() {
	m_r = FromBool(Type(Pop()) == TS_MACRO);
}

void CLispEng::F_SymbolMacroP() {
	m_r = FromBool(Type(Pop()) == TS_SYMBOLMACRO);
}

void CLispEng::F_MakeSymbolMacro() {
	m_r = FromSValueT(CreateSymbolMacro(Pop()), TS_SYMBOLMACRO);
}

void CLispEng::F_MakeGlobalSymbolMacro() {
	F_MakeSymbolMacro();
	m_r = FromSValueT(CreateSymbolMacro(m_r), TS_GLOBALSYMBOLMACRO);
}

void CLispEng::F_SymbolMacroExpand() {
	CSymbolValue *sv = ToSymbol(SV);
	if (sv->SymMacroP) {
		CP q = Get(SV, S(L_SYMBOLMACRO));
		if (m_r = FromBool(q!=V_U)) {
			m_arVal[1] = ToSymbolMacro(ToGlobalSymbolMacro(q)->m_macro)->m_macro;
			m_cVal = 2;
		}
		else
			sv->SymMacroP = false;
	}
	SkipStack(1);
}

void CLispEng::F_GlobalSymbolMacroDefinition() {
	m_r = ToGlobalSymbolMacro(Pop())->m_macro;
}

void CLispEng::F_FindSubr() {
	if ((m_r=Get(SV, S(L_TRACED_DEFINITION))) == V_U)
		AsSymbol(SV)->GetFun();
	else if (Type(m_r) != TS_SUBR)
		E_SeriousCondition();
	SkipStack(1);
}

CP *CLispEng::FindVarBind(CP sym, CP env) {
	bool bFromInsideMacrolet = false;
	while (true) {
		switch (Type(env)) {
		case TS_ARRAY:
			{
				CArrayValue *vv = AsArray(env);
				int count = AsFixnum(vv->m_dims)/2;
				CP *pData = vv->m_pData;
				int i=0;
				for (; i<count; i++)
					if (pData[i*2] == sym) {
						CP& q = pData[i*2+1];
						if (bFromInsideMacrolet && q!=V_SPEC && Type(q)!=TS_SYMBOLMACRO)
							E_SeriousCondition();
						return &q;
					}
					env = pData[i*2];
			}
			break;
		case TS_CONS:
			if (env) {
#ifdef X_DEBUG//!!!D
				cerr << "sym= ";
				Print(sym);
				cerr << "\nenv= ";
				Print(env);
#endif
				ASSERT(Car(env)==S(L_MACROLET));
				bFromInsideMacrolet = true;
				Inc(env);
				break;
			}
		default:
			return 0;
		}
	}
}

void CLispEng::F_SpecialVariableP() {
	CP env = ToOptionalNIL(Pop());
	CP sym = Pop();
	CSymbolValue *sv = ToSymbol(sym);
	if (sv->m_fun & SYMBOL_FLAG_SPECIAL)
		m_r = V_T;
	else if (env) {
		if (env == V_T)
			env = m_env.m_varEnv;
		else if (Type(env) == TS_ARRAY) {
			CArrayValue *av = ToVector(env); //!!!
			size_t len = av->DataLength;
			if (len==2 || len==5)
				env = av->m_pData[0];
		}
		if (CP *pv = FindVarBind(sym, env))
			m_r = FromBool(*pv==V_SPEC);
	}
}

void CLispEng::CheckAllowOtherKeys(CP *pBase, size_t nArg) {
	size_t j;
	for (j=nArg; j;) {
		if (pBase[(j-=2)+1] == S(L_K_ALLOW_OTHER_KEYS)) {
			pBase[j+1] = 0;
			if (pBase[j])
				return;
			break;
		}
	}
	for (j=nArg; j;)
		if (pBase[(j-=2)+1])
			E_ProgramErr();
}

#if UCFG_LISP_TEST //!!!
	int g_maxDeep;
#endif

#if UCFG_LISP_TAIL_REC

void CLispEng::TailRecApplyIntFunc(CP fun, ssize_t nArg) {
	for (CP *p=m_pStack; p!=m_pStackTop; ++p) {
		CP q = *p;
		if (Type(q) == TS_FRAMEINFO) {
			int idx = AsIndex(q);
			CFrameType ft = CFrameType(idx & FRAME_TYPE_MASK);
			switch (ft) {
			case FT_APPLY:
				m_tailedFun = fun;
				m_nTailArgs = nArg;
				m_pTailStack = m_pStack;
				return;
			case FT_VAR:
			{
				int nv = idx>>8;
				for (int i=1; i<nv; ++i) {
					CP sym = p[i];
					if (sym & FLAG_DYNAMIC)
						goto LAB_OUT;
				}
				p += nv-1;
			}
			break;
			case FT_EVAL:
			case FT_ENV1V:
			case FT_ENV2VD:
			case FT_ENV5:
			case FT_ENV1B:
			case FT_ENV1F:
			case FT_ENV1G:
			case FT_ITAGBODY:
			case FT_IBLOCK:
				break;
			case FT_DYNBIND:
			case FT_FUN:
			case FT_UNWINDPROTECT:
				goto LAB_OUT;
			default:
				goto LAB_OUT;
			}
		}
	}
LAB_OUT:												//!!!BUG of VS2015. Stack is not unwinded
	LISP_TAIL_REC_ENABLED = false;
}
#endif // UCFG_LISP_TAIL_REC

void CLispEng::ApplyIntFunc(CP fun, ssize_t nArg) {
#if UCFG_LISP_TAIL_REC
	if (LISP_TAIL_REC_ENABLED)
		TailRecApplyIntFunc(fun, nArg);

	CP *pStack = m_pStack;
LAB_TAIL_RET:
	//	CStackKeeper stackKeeper(_self);

	if (m_tailedFun) {

#ifdef X_DEBUG
		static ostream& s_fos = cerr; //("c:\\out\\debug\\lisp.log");
		s_fos << "\ntail returning " << endl;
		Print(s_fos, fun);
		s_fos << "  ->  ";
		Print(s_fos, m_tailedFun);
		s_fos << "---" << endl;
#endif

		fun = exchange(m_tailedFun, 0);
		m_pStack = pStack + nArg - m_nTailArgs;
		memmove(m_pStack, m_pTailStack, (nArg = m_nTailArgs)*sizeof(CP));
		pStack = m_pStack;
		//!!!		LISP_TAIL_REC_ENABLED = false;
	}

#endif	// UCFG_LISP_TAIL_REC

#ifdef _X_TEST //!!!
	static int base = (int)&fun;
	g_maxDeep = _MAX(g_maxDeep, base-(int)&fun);
#endif
#if UCFG_LISP_LAZY_EXPAND
	if (!m_bExpandingLambda && !f.m_bExpanded && f.m_form) {
		AsIntFunc(fun)->m_bExpanded = true;
		Push(fun);
		//!!!		Print(f.m_form);
		Push(f.m_form, f.m_env.m_varEnv, f.m_env.m_funEnv);
		CBoolKeeper keeper(m_bExpandingLambda, true);
		Funcall(S(L_EXPAND_LAMBDABODY_MAIN), 3);
		SkipStack(1);
		ParseDD(Cdr(m_r), true);
		//!!!		Print(m_r);
		//!!!		cerr << endl << endl;
		AsIntFunc(fun)->m_body = m_r;
		f = *AsIntFunc(fun);
	}
#endif // UCFG_LISP_LAZY_EXPAND
	CP *pArgs = m_pStack+nArg;
	{
		CApplyFrame applyFrame(_self, pArgs, fun);
		if (m_bDebugFrames) {
#if UCFG_LISP_SJLJ && LISP_ALLOCA_JMPBUF
			applyFrame.m_pJmpbuf = (jmp_buf*)alloca(sizeof(jmp_buf));
#endif

			LISP_TRY(applyFrame) {
				//!!!?

			} LISP_CATCH(applyFrame) {
				if (m_cVal)
					goto LAB_RET;
				else
					fun = m_pStack[1]; //!!! frame_closure
			} LISP_CATCH_END
		}
		CP *pTop = m_pStack;
		CIntFuncValue *fp = AsIntFunc(fun);
#ifdef X_DEBUG//!!!D
		if (g_print) {
			Disassemble(cerr, CurClosure);
			Print(fp->m_name);
			cout << "\n";
		}
#endif
		PROF_POINT(fp->m_profInfo)
		CArrayValue *av = ToArray(fp->m_vars);
		CP *p = av->m_pData;
		int count = AsFixnum(av->m_dims); //!!!E TotalSize();
		int nSpec = fp->m_nSpec;
		int i;
		for (i=0; i<nSpec; i++)
			Push(V_SPEC, *p++ | FLAG_ACTIVE);
		CVarFrame varFrame(m_pStack);
		byte *varFlags = fp->m_varFlags;
		for (i=0; i<count-nSpec; i++) {
			CP sym = *p++;
			if ((ToSymbol(sym)->m_fun & (SYMBOL_FLAG_SPECIAL|SYMBOL_FLAG_CONSTANT)) == SYMBOL_FLAG_SPECIAL)
				sym = sym | FLAG_DYNAMIC;
			if (varFlags)
				sym |= *varFlags++;
			Push(0, sym);
		}
		Push(fp->m_env.m_varEnv, CreateFixnum(count)); //!!!
		varFrame.Finish(_self, pTop);
		CP env = CreateFramePtr(m_pStack);
		CEnv5Frame env5Frame(_self);
		m_env.m_varEnv = env;
		m_env.m_funEnv = fp->m_env.m_funEnv;
		m_env.m_blockEnv = fp->m_env.m_blockEnv;
		m_env.m_goEnv = fp->m_env.m_goEnv;
		m_env.m_declEnv = fp->m_env.m_declEnv;
		int n = nArg;
		CP body = fp->m_body;
		{
			CIntFunc ff = *AsIntFunc(fun);
			CIntFunc *f = &ff;
			//!!!		CIntFuncChain f(AsIntFunc(fun));
			int nReq = f->m_nReq,
				nOpt = f->m_nOpt;		 
			if (n < nReq)
				E_ProgramErr();
			n -= nReq;
			for (int i=0; i<nReq; i++)
				varFrame.Bind(*--pArgs);
			count = nOpt;
			CP inits = f->m_optInits;
			for (; count; count--, Inc(inits)) {
				if (!n) {
					Push(inits);
					while (count--) {
						CP car = 0;
						SplitPair(SV, car);
						if (varFrame.Bind(Eval(car)))
							varFrame.Bind(0); //!!!
					}
					if (f->m_bRestFlag)
						varFrame.Bind(0);
					SV = f->m_keyInits;
					for (count=f->m_nKey; count--; Inc(inits)) {
						CP car = 0;
						SplitPair(SV, car);
						if (varFrame.Bind(Eval(car)))
							varFrame.Bind(0); //!!!
					}
					SkipStack(1);
					goto LAB_AUX;
				}
				n--;
				if (varFrame.Bind(*--pArgs))
					varFrame.Bind(V_T); //!!!
			}
			if (!f->m_keywords && !f->m_bAllowFlag && !f->m_bRestFlag) {
				if (n)
					E_ProgramErr();
			} else {
				if (f->m_bRestFlag) {
					Push(0);
					for (int i=0; i<n; i++)
						SV = Cons(pArgs[i-n], SV);
					varFrame.Bind(Pop());
				}
				if (f->m_keywords) {
					if (n & 1)
						E_ProgramErr();
					n >>= 1;
					CP keywords = f->m_keywords,
						keyInits = f->m_keyInits;
					bool bAllow = f->m_bAllowFlag;
					for (int i=0; i<f->m_nKey; i++, Inc(keyInits)) {
						CP keyword = 0, val = 0, svar = 0;
						SplitPair(keywords, keyword);
						bool bAllowKey = false;
						for (ssize_t j=n-1; j>=0; j--) {
							if (pArgs[-j*2-1] == keyword) {
								svar = V_T;
								val = pArgs[-j*2-2];
								if (keyword == S(L_K_ALLOW_OTHER_KEYS))
									bAllowKey = val;			//!!!Q may be must be T?
								pArgs[-j*2-1] = 0;						
							}
						}
						if (!svar)
							val = Eval(Car(keyInits));
						if (varFrame.Bind(val))
							varFrame.Bind(svar);
						bAllow = bAllow || bAllowKey;
					}
					if (!bAllow)
						CheckAllowOtherKeys(pArgs-(n<<1), n<<1);
				}
			}
LAB_AUX:
			Push(f->m_auxInits);
			for (int j=f->m_nAux; j--;) {
				CP car = 0; //!!!
				SplitPair(SV, car);
				varFrame.Bind(Eval(car));
			}
			SkipStack(1);
		}

		LISP_TAIL_REC_KEEPER(true);
#if UCFG_LISP_TAIL_REC == 2
		CTailRecKeeper trKeeper2(_self, false);	// for right work of Progn()
#endif

		Progn(body);

#if UCFG_LISP_TAIL_REC
		if (m_tailedFun) {
			goto LAB_TAIL_RET;
		}
#endif
		return;
	}
LAB_RET:
	m_cVal = 1;
	m_r = Eval(m_r);
}

class CLevelKeeper {
	int& m_level;
public:
	int m_prev;

	CLevelKeeper(int& level)
		:	m_level(level)
		,	m_prev(m_level)
	{
	}

	~CLevelKeeper() {
		m_level = m_prev;
	}
};

class CTracer {
	LISP_LISPREF;

	CLevelKeeper m_levelKeeper;
	ptr<StandardStream> m_traceStream;
	ostream *m_os;
	CP m_fun;
public:
	bool m_bNormalExit;

	CTracer(CLispEng& lisp, CP fun, int nArg)
		:	LISP_SAVE_LISPREF(lisp)
			m_os(0)
		,	m_levelKeeper(lisp.m_level)
		,	m_bNormalExit(false)
	{
		if (!lisp.m_bTrace)
			return;
		lisp.m_level++;
		m_traceStream = lisp.m_streams[STM_TraceOutput];
		m_os = m_traceStream->GetOstream(); //!!!? may be dybamic_cast
		int i;
		for (i=m_levelKeeper.m_prev; i--;)
			*m_os << "  ";
		pair<CP, CP> pp = lisp.GetFunctionName(fun);
		m_fun = pp.second;
		lisp.Print(pp.first);
		*m_os << " ";
		lisp.Print(pp.second);
		*m_os << " > ";
		for (i=0; i<nArg; i++) {
			*m_os << ' ';
			lisp.Print(lisp.m_pStack[nArg-i-1]);
		}
		*m_os << '\n';
	}

	~CTracer() {
		if (!m_os || !m_bNormalExit)
			return;
		for (int i=m_levelKeeper.m_prev; i--;)
			*m_os << "  ";
		CLispEng& lisp = LISP_GET_LISPREF;
		lisp.Print(m_fun);
		*m_os << " < ";
		lisp.PrintValues(*m_os);
		*m_os << '\n';
	}
};

#if UCFG_LISP_FAST_HOOKS
void CLispEng::ApplyImp(CP fun, ssize_t nArg) {
#else
void CLispEng::Apply(CP fun, ssize_t nArg) {
	if (Signal)
		return ApplyHooked(fun, nArg);
#endif

#ifdef X_DEBUG//!!!D
	static int s_i;
	if (++s_i == 6660) {//6420) // In Interpreted 14500
		StackOverflowAddress = (void*)1;

		E_Error(S(L_STACK_OVERFLOW_ERROR));
	}
	if (s_i == 170000) {
		//PrintFrames();
	}
#endif

	LISP_TRACER;

	switch (Type(fun)) {
	case TS_CCLOSURE:
		ApplyClosure(fun, nArg);	//!!!Q may be direct call to ApplyClosure(...rest)?
		break;
	case TS_SUBR:
		ApplySubr(fun, nArg);
		break;
	case TS_INTFUNC:
		ApplyIntFunc(fun, nArg);
		break;
	default:
		E_SeriousCondition();
	}

	LISP_TRACER_NORMAL_EXIT;
}

void CLispEng::ApplyHooked(CP fun, ssize_t nArg) {
	while (Signal) {		//!!!
		int sig = exchange(m_Signal, 0);
		switch (sig) {
		case SIGINT:
			E_Signal(S(L_INTERRUPT_CONDITION));
			break;
		case int(HRESULT_OF_WIN32(ERROR_STACK_OVERFLOW)):
#ifdef X_DEBUG//!!!D
			PrintFrames();
#endif
			Push(Spec(L_STACK_OVERFLOW_INSTANCE));
			F_InvokeHandlers();
			abort();
			break;
		case SIGTERM: Throw(E_EXT_NormalExit); //!!!
		}
		Push(fun);
		//!!!    Push(args);
		Call("BREAK", 0);
		//!!!    args = Pop();
		fun = Pop();
	}

#if UCFG_LISP_FAST_HOOKS

	if (CP hook = Spec(L_S_APPLYHOOK)) {
		Call(hook, Listof(nArg));					//!!! may optimize
		return;
	}
	m_mfnApply = &class_type::ApplyImp;
#endif
	Apply(fun, nArg);
}

void CLispEng::Apply(CP form, ssize_t nArg, CP rest) {
#ifdef _DEBUG
	{
		//		Print(form);
		//		Disassemble(cerr, *m_pClosure);
	}
#endif
	for (CP car; SplitPair(rest, car); nArg++)
		Push(car);
	Funcall(form, nArg);
}

void CLispEng::Call(CP p, CP a) {
	Push(a);
	Funcall(p, 1);
}

void CLispEng::Call(CP p, CP a, CP b) {
	Push(a, b);
	Funcall(p, 2);
}

void CLispEng::Call(CP p, CP a, CP b, CP c) {
	Push(a, b, c);
	Funcall(p, 3);
}

void CLispEng::Call(CP p, CP a, CP b, CP c, CP d) {
	Push(a, b, c, d);
	Funcall(p, 4);
}

void CLispEng::Call(CP p, CP a, CP b, CP c, CP d, CP e) {
	Push(a, b, c, d, e);
	Funcall(p, 5);
}

CP * __fastcall CLispEng::SymValue(CSymbolValue *sym, CP env, CP *pSymMacro) {
	if (!(sym->m_fun & SYMBOL_FLAG_SPECIAL)) {
		CP symp = FromSValue(sym),
			sympA = symp | FLAG_ACTIVE;
		for (CP *frame; Type(env)==TS_FRAME_PTR; env=frame[FRAME_NEXT_ENV]) {
			CP *ptr = (frame=AsFrame(env))+FRAME_BINDINGS;
			for (int count=AsFixnum(frame[FRAME_COUNT]); count--; ptr+=2) //!!! without check for speed
				if (*ptr == sympA) { 		//!!!        if ((*ptr & (MASK_WITHOUT_FLAGS|FLAG_ACTIVE)) == sympA)
					CP& v = ptr[1];
					return v==V_SPEC ? &sym->m_dynValue : &v;
				}
		}
		if (CP *pv = FindVarBind(symp, env)) {
			if (*pv != V_SPEC) {
				if (Type(*pv) != TS_SYMBOLMACRO)
					return pv;
				*pSymMacro = *pv;
				return 0;
			}
		} else if (sym->SymMacroP) {
			CP sm = Get(symp, S(L_SYMBOLMACRO));
			if (sm != V_U)
				*pSymMacro = ToGlobalSymbolMacro(sm)->m_macro;
			else
				sym->SymMacroP = false;
		}
	}
	return &sym->m_dynValue;
}

bool CLispEng::SymMacroP(CP p) {
	CP symMacro = 0;
	SymValue(ToSymbol(p), m_env.m_varEnv, &symMacro);
	return symMacro;
}

CP __fastcall CLispEng::GetSymFunction(CP sym, CP fenv) {
	CP val = 0;
	{
		for (CP *frame; Type(fenv)==TS_FRAME_PTR; fenv=frame[FRAME_NEXT_ENV]) {
			frame = AsFrame(fenv);
			CP *binds = frame+FRAME_BINDINGS;
			for (int count=AsNumber(frame[FRAME_COUNT]); count--; binds+=2)
				if (Equal(binds[0], sym)) {
					val = binds[1];
					goto LAB_OUT;
				}    
		}
		bool bFromInsideMacrolet = false;
		while (true) {
			switch (Type(fenv)) {
			case TS_ARRAY:
				{
					CArrayValue *vv = AsArray(fenv);
					CP *p = vv->m_pData;
					for (int count=AsFixnum(vv->m_dims)/2; count--; p+=2)  {	//!!!Evv->TotalSize()/2
						if (Equal(p[0], sym)) {
							val = p[1];
							if (bFromInsideMacrolet && Type(val)!=TS_MACRO)
								E_SeriousCondition();
							goto LAB_OUT;
						}
					}
					fenv = *p;
				}
				break;
			case TS_CONS:
				if (fenv) {
					ASSERT(Car(fenv)==S(L_MACROLET));
					bFromInsideMacrolet = true;
					Inc(fenv);
					break;
				}
			default:
				if (Type(sym) != TS_SYMBOL) {
					sym = GetSymProp(Car(Cdr(sym)), S(L_SETF_FUNCTION));
					if (Type(sym) != TS_SYMBOL)
						return V_U;
				}
				return AsSymbol(sym)->GetFun();
			}
		}
	}
LAB_OUT:
	return val ? val : V_U;
}

void CLispEng::ApplySubr(CP fun, ssize_t nArg) {
	LISP_TAIL_REC_KEEPER(false);

	m_subrSelf = fun;
	CReqOptRest ror = AsReqOptRest(fun);
	if ((nArg-=ror.m_nReq) < 0) {
		CP name = GetSubrName(fun);
		Push(name);
		int n = nArg+ror.m_nReq;
		for (int i=0; i<n; ++i)
			Push(m_pStack[n]);
		E_ProgramErr(IDS_E_TooFewArguments, name, Listof(n+1));
	}
	if ((nArg-=ror.m_nOpt) < 0)
		PushUnbounds(- exchange(nArg, 0));
	size_t idx = AsIndex(fun) & 0x3FF;
	ClearResult();
	if (!WithRestP(fun)) {
		if (WithKeywordsP(fun)) {
			if (nArg & 1)
				E_ProgramErr();

			const byte *pk = s_stFuncInfo[idx].m_keywords;

			pair<size_t, bool> pp = KeywordsLen(pk);
			size_t nKey = pp.first;			

			CP *vals = (CP*)alloca(nKey*sizeof(CP));
			FillMem(vals, nKey, V_U);

			for (size_t i=0; i<nKey; ++i) {
				CP kw = get_Sym(CLispSymbol(pk[i]));
				for (int j=0; j<nArg; j+=2)
					if (m_pStack[j+1] == kw) {
						vals[nKey-i-1] = m_pStack[j];
						m_pStack[j+1] = 0;
					}
			}
			if (!pp.second)
				CheckAllowOtherKeys(m_pStack, nArg);
			memcpy(m_pStack += nArg-nKey, vals, nKey*sizeof(CP));		// Replace nArg args with nKey vals in Stack
		} else if (nArg)
			E_ProgramErr();
		PROF_POINT(m_arSubrProfile[idx]);
		(this->*s_stFuncAddrs[idx])();
	} else {
		PROF_POINT(m_arSubrProfile[idx]);
		(this->*s_stFuncRAddrs[idx-SUBR_FUNCS])(nArg);
	}
}



//!!!D int g_nn;

#if UCFG_LISP_FAST_HOOKS
CP CLispEng::EvalImp(CP p) {
#else
CP CLispEng::EvalCall(CP p) {
#endif


#if	UCFG_LISP_FAST_EVAL_ATOMS == 1
	if (IsSelfEvaluated(p)) {
		m_cVal = 1;
		return m_r = p;
	}
#endif
	

#ifdef X_DEBUG//!!!D
	if (m_pStackTop-m_pStack > 80000)
//	if (p == 0xEEAF00)
	{
		E_Error();
//		cerr << "\n-----------Eval----------------------" << '\n';
		Print(p);
		cerr << "\n";
	}
#endif 

	

#if 0
	struct CEvalFrameKeeper {
		CEvalFrame *m_p;
		CP *m_pStack;

		CEvalFrameKeeper(CP form)
			:	m_p (0)
		{
			CLispEng& lisp = Lisp();
			m_pStack = lisp.m_pStack;
			lisp.Push(form);
		}

		~CEvalFrameKeeper() {
/*			if (Lisp().m_pStack != m_pStack)
			{
				Lisp().Print(m_form);
				cerr << hex << m_form;
				m_p = m_p;
			} */
			if (m_p)
				m_p->~CEvalFrame();
			Lisp().m_pStack = m_pStack;

		}
	} keepEval(p); // = { 0 };
#endif

#if UCFG_LISP_DEBUG_FRAMES
	CEvalFrame evalFrame(_self, p);
#	if		!UCFG_LISP_FAST_HOOKS

	if (m_bDebugFrames) {
#		if UCFG_LISP_SJLJ && LISP_ALLOCA_JMPBUF
		evalFrame.m_pJmpbuf = (jmp_buf*)alloca(sizeof(jmp_buf));
#		endif

//		new(keepEval.m_p = (CEvalFrame*)alloca(sizeof CEvalFrame)) CEvalFrame(_self, p);

		//CEvalFrame frame(_self, p);

		LISP_TRY(evalFrame) {			// m_bDebugFrames && 
			//!!!?

		} LISP_CATCH(evalFrame) {
			if (m_cVal)
				m_pStack[FRAME_FORM] = m_r;
			p = m_pStack[FRAME_FORM];
			if (IsSelfEvaluated(p)) {
				m_cVal = 1;
				return m_r = p;
			}

			ts = Type(p);
#		ifdef _DEBUG//!!!D
			E_Error();
#		endif
		} LISP_CATCH_END
	}

	if (CP hook = Spec(L_S_EVALHOOK)) {
		CDynBindFrame bind;
		bind.Bind(_self, S(L_S_EVALHOOK), 0);
		bind.Bind(_self, S(L_S_APPLYHOOK), 0);
		bind.Finish(_self, 2);
		Push(p);
		PushNestEnvAsArray(m_env);
		Funcall(hook, 2);
	} else 
#	endif	// !UCFG_LISP_FAST_HOOKS
#else
	CP *prevStackP = m_pStack;						// faster without Frame
	Push(p, FromValue(FT_EVAL|(2<<8), TS_FRAMEINFO));
//!!!D	CPseudoEvalFrame pseudoEvalFrame(_self, p);

#endif	//UCFG_LISP_DEBUG_FRAMES

	if (Type(p) == TS_CONS) {
#if !UCFG_LISP_FAST_EVAL_ATOMS
		if (!p) {		//!!!
			m_cVal = 1;
			m_r = 0;
			goto LAB_RET;
		}
#endif
		CheckStack(); //!!!
		CConsValue *form = AsCons(p);
		CP sym = form->m_car;
		CSPtr fun;

		if (Type(sym)==TS_SYMBOL && Type(fun=GetSymFunction(sym, m_env.m_funEnv))==TS_MACRO) {
			LISP_TAIL_REC_KEEPER(false);

#ifdef C_LISP_QUICK_MACRO
			Push(m_env.m_funEnv, m_env.m_varEnv);
			CP env = CreateFramePtr(m_pStack);
			Push(AsMacro(fun)->m_macro, p, env);
#else
			Push(AsMacro(fun)->m_expander, p);
			Push(NestVar(m_env.m_varEnv));
			Push(NestFun(m_env.m_funEnv));
			CArrayValue *vec = CreateVector(2);
			vec->m_pData[1] = Pop();
			vec->m_pData[0] = Pop();
			Push(FromSValue(vec));
#endif
			Funcall(Spec(L_S_MACROEXPAND_HOOK), 3);
			m_cVal = 1;
			m_r = Eval(m_r);
			goto LAB_RET;
		}
		if (!fun) {
			if (!FunnameP(sym)) {
				CP q = sym, car;
				if (!SplitPair(q, car) || car!=S(L_LAMBDA))
					E_ProgramErr();
				fun = GetClosure(q, sym, false, m_env);//!!!						
			} else
				fun = GetSymFunction(sym, m_env.m_funEnv);

			form = AsCons(p);
		}
FUN_DISPATCH:
		switch (Type(fun)) {
		case TS_FUNCTION_MACRO:
			fun = ToFunctionMacro(fun)->m_function; //!!!O
			goto FUN_DISPATCH;
		case TS_SPECIALOPERATOR:
			{
				CP args = form->m_cdr;
				CReqOptRest ror = AsReqOptRest(fun);
				CP car;
				for (; ror.m_nReq--;) {
					if (!SplitPair(args, car))
						E_ProgramErr();
					else
						Push(car);
				}
				for (; ror.m_nOpt--;) {
					if (SplitPair(args, car))
						Push(car);
					else
						Push(V_U);
				}
				if (WithRestP(fun))
					Push(args);
				else if (args) {
					E_ProgramErr();
				}
				ClearResult();

#if UCFG_LISP_TAIL_REC == 2
				LISP_TAIL_REC_KEEPER(false);
#endif
				(this->*AsSpecialOperator(fun))();
			}
			break;
		case TS_CCLOSURE:
		case TS_INTFUNC:
			Push(fun); //!!! we need it save from GC
		case TS_SUBR:
			{
				int nArg = 0;
				{
					LISP_TAIL_REC_KEEPER(false);

					for (CP args=form->m_cdr, car; SplitPair(args, car); nArg++)
						Push(Eval(car));
				}
#if !UCFG_LISP_FAST_HOOKS
				if (CP hook = Spec(L_S_APPLYHOOK)) {
					Call(hook, Listof(nArg));					//!!! may optimize
				} else
#endif
					Apply(fun, nArg);

			}
			break;
		default:
#ifdef _DEBUG //!!!D
			Print(p);
			Print(sym);
			Print(AsSymbol(sym)->HomePackage);
			CSymbolValue *sv = AsSymbol(sym);
#endif
			E_UndefinedFunction(sym);
		}
	}
#if UCFG_LISP_FAST_EVAL_ATOMS
	else {
#else
	else if (Type(p) == TS_SYMBOL) {
#endif
		CP symMacro = 0;
		CP *pr = SymValue(AsSymbol(p), m_env.m_varEnv, &symMacro);
		if (symMacro) {
			m_cVal = 1;
			m_r = Eval(ToSymbolMacro(symMacro)->m_macro);
		} else if ((m_r=*pr) == V_U)
			CheckSymbolValue(p);
		else
			m_cVal = 1;
	}
#if	!UCFG_LISP_FAST_EVAL_ATOMS
	else {
		m_r = p;
		m_cVal = 1;
	}
#endif

LAB_RET:
#if !UCFG_LISP_DEBUG_FRAMES
	m_pStack = prevStackP;
#endif
	return m_r;
}

#if UCFG_LISP_FAST_HOOKS

CP CLispEng::EvalHooked(CP p) {

	CEvalFrame evalFrame(_self, p);

	if (m_bDebugFrames) {
#if UCFG_LISP_SJLJ && LISP_ALLOCA_JMPBUF
		evalFrame.m_pJmpbuf = (jmp_buf*)alloca(sizeof(jmp_buf));
#endif

//		new(keepEval.m_p = (CEvalFrame*)alloca(sizeof CEvalFrame)) CEvalFrame(_self, p);

		//CEvalFrame frame(_self, p);

		LISP_TRY(evalFrame) {			// m_bDebugFrames && 
			//!!!?

		} LISP_CATCH(evalFrame) {
			if (m_cVal)
				m_pStack[FRAME_FORM] = m_r;
			p = m_pStack[FRAME_FORM];
#ifdef X_DEBUG//!!!D
			E_Error();
#endif
		} LISP_CATCH_END
	}


	if (CP hook = Spec(L_S_EVALHOOK)) {
		CDynBindFrame bind;
		bind.Bind(_self, S(L_S_EVALHOOK), 0);
		bind.Bind(_self, S(L_S_APPLYHOOK), 0);
		bind.Finish(_self, 2);
		Push(p);
		PushNestEnvAsArray(m_env);
		Funcall(hook, 2);
		return m_r;
	}
#if UCFG_LISP_FAST_EVAL_ATOMS == 2
	if (IsSelfEvaluated(p)) {
		m_cVal = 1;
		return m_r = p;
	}
#endif
	return EvalImp(p);
}

void CLispEng::CheckBeforeSetSymVal(CP sym, CP v) {
	switch (sym) {
	case S(L_S_EVALHOOK):
		m_mfnEval = v ? &class_type::EvalHooked : &class_type::EvalImp;
#if UCFG_LISP_FAST_EVAL_ATOMS == 2
		m_maskEvalHook = v ? ((CP)0)-1 : CP((CP(2)<<(sizeof(CP)*8-VALUE_SHIFT))-2);
#endif						 
		break;
	case S(L_S_APPLYHOOK):
		m_mfnApply = v ? &class_type::ApplyHooked : &class_type::ApplyImp;
		if (Signal)									// can be set asynchronously
			m_mfnApply = &class_type::ApplyHooked;
		break;
	}
}

#endif  // UCFG_LISP_FAST_HOOKS

CP CLispEng::SwapIfDynSymVal(CP symWithFlags, CP v) {
	if (symWithFlags & FLAG_DYNAMIC) {
		CP sym = symWithFlags & MASK_WITHOUT_FLAGS;
#if UCFG_LISP_FAST_HOOKS
		CheckBeforeSetSymVal(sym, v);
#endif
		return exchange(ToSymbol(sym)->m_dynValue, v);
	} else
		return v;
}

void CLispEng::F_MacroFunction() {
	CSPtr env = ToOptionalNIL(Pop()),
		sym = Pop();
	ToSymbol(sym);
	CSPtr fenv;
#ifdef C_LISP_QUICK_MACRO
	if (Type(env) == TS_FRAME_PTR)
		fenv = ToFrame(env)[1];
	else
#endif
		if (env)
			fenv = ToVector(env)->GetElement(1);
	CP fun = GetSymFunction(sym, fenv);
	switch (Type(fun)) {
	case TS_SPECIALOPERATOR:
		m_r = GetSymProp(sym, S(L_MACRO));
		break;
	case TS_MACRO:
		m_r = AsMacro(fun)->m_expander;
	}
}

void CLispEng::F_Macroexpand1() {
	CSPtr env = ToOptionalNIL(Pop()),
		varEnv, funEnv, fun , car, val;
	if (env) {
#ifdef C_LISP_QUICK_MACRO
		if (Type(env) == TS_FRAME_PTR) {
			CP *pEnvs = ToFrame(env);
			varEnv = pEnvs[0];
			funEnv = pEnvs[1];
		}
		else
#endif
		{
			CArrayValue *vec = ToVector(env);
			varEnv = vec->GetElement(0);
			funEnv = vec->GetElement(1);
		}
	}
	m_arVal[1] = 0;
	CP r = Pop();
	switch (Type(r)) {
	case TS_CONS:
		{
			if (Type(car=Car(r)) != TS_SYMBOL)
				break;
			CSPtr fenv;
#ifdef C_LISP_QUICK_MACRO
			if (Type(env) == TS_FRAME_PTR)
				fenv = ToFrame(env)[1];
			else
#endif
				if (env)
					fenv = ToVector(env)->GetElement(1);
			CP fun = GetSymFunction(car, fenv);
			switch (Type(fun)) {
			case TS_SPECIALOPERATOR:
				if ((m_r=Get(car, S(L_MACRO))) == V_U)
					goto LAB_RET;
				break;
			case TS_MACRO:
				m_r = AsMacro(fun)->m_expander;
				break;
			case TS_SYMBOL:
				r = Cons(S(L_FUNCALL), Cons(fun, Cdr(r)));
				m_arVal[1] = V_T;
			default:
				goto LAB_RET;
			}
			Push(m_r, r, env);
			Funcall(Spec(L_S_MACROEXPAND_HOOK), 3);
			r = m_r;
			m_arVal[1] = V_T;
		}
		break;
	case TS_SYMBOL:
		CP symMacro = 0;
		SymValue(AsSymbol(r), varEnv, &symMacro);
		if (m_arVal[1] = FromBool(symMacro))
			r = ToSymbolMacro(symMacro)->m_macro;
	}
LAB_RET:
	m_r = r;
	m_cVal = 2;
}

void CLispEng::Eval5Env(CP form, const CEnvironment& env) {
	CEnv5Frame env5Frame(_self);
	m_env = env;
	m_cVal = 1;
	m_r = Eval(form);
}

void CLispEng::Setq(CP name, CP val) {
	CP symMacro = 0;
	SetSymValue(name, m_env.m_varEnv, &symMacro, val);
	if (symMacro)
		E_ProgramErr();
}

CEnv1VFrame::CEnv1VFrame() {
	CLispEng& lisp = Lisp();
	lisp.Push(lisp.m_env.m_varEnv);
	Finish(lisp, FT_ENV1V, lisp.m_pStack+1);
}

CEnv1VFrame::~CEnv1VFrame() {
	CLispEng& lisp = Lisp();
	lisp.m_env.m_varEnv = GetStackP(lisp)[1];
	base::ReleaseStack(lisp);
}

CEnv1FFrame::CEnv1FFrame() {
	CLispEng& lisp = Lisp();
	lisp.Push(lisp.m_env.m_funEnv);
	Finish(lisp, FT_ENV1F, lisp.m_pStack+1);
}

CEnv1FFrame::~CEnv1FFrame() {
	CLispEng& lisp = Lisp();
	lisp.m_env.m_funEnv = GetStackP(lisp)[1];
	base::ReleaseStack(lisp);
}

CEnv1BFrame::~CEnv1BFrame() {
	CLispEng& lisp = Lisp();
	lisp.m_env.m_blockEnv = GetStackP(lisp)[1];
	base::ReleaseStack(lisp);
}

CEnv1GFrame::CEnv1GFrame() {
	CLispEng& lisp = Lisp();
	lisp.Push(lisp.m_env.m_goEnv);
	Finish(lisp, FT_ENV1G, lisp.m_pStack+1);
}

CEnv1GFrame::~CEnv1GFrame() {
	CLispEng& lisp = Lisp();
	lisp.m_env.m_goEnv = GetStackP(lisp)[1];
	base::ReleaseStack(lisp);
}

CEnv1DFrame::CEnv1DFrame() {
	CLispEng& lisp = Lisp();
	lisp.Push(lisp.m_env.m_declEnv);
	Finish(lisp, FT_ENV1D, lisp.m_pStack+1);
}

CEnv1DFrame::~CEnv1DFrame() {
	CLispEng& lisp = Lisp();
	lisp.m_env.m_declEnv = GetStackP(lisp)[1];
	base::ReleaseStack(lisp);
}

CEnv2VDFrame::CEnv2VDFrame() {
	CLispEng& lisp = Lisp();
	lisp.Push(lisp.m_env.m_declEnv);
	lisp.Push(lisp.m_env.m_varEnv);
	Finish(lisp, FT_ENV2VD, lisp.m_pStack+2);
}

CEnv2VDFrame::~CEnv2VDFrame() {
	CLispEng& lisp = Lisp();
	CP *p = GetStackP(lisp)+1;
	lisp.m_env.m_varEnv = *p++;
	lisp.m_env.m_declEnv = *p;
	base::ReleaseStack(lisp);
}

CDynBindFrame::CDynBindFrame(CP syms, CP vals) {
	CLispEng& lisp = Lisp();
	int count = 0;
	for (CP sym; lisp.SplitPair(syms, sym); lisp.Inc(vals), count++)
		Bind(lisp, sym, vals ? Car(vals) : V_U);
	Finish(lisp, count);
}

CDynBindFrame::CDynBindFrame(CP sym, CP val, bool bBind) {
	CLispEng& lisp = Lisp();
	if (bBind)
		Bind(lisp, sym, val);
	Finish(lisp, bBind);
}

CDynBindFrame::~CDynBindFrame() {
	CLispEng& lisp = Lisp();
	CP *stackP = GetStackP(lisp);
	CP *top = AsFrameTop(stackP);
	for (CP *cur = stackP+1; cur!=top; cur+=2)
		lisp.SetSymVal(cur[0], cur[1]);
	base::ReleaseStack(lisp);
}

void CDynBindFrame::Bind(CLispEng& lisp, CP sym, CP val) {
	lisp.Push(lisp.ToVariableSymbol(sym)->m_dynValue, sym); 
	lisp.SetSymVal(sym, val);		//!!! check constant
}

void CDynBindFrame::Finish(CLispEng& lisp, size_t count) {
	base::Finish(lisp, FT_DYNBIND, lisp.m_pStack+count*2);
}

CVarFrame::~CVarFrame() {
	CLispEng& lisp = Lisp();
	CP *top = AsFrameTop(GetStackP(lisp));
	for (; m_pVars!=top; m_pVars+=2) {
		CP symFlags = *m_pVars;
		if ((symFlags & (FLAG_ACTIVE|FLAG_DYNAMIC)) == (FLAG_ACTIVE|FLAG_DYNAMIC))
			lisp.SetSymVal(symFlags & MASK_WITHOUT_FLAGS, m_pVars[1]);					// symFlags already checked to be TS_SYMBOL during Bind
	}
	base::ReleaseStack(lisp);
}

bool __fastcall CVarFrame::Bind(CP val) {
	m_pVars -= 2;
	CP& sym = m_pVars[0];
	/*!!!  if (Lisp().m_bTrace) //!!!
	{
	ostream os(new CTextStreambuf(Lisp().m_arStream[CLisp::STM_TraceOutput]));//!!!
	os << "VALUE of ";
	Lisp().PrintForm(os, sym & MASK_WITHOUT_FLAGS);
	os << " is ";
	Lisp().PrintForm(os, val);
	os << "\n\n";
	}*/
	bool r = sym & FLAG_SP;
	sym &= ~FLAG_SP;
	m_pVars[1] = CLispEng::StaticSwapIfDynSymVal(sym, val);
	sym |= FLAG_ACTIVE;
	return r;
}

CCatchFrame::CCatchFrame(CLispEng& lisp, CP tag) {
	lisp.Push(tag);
	lisp.Push(CreateFixnum(lisp.m_pSPTop-lisp.m_pSP));
	Finish(lisp, FT_CATCH, lisp.m_pStack+2);
}

CFunFrame::CFunFrame(CP *pTop) {
	CLispEng& lisp = Lisp();  
	lisp.Push(lisp.m_env.m_funEnv);
	lisp.Push(CreateFixnum((pTop-lisp.m_pStack-1)/2));
	Finish(lisp, FT_FUN, pTop);
}

} // Lisp::


