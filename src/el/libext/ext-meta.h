#pragma once

#ifndef STATIC_ASSERT
#	define _PasteToken(x,y) x##y
#	define _Join(x,y) _PasteToken(x,y)
#	define STATIC_ASSERT(e) typedef char _Join(_STATIC_ASSERT_, __COUNTER__) [(e)?1:-1]
#endif


#define STATIC_ASSERT_POWER_OF_2(x) STATIC_ASSERT( (((x) | (x)-1) >> 1) == (x)-1 )

#if UCFG_STLSOFT
#	include <stlsoft/stlsoft.h>
#	include <stlsoft/util/true_typedef.hpp>
	using stlsoft::true_typedef;
#	define TRUE_TYPEDEF(def, name) stlsoft_gen_opaque(name##_u) typedef true_typedef<def,name##_u> name;

#	ifdef __unix__
#		include <unixstl/filesystem/glob_sequence.hpp>
#	endif

#else
#	define TRUE_TYPEDEF(def, name) typedef def name;
#endif


namespace ExtSTL {

using std::true_type;
using std::false_type;
using std::is_integral;
using std::is_floating_point;


	/*!!!
struct yes_type {
};

struct no_type {
};

struct no_base {
	enum { value = 0 };
	typedef false_type type;
};

struct yes_base {
	enum { value = 1 };
	typedef true_type type;
};*/

template <typename T1, typename T2, bool CHOOSE_FIRST_TYPE> struct select_first_type_if {
	typedef T1 type;
};

template <typename T1, typename T2> struct select_first_type_if<T1, T2, false> {
	typedef T2 type;
};

/*!!!R
template <typename T> struct is_integral_type : false_type {};
#define DECLARE_INTEGRAL_TYPE(name) template <> struct is_integral_type<name> : true_type {};

DECLARE_INTEGRAL_TYPE(char)
DECLARE_INTEGRAL_TYPE(signed char)
DECLARE_INTEGRAL_TYPE(unsigned char)
DECLARE_INTEGRAL_TYPE(short)
DECLARE_INTEGRAL_TYPE(unsigned short)
#ifdef _NATIVE_WCHAR_T_DEFINED
	DECLARE_INTEGRAL_TYPE(wchar_t)
#endif
DECLARE_INTEGRAL_TYPE(int)
DECLARE_INTEGRAL_TYPE(unsigned int)
DECLARE_INTEGRAL_TYPE(long)
DECLARE_INTEGRAL_TYPE(unsigned long)
DECLARE_INTEGRAL_TYPE(long long)
DECLARE_INTEGRAL_TYPE(unsigned long long)
*/

/*!!!
template <> struct is_integral_type<char>				: yes_base {};
template <> struct is_integral_type<signed char>		: yes_base {};
template <> struct is_integral_type<unsigned char>		: yes_base {};
template <> struct is_integral_type<short>				: yes_base {};
template <> struct is_integral_type<unsigned short>		: yes_base {};
template <> struct is_integral_type<int>				: yes_base {};
template <> struct is_integral_type<unsigned int>		: yes_base {};
template <> struct is_integral_type<long>				: yes_base {};
template <> struct is_integral_type<unsigned long>		: yes_base {};
template <> struct is_integral_type<__int64>			: yes_base {};
template <> struct is_integral_type<unsigned __int64>	: yes_base {};

*/

/*!!!
template <typename T> struct is_floating_point_type	: no_base {};
#define DECLARE_FLOATING_POINT_TYPE(name) template <> struct is_floating_point_type<name> : yes_base {};


DECLARE_FLOATING_POINT_TYPE(float)
DECLARE_FLOATING_POINT_TYPE(double)
DECLARE_FLOATING_POINT_TYPE(long double)
*/


/*!!!

template <> struct is_floating_point_type<float>		: yes_base {};
template <> struct is_floating_point_type<double>		: yes_base {};
template <> struct is_floating_point_type<long double>	: yes_base {};
*/

/*!!!R
template <typename T> struct is_numeric_type {
	enum { value = is_integral<T>::value ||
					is_floating_point<T>::value	
	};

	typedef typename select_first_type_if<true_type, false_type, value>::type type;
};
*/


} // ExtSTL::


#if UCFG_EXTENDED && ((defined(_MSC_VER) && _MSC_VER < 1700) || (!defined(_MSC_VER) && !UCFG_STDSTL))

namespace std {

template<class _Iter>
	struct _Is_iterator
//!!!	: public ExtSTL::tr1::integral_constant<bool, !ExtSTL::tr1::is_integral<_Iter>::value>
	: public ExtSTL::integral_constant<bool, !ExtSTL::is_integral<_Iter>::value>
	{	// tests for reasonable iterator candidate
	};

}

#endif

