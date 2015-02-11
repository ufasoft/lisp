#include <el/ext.h>

#include "lispeng.h"

#if UCFG_LISP_FFI
#	ifdef _M_IX86
#		include <el/comp/ia32-codegen.h>
	using namespace Ia32;
#	endif
#endif

namespace Lisp {

#if UCFG_OLE
COleVariant CLispEng::ToOleVariant(CP p) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

#ifdef X_DEBUG
	static ofstream os("c:\\out\\lisp.log");
	Print(os, p);
	os << endl;
#endif
	COleVariant r;
	switch (Type(p)) {
	case TS_CONS:
		if (!p)
			r.vt = VT_NULL;
		else
			E_Error();
		break;
	case TS_FIXNUM:
		return COleVariant((long)AsNumber(p));
	case TS_BIGNUM:
		{
			__int64 n;
			if (!ToBigInteger(p).AsInt64(n))
				E_Error();
			return COleVariant((long)n); // only lower 32 bits
		}
	case TS_SYMBOL:
		if (p == V_T)
			return COleVariant(true);
		E_Error();
	case TS_ARRAY:
		if (StringP(p))
			return COleVariant(AsString(p));
		else if (VectorP(p))
		{
			CArrayValue *av = ToVector(p);
			COleSafeArray sa;
			size_t len = av->GetVectorLength();
			sa.CreateOneDim(VT_VARIANT, (DWORD)len);
			for (long i=0; i<(long)len; ++i) {
				COleVariant v = ToOleVariant(av->GetElement(i));
				sa.PutElement(&i, &v);
			}
			return sa;
		}
		E_Error();
	default:
		E_Error();
	}
	return r;
}

CP CLispEng::FromOleVariant(const VARIANT& v) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (V_ISARRAY(&v)) {
		SAFEARRAY *psa = v.parray;
		CSafeArray sa(psa);
		int count = sa.GetUBound()+1;
		Push(FromSValue(CreateVector(count)));
		for (long i=0; i<count; i++) {
			COleVariant vr;
			vr.vt = v.vt & ~VT_ARRAY;
			if (vr.vt == VT_VARIANT)
				sa.GetElement(i, &vr);
			else
				sa.GetElement(i, &vr.boolVal);
			AsArray(SV)->SetElement(i, FromOleVariant(vr));
		}
		return Pop();
	}
	switch (v.vt) {
	case VT_NULL: return 0;
	case VT_UI1: return CreateFixnum(v.bVal);
	case VT_BOOL: return FromBool(v.boolVal);
	case VT_I4: return CreateInteger(v.lVal);
	case VT_BSTR: return CreateString(AsOptionalString(v));
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IUnknown> iUnk(v.punkVal);
			CComPtr<IStream> iStm;
			if (iUnk.TryQueryInterface(&iStm)) {
				Push(List(S(L_UNSIGNED_BYTE), V_8));
				CStreamValue *sv = CreateStream();
				sv->m_elementType = Pop();
				sv->m_elementBits = 8;
				sv->m_bSigned = false;
				sv->m_subtype = STS_FILE_STREAM;
				sv->m_stream = new OleStandardStream(iStm);
				sv->m_flags = STREAM_FLAG_INPUT | STREAM_FLAG_OUTPUT;
				return FromSValue(sv);
			} else
				Throw(E_INVALIDARG);
		}
		break;
	default:
		Throw(E_INVALIDARG);
	}
}

COleVariant CLispEng::VCall(CP p, const vector<COleVariant>& params) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CLispThreadKeeper lispThreadKeeper(this);
	for (size_t i=0; i<params.size(); ++i)
		Push(FromOleVariant(params[i]));
	Funcall(p, params.size());
	return ToOleVariant(m_r);
}
#endif // UCFG_OLE

#if UCFG_LISP_FFI
	
CP CLispEng::CreatePointer(void *ptr) {
	CForeignPointer *fp = new(m_consMan.CreateInstance()) CForeignPointer(ptr);
	return FromSValueT(fp, TS_FF_PTR);
}

void CLispEng::F_PointerAddress() {
	void *ptr = ToPointer(Pop());
	m_r = CreateIntegerU64((uintptr_t)ptr);
}

