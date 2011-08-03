/* gdbmsetopt.c - set options pertaining to a GDBM descriptor. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 1994, 2007, 2011 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.   */

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"

/* operate on an already open descriptor. */

/* ARGSUSED */
int
gdbm_setopt(GDBM_FILE dbf, int optflag, int *optval, int optlen)
{
  switch (optflag)
    {
      case GDBM_CACHESIZE:
        /* Optval will point to the new size of the cache. */
        if (dbf->bucket_cache != NULL)
          {
            gdbm_errno = GDBM_OPT_ALREADY_SET;
            return -1;
          }

        return _gdbm_init_cache(dbf, ((*optval) > 9) ? (*optval) : 10);

      case GDBM_FASTMODE:
      	/* Obsolete form of SYNCMODE. */
	if (!optval || ((*optval != TRUE) && (*optval != FALSE)))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }

	dbf->fast_write = *optval;
	break;

      case GDBM_SYNCMODE:
      	/* Optval will point to either true or false. */
	if (!optval || ((*optval != TRUE) && (*optval != FALSE)))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }

	dbf->fast_write = !(*optval);
	break;

      case GDBM_CENTFREE:
      	/* Optval will point to either true or false. */
	if (!optval || ((*optval != TRUE) && (*optval != FALSE)))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }

	dbf->central_free = *optval;
	break;

      case GDBM_COALESCEBLKS:
      	/* Optval will point to either true or false. */
	if (!optval || ((*optval != TRUE) && (*optval != FALSE)))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }

	dbf->coalesce_blocks = *optval;
	break;

      default:
        gdbm_errno = GDBM_OPT_ILLEGAL;
        return -1;
    }

  return 0;
}
