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
/* $XConsortium: XGetProp.c,v 11.17 91/01/06 11:46:15 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

#define NEED_REPLIES
#include "Xlibint.h"

int
XGetWindowProperty(dpy, w, property, offset, length, delete, 
	req_type, actual_type, actual_format, nitems, bytesafter, prop)
    register Display *dpy;
    Window w;
    Atom property;
    Bool delete;
    Atom req_type;
    Atom *actual_type;		/* RETURN */
    int *actual_format;  	/* RETURN  8, 16, or 32 */
    long offset, length;
    unsigned long *nitems; 	/* RETURN  # of 8-, 16-, or 32-bit entities */
    unsigned long *bytesafter;	/* RETURN */
    unsigned char **prop;	/* RETURN */
    {
    xGetPropertyReply reply;
    register xGetPropertyReq *req;
    LockDisplay(dpy);
    GetReq (GetProperty, req);
    req->window = w;
    req->property = property;
    req->type = req_type;
    req->delete = delete;
    req->longOffset = offset;
    req->longLength = length;
    
    if (!_XReply (dpy, (xReply *) &reply, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (1);	/* not Success */
	}	

    *prop = (unsigned char *) NULL;
    if (reply.propertyType != None) {
	long nbytes, netbytes;
	switch (reply.format) {
      /* 
       * One extra byte is malloced than is needed to contain the property
       * data, but this last byte is null terminated and convenient for 
       * returning string properties, so the client doesn't then have to 
       * recopy the string to make it null terminated. 
       */
	  case 8:
	    nbytes = netbytes = reply.nItems;
	    if (*prop = (unsigned char *) Xmalloc ((unsigned)nbytes + 1))
		_XReadPad (dpy, (char *) *prop, netbytes);
	    break;

	  case 16:
	    nbytes = reply.nItems * sizeof (short);
	    netbytes = reply.nItems << 1;
	    if (*prop = (unsigned char *) Xmalloc ((unsigned)nbytes + 1))
		_XRead16Pad (dpy, (short *) *prop, netbytes);
	    break;

	  case 32:
	    nbytes = reply.nItems * sizeof (unsigned long);
	    netbytes = reply.nItems << 2;
	    if (*prop = (unsigned char *) Xmalloc ((unsigned)nbytes + 1)) {
		_XRead32 (dpy, (CARD32 *) *prop, netbytes);
#ifndef WORD64 /* 64 bit architectures other than cray */
#if ( defined (__osf__) && defined(__alpha) )
		{
		/* sizeof(CARD32) != sizeof(long) */
		/* per clarification of spec, we return unsigned longs
		   here & so must unpack the data into the right thing */
		unsigned long *to = (unsigned long *) *prop; 
		int i;
		for(i=reply.nItems-1;i>=0;i--)
		    to[i] = ((CARD32 *)(*prop))[i] & 0x80000000 ?
			    ((CARD32 *)(*prop))[i] | 0xffffffff00000000:
			    ((CARD32 *)(*prop))[i];
		}
#endif
#endif /* ! WORD64 */
	    }
	    break;

	  default:
	    /*
	     * This part of the code should never be reached.  If it is,
	     * the server sent back a property with an invalid format.
	     * This is a BadImplementation error. 
	     */
	    {
		xError error;
		error.sequenceNumber = dpy->request;
		error.type = X_Error;
		error.majorCode = X_GetProperty;
		error.minorCode = 0;
		error.errorCode = BadImplementation;
		_XError(dpy, &error);
	    }
	    netbytes = 0L;
	    break;
	}
	if (! *prop) {
	    _XEatData(dpy, (unsigned long) netbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return(BadAlloc);	/* not Success */
	}
	(*prop)[nbytes] = '\0';
    }
    *actual_type = reply.propertyType;
    *actual_format = reply.format;
    *nitems = reply.nItems;
    *bytesafter = reply.bytesAfter;
    UnlockDisplay(dpy);
    SyncHandle();
    return(Success);
}

