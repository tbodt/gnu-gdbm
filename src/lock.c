/* lock.c - Implement basic file locking for GDBM. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2008  Free Software Foundation, Inc.

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
#include "extern.h"

#if HAVE_FLOCK
#ifndef LOCK_SH
#define LOCK_SH 1
#endif

#ifndef LOCK_EX
#define LOCK_EX 2
#endif

#ifndef LOCK_NB
#define LOCK_NB 4
#endif

#ifndef LOCK_UN
#define LOCK_UN 8
#endif
#endif

#if (defined(F_SETLK) || defined(F_SETLK64)) && defined(F_RDLCK) && defined(F_WRLCK)
#define HAVE_FCNTL_LOCK 1
#else
#define HAVE_FCNTL_LOCK 0
#endif

#if defined(F_SETLK64) && (defined(_LARGE_FILES) || _FILE_OFFSET_BITS == 64)
#define _FLOCK flock64
#define _SETLK F_SETLK64
#else
#define _FLOCK flock
#define _SETLK F_SETLK
#endif

#define LOCKING_NONE	0
#define LOCKING_FLOCK	1
#define LOCKING_LOCKF	2
#define LOCKING_FCNTL	3

static int _gdbm_lock_type = LOCKING_NONE;

void
_gdbm_unlock_file (gdbm_file_info *dbf)
{
#if HAVE_FCNTL_LOCK
  struct _FLOCK flock;
#endif

  switch (_gdbm_lock_type)
    {
      case LOCKING_FLOCK:
#if HAVE_FLOCK
	flock (dbf->desc, LOCK_UN);
#endif
	break;

      case LOCKING_LOCKF:
#if HAVE_LOCKF
	lockf (dbf->desc, F_ULOCK, (off_t)0L);
#endif
	break;

      case LOCKING_FCNTL:
#if HAVE_FCNTL_LOCK
	flock.l_type = F_UNLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = flock.l_len = (off_t)0L;
	fcntl (dbf->desc, _SETLK, &flock);
#endif
	break;
    }
}

/* Try each supported locking mechanism. */
int
_gdbm_lock_file (gdbm_file_info *dbf)
{
#if HAVE_FCNTL_LOCK
  struct _FLOCK flock;
#endif
  int lock_val = -1;

#if HAVE_FLOCK
  if (dbf->read_write == GDBM_READER)
    lock_val = flock (dbf->desc, LOCK_SH + LOCK_NB);
  else
    lock_val = flock (dbf->desc, LOCK_EX + LOCK_NB);

  if (lock_val != -1)
    {
      _gdbm_lock_type = LOCKING_FLOCK;
      return lock_val;
    }
#endif

#if HAVE_LOCKF
  /* Mask doesn't matter for lockf. */
  lock_val = lockf (dbf->desc, F_LOCK, (off_t)0L);
  if (lock_val != -1)
    {
      _gdbm_lock_type = LOCKING_LOCKF;
      return lock_val;
    }
#endif

#if HAVE_FCNTL_LOCK
  /* If we're still here, try fcntl. */
  if (dbf->read_write == GDBM_READER)
    flock.l_type = F_RDLCK;
  else
    flock.l_type = F_WRLCK;
  flock.l_whence = SEEK_SET;
  flock.l_start = flock.l_len = (off_t)0L;
  lock_val = fcntl (dbf->desc, _SETLK, &flock);
  if (lock_val != -1)
    _gdbm_lock_type = LOCKING_FCNTL;
#endif

  if (lock_val == -1)
    _gdbm_lock_type = LOCKING_NONE;
  return lock_val;
}
