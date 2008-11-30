# This file is part of GDBM.                                   -*- Makefile -*-
# Copyright (C) 2007 Free Software Foundation, Inc.
#
# GDBM is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GDBM is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDBM. If not, see <http://www.gnu.org/licenses/>. */

# Flags
AM_CPPFLAGS=-DIN_GDBM

# Built sources
BUILT_SOURCES = gdbm.h
DISTCLEANFILES = gdbm.h

gdbm.h:	$(top_srcdir)/src/gdbm.proto $(top_srcdir)/src/gdbmerrno.h $(top_srcdir)/src/gdbm.proto2
	rm -f gdbm.h
	cp $(top_srcdir)/src/gdbm.proto gdbm.h
	chmod +w gdbm.h
	grep _ $(top_srcdir)/src/gdbmerrno.h >> gdbm.h
	cat $(top_srcdir)/src/gdbm.proto2 >> gdbm.h
	chmod -w gdbm.h

# Extras
EXTRA_DIST = gdbm.proto gdbmerrno.h gdbm.proto2
noinst_HEADERS = \
 extern.h\
 gdbmconst.h\
 gdbmdefs.h\
 gdbmerrno.h\
 proto.h\
 systems.h

# The libraries
VI_CURRENT  = 4
VI_REVISION = 0
VI_AGE      = 0

lib_LTLIBRARIES = libgdbm.la
libgdbm_la_SOURCES = \
 gdbmopen.c\
 gdbmdelete.c\
 gdbmfetch.c\
 gdbmstore.c\
 gdbmclose.c\
 gdbmreorg.c\
 gdbmseq.c\
 gdbmsync.c\
 gdbmerrno.c\
 gdbmexists.c\
 gdbmfdesc.c\
 gdbmsetopt.c\
 bucket.c\
 falloc.c\
 findkey.c\
 flatfile.c\
 global.c\
 hash.c\
 lock.c\
 mmap.c\
 update.c\
 version.c

libgdbm_la_LDFLAGS = -version-info $(VI_CURRENT):$(VI_REVISION):$(VI_AGE)

# Programs
bin_PROGRAMS = testgdbm 
testgdbm_LDADD  = ./libgdbm.la
