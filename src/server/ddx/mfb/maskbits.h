/*
 * *****************************************************************
 * *                                                               *
 * *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
 * *                                                               *
 * *   All Rights Reserved.  Unpublished rights  reserved  under   *
 * *   the copyright laws of the United States.                    *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Digital   *
 * *   Equipment Corporation.  Possession, use,  duplication  or   *
 * *   dissemination of the software and media is authorized only  *
 * *   pursuant to a valid written license from Digital Equipment  *
 * *   Corporation.                                                *
 * *                                                               *
 * *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
 * *   by the U.S. Government is subject to restrictions  as  set  *
 * *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
 * *   or  in  FAR 52.227-19, as applicable.                       *
 * *                                                               *
 * *****************************************************************
 */
/*
 * HISTORY
 */
/* Combined Purdue/PurduePlus patches, level 2.1, 1/24/89 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: maskbits.h,v 1.26 91/07/09 19:44:24 keith Exp $ */
#include "X.h"
#include "Xmd.h"
#include "servermd.h"


/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

starttab[n]
	bits[0,n-1] = 0	bits[n,PLST] = 1
endtab[n] =
	bits[0,n-1] = 1	bits[n,PLST] = 0

startpartial[], endpartial[]
	these are used as accelerators for doing putbits and masking out
bits that are all contained between longword boudaries.  the extra
256 bytes of data seems a small price to pay -- code is smaller,
and narrow things (e.g. window borders) go faster.

the names may seem misleading; they are derived not from which end
of the word the bits are turned on, but at which end of a scanline
the table tends to be used.

look at the tables and macros to understand boundary conditions.
(careful readers will note that starttab[n] = ~endtab[n] for n != 0)

-----------------------------------------------------------------------
these two macros depend on the screen's bit ordering.
in both of them x is a screen position.  they are used to
combine bits collected from multiple longwords into a
single destination longword, and to unpack a single
source longword into multiple destinations.

SCRLEFT(dst, x)
	takes dst[x, PPW] and moves them to dst[0, PPW-x]
	the contents of the rest of dst are 0.
	this is a right shift on LSBFirst (forward-thinking)
	machines like the VAX, and left shift on MSBFirst
	(backwards) machines like the 680x0 and pc/rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[PPW-x, PPW]
	the contents of the rest of dst are 0.
	this is a left shift on LSBFirst, right shift
	on MSBFirst.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged bits at start, mask for ragged bits at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the bits are in the
	same longword (i.e. (x&PIM + w) <= PPW)

maskPPWbits(x, w, startmask, endmask, nlw)
	as maskbits, but does not calculate nlw.  it is used by
	mfbGlyphBlt to put down glyphs <= PPW bits wide.

-------------------------------------------------------------------

NOTE
	any pointers passed to the following 4 macros are
	guranteed to be PPW-bit aligned.
	The only non-PPW-bit-aligned references ever made are
	to font glyphs, and those are made with getleftbits()
	and getshiftedleftbits (qq.v.)

	For 64-bit server, it is assumed that we will never have font padding
	of more than 4 bytes. The code uses int's to access the fonts
	intead of longs.

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < PPW), collect w
	bits and put them in the screen left portion of dst.
	psrc is a longword pointer.  this may span longword boundaries.
	it special-cases fetching all w bits from one longword.

	+--------+--------+		+--------+
	|    | m |n|      |	==> 	| m |n|  |
	+--------+--------+		+--------+
	    x      x+w			0     w
	psrc     psrc+1			dst
			m = PPW - x
			n = w - m

	implementation:
	get m bits, move to screen-left of dst, zeroing rest of dst;
	get n bits from next word, move screen-right by m, zeroing
		 lower m bits of word.
	OR the two things together.

putbits(src, x, w, pdst)
	starting at position x in pdst, put down the screen-leftmost
	w bits of src.  pdst is a longword pointer.  this may
	span longword boundaries.
	it special-cases putting all w bits into the same longword.

	+--------+			+--------+--------+
	| m |n|  |		==>	|    | m |n|      |
	+--------+			+--------+--------+
	0     w				     x     x+w
	dst				pdst     pdst+1
			m = PPW - x
			n = w - m

	implementation:
	get m bits, shift screen-right by x, zero screen-leftmost x
		bits; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-PPW;
		zero leftmost n bits of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, ROP)
	like putbits but calls DoRop with the rasterop ROP (see mfb.h for
	DoRop)

