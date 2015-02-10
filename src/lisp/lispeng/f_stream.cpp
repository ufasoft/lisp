#include <el/ext.h>

#include "lispeng.h"

#if UCFG_USE_READLINE
#	define USE_READLINE_STATIC
#	include <readline/readline.h>
#	include <readline/history.h>

#	ifdef _MSC_VER
#		pragma comment(lib, "readline")
//#		pragma comment(lib, "history")
#	endif
#endif


namespace Lisp {

bool CLispEng::StreamP(CP p) {
	return Type(p)==TS_STREAM || InstanceOf(p, Spec(L_S_FUNDAMENTAL_STREAM));
}

bool CLispEng::TerminalStreamP(CP p) {
	if (Type(p) != TS_STREAM)
		return false;
	if (p == Spec(L_S_TERMINAL_READ_STREAM))
		return true;
	return false;
	/*!!!speed
	CStreamValue *sv = AsStream(p);
	if (!sv->m_stream)
		return false;
	return sv->m_stream->IsInteractive();
	*/
}

void CLispEng::F_StreamP() {
	m_r = FromBool(StreamP(Pop()));
}

void CLispEng::F_BuiltInStreamP() {
	m_r = FromBool(Type(Pop()) == TS_STREAM);
}

void CLispEng::F_MakeStream() {
	CStreamValue *stm = CreateStream();
	stm->m_reader = ToOptionalNIL(Pop());
	stm->m_writer = ToOptionalNIL(Pop());
	stm->m_out = Pop();
	m_r = FromSValue(stm);
}

CStreamValue *CLispEng::GetTargetStream(CP p) {
	CStreamValue *stm = ToStream(p);
	while (stm->m_subtype == STS_SYNONYM_STREAM)
		stm = ToStream(SymbolValue(stm->m_in));
	return stm;
}

void CLispEng::F_InputStreamP() {
	CP p = Pop();
	if (InstanceOf(p, Spec(L_S_FUNDAMENTAL_INPUT_STREAM)))
		m_r = V_T;
	else {
		CStreamValue *stm = GetTargetStream(p);
		if (!(m_r=FromBool(stm->m_flags & STREAM_FLAG_INPUT))) {
			StandardStream *stream = stm->m_stream;
			if (!stream && stm->m_nStandard!=-1)
				stream = m_streams[stm->m_nStandard];
			if (stream)
				m_r = FromBool(stream->CanRead);
		}
	}
}

void CLispEng::F_OutputStreamP() {
	CP p = Pop();
	if (InstanceOf(p, Spec(L_S_FUNDAMENTAL_OUTPUT_STREAM)))
		m_r = V_T;
	else {
		CStreamValue *stm = GetTargetStream(p);
		if (!(m_r=FromBool(stm->m_flags & STREAM_FLAG_OUTPUT))) {
			StandardStream *stream = stm->m_stream;
			if (!stream && stm->m_nStandard!=-1)
				stream = m_streams[stm->m_nStandard];
			if (stream)
				m_r = FromBool(stream->CanWrite);
		}
	}
}

void CLispEng::EnsureInputStream(CP p) {
	Push(p);
	F_InputStreamP();
	if (!m_r)
		E_TypeErr(p, Spec(L_S_TYPE_INPUT_STREAM));
}

void CLispEng::EnsureOutputStream(CP p) {
	Push(p);
	F_OutputStreamP();
	if (!m_r)
		E_TypeErr(p, Spec(L_S_TYPE_OUTPUT_STREAM));
}

void CLispEng::F_MakeEchoStream() {
	EnsureOutputStream(SV);
	EnsureInputStream(SV1);
	m_r = FromSValue(CreateStream(STS_ECHO_STREAM, SV1, SV, STREAM_FLAG_INPUT|STREAM_FLAG_OUTPUT));
	SkipStack(2);
}

void CLispEng::F_MakeSynonymStream() {
	ToSymbol(SV);
	m_r = CreateSynonymStream(Pop());
}

CStreamValue *CLispEng::CheckStreamType(CP p, int sts) {
	CStreamValue *stm = ToStream(p);
	if (stm->m_subtype != sts)
		E_StreamErr(p);
	return stm;
}

void CLispEng::PopStreamIn(int sts) {
	m_r = CheckStreamType(Pop(), sts)->m_in;
}

void CLispEng::PopStreamOut(int sts) {
	m_r = CheckStreamType(Pop(), sts)->m_out;
}

void CLispEng::F_EchoStreamInputStream() {
	PopStreamIn(STS_ECHO_STREAM);
}

void CLispEng::F_EchoStreamOutputStream() {
	PopStreamOut(STS_ECHO_STREAM);
}

void CLispEng::F_SynonymStreamSymbol() {
	PopStreamIn(STS_SYNONYM_STREAM);
}

void CLispEng::F_MakeTwoWayStream() {
	EnsureOutputStream(SV);
	EnsureInputStream(SV1);
	m_r = FromSValue(CreateStream(STS_TWO_WAY_STREAM, SV1, SV, STREAM_FLAG_INPUT|STREAM_FLAG_OUTPUT));
	SkipStack(2);
}

void CLispEng::F_TwoWayStreamInputStream() {
	PopStreamIn(STS_TWO_WAY_STREAM);
}

void CLispEng::F_TwoWayStreamOutputStream() {
	PopStreamOut(STS_TWO_WAY_STREAM);
}

void CLispEng::F_MakeBroadcastStream(size_t nArgs) {
	for (size_t i=nArgs; i--;)
		EnsureOutputStream(m_pStack[i]);
	m_r = FromSValue(CreateStream(STS_BROADCAST_STREAM, 0, Listof(nArgs), STREAM_FLAG_OUTPUT));
}

void CLispEng::F_BroadcastStreamStreams() {
	PopStreamOut(STS_BROADCAST_STREAM);
}

void CLispEng::F_MakeConcatenatedStream(size_t nArgs) {
	for (size_t i=nArgs; i--;)
		EnsureInputStream(m_pStack[i]);
	m_r = FromSValue(CreateStream(STS_CONCATENATED_STREAM, Listof(nArgs), 0, STREAM_FLAG_INPUT));
}

void CLispEng::F_ConcatenatedStreamStreams() {
	PopStreamIn(STS_CONCATENATED_STREAM);
}

pair<size_t, size_t> CLispEng::PopStringBoundingIndex(CP str) {
	if (!StringP(str))
		E_TypeErr(str, S(L_STRING));
	size_t len;
	return PopSeqBoundingIndex(str, len);
}

void CLispEng::CharToStream(CP c, CP stm) {
	if (Type(stm) == TS_OBJECT)
		Call(S(L_STREAM_WRITE_CHAR), stm, c);
	else {
		wchar_t ch = AsChar(c);
		CStreamValue *sv;
		if (sv = CastToPPStream(stm)) {
			switch (ch) {
			case '\n':
				sv->m_bMultiLine = true;
				ConsPPString(sv, 0);
				break;
			case ' ':
			case '\t':
				if (Spec(L_S_PRINT_PRETTY_FILL)) {
					CP q = sv->m_out;
					if (SeqLength(Car(q)) || !ConsP(Cdr(q)) || !ConsP(Car(Cdr(q))) || Car(Car(Cdr(q)))!=S(L_K_FILL)) {
						Push(c, Car(q), V_U);
						F_VectorPushExtend();
						ConsPPString(sv, S(L_K_FILL));
					}
					break;
				}
			default:
				Push(c, Car(sv->m_out), V_U);
				F_VectorPushExtend();
			}
		} else {
			sv = AsStream(stm);
			if (sv->m_writer)
				Call(sv->m_writer, c, sv->m_out);
			else
				try {
					GetOutputTextStream(sv)->put(ch);
				} catch (RCExc) {
					E_Error();
				}
		}
		sv = AsStream(stm);
		switch (ch) {
		case '\n':
			sv->LinePosRef() = 0;
			break;
		case '\b':
			if (StandardStream *ss = GetOutputTextStream(sv))
				if (ss->IsInteractive()) {
					if (sv->LinePosRef())
						sv->LinePosRef()--;
					break;
				}
		default:
			sv->LinePosRef()++;
		}
	}
}

void CLispEng::WriteChar(wchar_t ch) {
	struct CStreamHandler : public COutputStreamHandler {
		CP m_char;

		void Fun(CP p) {
			LISP_GET_LISPREF.CharToStream(m_char, p);
		}
	} sh;
	sh.m_char = CreateChar(ch);
	ProcessOutputStream(m_stm, sh);
}

void CLispEng::F_WriteChar() {
	CStm sk(TestOStream(SV));
	WriteChar(AsChar(SV1));
	m_r = SV1;
	SkipStack(2);
}

void CLispEng::WritePChar(const char *p) {
	for (; *p; p++)
		WriteChar(*p);
}

void CLispEng::WriteStr(RCString s) {
	size_t len = s.length();
	for (size_t i=0; i<len; ++i)
		WriteChar(s[i]);
}

void CLispEng::F_Write() {
	CP *pStack = m_pStack+15;
	CP p = pStack[1],
		stm = TestOStream(pStack[0]);
#ifdef X_DEBUG//!!!D
	if ((p & 0xFF) >= 0x20) {
		p = p;
		E_Error();
	}
#endif

	{
		CDynBindFrame bind;
		size_t count = 0;
		for (int i=0; i<15; i++) {
			CP v = pStack[-1-i];
			if (v != V_U) {
				bind.Bind(_self, get_Sym(CLispSymbol(ENUM_L_S_PRINT_ARRAY+i)), v);
				count++;
			}
		}
		bind.Finish(_self, count);
		CStm sk(stm);
		Prin1(p);
	}
	m_pStack = pStack+2;
	m_r = p;
	m_cVal = 1;
}

void CLispEng::F_WriteString() {
	struct CStreamHandler : public COutputStreamHandler
	{
		CP m_s;
		pair<size_t, size_t> m_bi;
		CP m_char;

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			for (size_t i=m_bi.first; i<m_bi.second; i++)
				lisp.CharToStream(lisp.AsArray(m_s)->GetElement(i), p);
		}
	} sh;
	sh.m_s = SV3;
	sh.m_bi = PopStringBoundingIndex(SV3);
	ProcessOutputStream(TestOStream(SV), sh);
	SkipStack(1);
	m_r = Pop();
}