void CLispEng::F_MakePointer() {
	BigInteger bi = ToBigInteger(Pop());
	int64_t pv;
	if (!bi.AsInt64(pv))
		E_Error();
	m_r = CreatePointer((void*)pv);
}

void CLispEng::F_ForeignAlloc() {
	void *ptr = new byte[AsPositive(Pop())];
	m_r = CreatePointer(ptr);
}

void CLispEng::F_ForeignFree() {	
	delete[] (byte*)(ToPointer(Pop()));
}


void CLispEng::F_ForeignTypeSize() {
	CP typ = Pop();
	int cb;
	switch (typ) {
	case S(L_K_CHAR):
	case S(L_K_UNSIGNED_CHAR):
	case S(L_K_UCHAR):
		cb = sizeof(char); break;
	case S(L_K_SHORT):
	case S(L_K_UNSIGNED_SHORT):
	case S(L_K_USHORT):
		cb = sizeof(short); break;
	case S(L_K_BOOLEAN):
	case S(L_K_INT):
	case S(L_K_UNSIGNED_INT):
	case S(L_K_UINT):
		cb = sizeof(int); break;
	case S(L_K_LONG):
	case S(L_K_UNSIGNED_LONG):
	case S(L_K_ULONG):
		cb = sizeof(long); break;
	case S(L_K_LONG_LONG):
	case S(L_K_UNSIGNED_LONG_LONG):
	case S(L_K_LLONG):
	case S(L_K_ULLONG):
		cb = sizeof(long long); break;
	case S(L_K_INT8):
	case S(L_K_UINT8):
		cb = 1; break;
	case S(L_K_INT16):
	case S(L_K_UINT16):
		cb = sizeof(int16_t); break;
	case S(L_K_INT32):
	case S(L_K_UINT32):
		cb = sizeof(int32_t); break;
	case S(L_K_INT64):
	case S(L_K_UINT64):
		cb = sizeof(int64_t); break;
	case S(L_K_FLOAT):
		cb = sizeof(float); break;
	case S(L_K_DOUBLE):
		cb = sizeof(double); break;
	case S(L_K_LONG_DOUBLE):
		cb = sizeof(long double); break;		
	case S(L_K_POINTER):
	case S(L_K_STRING):
		cb = sizeof(void*); break;		
	case S(L_K_OBJECT):
		cb = sizeof(CP); break;
	default:
		E_TypeErr(typ, 0); 	//!!!
	}
	m_r = CreateFixnum(cb);
}

void CLispEng::F_ForeignTypeAlignment() {
	CP typ = Pop();
	int cb;
	switch (typ) {
	case S(L_K_CHAR):
	case S(L_K_UNSIGNED_CHAR):
	case S(L_K_UCHAR):
		cb = __alignof(char); break;
	case S(L_K_SHORT):
	case S(L_K_UNSIGNED_SHORT):
	case S(L_K_USHORT):
		cb = __alignof(short); break;
	case S(L_K_INT):
	case S(L_K_UNSIGNED_INT):
	case S(L_K_UINT):
		cb = __alignof(int); break;
	case S(L_K_LONG):
	case S(L_K_UNSIGNED_LONG):
	case S(L_K_ULONG):
		cb = __alignof(long); break;
	case S(L_K_LONG_LONG):
	case S(L_K_UNSIGNED_LONG_LONG):
	case S(L_K_LLONG):
	case S(L_K_ULLONG):
		cb = __alignof(long long); break;
	case S(L_K_INT8):
	case S(L_K_UINT8):
		cb = 1; break;
	case S(L_K_INT16):
	case S(L_K_UINT16):
		cb = __alignof(int16_t); break;
	case S(L_K_INT32):
	case S(L_K_UINT32):
		cb = __alignof(int32_t); break;
	case S(L_K_INT64):
	case S(L_K_UINT64):
		cb = __alignof(int64_t); break;
	case S(L_K_FLOAT):
		cb = __alignof(float); break;
	case S(L_K_DOUBLE):
		cb = __alignof(double); break;
	case S(L_K_LONG_DOUBLE):
		cb = __alignof(long double); break;		
	case S(L_K_POINTER):
		cb = __alignof(void*); break;		
	case S(L_K_OBJECT):
		cb = __alignof(CP); break;
	default:
		E_TypeErr(typ, 0); 	//!!!
	}
	m_r = CreateFixnum(cb);
}

