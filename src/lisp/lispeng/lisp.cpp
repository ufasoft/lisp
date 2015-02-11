#include <el/ext.h>

#include EXT_HEADER_FILESYSTEM

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_USE_POSIX
#	include <getopt.h>
#endif

#include "lispeng.h"
#include "lisp_itf.h"

namespace Lisp {

CLisp::CLisp()
	:	m_bProfile(false)
	,	m_bDebug(false)
	,	m_bTraceError(false)
	,	m_exitCode(0)
	,	m_bVerifyVersion(true)
{
	Signal = 0;
	m_streams.resize(7);

//!!!  m_pSink = &m_sink;
}

CLisp::~CLisp() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
}

CLisp *CLisp::CreateObject() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	return new CLispEng;
}

CLispEng::CLispEng()
	:	m_bLockGC(false)
	,	m_bShowEval(false)
	//!!!	, m_r(m_arVal[0])
	,	m_traceLevel(0)
	,	m_bRun(false)
	,	m_bTrace(false)
	,	m_bVerbose(UCFG_DEBUG)
	,	m_bCompile(false)
	,	m_bInit(false)
	,	m_bBuild(false)
	,	m_bExpandingLambda(false)
	,	m_level(0)
	,	m_pStackBase(new CP[LISP_STACK_SIZE])
	,	m_pStackWarning(m_pStackBase+STACK_WARNING_SIZE)
	,	m_pStackTop(m_pStackBase+LISP_STACK_SIZE)
	,	m_pSPBase(new ssize_t[LISP_SP_SIZE])
	,	m_pSPTop(m_pSPBase+LISP_SP_SIZE)
	,	m_pSP(m_pSPTop)
	#if UCFG_LISP_FAST_HOOKS
	,	m_mfnApply(&CLispEng::ApplyImp)
	,	m_mfnEval(&CLispEng::EvalImp)
	#endif
	,	m_locale("")
	#if UCFG_LISP_FAST_EVAL_ATOMS == 2
	,	m_maskEvalHook((CP(2)<<(sizeof(CP)*8-VALUE_SHIFT))-2)
	#endif
	//!!!   m_alloc(_self)
{
#if UCFG_WCE
	m_initDir = AddDirSeparator(System.GetExeDir());
#endif


#if UCFG_LISP_TAIL_REC == 2
	m_stackTailRec.push_back(false);
#endif

	m_cVal = 1;
	m_pStack = m_pStackTop;
	ZeroStruct(m_arVal);
	{
		Features.insert("UFASOFT-LISP");
//!!!R		Features.insert("CLISP");

		Features.insert("ANSI-CL");
	//!!! added by module	Features.insert("CLOS");
	//!!! added by module	Features.insert("LOOP");

#ifdef WIN32
		Features.insert("WIN32");
#	ifdef _M_IX86
		Features.insert("PC386");
#	endif
#endif

		Features.insert("COMMON-LISP");
		Features.insert("LOGICAL-PATHNAMES");
		Features.insert("IEEE-FLOATING-POINT");

		Features.insert("INTERPRETER");

#if UCFG_LISP_FFI
		Features.insert("CFFI-SYS");
#endif

//!!!?		Features.insert("UNICODE");

#if UCFG_LISP_DEBUG
		Features.insert("CLISP-DEBUG"); //!!!
#endif	

#ifdef C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR
		Features.insert("ULISP-BACKQUOTE-AS-SPECIAL-OPERATOR");
#endif
		//!!! Features.insert("UNICODE");
	}


#if UCFG_USE_READLINE
	if (isatty(fileno(stdin)))
		m_streams[STM_StandardInput] = new TerminalInputStream(stdin);
	else
#endif
		m_streams[STM_StandardInput] = new InputStream(stdin);

	m_streams[STM_StandardOutput] = new OutputStream(stdout);
	m_streams[STM_ErrorOutput] = new OutputStream(stderr);

	m_dtStart = DateTime::UtcNow();
	m_spanStartUser = Thread::CurrentThread->TotalProcessorTime;

//!!!  Init();
}

CLispEng::~CLispEng() {
	Clear();
	for (size_t i=0; i<m_arValueMan.size(); ++i)
		m_arValueMan[i]->Destroy();

//!!!  Collect();
	delete[] m_pStackBase;
	delete[] m_pSPBase;
	
//!!!  delete[] m_pSPBase;
}

