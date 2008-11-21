/* version.c - This is file contains the version number for gdbm source. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "autoconf.h"

/* Keep a string with the version number in it! */
const char * gdbm_version = "GDBM version 1.9.0. xx/xx/2007"
#if defined(__STDC__) && defined(__DATE__) && defined(__TIME__)
		" (built " __DATE__ " " __TIME__ ")"
#endif
		;
