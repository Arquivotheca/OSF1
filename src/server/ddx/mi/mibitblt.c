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
/* $XConsortium: mibitblt.c,v 5.15 91/12/18 18:52:23 keith Exp $ */
/* Author: Todd Newman  (aided and abetted by Mr. Drewry) */

#include "X.h"
#include "Xprotostr.h"

#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "regionstr.h"
#include "Xmd.h"
#include "servermd.h"

/* MICOPYAREA -- public entry for the CopyArea request 
 * For each rectangle in the source region
 *     get the pixels with GetSpans
 *     set them in the destination with SetSpans
 * We let SetSpans worry about clipping to the destination.
 */
RegionPtr
miCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, xIn, yIn, widthSrc, heightSrc, xOut, yOut)
    register DrawablePtr 	pSrcDrawable;
    register DrawablePtr 	pDstDrawable;
    GCPtr 			pGC;
    int 			xIn, yIn;
    int 			widthSrc, heightSrc;
    int 			xOut, yOut;
{
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    BoxRec 		srcBox, *prect;
    			/* may be a new region, or just a copy */
    RegionPtr 		prgnSrcClip;
    			/* non-0 if we've created a src clip */
    RegionPtr		prgnExposed;
    int 		realSrcClip = 0;
    int			srcx, srcy, dstx, dsty, i, j, y, width, height,
    			xMin, xMax, yMin, yMax;
    unsigned int	*ordering;
    int			numRects;
    BoxPtr		boxes;

    srcx = xIn + pSrcDrawable->x;
    srcy = yIn + pSrcDrawable->y;

    /* If the destination isn't realized, this is easy */
    if (pDstDrawable->type == DRAWABLE_WINDOW &&
	!((WindowPtr)pDstDrawable)->realized)
	return (RegionPtr)NULL;

    /* clip the source */
    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	BoxRec box;

	box.x1 = pSrcDrawable->x;
	box.y1 = pSrcDrawable->y;
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;

	prgnSrcClip = (*pGC->pScreen->RegionCreate)(&box, 1);
	realSrcClip = 1;
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors) {
	    prgnSrcClip = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    realSrcClip = 1;
	} else
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
    }

    /* If the src drawable is a window, we need to translate the srcBox so
     * that we can compare it with the window's clip region later on. */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx  + widthSrc;
    srcBox.y2 = srcy  + heightSrc;

    dstx = xOut;
    dsty = yOut;
    if (pGC->miTranslate)
    {
	dstx += pDstDrawable->x;
	dsty += pDstDrawable->y;
    }

    pptFirst = ppt = (DDXPointPtr)
        ALLOCATE_LOCAL(heightSrc * sizeof(DDXPointRec));
    pwidthFirst = pwidth = (unsigned int *)
        ALLOCATE_LOCAL(heightSrc * sizeof(unsigned int));
    numRects = REGION_NUM_RECTS(prgnSrcClip);
    boxes = REGION_RECTS(prgnSrcClip);
    ordering = (unsigned int *)
        ALLOCATE_LOCAL(numRects * sizeof(unsigned int));
    if(!pptFirst || !pwidthFirst || !ordering)
    {
       if (ordering)
	   DEALLOCATE_LOCAL(ordering);
       if (pwidthFirst)
           DEALLOCATE_LOCAL(pwidthFirst);
       if (pptFirst)
           DEALLOCATE_LOCAL(pptFirst);
       return (RegionPtr)NULL;
    }

    /* If not the same drawable then order of move doesn't matter.
       Following assumes that boxes are sorted from top
       to bottom and left to right.
    */
    if ((pSrcDrawable != pDstDrawable) &&
	((pGC->subWindowMode != IncludeInferiors) ||
	 (pSrcDrawable->type == DRAWABLE_PIXMAP) ||
	 (pDstDrawable->type == DRAWABLE_PIXMAP)))
      for (i=0; i < numRects; i++)
        ordering[i] = i;
    else { /* within same drawable, must sequence moves carefully! */
      if (dsty <= srcBox.y1) { /* Scroll up or stationary vertical.
                                  Vertical order OK */
        if (dstx <= srcBox.x1) /* Scroll left or stationary horizontal.
                                  Horizontal order OK as well */
          for (i=0; i < numRects; i++)
            ordering[i] = i;
        else { /* scroll right. must reverse horizontal banding of rects. */
          for (i=0, j=1, xMax=0; i < numRects; j=i+1, xMax=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j < numRects) && (boxes[j].y1 == y))
              j++;
            /* reverse the horizontal band in the output ordering */
            for (j-- ; j >= xMax; j--, i++)
              ordering[i] = j;
          }
        }
      }
      else { /* Scroll down. Must reverse vertical banding. */
        if (dstx < srcBox.x1) { /* Scroll left. Horizontal order OK. */
          for (i=numRects-1, j=i-1, yMin=i, yMax=0;
              i >= 0;
              j=i-1, yMin=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j >= 0) && (boxes[j].y1 == y))
              j--;
            /* reverse the horizontal band in the output ordering */
            for (j++ ; j <= yMin; j++, i--, yMax++)
              ordering[yMax] = j;
          }
        }
        else /* Scroll right or horizontal stationary.
                Reverse horizontal order as well (if stationary, horizontal
                order can be swapped without penalty and this is faster
                to compute). */
          for (i=0, j=numRects-1; i < numRects; i++, j--)
              ordering[i] = j;
      }
    }
 
     for(i = 0; i < numRects; i++)
     {
        prect = &boxes[ordering[i]];
  	xMin = max(prect->x1, srcBox.x1);
  	xMax = min(prect->x2, srcBox.x2);
  	yMin = max(prect->y1, srcBox.y1);
	yMax = min(prect->y2, srcBox.y2);
	/* is there anything visible here? */
	if(xMax <= xMin || yMax <= yMin)
	    continue;

        ppt = pptFirst;
	pwidth = pwidthFirst;
	y = yMin;
	height = yMax - yMin;
	width = xMax - xMin;

	for(j = 0; j < height; j++)
	{
	    /* We must untranslate before calling GetSpans */
	    ppt->x = xMin;
	    ppt++->y = y++;
	    *pwidth++ = width;
	}
	pbits = (unsigned int *)xalloc(height * PixmapBytePad(width,
						     pSrcDrawable->depth));
	if (pbits)
	{
	    (*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, width, pptFirst,
					       pwidthFirst, height, pbits);
	    ppt = pptFirst;
	    pwidth = pwidthFirst;
	    xMin -= (srcx - dstx);
	    y = yMin - (srcy - dsty);
	    for(j = 0; j < height; j++)
	    {
		ppt->x = xMin;
		ppt++->y = y++;
		*pwidth++ = width;
	    }

	    (*pGC->ops->SetSpans)(pDstDrawable, pGC, pbits, pptFirst,
				  pwidthFirst, height, TRUE);
	    xfree(pbits);
	}
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
		      widthSrc, heightSrc, xOut, yOut, (unsigned long)0);
    if(realSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		
    DEALLOCATE_LOCAL(ordering);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);
    return prgnExposed;
}