void CLispEng::Load(const path& fileName, bool bBuild) {
	String ext = fileName.extension().native().ToLower();
	path p = fileName;

#if UCFG_USE_POSIX
	path mypath = System.ExeFilePath;
#else
	path mypath = AfxGetModuleState()->FileName;
#endif
	path exeDir = mypath.parent_path();
	if (ext.empty()) {
		path bin = exeDir / fileName.stem();
		bin += ".bls";
		if (!(bBuild || m_bBuild)) {
			if (exists(bin))
				p = bin;
		}
		/*!!!
		if (fileName.ToLower() == "init")
		{
			if (m_initDir.IsEmpty())
			m_initDir = AddDirSeparator(ExtractFilePath(FindInLISPINC("init.lisp", exeDir)));
			text = m_initDir+"init.lisp";
			bin = exeDir+filenName+".bls";
		}*/
	}

/*!!!
    if (m_initDir.IsEmpty())
			m_initDir = AddDirSeparator(ExtractFilePath(FindInLISPINC("init.lisp", exeDir)));
		text = m_initDir+"init.lisp";
		bin = exeDir+"init.bls";
		*/

//!!!  CDirectoryKeeper dirKeeper(ExtractFilePath(text));



	vector<String> arModule;
	CBoolKeeper keeper(m_bInit);

	if (p.extension().native().ToLower() == ".bls") {
		LoadImage(p);

		for (int i=STM_StandardInput; i<=STM_TerminalIO; i++)
			if (m_streams[i])
				SetSpecial(IDX_TS(ENUM_L_S_STANDARD_INPUT+i, TS_SYMBOL), CreateStandardStream(CEnumStream(i)));

		/*!!!
		CReadFileStream fstm(path);
		arModule = ReadBinHeader(fstm);
		String dir = AddDirSeparator(ExtractFilePath(path));
//!!!		for (int i=0; i<arModule.size(); i++) {
				path p = dir / arModule[i];
				if (exists(p) && last_write_time(p) > tmBin)
//!!!					goto out;
			}
		LoadMem(fstm);
		SetVars();
		m_arModule = arModule;*/
	} else {
		InitValueMans();
		InitVars();
		m_arModule.push_back(p.filename());
		{
//!!!R			CDynBindFrame bindDef(S(L_S_DEFAULT_PATHNAME_DEFAULTS), FromSValue(CreatePathname(m_initDir)), true);
			if (SearchFile(p).empty())
				Throw(E_LISP_NoInitFileFound);
			LoadFile(p);
		}
		String bin;
		if (!m_outfile.empty()) {
			if (!m_arg.empty())
				return;
			bin = m_outfile;
		} else {
			bin = exeDir / fileName.stem();
			bin += ".bls";
		}
		TRC(1, bin);
		Ext::FileStream fs(bin, FileMode::Create, FileAccess::Write);
		SaveMem(fs);
	}
//!!!  m_initDir = "";
}

void CLispEng::SetSpecial(CP sym, CP val) {
	CSymbolValue *sv = ToSymbol(sym);
	(sv->m_fun &= ~(SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL)) |= SYMBOL_FLAG_SPECIAL;
	SetSymVal(sym, val);
}

void CLispEng::SetConstant(CP sym, CP val) {
	CSymbolValue *sv = ToSymbol(sym);
	sv->m_fun |= SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL;
	sv->m_dynValue = val;
}

void CLispEng::InitStreams() {
	Clear();
	/*!!!
	for (int i=STM_StandardInput; i<STM_TerminalIO; i++)
	if (!m_arStream[i])
	  m_arStream[i] = m_arStream[STM_TerminalIO];
			*/
}

void CLispEng::Load(Stream& stm) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	InitStreams();
	ReadBinHeader(BinaryReader(stm));
	LoadMem(stm);
	SetVars();
	m_bInited = true;
}

void CLispEng::ParseArgs(char **argv) {
	for (; *argv; argv++)
		m_arCommandLineArg.push_back(*argv);
}

void CLispEng::LoadImage(RCString filename) {
	TRC(0, "Loading image " << filename);

	Ext::FileStream fs(filename, FileMode::Open, FileAccess::Read);
	Load(fs);
}

