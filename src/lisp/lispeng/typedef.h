/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
//               	IsNum	Container

LTYPE(TS_CONS		, 0		, m_consMan)
LTYPE(TS_SYMBOL		, 0		, m_symbolMan)
LTYPE(TS_OBJECT		, 0		, m_arrayMan)
LTYPE(TS_INTFUNC	, 0		, m_intFuncMan)
LTYPE(TS_CCLOSURE	, 0		, m_arrayMan)
LTYPE(TS_STREAM		, 0		, m_streamMan)
LTYPE(TS_PACKAGE	, 0		, m_packageMan)
LTYPE(TS_ARRAY		, 0		, m_arrayMan)

LTYPE(TS_HASHTABLE	, 0		, m_hashTableMan)
LTYPE(TS_READTABLE	, 0		, m_readtableMan)
LTYPE(TS_PATHNAME	, 0		, m_pathnameMan)
LTYPE(TS_COMPLEX	, 1		, m_consMan)
LTYPE(TS_RATIO		, 1		, m_consMan)

#if UCFG_LISP_BUILTIN_RANDOM_STATE
LTYPE(TS_RANDOMSTATE, 0		, m_consMan)
#endif

LTYPE(TS_MACRO		, 0		, m_consMan)
LTYPE(TS_SYMBOLMACRO, 0		, m_consMan)
LTYPE(TS_GLOBALSYMBOLMACRO, 0, m_consMan)

LTYPE(TS_FUNCTION_MACRO	, 0	, m_consMan)
LTYPE(TS_READLABEL	, 0		, m_consMan)
LTYPE(TS_STRUCT		, 0		, m_arrayMan)			// < TS_BIGNUM are compound types
LTYPE(TS_BIGNUM		, 1		, m_bignumMan)

#if UCFG_LISP_GC_USE_ONLY_BITMAP
	LTYPE(TS_FLONUM		, 1		, m_consMan)
#else
	LTYPE(TS_FLONUM		, 1		, m_floatMan)
#endif

#if UCFG_LISP_FFI
	LTYPE(TS_FF_PTR		, 0		, m_consMan)
#endif

LTYPE(TS_WEAKPOINTER, 0		, m_weakPointerMan)
LTYPE(TS_CHARACTER	, 0		, NONE_MAN)				// >=TS_CHARACTER are non-object types

LTYPE(TS_FIXNUM		, 1		, NONE_MAN)
LTYPE(TS_FRAME_PTR	, 0		, NONE_MAN)
LTYPE(TS_SPECIALOPERATOR, 0	, NONE_MAN)
LTYPE(TS_SUBR		, 0		, NONE_MAN)
LTYPE(TS_FRAMEINFO	, 0		, NONE_MAN)
LTYPE(TS_STACK_RANGE, 0		, NONE_MAN)

//*****************************************

LMAN(CConsValue			, TS_CONS			, m_consMan			, INIT_HEAPSIZE_CONS)
LMAN(CSymbolValue		, TS_SYMBOL			, m_symbolMan		, INIT_HEAPSIZE_SYMBOL)

#if !UCFG_LISP_GC_USE_ONLY_BITMAP
LMAN(CFloat				, TS_FLONUM			, m_floatMan		, INIT_HEAPSIZE_FLOAT)
#endif

LMAN(CIntFuncValue		, TS_INTFUNC		, m_intFuncMan		, INIT_HEAPSIZE_INTFUNC)
LMAN(CStreamValue		, TS_STREAM			, m_streamMan		, INIT_HEAPSIZE_STREAM)
LMAN(CPackage			, TS_PACKAGE		, m_packageMan		, INIT_HEAPSIZE_PACKAGE)
LMAN(CArrayValue		, TS_ARRAY			, m_arrayMan		, INIT_HEAPSIZE_ARRAY)
LMAN(CReadtable			, TS_READTABLE		, m_readtableMan	, INIT_HEAPSIZE_READTABLE)
LMAN(CBignum			, TS_BIGNUM			, m_bignumMan		, INIT_HEAPSIZE_BIGNUM)
LMAN(CPathname			, TS_PATHNAME		, m_pathnameMan		, INIT_HEAPSIZE_PATHNAME)
LMAN(CWeakPointer		, TS_WEAKPOINTER	, m_weakPointerMan	, INIT_HEAPSIZE_WEAKPOINTER)
LMAN(CHashTable			, TS_HASHTABLE		, m_hashTableMan	, INIT_HEAPSIZE_HASHTABLE)



