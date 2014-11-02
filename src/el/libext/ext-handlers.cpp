#define UCFG_DEFINE_OLD_NAMES 1

#include <el/ext.h>

#if UCFG_WIN32 && !UCFG_MINISTL
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

//#include <el/libext/ext-os-api.h>

#ifdef _WIN32

#undef _open

#if !UCFG_WDM

#	if (!defined(_INC_IO) || !UCFG_STDSTL) && UCFG_CRT!='U'
	extern "C" __declspec(dllimport) int __cdecl _open(const char *fn, int flags, ...);
#	endif

#endif // !UCFG_WDM

#endif

#if UCFG_ALLOCATOR=='T'

#	ifndef _M_ARM
#		define set_new_handler my_set_new_handler
#	endif

#	define close API_close
#	define open _open
#	define read _read
#	define write _write


#	pragma warning(disable: 4146 4244 4291 4295 4310 4130 4232 4242 4018 4057 4090 4101 4152 4245 4267 4459 4505 4668 4700 4701 4716)
#	include "../el-std/tc_malloc.cpp"

void NTAPI TCMalloc_on_tls_callback(PVOID dllHandle, DWORD reason, PVOID reserved) {
	on_tls_callback((HINSTANCE)dllHandle, reason, reserved);
}


#pragma section(".CRT$XLC",long,read)
 __declspec(allocate(".CRT$XLC")) PIMAGE_TLS_CALLBACK _xl_y  = TCMalloc_on_tls_callback;


/*!!!
extern "C" {
	void* __cdecl tc_malloc(size_t size);
	void __cdecl tc_free(void *p);
	void* __cdecl tc_realloc(void* old_ptr, size_t new_size);
	size_t __cdecl tc_malloc_size(void* p);
}*/
#endif	// UCFG_ALLOCATOR=='T'


namespace Ext {
using namespace std;

#if UCFG_INDIRECT_MALLOC
void * (AFXAPI *CAlloc::s_pfnMalloc			)(size_t size)					= &CAlloc::Malloc;
void   (AFXAPI *CAlloc::s_pfnFree			)(void *p)						= &CAlloc::Free;
size_t (AFXAPI *CAlloc::s_pfnMSize			)(void *p)						= &CAlloc::MSize;
void * (AFXAPI *CAlloc::s_pfnRealloc		)(void *p, size_t size) 		= &CAlloc::Realloc;
void * (AFXAPI *CAlloc::s_pfnAlignedMalloc	)(size_t size, size_t align) 	= &CAlloc::AlignedMalloc;
void (AFXAPI *CAlloc::s_pfnAlignedFree	)(void *p)						= &CAlloc::Free;



#	if UCFG_WCE
static int s_bCeAlloc = SetCeAlloc();
#	endif

#endif

/*!!!
CAlloc::~CAlloc() {
}*/

void *CAlloc::Malloc(size_t size) {
#ifdef WDM_DRIVER
	if (void *p = ExAllocatePoolWithTag(NonPagedPool, size, UCFG_POOL_TAG))
		return p;
#elif UCFG_ALLOCATOR=='T'
#	if UCFG_HEAP_CHECK
	if (void *p = tc_malloc(size)) 
#	else
	if (void *p = do_malloc(size)) 
#	endif
		return p;
#else
	if (void *p = malloc(size))
		return p;
#endif
	Throw(E_OUTOFMEMORY);
}

void CAlloc::Free(void *p) EXT_NOEXCEPT {
#ifdef WDM_DRIVER
	if (!p)
		return;
	ExFreePoolWithTag(p, UCFG_POOL_TAG);
#elif UCFG_ALLOCATOR=='T'
#	if UCFG_HEAP_CHECK
	tc_free(p);
#	else
	do_free(p);
#	endif
#else
	free(p);
#endif
}

size_t CAlloc::MSize(void *p) {
#if UCFG_WIN32 && (UCFG_ALLOCATOR=='S')
	size_t r = _msize(p);
	CCheck(r != size_t(-1));
	return r;
#elif UCFG_ALLOCATOR=='T'
	return tc_malloc_size(p);
#else
	Throw (E_NOTIMPL);
#endif
}

void *CAlloc::Realloc(void *p, size_t size) {
#ifndef WDM_DRIVER
#if UCFG_ALLOCATOR=='T'
	if (void *r = tc_realloc(p, size))
		return r;
#else
	if (void *r = realloc(p, size))
		return r;
#endif
	Throw(E_OUTOFMEMORY);
#else
	Throw (E_NOTIMPL);
#endif
}

#if !UCFG_MINISTL
void *CAlloc::AlignedMalloc(size_t size, size_t align) {
#if UCFG_ALLOCATOR=='T'
	if (void *p = do_memalign(align, size)) 
		return p;
#elif UCFG_USE_POSIX
	void *r;
	CCheckErrcode(posix_memalign(&r, align, size));
	return r;
#elif UCFG_MSC_VERSION && !UCFG_WDM
	if (void *r = _aligned_malloc(size, align))
		return r;
	else
		CCheck(-1);
#else
	Throw(E_NOTIMPL);
#endif
	Throw(E_OUTOFMEMORY);
}

void CAlloc::AlignedFree(void *p) {
#if UCFG_ALLOCATOR=='T'
	do_free(p);
#elif UCFG_USE_POSIX
	free(p);
#elif UCFG_MSC_VERSION && !UCFG_WDM
	_aligned_free(p);
#else
	Throw(E_NOTIMPL);
#endif
}
#endif // !UCFG_MINISTL


} // Ext::


