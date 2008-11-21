/* gdbmerrno.c - convert gdbm errors into english. */

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

#include "gdbmerrno.h"

/* this is not static so that applications may access the array if they
   like. it must be in the same order as the error codes! */

const char * const gdbm_errlist[] = {
  "No error", "Malloc error", "Block size error", "File open error",
  "File write error", "File seek error", "File read error",
  "Bad magic number", "Empty database", "Can't be reader", "Can't be writer",
  "Reader can't delete", "Reader can't store", "Reader can't reorganize",
  "Unknown update", "Item not found", "Reorganize failed", "Cannot replace",
  "Illegal data", "Option already set", "Illegal option", "Byte-swapped file",
  "Wrong file offset", "Bad file flags"
};

const char *
gdbm_strerror(gdbm_error error)
{
  if(((int)error < _GDBM_MIN_ERRNO) || ((int)error > _GDBM_MAX_ERRNO))
    {
      return("Unknown error");
    }
  else
    {
      return(gdbm_errlist[(int)error]);
    }
}
