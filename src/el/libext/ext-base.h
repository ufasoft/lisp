#pragma once

#include EXT_HEADER(map)

namespace Ext {

class CCriticalSection;
class Exception;

typedef const std::exception& RCExc;

class CPrintable {
public:
	virtual ~CPrintable() {}
	virtual String ToString() const =0;
};


#if !UCFG_WCE
class CStackTrace : public CPrintable {
public:
	EXT_DATA static bool Use;

	int AddrSize;
	std::vector<UInt64> Frames;

	CStackTrace()
		:	AddrSize(sizeof(void*))
	{}

	static CStackTrace AFXAPI FromCurrentThread();
	String ToString() const;
};
#endif

const std::error_category& hresult_category();

class Exception : public std::system_error, public CPrintable {
	typedef std::system_error base;
public:
	typedef Exception class_type;

#if !UCFG_WDM
	static thread_specific_ptr<String> t_LastStringArg;
#endif

	HRESULT HResult;
	mutable String m_message;

	typedef std::map<String, String> CDataMap;
	CDataMap Data;

	explicit Exception(HRESULT hr = 0, RCString message = "");
	~Exception() noexcept {}		//!!! necessary for GCC 4.6
	String ToString() const;

	virtual String get_Message() const;
	DEFPROP_VIRTUAL_GET_CONST(String, Message);

	const char *what() const noexcept override;
private:

#ifdef _WIN64
	LONG dummy;
#endif

public:
#if !UCFG_WCE
	CStackTrace StackTrace;
#endif	
};

class ExcLastStringArgKeeper {
public:
	String m_prev;

	ExcLastStringArgKeeper(RCString s) {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		m_prev = exchange(*ps, s);
#endif
	}

	~ExcLastStringArgKeeper() {
#if !UCFG_WDM
		*Exception::t_LastStringArg = m_prev;
#endif
	}

	operator const String&() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return *ps;
#endif
	}

	operator const char *() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return ps->c_str();
#endif
	}

	operator const String::Char *() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return *ps;
#endif
	}
};

#define EXT_DEFINE_EXC(c, b, code) class c : public b { public: c(HRESULT hr = code) : b(hr) {} };

EXT_DEFINE_EXC(ArithmeticExc, Exception, E_EXT_Arithmetic)
EXT_DEFINE_EXC(OverflowExc, ArithmeticExc, E_EXT_Overflow)
EXT_DEFINE_EXC(ArgumentExc, Exception, E_INVALIDARG)
EXT_DEFINE_EXC(EndOfStreamException, Exception, E_EXT_EndOfStream)
EXT_DEFINE_EXC(FileFormatException, Exception, E_EXT_FileFormat)
EXT_DEFINE_EXC(NotImplementedExc, Exception, E_NOTIMPL)
EXT_DEFINE_EXC(UnspecifiedException, Exception, E_FAIL)
EXT_DEFINE_EXC(AccessDeniedException, Exception, E_ACCESSDENIED)


class thread_interrupted : public Exception {
public:
	thread_interrupted()
		:	Exception(E_EXT_ThreadInterrupted)
	{}
};

class SystemException : public Exception {
public:
	SystemException(HRESULT hr = 0, RCString message = "")
		:	Exception(hr, message)
	{
	}
};


class CryptoException : public Exception {
	typedef Exception base;
public:
	explicit CryptoException(HRESULT hr, RCString message)
		:	base(hr, message)
	{}
};

class StackOverflowExc : public Exception {
	typedef Exception base;
public:
	StackOverflowExc()
		:	base(HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW))
	{}

	CInt<void *> StackAddress;
};

class ProcNotFoundExc : public Exception {
	typedef Exception base;
public:
	String ProcName;

	ProcNotFoundExc()
		:	base(HRESULT_OF_WIN32(ERROR_PROC_NOT_FOUND))
	{
	}

	~ProcNotFoundExc() noexcept {}

	String get_Message() const override {
		String r = base::get_Message();
		if (!ProcName.IsEmpty())
			r += " "+ProcName;
		return r;
	}
};

