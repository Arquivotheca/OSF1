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
/* $XConsortium: resource.c,v 1.5 91/07/18 22:36:40 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this protoype software
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * %W%	%G%
 *
 */
/*
 *      a resource is a 32 bit quantity.  the upper 12 bits are client id.
 *      client provides a 19 bit resource id. this is "hashed" by me by
 *      taking the 10 lower bits and xor'ing with the mid 10 bits.
 *
 *      It is sometimes necessary for the server to create an ID that looks
 *      like it belongs to a client.  This ID, however,  must not be one
 *      the client actually can create, or we have the potential for conflict.
 *      The 20th bit of the ID is resevered for the server's use for this
 *      purpose.  By setting CLIENT_ID(id) to the client, the SERVER_BIT to
 *      1, and an otherwise unused ID in the low 19 bits, we can create a
 *      resource "owned" by the client.
 *
 *      The following IDs are currently reserved for siccing on the client:
 *      1 - allocated color to be freed when the client dies
 */

#include "FS.h"
#include "misc.h"
#include "os.h"
#include "resource.h"
#include "clientstr.h"
#include "globals.h"

static void rebuild_table();

#define INITBUCKETS 64
#define INITHASHSIZE 6
#define MAXHASHSIZE 11

typedef struct _Resource {
    struct _Resource *next;
    FSID        id;
    RESTYPE     type;
    pointer     value;
}           ResourceRec, *ResourcePtr;

#define NullResource ((ResourcePtr)NULL)

typedef struct _ClientResource {
    ResourcePtr *resources;
    int         elements;
    int         buckets;
    int         hashsize;	/* log(2)(buckets) */
    FSID        fakeID;
    FSID	endFakeID;
    FSID        expectID;
}           ClientResourceRec;

static RESTYPE lastResourceType;
static RESTYPE lastResourceClass;
static RESTYPE TypeMask;

typedef int (*DeleteType) ();

static DeleteType *DeleteFuncs = (DeleteType *) NULL;

#ifdef NOTYET
RESTYPE
CreateNewResourceType(deleteFunc)
    DeleteType  deleteFunc;
{
    RESTYPE     next = lastResourceType + 1;
    DeleteType *funcs;

    if (next & lastResourceClass)
	return 0;
    funcs = (DeleteType *) fsrealloc(DeleteFuncs,
				     (next + 1) * sizeof(DeleteType));
    if (!funcs)
	return 0;
    lastResourceType = next;
    DeleteFuncs = funcs;
    DeleteFuncs[next] = deleteFunc;
    return next;
}

RESTYPE
CreateNewResourceClass()
{
    RESTYPE     next = lastResourceClass >> 1;

    if (next & lastResourceType)
	return 0;
    lastResourceClass = next;
    TypeMask = next - 1;
    return next;
}

#endif				/* NOTYET */

ClientResourceRec clientTable[MAXCLIENTS];

/*****************
 * InitClientResources
 *    When a new client is created, call this to allocate space
 *    in resource table
 *****************/

int
NoneDeleteFunc ()
{
}

Bool
InitClientResources(client)
    ClientPtr   client;
{
    register int i,
                j;

    if (client == serverClient) {
	extern int  CloseClientFont();
	extern int  DeleteAuthCont ();

	lastResourceType = RT_LASTPREDEF;
	lastResourceClass = RC_LASTPREDEF;
	TypeMask = RC_LASTPREDEF - 1;
	if (DeleteFuncs)
	    fsfree(DeleteFuncs);
	DeleteFuncs = (DeleteType *) fsalloc((lastResourceType + 1) *
					     sizeof(DeleteType));
	if (!DeleteFuncs)
	    return FALSE;
	DeleteFuncs[RT_NONE & TypeMask] = NoneDeleteFunc;
	DeleteFuncs[RT_FONT & TypeMask] = CloseClientFont;
	DeleteFuncs[RT_AUTHCONT & TypeMask] = DeleteAuthCont;
    }
    clientTable[i = client->index].resources =
	(ResourcePtr *) fsalloc(INITBUCKETS * sizeof(ResourcePtr));
    if (!clientTable[i].resources)
	return FALSE;
    clientTable[i].buckets = INITBUCKETS;
    clientTable[i].elements = 0;
    clientTable[i].hashsize = INITHASHSIZE;
    clientTable[i].fakeID =  SERVER_BIT;
    clientTable[i].endFakeID = (clientTable[i].fakeID | RESOURCE_ID_MASK) + 1;
    for (j = 0; j < INITBUCKETS; j++) {
	clientTable[i].resources[j] = NullResource;
    }
    return TRUE;
}

