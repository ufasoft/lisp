/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

const CLispEng::CCharDispatcher CLispEng::s_arMacroChar[] = {
	'#',	&CLispEng::F_M_Sharp,
	'\'',	&CLispEng::F_M_Quote,
	'\"',	&CLispEng::F_M_DQuote,
	'`',	&CLispEng::F_M_BQuote,
	',',	&CLispEng::F_M_Comma,
	'(',	&CLispEng::F_M_LeftParenthesis,
	')',	&CLispEng::F_M_RightParenthesis,
	';',	&CLispEng::F_M_Semicolon
};

const CLispEng::CCharDispatcher CLispEng::s_arSharpDispatcher[] = {
	'|',	&CLispEng::F_Sharp_VerticalBar,
	'\'',	&CLispEng::F_Sharp_Quote,
	'\\',	&CLispEng::F_Sharp_Backslash,
	'.',	&CLispEng::F_Sharp_Dot,
	':',	&CLispEng::F_Sharp_Colon,
	'+',	&CLispEng::F_Sharp_Plus,
	'-',  	&CLispEng::F_Sharp_Minus,
	'(',  	&CLispEng::F_Sharp_LPar,
	'*',  	&CLispEng::F_Sharp_Asterisk,
	//!!!R	'H',	&CLispEng::F_Sharp_H,
	'S',	&CLispEng::F_Sharp_S,
	'Y',	&CLispEng::F_Sharp_Y
};

void CLispEng::CopyReadtable(CReadtable *from, CReadtable *to) {
	*to = *from;
	for (int i=0; i<_countof(from->m_ar); i++)
		to->m_ar[i].m_disp = CopyList(from->m_ar[i].m_disp);
	for (CReadtable::CCharMap::iterator it=from->m_map.begin(); it!=from->m_map.end(); ++it)
		to->m_map[it->first].m_disp = CopyList(it->second.m_disp);
}

CReadtable *CLispEng::GetReadtable() {
	return ToReadtable(Spec(L_S_READTABLE));
}

CReadtable *CLispEng::FromReadtableDesignator(CP p) {
	if (p == V_U)
		return GetReadtable();
	else if (!p)
		p = Spec(L_S_STANDARD_READTABLE);
	return ToReadtable(p);
}

void CLispEng::F_CopyReadtable() {
	CP to = ToOptionalNIL(Pop());
	CReadtable *rtTo = !to ? CreateReadtable() : ToReadtable(to);
	m_r = FromSValue(rtTo);
	CopyReadtable(FromReadtableDesignator(Pop()), rtTo);
}

void CLispEng::InitReader() {
	//!!!D	m_bPreservingWhitespace = false;
	const char *WSPACE = "\t\n \f\r",
		*ALPHABETICS = "!\"#$%&\'()*,;<=>?@[\\]^_`|~{}",
		*ALPHADIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
		*EXPONENTS = "DEFLSdefls";
	CReadtable *rt = CreateReadtable();
	rt->m_ar['\\'].m_syntax = ST_SESCAPE;
	rt->m_ar['|'].m_syntax = ST_MESCAPE;

	const char *p;
	int ch;
	for (p=WSPACE; ch=*p++;)
		rt->m_ar[ch].m_syntax = ST_WHITESPACE;
	for (p=ALPHABETICS; ch=*p++;)
		rt->m_ar[ch].m_traits = TRAIT_ALPHABETIC;
	for (p=ALPHADIGITS; ch=*p++;)
		rt->m_ar[ch].m_traits = TRAIT_ALPHADIGIT;
	for (p=EXPONENTS; ch=*p++;)
		rt->m_ar[ch].m_traits |= TRAIT_EXPONENT_MARKER;
	rt->m_ar['-'].m_traits = TRAIT_ALPHABETIC | TRAIT_MINUS;
	rt->m_ar['+'].m_traits = TRAIT_ALPHABETIC | TRAIT_PLUS;
	rt->m_ar[':'].m_traits = TRAIT_PACKAGE;
	rt->m_ar['.'].m_traits = TRAIT_ALPHABETIC | TRAIT_DOT | TRAIT_POINT;
	rt->m_ar['/'].m_traits = TRAIT_RATIO;

	rt->m_ar['^'].m_traits |= TRAIT_EXTENSION;
	rt->m_ar['_'].m_traits |= TRAIT_EXTENSION;

	int i;
	for (i=0; i<_countof(s_arMacroChar); i++) {
		const CCharDispatcher& d = s_arMacroChar[i];
		rt->m_ar[d.m_ch].m_syntax = ST_MACRO;
		rt->m_ar[d.m_ch].m_bTerminating = true;
		rt->m_ar[d.m_ch].m_macro = FindSubr(d.m_fn, 2, 0, 0);
	}
	rt->m_ar['#'].m_bTerminating = false;
	for (i=0; i<_countof(s_arSharpDispatcher); i++) {
		const CCharDispatcher& d = s_arSharpDispatcher[i];
		Push(CreateChar(d.m_ch), FindSubr(d.m_fn, 3, 0, 0));
	}
	rt->m_ar['#'].m_disp = Listof(_countof(s_arSharpDispatcher)*2);

	rt->m_case = S(L_K_UPCASE);

	CP pRT = FromSValue(rt);
	SetSpecial(S(L_S_STANDARD_READTABLE), pRT);
	Push(0, V_U);
	F_CopyReadtable();
	SetSpecial(S(L_S_READTABLE), m_r);
	SetSpecial(S(L_S_READ_SUPPRESS), 0);
}

