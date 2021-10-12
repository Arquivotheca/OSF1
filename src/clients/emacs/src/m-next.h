/* Configuration file for NeXT machine.
   Copyright (C) 1990 Free Software Foundation, Inc.

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

/* Say this machine is a next if not previously defined */

#ifndef NeXT
#define NeXT
#endif

/* The following three symbols give information on
 the size of various data types.  */

#define SHORTBITS 16		/* Number of bits in a short */

#define INTBITS 32		/* Number of bits in an int */

#define LONGBITS 32		/* Number of bits in a long */

/* Let the compiler tell us what byte order architecture we're compiling for */

#ifdef __BIG_ENDIAN__
#define BIG_ENDIAN
#endif

/* Define how to take a char and sign-extend into an int.
   On machines where char is signed, this is a no-op.  */

#define SIGN_EXTEND_CHAR(c) (c)

/* Use type int rather than a union, to represent Lisp_Object */

#define NO_UNION_TYPE

/* XINT must explicitly sign-extend */

#define EXPLICIT_SIGN_EXTEND

/* Data type of load average, as read out of kmem.  */

#define LOAD_AVE_TYPE long

/* Convert that into an integer that is 100 for a load average of 1.0  */

#define LOAD_AVE_CVT(x) (int) (((double) (x)) * 100.0 / FSCALE)

/* Say that the text segment of a.out includes the header;
   the header actually occupies the first few bytes of the text segment
   and is counted in hdr.a_text.  */

#define A_TEXT_OFFSET(HDR) sizeof (HDR)

/* Use dk.h, not dkstat.h, in loadst.c.  */

#define DK_HEADER_FILE

/* Mask for address bits within a memory segment */

#define SEGSIZ 0x20000
#define SEGMENT_MASK (SEGSIZ - 1)

#define HAVE_ALLOCA

#define SYSTEM_MALLOC

#define HAVE_UNIX_DOMAIN

#define LIB_X11_LIB -L/usr/lib/X11 -lX11

/* Conflicts in process.c between ioctl.h & tty.h use of t_foo fields */

#define NO_T_CHARS_DEFINES

/* Use our own unexec routines */

#define UNEXEC unexnext.o

/* We don't use _start, etext, or edata */

#define TEXT_START	NULL /* wrong: but nobody uses it anyway */
#define TEXT_END	get_etext()
#define DATA_END	get_edata()

/* We don't have a g library either, so override the -lg LIBS_DEBUG switch */

#define LIBS_DEBUG

/* We don't have a libgcc.a, so we can't let LIB_GCC default to -lgcc */

#define LIB_GCC

/* Compile "strict bsd" to avoid warnings from include files */

#define C_SWITCH_MACHINE	-bsd

/* Link this program just by running cc.  */
#define ORDINARY_LINK

/* This was needed before we used ORDINARY_LINK.  */
#if 0
/* Use our own crt0 routine */
#define NO_REMAP

/* Standard library is libsys_s, not libc */

#define LIB_STANDARD -lsys_s
#endif

/* Where to find the kernel, for load average.  */
#define KERNEL_FILE "/mach"
