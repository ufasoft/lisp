#pragma once

typedef void *LISPHANDLE;

__BEGIN_DECLS

	LISPHANDLE __stdcall LispCreate();
	void __stdcall LispClose(LISPHANDLE);
	HRESULT __stdcall LispLoad(LISPHANDLE h, const char *filename);
	void __stdcall LispBreak(LISPHANDLE);
	LISPHANDLE __stdcall LispGetCurrent();
#ifdef WIN32
	BSTR __stdcall LispEval(LISPHANDLE, const char *s);
#endif
	void __stdcall LispSetStandardStream(LISPHANDLE, int idx, void *stdStream);

__END_DECLS


