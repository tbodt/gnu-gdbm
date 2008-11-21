/* gdbmsync.c - Sync the disk with the in memory state. */

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
#include "gdbmerrno.h"

/* Make sure the database is all on disk. */

void
gdbm_sync (gdbm_file_info *dbf)
{

  /* Initialize the gdbm_errno variable. */
  gdbm_errno = GDBM_NO_ERROR;

  /* Do the sync on the file. */
  __fsync (dbf);

}