void CLispEng::F_MakeStringInputStream() {
	pair<size_t, size_t> bi = PopStringBoundingIndex(SV2);
	CStreamValue *stm = CreateStream(STS_STRING_STREAM, Pop(), 0, STREAM_FLAG_INPUT);
	stm->m_nCur = bi.first;
	stm->m_nEnd = bi.second;
	m_r = FromSValue(stm);
}

/*!!!D
CP CLispEng::CreateOutputStringStream() {
CStreamValue *stm = m_streamMan.CreateInstance();
stm->m_pStm.SetOwned(new CStringOTextStream);
stm->m_subtype = STS_STRING_STREAM;
return FromSValue(stm);
}
*/

void CLispEng::F_MakeStringPushStream() {
	CP pPos = ToOptional(Pop(), V_0);
	//!!!  m_r = CreateOutputStringStream();
	CStreamValue *stm = CreateStream();
	if (pPos)
		stm->LinePosRef() = AsPositive(pPos);
	stm->m_writer = S(L_VECTOR_PUSH_EXTEND);
	if (!(stm->m_out = Pop()))
		AsArray(stm->m_out = CreateString(""))->m_fillPointer = V_0;
	stm->m_subtype = STS_STRING_STREAM;
	stm->m_flags = STREAM_FLAG_OUTPUT;
	m_r = FromSValue(stm);
}

void CLispEng::F_MakePphelpStream() {
	m_r = FromSValue(CreateStream(STS_PPHELP_STREAM, 0, ConsPPString(0, 0), STREAM_FLAG_OUTPUT));
}

