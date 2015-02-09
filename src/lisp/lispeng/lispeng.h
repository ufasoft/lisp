#pragma once

#ifdef _WIN32
#	include <el/libext/ext-os-api.h>
#endif

#include <cmath>
#include <random>

#include "lisp.h"
#include "resource.h"

#if defined(_WIN32) && !UCFG_WCE
#	include <corhdr.h>
#elif UCFG_WCE && UCFG_STDSTL

typedef enum  CorPinvokeMap {
    pmNoMangle          = 0x0001,   // Pinvoke is to use the member name as specified.

    // Use this mask to retrieve the CharSet information.
    pmCharSetMask       = 0x0006,
    pmCharSetNotSpec    = 0x0000,
    pmCharSetAnsi       = 0x0002,
    pmCharSetUnicode    = 0x0004,
    pmCharSetAuto       = 0x0006,


    pmBestFitUseAssem   = 0x0000,
    pmBestFitEnabled    = 0x0010,
    pmBestFitDisabled   = 0x0020,
    pmBestFitMask       = 0x0030,

    pmThrowOnUnmappableCharUseAssem   = 0x0000,
    pmThrowOnUnmappableCharEnabled    = 0x1000,
    pmThrowOnUnmappableCharDisabled   = 0x2000,
    pmThrowOnUnmappableCharMask       = 0x3000,

    pmSupportsLastError = 0x0040,   // Information about target function. Not relevant for fields.

    // None of the calling convention flags is relevant for fields.
    pmCallConvMask      = 0x0700,
    pmCallConvWinapi    = 0x0100,   // Pinvoke will use native callconv appropriate to target windows platform.
    pmCallConvCdecl     = 0x0200,
    pmCallConvStdcall   = 0x0300,
    pmCallConvThiscall  = 0x0400,   // In M9, pinvoke will raise exception.
    pmCallConvFastcall  = 0x0500,

    pmMaxValue          = 0xFFFF,
} CorPinvokeMap;


#elif !UCFG_USE_POSIX
#	include <windows/corhdr.h>
#endif


#include <el/bignum.h>
//!!!#include <BitStream.h>

#include "gc.h"


//!!! #pragma warning(disable : 4651 4652 4200)



namespace Lisp {

#include "params.h"

#if UCFG_LISP_SPARE_STACK
#	define LISP_LISPREF
#	define LISP_SAVE_LISPREF(ref)
#	define LISP_GET_LISPREF Lisp()
#else
#	define LISP_LISPREF CLispEng& m_lisp;
#	define LISP_SAVE_LISPREF(ref) m_lisp(ref),
#	define LISP_GET_LISPREF m_lisp
#endif



const int E_LISP_BASE = E_LISP_InvalidSyntax,
	E_LISP_UPPER = E_LISP_BASE+300;

const DWORD E_LISP_Base = E_LISP_InvalidSyntax-1;

const size_t RESERVED_CONS = 6; //!!!

const HRESULT HR_UNWIND   = 0x03000000,
	UNWIND_MASK = 0xFF000000;

const INT_PTR FIXNUM_LIMIT = INT_PTR(1) << (INT_BITS-1);
const int TYPE_BITS = LISP_BITS_IN_CP-VAL_BITS;


class CSValue;
class CSValueEx;
//!!!class CObjectValue;
class CLispEng;
class CStreamValue;
class CIntFuncValue;
class CPackage;
class CSymbolValue;
class CHashTable;
class CVarFrame;
class CJmpBufBase;

const DWORD FLAG_DYNAMIC = 0x00000080,
	FLAG_ACTIVE  = 0x00000040,
	FLAG_SP      = 0x00000020,
	FLAG_USED    = 0x00000080,
	MASK_WITHOUT_FLAGS = 0xFFFFFF1F,
	FLAG_NESTED     = 0x80,
	FRAME_TYPE_MASK = 0x7F;

//!!! #define WITHOUT_FLAGS(p) (p & MASK_WITHOUT_FLAGS)
#define WITHOUT_FLAGS(p) (p)

const size_t FRAME_FORM          = 1,
	FRAME_CLOSURE       = 2,
	FRAME_COUNT         = 1,
	FRAME_NEXT_ENV      = 2,
	FRAME_CTAG          = 1,
	FRAME_TAG           = 2,
	FRAME_HANDLERS      = 3,
	FRAME_NAME          = 1,
	FRAME_ARGS          = 2,
	FRAME_BINDINGS      = 3,
	FRAME_TAG_BINDINGS  = 2,
	FRAME_SP            = 1,
	FRAME_TAG_NEXT_ENV  = 1;

const byte FLAG_Mark = 0x80;

enum CTypeSpec {
#	define LTYPE(ts, isnum, man) ts,
#	define LMAN(typ, ts, man, init)
#	include "typedef.h"
#	undef LTYPE
#	undef LMAN
};


enum {
#	define LTYPE(ts, isnum, man)
#	define LMAN(typ, ts, man, init) ENUM_##man,
#	include "typedef.h"
#	undef LTYPE
#	undef LMAN
	ENUM_NONE_MAN = -1
};


	

#define LSYM(name, sym) ENUM_##sym,

enum CLispSymbol {
#include "symdef.h"
	L_L_LAST_LABEL
};

const int VALUE_SHIFT = 8;

class CSPtr {
	CP m_p;
public:

	CSPtr(CP p = 0)
		:	m_p(p)
	{}

	operator CP() const { return m_p; }
	inline CSPtr& operator=(CP p) { m_p = p; return _self; }
};

const DWORD BINLISP_SIG = 0x0050534C; // "LSP"

#pragma pack(push, 4)

struct CBlsHeader {
	uint32_t m_sig;
	uint64_t m_ver;
	//!!!            m_verLastCompatible;

	CBlsHeader(bool bVerifyVersion);
};

#pragma pack(pop)

class CSValue {
public:
	void *operator new(size_t size, void *p) { return p; }
	void operator delete(void *, void*){}

#if UCFG_WCE //!!!CE
	void operator delete(void *p)
	{}
#endif
};

#if UCFG_LISP_FFI

const byte TS_EX_FF_PTR = 64+TS_FF_PTR;

#endif

class CSValueEx : public CSValue {
public:
	byte m_type;
	byte m_subtype;

	CSValueEx()
		:	m_type(0)
		,	m_subtype(0)
	{}
};

#if UCFG_LISP_MT
	extern EXT_THREAD_PTR(CLispEng) t_pLisp;
#else
	extern CLispEng *t_pLisp;
#endif

#define Lisp() (*t_pLisp)

class CLispThreadKeeper {
	CLispEng *m_prev;
public:
	CLispThreadKeeper(CLispEng *cur) {
		m_prev = t_pLisp;
		t_pLisp = cur;
	}

	~CLispThreadKeeper() {
		t_pLisp = m_prev;
	}
};

CLispEng& __stdcall GetLisp() noexcept;

#ifdef _LISPENG
#	define LISP Lisp()
#else
#	define LISP GetLisp()
#endif

#ifdef _DEBUG
#	define DEBUG_NOT_INLINE // to decrease using of stack
#else
#	define DEBUG_NOT_INLINE __forceinline
#endif

__forceinline CP FromValue(intptr_t s, CTypeSpec ts) noexcept {
	return ts | (uintptr_t(s) << VALUE_SHIFT);
}

inline CP CreateChar(wchar_t ch) {
	return FromValue(char_traits<wchar_t>::to_int_type(ch), TS_CHARACTER);
}

inline CP CreateFixnum(uint16_t n) { return FromValue(n, TS_FIXNUM); }
inline CP CreateFixnum(intptr_t n) { return FromValue(n, TS_FIXNUM); }
inline CP CreateFixnum(size_t i) { return CreateFixnum((intptr_t)i); }

#if SIZE_MAX > UINT_MAX
	inline CP CreateFixnum(int n) { return FromValue(n, TS_FIXNUM); }
#endif

inline CTypeSpec Type(CP p) noexcept { return CTypeSpec(byte(p)); }

inline size_t AsIndex(uintptr_t p) noexcept {
	return p >> VALUE_SHIFT;
}


inline CP CreateStackRange(int off, int len) {				// max len == 31
	return FromValue((off<<5)|len, TS_STACK_RANGE);
}

inline pair<int, int> FromStackRange(CP p) {
	int idx = AsIndex(p);
	return make_pair(idx>>5, idx & 31);
}


DECLSPEC_NORETURN void E_Error();


//!!!const MAN_COUNT = 14; //!!!

class CObMap {
public:
	//!!!uint32_t *m_arP[MAN_COUNT];

	struct CVal {
		typedef CVal class_type;

		static const intptr_t FLAG_MASK = intptr_t(1) << (sizeof(intptr_t)*8-1);

		intptr_t m_val;

		CVal(intptr_t val = 0)
			:	m_val(val)
		{
		}

		bool get_Flag() {			
			return m_val & FLAG_MASK;
		}

		void put_Flag(bool v) {
			m_val = m_val & ~FLAG_MASK | (v ? FLAG_MASK : 0);
		}
		DEFPROP(bool, Flag);

		operator intptr_t() const {
			return m_val;
		}		
	};

	typedef vector<CVal> ContType;
	vector<ContType> m_arP;				

	CObMap();

	~CObMap() {
		//!!!D    for (int i=0; i<MAN_COUNT; i++)
		//!!!D      delete[] m_arP[i];
	}
};

#if UCFG_LISP_FFI
class CForeignLibrary;
#endif

class BlsBase {
public:
	vector<size_t> m_arCount;

#if UCFG_LISP_FFI
	mutable vector<CForeignLibrary*> m_vForeignLib;
#endif

	BlsBase();
};

class BlsWriter : public BinaryWriter, public BlsBase {
	typedef BinaryWriter base;
public:
	CObMap m_obMap;

	BlsWriter(Stream& stm)
		:	base(stm)
	{}

	void WriteBytes(int n, uintptr_t dw) {
		base::Write(&dw, n);
	}

	void WriteLens();
	void WriteVal(byte ts, uintptr_t idx);
	void WriteSignedVal(byte ts, intptr_t idx);
	void WriteCP(const CSPtr& p);

	/*!!!
	BlsWriter& operator<<(const CSPtr& p) {
		WriteCP(p);
		return _self;
	}

	BlsWriter& operator<<(CP p) {
		WriteCP(p);
		return _self;
	}*/

	//!!!  static int CalculateBits(uint32_t count);
	//!!!  void CalculateBits();
};

inline BlsWriter& operator<<(BlsWriter& wr, CP p) {
	wr.WriteCP(p);
	return wr;
}

inline BlsWriter& operator<<(BlsWriter& wr, const CSPtr& p) {
	wr.WriteCP(p);
	return wr;
}


class BlsReader : public BinaryReader, public BlsBase {
	typedef BinaryReader base;
public:
	BlsReader(Stream& stm)
		:	base(stm)
	{}

	DWORD_PTR ReadBytes(int n) const {
		DWORD_PTR r = 0;
		Read(&r, n);
		return r;
	}

	void ReadLens();
	pair<byte, uintptr_t> ReadByteVal() const;
	CP ReadVal(byte ts) const;

};

const BlsReader& operator>>(const BlsReader& rd, CSPtr& p);
inline const BlsReader& operator>>(const BlsReader& rd, CP& p) { return operator>>(rd, (CSPtr&)p); }

/*!!!
inline BlsWriter& operator<<(BlsWriter& wr, const CSPtr& p) {
	wr.Write(p);
	return wr;
}

inline BlsWriter& operator<<(BlsWriter& wr, CP p) {
	wr.Write(p);
	return wr;
}*/

class CFloat : public
#if UCFG_LISP_GC_USE_ONLY_BITMAP
	CSValue
#else
	CSValueEx
#endif
{
public:
	double m_d;

	CFloat(double d = 0)
		:	m_d(d)
	{
	}

	void Read(const BlsReader& rd) {
		rd >> m_d;
	}

	void Write(BlsWriter& wr) {
		(BinaryWriter&)wr << m_d;
	}
};

class CBignum : public CSValue {
public:
	BigInteger m_int;
#if UCFG_BIGNUM=='N'		// sizeof(CBignum) should be >= 8
	uint32_t m_dummy;
#endif

	CBignum(const BigInteger& i = 0)
		:	m_int(i)
	{
	}

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
private:
	void Assert() {
		static_assert(sizeof(CBignum) >= 8, "sizeof_Bignum_very_small");
	}
};

const uint32_t FLAG_ENTERED = 0x80000000;

#pragma pack(push, 4)
struct CProfInfo {
	int64_t m_nTicks;
	uint32_t m_nCount;

	CProfInfo()
	{}

	CProfInfo(bool)
		:	m_nCount(0)
		,	m_nTicks(0)
	{
	}
};
#pragma pack(pop)

class CProfPoint;

#if UCFG_LISP_PROFILE
extern CProfInfo *g_arProfBegin[],
	**g_arProfEnd;

class CProfPoint {
public:
	__forceinline CProfPoint(CProfInfo& profInfo) {
		profInfo.m_nCount++;
		if (*g_arProfEnd++ = *((uint32_t*)&profInfo.m_nTicks+1) & 0x80000000 ? 0 : &profInfo)
			profInfo.m_nTicks -= __rdtsc();
	}

	__forceinline ~CProfPoint() {
		if (CProfInfo *pi = *--g_arProfEnd)
			pi->m_nTicks += __rdtsc();
	}
};

#	define PROF_POINT(profInfo) CProfPoint _profPoint(profInfo);

#else
#	define PROF_POINT(profInfo)
#endif //LISP_PROFILE

#pragma pack(push, 1)

class CQuickString {
public:
	typedef String::value_type char_type;
	typedef char_traits<char_type> traits_type;

	byte m_len,
		m_stub;  // to align
	String::value_type m_buf[3];
	uint32_t m_sObj;
	String::value_type m_bufStub[4];

	CQuickString()
		:	m_len(0)
	{}

	CQuickString(const char *p) {
		Init(String(p));
	}

	CQuickString(const String::value_type *p) {
		Init(p);
	}

	CQuickString(RCString s) {
		Init(s);
	}

	~CQuickString() {
		if (m_len == 255)
			StringRef().~String();
	}

	CQuickString& operator=(const CQuickString& s) {
		if (this != &s) {
			Clear();
			if (s.m_len == 255)
				new(&m_sObj) String(s.StringRef());
			else
				memcpy(&m_len, &s.m_len, sizeof(CQuickString)-4);
		}
		return _self;
	}

	CQuickString& operator=(RCString s) {
		Clear();
		size_t len = s.length();
		if (len <= MAX_EMBEDDED_LEN) {
			m_len = (byte)len;
			Ext::StrCpy<String::value_type>(m_buf, s);
		} else {
			m_len = 255;
			new(&m_sObj) String(s);
		}
		return _self;
	}

	const String::value_type *GetPtr() const { return m_len==255 ? (const String::value_type*)(String&)m_sObj : m_buf; }
	operator const String::value_type *() const { return GetPtr(); } //!!!

	operator String() const {
		return m_len==255 ? StringRef() : m_buf;
	}

	String& StringRef() const {
		return (String&)m_sObj;
	}

	void Clear() {
		if (m_len == 255)
			StringRef().~String();
	}
private:
	//!!!	static const MAX_EMBEDDED_LEN = 8;
	static const size_t MAX_EMBEDDED_LEN;

	void Init(const String::value_type *p) {
		size_t len = traits_type::length(p);
		if (len <= MAX_EMBEDDED_LEN) {
			m_len = (byte)len;
			StrCpy(m_buf, p);
		} else {
			m_len = 255;
			new(&m_sObj) String(p);
		}	
	}
};

inline bool operator<(const CQuickString& s1, const CQuickString& s2) {
	return StrCmp(s1.GetPtr(), s2.GetPtr()) < 0;
}

#pragma pack(pop)

//!!!typedef CQuickString CLString;
typedef String CLString;

const uint32_t SYMBOL_FLAG_CONSTANT = 0x40,
	SYMBOL_FLAG_SPECIAL = 0x20;
//!!!            SYMBOL_FLAG_MACRO    = 0x80000000,
//!!!            SYMBOL_FLAG_MACROLET = 0x40000000;

#pragma pack(push, 1)

class CSymbolValue : public CSValue {
	typedef CSymbolValue class_type;
public:
	CP m_fun,
		m_dynValue;

	//!!!  byte m_byFlags;
	intptr_t m_byPackage : 8;
	//!!!  byte m_fun[3];
	//!!!byte m_props[3];
	intptr_t m_plist : CP_VALUE_BITS;

#if !UCFG_LISP_SPLIT_SYM
	CLString m_s;
#endif

	CP GetFun() {
		return m_fun & (MASK_WITHOUT_FLAGS & 0x3FFFFFFF);
	}

	void SetFun(CP p) {
		m_fun = m_fun & ~(MASK_WITHOUT_FLAGS & 0x3FFFFFFF) | p;
	}

	CP GetPropList() {
		return (m_plist << VALUE_SHIFT) | TS_CONS;
	}

	void SetPropList(CP p) {
		m_plist = AsIndex(p);
	}

	CP get_HomePackage() {
		if (m_byPackage)
			return ((m_byPackage-1) << VALUE_SHIFT) | TS_PACKAGE;
		else
			return 0;
	}

	void put_HomePackage(CP p) {
		m_byPackage = p ? byte(AsIndex(p)+1) : 0;
	}
	DEFPROP(CP, HomePackage);

	/*!!!
	CSPtr m_propList;
	CSPtr m_func;
	CSPtr m_package;
	union
	{
	struct
	{
	bool m_bConstant      : 1;
	bool m_bDynValue      : 1;
	bool m_bMacro         : 1;
	bool m_bMacrolet      : 1;
	};
	uint32_t m_dwFlags;
	};*/

