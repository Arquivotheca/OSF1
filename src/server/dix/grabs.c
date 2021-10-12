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
/* $XConsortium: grabs.c,v 5.7 89/12/11 15:43:47 keith Exp $ */
/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  
DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN action OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

#include "X.h"
#include "misc.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "windowstr.h"
#include "inputstr.h"
#include "cursorstr.h"

extern InputInfo inputInfo;

#define BITMASK(i) (((Mask)1) << ((i) & 31))
#define MASKIDX(i) ((i) >> 5)
#define MASKWORD(buf, i) buf[MASKIDX(i)]
#define BITSET(buf, i) MASKWORD(buf, i) |= BITMASK(i)
#define BITCLEAR(buf, i) MASKWORD(buf, i) &= ~BITMASK(i)
#define GETBIT(buf, i) (MASKWORD(buf, i) & BITMASK(i))

GrabPtr
CreateGrab(client, device, window, eventMask, ownerEvents, keyboardMode,
	   pointerMode, modDevice, modifiers, type, keybut, confineTo, cursor)
    int client;
    DeviceIntPtr device;
    WindowPtr window;
    Mask eventMask;
    Bool ownerEvents, keyboardMode, pointerMode;
    DeviceIntPtr modDevice;
    unsigned short modifiers;
    int type;
    KeyCode keybut;	/* key or button */
    WindowPtr confineTo;
    CursorPtr cursor;
{
    GrabPtr grab;

    grab = (GrabPtr)xalloc(sizeof(GrabRec));
    if (!grab)
	return (GrabPtr)NULL;
    grab->resource = FakeClientID(client);
    grab->device = device;
    grab->coreGrab = ((device == inputInfo.keyboard) ||
		      (device == inputInfo.pointer));
    grab->window = window;
    grab->eventMask = eventMask;
    grab->ownerEvents = ownerEvents;
    grab->keyboardMode = keyboardMode;
    grab->pointerMode = pointerMode;
    grab->modifiersDetail.exact = modifiers;
    grab->modifiersDetail.pMask = NULL;
    grab->modifierDevice = modDevice;
    grab->coreMods = ((modDevice == inputInfo.keyboard) ||
		      (modDevice == inputInfo.pointer));
    grab->type = type;
    grab->detail.exact = keybut;
    grab->detail.pMask = NULL;
    grab->confineTo = confineTo;
    grab->cursor = cursor;
    if (cursor)
	cursor->refcnt++;
    return grab;

}

static void
FreeGrab(pGrab)
    GrabPtr pGrab;
{
    if (pGrab->modifiersDetail.pMask != NULL)
	xfree(pGrab->modifiersDetail.pMask);

    if (pGrab->detail.pMask != NULL)
	xfree(pGrab->detail.pMask);

    if (pGrab->cursor)
	FreeCursor(pGrab->cursor, (Cursor)0);

    xfree(pGrab);
}

/*ARGSUSED*/
int
DeletePassiveGrab(pGrab, id)
    GrabPtr pGrab;
    XID   id;
{
    register GrabPtr g, prev;

    /* it is OK if the grab isn't found */
    prev = 0;
    for (g = (wPassiveGrabs (pGrab->window)); g; g = g->next)
    {
	if (pGrab == g)
	{
	    if (prev)
		prev->next = g->next;
	    else
		if (!(pGrab->window->optional->passiveGrabs = g->next))
		    CheckWindowOptionalNeed (pGrab->window);
	    break;
	}
	prev = g;
    }
    FreeGrab(pGrab);
}

static Mask *
DeleteDetailFromMask(pDetailMask, detail)
    Mask *pDetailMask;
    unsigned short detail;
{
    register Mask *mask;
    register int i;

    mask = (Mask *)xalloc(sizeof(Mask) * MasksPerDetailMask);
    if (mask)
    {
	if (pDetailMask)
	    for (i = 0; i < MasksPerDetailMask; i++)
		mask[i]= pDetailMask[i];
	else
	    for (i = 0; i < MasksPerDetailMask; i++)
		mask[i]= ~0L;
	BITCLEAR(mask, detail);
    }
    return mask; 
}