int CLispEng::ToForeignInt(CP arg) {
	BigInteger bi = ToBigInteger(arg);
	int64_t pv;
	if (!bi.AsInt64(pv))
		E_Error();
	return (int)pv;
}

void CLispEng::F_MemRef() {
	int offset = AsNumber(ToOptional(Pop(), V_0));
	int typ = SV;
	byte *ptr = (byte*)ToPointer(SV1)+offset;
	switch (typ) {
	case S(L_K_CHAR): m_r = CreateChar(*(char*)ptr); break;
	case S(L_K_INT): m_r = CreateInteger64(*(int*)ptr); break;
	case S(L_K_POINTER): m_r = CreatePointer(*(void**)ptr); break;
	default:
		E_Error();
	}
	SkipStack(2);
}

void CLispEng::F_MemSet() {
	int offset = AsNumber(ToOptional(Pop(), V_0));
	int typ = SV, val = SV2;
	byte *ptr = (byte*)ToPointer(SV1)+offset;
	switch (typ) {
	case S(L_K_CHAR):
	case S(L_K_UNSIGNED_CHAR):
		*(char*)ptr = Type(val)==TS_CHARACTER ? AsChar(val) : (char)AsNumber(val);
		break;
	case S(L_K_INT): *(int*)ptr = ToForeignInt(val); break;
	case S(L_K_POINTER): *(void**)ptr = ToPointer(val); break;
	default:
		Print(typ);
		E_Error();
	}
	SkipStack(3);
	m_r = val;
}

void CLispEng::WriteArg(byte *&pArgs, vector<ptr<ForeignArg> >& fargs, CP typ, CP arg) {
#ifdef X_DEBUG//!!!D
	Print(typ);
	Print(arg);
#endif
	switch (typ) {
	case S(L_K_INT):
		*(int*)pArgs = ToForeignInt(arg);
		pArgs += sizeof(int);
		break;
	case S(L_K_POINTER):
		*(void**)pArgs = ToPointer(arg);
		pArgs += sizeof(void*);
		break;
	case S(L_K_STRING):
		{
			auto a = new StringForeignArg(AsTrueString(arg));
			fargs.push_back(a);

			*(const char **)pArgs = a->Value;
			pArgs += sizeof(char*);
		}
		break;
	case S(L_K_OBJECT):
		*(CP*)pArgs = arg;
		pArgs += sizeof(CP);
		break;
	default:
		Throw(E_NOTIMPL);
	}
}

void CLispEng::F_CloseForeignLibrary() {
	HMODULE hModule = (HMODULE)ToPointer(Pop());
	for (CName2lib::iterator i(m_name2lib.begin()), e(m_name2lib.end()); i!=e; ++i) {
		if ((HMODULE)(i->second->m_dll) == hModule) {
			m_name2lib.erase(i);
			break;
		}
	}
}

void CLispEng::F_LoadForeignLibrary() {
	String path = AsTrueString(SV),
		   name = AsString(SV1);

	CForeignLibrary *lib;
	CName2lib::iterator it = m_name2lib.find(name);
	if (it != m_name2lib.end())
		lib = it->second.get();
	else
		m_name2lib[name] = lib = new CForeignLibrary(path);
	m_r = CreatePointer((HMODULE)lib->m_dll);

	SkipStack(2);
}

void CLispEng::F_ForeignSymbolPointer() {
	HMODULE hLib = (HMODULE)ToPointer(SV);
	String name = AsTrueString(SV1);
	CStringHandle nameHandle(name, hLib);
	CForeignFunction2ptr::iterator it = m_foreignFunction2ptr.find(nameHandle);	
	if (it != m_foreignFunction2ptr.end())
		m_r = it->second;
	else if (FARPROC pfn = ::GetProcAddress(hLib, name))
		m_r = m_foreignFunction2ptr[nameHandle] = CreatePointer(pfn);
	else
		m_r = 0;
	SkipStack(2);
}

