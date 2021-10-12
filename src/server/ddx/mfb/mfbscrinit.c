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
/* $XConsortium: mfbscrinit.c,v 5.12 90/09/24 10:19:29 rws Exp $ */

#include "X.h"
#include "Xproto.h"	/* for xColorItem */
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "mfb.h"
#include "mistruct.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "servermd.h"

extern RegionPtr mfbPixmapToRegion();

int mfbWindowPrivateIndex;
int mfbGCPrivateIndex;
static unsigned long mfbGeneration = 0;

static VisualRec visual = {
/* vid  class       bpRGB cmpE nplan rMask gMask bMask oRed oGreen oBlue */
   0,   StaticGray, 1,    2,   1,    0,    0,    0,    0,   0,     0
};

static VisualID VID;

static DepthRec depth = {
/* depth	numVid		vids */
    1,		1,		&VID
};

miBSFuncRec mfbBSFuncRec = {
    mfbSaveAreas,
    mfbRestoreAreas,
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};

Bool
mfbAllocatePrivates(pScreen, pWinIndex, pGCIndex)
    ScreenPtr pScreen;
    int *pWinIndex, *pGCIndex;
{
    if (mfbGeneration != serverGeneration)
    {
	mfbWindowPrivateIndex = AllocateWindowPrivateIndex();
	mfbGCPrivateIndex = AllocateGCPrivateIndex();
	visual.vid = FakeClientID(0);
	VID = visual.vid;
	mfbGeneration = serverGeneration;
    }
    if (pWinIndex)
	*pWinIndex = mfbWindowPrivateIndex;
    if (pGCIndex)
	*pGCIndex = mfbGCPrivateIndex;
    return (AllocateWindowPrivate(pScreen, mfbWindowPrivateIndex,
				  sizeof(mfbPrivWin)) &&
	    AllocateGCPrivate(pScreen, mfbGCPrivateIndex, sizeof(mfbPrivGC)));
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
mfbScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    if 	(!mfbAllocatePrivates(pScreen, (int *)NULL, (int *)NULL))
	return FALSE;
    pScreen->defColormap = (Colormap) FakeClientID(0);
    /* whitePixel, blackPixel */
    pScreen->QueryBestSize = mfbQueryBestSize;
    /* SaveScreen */
    pScreen->GetImage = mfbGetImage;
    pScreen->GetSpans = mfbGetSpans;
    pScreen->CreateWindow = mfbCreateWindow;
    pScreen->DestroyWindow = mfbDestroyWindow;
    pScreen->PositionWindow = mfbPositionWindow;
    pScreen->ChangeWindowAttributes = mfbChangeWindowAttributes;
    pScreen->RealizeWindow = mfbMapWindow;
    pScreen->UnrealizeWindow = mfbUnmapWindow;
    pScreen->PaintWindowBackground = mfbPaintWindow;
    pScreen->PaintWindowBorder = mfbPaintWindow;
    pScreen->CopyWindow = mfbCopyWindow;
    pScreen->CreatePixmap = mfbCreatePixmap;
    pScreen->DestroyPixmap = mfbDestroyPixmap;
    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CreateGC = mfbCreateGC;
    pScreen->CreateColormap = mfbCreateColormap;
    pScreen->DestroyColormap = mfbDestroyColormap;
    pScreen->InstallColormap = mfbInstallColormap;
    pScreen->UninstallColormap = mfbUninstallColormap;
    pScreen->ListInstalledColormaps = mfbListInstalledColormaps;
    pScreen->StoreColors = NoopDDA;
    pScreen->ResolveColor = mfbResolveColor;
    pScreen->BitmapToRegion = mfbPixmapToRegion;
    return miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
			1, 1, &depth, VID, 1, &visual, &mfbBSFuncRec);
}
