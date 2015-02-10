#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

/*!!!D
CLispEng::CCharName CLispEng::s_arCharName[] =
{
'\0',	"NULL",
'\a',	"BELL",
'\b',	"BS",
'\t',	"TAB",
'\n',	"NEWLINE",
'\n',	"NL",
'\n',	"LINEFEED",
'\n',	"LF",
'\v',	"VT",
'\f',	"PAGE",
'\f',	"FORM",
'\f',	"FORMFEED",
'\f',	"FF",
'\r',	"RETURN",
'\r',	"CR",
'\x1B',	"ESCAPE",
'\x1B',	"ESC",
'\x1B',	"ALTMODE",
'\x1B',	"ALT",
' ',	"SPACE",
' ',	"SP",
'\x7F',	"DELETE",
'\x7F',	"RUBOUT"
};*/

void CLispEng::F_CharCode() {  // CHAR-CODE/CHAR-INT
	m_r = CreateFixnum((uint16_t)AsChar(Pop()));
}

void CLispEng::F_CodeChar() {
	m_r = CreateChar((wchar_t)AsNumber(Pop()));
}

void CLispEng::F_CharUpcase() {
	wchar_t ch = AsChar(Pop());
	m_r = CreateChar(toupper(ch, m_locale));
}

void CLispEng::F_CharDowncase() {
	wchar_t ch = AsChar(Pop());
	m_r = CreateChar(tolower(ch, m_locale));
}

void CLispEng::F_StandardCharP() {
	wchar_t ch = AsChar(Pop());
	m_r = FromBool(ch >= ' ' && ch <= '~' || ch == '\n');
}

void CLispEng::F_AlphaCharP() {
	wchar_t ch = AsChar(Pop());
	m_r = FromBool(isalpha(ch, m_locale));
}

void CLispEng::F_AlphanumericP() {
	Push(SV);
	F_AlphaCharP();
	if (m_r)
		SkipStack(1);
	else {
		Push(V_U);
		F_DigitCharP();
	}
}

int CLispEng::TestRadix(CP p) {
	int radix = AsNumber(p);
	if (radix<2 || radix>36)
		E_TypeErr(p, List(S(L_INTEGER), V_2, V_36));
	return radix;
}

int CLispEng::TestRadix() {
	return TestRadix(ToOptional(Pop(), V_10));
}

void CLispEng::F_DigitCharP() {
	int radix = TestRadix();
	wchar_t ch = AsChar(Pop());
	if (ch > 'z' || ch < '0')
		return;
	if (ch >= 'a')
		ch -= 'a'-'A';
	if (ch <= '9')
		ch -= '0';
	else if (ch >= 'A')
		ch -= 'A'-10;
	else
		return;
	if (ch < radix)
		m_r = CreateFixnum(ch);
}

void CLispEng::F_DigitChar() {
	int radix = TestRadix();
	int w = (int)AsPositive(Pop());
	if (w < radix)
		m_r = DigitChar(w);
}

void CLispEng::F_UpperCaseP() {
	wchar_t ch = AsChar(Pop());
	m_r = FromBool(isupper(ch, m_locale));
}

void CLispEng::F_LowerCaseP() {
	wchar_t ch = AsChar(Pop());
	m_r = FromBool(islower(ch, m_locale));
}

void CLispEng::F_BothCaseP() {
	Push(SV);
	F_UpperCaseP();
	if (m_r)
		SkipStack(1);
	else
		F_LowerCaseP();
}



} // Lisp::

