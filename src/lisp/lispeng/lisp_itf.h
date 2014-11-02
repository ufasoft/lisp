/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#pragma once

typedef void *LISPHANDLE;

__BEGIN_DECLS

	LISPHANDLE __stdcall LispCreate();
	void __stdcall LispClose(LISPHANDLE);
	HRESULT __stdcall LispLoad(LISPHANDLE h, const WCHAR *filename);
	void __stdcall LispBreak(LISPHANDLE);
	LISPHANDLE __stdcall LispGetCurrent();
	BSTR __stdcall LispEval(LISPHANDLE, const WCHAR *s);
	void __stdcall LispSetStandardStream(LISPHANDLE, int idx, void *stdStream);

__END_DECLS