/*!!!CP CLispEng::ReadList(CP stm)
{
for (int n=0; true; n++)
{
CP p = ReadSValue(stm, false, true);
if (p == m_specRPar)    
return Listof(n);
if (p == m_specEOF)
Throw(E_LISP_AbsentCloseParen);
if (p == m_specDot)
{
Push(ReadSValue(stm));
ReadSValue(stm, true, true);
return ListofEx(n+1);
}
Push(p);   
}
}*/

CP CLispEng::FromSyntaxType(CSyntaxType st) {
	CP ar[] = { m_sConstituent, m_sInvalid, m_sMacro, m_sWhitespace, m_sSescape, m_sMescape };
	return ar[st];
}

void CLispEng::F_ReadtableCase() {
	m_r = ToReadtable(Pop())->m_case;
}

void CLispEng::F_SetfReadtableCase() {
	m_r = Pop();
	ToReadtable(Pop())->m_case = m_r;
}

CCharType CLispEng::GetCharType(wchar_t ch, CReadtable *rt) {
	if (!rt)
		rt = GetReadtable();
	if (ch < _countof(rt->m_ar))
		return rt->m_ar[ch];
	CCharType ct;
	if (Lookup(rt->m_map, ch, ct))
		return ct;
	ct.m_traits = TRAIT_ALPHABETIC; //!!!
	return ct;
}

void CLispEng::F_GetMacroCharacter() {
	CCharType ct = GetCharType(AsChar(SV1), FromReadtableDesignator(SV));
	m_r = ct.m_macro;
	m_arVal[1] = FromBool(ct.m_bTerminating);
	m_cVal = 2;
	SkipStack(2);
}

void CLispEng::F_SetMacroCharacter() {
	if (!SV)
		E_Error();
	CReadtable *rt = FromReadtableDesignator(SV);
	wchar_t ch = AsChar(SV3);
	CCharType ct = GetCharType(ch, rt);
	ct.m_macro = FromFunctionDesignator(SV2);
	ct.m_bTerminating = SV1 && SV1!=V_U;
	if (ch < _countof(rt->m_ar))
		rt->m_ar[ch] = ct;
	else
		rt->m_map[ch] = ct;
	SkipStack(4);
	m_r = V_T;
}

void CLispEng::F_GetDispatchMacroCharacter() {
	CCharType ct = GetCharType(AsChar(SV2), FromReadtableDesignator(SV));
	if (ct.m_macro != FromReadtableDesignator(0)->m_ar['#'].m_macro)
		E_Error();
	SV2 = ct.m_disp;
	SV = 0;
	F_Getf();
	/*!!!R
	m_r = Getf(ct.m_disp, SV1);
	SkipStack(3);
	*/
}

void CLispEng::F_SetDispatchMacroCharacter() {
	wchar_t ch = AsChar(SV2),
		dispCh = AsChar(SV3);
	if (isdigit(ch))
		E_Error();
	CReadtable *rt = FromReadtableDesignator(SV);
	CCharType ct = GetCharType(dispCh, rt);
	if (ct.m_macro != FromReadtableDesignator(0)->m_ar['#'].m_macro)
		E_Error();
	if (SV1) {
		Push(ct.m_disp, SV2, FromFunctionDesignator(SV1));
		F_PPutf();
		if (m_r)
			ct.m_disp = m_r;
	}
	else
		Remf(ct.m_disp, SV2);
	rt = FromReadtableDesignator(SV);
	if (dispCh < _countof(rt->m_ar))
		rt->m_ar[dispCh] = ct;
	else
		rt->m_map[dispCh] = ct;
	SkipStack(4);
	m_r = V_T;
}

int CLispEng::ToDigit(wchar_t ch) {
	int code;
	if (ch >= '0' && ch <= '9')
		code = ch-'0';
	else if (ch >= 'A' && ch <= 'Z')
		code = ch-'A'+10;
	else if (ch >= 'a' && ch <= 'z')
		code = ch-'a'+10;
	else
		Throw(E_FAIL);
	return code;
}

CP CLispEng::ToInteger(const CTokenVec& vec, int readBase) {
	bool bMinus = false;
	CTokenVec::const_iterator i = vec.begin();
	if (i == vec.end())
		E_Error();
	if (i->m_traits & TRAIT_SIGN) {
		bMinus = (i->m_traits & TRAIT_SIGN)==TRAIT_MINUS;
		i++;
	}
	if (i == vec.end())
		E_Error();
	BigInteger num;
	for (; i!=vec.end(); ++i)
		if (!(i->m_traits & TRAIT_ALPHADIGIT))
			E_Error();
		else
			(num *= readBase) += ToDigit(i->m_ch);
	if (bMinus)
		num = -num;
	return FromCInteger(num);
}

String CLispEng::TokenToString(CTokenVec& vec) {
	String s(' ', vec.size());
	for (int i=0; i<vec.size(); i++)
		s.SetAt(i, (String::Char)vec[i].m_ch);
	return s;
}

CP CLispEng::ToFloat(CTokenVec& vec) {
	String s = TokenToString(vec);
	s.Replace("f", "e");
	s.Replace("F", "E");
	s.Replace("l", "e");
	s.Replace("L", "E");
	s.Replace("s", "e");
	s.Replace("S", "E");
	return FromFloat(strtod(s, 0));
}

