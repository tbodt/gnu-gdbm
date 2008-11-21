/* close.c - Close the "original" style database. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 2007  Free Software Foundation, Inc.

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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "extern.h"


/* It's unclear whether dbmclose() is *always* a void function in old
   C libraries.  We use int, here. */

int
dbmclose()
{
  if (_gdbm_file != NULL)
    {
      gdbm_close (_gdbm_file);
      _gdbm_file = NULL;
      if (_gdbm_memory.dptr != NULL) free(_gdbm_memory.dptr);
      _gdbm_memory.dptr = NULL;
      _gdbm_memory.dsize = 0;
    }
  return (0);
}
