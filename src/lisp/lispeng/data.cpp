/*######   Copyright (c) 2002-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#include "lispeng.h"
#include "lisp_itf.h"

namespace Lisp {

#undef LSYM
#define LSYM(name, sym) name,

static const char * const g_arSymbol[] = {
#include "symdef.h"
	0
};


#undef LFUN
#define LFUN(n, a, r, o, p) &CLispEng::a,
#undef LFUN_REPEAT
#define LFUN_REPEAT(n, a, r, o, p) &CLispEng::a,

#undef LFUN_K
#define LFUN_K(n, a, r, o, p, k) &CLispEng::a,

#undef LFUNR
#define LFUNR(n, a, r, o, p) &CLispEng::a,

#undef LSO
#define LSO(n, a, r, o, rest, p) &CLispEng::a,
#undef LFUN_END
#define LFUN_END  0};

#undef LFUN_BEGIN_R
#define LFUN_BEGIN_R const CLispEng::FPLispFuncRest CLispEng::s_stFuncRAddrs[] = {

#undef LFUN_END_R
#define LFUN_END_R  0};

#undef LFUN_BEGIN_SO
#define LFUN_BEGIN_SO const CLispEng::FPLispFunc CLispEng::s_stSOAddrs[] = {

#undef LFUN_END_SO
#define LFUN_END_SO 0};

const CLispEng::FPLispFunc CLispEng::s_stFuncAddrs[SUBR_FUNCS] = {
#include "fundef.h"

#undef LFUN
#define LFUN(n, a, r, o, p) { n, r, o, p },
#undef LFUN_REPEAT
#define LFUN_REPEAT(n, a, r, o, p) { n, r, o, p },


#undef LFUN_K
#define LFUN_K(n, a, r, o, p, k) { n, r, o, p, (const byte*)k },

#undef LFUNR
#define LFUNR(n, a, r, o, p) { n, r, o, p },

#undef LSO
#define LSO(n, a, r, o, rest, p) { n, r, o, rest, p },

#undef LFUN_BEGIN_R
#define LFUN_BEGIN_R const CLispEng::CLispFuncR CLispEng::s_stFuncRInfo[] = {

#undef LFUN_BEGIN_SO
#define LFUN_BEGIN_SO const CLispEng::CLispSO CLispEng::s_stSOInfo[] = {

	const CLispEng::CLispFunc CLispEng::s_stFuncInfo[SUBR_FUNCS] = {
#include "fundef.h"

const CP g_fnEq = CLispEng::FindSubr(&CLispEng::F_Eq, 2, 0, 0),									// All Global inits in this file and order
	g_fnEql = CLispEng::FindSubr(&CLispEng::F_Eql, 2, 0, 0),
	g_fnEqual = CLispEng::FindSubr(&CLispEng::F_Equal, 2, 0, 0),
	g_fnEqualP = CLispEng::FindSubr(&CLispEng::F_EqualP, 2, 0, 0),
	g_fnEqNum = CLispEng::FindSubr((CLispEng::FPLispFunc)&CLispEng::F_EqNum, 1, 0, 1);

CP CLispEng::CreateSpecialOperator(uint32_t n) {
	const CLispSO& func = s_stSOInfo[n];
	return func.m_name ? CreateSpecialOperator(n, func.m_nReq, func.m_nOpt, func.m_bRest) : 0;
}

CP CLispEng::CreateSubr(uint32_t n) {
	if (n < SUBR_FUNCS) {
		const CLispFunc& func = s_stFuncInfo[n];
		return func.m_name ? CreateSubr(n, func.m_nReq, func.m_nOpt, 0, func.m_keywords) : 0;
	} else {
		const CLispFuncR& func = s_stFuncRInfo[n-SUBR_FUNCS];
		return func.m_name ? CreateSubr(n, func.m_nReq, func.m_nOpt, true) : 0;
	}
}

CP CLispEng::FindSubr(FPLispFunc pfn, byte nReq, byte nOpt, byte bRest) {
	for (size_t i=0; i<size(s_stFuncAddrs); i++)
		if (s_stFuncAddrs[i] == pfn)
			return CreateSubr(i, nReq, nOpt, bRest, 0);
	for (size_t i=0; i<size(s_stFuncRAddrs); i++)
		if ((FPLispFunc)s_stFuncRAddrs[i] == pfn)
			return CreateSubr(i+SUBR_FUNCS, nReq, nOpt, bRest, 0);
	E_Error();
}

CP CLispEng::PackageFromIndex(int idx) {
	switch (idx) {
	case 0: return m_packSYS;
	case 1: return m_packCL;
	case 2: return m_packCLOS;
	case 3: return m_packEXT;
	case 4: return m_packGray;
	case 5: return m_packCustom;
#if UCFG_LISP_FFI
	case 7: return m_packCffiSys;
#endif
	default:
		Throw(E_FAIL);
	}
}

void CLispEng::F_FunTabRef() {
	size_t idx = AsPositive(Pop());
	if (idx < size(s_stFuncInfo)) {
		const CLispFunc& lispFunc = s_stFuncInfo[idx];
		if (lispFunc.m_name)
			m_r = GetSymbol(lispFunc.m_name, PackageFromIndex(lispFunc.m_bCL));
	} else {
		idx -= SUBR_FUNCS;
		if (idx < size(s_stFuncRInfo)) {
			const CLispFuncR& lispFunc = s_stFuncRInfo[idx];
			if (lispFunc.m_name)
				m_r = GetSymbol(lispFunc.m_name, PackageFromIndex(lispFunc.m_bCL));
		} else
			m_r = 0;
	}
}

void CLispEng::CommonInit() {
	//!!!  m_specRPar      = FromSValue(m_consMan.m_pBase+1);
	//!!!  m_specUnbound = FromSValue(m_consMan.Base+2);
	//!!!  m_specDisabled = FromSValue(m_consMan.Base+3);
	//!!!  m_specSpecDecl = FromSValue(m_consMan.Base+4);
	m_specDot = FromSValue(m_consMan.Base+5);

	m_packSYS     = m_mapPackage["SYS"];
	m_packCL      = m_mapPackage["CL"];
	m_packEXT     = m_mapPackage["EXT"];
	m_packCustom     = m_mapPackage["CUSTOM"];
	m_packGray     = m_mapPackage["GRAY"];
	m_packKeyword = m_mapPackage["KEYWORD"];
	m_packCLOS    = m_mapPackage["CLOS"];
	//!!!  m_packCompiler = m_mapPackage["COMPILER"];
}

CLispSymbol g_arStreamSymbol[] = {ENUM_L_S_STANDARD_INPUT, ENUM_L_S_STANDARD_OUTPUT, ENUM_L_S_ERROR_OUTPUT, ENUM_L_S_TRACE_OUTPUT, ENUM_L_S_DEBUG_IO, ENUM_L_S_QUERY_IO, ENUM_L_S_TERMINAL_IO};

CP CLispEng::CreateStandardStream(CEnumStream n) {
	CStreamValue *stm = CreateStream();
	stm->m_nStandard = n;
	return FromSValue(stm);
}

#ifdef WIN32
void CLispEng::put_Stream(CEnumStream idx, ptr<StandardStream> p) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_streams[idx] = p;
	if (m_bInited)
		SetSpecial(get_Sym(g_arStreamSymbol[idx]), CreateStandardStream(idx));
}
#endif

#if UCFG_OLE
void CLispEng::put_Stream(CEnumStream idx, IStandardStream *iStream) {
	put_Stream(idx, new ComStandardStream(iStream));
}

void LispSetStandardStream(LISPHANDLE h, int idx, void *stdStream) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
		CLispEng& lisp = *(CLispEng*)h;
	IStandardStream *stm = (IStandardStream*)stdStream;
	lisp.put_Stream((CEnumStream)idx, stm);
}

#endif

void CLispEng::SetPathVars() {
	{
		CListConstructor lc;
		SetSpecial(S(L_S_DEFAULT_PATHNAME_DEFAULTS), 0);  	//  to prevent self-loop
		SetSpecial(S(L_S_DEFAULT_PATHNAME_DEFAULTS), CreatePathname(AddDirSeparator(current_path())));

		for (size_t i=0; i<LoadPaths.size(); ++i)
			lc.Add(CreatePathname(AddDirSeparator(LoadPaths[i].ToOsString())));
#if !UCFG_WCE
		String lispinc = Environment::GetEnvironmentVariable("LISPINC");
		if (!!lispinc) 	{
			vector<String> ar = lispinc.Split(String(Path::PathSeparator));
			for (size_t i=0; i<ar.size(); i++)
				lc.Add(CreatePathname(AddDirSeparator(ar[i].ToOsString())));
		}
#endif
		SetSpecial(GetSymbol("*LOAD-PATHS*", m_packCustom), lc);
	}
}

pair<String, CP> CLispEng::GetStaticSymbolNamePack(size_t i) {
	const char *p = nullptr;
	CP pack = 0;
	if (i < size(g_arSymbol)-1) {
		p = g_arSymbol[i];
		pack = m_packCL;
		switch (p[0]) {
		case '^':
			p++;
		case '%':
			pack = m_packSYS;
			break;
		case '$':
			pack = m_packEXT;
			p++;
			break;
		case '@':
			pack = m_packGray;
			p++;
			break;
		case '~':
			pack = m_packCustom;
			p++;
			break;
		case '#':
			pack = m_packCLOS;
			p++;
			break;
		case ':':
			pack = m_packKeyword;
			p++;
			break;
#if UCFG_LISP_FFI
		case '!':
			pack = m_packCffiSys;
			p++;
			break;
#endif
		}
	}
	return pair<String, CP>(String(p), pack);
}

void CLispEng::InitVars() {
	Cons(0, 0); //!!! NIL
	//!!!  m_specRPar      = CreateEmptyCons();
	Cons(0, 0); // V_EOF
	Cons(0, 0); // V_U
	Cons(0, 0); // V_D
	Cons(0, 0); // V_SPEC
	m_specDot = Cons(0, 0);

	vector<String> nicks;
	nicks.push_back("SYS");
	nicks.push_back("COMPILER");
	m_packSYS = FromSValue(CreatePackage("SYSTEM", nicks));
	nicks.clear();
	nicks.push_back("CL");
	nicks.push_back("LISP");
	nicks.push_back("CS-CL"); //!!!
	m_packCL = FromSValue(CreatePackage("COMMON-LISP", nicks));
	m_packCLOS = FromSValue(CreatePackage("CLOS", vector<String>()));
	m_packEXT = FromSValue(CreatePackage("EXT", vector<String>()));
	m_packGray = FromSValue(CreatePackage("GRAY", vector<String>()));
	m_packCustom = FromSValue(CreatePackage("CUSTOM", vector<String>()));

	ToPackage(m_packSYS)->m_useList = List(m_packCL, m_packEXT, m_packCustom);  
	ToPackage(m_packEXT)->m_useList = List(m_packCL, m_packCustom);
	ToPackage(m_packCLOS)->m_useList = List(m_packCL, m_packSYS);

	CreatePackage("KEYWORD", vector<String>());

#if UCFG_LISP_FFI
	m_packCffiSys = FromSValue(CreatePackage("CFFI-SYS", vector<String>()));
	ToPackage(m_packCffiSys)->m_useList = List(m_packCL, m_packSYS);
#endif

	CreatePackage("CHARSET", vector<String>());

	CommonInit();

	size_t i;
	for (i=0; i<size(g_arSymbol)-1; i++) {
		pair<String, CP> pp = GetStaticSymbolNamePack(i);
		GetSymbol(pp.first, pp.second);
	}
	//!!!  ToSymbol(Syms[L_NIL])->m_s = "NIL";

	SetConstant(0, 0);
	SetConstant(V_T, V_T);

	SetSpecial(S(L_S_FEATURES), 0);

#if UCFG_WIN32_FULL
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hCon != INVALID_HANDLE_VALUE) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (::GetConsoleScreenBufferInfo(hCon, &csbi))
			SetSpecial(S(L_S_PRIN_LINELENGTH), CreateFixnum(csbi.dwSize.X-1));
	}
#endif

	SetVars();

	for (i=0; i<size(s_stSOInfo); i++) {
		const CLispSO& func = s_stSOInfo[i];
		if (!func.m_name)
			break;
		ToSymbol(GetSymbol(func.m_name, PackageFromIndex(func.m_bCL)))->SetFun(CreateSpecialOperator(i));
	}
	for (i=0; i<size(s_stFuncInfo); i++) {
		const CLispFunc& func = s_stFuncInfo[i];
		if (!func.m_name)
			break;
		ToSymbol(GetSymbol(func.m_name, PackageFromIndex(func.m_bCL)))->SetFun(CreateSubr(i));
	}
	for (i=0; i<size(s_stFuncRInfo); i++) {
		const CLispFuncR& func = s_stFuncRInfo[i];
		if (!func.m_name)
			break;
		ToSymbol(GetSymbol(func.m_name, PackageFromIndex(func.m_bCL)))->SetFun(CreateSubr(i+SUBR_FUNCS));
	}

	CP stmIn = CreateStandardStream(STM_StandardInput),
		stmOut = CreateStandardStream(STM_StandardOutput),
		stmErr = CreateStandardStream(STM_ErrorOutput);				 
	SetSpecial(S(L_S_TERMINAL_IO), FromSValue(CreateStream(STS_TWO_WAY_STREAM, stmIn, stmOut, STREAM_FLAG_INPUT|STREAM_FLAG_OUTPUT)));
	CP termSyn = CreateSynonymStream(S(L_S_TERMINAL_IO));
	SetSpecial(S(L_S_QUERY_IO), termSyn);
	SetSpecial(S(L_S_DEBUG_IO), termSyn);
	SetSpecial(S(L_S_TRACE_OUTPUT), termSyn);
	SetSpecial(S(L_S_ERROR_OUTPUT), stmErr);
	SetSpecial(S(L_S_STANDARD_INPUT), termSyn);
	SetSpecial(S(L_S_STANDARD_OUTPUT), termSyn);

	/*!!!
	for (i=0; i!=STM_TerminalIO; i++)
	SetSpecial(get_Sym(g_arStreamSymbol[i]), CreateStandardStream(CEnumStream(i)));
	*/


	SetSpecial(S(L_S_READ_REFERENCE_TABLE), 0);
	SetSpecial(S(L_S_LOAD_VERBOSE), FromBool(m_bVerbose));

	CPathname *pn = m_pathnameMan.CreateInstance();
	pn->LogicalP = true;
	SetSpecial(GetSymbol("_*EMPTY-LOGICAL-PATHNAME*", m_packSYS), FromSValue(pn));

	{
		CSymbolValue *sv = ToSymbol(GetSymbol("*BIG-ENDIAN*", m_packSYS));
		sv->m_dynValue = 0;
		sv->m_fun |= SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL;
	}

	SetSpecial(S(L_S_READ_BASE), V_10);
	SetSpecial(S(L_S_READ_EVAL), V_T);

	m_indent = 0;
	SetSpecial(S(L_S_PACKAGE), m_packSYS);
	//!!!  SetSpecial(S(L_S_MACROEXPAND_HOOK), FindSubr((FPLispFunc)F_Funcall, 1, 0, 1));

	SetConstant(S(L_MOST_POSITIVE_FIXNUM), CreateInteger(FIXNUM_LIMIT-1));


	CP epsilon       =  FromFloat(DBL_EPSILON),
		negEpsilon    = FromFloat(DBL_EPSILON),
		mostPositive  = FromFloat(DBL_MAX),
		leastPositive = FromFloat(DBL_MIN),
		mostNegative  = FromFloat(-DBL_MAX),
		leastNegative = FromFloat(-DBL_MIN);

	SetConstant(S(L_SINGLE_FLOAT_EPSILON)               , epsilon);
	SetConstant(S(L_SINGLE_FLOAT_NEGATIVE_EPSILON) 		, negEpsilon);
	SetConstant(S(L_MOST_POSITIVE_SINGLE_FLOAT)         , mostPositive);
	SetConstant(S(L_LEAST_POSITIVE_SINGLE_FLOAT)        , leastPositive);
	SetConstant(S(L_MOST_NEGATIVE_SINGLE_FLOAT)			, mostNegative);
	SetConstant(S(L_LEAST_NEGATIVE_SINGLE_FLOAT)        , leastNegative);
	SetConstant(S(L_LEAST_POSITIVE_NORMALIZED_SINGLE_FLOAT), leastPositive);
	SetConstant(S(L_LEAST_NEGATIVE_NORMALIZED_SINGLE_FLOAT), leastNegative);

	SetConstant(S(L_INTERNAL_TIME_UNITS_PER_SECOND), CreateInteger64(UCFG_LISP_INTERNAL_TIME_UNITS_PER_SECOND::period::den));

	SetPathVars();

	InitReader();

	SetConstant(S(L_MULTIPLE_VALUES_LIMIT), CreateFixnum(MULTIPLE_VALUES_LIMIT));

	SetSpecial(S(L_S_GENSYM_COUNTER), V_0);

	SetSpecial(S(L_S_EVALHOOK), 0);
	SetSpecial(S(L_S_APPLYHOOK), 0);

	SetSpecial(S(L_S_BREAK_DRIVER), AsSymbol(S(L_DEFAULT_BREAK_DRIVER))->GetFun());

