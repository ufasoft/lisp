#include <el/ext.h>

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_DefaultBreakDriver() {
#ifdef _DEBUG//!!!D
	E_Error();
#endif
	Throw(E_LISP_UnhandledCondition);
}

void CLispEng::StackOverflow() {
	E_Error();//!!!
	Push(CreateInteger(int(E_LISP_StackOverflow)-E_LISP_BASE+1));
	F_GetErrMessage();
	CArrayValue *av = ToArray(m_r);
	CP symMacro = 0;
	CP stm = *SymValue(AsSymbol(S(L_S_ERROR_OUTPUT)), 0, &symMacro);
	for (size_t i=0; i<av->TotalSize(); ++i) {
		Push(av->GetElement(i));
		Push(stm);
		F_WriteChar();
	}
	ThrowTo(GetSymbol("REPL", m_packSYS));
}

void CLispEng::ErrorCode(HRESULT hr, RCString s) {
#ifdef WIN32
	TCHAR buf[256];
	const char *p = s;
	if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY, LPCVOID(AfxGetInstanceHandle()), hr, 0,
		buf, size(buf), (char**)&p))
		Throw(E_FAIL);
	CPutCharOstream os = m_streams[STM_ErrorOutput]; //!!!? may be dynamic_cast
	os << String(buf);
	Call(GetSymbol("_BREAK-LOOP", m_packSYS));
#else
	Throw(E_FAIL);
#endif
}

/*!!!R
void CLispEng::F_Err1() {
	String s = AsString(Pop());
	HRESULT hr = E_LISP_Base+AsNumber(Pop())+1;
	SkipStack(1);
	ErrorCode(hr, s);
}
*/

void CLispEng::F_GetErrMessage() {
	E_Error();
	/*!!!  HRESULT hr = E_LISP_Base+AsNumber(Pop());
	char buf[256];
	if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY, LPCVOID(AfxGetInstanceHandle()), hr, 0,
	buf, sizeof buf, 0))
	Throw(E_FAIL);
	int len = strlen(buf);
	if (len >= 2 && buf[len-2] == '\r' && buf[len-1] == '\n')
	buf[len-2] = 0;
	m_r = CreateString(buf);*/
}

void E_Error() {
	CLispEng& lisp = Lisp();
#ifdef _DEBUG
	lisp.m_bTraceError = true;
//	MessageBox::Show("Error");
#endif

	/*!!!
	if (CP eh = lisp.get_Special(L_S_ERROR_HANDLER))
	{
	lisp.SetSpecial(S(L_S_ERROR_HANDLER), 0); //!!! must be BIND
	lisp.Push(0, lisp.CreateString("Format string"));
	lisp.Apply(eh, 2, 0);
	Throw(E_FAIL);//!!!
	}
	*/

//	lisp.Disassemble(cerr, lisp.CurClosure);
	//  if (lisp.m_bTraceError)
	{		
		lisp.PrintFrames();
		//!!!    Sleep(1000000);//!!!
	}
	//!!!  Throw(hr);

	lisp.Error(E_LISP_UnknownError, 0);
}

void CLispEng::Error(HRESULT hr, CP p1) {
	//!!!	PrintForm(cerr, p1);
#ifdef _DEBUG//!!!D
	E_Error();//!!!
#endif
	Call(GetSymbol("_AS-STRING", m_packSYS), p1);
	ErrorCode(hr, AsString(m_r));
	Throw(E_EXT_CodeNotReachable);
}

/*!!!R
void CLispEng::F_S_Print() {
	Print(Pop());
}*/

CP CLispEng::GetCond(CP sym) {
#ifdef X_DEBUG//!!!D
//	Disassemble(cerr, CurClosure);
#endif
	CP ar = Spec(L_S_CONDS);
	if (Type(ar) != TS_ARRAY) {
		Exception exc(E_LISP_NoConditionSystem);
		exc.Data["Error"] = AsString(sym);
		throw exc;
	}
	return Cdr(ToArray(ar)->GetElement(AsIndex(sym) - ENUM_L_CONDITION)); //!!! may be CDR?
}

void CLispEng::Err(int idx) {
	Throw(E_FAIL);//!!!
}

void CLispEng::E_Signal(CP sym) {
#ifdef X_DEBUG//!!!
	E_Error();//!!!
#endif
	if (!ToSymbol(S(L_SIGNAL))->GetFun()) {
		cerr << "\n" << ToSymbol(sym)->m_s << endl;
		abort();
	}
	Push(GetCond(sym));
	Apply(S(L_SIGNAL), 1, 0);
}

void CLispEng::E_Error(CP sym) {
	E_Signal(sym);
	cerr << "\n" << ToSymbol(sym)->m_s << endl;
	abort();
}