	CSymbolValue() {
		*(CP*)this = 0;
		*((CP*)this+1) = 0;
		*((CP*)this+2) = 0;

		/*!!!    m_dynValue = 0;
		m_bConstant = false;
		m_bDynValue = false;
		m_bMacro = false;
		m_bMacrolet = false;*/
	}

#if !UCFG_LISP_SPLIT_SYM
	CSymbolValue(const String::value_type *s)
		:	m_s(s)
	{
		*(CP*)this = 0;
		*((CP*)this+1) = 0;
		*((CP*)this+2) = 0;
	}
#endif

	bool get_SymMacroP() { return (m_fun & (SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL)) == SYMBOL_FLAG_CONSTANT; }
	void put_SymMacroP(bool b) { m_fun = (m_fun & ~(SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL)) | ( b ? SYMBOL_FLAG_CONSTANT : 0); }
	DEFPROP(bool, SymMacroP);

	bool get_SpecialP() { return m_fun & SYMBOL_FLAG_SPECIAL; }
	DEFPROP_GET(bool, SpecialP);

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
private:
	enum {
		STREAM_FLAG_STANDARD_NAME = 4
	};
};

#pragma pack(pop)



/*!!!

class CSymbolCompare
{
public:
uint32_t operator()(CSymbolValue *sv) const
{
size_t r = 0;
const wchar_t *p = sv->m_s;
int count = sv->m_s.Length;
while (count--)
r = _lrotl(r, 8)+*p++;
return r;
}

bool Equals(CSymbolValue *sv1, CSymbolValue *sv2) const { return sv1->m_s == sv2->m_s; }
};

typedef hash_set<CSymbolValue*, CSymbolCompare> CSymMap;
*/

const byte PACKAGE_SYM_EXTERNAL = 1,
			PACKAGE_SYM_SHADOWING = 2,
			PACKAGE_SYM_OWNS = 4;

#if UCFG_LISP_SPLIT_SYM

	class CSymMap : public map<String, CSymbolValue*> {
		CLispEng& m_lisp;
	public:
		CSymMap()
			:	m_lisp(Lisp())
		{}

		CSymMap(const CSymMap& sm)
			:	m_lisp(sm.m_lisp)
		{}

		CSymbolValue *Find(RCString name);
		bool FindCP(RCString name, CP& p);

		template <typename C> void ForEachSymbolValue(C& c) {
			for (iterator i=begin(); i!=end(); ++i)
				c(i->second);
		}
	};

#else

	struct CSymbolValueHash {
		size_t operator()(CSymbolValue *sv) const {
			return hash<CLString>()(sv->m_s);
		}
	};

	struct CSymbolValueEqual {
		size_t operator()(CSymbolValue *sv1, CSymbolValue *sv2) const {
			return sv1->m_s == sv2->m_s;
		}
	};

	class CSymMap : public unordered_map<CSymbolValue*, byte, CSymbolValueHash, CSymbolValueEqual> {
		typedef unordered_map<CSymbolValue*, byte, CSymbolValueHash, CSymbolValueEqual> base;

		CLispEng& m_lisp;
	public:
		CSymMap()
			:	m_lisp(Lisp())
		{}

		/*!!!R
		CSymMap(const CSymMap& sm)
			:	base(CSymbolCompare(sm.m_lisp)),
				m_lisp(sm.m_lisp)
		{}*/

		CSymbolValue *Find(RCString name);
		bool FindCP(RCString name, CP& p, byte& flags);

		template <typename C> void ForEachSymbolValue(C& c) {
			for (iterator i=begin(), e(end()); i!=e; ++i)
				c(*i);
		}
	};

#endif

class CPackage : public CSValue {
public:
	CSPtr m_useList;
	CSymMap m_mapSym;
	CSPtr m_docString;
	String m_name;
	CStringVector m_arNick;

	~CPackage();
	void Connect();
	void Disconnect();
	//!!!  CSymbolValue *FindSymbol(RCString name);
	CP __fastcall GetSymbol(CLispEng& lisp, const CLString& name, bool bFindInherited = true);	
	void AdjustSymbols(ssize_t offset);
	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};

class CConsValue : public CSValue {
public:
	CP m_car,
		m_cdr;

	void Write(BlsWriter& wr) {
		wr << m_car << m_cdr;
	}

	void Read(const BlsReader& rd) {
		rd >> m_car >> m_cdr;
	}
};

class LispFileStream : public InputStream, public OutputStream {
public:
	//!!! fstream m_fs;
	FileStream m_stm;

	void Open(RCString path, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::Read) {
		m_stm.Open(path, mode, access, share);
		InputStream::Init(m_stm.m_fstm);
		OutputStream::Init(m_stm.m_fstm);
	}

	int64_t get_Length() {
		return m_stm.Length;
	}

	int64_t Seek(int64_t offset, SeekOrigin origin) {
		return m_stm.Seek(offset, origin);
	}

	void Flush() {
		m_stm.Flush();
	}

	void Close() {
		m_stm.Close();
	}
};


enum {
	STS_STREAM = 0,
	STS_SYNONYM_STREAM,
	STS_TWO_WAY_STREAM,
	STS_STRING_STREAM,
	STS_FILE_STREAM,
	STS_CONCATENATED_STREAM, 
	STS_BROADCAST_STREAM,
	STS_ECHO_STREAM,
	STS_PPHELP_STREAM
};

const byte STREAM_FLAG_INPUT = 1,
	STREAM_FLAG_OUTPUT = 2,
	STREAM_FLAG_FASL = 4;

class CStreamValue : public CSValueEx {
public:
	CSPtr m_in, // or symbol for synonym-stream
		m_out,
		m_writer,
		m_reader,
		m_elementType,
		m_pathname;
	ptr<StandardStream> m_stream;


	CSPtr m_char;
	path TrueName;
	int m_nStandard;
	int m_nLine;	
	int m_nCur, m_nEnd;
	int m_elementBits;
	int m_curOctet;
	byte m_flags;
	CBool m_bNeedFlushOctet;
	bool m_bUnread,
		m_bEchoed,
		m_bMultiLine,
		m_bSigned,
		m_bClosed;
	FileMode m_mode;


	//!!!D	int ReadChar();
	//!!!D	void PutBackChar(wchar_t ch);

	CStreamValue();
	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
	int64_t& LinePosRef();
private:
	int64_t m_nPos;
};

class CHashMap;

class CHashTable : public CSValueEx {
	EXT_MOVABLE_BUT_NOT_COPYABLE(CHashTable);
public:
	CSPtr m_rehashSize,
		m_rehashThreshold;

	unique_ptr<CHashMap> m_pMap;
	
	CHashTable();

	CHashTable(EXT_RV_REF(CHashTable) rv)
		:	m_rehashSize(rv.m_rehashSize)
		,	m_rehashThreshold(rv.m_rehashThreshold)
		,	m_pMap(rv.m_pMap.release())
	{
	}

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};


/*!!!

const CCV_SPDEPTH_1           = 0,
CCV_SPDEPTH_JMPBUFSIZE  = 2,
CCV_NUMREQ              = 4,
CCV_NUMOPT              = 6,
CCV_FLAGS               = 8,
CCV_SIGNATURE           = 9,
CCV_NUMKEY              = 10,
CCV_KEYCONSTS           = 12,

const BYTECODE_BEGIN = sizeof(CCodevec);
*/

const size_t CCV_START_NONKEY        = 10,
	CCV_START_KEY           = 14;

class CRatio : public CSValue {
public:
	CSPtr m_numerator,
		m_denominator;
};

class CComplex : public CSValue {
public:
	CSPtr m_real,
		m_imag;
};

class CRandomState : public CSValue {
public:
	CSPtr m_rnd,
		m_stub;
};

class CSymbolMacro : public CSValue {
public:
	CSPtr m_macro,
		m_stub;
};

class CFunctionMacro : public CSValue {
public:
	CSPtr m_function,
		m_expander;
};

class CMacro : public CSValue {
public:
	CSPtr m_expander,
			m_lambdaList;
};

enum {
	ELTYPE_T = 0,
	ELTYPE_BIT,
	ELTYPE_CHARACTER,
	ELTYPE_BASECHAR,
	ELTYPE_BYTE
};

class CMultiIterator {
	void Inc(ssize_t i);
public:
	vector<int> m_dims,
		m_idxs;
	uintptr_t *m_pData;
	byte m_elType;

	CBool m_bEnd;

	CMultiIterator(uintptr_t *pData, byte elType, CP dims);	
	operator bool() { return !m_bEnd; }
	void SetElement(CP v);

	void Inc() { Inc(m_idxs.size()-1); }
};

const byte FLAG_BeingUpdated = 8,
	FLAG_Displaced    = 0x20;

class CArrayValue : public CSValue {
	typedef CArrayValue class_type;
public:
	CSPtr m_fillPointer,
		m_displace,
		m_dims;

#if UCFG_LISP_PROFILE
	CProfInfo m_profInfo;
#endif

	union {
		uintptr_t *m_pData;
		uint32_t m_nDisplaceIndex;
	};

	byte m_stub,
		m_elType,
		m_flags;

	CArrayValue() {
		ZeroStruct(_self);
	}

	CArrayValue(const CArrayValue& av) {
		operator=((CArrayValue&)av);
	}

	~CArrayValue() { Dispose(); }
	CArrayValue& operator=(CArrayValue& av);
	void Dispose();
	bool SimpleP() { return true; }
	size_t GetRank();
	byte GetElementType();
	static size_t TotalSize(CP p);
	static size_t ElementBitSize(byte elType);
	static vector<int> GetDims(CP dims);

	size_t TotalSize() {
		return TotalSize(m_dims);
	}

	size_t CalculateRowIndex(CP *pRest, size_t nArgs);
	static uintptr_t *Alloc(size_t size);
	static void Fill(uintptr_t *pData, byte elType, size_t beg, size_t end, CP initEl);
	static uintptr_t *CreateData(byte elType, CP dims, CP initEl);
	CP GetElement(size_t i);
	void SetElement(size_t i, CP p);
	CP GetByIterator(const vector<int>& vdims, const CMultiIterator& mi);
	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);

	inline size_t GetVectorLength();

	inline size_t get_DataLength();
	DEFPROP_GET(size_t, DataLength);

	size_t GetByteLength() {
		size_t r = DataLength;
		switch (m_elType) {
		case ELTYPE_T: r *= sizeof(CP); break;
		case ELTYPE_BIT: r = (r+31)/32*4; break;
		case ELTYPE_CHARACTER: r *= 2; break;
		}
		return r;
	}  
};

const byte FLAG_CLOSURE_INSTANCE = 0x80;

class CClosure : public CSValue {
	typedef CClosure class_type;
public:
	struct CClosureHeader {
		WORD m_depth,
			m_depth_jmpbufsize,
			m_numreq,
			m_numopt;
		byte m_flags,
			m_sig;
		WORD m_numkey,
			m_keyconsts;
	};

	CP NameOrClassVersion,
		CodeVec,
		m_dims;

#if UCFG_LISP_PROFILE
	CProfInfo m_profInfo;
#endif

	CClosure() {
		ZeroStruct(_self); //!!!
	}

	CClosure(const CClosure& c) {
		memcpy(this, &c, sizeof(CClosure));
		//!!!((CClosure&)c).m_code = 0;
		//!!!D 		((CClosure&)c).m_profInfo = 0;
	}

	~CClosure() {
		//!!!D		delete[] m_code;
	}

	inline CClosureHeader& get_Header();
	DEFPROP_GET(CClosureHeader&, Header);

	WORD get_NumReq() { return get_Header().m_numreq; }
	void put_NumReq(WORD w) { get_Header().m_numreq = w; }
	DEFPROP(WORD, NumReq);

	WORD get_NumOpt() { return get_Header().m_numopt; }
	DEFPROP_GET(WORD, NumOpt);

	WORD get_NumKey() { return get_Header().m_numkey; }
	DEFPROP_GET(WORD, NumKey);

	WORD get_KeyConsts() { return get_Header().m_keyconsts; }
	DEFPROP_GET(WORD, KeyConsts);

	byte get_Flags() { return get_Header().m_flags; }
	DEFPROP_GET(byte, Flags);

	bool get_IsGeneric() { return Flags & 16; }
	DEFPROP_GET(bool, IsGeneric);

	byte get_Sig() { return get_Header().m_sig; }
	DEFPROP_GET(byte, Sig);

	CP get_VEnv();
	void put_VEnv(CP venv);
	DEFPROP(CP, VEnv);

	CP *get_Consts() { return ((CArrayValue*)this)->m_pData; }
	DEFPROP_GET(CP*, Consts);

	/*!!!
	void Read(CBlsStream& stm);
	void Write(CBlsStream& stm);
	*/
};

struct CIntFunc {
	CP m_form; // at 1
	CP m_vars, // at 9
		m_optInits, // at 10
		m_keyInits, // at 11
		//				m_req, // at 12
		//				m_opt, // at 13
		m_auxInits, // at 14
		m_keywords; // at 16
	bool m_bAllowFlag,  // at 18
		m_bRestFlag;  // at 19
	WORD m_nReq,
		m_nOpt;
	byte m_nSpec,       
		m_nKey,
		m_nAux;
};



enum CSyntaxType {
	ST_CONSTITUENT,
	ST_INVALID,
	ST_MACRO,
	ST_WHITESPACE,
	ST_SESCAPE,
	ST_MESCAPE
};

enum CTraits {
	TRAIT_ALPHABETIC = 1,
	TRAIT_ALPHADIGIT = 2,
	TRAIT_EXPONENT_MARKER = 4,
	TRAIT_MINUS      = 8,
	TRAIT_PLUS       = 16,
	TRAIT_SIGN       = 24,
	TRAIT_DOT        = 32,
	TRAIT_POINT      = 64,
	TRAIT_PACKAGE    = 128,
	TRAIT_RATIO      = 256,
	TRAIT_INVALID    = 512,
	TRAIT_EXTENSION  = 1024
};

struct CCharType {
	CSyntaxType m_syntax;
	WORD m_traits;
	bool m_bTerminating;

	CSPtr m_macro,
		m_disp;

	CCharType()
		:	m_syntax(ST_CONSTITUENT)
		,	m_traits(TRAIT_INVALID)
		,	m_bTerminating(false)
	{}
};

BlsWriter& operator<<(BlsWriter& wr, const CCharType& ct);
const BlsReader& operator>>(const BlsReader& rd, CCharType& ct);

class CReadtable : public CSValue {
public:
	CSPtr m_case;
	CCharType m_ar[128];

	typedef unordered_map<Char16, CCharType> CCharMap;
	CCharMap m_map;	

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};

class CWeakPointer : public CSValue {
public:
	CSPtr m_p,
		m_stub; //!!! for ref to next

	void Write(BlsWriter& wr) {
		wr << m_p;
	}

	void Read(const BlsReader& rd) {
		rd >> m_p;
	}
};

#define IDX_TS(idx, ts) ((int(idx)<<TYPE_BITS) | ts)

const CP V_T	= IDX_TS(ENUM_L_T, TS_SYMBOL),
	V_EOF = IDX_TS(1, TS_CONS),
	V_U	= IDX_TS(2, TS_CONS),
	V_D	= IDX_TS(3, TS_CONS),
	V_SPEC = IDX_TS(4, TS_CONS),
	//!!!				 V_US = V_D,  //!!! unbound slot, may be V_US=VU ?
	V_M1 = CP(IDX_TS(-1, TS_FIXNUM)),
	V_0	= IDX_TS(0, TS_FIXNUM),
	V_1	= IDX_TS(1, TS_FIXNUM),
	V_2	= IDX_TS(2, TS_FIXNUM),
	V_6	= IDX_TS(6, TS_FIXNUM),
	V_8	= IDX_TS(8, TS_FIXNUM),
	V_10 = IDX_TS(10, TS_FIXNUM),
	V_16 = IDX_TS(16, TS_FIXNUM),
	V_36 = IDX_TS(36, TS_FIXNUM),
	V_64 = IDX_TS(64, TS_FIXNUM),
	V_SP = IDX_TS(' ', TS_CHARACTER),
	V_NL = IDX_TS('\n', TS_CHARACTER),
	V_BACKSLASH = IDX_TS('\\', TS_CHARACTER),
	V_DQUOTE = IDX_TS('\"', TS_CHARACTER),
	V_RPAR   = IDX_TS(')', TS_CHARACTER);

#define S(ls) IDX_TS(ENUM_##ls, TS_SYMBOL)


class CPathname : public CSValue {
	typedef CPathname class_type;
public:
	CSPtr m_host,
		m_dev,
		m_dir,
		m_name,
		m_type,
		m_ver;

	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);

	bool get_LogicalP() { return m_dev == V_U; }

	void put_LogicalP(bool b) {
		if (LogicalP != b)
			m_dev = b ? V_U : 0;
	}

	DEFPROP(bool, LogicalP);

	path ToString(bool bWithoutNameExt = false);
};

struct CEnvironment : public CSValue {
	CP m_varEnv,
		m_funEnv,
		m_blockEnv,
		m_goEnv,
		m_declEnv;

	CEnvironment() {
		ZeroStruct(_self);
	}

	CEnvironment(bool) {
	}
};

class CIntFuncValue : public CSValue, public CIntFunc {
public:
	CP m_body; // at 3
	CEnvironment m_env; //at 4'th slot
	bool m_bGeneric;
	byte *m_varFlags;

#if UCFG_LISP_PROFILE
	CProfInfo m_profInfo;
#endif

	CP m_name, // at 0
		m_docstring;  // at 2

	CIntFuncValue()
		:	m_env(false)
	{
		ZeroStruct(_self);
	}

	CIntFuncValue(const CIntFuncValue& ifv);

	~CIntFuncValue() {
		delete m_varFlags;
	}

	CIntFuncValue& operator=(CIntFuncValue& ifv) {
		memcpy(this, &ifv, sizeof(CIntFuncValue));
		//!!!D #ifdef LISP_PROFILE
		//!!!D 		ifv.m_profInfo = 0;
		//!!!D #endif
		ifv.m_varFlags = 0;
		return _self;
	}

	CP GetField(size_t idx);
	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};

class CIntFuncChain {
public:
	CIntFuncValue *m_p;
	CIntFuncChain *m_prev;

