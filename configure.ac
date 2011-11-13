# This file is part of GDBM.                                   -*- autoconf -*-
# Copyright (C) 2007, 2009, 2011 Free Software Foundation, Inc.
#
# GDBM is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GDBM is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDBM. If not, see <http://www.gnu.org/licenses/>. */

m4_define([_GDBM_VERSION_MAJOR], 1)
m4_define([_GDBM_VERSION_MINOR], 10)
dnl m4_define([_GDBM_VERSION_PATCH], 90)

AC_INIT([gdbm],
        _GDBM_VERSION_MAJOR._GDBM_VERSION_MINOR[]m4_ifdef([_GDBM_VERSION_PATCH],._GDBM_VERSION_PATCH),
	[bug-gdbm@gnu.org])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([src/gdbmdefs.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([autoconf.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([gnits 1.11 std-options silent-rules])

# Enable silent rules by default:
AM_SILENT_RULES([yes])

AC_SUBST([GDBM_VERSION_MAJOR], _GDBM_VERSION_MAJOR)
AC_SUBST([GDBM_VERSION_MINOR], _GDBM_VERSION_MINOR)
AC_SUBST([GDBM_VERSION_PATCH], m4_ifdef([_GDBM_VERSION_PATCH],_GDBM_VERSION_PATCH,0))

AC_ARG_ENABLE([memory-mapped-io],
  AC_HELP_STRING(
    [--enable-memory-mapped-io]
    [Use mmap(2) for disk I/O. (Default is YES.)]),
    [mapped_io=$enableval],
    [mapped_io=yes])

AC_ARG_ENABLE([libgdbm-compat],
  AC_HELP_STRING(
    [--enable-libgdbm-compat]
    [Build and install libgdbm_compat. (Default is NO.)]),
    [want_compat=$enableval],
    [want_compat=no])

AC_ARG_ENABLE([gdbm-export],
  AC_HELP_STRING(
    [--enable-gdbm-export],
    [Build and install gdbmexport with specified gdbm 1.8 library. (Default is NO.)]),
    [want_export=$enableval],
    [want_export=no])

AC_ARG_WITH([gdbm183-library],
  AC_HELP_STRING(
    [--with-gdbm183-library],
    [Build gdbmexport with specified (static) library.]),
    [GDBM183_LIBRARY=$withval],
    [GDBM183_LIBRARY="-lgdbm"])

AC_ARG_WITH([gdbm183-libdir],
  AC_HELP_STRING(
    [--with-gdbm183-libdir],
    [Build gdbmexport with the gdbm library in the specified directory.]),
    [GDBM183_LIBDIR=$withval],
    [GDBM183_LIBDIR="/usr/local/lib"])

AC_ARG_WITH([gdbm183-includedir],
  AC_HELP_STRING(
    [--with-gdbm183-includedir],
    [Build gdbmexport with gdbm.h in the specified directory.]),
    [GDBM183_INCLUDEDIR=$withval],
    [GDBM183_INCLUDEDIR="/usr/local/include"])
    
dnl Check for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE
dnl AC_PROG_RANLIB
dnl AC_C_BIGENDIAN([])
AC_C_CONST

dnl Internationalization macros.
AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.18)

AC_CHECK_HEADERS([stdlib.h string.h sys/file.h unistd.h fcntl.h sys/types.h memory.h sys/termios.h locale.h])

AC_CHECK_LIB(dbm, main)
AC_CHECK_LIB(ndbm, main)
AC_CHECK_FUNCS([rename ftruncate flock lockf fsync setlocale])

if test x$mapped_io = xyes
then
  AC_FUNC_MMAP()
  AC_CHECK_FUNCS([msync])
fi
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_MEMBERS([struct stat.st_blksize])

AC_SUBST(GDBM183_LIBRARY)
AC_SUBST(GDBM183_LIBDIR)
AC_SUBST(GDBM183_INCLUDEDIR)
AM_CONDITIONAL([COMPAT_OPT], [test "$want_compat" = yes])
AM_CONDITIONAL([ENABLE_EXPORT], [test "$want_export" = yes])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal po/Makefile.in])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 src/gdbm.h
		 doc/Makefile
		 compat/Makefile
		 export/Makefile])
AC_OUTPUT
