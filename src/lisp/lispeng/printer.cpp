#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

void CLispEng::ApplyMark(CMarkSweepHandler& h, CP& place, bool bMark) {
	CP p = place & ~FLAG_Mark;
LAB_START:

#ifdef _X_DEBUG//!!!D
	if (p == 0x7B06100)
	{
		Print(p);
		cerr << "\n-----------\n";
	}
#endif

	int ts = Type(p);
	int tidx = g_arTS[ts];
	if (tidx == -1 || !p)
		return;
	CValueManBase *vman = m_arValueMan[tidx];
	size_t i = AsIndex(p);
	bool bMarked = BitOps::BitTest(vman->m_pBitmap, i); //!!!
	h.OnMark(p, bMarked);
	if (!bMarked) {
		BitOps::BitTestAndSet(vman->m_pBitmap, (int)i);
		if (ts < TS_BIGNUM)
			m_arDeferred.push_back(p);
		if (ts == TS_WEAKPOINTER) {
			p = AsWeakPointer(p)->m_p;
			goto LAB_START;
		}
	}


/*!!!R

	byte& b = *((byte*)vman->m_pBase+i*vman->m_sizeof);
	if (bMark)
		h.OnMark(p, b & FLAG_Mark);
	if ((b & FLAG_Mark) ^ (bMark ? FLAG_Mark : 0))
	{
		b ^= FLAG_Mark;
		m_arDeferred.push_back(p);
	}
	*/
}

void CLispEng::ApplyMarkSValue(CMarkSweepHandler& h, CP& p, bool bMark) {
	DWORD_PTR idx = AsIndex(p);
	if (!h.OnApply(p, bMark))
		return;
	switch (Type(p))
	{
	case TS_CONS:
		{
			CConsValue *cons = m_consMan.TryApplyCheck(idx);
			ApplyMark(h, cons->m_cdr, bMark);
			ApplyMark(h, cons->m_car, bMark);
		}
		break;
	case TS_ARRAY:
	case TS_CCLOSURE:
		{
			CArrayValue *av = m_arrayMan.TryApplyCheck(idx);
			if (!(av->m_flags & FLAG_Displaced) && av->m_elType==ELTYPE_T) {
				size_t size = av->TotalSize();
#ifdef _X_DEBUG//!!!D
				if (Type(p)==TS_CCLOSURE)
					for (int i=0; i<size; i++)
					{
						cerr << "\n---------- " << i << ":\n";
						Print(av->m_pData[i]);
					}
#endif
				for (size_t i=0; i<size; ++i)
					ApplyMark(h, av->m_pData[i], bMark); 
			}
		}
		break;
	default:
		return;
	}
}

void CLispEng::Mark(CMarkSweepHandler& h, bool bMark) {
	ClearBitmaps();
	m_arDeferred.clear();
	ApplyMark(h, h.m_p, bMark);
	while (!m_arDeferred.empty()) {
		CP q = m_arDeferred.back();
		m_arDeferred.pop_back();
		ApplyMarkSValue(h, q, bMark);
	}
}

void CLispEng::MarkSweep(CMarkSweepHandler& h) {
	class CMarkKeeper {
		CMarkSweepHandler& m_h;
	public:
		CMarkKeeper(CMarkSweepHandler& h)
			:	m_h(h)
		{
			Lisp().Mark(m_h, true);
		}

		~CMarkKeeper() {
	//		Lisp().Mark(m_h, false);
		}
	} markKeeper(h);
}

CP CLispEng::GetCircularities(CP p) {
	struct CCircularityHandler : CMarkSweepHandler {
		CP *m_pStart;
		bool m_bPrintArray,
			   m_bPrintClosure;

		bool OnApply(CP p, bool bMark) {
			switch (Type(p)) {
			case TS_ARRAY: return m_bPrintArray;
			case TS_CCLOSURE:
				{
					if (m_bPrintClosure)
						return true;
					CLispEng& lisp = LISP_GET_LISPREF;
					lisp.ApplyMark(_self, lisp.TheClosure(p).NameOrClassVersion, bMark);
					return false;
				}
			default:
				return true;
			}
		}

		void OnMark(CP p, bool bAlreadyMarked) {
			if (bAlreadyMarked) {
				CLispEng& lisp = LISP_GET_LISPREF;
				switch (Type(p)) {
				case TS_SYMBOL:
					if (lisp.AsSymbol(p)->HomePackage)
						return;
				}
#ifdef _X_DEBUG //!!!D
				m_lisp.Print(p);
#endif
				for (int i=m_pStart-lisp.m_pStack; i--;)
					if (m_pStart[-i-1] == p)
						return;
				lisp.Push(p);
			}
		}
	} h;
#ifdef _X_DEBUG//!!!D
	static int n = 0;
	if (++n == 100)
		E_Error();
	Print(p);
	cerr << "\n\n";
#endif
	h.m_p = p;
	h.m_pStart = m_pStack;
	h.m_bPrintArray = Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_ARRAY);
	h.m_bPrintClosure = Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_CLOSURE);
	MarkSweep(h);
	if (m_pStack == h.m_pStart)
		return 0;
	size_t len = h.m_pStart-m_pStack;
	CArrayValue *av = CreateVector(len+1);
	av->SetElement(0, V_0);
	memcpy(av->m_pData+1, m_pStack, len*sizeof(CP));
	m_pStack = h.m_pStart;
	return FromSValue(av);
}

