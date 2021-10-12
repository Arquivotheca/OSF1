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
/* $XConsortium: mipolycon.c,v 5.0 89/06/09 15:08:33 keith Exp $ */
#include "gcstruct.h"
#include "pixmap.h"
#include "miscanfill.h"

#include <stdio.h>

static int getPolyYBounds();

/*
 *     convexpoly.c
 *
 *     Written by Brian Kelleher; Dec. 1985.
 *
 *     Fill a convex polygon.  If the given polygon
 *     is not convex, then the result is undefined.
 *     The algorithm is to order the edges from smallest
 *     y to largest by partitioning the array into a left
 *     edge list and a right edge list.  The algorithm used
 *     to traverse each edge is an extension of Bresenham's
 *     line algorithm with y as the major axis.
 *     For a derivation of the algorithm, see the author of
 *     this code.
 */
Bool
miFillConvexPoly(dst, pgc, count, ptsIn)
    DrawablePtr dst;
    GCPtr	pgc;
    int		count;                /* number of points        */
    DDXPointPtr ptsIn;                /* the points              */
{
    register int xl, xr;        /* x vals of left and right edges */
    register int dl, dr;        /* decision variables             */
    register int ml, m1l;       /* left edge slope and slope+1    */
    int mr, m1r;                /* right edge slope and slope+1   */
    int incr1l, incr2l;         /* left edge error increments     */
    int incr1r, incr2r;         /* right edge error increments    */
    int dy;                     /* delta y                        */
    int y;                      /* current scanline               */
    int left, right;            /* indices to first endpoints     */
    int i;                      /* loop counter                   */
    int nextleft, nextright;    /* indices to second endpoints    */
    DDXPointPtr ptsOut, FirstPoint; /* output buffer               */
    int *width, *FirstWidth;    /* output buffer                  */
    int imin;                   /* index of smallest vertex (in y) */
    int ymin;                   /* y-extents of polygon            */
    int ymax;

    /*
     *  find leftx, bottomy, rightx, topy, and the index
     *  of bottomy. Also translate the points.
     */
    imin = getPolyYBounds(ptsIn, count, &ymin, &ymax);

    dy = ymax - ymin + 1;
    if ((count < 3) || (dy < 0))
	return(TRUE);
    ptsOut = FirstPoint = (DDXPointPtr )ALLOCATE_LOCAL(sizeof(DDXPointRec)*dy);
    width = FirstWidth = (int *)ALLOCATE_LOCAL(sizeof(int) * dy);
    if(!FirstPoint || !FirstWidth)
    {
	if (FirstWidth) DEALLOCATE_LOCAL(FirstWidth);
	if (FirstPoint) DEALLOCATE_LOCAL(FirstPoint);
	return(FALSE);
    }

    nextleft = nextright = imin;
    y = ptsIn[nextleft].y;

    /*
     *  loop through all edges of the polygon
     */
    do {
        /*
         *  add a left edge if we need to
         */
        if (ptsIn[nextleft].y == y) {
            left = nextleft;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextleft++;
            if (nextleft >= count)
                nextleft = 0;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(ptsIn[nextleft].y-ptsIn[left].y,
                         ptsIn[left].x,ptsIn[nextleft].x,
                         xl, dl, ml, m1l, incr1l, incr2l);
        }

        /*
         *  add a right edge if we need to
         */
        if (ptsIn[nextright].y == y) {
            right = nextright;

            /*
             *  find the next edge, considering the end
             *  conditions of the array.
             */
            nextright--;
            if (nextright < 0)
                nextright = count-1;

            /*
             *  now compute all of the random information
             *  needed to run the iterative algorithm.
             */
            BRESINITPGON(ptsIn[nextright].y-ptsIn[right].y,
                         ptsIn[right].x,ptsIn[nextright].x,
                         xr, dr, mr, m1r, incr1r, incr2r);
        }

        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = min(ptsIn[nextleft].y, ptsIn[nextright].y) - y;
	/* in case we're called with non-convex polygon */
	if(i < 0)
        {
	    DEALLOCATE_LOCAL(FirstWidth);
	    DEALLOCATE_LOCAL(FirstPoint);
	    return(TRUE);
	}
        while (i-- > 0) 
        {
            ptsOut->y = y;

            /*
             *  reverse the edges if necessary
             */
            if (xl < xr) 
            {
                *(width++) = xr - xl;
                (ptsOut++)->x = xl;
            }
            else 
            {
                *(width++) = xl - xr;
                (ptsOut++)->x = xr;
            }
            y++;

            /* increment down the edges */
            BRESINCRPGON(dl, xl, ml, m1l, incr1l, incr2l);
            BRESINCRPGON(dr, xr, mr, m1r, incr1r, incr2r);
        }
    }  while (y != ymax);

    /*
     * Finally, fill the <remaining> spans
     */
    (*pgc->ops->FillSpans)(dst, pgc, 
		      ptsOut-FirstPoint,FirstPoint,FirstWidth,
		      1);
    DEALLOCATE_LOCAL(FirstWidth);
    DEALLOCATE_LOCAL(FirstPoint);
    return(TRUE);
}


/*
 *     Find the index of the point with the smallest y.
 */
static
int
getPolyYBounds(pts, n, by, ty)
    DDXPointPtr pts;
    int n;
    int *by, *ty;
{
    register DDXPointPtr ptMin;
    int ymin, ymax;
    DDXPointPtr ptsStart = pts;

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

    *by = ymin;
    *ty = ymax;
    return(ptMin-ptsStart);
}