CForeignLibrary& CLispEng::GetForeignLibrary(HMODULE h) {
	for (CName2lib::iterator i=m_name2lib.begin(), e=m_name2lib.end(); i!=e; ++i) {
		if ((HMODULE)i->second->m_dll == h)
			return *i->second.get();
	}
	E_Error();
}

void CLispEng::F_ForeignFuncall(size_t nArgs) {
	CP conv = m_pStack[nArgs], libh = m_pStack[nArgs+1], name = m_pStack[nArgs+2];

	FARPROC pfn;
	if (Type(name) == TS_FF_PTR)
		pfn = (FARPROC)ToPointer(name);
	else {
		String sname = AsTrueString(name);
		CForeignLibrary& lib = GetForeignLibrary((HMODULE)ToPointer(libh));
		CResID resId;
		if (int n = atoi(sname))
			resId = n;
		else
			resId = sname;
		pfn = lib.GetPointer(resId);
	}
	if (!pfn)
		E_Error();

	CP retType = (nArgs & 1) ? SV : S(L_K_VOID);
	CP *basArg = m_pStack+nArgs;

	int stackSize = 0;
	for (int i=0; i<int(nArgs-1); i+=2) {
		Push(basArg[-i-1]);
		F_ForeignTypeSize();
		stackSize += std::max((int)AsNumber(m_r), (int)sizeof(void*));
	}
	vector<ptr<ForeignArg> > fargs;
	byte *pArgsBase = (byte*)alloca(stackSize),
		  *pArgs = pArgsBase;
	for (int i=0; i<int(nArgs-1); i+=2)
		WriteArg(pArgs, fargs, basArg[-i-1], basArg[-i-2]);

	int64_t r;
	switch (conv) {
	case S(L_K_CDECL): r = CDeclTrampoline(pArgsBase, stackSize, (PFNCDecl)pfn); break;
	case S(L_K_STDCALL): r = StdCallTrampoline(pArgsBase, stackSize, (PFNStdcall)pfn); break;
	default:
		Throw(E_NOTIMPL);
	}
	switch (retType)
	{
	case S(L_K_INT): m_r = CreateInteger64((int)r); break;
	case S(L_K_VOID): m_r = 0; break;
	case S(L_K_CHAR): m_r = CreateChar((char)r); break;
	case S(L_K_OBJECT): m_r = (CP)r; break;
	default:
		Throw(E_NOTIMPL);
	}
	
	SkipStack(3+nArgs);
}

void *CLispEng::OnCallback(CP name, void *pArgs) {
	CP prop = Get(name, S(L_CALLBACK));
	if (prop == V_U)
		Throw(E_FAIL);
	CP fun = Car(prop),
	   ret = Car(Cdr(prop)),
	   sig = Cdr(Cdr(prop));

	byte *pb = (byte*)pArgs;
	int nArg = 0;
	for (CP car; SplitPair(sig, car); ++nArg) {
		switch (car) {
		case S(L_K_INT):
			Push(CreateInteger(*(int*)pb));
			pb += sizeof(int);
			break;
		default:
			Throw(E_NOTIMPL);
		}
	}
	Apply(fun, nArg);
	switch (ret) {
	case S(L_K_INT):
		{
			BigInteger bi = ToBigInteger(Pop());
			int64_t pv;
			if (!bi.AsInt64(pv))
				E_Error();
			return (void*)pv;
		}
	case S(L_K_VOID):
		return 0;
	default:
		Throw(E_NOTIMPL);
	}
}

void *CLispEng::StaticCallbackStub(CP name, void *pArgs) {
	return Lisp().OnCallback(name, pArgs);
}

