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
/* $XConsortium: mfbtegblt.c,v 5.7 91/05/26 09:02:16 rws Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
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
#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"mfb.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"maskbits.h"

/*
    this works for fonts with glyphs <= PPW bits wide.

    This should be called only with a terminal-emulator font;
this means that the FIXED_METRICS flag is set, and that
glyphbounds == charbounds.

    in theory, this goes faster; even if it doesn't, it reduces the
flicker caused by writing a string over itself with image text (since
the background gets repainted per character instead of per string.)
this seems to be important for some converted X10 applications.

    Image text looks at the bits in the glyph and the fg and bg in the
GC.  it paints a rectangle, as defined in the protocol dcoument,
and the paints the characters.

   to avoid source proliferation, this file is compiled
two times:
	MFBTEGLYPHBLT		OP
	mfbTEGlyphBltWhite		(white text, black bg )
	mfbTEGlyphBltBlack	~	(black text, white bg )

64-bit server notes:
	fonts are less than 32bits, but mfb is 64 bits. So we keep the
	same code to reference the fonts, but double the amount we
	render at any time. For instance, when padding is 1 byte,
	we can copy in 8 glyphs rather than just 4.
*/

#if defined(NO_3_60_CG4) && defined(FASTPUTBITS) && defined(FASTGETBITS)
#define FASTCHARS
#endif

/*
 * this macro "knows" that only characters <= 8 bits wide will
 * fit this case (which is why it is independent of GLYPHPADBYTES)
 */

#if (BITMAP_BIT_ORDER == MSBFirst) && (GLYPHPADBYTES != 4)
#if GLYPHPADBYTES == 1
#define ShiftAmnt   24
#else
#define ShiftAmnt   16
#endif

#if LONG_BIT == 32
#define GetBits4    c = (*char1++ << ShiftAmnt) | \
			SCRRIGHT (*char2++ << ShiftAmnt, xoff2) | \
			SCRRIGHT (*char3++ << ShiftAmnt, xoff3) | \
			SCRRIGHT (*char4++ << ShiftAmnt, xoff4);
#else /* LONG_BIT */
#define GetBits4    c = ((unsigned long)(*char1++ << ShiftAmnt) << 32 )  | \
			(SCRRIGHT (*char2++ << ShiftAmnt, xoff2) << 32 ) | \
			(SCRRIGHT (*char3++ << ShiftAmnt, xoff3) << 32 ) | \
			(SCRRIGHT (*char4++ << ShiftAmnt, xoff4) << 32 ) | \
			(*char5++ << ShiftAmnt) 			 | \
			SCRRIGHT (*char6++ << ShiftAmnt, xoff6) 	 | \
			SCRRIGHT (*char7++ << ShiftAmnt, xoff7) 	 | \
			SCRRIGHT (*char8++ << ShiftAmnt, xoff8);
#endif /* LONG_BIT */
#else
#if LONG_BIT == 32
#define GetBits4    c = *char1++ | \
			SCRRIGHT (*char2++, xoff2) | \
			SCRRIGHT (*char3++, xoff3) | \
			SCRRIGHT (*char4++, xoff4);
#elif LONG_BIT == 64
#define GetBits4    c = (unsigned long)(((*char1++) << 64 ) | \
                        (SCRRIGHT (*char2++, xoff2) << 64 ) | \
                        (SCRRIGHT (*char3++, xoff3) << 64 ) | \
                        (SCRRIGHT (*char4++, xoff4) << 64 ) | \
                        SCRRIGHT (*char5++, xoff5)          | \
                        SCRRIGHT (*char6++, xoff6)          | \
                        SCRRIGHT (*char7++, xoff7)          | \
                        SCRRIGHT (*char8++, xoff8));
#else /* LONG_BIT */
#define GetBits4    c = ((unsigned long)(*char1++) << 32 )  | \
			(SCRRIGHT (*char2++, xoff2) << 32 ) | \
			(SCRRIGHT (*char3++, xoff3) << 32 ) | \
			(SCRRIGHT (*char4++, xoff4) << 32 ) | \
			(*char5++) 			    | \
			SCRRIGHT (*char6++, xoff6) 	    | \
			SCRRIGHT (*char7++, xoff7) 	    | \
			SCRRIGHT (*char8++, xoff8);
#endif /* LONG_BIT */
#endif


#if GLYPHPADBYTES == 1
typedef	unsigned char	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 2
typedef unsigned short	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 4
typedef unsigned int	*glyphPointer;
#endif

#ifdef USE_LEFTBITS
#define GetBits1    getleftbits (char1, widthGlyph, c); \
		    c &= glyphMask; \
		    char1 = (glyphPointer) (((char *) char1) + glyphBytes);
#else
#define GetBits1    c = *char1++;
#endif

void
MFBTEGLYPHBLT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    FontPtr	pfont = pGC->font;
    int widthDst;
    unsigned long *pdstBase;	/* pointer to longword with top row 
				   of current glyph */

    int h;			/* height of glyph and char */
    register int xpos;		/* current x  */
    int ypos;			/* current y */
    int widthGlyph;

    int hTmp;			/* counter for height */
    register long startmask, endmask;
    int nfirst;			/* used if glyphs spans a longword boundary */
    BoxRec bbox;		/* for clipping */
    int	widthGlyphs;
    register unsigned long  *dst;
    register unsigned long  c;
    register int	    xoff1, xoff2, xoff3, xoff4;
    register glyphPointer   char1, char2, char3, char4;
#if LONG_BIT == 64
    register int	    xoff5, xoff6, xoff7, xoff8;
    register glyphPointer   char5, char6, char7, char8;