CP CLispEng::ToNumber(CTokenVec& vec) {
	int readBase = AsNumber(Spec(L_S_READ_BASE));
	CTokenVec::iterator i;
	for (i=vec.begin(); i!=vec.end(); ++i) {
		if (i->m_traits & TRAIT_RATIO) {
			Push(ToInteger(CTokenVec(vec.begin(), i), readBase));
			Push(ToInteger(CTokenVec(i+1, vec.end()), readBase));
			MakeRatio();
			return m_r;
		}
	}
	if (vec.back().m_traits & TRAIT_POINT)
		return ToInteger(CTokenVec(vec.begin(), vec.end()-1), 10);
	for (i=vec.begin(); i!=vec.end(); ++i)
		if (!(i->m_traits & TRAIT_ALPHADIGIT) && !(i==vec.begin() && (i->m_traits & TRAIT_SIGN)))
			return ToFloat(vec);
	return ToInteger(vec, readBase);
}

void CLispEng::F_CharType() {
	CP p = Pop();
	CReadtable *rt = ToReadtable(p==V_U ? Spec(L_S_READTABLE) : p);
	wchar_t ch = AsChar(Pop());
	m_cVal = 2;
	if (ch < _countof(rt->m_ar)) {
		if (rt->m_ar[ch].m_syntax == ST_MACRO)
			m_r = rt->m_ar[ch].m_macro;
		else
			m_r = FromSyntaxType(rt->m_ar[ch].m_syntax);
		m_arVal[1] = rt->m_ar[ch].m_disp;
	} else {
		m_r = m_sConstituent; //!!! may be UNICODE specchars
		m_arVal[1] = CreateInteger(TRAIT_ALPHABETIC);
	}
}

/*!!!
void CLispEng::F_SetfCharType()
{
CReadtable *rt = ToReadTable(GetStack(2));
wchar_t ch = AsChar(GetStack(3));
if (ch >= 256)
E_Error();
rt->m_ar[ch][1] = Pop();
rt->m_ar[ch][0] = Pop();
SkipStack(2);
}
*/


CLispEng::CTokenVec CLispEng::ReadExtendedToken(CP stm, bool bSescape, bool bPreservingWhitespace) {
	CTokenVec vec;
	bool bMode = false;
	while (true) {
		int ich = ReadChar(stm, false);
		if (ich == EOF)
			if (bSescape)
				E_Error();
			else
				break;
		wchar_t ch = (wchar_t)ich;
		if (bSescape) {
			vec.push_back(CCharWithAttrs(ch, false, TRAIT_ALPHABETIC));
			bSescape = false;
		} else {
			CCharType ct = GetCharType(ch);
			switch (ct.m_syntax)
			{
			case ST_SESCAPE:
				vec.HasEscapes = true;
				bSescape = true;
				break;
			case ST_MESCAPE:
				vec.HasEscapes = true;
				bMode = !bMode;
				break;
			default:
				if (bMode)
					vec.push_back(CCharWithAttrs(ch, false, TRAIT_ALPHABETIC));
				else
					switch (ct.m_syntax)
					{
						case ST_INVALID:
							E_Error();
						case ST_WHITESPACE:
							if (bPreservingWhitespace)
								PutBackChar(stm, ch);
							goto tokenComplete;
						case ST_MACRO:
							if (ct.m_bTerminating)
							{
								PutBackChar(stm, ch);
								goto tokenComplete;
							}
						case ST_CONSTITUENT:
							vec.push_back(CCharWithAttrs(ch, true, ct.m_traits));
					}
			}
		}
	}
	if (bMode)
		E_Error();
tokenComplete:
	CReadtable *rt = GetReadtable();
	CP cas = rt->m_case;
	if (cas == S(L_K_INVERT)) {
		bool bUpper = false,
			bLower = false;
		for (int i=0; i<vec.size(); i++) {
			CCharWithAttrs& ca = vec[i];
			if (ca.m_bReplaceable) {
				CP ch = CreateChar(ca.m_ch);
				Push(ch);
				F_AlphaCharP();
				if (!m_r)
					continue;
				Push(ch);
				F_UpperCaseP();
				if (m_r)
					bUpper = true;
				else
					bLower = true;
				if (bUpper && bLower)
					goto out;
			}
		}
		if (bUpper)
			cas = S(L_K_DOWNCASE);
		else if (bLower)
			cas = S(L_K_UPCASE);
	}
	if (cas==S(L_K_UPCASE) || cas==S(L_K_DOWNCASE)) {
		FPLispFunc pfn = cas==S(L_K_UPCASE) ? &CLispEng::F_CharUpcase : &CLispEng::F_CharDowncase;
		for (int i=0; i<vec.size(); i++) {
			CCharWithAttrs& ca = vec[i];
			if (ca.m_bReplaceable) {
				Push(CreateChar(ca.m_ch));
				(this->*pfn)();
				ca.m_ch = AsChar(m_r);
			}
		}
	}
out:
	return vec;
}

bool CLispEng::OnlyDotsOrEmpty(CTokenVec& vec) {
	for (int i=0; i<vec.size(); i++)
		if (!(vec[i].m_traits & TRAIT_DOT))
			return false;
	return true;
}