CP CLispEng::CInputStreamHandler::OnConcatenated(CP p) {
	CLispEng& lisp = LISP_GET_LISPREF;
	lisp.Push(p);
	for (CP stms, car; lisp.SplitPair(stms=lisp.AsStream(p)->m_in, car); lisp.AsStream(p)->m_in=stms) {
		lisp.ProcessStream(car, _self);
		if (lisp.m_r != S(L_K_EOF)) {
			lisp.SkipStack(1);
			return car;
		}
	}
	lisp.SkipStack(1);
	return 0;
}

void CLispEng::CInputStreamHandler::OnEcho(CP p) {
	CLispEng& lisp = LISP_GET_LISPREF;
	lisp.Push(p);
	lisp.ProcessStream(lisp.AsStream(p)->m_in, _self);
	if (lisp.m_r != S(L_K_EOF)) {
		lisp.Push(lisp.m_r);
		FunOut(lisp.AsStream(p)->m_out, lisp.m_r);
		lisp.m_r = lisp.Pop();
	}
	lisp.SkipStack(1);
}

CP CLispEng::CInputStreamHandler::OnTwoWay(CP p) {
	return LISP_GET_LISPREF.AsStream(p)->m_in;
}

CP CLispEng::COutputStreamHandler::OnTwoWay(CP p) {
	return LISP_GET_LISPREF.AsStream(p)->m_out;
}

void CLispEng::ProcessStream(CP p, IStreamHandler& sh) {
	while (Type(p) == TS_STREAM) {
		CStreamValue *stm = AsStream(p);
		switch (stm->m_subtype) {
		case STS_SYNONYM_STREAM:
			p = SymbolValue(stm->m_in);
			continue;
		case STS_CONCATENATED_STREAM:
			{
				CP q = sh.OnConcatenated(p);
				if (sh.m_bReturn)
					return;
				if (q == p)
					break;
				p = q;
			}
			continue;
		case STS_ECHO_STREAM:
			sh.OnEcho(p);
			return;
		case STS_TWO_WAY_STREAM:
			{
				CP q = sh.OnTwoWay(p);
				if (q == p)
					break;
				p = q;
			}
			continue;
		case STS_BROADCAST_STREAM:
			for (CP p1=stm->m_out, car; SplitPair(p1, car);)
				ProcessStream(car, sh);
			return;
		}
		break;
	}
	sh.Fun(p);
}

void CLispEng::ProcessInputStream(CP p, IStreamHandler& sh) {
	Push(p);
	F_InputStreamP();
	if (!m_r)
		E_Error();
	ProcessStream(p, sh);
}

void CLispEng::ProcessOutputStream(CP p, IStreamHandler& sh) {
	Push(p);
	F_OutputStreamP();
	if (!m_r) {
#ifdef _DEBUG//!!!D
		Push(p);
		F_OutputStreamP();
#endif
		E_Error();
	}
	ProcessStream(p, sh);
}

void CLispEng::ProcessOutputStreamDesignator(CP p, IStreamHandler& sh) {
	ProcessStream(TestOStream(p), sh);
}

void CLispEng::F_BuiltInStreamElementType() {
	struct CStreamHandler : public IStreamHandler {
		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_ELEMENT_TYPE), p);
			else
				lisp.m_r = lisp.AsStream(p)->m_elementType;
		}
	} sh;
	ProcessStream(Pop(), sh);
}

void CLispEng::F_InteractiveStreamP() {
	struct CStreamHandler : public IStreamHandler {
		CP OnConcatenated(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;			
			CP in = lisp.AsStream(p)->m_in;
			return in ? lisp.Car(in) : p;
		}

		CP OnTwoWay(CP p) {
			return LISP_GET_LISPREF.AsStream(p)->m_out;
		}

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.m_r = 0;
			else if (StandardStream *ss = lisp.GetOutputTextStream(lisp.AsStream(p)))
				lisp.m_r = FromBool(ss->IsInteractive());
			else
				lisp.m_r = 0;
		}
	} sh;
	ProcessStream(Pop(), sh);

	/*!!!
	if (StandardStream *ss = GetOutputTextStream(GetTargetStream(Pop())))
	m_r = FromBool(ss->IsInteractive());
	*/
}

void CLispEng::CheckFlushFileStream(CStreamValue *stm) {
	if (stm->m_bNeedFlushOctet && stm->m_curOctet != -1) {
		stm->m_bNeedFlushOctet = false;
		stm->m_stream->WriteByte(byte(exchange(stm->m_curOctet, -1)));
	}
}

void CLispEng::F_FilePosition() {
	CP pos = Pop(),
		p = Pop();

	struct CStreamHandler : public IStreamHandler {
		CP m_pos;

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_POSITION), p, lisp.ToOptionalNIL(m_pos));
			else {
				CStreamValue *stm = lisp.CheckStreamType(p, STS_FILE_STREAM);
				if (stm->m_bClosed)
					lisp.E_StreamErr(IDS_E_FileClosed, p, p);
				StandardStream *ss = stm->m_stream;
				if (m_pos == V_U) {
					if (stm->LinePosRef() == -1) {
						int64_t pos = ss->Seek(0, SeekOrigin::Current);
						stm->LinePosRef() = pos*8/stm->m_elementBits;
					}
					lisp.m_r = lisp.CreateInteger64(stm->LinePosRef());
				} else {
					int64_t off = 0;
					lisp.CheckFlushFileStream(stm);
					SeekOrigin origin = SeekOrigin::Begin;
					if (m_pos == S(L_K_START)) {
						stm->LinePosRef() = 0;
					} else if (m_pos == S(L_K_END)) {
						origin = SeekOrigin::End;
						stm->LinePosRef() = -1;
					} else {
						BigInteger i = lisp.ToBigInteger(m_pos);
						if (Sign(i) == -1)
							E_Error();
						if (!i.AsInt64(off))
							E_Error();
						stm->LinePosRef() = off;
						off = off*stm->m_elementBits/8;
					}
					lisp.m_r = FromBool(ss->Seek(off, origin) != -1);
				}
			}
		}
	} sh;
	sh.m_pos = pos;
	ProcessStream(p, sh);
}

