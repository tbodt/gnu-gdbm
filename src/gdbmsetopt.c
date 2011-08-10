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

static int
getbool (void *optval, int optlen)
{
  int n;
  
  if (!optval || optlen != sizeof (int) ||
      (((n = *(int*)optval) != TRUE) && n != FALSE))
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  return n;
}

static int
get_size (void *optval, int optlen, size_t *ret)
{
  if (!optval)
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  if (optlen == sizeof (unsigned))
    *ret = *(unsigned*) optval;
  else if (optlen == sizeof (unsigned long))
    *ret = *(unsigned long*) optval;
  else if (optlen == sizeof (size_t))
    *ret = *(size_t*) optval;
  else
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  return 0;
}

int
gdbm_setopt (GDBM_FILE dbf, int optflag, void *optval, int optlen)
{
  int n;
  size_t sz;
  
  switch (optflag)
    {
      case GDBM_CACHESIZE:
        /* Optval will point to the new size of the cache. */
        if (dbf->bucket_cache != NULL)
          {
            gdbm_errno = GDBM_OPT_ALREADY_SET;
            return -1;
          }

	if (get_size (optval, optlen, &sz))
	  return -1;
        return _gdbm_init_cache (dbf, (sz > 9) ? sz : 10);

      case GDBM_FASTMODE:
      	/* Obsolete form of SYNCMODE. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->fast_write = n;
	break;

      case GDBM_SYNCMODE:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->fast_write = !n;
	break;

      case GDBM_CENTFREE:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->central_free = n;
	break;

      case GDBM_COALESCEBLKS:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->coalesce_blocks = n;
	break;

      case GDBM_SETMAXMAPSIZE:
	{
	  size_t page_size = sysconf (_SC_PAGESIZE);

	  if (get_size (optval, optlen, &sz))
	    return -1;
	  dbf->mapped_size_max = ((sz + page_size - 1) / page_size) *
	                          page_size;
	  _gdbm_mapped_init (dbf);
	  break;
	}
	
      default:
        gdbm_errno = GDBM_OPT_ILLEGAL;
        return -1;
    }

  return 0;
}
