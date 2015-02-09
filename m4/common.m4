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
	has_regex=yes
fi

AS_IF([test "x$has_regex" = "xyes"]
	, 
	,[AC_CHECK_LIB(pcre, pcre_compile,, AC_MSG_ERROR("Library PCRE not found: install libpcre3-dev / pcre-devel"))]
)

AM_CONDITIONAL(HAS_REGEX, [test "x$has_regex" = "xyes"])


AC_CHECK_LIB([pthread], [pthread_create],		, [AC_MSG_ERROR([Library libpthread not found])					])
AC_SEARCH_LIBS([iconv],  [iconv], []			, [AC_MSG_ERROR([Unable to find the iconv() function])			])




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
    echo "  CPPFLAGS.............: $CPPFLAGS"
    echo "  CFLAGS...............: $CFLAGS"
    echo "  CXXFLAGS.............. $CXXFLAGS"
    echo "  LIBS.................. $LIBS"
    echo
    echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
    echo "  prefix...............: $prefix"
    echo
])



