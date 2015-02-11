#pragma once

extern "C" __int64 CDeclTrampoline(void *pArgs, size_t stacksize, PFNCDecl pfn);

#if UCFG_64
inline __int64 StdCallTrampoline(void *pArgs, size_t stacksize, PFNStdcall pfn) { return CDeclTrampoline(pArgs, stacksize, pfn); }
#else
extern "C" __int64 StdCallTrampoline(void *pArgs, size_t stacksize, PFNStdcall pfn);
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

class CForeignLibrary : public Object {
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


