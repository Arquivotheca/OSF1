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
/* $Header: /usr/sde/x11/rcs/x11/src/./extensions/server/xinput/xchgfctl.c,v 1.2 91/12/15 12:42:16 devrcs Exp $ */

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
 *  Change feedback control attributes for an extension device.
 *
 */

#define	 NEED_EVENTS			/* for inputstr.h    */
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "XI.h"
#include "XIproto.h"			/* control constants */

#define DO_ALL    (-1)

extern	int 	IReqCode;
extern	int	BadDevice;
DeviceIntPtr	LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure changes the control attributes for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int
SProcXChangeFeedbackControl(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xChangeFeedbackControlReq);
    swaps(&stuff->length, n);
    swapl(&stuff->mask, n);
    return(ProcXChangeFeedbackControl(client));
    }

/***********************************************************************
 *
 * Change the control attributes.
 *
 */

ProcXChangeFeedbackControl(client)
    ClientPtr client;
    {
    int len;
    DeviceIntPtr dev;
    KbdFeedbackPtr k;
    PtrFeedbackPtr p;
    IntegerFeedbackPtr i;
    StringFeedbackPtr s;
    BellFeedbackPtr b;
    LedFeedbackPtr l;

    REQUEST(xChangeFeedbackControlReq);
    REQUEST_AT_LEAST_SIZE(xChangeFeedbackControlReq);

    len = stuff->length - (sizeof(xChangeFeedbackControlReq) >>2);
    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadDevice);
	return Success;
	}

    switch (stuff->feedbackid)
	{
	case KbdFeedbackClass:
	    if (len != (sizeof(xKbdFeedbackCtl)>>2))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (k=dev->kbdfeed; k; k=k->next)
		if (k->ctrl.id == ((xKbdFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangeKbdFeedback (client, dev, stuff->mask, k, &stuff[1]);
		    return Success;
		    }
	    break;
	case PtrFeedbackClass:
	    if (len != (sizeof(xPtrFeedbackCtl)>>2))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (p=dev->ptrfeed; p; p=p->next)
		if (p->ctrl.id == ((xPtrFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangePtrFeedback (client, dev, stuff->mask, p, &stuff[1]);
		    return Success;
		    }
	    break;
	case StringFeedbackClass:
	    {
	    register char n;
	    xStringFeedbackCtl *f = ((xStringFeedbackCtl *) &stuff[1]);
	    if (client->swapped)
		{
		swaps(&f->num_keysyms,n);
		}
	    if (len != ((sizeof(xStringFeedbackCtl)>>2) + f->num_keysyms))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (s=dev->stringfeed; s; s=s->next)
		if (s->ctrl.id == ((xStringFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangeStringFeedback (client, dev, stuff->mask,s,&stuff[1]);
		    return Success;
		    }
	    break;
	    }
	case IntegerFeedbackClass:
	    if (len != (sizeof(xIntegerFeedbackCtl)>>2))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (i=dev->intfeed; i; i=i->next)
		if (i->ctrl.id == ((xIntegerFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangeIntegerFeedback (client, dev,stuff->mask,i,&stuff[1]);
		    return Success;
		    }
	    break;
	case LedFeedbackClass:
	    if (len != (sizeof(xLedFeedbackCtl)>>2))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (l=dev->leds; l; l=l->next)
		if (l->ctrl.id == ((xLedFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangeLedFeedback (client, dev, stuff->mask, l, &stuff[1]);
		    return Success;
		    }
	    break;
	case BellFeedbackClass:
	    if (len != (sizeof(xBellFeedbackCtl)>>2))
		{
		SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 
			0, BadLength);
		return Success;
		}
	    for (b=dev->bell; b; b=b->next)
		if (b->ctrl.id == ((xBellFeedbackCtl *) &stuff[1])->id)
		    {
		    ChangeBellFeedback (client, dev, stuff->mask, b, &stuff[1]);
		    return Success;
		    }
	    break;
	default:
	    break;
	}

    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, BadMatch);
    return Success;
    } 

/******************************************************************************
 *
 * This procedure changes KbdFeedbackClass data.
 *
 */

ChangeKbdFeedback (client, dev, mask, k, f)
    ClientPtr client;
    DeviceIntPtr dev;
    unsigned long 	mask;
    KbdFeedbackPtr	k;
    xKbdFeedbackCtl 	*f;
    {
    register char n;
    KeybdCtrl kctrl;
    int t;
    int key = DO_ALL;

    if (client->swapped)
	{
	swaps(&f->length,n);
	swaps(&f->pitch,n);
	swaps(&f->duration,n);
	swapl(&f->led_mask,n);
	swapl(&f->led_values,n);
	}

    kctrl = k->ctrl;
    if (mask & DvKeyClickPercent)
	{
	t = f->click;
	if (t == -1)
	    t = defaultKeyboardControl.click;
	else if (t < 0 || t > 100)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	kctrl.click = t;
        }

    if (mask & DvPercent)
	{
	t = f->percent;
	if (t == -1)
	    t = defaultKeyboardControl.bell;
	else if (t < 0 || t > 100)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	kctrl.bell = t;
	}

    if (mask & DvPitch)
	{
	t = f->pitch;
	if (t == -1)
	    t = defaultKeyboardControl.bell_pitch;
	else if (t < 0)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	kctrl.bell_pitch = t;
	}

    if (mask & DvDuration)
	{
	t = f->duration;
	if (t == -1)
	    t = defaultKeyboardControl.bell_duration;
	else if (t < 0)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	kctrl.bell_duration = t;
        }

    if (mask & DvLed)
        {
	kctrl.leds &= ~(f->led_mask);
	kctrl.leds |= (f->led_mask & f->led_values);
        }

    if (mask & DvKey)
	{
	key = (KeyCode) f->key;
	if (key < 8 || key > 255)
	    {
	    client->errorValue = key;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	if (!(mask & DvAutoRepeatMode))
	    {
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadMatch);
	    return Success;
	    }
	}

    if (mask & DvAutoRepeatMode)
	{
	int index = (key >> 3);
	int kmask = (1 << (key & 7));
	t = (CARD8) f->auto_repeat_mode;
	if (t == AutoRepeatModeOff)
	    {
	    if (key == DO_ALL)
		kctrl.autoRepeat = FALSE;
	    else
		kctrl.autoRepeats[index] &= ~kmask;
	    }
	else if (t == AutoRepeatModeOn)
	    {
	    if (key == DO_ALL)
		kctrl.autoRepeat = TRUE;
	    else
		kctrl.autoRepeats[index] |= kmask;
	    }
	else if (t == AutoRepeatModeDefault)
	    {
	    if (key == DO_ALL)
		kctrl.autoRepeat = defaultKeyboardControl.autoRepeat;
	    else
		kctrl.autoRepeats[index] &= ~kmask;
		kctrl.autoRepeats[index] =
			(kctrl.autoRepeats[index] & ~kmask) |
			(defaultKeyboardControl.autoRepeats[index] & kmask);
	    }
	else
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
        }

    k->ctrl = kctrl;
    (*k->CtrlProc)(dev, &k->ctrl);
    return Success;
    }

/******************************************************************************
 *
 * This procedure changes PtrFeedbackClass data.
 *
 */

ChangePtrFeedback (client, dev, mask, p, f)
    ClientPtr 		client;
    DeviceIntPtr 	dev;
    unsigned long 	mask;
    PtrFeedbackPtr 	p;
    xPtrFeedbackCtl 	*f;
    {
    register char n;
    PtrCtrl pctrl;		/* might get BadValue part way through */

    if (client->swapped)
	{
	swaps(&f->length,n);
	swaps(&f->num,n);
	swaps(&f->denom,n);
	swaps(&f->thresh,n);
	}

    pctrl = p->ctrl;
    if (mask & DvAccelNum)
	{
	int	accelNum;

	accelNum = f->num;
	if (accelNum == -1)
	    pctrl.num = defaultPointerControl.num;
	else if (accelNum < 0)
	    {
	    client->errorValue = accelNum;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	else pctrl.num = accelNum;
	}

    if (mask & DvAccelDenom)
	{
	int	accelDenom;

	accelDenom = f->denom;
	if (accelDenom == -1)
	    pctrl.den = defaultPointerControl.den;
	else if (accelDenom <= 0)
	    {
	    client->errorValue = accelDenom;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	else pctrl.den = accelDenom;
        }

    if (mask & DvThreshold)
	{
	int	threshold;

	threshold = f->thresh;
	if (threshold == -1)
	    pctrl.threshold = defaultPointerControl.threshold;
	else if (threshold < 0)
	    {
	    client->errorValue = threshold;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	else pctrl.threshold = threshold;
        }

    p->ctrl = pctrl;
    (*p->CtrlProc)(dev, &p->ctrl);
    return Success;
    }

/******************************************************************************
 *
 * This procedure changes IntegerFeedbackClass data.
 *
 */

ChangeIntegerFeedback (client, dev, mask, i, f)
    ClientPtr 			client;
    DeviceIntPtr 		dev;
    unsigned long 		mask;
    IntegerFeedbackPtr 		i;
    xIntegerFeedbackCtl 	*f;
    {
    register char n;

    if (client->swapped)
	{
	swaps(&f->length,n);
	swapl(&f->int_to_display,n);
	}

    i->ctrl.integer_displayed = f->int_to_display;
    (*i->CtrlProc)(dev, &i->ctrl);
    return Success;
    }

/******************************************************************************
 *
 * This procedure changes StringFeedbackClass data.
 *
 */

ChangeStringFeedback (client, dev, mask, s, f)
    ClientPtr 		client;
    DeviceIntPtr 	dev;
    unsigned long 	mask;
    StringFeedbackPtr 	s;
    xStringFeedbackCtl 	*f;
    {
    register char n;
    register long *p;
    int		i, j, len;
    KeySym	*syms, *sup_syms;

    syms = (KeySym *) (f+1);
    if (client->swapped)
	{
	swaps(&f->length,n);	/* swapped num_keysyms in calling proc */
	p = (long *) (syms);
	for (i=0; i<f->num_keysyms; i++)
	    {
	    swapl(p, n);
	    p++;
	    }
	}

    if (f->num_keysyms > s->ctrl.max_symbols)
	{
	SendErrorToClient(client, IReqCode, X_ChangeFeedbackControl, 0, 
	    BadValue);
	return Success;
	}
    sup_syms = s->ctrl.symbols_supported;
    for (i=0; i<f->num_keysyms; i++)
	{
        for (j=0; j<s->ctrl.num_symbols_supported; j++)
	    if (*(syms+i) == *(sup_syms+j))
		break;
	if (j==s->ctrl.num_symbols_supported)
	    {
	    SendErrorToClient(client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadMatch);
	    return Success;
	    }
	}

    s->ctrl.num_symbols_displayed  = f->num_keysyms;
    for (i=0; i<f->num_keysyms; i++)
	*(s->ctrl.symbols_displayed+i) = *(syms+i);
    (*s->CtrlProc)(dev, &s->ctrl);
    return Success;
    }

/******************************************************************************
 *
 * This procedure changes BellFeedbackClass data.
 *
 */

ChangeBellFeedback (client, dev, mask, b, f)
    ClientPtr 		client;
    DeviceIntPtr 	dev;
    unsigned long 	mask;
    BellFeedbackPtr 	b;
    xBellFeedbackCtl 	*f;
    {
    register char n;
    int t;
    BellCtrl bctrl;		/* might get BadValue part way through */

    if (client->swapped)
	{
	swaps(&f->length,n);
	swaps(&f->pitch,n);
	swaps(&f->duration,n);
	}

    bctrl = b->ctrl;
    if (mask & DvPercent)
	{
	t = f->percent;
	if (t == -1)
	    t = defaultKeyboardControl.bell;
	else if (t < 0 || t > 100)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	bctrl.percent = t;
	}

    if (mask & DvPitch)
	{
	t = f->pitch;
	if (t == -1)
	    t = defaultKeyboardControl.bell_pitch;
	else if (t < 0)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	bctrl.pitch = t;
	}

    if (mask & DvDuration)
	{
	t = f->duration;
	if (t == -1)
	    t = defaultKeyboardControl.bell_duration;
	else if (t < 0)
	    {
	    client->errorValue = t;
	    SendErrorToClient (client, IReqCode, X_ChangeFeedbackControl, 0, 
		BadValue);
	    return Success;
	    }
	bctrl.duration = t;
        }
    b->ctrl = bctrl;
    (*b->CtrlProc)(dev, &b->ctrl);
    return Success;
    }

/******************************************************************************
 *
 * This procedure changes LedFeedbackClass data.
 *
 */

ChangeLedFeedback (client, dev, mask, l, f)
    ClientPtr 		client;
    DeviceIntPtr 	dev;
    unsigned long 	mask;
    LedFeedbackPtr 	l;
    xLedFeedbackCtl 	*f;
    {
    register char n;
    LedCtrl lctrl;		/* might get BadValue part way through */

    if (client->swapped)
	{
	swaps(&f->length,n);
	swapl(&f->led_values,n);
	swapl(&f->led_mask,n);
	}

    f->led_mask &= l->ctrl.led_mask;	/* set only supported leds */
    f->led_values &= l->ctrl.led_mask;	/* set only supported leds */
    if (mask & DvLed)
        {
	lctrl.led_mask = f->led_mask;
	lctrl.led_values = f->led_values;
	(*l->CtrlProc)(dev, &lctrl);
	l->ctrl.led_values &= ~(f->led_mask);		/* zero changed leds */
	l->ctrl.led_values |= (f->led_mask & f->led_values);/* OR in set leds*/
        }

    return Success;
    }