putbitsrrop(src, x, w, pdst, ROP)
	like putbits but calls DoRRop with the reduced rasterop ROP
	(see mfb.h for DoRRop)

-----------------------------------------------------------------------
	The two macros below are used only for getting bits from glyphs
in fonts, and glyphs in fonts are gotten only with the following two
mcros.
	You should tune these macros toyour font format and cpu
byte ordering.

NOTE
getleftbits(psrc, w, dst)
	get the leftmost w (w<=32) bits from *psrc and put them
	in dst.  this is used by the mfbGlyphBlt code for glyphs
	<=PPW bits wide.
	psrc is declared (unsigned char *)

	psrc is NOT guaranteed to be PPW-bit aligned.  on  many
	machines this will cause problems, so there are several
	versions of this macro.

	this macro is called ONLY for getting bits from font glyphs,
	and depends on the server-natural font padding.

	for blazing text performance, you want this macro
	to touch memory as infrequently as possible (e.g.
	fetch longwords) and as efficiently as possible
	(e.g. don't fetch misaligned longwords)

getshiftedleftbits(psrc, offset, w, dst)
	used by the font code; like getleftbits, but shifts the
	bits SCRLEFT by offset.
	this is implemented portably, calling getleftbits()
	and SCRLEFT().
	psrc is declared (unsigned char *).
*/

/* to match CFB and allow algorithm sharing ... */

/*
 * name	    mfb/mfb64	    cfb	    explanation
 * ----	    -----	    ---	    -----------
 * PPW	    32/64	    4	    pixels per word
 * PLST	    31/63	    3	    last pixel in a word (should be PPW-1)
 * PIM	    0x1f/0x3f       0x03    pixel index mask (index within a word)
 * PWSH	    5/6		    2	    pixel-to-word shift
 * PSZ	    1		    8	    pixel size (bits)
 * PMSK	    0x01	    0xFF    single-pixel mask
 */
#if LONG_BIT == 32
#define PPW	32
#define PLST	31
#define PIM	0x1f
#define PWSH	5
#else /* LONG_BIT */
#define PPW	64
#define PLST	63
#define PIM	0x3f
#define PWSH	6
#endif /* LONG_BIT */
#define PSZ	1
#define PMSK	0x01
#define BitLeft(b,s)	SCRLEFT(b,s)
#define BitRight(b,s)	SCRRIGHT(b,s)

#define WRDSZ	(8*sizeof(long))

extern unsigned long starttab[];
extern unsigned long endtab[];
extern unsigned long partmasks[WRDSZ][WRDSZ];
extern unsigned long rmask[];
extern unsigned long mask[];


#if (BITMAP_BIT_ORDER == IMAGE_BYTE_ORDER)
#define LONG2CHARS(x) (x)
#else
/*
 *  the unsigned case below is for compilers like
 *  the Danbury C and i386cc
 */
#if LONG_BIT == 32
#define LONG2CHARS( x ) ( ( ( ( x ) & 0x000000FF ) << 0x18 ) \
                      | ( ( ( x ) & 0x0000FF00 ) << 0x08 ) \
                      | ( ( ( x ) & 0x00FF0000 ) >> 0x08 ) \
                      | ( ( ( x ) & (unsigned long)0xFF000000 ) >> 0x18 ) )
#else /* LONG_BIT */
#define LONG2CHARS( x ) \
      ( ( ( ( x ) & 0x000000FFL ) << 0x18 ) \
      | ( ( ( x ) & 0x0000FF00L ) << 0x08 ) \
      | ( ( ( x ) & 0x00FF0000L ) >> 0x08 ) \
      | ( ( ( x ) & (unsigned long)0xFF000000L ) >> 0x18 ) \
      | ( ( ( x ) & 0x000000FF00000000L ) << 0x18 ) \
      | ( ( ( x ) & 0x0000FF0000000000L ) << 0x08 ) \
      | ( ( ( x ) & 0x00FF000000000000L ) >> 0x08 ) \
      | ( ( ( x ) & (unsigned long)0xFF00000000000000L ) >> 0x18 ) )
#endif /* LONG_BIT */
#endif