	inline CIntFuncChain(CIntFuncValue *p);
	inline ~CIntFuncChain();

	CIntFuncValue *operator->() { return m_p; }
};

typedef void (__stdcall *PFNStdcall)();
typedef void (__cdecl *PFNCDecl)();

#if UCFG_LISP_FFI
#	include "ffi.h"
#endif 

CPackage *ToPackage(CP p);
CPathname * __stdcall ToPathname(CP p);
CWeakPointer *ToWeakPointer(CP p);
CArrayValue *ToObject(CP p);
CClosure *ToCClosure(CP p);
CMacro *ToMacro(CP p);
String AsString(CP p);
DWORD_PTR AsFrameTop(CP p);
double AsFloatVal(CP p);

inline CP *AsFrameTop(CP *pP) {
	return pP+(*pP >> (VALUE_SHIFT+8));
}

enum CFrameType {
	FT_EVAL = 1,    FT_DYNBIND,     FT_APPLY,
	FT_ENV1V,       FT_ENV1F,     FT_ENV1B,       FT_ENV1G,   FT_ENV1D,
	FT_VAR,			FT_FUN,

	FT_ENV2VD,      FT_ENV5,      
	FT_IBLOCK,      FT_ITAGBODY,
	FT_CBLOCK,      FT_CTAGBODY,
	FT_CATCH,       FT_UNWINDPROTECT,   FT_HANDLER,   FT_DRIVER
};

CFrameType AsFrameType(CP p);

enum CParamType {
	PT_NONE, PT_END, PT_OPTIONAL, PT_REST, PT_KEY, PT_ALLOW_OTHER_KEYS, PT_AUX, PT_BODY
};

#if UCFG_LISP_TEST //!!!
#	define LISP_FUNC __stdcall
#else
#	define LISP_FUNC __fastcall
#endif


//!!!const uint32_t FREE_FLAG = 0x80000000;

class CLispGC;

class CValueManBase {
public:
	void *m_pBase;
	void *m_pHeap;
	void *m_pBitmap;
	//!!!	CLispGC& m_gc;

	size_t m_size,
		m_defaultSize,
		m_sizeof;				 
	CTypeSpec m_ts;
	CInt<uint64_t> m_cbFreed;

	CValueManBase(size_t defaultSize, size_t szof, CTypeSpec ts);
	void Destroy();
	void Allocate(size_t size);
	void Allocate() { Allocate(m_defaultSize); }

	virtual void PrepareType(size_t count) =0;
	virtual void WriteType(BlsWriter& wr) =0;
	virtual void ReadType(const BlsReader& rd) =0;
	virtual void Delete(void *p) =0;
//!!!R	virtual void Copy(void *p, const void *q) =0;
	virtual void Move(void *p, const void *q) =0;

	void *GetCleanBitmap() {
		if (!m_pBitmap)
			m_pBitmap = new BASEWORD[m_size/BASEWORD_BITS];
		return memset(m_pBitmap, 0, m_size/8);
	}

	void FreeBitmap() {
		delete[] (BASEWORD*)exchange(m_pBitmap, nullptr);
	}

	void FillEmpty(size_t i);
	void Resize(size_t n);
	void AfterApplyCheck(bool bSimple);
	size_t ScanType(CObMap& obMap);
};

template <class T, CTypeSpec Tts>
class CValueMan : public CValueManBase {
	typedef CValueMan class_type;
public:
	CValueMan(size_t defaultSize)
		:	CValueManBase(defaultSize, sizeof(T), Tts)
	{
	}

	void Delete(void *p) override {
		m_cbFreed += m_sizeof;
		((T*)p)->~T();
	}

/*!!!R	void Copy(void *p, const void *q) override {
		new(p) T(*(const T*)q);
	}*/

	void Move(void *p, const void *q) override {
		new(p) T(std::move(*(T*)q));
		m_cbFreed += m_sizeof;
	}

	T *get_Base() { return (T*)m_pBase; }
	void put_Base(T *p) { m_pBase = p; }
	DEFPROP(T*, Base);

	T *get_Heap() { return (T*)m_pHeap; }
	DEFPROP_GET(T*, Heap);

	CP FromSValue(T *v) { return ((v-Base)<<TYPE_BITS) | Tts; }
	T *AsValue(CP p) { return Base+AsIndex(p); }

	T *CreateInstance();
	T *CreateInstance(const String::value_type *s);
	T *CreateInstance(const BigInteger& i);

	T *TryApplyCheck(size_t idx) {		//!!! rename it
		return Base+idx;
	}

	void WriteType(BlsWriter& wr) override {
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		byte *p = (byte*)m_pBase;
		for (byte *pHeap=(byte*)m_pHeap; ;) {
			byte *next = pHeap ? pHeap : (byte*)m_pBase+m_size*m_sizeof;
		
			for (; p<next; p+=m_sizeof)
				((T*)p)->Write(wr);
			p += m_sizeof;
			if (!pHeap)
				break;
			pHeap = ((byte**)next)[1];
		}
#else
		for (int i=0; i<m_size; i++) {
			T *p = Base+i;
			if (*(byte*)p != 0xFF)
				p->Write(wr);
		}
#endif
	}

	void PrepareType(size_t count) override {
		//!!!D Allocate(count*2);
		Allocate(max((count*2+BASEWORD_BITS-1)&~(BASEWORD_BITS-1), m_defaultSize)); //!!! HEAP_APP
		/*!!!
		if (count > m_size)
		Resize(count);*/
		while (count--)
			CreateInstance();
	}

	void ReadType(const BlsReader& rd) override {
		for (T *p = Base; p!=m_pHeap;)
			p++->Read(rd);
	}
};

enum CBitOp {
	BITOP_AND,
	BITOP_IOR,
	BITOP_XOR,
	BITOP_EQV,
	BITOP_NAND,
	BITOP_NOR,
	BITOP_ANDC1,
	BITOP_ANDC2,
	BITOP_ORC1,
	BITOP_ORC2,
	BITOP_NOT
};

extern const int g_arTS[32];

class CVerifier {
public:
	CObMap& m_obMap;

	CVerifier(CObMap& obMap)
		:	m_obMap(obMap)
	{
	}

	void Verify(CP p);
	void VerifySymbol(CSymbolValue *sv);
};


struct CInstance {
	CP ClassVersion;
};

struct CClass {
	CP Hashcode,
		DirectMethods,
		Classname,
		DirectSubClasses,

		DirectSuperclasses,
		AllSuperclasses,
		PrecedenceList,
		DirectSlots,
		Slots,
		SlotLocationTable,
		DirectDefaultInitargs,
		DefaultInitargs,
		Documentation,
		Listeners,
		Initialized,

		SubclassOrStablehashP,
		GenericAccessors,
		DirectAccessors,
		ValidInitargsFromSlots,
		InstanceSize,

		CurrentVersion,
		FuncallableP,
		FixedSlotLocations,
		Instantiated,
		DirectInstanceSpecializers,
		FinalizedDirectSubclasses,
		Prototype,

		Other[];
};

struct CClassVersion {
	CP	NewestClass,
		Class,
		SharedSlots,
		Serial,
		Next,
		SlotListsValidP,
		KeptSlotLocations,
		AddedSlots,
		DiscardedSlots,
		DiscardedSlotLocations;
};

struct CSlotDefinition {
	CP Name,
		Initargs,
		Type,
		Allocation,
		InheritableIniter,
		InheritebleDoc,

		Location, // from here on only for class <effective-slot-definition>
		EfmSvuc,
		EfmSsvuc,
		EfmSbuc,
		EfmSmuc;
};

struct CStructInst {
	CP Types;
	//!!!	CP Data[];
};

inline bool ConsP(CP p) { return Type(p)==TS_CONS && p; }

class CLispGC : public CLisp {
	virtual void CollectFreachable() =0;
	virtual void AddToFreachable(pair<CP, CP> pf) =0;
public:
	CLispThreadKeeper m_lispThreadKeeper;

	vector<CValueManBase*> m_arValueMan;

#	define LTYPE(ts, isnum, man)
#	define LMAN(typ, ts, man, init) CValueMan<typ, ts> man;
#	include "typedef.h"
#	undef LTYPE
#	undef LMAN


#if UCFG_LISP_SPLIT_SYM

	vector<String> m_symNames;

	String& SymNameByIdx(size_t idx) {
		if (idx >= m_symNames.size())
			m_symNames.resize(idx+1);
		return m_symNames[idx];
	}

#endif



	//!!!  CRecordAlloc m_alloc;


	
	CP *m_pStack;

	size_t m_cVal;

	union {
		CP m_r;
		CP m_arVal[MULTIPLE_VALUES_LIMIT-1];
	};

	CBool m_bOutOfMem;
	CBool m_bUnwinding;
	CSPtr m_stm;
	CSPtr m_subrSelf;

	void InitValueMans();

	typedef map<CP, CP> CMapFinalizable;
	CMapFinalizable m_mapFinalizable;


	CLispGC();
	~CLispGC();

	CSValue *ToSValue(CP p);

	inline CP FromSValue(CSymbolValue *sv) {
		if (ssize_t idx = sv-m_symbolMan.Base)
			return IDX_TS(idx, TS_SYMBOL);
		else
			return 0;
	}

	inline CP FromSValue(CConsValue *v)		{ return m_consMan.FromSValue(v); }
	
#if !UCFG_LISP_GC_USE_ONLY_BITMAP
	inline CP FromSValue(CFloat *v) {
		return m_floatMan.FromSValue(v);
	}
#endif

	inline CP FromSValue(CHashTable *v)		{ return m_hashTableMan.FromSValue(v); }
	inline CP FromSValue(CBignum *v)			{ return m_bignumMan.FromSValue(v); }
	inline CP FromSValue(CIntFuncValue *v){ return m_intFuncMan.FromSValue(v); }


	//!!!R  inline CP FromSValue(CClosure *v)			{ return m_closureMan.FromSValue(v); }
	inline CP FromSValue(CPackage *v)  		{ return m_packageMan.FromSValue(v); }
	//!!!  inline CP FromSValue(CObjectValue *v)	{ return m_objectMan.FromSValue(v); }
	inline CP FromSValue(CStreamValue *v) { return m_streamMan.FromSValue(v); }
	inline CP FromSValue(CArrayValue *v)  { return m_arrayMan.FromSValue(v); }
	inline CP FromSValue(CPathname *v)		{ return m_pathnameMan.FromSValue(v); }
	inline CP FromSValue(CReadtable *v)   { return m_readtableMan.FromSValue(v); }

	CP FromSValue(CEnvironment *env) { Throw(E_FAIL); }

	inline CConsValue *AsCons(CP p)				{ return m_consMan.AsValue(p); }
	CRatio *AsRatio(CP p)						{ return (CRatio*)AsCons(p); }
	CComplex *AsComplex(CP p)					{ return (CComplex*)AsCons(p); }
	CRandomState *AsRandomState(CP p)			{ return (CRandomState*)AsCons(p); }
	CMacro *AsMacro(CP p)				          { return (CMacro*)AsCons(p); }

	inline CSymbolValue *AsSymbol(CP p)		{ return m_symbolMan.AsValue(p); }  
	inline CReadtable *AsReadtable(CP p)		{ return m_readtableMan.AsValue(p); }  
	inline CHashTable *AsHashTable(CP p)		{ return m_hashTableMan.AsValue(p); }  
	inline CArrayValue *AsArray(CP p)			{ return m_arrayMan.AsValue(p); }
	
	inline CFloat *AsFloat(CP p) {
#if UCFG_LISP_GC_USE_ONLY_BITMAP
		return (CFloat*)m_consMan.AsValue(p);
#else
		return m_floatMan.AsValue(p); 
#endif
	}

	inline CPackage *AsPackage(CP p)				{ return m_packageMan.AsValue(p); }
	inline CIntFuncValue *AsIntFunc(CP p)	{ return m_intFuncMan.AsValue(p); }

	inline CBignum *AsBignum(CP p)					{ return m_bignumMan.AsValue(p); }
	//!!!R  inline CClosure *AsClosure(CP p)				{ return m_closureMan.AsValue(p); }
	inline CStreamValue *AsStream(CP p)		{ return m_streamMan.AsValue(p); }
	inline CPathname *AsPathname(CP p)			{ return m_pathnameMan.AsValue(p); }
	inline CWeakPointer *AsWeakPointer(CP p){ return m_weakPointerMan.AsValue(p); }

	CStreamValue *CastToPPStream(CP p) {
		if (Type(p)==TS_STREAM) {
			CStreamValue *sv = AsStream(p);
			return sv->m_subtype==STS_PPHELP_STREAM ? sv : 0;
		}
		return 0;
	}

	inline CClass& TheClass(CP p) { return *(CClass*)AsArray(p)->m_pData; }
	inline CSlotDefinition& TheSlotDefinition(CP p) { return *(CSlotDefinition*)AsArray(p)->m_pData; }
	inline CInstance& TheInstance(CP p) { return *(CInstance*)AsArray(p); } //!!! was ->m_pData
	inline CClosure& TheClosure(CP p) { return *(CClosure*)AsArray(p); } //!!! was ->m_pData
	inline CStructInst& TheStruct(CP p) { return *(CStructInst*)AsArray(p); }
	inline CClassVersion& TheClassVersion(CP p) { return *(CClassVersion*)AsArray(p)->m_pData; }

	DEBUG_NOT_INLINE void ApplyCheckArray(size_t idx);
	DEBUG_NOT_INLINE void ApplyCheckReadtable(size_t idx);
	DEBUG_NOT_INLINE void ApplyCheckPackage(size_t idx);
	DEBUG_NOT_INLINE void ApplyCheckHashTable(size_t idx);
	DEBUG_NOT_INLINE void ApplyCheckIntFunc(size_t idx);
	void __fastcall ApplyCheckSValue(CP p);

	bool inline MarkedP(CP p) {
		int tidx = g_arTS[Type(p)];
		if (tidx == -1)
			return true;
		CValueManBase *vman = m_arValueMan[tidx];
		return BitOps::BitTest(vman->m_pBitmap, AsIndex(p));
	}

	void *m_pBitmapCons;

	typedef void (__fastcall CLispGC::*PFN_ApplyCheckEx)(CP p);
	PFN_ApplyCheckEx m_mfnApplyCheckEx;

	void __fastcall ApplyCheckExImp(CP p);
	void __fastcall ApplyCheckExAndMapConsImp(CP p);

	void __fastcall ApplyCheckEx(CSymbolValue *sv);

	__forceinline void ApplyCheck(CP p) {
		if (Type(p) < TS_CHARACTER)
			(this->*m_mfnApplyCheckEx)(p);
	}

	virtual void CallFinalizer(CP fn, CP p) =0;
protected:
	vector<CP> m_arDeferred;
};

struct CStackRange {
	CP *Low,
		*High;			 
	CStackRange *Next;
};

inline CP FromBool(bool b) { return b ? V_T : 0;	}

inline intptr_t AsFixnum(CP p) { return ((intptr_t)p >> VALUE_SHIFT); } //!!!

class CVMContextBase {
public:
	CP m_closure;
	byte *m_pb;
};

