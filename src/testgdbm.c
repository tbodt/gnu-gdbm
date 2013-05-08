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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"
#include "gdbm.h"
#include "gdbmapp.h"

#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_TERMIOS_H
# include <sys/termios.h>
#endif
#include <stdarg.h>
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

const char *progname;                     /* Program name */

#define DEFAULT_PROMPT "gdbmtool> "
char *prompt;

char *file_name = NULL;             /* Database file name */   
GDBM_FILE gdbm_file = NULL;   /* Database to operate upon */
int interactive;                    /* Are we running in interactive mode? */
datum key_data;                     /* Current key */
datum return_data;                  /* Current data */
int key_z = 1;                      /* Keys are nul-terminated strings */
int data_z = 1;                     /* Data are nul-terminated strings */
int quiet_option = 0;               /* Omit usual welcome banner at startup */

#define SIZE_T_MAX ((size_t)-1)

unsigned input_line;


void
terror (int code, const char *fmt, ...)
{
  va_list ap;
  if (!interactive)
    fprintf (stderr, "%s: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  if (code)
    exit (code);
}

void
syntax_error (const char *fmt, ...)
{
  va_list ap;
  if (!interactive)
    fprintf (stderr, "%s: ", progname);
  fprintf (stderr, "%u: ", input_line);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  if (!interactive)
    exit (EXIT_USAGE);
}
  


size_t
bucket_print_lines (hash_bucket *bucket)
{
  return 6 + gdbm_file->header->bucket_elems + 3 + bucket->av_count;
}

/* Debug procedure to print the contents of the current hash bucket. */
void
print_bucket (FILE *fp, hash_bucket *bucket, const char *mesg)
{
  int             index;

  fprintf (fp,
	   _("******* %s **********\n\nbits = %d\ncount= %d\nHash Table:\n"),
	   mesg, bucket->bucket_bits, bucket->count);
  fprintf (fp,
	   _("     #    hash value     key size    data size     data adr  home\n"));
  for (index = 0; index < gdbm_file->header->bucket_elems; index++)
    fprintf (fp, "  %4d  %12x  %11d  %11d  %11lu %5d\n", index,
	     bucket->h_table[index].hash_value,
	     bucket->h_table[index].key_size,
	     bucket->h_table[index].data_size,
	     (unsigned long) bucket->h_table[index].data_pointer,
	     bucket->h_table[index].hash_value %
	     gdbm_file->header->bucket_elems);

  fprintf (fp, _("\nAvail count = %1d\n"), bucket->av_count);
  fprintf (fp, _("Avail  adr     size\n"));
  for (index = 0; index < bucket->av_count; index++)
    fprintf (fp, "%9lu%9d\n",
	     (unsigned long) bucket->bucket_avail[index].av_adr,
	     bucket->bucket_avail[index].av_size);
}

size_t
_gdbm_avail_list_size (GDBM_FILE dbf, size_t min_size)
{
  int             temp;
  int             size;
  avail_block    *av_stk;
  size_t          lines;
  int             rc;
  
  lines = 4 + dbf->header->avail.count;
  if (lines > min_size)
    return lines;
  /* Initialize the variables for a pass throught the avail stack. */
  temp = dbf->header->avail.next_block;
  size = (((dbf->header->avail.size * sizeof (avail_elem)) >> 1)
	  + sizeof (avail_block));
  av_stk = emalloc (size);

  /* Traverse the stack. */
  while (temp)
    {
      if (__lseek (dbf, temp, SEEK_SET) != temp)
	{
	  terror (0, "lseek: %s", strerror (errno));
	  break;
	}
      
      if ((rc = _gdbm_full_read (dbf, av_stk, size)))
	{
	  if (rc == GDBM_FILE_EOF)
	    terror (0, "read: %s", gdbm_strerror (rc));
	  else
	    terror (0, "read: %s (%s)", gdbm_strerror (rc), strerror (errno));
	  break;
	}

      lines += av_stk->count;
      if (lines > min_size)
	break;
      temp = av_stk->next_block;
    }
  free (av_stk);

  return lines;
}

void
_gdbm_print_avail_list (FILE *fp, GDBM_FILE dbf)
{
  int             temp;
  int             size;
  avail_block    *av_stk;
  int             rc;
  
  /* Print the the header avail block.  */
  fprintf (fp, _("\nheader block\nsize  = %d\ncount = %d\n"),
	   dbf->header->avail.size, dbf->header->avail.count);
  for (temp = 0; temp < dbf->header->avail.count; temp++)
    {
      fprintf (fp, "  %15d   %10lu \n",
	       dbf->header->avail.av_table[temp].av_size,
	       (unsigned long) dbf->header->avail.av_table[temp].av_adr);
    }

  /* Initialize the variables for a pass throught the avail stack. */
  temp = dbf->header->avail.next_block;
  size = (((dbf->header->avail.size * sizeof (avail_elem)) >> 1)
	  + sizeof (avail_block));
  av_stk = emalloc (size);

  /* Print the stack. */
  while (temp)
    {
      if (__lseek (dbf, temp, SEEK_SET) != temp)
	{
	  terror (0, "lseek: %s", strerror (errno));
	  break;
	}
      
      if ((rc = _gdbm_full_read (dbf, av_stk, size)))
	{
	  if (rc == GDBM_FILE_EOF)
	    terror (0, "read: %s", gdbm_strerror (rc));
	  else
	    terror (0, "read: %s (%s)", gdbm_strerror (rc), strerror (errno));
	  break;
	}

      /* Print the block! */
      fprintf (fp, _("\nblock = %d\nsize  = %d\ncount = %d\n"), temp,
	       av_stk->size, av_stk->count);
      for (temp = 0; temp < av_stk->count; temp++)
	{
	  fprintf (fp, "  %15d   %10lu \n", av_stk->av_table[temp].av_size,
		   (unsigned long) av_stk->av_table[temp].av_adr);
	}
      temp = av_stk->next_block;
    }
  free (av_stk);
}

void
_gdbm_print_bucket_cache (FILE *fp, GDBM_FILE dbf)
{
  int             index;
  char            changed;

  if (dbf->bucket_cache != NULL)
    {
      fprintf (fp,
	_("Bucket Cache (size %d):\n  Index:  Address  Changed  Data_Hash \n"),
	 dbf->cache_size);
      for (index = 0; index < dbf->cache_size; index++)
	{
	  changed = dbf->bucket_cache[index].ca_changed;
	  fprintf (fp, "  %5d:  %7lu %7s  %x\n",
		   index,
		   (unsigned long) dbf->bucket_cache[index].ca_adr,
		   (changed ? _("True") : _("False")),
		   dbf->bucket_cache[index].ca_data.hash_val);
	}
    }
  else
    fprintf (fp, _("Bucket cache has not been initialized.\n"));
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
      terror (0, _("cannot open file `%s' for reading: %s"),
	     name, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof buf, fp))
    {
      char *p;

      if (!trimnl (buf))
	{
	  terror (0, _("%s:%d: line too long"), name, line);
	  continue;
	}

      line++;
      p = strchr (buf, ' ');
      if (!p)
	{
	  terror (0, _("%s:%d: malformed line"), name, line);
	  continue;
	}

      for (*p++ = 0; *p && isspace (*p); p++)
	;
      key.dptr = buf;
      key.dsize = strlen (buf) + key_z;
      data.dptr = p;
      data.dsize = strlen (p) + data_z;
      if (gdbm_store (gdbm_file, key, data, flag) != 0)
	terror (0, _("%d: item not inserted: %s"),
	       line, gdbm_strerror (gdbm_errno));
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

#define NARGS 5

struct handler_param
{
  int argc;
  char **argv;
  FILE *fp;
  void *data;
};
	     

/* c - count */
void
count_handler (struct handler_param *param)
{
  int count = get_record_count ();
  fprintf (param->fp, ngettext ("There is %d item in the database.\n",
				"There are %d items in the database.\n", count),
	   count);
}

/* d key - delete */
void
delete_handler (struct handler_param *param)
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data.dptr = strdup (param->argv[0]);
  key_data.dsize = strlen (param->argv[0]) + key_z;
  if (gdbm_delete (gdbm_file, key_data) != 0)
    {
      if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	terror (0, _("Item not found"));
      else
	terror (0, _("Can't delete: %s"),  gdbm_strerror (gdbm_errno));
    }
}

/* f key - fetch */
void
fetch_handler (struct handler_param *param)
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data.dptr = strdup (param->argv[0]);
  key_data.dsize = strlen (param->argv[0]) + key_z;
  return_data = gdbm_fetch (gdbm_file, key_data);
  if (return_data.dptr != NULL)
    {
      fprintf (param->fp, "%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    fprintf (stderr, _("No such item found.\n"));
}

/* s key data - store */
void
store_handler (struct handler_param *param)
{
  datum key;
  datum data;
  
  key.dptr = param->argv[0];
  key.dsize = strlen (param->argv[0]) + key_z;
  data.dptr = param->argv[1];
  data.dsize = strlen (param->argv[1]) + data_z;
  if (gdbm_store (gdbm_file, key, data, GDBM_REPLACE) != 0)
    fprintf (stderr, _("Item not inserted.\n"));
}

/* 1 - begin iteration */

void
firstkey_handler (struct handler_param *param)
{
  if (key_data.dptr != NULL)
    free (key_data.dptr);
  key_data = gdbm_firstkey (gdbm_file);
  if (key_data.dptr != NULL)
    {
      fprintf (param->fp, "%.*s\n", key_data.dsize, key_data.dptr);
      return_data = gdbm_fetch (gdbm_file, key_data);
      fprintf (param->fp, "%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    fprintf (param->fp, _("No such item found.\n"));
}

/* n [key] - next key */
void
nextkey_handler (struct handler_param *param)
{
  if (param->argv[0])
    {
      if (key_data.dptr != NULL)
	free (key_data.dptr);
      key_data.dptr = strdup (param->argv[0]);
      key_data.dsize = strlen (param->argv[0]) + key_z;
    }
  return_data = gdbm_nextkey (gdbm_file, key_data);
  if (return_data.dptr != NULL)
    {
      key_data = return_data;
      fprintf (param->fp, "%.*s\n", key_data.dsize, key_data.dptr);
      return_data = gdbm_fetch (gdbm_file, key_data);
      fprintf (param->fp, "%.*s\n", return_data.dsize, return_data.dptr);
      free (return_data.dptr);
    }
  else
    {
      fprintf (stderr, _("No such item found.\n"));
      free (key_data.dptr);
      key_data.dptr = NULL;
    }
}

/* r - reorganize */
void
reorganize_handler (struct handler_param *param ARG_UNUSED)
{
  if (gdbm_reorganize (gdbm_file))
    fprintf (stderr, _("Reorganization failed.\n"));
  else
    fprintf (stderr, _("Reorganization succeeded.\n"));
}

/* A - print available list */
int
avail_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{ 
  if (exp_count)
    *exp_count = _gdbm_avail_list_size (gdbm_file, SIZE_T_MAX);
  return 0;
}

void
avail_handler (struct handler_param *param)
{
  _gdbm_print_avail_list (param->fp, gdbm_file);
}

/* C - print current bucket */
int
print_current_bucket_begin (struct handler_param *param ARG_UNUSED,
			    size_t *exp_count)
{
  if (exp_count)
    *exp_count = bucket_print_lines (gdbm_file->bucket) + 3;
  return 0;
}

void
print_current_bucket_handler (struct handler_param *param)
{
  print_bucket (param->fp, gdbm_file->bucket, _("Current bucket"));
  fprintf (param->fp, _("\n current directory entry = %d.\n"),
	   gdbm_file->bucket_dir);
  fprintf (param->fp, _(" current bucket address  = %lu.\n"),
	   (unsigned long) gdbm_file->cache_entry->ca_adr);
}

int
getnum (int *pnum, char *arg, char **endp)
{
  char *p;
  unsigned long x = strtoul (arg, &p, 10);
  if (*p && !isspace (*p))
    {
      printf (_("not a number (stopped near %s)\n"), p);
      return 1;
    }
  while (*p && isspace (*p))
    p++;
  if (endp)
    *endp = p;
  else if (*p)
    {
      printf (_("not a number (stopped near %s)\n"), p);
      return 1;
    }
  *pnum = x;
  return 0;
}
  
/* B num - print a bucket and set is a current one.
   Uses print_current_bucket_handler */
int
print_bucket_begin (struct handler_param *param, size_t *exp_count)
{
  int temp;

  if (getnum (&temp, param->argv[0], NULL))
    return 1;

  if (temp >= gdbm_file->header->dir_size / 4)
    {
      fprintf (stderr, _("Not a bucket.\n"));
      return 1;
    }
  _gdbm_get_bucket (gdbm_file, temp);
  if (exp_count)
    *exp_count = bucket_print_lines (gdbm_file->bucket) + 3;
  return 0;
}


/* D - print hash directory */
int
print_dir_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{
  if (exp_count)
    *exp_count = gdbm_file->header->dir_size / 4 + 3;
  return 0;
}

void
print_dir_handler (struct handler_param *param)
{
  int i;

  fprintf (param->fp, _("Hash table directory.\n"));
  fprintf (param->fp, _("  Size =  %d.  Bits = %d. \n\n"),
	   gdbm_file->header->dir_size, gdbm_file->header->dir_bits);
  
  for (i = 0; i < gdbm_file->header->dir_size / 4; i++)
    fprintf (param->fp, "  %10d:  %12lu\n",
	     i, (unsigned long) gdbm_file->dir[i]);
}

/* F - print file handler */
int
print_header_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{
  if (exp_count)
    *exp_count = 14;
  return 0;
}

void
print_header_handler (struct handler_param *param)
{
  FILE *fp = param->fp;
  
  fprintf (fp, _("\nFile Header: \n\n"));
  fprintf (fp, _("  table        = %lu\n"),
	   (unsigned long) gdbm_file->header->dir);
  fprintf (fp, _("  table size   = %d\n"), gdbm_file->header->dir_size);
  fprintf (fp, _("  table bits   = %d\n"), gdbm_file->header->dir_bits);
  fprintf (fp, _("  block size   = %d\n"), gdbm_file->header->block_size);
  fprintf (fp, _("  bucket elems = %d\n"), gdbm_file->header->bucket_elems);
  fprintf (fp, _("  bucket size  = %d\n"), gdbm_file->header->bucket_size);
  fprintf (fp, _("  header magic = %x\n"), gdbm_file->header->header_magic);
  fprintf (fp, _("  next block   = %lu\n"),
	   (unsigned long) gdbm_file->header->next_block);
  fprintf (fp, _("  avail size   = %d\n"), gdbm_file->header->avail.size);
  fprintf (fp, _("  avail count  = %d\n"), gdbm_file->header->avail.count);
  fprintf (fp, _("  avail nx blk = %lu\n"),
	   (unsigned long) gdbm_file->header->avail.next_block);
}  

/* H key - hash the key */
void
hash_handler (struct handler_param *param)
{
  datum key;
  
  key.dptr = param->argv[0];
  key.dsize = strlen (param->argv[0]) + key_z;
  fprintf (param->fp, _("hash value = %x. \n"), _gdbm_hash (key));
}

/* K - print the bucket cache */
int
print_cache_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{
  if (exp_count)
    *exp_count = gdbm_file->bucket_cache ? gdbm_file->cache_size + 1 : 1;
  return 0;
}
    
void
print_cache_handler (struct handler_param *param)
{
  _gdbm_print_bucket_cache (param->fp, gdbm_file);
}

/* V - print GDBM version */
void
print_version_handler (struct handler_param *param)
{
  fprintf (param->fp, "%s\n", gdbm_version);
}

/* < file [replace] - read entries from file and store */
void
read_handler (struct handler_param *param)
{
  read_from_file (param->argv[0],
		  param->argv[1] && strcmp (param->argv[1], "replace") == 0);
}

/* l - List all entries */
int
list_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{
  if (exp_count)
    *exp_count = get_record_count ();
  return 0;
}

void
list_handler (struct handler_param *param)
{
  datum key;
  datum data;

  key = gdbm_firstkey (gdbm_file);
  while (key.dptr)
    {
      datum nextkey = gdbm_nextkey (gdbm_file, key);

      data = gdbm_fetch (gdbm_file, key);
      if (!data.dptr)
	terror (0, _("cannot fetch data (key %.*s)"), key.dsize, key.dptr);
      else
	{
	  fprintf (param->fp, "%.*s %.*s\n", key.dsize, key.dptr, data.dsize,
		   data.dptr);
	  free (data.dptr);
	}
      free (key.dptr);
      key = nextkey;
    }
}

/* q - quit the program */
void
quit_handler (struct handler_param *param ARG_UNUSED)
{
  if (gdbm_file != NULL)
    gdbm_close (gdbm_file);

  exit (EXIT_OK);
}

/* e file [truncate] - export to a flat file format */
void
export_handler (struct handler_param *param)
{
  int format = GDBM_DUMP_FMT_ASCII;
  int flags = GDBM_WRCREAT;
  int i;
  
  for (i = 1; i < param->argc; i++)
    {
      if (strcmp (param->argv[i], "truncate") == 0)
	flags = GDBM_NEWDB;
      else if (strcmp (param->argv[i], "binary") == 0)
	format = GDBM_DUMP_FMT_BINARY;
      else if (strcmp (param->argv[i], "ascii") == 0)
	format = GDBM_DUMP_FMT_ASCII;
      else
	{
	  syntax_error (_("unrecognized argument: %s"), param->argv[i]);
	  return;
	}
    }

  if (gdbm_dump (gdbm_file, param->argv[0], format, flags, 0600))
    {
      terror (0, _("error dumping database: %s"), gdbm_strerror (gdbm_errno));
    }
}

/* i file [replace] - import from a flat file */
void
import_handler (struct handler_param *param)
{
  int flag = GDBM_INSERT;
  unsigned long err_line;
  int meta_mask = 0;
  int i;
  
  for (i = 1; i < param->argc; i++)
    {
      if (strcmp (param->argv[i], "replace") == 0)
	flag = GDBM_REPLACE;
      else if (strcmp (param->argv[i], "nometa") == 0)
	meta_mask = GDBM_META_MASK_MODE | GDBM_META_MASK_OWNER;
      else
	{
	  syntax_error (_("unrecognized argument: %s"), param->argv[i]);
	  return;
	}
    }

  if (gdbm_load (&gdbm_file, param->argv[0], flag, meta_mask, &err_line))
    {
      switch (gdbm_errno)
	{
	case GDBM_ERR_FILE_OWNER:
	case GDBM_ERR_FILE_MODE:
	  terror (0, _("error restoring metadata: %s (%s)"),
		  gdbm_strerror (gdbm_errno), strerror (errno));
	  break;
	  
	default:
	  if (err_line)
	    terror (0, "%s:%lu: %s", param->argv[0], err_line,
		    gdbm_strerror (gdbm_errno));
	  else
	    terror (0, _("cannot load from %s: %s"), param->argv[0],
		    gdbm_strerror (gdbm_errno));
	}
    }
}

static const char *
boolstr (int val)
{
  return val ? _("yes") : _("no");
}

/* S - print current program status */
void
status_handler (struct handler_param *param)
{
  fprintf (param->fp, _("Database file: %s\n"), file_name);
  fprintf (param->fp, _("Zero terminated keys: %s\n"), boolstr (key_z));
  fprintf (param->fp, _("Zero terminated data: %s\n"), boolstr (data_z));
}

/* z - toggle key nul-termination */
void
key_z_handler (struct handler_param *param)
{
  key_z = !key_z;
  fprintf (param->fp, _("Zero terminated keys: %s\n"), boolstr (key_z));
}

/* Z - toggle data nul-termination */
void
data_z_handler (struct handler_param *param)
{
  data_z = !data_z;
  fprintf (param->fp, _("Zero terminated data: %s\n"), boolstr (data_z));
}

struct prompt_exp;

void
pe_file_name (struct prompt_exp *p)
{
  fwrite (file_name, strlen (file_name), 1, stdout);
}

struct prompt_exp
{
  int ch;
  void (*fun) (struct prompt_exp *);
  char *cache;
};

struct prompt_exp prompt_exp[] = {
  { 'f', pe_file_name },
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

void
outprompt ()
{
  char *s;
  
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

void
prompt_handler (struct handler_param *param)
{
  free (prompt);
  prompt = estrdup (param->argv[0]);
}

void help_handler (struct handler_param *param);
int help_begin (struct handler_param *param, size_t *exp_count);

struct command
{
  char *name;           /* Command name */
  size_t len;           /* Name length */
  int abbrev;           /* Single-letter shortkey (optional) */
  int  (*begin) (struct handler_param *param, size_t *);
  void (*handler) (struct handler_param *param);
  void (*end) (void *data);
  char *args[NARGS];
  char *doc;
};


struct command command_tab[] = {
#define S(s) #s, sizeof (#s) - 1
  { S(count), 'c',
    NULL, count_handler, NULL,
    { NULL, NULL, }, N_("count (number of entries)") },
  { S(delete), 'd',
    NULL, delete_handler, NULL,
    { N_("key"), NULL, }, N_("delete") },
  { S(export), 'e',
    NULL, export_handler, NULL,
    { N_("file"), "[truncate]", "[binary|ascii]" }, N_("export") },
  { S(fetch), 'f',
    NULL, fetch_handler, NULL,
    { N_("key"), NULL }, N_("fetch") },
  { S(import), 'i',
    NULL, import_handler, NULL,
    { N_("file"), "[replace]", "[nometa]" }, N_("import") },
  { S(list), 'l',
    list_begin, list_handler, NULL,
    { NULL, NULL }, N_("list") },
  { S(next), 'n',
    NULL, nextkey_handler, NULL,
    { N_("[key]"), NULL }, N_("nextkey") },
  { S(store), 's',
    NULL, store_handler, NULL,
    { N_("key"), N_("data") }, N_("store") },
  { S(first), '1',
    NULL, firstkey_handler, NULL,
    { NULL, NULL }, N_("firstkey") },
  { S(read), '<',
    NULL, read_handler, NULL,
    { N_("file"), "[replace]" },
    N_("read entries from file and store") },
  { S(reorganize), 'r',
    NULL, reorganize_handler, NULL,
    { NULL, NULL, }, N_("reorganize") },
  { S(key-zero), 'z',
    NULL, key_z_handler, NULL,
    { NULL, NULL }, N_("toggle key nul-termination") },
  { S(avail), 'A',
    avail_begin, avail_handler, NULL,
    { NULL, NULL, }, N_("print avail list") }, 
  { S(bucket), 'B',
    print_bucket_begin, print_current_bucket_handler, NULL,
    { N_("bucket-number"), NULL, }, N_("print a bucket") },
  { S(current), 'C',
    print_current_bucket_begin, print_current_bucket_handler, NULL,
    { NULL, NULL, },
    N_("print current bucket") },
  { S(dir), 'D',
    print_dir_begin, print_dir_handler, NULL,
    { NULL, NULL, }, N_("print hash directory") },
  { S(header), 'F',
    print_header_begin , print_header_handler, NULL,
    { NULL, NULL, }, N_("print file header") },
  { S(hash), 'H',
    NULL, hash_handler, NULL,
    { N_("key"), NULL, }, N_("hash value of key") },
  { S(cache), 'K',
    print_cache_begin, print_cache_handler, NULL,
    { NULL, NULL, }, N_("print the bucket cache") },
  { S(status), 'S',
    NULL, status_handler, NULL,
    { NULL, NULL }, N_("print current program status") },
  { S(version), 'v',
    NULL, print_version_handler, NULL,
    { NULL, NULL, }, N_("print version of gdbm") },
  { S(data-zero), 'Z',
    NULL, data_z_handler, NULL,
    { NULL, NULL }, N_("toggle data nul-termination") },
  { S(help), '?',
    help_begin, help_handler, NULL,
    { NULL, NULL, }, N_("print this help list") },
  { S(prompt), 0,
    NULL, prompt_handler, NULL,
    { N_("text") }, N_("set command prompt") },
  { S(quit), 'q',
    NULL, quit_handler, NULL,
    { NULL, NULL, }, N_("quit the program") },
#undef S
  { 0 }
};

static int
cmdcmp (const void *a, const void *b)
{
  struct command const *ac = a;
  struct command const *bc = b;
  return strcmp (ac->name, bc->name);
}

void
sort_commands ()
{
  qsort (command_tab, sizeof (command_tab) / sizeof (command_tab[0]) - 1,
	 sizeof (command_tab[0]), cmdcmp);
}


/* ? - help handler */
#define CMDCOLS 30

int
help_begin (struct handler_param *param ARG_UNUSED, size_t *exp_count)
{
  if (exp_count)
    *exp_count = sizeof (command_tab) / sizeof (command_tab[0]) + 1;
  return 0;
}

void
help_handler (struct handler_param *param)
{
  struct command *cmd;
  FILE *fp = param->fp;
  
  for (cmd = command_tab; cmd->name; cmd++)
    {
      int i;
      int n;

      if (cmd->abbrev)
	n = fprintf (fp, " %c, ", cmd->abbrev);
      else
	n = fprintf (fp, " ");

      n += fprintf (fp, "%s", cmd->name);

      for (i = 0; i < NARGS && cmd->args[i]; i++)
	n += fprintf (fp, " %s", gettext (cmd->args[i]));

      if (n < CMDCOLS)
	fprintf (fp, "%*.s", CMDCOLS-n, "");
      fprintf (fp, " %s", gettext (cmd->doc));
      fputc ('\n', fp);
    }
}

struct command *
find_command (char *str)
{
  enum { fcom_init, fcom_found, fcom_ambig, fcom_abort } state = fcom_init;
  struct command *cmd, *found = NULL;
  size_t len = strlen (str);
  
  if (len == 1)
    {
      for (cmd = command_tab; cmd->name; cmd++)
	if (cmd->abbrev == *str)
	  return cmd;
    }
  
  for (cmd = command_tab; state != fcom_abort && cmd->name; cmd++)
    {
      if (memcmp (cmd->name, str, len < cmd->len ? len : cmd->len) == 0)
	{
	  switch (state)
	    {
	    case fcom_init:
	      found = cmd;
	      state = fcom_found;
	      break;

	    case fcom_found:
	      if (!interactive)
		{
		  state = fcom_abort;
		  found = NULL;
		  continue;
		}
	      fprintf (stderr, "ambiguous command: %s\n", str);
	      fprintf (stderr, "    %s\n", found->name);
	      found = NULL;
	      state = fcom_ambig;
	      /* fall through */
	    case fcom_ambig:
	      fprintf (stderr, "    %s\n", cmd->name);
	    }
	}
    }

  if (state == fcom_init)
    terror (0, interactive ? _("Invalid command. Try ? for help.") :
		             _("Unknown command"));
  return found;
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

char *parseopt_program_doc = N_("examine and/or modify a GDBM database");
char *parseopt_program_args = N_("DBFILE");

struct gdbm_option optab[] = {
  { 'b', "block-size", N_("SIZE"), N_("set block size") },
  { 'c', "cache-size", N_("SIZE"), N_("set cache size") },
  { 'g', NULL, "FILE", NULL, PARSEOPT_HIDDEN },
  { 'l', "no-lock",    NULL,       N_("disable file locking") },
  { 'm', "no-mmap",    NULL,       N_("do not use mmap") },
  { 'n', "newdb",      NULL,       N_("create database") },
  { 'r', "read-only",  NULL,       N_("open database in read-only mode") },
  { 's', "synchronize", NULL,      N_("synchronize to disk after each write") },
  { 'q', "quiet",      NULL,       N_("don't print initial banner") },
  { 0 }
};

#define ARGINC 16

int
main (int argc, char *argv[])
{
  char cmdbuf[1000];

  int cache_size = DEFAULT_CACHESIZE;
  int block_size = 0;
  
  int opt;
  char reader = FALSE;
  char newdb = FALSE;
  int flags = 0;
  char *pager = getenv ("PAGER");

  struct handler_param param;
  size_t argmax;
  
  set_progname (argv[0]);

#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  sort_commands ();
  
  for (opt = parseopt_first (argc, argv, optab);
       opt != EOF;
       opt = parseopt_next ())
    switch (opt)
      {
      case 'l':
	flags = flags | GDBM_NOLOCK;
	break;

      case 'm':
	flags = flags | GDBM_NOMMAP;
	break;

      case 's':
	if (reader)
	  terror (EXIT_USAGE, _("-s is incompatible with -r"));

	flags = flags | GDBM_SYNC;
	break;
	
      case 'r':
	if (newdb)
	  terror (EXIT_USAGE, _("-r is incompatible with -n"));

	reader = TRUE;
	break;
	
      case 'n':
	if (reader)
	  terror (EXIT_USAGE, _("-n is incompatible with -r"));

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

      case 'q':
	quiet_option = 1;
	break;
	
      default:
	terror (EXIT_USAGE,
		_("unknown option; try `%s -h' for more info\n"), progname);
      }

  argc -= optind;
  argv += optind;

  if (argc > 1)
    terror (EXIT_USAGE, _("too many arguments"));
  if (argc == 1)
    file_name = argv[0];
  else
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
    terror (EXIT_FATAL, _("gdbm_open failed: %s"), gdbm_strerror (gdbm_errno));

  if (gdbm_setopt (gdbm_file, GDBM_CACHESIZE, &cache_size, sizeof (int)) ==
      -1)
    terror (EXIT_FATAL, _("gdbm_setopt failed: %s"),
	    gdbm_strerror (gdbm_errno));

  signal (SIGPIPE, SIG_IGN);

  /* Welcome message. */
  if (interactive && !quiet_option)
    printf (_("\nWelcome to the gdbm tool.  Type ? for help.\n\n"));
  if (interactive)
    prompt = estrdup (DEFAULT_PROMPT);
		      
  memset (&param, 0, sizeof (param));
  argmax = 0;

  while (1)
    {
      int i;
      char *p, *sp;
      char argbuf[128];
      struct command *cmd;
      size_t expected_lines, *expected_lines_ptr;
      FILE *pagfp = NULL;
      
      for (i = 0; i < param.argc; i++)
	free (param.argv[i]);
      param.argc = 0;
      
      input_line++;
      
      if (interactive)
	outprompt ();
      
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
	continue;

      for (i = 0; cmd->args[i]; i++)
	{
	  char *arg = cmd->args[i];
	  
	  p = getword (sp, &sp);
	  if (!*p)
	    {
	      if (*arg == '[')
		/* Optional argument */
		break;
	      if (!interactive)
		syntax_error (_("%s: not enough arguments"), cmd->name);
	      printf ("%s? ", arg);
	      if (fgets (argbuf, sizeof argbuf, stdin) == NULL)
		terror (EXIT_USAGE, _("unexpected eof"));

	      trimnl (argbuf);
	      p = argbuf;
	    }
	  
	  if (i >= argmax)
	    {
	      argmax += ARGINC;
	      param.argv = erealloc (param.argv,
				     sizeof (param.argv[0]) * argmax);
	    }
	  param.argv[i] = estrdup (p);
	}

      if (*sp)
	{
	  syntax_error (_("%s: too many arguments"), cmd->name);
	  continue;
	}
      
      /* Prepare for calling the handler */
      param.argc = i;
      param.fp = NULL;
      param.data = NULL;
      pagfp = NULL;
      
      expected_lines = 0;
      expected_lines_ptr = (interactive && pager) ? &expected_lines : NULL;
      if (cmd->begin && cmd->begin (&param, expected_lines_ptr))
	continue;
      if (pager && expected_lines > get_screen_lines ())
	{
	  pagfp = popen (pager, "w");
	  if (pagfp)
	    param.fp = pagfp;
	  else
	    {
	      terror (0, _("cannot run pager `%s': %s"), pager,
		      strerror (errno));
	      pager = NULL;
	      param.fp = stdout;
	    }	  
	}
      else
	param.fp = stdout;

      cmd->handler (&param);
      if (cmd->end)
	cmd->end (param.data);
      else if (param.data)
	free (param.data);

      if (pagfp)
	pclose (pagfp);

    }
  
  /* Quit normally. */
  quit_handler (NULL);
  return 0;
}
