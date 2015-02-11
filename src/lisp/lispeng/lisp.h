#pragma once

#if UCFG_OLE
#	include <el/libext/win32/ext-win.h>
#endif

#include "lispmsg.h"

#ifdef WIN32
#	include "LispEng_h.h"
#endif

#ifdef _AFXDLL
#	ifdef _LISPENG
#		define LISPENG_CLASS       AFX_CLASS_EXPORT
#	else
#		pragma comment(lib, "lispeng")
#		define LISPENG_CLASS       AFX_CLASS_IMPORT
#	endif
#else
#	define LISPENG_CLASS
#endif

#pragma warning(disable: 4244) // conversion from 'const Lisp::CP' to 'unsigned long', possible loss of data 
#pragma warning(disable: 4611) // interaction between ' function ' and C++ object destruction is non-portable 


namespace Lisp {

const byte //!!!D STREAM_FLAG_INPUT       = 1,
	//!!!DSTREAM_FLAG_OUTPUT      = 2,
	STREAM_FLAG_FILE        = 4,
	STREAM_FLAG_OPEN        = 8,
	//!!!           STREAM_FLAG_INTERACTIVE = 16,
	STREAM_FLAG_BINARY      = 32;


enum CEnumStream {
		STM_StandardInput, STM_StandardOutput, STM_ErrorOutput, STM_TraceOutput,
		STM_DebugIO, STM_QueryIO, STM_TerminalIO
};

class IPutChar {
public:
	virtual void put(String::value_type ch) =0;
};

class StandardStream : public Object, public IPutChar {
	typedef StandardStream class_type;
public:
	DWORD m_flags;
	observer_ptr<Ext::Encoding> Encoding;
	int64_t m_nPos;

	StandardStream(DWORD flags = 0)
		:	m_flags(flags)
		,	m_nPos(0)
	{}

	virtual bool operator!() {return false;}
	//!!!D  virtual void putback(wchar_t ch) {}
	virtual int get();
	virtual void put(String::value_type ch);
	virtual int _Fileno() { return -1; }
	virtual int ReadByte() { Throw(E_FAIL); }
	virtual void WriteByte(byte b) { Throw(E_FAIL); }

	virtual int64_t Seek(int64_t offset, SeekOrigin origin) { Throw(E_FAIL); }
	virtual void Flush() {}
	virtual void Close() {}
	virtual ostream *GetOstream() { Throw(E_FAIL); }
	virtual bool IsInteractive() { return false; }
	virtual bool Listen() { return true; } //!!!
	virtual void ClearInput() {}

	virtual int64_t get_Length() { Throw(E_FAIL); }
	DEFPROP_VIRTUAL_GET(int64_t, Length);

	virtual bool get_CanRead() { return false; }
	DEFPROP_VIRTUAL_GET(bool, CanRead);

	virtual bool get_CanWrite() { return false; }
	DEFPROP_VIRTUAL_GET(bool, CanWrite);
};

#if UCFG_OLE

class ComStandardStream : public StandardStream {
public:
	ComStandardStream(IStandardStream *iSS)
		:	m_iSS(iSS)
	{}

	bool IsInteractive() {
		BOOL b;
		OleCheck(m_iSS->get_IsInteractive(&b));
		return b;
	}

	bool get_CanRead() {
		BOOL b;
		OleCheck(m_iSS->get_CanRead(&b));
		return b;
	}

	bool get_CanWrite() {
		BOOL b;
		OleCheck(m_iSS->get_CanWrite(&b));
		return b;
	}

	int get();

	void put(String::value_type ch) override { OleCheck(m_iSS->put(ch)); }
private:
	CComPtr<IStandardStream> m_iSS;
};

class OleStandardStream : public StandardStream {
public:
	OleStandardStream(IStream *iStm)
		:	m_iStm(iStm)
	{}

	void WriteByte(byte b) override {
		ULONG cb;
		OleCheck(m_iStm->Write(&b, 1, &cb));
		if (cb != 1)
			Throw(E_FAIL);
	}
private:
	CComPtr<IStream> m_iStm;
};


#endif

//!!!D typedef ios StreamImp;
//!!!D typedef ostream OStreamImp;
typedef FILE StreamImp;
typedef FILE OStreamImp;

class IosStream : public StandardStream {
public:
	StreamImp *m_ios;
	int m_mode;

	IosStream()
		:	m_ios(0)
		,	m_mode(0)
	{}

	void Init(StreamImp *ios) {
		m_ios = ios;
	}

	bool operator!() { return ferror(m_ios); }
	bool get_CanRead() { return m_mode & ios::in; }
	bool get_CanWrite() { return m_mode & ios::out; }
	void ClearInput() { CCheck(fflush(m_ios)); }
	int _Fileno() { return (int)fileno(m_ios); } //!!! CE
	//!!!D int _Fileno() { return m_ios->rdbuf()->_Fileno(); }
	bool IsInteractive() {
#if UCFG_WCE
		return m_ios==stdin || m_ios==stdout || m_ios==stderr;
#else
		return isatty(_Fileno());		
#endif
	}
};

class InputStream : public virtual IosStream {
	typedef IosStream base;
public:
	FILE *m_is;

