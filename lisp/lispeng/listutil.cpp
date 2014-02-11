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

int CLispEng::Length(CP p) {
	int r = 0;
	for (CP car; SplitPair(p, car);)
		r++;
	return r;
}

bool __fastcall CLispEng::Memq(CP x, CP list) {
	for (CP car; SplitPair(list, car);)
		if (x == car)
			return true;
	return false;
}

CP CLispEng::Adjoin(CP x, CP list) {
	return Memq(x, list) ? list : Cons(x, list);
}

CConsValue *CLispEng::Assoc(CP item, CP alist) {
	for (CP car; SplitPair(alist, car);) {
		CConsValue *cons = AsCons(car);
		if (cons->m_car == item)
			return cons;
	}
	return 0;
}

CP CLispEng::ListofEx(ssize_t n) {
	for (; n-- > 1; SkipStack(1))
		SV1 = Cons(SV1, SV);
	return Pop();
}

CP __fastcall CLispEng::Listof(ssize_t n) {
	Push(0);
	return ListofEx(n+1);
}

void CLispEng::Remf(CSPtr& plist, CP p) {
	for (CSPtr* q=&plist; *q; q=(CSPtr*)&ToCons(Cdr(*q))->m_cdr)
		if (Car(*q) == p) {
			*q = Cdr(Cdr(*q));
			return;
		}
}

CP CLispEng::Remq(CP x, CP list) {
	int count = 0;
	for (CP p=list, car; SplitPair(p, car); ++count) {
		if (car == x) {
			Push(p);
			return ListofEx(count+1);
		} else
			Push(car);
	}
	SkipStack(count);
	return list;
}

void CLispEng::DeleteFromList(CP x, CSPtr& list) {
	for (CSPtr* q=&list; *q; q=(CSPtr*)&ToCons(*q)->m_cdr)
		if (Car(*q) == x) {
			*q = Cdr(*q);
			return;
		}
}

CP CLispEng::CopyList(CP p) {
	Push(p);
	CP r = 0;
	{
		if (Type(p) != TS_CONS)
			E_TypeErr(p, S(L_LIST));
		{
			CListConstructor lc;
			for (CP car; SplitPair(p, car);) {
				lc.Add(car);
				if (!ConsP(p)) {
					lc.SetCdr(p);
					r = lc;
					break;
				}
			}
		}
	}
	SkipStack(1);
	return r;
}

CP __fastcall CLispEng::NReverse(CP p) {
	if (p) {
		CConsValue *consP = ToCons(p);
		if (CSPtr p3 = consP->m_cdr) {
			CConsValue *consP3 = ToCons(p3);
			if (consP3->m_cdr) {
				CSPtr p1 = p3,
					p2;
				do {
					consP3 = ToCons(p3 = SwapRet((CSPtr&)consP3->m_cdr, SwapRet(p2, p3)));
				} while (consP3->m_cdr);
				consP->m_cdr = p2;
				ToCons(p1)->m_cdr = p3;
			}
			consP->m_car = SwapRet(consP3->m_car, consP->m_car);
		}
	}
	return p;
}

CP CLispEng::AppendEx(CP p, CP q) {
	if (q) {
		int n = 0;
		for (CP car; SplitPair(p, car); n++)
			Push(car);
		Push(q);
		return ListofEx(n+1);
	}
	return p;
}

void CLispEng::MvToStack() {
	for (int i=0; i<m_cVal; i++)
		Push(m_arVal[i]);
}

void CLispEng::StackToMv(size_t n) {
	if (n > _countof(m_arVal))
		E_Err(IDS_E_TooManyValues, GetSubrName(m_subrSelf));
	if (m_cVal = n) {
		do {
			m_arVal[n-1] = m_pStack[m_cVal-n];
		} while (--n);
		SkipStack(m_cVal);
	} else
		m_r = 0;
}

void CLispEng::MvToList() {
	CSPtr p;
	for (size_t i=m_cVal; i--;)
		p = Cons(m_arVal[i], p);
	m_r = p;
	m_cVal = 1;
}

void CLispEng::ListToMv(CP p) {
	m_cVal = 0;
	for (CP car; SplitPair(p, car);)
		m_arVal[m_cVal++] = car;
}

CP CLispEng::List(CP a0) {
	Push(a0);
	return Listof(1);
}

CP CLispEng::List(CP a0, CP a1) {
	Push(a0, a1);
	return Listof(2);
}

CP CLispEng::List(CP a0, CP a1, CP a2) {
	Push(a0, a1, a2);
	return Listof(3);
}

CP CLispEng::List(CP a0, CP a1, CP a2, CP a3) {
	Push(a0, a1, a2, a3);
	return Listof(4);
}

CP CLispEng::List(CP a0, CP a1, CP a2, CP a3, CP a4) {
	Push(a0, a1, a2, a3);
	Push(a4);
	return Listof(5);
}

CP CLispEng::ListEx(CP a0, CP a1, CP a2) {
	Push(a0, a1, a2);
	return ListofEx(3);
}

} // Lisp::