#ifdef STRICT_ANSI_SHIFT
#define SHL(x,y)    ((y) >= WRDSZ ? 0 : LONG2CHARS(LONG2CHARS(x) << (y)))
#define SHR(x,y)    ((y) >= WRDSZ ? 0 : LONG2CHARS(LONG2CHARS(x) >> (y)))
#else
#define SHL(x,y)    LONG2CHARS(LONG2CHARS(x) << (y))
#define SHR(x,y)    LONG2CHARS(LONG2CHARS(x) >> (y))
#endif

#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#define SCRLEFT(lw, n)	SHL((unsigned long)(lw),(n))
#define SCRRIGHT(lw, n)	SHR((unsigned long)(lw),(n))
#else					/* vax, intel */
#define SCRLEFT(lw, n)	SHR((unsigned long)(lw),(n))
#define SCRRIGHT(lw, n)	SHL((unsigned long)(lw),(n))
#endif

#define DoRRop(alu, src, dst) \
(((alu) == RROP_BLACK) ? ((dst) & ~(src)) : \
 ((alu) == RROP_WHITE) ? ((dst) | (src)) : \
 ((alu) == RROP_INVERT) ? ((dst) ^ (src)) : \
  (dst))

#if LONG_BIT == 32
/* A generalized form of a x4 Duff's Device */
#define Duff(counter, block) { \
  while (counter >= 4) {\
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     counter -= 4; \
  } \
     switch (counter & 3) { \
     case 3:	{ block; } \
     case 2:	{ block; } \
     case 1:	{ block; } \
     case 0: \
     counter = 0; \
   } \
}
#else /* LONG_BIT */
/* A generalized form of a x8 Duff's Device */
#define Duff(counter, block) { \
  while (counter >= 8) {\
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     { block; } \
     counter -= 8; \
  } \
     switch (counter & 7) { \
     case 7:	{ block; } \
     case 6:	{ block; } \
     case 5:	{ block; } \
     case 4:	{ block; } \
     case 3:	{ block; } \
     case 2:	{ block; } \
     case 1:	{ block; } \
     case 0: \
     counter = 0; \
   } \
}
#endif /* LONG_BIT */
#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = starttab[(x)&PIM]; \
    endmask = endtab[((x)+(w)) & PIM]; \
    if (startmask) \
	nlw = (((w) - (WRDSZ - ((x)&PIM))) >> PWSH); \
    else \
	nlw = (w) >> PWSH;

#define maskpartialbits(x, w, mask) \
    mask = partmasks[(x)&PIM][(w)&PIM];

#define maskPPWbits(x, w, startmask, endmask) \
    startmask = starttab[(x)&PIM]; \
    endmask = endtab[((x)+(w)) & PIM];

#if defined(__GNUC__) && LONG_BIT != 64
#ifdef vax
#define FASTGETBITS(psrc,x,w,dst) \
    __asm ("extzv %1,%2,%3,%0" \
	 : "=g" (dst) \
	 : "g" (x), "g" (w), "m" (*(char *)(psrc)))
#define getbits(psrc,x,w,dst) FASTGETBITS(psrc,x,w,dst)

#define FASTPUTBITS(src, x, w, pdst) \
    __asm ("insv %3,%1,%2,%0" \
	 : "=m" (*(char *)(pdst)) \
	 : "g" (x), "g" (w), "g" (src))
#define putbits(src, x, w, pdst) FASTPUTBITS(src, x, w, pdst)
#endif /* vax */
#ifdef mc68020
#define FASTGETBITS(psrc, x, w, dst) \
    __asm ("bfextu %3{%1:%2},%0" \
    : "=d" (dst) : "di" (x), "di" (w), "o" (*(char *)(psrc)))

#define getbits(psrc,x,w,dst) \
{ \
    FASTGETBITS(psrc, x, w, dst);\
    dst = SHL(dst,(32-(w))); \
}

#define FASTPUTBITS(src, x, w, pdst) \
    __asm ("bfins %3,%0{%1:%2}" \
	 : "=o" (*(char *)(pdst)) \
	 : "di" (x), "di" (w), "d" (src), "0" (*(char *) (pdst)))

#define putbits(src, x, w, pdst) FASTPUTBITS(SHR((src),32-(w)), x, w, pdst)

