/* Definitions file for GNU Emacs running on Stride Micro System-V.2.2
   Copyright (C) 1985, 1986 Free Software Foundation, Inc.

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


/* The following three symbols give information on
 the size of various data types.  */

#define SHORTBITS 16		/* Number of bits in a short */

#define INTBITS 32		/* Number of bits in an int */

#define LONGBITS 32		/* Number of bits in a long */

/* 68000 has lowest-numbered byte as most significant */

#define BIG_ENDIAN

/* Define NO_ARG_ARRAY if you cannot take the address of the first of a
 * group of arguments and treat it as an array of the arguments.  */

#undef NO_ARG_ARRAY

/* Define WORD_MACHINE if addresses and such have
 * to be corrected before they can be used as byte counts.  */

#undef WORD_MACHINE

/* Define how to take a char and sign-extend into an int.
   On machines where char is signed, this is a no-op.  */

#define SIGN_EXTEND_CHAR(c) (c)

/* Now define a symbol for the cpu type, if your compiler
   does not define it automatically:
   vax, m68000, ns16000, pyramid, orion, tahoe, APOLLO and STRIDE
   are the ones defined so far.  */

#define m68000			/* because the SGS compiler defines "m68k" */
#ifndef STRIDE
#define STRIDE
#endif

/* Use type int rather than a union, to represent Lisp_Object */
/* This is desirable for most machines.  */

#define NO_UNION_TYPE

/* Define EXPLICIT_SIGN_EXTEND if XINT must explicitly sign-extend
   the 24-bit bit field into an int.  In other words, if bit fields
   are always unsigned.

   If you use NO_UNION_TYPE, this flag does not matter.  */

#define EXPLICIT_SIGN_EXTEND

/* Data type of load average, as read out of kmem.  */

#define LOAD_AVE_TYPE double

/* Convert that into an integer that is 100 for a load average of 1.0  */

#define LOAD_AVE_CVT(x) ((int) ((x) * 100.0))

/* Define CANNOT_DUMP on machines where unexec does not work.
   Then the function dump-emacs will not be defined
   and temacs will do (load "loadup") automatically unless told otherwise.  */

#undef CANNOT_DUMP

/* Define VIRT_ADDR_VARIES if the virtual addresses of
   pure and impure space as loaded can vary, and even their
   relative order cannot be relied on.

   Otherwise Emacs assumes that text space precedes data space,
   numerically.  */

#undef VIRT_ADDR_VARIES

/* The STRIDE system is more powerful than standard USG5.  */

#define HAVE_PTYS
#define HAVE_TIMEVAL
#define HAVE_SELECT
#define HAVE_GETTIMEOFDAY
#define BSTRING
#define SKTPAIR
#define HAVE_SOCKETS

#define MAIL_USE_FLOCK
#undef TERMINFO
#define EXEC_MAGIC 0413

/* USG wins again: Foo! I can't get SIGIO to work properly on the Stride, because I'm
   running a System V variant, and don't have a reliable way to block SIGIO
   signals without losing them.  So, I've gone back to non-SIGIO mode, so
   please append this line to the file "m-stride.h":
 */
#undef SIGIO

/* Specify alignment requirement for start of text and data sections
   in the executable file.  */

#define SECTION_ALIGNMENT (getpagesize() - 1)

/*
 * UniStride has this in /lib/libc.a.
 */
#undef NONSYSTEM_DIR_LIBRARY

/* UniStride defines getwd.  */

#define HAVE_GETWD