bool CLispEng::PotentialNumberP(CTokenVec& vec, CP pbase) {
	if (vec.HasEscapes || vec.empty() || (vec.back().m_traits & TRAIT_SIGN))
		return false;
	int base = AsNumber(pbase);
	bool bHasPoint = false,
		bHasDigit = false;
	int i;
	for (i=0; i<vec.size() && !(bHasPoint = vec[i].m_traits & TRAIT_POINT); i++)
		;
	for (i=0; i<vec.size(); i++) {
		CCharWithAttrs& ca = vec[i];
		if (ca.m_traits & TRAIT_ALPHADIGIT) {
			wchar_t ch = ca.m_ch;
			if (bHasPoint) {
				if (!isdigit(ch))
					ca.m_traits &= ~TRAIT_ALPHADIGIT;
			} else {
				int code = ToDigit(ch);
				if (code >= base)
					ca.m_traits &= ~TRAIT_ALPHADIGIT;
			}
			bHasDigit = bHasDigit || (ca.m_traits & TRAIT_ALPHADIGIT);
		}
		if (!(ca.m_traits & (TRAIT_ALPHADIGIT|TRAIT_SIGN|TRAIT_EXPONENT_MARKER|TRAIT_POINT|TRAIT_RATIO|TRAIT_EXTENSION)) ||
			!i && !(ca.m_traits & (TRAIT_ALPHADIGIT|TRAIT_SIGN|TRAIT_POINT|TRAIT_EXTENSION)))
			return false;
	}
	return bHasDigit && (vec.front().m_traits & (TRAIT_POINT|TRAIT_SIGN|TRAIT_ALPHADIGIT|TRAIT_EXPONENT_MARKER));
}

CP CLispEng::ReadToken(CP stm, bool bPreservingWhitespace) {
	CDynBindFrame dynBind(S(L_S_TERMINAL_READ_OPEN_OBJECT), S(L_SYMBOL), TerminalStreamP(stm));

	CTokenVec vec = ReadExtendedToken(stm, false, bPreservingWhitespace);
	if (Spec(L_S_READ_SUPPRESS))
		return 0;
	CP readBase = Spec(L_S_READ_BASE);
	if (PotentialNumberP(vec, readBase))
		return ToNumber(vec);
	CP pack = Spec(L_S_PACKAGE);
	for (CTokenVec::iterator i=vec.begin(), e=vec.end(); i!=e; ++i) {
		if (i->m_traits & TRAIT_PACKAGE) {
			CTokenVec::iterator j = i + 1;
			if (j == vec.end())
				E_Error();
			bool bExternal = !(j->m_traits & TRAIT_PACKAGE);
			if (!bExternal)
				++j;
			bool bKeyword = i==vec.begin() && j==i+1;
			String packPref;
			if (!bKeyword) {
				CTokenVec tokPack(vec.begin(), i);
				tokPack.HasEscapes = vec.HasEscapes;
				if (PotentialNumberP(tokPack, readBase))
					E_Error();
				packPref = TokenToString(tokPack);
			}
			CTokenVec tokSym(j, vec.end());
			tokSym.HasEscapes = vec.HasEscapes;
			if (PotentialNumberP(tokSym, readBase))
				E_Error();
			String name = TokenToString(tokSym);
			if (bKeyword)
				return GetKeyword(name);
			CSPtr pack;
			if (!Lookup(m_mapPackage, packPref, pack)) {
				CP packName = CreateString(packPref);
				E_PackageErr(packName, IDS_E_NoPackageWithName, packName);
			}
			CPackage *pPack = AsPackage(pack);
			
			if (!bExternal)
				return pPack->GetSymbol(_self, name);

			CP sym;
			byte flags;
			if (pPack->m_mapSym.FindCP(name, sym, flags) && (flags & PACKAGE_SYM_EXTERNAL))
				return sym;
			E_PackageErr(pack, IDS_E_NoExternalSymbol, pack, CreateString(name));
		}
	}
	if (OnlyDotsOrEmpty(vec)) {
		switch (vec.size())
		{
		case 0:
			goto LAB_NOT_DOTS;
		case 1:
			return m_specDot;
		}
		E_ReaderErr();
	}
LAB_NOT_DOTS:
	return GetSymbol(TokenToString(vec), pack);
}

/*!!!D
CSPtr CLispEng::ReadSymbol(CTextStream& is)
{
CSPtr r; //!!!
ReadToken(is);
const char *t = m_bufToken;
const char *p = strchr(t, ':');
if (p)
{
if (const char *q = strchr(p+1, ':'))
{
if (q != p+1)
E_Error();
CSPtr pack;
if (m_mapPackage.Lookup(NormalizeSymbol(String(t, p-t)), pack))
r = ToPackage(pack)->GetSymbol(_self, NormalizeSymbol(q+1));
else
E_Error();
}
else
{
if (p == t)
r = GetKeyword(t+1);
else if (p[1])
{
CSPtr pack;
if (m_mapPackage.Lookup(NormalizeSymbol(String(t, p-t)), pack))
r = ToPackage(pack)->GetExternalSymbol(NormalizeSymbol(p+1));
else
E_Error();
}
else
E_Error();
}
}
else if (!(r = ToNumber(m_bufToken)))
r = GetSymbol(NormalizeSymbol(m_bufToken), GetSymRef(ToSymbol(S(L_VAR_PACKAGE))));
return r;
}*/

