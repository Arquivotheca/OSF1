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
**************************************************************************
**                   DIGITAL EQUIPMENT CORPORATION                      **
**                         CONFIDENTIAL                                 **
**    NOT FOR MODIFICATION OR REDISTRIBUTION IN ANY MANNER WHATSOEVER   **
**************************************************************************
*/
/****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991 by Digital Equipment Corp., Maynard, MA

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

*****************************************************************************/
/*
 *  ABSTRACT:
 *
 *      This module is contains Vector tables used for swapping and general   
 *      dispatch by the XTrap server extension.
 *
 *  CONTRIBUTORS:
 *
 *      Ken Miller
 *      Marc Evans
 *
 */
#ifndef lint
static char RCSID[] = "$Header: /alphabits/u3/x11/ode/rcs/x11/src/extensions/server/xtrap/xtrapditables.c,v 1.1.2.2 92/02/06 11:25:20 Jim_Ludwig Exp $";
#endif

/*-----------------*
 *  Include Files  *
 *-----------------*/
#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "Xprotostr.h"
#include "xtrapdi.h"
#include "input.h"          /* Server DevicePtr definitions */
#include "misc.h"
#include "dixstruct.h"
#include "xtrapddmi.h"      /* has to be after misc.h & dixstruct.h for U*IX */
#include "xtrapproto.h"

globaldef void_function XETSwProcVector[256L] =
{
    (void_function)ProcBadRequest,
    XETSwCreateWindow,
    XETSwChangeWindowAttributes,
    XETSwResourceReq,			/* GetWindowAttributes */
    XETSwResourceReq,			/* DestroyWindow */
    XETSwResourceReq,			/* 5 DestroySubwindows */
    XETSwResourceReq,			/* XETSwChangeSaveSet, */
    XETSwReparentWindow,
    XETSwResourceReq,			/* MapWindow */
    XETSwResourceReq,			/* MapSubwindows */
    XETSwResourceReq,			/* 10 UnmapWindow */
    XETSwResourceReq,			/* UnmapSubwindows */
    XETSwConfigureWindow,
    XETSwResourceReq,			/* XETSwCirculateWindow, */
    XETSwResourceReq,			/* GetGeometry */
    XETSwResourceReq,			/* 15 QueryTree */
    XETSwInternAtom,
    XETSwResourceReq,			/* XETSwGetAtomName, */
    XETSwChangeProperty,
    XETSwDeleteProperty,
    XETSwGetProperty,			/* 20 */
    XETSwResourceReq,			/* XETSwListProperties, */
    XETSwSetSelectionOwner,
    XETSwResourceReq, 			/* XETSwGetSelectionOwner, */
    XETSwConvertSelection,
    XETSwSendEvent,			/* 25 */
    XETSwGrabPointer,
    XETSwResourceReq, 			/* XETSwUngrabPointer, */
    XETSwGrabButton,
    XETSwUngrabButton,
    XETSwChangeActivePointerGrab,	/* 30 */
    XETSwGrabKeyboard,
    XETSwResourceReq,			/* XETSwUngrabKeyboard, */
    XETSwGrabKey,
    XETSwUngrabKey,
    XETSwResourceReq,			/* 35 XETSwAllowEvents, */
    XETSwSimpleReq,			/* XETSwGrabServer, */
    XETSwSimpleReq,			/* XETSwUngrabServer, */
    XETSwResourceReq,			/* XETSwQueryPointer, */
    XETSwGetMotionEvents,
    XETSwTranslateCoords,		/*40 */
    XETSwWarpPointer,
    XETSwSetInputFocus,
    XETSwSimpleReq,			/* XETSwGetInputFocus, */
    XETSwSimpleReq,			/* QueryKeymap, */
    XETSwOpenFont,			/* 45 */
    XETSwResourceReq,			/* XETSwCloseFont, */
    XETSwResourceReq, 			/* XETSwQueryFont, */
    XETSwResourceReq,			/* XETSwQueryTextExtents,  */
    XETSwListFonts,
    XETSwListFontsWithInfo,		/* 50 */
    XETSwSetFontPath,
    XETSwSimpleReq,			/* GetFontPath, */
    XETSwCreatePixmap,
    XETSwResourceReq,			/* XETSwFreePixmap, */
    XETSwCreateGC,			/* 55 */
    XETSwChangeGC,
    XETSwCopyGC,
    XETSwSetDashes,
    XETSwSetClipRectangles,
    XETSwResourceReq,			/* 60 XETSwFreeGC, */
    XETSwClearToBackground,
    XETSwCopyArea,
    XETSwCopyPlane,
    XETSwPoly,				/* PolyPoint, */
    XETSwPoly,				/* 65 PolyLine */
    XETSwPoly,				/* PolySegment, */
    XETSwPoly,				/* PolyRectangle, */
    XETSwPoly,				/* PolyArc, */
    XETSwFillPoly,
    XETSwPoly,				/* 70 PolyFillRectangle */
    XETSwPoly,				/* PolyFillArc, */
    XETSwPutImage,
    XETSwGetImage,
    XETSwPolyText,
    XETSwPolyText,			/* 75 */
    XETSwImageText,
    XETSwImageText,
    XETSwCreateColormap,
    XETSwResourceReq,			/* XETSwFreeColormap, */
    XETSwCopyColormapAndFree,		/* 80 */
    XETSwResourceReq,			/* XETSwInstallColormap, */
    XETSwResourceReq,			/* XETSwUninstallColormap, */
    XETSwResourceReq, 			/* XETSwListInstalledColormaps, */
    XETSwAllocColor,
    XETSwAllocNamedColor,		/* 85 */
    XETSwAllocColorCells,
    XETSwAllocColorPlanes,
    XETSwFreeColors,
    XETSwStoreColors,
    XETSwStoreNamedColor,		/* 90 */
    XETSwQueryColors,
    XETSwLookupColor,
    XETSwCreateCursor,
    XETSwCreateGlyphCursor,
    XETSwResourceReq,			/* 95 XETSwFreeCursor, */
    XETSwRecolorCursor,
    XETSwQueryBestSize,
    XETSwQueryExtension,
    XETSwSimpleReq,			/* ListExtensions, */
    XETSwChangeKeyboardMapping,		/* 100 */
    XETSwSimpleReq,			/* GetKeyboardMapping, */
    XETSwChangeKeyboardControl,
    XETSwSimpleReq,			/* GetKeyboardControl, */
    XETSwSimpleReq,			/* Bell, */
    XETSwChangePointerControl,		/* 105 */
    XETSwSimpleReq,			/* GetPointerControl, */
    XETSwSetScreenSaver,
    XETSwSimpleReq,			/* GetScreenSaver, */
    XETSwChangeHosts,
    XETSwSimpleReq,			/* 110 ListHosts, */
    XETSwSimpleReq,			/* XETSwChangeAccessControl, */
    XETSwSimpleReq,			/* XETSwChangeCloseDownMode, */
    XETSwResourceReq,			/* XETSwKillClient, */
    XETSwRotateProperties,
    XETSwSimpleReq,			/* 115 ForceScreenSaver */
    XETSwSimpleReq,			/* SetPointerMapping, */
    XETSwSimpleReq,			/* GetPointerMapping, */
    XETSwSimpleReq,			/* SetModifierMapping, */
    XETSwSimpleReq,			/* GetModifierMapping, */
    NotImplemented,			/* 120 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 125 */
    NotImplemented,
    XETSwNoOperation
};

