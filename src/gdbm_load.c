/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2011 Free Software Foundation, Inc.

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

# include "autoconf.h"
# include "gdbm.h"
# include "gdbmapp.h"
# include "gdbmdefs.h"

/* usage: gdbm_load [OPTIONS] FILE [DB_FILE]
   FILE is either "-" or a file name.
   OPTIONS are:
    -h, --help
    -u, --usage
    -v, --version

    -p, --permissions OOO
    -u, --user NAME-OR-UID
    -g, --group NAME-OR-GID
*/

int replace = 0;

char *parseopt_program_doc = "load a GDBM database from a file";
char *parseopt_program_args = "FILE [DB_FILE]";
struct gdbm_option optab[] = {
  { 'r', "replace", NULL, N_("replace records in the existing database") },
  { 0 }
};

int
main (int argc, char **argv)
{
  GDBM_FILE dbf = NULL;
  int rc, opt;
  char *dbname, *filename;
  FILE *fp;
  unsigned long err_line;

  set_progname (argv[0]);

  for (opt = parseopt_first (argc, argv, optab);
       opt != EOF;
       opt = parseopt_next ())
    {
    switch (opt)
      {
      case 'r':
	replace = 1;
	break;
	
      default:
	error (_("unknown option"));
	exit (2);
      }
    }

  argc -= optind;
  argv += optind;
  
  if (argc == 0)
    {
      parseopt_print_help ();
      exit (0);
    }

  if (argc > 2)
    {
      error (_("too many arguments; try `%s -h' for more info"), progname);
      exit (2);
    }
  
  filename = argv[0];
  if (argc == 2)
    dbname = argv[1];
  else
    dbname = NULL;

  if (strcmp (filename, "-") == 0)
    {
      filename = "<stdin>";
      fp = stdin;
    }
  else
    {
      fp = fopen (filename, "r");
      if (!fp)
	{
	  sys_perror (errno, _("cannot open %s"), filename);
	  exit (1);
	}
    }
  
  if (dbname)
    {
      dbf = gdbm_open (dbname, 0, GDBM_NEWDB, 0600, NULL);
      if (!dbf)
	{
	  gdbm_perror (_("gdbm_open failed"));
	  exit (1);
	}
    }
  
  rc = gdbm_load_from_file (&dbf, fp, replace, &err_line);
  if (rc)
    {
      if (err_line)
	gdbm_perror ("%s:%lu", filename, err_line);
      else
	gdbm_perror ("cannot load from %s", filename);
    }
  
  if (dbf)
    {
      if (!dbname)
	{
	  if (gdbm_setopt (dbf, GDBM_GETDBNAME, &dbname, sizeof (dbname)))
	    gdbm_perror (_("gdbm_setopt failed"));
	  else
	    {
	      printf ("%s: created %s\n", progname, dbname);
	      free (dbname);
	    }
	}
      gdbm_close (dbf);
    }
  exit (!!rc);
}
