AC_INIT(ufasoft_lisp, 4.36, support@ufasoft.com,, http://ufasoft.com/lisp)
AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/common.m4])


AC_CONFIG_SRCDIR([src/lisp/lispeng/lisp.cpp])
AC_CONFIG_HEADER([config.h])



AC_PROG_PERL_VERSION(5.8.0,, AC_MSG_ERROR("Perl 5.8 not found"))

AC_CHECK_LIB(gmp, __gmpz_init,, AC_MSG_ERROR("Library GMP not found"))
AC_CHECK_LIB([readline], 	[history_list],	[AC_DEFINE(HAVE_READLINE), [has_readline=true]]	, [			])







AC_OUTPUT(Makefile)

AU_PRINT_SETTINGS


if test x$has_readline = xtrue; then
	echo "  readline................: yes"
else
	echo "  readline................: no"
fi