//!!!R	SetSpecial(S(L_S_RANDOM_STATE), FromSValueT(CreateRandomState(0), TS_RANDOMSTATE));

	/*!!!  CLispSymbol arLKeys[] = {L_ALLOW_OTHER_KEYS, L_AUX, L_BODY, L_ENVIRONMENT, L_KEY, L_OPTIONAL, L_REST, L_WHOLE};
	for (i=0; i<size(arLKeys); i++)
	ToSymbol(Syms[arLKeys[i]])->m_bLambdaKeyword = true;*/
	m_bVarsInited = true;
}

void CLispEng::SetVars() {
	CommonInit();

	m_sWhitespace = GetSymbol("WHITESPACE", m_packKeyword);
	m_sConstituent = GetSymbol("CONSTITUENT", m_packKeyword);
	m_sMacro = GetSymbol("MACRO", m_packKeyword);
	m_sInvalid = GetSymbol("INVALID", m_packKeyword);
	m_sSescape = GetSymbol("SESCAPE", m_packKeyword);
	m_sMescape = GetSymbol("MESCAPE", m_packKeyword);
	//!!!  m_sGensymCounter = GetSymbol("*GENSYM-COUNTER*");

	{
		CListConstructor lc;
		for (size_t i=0; i<m_arCommandLineArg.size(); i++)
			lc.Add(CreateString(m_arCommandLineArg[i]));
		SetSpecial(GetSymbol("*ARGS*", m_packEXT), lc);
	}

	SetSpecial(GetSymbol("*LOAD-COMPILING*", m_packEXT), FromBool(m_bLoadCompiling));

	{
		CP p = Spec(L_S_FEATURES);
		for (set<String>::iterator i=Features.begin(); i!=Features.end(); ++i) {
			CP k = GetKeyword(*i);
			if (!Memq(k, p))
				SetSpecial(S(L_S_FEATURES), p = Cons(k, p));
		}		
	}

	SetPathVars();
}


} // Lisp::


