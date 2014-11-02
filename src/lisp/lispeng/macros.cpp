/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

#include "lispeng.h"

// Macros, implemented as Special Operators for efficiency
#if UCFG_LISP_MACRO_AS_SPECIAL_OPERATOR

namespace Lisp {

void CLispEng::F_Cond() {
	LISP_TAIL_REC_DISABLE_1;

	for (CP clause, test; SplitPair(SV, clause);) {
		if (!SplitPair(clause, test))
			E_Error();
		else if (m_r = Eval(test)) {
			if (!clause)
				break;
			SkipStack(1);

#if UCFG_LISP_TAIL_REC == 1
			LISP_TAIL_REC_RESTORE;
#endif
			Progn(clause);
			return;
		}
	}
	SkipStack(1);
	m_cVal = 1;
}

void CLispEng::F_Case() {
	LISP_TAIL_REC_DISABLE_1;

	m_cVal = 1;
	m_r = Eval(SV1);
	CP clauses = SV,
		 c;
	SkipStack(2);
	for (CP keys; SplitPair(clauses, c);) {
		if (!SplitPair(c, keys))
			E_Error();
		else if (Type(keys) != TS_CONS) {
			if (keys==V_T || keys==S(L_OTHERWISE)) {
				if (!clauses)
					goto LAB_EVAL;
				else
					E_Error();
			} else if (Eql(keys, m_r))
				goto LAB_EVAL;
		} else {
			for (CP car; SplitPair(keys, car);)
				if (Eql(car, m_r))
					goto LAB_EVAL;
		}
	}
	c = 0;
LAB_EVAL:
#if UCFG_LISP_TAIL_REC == 1
	LISP_TAIL_REC_RESTORE;
#endif
	Progn(c);
}

void CLispEng::AndOr(bool bAnd) {
	LISP_TAIL_REC_DISABLE_1;

	m_r = FromBool(bAnd);
	for (CP car; SplitPair(SV, car);) {
#if UCFG_LISP_TAIL_REC
		if (!SV) {
			LISP_TAIL_REC_RESTORE;
		}
#endif
		m_cVal = 1;
		m_r = Eval(car);
		if (!SV)
			break;
		if (bool(m_r) ^ bAnd) {
			m_cVal = 1;
			break;
		}
	}
	SkipStack(1);
}

void CLispEng::F_And() {
	AndOr(true);
}

void CLispEng::F_Or() {
	AndOr(false);
}

CP CLispEng::BindVarMVB(int n, CP& sym, CP form) {
	return n>=m_cVal ? 0 : m_arVal[n];
}

void CLispEng::F_MultipleValueBind() {
	LISP_TAIL_REC_DISABLE_1;

	if (ParseCompileEvalForm(3))
		return;
	SV = m_r;
	for (CP p=SV2, car; SplitPair(p, car);)
		if (Type(car) != TS_SYMBOL)
			E_Error();
	m_cVal = 1;
	m_r = Eval(SwapRet(SV1, m_arVal[1]));

#if UCFG_LISP_TAIL_REC == 1
	LISP_TAIL_REC_RESTORE;
#endif

	CallBindedForms(&CLispEng::BindVarMVB, 0, SV2, SV1, SV);
	SkipStack(3); //!!! Check SP
}

void CLispEng::F_PSetq() {
	LISP_TAIL_REC_KEEPER(false);

	if (CheckSetqBody(S(L_PSETF)))
		return;
	CP body = SV;
	int len = Length(body)/2;
	int i;
	for (i=len; i--;) {
		CP p, q;
		SplitPair(body, p);
		SplitPair(body, q);
		Push(p);
		Push(Eval(q));
	}
	for (i=len; i--; SkipStack(2))
		Setq(SV1, SV);
	SkipStack(1);
	m_cVal = 1;
	m_r = 0;
}


} // Lisp::

#endif

