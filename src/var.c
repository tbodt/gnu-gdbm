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
  } v;
};

static struct variable vartab[] = {
  /* Top-level prompt */
  { "ps1", VART_STRING, VARF_DFL, { "%p>%_" } },
  /* Second-level prompt (used within "def" block) */
  { "ps2", VART_STRING, VARF_DFL, { "%_>%_" } },
  /* This delimits array members */
  { "delim1", VART_STRING, VARF_DFL, { "," } },
  /* This delimits structure members */
  { "delim2", VART_STRING, VARF_DFL, { "," } },
  { NULL }
};

static struct variable *
varfind (const char *name)
{
  struct variable *vp;

  for (vp = vartab; vp->name; vp++)
    if (strcmp (vp->name, name) == 0)
      return vp;
  
  return NULL;
}

int
variable_set (const char *name, int type, void *val)
{
  struct variable *vp = varfind (name);

  if (!vp)
    return VAR_ERR_NOTDEF;
  
  if (type != vp->type)
    return VAR_ERR_BADTYPE;
  
  switch (vp->type)
    {
    case VART_STRING:
      if (vp->flags && VARF_SET)
	free (vp->v.string);
      vp->v.string = estrdup (val);
      break;
 
    case VART_BOOL:
      if (val == NULL)
	vp->v.bool = 0;
      else
	vp->v.bool = *(int*)val;
      break;
    }

  vp->flags |= VARF_SET;

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
	   