#endif /* mc68020 */
#endif /* __GNUC__ */

/*  The following flag is used to override a bugfix for sun 3/60+CG4 machines,
 */

/*  We don't need to be careful about this unless we're dealing with sun3's 
 *  We will default its usage for those who do not know anything, but will
 *  override its effect if the machine doesn't look like a sun3 
 */
#if !defined(mc68020) || !defined(sun)
#define NO_3_60_CG4
#endif

/* This is gross.  We want to #define u_putbits as something which can be used
 * in the case of the 3/60+CG4, but if we use /bin/cc or are on another
 * machine type, we want nothing to do with u_putbits.  What a hastle.  Here
 * I used slo_putbits as something which either u_putbits or putbits could be
 * defined as.
 *
 * putbits gets it iff it is not already defined with FASTPUTBITS above.
 * u_putbits gets it if we have FASTPUTBITS (putbits) from above and have not
 * 	overridden the NO_3_60_CG4 flag.
 */

#define slo_putbits(src, x, w, pdst) \
{ \
    register int n = (x)+(w)-WRDSZ; \
    \
    if (n <= 0) \
    { \
	register long tmpmask; \
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | \
		(SCRRIGHT(src, x) & tmpmask); \
    } \
    else \
    { \
	*(pdst) = (*(pdst) & endtab[x]) | (SCRRIGHT((src), x)); \
	(pdst)[1] = ((pdst)[1] & starttab[n]) | \
		(SCRLEFT(src, WRDSZ-(x)) & endtab[n]); \
    } \
}

#if defined(putbits) && !defined(NO_3_60_CG4)
#define u_putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#else
#define u_putbits(src, x, w, pdst) putbits(src, x, w, pdst)
#endif

#if !defined(putbits) 
#define putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#endif

/* Now if we have not gotten any really good bitfield macros, try some
 * moderately fast macros.  Alas, I don't know how to do asm instructions
 * without gcc.
 */

#ifndef getbits
#define getbits(psrc, x, w, dst) \
{ \
    dst = SCRLEFT(*(psrc), (x)); \
    if ( ((x) + (w)) > WRDSZ) \
	dst |= (SCRRIGHT(*((psrc)+1), WRDSZ-(x))); \
}
#endif

/*  We have to special-case putbitsrop because of 3/60+CG4 combos
 */

#define u_putbitsrop(src, x, w, pdst, rop) \
{\
	register unsigned long t1, t2; \
	register int n = (x)+(w)-WRDSZ; \
	\
	t1 = SCRRIGHT((src), (x)); \
	DoRop(t2, rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register unsigned long tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = WRDSZ-(x); \
	*(pdst) = (*(pdst) & endtab[x]) | (t2 & starttab[x]); \
	t1 = SCRLEFT((src), m); \
	DoRop(t2, rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & starttab[n]) | (t2 & endtab[n]); \
    } \
}

/* If our getbits and putbits are FAST enough,
 * do this brute force, it's faster
 */

