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
/* $XConsortium: xextinit.c,v 1.13 91/02/22 15:31:41 rws Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Hewlett-Packard or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/********************************************************************
 *
 *  Dispatch routines and initialization routines for the X input extension.
 *
 */

#define	 NUMTYPES 15

#define	 NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "gcstruct.h"   		/* pointer for extnsionst.h*/
#include "extnsionst.h"			/* extension entry   */
#include "XI.h"
#include "XIproto.h"

static Mask 	lastExtEventMask = 1;
int  		ExtEventIndex;
Mask		ExtValidMasks[EMASKSIZE];
Mask		ExtExclusiveMasks[EMASKSIZE];

struct  dev_type
    {
    Atom	type;
    char	*name;
    }dev_type [] = {{0,XI_KEYBOARD},
	    {0,XI_MOUSE},
	    {0,XI_TABLET},
	    {0,XI_TOUCHSCREEN},
	    {0,XI_TOUCHPAD},
	    {0,XI_BARCODE},
	    {0,XI_BUTTONBOX},
	    {0,XI_KNOB_BOX},
	    {0,XI_ONE_KNOB},
	    {0,XI_NINE_KNOB},
	    {0,XI_TRACKBALL},
	    {0,XI_QUADRATURE},
	    {0,XI_ID_MODULE},
	    {0,XI_SPACEBALL},
	    {0,XI_DATAGLOVE},
	    {0,XI_EYETRACKER},
	    {0,XI_CURSORKEYS},
	    {0,XI_FOOTMOUSE}};

CARD8  event_base [numInputClasses];
XExtEventInfo EventInfo[32];

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */

int 	IReqCode = 0;
int	BadDevice = 0;
int	BadEvent = 1;
int	BadMode = 2;
int	DeviceBusy = 3;
int	BadClass = 4;

Mask	DevicePointerMotionMask;
Mask	DevicePointerMotionHintMask;
Mask	DeviceFocusChangeMask;
Mask	DeviceStateNotifyMask;
Mask	ChangeDeviceNotifyMask;
Mask	DeviceMappingNotifyMask;
Mask	DeviceOwnerGrabButtonMask;
Mask	DeviceButtonGrabMask;
Mask	DeviceButton1Mask;
Mask	DeviceButtonMotionMask;

int	DeviceValuator;
int	DeviceKeyPress;
int	DeviceKeyRelease;
int	DeviceButtonPress;
int	DeviceButtonRelease;
int	DeviceMotionNotify;
int	DeviceFocusIn;
int	DeviceFocusOut;
int	ProximityIn;
int	ProximityOut;
int	DeviceStateNotify;
int	DeviceKeyStateNotify;
int	DeviceButtonStateNotify;
int	DeviceMappingNotify;
int	ChangeDeviceNotify;

int	RT_INPUTCLIENT;

/*****************************************************************
 *
 * Externs defined elsewhere in the X server.
 *
 */

extern	void			(* ReplySwapVector[256]) ();
extern	void			(* EventSwapVector[128]) ();
extern	XExtensionVersion	AllExtensionVersions[];
extern	InputInfo		inputInfo;
Mask	PropagateMask[MAX_DEVICES];

/*****************************************************************
 *
 * Declarations of local routines.
 *
 */

int	ProcIDispatch();
int	SProcIDispatch();
void	SReplyIDispatch();
void	IResetProc();
void	SEventIDispatch();
void	NotImplemented();
static	XExtensionVersion	thisversion = 
					{XI_Present, 
					 XI_Add_XSetDeviceValuators_Major, 
					 XI_Add_XSetDeviceValuators_Minor};

/**********************************************************************
 *
 * IExtensionInit - initialize the input extension.
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * This extension has several events and errors.
 *
 */

