/* Copyright (C) 1985, 1986, 1987 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#ifdef hpux
/* needed by <pwd.h> */
#include <stdio.h>
#undef NULL
#endif
#include "lisp.h"
#include "paths.h"
#include "buffer.h"

#ifdef VMS
#include "vms-pwd.h"
#else
#include <pwd.h>
#endif

#include <errno.h>
#include <sys/file.h>
#ifdef USG
#include <fcntl.h>
#endif /* USG */

#include "proto.h"

#include "filetypes.h"

extern int errno;

#ifdef VMS
/* Prevent the file from being totally empty.  */
static dummy () {}
#endif

#ifdef CLASH_DETECTION
  
/* If system does not have symbolic links, it does not have lstat.
   In that case, use ordinary stat instead.  */

#ifndef S_IFLNK
#define lstat stat
#endif

#ifdef _NO_EMACS_PROTO
static Lisp_Object
lock_file_owner_name (lfname)
     char *lfname;
#else /* _NO_EMACS_PROTO */
static Lisp_Object
lock_file_owner_name (char * lfname)
#endif /* _NO_EMACS_PROTO */
{
  struct stat s;
  struct passwd *the_pw;
  extern struct passwd *getpwuid ();

  if (lstat (lfname, &s) == 0)
    the_pw = getpwuid (s.st_uid);
  return (the_pw == 0 ? Qnil : build_string (the_pw->pw_name));
}


/* lock_file locks file fn,
   meaning it serves notice on the world that you intend to edit that file.
   This should be done only when about to modify a file-visiting
   buffer previously unmodified.
   Do not (normally) call lock_buffer for a buffer already modified,
   as either the file is already locked, or the user has already
   decided to go ahead without locking.

   When lock_buffer returns, either the lock is locked for us,
   or the user has said to go ahead without locking.

   If the file is locked by someone else, lock_buffer calls
   ask-user-about-lock (a Lisp function) with two arguments,
   the file name and the name of the user who did the locking.
   This function can signal an error, or return t meaning
   take away the lock, or return nil meaning ignore the lock.  */

/* The lock file name is the file name with "/" replaced by "!"
   and put in the Emacs lock directory.  */
/* (ie., /ka/king/junk.tex -> /!/!ka!king!junk.tex). */

/* If SHORT_FILE_NAMES is defined, the lock file name is the hex
   representation of a 14-bytes CRC generated from the file name
   and put in the Emacs lock directory (not very nice, but it works).
   (ie., /ka/king/junk.tex -> /!/ec92d3ed24a8f0). */

#ifdef SHORT_FILE_NAMES

# define CREATE_LOCK_FILE_NAME(lfname, fn) \
  { lfname = (char *) alloca (14 + strlen (PATH_LOCK) + 1); \
  fill_in_lock_short_file_name (lfname, fn); }

#else

# define CREATE_LOCK_FILE_NAME(lfname, fn) \
  { lfname = (char *) alloca (XSTRING (fn)->size + strlen (PATH_LOCK) + 1); \
  fill_in_lock_file_name (lfname, fn); }

#endif
#ifdef _NO_EMACS_PROTO
void
lock_file (fn)
     register Lisp_Object fn;
#else /* _NO_EMACS_PROTO */
void
lock_file (register Lisp_Object fn)
#endif /* _NO_EMACS_PROTO */
{
  register Lisp_Object attack;
  register char *lfname;

  /* Create the name of the lock-file for file fn */
  CREATE_LOCK_FILE_NAME (lfname, fn);

  /* See if this file is visited and has changed on disk since it was visited.  */
  {
    register Lisp_Object subject_buf = Fget_file_buffer (fn);
    if (!NULL (subject_buf)
	&& NULL (Fverify_visited_file_modtime (subject_buf))
	&& !NULL (Ffile_exists_p (fn)))
      call1 (intern ("ask-user-about-supersession-threat"), fn);
  }

  /* Try to lock the lock. */
  if (lock_if_free (lfname) <= 0)
    /* Return now if we have locked it, or if lock dir does not exist */
    return;

  /* Else consider breaking the lock */
  attack = call2 (intern ("ask-user-about-lock"), fn,
		  lock_file_owner_name (lfname));
  if (!NULL (attack))
    /* User says take the lock */
    {
      lock_superlock (lfname);
      lock_file_1 (lfname, O_WRONLY) ;
      unlink (PATH_SUPERLOCK);
      return;
    }
  /* User says ignore the lock */
}

#ifdef _NO_EMACS_PROTO
fill_in_lock_file_name (lockfile, fn)
     register char *lockfile;
     register Lisp_Object fn;
#else /* _NO_EMACS_PROTO */
void
fill_in_lock_file_name (register char *lockfile, register Lisp_Object fn)
#endif /* _NO_EMACS_PROTO */
{
  register char *p;

  strcpy (lockfile, PATH_LOCK);

  p = lockfile + strlen (lockfile);

  strcpy (p, XSTRING (fn)->data);

  for (; *p; p++)
    {
      if (*p == '/')
	*p = '!';
    }
}