static int
hash(client, id)
    int         client;
    register FSID id;
{
    id &= RESOURCE_ID_MASK;
    switch (clientTable[client].hashsize) {
    case 6:
	return ((int) (0x03F & (id ^ (id >> 6) ^ (id >> 12))));
    case 7:
	return ((int) (0x07F & (id ^ (id >> 7) ^ (id >> 13))));
    case 8:
	return ((int) (0x0FF & (id ^ (id >> 8) ^ (id >> 16))));
    case 9:
	return ((int) (0x1FF & (id ^ (id >> 9))));
    case 10:
	return ((int) (0x3FF & (id ^ (id >> 10))));
    case 11:
	return ((int) (0x7FF & (id ^ (id >> 11))));
    }
    return -1;
}


static Font
AvailableID(client, id, maxid, goodid)
    register int client;
    register FSID id, maxid, goodid;
{
    register ResourcePtr res;

    if ((goodid >= id) && (goodid <= maxid))
	return goodid;
    for (; id <= maxid; id++)
    {
	res = clientTable[client].resources[hash(client, id)];
	while (res && (res->id != id))
	    res = res->next;
	if (!res)
	    return id;
    }
    return 0;
}

/*
 * Return the next usable fake client ID.
 *
 * Normally this is just the next one in line, but if we've used the last
 * in the range, we need to find a new range of safe IDs to avoid
 * over-running another client.
 */

FSID
FakeClientID(client)
    int	client;
{
    register FSID id, maxid;
    register ResourcePtr *resp;
    register ResourcePtr res;
    register int i;
    FSID goodid;

    id = clientTable[client].fakeID++;
    if (id != clientTable[client].endFakeID)
	return id;
    id = ((Mask)client << CLIENTOFFSET) | SERVER_BIT;
    maxid = id | RESOURCE_ID_MASK;
    goodid = 0;
    for (resp = clientTable[client].resources, i = clientTable[client].buckets;
	 --i >= 0;)
    {
	for (res = *resp++; res; res = res->next)
	{
	    if ((res->id < id) || (res->id > maxid))
		continue;
	    if (((res->id - id) >= (maxid - res->id)) ?
		(goodid = AvailableID(client, id, res->id - 1, goodid)) :
		!(goodid = AvailableID(client, res->id + 1, maxid, goodid)))
		maxid = res->id - 1;
	    else
		id = res->id + 1;
	}
    }
    if (id > maxid) {
	if (!client)
	    FatalError("FakeClientID: server internal ids exhausted\n");
	MarkClientException(clients[client]);
	id = ((Mask)client << CLIENTOFFSET) | (SERVER_BIT * 3);
	maxid = id | RESOURCE_ID_MASK;
    }
    clientTable[client].fakeID = id + 1;
    clientTable[client].endFakeID = maxid + 1;
    return id;
}

Bool
AddResource(cid, id, type, value)
    int         cid;
    FSID        id;
    RESTYPE     type;
    pointer     value;
{
    register ClientResourceRec *rrec;
    register ResourcePtr res,
               *head;

    rrec = &clientTable[cid];
    if (!rrec->buckets) {
	ErrorF("AddResource(%x, %x, %x), client=%d \n",
	       id, type, value, cid);
	FatalError("client not in use\n");
    }
    if ((rrec->elements >= 4 * rrec->buckets) &&
	    (rrec->hashsize < MAXHASHSIZE))
	rebuild_table(cid);
    head = &rrec->resources[hash(cid, id)];
    res = (ResourcePtr) fsalloc(sizeof(ResourceRec));
    if (!res) {
	(*DeleteFuncs[type & TypeMask]) (value, id);
	return FALSE;
    }
    res->next = *head;
    res->id = id;
    res->type = type;
    res->value = value;
    *head = res;
    rrec->elements++;
    if (!(id & SERVER_BIT) && (id >= rrec->expectID))
	rrec->expectID = id + 1;
    return TRUE;
}

static void
rebuild_table(client)
    int         client;
{
    register int j;
    register ResourcePtr res,
                next;
    ResourcePtr **tails,
               *resources;
    register ResourcePtr **tptr,
               *rptr;

    /*
     * For now, preserve insertion order, since some ddx layers depend on
     * resources being free in the opposite order they are added.
     */

    j = 2 * clientTable[client].buckets;
    tails = (ResourcePtr **) ALLOCATE_LOCAL(j * sizeof(ResourcePtr *));
    if (!tails)
	return;
    resources = (ResourcePtr *) fsalloc(j * sizeof(ResourcePtr));
    if (!resources) {
	DEALLOCATE_LOCAL(tails);
	return;
    }
    for (rptr = resources, tptr = tails; --j >= 0; rptr++, tptr++) {
	*rptr = NullResource;
	*tptr = rptr;
    }
    clientTable[client].hashsize++;
    for (j = clientTable[client].buckets,
	    rptr = clientTable[client].resources;
	    --j >= 0;
	    rptr++) {
	for (res = *rptr; res; res = next) {
	    next = res->next;
	    res->next = NullResource;
	    tptr = &tails[hash(client, res->id)];
	    **tptr = res;
	    *tptr = &res->next;
	}
    }
    DEALLOCATE_LOCAL(tails);
    clientTable[client].buckets *= 2;
    fsfree(clientTable[client].resources);
    clientTable[client].resources = resources;
}