void CLispEng::F_StringInputStreamIndex() {
	CStreamValue *stm = CheckStreamType(Pop(), STS_STRING_STREAM);
	size_t pos = stm->m_nCur;
	if (stm->m_bUnread)
		pos--;
	m_r = CreateFixnum(pos);
}

void CLispEng::F_GetOutputStreamString() {
	CStreamValue *stm = CheckStreamType(SV, STS_STRING_STREAM);
	m_r = stm->m_out;
#ifdef _X_DEBUG
	CArrayValue *av = ToArray(m_r);
	static i;
	if (++i == 2)
	{
		PrintForm(cerr, m_r);
		E_Error();
	}
#endif
	CArrayValue *vec = CreateVector(0, ToArray(stm->m_out)->GetElementType());
	vec->m_fillPointer = V_0;
	AsStream(SV)->m_out = FromSValue(vec);

	/*!!!  if (stm->m_subtype != STS_STRING_STREAM || !(stm->m_pStm->m_flags|STREAM_FLAG_OUTPUT))
	E_Error();
	CStringOTextStream* pOS = (CStringOTextStream*)stm->m_pStm.get();
	pOS->m_ios << ends;
	m_r = CreateString(((strstreambuf*)pOS->m_ios.rdbuf())->str());
	stm->m_pStm.SetOwned(new CStringOTextStream);*/
	SkipStack(1);
}

void CLispEng::F_FileLength() {
	CStreamValue *stm = GetTargetStream(Pop());
	if (stm->m_subtype != STS_FILE_STREAM || stm->m_bClosed)
		E_StreamErr(FromSValue(stm));
	m_r = CreateInteger64(stm->m_stream->Length);
}

void CLispEng::F_Open() {
	CP externalFormat = ToOptional(Pop(), S(L_K_DEFAULT)),
		ifNotExists = Pop(),
		ifExists = Pop(),
		elementType = ToOptional(Pop(), S(L_CHARACTER)),
		direction = ToOptional(Pop(), S(L_K_INPUT));

	F_Pathname();
	CP originalPathname = m_r, pathname = originalPathname;
	Push(originalPathname);

	if (AsSymbol(S(L_MERGE_PATHNAMES))->GetFun()) {			// during boot this function is not defined
		Call(S(L_MERGE_PATHNAMES), pathname);
		pathname = SV = m_r;
	}

	path name;
	if (AsSymbol(S(L_NAMESTRING))->GetFun()) {
		Call(S(L_NAMESTRING), pathname);
		name = AsString(m_r);
	} else
		name = ToPathname(pathname)->ToString();
	
	CPathname *pn = ToPathname(pathname);
		
	TRC(2, name);

	if (m_bInit)
		m_arModule.push_back(name);

#if UCFG_WCE			//!!!? already merged
	if (!Path::IsPathRooted(name))
		name = Path::Combine(AsPathname(Spec(L_S_DEFAULT_PATHNAME_DEFAULTS))->ToString(), name);
#endif

	if (ifExists == V_U) {
		if (direction == S(L_K_OUTPUT) || direction == S(L_K_IO))
			ifExists = S(L_K_SUPERSEDE);							// reasonable, but it's unclear in the HyperSpec
		else
			ifExists = pn->m_ver==S(L_K_NEWEST) ? S(L_K_NEW_VERSION) : S(L_K_ERROR);
	}

	if (ifNotExists == V_U) {
		if (direction==S(L_K_INPUT) || direction==S(L_K_INPUT_IMMUTABLE) ||
			direction==S(L_K_IO) && (ifExists==S(L_K_OVERWRITE) || ifExists==S(L_K_APPEND))) {
			ifNotExists = S(L_K_ERROR);
		} else if (direction==S(L_K_OUTPUT) || direction==S(L_K_IO))
			ifNotExists = S(L_K_CREATE);
		else
			ifNotExists = 0;
	}

	LispFileStream *fs = new LispFileStream;
	ptr<StandardStream> pfs = fs;

	if (StringP(externalFormat))
		fs->Encoding.reset(Encoding::GetEncoding(AsString(externalFormat)));
	
	FileAccess access = FileAccess::Read;
	switch (direction) {
	case S(L_K_INPUT):
	case S(L_K_INPUT_IMMUTABLE):
	case S(L_K_PROBE):
		access = FileAccess::Read;
		break;
	case S(L_K_OUTPUT):
		access = FileAccess::Write;
		break;
	case S(L_K_IO):
		access = FileAccess::ReadWrite;
		break;
	default:
		E_TypeErr(direction, S(L_K_DIRECTION));
	}

	switch (ifExists) {
	case S(L_K_ERROR):
	case S(L_K_NEW_VERSION):
	case S(L_K_RENAME):
	case S(L_K_RENAME_AND_DELETE):
	case S(L_K_OVERWRITE):	
	case S(L_K_APPEND):
	case S(L_K_SUPERSEDE):
	case 0:
		break;
	default:
		E_TypeErr(ifExists, S(L_K_IF_EXISTS));
	}

	switch (ifNotExists) {
	case S(L_K_ERROR):
	case S(L_K_CREATE):
	case 0:
		break;
	default:
		E_TypeErr(ifNotExists, S(L_K_IF_DOES_NOT_EXIST));
	}

	fs->m_stm.TextMode = elementType==S(L_CHARACTER) || elementType==S(L_BASE_CHAR);

	FileMode mode = FileMode::Open;

	if (int(access) & int(FileAccess::Write)) {
		switch (ifExists) {
		case S(L_K_APPEND):			mode = FileMode::Append; break;
		case S(L_K_OVERWRITE):	
			if (ifNotExists == S(L_K_ERROR))
				mode = FileMode::Open;
			else
				mode = FileMode::OpenOrCreate;			
			break;

		case S(L_K_SUPERSEDE):
		case S(L_K_NEW_VERSION):
			mode = FileMode::Create;
			break;

		case S(L_K_RENAME_AND_DELETE):
			{
				error_code ec;
				sys::rename(name, Path::GetTempFileName(name.parent_path(), name.filename()).first, ec);
			}
		case S(L_K_ERROR):
		case 0:
			mode = FileMode::CreateNew;
			break;
		}	
	}	


	//!!!    if (ExtractFilePath(name) == "")
	//!!!      name = m_initDir+name;
	try {
		DBG_LOCAL_IGNORE_CONDITION(errc::no_such_file_or_directory);
		DBG_LOCAL_IGNORE(E_ACCESSDENIED);
		DBG_LOCAL_IGNORE(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));

		fs->Open(name, mode, access);
	} catch (RCExc ex) {
		m_r = 0;
		if (mode == FileMode::CreateNew && ifExists || ifNotExists == S(L_K_ERROR)) {
			E_FileErr(HResultInCatch(ex), ex.what(), pathname);
		}
	}

	if (fs->m_stm.m_fstm) {
		if (direction == S(L_K_PROBE))
			fs->m_stm.Close();
		CStreamValue *stm = CreateStream();
		m_r = FromSValue(stm);
		stm->m_subtype = STS_FILE_STREAM;
		stm->m_mode = mode;				
		stm->TrueName = Path::GetTruePath(name);

		if (elementType==S(L_SIGNED_BYTE) || elementType==S(L_UNSIGNED_BYTE))
			elementType = List(elementType, V_8);
		stm->m_elementType = elementType;
		if (stm->m_elementType == S(L_BIT))
			stm->m_elementBits = 1;
		else if (ConsP(stm->m_elementType) && (Car(stm->m_elementType)==S(L_SIGNED_BYTE) || Car(stm->m_elementType)==S(L_UNSIGNED_BYTE))) {
			stm->m_elementBits = AsPositive(Car(Cdr(stm->m_elementType)));
			stm->m_bSigned = Car(stm->m_elementType) == S(L_SIGNED_BYTE);
		}

		stm->m_stream = pfs;
		stm->m_pathname = originalPathname;
		stm->m_flags = (int(access) & int(FileAccess::Read) ? STREAM_FLAG_INPUT : 0) | (int(access) & int(FileAccess::Write) ? STREAM_FLAG_OUTPUT : 0);		
	}
	SkipStack(1);
}

