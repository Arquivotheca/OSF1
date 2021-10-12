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
/* $XConsortium: xstubs.c,v 1.8 91/07/24 15:48:40 rws Exp $ */

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

#define	 NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "XI.h"
#include "XIproto.h"

/***********************************************************************
 *
 * Caller:	ProcXChangeKeyboardDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the keyboard device.
 *
 * The X keyboard device has a FocusRec.  If the device that has been 
 * made into the new X keyboard did not have a FocusRec, 
 * ProcXChangeKeyboardDevice will allocate one for it.
 *
 * If you do not want clients to be able to focus the old X keyboard
 * device, call DeleteFocusClassDeviceStruct to free the FocusRec.
 *
 * If you support input devices with keys that you do not want to be 
 * used as the X keyboard, you need to check for them here and return 
 * a BadDevice error.
 *
 * The default implementation is to do nothing (assume you do want
 * clients to be able to focus the old X keyboard).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 */

int
ChangeKeyboardDevice (old_dev, new_dev)
    DeviceIntPtr	old_dev;
    DeviceIntPtr	new_dev;
    {
    /***********************************************************************
     DeleteFocusClassDeviceStruct(old_dev);	 * defined in xchgptr.c *
    **********************************************************************/
    }


/***********************************************************************
 *
 * Caller:	ProcXChangePointerDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the pointer device.
 *
 * The X pointer device does not have a FocusRec.  If the device that
 * has been made into the new X pointer had a FocusRec, 
 * ProcXChangePointerDevice will free it.
 *
 * If you want clients to be able to focus the old pointer device that
 * has now become accessible through the input extension, you need to 
 * add a FocusRec to it here.
 *
 * The XChangePointerDevice protocol request also allows the client
 * to choose which axes of the new pointer device are used to move 
 * the X cursor in the X- and Y- directions.  If the axes are different
 * than the default ones, you need to keep track of that here.
 *
 * If you support input devices with valuators that you do not want to be 
 * used as the X pointer, you need to check for them here and return a 
 * BadDevice error.
 *
 * The default implementation is to do nothing (assume you don't want
 * clients to be able to focus the old X pointer).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 */

int
ChangePointerDevice (old_dev, new_dev, x, y)
    DeviceIntPtr	old_dev, new_dev;
    unsigned char	x, y;
    {
    /***********************************************************************
    InitFocusClassDeviceStruct(old_dev);	* allow focusing old ptr*

    x_axis = x;					* keep track of new x-axis*
    y_axis = y;					* keep track of new y-axis*
    if (x_axis != 0 || y_axis != 1)
	axes_changed = TRUE;			* remember axes have changed*
    else
	axes_changed = FALSE;
    *************************************************************************/
    }

/***********************************************************************
 *
 * Caller:	ProcXCloseDevice
 *
 * Take care of implementation-dependent details of closing a device.
 * Some implementations may actually close the device, others may just
 * remove this clients interest in that device.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 *
 */

void
CloseInputDevice (d, client)
    DeviceIntPtr d;
    ClientPtr client;
    {
    }

/***********************************************************************
 *
 * Caller:	ProcXListInputDevices
 *
 * This is the implementation-dependent routine to initialize an input 
 * device to the point that information about it can be listed.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  If some other process has the device open, the
 * server may not be able to get information about the device to list it.
 *
 * This procedure should be used by implementations that do not initialize
 * all input devices at server startup.  It should do device-dependent
 * initialization for any devices not previously initialized, and call
 * AddInputDevice for each of those devices so that a DeviceIntRec will be 
 * created for them.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 * The commented-out sample code shows what you might do if you don't want 
 * the default.
 *
 */

void
AddOtherInputDevices ()
    {
    DeviceIntPtr dev;
    DeviceProc deviceProc;
    pointer private;

    /**********************************************************************
     for each uninitialized device, do something like: 

    dev = (DeviceIntPtr) AddInputDevice(deviceProc, TRUE);
    dev->public.devicePrivate = private;
    RegisterOtherDevice(dev);
    dev->inited = ((*dev->deviceProc)(dev, DEVICE_INIT) == Success);
    ************************************************************************/

    }

/***********************************************************************
 *
 * Caller:	ProcXOpenDevice
 *
 * This is the implementation-dependent routine to open an input device.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  This entry point is for the latter type of 
 * implementation.
 *
 * If the physical device is not already open, do it here.  In this case,
 * you need to keep track of the fact that one or more clients has the
 * device open, and physically close it when the last client that has
 * it open does an XCloseDevice.
 *
 * The default implementation is to do nothing (assume all input devices
 * are opened during X server initialization and kept open).
 *
 */

void
OpenInputDevice (dev, client, status)
    DeviceIntPtr dev;
    ClientPtr client;
    int *status;
    {
    }

/****************************************************************************
 *
 * Caller:	ProcXSetDeviceMode
 *
 * Change the mode of an extension device.
 * This function is used to change the mode of a device from reporting
 * relative motion to reporting absolute positional information, and
 * vice versa.
 * The default implementation below is that no such devices are supported.
 *
 */

int
SetDeviceMode (client, dev, mode)
    register	ClientPtr	client;
    DeviceIntPtr dev;
    int		mode;
    {
    return BadMatch;
    }

/****************************************************************************
 *
 * Caller:	ProcXSetDeviceValuators
 *
 * Set the value of valuators on an extension input device.
 * This function is used to set the initial value of valuators on
 * those input devices that are capable of reporting either relative
 * motion or an absolute position, and allow an initial position to be set.
 * The default implementation below is that no such devices are supported.
 *
 */

int
SetDeviceValuators (client, dev, valuators, first_valuator, num_valuators)
    register	ClientPtr	client;
    DeviceIntPtr dev;
    int		*valuators;
    int		first_valuator;
    int		num_valuators;
    {
    return BadMatch;
    }

/****************************************************************************
 *
 * Caller:	ProcXChangeDeviceControl
 *
 * Change the specified device controls on an extension input device.
 *
 */

int
ChangeDeviceControl (client, dev, control)
    register	ClientPtr	client;
    DeviceIntPtr dev;
    xDeviceCtl	*control;
    {
    switch (control->control)
	{
	case DEVICE_RESOLUTION:
	    return (BadMatch);
	default:
	    return (BadMatch);
	}
    }
