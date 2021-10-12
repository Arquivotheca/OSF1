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
/* $XConsortium: mfbimage.c,v 5.3 89/09/14 16:26:42 rws Exp $ */

#include "X.h"

#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"

#include "mfb.h"
#include "mi.h"
#include "Xmd.h"

#include "maskbits.h"

#include "servermd.h"

/* Put and Get images on a monochrome frame buffer
 *
 *   we do this by creating a temporary pixmap and making its
 * pointer to bits point to the buffer read in from the client.
 * this works because of the padding rules specified at startup
 *
 * Note that CopyArea must know how to copy a bitmap into the server-format
 * temporary pixmap.
 *
 * For speed, mfbPutImage should allocate the temporary pixmap on the stack.
 *
 *     even though an XYBitmap and an XYPixmap have the same
 * format (for this device), PutImage has different semantics for the
 * two.  XYPixmap just does the copy; XYBitmap takes gc.fgPixel for
 * a 1 bit, gc.bgPixel for a 0 bit, which we notice is exactly
 * like CopyPlane.
 *
 *   written by drewry, september 1986
 */


/*ARGSUSED*/
void
mfbPutImage(dst, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, x, y, w, h;
    int leftPad;
    unsigned int format;
    int 	*pImage;
{
    PixmapRec	FakePixmap;

    if (!(pGC->planemask & 1))
	return;

    /* 0 may confuse CreatePixmap, and will sometimes be
       passed by the mi text code
    */
    if ((w == 0) || (h == 0))
	return;

    FakePixmap.drawable.type = DRAWABLE_PIXMAP;
    FakePixmap.drawable.class = 0;
    FakePixmap.drawable.pScreen = dst->pScreen;
    FakePixmap.drawable.depth = 1;
    FakePixmap.drawable.bitsPerPixel = 1;
    FakePixmap.drawable.id = 0;
    FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    FakePixmap.drawable.x = 0;
    FakePixmap.drawable.y = 0;
    FakePixmap.drawable.width = w+leftPad;
    FakePixmap.drawable.height = h;
    FakePixmap.devKind = PixmapBytePad(FakePixmap.drawable.width, 1);
    FakePixmap.refcnt = 1;
    FakePixmap.devPrivate.ptr = (pointer)pImage;
    ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->fExpose = FALSE;
    if (format != XYBitmap) {
	(*pGC->ops->CopyArea)(&FakePixmap, dst, pGC, leftPad, 0, w, h, x, y);
    }
    else {
	(*pGC->ops->CopyPlane)(&FakePixmap, dst, pGC, leftPad, 0, 
		w, h, x, y, (long) 1);
    }
    ((mfbPrivGC*)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->fExpose = TRUE;
}


/*
 * pdstLine points to space allocated by caller, which he can do since
 * he knows dimensions of the pixmap
 * we can call mfbDoBitblt because the dispatcher has promised not to send us
 * anything that would require going over the edge of the screen.
 *
 *	XYPixmap and ZPixmap are the same for mfb.
 *	For any planemask with bit 0 == 0, just fill the dst with 0.
 */
/*ARGSUSED*/
void
mfbGetImage( pDrawable, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr pDrawable;
    int		sx, sy, w, h;
    unsigned int format;
    unsigned long planeMask;
    pointer	pdstLine;
{
    PixmapRec FakePixmap;
    BoxRec box;
    DDXPointRec ptSrc;
    RegionRec rgnDst;

    if (planeMask & 0x1)
    {
	FakePixmap.drawable.type = DRAWABLE_PIXMAP;
	FakePixmap.drawable.class = 0;
	FakePixmap.drawable.pScreen = pDrawable->pScreen;
	FakePixmap.drawable.depth = 1;
	FakePixmap.drawable.bitsPerPixel = 1;
	FakePixmap.drawable.id = 0;
	FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
	FakePixmap.drawable.x = 0;
	FakePixmap.drawable.y = 0;
	FakePixmap.drawable.width = w;
	FakePixmap.drawable.height = h;
	FakePixmap.devKind = PixmapBytePad(w, 1);
	FakePixmap.refcnt = 1;
        FakePixmap.devPrivate.ptr = (pointer)pdstLine;
        ptSrc.x = sx + pDrawable->x;
        ptSrc.y = sy + pDrawable->y;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        (*pDrawable->pScreen->RegionInit)(&rgnDst, &box, 1);
        mfbDoBitblt(pDrawable, (DrawablePtr)&FakePixmap,
		    GXcopy, &rgnDst, &ptSrc);
        (*pDrawable->pScreen->RegionUninit)(&rgnDst);

    }
    else
    {
	bzero((char *)pdstLine, PixmapBytePad(w, 1) * h);
    }
}
