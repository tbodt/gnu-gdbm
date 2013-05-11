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

#include "autoconf.h"
#include "gdbmdefs.h"
#include "gdbm.h"
#include "gdbmapp.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* Position in input file */
struct point
{
  char *file;             /* file name */
  unsigned line;          /* line number */  
  unsigned col;           /* column number */ 
};

/* Location in input file */
struct locus
{
  struct point beg, end;
};

typedef struct locus gdbm_yyltype_t;
 
#define YYLTYPE gdbm_yyltype_t 

#define YYLLOC_DEFAULT(Current, Rhs, N)			      \
  do							      \
    {							      \
      if (N)						      \
	{						      \
	  (Current).beg = YYRHSLOC(Rhs, 1).beg;		      \
	  (Current).end = YYRHSLOC(Rhs, N).end;		      \
	}						      \
      else						      \
	{						      \
	  (Current).beg = YYRHSLOC(Rhs, 0).end;		      \
	  (Current).end = (Current).beg;		      \
	}						      \
    }							      \
  while (0)

#define YY_LOCATION_PRINT(File, Loc)			  \
  do							  \
    {							  \
      if ((Loc).beg.col == 0)				  \
	fprintf (File, "%s:%u",				  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line);			  \
      else if (strcmp ((Loc).beg.file, (Loc).end.file))	  \
	fprintf (File, "%s:%u.%u-%s:%u.%u",		  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.file,			  \
		 (Loc).end.line, (Loc).end.col);	  \
      else if ((Loc).beg.line != (Loc).end.line)	  \
	fprintf (File, "%s:%u.%u-%u.%u",		  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.line, (Loc).end.col);	  \
      else if ((Loc).beg.col != (Loc).end.col)		  \
	fprintf (File, "%s:%u.%u-%u",			  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.col);			  \
      else						  \
	fprintf (File, "%s:%u.%u",			  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line,			  \
		 (Loc).beg.col);			  \
    }							  \
  while (0)

void vparse_error (struct locus *loc, const char *fmt, va_list ap);
void parse_error (struct locus *loc, const char *fmt, ...);

void syntax_error (const char *fmt, ...);

void print_prompt (void);

void setsource (const char *filename, FILE *file);

extern int interactive;

/* Argument to a command handler */
struct gdbmarg
{
  struct gdbmarg *next;
  char *string;
};

/* List of arguments */
struct gdbmarglist
{
  struct gdbmarg *head, *tail;
};

void gdbmarglist_init (struct gdbmarglist *, struct gdbmarg *);
void gdbmarglist_add (struct gdbmarglist *, struct gdbmarg *);
void gdbmarglist_free (struct gdbmarglist *lst);

struct gdbmarg *gdbmarg_new (char *);
		       
int run_command (const char *verb, struct gdbmarglist *arglist);