CP CLispEng::ReadSValue(CP stm, bool bEofError, CP eofVal, bool bRec, bool bPreservingWhitespace) {
#ifdef _X_DEBUG//!!!D
	if (g_b)
		cout << ToBigInteger(0x10315) << endl;
#endif

	Push(stm, eofVal);
	CP r = eofVal;
	while (true) {
		int ich = ReadChar(stm, false);
		if (ich == EOF)
			if (bEofError)
				E_ReaderErr();
			else
				break;
		wchar_t ch = (wchar_t)ich;
		CCharType ct = GetCharType(ch);
		switch (ct.m_syntax)
		{
		case ST_WHITESPACE:
			continue;
		case ST_CONSTITUENT:
		case ST_SESCAPE:
		case ST_MESCAPE:
			PutBackChar(stm, ch);
			r = ReadToken(stm, bPreservingWhitespace);
			break;
		case ST_INVALID:
			E_ReaderErr();
		case ST_MACRO:
			Call(ct.m_macro, stm, CreateChar(ch));
			if (!m_cVal)
				continue;
			r = m_r;
		}
		/*!!!      bool tMust = m_bMustRPar,
		tMay = m_bMayRPar;
		m_bMustRPar = bMustRPar;
		m_bMayRPar = bMayRPar;*/
		//!!!        r = Eval(p);
		/*!!!      m_bMustRPar = tMust;
		m_bMayRPar = tMay;
		if (bMustRPar && m_r != m_specRPar)
		Throw(E_LISP_InvalidDottedPair);*/
		//    }
		//!!!    else
		//      return m_specEOF;
		break;
	}
	SkipStack(2);
	return r;
}

CP CLispEng::ReadRec(CP stm) {
	return ReadSValue(stm);/// (read t nil t);
}

class ReadLabelExc : public Exc {
public:
	ReadLabelExc()
		:	Exc(E_LISP_ReadLabel)
	{}
};

void CLispEng::ApplyRefs(CP& place, bool bMark) {
LAB_START:
	CP p = place & ~FLAG_Mark;
	switch (Type(p))
	{
	case TS_CONS:
	case TS_ARRAY:
	case TS_OBJECT:
	case TS_STRUCT:
		break;
	case TS_READLABEL:
		if (bMark) {
			for (CP alist=Spec(L_S_READ_REFERENCE_TABLE), cons; SplitPair(alist, cons);) {
				if (Car(cons) == p) {
					place = Cdr(cons) | (place & FLAG_Mark);
					goto LAB_START;
				}
			}
			throw ReadLabelExc();
		}
	default:
		return;
	}
	int ts = Type(p);
	int tidx = g_arTS[ts];
	if (tidx == -1)
		return;
	CValueManBase *vman = m_arValueMan[tidx];
	size_t i = AsIndex(p);
	bool bMarked = BitOps::BitTest(vman->m_pBitmap, i); //!!!
	if (!bMarked) {
		BitOps::BitTestAndSet(vman->m_pBitmap, (int)i);
		if (ts < TS_BIGNUM)
			m_arDeferred.push_back(p);  //!!!? WEAK-POINTER
	}

	/*!!!R
	byte& b = *((byte*)vman->m_pBase+i*vman->m_sizeof);
	if ((b & FLAG_Mark) ^ (bMark ? FLAG_Mark : 0))
	{
	b ^= FLAG_Mark;
	m_arDeferred.push_back(p);
	}
	*/
}

void CLispEng::ApplyRefsSValue(CP& p, bool bMark) {
	DWORD_PTR idx = AsIndex(p);
	switch (Type(p))
	{
	case TS_CONS:
		{
			CConsValue *cons = m_consMan.TryApplyCheck(idx);
			ApplyRefs(cons->m_cdr, bMark);
			ApplyRefs(cons->m_car, bMark);
		}
		break;
	case TS_ARRAY:
	case TS_OBJECT:
	case TS_STRUCT:
		{
			CArrayValue *av = m_arrayMan.TryApplyCheck(idx);
			if (!(av->m_flags & FLAG_Displaced) && av->m_elType==ELTYPE_T) {
				size_t size = av->TotalSize();
				for (int i=0; i<size; i++)
					ApplyRefs(av->m_pData[i], bMark); 
			}
		}
		break;
	default:
		return;
	}
}

void CLispEng::MarkRefs(CP p, bool bMark) {
	ClearBitmaps();
	m_arDeferred.clear();
	ApplyRefs(p, bMark);
	while (!m_arDeferred.empty()) {
		CP q = m_arDeferred.back();
		m_arDeferred.pop_back();
		ApplyRefsSValue(q, bMark);
	}
}

void CLispEng::MakeReferences(CP p) {
	if (!Spec(L_S_READ_REFERENCE_TABLE))
		return;
	try {
		class CMarkKeeper {
			CP& m_p;
		public:
			CMarkKeeper(CP& p)
				:	m_p(p)
			{
				Lisp().MarkRefs(m_p, true);
			}

			~CMarkKeeper() {
				//!!!R				Lisp().MarkRefs(m_p, false);
			}
		} markKeeper(p);	
	} catch (ReadLabelExc&) {
		E_Error();
	}
}

CP CLispEng::ReadTop(CP stm, bool bEofError, CP eofVal, bool bPreservingWhitespace) {
	CDynBindFrame dynBind(S(L_S_READ_REFERENCE_TABLE), 0, true);
	m_r = ReadSValue(stm, bEofError, eofVal, false, bPreservingWhitespace);
	MakeReferences(m_r);
	return m_r;
}