void CLispEng::Init(bool bBuild) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInited) {
		InitStreams();
		Load("init", bBuild);
		OnInit();
		m_bInited = true;
	}
}

void CLispEng::Clear() {
	ClearResult();
	m_mapPackage.clear();
	m_setPackage.clear();
	m_specDot = 0;
}

void CLispEng::SetSignal(bool bSignal) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
//!!!  m_bSignal = bSignal;
}

String CLispEng::Eval(RCString sForm) {
	Init(false); // to init by default
	Call(GetSymbol("_EVAL-STRING", m_packSYS), CreateString(sForm));
	return m_r ? AsString(m_r) : String(nullptr);
}

void CLispEng::InstallGlobalHandlers(on_error_t onErr) {
	if (!AsSymbol(S(L_SET_GLOBAL_HANDLER))->GetFun())
		return;
	switch (onErr) {
	case ON_ERROR_EXIT:
		Call(S(L_SET_GLOBAL_HANDLER), S(L_INTERRUPT_CONDITION), AsSymbol(S(L_EXITUNCONDITIONALLY))->GetFun());
		Call(S(L_SET_GLOBAL_HANDLER), S(L_SERIOUS_CONDITION), AsSymbol(S(L_EXITONERROR))->GetFun());
		break;
	default:
		Throw(E_FAIL);
	}
	Call(S(L_SET_GLOBAL_HANDLER), S(L_ERROR), AsSymbol(S(L_APPEASE_CERROR))->GetFun());
}

//!!!R #include <getopt.h>

int g_bOptVersion, g_bOptNoLogo, g_bOptNoRc,
			g_bOptDD, g_bOptLP;

const int OPT_LP = 257;

static option s_long_options[] = {
	{"version"	, no_argument,       &g_bOptVersion	, 1},
	{"nologo"	, no_argument,       &g_bOptNoLogo	, 1},
	{"norc"		, no_argument,       &g_bOptNoRc	, 1},
	{"lp", required_argument,       0, OPT_LP},
#if UCFG_LISP_DEBUGGER
	{"dd", no_argument,       &g_bOptDD, 1},
#endif
	{0, 0, 0, 0}
};

