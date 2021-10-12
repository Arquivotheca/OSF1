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
/* $XConsortium: xselectev.c,v 1.9 90/05/18 15:35:37 rws Exp $ */

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

/***********************************************************************
 *
 * Request to select input from an extension device.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES

#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structure  */
#include "XI.h"
#include "XIproto.h"

extern	int 		IReqCode;
extern	Mask		ExtExclusiveMasks[];
extern	Mask		ExtValidMasks[];
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * Handle requests from clients with a different byte order.
 *
 */

int
SProcXSelectExtensionEvent (client)
register ClientPtr client;
    {
    register char n;
    register long *p;
    register int i;

    REQUEST(xSelectExtensionEventReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    swaps(&stuff->count, n);
    p = (long *) &stuff[1];
    for (i=0; i<stuff->count; i++)
        {
        swapl(p, n);
	p++;
        }
    return(ProcXSelectExtensionEvent(client));
    }

/***********************************************************************
 *
 * This procedure selects input from an extension device.
 *
 */

int
ProcXSelectExtensionEvent (client)
    register ClientPtr client;
    {
    int			ret;
    int			i;
    WindowPtr 		pWin;
    struct tmask	tmp[EMASKSIZE];

    REQUEST(xSelectExtensionEventReq);
    REQUEST_AT_LEAST_SIZE(xSelectExtensionEventReq);

    if (stuff->length !=(sizeof(xSelectExtensionEventReq)>>2) + stuff->count)
	{
	SendErrorToClient (client, IReqCode, X_SelectExtensionEvent, 0, 
		BadLength);
	return Success;
	}

    pWin = (WindowPtr) LookupWindow (stuff->window, client);
    if (!pWin)
        {
	client->errorValue = stuff->window;
	SendErrorToClient(client, IReqCode, X_SelectExtensionEvent, 0, 
		BadWindow);
	return Success;
        }

    if ((ret = CreateMaskFromList (client, (XEventClass *)&stuff[1], 
	stuff->count, tmp, NULL, X_SelectExtensionEvent)) != Success)
	return Success;

    for (i=0; i<EMASKSIZE; i++)
	if (tmp[i].dev != NULL)
	    {
	    if ((ret = SelectForWindow(tmp[i].dev, pWin, client, tmp[i].mask, 
		ExtExclusiveMasks[i], ExtValidMasks[i])) != Success)
		{
		SendErrorToClient(client, IReqCode, X_SelectExtensionEvent, 0, 
			ret);
		return Success;
		}
	    }

    return Success;
    }