void CLispEng::Read(bool bPreservingWhitespace) {
	bool bRecur = ToOptionalNIL(Pop());
	CP *sp = m_pStack;
	if (!bRecur) {
		CDynBindFrame dynBind(S(L_S_READ_REFERENCE_TABLE), 0, true);
		m_r = ReadTop(TestIStream(sp[2]), ToOptional(sp[1], V_T), ToOptionalNIL(sp[0]), bPreservingWhitespace);
		MakeReferences(m_r);
	} else
		m_r = ReadSValue(TestIStream(sp[2]), ToOptional(sp[1], V_T), ToOptionalNIL(sp[0]), bRecur, bPreservingWhitespace);
	m_cVal = 1;
	SkipStack(3);
}

void CLispEng::LoadFromStream(CP stm) {
	Push(stm);
	for (CP form; (form=ReadTop(SV, false, V_EOF, false))!=V_EOF;) {
		if (Type(form) == TS_CCLOSURE)
			Call(form);
		else {
			m_cVal = 1;
			m_r = Eval(form);
		}
	}
	SkipStack(1);
}

void CLispEng::F_Read() {
	Read(false);
}

void CLispEng::F_ReadPreservingWhitespace() {
	Read(true);
}

void CLispEng::F_ReadDelimitedList() {
	CP dotAllowed = ToOptionalNIL(Pop()),
		recur = ToOptionalNIL(Pop()),
		stm = Pop(),
		ch = Pop();
	CListConstructor lc;
	bool bDot = false,
		bAtStart = true,
		bEnded = false;

	CDynBindFrame dynBind;
	if (!recur)
		dynBind.Bind(_self, S(L_S_READ_REFERENCE_TABLE), 0);
	int count = !recur;
	if (TerminalStreamP(stm)) {
		dynBind.Bind(_self, S(L_S_TERMINAL_READ_OPEN_OBJECT), S(L_LIST));
		++count;
	}
	dynBind.Finish(_self, count);

	while (true) {
		Push(V_T, stm, V_T, 0, recur);
		F_PeekChar();  //!!! save because m_r cleared by Peek-char
		if (!m_r)
			E_Error();
		if (m_r == ch) {
			if (bDot)
				E_Error();
			break;
		}
		CCharType ct = GetCharType(AsChar(m_r));
		if (ct.m_syntax == ST_MACRO) {
			Call(ct.m_macro, stm, CreateChar((wchar_t)ReadChar(stm)));
			if (!m_cVal)
				continue;
		} else {
			if (bEnded)
				E_Error();
			m_r = ReadSValue(stm);
			if (m_r == m_specDot) {
				if (bDot || bAtStart)
					E_Error();
				bDot = true;
				continue;
			}
		} if (bDot) {
			lc.SetCdr(m_r);
			bDot = false;
			bEnded = true;
		} else {
			lc.Add(m_r);
			bAtStart = false;
		}
	}
	ReadChar(stm);
	m_r = lc;
}

void CLispEng::F_M_LeftParenthesis() {
	SkipStack(1);
	CP stm = Pop();
	Push(V_RPAR, stm, V_T, V_T); //!!!Q need recursive?
	F_ReadDelimitedList();  //!!! non-standard &optional arg CanDot
}

bool CLispEng::EvalFeature(CP p) {
	return Memq(p, Spec(L_S_FEATURES)); //!!!D GetSymRef(ToSymbol(GetSymbol("*FEATURES*", m_packCL))));
}

/*!!!D
CSPtr CLispEng::ReadPlus(CP stm)
{
CSPtr t = ReadSValue(stm), //!!!
e = ReadSValue(stm);
if (EvalFeature(t))
return e;
else
return ReadSValue(stm, false, true);
}

CSPtr CLispEng::ReadMinus(CP stm)
{
CSPtr t = ReadSValue(stm), //!!!
e = ReadSValue(stm);
if (EvalFeature(t))
return ReadSValue(stm, false, true);
else
return e;
}
*/

void CLispEng::F_Defio() {
	SkipStack(2);
	//!!!?
}

void CLispEng::F_M_Sharp() {
	CP dispChar = Pop();
	int ch = ReadChar(SV);
	int n = -1;
	if (isdigit(ch)) {
		n = 0;
		do {
			n = n*10+ch-'0';
			ch = ReadChar(SV);
		} while (isdigit(ch));
	}
	Push(m_r);
	F_CharUpcase();
	CP subChar = m_r;
	Push(dispChar, subChar, V_U);
	F_GetDispatchMacroCharacter();
	if (!m_r)
		E_Error();
	Call(m_r, SV, subChar, n==-1 ? 0 : CreateInteger(n)); //!!!Q may be need toupper()?
	SkipStack(1);
}

void CLispEng::F_M_Semicolon() {
	for (int ch; (ch=ReadChar(SV1, false))!=EOF && ch!='\n';)
		;
	m_cVal = 0;
	SkipStack(2);
}

void CLispEng::F_M_Quote() {
	m_r = List(S(L_QUOTE), ReadRec(SV1));
	SkipStack(2);
}

void CLispEng::F_M_DQuote() {
	vector<String::Char> vec;
	for (int chTerm=AsChar(SV), ch; (ch=ReadChar(SV1))!=chTerm;) {
		if (ch == '\\')
			ch = ReadChar(SV1);
		vec.push_back((String::Char)ch);
	}
	m_r = CreateString(vec.empty() ? String() : String(&vec[0], vec.size()));
	SkipStack(2);
	return;
}

void CLispEng::F_M_BQuote() {
	SkipStack(1);
	CP p = ReadSValue(Pop());
	if (ConsP(p) && (Car(p)==S(L_SPLICE) || Car(p)==S(L_NSPLICE)))
		E_Error();
	m_r = List(S(L_BACKQUOTE), p);
}