void CLispEng::ProcessCommandLineImp(int argc, char *argv[]) {
	try {
	#ifdef _X_DEBUG//!!!D
		cout << argc << endl;
		for (int i=0; i<argc; i++)
			cout << argv[i] << endl;
	#endif
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		vector<pair<String, String> > arForCompile;
		vector<String> preloads;
		vector<String> exprs;
		
		while (true) {
			int option_index = 0;
			int ch = getopt_long(argc, argv, "bCc:D:hi:l:M:o:O:ptTU:vx:?", s_long_options, &option_index);
			if (ch == -1)
				break;
		//for (int ch; (ch = getopt(argc, argv, "bCc:D:hi:M:o:O:ptTU:x:?")) != -1;)
			switch (ch) {
			case 0:
				break;
			case 'b':
				m_bBuild = true;
				break;
			case 'D':
				Features.insert(String(optarg).ToUpper());
				break;
			case 'U':
				{
					String feature(optarg);
					feature.MakeUpper();
					Features.erase(feature);
				}
				break;
			case 'x':
				exprs.push_back(optarg);
				break;
			case 'i':
				m_initDir = AddDirSeparator(optarg);
				break;
			case 'l':
				preloads.push_back(optarg);
				break;
			case 'M':
				LoadImage(optarg);
				break;
			case 'C':
				m_bLoadCompiling = true;
				break;
			case 'c':
				arForCompile.push_back(pair<String, String>(optarg, ""));
				break;
			case 'o':
				if (!arForCompile.empty() && arForCompile.back().second.empty())
					arForCompile.back().second = optarg;
				else
					m_outfile = optarg;
				break;
			case 'O':
				m_destFile = optarg;
				break;
			case 'p':
				m_bProfile = true;
				break;
			case 't':
				m_bTraceError = true;
				break;
			case 'T':
				m_bTrace = true;
				break;
			case 'v':
				m_bVerbose = true;
				break;
			case OPT_LP:
				LoadPaths.push_back(optarg);
				break;
			case 'h':
			case '?':
			default:
				Throw(1); //!!!
			}
		}

		m_bPrintLogo = !g_bOptNoLogo;
	#if UCFG_EXTENDED
		if (g_bOptVersion) {
			FileVersionInfo vi;
			cerr << vi.FileDescription << ' ' << vi.GetProductVersionN().ToString(3) << '\t' << vi.LegalCopyright;
			String url = TryGetVersionString(vi, "URL");
			if (!url.empty())
				cerr << "\t" << url;
			cerr << endl;
			return;
		}
	#endif

		if (argv[optind]) {
			char **p = &argv[optind];
			m_arg = String(*p++).Trim();
			ParseArgs(p);
		}
		Init(false);
		bool bBatchMode = !arForCompile.empty() || !exprs.empty() || !m_arg.empty();
		if (bBatchMode) {
			InstallGlobalHandlers(ON_ERROR_EXIT);
		}
		if (!g_bOptNoRc && m_bInited) {
			Push(V_U);
			F_UserHomedirPathname();

			Push(V_U, V_U, V_U, CreateString(".lisprc"));
			Push(V_U, V_U, m_r, V_U, V_U);
			F_MakePathname();

			Push(m_r, S(L_K_IF_DOES_NOT_EXIST), 0);
			Funcall(S(L_LOAD), 3);
		}
		for (size_t i=0; i<arForCompile.size(); ++i) {
			pair<String, String>& p = arForCompile[i];
			Push(CreateString(p.first));
			Push(S(L_K_VERBOSE));
			Push(V_T);
			int nArg = 3;
			if (!p.second.empty()) {
				Push(S(L_K_OUTPUT_FILE));
				Push(CreateString(p.second));
				nArg += 2;
			}
			Funcall(S(L_COMPILE_FILE), nArg);
			if (!m_r)
				Throw(3);
		}

		for (size_t i=0; i<preloads.size(); ++i)
			LoadFile(preloads[i]);

		if (!exprs.empty()) {
			String ins;
			for (size_t i=0; i<exprs.size(); ++i)
				ins += exprs[i];
			Push(CreateString(ins), V_U, V_U);
			F_MakeStringInputStream();
			CDynBindFrame bindDef(S(L_S_STANDARD_INPUT), m_r, true);
			Call(Spec(L_S_DRIVER));
		} else if (!m_arg.empty()) {
			if (!m_destFile.empty()) {
				Push(CreateString(m_arg));
#if UCFG_USE_POSIX
				path mypath = System.ExeFilePath;
#else
				path mypath = AfxGetModuleState()->FileName;
#endif
				Push(CreateString(mypath.parent_path() / "constub.bin"));
				Push(CreateString(m_destFile));
				Funcall(GetSymbol("MAKE-EXE", m_packEXT), 3);
			} else {
				LoadFile(m_arg);
				if (!m_outfile.empty()) {
					Ext::FileStream fs(m_outfile, FileMode::Create, FileAccess::Write);
					SaveMem(fs);
				}
			}
	#ifdef _DEBUG	//!!!T
	//		Loop();
	#endif 
		} else if (arForCompile.empty())
			Loop();
	} catch (StackOverflowExc e) {
		cerr << e << endl;		
		exit(e.code().value());
	}
}

#if UCFG_WIN32

static int ExcFilter(EXCEPTION_POINTERS *ep) {
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	if (er->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {

#	if UCFG_LISP_FAST_HOOKS
		CLispEng& lisp = Lisp();
		lisp.m_mfnApply = &CLispEng::ApplyHooked;
#	endif
		lisp.Signal = HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW);
		lisp.StackOverflowAddress = (void*)er->ExceptionInformation[1];
		return EXCEPTION_CONTINUE_EXECUTION;
	} else
		return EXCEPTION_CONTINUE_SEARCH;
}


void CLispEng::ProcessCommandLine(int argc, char *argv[]) {
	__try {
		ProcessCommandLineImp(argc, argv);
	} __except(ExcFilter(GetExceptionInformation())) {
	}
}

#else
void CLispEng::ProcessCommandLine(int argc, char *argv[]) {
	ProcessCommandLineImp(argc, argv);
}
#endif

int CLispEng::Run() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	Loop();
	//!!!Call(GetSymbol("_RUN", m_packSYS));
	return AsNumber(m_r);
}