#endif /* LONG_BIT */

#ifdef USE_LEFTBITS
    register unsigned long  glyphMask;
    register unsigned long  tmpSrc;
    register int	    glyphBytes;
#endif

    if (!(pGC->planemask & 1))
	return;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) /
		      sizeof(long);
    }
    else
    {
	pdstBase = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) / sizeof(long);
    }

    xpos = x + pDrawable->x;
    ypos = y + pDrawable->y;

    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);

    xpos += FONTMAXBOUNDS(pfont,leftSideBearing);
    ypos -= FONTASCENT(pfont);

    bbox.x1 = xpos;
    bbox.x2 = xpos + (widthGlyph * nglyph);
    bbox.y1 = ypos;
    bbox.y2 = ypos + h;

    switch ((*pGC->pScreen->RectIn)(
                ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnPART:
	/* this is the WRONG thing to do, but it works.
	   calling the non-terminal text is easy, but slow, given
	   what we know about the font.

	   the right thing to do is something like:
	    for each clip rectangle
		compute at which row the glyph starts to be in it,
		   and at which row the glyph ceases to be in it
		compute which is the first glyph inside the left
		    edge, and the last one inside the right edge
		draw a fractional first glyph, using only
		    the rows we know are in
		draw all the whole glyphs, using the appropriate rows
		draw any pieces of the last glyph, using the right rows

	   this way, the code would take advantage of knowing that
	   all glyphs are the same height and don't overlap.

	   one day...
	*/
	CLIPTETEXT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
      case rgnOUT:
	return;
    }
    pdstBase += widthDst * ypos;
    widthGlyphs = widthGlyph * sizeof(long);

#ifdef USE_LEFTBITS
    glyphMask = endtab[widthGlyph];
    glyphBytes = GLYPHWIDTHBYTESPADDED(*ppci);
#endif

    if (nglyph >= sizeof(long) && widthGlyphs <= PPW)
    {
	while (nglyph >= sizeof(long))
	{
	    nglyph -= sizeof(long);
	    xoff1 = xpos & PIM;
	    xoff2 = widthGlyph;
	    xoff3 = xoff2 + widthGlyph;
	    xoff4 = xoff3 + widthGlyph;
#if LONG_BIT == 64
	    xoff5 = xoff4 + widthGlyph;
	    xoff6 = xoff5 + widthGlyph;
	    xoff7 = xoff6 + widthGlyph;
	    xoff8 = xoff7 + widthGlyph;
#endif /* LONG_BIT */
	    char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char2 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char3 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char4 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
#if LONG_BIT == 64
	    char5 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char6 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char7 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	    char8 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
#endif /* LONG_BIT */

	    hTmp = h;
	    dst = pdstBase + (xpos >> PWSH);

#ifndef FASTCHARS
	    if (xoff1 + widthGlyphs <= PPW)
	    {
		maskpartialbits (xoff1, widthGlyphs, startmask);
#endif
		while (hTmp--)
		{
		    GetBits4
#ifdef FASTCHARS
# if BITMAP_BIT_ORDER == MSBFirst
		    c >>= PPW - widthGlyphs;
# endif
		    FASTPUTBITS(OP(c), xoff1, widthGlyphs, dst);
#else
		    *dst = *dst & ~startmask | OP(SCRRIGHT(c, xoff1)) & startmask;
#endif
		    dst += widthDst;
		}
#ifndef FASTCHARS
	    }
	    else
	    {
		maskPPWbits (xoff1, widthGlyphs, startmask, endmask);
		nfirst = PPW - xoff1;
		while (hTmp--)
		{
		    GetBits4
		    dst[0] = dst[0] & ~startmask |
			     OP(SCRRIGHT(c,xoff1)) & startmask;
		    dst[1] = dst[1] & ~endmask |
			     OP(SCRLEFT(c,nfirst)) & endmask;
		    dst += widthDst;
		}
	    }
#endif
	    xpos += widthGlyphs;
	}
    }

    while(nglyph--)
    {
	xoff1 = xpos & PIM;
	char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase,(*ppci++));
	hTmp = h;
	dst = pdstBase + (xpos >> PWSH);

#ifndef FASTCHARS
	if (xoff1 + widthGlyph <= PPW)
	{
	    maskpartialbits (xoff1, widthGlyph, startmask);
#endif
	    while (hTmp--)
	    {
#ifdef FASTCHARS
#ifdef USE_LEFTBITS
		FASTGETBITS (char1,0,widthGlyph,c);
		char1 = (glyphPointer) (((char *) char1) + glyphBytes);
#else
		c = *char1++;
#if BITMAP_BIT_ORDER == MSBFirst
		c >>= PPW - widthGlyph;
#endif
#endif
		FASTPUTBITS (OP(c),xoff1,widthGlyph,dst);
#else
		GetBits1
		*dst = *dst & ~startmask | OP(SCRRIGHT(c, xoff1)) & startmask;
#endif
		dst += widthDst;
	    }
#ifndef FASTCHARS
	}
	else
	{
	    maskPPWbits (xoff1, widthGlyph, startmask, endmask);
	    nfirst = PPW - xoff1;
	    while (hTmp--)
	    {
		GetBits1
		dst[0] = dst[0] & ~startmask |
			 OP(SCRRIGHT(c,xoff1)) & startmask;
		dst[1] = dst[1] & ~endmask |
			 OP(SCRLEFT(c,nfirst)) & endmask;
		dst += widthDst;
	    }
	}
#endif
	xpos += widthGlyph;
    }
}
