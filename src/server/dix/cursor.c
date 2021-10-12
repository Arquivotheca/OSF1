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


/* $XConsortium: cursor.c,v 1.41 91/12/23 12:08:35 keith Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "dixstruct.h"
#include "cursorstr.h"
#include "dixfontstr.h"
#include "opaque.h"

typedef struct _GlyphShare {
    FontPtr font;
    unsigned short sourceChar;
    unsigned short maskChar;
    CursorBitsPtr bits;
    struct _GlyphShare *next;
} GlyphShare, *GlyphSharePtr;

static GlyphSharePtr sharedGlyphs = (GlyphSharePtr)NULL;

static void
FreeCursorBits(bits)
    CursorBitsPtr bits;
{
    if (--bits->refcnt > 0)
	return;
    xfree(bits->source);
    xfree(bits->mask);
    if (bits->refcnt == 0)
    {
	register GlyphSharePtr *prev, this;

	for (prev = &sharedGlyphs;
	     (this = *prev) && (this->bits != bits);
	     prev = &this->next)
	    ;
	if (this)
	{
	    *prev = this->next;
	    CloseFont(this->font, (Font)0);
	    xfree(this);
	}
	xfree(bits);
    }
}

/*
 * To be called indirectly by DeleteResource; must use exactly two args
 */
/*ARGSUSED*/
int
FreeCursor( pCurs, cid)
    CursorPtr 	pCurs;
    Cursor 	cid;	
{
    int		nscr;

    ScreenPtr	pscr;

    if ( --pCurs->refcnt > 0)
	return(Success);

    for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
    {
	pscr = screenInfo.screens[nscr];
	(void)( *pscr->UnrealizeCursor)( pscr, pCurs);
    }
    FreeCursorBits(pCurs->bits);
    xfree( pCurs);
    return(Success);
}

/*
 * does nothing about the resource table, just creates the data structure.
 * does not copy the src and mask bits
 */
CursorPtr 
AllocCursor(psrcbits, pmaskbits, cm,
	    foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue)
    unsigned char *	psrcbits;		/* server-defined padding */
    unsigned char *	pmaskbits;		/* server-defined padding */
    CursorMetricPtr	cm;
    unsigned		foreRed, foreGreen, foreBlue;
    unsigned		backRed, backGreen, backBlue;
{
    CursorBitsPtr  bits;
    CursorPtr 	pCurs;
    int		nscr;
    ScreenPtr 	pscr;

    pCurs = (CursorPtr)xalloc(sizeof(CursorRec) + sizeof(CursorBits));
    if (!pCurs)
	return (CursorPtr)NULL;
    bits = (CursorBitsPtr)((char *)pCurs + sizeof(CursorRec));
    bits->source = psrcbits;
    bits->mask = pmaskbits;
    bits->width = cm->width;
    bits->height = cm->height;
    bits->xhot = cm->xhot;
    bits->yhot = cm->yhot;
    bits->refcnt = -1;

    pCurs->bits = bits;
    pCurs->refcnt = 1;		

    pCurs->foreRed = foreRed;
    pCurs->foreGreen = foreGreen;
    pCurs->foreBlue = foreBlue;

    pCurs->backRed = backRed;
    pCurs->backGreen = backGreen;
    pCurs->backBlue = backBlue;

    /*
     * realize the cursor for every screen
     */
    for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
    {
	pscr = screenInfo.screens[nscr];
        if (!( *pscr->RealizeCursor)( pscr, pCurs))
	{
	    while (--nscr >= 0)
	    {
		pscr = screenInfo.screens[nscr];
		( *pscr->UnrealizeCursor)( pscr, pCurs);
	    }
	    xfree(pCurs);
	    return (CursorPtr)NULL;
	}
    }
    return pCurs;
}

