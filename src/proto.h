/* proto.h - The prototypes for the dbm routines. */

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

/* From bucket.c */
void _gdbm_new_bucket	(gdbm_file_info *, hash_bucket *, int);
void _gdbm_get_bucket	(gdbm_file_info *, int);
void _gdbm_split_bucket (gdbm_file_info *, int);
void _gdbm_write_bucket (gdbm_file_info *, cache_elem *);

/* From falloc.c */
off_t _gdbm_alloc       (gdbm_file_info *, int);
void _gdbm_free         (gdbm_file_info *, off_t, int);
int  _gdbm_put_av_elem  (avail_elem, avail_elem [], int *, int);

/* From findkey.c */
char *_gdbm_read_entry  (gdbm_file_info *, int);
int _gdbm_findkey       (gdbm_file_info *, datum, char **, int *);

/* From hash.c */
int _gdbm_hash (datum);

/* From update.c */
void _gdbm_end_update   (gdbm_file_info *);
void _gdbm_fatal	(gdbm_file_info *, const char *);

/* From gdbmopen.c */
int _gdbm_init_cache	(gdbm_file_info *, int);

/* From mmap.c */
int _gdbm_mapped_init (gdbm_file_info *dbf);
void _gdbm_mapped_unmap (gdbm_file_info *dbf);
ssize_t _gdbm_mapped_read(gdbm_file_info *dbf, void *buffer, size_t len);
ssize_t _gdbm_mapped_write(gdbm_file_info *dbf, void *buffer, size_t len);
off_t _gdbm_mapped_lseek(gdbm_file_info *dbf, off_t offset, int whence);
int _gdbm_mapped_sync(gdbm_file_info *dbf);

/* The user callable routines.... */
void  gdbm_close	(gdbm_file_info *);
int   gdbm_delete	(gdbm_file_info *, datum);
datum gdbm_fetch	(gdbm_file_info *, datum);
gdbm_file_info *gdbm_open (char *, int, int, int, void (*) (const char *));
int   gdbm_reorganize	(gdbm_file_info *);
datum gdbm_firstkey     (gdbm_file_info *);
datum gdbm_nextkey      (gdbm_file_info *, datum);
int   gdbm_store        (gdbm_file_info *, datum, datum, int);
int   gdbm_exists	(gdbm_file_info *, datum);
void  gdbm_sync		(gdbm_file_info *);
int   gdbm_setopt	(gdbm_file_info *, int, int *, int);
int   gdbm_fdesc	(gdbm_file_info *);