void
XInputExtensionInit()
    {
    ExtensionEntry *extEntry, *AddExtension();
    int XShutDownDevice();
    int InputClientGone();

    extEntry = AddExtension(INAME, IEVENTS, IERRORS, ProcIDispatch,
		   SProcIDispatch, IResetProc, StandardMinorOpcode);
    if (extEntry)
        {
	IReqCode = extEntry->base;
	AllExtensionVersions[IReqCode-128] = thisversion;
	(void)MakeAtom(INAME, 11, TRUE);
	MakeDeviceTypeAtoms ();
	RT_INPUTCLIENT = CreateNewResourceType(InputClientGone);
	FixExtensionEvents (extEntry);
	ReplySwapVector[IReqCode] = SReplyIDispatch;
	EventSwapVector[DeviceValuator] = SEventIDispatch;
	EventSwapVector[DeviceKeyPress] = SEventIDispatch;
	EventSwapVector[DeviceKeyRelease] = SEventIDispatch;
	EventSwapVector[DeviceButtonPress] = SEventIDispatch;
	EventSwapVector[DeviceButtonRelease] = SEventIDispatch;
	EventSwapVector[DeviceMotionNotify] = SEventIDispatch;
	EventSwapVector[DeviceFocusIn] = SEventIDispatch;
	EventSwapVector[DeviceFocusOut] = SEventIDispatch;
	EventSwapVector[ProximityIn] = SEventIDispatch;
	EventSwapVector[ProximityOut] = SEventIDispatch;
	EventSwapVector[DeviceStateNotify] = SEventIDispatch;
	EventSwapVector[DeviceKeyStateNotify] = SEventIDispatch;
	EventSwapVector[DeviceButtonStateNotify] = SEventIDispatch;
	EventSwapVector[DeviceMappingNotify] = SEventIDispatch;
	EventSwapVector[ChangeDeviceNotify] = SEventIDispatch;
	}
    else 
	{
	FatalError("IExtensionInit: AddExtensions failed\n");
	}
    }

/*************************************************************************
 *
 * ProcIDispatch - main dispatch routine for requests to this extension.
 * This routine is used if server and client have the same byte ordering.
 *
 */

int
ProcIDispatch (client)
    register ClientPtr client;
    {
    REQUEST(xReq);
    if (stuff->data == X_GetExtensionVersion)
	return(ProcXGetExtensionVersion(client));
    if (stuff->data == X_ListInputDevices)
	return(ProcXListInputDevices(client));
    else if (stuff->data == X_OpenDevice)
        return(ProcXOpenDevice(client));
    else if (stuff->data == X_CloseDevice)
        return(ProcXCloseDevice(client));
    else if (stuff->data == X_SetDeviceMode)
	return(ProcXSetDeviceMode(client));
    else if (stuff->data == X_SelectExtensionEvent)
	return(ProcXSelectExtensionEvent(client));
    else if (stuff->data == X_GetSelectedExtensionEvents)
        return(ProcXGetSelectedExtensionEvents(client));
    else if (stuff->data == X_ChangeDeviceDontPropagateList)
        return(ProcXChangeDeviceDontPropagateList(client));
    else if (stuff->data == X_GetDeviceDontPropagateList)
        return(ProcXGetDeviceDontPropagateList(client));
    else if (stuff->data == X_GetDeviceMotionEvents)
	return(ProcXGetDeviceMotionEvents(client));
    else if (stuff->data == X_ChangeKeyboardDevice)
	return(ProcXChangeKeyboardDevice(client));
    else if (stuff->data == X_ChangePointerDevice)
	return(ProcXChangePointerDevice(client));
    else if (stuff->data == X_GrabDevice)
	return(ProcXGrabDevice(client));
    else if (stuff->data == X_UngrabDevice)
	return(ProcXUngrabDevice(client));
    else if (stuff->data == X_GrabDeviceKey)
	return(ProcXGrabDeviceKey(client));
    else if (stuff->data == X_UngrabDeviceKey)
	return(ProcXUngrabDeviceKey(client));
    else if (stuff->data == X_GrabDeviceButton)
	return(ProcXGrabDeviceButton(client));
    else if (stuff->data == X_UngrabDeviceButton)
	return(ProcXUngrabDeviceButton(client));
    else if (stuff->data == X_AllowDeviceEvents)
        return(ProcXAllowDeviceEvents(client));
    else if (stuff->data == X_GetDeviceFocus)
	return(ProcXGetDeviceFocus(client));
    else if (stuff->data == X_SetDeviceFocus)
	return(ProcXSetDeviceFocus(client));
    else if (stuff->data == X_GetFeedbackControl)
	return(ProcXGetFeedbackControl(client));
    else if (stuff->data == X_ChangeFeedbackControl)
	return(ProcXChangeFeedbackControl(client));
    else if (stuff->data == X_GetDeviceKeyMapping)
        return(ProcXGetDeviceKeyMapping(client));
    else if (stuff->data == X_ChangeDeviceKeyMapping)
        return(ProcXChangeDeviceKeyMapping(client));
    else if (stuff->data == X_GetDeviceModifierMapping)
        return(ProcXGetDeviceModifierMapping(client));
    else if (stuff->data == X_SetDeviceModifierMapping)
        return(ProcXSetDeviceModifierMapping(client));
    else if (stuff->data == X_GetDeviceButtonMapping)
        return(ProcXGetDeviceButtonMapping(client));
    else if (stuff->data == X_SetDeviceButtonMapping)
        return(ProcXSetDeviceButtonMapping(client));
    else if (stuff->data == X_QueryDeviceState)
        return(ProcXQueryDeviceState(client));
    else if (stuff->data == X_SendExtensionEvent)
        return(ProcXSendExtensionEvent(client));
    else if (stuff->data == X_DeviceBell)
        return(ProcXDeviceBell(client));
    else if (stuff->data == X_SetDeviceValuators)
        return(ProcXSetDeviceValuators(client));
    else
        {
	SendErrorToClient(client, IReqCode, stuff->data, 0, BadRequest);
        }
    return(BadRequest);
    }