static Bool
IsInGrabMask(firstDetail, secondDetail, exception)
    DetailRec firstDetail, secondDetail;
    unsigned short exception;
{
    if (firstDetail.exact == exception)
    {
	if (firstDetail.pMask == NULL)
	    return TRUE;
	
	/* (at present) never called with two non-null pMasks */
	if (secondDetail.exact == exception)
	    return FALSE;

 	if (GETBIT(firstDetail.pMask, secondDetail.exact))
	    return TRUE;
    }
    
    return FALSE;
}

static Bool 
IdenticalExactDetails(firstExact, secondExact, exception)
    unsigned short firstExact, secondExact, exception;
{
    if ((firstExact == exception) || (secondExact == exception))
	return FALSE;
   
    if (firstExact == secondExact)
	return TRUE;

    return FALSE;
}

static Bool 
DetailSupersedesSecond(firstDetail, secondDetail, exception)
    DetailRec firstDetail, secondDetail;
    unsigned short exception;
{
    if (IsInGrabMask(firstDetail, secondDetail, exception))
	return TRUE;

    if (IdenticalExactDetails(firstDetail.exact, secondDetail.exact,
			      exception))
	return TRUE;
  
    return FALSE;
}

static Bool
GrabSupersedesSecond(pFirstGrab, pSecondGrab)
    GrabPtr pFirstGrab, pSecondGrab;
{
    if (!DetailSupersedesSecond(pFirstGrab->modifiersDetail,
				pSecondGrab->modifiersDetail, 
				(unsigned short)AnyModifier))
	return FALSE;

    if (DetailSupersedesSecond(pFirstGrab->detail,
			       pSecondGrab->detail, (unsigned short)AnyKey))
	return TRUE;
 
    return FALSE;
}

Bool
GrabMatchesSecond(pFirstGrab, pSecondGrab)
    GrabPtr pFirstGrab, pSecondGrab;
{
    if ((pFirstGrab->device != pSecondGrab->device) ||
	(pFirstGrab->modifierDevice != pSecondGrab->modifierDevice) ||
	(pFirstGrab->type != pSecondGrab->type))
	return FALSE;

    if (GrabSupersedesSecond(pFirstGrab, pSecondGrab) ||
	GrabSupersedesSecond(pSecondGrab, pFirstGrab))
	return TRUE;
 
    if (DetailSupersedesSecond(pSecondGrab->detail, pFirstGrab->detail,
			       (unsigned short)AnyKey) 
	&& 
	DetailSupersedesSecond(pFirstGrab->modifiersDetail,
			       pSecondGrab->modifiersDetail,
			       (unsigned short)AnyModifier))
	return TRUE;

    if (DetailSupersedesSecond(pFirstGrab->detail, pSecondGrab->detail,
			       (unsigned short)AnyKey)
	&& 
	DetailSupersedesSecond(pSecondGrab->modifiersDetail,
			       pFirstGrab->modifiersDetail,
			       (unsigned short)AnyModifier))
	return TRUE;

    return FALSE;
}

int
AddPassiveGrabToList(pGrab)
    GrabPtr pGrab;
{
    GrabPtr grab;

    for (grab = wPassiveGrabs(pGrab->window); grab; grab = grab->next)
    {
	if (GrabMatchesSecond(pGrab, grab))
	{
	    if (CLIENT_BITS(pGrab->resource) != CLIENT_BITS(grab->resource))
	    {
		FreeGrab(pGrab);
		return BadAccess;
	    }
	}
    }

    if (!pGrab->window->optional && !MakeWindowOptional (pGrab->window))
    {
	FreeGrab(pGrab);
	return BadAlloc;
    }
    pGrab->next = pGrab->window->optional->passiveGrabs;
    pGrab->window->optional->passiveGrabs = pGrab;
    if (AddResource(pGrab->resource, RT_PASSIVEGRAB, (pointer)pGrab))
	return Success;
    return BadAlloc;
}

/* the following is kinda complicated, because we need to be able to back out
 * if any allocation fails
 */