class CLispEng : //!!! public CLisp,
	public CGC<CP, CLispGC>
{
	typedef CLispEng class_type;
private:
	CBool m_bVarsInited;
	set<String> Features;
	vector<String> LoadPaths;

	DateTime m_dtStart;
	CInt<uint32_t> m_nGC;	
	TimeSpan m_spanStartUser, m_spanGC;
public:
	int m_indent;

	//!!!	CSPtr m_subrEQL;
	CSPtr m_sWhitespace,
		m_sConstituent,
		m_sMacro,
		m_sInvalid,
		m_sSescape,
		m_sMescape;
	//!!!        m_sGensymCounter;

	vector<path> m_arModule;

	int m_level;
	String m_initDir;
	String m_arg;
	vector<String> m_arCommandLineArg;
	path m_outfile,
		m_destFile;
	bool m_bInit;
	bool m_bBuild,
		m_bExpandingLambda;
	CBool m_bDebugFrames;
	//!!!D	bool m_bPreservingWhitespace;

	bool m_bCompile  : 1,
		m_bLockGC   : 1,
		m_bShowEval : 1,
				  //!!!       m_bMustRPar : 1,
				  //!!!       m_bMayRPar  : 1,
		m_bRun      : 1,
		m_bTrace    : 1,
		m_bVerbose    : 1;    

#if UCFG_LISP_TAIL_REC
#	if UCFG_LISP_TAIL_REC == 1
	CBool m_bAllowTailRec;
#	else
	vector<byte> m_stackTailRec;
#	endif

	CSPtr m_tailedFun;
	CP *m_pTailStack;
	ssize_t m_nTailArgs;
	
	void TailRecApplyIntFunc(CP fun, ssize_t nArg);
#endif // UCFG_LISP_TAIL_REC


	CSPtr m_specDot;
	//!!!        m_specRPar,
	//!!!        m_specUnbound;

	CEnvironment m_env;

	CP *m_pStackBase,
		*m_pStackTop,
		*m_pStackWarning;

	ssize_t *m_pSPBase,
		*m_pSPTop,
		*m_pSP;

	observer_ptr<CJmpBufBase> m_pJmpBuf; //!!!

	int m_nRestBinds;

	observer_ptr<CIntFuncChain> m_pIntFuncChain;

	observer_ptr<CStackRange> m_pInactiveHandlers;

//!!!R	byte *m_pb;
	//!!!  CP *m_pClosure;
//!!!R	CSPtr CurClosure;
	observer_ptr<CVMContextBase> CurVMContext;

	CP get_CurClosure() { return CurVMContext->m_closure; }
	DEFPROP_GET(CP, CurClosure);

	CClosure& TheCurClosure() { return TheClosure(CurClosure); }

	ssize_t get_CurOffset() { return CurVMContext->m_pb-(byte*)AsArray(TheCurClosure().CodeVec)->m_pData; }
	DEFPROP_GET(ssize_t, CurOffset);

	typedef unordered_map<String, CSPtr> CPackageMap;
	CPackageMap m_mapPackage;

	typedef unordered_set<CP> CPackageSet;
	CPackageSet m_setPackage;

	CSPtr m_packSYS
		, m_packCL
		, m_packCLOS
		, m_packCustom
		, m_packEXT
		, m_packGray
		//!!!				m_packCompiler,
		, m_packKeyword;

	typedef unordered_map<CP, CP> CTracedFuns;
	CTracedFuns m_tracedFuns;
	int m_traceLevel;

	map<CP, String> m_mapObfuscated;

	locale m_locale;

	typedef void (LISP_FUNC CLispEng::*FPLispFunc)();
	typedef void (LISP_FUNC CLispEng::*FPLispFuncRest)(size_t nArgs);
	typedef CP (CLispEng::*FPBindVar)(size_t n, CP& sym, CP form);

	typedef size_t (CLispEng::*PFNHash)(CP p);

	struct CCharWithAttrs {
		wchar_t m_ch;
		WORD m_traits;
		bool m_bReplaceable : 1;

		CCharWithAttrs(wchar_t ch = 0, bool bReplaceable = false, WORD traits = TRAIT_INVALID)
			:	m_ch(ch)
			,	m_bReplaceable(bReplaceable)
			,	m_traits(traits)
		{}
	};

	class CTokenVec : public vector<CCharWithAttrs> {
		typedef vector<CCharWithAttrs> base;
	public:
		CTokenVec() {
			reserve(10);			//!!!Param
		}

		template <typename I>
		CTokenVec(I from, I to)
			:	base(from, to)
		{}

		CBool HasEscapes;			// to treat tokens like 5|| as symbols
	};

#if UCFG_LISP_PROFILE
	struct CLispProfiled : CProfInfo {
		CLispProfiled()
			:	CProfInfo(true)
		{}
	};

	CLispProfiled m_arSubrProfile[512];
	__forceinline CProfInfo& AsProfiled(CP p) { return m_arSubrProfile[AsIndex(p) & 0x3FF]; }
#endif


	static const FPLispFunc s_stFuncAddrs[];
	static const FPLispFuncRest s_stFuncRAddrs[];
	static const FPLispFunc s_stSOAddrs[];

	struct CLispFunc {
		const char *m_name;
		//!!!		CLispEng::FPLispFunc m_pFunc;
		byte m_nReq,
			m_nOpt,
			m_bCL;
		const byte *m_keywords;
		//!!!uint32_t m_keywords;
	};

	struct CLispFuncR {
		const char *m_name;
		//!!!		CLispEng::FPLispFuncRest m_pFunc;
		byte m_nReq,
			m_nOpt,
			m_bCL;
		byte m_bRest;
	};

	struct CLispSO {
		const char *m_name;
		//!!!		CLispEng::FPLispFunc m_pFunc;
		byte m_nReq,
			m_nOpt,
			m_bRest,
			m_bCL;
	};


	static const CLispFunc s_stFuncInfo[]; 
	static const CLispFuncR s_stFuncRInfo[]; 
	static const CLispSO s_stSOInfo[];

private:
	// Bytecodes

	void Skip_RetGF(CP closure, int n);

#define CFUN //__forceinline //!!!
#define CFUN_INLINE  __forceinline

	CFUN void C_Nil();
	CFUN void C_Nil_Push();
	CFUN void C_Push_Nil();
	CFUN void C_T();
	CFUN void C_T_Push();
	CFUN void C_Const();
	CFUN void C_Const_Push();
	CFUN CP C_LoadcHelper(CP pvec);
	CFUN void C_Load();
	CFUN void C_LoadPush();
	CFUN void C_Loadi();
	CFUN void C_Loadi_Push();
	CFUN void C_Loadc();
	CFUN void C_Loadc_Push();
	CFUN void C_Loadv();
	CFUN void C_Loadv_Push();
	CFUN void C_Loadic();
	CFUN void C_Store();
	CFUN void C_Pop_Store();
	CFUN void C_Storei();
	CFUN void C_Load_Storec();
	CFUN void C_StorecHelper(CP pvec);
	CFUN void C_Storec();
	CFUN void C_Storev();
	CFUN void C_Storeic();
	CFUN void C_GetValue();
	CFUN void C_GetValue_Push();
	CFUN void C_SetValue();
	CFUN void C_Bind();
	CFUN void C_Unbind1();
	CFUN void C_Unbind();
	CFUN void C_Progv();
	CFUN void C_Push();
	CFUN void C_Pop();
	CFUN void C_Skip();
	CFUN void C_Skipi();
	CFUN void C_SkipSP();
	CFUN void C_Skip_RetGF();
	CFUN void C_Jmp();
	CFUN void C_JmpIf();
	CFUN void C_JmpIfNot();
	CFUN void C_JmpIf1();
	CFUN void C_JmpIfNot1();
	CFUN void C_JmpIfAtom();
	CFUN void C_JmpIfConsp();
	CFUN void C_JmpIfEq();
	CFUN void C_JmpIfNotEq();
	CFUN void C_JmpIfEqTo();
	CFUN void C_JmpIfNotEqTo();
	void JmpHash(CP consts);
	CFUN void C_JmpHash();
	CFUN void C_JmpHashv();
	CFUN_INLINE void C_Jsr();
	CFUN void C_JsrPush();
	CFUN void C_JmpTail();
	CFUN void C_Venv();
	CFUN void C_MakeVector1_Push();
	CFUN void C_CopyClosure();
	CFUN void C_CopyClosure_Push();
	CFUN_INLINE void C_Call();
	CFUN_INLINE void C_Call_Push();
	CFUN void C_Call0();
	CFUN_INLINE void C_Call1();
	CFUN_INLINE void C_Call1_Push();
	CFUN_INLINE void C_Call2();
	CFUN_INLINE void C_Call2_Push();
	void Calls(size_t idx);
	CFUN void C_Calls1();
	CFUN void C_Calls1_Push();
	CFUN_INLINE void C_Calls2();
	CFUN_INLINE void C_Calls2_Push();
	CFUN void C_Callsr();
	CFUN void C_Callsr_Push();
	CFUN void C_Callc();
	CFUN void C_Callc_Push();
	CFUN void C_CallcKey();
	CFUN void C_CallcKey_Push();
	CFUN_INLINE void C_Funcall();
	CFUN_INLINE void C_Funcall_Push();
	CFUN void C_Apply();
	CFUN void C_Apply_Push();
	CFUN void C_Push_Unbound();
	CFUN void C_Unlist();
	CFUN void C_UnlistStern();
	CFUN void C_JmpIfBoundp();
	CFUN void C_Boundp();
	CFUN void C_Unbound_Nil();
	CFUN void C_Values0();
	CFUN void C_Values1();
	CFUN void C_StackToMv();
	CFUN void C_MvToStack();
	CFUN void C_NvToStack();
	CFUN void C_MvToList();
	CFUN void C_ListToMv();
	CFUN void C_MvCallp();
	CFUN void C_MvCall();
	void C_BlockOpen();
	CFUN void C_BlockClose();
	CFUN void ReturnFrom(CP cons);
	CFUN void C_ReturnFrom();
	CFUN void C_ReturnFromI();
	void C_TagbodyOpen();
	CFUN void C_TagbodyClose_Nil();
	CFUN void C_TagbodyClose();
	CFUN void Go(CP cons);
	CFUN void C_Go();
	CFUN void C_GoI();
	void C_CatchOpen();
	CFUN void C_CatchClose();
	CFUN void C_Throw();
	CFUN void C_UwpOpen();
	CFUN void C_UwpNormalExit();
	CFUN void C_UwpClose();
	CFUN void C_UwpCleanup();
	CFUN void C_HandlerOpen();
	CFUN void C_HandlerBegin_Push();
	CFUN void C_Not();
	CFUN void C_Eq();
	CFUN void C_Car();
	CFUN void C_Car_Push();
	CFUN void C_Load_Car_Push();
	CFUN void C_Load_Car_Store();
	CFUN void C_Cdr();
	CFUN void C_Cdr_Push();
	CFUN void C_Load_Cdr_Push();
	CFUN void C_Load_Cdr_Store();
	CFUN void C_Cons();
	CFUN void C_Cons_Push();
	CFUN void C_Load_Cons_Store();
	CFUN void C_SymbolFunction();
	CFUN void C_Const_SymbolFunction();
	CFUN void C_Const_SymbolFunction_Push();
	CFUN void C_ConstSymbolFunctionStore();
	CFUN void C_SvRef();
	CFUN void C_SvSet();
	CFUN void C_List();
	CFUN void C_List_Push();
	CFUN void C_ListStern();
	CFUN void C_ListStern_Push();
	CFUN void C_Nil_Store();
	CFUN void C_T_Store();
	CFUN void C_Calls1_Store();
	CFUN void C_Calls2_Store();
	CFUN void C_Callsr_Store();
	CFUN void C_Load_Inc_Push();
	CFUN void C_Load_Inc_Store();
	CFUN void C_Load_Dec_Push();
	CFUN void C_Load_Dec_Store();
	CFUN void C_Call1_JmpIf();
	CFUN void C_Call1_JmpIfNot();
	CFUN void C_Call2_JmpIf();
	CFUN void C_Call2_JmpIfNot();
	CFUN void C_Calls1_JmpIf();
	CFUN void C_Calls1_JmpIfNot();
	CFUN void C_Calls2_JmpIf();
	CFUN void C_Calls2_JmpIfNot();
	CFUN void C_Callsr_JmpIf();
	CFUN void C_Callsr_JmpIfNot();
	CFUN void C_Load_JmpIf();
	CFUN void C_Load_JmpIfNot();
	CFUN void C_Apply_Skip_Ret();
	CFUN void C_Funcall_Skip_RetGF();
	CFUN void C_Load(int n);
	CFUN void C_Load_Push(int n);
	CFUN void C_Const(int n);
	CFUN_INLINE void C_Const_Push(int n);
	CFUN void C_Store(int n);

	CP GetSymProp(CP sym, CP name);
	CP Get(CP sym, CP name);
	void Setq(CP name, CP val);

#ifdef C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR
	CP BackquoteEx(CP p, bool& bAppend, int level);
#else
	CP BqTransform(CP form);
	CP BqExpandList(CP form);
	CP BqExpand(CP form);  
#endif


	//!!!D CSPtr ReadSymbol(CTextStream& is);
	CCharType GetCharType(wchar_t ch, CReadtable *rt = 0);
	CReadtable *GetReadtable();
	CTokenVec ReadExtendedToken(CP stm, bool bSescape = false, bool bPreservingWhitespace = false);
	bool OnlyDotsOrEmpty(CTokenVec& vec);
	bool PotentialNumberP(CTokenVec& vec, CP pbase);
	CP ReadToken(CP stm, bool bPreservingWhitespace);
	CSPtr ReadHexAsmList(CP stm);
	CSPtr ReadClosure(CP stm, int n);
	//!!!D  String ReadWord(CTextStream& is);

	bool EvalFeature(CP p);
	//!!!D  CSPtr ReadPlus(CP stm);
	//!!!D  CSPtr ReadMinus(CP stm);
	//!!!  CSPtr ReadStruct(CFileITextStream& is);
	//!!!  CP ReadNumber(CTextStream& is, int radix);
	//!!!  CP ReadString(CTextStream& is);
	//!!!  CPtr ReadComplex(CFileITextStream& is);
	int ToDigit(wchar_t ch);
	CP ToInteger(const CTokenVec& vec, int readBase);
	String TokenToString(CTokenVec& vec);
	CP ToFloat(CTokenVec& vec);
	CP ToNumber(CTokenVec& vec);
	//!!!  void CheckTypeSpec(CP p, CTypeSpec ts);
	bool CheckType(CP typChain, CP formTyp);
	void TypepCheck(CP datum, CP typ);
	CP CheckInteger(CP p);
	void Debug();
	//!!!CP ReadList(CP stm);

	vector<String> ReadBinHeader(const BinaryReader& rd);
	void LoadMem(Stream& bstm);
	void SaveMem(Stream& bstm);
	//!!!  void SaveNewMem();

#if UCFG_LISP_FFI
	CSPtr m_packCffiSys;
	unordered_map<void*, CP> m_funPtr2Symbol;


	typedef unordered_map<String, ptr<CForeignLibrary> > CName2lib;   //!!!shold be map<CP, ...>
	CName2lib m_name2lib;

	typedef pair<String, HANDLE> CStringHandle;
	typedef unordered_map<CStringHandle, CP> CForeignFunction2ptr;
	CForeignFunction2ptr m_foreignFunction2ptr;

/*!!!R	inline CP FromSValue(CForeignLib *v) { return m_foreignLib.FromSValue(v); }
	inline CForeignLib *AsForeignLib(CP p)	{ return m_foreignLibMan.AsValue(p); } */

	CP CreatePointer(void *ptr);

	inline void *ToPointer(CP p) {
		if (Type(p) == TS_FF_PTR && p)
			return ((CForeignPointer*)(m_consMan.Base+AsIndex(p)))->m_ptr;
		E_TypeErr(p, S(L_FOREIGN_POINTER));
	}

	int ToForeignInt(CP arg);
	void WriteArg(byte *&pArgs, vector<ptr<ForeignArg> >& fargs, CP typ, CP arg);
	CForeignLibrary& GetForeignLibrary(HMODULE h);

	void *OnCallback(CP name, void *pArgs);
	static void * __fastcall StaticCallbackStub(CP name, void *pArgs);
	void *CreateCallback(CP name, CP rettype, CP sig, CP conv);

	void PrPointer(CP p);

	void SaveFFI(BlsWriter& wr);
	void LoadFFI(const BlsReader& rd);
#endif // UCFG_LISP_FFI

public:
	// API
	void Init(bool bBuild);
	void SetSignal(bool bSignal);
	void Load(Stream& stm);
	void LoadImage(RCString filename);
	int Run();
	void Compile(RCString name);
	void LoadFile(RCString fileName);
	bool IsProperList(CP p);
	void SaveImage(Stream& stm);
	String Eval(RCString sForm);
	CP VGetSymbol(RCString name, RCString pack);
	void ParseArgs(char **argv);

	void VCall(CP p);
#if UCFG_OLE

	COleVariant ToOleVariant(CP p);
	CP FromOleVariant(const VARIANT& v);
	COleVariant VCall(CP p, const vector<COleVariant>& params);
	void put_Stream(CEnumStream idx, IStandardStream *iStream);
#endif
	void put_Stream(CEnumStream idx, ptr<StandardStream> p);
	CP CreateStandardStream(CEnumStream n);

	enum on_error_t {
		ON_ERROR_DEFAULT,
		ON_ERROR_DEBUG,
		ON_ERROR_ABORT,
		ON_ERROR_APPEASE,
		ON_ERROR_EXIT
	};
	void InstallGlobalHandlers(on_error_t onErr);

	void ProcessCommandLineImp(int argc, char *argv[]);
	void ProcessCommandLine(int argc, char *argv[]);

	CBool m_bInited,
		m_bLoadCompiling;



	void __fastcall ContinueBytecode();
	void InterpretBytecode(CP closure, ssize_t offset);		// to save stack  // _forceinline

	void InitStreams();

	void Push(CP a) { *--m_pStack = a; }

	void Push(CP a, CP b) { m_pStack -= 2;  m_pStack[1] = a; *m_pStack = b; }
	void Push(CP a, CP b, CP c) { m_pStack -= 3;  m_pStack[2] = a; m_pStack[1] = b; *m_pStack = c; }

	void Push(CP a, CP b, CP c, CP d) { Push(a, b); Push(c, d); }
	void Push(CP a, CP b, CP c, CP d, CP e) { Push(a, b, c); Push(d, e); }
	void Push(CP a, CP b, CP c, CP d, CP e, CP f) { Push(a, b, c); Push(d, e, f); }

	void FillMem(CP *p, size_t count, CP v) {
		while (count--)
			p[count] = v;
	}

	void PushFill(size_t count, CP v) { FillMem(m_pStack-=count, count, v); }
	void PushUnbounds(size_t count) { FillMem(m_pStack-=count, count, V_U); }

	CP Pop()                   { return *m_pStack++; }
	CP GetStack(ssize_t n)         { return m_pStack[n]; }
	void SetStack(ssize_t n, CP p) { m_pStack[n] = p; }
	void SkipStack(ssize_t n)      { m_pStack += n; }

#define SV m_pStack[0]
#define SV1 m_pStack[1]
#define SV2 m_pStack[2]
#define SV3 m_pStack[3]
#define SV4 m_pStack[4]
#define SV5 m_pStack[5]
#define SV6 m_pStack[6]
#define SV7 m_pStack[7]
#define SV8 m_pStack[8]
#define SV9 m_pStack[9]
#define SV10 m_pStack[10]

#define V_SESCAPE m_sSescape
#define V_MESCAPE m_sMescape
#define V_CONSTITUENT m_sConstituent

	CP ToOptional(CP p, CP def) { return p==V_U ? def : p; }
	CP ToOptionalNIL(CP p) { return ToOptional(p, 0); }

	CP __fastcall FromSValueT(CSValue *pv, CTypeSpec ts);

	CP FromSValueEx(CSValueEx *pv) { return FromSValueT(pv, CTypeSpec(pv->m_type)); }

	CP FromSyntaxType(CSyntaxType st);

	
	CP GetCond(CP sym);
	DECLSPEC_NORETURN void Err(int idx);
	void E_Signal(CP sym);
	DECLSPEC_NORETURN void E_Error(CP sym);
	static DECLSPEC_NORETURN void E_Error() { Lisp::E_Error(); }
	DECLSPEC_NORETURN void E_SignalErr(CP typ, int errCode, int nArg);
	DECLSPEC_NORETURN void E_CellErr(CP name);
	DECLSPEC_NORETURN void E_UndefinedFunction(CP name);
	DECLSPEC_NORETURN void E_PackageErr(CP pack);
	DECLSPEC_NORETURN void E_PackageErr(CP pack, int errCode, CP a, CP b = V_U);
	DECLSPEC_NORETURN void E_TypeErr(CP datum, CP typ, int errCode = IDS_E_IsNot, CP a = V_U, CP b = V_U);
	DECLSPEC_NORETURN void E_RangeErr(CP idx, CP bound);
	DECLSPEC_NORETURN void E_StreamErr(CP stm);
	DECLSPEC_NORETURN void E_ParseErr();
	DECLSPEC_NORETURN void E_DivisionByZero(CP n, CP d);
	DECLSPEC_NORETURN void E_EndOfFileErr(CP stm);
	DECLSPEC_NORETURN void E_FileErr(CP path);
	DECLSPEC_NORETURN void E_FileErr(HRESULT hr, RCString message, CP pathname);
	DECLSPEC_NORETURN void E_ReaderErr();
	DECLSPEC_NORETURN void E_ProgramErr();	
	DECLSPEC_NORETURN void E_ProgramErr(int errCode, CP a, CP b = V_U);
	DECLSPEC_NORETURN void E_ControlErr(int errCode, CP a, CP b = V_U);
	DECLSPEC_NORETURN void E_UnboundVariableErr(CP name, int errCode, CP a, CP b = V_U);
	DECLSPEC_NORETURN void E_PathnameErr(CP datum);
	DECLSPEC_NORETURN void E_StreamErr(int errCode, CP stm, CP arg);
	DECLSPEC_NORETURN void E_ArithmeticErr(int errCode, CP a = V_U);
	DECLSPEC_NORETURN void E_Err(int errCode, CP a = V_U);
	DECLSPEC_NORETURN void E_SeriousCondition(int errCode = IDS_E_SeriousCondition, CP a = V_U, CP b = V_U);
	

	void CerrorOverflow();
	void CerrorUnderflow();

	void PrepareCond(int errCode, CP type);
	CP CorrectableError(int nArg);


	__forceinline CConsValue *ToCons(CP p) {
		if (Type(p) == TS_CONS && p)
			return m_consMan.Base+AsIndex(p);
		E_TypeErr(p, S(L_CONS));
	}

	CArrayValue* __fastcall ToArray(CP p);
	CStructInst* __fastcall ToStruct(CP p);


	CIntFuncValue *ToIntFunc(CP p);

	__forceinline CP Car(CP p) {
		if (Type(p) == TS_CONS)
			return AsCons(p)->m_car;
		E_TypeErr(p, S(L_LIST));
	}

	__forceinline CP Cdr(CP p) {
		if (Type(p) == TS_CONS)
			return AsCons(p)->m_cdr;
		E_TypeErr(p, S(L_LIST));
	}

	void Inc(CP& p) {
		p = Cdr(p);
	}

	void Inc(CSPtr& p) {
		p = Cdr(p);
	}

	__forceinline bool SplitPair(CP& p, CP& car) {		//!!!
		if (Type(p) == TS_CONS) {
			if (p) {
				CConsValue *pv = AsCons(p);
				car = pv->m_car;
				p = pv->m_cdr;
				return true;
			}
			return false;
		}
		E_TypeErr(p, S(L_LIST));
	}

	__forceinline bool SplitPair(CSPtr& p, CSPtr& car) {
		if (Type(p) == TS_CONS) {
			if (p) {
				CConsValue *pv = AsCons(p);
				car = pv->m_car;
				p = pv->m_cdr;
				return true;
			}
			return false;
		}
		E_TypeErr(p, S(L_LIST));
		/*!!!    if (p)
		{
		CConsValue *pv = ToCons(p);
		car = pv->m_car;
		p = pv->m_cdr;
		return true;
		}
		return false;*/
	}

	__forceinline CSymbolValue *ToSymbol(CP p) {
		if (Type(p) == TS_SYMBOL || !p)
			return AsSymbol(p);
		E_TypeErr(p, S(L_SYMBOL));
	}

	__forceinline CP *ToFrame(CP p) {
		if (Type(p) == TS_FRAME_PTR)
			return m_pStackTop-AsIndex(p);
		E_TypeErr(p, 0); //!!!
	}

	__forceinline CP *AsFrame(CP p) {
		return m_pStackTop-AsIndex(p);
	}

	static __forceinline bool IsFrameNested(CP p) {
		return AsIndex(p) & FLAG_NESTED;
	}

	bool IsFunc(CP p) {
		switch (Type(p)) {
		case TS_CCLOSURE:
		case TS_INTFUNC:
		case TS_SUBR:
			return true;
		default:
			return false;
		}
	}

	bool VectorP(CP p);
	bool StringP(CP p);
	
	bool PosFixNumP(CP p) {
		return Type(p)==TS_FIXNUM && AsFixnum(p)>=0;
	}

	CP Const(int idx) {
		return AsArray(CurClosure)->GetElement(idx);
	}

	DECLSPEC_NORETURN void Error(HRESULT hr, CP p1);
	//!!!D  DECLSPEC_NORETURN void E_TypeError(HRESULT hr, CP p);

	void StackOverflow();
	void ErrorCode(HRESULT hr, RCString s);

	void Verify(CObMap& obMap);

	String AsTrueString(CP p);
	String FromStringDesignator(CP p);

	CArrayValue *ToVector(CP p);
	CReadtable *ToReadtable(CP p);
	CHashTable *ToHashTable(CP p);
	CStreamValue *ToStream(CP p);

#if UCFG_LISP_BUILTIN_RANDOM_STATE
	CRandomState *ToRandomState(CP p);
#endif

	CSymbolMacro *ToSymbolMacro(CP p);
	CSymbolMacro *ToGlobalSymbolMacro(CP p);
	CFunctionMacro *ToFunctionMacro(CP p);
	//!!!D  CLispSubr* _fastcall ToSpecialOperator(CP p);
	//!!!R  CStreamValue *Geteam(CP p, FPLispFunc pfn);


	FPLispFunc AsSpecialOperator(CP p) {
		return s_stSOAddrs[AsIndex(p) & 0x3FF];
	}

	CComplex *ToComplex(const char *s);

	struct CReqOptRest {
		byte m_nReq,
			m_nOpt;
	};

	__forceinline CReqOptRest AsReqOptRest(CP p) {
		CReqOptRest r;
		r.m_nReq = byte((p>>(TYPE_BITS+10)) & 7);
		r.m_nOpt = byte((p>>(TYPE_BITS+13)) & 7);
		return r;
	}

	__forceinline bool WithRestP(CP p) { return p & (1 << (TYPE_BITS+16)); }
	__forceinline bool WithKeywordsP(CP p) { return p & (1 << (TYPE_BITS+17)); }

	int ReadU();
	int ReadS();
	void IgnoreS();
	byte *ReadL();
	size_t ReadOffset();
	CP& ReadKKN();
	ssize_t GetSP(int n) { return m_pSP[n]; }
	CP *GetFrame(int k) { return m_pStackTop-GetSP(k); }	
#ifdef X_DEBUG//!!!D
	void SkipSP(int n);
	CP *PopSPFrame();
	void PushSP(ssize_t v);
#else
	void SkipSP(int n) { m_pSP += n; }
	CP *PopSPFrame() { return m_pStackTop-*m_pSP++; }
	void PushSP(ssize_t v) { *--m_pSP = v; }
#endif
	void PushSPFrame(CP *p) { PushSP(m_pStackTop-p); }
	void PushSPStack() { PushSPFrame(m_pStack); }
	void JmpIf(bool b);

	void IgnoreL() {IgnoreS();}
	byte ReadB() {return *CurVMContext->m_pb++;}
	CSPtr IncNumber(CP p);
	CSPtr DecNumber(CP p);

//!!!R	size_t HashString(RCString s);
	size_t SxHash(CP p);
	bool DimsEqual(CP p, CP q);
	void BitUp(CBitOp bitOp);

	CLispEng();
	~CLispEng();
	void CheckGarbageCollection();
	void ClearBitmaps();

	void CollectDeferred();

	void CollectEx();
	void Collect();
	void FreeReservedBuffers();
	uint64_t GetFreedBytes();

	void CommonInit();
	void SetPathVars();
	void InitVars();
	void SetVars();
	static CP FindSubr(FPLispFunc pfn, byte nReq, byte nOpt, byte bRest);

	struct CCharDispatcher {
		char m_ch;
		FPLispFunc m_fn;
	};

	static DateTime s_dtSince1900;

	static const CCharDispatcher s_arMacroChar[],
		s_arSharpDispatcher[];

	void InitReader();
	virtual void OnInit() {}
	void Clear();

	void CheckStack() {
		if (m_pStack < m_pStackWarning)
			StackOverflow();
	}

#if UCFG_LISP_DEBUG
	CInt<int> m_printIndent;

	class CIndentKeeper {
		int m_prev;
	public:
		CIndentKeeper() {
			m_prev = Lisp().m_printIndent++;
		}

		~CIndentKeeper() {
			Lisp().m_printIndent = m_prev;
		}
	};
#endif


	void PrintList(ostream& os, CP form);
	void Print(ostream& os, CP form);
	void Print(CP form) { Print(cerr, form); }
	void ReadPrintLabel(ostream& os);
	void Disassemble(ostream& os, CP p);

#define LFUN(n, a, r, o, p) void LISP_FUNC a();
#define LFUN_REPEAT(n, a, r, o, p)
#define LFUN_K(n, a, r, o, p, k)  void LISP_FUNC a();
#define LFUN_END

#define LFUN_BEGIN_R
#define LFUNR(n, a, r, o, p)  void LISP_FUNC a(size_t nArgs);
#define LFUN_END_R

#define LFUN_BEGIN_SO
#define LSO(n, a, r, o, rest, p)  void LISP_FUNC a();
#define LFUN_END_SO

#include "fundef.h"


	void PrintFormHelper(CP stm, CP form);
	void PrintOneBind(const char *envName, CP *frame);
	void PrintString(CP stm, RCString s);

	// List Utils
	int Length(CP p);
	CP& PlistFind(CP& p, CP k);

	bool __fastcall Memq(CP x, CP list);
	CP Adjoin(CP x, CP list);
	CConsValue *Assoc(CP item, CP alist);
	CP __fastcall Listof(ssize_t n);
	CP __fastcall ListofEx(ssize_t n);
	CP List(CP a0);
	CP List(CP a0, CP a1);
	CP List(CP a0, CP a1, CP a2);
	CP List(CP a0, CP a1, CP a2, CP a3);
	CP List(CP a0, CP a1, CP a2, CP a3, CP a4);
	CP ListEx(CP a0, CP a1, CP a2);

	//!!!R	void Putf(CSPtr& plist, CP p, CP v);
	void Remf(CSPtr& plist, CP p);
	CP Remq(CP x, CP list);
	void DeleteFromList(CP x, CSPtr& list);
	CP CopyList(CP p);
	CP __fastcall NReverse(CP p);
	CP __fastcall AppendEx(CP p, CP q);

	void MvToStack();
	void StackToMv(size_t n);
	void MvToList();
	void ListToMv(CP p);

	//!!!	bool IsMacro(CSymbolValue *sv);
	
	enum EStringCase {
		CASE_ALL_UPPER,
		CASE_ALL_LOWER,
		CASE_ALL_MIXED
	};

	EStringCase StringCase(RCString s);
	String CasedString(RCString s, CP cas);
	CP CasedPathnameComponent(CP c, CP cas, CP defaultComp);

	void PrintFrames();
	//!!!  void Apply(CP f, CP a0, CP a1);
	CP __fastcall NestFun(CP env);
	CP __fastcall NestVar(CP env);
	CEnvironment *NestEnv(CEnvironment& env5);
	void CheckAllowOtherKeys(CP *pBase, size_t nArg);
	void __fastcall ApplySubr(CP fun, ssize_t nArg);

	void ApplyIntFunc(CP fun, ssize_t nArg);

	void __fastcall ApplyHooked(CP fun, ssize_t nArg);
	void __fastcall ApplyTraced(CP fun, ssize_t nArg);

	CInt<void*> StackOverflowAddress;
#if UCFG_LISP_FAST_HOOKS
	void __fastcall ApplyImp(CP fun, ssize_t nArg);

	typedef void (__fastcall CLispEng::*PFN_Apply)(CP fun, ssize_t nArg);
	PFN_Apply m_mfnApply;

	__forceinline void Apply(CP fun, ssize_t nArg) {
		(this->*m_mfnApply)(fun, nArg);
	}

	void put_Signal(int sig) {
		CLisp::put_Signal(sig);					// order of operators important
		m_mfnApply = &CLispEng::ApplyHooked;
	}

	CP __fastcall EvalImp(CP p);
	CP __fastcall EvalHooked(CP p);
	CP (__fastcall class_type::* m_mfnEval)(CP p);

	__forceinline CP EvalCall(CP p) {
		return (this->*m_mfnEval)(p);
	}

#else
	void __fastcall Apply(CP fun, ssize_t nArg);
	CP __fastcall EvalCall(CP p);
#endif

//!!!O	CP& get_Special(CLispSymbol ls) { return m_symbolMan.get_Base()[ls].m_dynValue; } //!!!Q may be need to check Unbound?
#define get_Special(ls) m_symbolMan.get_Base()[ls].m_dynValue				//!!!O macro to optimize assembly code in VC

#define Spec(ls) get_Special(ENUM_##ls)

	static inline bool IsSelfEvaluated(CP p) {
#ifdef _MSC_VER
#	ifdef _WIN64
		return uint64_t(_rotr64(p, VALUE_SHIFT)-1) >= 0x01FFFFFFFFFFFFFFUL;
#	else
		return uint32_t(_rotr(p, VALUE_SHIFT)-1) >= 0x01FFFFFF;
#	endif
#else
		return Type(p)>TS_SYMBOL || !p;
#endif
	}

#if UCFG_LISP_FAST_EVAL_ATOMS == 2
	CP m_maskEvalHook;

	inline bool IsSelfEvaluatedAndNotHooked(CP p) {
#ifdef _MSC_VER
#	ifdef _WIN64
		return uint64_t(_rotr64(p, VALUE_SHIFT)-1) > m_maskEvalHook;
#	else
		return uint32_t(_rotr(p, VALUE_SHIFT)-1) > m_maskEvalHook;
#	endif
#else
		return CP(((p>>VALUE_SHIFT)|(p<<(sizeof(CP)*8-VALUE_SHIFT)))-1) > m_maskEvalHook;
#endif
	}
#endif

	__forceinline CP Eval(CP p) {
#if UCFG_LISP_FAST_EVAL_ATOMS == 2
		if (IsSelfEvaluatedAndNotHooked(p)) {
			return p;
		}
#endif
		return EvalCall(p);
	}

	void ApplyClosure(CP closure, ssize_t nArgs, CP args = 0);


	//!!!void Funcall(CP p, int nArgs);
	void Funcall(CP fun, ssize_t nArgs) { Apply(FromFunctionDesignator(fun), nArgs); }
	void Funcall(RCString fname, ssize_t nArgs) { Funcall(GetSymbol(fname, m_packCL), nArgs); }

	void Apply(CP form, ssize_t nArg, CP rest);

	void Call(RCString fname, CP a)								{ Push(a); 				Funcall(fname, 1); }
	void Call(RCString fname, CP a, CP b)						{ Push(a, b); 			Funcall(fname, 2); }
	void Call(RCString fname, CP a, CP b, CP c)					{ Push(a, b, c); 		Funcall(fname, 3); }
	void Call(RCString fname, CP a, CP b, CP c, CP d)			{ Push(a, b, c, d); 	Funcall(fname, 4); }
	void Call(RCString fname, CP a, CP b, CP c, CP d, CP e)		{ Push(a, b, c, d, e); 	Funcall(fname, 5); }
	void Call(CP p) { Apply(p, 0, 0); }
	void Call(CP p, CP a);
	void Call(CP p, CP a, CP b);
	void Call(CP p, CP a, CP b, CP c);
	void Call(CP p, CP a, CP b, CP c, CP d);
	void Call(CP p, CP a, CP b, CP c, CP d, CP e);

	//!!!void FuncallSubr(CP fun, int nArgs);
	bool FindKeywordValue(CP key, ssize_t nArgs, CP *pRest, CP& val);
	CP ParseDD(CP body, bool bAllowDoc);
	bool GetParamType(CP& p, CP& car, CParamType& pt);
	CP LambdabodySource(CP lb);
	CP FunnameBlockname(CP p);
	void AddImplicitBlock();
	CP GetClosure(CP lambdaBody, CP name, bool bBlock, CEnvironment& env);
	void __fastcall MatchClosureKey(CP closure, ssize_t nArgs, CP *pKeys, CP *pRest);
	//!!!  void EvalClosure(CP closure);
	//!!!  void __fastcall FuncallClosure(CP closure, int nArgs);
	bool __fastcall FunnameP(CP p);
	bool SymMacroP(CP p);

	void CallFinalizer(CP fn, CP p) { Call(fn, p); }

	void Eval5Env(CP form, const CEnvironment& env);
	CP __fastcall Cons(CP a, CP b);
	CP BignumFrom(int64_t n);
	
	CP CreateInteger(intptr_t n) {
		return (n < FIXNUM_LIMIT && n >= -FIXNUM_LIMIT) ? CreateFixnum(n) : BignumFrom(n);
	}

	CP CreateInteger64(int64_t n);
	CP CreateIntegerU64(uint64_t n);
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	CRandomState *CreateRandomState(CP seed);
#endif

	template <class T, class U>
	CP TimeSpanToInternalTimeUnits(const duration<T, U>& span) {
		return CreateInteger64(duration_cast<UCFG_LISP_INTERNAL_TIME_UNITS_PER_SECOND>(span).count());
	}

	CP CpFromRandomState(CP rs);
	void FromRandomState(CP rs, default_random_engine& rngeng);
	void ModifyRandomState(CP rs, CP p);
	CP SerializedRandomeEngine(const default_random_engine& rneng);
	void ModifyRandomState(CP rs, const default_random_engine& rneng);


	CP CreateMacro(CP expander, CP lambdaList);
	CSymbolMacro *CreateSymbolMacro(CP p);
	CP CreateReadLabel(CP lab);
	//!!!  CP CreateSpecial(char c);
	CFloat *CreateFloat(double d);
	CP FromFloat(double d) { return FromSValueT(CreateFloat(d), TS_FLONUM); }
	CBignum *CreateBignum(const BigInteger& i);

	CConsValue *CreateCons() {
		void *pHeap = m_consMan.m_pHeap;
		if (!pHeap) {
			Collect();
			pHeap = m_consMan.m_pHeap;
		}
		m_consMan.m_pHeap = ((CConsValue**)pHeap)[1];
		return (CConsValue*)pHeap;
	}

	CP CreateRatio(CP num, CP den);
	CP CreateComplex(CP real, CP imag);

	CSymbolValue *CreateSymbol(const String::value_type *s) {
		CSymbolValue *sv = m_symbolMan.CreateInstance(s);
		sv->m_dynValue = V_U;
		return sv;
	}

	CP CreateKeyword();
	CClosure *CreateClosure(size_t len);
	CClosure *CopyClosure(CP oldClos);

	__forceinline CP CreateFrameInfo(CFrameType frameType, CP *pTop) {
		return FromValue(frameType|((pTop-m_pStack+1)<<8), TS_FRAMEINFO);
	}

	__forceinline CP CreateFramePtr(CP *pStack) {
		return FromValue(m_pStackTop-pStack, TS_FRAME_PTR);
	}

	void AdjustSymbols(ssize_t offset);
	CStreamValue *CreateStream(int subtype = STS_STREAM, CP in = 0, CP out = 0, byte flags = 0);
	CP CreateSynonymStream(CP p);
	CP CreatePathname(const path& p);
	CPathname *CopyPathname(CP p);
	CWeakPointer *CreateWeakPointer(CP p);
	CP CreateString(RCString s);
	//!!!D  CP CreateOutputStringStream();
	//!!!D  CObjectValue  *CreateObject();

	static CP CreateSpecialOperator(uint32_t n, uint32_t nReq, uint32_t nOpt, uint32_t bRest) {
		return TS_SPECIALOPERATOR | (n<<TYPE_BITS) | (nReq<<(TYPE_BITS+10)) | (nOpt<<(TYPE_BITS+13)) | (bRest<<(TYPE_BITS+16));
	}

	static CP CreateSpecialOperator(uint32_t n);

	static CP CreateSubr(uint32_t n, uint32_t nReq, uint32_t nOpt, uint32_t bRest, const byte *keywords = 0) {
		return TS_SUBR | (n<<TYPE_BITS) | (nReq<<(TYPE_BITS+10)) | (nOpt<<(TYPE_BITS+13)) | (bRest<<(TYPE_BITS+16)) | (bool(keywords)<<(TYPE_BITS+17));
	}

	static CP CreateSubr(uint32_t n);

	CIntFuncValue *CreateIntFunc() { return m_intFuncMan.CreateInstance(); }
	CPackage *CreatePackage(RCString name, const vector<String>& nicks);
	CArrayValue *CreateVector(size_t size, byte elType = ELTYPE_T);
	CP CopyVector(CP p);
	CArrayValue *CreateArray();
	CReadtable *CreateReadtable();
	CHashTable *CreateHashTable();

	bool __fastcall Eql(CP p, CP q);
	bool EqualP(CP x, CP y);
	bool Equal(CP p, CP q);
	CSPtr DestructiveAppend(CP p, CP q);

	CP * __fastcall FindVarBind(CP sym, CP env);

	CP * __fastcall SymValue(CSymbolValue *sym, CP env, CP *pSymMacro);

	void __fastcall SetSymValue(CP psym, CP env, CP *pSymMacro, CP v) {
		CSymbolValue *sym = ToVariableSymbol(psym);
		if (CP *p = SymValue(sym, env, pSymMacro)) {
#if UCFG_LISP_FAST_HOOKS
			if (p == &sym->m_dynValue)
				CheckBeforeSetSymVal(psym, v);
#endif
			*p = v;
		}
	}

	static CP get_Sym(CLispSymbol ls) { return IDX_TS(ls, TS_SYMBOL); }
	//!!!	__declspec(property(get=get_Sym)) CP Syms[];


	void SetSpecial(CP sym, CP val);
	void SetConstant(CP sym, CP val);

	CP __fastcall GetSymFunction(CP sym, CP fenv);
	CSPtr FindPair(CP slots, CP name);
	void RemoveSlot(CSPtr& p, CP slot);
	void ReplaceA(CP p, CP q);
	CP ReadRec(CP stm);

	__forceinline CP GetSymbol(const CLString& s, CP pack) {
		return AsPackage(pack)->GetSymbol(_self, s);
	}

	CP GetKeyword(const CLString& s) { return GetSymbol(s, m_packKeyword); }

	path FindInLISPINC(const path& filename, const path& defaultDir);
	path SearchFile(const path& name);
	void Load(const path& fileName, bool bBuild);
	void F_GetFilePosition(CP args);
	void F_MakePphelpStream();

	//!!!D  void Load();
	//!!!D  void Load(CP sym);
	void LoadFromStream(CP stm);
	void Loop();
	size_t SizeOf(CP p, int level = 20); //!!!
	void ShowInfo();
	void CallBindedFormsEx(FPBindVar pfn, CP forms, CP *pBind, int nBinds);
	void CallBindedForms(FPBindVar pfn, CP caller, CP varSpecs, CP declars, CP forms);
	CP AugmentDeclEnv(CP declSpec, CP env);
	void PushNestEnvAsArray(CEnvironment& env);
	void PushNestEnv(CEnvironment& penv);
	bool ParseCompileEvalForm(size_t skip);
	void GeneralLet(FPBindVar pfn);

	typedef unordered_set<CP> CObjectSet;
	void MapSym(CP fun, CObjectSet& syms);

	void CopyArray(CArrayValue *avSrc, CArrayValue *avDst, CP subs, int nArgs = 0);

	void UpdateInstance(CP p);

	__forceinline bool FuncallableInstanceP(CP p) {
		return Type(p)==TS_CCLOSURE && (AsArray(p)->m_flags & FLAG_CLOSURE_INSTANCE);
	}

	bool InstanceP(CP p);
	bool InstanceOf(CP p, CP c);
	bool ClosureInstanceP(CP p) { return FuncallableInstanceP(p); }

	bool InstanceValidP(CP p) { return !TheClassVersion(TheInstance(p).ClassVersion).Next; }

	void InstanceUpdate(CP p) {
		if (!InstanceValidP(p))
			UpdateInstance(p);
	}

	bool SomeClassP(CP p, CLispSymbol ls);

	bool DefinedClassP(CP p);

	void ProgNoRec() {
		for (CP car; SplitPair(SV, car);) {
			m_cVal = 1;
			m_r = Eval(car);
		}
		SkipStack(1);
	}

	void __fastcall Prog();

	inline void __fastcall Progn(CP p) {		// inline to save stack space
		if (p) {
			Push(p);
			Prog();
		} else
			ClearResult();
	}

	void __fastcall PrognNoRec(CP p);


	void UnwindTo(CP *frame);
	void ThrowTo(CP tag);
	void Reset(size_t count);

	void ClearResult() {
		m_cVal = 1;
		m_r = 0;
	}

	// Arithmetic functions
	BigInteger ToBigInteger(CP p);
	CP FromCInteger(const BigInteger& v);

	CSymbolValue *ToVariableSymbol(CP p) {
		CSymbolValue *sv = ToSymbol(p);
		if (!(~sv->m_fun & (SYMBOL_FLAG_CONSTANT|SYMBOL_FLAG_SPECIAL)))
			E_Error(S(L_SOURCE_PROGRAM_ERROR));
		return sv;
	}

#if UCFG_LISP_FAST_HOOKS
	 void CheckBeforeSetSymVal(CP sym, CP v);

	void SetSymVal(CP sym, CP v) {
		if (sym==S(L_S_EVALHOOK) || sym==S(L_S_APPLYHOOK))
			CheckBeforeSetSymVal(sym, v);
		AsSymbol(sym)->m_dynValue = v;
	}
#else
	__forceinline void SetSymVal(CP sym, CP v) {
		AsSymbol(sym)->m_dynValue = v;
	}
#endif
	CP SwapIfDynSymVal(CP symWithFlags, CP v);
	
	static __forceinline CP StaticSwapIfDynSymVal(CP symWithFlags, CP v) {
		return symWithFlags & FLAG_DYNAMIC ? Lisp().SwapIfDynSymVal(symWithFlags, v) : v;
	}

	void PushSymbols(CPackage *pack, uint32_t& n);
	void PrintValues(ostream& os);

	// Special Operators

	CP m_cond,
		m_spDepth;
	ssize_t m_stackOff,
		m_spOff;


	CFunctionMacro *CreateFunctionMacro(CP f, CP e);
	bool FunctionMacroP(CP p) { return Type(p) == TS_FUNCTION_MACRO; }

	void FinishFLet(CP *pTop, CP body);
	CP SkipDeclarations(CP body);

	CP BindLet(size_t n, CP& sym, CP form);
	CP BindLetA(size_t n, CP& sym, CP form);

	bool CheckSetqBody(CP p);
	CP BindSymbolMacrolet(size_t n, CP& sym, CP form);

#if UCFG_LISP_DEBUGGER
	// Debugger functions
	bool StackUpendP(CP *ps);
	bool StackDownendP(CP *ps);
	CP *TestFrameMove(CP *frame, size_t mode);
	CP *FrameUp(CP *pStack, size_t mode);
	CP *FrameDown(CP *ps, size_t mode);
	void PrintNextEnv(CP env);
	void PrintNextVarFunEnv(CP env);
	CP *PrintStackItem(CP *frame);
	CEnvironment SameEnvAs();
#endif



#if UCFG_LISP_MACRO_AS_SPECIAL_OPERATOR
	// Macros
	void AndOr(bool bAnd);
	CP BindVarMVB(size_t n, CP& sym, CP form);
#endif

	// Number Functions
	static const byte s_ts2numberP[];
	bool NumberP(CP p) { return s_ts2numberP[Type(p)]; }
	bool IntegerP(CP p) { return Type(p)==TS_FIXNUM || Type(p)==TS_BIGNUM; }
	bool FloatP(CP p) { return Type(p)==TS_FLONUM; }
	bool RationalP(CP p);
	bool RealP(CP p);

	double ToFloatValue(CP x);
	CP CoerceTo(CP x, CTypeSpec ts);
	void CheckNumbers(size_t nArgs);
	void CheckIntegers(size_t nArgs);
	void CheckReal(CP x);
	void CheckReals(size_t nArgs);

	typedef void (CLispEng::*FPAddSub)(CP n1, CP n2);

	void Add(CP n1, CP n2);
	void Sub(CP n1, CP n2);
	void NormPair();
	void AddSub(FPAddSub pfn, size_t nArgs, bool bMinus);

	void Mul(const CComplex& x, const CComplex& y);
	void Mul(const CRatio& x, const CRatio& y);
	
	bool EqualpReal(CP x, CP y);
	bool EqualpNum(const CComplex& x, const CComplex& y);

	void MakeRatio();

	CComplex CoerceToComplex(CP p) {
		if (Type(p) == TS_COMPLEX)
			return *AsComplex(p);
		CComplex c;
		c.m_real = p;
		c.m_imag = V_0;
		return c;
	}

	CRatio CoerceToRatio(CP p) {
		if (Type(p) == TS_RATIO)
			return *AsRatio(p);
		CRatio c;
		c.m_numerator = p;
		c.m_denominator = V_1;
		return c;
	}

	void EqualpNum(CP x, CP y);

	void Lesser(CP x, CP y);
	void Greater(CP x, CP y);
	void GreaterOrEqual(CP x, CP y);
	void LesserOrEqual(CP x, CP y);
	void BinCompare(FPAddSub pfn, size_t nArgs);


	bool HeapedP(CP p) {
		switch (Type(p)) {
		case TS_CHARACTER:
		case TS_FIXNUM:
		case TS_SUBR:
			return false;
		default:
			return true;
		}
	}

	//!!!  CPtr CreateRatio(int n, int d, bool bMinus);

	// Float functions
	double CheckFinite(double d);
	void FloatResult(double d);



	CP PackageFromIndex(int idx);


	pair<CP, CP> ListLength(CP p);

	// Fast Functions


	class CMapCB {
	public:
		virtual ~CMapCB() { Lisp().m_cVal = 1; }

		virtual void OnResult(CP x) {};
	};

	// Sequence functions
	void Map(CMapCB& mapCB, CP *pStack, size_t nArgs, bool bCar);

	CP SeqType(CP td) { return AsArray(td)->GetElement(0); } //!!!
	CP SeqInit(CP td) { return AsArray(td)->GetElement(1); } //!!!
	CP SeqUpd(CP td) { return AsArray(td)->GetElement(2); } //!!!
	CP SeqFeUpd(CP td) { return AsArray(td)->GetElement(5); } //!!!
	CP SeqAccess(CP td) { return AsArray(td)->GetElement(7); } //!!!
	CP SeqAccessSet(CP td) { return AsArray(td)->GetElement(8); } //!!!
	CP SeqMake(CP td) { return AsArray(td)->GetElement(11); } //!!!
	CP SeqInitStart(CP td) { return AsArray(td)->GetElement(14); } //!!!
	CP SeqFeInitEnd(CP td) { return AsArray(td)->GetElement(15); } //!!!

	CP DoSeqAccess(CP seq, CP ptr);
	void DoSeqSetAccess(CP seq, CP ptr, CP v);
	typedef CP (CLispEng::*FPSeqUpdate)(CP ptr);
	CP DoListUpdate(CP ptr);
	CP DoVectorUpdate(CP ptr);
	CP DoVectorFeUpdate(CP ptr);
	FPSeqUpdate GetSeqUpdate(CP seq);
	FPSeqUpdate GetSeqFeUpdate(CP seq);

	void SetSeqElt(CP rseq, CP& p, CP el);

	class ILispHandler {
	protected:
		LISP_LISPREF;
	public:
		bool m_bReturn;

		ILispHandler()
			:	LISP_SAVE_LISPREF(Lisp())
				m_bReturn(false)
		{}

		virtual ~ILispHandler() {}
	};

	class ISeqHandler : public ILispHandler {
	public:
		CP m_seq;
		CP m_td;

		virtual bool OnItem(CP x, CP z, CP i) =0;
		virtual void Fun(CP bv) {}
	};

	void ProcessSeq(CP seq, CP fromEnd, CP start, CP end, CP count, CP key, ISeqHandler& sh, bool bCreateBV = false);

	struct CSeqTestHandler;
	void ProcessSeqTest(CP seq, CP fromEnd, CP start, CP end, CP count, CP key, CSeqTestHandler& sh, bool bCreateBV, CP item, CP test, CP testNot);

	void MemberAssoc(bool bAssoc);

	int TestRadix(CP p);
	int TestRadix();

	CP DigitChar(int w) { return CreateChar(wchar_t(w + (w<10 ? '0' : 'A'-10))); }

	// Array Functions
	byte EltypeCode(CP p);

	byte ToElType(CP p);
	size_t CheckDims(CP& dims);
	void TestIndex(CArrayValue*& av, size_t& idx);


	// Stream Functions

	class IStreamHandler : public ILispHandler {
	public:
		virtual CP OnConcatenated(CP p) { return p; }
		virtual void OnEcho(CP p) { return Fun(p); }
		virtual CP OnTwoWay(CP p) { return p; }
		virtual void Fun(CP p) { Throw(E_NOTIMPL); }
	};

	class CInputStreamHandler : public IStreamHandler {
	public:
		virtual CP OnConcatenated(CP p);
		void OnEcho(CP p);
		CP OnTwoWay(CP p);
		virtual void FunOut(CP s, CP val) {}
	};

	class COutputStreamHandler : public IStreamHandler {
	public:
		CP OnTwoWay(CP p);
	};

	void ProcessStream(CP p, IStreamHandler& sh);
	void ProcessInputStream(CP p, IStreamHandler& sh);
	void ProcessOutputStream(CP p, IStreamHandler& sh);
	void ProcessOutputStreamDesignator(CP p, IStreamHandler& sh);

	void CheckFlushFileStream(CStreamValue *stm);
	CStreamValue *GetTargetStream(CP p);
	CStreamValue *CheckOpened(CStreamValue *stm);
	CP __fastcall TestIStream(CP p);
	CP __fastcall TestOStream(CP p);
	CP GetOutputStream(CP p);
	CP GetUniStream(CP p);
	bool StreamP(CP p);
	bool TerminalStreamP(CP p);
	void EnsureInputStream(CP p);
	void EnsureOutputStream(CP p);
	CStreamValue *CheckStreamType(CP p, int sts);
	void PopStreamIn(int sts);
	void PopStreamOut(int sts);
	size_t SeqLength(CP seq);
	pair<size_t, size_t> PopSeqBoundingIndex(CP seq, size_t& len);
	pair<size_t, size_t> PopStringBoundingIndex(CP str);

	CP ToUniversalTime(const DateTime& utc);
	path FromPathnameDesignator(CP p);
	CReadtable *FromReadtableDesignator(CP p);
	CPackage *FromPackageDesignator(CP p);
	CP FromFunctionDesignator(CP p);

	// Character Functions
	struct CCharName {
		char m_ch;
		const char *m_name;
	};

	//!!!D static CCharName s_arCharName[];



	//Number Functions

	//Package Functions
	void MakePresent(CP sym, CPackage *pack, bool bCheckSymMacro = true);
	void Unpresent(CP sym, CP package);
	bool FindInherited(RCString name, CPackage *pack, CP& r);
	CP FindSymbol(RCString name, CP package, CP& sym, byte& flags);
	
	typedef void (CLispEng::*PFN_PackageFun)(CP, CP);
	typedef void (CLispEng::*PFN_PackageCheckFun)(CP);

	void CheckStringDesignator(CP name);	
	void CheckSymbol(CP name);
	void ApplySymbols(PFN_PackageFun packageFun, PFN_PackageCheckFun checkFun);

	void Shadow(CP name, CP package);
	void ShadowingImport(CP sym, CP package);
	void Import(CP sym, CP package);
	void Export(CP sym, CP package);
	void Unexport(CP sym, CP package);
		

	// Pathname Functions
	bool LogicalHostP(CP p);
	CP ToOptionalHost(CP p);
	CP ToDefaultPathname(CP p);

	// File Functions
	path GetDirectoryName(CP pathname);

	// Environment Functions

	// Random-State Functions

	// Reader Functions
	void CopyReadtable(CReadtable *from, CReadtable *to);



	void ApplyRefs(CP& place, bool bMark);
	void ApplyRefsSValue(CP& p, bool bMark);
	void MarkRefs(CP p, bool bMark);
	void MakeReferences(CP p);
	CP ReadSValue(CP stm, bool bEofError = true, CP eofVal = 0, bool bRec = false, bool bPreservingWhitespace = false);
	CP ReadTop(CP stm, bool bEofError, CP eofVal, bool bPreservingWhitespace);
	void Read(bool bPreservingWhitespace);



	// Sharp Dispatcher Functions
	void CheckNilInfix(CP infix, CP stm);

	bool InterpretFeature(CP p);
	void SharpCond(bool bPlus);

	// Printer Functions

	struct CMarkSweepHandler : public ILispHandler {
		CP m_p;

		virtual bool OnApply(CP p, bool bMark) =0;
		virtual void OnMark(CP p, bool bAlreadyMarked) =0;
	};

	void ApplyMark(CMarkSweepHandler& h, CP& place, bool bMark);
	void ApplyMarkSValue(CMarkSweepHandler& h, CP& p, bool bMark);
	void Mark(CMarkSweepHandler& h, bool bMark);
	void MarkSweep(CMarkSweepHandler& h);
	CP GetCircularities(CP p);

	void WriteUint(size_t n);

	struct CCircleInfo {
		bool m_bFirst;
		size_t m_idx;
	};

	bool CircleP(CP p, CCircleInfo *ci);
	size_t GetLengthLimit();
	bool ListLengthIs1(CP p);
	void PrCons(CP p);
	void PrString(CP p);
	void PrBitVector(CP p);
	void PrVector(CP p);
	void PrArrayRec(size_t depth, CP restDims);
	void PrArray(CP p);
	void PrRealNumber(CP p);
	void PrNumber(CP p);
	void WriteStr(RCString s);
	

	void WriteUpDownStr(RCString s, FPLispFunc pfnConv);
	void WriteUpcaseStr(RCString s);
	void WriteDowncaseStr(RCString s);
	void WriteCapitalizeStr(RCString s, FPLispFunc pfnConv);
	void WriteCaseString(RCString s);
	void WriteEscapeName(RCString s);
	void PrSymbolPart(RCString s);
	void PrSymbol(CP p);
	void PrFramePtr(CP p);
	void PrFrameInfo(CP p);
	void PrInstance(CP p);
	void PrCClosureCodeVector(CP p);
	void PrCClosure(CP p);

	void PrCircle(CP p);
	void PrinObject(CP p);
	bool StringFitLineP(CP stm, CP p, int off);
	void PrEnter1(CP p);
	void PrEnter2(CP p);
	void PrEnter(CP stm, CP p);
	void PrOtherObj(RCString obj, RCString s);
	void PrOtherObj(CP obj, RCString s);
	CP GetSubrName(CP p);

	CP FunnameToSymbol(CP p);
	CP GetHash(CP key, CP ht);
	//!!!  CPtr GetTypes(CP p);

	// FFI Functions


	void CheckSymbolValue(CP sym);

	CP SymbolValue(CP sym);

	void PutBackChar(CP stm, wchar_t ch);

	void ReadCharHelper(CP stm, bool bEofErr, CP eofVal, bool bRec, bool bNoHang, CP gen);
	int ReadChar(CP stm, bool bEofErr = true);

	StandardStream *GetInputTextStream(CStreamValue *stm);
	StandardStream *GetOutputTextStream(CStreamValue *stm);

//Pretty-Printer
	void MultiLineSubBlockOut(CP block);
	int FormatTab(CP stm, CP vec);
	int PprintPrefix(bool bPrint, int indent);
	CP RightMargin();
	CP CreatePPString();
	CP ConsPPString(CStreamValue *stm, CP nlType);
	void PPNewLine();
	void JustifyEmpty1();
	void JustifyEmpty();
	void JustifySpace();
	void JustifyLast();
	bool CheckLinesLimit();
	bool CheckLengthLimit(bool b);
	void PrettyPrintCall(CP x);
	void PrPair(CP car, CP cdr);

	void PrinObjectDispatch(CP p);
	void PPrintObjectDispatch(CP p);

	typedef void(CLispEng::*MFNPrintDispatch)(CP p);
	MFNPrintDispatch m_mfnPrintDispatch;

	class CPrintDispatchKeeper {
	public:
		CPrintDispatchKeeper(MFNPrintDispatch mfn)
			:	m_mfn(exchange(Lisp().m_mfnPrintDispatch, mfn))
		{
		}

		~CPrintDispatchKeeper() {
			Lisp().m_mfnPrintDispatch = m_mfn;
		}
	private:
		MFNPrintDispatch m_mfn;
	};

	void PrinDispatch(CP p);

	void CharToStream(CP c, CP stm);
	void WriteChar(wchar_t ch);
	void WritePChar(const char *p);
	void Prin1(CP p);

	StandardStream *DoOutputOperation(CP sym);

	//	*/

	// Hash-Table Functions
	size_t HashEq(CP p);
	size_t HashEql(CP p);
	size_t HashcodeCons(CP p, int depth = 0);
	size_t HashEqual(CP p);

	size_t HashcodeTuple(size_t n, CP *pRest, int depth = 0);
	bool EqualTuple(CP p, size_t n, CP *pRest);
	bool EqualTuple(CP p, CP q);



	//!!!  CSPtr CreateArray(CP dim1, CP cdr, CP typ);
	//!!!  void FillArrayByElement(CP ar, CP initElement);
	//!!!  void FillArrayByCont(CP ar, CP initCont);




	//!!!D	void SetCodevecFromList(CP closure, CP p);

	// Closures
	bool ClosureP(CP p) {
		switch (Type(p)) {
		case TS_INTFUNC:
		case TS_CCLOSURE:
			return true;
		}
		return false;
	}

	pair<CP, CP> GetFunctionName(CP fun);
	CP& ClosureName();


	// Structure Functions
	CP& GetStructRef();


	// CLOS Functions
	void AllocateInstance(CP cl);
	void CheckInitArgList(size_t nArgs);
	void KeywordTest(CP *pStack, size_t nArgs, CP vk);

	pair<CP, CP> GetSlotInfo(CP inst, CP slotname, CP fun, bool bArgInStack);
	CP ClassOf(CP p);
	CP& PtrToSlot(CP inst, CP slotinfo);
	CP& SlotAccess();

	CP& SlotUsingClass();

	void InitStdInstance(size_t len);



	// GC Functions

	// Print functions
	bool RtCaseDifferent(CP rtCase, CP pch);
	void PrintInt(CP n, int base);

	pair<String, CP> GetStaticSymbolNamePack(int i);

};  // CLispEng

