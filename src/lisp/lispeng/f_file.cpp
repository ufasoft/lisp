/*######   Copyright (c) 1999-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_FileWriteDate() {
	path p = FromPathnameDesignator(SV);
	if (exists(p))
		m_r = ToUniversalTime(last_write_time(p));
	else
		E_FileErr(SV);
	SkipStack(1);
}

void CLispEng::F_Directory() {
	path filespec = FromPathnameDesignator(Pop()),
		dir = filespec.parent_path();					//!!!TODO process wildcards
	vector<path> files;
	error_code ec;
	for (directory_iterator it(dir, ec), e; it!=e; it.increment(ec))
		files.push_back(it->path());
	if (ec)
		return void(m_r = 0);
	for (size_t i=0; i<files.size(); ++i)
		Push(CreatePathname(files[i]));
	m_r = Listof(files.size());
}

void CLispEng::F_ProbeFile() {
	path p = FromPathnameDesignator(Pop());
	if (exists(p)) {
		Push(CreateString(p.native()));
		F_Pathname();
	}
}

void CLispEng::F_FileAuthor() {
	path p = FromPathnameDesignator(SV);
	if (!exists(p))
		E_FileErr(SV);
	SkipStack(1);
}

void CLispEng::F_DeleteFile() {
	error_code ec;
	sys::remove(FromPathnameDesignator(SV), ec);
	if (ec)
		E_FileErr(SV);
	m_r = V_T;
	SkipStack(1);
}

path CLispEng::GetDirectoryName(CP pathname) {
	Call(S(L_MERGE_PATHNAMES), pathname);
	CPathname *pn = ToPathname(m_r);
	pn->m_name = 0;
	pn->m_type = 0;
	pn->m_ver = 0;
	Call(S(L_NAMESTRING), m_r);
	return AsTrueString(m_r).ToOsString();
}

void CLispEng::F_EnsureDirectoriesExist() {
	bool bVerbose = ToOptionalNIL(Pop());

	path s = GetDirectoryName(SV);

	if (is_directory(s))
		m_arVal[1] = 0;
	else {
		if (bVerbose)
			Call(S(L_FORMAT), V_T, CreateString("Creating directory: \n" + s.native()));
		error_code ec;
		create_directories(s, ec);
		if (ec)
			E_FileErr(SV);
		m_arVal[1] = V_T;
	}

	m_cVal = 2;
	m_r = Pop();
}

void CLispEng::F_DeleteDirectory() {
	error_code ec;
	sys::remove(GetDirectoryName(SV), ec);
	if (ec)
		E_FileErr(SV);
	m_r = V_T;
	SkipStack(1);
}


} // Lisp::