void CLispEng::E_SignalErr(CP typ, int errCode, int nArg) {
	ASSERT((nArg & 1)==0);

	m_pStack[nArg-1] = GetCond(typ);
	m_pStack[nArg-2] = CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, errCode));
	m_pStack[nArg-3] = CreateString(AfxLoadString(errCode)); 
	Funcall(S(L_COERCE_TO_CONDITION_EX), nArg);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_CellErr(CP name) {
#ifdef X_DEBUG//!!!D
	Print(name);
	E_Error();//!!!
#endif

#ifdef X_C_LISP_DEBUG //!!!D
	Print(name);
	//		Disassemble(cerr, CurClosure);
	/*!!!		CP *frame = AsFrame(m_env.m_varEnv);
	frame = AsFrame(frame[2]);
	Print(frame[4]);
	Print(frame[3]);
	Print(frame[2]);
	Print(frame[1]);*/
	
#endif

#ifdef _DEBUG//!!!D
	Print(name);
	E_Error();
#endif

	Push(GetCond(S(L_CELL_ERROR)));
	Push(S(L_K_NAME), name);
	Apply(S(L_ERROR), 3, 0);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_UndefinedFunction(CP name) {
#ifdef X_UCFG_LISP_DEBUGGER //!!!D
	if (g_bOptDD) {
		Print(name);
	
	}
#endif
//		E_Error();//!!!D
	
#ifdef _DEBUG//!!!D
	Print(name);
	ostringstream os;
	Print(os, name);
	String n = os.str();
	E_Error();
#endif

	Push(0, 0, 0);
	Push(List(name));
	Push(S(L_K_NAME), name);
	E_SignalErr(S(L_UNDEFINED_FUNCTION), IDS_E_FunctionUndefined, 6);
}


void CLispEng::E_PackageErr(CP pack) {
	Push(GetCond(S(L_PACKAGE_ERROR)));
	Push(S(L_K_PACKAGE), pack);
	Apply(S(L_ERROR), 3, 0);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_PackageErr(CP pack, int errCode, CP a, CP b) {
	Push(0, 0, 0);
	Push(b==V_U ? List(a) : List(a, b));   			// save args before GC-calling creates				
	Push(S(L_K_PACKAGE));
	Push(pack);
	E_SignalErr(S(L_PACKAGE_ERROR), errCode, 6);
}

void CLispEng::E_TypeErr(CP datum, CP typ, int errCode, CP a, CP b) {
	Push(0, 0, 0);
	Push(0);
	Push(S(L_K_DATUM), datum);
	Push(S(L_K_EXPECTED_TYPE), typ);

	if (errCode == IDS_E_IsNot)
		SV4 = List(datum, typ);
	else
		SV4 = b==V_U ? List(a) : List(a, b);
	E_SignalErr(S(L_TYPE_ERROR), errCode, 8);
}

void CLispEng::E_RangeErr(CP idx, CP bound) {
	Push(idx);
	Push(List(S(L_INTEGER), V_0, List(bound)));
	E_TypeErr(SV1, SV);
}

void CLispEng::E_StreamErr(CP stm) {
	Push(GetCond(S(L_STREAM_ERROR)));
	Push(S(L_K_STREAM), stm);
	Apply(S(L_ERROR), 3, 0);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_StreamErr(int errCode, CP stm, CP arg) {	
	Push(0, 0, 0);
	Push(List(arg));
	Push(S(L_K_STREAM), stm);
	E_SignalErr(S(L_READER_ERROR), errCode, 6);
}

void CLispEng::E_FileErr(CP path) {
	Push(GetCond(S(L_FILE_ERROR)));
	Push(S(L_K_PATHNAME), path);
	Apply(S(L_ERROR), 3, 0);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_FileErr(HRESULT hr, RCString message, CP pathname) {
	Push(GetCond(S(L_FILE_ERROR)));
	Push(CreateInteger(hr));
	Push(CreateString("~S"));
	Push(List(CreateString(message)));
	Push(S(L_K_PATHNAME));
	Push(pathname);
	Funcall(S(L_COERCE_TO_CONDITION_EX), 6);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_ParseErr() {	
	Push(0, 0, 0);
	Push(0);
	E_SignalErr(S(L_PARSE_ERROR), IDS_E_ParseError, 4);
}

void CLispEng::E_DivisionByZero(CP n, CP d) {
	Push(0, 0, 0);
	Push(0);
	Push(S(L_K_OPERATION), m_subrSelf);
	Push(S(L_K_OPERANDS), List(n, d));
	E_SignalErr(S(L_DIVISION_BY_ZERO), IDS_E_DivideByZero, 8);
}

void CLispEng::E_EndOfFileErr(CP stm) {
	Push(GetCond(S(L_END_OF_FILE)));
	Push(S(L_K_STREAM), stm);
	Apply(S(L_ERROR), 3, 0);
	Throw(E_EXT_CodeNotReachable);
}

void CLispEng::E_ReaderErr() {
#ifdef _DEBUG//!!!D
	E_Error();
#endif

	E_Error(S(L_READER_ERROR));
}

void CLispEng::E_ProgramErr() {
#if UCFG_LISP_DEBUGGER
	E_Error(); //!!!
#endif
	Push(GetCond(S(L_PROGRAM_ERROR)));
	Apply(S(L_ERROR), 1, 0);
}

void CLispEng::E_ProgramErr(int errCode, CP a, CP b) {
#ifdef _DEBUG//!!!D
	E_Error();
#endif
	Push(0, 0, 0);
	Push(b==V_U ? List(a) : List(a, b));   			// save args before GC-calling creates				
	E_SignalErr(S(L_PROGRAM_ERROR), errCode, 4);
}

void CLispEng::E_ControlErr(int errCode, CP a, CP b) {
	Push(0, 0, 0);
	Push(b==V_U ? List(a) : List(a, b));   			// save args before GC-calling creates				
	E_SignalErr(S(L_CONTROL_ERROR), errCode, 4);
}

void CLispEng::E_UnboundVariableErr(CP name, int errCode, CP a, CP b) {
	Push(0, 0, 0);
	Push(b==V_U ? List(a) : List(a, b));   			// save args before GC-calling creates				
	Push(S(L_K_NAME));
	Push(name);
#ifdef _DEBUG//!!!D
	Print(name);
//	E_Error();
#endif
	E_SignalErr(S(L_UNBOUND_VARIABLE), errCode, 6);
}

void CLispEng::E_ArithmeticErr(int errCode, CP a) {
	Push(0, 0, 0);
	Push(a==V_U ? 0 : List(a));
	E_SignalErr(S(L_ARITHMETIC_ERROR), errCode, 4);
}

void CLispEng::E_Err(int errCode, CP a) {
	Push(0, 0, 0);
	Push(a==V_U ? 0 : List(a));
	E_SignalErr(S(L_ERROR), errCode, 4);
}

void CLispEng::E_SeriousCondition(int errCode, CP a, CP b) {
	Push(0, 0, 0);
	Push(errCode==IDS_E_SeriousCondition ? List(GetSubrName(m_subrSelf)) : a==V_U ? 0 : b==V_U ? List(a) : List(a, b));   			// save args before GC-calling creates				
	E_SignalErr(S(L_SERIOUS_CONDITION), errCode, 4);
}

void CLispEng::CerrorOverflow() {
	Call(S(L_CERROR), CreateString("Continue with INFINITY values"), S(L_FLOATING_POINT_OVERFLOW));
}

void CLispEng::CerrorUnderflow() {
	Call(S(L_CERROR), CreateString("Continue with Denormalized values"), S(L_FLOATING_POINT_UNDERFLOW));
}

void CLispEng::PrepareCond(int errCode, CP type) {
	Push(GetCond(type));
	Push(CreateInteger(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LISP, errCode)));
	Push(CreateString(AfxLoadString(errCode)));
}

CP CLispEng::CorrectableError(int nArg) {
	Funcall(S(L_CORRECTABLE_ERROR_EX), nArg);
	return m_r;
}

/*!!!D
void CLispEng::E_TypeError(HRESULT hr, CP p) {
Disassemble(cerr, CurClosure);
Error(hr, p);//!!!
}*/

void CLispEng::Debug() {
	ostream& os = *m_streams[STM_DebugIO]->GetOstream(); //!!!? may be dybamic_cast
	os << "Evaluation breaked!" << endl;
	PrintFrames();
	Throw(E_FAIL);
	//!!!  m_r = CreateThrow(m_specEOF);
	//!!!Loop();
}

void CLispEng::PrintFormHelper(CP stm, CP form) {
	Call("PRINC", form, stm);  
}

void CLispEng::PrintString(CP stm, RCString s) {
	PrintFormHelper(stm, CreateString(s));
}

CP GetFuncName(CP p) {
	switch (Type(p)) {
	case TS_INTFUNC: return Lisp().AsIntFunc(p)->m_name;
	case TS_CCLOSURE: return Lisp().TheClosure(p).NameOrClassVersion;
	default: Throw(E_FAIL);
	}
}

const char *g_arFrameTypeName[] = {
	"", "EVAL", "IBLOCK", "DYNBIND", "APPLY", "ENV", "ENV", "ENV", "ENV", "ENV", "ENV", "ENV"
	"ITAGBODY", "CATCH", "UNWIND-PROTECT", "VAR", "FUNC", "DRIVER"
};


struct CStringInt {
	CLString m_s;
	int m_nCount;
	LONGLONG m_nTicks;
	size_t m_size;

	CStringInt()
		: m_size(0)
	{}
};

size_t CLispEng::SizeOf(CP p, int level) {
	int r = 0;
	if (!level--)
		return 0;
	switch (Type(p)) {
	case TS_CONS:
		if (p)
			return 8+SizeOf(Car(p), level)+SizeOf(Cdr(p), level);
		return 0;
	case TS_INTFUNC:
		{
			CIntFuncValue *ifv = AsIntFunc(p);
			return sizeof(CIntFunc)+SizeOf(ifv->m_body, level)
				+SizeOf(ifv->m_env.m_varEnv, level)
				+SizeOf(ifv->m_env.m_funEnv, level)
				+SizeOf(ifv->m_env.m_blockEnv, level)
				+SizeOf(ifv->m_env.m_goEnv, level)
				+SizeOf(ifv->m_env.m_declEnv, level)
				+SizeOf(ifv->m_vars, level)
				+SizeOf(ifv->m_optInits, level)
				+SizeOf(ifv->m_keyInits, level);
		}
		/*!!!	case TS_SYMBOL:
		{
		CSymbolValue *sv = AsSymbol(p);
		return sizeof(CSymbolValue)+SizeOf(sv->m_dynValue, level)+SizeOf(sv->GetPropList(), level)+(sv->m_s.Length*2+8); //!!!
		}*/
	case TS_ARRAY:
		{
			CArrayValue *av = AsArray(p);
			size_t size = (av->TotalSize())*CArrayValue::ElementBitSize(av->m_elType)/8;
			return sizeof(CArrayValue*)+size;
		}
	default:
		return 0;
	}
}

void CLispEng::ShowInfo() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
#if UCFG_LISP_PROFILE
		ostream& os = cerr; //!!!? may be dybamic_cast
	if (m_bProfile) {
		/*!!!    CPackage::CSymMap setSym;
		CSPtr pack;
		for (CPackageMap::CIterator i=m_mapPackage; i; i++)
		for (CPackage::CSymMap::CIterator j(ToPackage(i.Val)->m_mapSym); j; j++)
		setSym.insert(*j);

		for (CPackage::CSymMap::CIterator k=setSym; k; k++)
		if (CP func = (*k)->GetFun())*/

		typedef multimap<__int64, CStringInt> CMM;
		CMM mm;  //!!! make sorted list
		for (int i=0; i<m_symbolMan.m_size; i++) {
			CSymbolValue *sv = m_symbolMan.Base+i;
			if (*(byte*)sv != 0xFF)
				if (CP func = sv->GetFun()) {
					CProfInfo *pi = 0;
LAB_LOOP:
					switch (Type(func)) {
					case TS_SUBR:
						pi = &AsProfiled(func);
						break;
					case TS_INTFUNC:
						pi = &ToIntFunc(func)->m_profInfo;
						break;
					case TS_CCLOSURE:
						pi = &ToCClosure(func)->m_profInfo;
						break;
					case TS_MACRO:
						func = AsMacro(func)->m_expander;
						goto LAB_LOOP;
					case TS_SPECIALOPERATOR:
						//!!!            pi = ToSpecialOperator(func);
						break;
					case TS_CONS:
						break;
					default:
						Throw(E_FAIL);
					}
					if (pi) {
						CStringInt si;
#if UCFG_LISP_SPLIT_SYM
						si.m_s = SymNameByIdx(i);
#else
						si.m_s = sv->m_s;
#endif
						si.m_nCount = (int)pi->m_nCount;
						si.m_nTicks = pi->m_nTicks;
						si.m_size = SizeOf(func, 10);
						mm.insert(CMM::value_type(-si.m_nTicks, si));  //!!! m_nCount | m_nTicks
					}
				}
		}
		os << '\n';
		for (CMM::iterator n(mm.begin()); n!=mm.end(); ++n)
			os << n->second.m_nCount << '\t' << DWORD(n->second.m_nTicks / 1000)
			<< '\t' << n->second.m_s << "\t" << (DWORD)n->second.m_size << '\n';
	}
#endif
}

void CLispEng::F_BreakDriver() {
	//!!!  CSPtr c = Pop();   //!!!
	//!!!  ostream os(new CTextStreambuf(m_arStream[STM_DebugIO]));
	//!!!  PrintForm(os, c);
	//!!!  if (m_bTraceError)
	//!!!    PrintFrames();//!!!
	Call(GetSymbol("_BREAK-LOOP", m_packSYS), Pop());
}

void CLispEng::F_FrameInfo() {
	CP p = Pop();
	CP *frame = 0;
	if (p != V_U)
		frame = ToFrame(p);
	else {
		for (CP *f = m_pStack; f<m_pStackTop; f++) {
			if (Type(*f) == TS_FRAMEINFO) {
				frame = f;
				break;
			}
		}
	}
	if (frame) {
		CFrameType ft = AsFrameType(*frame);
		CP *pStackBeg = m_pStack;
		Push(GetSymbol(g_arFrameTypeName[ft], m_packSYS));
		switch (ft) {
		case FT_EVAL:
		case FT_APPLY:
			Push(frame[1]); break;
		}
		m_r = Listof(pStackBeg-m_pStack);
	}
}

 #if UCFG_LISP_DEBUGGER

bool CLispEng::StackUpendP(CP *ps) {
	if (ps==m_pStackTop || 
		Type(*ps)==TS_FRAMEINFO && AsFrameType(*ps)==FT_DRIVER)
		return true;
	CP lim = SymbolValue(GetSymbol("*FRAME-LIMIT-UP*", m_packSYS));
	return Type(lim)==TS_FRAME_PTR && ps>AsFrame(lim);
}

bool CLispEng::StackDownendP(CP *ps) {
	if (Type(*ps)==TS_FRAMEINFO && AsFrameType(*ps)==FT_DRIVER)
		return true;
	CP lim = SymbolValue(GetSymbol("*FRAME-LIMIT-DOWN*", m_packSYS));
	return Type(lim)==TS_FRAME_PTR && ps<AsFrame(lim);
}

CP *CLispEng::TestFrameMove(CP *frame, size_t mode) {
	if (Type(*frame) != TS_FRAMEINFO)
		return 0;
	switch (mode) {
	case 1:
	case 2:
		return frame;
	case 3:
		switch (AsFrameType(*frame)) {
		case FT_VAR:
		case FT_FUN:
		case FT_ENV1V:
		case FT_ENV2VD:
		case FT_ENV5:
			return frame;
		}
		return 0;
	case 4:
		switch (AsFrameType(*frame)) {
		case FT_EVAL:
		case FT_APPLY:
			return frame;
		}
		return 0;
	case 5:
		switch (AsFrameType(*frame)) {
		case FT_APPLY:
			return frame;
		}
		return 0;
	default:
		E_Error();
	}
}

CP *CLispEng::FrameUp(CP *ps, size_t mode) {
	for (CP *frame=Type(*ps)==TS_FRAMEINFO ? AsFrameTop(ps) : ps+1; !StackUpendP(frame);) {
		if (mode == 1)
			return frame;
		if (CP *r = TestFrameMove(frame, mode))
			return r;
		if (Type(*frame) == TS_FRAMEINFO)
			frame = AsFrameTop(frame);
		else
			frame++;
	}
	return ps;
}

CP *CLispEng::FrameDown(CP *ps, size_t mode) {
	CP *frame = ps;
	while (true) {
		while (Type(*--frame) != TS_FRAMEINFO)
			;
		if (StackDownendP(frame))
			return ps;
		if (CP *r = TestFrameMove(frame, mode))
			return r;
	}
}

void CLispEng::F_FrameUp1() {
	size_t mode = AsPositive(Pop());
	m_r = CreateFramePtr(FrameUp(ToFrame(Pop()), mode));
}

void CLispEng::F_FrameDown1() {
	size_t mode = AsPositive(Pop());
	m_r = CreateFramePtr(FrameDown(ToFrame(Pop()), mode));
}

void CLispEng::F_TheFrame() {
	m_r = CreateFramePtr(FrameUp(m_pStack, 2));
}

void CLispEng::F_DriverFrameP() {
	CP p = *ToFrame(Pop()); //!!!
	m_r = FromBool(Type(p)==TS_FRAMEINFO && AsFrameType(p)==FT_DRIVER);
}

void CLispEng::F_EvalFrameP() {
	CP p = *ToFrame(Pop()); //!!!
	m_r = FromBool(Type(p)==TS_FRAMEINFO && (AsFrameType(p)==FT_EVAL || AsFrameType(p)==FT_APPLY));
}

class CDriverFrame : public CJmpBuf {
public:
	CDriverFrame() {
		CLispEng& lisp = Lisp();
		Finish(lisp, FT_DRIVER, lisp.m_pStack);
	}
};

void CLispEng::F_Driver() {
	CP fun = SV;
	CDriverFrame driverFrame;

	LISP_TRY(driverFrame) {
		while (true)
			Call(fun);
	} LISP_CATCH(driverFrame) {
	} LISP_CATCH_END

	SkipStack(1);
}

void CLispEng::Reset(size_t count) {
	m_cVal = 0;
	CP *drv = 0;
	for (CP *p=m_pStack; p!=m_pStackTop; p++) {
		if (Type(*p)==TS_FRAMEINFO && AsFrameType(*p)==FT_DRIVER) {
			drv = p;
			if (!--count)
				break;
		}
	}
	UnwindTo(drv);
}

void CLispEng::F_UnwindToDriver() {
	size_t count = 1;
	if (CP p = Pop())
		count = IntegerP(p) ? AsPositive(p) : 0;
	Reset(count);
}

void CLispEng::F_ReturnFromEvalFrame() {
	m_r = Pop();
	CP *frame = ToFrame(Pop());
	switch (AsFrameType(*frame)) {
	case FT_EVAL:
	case FT_APPLY:
		break;
	default:
		E_Error();
	}
	UnwindTo(frame);
}

static void AssignEnv(CP& var, CP val) {
	if (!var)
		var = val;
}

CEnvironment CLispEng::SameEnvAs() {
	CEnvironment env;
	for (CP *frame=ToFrame(Pop()); --frame!=m_pStack;) {
		if (Type(*frame) == TS_FRAMEINFO) {
			switch (AsFrameType(*frame)) {
			case FT_ENV1V:
				AssignEnv(env.m_varEnv, frame[1]);
				break;
			case FT_ENV1F:
				AssignEnv(env.m_funEnv, frame[1]);
				break;
			case FT_ENV1B:
				AssignEnv(env.m_blockEnv, frame[1]);
				break;
			case FT_ENV1G:
				AssignEnv(env.m_goEnv, frame[1]);
				break;
			case FT_ENV1D:
				AssignEnv(env.m_declEnv, frame[1]);
				break;
			case FT_ENV2VD:
				AssignEnv(env.m_varEnv, frame[1]);
				AssignEnv(env.m_declEnv, frame[2]);
				break;
			case FT_ENV5:
				AssignEnv(env.m_varEnv, frame[1]);
				AssignEnv(env.m_funEnv, frame[2]);
				AssignEnv(env.m_blockEnv, frame[3]);
				AssignEnv(env.m_goEnv, frame[4]);
				AssignEnv(env.m_declEnv, frame[5]);
				break;
			}
		}
	}
	AssignEnv(env.m_varEnv, m_env.m_varEnv);
	AssignEnv(env.m_funEnv, m_env.m_funEnv);
	AssignEnv(env.m_blockEnv, m_env.m_blockEnv);
	AssignEnv(env.m_goEnv, m_env.m_goEnv);
	AssignEnv(env.m_declEnv, m_env.m_declEnv);
	return env;
}

void CLispEng::F_SameEnvAs() {
	CP fun = Pop();
	CEnvironment env = SameEnvAs();
	CEnv5Frame env5Frame(_self);
	m_env = env;
	Call(fun);
}

#endif // UCFG_LISP_DEBUGGER

void CLispEng::F_ObjPtr() {
	m_r = CreateInteger64(DWORD(Pop()));
}


class CHandlerKeeper {
	CStackRange *m_range;
public:
	CHandlerKeeper() {
		m_range = Lisp().m_pInactiveHandlers;
	}

	~CHandlerKeeper() {
		Lisp().m_pInactiveHandlers.reset(m_range);
	}
};

void CLispEng::F_InvokeHandlers() {
	CP cond = SV;
	CStackRange *other = m_pInactiveHandlers;
	for (CP *frame=m_pStack; frame!=m_pStackTop;) {
		if (other && frame==other->Low)
			frame = exchange(other, other->Next)->High;
		else if (Type(frame[0]) == TS_FRAMEINFO) {
			if (AsFrameType(frame[0]) == FT_HANDLER) {
				CP handlers = frame[FRAME_HANDLERS];
				CArrayValue *vec = ToVector(Car(handlers));
				size_t m2 = vec->GetVectorLength();
				for (size_t i=0; i<m2; i+=2) {
					Call(S(L_SAFE_TYPEP), cond, AsArray(Car(handlers))->GetElement(i));
					if (m_r) {
						CHandlerKeeper handlerKeeper;
						CStackRange newRange = { m_pStack, AsFrameTop(frame), other };
						m_pInactiveHandlers.reset(&newRange);
						vec = ToVector(Car(handlers));
						if (Cdr(handlers)) {
							m_cond = cond; //!!!
							m_stackOff = m_pStackTop-(frame+4);
							m_spOff = AsNumber(frame[FRAME_SP]);
							m_spDepth = Cdr(handlers);
							CP closure = frame[FRAME_CLOSURE];
							InterpretBytecode(closure, (TheClosure(closure).Flags & 0x80 ? CCV_START_KEY : CCV_START_NONKEY)+AsNumber(vec->GetElement(i+1)));
						} else {
							Throw(E_FAIL); //!!! todo
						}
					}
				}
			}
			frame = AsFrameTop(frame);
		} else
			frame++;
	}
	if (StackOverflowAddress) {
		StackOverflowExc e;
		e.StackAddress = StackOverflowAddress;
		StackOverflowAddress = 0;
		throw e;
	}
	if (CP gh = AsSymbol(S(L_GLOBAL_HANDLER))->GetFun())
		Apply(gh, 1);
	else
		SkipStack(1);
}

static const char *s_sFrameBinding = "frame binding environments";

void CLispEng::PrintOneBind(const char *envName, CP *frame) {
	WritePChar(s_sFrameBinding);
	WriteStr("\n  "+String(envName)+"_ENV <--> ");
	Prin1(frame[1]);
}

void CLispEng::PrintNextEnv(CP env) {
	Call(S(L_TERPRI), m_stm);
	WritePChar("  Next environment: ");
	if (ConsP(env))
		for (CP car; SplitPair(env, car);) {
			if (!ConsP(car))
				E_Error();
			WritePChar("\n  | ");
			Prin1(Car(car));
			WritePChar("\n  --> ");
			Prin1(Cdr(car));
		}
	else
		Prin1(env);
}

void CLispEng::PrintNextVarFunEnv(CP env) {
	Call(S(L_TERPRI), m_stm);
	WritePChar("  Next environment: ");
	if (VectorP(env))
		for (size_t i=0; VectorP(env); env=ToVector(env)->GetElement(i))
			for (size_t size=ToVector(env)->GetVectorLength(); i<size-1;) {
				WritePChar("\n  | ");
				Prin1(ToVector(env)->GetElement(i++));
				WritePChar("\n  <--> ");
				Prin1(ToVector(env)->GetElement(i++));
			}
	else
		Prin1(env);
}

CP *CLispEng::PrintStackItem(CP *frame) {
	if (Type(*frame) != TS_FRAMEINFO) {
		WriteStr("- ");
		Prin1(*frame & MASK_WITHOUT_FLAGS);
		return frame+1;
	}
	CP *top = AsFrameTop(frame);
	switch (AsFrameType(*frame)) {
	case FT_EVAL:
		WriteStr("EVAL frame for form ");
		Prin1(frame[FRAME_FORM]);
		break;
	case FT_DYNBIND:
		WriteStr("frame binding variables (~ = dynamically):");
		for (frame++; frame!=top; frame+=2) {
			WriteStr("\n  | ~ ");
			Prin1(frame[0]);
			WriteStr(" <--> ");
			Prin1(frame[1]);
		}
		break;
	case FT_APPLY:
		{
			WriteStr("APPLY frame for call ");
			CParen paren;
			Prin1(ToIntFunc(frame[1])->m_name);  //!!! must be FRAME_CLOSURE
			for (CP *args=top; args>=frame+FRAME_ARGS;) {
				WritePChar(" \'");
				Prin1(*--args);
			}
		}
		break;
	case FT_ENV1V:
		PrintOneBind("VAR", frame);
		break;
	case FT_ENV1F:
		PrintOneBind("FUN", frame);
		break;
	case FT_ENV1B:
		PrintOneBind("BLOCK", frame);
		break;
	case FT_ENV1G:
		PrintOneBind("GO", frame);
		break;
	case FT_ENV1D:
		PrintOneBind("DECL", frame);
		break;
	case FT_ENV2VD:
		WritePChar(s_sFrameBinding);
		WriteStr("\n  VAR_ENV <--> ");
		Prin1(frame[1]);
		WriteStr("\n  DECL_ENV <--> ");
		Prin1(frame[2]);
		break;
	case FT_ENV5:
		WritePChar(s_sFrameBinding);
		WriteStr("\n  VAR_ENV <--> ");
		Prin1(frame[1]);
		WriteStr("\n  FUN_ENV <--> ");
		Prin1(frame[2]);
		WriteStr("\n  BLOCK_ENV <--> ");
		Prin1(frame[3]);
		WriteStr("\n  GO_ENV <--> ");
		Prin1(frame[4]);
		WriteStr("\n  DECL_ENV <--> ");
		Prin1(frame[5]);
		break;
	case FT_IBLOCK:
		WritePChar("block frame ");
		Prin1(CreateFramePtr(frame));
		WritePChar(" for ");
		PrintNextEnv(frame[FRAME_NEXT_ENV]);
		break;
	case FT_ITAGBODY:
		{
			CP env = frame[1]; //!!! reduced FRAME_NEXT_ENV
			Prin1(CreateFramePtr(frame));
			WritePChar(" for");
			for (frame+=2; frame!=top; frame+=2) //!!! reduced FRAME_BINDINGS
			{
				WritePChar("\n  | ");
				Prin1(frame[0]);
				WritePChar("\n  --> ");
				Prin1(frame[1]);
			}
			PrintNextEnv(env);
		}
		break;
	case FT_CBLOCK:
	case FT_CTAGBODY:
		{			
			CP p = Car(frame[FRAME_CTAG]);
			WritePChar(Type(p)==TS_ARRAY ? "compiled tagbody frame for " : "compiled block frame for ");
			Prin1(p);
		}
		break;
	case FT_CATCH:
		WritePChar("catch frame for tag ");
		Prin1(frame[FRAME_TAG]);
		break;
	case FT_UNWINDPROTECT:
		WritePChar("unwind-protect frame");
		break;
	case FT_HANDLER:
		WritePChar("handler frame for conditions");
		{
			CP carHandlers = Car(frame[FRAME_HANDLERS]);
			size_t len = ToVector(carHandlers)->GetVectorLength();
			for (size_t i=0; i<len; i+=2) {
				WriteChar(' ');
				Prin1(ToVector(carHandlers)->GetElement(i));
			}
		}
		break;
	case FT_DRIVER:
		Call(S(L_TERPRI), m_stm);
		WritePChar("driver frame");
		break;
	case FT_VAR:
		WritePChar("frame binding variables ");
		goto LAB_VARFUN_FRAME;
	case FT_FUN:
		WritePChar("frame binding functions ");
LAB_VARFUN_FRAME:
		Prin1(CreateFramePtr(frame));
		WritePChar(" binds (~ = dynamically):");
		Push(frame[FRAME_NEXT_ENV]);
		for (frame+=FRAME_BINDINGS; frame!=top; frame+=2)
			if (frame[0] & FLAG_ACTIVE) {
				WritePChar("\n  | ");
				if (frame[0] & FLAG_DYNAMIC)
					WriteChar('~');
				WriteChar(' ');
				Prin1(frame[0] & MASK_WITHOUT_FLAGS);
				WritePChar(" <--> ");
				Prin1(frame[1]);
			}
			PrintNextVarFunEnv(Pop());
			break;
	default:
		Throw(E_FAIL);
	}
	return top;
}

void CLispEng::F_DescribeFrame() {
	CP *frame = ToFrame(Pop());
	CP stm = SV;
	Call(S(L_TERPRI), stm);
	CStm sk(stm);
	PrintStackItem(frame);  
	Call(S(L_TERPRI), stm);
	SkipStack(1);
	m_cVal = 0;
}

void CLispEng::F_DescribeFrame0() {
	CP *frame = ToFrame(Pop());
	CStm sk(SV);
	PrintStackItem(frame);  
	SkipStack(1);
}

void CLispEng::F_ShowStack() {
	CP p = ToOptionalNIL(SV);
	CP *frameStart = p ? ToFrame(p) : m_pStack+2;
	size_t limit = ToOptionalNIL(SV1) ? AsNumber(SV1) : 0; //!!! must be positive
	int mode = ToOptionalNIL(SV2) ? AsNumber(SV2) : -1; //!!! must be positive
	SkipStack(3);
	CP stm = Spec(L_S_STANDARD_OUTPUT);
	CStm sk(stm);
	CP *frame = frameStart;
	for (size_t count=0; frame<m_pStackTop && (!limit || count<limit); count++) {
		CP *next = PrintStackItem(frame);
		frame = next;
		Call(S(L_TERPRI), stm);
	}
}

struct CApplyPfnKeeper {
	CLispEng::PFN_Apply m_prev;

	CApplyPfnKeeper() {
		CLispEng& lisp = Lisp();
		m_prev = lisp.m_mfnApply;
		lisp.m_mfnApply = &CLispEng::ApplyImp;
	}

	~CApplyPfnKeeper() {
		Lisp().m_mfnApply = m_prev;
	}
};

void CLispEng::ApplyTraced(CP fun, ssize_t nArg) {
	CTracedFuns::iterator it = m_tracedFuns.find(fun);
	if (it != m_tracedFuns.end()) {
		Keeper<int> levelKeeper(m_traceLevel, m_traceLevel+1);
		CP name = it->second;
		{
			CApplyPfnKeeper applyKeeper;
			for (int i=0; i<nArg; ++i)
				Push(m_pStack[nArg-1]);
			Call(S(L_TRACE_ENTER), name, CreateFixnum(m_traceLevel-1), Listof(nArg));
		}
		ApplyImp(fun, nArg);
		MvToStack();
		int cVal = m_cVal;
		{
			CApplyPfnKeeper applyKeeper;
			for (int i=0; i<cVal; ++i)
				Push(m_pStack[cVal-1]);
			Call(S(L_TRACE_EXIT), name, CreateFixnum(m_traceLevel-1), Listof(cVal));
		}
		StackToMv(cVal);
	} else
		ApplyImp(fun, nArg);
}

void CLispEng::F_UpdateTrace() {
	m_tracedFuns.clear();
	for (CP p=Spec(L_S_TRACE), car; SplitPair(p, car);) {
		Push(car);
		F_FDefinition();
		m_tracedFuns.insert(make_pair(m_r, car));
	}
	m_mfnApply = m_tracedFuns.empty() ? &CLispEng::ApplyImp : &CLispEng::ApplyTraced;
	m_r = 0;
}


/*!!!
void CLispEng::F_Signal(DWORD nArgs)
{
CP args = Listof(nArgs),
datum = Pop(); //!!! optimize
Call(GetSymbol("COERCE-TO-CONDITION", m_packSYS), datum, args, GetSymbol("SIGNAL", m_packCL), GetSymbol("SIMPLE-CONDITION", m_packCL));
Push(m_r);
Call(GetSymbol("SAFE-TYPEP", m_packSYS), m_r, AsSymbol(GetSymbol("*BREAK-ON-SIGNALS*", m_packCL))->m_dynVal));
if (m_r)
Call(get_Special(L_S_BREAK_DRIVER));
//!!! todo

}*/

} // Lisp::

