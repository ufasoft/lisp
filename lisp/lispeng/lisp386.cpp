/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

__int64 CDeclTrampoline(void *pArgs, size_t stacksize, PFNCDecl pfn) {
	__asm
	{
		sub ESP, stacksize;
		mov EDI, ESP
		mov ESI, pArgs
		mov ECX, stacksize
		rep movsb
		call pfn
		add	ESP, stacksize
	}
}

__int64 StdCallTrampoline(void *pArgs, size_t stacksize, PFNStdcall pfn) {
	__asm
	{
		sub ESP, stacksize;
		mov EDI, ESP
		mov ESI, pArgs
		mov ECX, stacksize
		rep movsb
		call pfn
	}
}



} // Lisp::


