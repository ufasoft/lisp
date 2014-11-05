#pragma once

// compatibility hacks for old compilers

#if UCFG_CPP11_RVALUE
#	define EXT_REF &&
#	else
#	define EXT_REF &
#endif

#if UCFG_CPP11_ENUM

#	define ENUM_CLASS(name) enum class name
#	define ENUM_CLASS_BASED(name, base) enum class name : base
#	define END_ENUM_CLASS(name) ; 	inline name operator|(name a, name b) { return (name)((int)a|(int)b); } \
									inline name operator&(name a, name b) { return (name)((int)a&(int)b); }

#else // UCFG_CPP11_ENUM

#	define ENUM_CLASS(name) struct enum_##name { enum E
#	define ENUM_CLASS_BASED(name, base) struct enum_##name { enum E : base
#	define END_ENUM_CLASS(name) ; }; typedef enum_##name::E name; 											\
									inline name operator|(name a, name b) { return (name)((int)a|(int)b); } \
									inline name operator&(name a, name b) { return (name)((int)a&(int)b); }

#endif // UCFG_CPP11_ENUM

#if defined(_MSC_VER) && _MSC_VER < 1700
#	define EXT_FOR(decl, cont) for each (decl in cont)
#else
#	define EXT_FOR(decl, cont) for (decl : cont)
#endif

#if !UCFG_CPP11_NULLPTR

namespace std {

class nullptr_t {
public:
	template<class T> operator T*() const { return 0; }
	template<class C, class T> operator T C::*() const { return 0; }

	operator const char *() const { return NULL; }
private:
	void operator&() const;
};

} // ::std

const std::nullptr_t nullptr = {};

#endif // UCFG_CPP11_NULLPTR

#if UCFG_CPP11_NULLPTR && defined(__clang__)
namespace std {
	typedef decltype(nullptr) nullptr_t;
}
#endif

#if !UCFG_CPP11_OVERRIDE
#	define override
#endif

#if	UCFG_CPP11_THREAD_LOCAL
#	define THREAD_LOCAL thread_local
#else
#	define THREAD_LOCAL __declspec(thread)
#endif

#if UCFG_CPP11_EXPLICIT_CAST
#	define EXPLICIT_OPERATOR_BOOL explicit operator bool
#	define EXT_OPERATOR_BOOL explicit operator bool
#	define EXT_CONVERTIBLE_TO_TRUE true
#	define _TYPEDEF_BOOL_TYPE
#else
#	define EXPLICIT_OPERATOR_BOOL struct _Boolean { int i; }; operator int _Boolean::* 
#	define EXT_OPERATOR_BOOL operator _Bool_type
#	define EXT_CONVERTIBLE_TO_TRUE (&_Boolean::i)
#endif

namespace std {

#if !UCFG_CPP11_BEGIN

template <class C>
typename C::iterator begin(C& c) {
	return c.begin();
}

template <class C>
typename C::const_iterator begin(const C& c) {
	return c.begin();
}

template <class C>
typename C::iterator end(C& c) {
	return c.end();
}

template <class C>
typename C::const_iterator end(const C& c) {
	return c.end();
}

template <class T, size_t size>
inline T *begin(T (&ar)[size]) {
	return ar;
}

template <class T, size_t size>
inline T *end(T (&ar)[size]) {
	return ar+size;
}


#endif // UCFG_CPP11_BEGIN

#if !UCFG_HAVE_STATIC_ASSERT
#	ifndef NDEBUG
#		define static_assert(test, errormsg)                         \
    do {                                                        \
        struct ERROR_##errormsg {};                             \
        typedef ww::compile_time_check< (test) != 0 > tmplimpl; \
        tmplimpl aTemp = tmplimpl(ERROR_##errormsg());          \
        sizeof(aTemp);                                          \
    } while (0)
#	else
#		define static_assert(test, errormsg)                         \
    do {} while (0)
#	endif
#endif // !UCFG_HAVE_STATIC_ASSERT


} // std::

