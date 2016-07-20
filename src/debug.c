/* This file is part of GDBM, the GNU data base manager.
   Copyright 2016 Free Software Foundation, Inc.

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

#include "autoconf.h"
#include "gdbmdefs.h"

struct hook_list
{
  struct hook_list *next;
  struct hook_list *prev;
  char *id;
  gdbm_debug_hook hook;
  void *data;
  int retval;
};

static struct hook_list *hook_head, *hook_tail;
static struct hook_list *hook_recent;

static struct hook_list *
hook_lookup_or_install (char const *id, int install)
{
  struct hook_list *p;

  for (p = hook_head; p; p = p->next)
    {
      int res = strcmp (p->id, id);
      if (res == 0)
	return p;
      if (res > 0)
	break;
    }

  if (install)
    {
      struct hook_list *elt = malloc (sizeof *elt);
      if (!elt)
	return NULL;
      elt->id = strdup (id);
      if (!elt->id)
	{
	  SAVE_ERRNO (free (elt));
	  return NULL;
	}
      elt->hook = NULL;
      elt->next = p;
      if (p)
	{
	  if (p->prev)
	    p->prev->next = elt;
	  else
	    hook_head = elt;
	  elt->prev = p->prev;
	}
      else
	{
	  elt->prev = hook_tail;
	  if (hook_tail)
	    hook_tail->next = elt;
	  else
	    hook_head = elt;
	  hook_tail = elt;
	}
      return elt;
    }
  
  return NULL;
}

static struct hook_list *
hook_lookup (char const *id)
{
  if (!(hook_recent && strcmp (hook_recent->id, id) == 0))
    hook_recent = hook_lookup_or_install (id, FALSE);
  return hook_recent;
}

static void
hook_remove (char const *id)
{
  struct hook_list *p;

  p = hook_lookup (id);
  if (!p)
    return;

  hook_recent = NULL;
  
  if (p->prev)
    p->prev->next = p->next;
  else
    hook_head = p->next;

  if (p->next)
    p->next->prev = p->prev;
  else
    hook_tail = p->prev;

  free (p->id);
  free (p);
}

static int
default_hook (char const *file, int line, char const *id, void *data)
{
  fprintf (stderr, "%s:%d: hit debug hook %s\n", file, line, id);
  return 1;
}
  
void
_gdbm_debug_hook_install (char const *id, gdbm_debug_hook hook, void *data)
{
  struct hook_list *p;

  p = hook_lookup_or_install (id, TRUE);
  p->hook = hook ? hook : default_hook;
  p->data = data;
}

void
_gdbm_debug_hook_remove (char const *id)
{
  hook_remove (id);
}

int
_gdbm_debug_hook_check (char const *file, int line, char const *id)
{
  struct hook_list *p = hook_lookup (id);
  if (p)
    return p->retval = p->hook (file, line, id, p->data);
  return 0;
}

int
_gdbm_debug_hook_val (char const *id)
{
  struct hook_list *p = hook_lookup (id);
  if (p)
    return p->retval;
  return 0;
}
