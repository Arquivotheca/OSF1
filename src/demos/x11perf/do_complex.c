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
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************************/

#include "x11perf.h"

#define NUM_POINTS 4    /* 4 points to an arrowhead */
#define NUM_ANGLES 3    /* But mostly it looks like a triangle */
static XPoint   *points;
static GC       pgc;

extern double sin();
extern double cos();
extern double sqrt();
#define PI  3.14159265357989

int InitComplexPoly(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i, j, numPoints;
    int     x, y;
    int     size, iradius;
    double  phi, phiinc, radius, delta, phi2;
    XPoint  *curPoint;

    pgc = xp->fggc;

    size = p->special;
    phi = 0.0;
    delta = 2.0 * PI / ((double) NUM_ANGLES);
    if (xp->version == VERSION1_2) {
	radius = ((double) size) * sqrt(3.0)/2.0;
	phiinc = delta/10.0;
    } else {
	/* Version 1.2's radius computation was completely bogus, and resulted
	   in triangles with sides about 50% longer than advertised.  Since
	   in version 1.3 triangles are scaled to cover size^2 pixels, we do
	   the same computation here.  The arrowheads are a little larger than
	   simple triangles, because they lose 1/3 of their area due to the
	   notch cut out from them, so radius has to be sqrt(3/2) larger than
	   for simple triangles.
	 */
	radius = ((double) size) * sqrt(sqrt(4.0/3.0));
	phiinc = 1.75*PI / ((double) p->objects);
    }
    iradius = (int) radius + 1;

    numPoints = (p->objects) * NUM_POINTS;  
    points = (XPoint *)malloc(numPoints * sizeof(XPoint));
    curPoint = points;
    x = iradius;
    y = iradius;
    for (i = 0; i != p->objects; i++) {
	for (j = 0; j != NUM_ANGLES; j++) {
	    phi2 = phi + ((double) j) * delta;
	    curPoint->x = (int) ((double)x + (radius * cos(phi2)) + 0.5);
	    curPoint->y = (int) ((double)y + (radius * sin(phi2)) + 0.5);
	    curPoint++;
	}
	curPoint->x = x;
	curPoint->y = y;
	curPoint++;

	phi += phiinc;
	y += 2 * iradius;
	if (y + iradius >= HEIGHT) {
	    y = iradius;
	    x += 2 * iradius;
	    if (x + iradius >= WIDTH) {
		x = iradius;
	    }
	}
    }
    return reps;
}

void DoComplexPoly(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i, j;
    XPoint  *curPoint;

    for (i = 0; i != reps; i++) {
        curPoint = points;
        for (j = 0; j != p->objects; j++) {
            XFillPolygon(xp->d, xp->w, pgc, curPoint, NUM_POINTS, Complex, 
			 CoordModeOrigin);
            curPoint += NUM_POINTS;
	  }
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

void EndComplexPoly(xp, p)
    XParms  xp;
    Parms   p;
{
    free(points);
}