#if defined(FASTPUTBITS) && defined(FASTGETBITS) && defined(NO_3_60_CG4)
#if (BITMAP_BIT_ORDER == MSBFirst)
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register unsigned long _tmp, _tmp2; \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, WRDSZ-(w)); \
  DoRop(_tmp, rop, _tmp2, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register unsigned long _tmp, _tmp2; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, WRDSZ-(w)); \
  _tmp= DoRRop(rop, _tmp2, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#else
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register unsigned long _tmp; \
  FASTGETBITS(pdst, x, w, _tmp); \
  DoRop(_tmp, rop, src, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register long _tmp; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp= DoRRop(rop, src, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#endif
#endif

#ifndef putbitsrop
#define putbitsrop(src, x, w, pdst, rop)  u_putbitsrop(src, x, w, pdst, rop)
#endif 

#ifndef putbitsrrop
#define putbitsrrop(src, x, w, pdst, rop) \
{\
	register unsigned long t1, t2; \
	register int n = (x)+(w)-WRDSZ; \
	\
	t1 = SCRRIGHT((src), (x)); \
	t2 = DoRRop(rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register unsigned long tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = WRDSZ-(x); \
	*(pdst) = (*(pdst) & endtab[x]) | (t2 & starttab[x]); \
	t1 = SCRLEFT((src), m); \
	t2 = DoRRop(rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & starttab[n]) | (t2 & endtab[n]); \
    } \
}
#endif

#if GETLEFTBITS_ALIGNMENT == 1
#define getleftbits(psrc, w, dst)	dst = *((unsigned int *) psrc)
#endif /* GETLEFTBITS_ALIGNMENT == 1 */

#if GETLEFTBITS_ALIGNMENT == 2
#define getleftbits(psrc, w, dst) \
    { \
	if ( ((int)(psrc)) & 0x01 ) \
		getbits( ((unsigned int *)(((char *)(psrc))-1)), 8, (w), (dst) ); \
	else \
		getbits(psrc, 0, w, dst);
    }
#endif /* GETLEFTBITS_ALIGNMENT == 2 */

#if GETLEFTBITS_ALIGNMENT == 4
#define getleftbits(psrc, w, dst) \
    { \
	int off, off_b; \
	off_b = (off = ( ((int)(psrc)) & 0x03)) << 3; \
	getbits( \
		(unsigned int *)( ((char *)(psrc)) - off), \
		(off_b), (w), (dst) \
	       ); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 4 */


#define getshiftedleftbits(psrc, offset, w, dst) \
	getleftbits((psrc), (w), (dst)); \
	dst = SCRLEFT((dst), (offset));

/* FASTGETBITS and FASTPUTBITS are not necessarily correct implementations of
 * getbits and putbits, but they work if used together.
 *
 * On a MSBFirst machine, a cpu bitfield extract instruction (like bfextu)
 * could normally assign its result to a long word register in the screen
 * right position.  This saves canceling register shifts by not fighting the
 * natural cpu byte order.
 *
 * Unfortunately, these fail on a 3/60+CG4 and cannot be used unmodified. Sigh.
 */
#if defined(FASTGETBITS) && defined(FASTPUTBITS)
#ifdef NO_3_60_CG4
#define u_FASTPUT(aa, bb, cc, dd)  FASTPUTBITS(aa, bb, cc, dd)
#else
#define u_FASTPUT(aa, bb, cc, dd)  u_putbits(SCRLEFT(aa, WRDSZ-(cc)), bb, cc, dd)
#endif

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register unsigned long _tmpbits; \
    FASTGETBITS(psrc, srcbit, width, _tmpbits); \
    u_FASTPUT(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register unsigned long _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  DoRop(_tmpdst, rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register unsigned long _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  _tmpdst = DoRRop(rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputbits0(psrc, srcbit, width, pdst) \
	getandputbits(psrc, srcbit, 0, width, pdst)

#define getandputrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrop(psrc, srcbit, 0, width, pdst, rop)

#define getandputrrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrrop(psrc, srcbit, 0, width, pdst, rop)


#else /* Slow poke */

/* pairs of getbits/putbits happen frequently. Some of the code can
 * be shared or avoided in a few specific instances.  It gets us a
 * small advantage, so we do it.  The getandput...0 macros are the only ones
 * which speed things here.  The others are here for compatibility w/the above
 * FAST ones
 */

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register unsigned long _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits); \
    putbits(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register unsigned long _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrop(_tmpbits, dstbit, width, pdst, rop) \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register unsigned long _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrrop(_tmpbits, dstbit, width, pdst, rop) \
}


#define getandputbits0(psrc, sbindex, width, pdst) \
{			/* unroll the whole damn thing to see how it * behaves */ \
    register int          _flag = WRDSZ - (sbindex); \
    register unsigned long _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
 \
    *(pdst) = (*(pdst) & starttab[(width)]) | (_src & endtab[(width)]); \
}


#define getandputrop0(psrc, sbindex, width, pdst, rop) \
{			\
    register int          _flag = WRDSZ - (sbindex); \
    register unsigned long _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    DoRop(_src, rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & starttab[(width)]) | (_src & endtab[(width)]); \
}

#define getandputrrop0(psrc, sbindex, width, pdst, rop) \
{ \
    int             _flag = WRDSZ - (sbindex); \
    register unsigned long _src; \
 \
    _src = SCRLEFT (*(psrc), (sbindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    _src = DoRRop(rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & starttab[(width)]) | (_src & endtab[(width)]); \
}

#endif  /* FASTGETBITS && FASTPUTBITS */
