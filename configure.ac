#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libsysve], [3.1.0])
AC_CONFIG_SRCDIR([src/Makefile.am])
AC_CONFIG_AUX_DIR([./build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_AR()
LT_INIT()

AC_ARG_WITH([release-id], [AS_HELP_STRING([--with-release-id],
	    [Specify release id])],
	    [RELEASE=${with_release_id}],
	    [RELEASE="1"])
AC_SUBST(RELEASE)

AC_ARG_WITH(ve-prefix, [AS_HELP_STRING([--with-ve-prefix],
	    [Specify prefix of VE software
	    [/opt/nec/ve]])],
	    [ve_prefix=${with_ve_prefix}],
	    [ve_prefix=/opt/nec/ve])
AC_SUBST(ve_prefix)

AC_ARG_ENABLE([separatedlibs],
[  --enable-separatedlibs    Turn on separated libraries],
[case "${enableval}" in
  yes) separatedlibs=true ;;
  no)  separatedlibs=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-separatedlibs]) ;;
esac],[separatedlibs=false])
AM_CONDITIONAL([SEPARATEDLIBS], [test x$separatedlibs = xtrue])

AC_ARG_ENABLE([vhcallenhance],
  [AS_HELP_STRING([--disable-vhcallenhance],
  [disable vhcall enhanced API])],
  [enable_vhcallenhance=${enableval}], [])
AM_CONDITIONAL([VHCALLNOENHANCE], [test x"$enable_vhcallenhance" == x"no"])

CFLAGS="${CFLAGS} -I${top_srcdir}/include"

AC_PREFIX_DEFAULT([/opt/nec/ve])

# Checks for programs.
AC_PROG_CC([ncc])
AM_PROG_AS
AC_SUBST(LIBC_INC)
# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h sys/types.h sysve.h veos_defs.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_TYPE_SSIZE_T

# Checks for header files.
AC_CHECK_HEADERS([io_hook.h])

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 libsysve.spec
		 Doxyfile
                 src/Makefile
                 debian/control
                 debian/copyright
                 debian/libsysve-dev.install
                 debian/libsysve.install
                 debian/rules])
AC_OUTPUT