bool CLispEng::CircleP(CP p, CCircleInfo *ci) {
	if (!Spec(L_S_PRINT_CIRCLE))
		return false;
	if (CP table = Spec(L_S_PRINT_CIRCLE_TABLE)) {
		CArrayValue *av = ToVector(table);
		size_t i = AsPositive(av->GetElement(0));
		size_t index;
		size_t len = av->DataLength;
		for (index=1; index<len; index++)
			if (av->m_pData[index] == p) {
#ifdef _X_DEBUG//!!!D
				Print(p);
#endif
				if (index <= i) {
					if (ci) {
						ci->m_bFirst = false;
						ci->m_idx = index;
					}
				} else {
					i++;
					swap(av->m_pData[index], av->m_pData[i]);
					if (ci) {
						ci->m_bFirst = true;
						ci->m_idx = i;
						av->SetElement(0, CreateFixnum(i));
					}
				}
				return true;
			}
	}
	return false;
}

void CLispEng::PrettyPrintCall(CP x) {
	CP q = Spec(L_S_PRINT_PPRINT_DISPATCH);
	if (ConsP(q) && Car(q)==S(L_S_PRINT_PPRINT_DISPATCH) && Cdr(q)) {
		Push(x);
		Call(S(L_PPRINT_DISPATCH), x);
		x = Pop();
		if (m_arVal[1])
			return Call(m_r, m_stm, x);
	}
	PrinDispatch(x);
}

void CLispEng::PrEnter1(CP p) {
	if (Spec(L_S_PRINT_PRETTY)) {
		if (CStreamValue *sv = CastToPPStream(m_stm))
			PrettyPrintCall(p);
		else {
			Push(m_stm);
			F_LinePosition();
			CDynBindFrame dynFrame;
			dynFrame.Bind(_self, S(L_S_PRIN_L1), Type(m_r)==TS_FIXNUM && AsNumber(m_r)>=0 ? m_r : V_0);
			dynFrame.Bind(_self, S(L_S_PRIN_LM), V_0);
			dynFrame.Finish(_self, 2);
			F_MakePphelpStream();
			Push(m_r);
			{
				CStm sk(m_r);
				PrettyPrintCall(p);
			}
			CP ppstm = SV;
			sv = AsStream(ppstm);
			CP blocks = sv->m_out = NReverse(sv->m_out);
			CP first = Car(Cdr(blocks));
			bool bSkipFirstNL = StringP(first) && (!AsArray(first)->GetVectorLength() || AsArray(first)->GetElement(0)==V_NL)
				                || Spec(L_S_PRIN_L1)==V_0;
			if (!Cdr(Cdr(blocks)))  // DEFINITELY a single-liner
				bSkipFirstNL = true;
			else {
				bool bFitThisLine = false;
				if (!sv->m_bMultiLine) {
					int len = 0;
					for (CP cdr=Cdr(blocks), car; SplitPair(cdr, car);) {
						if (StringP(car))
							len += SeqLength(car);
						else if (VectorP(car))
							len += FormatTab(m_stm, car);
						else if (ConsP(car))
							if (!Car(car)) {
								AsStream(ppstm)->m_bMultiLine = true;
								goto MULTILINE_LAB;
							}
					}
					bFitThisLine = true;
					if (CP prm = RightMargin()) {
						sv = AsStream(ppstm);
						sv->m_bMultiLine = len > AsNumber(prm);
						if (bFitThisLine = len <= AsNumber(prm)-AsNumber(Spec(L_S_PRIN_L1)))
							bSkipFirstNL = true;
					}
					sv->m_bMultiLine = bSkipFirstNL && !bFitThisLine;
				}
MULTILINE_LAB:
				;
			}
			CP top;
			SplitPair(blocks, top);
			if (bSkipFirstNL)
				PprintPrefix(true, AsNumber(Cdr(top)));
			for (; SplitPair(blocks, top); ) {
				CP indent = V_0;
				if (ConsP(top)) {
					if (!sv->m_bMultiLine || Car(top)==S(L_K_FILL) && StringFitLineP(m_stm, blocks, 0))
						bSkipFirstNL = true;
					else {
						indent = Cdr(top);
						if (!blocks)
							break;
					}
					SplitPair(blocks, top);
				} else if (!StringP(top)) {
					if (!blocks)
						break;
					int nSpace = FormatTab(m_stm, top);
					if (!sv->m_bMultiLine || StringP(Car(blocks))
						&& Car(Car(blocks))==S(L_K_FILL)
						&& StringFitLineP(m_stm, Cdr(blocks), nSpace))
					{
						while (nSpace--)
							WriteChar(' ');
						bSkipFirstNL = true;
					}
					else if (ConsP(Car(blocks)))
						indent = Cdr(Car(blocks));
					SplitPair(blocks, top);
				}
				if (!bSkipFirstNL) {
					WriteChar('\n');
					PprintPrefix(true, indent ? AsNumber(indent) : 0);  //!!!?
				}
				bSkipFirstNL = false;
				if (StringP(top))
					WriteStr(AsString(top));
			}
		}
	}
	else
		PrCircle(p);
}