int
AllocGlyphCursor(source, sourceChar, mask, maskChar,
		 foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue,
		 ppCurs, client)
    Font source, mask;
    unsigned short sourceChar, maskChar;
    unsigned foreRed, foreGreen, foreBlue;
    unsigned backRed, backGreen, backBlue;
    CursorPtr *ppCurs;
    ClientPtr client;
{
    FontPtr  sourcefont, maskfont;
    unsigned char   *srcbits;
    unsigned char   *mskbits;
    CursorMetricRec cm;
    int res;
    CursorBitsPtr  bits;
    CursorPtr 	pCurs;
    int		nscr;
    ScreenPtr 	pscr;
    GlyphSharePtr pShare;

    sourcefont = (FontPtr) LookupIDByType(source, RT_FONT);
    maskfont = (FontPtr) LookupIDByType(mask, RT_FONT);

    if (!sourcefont)
    {
	client->errorValue = source;
	return(BadFont);
    }
    if (!maskfont && (mask != None))
    {
	client->errorValue = mask;
	return(BadFont);
    }
    if (sourcefont != maskfont)
	pShare = (GlyphSharePtr)NULL;
    else
    {
	for (pShare = sharedGlyphs;
	     pShare &&
	     ((pShare->font != sourcefont) ||
	      (pShare->sourceChar != sourceChar) ||
	      (pShare->maskChar != maskChar));
	     pShare = pShare->next)
	    ;
    }
    if (pShare)
    {
	pCurs = (CursorPtr)xalloc(sizeof(CursorRec));
	if (!pCurs)
	    return BadAlloc;
	bits = pShare->bits;
	bits->refcnt++;
    }
    else
    {
	if (!CursorMetricsFromGlyph(sourcefont, sourceChar, &cm))
	{
	    client->errorValue = sourceChar;
	    return BadValue;
	}
	if (!maskfont)
	{
	    register long n;
	    register unsigned char *bits;

	    n = BitmapBytePad(cm.width)*(long)cm.height;
	    bits = mskbits = (unsigned char *)xalloc(n);
	    if (!bits)
		return BadAlloc;
	    while (--n >= 0)
		*bits++ = ~0;
	}
	else
	{
	    if (!CursorMetricsFromGlyph(maskfont, maskChar, &cm))
	    {
		client->errorValue = maskChar;
		return BadValue;
	    }
	    if (res = ServerBitsFromGlyph(maskfont, maskChar, &cm, &mskbits))
		return res;
	}
	if (res = ServerBitsFromGlyph(sourcefont, sourceChar, &cm, &srcbits))
	{
	    xfree(mskbits);
	    return res;
	}
	if (sourcefont != maskfont)
	{
	    pCurs = (CursorPtr)xalloc(sizeof(CursorRec) + sizeof(CursorBits));
	    if (pCurs)
		bits = (CursorBitsPtr)((char *)pCurs + sizeof(CursorRec));
	    else
		bits = (CursorBitsPtr)NULL;
	}
	else
	{
	    pCurs = (CursorPtr)xalloc(sizeof(CursorRec));
	    if (pCurs)
		bits = (CursorBitsPtr)xalloc(sizeof(CursorBits));
	    else
		bits = (CursorBitsPtr)NULL;
	}
	if (!bits)
	{
	    xfree(pCurs);
	    xfree(mskbits);
	    xfree(srcbits);
	    return BadAlloc;
	}
	bits->source = srcbits;
	bits->mask = mskbits;
	bits->width = cm.width;
	bits->height = cm.height;
	bits->xhot = cm.xhot;
	bits->yhot = cm.yhot;
	if (sourcefont != maskfont)
	    bits->refcnt = -1;
	else
	{
	    bits->refcnt = 1;
	    pShare = (GlyphSharePtr)xalloc(sizeof(GlyphShare));
	    if (!pShare)
	    {
		FreeCursorBits(bits);
		return BadAlloc;
	    }
	    pShare->font = sourcefont;
	    sourcefont->refcnt++;
	    pShare->sourceChar = sourceChar;
	    pShare->maskChar = maskChar;
	    pShare->bits = bits;
	    pShare->next = sharedGlyphs;
	    sharedGlyphs = pShare;
	}
    }
    pCurs->bits = bits;
    pCurs->refcnt = 1;

    pCurs->foreRed = foreRed;
    pCurs->foreGreen = foreGreen;
    pCurs->foreBlue = foreBlue;

    pCurs->backRed = backRed;
    pCurs->backGreen = backGreen;
    pCurs->backBlue = backBlue;

    /*
     * realize the cursor for every screen
     */
    for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
    {
	pscr = screenInfo.screens[nscr];
        if (!( *pscr->RealizeCursor)( pscr, pCurs))
	{
	    while (--nscr >= 0)
	    {
		pscr = screenInfo.screens[nscr];
		( *pscr->UnrealizeCursor)( pscr, pCurs);
	    }
	    FreeCursorBits(pCurs->bits);
	    xfree(pCurs);
	    return BadAlloc;
	}
    }
    *ppCurs = pCurs;
    return Success;
}

/***********************************************************
 * CreateRootCursor
 *
 * look up the name of a font
 * open the font
 * add the font to the resource table
 * make a cursor from the glyphs
 * add the cursor to the resource table
 *************************************************************/

CursorPtr 
CreateRootCursor(pfilename, glyph)
    char *		pfilename;
    unsigned short	glyph;
{
    CursorPtr 	curs;
    FontPtr 	cursorfont;
    int	err;
    XID		fontID;

    fontID = FakeClientID(0);
    err = OpenFont(serverClient, fontID, FontLoadAll | FontOpenSync,
	(unsigned)strlen( pfilename), pfilename);
    if (err != Success)
	return NullCursor;

    cursorfont = (FontPtr)LookupIDByType(fontID, RT_FONT);
    if (!cursorfont)
	return NullCursor;
    if (AllocGlyphCursor(fontID, glyph, fontID, glyph + 1,
			 0, 0, 0, ~0, ~0, ~0, &curs, serverClient) != Success)
	return NullCursor;

    if (!AddResource(FakeClientID(0), RT_CURSOR, (pointer)curs))
	return NullCursor;

    return curs;
}