void *CLispEng::CreateCallback(CP name, CP rettype, CP sig, CP conv) {
	MemoryStream qs;

	int nStackSize = 0;
	for (CP car; SplitPair(sig, car);) {
		Push(car);
		F_ForeignTypeSize();
		int sz = AsNumber(m_r);
		Push(car);
		F_ForeignTypeAlignment();
		int align = AsNumber(m_r);
		nStackSize += std::max(sz, align);
	}

	const size_t THUNK_SIZE = 32;
	byte *p = new byte[THUNK_SIZE];			// enough for thunk
	CMemWriteStream stm(Buf(p, THUNK_SIZE));

#ifdef _M_IX86
	Ia32Codegen gen(stm);
	gen.Emit(Ia32Instr::INSTR_MOV, REG_A_CX, (int32_t)name);

	gen.EmitByte(0x8D);			// lea edx, [esp+4]
	gen.EmitByte(0x54);
	gen.EmitByte(0x24);
	gen.EmitByte(0x04);

	gen.Emit(Ia32Instr::INSTR_CALL, &CLispEng::StaticCallbackStub);
	
	switch (conv) {
	case S(L_K_CDECL): gen.Emit(Ia32Instr::INSTR_RET); break;
	case S(L_K_STDCALL): gen.Emit(Ia32Instr::INSTR_RET, nStackSize); break;
	default:
		E_Error();
	}
#else
	Throw(E_NOTIMPL);
#endif
	return p;
}

void CLispEng::F_CreateCallback() {
	CP conv = SV, sig = SV1, rettype = SV2, name = SV3;
	void *ptr = CreateCallback(name, rettype, sig, conv);
	m_funPtr2Symbol[ptr] = name;
	m_r = CreatePointer(ptr);
	SkipStack(3);
}

void CLispEng::PrPointer(CP p) {
	void *ptr = ToPointer(p);
	PrOtherObj("FOREIGN-ADDRESS", "#x"+Convert::ToString(uintptr_t(ptr), "X"+Convert::ToString(sizeof(ptr)*2)));
}

void CLispEng::SaveFFI(BlsWriter& wr) {
	wr.WriteSize(m_name2lib.size());
	for (CName2lib::iterator i=m_name2lib.begin(), e=m_name2lib.end(); i!=e; ++i) {
		(BinaryWriter&)wr << i->first << i->second->m_path;
		wr.m_vForeignLib.push_back(i->second.get());
	}
}

void CLispEng::LoadFFI(const BlsReader& rd) {
	size_t size = rd.ReadSize();
	for (size_t i=0; i<size; ++i) {
		String name, path;
		rd >> name >> path;
		CForeignLibrary *lib = new CForeignLibrary(path);
		m_name2lib[name] = lib;
		rd.m_vForeignLib.push_back(lib);
	}
}

FARPROC CForeignLibrary::GetPointer(const CResID& resId) {
	CFunMap::iterator it = m_funMmap.find(resId);
	if (it != m_funMmap.end())
		return it->second;
	return m_funMmap[resId] = m_dll.GetProcAddress(resId);
}

void CForeignPointer::Write(BlsWriter& wr) {
	for (uint32_t i=0; i<wr.m_vForeignLib.size(); ++i) {
		CForeignLibrary *lib = wr.m_vForeignLib[i];
		if ((HMODULE)lib->m_dll == m_ptr) {
			(BinaryWriter&)wr << byte(1) << i;
			return;
		}
		for (CForeignLibrary::CFunMap::iterator j=lib->m_funMmap.begin(), e=lib->m_funMmap.end(); j!=e; ++j) {
			if (j->second == m_ptr) {
				(BinaryWriter&)wr << byte(2) << i << j->first;
				return;
			}
		}
	}
	(BinaryWriter&)wr << byte(0) << (uintptr_t)m_ptr;
}

void CForeignPointer::Read(const BlsReader& rd) {
	m_type = TS_EX_FF_PTR;
	byte bToLib = rd.ReadByte();
	switch (bToLib) {
	case 0:
		(BinaryReader&)rd >> (uintptr_t&)m_ptr;
		break;
	case 1:
		m_ptr = (HMODULE)rd.m_vForeignLib[rd.ReadInt32()]->m_dll;
		break;
	case 2:
		{
			int32_t nLib;
			CResID resID;
			(BinaryReader&)rd >> nLib >> resID;
			m_ptr = rd.m_vForeignLib[nLib]->GetPointer(resID);
		}
		break;
	default:
		Throw(E_FAIL);
	}
}


#endif // UCFG_LISP_FFI


} // Lisp::


