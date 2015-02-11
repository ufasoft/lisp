#include <el/ext.h>

#include "../lispeng/lisp.h"
using namespace Lisp;

CLisp *g_lisp;

class CLispApp : public CConApp {
public: 	

	void Usage() {
		cerr << "\n"
			"Usage: lisp [options] [lispfile {argument}]\n"
			"  -b Rebuild init sources\n"
			"  -c <file>\tCompile <file>\n"
			"  -C Compile when loading\n"
			"  -D <feature>\tDefine <feature>\n"
			"  -h -?  print this help message\n"	
			"  -i <dir>\t: seek init sources in <dir>\n"
			"  --lp <dir>\tAdd <dir> to LOAD-PATHS\n"
			"  -M <file.bls>\tLoad image <file.bls>\n"
			"  --nologo\tSuppress startup banner\n"
			"  --norc\tDon't load ~/.lisprc.lisp\n"
			"  -O <destfile.exe>\tCompile to EXE\n"
			"  -U <feature>\tUndefine <feature>\n"
			"  -v verbose\n"
			"  --version\tPrint version information\n"
			"  -x \"<form>\"\tEval <form>\n"
			<< endl;	
	}

	void ParseParam(RCString pszParam, bool bFlag, bool bLast) {
		if (!bFlag || (pszParam=="nologo" || pszParam=="c"))
			m_bPrintLogo = false;
	}

	void Execute() {
//!!!		DBG_IGNORE(E_LISP_Exit); //!!!

#ifdef	_DEBUG
		CTrace::s_nLevel = 0xFF;
#endif

		unique_ptr<CLisp> pLisp(CLisp::CreateObject());
		CLisp& lisp = *pLisp;
		g_lisp = &lisp;
		
		g_lisp->ProcessCommandLine(Argc, Argv);
		lisp.ShowInfo();
		Environment::ExitCode = pLisp->m_exitCode;
		g_lisp = 0; // !!!must be in FINALLY	
	}

	bool OnSignal(int sig) {
		CConApp::OnSignal(sig);
		switch (sig)
		{
#ifdef WIN32
		case SIGBREAK: return false;
#endif
		}
		if (g_lisp)
			g_lisp->Signal = sig;
		return true;
	}

} theApp;

EXT_DEFINE_MAIN(theApp)





