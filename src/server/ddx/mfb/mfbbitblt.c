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
/* $XConsortium: mfbbitblt.c,v 5.21 91/07/02 19:54:31 keith Exp $ */
#include "X.h"
#include "Xprotostr.h"

#include "miscstruct.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mi.h"

#include "mfb.h"
#include "maskbits.h"


/* CopyArea and CopyPlane for a monchrome frame buffer


    clip the source rectangle to the source's available bits.  (this
avoids copying unnecessary pieces that will just get exposed anyway.)
this becomes the new shape of the destination.
    clip the destination region to the composite clip in the
GC.  this requires translating the destination region to (dstx, dsty).
    build a list of source points, one for each rectangle in the
destination.  this is a simple translation.
    go do the multiple rectangle copies
    do graphics exposures
*/
/** Optimized for drawing pixmaps into windows, especially when drawing into
 ** unobscured windows.  Calls to the general-purpose region code were
 ** replaced with rectangle-to-rectangle clipping comparisions.  This is
 ** possible, since the pixmap is a single rectangle.  In an unobscured
 ** window, the destination clip is also a single rectangle, and region
 ** code can be avoided entirely.  This is a big savings, since the region
 ** code uses XAlloc() and makes many function calls.
 **
 ** In addition, if source is a pixmap, there is no need to call the
 ** expensive miHandleExposures() routine.  Instead, we simply return NULL.
 **
 ** Previously, drawing a pixmap into an unobscured window executed at least
 ** 8 XAlloc()'s, 30 function calls, and hundreds of lines of code.
 **
 ** Now, the same operation requires no XAlloc()'s, no region function calls,
 ** and much less overhead.  Nice for drawing lots of small pixmaps.
 */
 
extern int  mfbDoBitbltCopy();
extern int  mfbDoBitbltXor();
extern int  mfbDoBitbltCopyInverted();
extern int  mfbDoBitbltOr();
extern int  mfbDoBitbltGeneral();

mfbDoBitblt (pSrc, pDst, alu, prgnDst, pptSrc)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
{
    switch (alu)
    {
    case GXcopy:
	return mfbDoBitbltCopy (pSrc, pDst, alu, prgnDst, pptSrc);
    case GXxor:
	return mfbDoBitbltXor (pSrc, pDst, alu, prgnDst, pptSrc);
    case GXcopyInverted:
	return mfbDoBitbltCopyInverted (pSrc, pDst, alu, prgnDst, pptSrc);
    case GXor:
	return mfbDoBitbltOr (pSrc, pDst, alu, prgnDst, pptSrc);
    default:
	return mfbDoBitbltGeneral (pSrc, pDst, alu, prgnDst, pptSrc);
    }
}