INT_PTR WINAPI AfxApiNotFound();
#define DEF_DELAYLOAD_THROW static FARPROC WINAPI DliFailedHook(unsigned dliNotify, PDelayLoadInfo  pdli) { return &AfxApiNotFound; } static struct InitDliFailureHook {  InitDliFailureHook() { __pfnDliFailureHook2 = &DliFailedHook; } } s_initDliFailedHook;

extern "C" AFX_API void _cdecl AfxTestEHsStub(void *prevFrame);

//!!! DBG_LOCAL_IGNORE_NAME(1, ignOne);	

#define DEF_TEST_EHS							\
	static DECLSPEC_NOINLINE int AfxTestEHs(void *p) {			\
		try {								\
			AfxTestEHsStub(&p);					\
		} catch (...) {							\
		}										\
		return 0;								\
	}											\
	static int s_testEHs  = AfxTestEHs(&AfxTestEHs);									\


class AssertFailedExc : public Exception {
	typedef Exception base;
public:
	String FileName;
	int LineNumber;

	AssertFailedExc(RCString exp, RCString fileName, int line)
		:	base(HRESULT_FROM_WIN32(ERROR_ASSERTION_FAILURE), "Assertion Failed: "+exp)
		,	FileName(fileName)
		,	LineNumber(line)
	{
	}

	~AssertFailedExc() noexcept {}
};




template <typename H>
struct HandleTraitsBase {
	typedef H handle_type;

	static bool ResourceValid(const handle_type& h) {
		return h != 0;
	}

	static handle_type ResourseNull() {
		return 0;
	}
};

template <typename H>
struct HandleTraits : HandleTraitsBase<H> {
};

/*!!!
template <typename H>
inline void ResourceRelease(const H& h) {
	Throw(E_NOTIMPL);
}*/

template <typename H>
class ResourceWrapper {
public:
	typedef HandleTraits<H> traits_type;
	typedef typename HandleTraits<H>::handle_type handle_type;

	handle_type m_h;		//!!! should be private
    
	ResourceWrapper()
		:	m_h(traits_type::ResourseNull())
	{}

	ResourceWrapper(const ResourceWrapper& res)
		:	m_h(traits_type::ResourseNull())
	{
		operator=(res);
	}

	~ResourceWrapper() {
		if (Valid()) {
			if (!std::uncaught_exception())
				traits_type::ResourceRelease(m_h);
			else {
				try {
					traits_type::ResourceRelease(m_h);
				} catch (RCExc&) {
				//	TRC(0, e);
				}
			}
		}
	}

	bool Valid() const {
		return traits_type::ResourceValid(m_h);
	}

	void Close() {
		if (Valid())
			traits_type::ResourceRelease(exchange(m_h, traits_type::ResourseNull()));
	}

	handle_type& OutRef() {
		if (Valid())
			Throw(E_EXT_AlreadyOpened);
		return m_h;
	}

	handle_type Handle() const {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;
	}

	handle_type operator()() const { return Handle(); }

	handle_type& operator()() {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;		
	}

	ResourceWrapper& operator=(const ResourceWrapper& res) {
		handle_type tmp = exchange(m_h, traits_type::ResourseNull());
		if (traits_type::ResourceValid(tmp))
			traits_type::ResourceRelease(tmp);
		if (traits_type::ResourceValid(res.m_h))
			m_h = traits_type::Retain(res.m_h);
		return *this;
	}

};


class CEscape {
public:
	virtual ~CEscape() {}

	virtual void EscapeChar(std::ostream& os, char ch) {
		os.put(ch);
	}

	virtual int UnescapeChar(std::istream& is) {
		return is.get();
	}

	static AFX_API String AFXAPI Escape(CEscape& esc, RCString s);
	static AFX_API String AFXAPI Unescape(CEscape& esc, RCString s);
};

struct CExceptionFabric {
	CExceptionFabric(int facility);
	virtual DECLSPEC_NORETURN void ThrowException(HRESULT hr, RCString msg) =0;
};

DECLSPEC_NORETURN void AFXAPI ThrowS(HRESULT hr, RCString msg);



} // Ext::