template <class T, CTypeSpec Tts>
T *CValueMan<T, Tts>::CreateInstance() {
	void *pHeap = m_pHeap;
	if (!pHeap) {
		Lisp().Collect();
		pHeap = m_pHeap;
	}
	m_pHeap = ((T**)pHeap)[1];
	return ::new(pHeap) T;
}

template <class T, CTypeSpec Tts>
T *CValueMan<T, Tts>::CreateInstance(const String::value_type *s) {
	if (!m_pHeap)
		Lisp().Collect();

#if UCFG_LISP_SPLIT_SYM
	T *r = CreateInstance();
	Lisp().SymNameByIdx(r-Base) = s;
	return r;
#else
	return ::new(exchange(m_pHeap, (T*)((DWORD_PTR*)m_pHeap)[1])) T(s);
#endif
}

template <class T, CTypeSpec Tts>
T *CValueMan<T, Tts>::CreateInstance(const BigInteger& i) {
	if (!m_pHeap)
		Lisp().Collect();
	return ::new(exchange(m_pHeap, (T*)((DWORD_PTR*)m_pHeap)[1])) T(i);
}	



inline CIntFuncChain::CIntFuncChain(CIntFuncValue *p) {
	CLispEng& lisp = Lisp();
	m_prev = lisp.m_pIntFuncChain.get();
	lisp.m_pIntFuncChain.reset(this);
	m_p = p;
}

