/* m-alliant.h  Alliant machine running system version 2 or 3.
   Copyright (C) 1985, 1986, 1987 Free Software Foundation, Inc.
   Note that for version 1 of the Alliant system
   you should use m-alliant1.h instead of this file.
   Use m-alliant4.h for version 4.

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

#define BIG_ENDIAN

/* Define NO_ARG_ARRAY if you cannot take the address of the first of a
 * group of arguments and treat it as an array of the arguments.  */

#ifdef ALLIANT_1
#define NO_ARG_ARRAY
#endif

/* Define WORD_MACHINE if addresses and such have
 * to be corrected before they can be used as byte counts.  */

#undef WORD_MACHINE

/* Define how to take a char and sign-extend into an int.
   On machines where char is signed, this is a no-op.  */

#define SIGN_EXTEND_CHAR(c) (c)

/* Now define a symbol for the cpu type, if your compiler
   does not define it automatically:
   vax, m68000, ns16000, pyramid, orion, tahoe and APOLLO
   are the ones defined so far.  */

#define ALLIANT

/* Use type int rather than a union, to represent Lisp_Object */
/* This is desirable for most machines.  */

#define NO_UNION_TYPE

/* Define EXPLICIT_SIGN_EXTEND if XINT must explicitly sign-extend
   the 24-bit bit field into an int.  In other words, if bit fields
   are always unsigned.

   If you use NO_UNION_TYPE, this flag does not matter.  */
/* On Alliants, bitfields are unsigned. */

#define EXPLICIT_SIGN_EXTEND

/* No load average information available for Alliants. */

#undef LOAD_AVE_TYPE
#undef LOAD_AVE_CVT

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

/* Define C_ALLOCA if this machine does not support a true alloca
   and the one written in C should be used instead.
   Define HAVE_ALLOCA to say that the system provides a properly
   working alloca function and it should be used.
   Define neither one if an assembler-language alloca
   in the file alloca.s should be used.  */

#undef C_ALLOCA
#define HAVE_ALLOCA

#ifdef ALLIANT_1
#define C_ALLOCA
#undef HAVE_ALLOCA
#endif /* ALLIANT_1 */

/* Define NO_REMAP if memory segmentation makes it not work well
   to change the boundary between the text section and data section
   when Emacs is dumped.  If you define this, the preloaded Lisp
   code will not be sharable; but that's better than failing completely.  */
/* Actually, Alliant CONCENTRIX does paging "right":
   data pages are copy-on-write, which means that the pure data areas
   are shared automatically and remapping is not necessary.  */

#define NO_REMAP

/* Alliant needs special crt0.o because system version is not reentrant */

#define START_FILES crt0.o

/* Alliant dependent code for dumping executing image.
   See crt0.c code for alliant.  */

#define ADJUST_EXEC_HEADER {\
extern int _curbrk, _setbrk;\
_setbrk = _curbrk;\
hdr.a_bss_addr = bss_start;\
unexec_text_start = hdr.a_text_addr;}

/* cc screws up on long names.  Try making cpp replace them.  */

#ifdef ALLIANT_1
#define Finsert_abbrev_table_description Finsert_abbrev_table_descrip
#define internal_with_output_to_temp_buffer internal_with_output_to_tem
#endif

/* "vector" is a typedef in /usr/include/machine/reg.h, so its use as
   a variable name causes errors when compiling under ANSI C.  */

#define vector xxvector
