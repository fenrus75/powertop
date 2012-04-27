#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_LANG([C++])
AC_INIT([powertop], [1.98], [power@bughost.org])
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign])
AC_CONFIG_FILES([Makefile src/Makefile pevent/Makefile po/Makefile.in])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
#AC_SUBST(LIBTOOL_DEPS)
#AC_LTDL_DLLIB
GETTEXT_PACKAGE=powertop
AC_SUBST(GETTEXT_PACKAGE)
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])

# Checks for programs.
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h limits.h locale.h malloc.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/statfs.h sys/time.h termios.h unistd.h ncurses.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([fdatasync getpagesize gettimeofday memmove memset mkdir munmap pow realpath regcomp select setlocale socket sqrt strcasecmp strchr strdup strerror strncasecmp strstr strtoul strtoull])

PKG_CHECK_MODULES([NCURSES], [ncurses]) 
PKG_CHECK_MODULES([PCIUTILS], [libpci]) 

PKG_CHECK_MODULES([LIBNIL], [libnl-1],[],[
	PKG_CHECK_MODULES([LIBNIL], [libnl-2.0],[AC_DEFINE([HAVE_LIBNL20],[1],[Define if you have libnl-2.0 or higher])] ,[
		PKG_CHECK_MODULES([LIBNIL], [libnl-3.0 libnl-genl-3.0],[AC_DEFINE([HAVE_LIBNL20],[1],[Define if you have libnl-3.0 or higher])] ,[AC_MSG_ERROR([libnl is required but was not found])])
	])
])
	

PKG_CHECK_MODULES([LIBZ], [zlib]) 

# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main], , AC_MSG_ERROR([libpthread is required but was not found]))
# FIXME: Replace `main' with a function in `-lresolv':
AC_CHECK_LIB([resolv], [main], , AC_MSG_ERROR([libresolv is required but was not found]))
# FIXME: Replace `main' with a function in `-lncursesw':
AC_CHECK_LIB([ncurses], [main], , AC_MSG_ERROR([ncurses is required but was not found]))

AC_OUTPUT