void CLispEng::PrEnter2(CP p) {
	CDynBindFrame bind;
	size_t count = 0;
	if (Spec(L_S_PRINT_CIRCLE) || Spec(L_S_PRINT_READABLY)) {
		CP table = GetCircularities(p);
		count++;
		if (!table && !Spec(L_S_PRINT_READABLY))
			bind.Bind(_self, S(L_S_PRINT_CIRCLE), 0);
		else {
			bind.Bind(_self, S(L_S_PRINT_CIRCLE_TABLE), table);
			if (!Spec(L_S_PRINT_CIRCLE)) {
				bind.Bind(_self, S(L_S_PRINT_CIRCLE), V_T);
				count++;
			}
		}
	}
	bind.Finish(_self, count);
	PrEnter1(p);
}

void CLispEng::PrEnter(CP stm, CP p) {
	if (stm == Spec(L_S_PRIN_STREAM)) {
		if (Spec(L_S_PRINT_CIRCLE_TABLE) == V_U)
			PrEnter2(p);
		else
			PrEnter1(p);
	} else {
		CDynBindFrame bind;
		bind.Bind(_self, S(L_S_PRIN_LEVEL), V_0);
		bind.Bind(_self, S(L_S_PRIN_LINES), V_0);
		bind.Bind(_self, S(L_S_PRIN_BQLEVEL), V_0);
		bind.Bind(_self, S(L_S_PRIN_L1), V_0);
		bind.Bind(_self, S(L_S_PRIN_LM), V_0);
		bind.Bind(_self, S(L_S_PRIN_TRAILLENGTH), V_0);
		bind.Finish(_self, 6);
		CStm sk(stm);
		PrEnter2(p);
	}
}

void CLispEng::Prin1(CP p) {
#ifdef X_DEBUG//!!!D
	if ((p & 0xFF) >= 0x20)
		p = p;
#endif
	CPrintDispatchKeeper pdk(&CLispEng::PrinObjectDispatch);
	PrEnter(m_stm, p);
}

void CLispEng::PrinObject(CP p) {
	CPrintDispatchKeeper pdk(&CLispEng::PrinObjectDispatch);
	PrCircle(p);
}

void CLispEng::WriteUint(size_t n) {
#if UCFG_USE_POSIX
	WritePChar(Convert::ToString(n));
#else
	char buf[20];
	_ltoa((long)n, buf, 10);
	WritePChar(buf);
#endif
}

size_t CLispEng::GetLengthLimit() {
	if (!Spec(L_S_PRINT_READABLY)) {
		CP lim = Spec(L_S_PRINT_LENGTH);
		if (Type(lim)==TS_FIXNUM) {
			int n = AsNumber(lim);
			if (n >= 0)
				return n;
		}
	}
	return numeric_limits<size_t>::max();
}

bool CLispEng::ListLengthIs1(CP p) {
	return ConsP(p) && !Cdr(p);
}