#ifdef SHORT_FILE_NAMES
fill_in_lock_short_file_name (lockfile, fn)
     register char *lockfile;
     register Lisp_Object fn;
{
  register union
    {
      unsigned int  word [2];
      unsigned char byte [8];
    } crc;
  register unsigned char *p, new;
  
  /* 7-bytes cyclic code for burst correction on byte-by-byte basis.
     the used polynomial is D^7 + D^6 + D^3 +1. pot@fdt.CNUCE.CNR.IT */

  crc.word[0] = crc.word[1] = 0;

  for (p = XSTRING (fn)->data; new = *p++; )
    {
      new += crc.byte[7];
      crc.byte[7] = crc.byte[6];
      crc.byte[6] = crc.byte[5] + new;
      crc.byte[5] = crc.byte[4];
      crc.byte[4] = crc.byte[3];
      crc.byte[3] = crc.byte[2] + new;
      crc.byte[2] = crc.byte[1];
      crc.byte[1] = crc.byte[0];
      crc.byte[0] = new;
    }
  sprintf (lockfile, "%s%.2x%.2x%.2x%.2x%.2x%.2x%.2x", PATH_LOCK,
	   crc.byte[0], crc.byte[1], crc.byte[2], crc.byte[3],
	   crc.byte[4], crc.byte[5], crc.byte[6]);
}
#endif /* SHORT_FILE_NAMES */

/* Lock the lock file named LFNAME.
   If MODE is O_WRONLY, we do so even if it is already locked.
   If MODE is O_WRONLY | O_EXCL | O_CREAT, we do so only if it is free.
   Return 1 if successful, 0 if not.  */

#ifdef _NO_EMACS_PROTO
int
lock_file_1 (lfname, mode)
     int mode; char *lfname; 
#else /* _NO_EMACS_PROTO */
int
lock_file_1 (char * lfname, int mode)
#endif /* _NO_EMACS_PROTO */
{
  register int fd;
  char buf[20];

  if ((fd = open (lfname, mode, 0666)) >= 0)
    {
#ifdef NO_FCHMOD
      chmod (lfname, 0666);
#else
      fchmod (fd, 0666);
#endif
      sprintf (buf, "%d ", getpid ());
      write (fd, buf, strlen (buf));
      close (fd);
      return 1;
    }
  else
    return 0;
}

/* Lock the lock named LFNAME if possible.
   Return 0 in that case.
   Return positive if lock is really locked by someone else.
   Return -1 if cannot lock for any other reason.  */

#ifdef _NO_EMACS_PROTO
int
lock_if_free (lfname)
     register char *lfname; 
#else /* _NO_EMACS_PROTO */
int
lock_if_free (register char *lfname)
#endif /* _NO_EMACS_PROTO */
{
  register int clasher;

  while (lock_file_1 (lfname, O_WRONLY | O_EXCL | O_CREAT) == 0)
    {
      if (errno != EEXIST)
	return -1;
      clasher = current_lock_owner (lfname);
      if (clasher != 0)
	if (clasher != getpid ())
	  return (clasher);
	else return (0);
      /* Try again to lock it */
    }
  return 0;
}

/* Return the pid of the process that claims to own the lock file LFNAME,
   or 0 if nobody does or the lock is obsolete,
   or -1 if something is wrong with the locking mechanism.  */

#ifdef _NO_EMACS_PROTO
int
current_lock_owner (lfname)
     char *lfname;
#else /* _NO_EMACS_PROTO */
int
current_lock_owner ( char *lfname)
#endif /* _NO_EMACS_PROTO */
{
  int owner = current_lock_owner_1 (lfname);
  if (owner == 0 && errno == ENOENT)
    return (0);
  /* Is it locked by a process that exists?  */
  if (owner != 0 && (kill (owner, 0) >= 0 || errno == EPERM))
    return (owner);
  if (unlink (lfname) < 0)
    return (-1);
  return (0);
}

#ifdef _NO_EMACS_PROTO
int
current_lock_owner_1 (lfname)
     char *lfname;
#else /* _NO_EMACS_PROTO */
int
current_lock_owner_1 (char *lfname)
#endif /* _NO_EMACS_PROTO */
{
  register int fd;
  char buf[20];
  int tem;

  fd = open (lfname, O_RDONLY, 0666);
  if (fd < 0)
    return 0;
  tem = read (fd, buf, sizeof buf);
  close (fd);
  return (tem <= 0 ? 0 : atoi (buf));
}


#ifdef _NO_EMACS_PROTO
void
unlock_file (fn)
     register Lisp_Object fn;
