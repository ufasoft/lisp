#include <el/ext.h>

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#include "lispeng.h"

#include "lispeng_i.c"


static AFX_EXTENSION_MODULE LispEngDLL = { NULL, NULL };

/*!!!
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("LISPENG.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(LispEngDLL, hInstance);

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(LispEngDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("LISPENG.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(LispEngDLL);
	}
	return 1;   // ok
}*/


namespace Lisp {

class CLispEngApp : public CWinApp {
public:
};

#ifdef _AFXDLL
	CLispEngApp theApp;
#endif


} // Lisp::

