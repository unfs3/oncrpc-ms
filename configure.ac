AC_PREREQ(2.59)

AC_INIT([oncrpc-ms],[1.15])
AC_CONFIG_SRCDIR([librpc/svc.c])
AC_CANONICAL_TARGET([])

AM_INIT_AUTOMAKE($PACKAGE_NAME,$PACKAGE_VERSION)

AC_LANG([C])

SHARED_VERSION_INFO="1:17:0"

AC_PROG_CC
AM_PROG_LIBTOOL

AC_PROG_INSTALL
AC_PROG_LN_S

AC_HEADER_STDC
AC_SYS_LARGEFILE

if test x$ac_cv_c_compiler_gnu = xyes ; then
    CFLAGS="$CFLAGS -W -Wall"
fi

AC_CHECK_FUNCS([gettimeofday])

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)

dnl The following line causes the libtool distributed with the source
dnl to be replaced if the build system has a more recent version.
AC_SUBST(LIBTOOL_DEPS)

AC_CONFIG_FILES([ \
    Makefile \
    librpc/Makefile])
dnl AC_CONFIG_FILES([ \
dnl     rpcgen/Makefile \
dnl     service/Makefile \
dnl     rpcinfo/Makefile \
dnl     test/Makefile])

AC_OUTPUT
