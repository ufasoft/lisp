#include <el/ext.h>


#include "lispeng.h"

namespace Lisp {

#if UCFG_LISP_CHECK_STACK
#	define ASSERT_FRAME(ft) {if (AsFrameType(SV) != ft) Throw(E_FAIL); }
#else
#	define ASSERT_FRAME(ft)
#endif


 void CLispEng::InterpretBytecode(CP closure, ssize_t offset) {		// to save stack  // _forceinline
	Push(closure);

	#ifdef X_DEBUG//!!!D
	if (StackOverflowAddress) {
		ostringstream os;
		Print(os, TheClosure(closure).NameOrClassVersion);
		String name = os.str();
		cerr << name << endl;
	}
	#endif

#ifdef X_DEBUG//!!!D
	static int s_cnt;
	CArrayValue *avDbg = ToVector(TheClosure(closure).CodeVec);
	if (avDbg->m_dims == 0xb919) {
		if (++s_cnt == 7)
			avDbg = avDbg;
	}
#endif

	CVMContextBase vmContext = { closure, (byte*)ToVector(TheClosure(closure).CodeVec)->m_pData+offset };
	CurVMContext.reset(&vmContext);

#if UCFG_LISP_PROFILE
	PROF_POINT(TheClosure(closure).m_profInfo);
//!!! m_pb = (byte*)ToVector(pClos->CodeVec)->m_pData+offset;
#else
//!!!	m_pb = (byte*)ToVector(TheClosure(CurClosure).CodeVec)->m_pData+offset;
#endif
	ContinueBytecode();
}

int CLispEng::ReadU() {
	byte b = *CurVMContext->m_pb++;
	if (b & 0x80)
		return ((b & 0x7F)<<8) | *CurVMContext->m_pb++;
	else
		return b;  
}

int CLispEng::ReadS() {
	byte *pb = CurVMContext->m_pb;
	int r = *pb++;
	if (r & 0x80) {
		short s = ((r & 0x7F)<<8) | *pb++;
		r = s ? int16_t(uint16_t(s) << 1) >> 1 : 
				(int32_t)_byteswap_ulong(GetLeUInt32((pb += 4) - 4));		
	} else
		r = int16_t(r << 9) >> 9;
	CurVMContext->m_pb = pb;
	return r;
}

void CLispEng::IgnoreS() {
	byte b = *CurVMContext->m_pb++;
	if ((b & 0x80) && !((b << 1) | *CurVMContext->m_pb++))
		CurVMContext->m_pb += 4;  
}

byte *CLispEng::ReadL() {
	int s = ReadS();
	return CurVMContext->m_pb+s;
}

size_t CLispEng::ReadOffset() {
	int s = ReadS();
	return CurOffset+s;
}

void CLispEng::JmpIf(bool b) {
	if (b)
		CurVMContext->m_pb = ReadL();
	else
		IgnoreL();
}

void CLispEng::C_Nil() {
	m_r = 0;
	m_cVal = 1;
}

void CLispEng::C_Nil_Push() {
	Push(0);
}

void CLispEng::C_Push_Nil() {
	int i = ReadU();
	while (i--)
		Push(0);
}

void CLispEng::C_T() {
	m_r = V_T;
	m_cVal = 1;
}

void CLispEng::C_T_Push() {
	Push(V_T);
}

void CLispEng::C_Const() {
	m_r = Const(ReadU());
	m_cVal = 1;
}

void CLispEng::C_Const_Push() {
	Push(Const(ReadU()));
}

void CLispEng::C_Load() {
	m_r = GetStack(ReadU());
	m_cVal = 1;
}

void CLispEng::C_LoadPush() {
	Push(GetStack(ReadU()));
}

CP& CLispEng::ReadKKN() {
	uint32_t k1 = ReadU(),
		k2 = ReadU(),
		n = ReadU();
	return GetFrame(k1)[n];
}

void CLispEng::C_Loadi() {
	m_r = ReadKKN();
	m_cVal = 1;
}

void CLispEng::C_Loadi_Push() {
#ifdef X_DEBUG//!!!D
	if (g_b) {
		static int s_i;
		s_i++;		
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "    C_Loadi_Push(" << s_i << ") " << (void*)m_pSP << endl;
		if (s_i == 82)
			Disassemble(cerr, CurClosure);
	}
#endif
	Push(ReadKKN());
}

CP CLispEng::C_LoadcHelper(CP pvec) {
	return ToVector(pvec)->GetElement(ReadU()+1);
}

void CLispEng::C_Loadc() {
	m_r = C_LoadcHelper(GetStack(ReadU()));
	m_cVal = 1;
}

void CLispEng::C_Loadc_Push() {
	Push(C_LoadcHelper(GetStack(ReadU())));
}

void CLispEng::C_Loadic() {
	m_r = C_LoadcHelper(ReadKKN());
	m_cVal = 1;
}

void CLispEng::C_Loadv() {
	CArrayValue *vec = ToArray(TheCurClosure().VEnv);
	for (int i=ReadU(); i--;)
		vec = ToArray(vec->m_pData[0]);
	m_r = vec->m_pData[ReadU()];
	m_cVal = 1;
}

void CLispEng::C_Loadv_Push() {
	C_Loadv();
	Push(m_r);
}

void CLispEng::C_Store() {
	SetStack(ReadU(), m_r);
	m_cVal = 1;
}

void CLispEng::C_Pop_Store() {
	SetStack(ReadU(), m_r=Pop());
}

void CLispEng::C_Storei() {
	ReadKKN() = m_r;
}

void CLispEng::C_StorecHelper(CP pvec) {
	ToVector(pvec)->SetElement(ReadU()+1, m_r);
	m_cVal = 1;
}

void CLispEng::C_Storec() {
	C_StorecHelper(GetStack(ReadU()));
}

void CLispEng::C_Load_Storec() {
	C_Load();
	C_Storec();
}

void CLispEng::C_Storeic() {
	C_StorecHelper(ReadKKN());
}

void CLispEng::C_Storev() {
	CArrayValue *vec = ToArray(TheCurClosure().VEnv);
	for (int i=ReadU(); i--;)
		vec = ToArray(vec->m_pData[0]);
	vec->m_pData[ReadU()] = m_r;
	m_cVal = 1;
}

void CLispEng::C_GetValue() {
	m_r = SymbolValue(Const(ReadU()));
	m_cVal = 1;
}

void CLispEng::C_GetValue_Push() {
	Push(SymbolValue(Const(ReadU())));
}

void CLispEng::C_SetValue() {
	CP sym = Const(ReadU());
	ToVariableSymbol(sym);
	SetSymVal(sym, m_r);
	m_cVal = 1;
}

void CLispEng::C_Bind() {
	CDynBindFrame dynBind(Const(ReadU()), m_r, true);
	ContinueBytecode();
}

void CLispEng::C_Unbind1() {
	m_nRestBinds = 1;
}

void CLispEng::C_Unbind() {
	m_nRestBinds = ReadU();
}

void CLispEng::C_Progv() {
	CP syms = Pop();
	PushSPStack();
	CDynBindFrame dynBind(syms, m_r);
	ContinueBytecode();
}

void CLispEng::C_Push() {
	Push(m_r);
}

void CLispEng::C_Pop() {
	m_r = Pop();
	m_cVal = 1;
}

void CLispEng::C_Skip() {
	SkipStack(ReadU());
}

void CLispEng::C_Skipi() {
	int k = ReadU();
	ReadU(); //!!! ignore k2
	m_pStack = GetFrame(k);
	SkipSP(k+1);
	SkipStack(ReadU());
}

void CLispEng::C_SkipSP() {
	SkipSP(ReadU());
	if (int k2 = ReadU()) {
		for (CJmpBufBase *p = m_pJmpBuf; p; p=p->m_pNext)
			if (!--k2) {
				m_bUnwinding = true;
				LISP_THROW(p);
			}
		Throw(E_FAIL); //!!! unreachable
	}
}

void CLispEng::C_Jmp() {
	CurVMContext->m_pb = ReadL();
}

void CLispEng::C_JmpIf() {
	JmpIf(m_r);
}

void CLispEng::C_JmpIfNot() {
	JmpIf(!m_r);
}

void CLispEng::C_JmpIf1() {
	JmpIf(m_r);
	m_cVal = 1;
}

void CLispEng::C_JmpIfNot1() {
	JmpIf(!m_r);
	m_cVal = 1;
}

void CLispEng::C_JmpIfAtom() {
	JmpIf(Type(m_r) != TS_CONS || !m_r); //!!!
}

void CLispEng::C_JmpIfConsp() {
	JmpIf(ConsP(m_r));
}

void CLispEng::C_JmpIfEq() {
	JmpIf(Pop() == m_r);
}

void CLispEng::C_JmpIfNotEq() {
	JmpIf(Pop() != m_r);
}

void CLispEng::C_JmpIfEqTo() {
	JmpIf(Pop() == Const(ReadU()));
}

void CLispEng::C_JmpIfNotEqTo() {
	JmpIf(Pop() != Const(ReadU()));
}

void CLispEng::JmpHash(CP consts) {
	Push(m_r, AsArray(consts)->GetElement(ReadU()), V_U);
	F_GetHash();
	if (!m_arVal[1])
		C_Jmp();
	else
		CurVMContext->m_pb += AsNumber(m_r);
}

void CLispEng::C_JmpHash() {
	JmpHash(CurClosure);
}

void CLispEng::C_JmpHashv() {
	JmpHash(Const(0));
}

void CLispEng::C_Jsr() {
	//	byte *b = ReadL(); //!!!
	size_t off = ReadOffset();
	
	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	InterpretBytecode(CurClosure, off);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_JsrPush() {
	C_Jsr();
	Push(m_r);
}

void CLispEng::C_JmpTail() {
	int m = ReadU(),
		n = ReadU();
	for (int i=m; i--;)
		m_pStack[n-m+i] = m_pStack[i];
	SkipStack(n-m);
	Push(CurClosure);
	C_Jmp();
}

void CLispEng::C_Venv() {
	m_r = TheCurClosure().VEnv;
	m_cVal = 1;
}

void CLispEng::C_MakeVector1_Push() {
	CArrayValue *vv = CreateVector(ReadU()+1);
	vv->m_pData[0] = m_r;
	Push(FromSValue(vv));
}

void CLispEng::C_CopyClosure() {
	CClosure *nc = CopyClosure(Const(ReadU()));
	CP *p = nc->Consts;
	for (int i=ReadU(); i--;)
		p[i] = Pop();
	m_r = FromSValueT(nc, TS_CCLOSURE);
	m_cVal = 1;
}

void CLispEng::C_CopyClosure_Push() {
	C_CopyClosure();
	Push(m_r);
}

void CLispEng::C_Call() {
	int k = ReadU(),
		n = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Funcall(Const(n), k);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Call_Push() {
	C_Call();
	Push(m_r);
}

void CLispEng::C_Call0() {
	int n = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Funcall(Const(n), 0);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Call1() {
	int n = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
#ifdef _X_DEBUG //!!!D
		if (n == 0x1C && CurClosure==0x560B)
			n = n;
#endif
	Funcall(Const(n), 1);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Call1_Push() {
	C_Call1();
	Push(m_r);
}

void CLispEng::C_Call2() {
	int n = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
#ifdef _X_DEBUG //!!!D
		if (CurClosure == 0x1D0B && n==2)
			Disassemble(cerr, CurClosure);
#endif
	Funcall(Const(n), 2);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Call2_Push() {
	C_Call2();
	Push(m_r);
}

void CLispEng::Calls(size_t idx) {
#ifdef _X_DEBUG//!!!D
	CP *pStack = m_pStack;
#endif
	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	ClearResult();
	PROF_POINT(m_arSubrProfile[idx]);
	(this->*s_stFuncAddrs[idx])();
	VM_RESTORE_CONTEXT;

#ifdef _X_DEBUG//!!!D
	CLispFunc& f = s_stFuncInfo[idx];
	if (!f.m_keywords && (m_pStack-pStack != f.m_nReq+f.m_nOpt))
		idx = idx;
#endif
}

void CLispEng::C_Calls1() {
	Calls(ReadB());
}

void CLispEng::C_Calls1_Push() {
	C_Calls1();
	Push(m_r);
}

void CLispEng::C_Calls2() {
	Calls(ReadB()+256);
}

void CLispEng::C_Calls2_Push() {
	C_Calls2();
	Push(m_r);
}

void CLispEng::C_Callsr() {
	int m = ReadU();
	byte b = ReadB();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	ClearResult();
	PROF_POINT(m_arSubrProfile[b+SUBR_FUNCS]);
	(this->*s_stFuncRAddrs[b])(m);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Callsr_Push() {
	C_Callsr();
	Push(m_r);
}

void CLispEng::C_Callc() {
	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	InterpretBytecode(m_r, CCV_START_NONKEY);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Callc_Push() {
	C_Callc();
	Push(m_r);
}

void CLispEng::C_CallcKey() {
	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	InterpretBytecode(m_r, CCV_START_KEY);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_CallcKey_Push() {
	C_CallcKey();
	Push(m_r);
}

void CLispEng::C_Funcall() {
	int n = ReadU();
	
	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Funcall(GetStack(n), n);
	m_pStack++;
	VM_RESTORE_CONTEXT;

#ifdef X_DEBUG//!!!D
	if (g_b == 2)
		Disassemble(cerr, CurVMContext->m_closure);
#endif
}

void CLispEng::C_Funcall_Push() {
	C_Funcall();
	Push(m_r);
}

void CLispEng::C_Apply() {
	int n = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Apply(GetStack(n), n, m_r);
	SkipStack(1);
	VM_RESTORE_CONTEXT;
}

void CLispEng::C_Apply_Push() {
	C_Apply();
	Push(m_r);
}

void CLispEng::C_Push_Unbound() {
	PushUnbounds(ReadU());
}

void CLispEng::C_List() {
	m_r = Listof(ReadU());
	m_cVal = 1;
}

void CLispEng::C_List_Push() {
	Push(Listof(ReadU()));
}

void CLispEng::C_ListStern() {
	Push(m_r);
	m_r = ListofEx(ReadU()+1);
	m_cVal = 1;
}

void CLispEng::C_ListStern_Push() {
	Push(m_r);
	Push(ListofEx(ReadU()+1));
}

void CLispEng::C_Unlist() {
	int n = ReadU();
	for (CP car; n && SplitPair(m_r, car); n--)
		Push(car);
	if (n > ReadU())
		E_SeriousCondition();
	PushUnbounds(n);
}

void CLispEng::C_UnlistStern() {
	C_Unlist();
	Push(m_r);
}

void CLispEng::C_JmpIfBoundp() {
	m_r = GetStack(ReadU());
	if (m_r == V_U)
		IgnoreL();
	else {
		CurVMContext->m_pb = ReadL();
		m_cVal = 1;
	}
}

void CLispEng::C_Boundp() {
	GetStack(ReadU()) == V_U ? C_Nil() : C_T();
}

void CLispEng::C_Unbound_Nil() {
	int n = ReadU();
	if (GetStack(n) == V_U)
		SetStack(n, 0);
}

void CLispEng::C_Values0() {
	m_r = 0;
	m_cVal = 0;
}

void CLispEng::C_Values1() {
	m_cVal = 1;
}

void CLispEng::C_StackToMv() {
	int n = ReadU();
	StackToMv(n);
}

void CLispEng::C_MvToStack() {
	MvToStack();
}

void CLispEng::C_NvToStack() {
	size_t count = (size_t)ReadU();
	for (size_t i=0; i<count; ++i)
		Push(i<m_cVal ? m_arVal[i] : 0);
}

void CLispEng::C_MvToList() {
	MvToList();
}

void CLispEng::C_ListToMv() {
	ListToMv(m_r);
}

/*!!!RD
void CLispEng::SkipSP(int n)  {
	if (g_b) {
		static int s_i;
		++s_i;
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "   SkipSP " << n << " (" << s_i << ")   " << (void*)m_pSP << endl;
	}

	m_pSP += n;
}

CP *CLispEng::PopSPFrame() { 
	if (g_b) {
		static int s_i;
		++s_i;
		cerr << "   PopSPFrame(" << s_i << ")   " << (void*)m_pSP << endl;
	}

	return m_pStackTop-*m_pSP++;
}

void CLispEng::PushSP(ssize_t v) {
	if (g_b) {
		static int s_i;
		++s_i;

		Print(TheClosure(CurClosure).NameOrClassVersion);
		cerr << "   PushSP(" << s_i << ")   " << (void*)m_pSP << endl;
	}
	*--m_pSP = v;
}
*/

void CLispEng::C_MvCallp() {
	PushSPStack();
	Push(m_r);
#ifdef X_DEBUG//!!!D
	if (g_b) {
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "   MvCallp " << (void*)m_pSP << endl;
//		Disassemble(cerr, CurClosure);
	}
#endif
}

void CLispEng::C_MvCall() {
#ifdef X_DEBUG//!!!D
	if (g_b) {
		static int s_i;
		++s_i;
//		if (s_i == 3)
//			Disassemble(cerr, CurClosure);
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "    MvCall(" << s_i << ")  " << (void*)m_pSP << endl;
	}
#endif
	CP *frame = PopSPFrame();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Funcall(frame[-1], frame-m_pStack-1);
	SkipStack(1);
	VM_RESTORE_CONTEXT;
}

class CCBlockFrame : public CJmpBuf {
public:

#ifdef X_DEBUG//!!!D
	CLispEng::CIndentKeeper m_indentKeeper;
#endif


	CCBlockFrame(CLispEng& lisp, CP name) {
		lisp.Push(0);
		lisp.Push(CreateFixnum(lisp.m_pSPTop-lisp.m_pSP));
		Finish(lisp, FT_CBLOCK, lisp.m_pStack+2);
		lisp.SV2 = lisp.Cons(name, lisp.CreateFramePtr(GetStackP(lisp)));
	}

	~CCBlockFrame() {
#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;
			CLispEng& lisp = Lisp();

			lisp.Print(ToCClosure(lisp.CurClosure)->NameOrClassVersion);
			cerr << "  ~CCBlockFrame(" << s_i << ")" << endl;
		}
#endif

		CLispEng& lisp = Lisp();		
		lisp.ToCons(GetStackP(lisp)[2])->m_cdr = V_D;
	}
};

void CLispEng::C_BlockOpen() {
	CCBlockFrame blockFrame(_self, Const(ReadU()));
	byte *pb = ReadL();
//!!!	ssize_t off = CurOffset;	

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	LISP_TRY(blockFrame) {
		ContinueBytecode();
		Throw(E_FAIL); //!!!
	} LISP_CATCH(blockFrame) {
		blockFrame.RestoreStacks();

#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;

			Print(ToCClosure(CurClosure)->NameOrClassVersion);
			cerr << "  RestoredStacks in CCBlockFrame(" << s_i << ")  " << m_pSP << endl;
		}
#endif
	} LISP_CATCH_END
	VM_RESTORE_CONTEXT;
	CurVMContext->m_pb = pb;
}

void CLispEng::C_BlockClose() {
	ASSERT_FRAME(FT_CBLOCK);

	UnwindTo(m_pStack);
}

void CLispEng::ReturnFrom(CP cons) {
	CP p = Cdr(cons);
	if (p == V_D)
		E_ControlErr(IDS_E_BlockHasLeft, Car(cons));
	UnwindTo(ToFrame(p)); 
}

void CLispEng::C_ReturnFrom() {
	ReturnFrom(Const(ReadU()));
}

void CLispEng::C_ReturnFromI() {
	ReturnFrom(ReadKKN());
}

class CCTagbodyFrame : public CJmpBuf {
public:
	~CCTagbodyFrame() {
		CLispEng& lisp = Lisp();		
		lisp.ToCons(GetStackP(lisp)[2])->m_cdr = V_D;
	}
};

void CLispEng::C_TagbodyOpen() {
	CCTagbodyFrame tagbodyFrame;
	CArrayValue *vec = ToVector(Const(ReadU()));
	size_t m = vec->GetVectorLength();
	for (size_t i=m; i--;)
		Push(CreateInteger((int)ReadOffset()));
	//!!!D		Push(CreateInteger(ReadL()-TheCurClosure().m_code));
	Push(0);
	//!!! was PushSP(closure)
	Push(CreateInteger(int(m_pSPTop-m_pSP)));
	tagbodyFrame.Finish(_self, FT_CTAGBODY, m_pStack+m+2);
	SV2 = Cons(FromSValue(vec), CreateFramePtr(m_pStack));
	ssize_t off; //!!!R = CurOffset;
//!!!R	VM_SAVE_CONTEXT;
	LISP_TRY(tagbodyFrame) {
		VM_CONTEXT;
		ContinueBytecode();
//!!!R		VM_RESTORE_CONTEXT;
		goto LAB_RET;
	} LISP_CATCH(tagbodyFrame) {
//!!!R		VM_RESTORE_CONTEXT;
		tagbodyFrame.RestoreStacks();

#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;

			Print(ToCClosure(CurClosure)->NameOrClassVersion);
			cerr << "  RestoredStacks in CCTagbodyFrame(" << s_i << ")  " << m_pSP << endl;
		}
#endif

		off = AsPositive(GetStack(m-AsPositive(m_r)+3));
		CurVMContext->m_pb = (byte*)ToVector(TheClosure(CurClosure).CodeVec)->m_pData+off;
	} LISP_CATCH_END
	{
//!!!R		VM_SAVE_CONTEXT;
		VM_CONTEXT;					//!!!? Should this code be protected by try
		ContinueBytecode();
	//!!!R	VM_RESTORE_CONTEXT;
	}
LAB_RET:
	;//!!!R	CurVMContext->m_pb = b;
}

void CLispEng::C_TagbodyClose() {
	ASSERT_FRAME(FT_CTAGBODY);

	CP p = SV2;
	ToCons(p)->m_cdr = V_D;


	//  SkipStack(3+ToVector(Car(p))->GetVectorLength());
	//!!!  m_pSP += 2;
}

void CLispEng::C_TagbodyClose_Nil() {
	ClearResult();
	C_TagbodyClose();
}

void CLispEng::Go(CP cons) {
	CP p = Cdr(cons);
	int lab = ReadU();
	if (p == V_D)
		E_ProgramErr();
	CP *frame = ToFrame(p);
	m_r = AsFrameType(frame[0])==FT_CTAGBODY ? CreateInteger(lab+1) : frame[FRAME_TAG_BINDINGS+2*lab+1];
	UnwindTo(frame);
}

void CLispEng::C_Go() {
	Go(Const(ReadU()));
}

void CLispEng::C_GoI() {
	Go(ReadKKN());
}

void CLispEng::C_CatchOpen() {
	byte *b = ReadL();
	CCatchFrame catchFrame(_self, m_r);
	VM_SAVE_CONTEXT;
	LISP_TRY(catchFrame) {
		VM_CONTEXT;
		ContinueBytecode();
		return;
	} LISP_CATCH(catchFrame) {
		catchFrame.RestoreStacks();

#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;

			Print(ToCClosure(CurClosure)->NameOrClassVersion);
			cerr << "  RestoredStacks in CCatchFrame(" << s_i << ")  " << m_pSP << endl;
		}
#endif

	} LISP_CATCH_END;
	VM_RESTORE_CONTEXT;
	CurVMContext->m_pb = b;
}

void CLispEng::C_CatchClose() {
	ASSERT_FRAME(FT_CATCH);
}

void CLispEng::C_Throw() {
	ThrowTo(Pop());
}

class CCUnwindProtectFrame : public CFrameBaseEx {
	CVMContextBase *m_pVMContext;
	ssize_t m_off;
public:
	CCUnwindProtectFrame(ssize_t off)
		:	m_off(off)
	{
		CLispEng& lisp = Lisp();

#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;

			Lisp().Print(ToCClosure(Lisp().CurClosure)->NameOrClassVersion);
			cerr << "  setting CUnwindProtectFrame(" << s_i << ")  " << Lisp().m_pSP << endl;
		}
#endif

		m_pVMContext = lisp.CurVMContext;
		lisp.Push(lisp.CreateInteger(int(lisp.m_pSPTop-lisp.m_pSP)));
		lisp.PushSP(0); //!!! msut be VM_CONTEXT
		lisp.PushSP(0);
		Finish(lisp, FT_UNWINDPROTECT, lisp.m_pStack+1);
	}

	~CCUnwindProtectFrame() {
		RestoreStacks();

#ifdef X_DEBUG//!!!D
		if (g_b) {
			static int s_i;
			++s_i;

			Lisp().Print(ToCClosure(Lisp().CurClosure)->NameOrClassVersion);
			cerr << "  RestoredStacks in CUnwindProtectFrame(" << s_i << ")  " << Lisp().m_pSP << endl;
		}
#endif

		ReleaseStack();
		CLispEng& lisp = Lisp();
		lisp.PushSPStack();
//		byte *b = lisp.CurVMContext->m_pb;
		size_t cVal = lisp.m_cVal;
		lisp.MvToStack();
//!!!		CVMContextBase *_curContext = lisp.CurVMContext;
		lisp.CurVMContext.reset(m_pVMContext);
		lisp.CurVMContext->m_pb = (byte*)lisp.ToVector(lisp.TheClosure(lisp.CurClosure).CodeVec)->m_pData+m_off;
		lisp.ContinueBytecode();
		lisp.StackToMv(cVal);
//		if (b)
//			lisp.CurVMContext->m_pb = b;
		lisp.SkipSP(1); //!!!
	}
};

void CLispEng::C_UwpOpen() {
#ifdef _X_DEBUG //!!!D
	Disassemble(cerr, CurClosure);
#endif
	CCUnwindProtectFrame uwpFrame(ReadOffset());
	ContinueBytecode();	
}

void CLispEng::C_UwpNormalExit() {
	ASSERT_FRAME(FT_UNWINDPROTECT);
	CurVMContext->m_pb = 0;
}

void CLispEng::C_UwpClose() {
}

void CLispEng::C_UwpCleanup() {
	ASSERT_FRAME(FT_UNWINDPROTECT);
}

class CHandlerFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CHandlerFrame(CP closure, CP handlers) {
		CLispEng& lisp = Lisp();
		lisp.Push(handlers, closure);
		lisp.Push(0); //!!! for compatibility with CLISP/compiler
		Finish(lisp, FT_HANDLER, lisp.m_pStack+3);
	}

	~CHandlerFrame() {
		base::ReleaseStack(Lisp());
	}
};

void CLispEng::C_HandlerOpen() {
#ifdef X_DEBUG//!!!D
	if (g_b) {
		static int s_i;
		++s_i;
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "   C_HandlerOpen(" << s_i << ")   " << (void*)m_pSP << endl;
	}
#endif

	//!!!CHandlerFrame handlerFrame(CurClosure, Const(ReadU()));
	Push(Const(ReadU()), CurClosure);
	Push(CreateInteger(int(m_pSPTop-m_pSP)));
	Push(CreateFrameInfo(FT_HANDLER, m_pStack+3));
}

void CLispEng::C_HandlerBegin_Push() {
	size_t count = AsPositive(Car(m_spDepth));

#ifdef X_DEBUG//!!!D
	if (g_b) {
		static int s_i;
		++s_i;
		Print(ToCClosure(CurClosure)->NameOrClassVersion);
		cerr << "   C_HandlerBegin_Push(" << s_i << ")   " << (void*)m_pSP << "  prevSP= " << (m_pSPTop-m_spOff) << endl;
		if (s_i == 1)
 			Disassemble(cerr, CurClosure);
	}
#endif

	memcpy(m_pSP-=count, m_pSPTop-m_spOff-count, sizeof(int)*count);
	PushSP(m_stackOff);
	Push(m_r = m_cond);
}

void CLispEng::C_Not() {
	m_r ? C_Nil() : C_T();
}

void CLispEng::C_Eq() {
	m_r == Pop() ? C_T() : C_Nil();
}

void CLispEng::C_Car() {
	m_r = Car(m_r);
	m_cVal = 1;
}

void CLispEng::C_Car_Push() {
	Push(Car(m_r));
}

void CLispEng::C_Load_Car_Push() {
	Push(Car(GetStack(ReadU())));
}

void CLispEng::C_Load_Car_Store() {
	int m = ReadU(),
		n = ReadU();
	SetStack(n, m_r=Car(GetStack(m)));
	m_cVal = 1;
}

void CLispEng::C_Cdr() {
	m_r = Cdr(m_r);
	m_cVal = 1;
}

void CLispEng::C_Cdr_Push() {
	Push(Cdr(m_r));
}

void CLispEng::C_Load_Cdr_Push() {
	Push(Cdr(GetStack(ReadU())));
}

void CLispEng::C_Load_Cdr_Store() {
	int n = ReadU();
	SetStack(n, m_r=Cdr(GetStack(n)));
	m_cVal = 1;
}

void CLispEng::C_Cons() {
	m_r = Cons(Pop(), m_r);
	m_cVal = 1;
}

void CLispEng::C_Cons_Push() {
	SV = Cons(SV, m_r);
}

void CLispEng::C_Load_Cons_Store() {
	int n = ReadU();
	CP p = Pop();
	SetStack(n, m_r=Cons(p, GetStack(n)));
	m_cVal = 1;
}

void CLispEng::C_SymbolFunction() {
	if (CP r = ToSymbol(m_r)->GetFun()) {
		m_r = r;
		m_cVal = 1;
	} else
		E_UndefinedFunction(m_r);
}

void CLispEng::C_Const_SymbolFunction() {
	m_r = Const(ReadU());
	C_SymbolFunction();
}

void CLispEng::C_Const_SymbolFunction_Push() {
	CP sym = Const(ReadU());
	if (CP fun = ToSymbol(sym)->GetFun())
		Push(fun);
	else
		E_UndefinedFunction(sym);
}

void CLispEng::C_ConstSymbolFunctionStore() {
	CP sym = Const(ReadU());
	if (m_r = ToSymbol(sym)->GetFun()) {
		SetStack(ReadU(), m_r);
		m_cVal = 1;
	} else
		E_UndefinedFunction(sym);
}

void CLispEng::C_SvRef() {
	m_r = ToArray(Pop())->GetElement(AsNumber(m_r));
	m_cVal = 1;
}

void CLispEng::C_SvSet() {
	CArrayValue *av = ToArray(Pop());
	size_t idx = AsPositive(m_r);
	av->SetElement(idx, m_r=Pop());
	m_cVal = 1;
}

void CLispEng::C_Nil_Store() {
	SetStack(ReadU(), m_r=0);
	m_cVal = 1;
}

void CLispEng::C_T_Store() {
	SetStack(ReadU(), m_r=V_T);
	m_cVal = 1;
}

void CLispEng::C_Calls1_Store() {
	C_Calls1();
	C_Store();
}

void CLispEng::C_Calls2_Store() {
	C_Calls2();
	C_Store();
}

void CLispEng::C_Callsr_Store() {
	C_Callsr();
	C_Store();
}

CSPtr CLispEng::IncNumber(CP p) {
	Push(p);
	F_Inc();
	return m_r;
}

CSPtr CLispEng::DecNumber(CP p) {
	Push(p);
	F_Dec();
	return m_r;
}

void CLispEng::C_Load_Inc_Push() {
	Push(IncNumber(GetStack(ReadU())));
}

void CLispEng::C_Load_Inc_Store() {
	int n = ReadU();
	SetStack(n, m_r=IncNumber(GetStack(n)));
	m_cVal = 1;
}

void CLispEng::C_Load_Dec_Push() {
	Push(DecNumber(GetStack(ReadU())));
}

void CLispEng::C_Load_Dec_Store() {
	int n = ReadU();
	SetStack(n, m_r=DecNumber(GetStack(n)));
	m_cVal = 1;
}

void CLispEng::C_Call1_JmpIf() {
	C_Call1();
	C_JmpIf();
}

void CLispEng::C_Call1_JmpIfNot() {
	C_Call1();
	C_JmpIfNot();
}

void CLispEng::C_Call2_JmpIf() {
	C_Call2();
	C_JmpIf();
}

void CLispEng::C_Call2_JmpIfNot() {
	C_Call2();
	C_JmpIfNot();
}

#ifdef X_DEBUG//!!!D
int s_cntCalls1;

#endif

void CLispEng::C_Calls1_JmpIf() {
/*!!!D	if (++s_cntCalls1 == 8)
		s_cntCalls1 = s_cntCalls1; */

	C_Calls1();
	C_JmpIf();
}

void CLispEng::C_Calls1_JmpIfNot() {
	C_Calls1();
	C_JmpIfNot();
}

void CLispEng::C_Calls2_JmpIf() {
	C_Calls2();
	C_JmpIf();
}

void CLispEng::C_Calls2_JmpIfNot() {
	C_Calls2();
	C_JmpIfNot();
}

void CLispEng::C_Callsr_JmpIf() {
	C_Callsr();
	C_JmpIf();
}

void CLispEng::C_Callsr_JmpIfNot() {
	C_Callsr();
	C_JmpIfNot();
}

void CLispEng::C_Load_JmpIf() {
	C_Load();
	C_JmpIf();
}

void CLispEng::C_Load_JmpIfNot() {
	C_Load();
	C_JmpIfNot();
}

void CLispEng::Skip_RetGF(CP closure, int n) {
	CClosure& c = TheClosure(closure);
	int r = c.NumReq;
	n -= r;
	if (c.Flags & 1) {
		SkipStack(n-1);
		Apply(m_r, r, Pop());
	} else {
		SkipStack(n);
		Apply(m_r, r);
	}
}

void CLispEng::C_Skip_RetGF() {
	int n = ReadU();
	if (TheCurClosure().Flags & 8) {
		SkipStack(n);
		m_cVal = 1;
	} else
		Skip_RetGF(CurClosure, n);
}

void CLispEng::C_Apply_Skip_Ret() {
	int n = ReadU(),
		k = ReadU();

	VM_SAVE_CONTEXT;
	VM_CONTEXT;
	Apply(GetStack(n), n, m_r);
	VM_RESTORE_CONTEXT;

	m_pStack += k+1;
}

void CLispEng::C_Funcall_Skip_RetGF() {
	int n = ReadU(),
		k = ReadU();
	//!!!CP closure = CurClosure;
	{
		VM_SAVE_CONTEXT;
		VM_CONTEXT;
		Funcall(m_pStack[n], n);
		VM_RESTORE_CONTEXT;
	}
	Skip_RetGF(CurClosure, k+1);
}

void CLispEng::C_Load(int n) {
	m_r = GetStack(n);
	m_cVal = 1;
}

void CLispEng::C_Load_Push(int n) {
	Push(GetStack(n));
}

void CLispEng::C_Const(int n) {
	m_r = Const(n);
	m_cVal = 1;
}

void CLispEng::C_Const_Push(int n) {
	Push(Const(n));
}

void CLispEng::C_Store(int n) {
	SetStack(n, m_r);
	m_cVal = 1;
}

//!!!D #define ASSERT_PB ASSERT(CurVMContext->m_pb < (byte*)ToVector(TheClosure(CurVMContext->m_closure).CodeVec)->m_pData + (ToVector(TheClosure(CurVMContext->m_closure).CodeVec)->m_dims>>8)) //!!!D


void __fastcall CLispEng::ContinueBytecode() {

	//!!!	byte *pEndCode = (byte*)codevec->m_pData+codevec->GetVectorLength();
#ifdef X_DEBUG //!!!D
	static int s_i;
	if (++s_i >= 132000) {
		Print(TheClosure(CurVMContext->m_closure).NameOrClassVersion);
		cerr << "   ";
	}
#endif
#ifdef _X_DEBUG //!!!D
	if (CurClosure==0x5004) {
		Disassemble(cerr, CurClosure);
		m_pb = m_pb;
	}
#endif

#ifdef X_DEBUG//!!!D
	CArrayValue *avDbg = ToVector(TheClosure(CurVMContext->m_closure).CodeVec);
	const byte *upper = (byte*)avDbg->m_pData+(avDbg->m_dims>>8);
#endif

	while (true) {
#ifdef X_DEBUG//!!!D
		if (CurVMContext->m_pb >= upper)
			upper = upper;
		byte *prevPb = CurVMContext->m_pb;
#endif
		byte b = *CurVMContext->m_pb++;
		switch (CByteCode(b)) {
		case COD_NIL:
			C_Nil();
			break;
		case COD_PUSH_NIL:
			C_Push_Nil();
			break;    
		case COD_T:
			C_T();
			break;
		case COD_CONST:
			C_Const();
			break;
		case COD_LOAD:
			C_Load();
			break;
		case COD_LOADI:
			C_Loadi();
			break;
		case COD_LOADC:
			C_Loadc();
			break;
		case COD_LOADV:
			C_Loadv();
			break;
		case COD_LOADIC:
			C_Loadic();
			break;
		case COD_STORE:
			C_Store();
			break;
		case COD_STOREI:
			C_Storei();
			break;
		case COD_STOREC:
			C_Storec();
			break;
		case COD_STOREV:
			C_Storev();
			break;
		case COD_STOREIC:
			C_Storeic();
			break;
		case COD_GETVALUE:
			C_GetValue();
			break;
		case COD_SETVALUE:
			C_SetValue();
			break;
		case COD_BIND:
			C_Bind();
			if (--m_nRestBinds)
				goto LAB_RET;
			break;
		case COD_UNBIND1:
			C_Unbind1();
			goto LAB_RET;
		case COD_UNBIND:
			C_Unbind();
			goto LAB_RET;
		case COD_PROGV:
			C_Progv();
			break;
		case COD_PUSH:
			C_Push();
			break;
		case COD_POP:
			C_Pop();
			break;
		case COD_SKIP:
			C_Skip();
			break;
		case COD_SKIPI:
			C_Skipi();
			break;
		case COD_SKIPSP:
			C_SkipSP();
			break;
		case COD_SKIP_RET:
			SkipStack(ReadU());
			goto LAB_RET;
		case COD_SKIP_RETGF:
			C_Skip_RetGF();
			goto LAB_RET;
		case COD_JMP:
			C_Jmp();
			break;
		case COD_JMPIF:
			C_JmpIf();
			break;
		case COD_JMPIFNOT:
			C_JmpIfNot();
			break;
		case COD_JMPIF1:
			C_JmpIf1();
			break;
		case COD_JMPIFNOT1:
			C_JmpIfNot();
			break;
		case COD_JMPIFATOM:
			C_JmpIfAtom();
			break;
		case COD_JMPIFCONSP:
			C_JmpIfConsp();
			break;
		case COD_JMPIFEQ:
			C_JmpIfEq();
			break;
		case COD_JMPIFNOTEQ:
			C_JmpIfNotEq();
			break;
		case COD_JMPIFEQTO:
			C_JmpIfEqTo();
			break;
		case COD_JMPIFNOTEQTO:
			C_JmpIfNotEqTo();
			break;
		case COD_JMPHASH:
			C_JmpHash();
			break;
		case COD_JMPHASHV:
			C_JmpHashv();
			break;
		case COD_JSR:
			C_Jsr();
			break;
		case COD_JMPTAIL:
			C_JmpTail();
			break;
		case COD_VENV:
			C_Venv();
			break;
		case COD_MAKEVECTOR1_PUSH:
			C_MakeVector1_Push();
			break;
		case COD_COPYCLOSURE:
			C_CopyClosure();
			break;
		case COD_CALL:
			C_Call();
			break;
		case COD_CALL0:
			C_Call0();
			break;
		case COD_CALL1:
			C_Call1();
			break;
		case COD_CALL2:
			C_Call2();
			break;
		case COD_CALLS1:
			C_Calls1();
			break;
		case COD_CALLS2:
			C_Calls2();
			break;
		case COD_CALLSR:
			C_Callsr();
			break;
		case COD_CALLC:
			C_Callc();
			break;
		case COD_CALLCKEY:
			C_CallcKey();
			break;
		case COD_FUNCALL:
			C_Funcall();
			break;
		case COD_APPLY:
			C_Apply();
			break;
		case COD_PUSH_UNBOUND:
			C_Push_Unbound();
			break;
		case COD_UNLIST:
			C_Unlist();
			break;
		case COD_UNLISTSTERN:
			C_UnlistStern();
			break;
		case COD_JMPIFBOUNDP:
			C_JmpIfBoundp();
			break;
		case COD_BOUNDP:
			C_Boundp();
			break;
		case COD_UNBOUND_NIL:
			C_Unbound_Nil();
			break;
		case COD_VALUES0:
			C_Values0();
			break;
		case COD_VALUES1:
			C_Values1();
			break;
		case COD_STACKTOMV:
			C_StackToMv();
			break;
		case COD_MVTOSTACK:
			C_MvToStack();
			break;
		case COD_NVTOSTACK:
			C_NvToStack();
			break;
		case COD_MVTOLIST:
			C_MvToList();
			break;
		case COD_LISTTOMV:
			C_ListToMv();
			break;
		case COD_MVCALLP:
			C_MvCallp();
			break;
		case COD_MVCALL:
			C_MvCall();
			break;
		case COD_BLOCKOPEN:
			C_BlockOpen();
			break;
		case COD_BLOCKCLOSE:
			C_BlockClose();
			Throw(E_FAIL);
		case COD_RETURNFROM:
			C_ReturnFrom();
			break;
		case COD_RETURNFROMI:
			C_ReturnFromI();
			break;
		case COD_TAGBODYOPEN:
			C_TagbodyOpen();
			break;
		case COD_TAGBODYCLOSE_NIL:
			C_TagbodyClose_Nil();
			goto LAB_RET;
		case COD_TAGBODYCLOSE:
			C_TagbodyClose();
			goto LAB_RET;
		case COD_GO:
			C_Go();
			break;
		case COD_GOI:
			C_GoI();
			break;
		case COD_CATCHOPEN:
			C_CatchOpen();
			break;
		case COD_CATCHCLOSE:
			C_CatchClose();
			goto LAB_RET;
		case COD_THROW:
			C_Throw();
			break;
		case COD_UWPOPEN:
			C_UwpOpen();
			break;
		case COD_UWPNORMALEXIT:
			C_UwpNormalExit();
			goto LAB_RET;
		case COD_UWPCLOSE:
			C_UwpClose();
			goto LAB_RET;
		case COD_UWPCLEANUP:
			C_UwpCleanup();
			goto LAB_RET;
		case COD_HANDLEROPEN:
			C_HandlerOpen();
			break;
		case COD_HANDLERBEGIN_PUSH:
			C_HandlerBegin_Push();
			break;
		case COD_NOT:
			C_Not();
			break;
		case COD_EQ:
			C_Eq();
			break;
		case COD_CAR:
			C_Car();
			break;
		case COD_CDR:
			C_Cdr();
			break;
		case COD_CONS:
			C_Cons();
			break;
		case COD_SYMBOLFUNCTION:
			C_SymbolFunction();
			break;
		case COD_SVREF:
			C_SvRef();
			break;
		case COD_SVSET:
			C_SvSet();
			break;
		case COD_LIST:
			C_List();
			break;
		case COD_LISTSTERN:
			C_ListStern();
			break;
		case COD_NIL_PUSH:
			C_Nil_Push();
			break;
		case COD_T_PUSH:
			C_T_Push();
			break;
		case COD_CONST_PUSH:
			C_Const_Push();
			break;
		case COD_LOAD_PUSH:
			C_Load_Push(ReadU());
			break;
		case COD_LOADI_PUSH:
			C_Loadi_Push();
			break;
		case COD_LOADC_PUSH:
			C_Loadc_Push();
			break;
		case COD_LOADV_PUSH:
			C_Loadv_Push();
			break;
		case COD_POP_STORE:
			C_Pop_Store();
			break;
		case COD_GETVALUE_PUSH:
			C_GetValue_Push();
			break;
		case COD_JSR_PUSH:
			C_JsrPush();
			break;
		case COD_COPYCLOSURE_PUSH:
			C_CopyClosure_Push();
			break;
		case COD_CALL_PUSH:
			C_Call_Push();
			break;
		case COD_CALL1_PUSH:
			C_Call1_Push();
			break;
		case COD_CALL2_PUSH:
			C_Call2_Push();
			break;
		case COD_CALLS1_PUSH:
			C_Calls1_Push();
			break;
		case COD_CALLS2_PUSH:
			C_Calls2_Push();
			break;
		case COD_CALLSR_PUSH:
			C_Callsr_Push();
			break;
		case COD_CALLC_PUSH:
			C_Callc_Push();
			break;
		case COD_CALLCKEY_PUSH:
			C_CallcKey_Push();
			break;
		case COD_FUNCALL_PUSH:
			C_Funcall_Push();
			break;
		case COD_APPLY_PUSH:
			C_Apply_Push();
			break;
		case COD_CAR_PUSH:
			C_Car_Push();
			break;
		case COD_CDR_PUSH:
			C_Cdr_Push();
			break;
		case COD_CONS_PUSH:
			C_Cons_Push();
			break;
		case COD_LIST_PUSH:
			C_List_Push();
			break;
		case COD_LISTSTERN_PUSH:
			C_ListStern_Push();
			break;
		case COD_NIL_STORE:
			C_Nil_Store();
			break;
		case COD_T_STORE:
			C_T_Store();
			break;
		case COD_LOAD_STOREC:
			C_Load_Storec();
			break;
		case COD_CALLS1_STORE:
			C_Calls1_Store();
			break;
		case COD_CALLS2_STORE:
			C_Calls2_Store();
			break;
		case COD_CALLSR_STORE:
			C_Callsr_Store();
			break;
		case COD_LOAD_CDR_STORE:
			C_Load_Cdr_Store();
			break;
		case COD_LOAD_CONS_STORE:
			C_Load_Cons_Store();
			break;
		case COD_LOAD_INC_STORE:
			C_Load_Inc_Store();
			break;
		case COD_LOAD_DEC_STORE:
			C_Load_Dec_Store();
			break;
		case COD_LOAD_CAR_STORE:
			C_Load_Car_Store();
			break;
		case COD_CALL1_JMPIF:
			C_Call1_JmpIf();
			break;
		case COD_CALL1_JMPIFNOT:
			C_Call1_JmpIfNot();
			break;
		case COD_CALL2_JMPIF:
			C_Call2_JmpIf();
			break;
		case COD_CALL2_JMPIFNOT:
			C_Call2_JmpIfNot();
			break;
		case COD_CALLS1_JMPIF:
			C_Calls1_JmpIf();
			break;
		case COD_CALLS1_JMPIFNOT:
			C_Calls1_JmpIfNot();
			break;
		case COD_CALLS2_JMPIF:
			C_Calls2_JmpIf();
			break;
		case COD_CALLS2_JMPIFNOT:
			C_Calls2_JmpIfNot();
			break;
		case COD_CALLSR_JMPIF:
			C_Callsr_JmpIf();
			break;
		case COD_CALLSR_JMPIFNOT:
			C_Callsr_JmpIfNot();
			break;
		case COD_LOAD_JMPIF:
			C_Load_JmpIf();
			break;
		case COD_LOAD_JMPIFNOT:
			C_Load_JmpIfNot();
			break;
		case COD_LOAD_CAR_PUSH:
			C_Load_Car_Push();
			break;
		case COD_LOAD_CDR_PUSH:
			C_Load_Cdr_Push();
			break;
		case COD_LOAD_INC_PUSH:
			C_Load_Inc_Push();
			break;
		case COD_LOAD_DEC_PUSH:
			C_Load_Dec_Push();
			break;
		case COD_CONST_SYMBOLFUNCTION:
			C_Const_SymbolFunction();
			break;
		case COD_CONST_SYMBOLFUNCTION_PUSH:
			C_Const_SymbolFunction_Push();
			break;
		case COD_CONST_SYMBOLFUNCTION_STORE:
			C_ConstSymbolFunctionStore();
			break;
		case COD_APPLY_SKIP_RET:
			C_Apply_Skip_Ret();
			goto LAB_RET;
		case COD_FUNCALL_SKIP_RETGF:
			C_Funcall_Skip_RetGF();
			goto LAB_RET;
		case COD_LOAD0:			case COD_LOAD1:			case COD_LOAD2:			case COD_LOAD3:			case COD_LOAD4:			case COD_LOAD5:			case COD_LOAD6:			case COD_LOAD7:
		case COD_LOAD8:			case COD_LOAD9:			case COD_LOAD10:		case COD_LOAD11:		case COD_LOAD12:		case COD_LOAD13:		case COD_LOAD14:
			C_Load(b-COD_LOAD0);
			break;
		case COD_LOAD_PUSH0:	case COD_LOAD_PUSH1:	case COD_LOAD_PUSH2:	case COD_LOAD_PUSH3:	case COD_LOAD_PUSH4:	case COD_LOAD_PUSH5:	case COD_LOAD_PUSH6:	case COD_LOAD_PUSH7:
		case COD_LOAD_PUSH8:	case COD_LOAD_PUSH9:	case COD_LOAD_PUSH10:	case COD_LOAD_PUSH11:	case COD_LOAD_PUSH12:	case COD_LOAD_PUSH13:	case COD_LOAD_PUSH14:	case COD_LOAD_PUSH15:
		case COD_LOAD_PUSH16:	case COD_LOAD_PUSH17:	case COD_LOAD_PUSH18:	case COD_LOAD_PUSH19:	case COD_LOAD_PUSH20:	case COD_LOAD_PUSH21:	case COD_LOAD_PUSH22:	case COD_LOAD_PUSH23:
		case COD_LOAD_PUSH24:
			C_Load_Push(b-COD_LOAD_PUSH0);
			break;
		case COD_CONST0:		case COD_CONST1:		case COD_CONST2:		case COD_CONST3:		case COD_CONST4:		case COD_CONST5:		case COD_CONST6:		case COD_CONST7:
		case COD_CONST8:		case COD_CONST9:		case COD_CONST10:		case COD_CONST11:		case COD_CONST12:		case COD_CONST13:		case COD_CONST14:		case COD_CONST15:
		case COD_CONST16:		case COD_CONST17:		case COD_CONST18:		case COD_CONST19:		case COD_CONST20:
			C_Const(b-COD_CONST0);
			break;
		case COD_CONST_PUSH0:	case COD_CONST_PUSH1:	case COD_CONST_PUSH2:	case COD_CONST_PUSH3:	case COD_CONST_PUSH4:	case COD_CONST_PUSH5:	case COD_CONST_PUSH6:	case COD_CONST_PUSH7:
		case COD_CONST_PUSH8:	case COD_CONST_PUSH9:	case COD_CONST_PUSH10:	case COD_CONST_PUSH11:	case COD_CONST_PUSH12:	case COD_CONST_PUSH13:	case COD_CONST_PUSH14:	case COD_CONST_PUSH15:
		case COD_CONST_PUSH16:	case COD_CONST_PUSH17:	case COD_CONST_PUSH18:	case COD_CONST_PUSH19:	case COD_CONST_PUSH20:	case COD_CONST_PUSH21:	case COD_CONST_PUSH22:	case COD_CONST_PUSH23:
		case COD_CONST_PUSH24:	case COD_CONST_PUSH25:	case COD_CONST_PUSH26:	case COD_CONST_PUSH27:	case COD_CONST_PUSH28:	case COD_CONST_PUSH29:
			C_Const_Push(b-COD_CONST_PUSH0);
			break;
		case COD_STORE0:		case COD_STORE1:		case COD_STORE2:		case COD_STORE3:		case COD_STORE4:		case COD_STORE5:		case COD_STORE6:		case COD_STORE7:
			C_Store(b-COD_STORE0);
			break;
		default:
			E_SeriousCondition(IDS_E_NoSuchOpcode, GetSubrName(m_subrSelf), CreateFixnum(b));
		}

#ifdef X_DEBUG//!!!D
		if (CurVMContext->m_pb > (byte*)ToVector(TheClosure(CurVMContext->m_closure).CodeVec)->m_pData + (ToVector(TheClosure(CurVMContext->m_closure).CodeVec)->m_dims>>8))
			prevPb = prevPb;
#endif
	}
LAB_RET:
	;
}

bool CLispEng::FindKeywordValue(CP key, ssize_t nArgs, CP *pRest, CP& val) {
	for (int i=0; i<nArgs; i++) {
		if (*--pRest == key) {
			val = *--pRest;
			return true;
		}
		--pRest;
	}
	return false;
}

void __fastcall CLispEng::MatchClosureKey(CP closure, ssize_t nArgs, CP *pKeys, CP *pRest) {
	if (nArgs & 1) {
#ifdef X_DEBUG//!!!D
		CClosure *c = ToCClosure(closure);
		ssize_t nOpt = c->NumOpt;
		ssize_t nReq = c->NumReq;
		Print(c->NameOrClassVersion);
#endif
		E_ProgramErr();
	}
	nArgs >>= 1;
	CClosure *c = ToCClosure(closure);
	uint16_t nKey = c->NumKey;
	CP *pKeyConst = (c->Flags & FLAG_GENERIC ? ToVector(c->Consts[0])->m_pData : c->Consts)+c->KeyConsts;
	for (int i=0; i<nKey; i++) {
		CP key = pKeyConst[i],
			val = 0;
		--pKeys;
		if (FindKeywordValue(key, nArgs, pRest, val))
			*pKeys = val;
	}
	m_pStack = pRest;
	InterpretBytecode(closure, CCV_START_KEY);
}

/*!!!D
void CLispEng::EvalClosure(CP closure)
{
CP args = Pop(),
car = 0;
Push(closure);
CP *pClosure = m_pStack;
CClosure *c = AsClosure(closure);
for (int i=0; i<c->m_numReq; i++)
if (SplitPair(args, car))
{
Push(args);
Eval(car);
args = exchange(*m_pStack, m_r);
}
else
E_Error();
for (int i=0; i<c->m_numOpt; i++)
if (SplitPair(args, car))
{
Push(args);
Eval(car);
args = exchange(*m_pStack, m_r);
}
else
Push(m_specUnbound);
if (c->m_flags & FLAG_REST)
Push(0);
closure = *pClosure;
if (c->m_flags & FLAG_KEY)
{
CP *pKey = 0,
*pRest = 0;
MatchClosureKey(closure, args, pKey, pRest);
}
else
InterpretBytecode(closure);//!!!, CCV_START_NONKEY);
SkipStack(1);
}*/


/*!!!Q
void __fastcall CLispEng::FuncallClosure(CP closure, int nArgs)
{
CClosure *c = AsClosure(closure);
if (nArgs < c->m_numReq)
E_Error();
nArgs -= c->m_numReq;
if (nArgs <= c->m_numOpt)
{
nArgs = c->m_numOpt-nArgs;
for (int i=0; i<nArgs; i++)
Push(0);
if (c->m_flags & FLAG_REST)
Push(0);
if (c->m_flags & FLAG_KEY)
{
for (int count=c->m_numKey; count--;)
Push(m_specUnbound);
}
InterpretBytecode(closure);
}
else
{
nArgs -= c->m_numOpt;
if (c->m_flags & FLAG_KEY)
{
int shift = c->m_numKey;
if (c->m_flags & FLAG_REST)
shift++;
DWORD count = nArgs;
CP *pArgs = m_pStack - shift,
*p1 = m_pStack,
*p2 = pArgs;
int i;
for (i=0; i<nArgs; i++)
*p2++ = *p1++;
if (c->m_flags & FLAG_REST)
*--p1 = m_specUnbound;
CP *pKey = p1,
*pRest = p2;
for (i=0; i<c->m_numKey; i++)
*--p1 = m_specUnbound;
m_pStack = pArgs;
MatchClosureKey(closure, count, pKey, pRest);
}
else
InterpretNoKey(closure, 0, nArgs);
}
}*/

void CLispEng::ApplyClosure(CP closure, ssize_t nArgs, CP args) {
	LISP_TAIL_REC_KEEPER(false);

#ifdef _X_DEBUG //!!!D
	static int s_n;
	if (++s_n == 0x2500)
	{
		s_n = s_n;
		Disassemble(cerr, closure);
	}
#endif

	CClosure *c = &TheClosure(closure);
	ssize_t nOpt = c->NumOpt;
	ssize_t count = 0;
	CP car;
	if (nArgs < c->NumReq) {
		for (ssize_t count=c->NumReq-nArgs; count--;)
			if (SplitPair(args, car))
				Push(car);
			else {
#ifdef X_DEBUG//!!!D
				Print(c->NameOrClassVersion);
				Disassemble(cerr, CurClosure);
				//				Disassemble(cerr, closure);
				void *pData = AsArray(c->CodeVec)->m_pData;
				pData = pData;
#endif
				E_ProgramErr();
			}
	} else {
		nArgs -= c->NumReq;
		if (nArgs >= nOpt)
			goto LAB_OUT;
		nOpt -= nArgs;
	}
	nArgs = nOpt; //!!!
	CP *pKey,
		*pRest;
	count = nOpt;
	while (SplitPair(args, car)) {
		if (count) {
			count--;
			Push(car);
		} else {
			if (!c->Flags)
				E_ProgramErr();
			if (c->Flags & FLAG_REST)
				Push(args);
			if (c->Flags & FLAG_KEY) {
				pKey = m_pStack;
				PushUnbounds(c->NumKey);
				pRest = m_pStack;
				count = 0;
				goto LAB_KEY_FROM_LIST;
			} else {
				InterpretBytecode(closure, CCV_START_NONKEY);
				return;
			}
		}
	}
	//!!!Dwhile (count--)
	//Push(m_specUnbound);
	PushUnbounds(count);
	if (c->Flags & FLAG_REST) {
		Push(0);
	}
	if (c->Flags & FLAG_KEY) {
		PushUnbounds(c->NumKey);
		InterpretBytecode(closure, CCV_START_KEY);
	} else
		InterpretBytecode(closure, CCV_START_NONKEY);
	return;
LAB_OUT:
	nArgs -= nOpt;
	if (c->Flags & FLAG_KEY) {
		{
			int shift = c->NumKey + bool(c->Flags & FLAG_REST);
			pKey = m_pStack+(count=nArgs);
			pRest = (m_pStack-=shift) + nArgs;
			memmove(m_pStack, m_pStack+shift, sizeof(CP)*nArgs);
			FillMem(pRest, c->NumKey, V_U);
			if (c->Flags & FLAG_REST) {
				*--pKey = args;
				Push(closure, args);
				for (int i=0; i<nArgs; i++)
					*pKey = Cons(m_pStack[i+2], *pKey);
				args = Pop();
				closure = Pop();
			}
		}
LAB_KEY_FROM_LIST:
		for ( ;SplitPair(args, car); count++)
			Push(car);
		MatchClosureKey(closure, count, pKey, pRest);
	} else {
		if (c->Flags & FLAG_REST) {
			m_r = closure; //!!! to save
			Push(args);
			Push(ListofEx(nArgs+1));
		} else if (nArgs) {
#ifdef X_DEBUG//!!!D
			Print(TheClosure(closure).NameOrClassVersion);
#endif
			E_ProgramErr();
		}
		InterpretBytecode(closure, CCV_START_NONKEY);
	}
}


} // Lisp::