void CLispEng::F_M_Comma() {
	SkipStack(1);
	int ch = ReadChar(SV);
	CSPtr q; //!!!
	switch (ch)
	{
	case '@':
		q = S(L_SPLICE);
		break;
	case '.':
		q = S(L_NSPLICE);
		break;
	default:
		Push(m_r, SV);
		F_UnreadChar();
		q = S(L_UNQUOTE);
	}
	m_r = List(q, ReadSValue(Pop()));
}

void CLispEng::F_M_RightParenthesis() {
	CStreamValue *stm = ToStream(SV1);

	E_StreamErr(IDS_E_ObjectCannotStart, SV1, SV);
}

void CLispEng::CheckNilInfix(CP infix, CP stm) {
	if (Type(stm)!=TS_STREAM || infix) //!!!
		E_Error();
}

void CLispEng::F_Sharp_VerticalBar() {
	CheckNilInfix(SV, SV2);
	CP stm = SV2;
	while (true) {
		switch (int ch = ReadChar(stm))
		{
		case '|':
			ch = ReadChar(stm);
			if (ch == '#') {
				SkipStack(3);
				m_cVal = 0;
				return;
			}
			PutBackChar(stm, (wchar_t)ch);
			break;
		case '#':
			ch = ReadChar(stm);
			if (ch == '|') {
				Push(stm, CreateChar((wchar_t)ch), 0);
				F_Sharp_VerticalBar();
			} else
				PutBackChar(stm, (wchar_t)ch);
			break;
		}
	}
}

void CLispEng::F_Sharp_Quote() {
	CheckNilInfix(SV, SV2);
	m_r = List(S(L_FUNCTION), ReadRec(SV2));
	SkipStack(3);
}

void CLispEng::F_Sharp_Backslash() {
	CheckNilInfix(SV, SV2);
	CTokenVec vec = ReadExtendedToken(SV2, true);
	if (Spec(L_S_READ_SUPPRESS))
		m_r = 0;
	else if (vec.size() == 1)
		m_r = CreateChar(vec.front().m_ch);
	else {
		Call(S(L_NAME_CHAR), CreateString(TokenToString(vec)));
		if (!m_r)
			E_Error();
	}
	SkipStack(3);
}

void CLispEng::F_Sharp_Dot() {			// must be SUBR coz need to load .FAS
	CP p = ReadSValue(SV2, true, 0, true);
	if (Spec(L_S_READ_SUPPRESS))
		m_r = 0;
	else {
		if (SV)
			E_Error();
		MakeReferences(p); //!!!
		if (!Spec(L_S_READ_EVAL))
			E_Error(); //!!!
		m_r = Eval(p);
	}
	m_cVal = 1;
	SkipStack(3);
}

void CLispEng::F_Sharp_Colon() {
	CheckNilInfix(SV, SV2);
	CTokenVec vec = ReadExtendedToken(SV2);
	for (CTokenVec::iterator i=vec.begin(); i!=vec.end(); ++i)
		if (i->m_traits & TRAIT_PACKAGE)
			E_Error();
	m_r = FromSValue(CreateSymbol(TokenToString(vec)));
	SkipStack(3);
}

bool CLispEng::InterpretFeature(CP p) {
	if (Type(p) == TS_SYMBOL)
		return Memq(p, Spec(L_S_FEATURES));
	CP car;
	if (!SplitPair(p, car))
		E_Error();
	switch (car)
	{
	case S(L_K_NOT):
		return !InterpretFeature(Car(p));
	case S(L_K_AND):
		while (SplitPair(p, car))
			if (!InterpretFeature(car))
				return false;
		return true;
	case S(L_K_OR):
		while (SplitPair(p, car))
			if (InterpretFeature(car))
				return true;
		return false;
	default:
		E_Error();
	}	
}

void CLispEng::SharpCond(bool bPlus) {
#ifdef _DEBUG //!!!D
	CSymbolValue *sv = AsSymbol(S(L_S_READ_SUPPRESS));
#endif
	CP stm = SV2;
	{
		CDynBindFrame dynBindSuppress(S(L_S_READ_SUPPRESS), 0, true);
		CDynBindFrame dynBind(S(L_S_PACKAGE), m_packKeyword, true);
		bPlus ^= InterpretFeature(ReadSValue(stm)) ^ 1;
	}
	{
		CDynBindFrame dynBind(S(L_S_READ_SUPPRESS), V_T, !bPlus);
		m_r = ReadSValue(stm);
		if (!bPlus)
			m_cVal = 0;
	}
	SkipStack(3);
}

void CLispEng::F_Sharp_Plus() {
	SharpCond(true);
}

void CLispEng::F_Sharp_Minus() {
	SharpCond(false);
}

void CLispEng::F_Sharp_LPar() {
	if (SV)
		E_Error(); //!!! we support only simple #(
	Push(V_RPAR, SV2, V_T, V_T);
	F_ReadDelimitedList();
	CArrayValue *vec = CreateVector(Length(m_r));
	int i = 0;
	for (CP car; SplitPair(m_r, car); i++)
		vec->SetElement(i, car);
	m_r = FromSValue(vec);
	SkipStack(3);
}