inline CIntFuncChain::~CIntFuncChain() {
	Lisp().m_pIntFuncChain.reset(m_prev);
}


inline CClosure::CClosureHeader& CClosure::get_Header() {
	return *(CClosureHeader*)Lisp().AsArray(CodeVec)->m_pData;
}

inline intptr_t AsNumber(CP p) {
	if (Type(p) != TS_FIXNUM)
		LISP.E_TypeErr(p, S(L_FIXNUM));
	return ((intptr_t)p >> VALUE_SHIFT);//!!!
}

inline byte AsBit(CP p) {
	switch (p) {
	case V_0: return 0;
	case V_1: return 1;
	default:
		Lisp().E_TypeErr(p, S(L_BIT));
	}
}

inline wchar_t AsChar(CP p) {
	if (Type(p) != TS_CHARACTER)
		Lisp().E_TypeErr(p, S(L_CHARACTER));
	return (wchar_t)(p >> VALUE_SHIFT);
}
#ifdef X_DEBUG//!!!D
#define DBG_WF(p) (p & ~FLAG_Mark)
inline size_t CArrayValue::GetVectorLength() { return AsNumber(DBG_WF(m_fillPointer) ? DBG_WF(m_fillPointer) : m_dims); }
#else
inline size_t CArrayValue::GetVectorLength() { return AsNumber(m_fillPointer ? m_fillPointer : m_dims); }
#endif

