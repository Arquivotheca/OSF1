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
/* $XConsortium: mifillrct.c,v 5.0 89/06/09 15:08:22 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"

#include "misc.h"

/* mi rectangles
   written by newman, with debts to all and sundry
*/

/* MIPOLYFILLRECT -- public entry for PolyFillRect request
 * very straight forward: translate rectangles if necessary
 * then call FillSpans to fill each rectangle.  We let FillSpans worry about
 * clipping to the destination
 */
void
miPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    int i;
    register int	height;
    register int	width;
    register xRectangle *prect; 
    int			xorg;
    register int	yorg;
    int			maxheight;
    DDXPointPtr		pptFirst;
    register DDXPointPtr ppt;
    int			*pwFirst;
    register int 	*pw;

    if (pGC->miTranslate)
    {
	xorg = pDrawable->x;
	yorg = pDrawable->y;
        prect = prectInit;
        maxheight = 0;
        for (i = 0; i<nrectFill; i++, prect++)
        {
	    prect->x += xorg;
	    prect->y += yorg;
	    maxheight = max(maxheight, prect->height);
        }
    }
    else
    {
        prect = prectInit;
        maxheight = 0;
        for (i = 0; i<nrectFill; i++, prect++)
	    maxheight = max(maxheight, prect->height);
    }

    pptFirst = (DDXPointPtr) ALLOCATE_LOCAL(maxheight * sizeof(DDXPointRec));
    pwFirst = (int *) ALLOCATE_LOCAL(maxheight * sizeof(int));
    if(!pptFirst || !pwFirst)
    {
	if (pwFirst) DEALLOCATE_LOCAL(pwFirst);
	if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
	return;
    }

    prect = prectInit;
    while(nrectFill--)
    {
	ppt = pptFirst;
	pw = pwFirst;
	height = prect->height;
	width = prect->width;
	xorg = prect->x;
	yorg = prect->y;
	while(height--)
	{
	    *pw++ = width;
	    ppt->x = xorg;
	    ppt->y = yorg;
	    ppt++;
	    yorg++;
	}
	(* pGC->ops->FillSpans)(pDrawable, pGC, 
			   prect->height, pptFirst, pwFirst,
			   1);
	prect++;
    }
    DEALLOCATE_LOCAL(pwFirst);
    DEALLOCATE_LOCAL(pptFirst);
}
