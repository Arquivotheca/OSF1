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
 * $XConsortium: compfuncs.c,v 2.17 89/10/06 15:02:26 converse Exp $
 *
 *
 * 		      COPYRIGHT 1987, 1989
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */

/* comp.c -- action procedures to handle composition buttons. */

#include "xmh.h"


/* Reset this composition widget to be one with just a blank message
   template. */

/*ARGSUSED*/
void DoResetCompose(widget, client_data, call_data)
    Widget	widget;		/* unused */
    XtPointer	client_data;
    XtPointer	call_data;	/* unused */
{
    Scrn	scrn = (Scrn) client_data;
    Msg		msg;
    XtCallbackRec	confirms[2];

    confirms[0].callback = (XtCallbackProc) DoResetCompose;
    confirms[0].closure = (XtPointer) scrn;
    confirms[1].callback = (XtCallbackProc) NULL;
    confirms[1].closure = (XtPointer) NULL;

    if (MsgSetScrn((Msg) NULL, scrn, confirms, (XtCallbackList) NULL) ==
	NEEDS_CONFIRMATION)
	return;

    msg = TocMakeNewMsg(DraftsFolder);
    MsgLoadComposition(msg);
    MsgSetTemporary(msg);
    MsgSetReapable(msg);
    (void) MsgSetScrn(msg, scrn, (XtCallbackList) NULL, (XtCallbackList) NULL);
}

/*ARGSUSED*/
void XmhResetCompose(w, event, params, num_params)
    Widget	w;
    XEvent	*event;
    String	*params;
    Cardinal	*num_params;
{
    Scrn scrn = ScrnFromWidget(w);
    DoResetCompose(w, (XtPointer) scrn, (XtPointer) NULL);
}


/* Send the message in this widget.  Avoid sending the same message twice.
   (Code elsewhere actually makes sure this button is disabled to avoid
   sending the same message twice, but it doesn't hurt to be safe here.) */

/*ARGSUSED*/
void XmhSend(w, event, params, num_params)
    Widget	w;
    XEvent	*event;
    String	*params;
    Cardinal	*num_params;
{
    Scrn scrn = ScrnFromWidget(w);
    if (scrn->msg == NULL) return;
    if (MsgChanged(scrn->msg) || ! MsgGetReapable(scrn->msg)) {
	MsgSend(scrn->msg);
	MsgSetReapable(scrn->msg);
    }
}


/* Save any changes to the message.  This also makes this message permanent. */

/*ARGSUSED*/
void XmhSave(w, event, params, num_params)
    Widget	w;
    XEvent	*event;
    String	*params;
    Cardinal	*num_params;
{
    Scrn scrn = ScrnFromWidget(w);
    DEBUG("XmhSave\n")
    if (scrn->msg == NULL) return;
    MsgSetPermanent(scrn->msg);
    if (MsgSaveChanges(scrn->msg))
	MsgClearReapable(scrn->msg);
}


/* Utility routine; creates a composition screen containing a forward message
   of the messages in the given msglist. */

CreateForward(mlist)
  MsgList mlist;
{
    Scrn scrn;
    Msg msg;
    scrn = NewCompScrn();
    msg = TocMakeNewMsg(DraftsFolder);
    MsgLoadForward(scrn, msg, mlist);
    MsgSetTemporary(msg);
    MsgSetScrnForComp(msg, scrn);
    MapScrn(scrn);
}
