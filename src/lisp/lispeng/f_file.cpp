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

void CLispEng::F_FileWriteDate() {
	String path = FromPathnameDesignator(SV);
	if (FileInfo fi=path)
		m_r = ToUniversalTime(fi.get_LastWriteTime());
	else
		E_FileErr(SV);
	SkipStack(1);
}

void CLispEng::F_Directory() {
	String filespec = FromPathnameDesignator(Pop()),
		dir = Path::GetDirectoryName(filespec);
	vector<String> files;
	try {
		DBG_LOCAL_IGNORE_NAME(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), ignERROR_PATH_NOT_FOUND);

		files = Directory::GetFiles(dir, Path::GetFileName(filespec));
	} catch (RCExc) {
		m_r = 0;
		return;
	}
	for (int i=0; i<files.size(); ++i)
		Push(CreatePathname(files[i]));
	m_r = Listof(files.size());
}

void CLispEng::F_ProbeFile() {
	String path = FromPathnameDesignator(Pop());
	if (File::Exists(path)) {
		Push(CreateString(path));
		F_Pathname();
	}
}

void CLispEng::F_FileAuthor() {
	String path = FromPathnameDesignator(SV);
	if (!File::Exists(path))
		E_FileErr(SV);
	SkipStack(1);
}

void CLispEng::F_DeleteFile() {
	try {
		File::Delete(FromPathnameDesignator(SV));
		m_r = V_T;
	} catch (RCExc) {
		E_FileErr(SV);
	}
	SkipStack(1);
}

String CLispEng::GetDirectoryName(CP pathname) {
	Call(S(L_MERGE_PATHNAMES), pathname);
	CPathname *pn = ToPathname(m_r);
	pn->m_name = 0;
	pn->m_type = 0;
	pn->m_ver = 0;
	Call(S(L_NAMESTRING), m_r);
	return AsTrueString(m_r);
}

void CLispEng::F_EnsureDirectoriesExist() {
	bool bVerbose = ToOptionalNIL(Pop());

	String s = GetDirectoryName(SV);

	if (Directory::Exists(s))
		m_arVal[1] = 0;
	else {
		if (bVerbose)
			Call(S(L_FORMAT), V_T, CreateString("Creating directory: \n"+s));
		try {
			Directory::CreateDirectory(s);
		} catch (RCExc) {
			E_FileErr(SV);
		}
		m_arVal[1] = V_T;
	}

	m_cVal = 2;
	m_r = Pop();
}

void CLispEng::F_DeleteDirectory() {
	try {
		Directory::Delete(GetDirectoryName(SV));
	} catch (RCExc) {
		E_FileErr(SV);
	}
	m_r = V_T;
	SkipStack(1);
}


} // Lisp::

