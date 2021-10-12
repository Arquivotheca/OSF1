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

/* $XConsortium: xgetselev.c,v 1.9 90/05/18 15:35:21 rws Exp $ */

/***********************************************************************
 *
 * Extension function to get the current selected events for a given window.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "XI.h"
#include "XIproto.h"
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window struct     */

extern	int 		IReqCode;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure gets the current selected extension events.
 *
 */

int
SProcXGetSelectedExtensionEvents(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetSelectedExtensionEventsReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return(ProcXGetSelectedExtensionEvents(client));
    }

/***********************************************************************
 *
 * This procedure gets the current device select mask,
 * if the client and server have a different byte ordering.
 *
 */

int
ProcXGetSelectedExtensionEvents(client)
    register ClientPtr client;
    {
    int					i;
    int					total_length = 0;
    xGetSelectedExtensionEventsReply	rep;
    WindowPtr				pWin;
    XEventClass				*buf;
    XEventClass				*tclient;
    XEventClass				*aclient;
    XEventClass 			*ClassFromMask ();
    void				Swap32Write();
    OtherInputMasks			*pOthers;
    InputClientsPtr			others;

    REQUEST(xGetSelectedExtensionEventsReq);
    REQUEST_SIZE_MATCH(xGetSelectedExtensionEventsReq);

    rep.repType = X_Reply;
    rep.RepType = X_GetSelectedExtensionEvents;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.this_client_count = 0;
    rep.all_clients_count = 0;

    if (!(pWin = LookupWindow(stuff->window, client)))
        {
	SendErrorToClient(client, IReqCode, X_GetSelectedExtensionEvents, 0, 
		BadWindow);
	return Success;
        }

    if (pOthers=wOtherInputMasks(pWin))
	{
	for (others = pOthers->inputClients; others; others=others->next)
	    for (i=0; i<EMASKSIZE; i++)
		tclient = ClassFromMask (NULL, others->mask[i], i, 
		    &rep.all_clients_count, COUNT);

	for (others = pOthers->inputClients; others; others=others->next)
	    if (SameClient(others, client))
		{
		for (i=0; i<EMASKSIZE; i++)
		    tclient = ClassFromMask (NULL, others->mask[i], i, 
			&rep.this_client_count, COUNT);
		break;
		}

	total_length = (rep.all_clients_count + rep.this_client_count) * 
	    sizeof (XEventClass);
	rep.length = (total_length + 3) >> 2;
	buf = (XEventClass *) Xalloc (total_length);

	tclient = buf;
	aclient = buf + rep.this_client_count;
	if (others)
	    for (i=0; i<EMASKSIZE; i++)
		tclient = ClassFromMask (tclient, others->mask[i], i, NULL, CREATE);

	for (others = pOthers->inputClients; others; others=others->next)
	    for (i=0; i<EMASKSIZE; i++)
		aclient = ClassFromMask (aclient, others->mask[i], i, NULL, CREATE);
	}

    WriteReplyToClient (client, sizeof(xGetSelectedExtensionEventsReply), &rep);

    if (total_length)
	{
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient( client, total_length, buf);
	Xfree (buf);
	}
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetSelectedExtensionEvents function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetSelectedExtensionEvents (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetSelectedExtensionEventsReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->this_client_count, n);
    swaps(&rep->all_clients_count, n);
    WriteToClient(client, size, rep);
    }
