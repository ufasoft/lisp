AC_INIT(ufasoft_lisp, 4.36, support@ufasoft.com,, http://ufasoft.com/lisp)
AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/common.m4])


AC_CONFIG_SRCDIR([src/lisp/lispeng/lisp.cpp])


dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_prog_perl_version.html
dnl
AC_DEFUN([AC_PROG_PERL_VERSION],[dnl
# Make sure we have perl
if test -z "$PERL"; then
AC_CHECK_PROG(PERL,perl,perl)
fi

# Check if version of Perl is sufficient
ac_perl_version="$1"

if test "x$PERL" != "x"; then
  AC_MSG_CHECKING(for perl version greater than or equal to $ac_perl_version)
  # NB: It would be nice to log the error if there is one, but we cannot rely
  # on autoconf internals
  $PERL -e "use $ac_perl_version;" > /dev/null 2>&1
  if test $? -ne 0; then
    AC_MSG_RESULT(no);
    $3
  else
    AC_MSG_RESULT(ok);
    $2
  fi
else
  AC_MSG_WARN(could not find perl)
fi
])dnl # AC_PROG_PERL_VERSION


AC_PROG_PERL_VERSION(5.8.0,, AC_MSG_ERROR("Perl 5.8 not found"))

AC_CHECK_LIB(gmp, __gmpz_init,, AC_MSG_ERROR("Library GMP not found"))

AC_CHECK_LIB([readline], 	[history_list],	[have_readline=true]	, [			])
if test x$have_readline = xtrue ; then
	AC_DEFINE([HAVE_READLINE])
fi

if -e src/el/ext.h ; then
	build_libext = true
else
	AC_CHECK_LIB([ext], [AfxTestEHsStub],		, [AC_MSG_ERROR([Library libext not found, install it from https://github./com/ufasoft/libext])])
fi

AM_CONDITIONAL([BUILD_LIBEXT], [x$build_libext = xtrue])



AC_OUTPUT(Makefile)

AU_PRINT_SETTINGS


if test x$has_readline = xtrue; then
	echo "  readline................: yes"
else
	echo "  readline................: no"
fi

