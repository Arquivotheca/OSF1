/* m- file for HLH Orion 1/05 (Clipper).
   Copyright (C) 1985 Free Software Foundation, Inc.
   Lee McLoughlin <lmjm%doc.imperial.ac.uk@nss.cs.ucl.ac.uk>

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

/* Define BIG_ENDIAN iff lowest-numbered byte in a word
   is the most significant byte.  */

#undef BIG_ENDIAN

/* Define NO_ARG_ARRAY if you cannot take the address of the first of a
 * group of arguments and treat it as an array of the arguments.  */

#define NO_ARG_ARRAY

/* Define WORD_MACHINE if addresses and such have
 * to be corrected before they can be used as byte counts.  */

#define SIGN_EXTEND_CHAR(c) ((int)(c))

/* Use type int rather than a union, to represent Lisp_Object */
/* This is desirable for most machines.  */

#define NO_UNION_TYPE

/* Data type of load average, as read out of kmem.  */
/* This used to be `double'.  */

#define LOAD_AVE_TYPE long

/* Convert that into an integer that is 100 for a load average of 1.0  */

/* This used to be 1.0.  */
#ifndef FSCALE
#define FSCALE 256
#endif
#define LOAD_AVE_CVT(x) (int) (((double) (x)) * 100.0 / FSCALE)

/* HLH have a SIGWINCH defined (but unimplemented) so we need a sigmask */
#ifndef sigmask
#define sigmask(m) (1 << ((m) - 1))
#endif

#define HAVE_ALLOCA

/* Here is where programs actually start running */
#define TEXT_START 0x8000
#define LD_TEXT_START_ADDR 8000

/* Arguments to ignore before argc in crt0.c.  */
#define DUMMIES dummy1, dummy2,