/* MIGETPLANE -- gets a bitmap representing one plane of pDraw
 * A helper used for CopyPlane and XY format GetImage 
 * No clever strategy here, we grab a scanline at a time, pull out the
 * bits and then stuff them in a 1 bit deep map.
 */
static
unsigned long	*
miGetPlane(pDraw, planeNum, sx, sy, w, h, result)
    DrawablePtr		pDraw;
    int			planeNum;	/* number of the bitPlane */
    int			sx, sy, w, h;
    unsigned long	*result;
{
    int			i, j, k, width, bitsPerPixel, widthInBytes;
    DDXPointRec 	pt;
    unsigned long	pixel;
    unsigned long	bit;
    unsigned char	*pCharsOut;

#if BITMAP_SCANLINE_UNIT == 8
#define OUT_TYPE unsigned char
#endif
#if BITMAP_SCANLINE_UNIT == 16
#define OUT_TYPE CARD16
#endif
#if BITMAP_SCANLINE_UNIT == 32
#define OUT_TYPE CARD32
#endif
#if BITMAP_SCANLINE_UNIT == 64
#define OUT_TYPE CARD64
#endif

    OUT_TYPE		*pOut;
    int			delta;

    sx += pDraw->x;
    sy += pDraw->y;
    widthInBytes = BitmapBytePad(w);
    if(!result)
        result = (unsigned long *)xalloc(h * widthInBytes);
    if (!result)
	return (unsigned long *)NULL;
    bitsPerPixel = pDraw->bitsPerPixel;
    bzero((char *)result, h * widthInBytes);
    pOut = (OUT_TYPE *) result;
    if(bitsPerPixel == 1)
    {
	pCharsOut = (unsigned char *) result;
    	width = w;
    }
    else
    {
	delta = (widthInBytes / (BITMAP_SCANLINE_UNIT / 8)) -
	    (w / BITMAP_SCANLINE_UNIT);
	width = 1;
#if IMAGE_BYTE_ORDER == MSBFirst
	planeNum += (32 - bitsPerPixel);
#endif
    }
    pt.y = sy;
    for (i = h; --i >= 0; pt.y++)
    {
	pt.x = sx;
	if(bitsPerPixel == 1)
	{
	    (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					(unsigned long *)pCharsOut);
	    pCharsOut += widthInBytes;
	}
	else
	{
	    k = 0;
	    for(j = w; --j >= 0; pt.x++)
	    {
		/* Fetch the next pixel */
		(*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					    &pixel);
		/*
		 * Now get the bit and insert into a bitmap in XY format.
		 */
		bit = (pixel >> planeNum) & 1;
		if (k == BITMAP_SCANLINE_UNIT)
		{
		    pOut++;
		    k = 0;
		}
		/* XXX assuming bit order == byte order */
#if BITMAP_BIT_ORDER == LSBFirst
		bit <<= k;
#else
		bit <<= ((BITMAP_SCANLINE_UNIT - 1) - k);
#endif
		*pOut |= (OUT_TYPE) bit;
		k++;
	    }
	    pOut += delta;
	}
    }
    return(result);    

}