/*******************************************************************************
 *
 * SProcXDispatch 
 *
 * Main swapped dispatch routine for requests to this extension.
 * This routine is used if server and client do not have the same byte ordering.
 *
 */

int
SProcIDispatch(client)
    register ClientPtr client;
    {
    REQUEST(xReq);
    if (stuff->data == X_GetExtensionVersion)
	return(SProcXGetExtensionVersion(client));
    if (stuff->data == X_ListInputDevices)
	return(SProcXListInputDevices(client));
    else if (stuff->data == X_OpenDevice)
        return(SProcXOpenDevice(client));
    else if (stuff->data == X_CloseDevice)
        return(SProcXCloseDevice(client));
    else if (stuff->data == X_SetDeviceMode)
	return(SProcXSetDeviceMode(client));
    else if (stuff->data == X_SelectExtensionEvent)
	return(SProcXSelectExtensionEvent(client));
    else if (stuff->data == X_GetSelectedExtensionEvents)
        return(SProcXGetSelectedExtensionEvents(client));
    else if (stuff->data == X_ChangeDeviceDontPropagateList)
        return(SProcXChangeDeviceDontPropagateList(client));
    else if (stuff->data == X_GetDeviceDontPropagateList)
        return(SProcXGetDeviceDontPropagateList(client));
    else if (stuff->data == X_GetDeviceMotionEvents)
	return(SProcXGetDeviceMotionEvents(client));
    else if (stuff->data == X_ChangeKeyboardDevice)
	return(SProcXChangeKeyboardDevice(client));
    else if (stuff->data == X_ChangePointerDevice)
	return(SProcXChangePointerDevice(client));
    else if (stuff->data == X_GrabDevice)
	return(SProcXGrabDevice(client));
    else if (stuff->data == X_UngrabDevice)
	return(SProcXUngrabDevice(client));
    else if (stuff->data == X_GrabDeviceKey)
	return(SProcXGrabDeviceKey(client));
    else if (stuff->data == X_UngrabDeviceKey)
	return(SProcXUngrabDeviceKey(client));
    else if (stuff->data == X_GrabDeviceButton)
	return(SProcXGrabDeviceButton(client));
    else if (stuff->data == X_UngrabDeviceButton)
	return(SProcXUngrabDeviceButton(client));
    else if (stuff->data == X_AllowDeviceEvents)
        return(SProcXAllowDeviceEvents(client));
    else if (stuff->data == X_GetDeviceFocus)
	return(SProcXGetDeviceFocus(client));
    else if (stuff->data == X_SetDeviceFocus)
	return(SProcXSetDeviceFocus(client));
    else if (stuff->data == X_GetFeedbackControl)
	return(SProcXGetFeedbackControl(client));
    else if (stuff->data == X_ChangeFeedbackControl)
	return(SProcXChangeFeedbackControl(client));
    else if (stuff->data == X_GetDeviceKeyMapping)
        return(SProcXGetDeviceKeyMapping(client));
    else if (stuff->data == X_ChangeDeviceKeyMapping)
        return(SProcXChangeDeviceKeyMapping(client));
    else if (stuff->data == X_GetDeviceModifierMapping)
        return(SProcXGetDeviceModifierMapping(client));
    else if (stuff->data == X_SetDeviceModifierMapping)
        return(SProcXSetDeviceModifierMapping(client));
    else if (stuff->data == X_GetDeviceButtonMapping)
        return(SProcXGetDeviceButtonMapping(client));
    else if (stuff->data == X_SetDeviceButtonMapping)
        return(SProcXSetDeviceButtonMapping(client));
    else if (stuff->data == X_QueryDeviceState)
        return(SProcXQueryDeviceState(client));
    else if (stuff->data == X_SendExtensionEvent)
        return(SProcXSendExtensionEvent(client));
    else if (stuff->data == X_DeviceBell)
        return(SProcXDeviceBell(client));
    else if (stuff->data == X_SetDeviceValuators)
        return(SProcXSetDeviceValuators(client));
    else
        {
	SendErrorToClient(client, IReqCode, stuff->data, 0, BadRequest);
        }
    return(BadRequest);
    }