RegionPtr
mfbCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty)
register DrawablePtr pSrcDrawable;
register DrawablePtr pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
{
    RegionPtr prgnSrcClip;	/* may be a new region, or just a copy */
    Bool freeSrcClip = FALSE;

    RegionPtr prgnExposed;
    RegionRec rgnDst;
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    register BoxPtr pbox;
    int i;
    register int dx;
    register int dy;
    xRectangle origSource;
    DDXPointRec origDest;
    int numRects;
    BoxRec fastBox;
    int fastClip = 0;		/* for fast clipping with pixmap source */
    int fastExpose = 0;		/* for fast exposures with pixmap source */
    int (*localDoBitBlt)();

    origSource.x = srcx;
    origSource.y = srcy;
    origSource.width = width;
    origSource.height = height;
    origDest.x = dstx;
    origDest.y = dsty;

    if ((pSrcDrawable != pDstDrawable) &&
	pSrcDrawable->pScreen->SourceValidate)
    {
	(*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, srcx, srcy, width, height);
    }

    switch (pGC->alu) {
    case GXcopy:
	localDoBitBlt = mfbDoBitbltCopy;
	break;
    case GXcopyInverted:
	localDoBitBlt = mfbDoBitbltCopyInverted;
	break;
    case GXxor:
	localDoBitBlt = mfbDoBitbltXor;
	break;
    case GXor:
	localDoBitBlt = mfbDoBitbltOr;
	break;
    default:
	localDoBitBlt = mfbDoBitbltGeneral;
	break;
    }

    srcx += pSrcDrawable->x;
    srcy += pSrcDrawable->y;

    /* clip the source */

    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	if ((pSrcDrawable == pDstDrawable) &&
	    (pGC->clientClipType == CT_NONE))
	{
	    prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	}
	else
	{
	    fastClip = 1;
	}
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors)
	{
	    if (!((WindowPtr) pSrcDrawable)->parent)
	    {
		/*
		 * special case bitblt from root window in
		 * IncludeInferiors mode; just like from a pixmap
		 */
		fastClip = 1;
	    }
	    else if ((pSrcDrawable == pDstDrawable) &&
		(pGC->clientClipType == CT_NONE))
	    {
		prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	    }
	    else
	    {
		prgnSrcClip = NotClippedByChildren((WindowPtr)pSrcDrawable);
		freeSrcClip = TRUE;
	    }
	}
	else
	{
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
	}
    }

    fastBox.x1 = srcx;
    fastBox.y1 = srcy;
    fastBox.x2 = srcx + width;
    fastBox.y2 = srcy + height;

    /* Don't create a source region if we are doing a fast clip */
    if (fastClip)
    {
	fastExpose = 1;
	/*
	 * clip the source; if regions extend beyond the source size,
 	 * make sure exposure events get sent
	 */
	if (fastBox.x1 < pSrcDrawable->x)
	{
	    fastBox.x1 = pSrcDrawable->x;
	    fastExpose = 0;
	}
	if (fastBox.y1 < pSrcDrawable->y)
	{
	    fastBox.y1 = pSrcDrawable->y;
	    fastExpose = 0;
	}
	if (fastBox.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	{
	    fastBox.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	    fastExpose = 0;
	}
	if (fastBox.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	{
	    fastBox.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
	    fastExpose = 0;
	}
    }
    else
    {
	(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	(*pGC->pScreen->Intersect)(&rgnDst, &rgnDst, prgnSrcClip);
    }

    dstx += pDstDrawable->x;
    dsty += pDstDrawable->y;

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	if (!((WindowPtr)pDstDrawable)->realized)
	{
	    if (!fastClip)
		(*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
    }

    dx = srcx - dstx;
    dy = srcy - dsty;

    /* Translate and clip the dst to the destination composite clip */
    if (fastClip)
    {
	RegionPtr cclip;

        /* Translate the region directly */
        fastBox.x1 -= dx;
        fastBox.x2 -= dx;
        fastBox.y1 -= dy;
        fastBox.y2 -= dy;

	/* If the destination composite clip is one rectangle we can
	   do the clip directly.  Otherwise we have to create a full
	   blown region and call intersect */
	cclip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
        if (REGION_NUM_RECTS(cclip) == 1)
        {
	    BoxPtr pBox = REGION_RECTS(cclip);

	    if (fastBox.x1 < pBox->x1) fastBox.x1 = pBox->x1;
	    if (fastBox.x2 > pBox->x2) fastBox.x2 = pBox->x2;
	    if (fastBox.y1 < pBox->y1) fastBox.y1 = pBox->y1;
	    if (fastBox.y2 > pBox->y2) fastBox.y2 = pBox->y2;

	    /* Check to see if the region is empty */
	    if (fastBox.x1 >= fastBox.x2 || fastBox.y1 >= fastBox.y2)
		(*pGC->pScreen->RegionInit)(&rgnDst, NullBox, 0);
	    else
		(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	}
        else
	{
	    /* We must turn off fastClip now, since we must create
	       a full blown region.  It is intersected with the
	       composite clip below. */
	    fastClip = 0;
	    (*pGC->pScreen->RegionInit)(&rgnDst, &fastBox,1);
	}
    }
    else
    {
        (*pGC->pScreen->TranslateRegion)(&rgnDst, -dx, -dy);
    }

    if (!fastClip)
    {
	(*pGC->pScreen->Intersect)(&rgnDst,
				   &rgnDst,
				 ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    }

    /* Do bit blitting */
    numRects = REGION_NUM_RECTS(&rgnDst);
    if (numRects && width && height)
    {
	if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(numRects *
						  sizeof(DDXPointRec))))
	{
	    (*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
	pbox = REGION_RECTS(&rgnDst);
	ppt = pptSrc;
	for (i = numRects; --i >= 0; pbox++, ppt++)
	{
	    ppt->x = pbox->x1 + dx;
	    ppt->y = pbox->y1 + dy;
	}
    
	if (pGC->planemask & 1)
	    (*localDoBitBlt) (pSrcDrawable, pDstDrawable, pGC->alu, &rgnDst, pptSrc);

	DEALLOCATE_LOCAL(pptSrc);
    }

    prgnExposed = NULL;
    if (((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->fExpose) 
    {
        /* Pixmap sources generate a NoExposed (we return NULL to do this) */
        if (!fastExpose)
	    prgnExposed =
		miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
				  origSource.x, origSource.y,
				  (int)origSource.width,
				  (int)origSource.height,
				  origDest.x, origDest.y, (unsigned long)0L);
    }
    (*pGC->pScreen->RegionUninit)(&rgnDst);
    if (freeSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
    return prgnExposed;
}

/*
 * Devices which use mfb for 1-bit pixmap support
 * must register a function for n-to-1 copy operations
 */

static unsigned long	copyPlaneGeneration;
static int		copyPlaneScreenIndex = -1;

Bool
mfbRegisterCopyPlaneProc (pScreen, proc)
    ScreenPtr	pScreen;
    RegionPtr	(*proc)();
{
    if (copyPlaneGeneration != serverGeneration)
    {
	copyPlaneScreenIndex = AllocateScreenPrivateIndex();
	if (copyPlaneScreenIndex < 0)
	    return FALSE;
	copyPlaneGeneration = serverGeneration;
    }
    pScreen->devPrivates[copyPlaneScreenIndex].ptr = (pointer) proc;
    return TRUE;
}

/*
    if fg == 1 and bg ==0, we can do an ordinary CopyArea.
    if fg == bg, we can do a CopyArea with alu = mfbReduceRop(alu, fg)
    if fg == 0 and bg == 1, we use the same rasterop, with
	source operand inverted.

    CopyArea deals with all of the graphics exposure events.
    This code depends on knowing that we can change the
alu in the GC without having to call ValidateGC() before calling
CopyArea().

*/

RegionPtr
mfbCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, plane)
DrawablePtr pSrcDrawable, pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
unsigned long plane;
{
    int alu;
    RegionPtr	prgnExposed;
    RegionPtr	(*copyPlane)();

    if (pSrcDrawable->depth != 1)
    {
	if (copyPlaneScreenIndex >= 0 &&
	    (copyPlane = (RegionPtr (*)()) 
		pSrcDrawable->pScreen->devPrivates[copyPlaneScreenIndex].ptr)
	    )
	{
	    return (*copyPlane) (pSrcDrawable, pDstDrawable,
			   pGC, srcx, srcy, width, height, dstx, dsty, plane);
	}  
	else
	{
	    FatalError ("No copyPlane proc registered for depth %d\n",
			pSrcDrawable->depth);
	}
    }
    if (plane != 1)
	return NULL;

    if ((pGC->fgPixel & 1L) == 1L && (pGC->bgPixel & 1L) == 0L)
    {
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else if ((pGC->fgPixel & 1L) == (pGC->bgPixel & 1L))
    {
	alu = pGC->alu;
	pGC->alu = mfbReduceRop(pGC->alu, pGC->fgPixel);
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
    else /* need to invert the src */
    {
	alu = pGC->alu;
	pGC->alu = InverseAlu[alu];
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
    return prgnExposed;
}