/* MIOPQSTIPDRAWABLE -- use pbits as an opaque stipple for pDraw.
 * Drawing through the clip mask we SetSpans() the bits into a 
 * bitmap and stipple those bits onto the destination drawable by doing a
 * PolyFillRect over the whole drawable, 
 * then we invert the bitmap by copying it onto itself with an alu of
 * GXinvert, invert the foreground/background colors of the gc, and draw
 * the background bits.
 * Note how the clipped out bits of the bitmap are always the background
 * color so that the stipple never causes FillRect to draw them.
 */
void
miOpqStipDrawable(pDraw, pGC, prgnSrc, pbits, srcx, w, h, dstx, dsty)
    DrawablePtr pDraw;
    GCPtr	pGC;
    RegionPtr	prgnSrc;
    unsigned long	*pbits;
    int		srcx, w, h, dstx, dsty;
{
    int		oldfill, i;
    unsigned long oldfg;
    int		*pwidth, *pwidthFirst;
    XID		gcv[6];
    PixmapPtr	pStipple, pPixmap;
    DDXPointRec	oldOrg;
    GCPtr	pGCT;
    DDXPointPtr ppt, pptFirst;
    xRectangle rect;
    RegionPtr	prgnSrcClip;

    pPixmap = (*pDraw->pScreen->CreatePixmap)
			   (pDraw->pScreen, w + srcx, h, 1);
    if (!pPixmap)
	return;

    /* Put the image into a 1 bit deep pixmap */
    pGCT = GetScratchGC(1, pDraw->pScreen);
    if (!pGCT)
    {
	(*pDraw->pScreen->DestroyPixmap)(pPixmap);
	return;
    }
    /* First set the whole pixmap to 0 */
    gcv[0] = 0;
    DoChangeGC(pGCT, GCBackground, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    miClearDrawable((DrawablePtr)pPixmap, pGCT);
    ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
    if(!pptFirst || !pwidthFirst)
    {
	if (pwidthFirst) DEALLOCATE_LOCAL(pwidthFirst);
	if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
	FreeScratchGC(pGCT);
	return;
    }

    /* we need a temporary region because ChangeClip must be assumed
       to destroy what it's sent.  note that this means we don't
       have to free prgnSrcClip ourselves.
    */
    prgnSrcClip = (*pGCT->pScreen->RegionCreate)(NULL, 0);
    (*pGCT->pScreen->RegionCopy)(prgnSrcClip, prgnSrc);
    (*pGCT->pScreen->TranslateRegion) (prgnSrcClip, srcx, 0);
    (*pGCT->funcs->ChangeClip)(pGCT, CT_REGION, prgnSrcClip, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);

    /* Since we know pDraw is always a pixmap, we never need to think
     * about translation here */
    for(i = 0; i < h; i++)
    {
	ppt->x = 0;
	ppt++->y = i;
	*pwidth++ = w + srcx;
    }

    (*pGCT->ops->SetSpans)(pPixmap, pGCT, pbits, pptFirst, pwidthFirst, h, TRUE);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);


    /* Save current values from the client GC */
    oldfill = pGC->fillStyle;
    pStipple = pGC->stipple;
    if(pStipple)
        pStipple->refcnt++;
    oldOrg = pGC->patOrg;

    /* Set a new stipple in the drawable */
    gcv[0] = FillStippled;
    gcv[1] = (long) pPixmap;
    gcv[2] = dstx - srcx;
    gcv[3] = dsty;

    DoChangeGC(pGC,
             GCFillStyle | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin,
	     gcv, 1);
    ValidateGC(pDraw, pGC);

    /* Fill the drawable with the stipple.  This will draw the
     * foreground color whereever 1 bits are set, leaving everything
     * with 0 bits untouched.  Note that the part outside the clip
     * region is all 0s.  */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Invert the tiling pixmap. This sets 0s for 1s and 1s for 0s, only
     * within the clipping region, the part outside is still all 0s */
    gcv[0] = GXinvert;
    DoChangeGC(pGCT, GCFunction, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    (*pGCT->ops->CopyArea)(pPixmap, pPixmap, pGCT, 0, 0, w + srcx, h, 0, 0);

    /* Swap foreground and background colors on the GC for the drawable.
     * Now when we fill the drawable, we will fill in the "Background"
     * values */
    oldfg = pGC->fgPixel;
    gcv[0] = (long) pGC->bgPixel;
    gcv[1] = (long) oldfg;
    gcv[2] = (long) pPixmap;
    DoChangeGC(pGC, GCForeground | GCBackground | GCStipple, gcv, 1); 
    ValidateGC(pDraw, pGC);
    /* PolyFillRect might have bashed the rectangle */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Now put things back */
    if(pStipple)
        pStipple->refcnt--;
    gcv[0] = (long) oldfg;
    gcv[1] = pGC->fgPixel;
    gcv[2] = oldfill;
    gcv[3] = (long) pStipple;
    gcv[4] = oldOrg.x;
    gcv[5] = oldOrg.y;
    DoChangeGC(pGC, 
        GCForeground | GCBackground | GCFillStyle | GCStipple | 
	GCTileStipXOrigin | GCTileStipYOrigin, gcv, 1);

    ValidateGC(pDraw, pGC);
    /* put what we hope is a smaller clip region back in the scratch gc */
    (*pGCT->funcs->ChangeClip)(pGCT, CT_NONE, NULL, 0);
    FreeScratchGC(pGCT);
    (*pDraw->pScreen->DestroyPixmap)(pPixmap);

}

/* MICOPYPLANE -- public entry for the CopyPlane request.
 * strategy: 
 * First build up a bitmap out of the bits requested 
 * build a source clip
 * Use the bitmap we've built up as a Stipple for the destination 
 */
RegionPtr
miCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    unsigned long	*ptile;
    BoxRec 		box;
    RegionPtr		prgnSrc, prgnExposed;

    /* incorporate the source clip */

    box.x1 = srcx + pSrcDrawable->x;
    box.y1 = srcy + pSrcDrawable->y;
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    /* clip to visible drawable */
    if (box.x1 < pSrcDrawable->x)
	box.x1 = pSrcDrawable->x;
    if (box.y1 < pSrcDrawable->y)
	box.y1 = pSrcDrawable->y;
    if (box.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
    if (box.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
    if (box.x1 > box.x2)
	box.x2 = box.x1;
    if (box.y1 > box.y2)
	box.y2 = box.y1;
    prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

    if (pSrcDrawable->type != DRAWABLE_PIXMAP) {
	/* clip to visible drawable */

	if (pGC->subWindowMode == IncludeInferiors)
	{
	    RegionPtr	clipList = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    (*pGC->pScreen->Intersect) (prgnSrc, prgnSrc, clipList);
	    (*pGC->pScreen->RegionDestroy) (clipList);
	} else
	    (*pGC->pScreen->Intersect)
		    (prgnSrc, prgnSrc, &((WindowPtr)pSrcDrawable)->clipList);
    }

    box = *(*pGC->pScreen->RegionExtents)(prgnSrc);
    (*pGC->pScreen->TranslateRegion)(prgnSrc, -box.x1, -box.y1);

    if ((box.x2 > box.x1) && (box.y2 > box.y1))
    {
	/* minimize the size of the data extracted */
	/* note that we convert the plane mask bitPlane into a plane number */
	box.x1 -= pSrcDrawable->x;
	box.x2 -= pSrcDrawable->x;
	box.y1 -= pSrcDrawable->y;
	box.y2 -= pSrcDrawable->y;
	ptile = miGetPlane(pSrcDrawable, ffs(bitPlane) - 1,
			   box.x1, box.y1,
			   box.x2 - box.x1, box.y2 - box.y1,
			   (unsigned long *) NULL);
	if (ptile)
	{
	    miOpqStipDrawable(pDstDrawable, pGC, prgnSrc, ptile, 0,
			      box.x2 - box.x1, box.y2 - box.y1,
			      dstx + box.x1 - srcx, dsty + box.y1 - srcy);
	    xfree(ptile);
	}
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
		      width, height, dstx, dsty, bitPlane);
    (*pGC->pScreen->RegionDestroy)(prgnSrc);
    return prgnExposed;
}

/* MIGETIMAGE -- public entry for the GetImage Request
 * We're getting the image into a memory buffer. While we have to use GetSpans
 * to read a line from the device (since we don't know what that looks like),
 * we can just write into the destination buffer
 *
 * two different strategies are used, depending on whether we're getting the
 * image in Z format or XY format
 * Z format:
 * Line at a time, GetSpans a line into the destination buffer, then if the
 * planemask is not all ones, we do a SetSpans into a temporary buffer (to get
 * bits turned off) and then another GetSpans to get stuff back (because
 * pixmaps are opaque, and we are passed in the memory to write into).  This is
 * pretty ugly and slow but works.  Life is hard.
 * XY format:
 * get the single plane specified in planemask
 */
void
miGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr 	pDraw;
    int			sx, sy, w, h;
    unsigned int 	format;
    unsigned long 	planeMask;
    pointer             pdstLine;
{
    unsigned char	depth;
    int			i, linelength, width, srcx, srcy;
    DDXPointRec		pt;
    XID			gcv[2];
    PixmapPtr		pPixmap = (PixmapPtr)NULL;
    GCPtr		pGC;
    pointer		pDst = pdstLine;

    depth = pDraw->depth;
    if(format == ZPixmap)
    {
	if ( (((1<<depth)-1)&planeMask) != (1<<depth)-1 )
	{
	    pGC = GetScratchGC(depth, pDraw->pScreen);
	    if (!pGC)
		return;
            pPixmap = (*pDraw->pScreen->CreatePixmap)
			       (pDraw->pScreen, w, h, depth);
	    if (!pPixmap)
	    {
		FreeScratchGC(pGC);
		return;
	    }
	    gcv[0] = GXcopy;
	    gcv[1] = planeMask;
	    DoChangeGC(pGC, GCPlaneMask | GCFunction, gcv, 0);
	    ValidateGC((DrawablePtr)pPixmap, pGC);
	}

        linelength = PixmapBytePad(w, depth);
	srcx = sx + pDraw->x;
	srcy = sy + pDraw->y;
	for(i = 0; i < h; i++)
	{
	    pt.x = srcx;
	    pt.y = srcy + i;
	    width = w;
	    (*pDraw->pScreen->GetSpans)(pDraw, w, &pt, &width, 1,
					(unsigned long *)pDst);
	    if (pPixmap)
	    {
	       pt.x = 0;
	       pt.y = 0;
	       width = w;
	       (*pGC->ops->SetSpans)(pPixmap, pGC, (unsigned long *)pDst,
				     &pt, &width, 1, TRUE);
	       (*pDraw->pScreen->GetSpans)(pPixmap, w, &pt, &width, 1,
					   (unsigned long *)pDst);
	    }
	    pDst += linelength;
	}
	if (pPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(pPixmap);
	    FreeScratchGC(pGC);
	}
    }
    else
    {
	(void) miGetPlane(pDraw, ffs(planeMask) - 1, sx, sy, w, h,
			  (unsigned long *)pDst);
    }
}


/* MIPUTIMAGE -- public entry for the PutImage request
 * Here we benefit from knowing the format of the bits pointed to by pImage,
 * even if we don't know how pDraw represents them.  
 * Three different strategies are used depending on the format 
 * XYBitmap Format:
 * 	we just use the Opaque Stipple helper function to cover the destination
 * 	Note that this covers all the planes of the drawable with the 
 *	foreground color (masked with the GC planemask) where there are 1 bits
 *	and the background color (masked with the GC planemask) where there are
 *	0 bits
 * XYPixmap format:
 *	what we're called with is a series of XYBitmaps, but we only want 
 *	each XYPixmap to update 1 plane, instead of updating all of them.
 * 	we set the foreground color to be all 1s and the background to all 0s
 *	then for each plane, we set the plane mask to only effect that one
 *	plane and recursive call ourself with the format set to XYBitmap
 *	(This clever idea courtesy of RGD.)
 * ZPixmap format:
 *	This part is simple, just call SetSpans
 */
void
miPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr		pDraw;
    GCPtr		pGC;
    int 		depth, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    DDXPointPtr		pptFirst, ppt;
    int			*pwidthFirst, *pwidth;
    RegionPtr		prgnSrc;
    BoxRec		box;
    unsigned long	oldFg, oldBg, gcv[3];
    unsigned long	oldPlanemask;
    unsigned long	i;
    long		bytesPer;

    if (!w || !h)
	return;

    switch(format)
    {
      case XYBitmap:

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = w;
	box.y2 = h;
	prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

        miOpqStipDrawable(pDraw, pGC, prgnSrc, (unsigned long *) pImage,
			  leftPad, w, h, x, y);
	(*pGC->pScreen->RegionDestroy)(prgnSrc);
	break;

      case XYPixmap:
	depth = pGC->depth;
	oldPlanemask = pGC->planemask;
	oldFg = pGC->fgPixel;
	oldBg = pGC->bgPixel;
	gcv[0] = ~0L;
	gcv[1] = 0;
	DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
	bytesPer = (long)h * BitmapBytePad(w + leftPad);

	for (i = 1 << (depth-1); i != 0; i >>= 1, pImage += bytesPer)
	{
	    if (i & oldPlanemask)
	    {
	        gcv[0] = i;
	        DoChangeGC(pGC, GCPlaneMask, gcv, 0);
	        ValidateGC(pDraw, pGC);
	        (*pGC->ops->PutImage)(pDraw, pGC, 1, x, y, w, h, leftPad,
			         XYBitmap, pImage);
	    }
	}
	gcv[0] = oldPlanemask;
	gcv[1] = oldFg;
	gcv[2] = oldBg;
	DoChangeGC(pGC, GCPlaneMask | GCForeground | GCBackground, gcv, 0);
	break;

      case ZPixmap:
    	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    	pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
	if(!pptFirst || !pwidthFirst)
        {
	   if (pwidthFirst)
               DEALLOCATE_LOCAL(pwidthFirst);
           if (pptFirst)
               DEALLOCATE_LOCAL(pptFirst);
           return;
        }
	if (pGC->miTranslate)
	{
	    x += pDraw->x;
	    y += pDraw->y;
	}

	for(i = 0; i < h; i++)
	{
	    ppt->x = x;
	    ppt->y = y + i;
	    ppt++;
	    *pwidth++ = w;
	}

	(*pGC->ops->SetSpans)(pDraw, pGC, pImage, pptFirst, pwidthFirst, h, TRUE);
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pptFirst);
	break;
    }
}
