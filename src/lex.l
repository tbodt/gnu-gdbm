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

#include "gdbmtool.h"
#include "gram.h"

struct point point;
 
/* Advance locus to the next line */
void
advance_line ()
{
  ++point.line;
  point.col = 0;
}

#define YY_USER_ACTION					    \
  do							    \
    {							    \
      if (YYSTATE == 0)					    \
	{						    \
	  yylloc.beg = point;				    \
	  yylloc.beg.col++;				    \
	}						    \
      point.col += yyleng;				    \
      yylloc.end = point;				    \
    }							    \
  while (0);

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size)                                   \
  do									\
    {									\
      result = read_input (buf, max_size);				\
    }									\
  while (0);
 
void string_begin (void);
void string_add (const char *s, int l);
void string_addc (int c);
char *string_end (void);
int unescape (int c);

static ssize_t read_input (char *buf, size_t size);
 
%}

%x STR
%s DEF

WS [ \t][ \t]*
IDENT [a-zA-Z_][a-zA-Z_0-9-]*
N [0-9][0-9]*
P [1-9][0-9]*
X [0-9a-fA-F]
O [0-7]

%%
^[ \t]*#[ \t]*line[ \t].*\n {
  char *p, *q;
  char *file = NULL;
  int line, len;
  
  for (p = strchr (yytext, '#') + 1; *p == ' ' || *p == '\t'; p++);
  p += 4;
  for (; *p == ' ' || *p == '\t'; p++);

  line = strtol (p, &p, 10);
  for (; *p == ' ' || *p == '\t'; p++);

  if (*p == '"')
    {
      p++;
      len = strcspn (p, "\"");
      if (p[len] == 0)
	{
	  yyerror (_("invalid #line statement"));
	  REJECT;
	}
      file = emalloc (len + 1);
      memcpy (file, p, len);
      file[len] = 0;
      for (p += len + 1; *p == ' ' || *p == '\t'; p++);
    }
  if (*p != '\n' )
    {
      yyerror (_("invalid #line statement"));
      free (file);
      REJECT;
    }
  if (file)
    point.file = file;
  point.line = line;
  point.col = 0;
}
#.*\n              advance_line ();
#.*     /* end-of-file comment */;

<DEF>off           { return T_OFF; }
<DEF>pad           { return T_PAD; }
<DEF>0[xX]{X}{X}*  { yylval.num = strtoul (yytext, NULL, 16);
                     return T_NUM; };
<DEF>0{O}{O}*      { yylval.num = strtoul (yytext, NULL, 8);
                     return T_NUM; };
<DEF>0|{P}         { yylval.num = strtoul (yytext, NULL, 10);
                     return T_NUM; };

^[ \t]*{IDENT}     { if (YYSTATE == DEF)
	               {
			 if (yylval.type = datadef_lookup (yytext))
			   return T_TYPE;
		       }
                     else
                       {
			 return command_lookup (yytext, &yylloc, &yylval.cmd);
		       }

		     yylval.string = estrdup (yytext);
		     return T_IDENT;
                   }
{IDENT}            { if (YYSTATE == DEF &&
			 (yylval.type = datadef_lookup (yytext)))
			 return T_TYPE;
                     else
		       {
			 yylval.string = estrdup (yytext);
			 return T_IDENT;
		       }
                   }
[^ \t\n\[\]{},=]+  { yylval.string = estrdup (yytext); return T_WORD; }
\"[^\\\"\n]*\"     { yylval.string = emalloc (yyleng - 1);
                     memcpy (yylval.string, yytext+1, yyleng-2);
		     yylval.string[yyleng-2] = 0;
		     return T_WORD; }
\"[^\\\"\n]*\\.    { string_begin ();
                     string_add (yytext + 1, yyleng - 3);
		     string_addc (unescape (yytext[yyleng-1]));
                     BEGIN (STR); }
<STR>[^\\\"\n]*\"  { if (yyleng > 1)
		       string_add (yytext, yyleng - 1);
		     yylval.string = string_end ();
		     BEGIN (INITIAL);
                     return T_WORD; }
<STR>[^\\\"\n]*\\. { string_add (yytext, yyleng - 2);
                     string_addc (unescape (yytext[yyleng-1])); }
{WS}               ;
\n                 { advance_line (); if (YYSTATE == INITIAL) return '\n'; }
.                  return yytext[0];
%%

void
setsource (const char *filename, FILE *file)
{
  point.file = (char*) filename;
  point.line = 1;
  point.col = 0;
  yyin = file;
}

int
yywrap ()
{
  return 1;
}

void
begin_def (void)
{
  BEGIN (DEF);
}

