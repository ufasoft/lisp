/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

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