/* NOTE: This array must align with the values of the constants used
 * as minor_opcode values in the request structure. Failure to do this
 * could result in random code paths.
 */
globaldef int_function XETrapDispatchVector[10L] = 
{
    XETrapReset,            /* 0 XETrap_Reset */
    XETrapGetAvailable,     /* 1 XETrap_GetAvailable */
    XETrapConfig,           /* 2 XETrap_Config */
    XETrapStartTrap,        /* 3 XETrap_StartTrap */
    XETrapStopTrap,         /* 4 XETrap_StopTrap */
    XETrapGetCurrent,       /* 5 XETrap_GetCurrent */
    XETrapGetStatistics,    /* 6 XETrap_GetStatistics */
#ifndef _XINPUT
    XETrapSimulateXEvent,   /* 7 XETrap_SimulateXEvent */
#endif
    XETrapGetVersion,       /* 8 XETrap_GetVersion */
    XETrapGetLastInpTime,   /* 9 XETrap_GetLastInpTime */
};

/* NOTE: This array must align with the values of the constants used
 * as minor_opcode values in the request structure. Failure to do this
 * could result in random code paths.
 */
globaldef int_function XETSwDispatchVector[10L] = 
{
    sXETrapReset,           /* 0 XETrap_Reset */
    sXETrapGetAvailable,    /* 1 XETrap_GetAvailable */
    sXETrapConfig,          /* 2 XETrap_Config */
    sXETrapStartTrap,       /* 3 XETrap_StartTrap */
    sXETrapStopTrap,        /* 4 XETrap_StopTrap */
    sXETrapGetCurrent,      /* 5 XETrap_GetCurrent */
    sXETrapGetStatistics,   /* 6 XETrap_GetStatistics */
#ifndef _XINPUT
    sXETrapSimulateXEvent,  /* 7 XETrap_SimulateXEvent */
#endif
    sXETrapGetVersion,      /* 8 XETrap_GetVersion */
    sXETrapGetLastInpTime,  /* 9 XETrap_GetLastInpTime */
};

/* ProcVector shadow vector */
globaldef int_function XETrapProcVector[256L]       = {XETrapRequestVector};
/*
 * Events are faked as if they're vectored since that's
 * the way it'll eventually be (X11 R5?).
 */
#ifndef VECTORED_EVENTS
globaldef int_function EventProcVector[XETrapCoreEvents]       = {NULL};
#endif
globaldef int_function XETrapEventProcVector[XETrapCoreEvents] = {NULL};