inline size_t CArrayValue::get_DataLength() { return AsNumber(m_dims); }

inline size_t AsPositive(CP p) {
	intptr_t n = AsNumber(p);
	if (n < 0) {
		CLispEng& lisp = Lisp();
		lisp.Push(p);
		lisp.E_TypeErr(lisp.SV, lisp.List(S(L_INTEGER), V_0));
	}
	return n;
}

inline CP __fastcall FromSValue(CSValue *pv, CTypeSpec ts) {
	return Lisp().FromSValueT(pv, ts);
}

inline CConsValue *ToCons(CP p) {
	return Lisp().ToCons(p);
}

inline CP Car(CP p) {
	return Lisp().Car(p);
}

inline CP Cdr(CP p) {
	return Lisp().Cdr(p);
}

inline void Inc(CSPtr& p) {
	Lisp().Inc(p);
}

inline bool SplitPair(CSPtr& p, CSPtr& car) {
	return Lisp().SplitPair(p, car);
}

inline CSymbolValue *ToSymbol(CP p) {
	return Lisp().ToSymbol(p);
}

class CStackKeeper {
public:
	LISP_LISPREF;

	ssize_t m_deep;

	CStackKeeper(CLispEng& lisp)
		:	LISP_SAVE_LISPREF(lisp)
			m_deep(lisp.m_pStackTop-lisp.m_pStack)
	{
	}

	~CStackKeeper() {
		CLispEng& lisp = LISP_GET_LISPREF;
		lisp.m_pStack = lisp.m_pStackTop-m_deep;
	}
};

class CListConstructor : public CStackKeeper {
	CSPtr m_next;
public:
	CListConstructor()
		:	CStackKeeper(Lisp())
	{
		LISP_GET_LISPREF.Push(0);
	}

	operator CP() { return LISP_GET_LISPREF.m_pStackTop[-m_deep-1]; } //!!!

	void Add(CP p) {
		CLispEng& lisp = LISP_GET_LISPREF;
		CP q = lisp.Cons(p, 0);
		if (m_next)
			lisp.AsCons(m_next)->m_cdr = q;
		else
			lisp.m_pStackTop[-m_deep-1] = q;
		m_next = q;
	}

	void SetCdr(CP p) {
		LISP_GET_LISPREF.AsCons(m_next)->m_cdr = p;
	}
};

#define KEYWORDS1(a) a 
#define KEYWORDS2(a, b) a b
#define KEYWORDS3(a, b, c) a b c

inline pair<size_t, bool> KeywordsLen(const byte *pk) {
	size_t nKey = strlen((const char*)pk);
	bool bAllowOtherKeys = pk[nKey-1] == L_K_SPECIAL_ALLOW_OTHER_KEYS;
	if (bAllowOtherKeys)
		--nKey;
	return make_pair(nKey, bAllowOtherKeys);
}


class CFrameBaseEx {
#if UCFG_LISP_FRAME_AS_OFFSET
	ssize_t m_nStack;
public:
	__forceinline CP *GetStackP(CLispEng& lisp = Lisp()) {
		return lisp.m_pStackTop-m_nStack;
	}

	__forceinline void SetStackP(CLispEng& lisp) {
		m_nStack = lisp.m_pStackTop-lisp.m_pStack;
	}
#else
	CP *m_stackP;
public:
	__forceinline CP *GetStackP() {
		return m_stackP;
	}

	__forceinline CP *GetStackP(CLispEng& lisp) {
		return m_stackP;
	}

	__forceinline void SetStackP(CLispEng& lisp) {
		m_stackP = lisp.m_pStack;
	}
#endif // UCFG_LISP_FRAME_AS_OFFSET

	__forceinline void Finish(CLispEng& lisp, CFrameType frameType, CP *pTop) {
		lisp.Push(lisp.CreateFrameInfo(frameType, pTop));
		SetStackP(lisp);
	}

	void RestoreStacks() {
		CLispEng& lisp = Lisp();
		lisp.m_pStack = GetStackP(lisp);
		lisp.m_pSP = lisp.m_pSPTop-AsNumber(lisp.m_pStack[FRAME_SP]);
	}
protected:
	void ReleaseStack(CLispEng& lisp = Lisp()) {
		lisp.m_pStack = AsFrameTop(GetStackP(lisp));
	}
};

class CVarFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CP *m_pVars;

	CVarFrame(CP *pVars)
		:	m_pVars(pVars)
	{
	}

	~CVarFrame();
	bool __fastcall Bind(CP val);

	void Finish(CLispEng& lisp, CP *pTop) {
		base::Finish(lisp, FT_VAR, pTop);
	}
};

class CEnv1VFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv1VFrame();
	~CEnv1VFrame();
};

class CEnv1FFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv1FFrame();
	~CEnv1FFrame();
};

class CEnv1BFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv1BFrame(CLispEng& lisp) {
		lisp.Push(lisp.m_env.m_blockEnv);
		Finish(lisp, FT_ENV1B, lisp.m_pStack+1);
	}

	~CEnv1BFrame();
};

class CEnv1GFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv1GFrame();
	~CEnv1GFrame();
};

class CEnv1DFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv1DFrame();
	~CEnv1DFrame();
};

class CEnv2VDFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv2VDFrame();
	~CEnv2VDFrame();
};

class CEnv5Frame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CEnv5Frame(CLispEng& lisp) {
		*(CEnvironment*)(lisp.m_pStack-=5) = lisp.m_env;
		//!!! memcpy(lisp.m_pStack-=5, &lisp.m_env, 5*sizeof(CP));//!!!
		Finish(lisp, FT_ENV5, lisp.m_pStack+5);
	}

	~CEnv5Frame() {
		CLispEng& lisp = Lisp();
		memcpy(&lisp.m_env, GetStackP(lisp)+1, 5*sizeof(CP));
		//!!!    lisp.m_env = *(CEnvironment*)(GetStackP(lisp)+1);
		base::ReleaseStack(lisp);
	}
};

class CJmpBufBase : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CJmpBufBase *m_pNext;

	CJmpBufBase() {
		CLispEng& lisp = Lisp();
		m_pNext = lisp.m_pJmpBuf;
		lisp.m_pJmpBuf.reset(this);
	}
	~CJmpBufBase() { 
		CLispEng& lisp = Lisp();
		lisp.m_pJmpBuf.reset(m_pNext);
		base::ReleaseStack(lisp);
	}

#if UCFG_LISP_SJLJ
#	if LISP_ALLOCA_JMPBUF
		jmp_buf *m_pJmpbuf;
#	else
		jmp_buf m_jmpbuf;
#	endif


	/*!!!	operator jmp_buf&() { return m_jmpbuf; }

	int SetJmp()
	{
	if (int r = setjmp(m_jmpbuf))
	{
	Lisp().m_bUnwinding = false;
	return r;
	}
	return 0;
	}*/
#endif
};


#if UCFG_LISP_SJLJ

class CJmpBuf : public CJmpBufBase {
#if LISP_ALLOCA_JMPBUF
	jmp_buf m_jmpbuf;
public:
	CJmpBuf() {
		m_pJmpbuf = &m_jmpbuf;
	}
#endif
};

#	if LISP_ALLOCA_JMPBUF
#		define SETJMP(jb) (setjmp(*(jb).m_pJmpbuf) ? ((m_bUnwinding = false), true) : false)
#	else
#		define SETJMP(jb) (setjmp((jb).m_jmpbuf) ? ((m_bUnwinding = false), true) : false)
#	endif

#	define LISP_TRY(frame) if (!SETJMP(frame)) {
#	define LISP_CATCH(frame) } else {

#	if LISP_ALLOCA_JMPBUF
#		define LISP_THROW(pframe) longjmp_unwind(*pframe->m_pJmpbuf, 1); //!!!	
#	else
#		define LISP_THROW(pframe) longjmp_unwind(pframe->m_jmpbuf, 1); //!!!		
#	endif

#else

typedef CJmpBufBase CJmpBuf;

class LispUnwindExc : public Exception {
	typedef Exception base;
public:
	CJmpBufBase *m_pFrame;

	LispUnwindExc(CJmpBufBase *pFrame)
		:	base(HR_UNWIND)		// this code ignored by Debugger in elrt for Fast Processing
		,	m_pFrame(pFrame)
	{
	}

	operator CJmpBufBase *() const  { return m_pFrame; }
};