/**********************************************************************
 *
 * SReplyIDispatch
 * Swap any replies defined in this extension.
 *
 */

void
SReplyIDispatch (client, len, rep)
    ClientPtr		client;
    int			len;
    xGrabDeviceReply	*rep;		/* All we look at is the type field */
    {					/* This is common to all replies    */
    if (rep->RepType == X_GetExtensionVersion)
	SRepXGetExtensionVersion (client, len, rep);
    else if (rep->RepType == X_ListInputDevices)
	SRepXListInputDevices (client, len, rep);
    else if (rep->RepType == X_OpenDevice)
	SRepXOpenDevice (client, len, rep);
    else if (rep->RepType == X_SetDeviceMode)
	SRepXSetDeviceMode (client, len, rep);
    else if (rep->RepType == X_GetSelectedExtensionEvents)
	SRepXGetSelectedExtensionEvents (client, len, rep);
    else if (rep->RepType == X_GetDeviceDontPropagateList)
	SRepXGetDeviceDontPropagateList (client, len, rep);
    else if (rep->RepType == X_GetDeviceMotionEvents)
	SRepXGetDeviceMotionEvents (client, len, rep);
    else if (rep->RepType == X_ChangeKeyboardDevice)
	SRepXChangeKeyboardDevice (client, len, rep);
    else if (rep->RepType == X_ChangePointerDevice)
	SRepXChangePointerDevice (client, len, rep);
    else if (rep->RepType == X_GrabDevice)
	SRepXGrabDevice (client, len, rep);
    else if (rep->RepType == X_GetDeviceFocus)
	SRepXGetDeviceFocus (client, len, rep);
    else if (rep->RepType == X_GetFeedbackControl)
	SRepXGetFeedbackControl (client, len, rep);
    else if (rep->RepType == X_GetDeviceKeyMapping)
	SRepXGetDeviceKeyMapping (client, len, rep);
    else if (rep->RepType == X_GetDeviceModifierMapping)
	SRepXGetDeviceModifierMapping (client, len, rep);
    else if (rep->RepType == X_SetDeviceModifierMapping)
	SRepXSetDeviceModifierMapping (client, len, rep);
    else if (rep->RepType == X_GetDeviceButtonMapping)
	SRepXGetDeviceButtonMapping (client, len, rep);
    else if (rep->RepType == X_SetDeviceButtonMapping)
	SRepXSetDeviceButtonMapping (client, len, rep);
    else if (rep->RepType == X_QueryDeviceState)
	SRepXQueryDeviceState (client, len, rep);
    else if (rep->RepType == X_SetDeviceValuators)
	SRepXSetDeviceValuators (client, len, rep);
    else
	{
	SendErrorToClient(client, IReqCode, rep, 0, BadRequest);
	}
    }

/*****************************************************************************
 *
 *	SEventIDispatch
 *
 *	Swap any events defined in this extension.
 */
void
SEventIDispatch (from, to)
    xEvent	*from;
    xEvent	*to;
    {
    int		type = from->u.u.type & 0177;

    if (type == DeviceValuator)
	SEventDeviceValuator (from, to);
    else if (type == DeviceKeyPress)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceKeyRelease)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceButtonPress)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceButtonRelease)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceMotionNotify)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceFocusIn)
        SEventFocus (from, to);
    else if (type == DeviceFocusOut)
        SEventFocus (from, to);
    else if (type == ProximityIn)
	{
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == ProximityOut)
	{ 
        SKeyButtonPtrEvent (from, to);
	to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
	}
    else if (type == DeviceStateNotify)
        SDeviceStateNotifyEvent (from, to);
    else if (type == DeviceKeyStateNotify)
        SDeviceKeyStateNotifyEvent (from, to);
    else if (type == DeviceButtonStateNotify)
        SDeviceButtonStateNotifyEvent (from, to);
    else if (type == DeviceMappingNotify)
        SDeviceMappingNotifyEvent (from, to);
    else if (type == ChangeDeviceNotify)
        SChangeDeviceNotifyEvent (from, to);
    else
	{
	FatalError("XInputExtension: Impossible event!\n");
	}
    }