void CLispEng::Compile(RCString name) {
	/*!!!
  AFX_MANAGE_STATE(AfxGetStaticModuleState())
  CStreamValue *pStm = CreateStream();
  pStm->m_subtype = STS_FILE_STREAM;
  CFileITextStream *p = new CFileITextStream(name);
  pStm->m_pStm.SetOwned(p);
  Call(GetSymbol("_COMPILE-STREAM", m_packSYS), FromSValue(pStm));
	*/
}

void CLispEng::Loop() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	try {
		Call(Spec(L_S_DRIVER));
	} catch (RCExc ex) {
		if (HResultInCatch(ex) != E_EXT_NormalExit) 
			throw;
	}
}

void CLispEng::SaveImage(Stream& stm) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	SaveMem(stm);
}

void CLispEng::F_GetFilePosition(CP args) {
//!!!  m_r = CreateInteger(short(ToStream(Car(args))->m_pis->tellg()));//!!!
}

/*!!!R
class CReadFileStream : public Stream {
	File m_file;
	mutable CMappedFile m_mf;

	mutable size_t m_size,
			m_off;
public:
	CReadFileStream(RCString fileName);
	void ReadBuffer(void *buf, size_t count) const;
	void WriteBuffer(const void *buf, size_t count);
	bool Eof() const;
};

CReadFileStream::CReadFileStream(RCString fileName)
	:	m_off(0)
{
	m_file.Open(fileName, FileMode::Open, FileAccess::ReadWrite, FileShare::ReadWrite);		//!!!
	m_mf.Map(m_file, m_size = (DWORD)m_file.Length);
}

void CReadFileStream::ReadBuffer(void *buf, size_t count) const {
	if (m_off+count > m_size)
		E_Error();
	memcpy(buf, m_mf.Address+m_off, count);
	m_off += count;
}

void CReadFileStream::WriteBuffer(const void *buf, size_t count) {
	E_Error();
}

bool CReadFileStream::Eof() const {
	return m_off >= m_size;
}
*/

/*!!!D
void CLispEng::Load()
{
  while (true)
  {
    CSPtr p = ReadSValue(SV, false, m_specEOF);
    if (p == m_specEOF)
      break;
//		cerr << '\n';
//    PrintForm(cerr, p);//!!!
    //!!!cout << "\n\n";//!!!
    Eval(p);
  }
}

void CLispEng::Load(CP sym)
{
  Call("LOAD", sym);
}*/

void CLispEng::LoadFile(RCString path) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	if (CP fun = ToSymbol(S(L_LOAD))->GetFun())
		Call(S(L_LOAD), CreateString(path));
	else
		Call(GetSymbol("_LOAD", m_packSYS), CreateString(path));
}

class CIntIncrementor {
	int& m_i;
public:
	CIntIncrementor(int& i)
		:	m_i(i)
	{
		m_i++;
	}

	~CIntIncrementor() {
		m_i--;
	}
};

/*!!!D
void CLispEng::F_LispName()
{
  String fn = AsString(Pop());
  if (ExtractFileExt(fn) == "")
    fn += ".lisp";
  m_r = CreateString(fn);
}*/

CSPtr CLispEng::DestructiveAppend(CP p, CP q) {
	if (p) {
		CSPtr r = p;
		while (Cdr(r))
			Inc(r);
		ToCons(r)->m_cdr = q;
		return p;
	}
	return q;  
}

void CLispEng::ReplaceA(CP p, CP q) {
	ToCons(p)->m_car = q;
}

bool CLispEng::CheckType(CP typChain, CP formTyp) {
	if (formTyp == V_T)
		return true;
	if (typChain) {
		CSPtr typ = Car(typChain);
		if (typ == formTyp)
			return true;
		return CheckType(Cdr(typChain), formTyp);
	}
	return false;
}

void CLispEng::TypepCheck(CP datum, CP typ) {
	Call(S(L_TYPEP), datum, typ);
	if (!m_r) {
		E_TypeErr(datum, typ);
	}
}

CP CLispEng::CheckInteger(CP p) {
	if (!IntegerP(p))
		E_TypeErr(p, S(L_INTEGER));
	return p;
}

CSPtr CLispEng::FindPair(CP slots, CP name) {
	for (CSPtr p=slots; p; Inc(p)) {
		CSPtr car = Car(p);
		if (Car(car) == name)
			return car;
	}
	return 0;
}

