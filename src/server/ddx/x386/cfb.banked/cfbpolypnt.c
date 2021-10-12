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
/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
no- tice appear in all copies and that both that copyright
no- tice and this permission notice appear in supporting
docu- mentation, and that the name of MIT not be used in
advertising or publicity pertaining to distribution of the
software without specific prior written permission.
M.I.T. makes no representation about the suitability of
this software for any purpose. It is provided "as is"
without any express or implied warranty.

********************************************************/

/* $XConsortium: cfbpolypnt.c,v 5.12 91/05/28 14:59:08 keith Exp $ */

#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "vgaBank.h"

#define isClipped(c,ul,lr)  ((((c) - (ul)) | ((lr) - (c))) & ClipMask)

#define PointLoop(fill) { \
    for (nbox = REGION_NUM_RECTS(cclip), pbox = REGION_RECTS(cclip); \
	 --nbox >= 0; \
	 pbox++) \
    { \
	c1 = *((long *) &pbox->x1) - off; \
	c2 = *((long *) &pbox->x2) - off - 0x00010001; \
	for (ppt = (long *) pptInit, i = npt; --i >= 0;) \
	{ \
	    pt = *ppt++; \
	    if (!isClipped(pt,c1,c2)) { \
		fill \
	    } \
	} \
    } \
}

void
cfbPolyPoint(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int mode;
    int npt;
    xPoint *pptInit;
{
    register long   pt;
    register long   c1, c2;
    register unsigned long   ClipMask = 0x80008000;
    register unsigned long   xor;
#if PPW == 4
    register unsigned char   *addrb;
    register int    nbwidth;
    unsigned char   *addrbt;
#else
    register unsigned long    *addrl;
    register int    nlwidth;
    unsigned long   *addrlt;
#endif
    register long   *ppt;
    RegionPtr	    cclip;
    int		    nbox;
    register int    i;
    register BoxPtr pbox;
    long	    and;
    int		    rop = pGC->alu;
    int		    off;
    cfbPrivGCPtr    devPriv;
    xPoint	    *pptPrev;

    devPriv = (cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr); 
    rop = devPriv->rop;
    if (rop == GXnoop)
	return;
    cclip = devPriv->pCompositeClip;
    xor = devPriv->xor;
    if ((mode == CoordModePrevious) && (npt > 1))
    {
	for (pptPrev = pptInit + 1, i = npt - 1; --i >= 0; pptPrev++)
	{
	    pptPrev->x += (pptPrev-1)->x;
	    pptPrev->y += (pptPrev-1)->y;
	}
    }
    off = *((int *) &pDrawable->x);
    off -= (off & 0x8000) << 1;
#if PPW == 4
    cfbGetByteWidthAndPointer(pDrawable, nbwidth, addrb);

    BANK_FLAG(addrb)

    addrb = addrb + pDrawable->y * nbwidth + pDrawable->x;
    if (rop == GXcopy)
    {
	if (!(nbwidth & (nbwidth - 1)))
	{
	    nbwidth = ffs(nbwidth) - 1;
	    PointLoop(addrbt = addrb + (intToY(pt) << nbwidth) + intToX(pt);
		      SETRW(addrbt); *addrbt = xor;)
	}
	else
	{
	    PointLoop(addrbt =  addrb + intToY(pt) * nbwidth + intToX(pt);
		      SETRW(addrbt); *addrbt = xor;)
	}
    }
    else
    {
	and = devPriv->and;
	PointLoop(  addrbt = addrb + intToY(pt) * nbwidth + intToX(pt);
		    SETRW(addrbt);
		    *addrbt = DoRRop (*addrbt, and, xor);)
    }
#else
    cfbGetLongWidthAndPointer(pDrawable, nlwidth, addrl);
    and = devPriv->and;
    PointLoop(	addrlt = addrl + intToY(pt) * nlwidth + intToX(pt);
		*addrlt = DoRRop (*addrlt, and, xor); )
#endif
}