void CLispEng::F_BuiltInStreamOpenP() {
	m_r = FromBool(!ToStream(Pop())->m_bClosed);
}

void CLispEng::F_Close() {						// sys::built-in-stream-close
	CP abort = ToOptionalNIL(Pop());		//!!! ignored
	Push(SV);
	F_BuiltInStreamOpenP();
	CStreamValue *stm = ToStream(SV);
	if (m_r) {
		stm->m_bClosed = true;
		if (stm->m_stream) {
			if (!abort)
				CheckFlushFileStream(stm);
			stm->m_stream->Close();
			stm->m_stream = nullptr;
		}
		if (abort && (stm->m_mode==FileMode::Create || stm->m_mode==FileMode::CreateNew)) {
			error_code ec;
			sys::remove(stm->TrueName);
			m_r = V_T;		
		}
	}
	SkipStack(1);
}

CStreamValue *CLispEng::CheckOpened(CStreamValue *stm) {
	if (stm->m_bClosed)
		E_Error();
	return stm;
}

CP __fastcall CLispEng::TestIStream(CP p) {
	p = ToOptionalNIL(p);
	switch (Type(p)) {
	case TS_CONS:
		if (p)
			E_Error();
		p = Spec(L_S_STANDARD_INPUT);
		break;
	case TS_SYMBOL:
		if (p != V_T)
			E_Error();
		p = Spec(L_S_TERMINAL_IO);
	default:
		if (!StreamP(p))
			E_TypeErr(p, S(L_STREAM));
	}
	if (Type(p) == TS_STREAM)
		CheckOpened(AsStream(p));
	return p;
}

CP __fastcall CLispEng::TestOStream(CP p) {
	p = ToOptionalNIL(p);
	switch (Type(p)) {
	case TS_CONS:
		if (p)
			E_Error();
		p = Spec(L_S_STANDARD_OUTPUT);
		break;
	case TS_SYMBOL:
		if (p != V_T)
			E_Error();
		p = Spec(L_S_TERMINAL_IO);
	default:
		if (!StreamP(p))
			E_TypeErr(p, S(L_STREAM));
	}
	if (Type(p) == TS_STREAM)
		CheckOpened(AsStream(p));
	return p;
}

CP CLispEng::GetOutputStream(CP p) {
	while (true) {
		CP s = TestOStream(p);
		if (Type(s) == TS_STREAM) {
			CStreamValue *stm = AsStream(s);
			switch (stm->m_subtype) {
			case STS_ECHO_STREAM:
			case STS_TWO_WAY_STREAM:
				p = stm->m_out;
				continue;
			case STS_SYNONYM_STREAM:
				p = SymbolValue(stm->m_in);
				continue;
			}
		}
		return s;
	}
}

CP CLispEng::GetUniStream(CP p) {
	struct CStreamHandler : public IStreamHandler
	{
		CP m_result;

		void Fun(CP p)
		{
			m_result = p;
		}
	} sh;
	ProcessStream(p, sh);
	return sh.m_result;
}

void CLispEng::F_StreamExternalFormat() {
	GetUniStream(Pop());
	m_r = S(L_K_DEFAULT);
}

