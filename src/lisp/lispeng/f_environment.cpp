#include <el/ext.h>

#include "lispeng.h"

#if UCFG_WCE
#	include <windows/ce/pwinreg.h>
#endif

namespace Lisp {

DateTime CLispEng::s_dtSince1900(1900, 1, 1);

void CLispEng::F_GetInternalRealTime() {
	m_r = TimeSpanToInternalTimeUnits(DateTime::UtcNow()-m_dtStart);
}

void CLispEng::F_GetInternalRunTime() {
	m_r = TimeSpanToInternalTimeUnits(Thread::CurrentThread->get_TotalProcessorTime() - m_spanStartUser);
}

uint64_t CLispEng::GetFreedBytes() {
	uint64_t r = 0;
	for (size_t i=0; i<m_arValueMan.size(); ++i)
		r += m_arValueMan[i]->m_cbFreed;
	return r;
}

void CLispEng::F_PPTime() {
	Push(V_0);
	F_GetInternalRealTime();
	Push(m_r);

	Push(V_0);
	F_GetInternalRunTime();
	Push(m_r);

	Push(V_0);
	Push(TimeSpanToInternalTimeUnits(m_spanGC));

	Push(V_0);
	Push(CreateInteger64(GetFreedBytes()));
	
	Push(CreateInteger(m_nGC));

	StackToMv(9);
}

void CLispEng::F_Room() {
	CP x = ToOptional(Pop(), S(L_K_DEFAULT));

	uint64_t cbUsed = 0, cbFreed = GetFreedBytes();
	for (size_t i=0; i<m_arValueMan.size(); ++i) {
		CValueManBase *man = m_arValueMan[i];
		byte *p = (byte*)man->m_pBase;
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		for (byte *pHeap=(byte*)man->m_pHeap; ;) {
			byte *next = pHeap ? pHeap : (byte*)man->m_pBase+man->m_size*man->m_sizeof;
		
			for (; p<next; p+=man->m_sizeof)
				cbUsed += man->m_sizeof;
			p += man->m_sizeof;
			if (!pHeap)
				break;
			pHeap = ((byte**)next)[1];
		}
#else
		for (int j=0; j<man->m_size; ++j) {
			if (p[0] != 0xFF)
				cbUsed += man->m_sizeof;
			p += man->m_sizeof;
		}
#endif
	}	

	Push(CreateInteger(cbUsed));
	Push(V_0);
	Push(V_0);
	Push(CreateInteger(m_nGC));
	Push(CreateInteger64(cbFreed));
	Push(TimeSpanToInternalTimeUnits(m_spanGC));

	Push(V_T, CreateString(AfxLoadString(IDS_ROOM)));
	Push(SV4, SV3, SV2, V_0, SV7, V_0);
	Funcall(S(L_FORMAT), 8);


	for (int i=6; i--;)
		m_arVal[i] = Pop();
	m_cVal = 6;
}

CP CLispEng::ToUniversalTime(const DateTime& utc) {
	return CreateInteger64(duration_cast<seconds>(utc-s_dtSince1900).count());
}

void CLispEng::F_GetUniversalTime() {
	m_r = ToUniversalTime(DateTime::UtcNow());
}

void CLispEng::F_DefaultTimeZone() {
	Push(CreateInteger(-duration_cast<seconds>(TimeZoneInfo::Local().BaseUtcOffset).count()), CreateFixnum(3600));
	F_Divide(1);
}

void CLispEng::F_DecodeUniversalTime() {
	CP pTz = SV;
	CP tim = SV1;
	BigInteger utc = ToBigInteger(tim);
	int64_t sec;
	LocalDateTime dt;
	bool bDaylight = false;
	if (pTz) {
		Push(pTz, CreateFixnum(3600));
		F_Multiply(2);
		BigInteger tz = ToBigInteger(m_r);
		BigInteger tint = utc-tz;
		if (!tint.AsInt64(sec))
			E_TypeErr(tim, S(L_INTEGER)); //!!!
		dt = LocalDateTime(s_dtSince1900+TimeSpan::FromSeconds(double(sec)));
		m_arVal[8] = pTz;
	} else {
		if (!utc.AsInt64(sec))
			E_TypeErr(tim, S(L_INTEGER)); //!!!
		DateTime dtUtc = s_dtSince1900+TimeSpan::FromSeconds(double(sec));
		dt = dtUtc.ToLocalTime();
		F_DefaultTimeZone();
		m_arVal[8] = m_r;
		bDaylight = dtUtc + TimeZoneInfo::Local().BaseUtcOffset != dt;		
	}

	m_r = CreateFixnum(dt.Second);
	m_arVal[1] = CreateFixnum(dt.Minute);
	m_arVal[2] = CreateFixnum(dt.Hour);
	m_arVal[3] = CreateFixnum(dt.Day);
	m_arVal[4] = CreateFixnum(dt.Month);
	m_arVal[5] = CreateFixnum(dt.Year);
	m_arVal[6] = CreateFixnum(((int)(DayOfWeek)dt.DayOfWeek+6)%7);
	m_arVal[7] = FromBool(bDaylight);
	m_cVal = 9;
	SkipStack(2);
}

void CLispEng::F_EncodeUniversalTime() {
	int y = (uint16_t)AsNumber(SV1),
		month = (uint16_t)AsNumber(SV2),
		d = (uint16_t)AsNumber(SV3),
		h = (uint16_t)AsNumber(SV4),
		minute = (uint16_t)AsNumber(SV5),
		sec = (uint16_t)AsNumber(SV6);
	LocalDateTime dt = LocalDateTime(DateTime(y, month, d, h, minute, sec));
	int64_t utc;

	CP pTz = SV;
	if (pTz) {
		Push(pTz, CreateFixnum(3600));
		F_Multiply(2);
		int tz = AsNumber(m_r);
		utc = duration_cast<seconds>(dt-s_dtSince1900).count() + tz;
	} else {
		utc = duration_cast<seconds>(dt.ToUniversalTime()-s_dtSince1900).count();
	}
	m_r = CreateInteger64(utc);
	SkipStack(7);
}

void CLispEng::F_Sleep() {
	Push(SV, V_U);
	F_Float();
	double secs = AsFloatVal(m_r);
	if (secs < 0)
		E_TypeErr(SV, List(S(L_REAL), V_0));
	SkipStack(1);
	Thread::Sleep(int(secs*1000));	
	m_r = 0;
}

void CLispEng::F_ED() {
	CP p = ToOptionalNIL(SV);
	String name;
	switch (Type(p))
	{
	case TS_ARRAY:
	case TS_PATHNAME:
		name = AsString(p);
		break;
	case TS_CONS:
		if (!p)
			break;
	default:
		E_TypeErr(p, List(S(L_OR), S(L_STRING), S(L_PATHNAME), S(L_NULL)));
	}
	SkipStack(1);
	m_pSink->OnED(name);
}

void CLispEng::F_UserHomedirPathname() {
	CP host = ToOptionalNIL(Pop());
	if (!host || host == S(L_K_UNSPECIFIC)) {
		String path;
#if UCFG_WCE
		WCHAR buf[_MAX_PATH];
		DWORD dwSize = size(buf);
		Win32Check(::GetUserDirectory(buf, &dwSize));
		path = buf;
#else
		path = Environment::GetEnvironmentVariable("HOME");
		if (path == nullptr) {
#	ifdef WIN32
			path = Environment::GetEnvironmentVariable("HOMEDRIVE")+Environment::GetEnvironmentVariable("HOMEPATH");
#	endif
		}
#endif
		m_r = CreatePathname(AddDirSeparator(path));
	}
}

void CLispEng::F_Getenv() {
#if UCFG_WCE
	SkipStack(1);
	m_r = 0;
#else
	if (CP p = ToOptionalNIL(Pop())) {
		String name = AsTrueString(p),
			val = Environment::GetEnvironmentVariable(name);
		m_r = val != nullptr ? CreateString(val) : 0;
	} else {
		map<String, String> m = Environment::GetEnvironmentVariables();
		CListConstructor lc;
		for (map<String, String>::iterator i=m.begin(); i!=m.end(); ++i) {
			Push(CreateString(i->first));
			CP p = Cons(SV, CreateString(i->second));
			SkipStack(1);
			lc.Add(p);
		}
		m_r = lc;
	}
#endif
}

void CLispEng::F_MachineInstance() {
	m_r = CreateString(System.ComputerName);
}

void CLispEng::F_MachineType() {
	m_r = CreateString(Environment::GetMachineType());
}

void CLispEng::F_MachineVersion() {
	m_r = CreateString(Environment::GetMachineVersion());
}

void CLispEng::F_LispImplementationVersion() {
#ifdef WIN32
	try {
#if UCFG_EXTENDED
		m_r = CreateString(FileVersionInfo().GetProductVersionN().ToString(2));
#endif
	} catch (RCExc) {
		E_Error();
	}
#else
	m_r = CreateString(VER_PRODUCTVERSION_STR);
#endif
}

void CLispEng::F_Version() {
	if (m_bVerifyVersion) {
		F_LispImplementationVersion();
		if (SV != V_U) {
			if (!Equal(m_r, SV))
				E_Error();
		}
	}
	SkipStack(1);
}

void CLispEng::F_SoftwareType() {
	m_r = CreateString(Environment.OSVersion.PlatformName);
}

void CLispEng::F_SoftwareVersion() {
	m_r = CreateString(String(Environment.OSVersion.PlatformName)+" "+Environment.OSVersion.VersionName);
}

void CLispEng::F_Shell() {
#if UCFG_WCE
	Throw(E_NOTIMPL);
#else
#	if UCFG_USE_POSIX
	String cmd = "/bin/sh";
#	else
	String cmd = "cmd.exe";
#	endif
	CP p = Pop();
	if (p != V_U)
		cmd = AsTrueString(p);	
	switch (int r = ::system(cmd))
	{
	case 0:
		m_r = 0;
		break;
	case -1:
		m_r = CreateInteger(-errno);
		break;
	default:
		m_r = CreateInteger(r);
	}		
#endif
}


} // Lisp::