typedef const LispUnwindExc& RCLispUnwindExc;

#	define LISP_TRY(frame) try {
#	define LISP_CATCH(frame) } catch (RCLispUnwindExc _pFrame) { if (_pFrame != &frame) throw; m_bUnwinding = false;
#	define LISP_THROW(pframe) throw RCLispUnwindExc(pframe);

#endif

#define LISP_CATCH_END }


class CEvalFrame : public CJmpBufBase {
public:
	inline CEvalFrame(CLispEng& lisp, CP p) {
		lisp.Push(p);
		Finish(lisp, FT_EVAL, lisp.m_pStack+1);
	}
};

class CPseudoEvalFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	inline CPseudoEvalFrame(CLispEng& lisp, CP p) {
		lisp.Push(p);
		Finish(lisp, FT_EVAL, lisp.m_pStack+1);
	}

	~CPseudoEvalFrame() {
		base::ReleaseStack(Lisp());
	}
};

class CApplyFrame : public CJmpBufBase {
public:
	CApplyFrame(CLispEng& lisp, CP *pTop, CP p) {
		lisp.Push(p);
		Finish(lisp, FT_APPLY, pTop);
	}
};

class CIBlockFrame : public CJmpBuf {
public:
	size_t m_spOff;

	CIBlockFrame(CLispEng& lisp, CP name) {
		m_spOff = lisp.m_pSPTop-lisp.m_pSP;
		lisp.Push(lisp.m_env.m_blockEnv, name);
		Finish(lisp, FT_IBLOCK, lisp.m_pStack+2);
	}

	~CIBlockFrame() {
		CLispEng& lisp = Lisp();
		CP *fr = GetStackP(lisp);
		if (CLispEng::IsFrameNested(fr[0]))
			lisp.ToCons(Car(fr[2]))->m_cdr = V_D;
	}
};

class CDynBindFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CDynBindFrame() {}
	CDynBindFrame(CP syms, CP vals);
	CDynBindFrame(CP sym, CP val, bool bBind); // form for one/zero pair;
	~CDynBindFrame();
	void Bind(CLispEng& lisp, CP sym, CP val);
	void Finish(CLispEng& lisp, size_t count);
};

class CCatchFrame  : public CJmpBuf {
public:
	CCatchFrame(CLispEng& lisp, CP tag);
};

class CFunFrame : public CFrameBaseEx {
	typedef CFrameBaseEx base;
public:
	CFunFrame(CP *pTop);
	
	~CFunFrame() {
		base::ReleaseStack(Lisp());
	}
};

const byte FLAG_REST              = 0x01,
	FLAG_RETURN_METHOD     = 0x04,
	FLAG_GENERIC           = 0x10,
	FLAG_ALLOW_OTHER_KEYS  = 0x40,
	FLAG_KEY               = 0x80; 


/*!!!
class CTracer
{
CIntFuncValue& m_op;
public:
CTracer(CIntFuncValue& op, CP p)
:m_op(op)
{
if (m_op.m_bTrace)
Enter(p);
}

~CTracer()
{
if (m_op.m_bTrace)
Exit();
}

void Enter(CP p);
void Exit();
};*/

class CLockGC {
	bool m_b;
public:
	CLockGC()
		:	m_b(Lisp().m_bLockGC)
	{
		Lisp().m_bLockGC = true;
	}

	~CLockGC() {
		Lisp().m_bLockGC = m_b;
	}
};

#if UCFG_LISP_LIGHT_VM_CONTEXT

//#	define VM_SAVE_CURCLOSURE CP _curClosure = CurClosure
//#	define VM_SAVE_PB byte *_pb = m_pb
#	define VM_SAVE_CONTEXT CVMContextBase *_curVMContext = CurVMContext

//#	define VM_RESTORE_CURCLOSURE CurClosure = _curClosure
//#	define VM_RESTORE_PB m_pb = _pb

#	define VM_RESTORE_CONTEXT CurVMContext.reset(_curVMContext)


#	define VM_CONTEXT

#else

#	define VM_SAVE_CURCLOSURE
#	define VM_SAVE_PB
#	define VM_SAVE_CONTEXT

#	define VM_RESTORE_CURCLOSURE
#	define VM_RESTORE_PB
#	define VM_RESTORE_CONTEXT

class CVMContext : CVMContextBase {
	typedef CVMContextBase base;

	LISP_LISPREF;

public:
	CVMContext(CLispEng& lisp)
		:	LISP_SAVE_LISPREF(lisp)
	{
		m_closure = lisp.CurClosure;
		m_pb = lisp.m_pb;
	}

	~CVMContext() {
		CLispEng& lisp = LISP_GET_LISPREF;
		lisp.CurClosure = m_closure;
		lisp.m_pb = m_pb;
	}
};

#	define VM_CONTEXT CVMContext vmContext(_self);

#endif // UCFG_LISP_LIGHT_VM_CONTEXT

enum CByteCode {
	COD_NIL,              COD_PUSH_NIL,         COD_T,                COD_CONST,				// 0x0
	COD_LOAD,             COD_LOADI,            COD_LOADC,            COD_LOADV,
	COD_LOADIC,           COD_STORE,            COD_STOREI,           COD_STOREC,
	COD_STOREV,           COD_STOREIC,          COD_GETVALUE,         COD_SETVALUE,

	COD_BIND,             COD_UNBIND1,          COD_UNBIND,           COD_PROGV,				// 0x10
	COD_PUSH,             COD_POP,              COD_SKIP,             COD_SKIPI,
	COD_SKIPSP,           COD_SKIP_RET,         COD_SKIP_RETGF,       COD_JMP,
	COD_JMPIF,            COD_JMPIFNOT,         COD_JMPIF1,           COD_JMPIFNOT1,

	COD_JMPIFATOM,        COD_JMPIFCONSP,       COD_JMPIFEQ,          COD_JMPIFNOTEQ,			// 0x20
	COD_JMPIFEQTO,        COD_JMPIFNOTEQTO,     COD_JMPHASH,          COD_JMPHASHV,
	COD_JSR,              COD_JMPTAIL,          COD_VENV,             COD_MAKEVECTOR1_PUSH,
	COD_COPYCLOSURE,      COD_CALL,             COD_CALL0,            COD_CALL1,

	COD_CALL2,            COD_CALLS1,           COD_CALLS2,           COD_CALLSR,				// 0x30
	COD_CALLC,            COD_CALLCKEY,         COD_FUNCALL,          COD_APPLY,
	COD_PUSH_UNBOUND,     COD_UNLIST,           COD_UNLISTSTERN,      COD_JMPIFBOUNDP,
	COD_BOUNDP,           COD_UNBOUND_NIL,      COD_VALUES0,          COD_VALUES1,

	COD_STACKTOMV,        COD_MVTOSTACK,        COD_NVTOSTACK,        COD_MVTOLIST,				// 0x40
	COD_LISTTOMV,         COD_MVCALLP,          COD_MVCALL,           COD_BLOCKOPEN,
	COD_BLOCKCLOSE,       COD_RETURNFROM,       COD_RETURNFROMI,      COD_TAGBODYOPEN,
	COD_TAGBODYCLOSE_NIL, COD_TAGBODYCLOSE,     COD_GO,               COD_GOI,

	COD_CATCHOPEN,        COD_CATCHCLOSE,       COD_THROW,            COD_UWPOPEN,				// 0x50
	COD_UWPNORMALEXIT,    COD_UWPCLOSE,         COD_UWPCLEANUP,       COD_HANDLEROPEN,
	COD_HANDLERBEGIN_PUSH, COD_NOT,              COD_EQ,               COD_CAR,
	COD_CDR,              COD_CONS,             COD_SYMBOLFUNCTION,   COD_SVREF,

	COD_SVSET,            COD_LIST,             COD_LISTSTERN,        COD_NIL_PUSH,				// 0x60
	COD_T_PUSH,           COD_CONST_PUSH,       COD_LOAD_PUSH,        COD_LOADI_PUSH,
	COD_LOADC_PUSH,       COD_LOADV_PUSH,       COD_POP_STORE,        COD_GETVALUE_PUSH,
	COD_JSR_PUSH,         COD_COPYCLOSURE_PUSH, COD_CALL_PUSH,        COD_CALL1_PUSH,

	COD_CALL2_PUSH,       COD_CALLS1_PUSH,      COD_CALLS2_PUSH,      COD_CALLSR_PUSH,			// 0x70
	COD_CALLC_PUSH,       COD_CALLCKEY_PUSH,    COD_FUNCALL_PUSH,     COD_APPLY_PUSH,
	COD_CAR_PUSH,         COD_CDR_PUSH,         COD_CONS_PUSH,        COD_LIST_PUSH,
	COD_LISTSTERN_PUSH,   COD_NIL_STORE,        COD_T_STORE,          COD_LOAD_STOREC,

	COD_CALLS1_STORE,     COD_CALLS2_STORE,     COD_CALLSR_STORE,     COD_LOAD_CDR_STORE,		// 0x80
	COD_LOAD_CONS_STORE,  COD_LOAD_INC_STORE,   COD_LOAD_DEC_STORE,   COD_LOAD_CAR_STORE,
	COD_CALL1_JMPIF,      COD_CALL1_JMPIFNOT,   COD_CALL2_JMPIF,      COD_CALL2_JMPIFNOT,
	COD_CALLS1_JMPIF,     COD_CALLS1_JMPIFNOT,  COD_CALLS2_JMPIF,     COD_CALLS2_JMPIFNOT,

	COD_CALLSR_JMPIF,     COD_CALLSR_JMPIFNOT,  COD_LOAD_JMPIF,       COD_LOAD_JMPIFNOT,		// 0x90
	COD_LOAD_CAR_PUSH,    COD_LOAD_CDR_PUSH,    COD_LOAD_INC_PUSH,    COD_LOAD_DEC_PUSH,  
	COD_CONST_SYMBOLFUNCTION, COD_CONST_SYMBOLFUNCTION_PUSH, COD_CONST_SYMBOLFUNCTION_STORE, COD_APPLY_SKIP_RET,
	COD_FUNCALL_SKIP_RETGF, COD_LOAD0,          COD_LOAD1,            COD_LOAD2,

	COD_LOAD3,            COD_LOAD4,            COD_LOAD5,            COD_LOAD6,				// 0xA0
	COD_LOAD7,            COD_LOAD8,            COD_LOAD9,            COD_LOAD10,
	COD_LOAD11,           COD_LOAD12,           COD_LOAD13,           COD_LOAD14,
	COD_LOAD_PUSH0,       COD_LOAD_PUSH1,       COD_LOAD_PUSH2,       COD_LOAD_PUSH3,

	COD_LOAD_PUSH4,       COD_LOAD_PUSH5,       COD_LOAD_PUSH6,       COD_LOAD_PUSH7,			// 0xB0
	COD_LOAD_PUSH8,       COD_LOAD_PUSH9,       COD_LOAD_PUSH10,      COD_LOAD_PUSH11,
	COD_LOAD_PUSH12,      COD_LOAD_PUSH13,      COD_LOAD_PUSH14,      COD_LOAD_PUSH15,
	COD_LOAD_PUSH16,      COD_LOAD_PUSH17,      COD_LOAD_PUSH18,      COD_LOAD_PUSH19,

	COD_LOAD_PUSH20,      COD_LOAD_PUSH21,      COD_LOAD_PUSH22,      COD_LOAD_PUSH23,			// 0xC0
	COD_LOAD_PUSH24,      COD_CONST0,           COD_CONST1,           COD_CONST2,
	COD_CONST3,           COD_CONST4,           COD_CONST5,           COD_CONST6,
	COD_CONST7,           COD_CONST8,           COD_CONST9,           COD_CONST10,

	COD_CONST11,          COD_CONST12,          COD_CONST13,          COD_CONST14,				// 0xD0
	COD_CONST15,          COD_CONST16,          COD_CONST17,          COD_CONST18,
	COD_CONST19,          COD_CONST20,          COD_CONST_PUSH0,      COD_CONST_PUSH1,
	COD_CONST_PUSH2,      COD_CONST_PUSH3,      COD_CONST_PUSH4,      COD_CONST_PUSH5,

	COD_CONST_PUSH6,      COD_CONST_PUSH7,      COD_CONST_PUSH8,      COD_CONST_PUSH9,			// 0xE0
	COD_CONST_PUSH10,     COD_CONST_PUSH11,     COD_CONST_PUSH12,     COD_CONST_PUSH13,
	COD_CONST_PUSH14,     COD_CONST_PUSH15,     COD_CONST_PUSH16,     COD_CONST_PUSH17,
	COD_CONST_PUSH18,     COD_CONST_PUSH19,     COD_CONST_PUSH20,     COD_CONST_PUSH21,

	COD_CONST_PUSH22,     COD_CONST_PUSH23,     COD_CONST_PUSH24,     COD_CONST_PUSH25,			// 0xF0
	COD_CONST_PUSH26,     COD_CONST_PUSH27,     COD_CONST_PUSH28,     COD_CONST_PUSH29,
	COD_STORE0,           COD_STORE1,           COD_STORE2,           COD_STORE3,
	COD_STORE4,           COD_STORE5,           COD_STORE6,           COD_STORE7
};

#ifdef _DEBUG//!!!D
extern bool g_print;
#endif


struct CPComparer {

	CLispEng& m_lisp;
	CHashMap& m_hm;

	CPComparer(CHashMap& hm)
		:	m_lisp(Lisp())
		,	m_hm(hm)			
	{
	}

	size_t operator()(CP p) const {
		if (Type(p)==TS_STACK_RANGE) {
			pair<int, int> pp = FromStackRange(p);
			return m_lisp.HashcodeTuple(pp.second, m_lisp.m_pStackBase+pp.first);
		}
		return m_lisp.SxHash(p);
	}

	inline bool operator()(CP p, CP q) const;
};


class CHashMap : public unordered_map<CP, CP, CPComparer, CPComparer> {
	typedef unordered_map<CP, CP, CPComparer, CPComparer> base;
public:
	//!!!bool m_bLoading;
	CSPtr m_func;
//!!!	CLispEng::PFNHash m_pfnHash;

	CHashMap()
		:	base(10, CPComparer(_self), CPComparer(_self))//!!!,
		//!!!m_bLoading(false)
	{}

	void SetTestFun(CP test);
	void Write(BlsWriter& wr);
	void Read(const BlsReader& rd);
};


inline bool CPComparer::operator()(CP p, CP q) const {
	//!!!  if (m_hm.m_bLoading)
	//!!!    return false;

	if (Type(p)==TS_STACK_RANGE || Type(q)==TS_STACK_RANGE)
		return m_lisp.EqualTuple(p, q);
	m_lisp.Call(m_hm.m_func, p, q);
	return m_lisp.m_r;
}


extern const CP g_fnEq, g_fnEql, g_fnEqual, g_fnEqualP,  g_fnEqNum;

#ifdef _DEBUG//!!!D
extern int g_b; 
#endif


class CPrinLevel : CDynBindFrame {
public:
	bool m_b;

	CPrinLevel(bool /*dummy*/) {
		CLispEng& lisp = Lisp();
		CP lev = lisp.Spec(L_S_PRIN_LEVEL),
			 lim = lisp.Spec(L_S_PRINT_LEVEL);
		if (m_b = (lisp.Spec(L_S_PRINT_READABLY) || Type(lim)!=TS_FIXNUM || AsNumber(lev)<AsNumber(lim)))
			Bind(lisp, S(L_S_PRIN_LEVEL), lev);
		else
			lisp.WriteChar('#');
		Finish(lisp, m_b);
	}

	operator bool() { return m_b; }
};

class CStm {
public:
	CStm(CP stm)
		:	m_stm(exchange(Lisp().m_stm, stm))
	{
	}

	~CStm() {
		Lisp().m_stm = m_stm;
	}
private:
	CP m_stm;
};

class CParen {
public:
	CParen() {
		CLispEng& lisp = Lisp();
		lisp.WriteChar('(');
	}

	~CParen() {
		CLispEng& lisp = Lisp();
		if (!lisp.m_bUnwinding)
			lisp.WriteChar(')');
	}
};

// Command Line options
extern int g_bOptVersion,	// print version
			g_bOptDD;		// _DEBUG-only: trace stack on error



#if UCFG_LISP_TAIL_REC == 1

typedef CBoolKeeper CTailRecKeeper;

#elif UCFG_LISP_TAIL_REC == 2

class CTailRecKeeper {
	LISP_LISPREF;
public:
	CTailRecKeeper(CLispEng& lisp, bool v)
#if !UCFG_LISP_SPARE_STACK
		:	m_lisp(lisp)
#endif
	{
		lisp.m_stackTailRec.push_back(v);
	}

	~CTailRecKeeper() {
		LISP_GET_LISPREF.m_stackTailRec.pop_back();
	}
};

#endif // UCFG_LISP_TAIL_REC


#if UCFG_LISP_TAIL_REC == 0
#	define LISP_TAIL_REC_DISABLE_1
#	define LISP_TAIL_REC_RESTORE
#	define LISP_TAIL_REC_KEEPER(v)
#elif UCFG_LISP_TAIL_REC == 1
#	define LISP_TAIL_REC_DISABLE_1	CBoolKeeper trKeeper(m_bAllowTailRec.Ref(), false);
#	define LISP_TAIL_REC_RESTORE	m_bAllowTailRec = trKeeper.m_prev;
#	define LISP_TAIL_REC_ENABLED (m_bAllowTailRec)
#	define LISP_TAIL_REC_KEEPER(v) CTailRecKeeper trKeeper(m_bAllowTailRec.Ref(), v);
#else
#	define LISP_TAIL_REC_DISABLE_1
#	define LISP_TAIL_REC_RESTORE	m_stackTailRec.back() = m_stackTailRec[m_stackTailRec.size()-2];
#	define LISP_TAIL_REC_ENABLED (m_stackTailRec.back())
#	define LISP_TAIL_REC_KEEPER(v) CTailRecKeeper trKeeper(_self, v);
#endif


} // Lisp::

