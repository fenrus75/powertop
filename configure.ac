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
AM_PROG_CC_C_O

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

PKG_CHECK_MODULES([NCURSES], [ncurses],[],[
	AC_CHECK_LIB([ncurses], [main],[AC_SUBST(NCURSES_LIBS, "-lncurses"," ")],AC_MSG_ERROR([ncurses is required but was not found]))
]) 
PKG_CHECK_MODULES([PCIUTILS], [libpci],[],[
	AC_CHECK_LIB([libpci], [main],[AC_SUBST(PCIUTILS_LIBS, "-lpci"," ")],AC_MSG_ERROR([libpci is required but was not found]))
]) 
PKG_CHECK_MODULES([LIBZ], [zlib],[],[
	AC_CHECK_LIB([zlib], [main],[AC_SUBST(LIBZ_LIBS, "-lz"," ")],AC_MSG_ERROR([zlib is required but was not found]))
]) 

has_libnl=no
PKG_CHECK_MODULES([LIBNL], [libnl-1], [has_libnl=yes],[
	AC_CHECK_LIB([libnl], [main],[has_libnl=yes AC_SUBST(LIBNL_LIBS, "-lnl","Update LIBNL_LIBS based on AC_CHECK_LIB")],[
		PKG_CHECK_MODULES([LIBNL], [libnl-2.0 libnl-3.0 libnl-genl-3.0], [has_libnl=yes AC_DEFINE([HAVE_LIBNL20],[1],[Define if you have libnl-2.0 or higher])],[ 
			AC_CHECK_LIB([libnl>= 2.0 libnl-genl], [main],[has_libnl=yes AC_SUBST(LIBNL_LIBS, "-lnl","Update LIBNL_LIBS based on AC_CHECK_LIB") AC_DEFINE([HAVE_LIBNL20],[1],[Define if you have libnl-2.0 or higher])],[dummy=no])
		])
	])
])
if (test "${has_libnl}" = "no"); then
	AC_MSG_ERROR(libnl is required but not found)
fi

AC_CHECK_LIB([pthread], [pthread_create],[AC_SUBST(PTHREAD_LIBS, "-lpthread"," ")] , AC_MSG_ERROR([libpthread is required but was not found]))

# FIXME: Replace `main' with a function in `-lresolv':
AC_CHECK_LIB([resolv], [main],[AC_SUBST(RESOLV_LIBS, "-lresolv"," ")], AC_MSG_ERROR([libresolv is required but was not found]))



AC_OUTPUT
