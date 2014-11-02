AC_PREREQ([2.67])
AC_INIT(ufasoft_lisp, 4.33, support@ufasoft.com,, http://ufasoft.com/lisp)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([dist-xz])

AC_CONFIG_SRCDIR([src/lispeng/lisp.cpp])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC([clang gcc])
AC_PROG_CXX([clang++ g++])
AC_LANG([C++])

AC_PATH_PROG([COMPILER], [$CXX])
if ! test -x "${COMPILER}"; then
	AC_MSG_ERROR([No C++ compiler found. Please install a C++ compiler.])
fi

AC_PROG_PERL_VERSION(5.8.0,, AC_MSG_ERROR("Perl 5.8 not found"))

AC_CHECK_LIB(pcre, pcre_compile,, AC_MSG_ERROR("Library not found"))
AC_CHECK_LIB(gmp, __gmpz_init,, AC_MSG_ERROR("Library not found"))
AC_CHECK_LIB(dl, dlopen,, AC_MSG_ERROR("Library not found"))
AC_CHECK_LIB(rt, clock_gettime,, AC_MSG_ERROR("Library not found"))
AC_CHECK_LIB(ntl, main,, AC_MSG_WARN("Library NTL not found"))

AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])], [CLANGCC="yes"], [CLANGCC="no"])
AC_MSG_RESULT([$CLANGCC])

AM_CONDITIONAL(CLANG, test "$CLANGCC" = "yes" )



AC_OUTPUT(Makefile)