/************************************************************************
 *
 * This function swaps the DeviceValuator event.
 *
 */

SEventDeviceValuator (from, to)
    deviceValuator	*from;
    deviceValuator	*to;
    {
    register char	n;
    register int	i;
    INT32 *ip B32;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    swaps(&to->device_state,n);
    ip = &to->valuator0;
    for (i=0; i<6; i++)
	{
        swapl((ip+i),n);	/* macro - braces are required	    */
	}
    }

SEventFocus (from, to)
    deviceFocus	*from;
    deviceFocus	*to;
    {
    register char	n;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    swapl(&to->time, n);
    swapl(&to->window, n);
    }

SDeviceStateNotifyEvent (from, to)
    deviceStateNotify	*from;
    deviceStateNotify	*to;
    {
    register int	i;
    register char	n;
    INT32 *ip B32;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    swapl(&to->time, n);
    ip = &to->valuator0;
    for (i=0; i<3; i++)
	{
        swapl((ip+i),n);	/* macro - braces are required	    */
	}
    }

SDeviceKeyStateNotifyEvent (from, to)
    deviceKeyStateNotify	*from;
    deviceKeyStateNotify	*to;
    {
    register char	n;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    }

SDeviceButtonStateNotifyEvent (from, to)
    deviceButtonStateNotify	*from;
    deviceButtonStateNotify	*to;
    {
    register char	n;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    }

SChangeDeviceNotifyEvent (from, to)
    changeDeviceNotify	*from;
    changeDeviceNotify	*to;
    {
    register char	n;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    swapl(&to->time, n);
    }

SDeviceMappingNotifyEvent (from, to)
    deviceMappingNotify	*from;
    deviceMappingNotify	*to;
    {
    register char	n;

    *to = *from;
    swaps(&to->sequenceNumber,n);
    swapl(&to->time, n);
    }

/************************************************************************
 *
 * This function sets up extension event types and masks.
 *
 */