#else /* _NO_EMACS_PROTO */
void
unlock_file (register Lisp_Object fn)
#endif /* _NO_EMACS_PROTO */
{
  register char *lfname;

  CREATE_LOCK_FILE_NAME (lfname, fn);

  lock_superlock (lfname);

  if (current_lock_owner_1 (lfname) == getpid ())
    unlink (lfname);

  unlink (PATH_SUPERLOCK);
}

#ifdef _NO_EMACS_PROTO
lock_superlock (lfname)
     char *lfname;
#else /* _NO_EMACS_PROTO */
void
lock_superlock (char *lfname)
#endif /* _NO_EMACS_PROTO */
{
  register int i, fd;

  for (i = -20; i < 0 && (fd = open (PATH_SUPERLOCK,
				     O_WRONLY | O_EXCL | O_CREAT, 0666)) < 0;
       i++)
    {
      if (errno != EEXIST)
	return;
      sleep (1);
    }
  if (fd >= 0)
    {
#ifdef NO_FCHMOD
      chmod (PATH_SUPERLOCK, 0666);
#else
      fchmod (fd, 0666);
#endif
      write (fd, lfname, strlen (lfname));
      close (fd);
    }
}

void
unlock_all_files ()
{
  register Lisp_Object tail;
  register struct buffer *b;

  for (tail = Vbuffer_alist; XGCTYPE (tail) == Lisp_Cons;
       tail = XCONS (tail)->cdr)
    {
      b = XBUFFER (XCONS (XCONS (tail)->car)->cdr);
      if (XTYPE (b->filename) == Lisp_String &&
	  b->save_modified < BUF_MODIFF (b))
	unlock_file (b->filename);
    }
}


#ifdef _NO_EMACS_PROTO
DEFUN ("lock-buffer", Flock_buffer, Slock_buffer,
  0, 1, 0,
  "Locks FILE, if current buffer is modified.\n\
FILE defaults to current buffer's visited file,\n\
or else nothing is done if current buffer isn't visiting a file.")
  (fn)
     Lisp_Object fn;
#else /* _NO_EMACS_PROTO */
DEFUN ("lock-buffer", Flock_buffer, Slock_buffer,
  0, 1, 0,
  "Locks FILE, if current buffer is modified.\n\
FILE defaults to current buffer's visited file,\n\
or else nothing is done if current buffer isn't visiting a file.")
  (Lisp_Object fn)
#endif /* _NO_EMACS_PROTO */
{
  if (NULL (fn))
    fn = current_buffer->filename;
  else
    CHECK_STRING (fn, 0);
  if (current_buffer->save_modified < MODIFF
      && !NULL (fn))
    lock_file (fn);
  return Qnil;    
}

DEFUN ("unlock-buffer", Funlock_buffer, Sunlock_buffer,
  0, 0, 0,
 "Unlocks the file visited in the current buffer,\n\
if it should normally be locked.")
  ()
{
  if (current_buffer->save_modified < MODIFF &&
      XTYPE (current_buffer->filename) == Lisp_String)
    unlock_file (current_buffer->filename);
  return Qnil;
}


/* Unlock the file visited in buffer BUFFER.  */

#ifdef _NO_EMACS_PROTO
unlock_buffer (buffer)
     struct buffer *buffer;
#else /* _NO_EMACS_PROTO */
void
unlock_buffer (struct buffer *buffer)
#endif /* _NO_EMACS_PROTO */
{
  if (buffer->save_modified < BUF_MODIFF (buffer)
      && XTYPE (buffer->filename) == Lisp_String)
    unlock_file (buffer->filename);
}

#ifdef _NO_EMACS_PROTO
DEFUN ("file-locked-p", Ffile_locked_p, Sfile_locked_p, 0, 1, 0,
  "Returns nil if the FILENAME is not locked,\n\
t if it is locked by you, else a string of the name of the locker.")
  (fn)
  Lisp_Object fn;
#else /* _NO_EMACS_PROTO */
DEFUN ("file-locked-p", Ffile_locked_p, Sfile_locked_p, 0, 1, 0,
  "Returns nil if the FILENAME is not locked,\n\
t if it is locked by you, else a string of the name of the locker.")
  (Lisp_Object fn)
#endif /* _NO_EMACS_PROTO */
{
  register char *lfname;
  int owner;

  fn = Fexpand_file_name (fn, Qnil);

  /* Create the name of the lock-file for file filename */
  CREATE_LOCK_FILE_NAME (lfname, fn);

  owner = current_lock_owner (lfname);
  if (owner <= 0)
    return (Qnil);
  else if (owner == getpid ())
    return (Qt);
  
  return (lock_file_owner_name (lfname));
}

void
syms_of_filelock ()
{
  defsubr (&Sunlock_buffer);
  defsubr (&Slock_buffer);
  defsubr (&Sfile_locked_p);
}

#endif /* CLASH_DETECTION */