void
end_def (void)
{
  BEGIN (INITIAL);
} 

static ssize_t
read_input (char *buf, size_t size)
{
  if (interactive)
    {
      if (YY_AT_BOL ())
	print_prompt ();
      if (fgets (buf, size, yyin) == NULL)
	return 0;
      return strlen (buf);
    }
  return fread (buf, 1, size, yyin);
}
  

struct strseg
{
  struct strseg *next;
  int len;
  char ptr[1];
};

static struct strseg *strseg_head, *strseg_tail;

void
string_begin (void)
{
  strseg_head = strseg_tail = NULL;
}

void
strseg_attach (struct strseg *seg)
{
  seg->next = NULL;
  if (strseg_tail)
    strseg_tail->next = seg;
  else
    strseg_head = seg;
  strseg_tail = seg;
}  

void
string_add (const char *s, int l)
{
  struct strseg *seg = emalloc (sizeof (*seg) + l);
  memcpy (seg->ptr, s, l);
  seg->len = l;
  strseg_attach (seg);
}

void
string_addc (int c)
{
  struct strseg *seg = emalloc (sizeof (*seg));
  seg->ptr[0] = c;
  seg->len = 1;
  strseg_attach (seg);
}

char *
string_end (void)
{
  int len = 1;
  struct strseg *seg;
  char *ret, *p;
  
  for (seg = strseg_head; seg; seg = seg->next)
    len += seg->len;

  ret = emalloc (len);
  p = ret;
  for (seg = strseg_head; seg; )
    {
      struct strseg *next = seg->next;
      memcpy (p, seg->ptr, seg->len);
      p += seg->len;
      free (seg);
      seg = next;
    }
  *p = 0;

  strseg_head = strseg_tail = NULL;

  return ret;
}

static char transtab[] = "\\\\\"\"a\ab\bf\fn\nr\rt\tv\v";

int
unescape (int c)
{
  char *p;

  for (p = transtab; *p; p += 2)
    {
      if (*p == c)
	return p[1];
    }
  return c;
}

int
escape (int c)
{
  char *p;
  for (p = transtab + sizeof (transtab) - 2; p > transtab; p -= 2)
    {
      if (*p == c)
	return p[-1];
    }
  return 0;
}

void
vparse_error (struct locus *loc, const char *fmt, va_list ap)
{
  if (!interactive)
    fprintf (stderr, "%s", progname);
  if (loc)
    YY_LOCATION_PRINT (stderr, *loc);
  fprintf (stderr, ": ");
  vfprintf (stderr, fmt, ap);
  fputc ('\n', stderr);
}

void
parse_error (struct locus *loc, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vparse_error (loc, fmt, ap);
  va_end (ap);
}


struct prompt_exp;

void
pe_file_name (struct prompt_exp *p)
{
  fwrite (file_name, strlen (file_name), 1, stdout);
}

void
pe_program_name (struct prompt_exp *p)
{
  fwrite (progname, strlen (progname), 1, stdout);
}

void
pe_package_name (struct prompt_exp *p)
{
  fwrite (PACKAGE_NAME, sizeof (PACKAGE_NAME) - 1, 1, stdout);
}

void
pe_program_version (struct prompt_exp *p)
{
  fwrite (PACKAGE_VERSION, sizeof (PACKAGE_VERSION) - 1, 1, stdout);
}

void
pe_space (struct prompt_exp *p)
{
  fwrite (" ", 1, 1, stdout);
}

struct prompt_exp
{
  int ch;
  void (*fun) (struct prompt_exp *);
  char *cache;
};

struct prompt_exp prompt_exp[] = {
  { 'f', pe_file_name },
  { 'p', pe_program_name },
  { 'P', pe_package_name },
  { 'v', pe_program_version },
  { '_', pe_space },
  { 0 }
};

static void
expand_char (int c)
{
  struct prompt_exp *p;

  if (c && c != '%')
    {
      for (p = prompt_exp; p->ch; p++)
	{
	  if (c == p->ch)
	    {
	      if (p->cache)
		free (p->cache);
	      return p->fun (p);
	    }
	}
    }
  putchar ('%');
  putchar (c);
}

char const *
psname ()
{
  if (YYSTATE == DEF)
    return "ps2";
  return "ps1";
}

void
print_prompt ()
{
  const char *s;
  const char *prompt;

  if (variable_get (psname (), VART_STRING, (void *) &prompt))
    abort ();

  for (s = prompt; *s; s++)
    {
      if (*s == '%')
	{
	  if (!*++s)
	    {
	      putchar ('%');
	      break;
	    }
	  expand_char (*s);
	}
      else
	putchar (*s);
    }

  fflush (stdout);
}