void CLispEng::ReadCharHelper(CP stm1, bool bEofErr, CP eofVal, bool bRec, bool bNoHang, CP gen) {
	ClearResult();

	struct CStreamHandler : public CInputStreamHandler {
		bool m_bNoHang;
		CP m_gen;
		bool m_bEcho;

		CStreamHandler()
			:	m_bEcho(true)
		{}

		void FunOut(CP s, CP val) {
			if (m_bEcho) {
				CStm sk(s);
				LISP_GET_LISPREF.WriteChar(AsChar(val));
			}
		}

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(m_gen, p);
			else {
				CStreamValue *stm = lisp.AsStream(p);
				if (stm->m_bUnread) {
					lisp.m_r = stm->m_char;
					stm->m_bUnread = false;
					m_bEcho = false;
					m_bReturn = true;
				} else {
					lisp.m_r = S(L_K_EOF);
					if (stm->m_subtype == STS_STRING_STREAM) {
						if (stm->m_nCur < stm->m_nEnd)
							lisp.m_r = lisp.ToArray(stm->m_in)->GetElement(stm->m_nCur++);
					} else {
						if (stm->m_reader) {
							lisp.Call(stm->m_reader, p);
							stm = lisp.AsStream(p);
						} else {
							StandardStream *ts = lisp.GetInputTextStream(stm);
							if (!m_bNoHang || ts->Listen()) {
								int ch;
								try {
									ch = ts->get();
								} catch (RCExc ex) {
									if (HResultInCatch(ex) == E_EXT_NormalExit)
										throw;
									E_Error();
								}
								if (ch != EOF)
									lisp.m_r = CreateChar((wchar_t)ch);
							} else
								lisp.m_r = 0;
						}
					}
					if (Type(lisp.m_r) == TS_CHARACTER) {	
						m_bReturn = true;
						stm->m_char = lisp.m_r;
						if (AsChar(lisp.m_r) == '\n')
							stm->m_nLine++;
					}
				}
			}
		}
	} sh;
	sh.m_bNoHang = bNoHang;
	sh.m_gen = gen;
	ProcessInputStream(stm1, sh);
	if (m_r == S(L_K_EOF)) {
		if (bEofErr)
			E_EndOfFileErr(stm1);
		m_r = eofVal;
	}
}

void CLispEng::F_ReadChar() {
	ReadCharHelper(TestIStream(SV3), ToOptional(SV2, V_T), ToOptionalNIL(SV1), ToOptionalNIL(SV), false, S(L_STREAM_READ_CHAR));
	SkipStack(4);
}

int CLispEng::ReadChar(CP stm, bool bEofErr) {
	ReadCharHelper(stm, bEofErr, 0, true, false, S(L_STREAM_READ_CHAR));
	return m_r ? AsChar(m_r) : EOF;
}

void CLispEng::F_ReadCharNoHang() {
	ReadCharHelper(TestIStream(SV3), ToOptional(SV2, V_T), ToOptionalNIL(SV1), ToOptionalNIL(SV), true, S(L_STREAM_READ_CHAR_NO_HANG)); //!!!
	SkipStack(4);
}

void CLispEng::F_Listen() {
	struct CStreamHandler : public CInputStreamHandler {
		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_LISTEN), p);
			else {
				CStreamValue *stm = lisp.AsStream(p);
				if (stm->m_bUnread)
					lisp.m_r = V_T;
				else if (stm->m_subtype==STS_STRING_STREAM)
					lisp.m_r = FromBool(stm->m_nCur < stm->m_nEnd);
				else {
					lisp.Push(p);
					lisp.F_InteractiveStreamP();
					stm = lisp.AsStream(p);
					if (!lisp.m_r)
						if (StandardStream *ts = lisp.GetInputTextStream(stm))
							lisp.m_r = FromBool(ts->Listen());
				}
			}
		}
	} sh;
	ProcessInputStream(TestIStream(Pop()), sh);
}

void CLispEng::F_ClearInput() {
	struct CStreamHandler : public CInputStreamHandler {
		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_CLEAR_INPUT), p);
			else {
				CStreamValue *stm = lisp.AsStream(p);
				stm->m_bUnread = false; //!!!
				if (StandardStream *ts = lisp.GetInputTextStream(stm))
					ts->ClearInput();
			}
		}
	} sh;
	ProcessInputStream(TestIStream(Pop()), sh);
	m_r = 0;
}

void CLispEng::F_UnreadChar() {
	ClearResult();
	SV = ToOptionalNIL(SV);
	AsChar(SV1);
	struct CStreamHandler : public CInputStreamHandler {
		CP m_char;

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_UNREAD_CHAR), p, m_char);
			else {
				CStreamValue *stm = lisp.AsStream(p);
				if (stm->m_bUnread || stm->m_char && stm->m_char!=m_char)
					E_Error();
				stm->m_bUnread = true;
				m_bReturn = true;
			}
		}
	} sh;
	sh.m_char = SV1;
	ProcessInputStream(TestIStream(SV), sh);
	m_r = 0;
	SkipStack(2);
}

void CLispEng::PutBackChar(CP stm, wchar_t ch) {
	Push(CreateChar(ch), stm);
	F_UnreadChar();
}

void CLispEng::F_PeekChar() {
	ClearResult();
	SV = ToOptionalNIL(SV);
	CP eofVal = SV1 = ToOptionalNIL(SV1),
		eofErr = SV2 = ToOptional(SV2, V_T),
		stm = SV3,
		typ = SV4 = ToOptionalNIL(SV4);
	if (typ && typ!=V_T)
		AsChar(typ);
	while (true) {
		struct CStreamHandler : public CInputStreamHandler {
			CP m_char;

			void Fun(CP p) {
				CLispEng& lisp = LISP_GET_LISPREF;
				if (Type(p) == TS_OBJECT)
					lisp.Call(S(L_STREAM_PEEK_CHAR), p);
				else {
					lisp.Push(p, 0, S(L_K_EOF), V_T);
					lisp.F_ReadChar();
					if (lisp.m_r != S(L_K_EOF)) {
						lisp.Push(lisp.m_r, lisp.m_r, p);
						lisp.F_UnreadChar();
						lisp.m_r = lisp.Pop();
					}
				}
			}
		} sh;
		ProcessInputStream(TestIStream(stm), sh);
		if (m_r == S(L_K_EOF)) {
			if (eofErr)
				E_EndOfFileErr(stm);
			m_r = eofVal;
			break;
		}
		if (!typ || typ==m_r)
			break;
		if (typ==V_T && GetCharType(AsChar(m_r)).m_syntax!=ST_WHITESPACE)
			break;
		Push(stm, 0, S(L_K_EOF), V_T);
		F_ReadChar();
	}
	SkipStack(5);
}

StandardStream *CLispEng::GetInputTextStream(CStreamValue *stm) {
	if (stm->m_subtype != STS_STREAM && stm->m_subtype != STS_FILE_STREAM)
		return 0;
	if (stm->m_stream)
		return stm->m_stream;
	if (stm->m_nStandard == -1)
		E_Error();
	StandardStream *ts = m_streams[stm->m_nStandard];
	if (!ts)
		ts = m_streams[STM_StandardInput];
	return ts;
}

