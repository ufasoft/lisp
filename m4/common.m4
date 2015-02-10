AC_PREREQ([2.67])
AM_INIT_AUTOMAKE([foreign subdir-objects dist-xz])

AM_SILENT_RULES([yes])

AC_PROG_CXX([clang++ g++])
AC_PROG_CC([clang gcc])
AC_LANG([C++])

AC_PATH_PROG([COMPILER], [$CXX])
if ! test -x "${COMPILER}"; then
	AC_MSG_ERROR([No C++ compiler found. Please install a C++ compiler, Clang++ preferred.])
fi


AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T


CXXFLAGS="$CXXFLAGS -Wno-invalid-offsetof"

AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"], [CXXFLAGS="$CXXFLAGS -std=c++0x"])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[
	#ifndef __clang__
		not clang
	#endif
	]])],
	[CLANG=yes], [CLANG=no])

if test "x$CLANG" = "xyes"; then
	CXXFLAGS="$CXXFLAGS -stdlib=libc++"
	AC_CHECK_LIB([c++abi], [__cxa_bad_cast],  	, [AC_MSG_ERROR([required libc++abi not found])					])
	have_regex=yes
fi

if ! test "x$have_regex" = "xyes"; then
	AC_CHECK_LIB(pcre, pcre_compile,, AC_MSG_ERROR("Library PCRE not found: install libpcre3-dev / pcre-devel"))
fi

AM_CONDITIONAL(HAVE_REGEX, [test "x$have_regex" = "xyes"])


AC_CHECK_LIB([pthread], [pthread_create],	[CXXFLAGS="$CXXFLAGS -pthread"]	, [AC_MSG_ERROR([Library libpthread not found])					])
AC_CHECK_FUNCS([pthread_setname_np])
AC_SEARCH_LIBS([iconv],  [iconv], []										, [AC_MSG_ERROR([Unable to find the iconv() function])			])




AC_DEFUN([AU_PRINT_SETTINGS], [
    echo
    echo
    echo "------------------------------------------------------------------------"
    echo "$PACKAGE $VERSION"
    echo "------------------------------------------------------------------------"
    echo
    echo
    echo "Configuration Options Summary:"
    echo
    echo "Compilation............: make (gmake on FreeBSD)"
    echo "  CXX................... $CXX"
    echo "  CPPFLAGS.............. $CPPFLAGS"
    echo "  CFLAGS................ $CFLAGS"
    echo "  CXXFLAGS.............. $CXXFLAGS"
    echo "  LIBS.................. $LIBS"
    echo "  LDFLAGS............... $LDFLAGS"
    echo
    echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
    echo "  prefix...............: $prefix"
    echo
])



