/* testgdbm.c - Driver program to test the database routines and to
   help debug gdbm.  Uses inside information to show "system" information */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007  Free Software Foundation, Inc.

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
#include "extern.h"

#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#endif
#include <stdarg.h>

extern const char *gdbm_version;

extern const char *gdbm_strerror (gdbm_error);

char *progname;                     /* Program name */

char *file_name = NULL;             /* Database file name */   
GDBM_FILE gdbm_file = NULL;   /* Database to operate upon */
int interactive;                    /* Are we running in interactive mode? */
datum key_data;                     /* Current key */
datum return_data;                  /* Current data */
int key_z = 1;                      /* Keys are nul-terminated strings */
int data_z = 1;                     /* Data are nul-terminated strings */


/* Debug procedure to print the contents of the current hash bucket. */
void
print_bucket (hash_bucket * bucket, char *mesg)
{
  int             index;

  printf ("******* %s **********\n\nbits = %d\ncount= %d\nHash Table:\n",
	  mesg, bucket->bucket_bits, bucket->count);
  printf
    ("     #    hash value     key size    data size     data adr  home\n");
  for (index = 0; index < gdbm_file->header->bucket_elems; index++)
    printf ("  %4d  %12x  %11d  %11d  %11d %5d\n", index,
	    bucket->h_table[index].hash_value,
	    bucket->h_table[index].key_size,
	    bucket->h_table[index].data_size,
	    bucket->h_table[index].data_pointer,
	    bucket->h_table[index].hash_value %
	    gdbm_file->header->bucket_elems);

  printf ("\nAvail count = %1d\n", bucket->av_count);
  printf ("Avail  adr     size\n");
  for (index = 0; index < bucket->av_count; index++)
    printf ("%9d%9d\n", bucket->bucket_avail[index].av_adr,
	    bucket->bucket_avail[index].av_size);
}


void
_gdbm_print_avail_list (GDBM_FILE dbf)
{
  int             temp;
  int             size;
  avail_block    *av_stk;

  /* Print the the header avail block.  */
  printf ("\nheader block\nsize  = %d\ncount = %d\n",
	  dbf->header->avail.size, dbf->header->avail.count);
  for (temp = 0; temp < dbf->header->avail.count; temp++)
    {
      printf ("  %15d   %10d \n", dbf->header->avail.av_table[temp].av_size,
	      dbf->header->avail.av_table[temp].av_adr);
    }

  /* Initialize the variables for a pass throught the avail stack. */
  temp = dbf->header->avail.next_block;
  size = (((dbf->header->avail.size * sizeof (avail_elem)) >> 1)
	  + sizeof (avail_block));
  av_stk = (avail_block *) malloc (size);
  if (av_stk == NULL)
    {
      printf ("Out of memory\n");
      exit (2);
    }

  /* Print the stack. */
  while (FALSE)
    {
      __lseek (dbf, temp, L_SET);
      __read (dbf, av_stk, size);

      /* Print the block! */
      printf ("\nblock = %d\nsize  = %d\ncount = %d\n", temp,
	      av_stk->size, av_stk->count);
      for (temp = 0; temp < av_stk->count; temp++)
	{
	  printf ("  %15d   %10d \n", av_stk->av_table[temp].av_size,
		  av_stk->av_table[temp].av_adr);
	}
      temp = av_stk->next_block;
    }
}

void
_gdbm_print_bucket_cache (FILE *fp, GDBM_FILE dbf)
{
  int             index;
  char            changed;

  if (dbf->bucket_cache != NULL)
    {
      fprintf
	(fp,
	 "Bucket Cache (size %d):\n  Index:  Address  Changed  Data_Hash \n",
	 dbf->cache_size);
      for (index = 0; index < dbf->cache_size; index++)
	{
	  changed = dbf->bucket_cache[index].ca_changed;
	  fprintf (fp, "  %5d:  %7d  %7s  %x\n",
		   index,
		   dbf->bucket_cache[index].ca_adr,
		   (changed ? "True" : "False"),
		   dbf->bucket_cache[index].ca_data.hash_val);
	}
    }
  else
    fprintf (fp, "Bucket cache has not been initialized.\n");
}

