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
/* $XConsortium: XGetBMap.c,v 1.4 89/12/06 20:38:18 rws Exp $ */

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
 * XGetDeviceButtonMapping - Get the button mapping of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#ifdef MIN			/* some systems define this in <sys/param.h> */
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int
XGetDeviceButtonMapping (dpy, device, map, nmap)
    register 	Display	*dpy;
    XDevice		*device;
    unsigned 	char 	map[];
    unsigned 	int 	nmap; 
    {
    int	status = 0;
    unsigned char mapping[256];				/* known fixed size */
    long nbytes;
    XExtDisplayInfo *info = (XExtDisplayInfo *) XInput_find_display (dpy);

    register xGetDeviceButtonMappingReq *req;
    xGetDeviceButtonMappingReply rep;

    LockDisplay(dpy);
    if (CheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq(GetDeviceButtonMapping, req);

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceButtonMapping;
    req->deviceid = device->device_id;

    status = _XReply (dpy, (xReply *)&rep, 0, xFalse);
    if (status == 1)
	{
	nbytes = (long)rep.length << 2;
	_XRead (dpy, (char *)mapping, nbytes);

	/* don't return more data than the user asked for. */
	if (rep.nElts) 
	    bcopy ((char *) mapping, (char *) map, MIN ((int)rep.nElts, nmap) );
	status = rep.nElts;
	}
    else
	status = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
    }
