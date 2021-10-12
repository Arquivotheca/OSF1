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
/*
 * Push Pixels for 8 bit displays.
 */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/
/* $XConsortium: cfbpush8.c,v 5.9 91/04/10 11:41:54 keith Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include        "vgaBank.h"

#if PPW == 4

extern void mfbPushPixels();

void
cfbPushPixels8 (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitmap;
    DrawablePtr	pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    register unsigned long   *src, *dst;
    register unsigned long   pixel;
    register unsigned long   c, bits;
    unsigned long   *pdstLine, *psrcLine;
    unsigned long   *pdstBase;
    int		    srcWidth;
    int		    dstWidth;
    int		    xoff;
    int		    nBitmapLongs, nPixmapLongs;
    int		    nBitmapTmp, nPixmapTmp;
    unsigned long   rightMask;
    BoxRec	    bbox;
    cfbPrivGCPtr    devPriv;

    bbox.x1 = xOrg;
    bbox.y1 = yOrg;
    bbox.x2 = bbox.x1 + dx;
    bbox.y2 = bbox.y1 + dy;
    devPriv = (cfbPrivGC *)pGC->devPrivates[cfbGCPrivateIndex].ptr;
    
    switch ((*pGC->pScreen->RectIn)(devPriv->pCompositeClip, &bbox))
    {
      case rgnPART:
	mfbPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
      case rgnOUT:
	return;
    }

    cfbGetLongWidthAndPointer (pDrawable, dstWidth, pdstBase)

    BANK_FLAG(pdstBase)

    psrcLine = (unsigned long *) pBitmap->devPrivate.ptr;
    srcWidth = (int) pBitmap->devKind >> 2;
    
    pixel = devPriv->xor;
    xoff = xOrg & 03;
    nBitmapLongs = (dx + xoff) >> 5;
    nPixmapLongs = (dx + 3 + xoff) >> 2;

    rightMask = ~cfb8BitLenMasks[((dx + xoff) & 0x1f)];

    pdstLine = pdstBase + (yOrg * dstWidth) + (xOrg >> 2);

    while (dy--)
    {
	c = 0;
	nPixmapTmp = nPixmapLongs;
	nBitmapTmp = nBitmapLongs;
	src = psrcLine;
	dst = pdstLine;
	SETRW(dst);
	while (nBitmapTmp--)
	{
	    bits = *src++;
	    c |= BitRight (bits, xoff);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	    nPixmapTmp -= 8;
	    c = 0;
	    if (xoff)
		c = BitLeft (bits, 32 - xoff);
	}
	if (BitLeft (rightMask, xoff))
	    c |= BitRight (*src, xoff);
	c &= rightMask;
	switch (nPixmapTmp) {
	case 8:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 7:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 6:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 5:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 4:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 3:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 2:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 1:
	    WriteFourBits(dst, pixel, GetFourBits(c));
	    NextFourBits(c);
	    dst++; CHECKRWO(dst);
	case 0:
	    break;
	}
	pdstLine += dstWidth;
	psrcLine += srcWidth;
    }
}

#endif
