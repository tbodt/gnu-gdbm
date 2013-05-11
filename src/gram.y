%{
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

#include <autoconf.h>
#include "gdbmtool.h"

%}

%error-verbose
%locations
     
%token <string> T_IDENT T_WORD
%type <string> arg verb
%type <arglist> arglist arg1list

%union {
  char *string;
  struct gdbmarglist arglist;
}

%%

input     : /* empty */
          | stmtlist
	  ;

stmtlist  : stmt
          | stmtlist stmt
          ;

stmt      : /* empty */ '\n'
          | verb arglist '\n'
            {
	      if (run_command ($1, &$2) && !interactive)
		exit (EXIT_USAGE);
	      gdbmarglist_free (&$2);
	    }
          | error '\n'
            {
	      if (interactive)
		{
		  yyclearin;
		  yyerrok;
		}
	      else
		YYERROR;
	    }
          ;

verb      : T_IDENT
          ;

arglist   : /* empty */
            {
	      gdbmarglist_init (&$$, NULL);
	    }
          | arg1list
	  ;

arg1list  : arg
            {
	      gdbmarglist_init (&$$, gdbmarg_new ($1));
	    }
          | arg1list arg
	    {
	      gdbmarglist_add (&$1, gdbmarg_new ($2));
	      $$ = $1;
	    }
          ;

arg       : T_IDENT
          | T_WORD
          ;

%%

void
syntax_error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vparse_error (&yylloc, fmt, ap);
  va_end (ap);
}

int
yyerror (char *s)
{
  syntax_error ("%s", s);
  return 0;
}
