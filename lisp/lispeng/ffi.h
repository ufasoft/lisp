/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#pragma once


#ifndef  _WIN64
	__int64 StdCallTrampoline(void *pArgs, size_t stacksize, PFNStdcall pfn);
	__int64 CDeclTrampoline(void *pArgs, size_t stacksize, PFNCDecl pfn);
#endif


class ForeignArg : public Object {
};

class StringForeignArg : public ForeignArg {
public:
	String Value;

	StringForeignArg(RCString v)
		:	Value(v)
	{}
};

class CForeignPointer : public CSValueEx {
public:
	void *m_ptr;

	CForeignPointer(void *ptr)
		:	m_ptr(ptr)
	{
		m_type = TS_EX_FF_PTR;
	}

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};

class CForeignLibrary {
public:
	CDynamicLibrary m_dll;
	String m_path;

	typedef unordered_map<CResID, FARPROC> CFunMap;
	CFunMap m_funMmap;

	CForeignLibrary(RCString path)
		:	m_dll(path)
		,	m_path(path)
	{}

	FARPROC GetPointer(const CResID& resId);
};