void CLispEng::PrString(CP p) {
	size_t len = AsArray(p)->GetVectorLength();
	if (Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_ESCAPE)) {
		WriteChar('\"');
		for (size_t i=0; i<len; ++i) {
			CP c = AsArray(p)->GetElement(i);
			if (c==V_BACKSLASH || c==V_DQUOTE)
				WriteChar('\\');
			WriteChar(AsChar(c));
		}
		WriteChar('\"');
	}
	else
		for (size_t i=0; i<len; ++i)
			WriteChar(AsChar(AsArray(p)->GetElement(i)));
}

void CLispEng::PrBitVector(CP p) {
	size_t len = AsArray(p)->GetVectorLength();
	WritePChar("#*");
	for (size_t i=0; i<len; ++i)
		WriteChar('0'+int(AsArray(p)->GetElement(i)!=V_0));
}


void CLispEng::PrintInt(CP pn, int base) {
#ifdef X_DEBUG//!!!D
	//if (n.m_count >= 2)
	if (pn == 0x13015)
		DebugBreak();
#endif
	BigInteger bigbase = base,
		       n = ToBigInteger(pn);

	CP pbase = CreateFixnum(base);
	if (Sign(n) == -1) {
		n = -n;
		WriteChar('-');
	}
	int k = 0;
	while (true) {
		pair<BigInteger, BigInteger> p = div(n, bigbase);
		S_BASEWORD d;
		p.second.AsBaseWord(d);
		Push(DigitChar(d));
		k++;
		if (!(n=p.first))
			break;
	}
	while (k--)
		WriteChar(AsChar(Pop()));
}

void CLispEng::PrRealNumber(CP p) {
	switch (Type(p)) {
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_RATIO:
		{
			int base = TestRadix(Spec(L_S_PRINT_BASE));
			if (Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_RADIX))
				switch (base) {
				case 2: WritePChar("#b"); break;
				case 8: WritePChar("#o"); break;
				case 16: WritePChar("#x"); break;
				case 10:
					if (IntegerP(p))
					{
						PrintInt(p, base);
						WriteChar('.');
						return;
					}
				default:
					WriteChar('#');
					WriteUint(base);
					WriteChar('r');
				}
			if (IntegerP(p))
				PrintInt(p, base);
			else {
				PrintInt(AsRatio(p)->m_numerator, base);
				WriteChar('/');
				PrintInt(AsRatio(p)->m_denominator, base);
			}
		}
		break;
	case TS_FLONUM:
		Push(p, m_stm);
		F_PrintFloat();
		break;
	default:
		E_Error();
	}
}

void CLispEng::WriteUpDownStr(RCString s, FPLispFunc pfnConv) {
	size_t len = s.length();
	for (size_t i=0; i<len; ++i) {
		Push(CreateChar(s[i]));
		(this->*pfnConv)();
		WriteChar(AsChar(m_r));
	}
}

void CLispEng::WriteUpcaseStr(RCString s) {
	WriteUpDownStr(s, &CLispEng::F_CharUpcase);
}

void CLispEng::WriteDowncaseStr(RCString s) {
	WriteUpDownStr(s, &CLispEng::F_CharDowncase);
}

void CLispEng::WriteCapitalizeStr(RCString s, FPLispFunc pfnConv) {
	size_t len = s.length();
	bool bFlag = false;
	for (size_t i=0; i<len; ++i) {
		bool bOld = bFlag;
		CP c = CreateChar(s[i]);
		Push(c);
		F_AlphanumericP();
		if ((bFlag=m_r) && bOld) {
			Push(c);
			(this->*pfnConv)();
			c = m_r;
		}
		WriteChar(AsChar(c));
	}
}

void CLispEng::WriteCaseString(RCString s) {
	CP cas = GetReadtable()->m_case;
	switch (cas) {
	case S(L_K_UPCASE):
		switch (Spec(L_S_PRINT_CASE)) {
		case S(L_K_UPCASE):
			WriteStr(s);
			break;
		case S(L_K_DOWNCASE):
			WriteDowncaseStr(s);
			break;
		case S(L_K_CAPITALIZE):
			WriteCapitalizeStr(s, &CLispEng::F_CharDowncase);
			break;
		default:
			E_Error();
		}
		break;
	case S(L_K_DOWNCASE):
		switch (Spec(L_S_PRINT_CASE)) {
		case S(L_K_UPCASE):
			WriteUpcaseStr(s);
			break;
		case S(L_K_DOWNCASE):
			WriteStr(s);
			break;
		case S(L_K_CAPITALIZE):
			WriteCapitalizeStr(s, &CLispEng::F_CharUpcase);
			break;
		default:
			E_Error();
		}
		break;
	case S(L_K_INVERT):
		{
			size_t len = s.length();
			bool bSeenUpper = false,
					 bSeenLower = false;
			for (size_t i=0; i<len; ++i) {
				CP c = CreateChar(s[i]);
				Push(c);
				F_CharUpcase();
				if (m_r != c)
					bSeenLower = true;
				Push(c);
				F_CharDowncase();
				if (m_r != c)
					bSeenUpper = true;
			}
			if (bSeenUpper ^ bSeenLower) {
				bSeenUpper ? WriteDowncaseStr(s) : WriteUpcaseStr(s);
				break;
			}
		}
	case S(L_K_PRESERVE):
		WriteStr(s);
		break;
	default:
		E_Error();
	}
}

