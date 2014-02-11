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

#ifdef C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR

CP CLispEng::BackquoteEx(RCPtr p, bool& bAppend, int level) {
	bAppend = false;
	if (!ConsP(p))
		return p;
	CP f = 0, r = p;
	SplitPair(r, f);
	bool b;
	if (f == S(L_BACKQUOTE))
		return List(f, BackquoteEx(Car(r), b, level+1));
	else if (f == S(L_UNQUOTE)) {
		m_cVal = 1;
		return level ? List(f, BackquoteEx(Car(r), b, level-1)) : Eval(Car(r));		
	} else if (f == S(L_SPLICE)) {
		if (level)
			return List(f, BackquoteEx(Car(r), b, level-1));
		bAppend = true;
		return Eval(Car(r));
	} else {
		bool b, b1;
		Push(BackquoteEx(f, b, level)); //!!!
		CP s = BackquoteEx(r, b1, level); //!!!
		if (b)
			return AppendEx(Pop(), s);
		return Cons(Pop(), s);
	}
}

void CLispEng::F_Backquote() {
  bool b;
  m_r = BackquoteEx(SV, b, 0);
#ifdef _X_DEBUG
	cerr << "\n--------------Backquote-------------\n";
	PrintForm(cerr, SV);
	cerr << "\n";
	PrintForm(cerr, m_r);
#endif
	SkipStack(1);
}

#else

CP CLispEng::BqTransform(CP form) {
	if (!ConsP(form))
		return List(S(L_LIST), BqExpand(form));
	CP p = form, car;
	SplitPair(p, car);
	if (car == S(L_UNQUOTE))
    return List(S(L_LIST), Car(p));
	if (car == S(L_SPLICE))
    return Car(p);
	if (car == S(L_NSPLICE))
		return List(S(L_BQ_NCONC), Car(p));
	if (car == S(L_BACKQUOTE))
    return List(S(L_LIST), List(S(L_BACKQUOTE), BqExpand(Car(p))));
  return List(S(L_LIST), BqExpand(form));
}

CP CLispEng::BqExpandList(CP form) {
	if (!form)
		return 0;
	CP d = Cdr(form),
		 r;
	if (ConsP(d)) {
		CP p = d, car;
		SplitPair(p, car);
		if (car == S(L_UNQUOTE))
			return List(BqTransform(Car(form)), Car(p));
		if (car==S(L_SPLICE) || car==S(L_NSPLICE))
		{
			Print(form); //!!!
			E_Error();
		}
		Push(d);
		Push(BqTransform(Car(form)));
		r = Cons(SV, BqExpandList(d));
		SkipStack(1);
	} else if (!d)
		return List(BqTransform(Car(form)));
	else {
		Push(BqTransform(Car(form)));
		r = List(SV, List(S(L_QUOTE), d));
	}
	SkipStack(1);
	return r;
}

CP CLispEng::BqExpand(CP form) {
	switch (Type(form))
	{
	case TS_CONS:
		{
			if (!form)
				return 0;
			CP p = form, car;
			SplitPair(p, car);
			if (car == S(L_UNQUOTE))
				return Car(p);
			if (car==S(L_SPLICE) || car==S(L_NSPLICE))
				E_Error();
			if (car == S(L_BACKQUOTE))
				return List(S(L_BACKQUOTE), BqExpand(Car(p)));
			return Cons(S(L_APPEND), BqExpandList(form));
		}
	case TS_ARRAY:
		if (VectorP(form) && AsArray(form)->m_elType==ELTYPE_T) {
			Call(S(L_MAP), S(L_LIST), S(L_IDENTITY), form);
			return List(S(L_APPLY), AsSymbol(S(L_VECTOR))->GetFun(), //!!!
				          Cons(S(L_NCONC), BqExpandList(m_r)));
		}
	default:
		return List(S(L_QUOTE), form);
	}
}

void CLispEng::F_BqExpand() {
#ifdef _DEBUG
//	cerr << "\n----------F_BqExpand-------------------\n";
//	PrintForm(cerr, SV);
#endif

	m_r = BqExpand(SV);
	SkipStack(1);
/*!!!  bool b;
  m_r = BackquoteEx(Pop(), b, 0);*/
}

#endif
} // Lisp::