StandardStream *CLispEng::GetOutputTextStream(CStreamValue *stm) {
	if (stm->m_subtype != STS_STREAM && stm->m_subtype != STS_FILE_STREAM)
		return 0;
	if (stm->m_stream)
		return stm->m_stream;
	if (stm->m_nStandard == -1)
		E_Error();
	StandardStream *ts = m_streams[stm->m_nStandard];
	if (!ts)
		ts = m_streams[STM_StandardOutput];
	return ts;
}

void CLispEng::F_WriteByte() {
	struct CStreamHandler : public COutputStreamHandler {
		CP m_byte;

		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_WRITE_BYTE), p, m_byte);
			else {
				CStreamValue *sv = lisp.AsStream(p);
				if (StandardStream *ts = lisp.GetInputTextStream(sv)) {
					if (sv->LinePosRef() == -1) {
						lisp.Push(p, V_U);
						lisp.F_FilePosition();
					}
					lisp.TypepCheck(m_byte, sv->m_elementType);

					int bits = (sv->LinePosRef() * sv->m_elementBits) & 7;
					int restBits = sv->m_elementBits & 7;

					int count = sv->m_elementBits/8+1;
					byte *buf = (byte*)alloca(count);
					BigInteger bi = lisp.ToBigInteger(m_byte);
					size_t n = bi.ToBytes(buf, count);
					memset(buf+n, ((buf[n-1]&0x80) ? -1 : 0), count-n);

					int nFull = (sv->m_elementBits+bits)/8;
					for (int i=0; i<nFull; ++i) {						
						ts->WriteByte(byte(sv->m_curOctet & ((1 << bits)-1) | (buf[i]<<bits)));
						sv->m_curOctet = buf[i]>>(8-bits);
					}

					int newBits = (restBits+bits) & 7;
					if (sv->m_bNeedFlushOctet = newBits)
						sv->m_curOctet = (sv->m_curOctet & ((1 << bits)-1) | (buf[nFull]<<bits)) & ((1<<newBits)-1);
					else
						sv->m_curOctet = -1;

					sv->LinePosRef()++;
				} else
					E_Error();
			}
		}
	} sh;
	if (!IntegerP(SV1))
		E_TypeErr(SV1, S(L_INTEGER));
	sh.m_byte = SV1;
	ProcessOutputStream(SV, sh);
	m_r = SV1;
	SkipStack(2);
}

void CLispEng::F_ReadByte() {
	SV = ToOptionalNIL(SV);
	struct CStreamHandler : public CInputStreamHandler {
		void Fun(CP p) {
			CLispEng& lisp = LISP_GET_LISPREF;
			if (Type(p) == TS_OBJECT)
				lisp.Call(S(L_STREAM_READ_BYTE), p);
			else {
				CStreamValue *sv = lisp.AsStream(p);
				if (StandardStream *ts = lisp.GetInputTextStream(sv)) {
					if (sv->LinePosRef() == -1) {
						lisp.Push(p, V_U);
						lisp.F_FilePosition();
					}
					lisp.CheckFlushFileStream(sv);

					int count = (sv->m_elementBits+7)/8,
						fullBytes = sv->m_elementBits/8;
					byte *buf = (byte*)alloca(count+1);
					
					int bits = (sv->LinePosRef() * sv->m_elementBits) & 7;
					int restBits = sv->m_elementBits & 7;

					for (int i=0; i<fullBytes; ++i) {						
						int b = ts->ReadByte();
						if (b == EOF) {
							lisp.m_r = S(L_K_EOF);
							return;
						}					
						buf[i] = byte(sv->m_curOctet & ((1 << bits)-1) | (b << bits));
						sv->m_curOctet = byte(b >> (8-bits));
					}
					if (restBits <= bits) {
						buf[fullBytes] = sv->m_curOctet & ((1 << restBits)-1);
						sv->m_curOctet >>= restBits;
					} else {
						int b = ts->ReadByte();
						buf[fullBytes] = byte(sv->m_curOctet & ((1 << bits)-1) | (b << bits)) & ((1 << restBits)-1);
						sv->m_curOctet = byte(b >> (restBits-bits));
					}

					if (sv->m_bSigned) {
						--count;
						buf[count] = byte((signed char)buf[count] << (8-restBits) >> (8-restBits));
					} else
						buf[count] = 0;
					lisp.m_r = lisp.FromCInteger(BigInteger(buf, count+1));
					sv->LinePosRef()++;					
				}
			}
		}

		void FunOut(CP s, CP val) {
			CLispEng& lisp = LISP_GET_LISPREF;
			lisp.Push(val, s);
			lisp.m_r = 0;
			lisp.F_WriteByte();
		}
	} sh;
	ProcessInputStream(SV2, sh);
	if (m_r == S(L_K_EOF)) {
		if (SV1)
			E_EndOfFileErr(SV2);
		m_r = SV;
	}
	SkipStack(3);
}

// (write-byte-sequence seq stm :start :end)
void CLispEng::F_WriteByteSequence() {
	Push(SV2);
	F_BuiltInStreamElementType();
	struct CWriteHandler : ISeqHandler {
		FPLispFunc m_pfn;
		CP m_stm;

		bool OnItem(CP x, CP z, CP i) {
			CLispEng& lisp = LISP_GET_LISPREF;
			lisp.Push(x, m_stm);
			lisp.ClearResult();
			(lisp.*m_pfn)();
			return true;
		}
	} sh;
	sh.m_stm = SV2;
	if (m_r == S(L_CHARACTER))
		E_Error(); //!!!
	sh.m_pfn = m_r==0 || m_r==S(L_CHARACTER) ? &CLispEng::F_WriteChar : &CLispEng::F_WriteByte;
	ProcessSeq(SV3, 0, SV1, SV, 0, 0, sh);
	m_r = SV3;
	SkipStack(4);
}