void
FreeResource(cid, id, skipDeleteFuncType)
    int         cid;
    FSID        id;
    RESTYPE     skipDeleteFuncType;
{
    register ResourcePtr res;
    register ResourcePtr *prev,
               *head;
    register int *eltptr;
    int         elements;
    Bool        gotOne = FALSE;

    if (clientTable[cid].buckets) {
	head = &clientTable[cid].resources[hash(cid, id)];
	eltptr = &clientTable[cid].elements;

	prev = head;
	while ((res = *prev) != (ResourcePtr) 0) {
	    if (res->id == id) {
		RESTYPE     rtype = res->type;

		*prev = res->next;
		elements = --*eltptr;
		if (rtype != skipDeleteFuncType)
		    (*DeleteFuncs[rtype & TypeMask]) (res->value, res->id);
		fsfree(res);
		if (*eltptr != elements)
		    prev = head;/* prev may no longer be valid */
		gotOne = TRUE;
	    } else
		prev = &res->next;
	}
    }
    if (!gotOne)
	FatalError("Freeing resource id=%X which isn't there", id);
}

#ifdef NOTYET
void
FreeResourceByType(cid, id, type, skipFree)
    int         cid;
    FSID        id;
    RESTYPE     type;
    Bool        skipFree;
{
    register ResourcePtr res;
    register ResourcePtr *prev,
               *head;

    if (clientTable[cid].buckets) {
	head = &clientTable[cid].resources[hash(cid, id)];

	prev = head;
	while (res = *prev) {
	    if (res->id == id && res->type == type) {
		*prev = res->next;
		if (!skipFree)
		    (*DeleteFuncs[type & TypeMask]) (res->value, res->id);
		fsfree(res);
		break;
	    } else
		prev = &res->next;
	}
    }
}

/*
 * Change the value associated with a resource id.  Caller
 * is responsible for "doing the right thing" with the old
 * data
 */

Bool
ChangeResourceValue(cid, id, rtype, value)
    int         cid;
    FSID        id;
    RESTYPE     rtype;
    pointer     value;
{
    register ResourcePtr res;

    if (clientTable[cid].buckets) {
	res = clientTable[cid].resources[hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type == rtype)) {
		res->value = value;
		return TRUE;
	    }
    }
    return FALSE;
}

#endif				/* NOTYET */

void
FreeClientResources(client)
    ClientPtr   client;
{
    register ResourcePtr *resources;
    register ResourcePtr this;
    int         j;

    /*
     * This routine shouldn't be called with a null client, but just in case
     * ...
     */

    if (!client)
	return;

    resources = clientTable[client->index].resources;
    for (j = 0; j < clientTable[client->index].buckets; j++) {
	/*
	 * It may seem silly to update the head of this resource list as we
	 * delete the members, since the entire list will be deleted any way,
	 * but there are some resource deletion functions "FreeClientPixels"
	 * for one which do a LookupID on another resource id (a Colormap id
	 * in this case), so the resource list must be kept valid up to the
	 * point that it is deleted, so every time we delete a resource, we
	 * must update the head, just like in free_resource. I hope that this
	 * doesn't slow down mass deletion appreciably. PRH
	 */

	ResourcePtr *head;

	head = &resources[j];

	for (this = *head; this; this = *head) {
	    RESTYPE     rtype = this->type;

	    *head = this->next;
	    (*DeleteFuncs[rtype & TypeMask]) (this->value, this->id);
	    fsfree(this);
	}
    }
    fsfree(clientTable[client->index].resources);
    clientTable[client->index].buckets = 0;
}

FreeAllResources()
{
    int         i;

    for (i = 0; i < currentMaxClients; i++) {
	if (clientTable[i].buckets)
	    FreeClientResources(clients[i]);
    }
}

/*
 *  lookup_id_by_type returns the object with the given id and type, else NULL.
 */
pointer
LookupIDByType(cid, id, rtype)
    int         cid;
    FSID        id;
    RESTYPE     rtype;
{
    register ResourcePtr res;

    if (clientTable[cid].buckets) {
	res = clientTable[cid].resources[hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type == rtype))
		return res->value;
    }
    return (pointer) NULL;
}

#ifdef NOTYET
/*
 *  lookup_ID_by_class returns the object with the given id and any one of the
 *  given classes, else NULL.
 */
pointer
LookupIDByClass(id, classes)
    FSID        id;
    RESTYPE     classes;
{
    int         cid;
    register ResourcePtr res;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets) {
	res = clientTable[cid].resources[hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type & classes))
		return res->value;
    }
    return (pointer) NULL;
}

#endif				/* NOTYET */