Bool
DeletePassiveGrabFromList(pMinuendGrab)
    GrabPtr pMinuendGrab;
{
    register GrabPtr grab;
    GrabPtr *deletes, *adds;
    Mask ***updates, **details;
    int i, ndels, nadds, nups;
    Bool ok;

#define UPDATE(mask,exact) \
	if (!(details[nups] = DeleteDetailFromMask(mask, exact))) \
	  ok = FALSE; \
	else \
	  updates[nups++] = &(mask)

    i = 0;
    for (grab = wPassiveGrabs(pMinuendGrab->window); grab; grab = grab->next)
	i++;
    if (!i)
	return TRUE;
    deletes = (GrabPtr *)ALLOCATE_LOCAL(i * sizeof(GrabPtr));
    adds = (GrabPtr *)ALLOCATE_LOCAL(i * sizeof(GrabPtr));
    updates = (Mask ***)ALLOCATE_LOCAL(i * sizeof(Mask **));
    details = (Mask **)ALLOCATE_LOCAL(i * sizeof(Mask *));
    if (!deletes || !adds || !updates || !details)
    {
	if (details) DEALLOCATE_LOCAL(details);
	if (updates) DEALLOCATE_LOCAL(updates);
	if (adds) DEALLOCATE_LOCAL(adds);
	if (deletes) DEALLOCATE_LOCAL(deletes);
	return FALSE;
    }
    ndels = nadds = nups = 0;
    ok = TRUE;
    for (grab = wPassiveGrabs(pMinuendGrab->window);
	 grab && ok;
	 grab = grab->next)
    {
	if ((CLIENT_BITS(grab->resource) != CLIENT_BITS(pMinuendGrab->resource)) ||
	    !GrabMatchesSecond(grab, pMinuendGrab))
	    continue;
	if (GrabSupersedesSecond(pMinuendGrab, grab))
	{
	    deletes[ndels++] = grab;
	}
	else if ((grab->detail.exact == AnyKey)
		 && (grab->modifiersDetail.exact != AnyModifier))
	{
	    UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);
	}
	else if ((grab->modifiersDetail.exact == AnyModifier) 
		 && (grab->detail.exact != AnyKey))
	{
	    UPDATE(grab->modifiersDetail.pMask,
		   pMinuendGrab->modifiersDetail.exact);
	}
	else if ((pMinuendGrab->detail.exact != AnyKey)
		 && (pMinuendGrab->modifiersDetail.exact != AnyModifier))
	{
	    GrabPtr pNewGrab;

	    UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);

	    pNewGrab = CreateGrab(CLIENT_ID(grab->resource), grab->device,
				  grab->window, (Mask)grab->eventMask,
				  (Bool)grab->ownerEvents,
				  (Bool)grab->keyboardMode,
				  (Bool)grab->pointerMode,
				  grab->modifierDevice,
				  AnyModifier, (int)grab->type,
				  pMinuendGrab->detail.exact,
				  grab->confineTo, grab->cursor);
	    if (!pNewGrab)
		ok = FALSE;
	    else if (!(pNewGrab->modifiersDetail.pMask =
		       DeleteDetailFromMask(grab->modifiersDetail.pMask,
					 pMinuendGrab->modifiersDetail.exact))
		     ||
		     (!pNewGrab->window->optional &&
		      !MakeWindowOptional(pNewGrab->window)))
	    {
		FreeGrab(pNewGrab);
		ok = FALSE;
	    }
	    else if (!AddResource(pNewGrab->resource, RT_PASSIVEGRAB,
				  (pointer)pNewGrab))
		ok = FALSE;
	    else
		adds[nadds++] = pNewGrab;
	}   
	else if (pMinuendGrab->detail.exact == AnyKey)
	{
	    UPDATE(grab->modifiersDetail.pMask,
		   pMinuendGrab->modifiersDetail.exact);
	}
	else
	{
	    UPDATE(grab->detail.pMask, pMinuendGrab->detail.exact);
	}
    }

    if (!ok)
    {
	for (i = 0; i < nadds; i++)
	    FreeResource(adds[i]->resource, RT_NONE);
	for (i = 0; i < nups; i++)
	    xfree(details[i]);
    }
    else
    {
	for (i = 0; i < ndels; i++)
	    FreeResource(deletes[i]->resource, RT_NONE);
	for (i = 0; i < nadds; i++)
	{
	    grab = adds[i];
	    grab->next = grab->window->optional->passiveGrabs;
	    grab->window->optional->passiveGrabs = grab;
	}
	for (i = 0; i < nups; i++)
	{
	    xfree(*updates[i]);
	    *updates[i] = details[i];
	}
    }
    DEALLOCATE_LOCAL(details);
    DEALLOCATE_LOCAL(updates);
    DEALLOCATE_LOCAL(adds);
    DEALLOCATE_LOCAL(deletes);
    return ok;

#undef UPDATE
}