FixExtensionEvents (extEntry)
    ExtensionEntry 	*extEntry;
    {
    Mask		mask, GetNextExtEventMask();
    void		SetMaskForExtEvent();
    void		SetEventInfo();
    void		AllowPropagateSuppress();
    void		SetExclusiveAccess();

    DeviceValuator  	    = extEntry->eventBase;
    DeviceKeyPress   	    = DeviceValuator + 1;
    DeviceKeyRelease 	    = DeviceKeyPress + 1;
    DeviceButtonPress       = DeviceKeyRelease + 1;
    DeviceButtonRelease     = DeviceButtonPress + 1;
    DeviceMotionNotify      = DeviceButtonRelease + 1;
    DeviceFocusIn  	    = DeviceMotionNotify + 1;
    DeviceFocusOut	    = DeviceFocusIn + 1;
    ProximityIn  	    = DeviceFocusOut + 1;
    ProximityOut  	    = ProximityIn + 1;
    DeviceStateNotify  	    = ProximityOut + 1;
    DeviceMappingNotify     = DeviceStateNotify + 1;
    ChangeDeviceNotify      = DeviceMappingNotify + 1;
    DeviceKeyStateNotify    = ChangeDeviceNotify + 1;
    DeviceButtonStateNotify = DeviceKeyStateNotify + 1;

    event_base[KeyClass] = DeviceKeyPress;
    event_base[ButtonClass] = DeviceButtonPress;
    event_base[ValuatorClass] = DeviceMotionNotify;
    event_base[ProximityClass] = ProximityIn;
    event_base[FocusClass] = DeviceFocusIn;
    event_base[OtherClass] = DeviceStateNotify;

    BadDevice += extEntry->errorBase;
    BadEvent += extEntry->errorBase;
    BadMode += extEntry->errorBase;
    DeviceBusy += extEntry->errorBase;
    BadClass += extEntry->errorBase;

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, DeviceKeyPress);
    AllowPropagateSuppress (mask);

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, DeviceKeyRelease);
    AllowPropagateSuppress (mask);

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, DeviceButtonPress);
    AllowPropagateSuppress (mask);

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, DeviceButtonRelease);
    AllowPropagateSuppress (mask);

    mask = GetNextExtEventMask ();
    DevicePointerMotionMask = mask;
    SetMaskForExtEvent (mask, DeviceMotionNotify);
    AllowPropagateSuppress (mask);

    DeviceFocusChangeMask = GetNextExtEventMask ();
    SetMaskForExtEvent (DeviceFocusChangeMask, DeviceFocusIn);
    SetMaskForExtEvent (DeviceFocusChangeMask, DeviceFocusOut);

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, ProximityIn);
    SetMaskForExtEvent (mask, ProximityOut);
    AllowPropagateSuppress (mask);

    mask = GetNextExtEventMask ();
    DeviceStateNotifyMask = mask;
    SetMaskForExtEvent (mask, DeviceStateNotify);

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, DeviceMappingNotify);
    DeviceMappingNotifyMask = mask;

    mask = GetNextExtEventMask ();
    SetMaskForExtEvent (mask, ChangeDeviceNotify);
    ChangeDeviceNotifyMask = mask;

    DevicePointerMotionHintMask = GetNextExtEventMask();
    SetEventInfo (DevicePointerMotionHintMask, _devicePointerMotionHint);
    DeviceButton1Mask = GetNextExtEventMask();
    SetEventInfo (DeviceButton1Mask, _deviceButton1Motion);
    SetEventInfo (GetNextExtEventMask(), _deviceButton2Motion);
    SetEventInfo (GetNextExtEventMask(), _deviceButton3Motion);
    SetEventInfo (GetNextExtEventMask(), _deviceButton4Motion);
    SetEventInfo (GetNextExtEventMask(), _deviceButton5Motion);
    DeviceButtonMotionMask = GetNextExtEventMask();
    SetEventInfo (DeviceButtonMotionMask, _deviceButtonMotion);

    DeviceButtonGrabMask = GetNextExtEventMask();
    SetEventInfo (DeviceButtonGrabMask, _deviceButtonGrab);
    SetExclusiveAccess (DeviceButtonGrabMask);

    DeviceOwnerGrabButtonMask = GetNextExtEventMask();
    SetEventInfo (DeviceOwnerGrabButtonMask, _deviceOwnerGrabButton);
    SetEventInfo (0, _noExtensionEvent);
    }

/************************************************************************
 *
 * This function restores extension event types and masks to their 
 * initial state.
 *
 */

RestoreExtensionEvents ()
    {
    int	i;

    IReqCode = 0;

    for (i=0; i<ExtEventIndex-1; i++)
	{
	if ((EventInfo[i].type >= LASTEvent) && (EventInfo[i].type < 128))
	    SetMaskForEvent(0,EventInfo[i].type);
        EventInfo[i].mask = 0;
        EventInfo[i].type = 0;
	}
    ExtEventIndex = 0;
    lastExtEventMask = 1;
    DeviceValuator = 0;
    DeviceKeyPress = 1;
    DeviceKeyRelease = 2;
    DeviceButtonPress = 3;
    DeviceButtonRelease = 4;
    DeviceMotionNotify = 5;
    DeviceFocusIn = 6;
    DeviceFocusOut = 7;
    ProximityIn = 8;
    ProximityOut = 9;
    DeviceStateNotify = 10;
    DeviceMappingNotify = 11;
    ChangeDeviceNotify = 12;
    DeviceKeyStateNotify = 13;
    DeviceButtonStateNotify = 13;

    BadDevice = 0;
    BadEvent = 1;
    BadMode = 2;
    DeviceBusy = 3;
    BadClass = 4;

    }

/***********************************************************************
 *
 * IResetProc.
 * Remove reply-swapping routine.
 * Remove event-swapping routine.
 *
 */