	InputStream(FILE *is = 0);

	void Init(FILE *is) {
		IosStream::Init(m_is = is);
	}

	int ReadByte() { return fgetc(m_is); } //!!!may be macro?
	int get();
	
	/*!!!D		int ReadByte()
	{
	int ch = m_is->get();
	cerr.put(ch);
	return ch;
	}*/
};

class TerminalInputStream : public InputStream {
	typedef InputStream base;
public:
	TerminalInputStream(FILE *file)
		:	base(file)
	{}	

	bool IsInteractive() { return true; }
	int get() override;
private:
	deque<wchar_t> m_queue;	
};

class OutputStream : public virtual IosStream {
	typedef IosStream base;
public:
	OStreamImp *m_os;

	OutputStream(OStreamImp *os = 0);

	void Init(FILE *os) {
		IosStream::Init(m_os = os);
	}

	//!!!	OStreamImp *GetOstream() { return m_os; }

	void WriteByte(byte b) override;
	void put(String::value_type ch) override;
};

class CPutCharOstream : public ostream {
public:
	class PutCharStreambuf : public streambuf {
		int overflow(int c) override {
#if UCFG_WIN32_FULL
			if (c != EOF) {
				int d;
				CharToOem((LPTSTR)&c, (LPSTR)&d);
				c = d;
			}
#endif
			if (c == 7) //!!! BELL
				return c; 
			return streambuf::overflow(c);
		}
	public:
		IPutChar *m_iPutChar;
	} m_sbuf;

	CPutCharOstream(IPutChar *iPutChar)
		:	ostream(&m_sbuf)
	{
		m_sbuf.m_iPutChar = iPutChar;
	}
};

class CLispSink {
public:
	virtual void OnExit() { exit(1); } //!!!
	virtual void OnED(RCString name) { Throw(E_FAIL); } //!!!
};

typedef uintptr_t CP;

class CLisp {
	typedef CLisp class_type;
protected:
	CLisp();

public:
	vector<ptr<StandardStream> > m_streams;

	/*!!!
	enum CEnumStream
	{
	STM_StandardInput, STM_StandardOutput, STM_ErrorOutput, STM_TraceOutput,
	STM_DebugIO, STM_QueryIO, STM_TerminalIO
	};
	*/

	bool m_bProfile,
		m_bDebug,
		m_bTraceError,
		m_bPrintLogo,
		m_bVerifyVersion;
	int m_exitCode;

	//!!!D  CPointerArray<CTextStream> m_arStream;

	CLispSink m_sink;
	observer_ptr<CLispSink> m_pSink;

	LISPENG_CLASS virtual ~CLisp();
	LISPENG_CLASS virtual void Init(bool bBuild = false) =0;
	LISPENG_CLASS virtual void Loop()  =0;
	LISPENG_CLASS virtual void ShowInfo()  =0;
	LISPENG_CLASS virtual void SetSignal(bool bSignal = true) =0;
	LISPENG_CLASS virtual void Load(Stream& stm) =0;
	LISPENG_CLASS virtual void Compile(RCString name) =0;
	LISPENG_CLASS virtual int Run() =0;
	LISPENG_CLASS virtual void LoadFile(const path& p)  =0;
	LISPENG_CLASS virtual void SaveImage(Stream& stm)  =0;
	LISPENG_CLASS virtual String Eval(RCString sForm)  =0;
	LISPENG_CLASS virtual CP VGetSymbol(RCString name, RCString pack = "CL-USER")  =0;
	
	LISPENG_CLASS virtual void VCall(CP p) =0;
#if UCFG_OLE
	LISPENG_CLASS virtual COleVariant VCall(CP p, const vector<COleVariant>& params) =0;

	LISPENG_CLASS virtual void put_Stream(CEnumStream idx, IStandardStream *iStream) =0;
#endif
	
	LISPENG_CLASS virtual void ParseArgs(char **argv) =0;
	LISPENG_CLASS virtual void ProcessCommandLine(int argc, char *argv[])  =0;


#ifdef _WIN32
	LISPENG_CLASS virtual void put_Stream(CEnumStream idx, ptr<StandardStream> p) =0;
	__declspec(property(put=put_Stream)) ptr<StandardStream> Streams[];
#endif


	int m_Signal;

	int get_Signal() { return m_Signal; }
	virtual void put_Signal(int sig) { m_Signal = sig; }
	DEFPROP_VIRTUAL(int, Signal);	


	LISPENG_CLASS static CLisp * AFXAPI CreateObject();
};


} // Lisp::






