#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Reqflow], [1.5.0], [], [reqflow], [http://goeb.github.io/reqflow])
AC_CONFIG_HEADERS([src/config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects])

# Set a OS specific settings
WINDOWS=no
MINGW=no
CYGWIN=no
case $target_os in
*linux*)
  ;;
*darwin*)
;;
GNU/kFreeBSD)
;;
*mingw*)
WINDOWS=yes
MINGW=yes
;;
*cygwin*)
WINDOWS=yes
CYGWIN=yes
  ;;
GNU)
  ;;
esac
AM_CONDITIONAL(WINDOWS, test x$WINDOWS = xyes)
AM_CONDITIONAL(CYGWIN, test x$MINGW = xyes)
AM_CONDITIONAL(MINGW, test x$MINGW = xyes)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for external libraries.
PKG_PROG_PKG_CONFIG()
if test x$WINDOWS = xyes; then
    PKG_CONFIG="$PKG_CONFIG --static"
fi
PKG_CHECK_MODULES(ZIP, libzip)
PKG_CHECK_MODULES(XML, libxml-2.0)
PKG_CHECK_MODULES(POPPLER, [poppler-cpp])
PKG_CHECK_MODULES(PCRE, [libpcreposix])

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
    [Enable test suite code coverage])])
AM_CONDITIONAL([GCOV_ENABLED],[test x$enable_gcov = xyes])
AC_ARG_ENABLE([gprof],
  [AS_HELP_STRING([--enable-gprof],
    [Enable test suite code profiling])])
AM_CONDITIONAL([GPROF_ENABLED],[test x$enable_gprof = xyes])
if test x$WINDOWS = xyes; then
    AC_ARG_VAR([WINE],     [Wine translator, used to run Inno Setup])
    AC_CHECK_PROG(WINE, wine, wine)
    if test -z "$WINE" ; then
        AC_MSG_ERROR([Please install wine before cross-compiling.])
    fi
    
    ISCC="$(find ~/.wine/drive_c -type f -name ISCC.exe)"
    if test -z "$ISCC" ; then
        AC_MSG_ERROR([Please install Inno Setup into wine before cross-compiling.])
    fi
    AC_SUBST(ISCC)
fi

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday localtime regcomp strcasecmp strchr strerror strstr strtoul])

AC_CONFIG_FILES([Makefile test/Makefile test/envtest reqflow.iss])
AC_OUTPUT