void CLispEng::RemoveSlot(CSPtr& p, CP slot) {  
	for (CSPtr *q = &p; *q; q=(CSPtr*)&ToCons(*q)->m_cdr)
		if (Car(Car(*q)) == slot) {
			*q = Cdr(*q);
			return;
		}
}

CP CLispEng::VGetSymbol(RCString name, RCString pack) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CLispThreadKeeper lispThreadKeeper(this);
	CSPtr p;
	if (!Lookup(m_mapPackage, pack, p))
		E_Error();
	return GetSymbol(name, p);
}

void CLispEng::VCall(CP p) {
	CLispThreadKeeper lispThreadKeeper(this);
	Funcall(p, 0);
}



#if UCFG_OLE

int ComStandardStream::get() {
	wchar_t ch;
	HRESULT hr = m_iSS->get(&ch);
	if (hr == 0x80131519) //!!!R
		return -1;
	OleCheck(hr);
	return char_traits<wchar_t>::to_int_type(ch);
}

#endif

#if UCFG_LISP_MT
	EXT_THREAD_PTR(CLispEng) t_pLisp;
#else
	CLispEng *t_pLisp;
#endif


CLispEng& __stdcall GetLisp() noexcept {
	return Lisp();
}


  
} // Lisp::

using namespace Lisp;

LISPHANDLE __stdcall LispCreate() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return CLisp::CreateObject();
}

void __stdcall LispClose(LISPHANDLE h) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	delete (CLispEng*)h;
}

HRESULT __stdcall LispLoad(LISPHANDLE h, const char *filename) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	FileStream stm(filename, FileMode::Open, FileAccess::Read);
	try {
		((CLispEng*)h)->Load(stm);
		return 0;
	} catch (RCExc ex) {
		return HResultInCatch(ex);
	}
}

void __stdcall LispBreak(LISPHANDLE h) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CLispEng& lisp = *(CLispEng*)h;
	lisp.Signal = SIGINT;
}

LISPHANDLE __stdcall LispGetCurrent() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return &Lisp();
}

#ifdef WIN32
BSTR __stdcall LispEval(LISPHANDLE h, const char *s) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CLispEng& lisp = *(CLispEng*)h;
	return lisp.Eval(s).AllocSysString();
}
#endif


using namespace Lisp;

#if UCFG_OLE

class LispObj : public IDispatchImpl<LispObj, ILisp> {
	CLispEng m_lisp;

	HRESULT __stdcall Init()
	METHOD_BEGIN {
		m_lisp.Init(false);
	} METHOD_END

	HRESULT __stdcall Loop()
	METHOD_BEGIN {
		m_lisp.Loop();
	} METHOD_END

	HRESULT __stdcall SetStandardStream(int idx, IStandardStream *stm)
	METHOD_BEGIN {
		m_lisp.put_Stream((CEnumStream)idx, stm);
	} METHOD_END

	HRESULT __stdcall GetSymbol(BSTR name, BSTR pack, void **sym)
	METHOD_BEGIN {
		*sym = (void*)(uintptr_t)m_lisp.VGetSymbol(name, pack);
	} METHOD_END

	HRESULT __stdcall Call(void *sym, SAFEARRAY *psa, VARIANT *r)
	METHOD_BEGIN {
		vector<COleVariant> params;
		CSafeArray sa(psa);
		for (int i=0; i<=sa.GetUBound(); i++)
			params.push_back(sa[i]);
		*r = m_lisp.VCall((CP)(uintptr_t)sym, params).Detach();
	} METHOD_END

	HRESULT __stdcall Break(EnumSignal sig)
	METHOD_BEGIN {
		m_lisp.Signal = sig;
	} METHOD_END

	HRESULT __stdcall Eval(BSTR s, BSTR *r)
	METHOD_BEGIN {
		*r = m_lisp.Eval(s).AllocSysString();
	} METHOD_END
};

ILisp * __stdcall LispCreateObj() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CComPtr<ILisp> iLisp = new LispObj;
	return iLisp.Detach();
}
#endif  // UCFG_OLE

static class CLispObj {
public:
	CLispObj() {
		CMessageProcessor::RegisterModule((DWORD)Lisp::E_LISP_BASE, (DWORD)Lisp::E_LISP_UPPER, "lispeng");
	}
} g_lispObj;