#if UCFG_WDM
//!!!POOLTAG DefaultPoolTag = UCFG_POOL_TAG;
#else

/*!!!
void * __cdecl operator new(size_t sz) {
	return Ext::Malloc(sz);
}

void __cdecl operator delete(void *p) {
	Ext::Free(p);
}

void __cdecl operator delete[](void *p) {
	Ext::Free(p);
}

void * __cdecl operator new[](size_t sz) {
	return Ext::Malloc(sz);
}
*/

#	if UCFG_STDSTL
void __cdecl operator delete[](void *p, const std::nothrow_t& ) {
	operator delete(p);//!!!
}	
#else
void __cdecl operator delete[](void *p, const ExtSTL::nothrow_t& ) {
	operator delete(p);//!!!
}	
#	endif // UCFG_STDSTL

#endif // UCFG_WDM

static void * (__cdecl *s_pfnNew)(size_t) = &operator new;
static void * (__cdecl *s_pfnNewArray)(size_t) = &operator new[];
static void (__cdecl *s_pfnDelete)(void*) = &operator delete;
static void (__cdecl *s_pfnDeleteArray)(void*) = &operator delete[];

#if defined(_MSC_VER) && !UCFG_STDSTL

extern "C" {

BOOL __cdecl __crtIsWin8orLater(void) {
	return false;
}

LONG __cdecl __crtUnhandledException
(
    _In_ EXCEPTION_POINTERS *exceptionInfo
) {
	Ext::ThrowImp(E_FAIL);
}

extern "C" void _cdecl API_terminate();

void __cdecl __crtTerminateProcess
(
    _In_ UINT uExitCode
) {
	API_terminate();
}

void __cdecl __crtCaptureCurrentContext
(
    _Out_ CONTEXT *pContextRecord
) {
	Ext::ThrowImp(E_FAIL);

}

void __cdecl __crtCapturePreviousContext
(
    _Out_ CONTEXT *pContextRecord
) {
	Ext::ThrowImp(E_FAIL);

}

#if UCFG_WDM
extern "C" void* __cdecl API_malloc(size_t size) {
	return CAlloc::Malloc(size);
}
#endif




} // "C"

#endif // !UCFG_STDSTL





