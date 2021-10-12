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
/* $XConsortium: mifpolycon.c,v 5.3 89/07/28 12:13:45 rws Exp $ */
#include <math.h>
#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "mifpoly.h"

static int GetFPolyYBounds();

#ifdef ICEILTEMPDECL
ICEILTEMPDECL
#endif

/*
 *	Written by Todd Newman; April. 1987.
 *
 *	Fill a convex polygon.  If the given polygon
 *	is not convex, then the result is undefined.
 *	The algorithm is to order the edges from smallest
 *	y to largest by partitioning the array into a left
 *	edge list and a right edge list.  The algorithm used
 *	to traverse each edge is digital differencing analyzer
 *	line algorithm with y as the major axis. There's some funny linear
 *	interpolation involved because of the subpixel postioning.
 */
void
miFillSppPoly(dst, pgc, count, ptsIn, xTrans, yTrans, xFtrans, yFtrans)
    DrawablePtr 	dst;
    GCPtr		pgc;
    int			count;          /* number of points */
    SppPointPtr 	ptsIn;          /* the points */
    int			xTrans, yTrans;	/* Translate each point by this */
    double		xFtrans, yFtrans;	/* translate before conversion
    						   by this amount.  This provides
						   a mechanism to match rounding
						   errors with any shape that must
						   meet the polygon exactly.
						 */
{
    double		xl, xr,		/* x vals of left and right edges */
          		ml,       	/* left edge slope */
          		mr,             /* right edge slope */
          		dy,             /* delta y */
    			i;              /* loop counter */
    int			y,              /* current scanline */
    			j,
    			imin,           /* index of vertex with smallest y */
    			ymin,           /* y-extents of polygon */
    			ymax,
    			*width,
    			*FirstWidth,    /* output buffer */
    	 		*Marked;	/* set if this vertex has been used */
    register int	left, right,	/* indices to first endpoints */
    			nextleft,
                 	nextright;	/* indices to second endpoints */
    DDXPointPtr 	ptsOut,
    			FirstPoint;	/* output buffer */

    if (pgc->miTranslate)
    {
	xTrans += dst->x;
	yTrans += dst->y;
    }

    imin = GetFPolyYBounds(ptsIn, count, yFtrans, &ymin, &ymax);

    y = ymax - ymin + 1;
    if ((count < 3) || (y <= 0))
	return;
    ptsOut = FirstPoint = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * y);
    width = FirstWidth = (int *) ALLOCATE_LOCAL(sizeof(int) * y);
    Marked = (int *) ALLOCATE_LOCAL(sizeof(int) * count);

    if(!ptsOut || !width || !Marked)
    {
	if (Marked) DEALLOCATE_LOCAL(Marked);
	if (width) DEALLOCATE_LOCAL(width);
	if (ptsOut) DEALLOCATE_LOCAL(ptsOut);
	return;
    }

    for(j = 0; j < count; j++)
	Marked[j] = 0;
    nextleft = nextright = imin;
    Marked[imin] = -1;
    y = ICEIL(ptsIn[nextleft].y + yFtrans);

    /*
     *  loop through all edges of the polygon
     */
    do
    {
        /* add a left edge if we need to */
        if ((y > (ptsIn[nextleft].y + yFtrans) ||
 	     ISEQUAL(y, ptsIn[nextleft].y + yFtrans)) &&
	     Marked[nextleft] != 1)
	{
	    Marked[nextleft]++;
            left = nextleft++;

            /* find the next edge, considering the end conditions */
            if (nextleft >= count)
                nextleft = 0;

            /* now compute the starting point and slope */
	    dy = ptsIn[nextleft].y - ptsIn[left].y;
	    if (dy != 0.0)
	    { 
		ml = (ptsIn[nextleft].x - ptsIn[left].x) / dy;
		dy = y - (ptsIn[left].y + yFtrans);
		xl = (ptsIn[left].x + xFtrans) + ml * max(dy, 0); 
	    }
        }

        /* add a right edge if we need to */
        if ((y > ptsIn[nextright].y + yFtrans) ||
 	     ISEQUAL(y, ptsIn[nextright].y + yFtrans)
	     && Marked[nextright] != 1)
	{
	    Marked[nextright]++;
            right = nextright--;

            /* find the next edge, considering the end conditions */
            if (nextright < 0)
                nextright = count - 1;

            /* now compute the starting point and slope */
	    dy = ptsIn[nextright].y - ptsIn[right].y;
	    if (dy != 0.0) 
	    { 
		mr = (ptsIn[nextright].x - ptsIn[right].x) / dy;
		dy = y - (ptsIn[right].y + yFtrans); 
		xr = (ptsIn[right].x + xFtrans) + mr * max(dy, 0);
	    }
        }


        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = (min(ptsIn[nextleft].y, ptsIn[nextright].y) + yFtrans) - y;

	if (i < EPSILON)
	{
	    if(Marked[nextleft] && Marked[nextright])
	    {
	        /* Arrgh, we're trapped! (no more points) 
	         * Out, we've got to get out of here before this decadence saps
	         * our will completely! */
	        break;
	    }
	    continue;
	}
	else
	{
		j = (int) i;
		if(!j)
		    j++;
	}
        while (j > 0) 
        {
	    int cxl, cxr;

            ptsOut->y = (y) + yTrans;

	    cxl = ICEIL(xl);
	    cxr = ICEIL(xr);
            /* reverse the edges if necessary */
            if (xl < xr) 
            {
                *(width++) = cxr - cxl;
                (ptsOut++)->x = cxl + xTrans;
            }
            else 
            {
                *(width++) = cxl - cxr;
                (ptsOut++)->x = cxr + xTrans;
            }
            y++;

            /* increment down the edges */
	    xl += ml;
	    xr += mr;
	    j--;
        }
    }  while (y <= ymax);

    /* Finally, fill the spans we've collected */
    (*pgc->ops->FillSpans)(dst, pgc, 
		      ptsOut-FirstPoint, FirstPoint, FirstWidth, 1);
    DEALLOCATE_LOCAL(Marked);
    DEALLOCATE_LOCAL(FirstWidth);
    DEALLOCATE_LOCAL(FirstPoint);
}


/* Find the index of the point with the smallest y.also return the
 * smallest and largest y */
static
int
GetFPolyYBounds(pts, n, yFtrans, by, ty)
    register SppPointPtr	pts;
    int 			n;
    double			yFtrans;
    int 			*by, *ty;
{
    register SppPointPtr	ptMin;
    double 			ymin, ymax;
    SppPointPtr			ptsStart = pts;

    ptMin = pts;
    ymin = ymax = (pts++)->y;

    while (--n > 0) {
        if (pts->y < ymin)
	{
            ptMin = pts;
            ymin = pts->y;
        }
	if(pts->y > ymax)
            ymax = pts->y;

        pts++;
    }

    *by = ICEIL(ymin + yFtrans);
    *ty = ICEIL(ymax + yFtrans - 1);
    return(ptMin-ptsStart);
}