void
IResetProc()
    {

    ReplySwapVector[IReqCode] = NotImplemented;
    EventSwapVector[DeviceValuator] = NotImplemented;
    EventSwapVector[DeviceKeyPress] = NotImplemented;
    EventSwapVector[DeviceKeyRelease] = NotImplemented;
    EventSwapVector[DeviceButtonPress] = NotImplemented;
    EventSwapVector[DeviceButtonRelease] = NotImplemented;
    EventSwapVector[DeviceMotionNotify] = NotImplemented;
    EventSwapVector[DeviceFocusIn] = NotImplemented;
    EventSwapVector[DeviceFocusOut] = NotImplemented;
    EventSwapVector[ProximityIn] = NotImplemented;
    EventSwapVector[ProximityOut] = NotImplemented;
    EventSwapVector[DeviceStateNotify] = NotImplemented;
    EventSwapVector[DeviceKeyStateNotify] = NotImplemented;
    EventSwapVector[DeviceButtonStateNotify] = NotImplemented;
    EventSwapVector[DeviceMappingNotify] = NotImplemented;
    EventSwapVector[ChangeDeviceNotify] = NotImplemented;
    RestoreExtensionEvents ();
    }

/***********************************************************************
 *
 * Assign an id and type to an input device.
 *
 */

int
AssignTypeAndName (dev, type, name)
    DeviceIntPtr dev;
    Atom type;
    char *name;
    {
    dev->type = type;
    dev->name = (char *) xalloc(strlen(name)+1);
    strcpy (dev->name, name);
    }

/***********************************************************************
 *
 * Make device type atoms.
 *
 */

int
MakeDeviceTypeAtoms ()
    {
    int i;

    for (i=0; i<NUMTYPES; i++)
	dev_type[i].type = 
		MakeAtom (dev_type[i].name, strlen(dev_type[i].name), 1);
    }

/**************************************************************************
 *
 * Return a DeviceIntPtr corresponding to a specified device id.
 * This will not return the pointer or keyboard, or devices that are not on.
 *
 */

DeviceIntPtr
LookupDeviceIntRec (id)
    CARD8 id;
    {
    DeviceIntPtr dev;

    for (dev=inputInfo.devices; dev; dev=dev->next)
	{
	if (dev->id == id)
	    {
	    if (id == inputInfo.pointer->id || id == inputInfo.keyboard->id)
		return (NULL);
	    return (dev);
	    }
	}
    return (NULL);
    }

/**************************************************************************
 *
 * Allow the specified event to be restricted to being selected by one
 * client at a time.
 * The default is to allow more than one client to select the event.
 *
 */

void
SetExclusiveAccess (mask)
    Mask mask;
    {
    int i;

    for (i=0; i<MAX_DEVICES; i++)
	ExtExclusiveMasks[i] |= mask;
    }

/**************************************************************************
 *
 * Allow the specified event to have its propagation suppressed.
 * The default is to not allow suppression of propagation.
 *
 */

void
AllowPropagateSuppress (mask)
    Mask mask;
    {
    int i;

    for (i=0; i<MAX_DEVICES; i++)
	PropagateMask[i] |= mask;
    }

/**************************************************************************
 *
 * Return the next available extension event mask.
 *
 */

Mask 
GetNextExtEventMask ()
    {
    int i;
    Mask mask = lastExtEventMask;

    if (lastExtEventMask == 0)
	{
	FatalError("GetNextExtEventMask: no more events are available.");
	}
    lastExtEventMask <<= 1;

    for (i=0; i<MAX_DEVICES; i++)
	ExtValidMasks[i] |= mask;
    return mask;
    }

/**************************************************************************
 *
 * Assign the specified mask to the specified event.
 *
 */

void
SetMaskForExtEvent(mask, event)
    Mask mask;
    int event;
    {

    EventInfo[ExtEventIndex].mask = mask;
    EventInfo[ExtEventIndex++].type = event;

    if ((event < LASTEvent) || (event >= 128))
	FatalError("MaskForExtensionEvent: bogus event number");
    SetMaskForEvent(mask,event);
    }

/**************************************************************************
 *
 * Record an event mask where there is no unique corresponding event type.
 * We can't call SetMaskForEvent, since that would clobber the existing
 * mask for that event.  MotionHint and ButtonMotion are examples.
 *
 * Since extension event types will never be less than 64, we can use
 * 0-63 in the EventInfo array as the "type" to be used to look up this
 * mask.  This means that the corresponding macros such as 
 * DevicePointerMotionHint must have access to the same constants.
 *
 */

void
SetEventInfo(mask, constant)
    Mask mask;
    int constant;
    {
    EventInfo[ExtEventIndex].mask = mask;
    EventInfo[ExtEventIndex++].type = constant;
    }
