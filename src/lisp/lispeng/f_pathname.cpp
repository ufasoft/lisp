/*######   Copyright (c) 2002-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::F_PathnameP() {
	m_r = FromBool(Type(Pop()) == TS_PATHNAME);
}


bool CLispEng::LogicalHostP(CP p) {
	return StringP(p) && GetHash(p, Spec(L_S_LOGICAL_PATHNAME_TRANSLATIONS));
}

CP CLispEng::ToOptionalHost(CP p) {
	if (p == V_U || !p || p==S(L_K_UNSPECIFIC))
		return 0;
	if (!StringP(p))
		E_TypeErr(p, List(S(L_OR), S(L_NULL), S(L_STRING)));
	return p;
}

CP CLispEng::ToDefaultPathname(CP p) {
	if (p == V_U)
		return Spec(L_S_DEFAULT_PATHNAME_DEFAULTS);
	Push(p);
	F_Pathname();
	return m_r;
}

static wregex s_reUrl(L"^(?:([A-Za-z0-9-]+)://([.A-Za-z0-9-]+(?::(\\d+))?))"			// proto - 1   host - 2   port - 3
	L"(/[/.A-Za-z0-9-]+)?"											// dir - 4
	L"([^/\\s]+)?");													// name - 5


static String sReNonUrl =
#if UCFG_USE_POSIX
"(?:([A-Za-z0-9-]+)(:))?"
"([^:]+/)?"
"(?:([^:/]+)\\.([^\\.:/]+)"
"|([^:/]+))?";
#else
"^(?:\\\\\\\\([^\\\\]+)\\\\)?"						// host - 1
"(?:([A-Za-z]):)?"									// dev - 2
"([^:;]*(?:/|\\\\))?"								// dir - 3	
"(?:([^:;/\\\\]+)\\.([^\\.:;/]+)"					// name - 4   type - 5
"|([^:;/\\\\]+))?";									// onlyname - 6
#endif

static wregex s_reNonUrl(sReNonUrl),
	s_reNonUrlTerminated(sReNonUrl + "$");

void CLispEng::F_ParseNamestring() {
	bool bJunkAllowed = ToOptionalNIL(Pop());
	SV1 = ToOptional(SV1, V_0);
	CP host = ToOptionalHost(SV3);
	bool bLogical = false;
	if (host) {
		if (bLogical = LogicalHostP(host)) {
			host = CreateString(AsString(host).ToUpper());
		}
	} else {
		if (CP defaults = ToDefaultPathname(SV2)) {
			bLogical = ToPathname(defaults)->LogicalP;
			host = ToPathname(defaults)->m_host;
		}
	}
	SV3 = host;
	CP thing = SV4;
	pair<size_t, size_t> pp;
	String s;
	size_t len, lenRecognized = 0;
	switch (Type(thing)) {
	case TS_PATHNAME:
		SV = thing;
		break;
	case TS_STREAM:
		if (AsStream(thing)->m_subtype != STS_FILE_STREAM)
			E_PathnameErr(thing);
		SV = AsStream(thing)->m_pathname;
		break;
	case TS_ARRAY:
		{
			if (!StringP(thing))
				E_PathnameErr(thing);
			s = AsString(thing);
			pp = PopSeqBoundingIndex(thing, len);
			Push(CreateFixnum(pp.first), CreateFixnum(pp.second));
			s = s.substr(pp.first, pp.second-pp.first);

			if (bLogical) {
				String sRe = "^(?:([A-Z0-9-]*):)?"  								// host - 1								allows ":" for CLISP
					"(;?(?:(?:\\*\\*|\\*([A-Z0-9-]\\*?)*|(?:[A-Z0-9-]\\*?)+);)*)"	// dir - 2 
					"(\\*(?:[A-Z0-9-]\\*?)*|(?:[A-Z0-9-]\\*?)+)?"					// name - 3
					"(?:\\.(\\*([A-Z0-9-]\\*?)*|(?:[A-Z0-9-]\\*?)+)"				// type - 4
					"(?:\\.(\\*|newest|NEWEST|\\d+))?)?";							// version - 5
				if (!bJunkAllowed)
					sRe += "$";
				wregex reLogical(sRe);
				Smatch m;
				if (regex_search(s, m, reLogical)) {
					String host = m[1],
						   dir = m[2],
						   name = m[3],
						   type = m[4],
						   version = m[5];
					Push(host.empty() ? 0 : CreateString(host));
					if (dir.empty())
						Push(0);
					else {
						vector<String> v = dir.Split(";");
						if (v.size() > 1)
							v.resize(v.size()-1);		//!!!?
						Push(v[0]=="" ? S(L_K_RELATIVE) : S(L_K_ABSOLUTE));
						for (size_t i=0; i<v.size(); ++i) {
							String c = v[i];
							if (c != "") {
								if (c == "**")
									Push(S(L_K_WILD_INFERIORS));
								else if (c == "*")
									Push(S(L_K_WILD));
								else
									Push(CreateString(c));
							}
						}
						Push(Listof(v.size()));
					}
					Push(name.empty() ? 0 : name=="*" ? S(L_K_WILD) : CreateString(name));
					Push(type.empty() ? 0 : type=="*" ? S(L_K_WILD) : CreateString(type));
					Push(version.empty() ? 0 : version=="*" ? S(L_K_WILD)
								 							  : version == "NEWEST" || version == "newest" ?  S(L_K_NEWEST)
															                                               : CreateInteger(atoi(version)));
					CPathname *pn = m_pathnameMan.CreateInstance();
					pn->LogicalP = true;
					pn->m_ver = Pop();
					pn->m_type = Pop();
					pn->m_name = Pop();
					pn->m_dir = Pop();
					pn->m_host = Pop();
					SV = FromSValue(pn);
					lenRecognized = m.length();
				} else
					E_ParseErr();
			} else {
				Smatch m;
               	if (regex_search(s, m, s_reUrl)) {
					String proto = m[1],
                           host = m[2],
						   port = m[3],
                           dir = m[4],
                           name = m[5];
                    if (!proto.empty()) {
					    Push(CreateString(proto));
					    Push(CreateString(host));
						Push(Listof(2));
					} else
						Push(0);
					Push (port.empty() ? 0 : CreateString(port));

					vector<String> v = dir.Split(";");
					Push(S(L_K_ABSOLUTE));
					int count = 1;
					for (size_t i=1; i<v.size(); ++i) {
						String s = v[i];
						if (!s.empty()) {
							++count;
							Push(CreateString(s));
						}
					}
					Push(Listof(count));
					CPathname *pn = m_pathnameMan.CreateInstance();
					pn->m_name = Pop();
					pn->m_dir = Pop();
					pn->m_dev = Pop();
					pn->m_host = Pop();
					SV = FromSValue(pn);
					lenRecognized = m.length();
				} else {
					Smatch m;
               		if (regex_search(s, m, bJunkAllowed ? s_reNonUrl : s_reNonUrlTerminated)) {
						String host = m[1],
	#if !UCFG_USE_POSIX
							   dev = m[2],

	#endif
							   dir = m[3],
							   name = m[4],
							   onlyname = m[6],
							   type = m[5];

						if (name.empty())
							name = onlyname;

					
						Push(host.empty() ? 0 : host=="?" || host=="."
	                                                   ? S(L_K_UNSPECIFIC)
	                                                   : CreateString(host));
	#if UCFG_USE_POSIX
						Push(S(L_K_UNSPECIFIC));
	#else
						Push(dev.empty() ? 0 : CreateString(dev));
	#endif

						if (dir.empty())
							Push(0);
						else {
							vector<String> v = dir.Split(String(path::preferred_separator) + String(Path::AltDirectorySeparatorChar));
							Push(v[0]!="" ? S(L_K_RELATIVE) : S(L_K_ABSOLUTE));
							int count = 1;
							for (size_t i=0; i<v.size(); ++i) {							
								String c = v[i];
								if (c != "") {
									if (c == "**")
										Push(S(L_K_WILD_INFERIORS));
									else if (c == "*")
										Push(S(L_K_WILD));
									else if (c == "..")
										Push(S(L_K_UP));
									else
										Push(CreateString(c));
									++count;
								}
							}
							Push(Listof(count));
						}
						Push(name.empty() ? 0 : name=="*" ? S(L_K_WILD) : CreateString(name));
						Push(type.empty() ? 0 : type=="*" ? S(L_K_WILD) : CreateString(type));

						CPathname *pn = m_pathnameMan.CreateInstance();
						pn->m_ver = S(L_K_UNSPECIFIC);
						pn->m_type = Pop();
						pn->m_name = Pop();
						pn->m_dir = Pop();
						pn->m_dev = Pop();
						pn->m_host = Pop();
						SV = FromSValue(pn);
						lenRecognized = m.length();
					} else
						E_ParseErr();
				}
			}
			if (!bJunkAllowed && pp.first+lenRecognized!=pp.second)
				E_ParseErr();
			if (host) {	// host 
				if (!ToPathname(SV)->m_host)
					ToPathname(SV)->m_host = host;
			}
			SV1 = CreateInteger(pp.first+lenRecognized);
			break;
		}
	default:
		E_PathnameErr(thing);
	}
	if (host && SV) {
		CP h = AsPathname(SV)->m_host;
		if (h && h!=S(L_K_UNSPECIFIC) && !Equal(host, h))
			E_TypeErr(h, List(S(L_EQUAL), host));		
	}
	m_r = SV;
	m_arVal[1] = SV1;
	m_cVal = 2;
	SkipStack(5);
}

CP CLispEng::CreatePathname(const path& p) {
	Push(CreateString(p.native()), V_U, V_U, V_U, V_U, V_U);
	F_ParseNamestring();
	m_cVal = 1;
	return m_r;	
}

CPathname *CLispEng::CopyPathname(CP p) {
	CPathname *r = m_pathnameMan.CreateInstance();
	*r = *ToPathname(p);
	return r;
}

void CLispEng::F_Pathname() {
	if (Type(SV) == TS_PATHNAME)
		m_r = Pop();
	else {
		Push(V_U, V_U, V_U, V_U, V_U);
		F_ParseNamestring();
		m_cVal = 1;
	}
}

void CLispEng::F_PathnameFieldEx() {
	CP pathname = Pop();
	int idx = AsNumber(Pop());
	if (pathname) {
		CPathname *pn = ToPathname(pathname);
		switch (idx) {
		case 0:
			m_r = pn->m_host;
			break;
		case 1:
			m_r = pn->m_dev==V_U ? CP(S(L_K_UNSPECIFIC)) : CP(pn->m_dev);
			break;
		case 2:
			m_r = pn->m_dir;
			break;
		case 3:
			m_r = pn->m_name;
			break;
		case 4:
			m_r = pn->m_type;
			break;
		case 5:
			m_r = pn->m_ver;
			break;
		default:
			E_Error();
		}
	}
}

void CLispEng::E_PathnameErr(CP datum) {
	E_TypeErr(datum, List(S(L_OR), S(L_STRING), S(L_PATHNAME), S(L_FILE_STREAM)));
}

CLispEng::EStringCase CLispEng::StringCase(RCString s) {
	int bHasLower = false;
	int bHasUpper = false;
	for (int i=0, len=s.length(); i<len; ++i) {
		wchar_t ch = s[i];
		bHasLower |= iswlower(ch);
		bHasUpper |= iswupper(ch);
	}
	if (bHasLower && bHasUpper)
		return CASE_ALL_MIXED;
	else
		return bHasLower ? CASE_ALL_LOWER : CASE_ALL_UPPER;
}

String CLispEng::CasedString(RCString s, CP cas) {
	if (cas == S(L_K_COMMON)) {
		EStringCase sc = StringCase(s);
		if (sc == CASE_ALL_MIXED)
			return s;
		return sc==CASE_ALL_LOWER ? s.ToUpper() : s.ToLower();
	}
	return s;
}

CP CLispEng::CasedPathnameComponent(CP c, CP cas, CP defaultComp) {
	if (StringP(c) && cas==S(L_K_COMMON))
		return CreateString(CasedString(AsString(c), cas));
	else
		return c!=V_U ? c : defaultComp;
}

void CLispEng::F_MakePathname() {
	bool bLogical = ToOptionalNIL(Pop());
	CP cas = ToOptional(Pop(), S(L_K_LOCAL)),
		defaults = ToOptional(Pop(), Spec(L_S_DEFAULT_PATHNAME_DEFAULTS));
	Push(defaults);
	F_Pathname();
	Push(CasedPathnameComponent(SV, S(L_K_LOCAL), AsPathname(m_r)->m_ver));

#ifdef X_DEBUG//!!!D
	static int s_i;
	if (++s_i == 7232) {
		s_i = s_i;
		String ttyp = AsString(SV2);
		E_Error();
		ttyp = ttyp;
	}
#endif

	Push(CasedPathnameComponent(SV2, cas, AsPathname(m_r)->m_type));
	Push(CasedPathnameComponent(SV4, cas, AsPathname(m_r)->m_name));

	CP dir = SV6;
	if (dir == S(L_K_WILD) || dir == S(L_K_WILD_INFERIORS))
		dir = List(S(L_K_ABSOLUTE), S(L_K_WILD_INFERIORS));
	else if (dir == V_U)
		dir = AsPathname(m_r)->m_dir;
	else if (StringP(dir))
		dir = List(S(L_K_ABSOLUTE), CasedPathnameComponent(dir, cas, AsPathname(m_r)->m_dir));
	else if (!dir)
		dir = 0;
	else if (ConsP(dir)) {
		CP car;
		SplitPair(dir, car);
		if (car!=S(L_K_ABSOLUTE) && car!=S(L_K_RELATIVE))
			E_Error();
		Push(car);
		int n = 1;
		while (SplitPair(dir, car)) {
			if (StringP(car)) {
				String s = AsTrueString(car);
				if (s == "" || s == ".")
					continue;
				else if (s == "..")
					car = S(L_K_UP);
				else if (s == "*")
					car = S(L_K_WILD);
				else if (s == "**")
					car = S(L_K_WILD_INFERIORS);
				else
					car = CasedPathnameComponent(car, cas, 0);
			}
			if (car==S(L_K_BACK) || car==S(L_K_UP)) {
				if (SV==S(L_K_ABSOLUTE) || SV==S(L_K_WILD_INFERIORS))
					E_FileErr(0); 									//!!! need args
				if (car==S(L_K_BACK) && n > 1) {		// don't collapse :UP because of symlinks
					SkipStack(1);
					--n;
					continue;
				}
			}
			Push(car);
			++n;
		}
		dir = Listof(n);
	} else
		E_TypeErr(dir, List(S(L_OR), S(L_LIST), S(L_STRING))); 	//!!! may be :WILDCARD yet
	Push(dir);
	
	Push(bLogical ? 0 : CasedPathnameComponent(SV8, cas, AsPathname(m_r)->m_dev));
	Push(CasedPathnameComponent(SV10, cas, AsPathname(m_r)->m_host));

	CPathname *pn = m_pathnameMan.CreateInstance();
	pn->m_host = Pop();
	pn->m_dev = Pop();
	pn->m_dir = Pop();
	pn->m_name = Pop();
	pn->m_type = Pop();
	pn->m_ver = Pop();
	pn->LogicalP = bLogical;
	m_r = FromSValue(pn);
	SkipStack(6);
#ifdef X_DEBUG//!!!D
	String ss = pn->ToString();
	cerr << ss << "====F_MakePathname===  " << ss << endl;
	if (strstr(ss, "macr")) {
		
		int i = s_i;
		if (pn->m_type) {
			String typ = AsString(pn->m_type);
			typ = typ;
		}
		
	}
#endif
}

void CLispEng::F_CD() {
	if (SV == V_U)
		SV = CreateString("");
	F_Pathname();
	Call(S(L_NAMESTRING), m_r);
	current_path(AsString(m_r).ToOsString());
	Push(CreateString(current_path().native()));
	F_Pathname();
	SetSpecial(S(L_S_DEFAULT_PATHNAME_DEFAULTS), m_r);
}

path CLispEng::FromPathnameDesignator(CP p) {
	Push(m_r);
	Call(S(L_MERGE_PATHNAMES), p);
	return ToPathname(exchange(m_r, Pop()))->ToString();
}

void CLispEng::F_Truename() {
	path truename;
	if (Type(SV) == TS_STREAM)
		truename = AsStream(SV)->TrueName;
	else {
		try {
			path spath = FromPathnameDesignator(SV);
			truename = Path::GetTruePath(spath);
			if (!spath.empty()) {
				wchar_t ch = spath.native()[spath.native().size()-1];
				if (ch==path::preferred_separator || ch==Path::AltDirectorySeparatorChar || is_directory(truename))
					truename = AddDirSeparator(truename);
			}
		} catch (RCExc ex) {
			E_FileErr(HResultInCatch(ex), ex.what(), SV);
		}
	}
	if (truename.empty())
		m_r = 0;
	else
		m_r = CreatePathname(truename);
	SkipStack(1);
}

void CLispEng::F_ProbePathname() {
	CP pn = SV;
	path p = FromPathnameDesignator(pn);

#ifdef X_DEBUG//!!!D
	cerr << "====F_ProbePathname===  " << path << endl;
#endif		

	if (is_regular_file(p)) {
		Push(CreateInteger64(file_size(p)));
		Push(ToUniversalTime(last_write_time(p)));

#ifdef X_DEBUG//!!!D
		String bt = ToPathname(pn)->ToString();
		cerr << "====F_ProbePathname: before True===  " << ToPathname(pn)->ToString() << endl;
#endif		

		Push(pn);
		F_Truename();
		Push(m_r);
#ifdef X_DEBUG//!!!D
		String cur = ToPathname(m_r)->ToString();
		if (cur.ToLower().Right(6) != bt.ToLower().Right(6)) {
			Push(pn);
			F_Truename();
		}
		cerr << "====F_ProbePathname: before copy===  " << ToPathname(m_r)->ToString() << endl;
#endif		
		CPathname *ppn = CopyPathname(m_r);
		Push(FromSValue(ppn));
#ifdef X_DEBUG//!!!D
		cerr << "====F_ProbePathname: after copy===  " << ppn->ToString() << endl;
#endif		
	} else if (is_directory(p)) {
		Push(0);
		Push(ToUniversalTime(last_write_time(p)));

		Push(pn);
		F_Truename();
		Push(m_r);
		CPathname *ppn = CopyPathname(m_r);
		ppn->m_name = 0;
		ppn->m_type = 0;
		Push(FromSValue(ppn));
	} else {
		m_r = 0;
		SkipStack(1);
		return;
	}
	m_arVal[1] = Pop();
	m_r = Pop();
	m_arVal[2] = Pop();
	m_arVal[3] = Pop();
	m_cVal = 4;	
	SkipStack(1);
}

path CPathname::ToString(bool bWithoutNameExt) {
	CLispEng& lisp = Lisp();
	ostringstream os;
	if (m_dev && m_dev!=S(L_K_UNSPECIFIC))
		os << AsString(m_dev) << ":";
	if (m_dir && m_dir!=S(L_K_UNSPECIFIC)) {
		CP dir = m_dir, p;
		if (lisp.SplitPair(dir, p)) {
			if (p == S(L_K_ABSOLUTE))
				os << String(path::preferred_separator);
			while (lisp.SplitPair(dir, p)) {
				String s;
				if (p == S(L_K_WILD))
					s = "*";
				else if (p == S(L_K_WILD_INFERIORS))
					s = "**";
				else if (p==S(L_K_UP) || p==S(L_K_BACK))
					s = "..";
				else
					s = AsString(p);
				os << s << String(path::preferred_separator);
			}
		}
	}
	if (!bWithoutNameExt) {
		if (m_name && m_name!=S(L_K_UNSPECIFIC)) {
			if (m_name == S(L_K_WILD))
				os << "*";
			else
				os << AsString(m_name);
		}
		if (m_type && m_type!=S(L_K_UNSPECIFIC)) {
			os << ".";
			if (m_type == S(L_K_WILD))
				os << "*";
			else
				os << AsString(m_type);
		}
	}
	String r = os.str();
	return r.ToOsString();
}





} // Lisp::