void
usage ()
{
  printf ("Usage: %s OPTIONS\n", progname);
  printf ("Test and modify a GDBM database.\n");
  printf ("\n");
  printf ("OPTIONS are:\n\n");
  printf ("  -b SIZE            set block size\n");
  printf ("  -c SIZE            set cache size\n");
  printf ("  -g FILE            operate on FILE instead of `junk.gdbm'\n");
  printf ("  -h                 print this help summary\n");
  printf ("  -l                 disable file locking\n");
  printf ("  -m                 disable file mmap\n");
  printf ("  -n                 create database\n");
  printf ("  -r                 open database in read-only mode\n");
  printf ("  -s                 synchronize to the disk after each write\n");
  printf ("  -v                 print program version\n");
  printf ("\n");
  printf ("Report bugs to <%s>.\n", PACKAGE_BUGREPORT);
}

void
version ()
{
  printf ("testgdbm (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  printf ("Copyright (C) 2007 Free Software Foundation, Inc.\n");
  printf ("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
  printf ("This is free software: you are free to change and redistribute it.\n");
  printf ("There is NO WARRANTY, to the extent permitted by law.\n");
}

void
error (int code, char *fmt, ...)
{
  va_list ap;
  fprintf (stderr, "%s: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (fmt);
  fputc ('\n', stderr);
  if (code)
    exit (code);
}
  
int
trimnl (char *str)
{
  int len = strlen (str);

  if (str[len - 1] == '\n')
    {
      str[--len] = 0;
      return 1;
    }
  return 0;
}

void
read_from_file (const char *name, int replace)
{
  int line = 0;
  char buf[1024];
  datum key;
  datum data;
  FILE *fp;
  int flag = replace ? GDBM_REPLACE : 0;
  
  fp = fopen (name, "r");
  if (!fp)
    {
      fprintf (stderr, "cannot open file `%s' for reading: %s\n",
	       name, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof buf, fp))
    {
      char *p;

      if (!trimnl (buf))
	{
	  fprintf (stderr, "%s:%d: line too long\n", name, line);
	  continue;
	}

      line++;
      p = strchr (buf, ' ');
      if (!p)
	{
	  fprintf (stderr, "%s:%d: malformed line\n", name, line);
	  continue;
	}

      for (*p++ = 0; *p && isspace (*p); p++)
	;
      key.dptr = buf;
      key.dsize = strlen (buf) + key_z;
      data.dptr = p;
      data.dsize = strlen (p) + data_z;
      if (gdbm_store (gdbm_file, key, data, flag) != 0)
	fprintf (stderr, "%d: item not inserted\n", line);
    }
  fclose (fp);
}

int
get_screen_lines ()
{
#ifdef TIOCGWINSZ
  if (isatty (1))
    {
      struct winsize ws;

      ws.ws_col = ws.ws_row = 0;
      if ((ioctl(1, TIOCGWINSZ, (char *) &ws) < 0) || ws.ws_row == 0)
	{
	  const char *lines = getenv ("LINES");
	  if (lines)
	    ws.ws_row = strtol (lines, NULL, 10);
	}
      return ws.ws_row;
    }
#else
  const char *lines = getenv ("LINES");
  if (lines)
    return strtol (lines, NULL, 10);
#endif
  return -1;
}

void
page_data (int count, void (*func) (FILE *, void *), void *data)
{
  FILE *out = stdout;
  if (interactive && (count < 0 || count > get_screen_lines ()))
    {
      char *pager = getenv ("PAGER");
      if (pager)
	{
	  out = popen (getenv ("PAGER"), "w");
	  if (!out)
	    {
	      fprintf (stderr, "cannot run pager `%s': %s\n", pager,
		       strerror (errno));
	      out = stdout;
	    }
	}
    }
  func (out, data);
  if (out != stdout)
    pclose (out);
}  

int
get_record_count ()
{
  datum key, data;
  int count = 0;
  data = gdbm_firstkey (gdbm_file);
  while (data.dptr != NULL)
    {
      count++;
      key = data;
      data = gdbm_nextkey (gdbm_file, key);
      free (key.dptr);
    }
  return count;
}


#define ARG_UNUSED __attribute__ ((__unused__))

#define NARGS 2

void
count_handler (char *arg[NARGS] ARG_UNUSED)
{
  printf ("There are %d items in the database.\n", get_record_count ());
}

void
delete_handler (char *arg[NARGS])
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data.dptr = strdup (arg[0]);
  key_data.dsize = strlen (arg[0]) + key_z;
  if (gdbm_delete (gdbm_file, key_data) != 0)
    printf ("Item not found or deleted\n");
}

void
fetch_handler (char *arg[NARGS])
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data.dptr = strdup (arg[0]);
  key_data.dsize = strlen (arg[0]) + key_z;
  return_data = gdbm_fetch (gdbm_file, key_data);
  if (return_data.dptr != NULL)
    {
      printf ("data is ->%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    printf ("No such item found.\n");
}

void
nextkey_handler (char *arg[NARGS])
{
  if (arg[0])
    {
      if (key_data.dptr != NULL)
	free (key_data.dptr);
      key_data.dptr = strdup (arg[0]);
      key_data.dsize = strlen (arg[0]) + key_z;
    }
  return_data = gdbm_nextkey (gdbm_file, key_data);
  if (return_data.dptr != NULL)
    {
      key_data = return_data;
      printf ("key is  ->%.*s\n", key_data.dsize, key_data.dptr);
      return_data = gdbm_fetch (gdbm_file, key_data);
      printf ("data is ->%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    {
      printf ("No such item found.\n");
      free (key_data.dptr);
      key_data.dptr = NULL;
    }
}

void
store_handler (char *arg[NARGS])
{
  datum key;
  datum data;
  
  key.dptr = arg[0];
  key.dsize = strlen (arg[0]) + key_z;
  data.dptr = arg[1];
  data.dsize = strlen (arg[1]) + data_z;
  if (gdbm_store (gdbm_file, key, data, GDBM_REPLACE) != 0)
    printf ("Item not inserted. \n");
}

void
firstkey_handler (char *arg[NARGS])
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data = gdbm_firstkey (gdbm_file);
  if (key_data.dptr != NULL)
    {
      printf ("key is  ->%.*s\n", key_data.dsize, key_data.dptr);
      return_data = gdbm_fetch (gdbm_file, key_data);
      printf ("data is ->%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    printf ("No such item found.\n");
}

void
next_on_last_handler (char *arg[NARGS] ARG_UNUSED)
{
  return_data = gdbm_nextkey (gdbm_file, key_data);
  if (return_data.dptr != NULL)
    {
      free (key_data.dptr);
      key_data = return_data;
      printf ("key is  ->%.*s\n", key_data.dsize, key_data.dptr);
      return_data = gdbm_fetch (gdbm_file, key_data);
      printf ("data is ->%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    printf ("No such item found.\n");
}

void
reorganize_handler (char *arg[NARGS] ARG_UNUSED)
{
  if (gdbm_reorganize (gdbm_file))
    printf ("Reorganization failed.\n");
  else
    printf ("Reorganization succeeded.\n");
}

void
avail_handler (char *arg[NARGS] ARG_UNUSED)
{
  _gdbm_print_avail_list (gdbm_file);
}

void
print_current_bucket_handler (char *arg[NARGS] ARG_UNUSED)
{
  print_bucket (gdbm_file->bucket, "Current bucket");
  printf ("\n current directory entry = %d.\n",
	  gdbm_file->bucket_dir);
  printf (" current bucket address  = %d.\n",
	  gdbm_file->cache_entry->ca_adr);
}

int
getnum (int *pnum, char *arg, char **endp)
{
  char *p;
  unsigned long x = strtoul (arg, &p, 10);
  if (*p && !isspace (*p))
    {
      printf ("not a number (stopped near %s)\n", p);
      return 1;
    }
  while (*p && isspace (*p))
    p++;
  if (endp)
    *endp = p;
  else
    {
      printf ("not a number (stopped near %s)\n", p);
      return 1;
    }
  *pnum = x;
  return 0;
}
  
void
print_bucket_handler (char *arg[NARGS])
{
  int temp;

  if (getnum (&temp, arg[0], NULL))
    return;

  if (temp >= gdbm_file->header->dir_size / 4)
    {
      printf ("Not a bucket. \n");
    }
  _gdbm_get_bucket (gdbm_file, temp);

  printf ("Your bucket is now ");
  print_current_bucket_handler (NULL);
}

void
_print_dir (FILE *out, void *data)
{
  int i;

  fprintf (out, "Hash table directory.\n");
  fprintf (out, "  Size =  %d.  Bits = %d. \n\n",
	   gdbm_file->header->dir_size, gdbm_file->header->dir_bits);
  
  for (i = 0; i < gdbm_file->header->dir_size / 4; i++)
    fprintf (out, "  %10d:  %12d\n", i, gdbm_file->dir[i]);
}

void
print_dir_handler (char *arg[NARGS] ARG_UNUSED)
{
  page_data (gdbm_file->header->dir_size / 4 + 3, _print_dir, NULL);
}

void
print_header_handler (char *arg[NARGS] ARG_UNUSED)
{
  printf ("\nFile Header: \n\n");
  printf ("  table        = %d\n", gdbm_file->header->dir);
  printf ("  table size   = %d\n", gdbm_file->header->dir_size);
  printf ("  table bits   = %d\n", gdbm_file->header->dir_bits);
  printf ("  block size   = %d\n", gdbm_file->header->block_size);
  printf ("  bucket elems = %d\n", gdbm_file->header->bucket_elems);
  printf ("  bucket size  = %d\n", gdbm_file->header->bucket_size);
  printf ("  header magic = %x\n", gdbm_file->header->header_magic);
  printf ("  next block   = %d\n", gdbm_file->header->next_block);
  printf ("  avail size   = %d\n", gdbm_file->header->avail.size);
  printf ("  avail count  = %d\n", gdbm_file->header->avail.count);
  printf ("  avail nx blk = %d\n",
	  gdbm_file->header->avail.next_block);
}  

void
hash_handler (char *arg[NARGS])
{
  datum key;
  
  key.dptr = arg[0];
  key.dsize = strlen (arg[0]) + key_z;
  printf ("hash value = %x. \n", _gdbm_hash (key));
}

static void
print_bucket_cache (FILE *out, void *data)
{
  _gdbm_print_bucket_cache (out, data);
}

void
print_cache_handler (char *arg[NARGS] ARG_UNUSED)
{
  page_data (gdbm_file->bucket_cache ? gdbm_file->cache_size + 1 : 1,
	     print_bucket_cache, gdbm_file);
}

void
print_version_handler (char *arg[NARGS] ARG_UNUSED)
{
  printf ("%s\n", gdbm_version);
}

void
read_handler (char *arg[NARGS])
{
  read_from_file (arg[0], arg[1] && strcmp (arg[1], "replace"));
}

void
list_all (FILE *out, void *x)
{
  datum key;
  datum data;

  key = gdbm_firstkey (gdbm_file);
  while (key.dptr)
    {
      datum nextkey = gdbm_nextkey (gdbm_file, key);

      data = gdbm_fetch (gdbm_file, key);
      if (!data.dptr)
	fprintf (out, "cannot fetch data (key %.*s)\n", key.dsize, key.dptr);
      else
	{
	  fprintf (out, "%.*s %.*s\n", key.dsize, key.dptr, data.dsize,
		   data.dptr);
	  free (data.dptr);
	}
      free (key.dptr);
      key = nextkey;
    }
}

void
list_handler (char *arg[NARGS] ARG_UNUSED)
{
  page_data (get_record_count (), list_all, NULL);
}

void
quit_handler (char *arg[NARGS] ARG_UNUSED)
{
  if (gdbm_file != NULL)
    gdbm_close (gdbm_file);

  exit (0);
}

void
export_handler (char *arg[NARGS])
{
  int flags = GDBM_WRCREAT;

  if (arg[1] != NULL && strcmp(arg[1], "truncate") == 0)
    flags = GDBM_NEWDB;

  if (gdbm_export (gdbm_file, arg[0], flags, 0600) == -1)
    printf ("gdbm_export failed, %s\n", gdbm_strerror (gdbm_errno));
}

void
import_handler (char *arg[NARGS])
{
  int flag = GDBM_INSERT;

  if (arg[1] != NULL && strcmp(arg[1], "replace") == 0)
    flag = GDBM_REPLACE;

  if (gdbm_import (gdbm_file, arg[0], flag) == -1)
    printf ("gdbm_import failed, %s\n", gdbm_strerror (gdbm_errno));
}

void
status_handler (char *arg[NARGS] ARG_UNUSED)
{
  printf ("Database file: %s\n", file_name);
  printf ("Zero terminated keys: %s\n", key_z ? "yes" : "no");
  printf ("Zero terminated data: %s\n", data_z ? "yes" : "no");
}

void
key_z_handler (char *arg[NARGS] ARG_UNUSED)
{
  key_z = !key_z;
  printf ("Zero terminated keys: %s\n", key_z ? "yes" : "no");
}

void
data_z_handler (char *arg[NARGS] ARG_UNUSED)
{
  data_z = !data_z;
  printf ("Zero terminated data: %s\n", data_z ? "yes" : "no");
}


void help_handler (char *arg[NARGS]);

struct command
{
  int abbrev;
  void (*handler) (char *[NARGS]);
  char *args[NARGS];
  char *doc;
};


struct command command_tab[] = {
  { 'c', count_handler, { NULL, NULL, }, "count (number of entries)" },
  { 'd', delete_handler, { "key", NULL, }, "delete" },
  { 'e', export_handler, { "file", "[truncate]", }, "export" },
  { 'f', fetch_handler, { "key", NULL }, "fetch" },
  { 'i', import_handler, { "file", "[replace]", }, "import" },
  { 'l', list_handler, { NULL, NULL }, "list" },
  { 'n', nextkey_handler, { "[key]", NULL }, "nextkey" },
  { 'q', quit_handler, { NULL, NULL }, "quit" },
  { 's', store_handler, { "key", "data" }, "store" },
  { '1', firstkey_handler, { NULL, NULL }, "firstkey" },
  { '2', next_on_last_handler, { NULL, NULL, },
    "nextkey on last key (from n, 1 or 2)" },
  { '<', read_handler, { "file", "[replace]" },
    "read entries from file and store" },
  { 'r', reorganize_handler, { NULL, NULL, }, "reorganize" },
  { 'z', key_z_handler, { NULL, NULL }, "toggle key nul-termination" },
  { 'A', avail_handler, { NULL, NULL, }, "print avail list" }, 
  { 'B', print_bucket_handler, { "bucket-number", NULL, }, "print a bucket" },
  { 'C', print_current_bucket_handler, { NULL, NULL, },
    "print current bucket" },
  { 'D', print_dir_handler, { NULL, NULL, }, "print hash directory" },
  { 'F', print_header_handler, { NULL, NULL, }, "print file header" },
  { 'H', hash_handler, { "key", NULL, }, "hash value of key" },
  { 'K', print_cache_handler, { NULL, NULL, }, "print the bucket cache" },
  { 'S', status_handler, { NULL, NULL }, "print current program status" },
  { 'V', print_version_handler, { NULL, NULL, }, "print version of gdbm" },
  { 'Z', data_z_handler, { NULL, NULL }, "toggle data nul-termination" },
  { '?', help_handler, { NULL, NULL, }, "print this help list" },
  { 'q', quit_handler, { NULL, NULL, }, "quit the program" },
  { 0 }
};

#define CMDCOLS 30

void
format_help_out (FILE *out, void *data)
{
  struct command *cmd;
  
  for (cmd = command_tab; cmd->abbrev; cmd++)
    {
      int i;
      int n = fprintf (out, " %c", cmd->abbrev);

      for (i = 0; i < NARGS && cmd->args[i]; i++)
	n += fprintf (out, " %s", cmd->args[i]);

      if (n < CMDCOLS)
	fprintf (out, "%*.s", CMDCOLS-n, "");
      fprintf (out, " %s", cmd->doc);
      fputc ('\n', out);
    }
}  

void
help_handler (char *arg[NARGS])
{
  page_data (sizeof (command_tab) / sizeof (command_tab[0]) + 1,
	     format_help_out, NULL);
}

struct command *
find_command (char *p)
{
  struct command *cmd;
  if (p[1])
    {
      printf ("Multicharacter commands are not yet implemented.\n");
      return NULL;
    }
  for (cmd = command_tab; cmd->abbrev; cmd++)
    if (cmd->abbrev == *p)
      return cmd;
  return NULL;
}

#define SKIPWS(p) while (*(p) && isspace (*(p))) (p)++
#define SKIPWORD(p) while (*(p) && !isspace (*(p))) (p)++

char *
getword (char *s, char **endp)
{
  char *p;
  SKIPWS (s);
  p = s;
  SKIPWORD (s);
  if (*s)
    {
      *s++ = 0;
      SKIPWS (s);
    }
  *endp = s;
  return p;
}

/* The test program allows one to call all the routines plus the hash function.
   The commands are single letter commands.  The user is prompted for all other
   information.  See the help command (?) for a list of all commands. */

int
main (int argc, char *argv[])
{
  char cmd_ch;

  datum key_data;
  datum data_data;
  datum return_data;

  char cmdbuf[1000];

  int cache_size = DEFAULT_CACHESIZE;
  int block_size = 0;
  
  int opt;
  char reader = FALSE;
  char newdb = FALSE;
  int flags = 0;

  progname = strrchr (argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];
  
  /* Argument checking. */
  if (argc == 2)
    {
      if (strcmp (argv[1], "--help") == 0)
	{
	  usage ();
	  exit (0);
	}
      else if (strcmp (argv[1], "--version") == 0)
	{
	  version ();
	  exit (0);
	}
    }
  
  opterr = 0;
  while ((opt = getopt (argc, argv, "lmsrnc:b:g:hv")) != -1)
    switch (opt)
      {
      case 'h':
	usage ();
	exit (0);

      case 'l':
	flags = flags | GDBM_NOLOCK;
	break;

      case 'm':
	flags = flags | GDBM_NOMMAP;
	break;

      case 's':
	if (reader)
	  error (2, "-s is incompatible with -r");

	flags = flags | GDBM_SYNC;
	break;
	
      case 'r':
	if (newdb)
	  error (2, "-r is incompatible with -n");

	reader = TRUE;
	break;
	
      case 'n':
	if (reader)
	  error (2, "-n is incompatible with -r");

	newdb = TRUE;
	break;
	
      case 'c':
	cache_size = atoi (optarg);
	break;
	
      case 'b':
	block_size = atoi (optarg);
	break;
	
      case 'g':
	file_name = optarg;
	break;

      case 'v':
	version ();
	exit (0);
	  
      default:
	error (2, "unknown option; try `%s -h' for more info\n", progname);
      }

  if (file_name == NULL)
    file_name = "junk.gdbm";

  /* Initialize variables. */
  interactive = isatty (0);

  if (reader)
    {
      gdbm_file = gdbm_open (file_name, block_size, GDBM_READER, 00664, NULL);
    }
  else if (newdb)
    {
      gdbm_file =
	gdbm_open (file_name, block_size, GDBM_NEWDB | flags, 00664, NULL);
    }
  else
    {
      gdbm_file =
	gdbm_open (file_name, block_size, GDBM_WRCREAT | flags, 00664, NULL);
    }
  if (gdbm_file == NULL)
    error (2, "gdbm_open failed: %s", gdbm_strerror (gdbm_errno));

  if (gdbm_setopt (gdbm_file, GDBM_CACHESIZE, &cache_size, sizeof (int)) ==
      -1)
    error (2, "gdbm_setopt failed: %s", gdbm_strerror (gdbm_errno));

  signal (SIGPIPE, SIG_IGN);

  /* Welcome message. */
  if (interactive)
    printf ("\nWelcome to the gdbm test program.  Type ? for help.\n\n");

  while (1)
    {
      int i;
      char *p, *sp;
      char argbuf[NARGS][128];
      char *args[NARGS];
      struct command *cmd;

      if (interactive)
	printf ("com -> ");
      
      if (fgets (cmdbuf, sizeof cmdbuf, stdin) == NULL)
	{
	  putchar ('\n');
	  break;
	}

      trimnl (cmdbuf);
      p = getword (cmdbuf, &sp);
      if (!*p)
	continue;
      cmd = find_command (p);
      if (!cmd)
	{
	  printf ("Invalid command. Try ? for help.\n");
	  continue;
	}

      memset (args, 0, sizeof (args));
      for (i = 0; i < NARGS && cmd->args[i]; i++)
	{
	  p = i < NARGS-1 ? getword (sp, &sp) : sp;
	  if (!*p)
	    {
	      char *prompt = cmd->args[i];
	      if (*prompt == '[')
		/* Optional argument */
		break;
	      if (!interactive)
		{
		  printf ("%c: not enough arguments\n", cmd->abbrev);
		  exit (1);
		}
	      
	      printf ("%s -> ", prompt);
	      if (fgets (argbuf[i], sizeof argbuf[i], stdin) == NULL)
		{
		  printf ("unexpected eof\n");
		  exit (0);
		}
	      trimnl (argbuf[i]);
	      args[i] = argbuf[i];
	    }
	  else
	    args[i] = p;
	}	  
      cmd->handler (args);
      printf ("\n");
    }
  
  /* Quit normally. */
  quit_handler (NULL);

}
