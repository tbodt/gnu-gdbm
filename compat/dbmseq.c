/* dbmseq.c - Visit all elements in the database.  This is the NDBM
   interface. */

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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "extern.h"


/* NDBM Start the visit of all keys in the database.  This produces
   something in hash order, not in any sorted order.  DBF is the dbm file
   information pointer. */

datum
dbm_firstkey (GDBM_FILE dbf)
{
  datum ret_val;

  /* Free previous dynamic memory, do actual call, and save pointer to new
     memory. */
  ret_val = gdbm_firstkey (dbf);
  if (_gdbm_memory.dptr != NULL) free (_gdbm_memory.dptr);
  _gdbm_memory = ret_val;

  /* Return the new value. */
  return ret_val;
}


/* NDBM Continue visiting all keys.  The next key in the sequence is returned.
   DBF is the file information pointer. */

datum
dbm_nextkey (GDBM_FILE dbf)
{
  datum ret_val;

  /* Make sure we have a valid key. */
  if (_gdbm_memory.dptr == NULL)
    return _gdbm_memory;

  /* Call gdbm nextkey with the old value. After that, free the old value. */
  ret_val = gdbm_nextkey (dbf,_gdbm_memory);
  if (_gdbm_memory.dptr != NULL) free (_gdbm_memory.dptr);
  _gdbm_memory = ret_val;

  /* Return the new value. */
  return ret_val;
}

