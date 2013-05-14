/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007, 2011, 2013 Free Software Foundation,
   Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.    */

#include "gdbmtool.h"

#define VARF_DFL    0x00
#define VARF_SET    0x01

struct variable
{
  char *name;
  int type;
  int flags;
  union
  {
    char *string;
    int bool;
    int num;
  } v;
  int (*hook) (struct variable *, int);
  void *hook_data;
};

static int mode_toggle (struct variable *var, int after);

static struct variable vartab[] = {
  /* Top-level prompt */
  { "ps1", VART_STRING, VARF_DFL, { "%p>%_" } },
  /* Second-level prompt (used within "def" block) */
  { "ps2", VART_STRING, VARF_DFL, { "%_>%_" } },
  /* This delimits array members */
  { "delim1", VART_STRING, VARF_DFL, { "," } },
  /* This delimits structure members */
  { "delim2", VART_STRING, VARF_DFL, { "," } },
  { "cachesize", VART_INT, VARF_DFL, { num: DEFAULT_CACHESIZE } },
  { "blocksize", VART_INT, VARF_DFL, { num: 0 } },
  { "readonly", VART_BOOL, VARF_DFL, { num: 0 }, mode_toggle },
  { "newdb", VART_BOOL, VARF_DFL, { 0 }, mode_toggle },
  { "lock", VART_BOOL, VARF_DFL, { num: 1 } },
  { "mmap", VART_BOOL, VARF_DFL, { num: 1 } },
  { "sync", VART_BOOL, VARF_DFL, { num: 0 } },
  { NULL }
};

static int
mode_toggle (struct variable *var, int after)
{
  if (after)
    {
      struct variable *vp;
      int newval = !var->v.bool;
      
      for (vp = vartab; vp->name; vp++)
	{
	  if (vp == var)
	    continue;
	  else if (vp->hook == mode_toggle)
	    {
	      vp->v.bool = newval;
	      newval = 0;
	    }
	}
    }
  return 0;
}

const char *
variable_mode_name ()
{
  struct variable *vp;

  for (vp = vartab; vp->name; vp++)
    if (vp->hook == mode_toggle && vp->v.bool)
      return vp->name;

  return NULL;
}

static struct variable *
varfind (const char *name)
{
  struct variable *vp;

  for (vp = vartab; vp->name; vp++)
    if (strcmp (vp->name, name) == 0)
      return vp;
  
  return NULL;
}

typedef int (*setvar_t) (struct variable *, void *);

static int
s2s (struct variable *vp, void *val)
{
  vp->v.string = estrdup (val);
  return 0;
}

static int
b2s (struct variable *vp, void *val)
{
  vp->v.string = estrdup (*(int*)val ? "true" : "false");
  return 0;
}

static int
i2s (struct variable *vp, void *val)
{
  char buf[128];
  snprintf (buf, sizeof buf, "%d", *(int*)val);
  vp->v.string = estrdup (buf);
  return 0;
}

static int
s2b (struct variable *vp, void *val)
{
  static char *trueval[] = { "on", "true", "yes", NULL };
  static char *falseval[] = { "off", "false", "no", NULL };
  int i;
  unsigned long n;
  char *p;
  
  for (i = 0; trueval[i]; i++)
    if (strcasecmp (trueval[i], val) == 0)
      {
	vp->v.bool = 1;
	return 0;
      }
  
  for (i = 0; falseval[i]; i++)
    if (strcasecmp (falseval[i], val) == 0)
      {
	vp->v.bool = 0;
	return 1;
      }
  
  n = strtoul (val, &p, 0);
  if (*p)
    return VAR_ERR_BADTYPE;
  vp->v.bool = !!n;
  return VAR_OK;
}
  
static int
s2i (struct variable *vp, void *val)
{
  char *p;
  int n = strtoul (val, &p, 0);

  if (*p)
    return VAR_ERR_BADTYPE;

  vp->v.num = n;
  return VAR_OK;
}

static int
b2b (struct variable *vp, void *val)
{
  vp->v.bool = !!*(int*)val;
  return VAR_OK;
}

static int
b2i (struct variable *vp, void *val)
{
  vp->v.num = *(int*)val;
  return VAR_OK;
}

static int
i2i (struct variable *vp, void *val)
{
  vp->v.num = *(int*)val;
  return VAR_OK;
}

static int
i2b (struct variable *vp, void *val)
{
  vp->v.bool = *(int*)val;
  return VAR_OK;
}

static setvar_t setvar[3][3] = {
            /*    s     b    i */
  /* s */    {   s2s,  b2s, i2s },
  /* b */    {   s2b,  b2b, i2b },
  /* i */    {   s2i,  b2i, i2i }
};

int
variable_set (const char *name, int type, void *val)
{
  struct variable *vp = varfind (name);
  int rc;
  
  if (!vp)
    return VAR_ERR_NOTDEF;

  if (vp->hook && vp->hook (vp, 0))
    return VAR_ERR_FAILURE;
  
  if (vp->type == VART_STRING && (vp->flags && VARF_SET))
    free (vp->v.string);
  rc = setvar[vp->type][type] (vp, val);

  if (rc)
    return rc;

  vp->flags |= VARF_SET;

  if (vp->hook)
    vp->hook (vp, 1);

  return VAR_OK;
}

int
variable_get (const char *name, int type, void **val)
{
  struct variable *vp = varfind (name);

  if (!vp)
    return VAR_ERR_NOTDEF;
  
  if (type != vp->type)
    return VAR_ERR_BADTYPE;
  
  switch (vp->type)
    {
    case VART_STRING:
      *val = vp->v.string;
      break;

    case VART_BOOL:
      *(int*)val = vp->v.bool;
      break;
      
    case VART_INT:
      *(int*)val = vp->v.num;
      break;
    }

  return VAR_OK;
}

void
variable_print_all (FILE *fp)
{
  struct variable *vp;
  char *s;
  
  for (vp = vartab; vp->name; vp++)
    {
      switch (vp->type)
	{
	case VART_INT:
	  fprintf (fp, "%s=%d", vp->name, vp->v.num);
	  break;
	  
	case VART_BOOL:
	  fprintf (fp, "%s%s", vp->v.bool ? "no" : "", vp->name);
	  break;

	case VART_STRING:
	  fprintf (fp, "%s=\"", vp->name);
	  for (s = vp->v.string; *s; s++)
	    {
	      int c;
	      
	      if (isprint (*s))
		fputc (*s, fp);
	      else if ((c = escape (*s)))
		fprintf (fp, "\\%c", c);
	      else
		fprintf (fp, "\\%03o", *s);
	    }
	  fprintf (fp, "\"");
	}
      fputc ('\n', fp);
    }
}
	   
int
variable_is_set (const char *name)
{
  int n;

  if (variable_get (name, VART_BOOL, (void **) &n))
    return 1;
  return n;
}