void CLispEng::WriteEscapeName(RCString s) {
	WriteChar('|');
	size_t len = s.length();
	for (size_t i=0; i<len; ++i) {
		switch (wchar_t ch = s[i]) {
		case '|':
		case '\\':
			WriteChar('\\');
		default:
			WriteChar(ch);
		}
	}
	WriteChar('|');
}

void CLispEng::PrSymbolPart(RCString s) {
	if (!Spec(L_S_PRINT_READABLY))
		if (size_t len = s.length()) {
			wchar_t ch = s[0];
			CCharType ct = GetCharType(ch);
			if (ct.m_syntax == ST_CONSTITUENT) {
				CP cas = GetReadtable()->m_case;
				size_t i = 0;
				while (true) {
					if (ct.m_traits & TRAIT_PACKAGE)
						goto LAB_ESCAPED;
					CP c = CreateChar(ch);
					switch (cas) {
					case S(L_K_UPCASE):
						Push(c);
						F_CharUpcase();
						if (c != m_r)
							goto LAB_ESCAPED;
						break;
					case S(L_K_DOWNCASE):
						Push(c);
						F_CharDowncase();
						if (c != m_r)
							goto LAB_ESCAPED;
						break;
					case S(L_K_PRESERVE):
					case S(L_K_INVERT):
						break;
					default:
						E_Error();
					}
					if (++i >= len)
						break;
					ch = s[i];
					ct = GetCharType(ch);
					switch (ct.m_syntax) {
					case ST_CONSTITUENT:
					case ST_MACRO: //!!! Not-terminating must be
						break;
					default:
						goto LAB_ESCAPED;
					}
				}
				CTokenVec vec;
				for (size_t i=0; i<len; ++i) {
					wchar_t ch = s[i];
					CCharType ct = GetCharType(ch);
					vec.push_back(CCharWithAttrs(ch, true, ct.m_traits));
				}
				if (!OnlyDotsOrEmpty(vec) && !PotentialNumberP(vec, Spec(L_S_PRINT_BASE))) {
					WriteCaseString(s);
					return;
				}
			}
		}
LAB_ESCAPED:
	WriteEscapeName(s);
}

void CLispEng::PrSymbol(CP p) {
	CSymbolValue *sv = ToSymbol(p);
#if UCFG_LISP_SPLIT_SYM
	String name = SymNameByIdx(AsIndex(p));
#else
	String name = sv->m_s;
#endif
	bool bReadably = Spec(L_S_PRINT_READABLY);
	if (!bReadably && !Spec(L_S_PRINT_ESCAPE))
		WriteCaseString(name);
	else {
		CP pack = sv->HomePackage;
		CP sym;
		byte flags;
		if (pack == m_packKeyword)
			WriteChar(':');
		else if (!pack) {
			if (bReadably || Spec(L_S_PRINT_GENSYM))
				WritePChar("#:");
		} else if (bReadably || !FindSymbol(name, Spec(L_S_PACKAGE), sym, flags) || sym!=p) {
			PrSymbolPart(ToPackage(pack)->m_name);
			WriteChar(':');
			if (bReadably || FindSymbol(name, pack, sym, flags)!= S(L_K_EXTERNAL))
				WriteChar(':');
		}
		PrSymbolPart(name);
	}
}

void CLispEng::PrFramePtr(CP p) {
	PrOtherObj("FRAME-PTR", Convert::ToString(m_pStackTop-AsFrame(p)));
}

void CLispEng::PrFrameInfo(CP p) {
	PrOtherObj("FRAME-INFO", Convert::ToString(AsIndex(p) & FRAME_TYPE_MASK));
}




} // Lisp::