void CLispEng::F_Sharp_Asterisk() {
	CTokenVec vec = ReadExtendedToken(SV2, false);
	ClearResult();
	if (!Spec(L_S_READ_SUPPRESS)) {
		size_t n;
		if (SV) {
			n = AsPositive(SV);
			if (n<vec.size() || n && vec.empty())
				E_Error();
		} else
			n = vec.size();
		int i;
		for (i=0; i<vec.size(); i++) {
			CCharWithAttrs& cwa = vec[i];
			if (!(cwa.m_traits & TRAIT_ALPHADIGIT) || cwa.m_ch!='0' && cwa.m_ch!='1')
				E_Error();
		}
		CArrayValue *av = CreateVector(n, ELTYPE_BIT);
		for (i=0; i<n; i++)
			av->SetElement(i, CreateFixnum((i<vec.size() ? vec[i] : vec.back()).m_ch-'0'));
		m_r = FromSValue(av);
	}
	SkipStack(3);
}

/*!!!R
class CChangeRadix
{
int m_prevRadix;
public:
CChangeRadix(int radix)
{
CLispEng& lisp = Lisp();
CP& base = lisp.AsSymbol(lisp.GetSymbol("*READ-BASE*", lisp.m_packCL))->m_dynValue;
m_prevRadix = AsNumber(base);
base = lisp.CreateInteger(radix);
}

~CChangeRadix()
{
CLispEng& lisp = Lisp();
lisp.AsSymbol(lisp.GetSymbol("*READ-BASE*", lisp.m_packCL))->m_dynValue = lisp.CreateInteger(m_prevRadix);
}
};


CSPtr CLispEng::ReadHexAsmList(CP stm)
{
CChangeRadix changeRadix(16);
return ReadSValue(stm); //!!! only opcodes, need ASM interpreter
}

void CLispEng::F_Sharp_H()
{
m_r = ReadHexAsmList(SV2);
SkipStack(3);
}
*/

void CLispEng::F_Sharp_S() {
	Push(ReadSValue(SV2));
	if (Car(SV) != S(L_HASH_TABLE))
		E_Error();
	Push(ToSymbol(Car(Cdr(SV)))->GetFun(), V_1, V_1, V_U);
	Push(V_U, V_U, V_U, V_U, V_U);
	F_MakeHashTable();
	Push(m_r);
	for (CP p=Cdr(Cdr(SV1)), car; SplitPair(p, car);) {
		Push(Car(car), SV, Cdr(car));
		F_PutHash();
	}
	m_r = Pop();
	SkipStack(4);
}

void CLispEng::F_MakeCodeVector() {
	CArrayValue *vec = CreateVector(Length(SV), ELTYPE_BYTE);
	byte *pb = (byte*)vec->m_pData;
	for (CP q; SplitPair(SV, q);)
		*pb++ = (byte)AsNumber(q);
	m_r = FromSValue(vec);
	SkipStack(1);
}

void CLispEng::F_MakeClosure() {
	SkipStack(3);
	CClosure *c = CreateClosure(Length(SV));
	for (CP p=Pop(), car, *q=c->Consts; SplitPair(p, car);)
		*q++ = car;
	m_r = FromSValueT(c, TS_CCLOSURE);
	c->CodeVec = Pop();
#ifdef X_DEBUG//!!!D
	CArrayValue *dbgAv = ToArray(c->CodeVec);
	dbgAv = dbgAv;
#endif
	if (ToArray(c->CodeVec)->GetElementType() == ELTYPE_T) {
		CArrayValue *oav = AsArray(c->CodeVec),
			*nav = CreateVector(oav->DataLength, ELTYPE_BYTE);
		for (int i=0; i<oav->DataLength; i++) {
			LONG_PTR v = AsNumber(oav->GetElement(i));
#ifdef X_DEBUG//!!!D
			if (v == 0x83)
				v = v;
#endif
			((byte*)nav->m_pData)[i] = (byte)v;
		}
		c->CodeVec = FromSValue(nav);
	}
	c->NameOrClassVersion = Pop();
}

void CLispEng::F_Sharp_Y() {
	CP arg = SV,
		stm = SV2;
	SkipStack(3);
	if (arg) {
		if (arg == V_0) {
			switch (int ch = ReadChar(stm))
			{
			case '_':
				ToStream(stm)->m_flags |= STREAM_FLAG_FASL;
				break;
			case '^':
				ToStream(stm)->m_flags &= ~STREAM_FLAG_FASL;
				break;
			default:
				PutBackChar(stm, (wchar_t)ch);
				CDynBindFrame dynBind(S(L_S_READ_SUPPRESS), V_T, true);
				m_r = ReadSValue(stm);
			}
			m_cVal = 0;
		} else {
			CDynBindFrame dynBind(S(L_S_READ_BASE), V_16, true);
			CP p = ReadSValue(stm, true, 0, true);
			m_cVal = 1;
			if (Spec(L_S_READ_SUPPRESS))
				m_r = 0;
			else {
				if (Length(p) != AsNumber(arg))
					E_Error();
				Push(p);
				F_MakeCodeVector();
			}
		}
	} else {
		CP p = ReadSValue(stm, true, 0, true);
		if (Spec(L_S_READ_SUPPRESS))
			m_r = 0;
		else {
			Push(Car(p), Car(Cdr(p)), Cdr(Cdr(Cdr(p))), Car(Cdr(Cdr(p))));
			Push(V_U, V_U); //!!!
			m_cVal = 1;
			F_MakeClosure();
		}
	}
}

void CLispEng::F_MakeReadLabel() {
	m_r = CreateReadLabel(Pop());
}


} // Lisp::