void CLispEng::F_LinePosition() {
	CP stm = GetOutputStream(Pop());
	if (Type(stm) == TS_STREAM) {
		CStreamValue *sv = AsStream(stm);
//!!!?		if (sv->m_writer || sv->m_subtype==STS_PPHELP_STREAM)

//		TRC(1, "m_nPos=" << sv->LinePosRef());

		m_r = CreateInteger64(sv->LinePosRef());
	} else
		Call(S(L_STREAM_LINE_COLUMN), stm);
}

void CLispEng::F_LineNumber() {
	m_r = CreateInteger(ToStream(Pop())->m_nLine);
}

void CLispEng::F_StreamFaslP() {
	CP flag = Pop(), stm = Pop();
	CStreamValue *stmv = ToStream(stm);
	if (flag != V_U) {		
		if (flag)
			stmv->m_flags |= STREAM_FLAG_FASL;
		else
			stmv->m_flags &= ~STREAM_FLAG_FASL;
	}
	m_r = FromBool(stmv->m_flags & STREAM_FLAG_FASL);
}

StandardStream *CLispEng::DoOutputOperation(CP sym) {
	CP p = GetOutputStream(Pop());
	if (Type(p) == TS_STREAM)
		return GetOutputTextStream(AsStream(p));
	Call(sym, p);
	m_r = 0;
	return 0;
}

void CLispEng::F_FinishOutput() {
	if (StandardStream *stm = DoOutputOperation(S(L_STREAM_FINISH_OUTPUT)))
		stm->Flush();
}

void CLispEng::F_ForceOutput() {
	if (StandardStream *stm = DoOutputOperation(S(L_STREAM_FORCE_OUTPUT)))
		stm->Flush();
}

void CLispEng::F_ClearOutput() {
	DoOutputOperation(S(L_STREAM_CLEAR_OUTPUT)); //!!!TODO
}

int StandardStream::get() {
	if (Encoding) {
		byte buf[10];
		for (int i=0; i<sizeof(buf);) {
			int ib = ReadByte();
			if (ib == EOF) {
				if (i)
					Throw(E_FAIL);
				else
					return ib;
			}
			buf[i++] = (byte)ib;
			try {
				DBG_LOCAL_IGNORE(E_EXT_InvalidUTF8String);

				String::value_type ch;
				int n = Encoding->GetChars(ConstBuf(buf, i), &ch, 1);
				if (n == 1)
					return ch;
				if (n != 0)
					Throw(E_FAIL);
			} catch (RCExc ex) {
				if (HResultInCatch(ex) != E_EXT_InvalidUTF8String)
					throw;
			}
		}
		Throw(E_FAIL);	
	} else
		return ReadByte();
}

void StandardStream::put(String::value_type ch) {
	byte buf[10];
	size_t size = (Encoding ? Encoding.get() : &Ext::Encoding::UTF8)->GetBytes(&ch, 1, buf, sizeof buf);
	for (size_t i=0; i<size; ++i)
		WriteByte(buf[i]);
}

InputStream::InputStream(FILE *is) {
	Init(is);
	m_mode |= ios::in;

	Encoding.reset(&Ext::Encoding::Default());
	if (is && IsInteractive()) {
#if UCFG_WIN32_FULL
		Encoding.reset(Ext::Encoding::GetEncoding(::GetConsoleCP()));
#endif
	}
}

int InputStream::get() {
	int r = base::get();
	if (r == '\n' && IsInteractive() && Lisp().m_streams[STM_StandardInput] == this) {
		Lisp().m_streams[STM_StandardOutput]->m_nPos = 0;
	}
	return r;
}

OutputStream::OutputStream(OStreamImp *os) {
	Init(os);
	m_mode |= ios::out;
	
	Encoding.reset(&Ext::Encoding::Default());
	if (os && IsInteractive()) {
		Encoding = nullptr;
#if UCFG_WIN32_FULL
//!!!		Encoding = Ext::Encoding::GetEncoding(::GetConsoleOutputCP());
#endif
	}
}

void OutputStream::WriteByte(byte b) {
	if (fputc(b, m_os) == EOF)
		CCheck(-1);
}

void OutputStream::put(String::value_type ch) {
#if UCFG_USE_POSIX
	base::put(ch);
#else
	if (Encoding)
		base::put(ch);
	else if (fputwc(ch, m_os) == WEOF)		// fputwc() by standard does not allow to alternate wide/narrow chars
		CCheck(-1);
#endif
}


#if UCFG_USE_READLINE

void ReadlineFree(void *p) {	
#if !UCFG_WIN32		//!!!L readline5.dll uses other CRT. Memory Leak better than crash
	free(p);
#endif
}

int TerminalInputStream::get() {
#if UCFG_WIN32
	static HMODULE s_hModule = ::LoadLibrary(_T("readline5.dll"));
	if (!s_hModule)
		return base::get();
#endif

	int r = EOF;
	if (m_queue.empty()) {
		if (char *line = readline(0)) {
			CLispEng& lisp = Lisp();
			if (lisp.m_streams[STM_StandardInput] == this)
				lisp.m_streams[STM_StandardOutput]->m_nPos = 0;
			vector<String::value_type> v = Encoding::Default().GetChars(ConstBuf(line, strlen(line)));
			m_queue.assign(v.begin(), v.end());
			m_queue.push_back('\n');

			if (line[0]) {
				if (lisp.Spec(L_S_TERMINAL_READ_OPEN_OBJECT) != V_U) {
					HIST_ENTRY **hist = history_list();
					if (!hist)
						Throw(E_FAIL);
					while (*(hist+1))
						++hist;
					HIST_ENTRY *last = *hist;
					char *new_line = (char*)malloc(2 + strlen(line) + strlen(last->line));
					strcat(strcat(strcpy(new_line, last->line), "\n"), line);
					ReadlineFree(exchange(last->line, new_line));
				} else
					add_history(line);
			}
			ReadlineFree(line);
		}
	}
	if (!m_queue.empty()) {
		r = m_queue.front();
		m_queue.pop_front();
	}
	return r;
}

#endif // UCFG_USE_READLINE

} // Lisp::

