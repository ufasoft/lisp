#include <el/ext.h>

#if UCFG_WIN32
#	include <winsock2.h>		// for timeval
#endif

using namespace Ext;

extern "C" {

#if UCFG_USE_OLD_MSVCRTDLL && UCFG_PLATFORM_IX86
errno_t __cdecl API_gmtime64_s(struct tm *ptm, const __time64_t *timp) {
	if (!ptm || !timp)
		return EINVAL;
	DateTime dt = DateTime::from_time_t(*timp);
	SYSTEMTIME st = dt;
	ZeroStruct(*ptm);
	ptm->tm_year = st.wYear-1900;
	ptm->tm_mon = st.wMonth-1;
	ptm->tm_hour = st.wHour;
	ptm->tm_min = st.wMinute;
	ptm->tm_sec = st.wSecond;
	ptm->tm_wday = st.wDayOfWeek;
	ptm->tm_yday = dt.DayOfYear;
	return 0;
}

#endif

tm * __cdecl gmtime_r(const time_t *timer, tm *result) {
	__time64_t t64 = *timer;
	if (!_gmtime64_s(result, &t64))
		return result;
	return 0;
}

#if UCFG_WIN32_FULL

int __stdcall gettimeofday(struct timeval *tp, void *) {
	Ext::Int64 res = DateTime::UtcNow().Ticks-Unix_DateTime_Offset;	
	tp->tv_sec = (long)(res/10000000);	//!!! can be overflow
	tp->tv_usec = (long)(res % 10000000) / 10; // Micro Seconds
	return 0;

}

#endif // UCFG_WIN32_FULL

} // extern "C"
