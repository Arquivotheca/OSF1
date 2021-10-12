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
/* $XConsortium: mipolygen.c,v 5.0 89/06/09 15:08:35 keith Exp $ */
#include "X.h"
#include "gcstruct.h"
#include "miscanfill.h"
#include "mipoly.h"
#include "pixmap.h"

extern void miloadAET(), micomputeWAET(), miFreeStorage();

/*
 *
 *     Written by Brian Kelleher;  Oct. 1985
 *
 *     Routine to fill a polygon.  Two fill rules are
 *     supported: frWINDING and frEVENODD.
 *
 *     See fillpoly.h for a complete description of the algorithm.
 */

Bool
miFillGeneralPoly(dst, pgc, count, ptsIn)
    DrawablePtr dst;
    GCPtr	pgc;
    int		count;              /* number of points        */
    DDXPointPtr ptsIn;              /* the points              */
{
    register EdgeTableEntry *pAET;  /* the Active Edge Table   */
    register int y;                 /* the current scanline    */
    register int nPts = 0;          /* number of pts in buffer */
    register EdgeTableEntry *pWETE; /* Winding Edge Table      */
    register ScanLineList *pSLL;    /* Current ScanLineList    */
    register DDXPointPtr ptsOut;      /* ptr to output buffers   */
    int *width;
    DDXPointRec FirstPoint[NUMPTSTOBUFFER]; /* the output buffers */
    int FirstWidth[NUMPTSTOBUFFER];
    EdgeTableEntry *pPrevAET;       /* previous AET entry      */
    EdgeTable ET;                   /* Edge Table header node  */
    EdgeTableEntry AET;             /* Active ET header node   */
    EdgeTableEntry *pETEs;          /* Edge Table Entries buff */
    ScanLineListBlock SLLBlock;     /* header for ScanLineList */
    int fixWAET = 0;

    if (count < 3)
	return(TRUE);

    if(!(pETEs = (EdgeTableEntry *)
        ALLOCATE_LOCAL(sizeof(EdgeTableEntry) * count)))
	return(FALSE);
    ptsOut = FirstPoint;
    width = FirstWidth;
    if (!miCreateETandAET(count, ptsIn, &ET, &AET, pETEs, &SLLBlock))
    {
	DEALLOCATE_LOCAL(pETEs);
	return(FALSE);
    }
    pSLL = ET.scanlines.next;

    if (pgc->fillRule == EvenOddRule) 
    {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) 
        {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL && y == pSLL->scanline) 
            {
                miloadAET(&AET, pSLL->edgelist);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;

            /*
             *  for each active edge
             */
            while (pAET) 
            {
                ptsOut->x = pAET->bres.minor;
		ptsOut++->y = y;
                *width++ = pAET->next->bres.minor - pAET->bres.minor;
                nPts++;

                /*
                 *  send out the buffer when its full
                 */
                if (nPts == NUMPTSTOBUFFER) 
		{
		    (*pgc->ops->FillSpans)(dst, pgc,
				      nPts, FirstPoint, FirstWidth,
				      1);
                    ptsOut = FirstPoint;
                    width = FirstWidth;
                    nPts = 0;
                }
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y)
                EVALUATEEDGEEVENODD(pAET, pPrevAET, y);
            }
            miInsertionSort(&AET);
        }
    }
    else      /* default to WindingNumber */
    {
        /*
         *  for each scanline
         */
        for (y = ET.ymin; y < ET.ymax; y++) 
        {
            /*
             *  Add a new edge to the active edge table when we
             *  get to the next edge.
             */
            if (pSLL && y == pSLL->scanline) 
            {
                miloadAET(&AET, pSLL->edgelist);
                micomputeWAET(&AET);
                pSLL = pSLL->next;
            }
            pPrevAET = &AET;
            pAET = AET.next;
            pWETE = pAET;

            /*
             *  for each active edge
             */
            while (pAET) 
            {
                /*
                 *  if the next edge in the active edge table is
                 *  also the next edge in the winding active edge
                 *  table.
                 */
                if (pWETE == pAET) 
                {
                    ptsOut->x = pAET->bres.minor;
		    ptsOut++->y = y;
                    *width++ = pAET->nextWETE->bres.minor - pAET->bres.minor;
                    nPts++;

                    /*
                     *  send out the buffer
                     */
                    if (nPts == NUMPTSTOBUFFER) 
                    {
			(*pgc->ops->FillSpans)(dst, pgc, nPts, FirstPoint,
			                  FirstWidth, 1);
                        ptsOut = FirstPoint;
                        width  = FirstWidth;
                        nPts = 0;
                    }

                    pWETE = pWETE->nextWETE;
                    while (pWETE != pAET)
                        EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
                    pWETE = pWETE->nextWETE;
                }
                EVALUATEEDGEWINDING(pAET, pPrevAET, y, fixWAET);
            }

            /*
             *  reevaluate the Winding active edge table if we
             *  just had to resort it or if we just exited an edge.
             */
            if (miInsertionSort(&AET) || fixWAET) 
            {
                micomputeWAET(&AET);
                fixWAET = 0;
            }
        }
    }

    /*
     *     Get any spans that we missed by buffering
     */
    (*pgc->ops->FillSpans)(dst, pgc, nPts, FirstPoint, FirstWidth, 1);
    DEALLOCATE_LOCAL(pETEs);
    miFreeStorage(SLLBlock.next);
    return(TRUE);
}
