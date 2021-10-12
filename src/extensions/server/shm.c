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
Copyright 1989 by The Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
no- tice appear in all copies and that both that copyright
no- tice and this permission notice appear in supporting
docu- mentation, and that the name of MIT not be used in
advertising or publicity pertaining to distribution of the
software without specific prior written permission.
M.I.T. makes no representation about the suitability of
this software for any purpose. It is provided "as is"
without any express or implied warranty.

********************************************************/

/* THIS IS NOT AN X CONSORTIUM STANDARD */

/* $XConsortium: shm.c,v 1.15 92/05/10 17:26:46 rws Exp $ */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "extnsionst.h"
#include "servermd.h"
#define _XSHM_SERVER_
#include "shmstr.h"
#include "Xfuncproto.h"

typedef struct _ShmDesc {
    struct _ShmDesc *next;
    int shmid;
    int refcnt;
    char *addr;
    Bool writable;
    unsigned long size;
} ShmDescRec, *ShmDescPtr;

#if NeedFunctionPrototypes

#if !defined(__osf__)
   /* defined in shm.h */
#if defined(SVR4)
void *shmat(int, void*, int);
#else
#if !defined(sgi) && !defined(hpux)
char *shmat(int, char*, int);
#endif
#endif

#endif
#endif

static void miShmPutImage(), fbShmPutImage();
static PixmapPtr fbShmCreatePixmap();
ExtensionEntry *AddExtension();
static int ProcShmDispatch(), SProcShmDispatch();
static int ShmDetachSegment();
static void ShmResetProc(), SShmCompletionEvent();

static unsigned char ShmReqCode;
static int ShmCompletionCode;
static int BadShmSegCode;
static RESTYPE ShmSegType, ShmPixType;
static ShmDescPtr Shmsegs;
static Bool sharedPixmaps;
static int pixmapFormat;
static int shmPixFormat[MAXSCREENS];
static ShmFuncsPtr shmFuncs[MAXSCREENS];
static ShmFuncs miFuncs = {NULL, miShmPutImage};
static ShmFuncs fbFuncs = {fbShmCreatePixmap, fbShmPutImage};

#define VERIFY_SHMSEG(shmseg,shmdesc,client) \
{ \
    shmdesc = (ShmDescPtr)LookupIDByType(shmseg, ShmSegType); \
    if (!shmdesc) \
    { \
	client->errorValue = shmseg; \
	return BadShmSegCode; \
    } \
}

#define VERIFY_SHMPTR(shmseg,offset,needwrite,shmdesc,client) \
{ \
    VERIFY_SHMSEG(shmseg, shmdesc, client); \
    if ((offset & 3) || (offset > shmdesc->size)) \
    { \
	client->errorValue = offset; \
	return BadValue; \
    } \
    if (needwrite && !shmdesc->writable) \
	return BadAccess; \
}

#define VERIFY_SHMSIZE(shmdesc,offset,len,client) \
{ \
    if ((offset + len) > shmdesc->size) \
    { \
	return BadAccess; \
    } \
}

void
ShmExtensionInit()
{
    ExtensionEntry *extEntry;
    int i;

    sharedPixmaps = xTrue;
    pixmapFormat = shmPixFormat[0];
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	if (!shmFuncs[i])
	    shmFuncs[i] = &miFuncs;
	if (!shmFuncs[i]->CreatePixmap)
	    sharedPixmaps = xFalse;
	if (shmPixFormat[i] && (shmPixFormat[i] != pixmapFormat))
	{
	    sharedPixmaps = xFalse;
	    pixmapFormat = 0;
	}
    }
    if (!pixmapFormat)
	pixmapFormat = ZPixmap;
    ShmSegType = CreateNewResourceType(ShmDetachSegment);
    ShmPixType = CreateNewResourceType(ShmDetachSegment);
    if (ShmSegType && ShmPixType &&
	(extEntry = AddExtension(SHMNAME, ShmNumberEvents, ShmNumberErrors,
				 ProcShmDispatch, SProcShmDispatch,
				 ShmResetProc, StandardMinorOpcode)))
    {
	ShmReqCode = (unsigned char)extEntry->base;
	ShmCompletionCode = extEntry->eventBase;
	BadShmSegCode = extEntry->errorBase;
	EventSwapVector[ShmCompletionCode] = SShmCompletionEvent;
    }
}

/*ARGSUSED*/
static void
ShmResetProc (extEntry)
ExtensionEntry	*extEntry;
{
    int i;

    for (i = 0; i < MAXSCREENS; i++)
    {
	shmFuncs[i] = (ShmFuncsPtr)NULL;
	shmPixFormat[i] = 0;
    }
}

void
ShmRegisterFuncs(pScreen, funcs)
    ScreenPtr pScreen;
    ShmFuncsPtr funcs;
{
    shmFuncs[pScreen->myNum] = funcs;
}

void
ShmSetPixmapFormat(pScreen, format)
    ScreenPtr pScreen;
    int format;
{
    shmPixFormat[pScreen->myNum] = format;
}

void
ShmRegisterFbFuncs(pScreen)
    ScreenPtr pScreen;
{
    shmFuncs[pScreen->myNum] = &fbFuncs;
}

static int
ProcShmQueryVersion(client)
    register ClientPtr client;
{
    REQUEST(xShmQueryVersionReq);
    xShmQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xShmQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.sharedPixmaps = sharedPixmaps;
    rep.majorVersion = SHM_MAJOR_VERSION;
    rep.minorVersion = SHM_MINOR_VERSION;
    rep.uid = geteuid();
    rep.gid = getegid();
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swaps(&rep.majorVersion, n);
	swaps(&rep.minorVersion, n);
	swaps(&rep.uid, n);
	swaps(&rep.gid, n);
    }
    WriteToClient(client, sizeof(xShmQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcShmAttach(client)
    register ClientPtr client;
{
    struct shmid_ds buf;
    ShmDescPtr shmdesc;
    REQUEST(xShmAttachReq);

    REQUEST_SIZE_MATCH(xShmAttachReq);
    LEGAL_NEW_RESOURCE(stuff->shmseg, client);
    if ((stuff->readOnly != xTrue) && (stuff->readOnly != xFalse))
    {
	client->errorValue = stuff->readOnly;
        return(BadValue);
    }
    for (shmdesc = Shmsegs;
	 shmdesc && (shmdesc->shmid != stuff->shmid);
	 shmdesc = shmdesc->next)
	;
    if (shmdesc)
    {
	if (!stuff->readOnly && !shmdesc->writable)
	    return BadAccess;
	shmdesc->refcnt++;
    }
    else
    {
	shmdesc = (ShmDescPtr) xalloc(sizeof(ShmDescRec));
	if (!shmdesc)
	    return BadAlloc;
	shmdesc->addr = shmat(stuff->shmid, 0,
			      stuff->readOnly ? SHM_RDONLY : 0);
	if ((shmdesc->addr == ((char *)-1)) ||
	    shmctl(stuff->shmid, IPC_STAT, &buf))
	{
	    xfree(shmdesc);
	    return BadAccess;
	}
	shmdesc->shmid = stuff->shmid;
	shmdesc->refcnt = 1;
	shmdesc->writable = !stuff->readOnly;
	shmdesc->size = buf.shm_segsz;
	shmdesc->next = Shmsegs;
	Shmsegs = shmdesc;
    }
    if (!AddResource((XID)stuff->shmseg, ShmSegType, (pointer)shmdesc))
	return BadAlloc;
    return(client->noClientException);
}

/*ARGSUSED*/
static int
ShmDetachSegment(shmdesc, shmseg)
    ShmDescPtr shmdesc;
    ShmSeg shmseg;
{
    ShmDescPtr *prev;

    if (--shmdesc->refcnt)
	return TRUE;
    shmdt(shmdesc->addr);
    for (prev = &Shmsegs; *prev != shmdesc; prev = &(*prev)->next)
	;
    *prev = shmdesc->next;
    xfree(shmdesc);
}

static int
ProcShmDetach(client)
    register ClientPtr client;
{
    ShmDescPtr shmdesc;
    REQUEST(xShmDetachReq);

    REQUEST_SIZE_MATCH(xShmDetachReq);
    VERIFY_SHMSEG(stuff->shmseg, shmdesc, client);
    FreeResource(stuff->shmseg, RT_NONE);
    return(client->noClientException);
}

static void
miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    PixmapPtr pmap;
    GCPtr putGC;

    putGC = GetScratchGC(depth, dst->pScreen);
    if (!putGC)
	return;
    pmap = (*dst->pScreen->CreatePixmap)(dst->pScreen, sw, sh, depth);
    if (!pmap)
    {
	FreeScratchGC(putGC);
	return;
    }
    ValidateGC((DrawablePtr)pmap, putGC);
    (*putGC->ops->PutImage)(pmap, putGC, depth, -sx, -sy, w, h, 0,
			    (format == XYPixmap) ? XYPixmap : ZPixmap, data);
    FreeScratchGC(putGC);
    if (format == XYBitmap)
	(void)(*pGC->ops->CopyPlane)(pmap, dst, pGC, 0, 0, sw, sh, dx, dy, 1L);
    else
	(void)(*pGC->ops->CopyArea)(pmap, dst, pGC, 0, 0, sw, sh, dx, dy);
    (*pmap->drawable.pScreen->DestroyPixmap)(pmap);
}

static void
fbShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    if ((format == ZPixmap) || (depth == 1))
    {
	PixmapRec FakePixmap;
    	FakePixmap.drawable.type = DRAWABLE_PIXMAP;
    	FakePixmap.drawable.class = 0;
    	FakePixmap.drawable.pScreen = dst->pScreen;
    	FakePixmap.drawable.depth = depth;
    	FakePixmap.drawable.bitsPerPixel = depth;
    	FakePixmap.drawable.id = 0;
    	FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    	FakePixmap.drawable.x = 0;
    	FakePixmap.drawable.y = 0;
    	FakePixmap.drawable.width = w;
    	FakePixmap.drawable.height = h;
    	FakePixmap.devKind = PixmapBytePad(w, depth);
    	FakePixmap.refcnt = 1;
    	FakePixmap.devPrivate.ptr = (pointer)data;
	if (format == XYBitmap)
	    (void)(*pGC->ops->CopyPlane)(&FakePixmap, dst, pGC,
					 sx, sy, sw, sh, dx, dy, 1L);
	else
	    (void)(*pGC->ops->CopyArea)(&FakePixmap, dst, pGC,
					sx, sy, sw, sh, dx, dy);
    }
    else
	miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy,
		      data);
}

static int
ProcShmPutImage(client)
    register ClientPtr client;
{
    register GCPtr pGC;
    register DrawablePtr pDraw;
    long length;	/* length of scanline server padded */
    long lengthProto; 	/* length of scanline protocl padded */
    char * tmpImage;
    ShmDescPtr shmdesc;
    REQUEST(xShmPutImageReq);
    int tmpAlloced = 0;

    REQUEST_SIZE_MATCH(xShmPutImageReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, FALSE, shmdesc, client);
    if ((stuff->sendEvent != xTrue) && (stuff->sendEvent != xFalse))
	return BadValue;
    if (stuff->format == XYBitmap)
    {
        if (stuff->depth != 1)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
        lengthProto = PixmapBytePadProto(stuff->totalWidth, 1);
    }
    else if (stuff->format == XYPixmap)
    {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
	length *= stuff->depth;
        lengthProto = PixmapBytePadProto(stuff->totalWidth, 1);
	lengthProto *= stuff->depth;
    }
    else if (stuff->format == ZPixmap)
    {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, stuff->depth);
        lengthProto = PixmapBytePadProto(stuff->totalWidth, stuff->depth);
    }
    else
    {
	client->errorValue = stuff->format;
        return BadValue;
    }

    /* handle 64 bit case where protocol may pad to 32 and we want 64 */
    if ( length != lengthProto ) {
	register int 	i;
	char 		* stuffptr, /* pointer into protocol data */
			* tmpptr;   /* new location to copy to */

        if(!(tmpImage = (char *) ALLOCATE_LOCAL(length*stuff->totalHeight)))
            return (BadAlloc);
	tmpAlloced = 1;
    
	bzero(tmpImage,length*stuff->totalHeight);
    
	if ( stuff->format == XYPixmap ) {
	    int lineBytes =  PixmapBytePad(stuff->totalWidth, 1);
	    int lineBytesProto = PixmapBytePadProto(stuff->totalWidth, 1);
	    int depth = stuff->depth;

	    stuffptr = shmdesc->addr + stuff->offset ;
	    tmpptr = tmpImage;
	    for ( i = 0; i < stuff->totalHeight*stuff->depth;
	        stuffptr += lineBytesProto,tmpptr += lineBytes, i++) 
	        bcopy(stuffptr,tmpptr,lineBytesProto);
	}
	else {
	    for ( i = 0,
		  stuffptr = shmdesc->addr + stuff->offset,
		  tmpptr=tmpImage;
		i < stuff->totalHeight;
	        stuffptr += lengthProto,tmpptr += length, i++) 
	        bcopy(stuffptr,tmpptr,lengthProto);
	}
    }
    /* handle 64-bit case where stuff is not 64-bit aligned */
    else if ((unsigned long)(shmdesc->addr+stuff->offset) & (sizeof(long)-1)) {
        if(!(tmpImage = (char *) ALLOCATE_LOCAL(length*stuff->totalHeight)))
            return (BadAlloc);
	tmpAlloced = 1;
	bcopy((char *)(shmdesc->addr+stuff->offset),
	      tmpImage,
	      length*stuff->totalHeight);
    }
    else
	tmpImage = (char *)(shmdesc->addr+stuff->offset);
	
    if ((((stuff->format == ZPixmap) && (stuff->srcX == 0)) ||
	 ((stuff->format != ZPixmap) &&
	  (stuff->srcX < screenInfo.bitmapScanlinePad) &&
	  ((stuff->format == XYBitmap) ||
	   ((stuff->srcY == 0) &&
	    (stuff->srcHeight == stuff->totalHeight))))) &&
	((stuff->srcX + stuff->srcWidth) == stuff->totalWidth))
	(*pGC->ops->PutImage) (pDraw, pGC, stuff->depth,
			       stuff->dstX, stuff->dstY,
			       stuff->totalWidth, stuff->srcHeight, 
			       stuff->srcX, stuff->format, 
			       tmpImage + (stuff->srcY * length));
    else
	(*shmFuncs[pDraw->pScreen->myNum]->PutImage)(
			       pDraw, pGC, stuff->depth, stuff->format,
			       stuff->totalWidth, stuff->totalHeight,
			       stuff->srcX, stuff->srcY,
			       stuff->srcWidth, stuff->srcHeight,
			       stuff->dstX, stuff->dstY,
			       tmpImage);

    if (stuff->sendEvent)
    {
	xShmCompletionEvent ev;

	ev.type = ShmCompletionCode;
	ev.drawable = stuff->drawable;
	ev.sequenceNumber = client->sequence;
	ev.minorEvent = X_ShmPutImage;
	ev.majorEvent = ShmReqCode;
	ev.shmseg = stuff->shmseg;
	ev.offset = stuff->offset;
	WriteEventsToClient(client, 1, (xEvent *) &ev);
    }
     if ( tmpAlloced )
        DEALLOCATE_LOCAL(tmpImage);
     return (client->noClientException);
}

static int
ProcShmGetImage(client)
    register ClientPtr client;
{
    register DrawablePtr pDraw;
    long		widthBytesLine, length;
    long		widthBytesLineProto, lengthProto;
    Mask		plane;
    xShmGetImageReply	xgi;
    ShmDescPtr		shmdesc;
    int			n;
    char 		* tmpImage;
    int			tmpAlloced = 0;
    REQUEST(xShmGetImageReq);

    REQUEST_SIZE_MATCH(xShmGetImageReq);
    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap))
    {
	client->errorValue = stuff->format;
        return(BadValue);
    }
    VERIFY_DRAWABLE(pDraw, stuff->drawable, client);
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);
    if (pDraw->type == DRAWABLE_WINDOW)
    {
      if( /* check for being viewable */
	 !((WindowPtr) pDraw)->realized ||
	  /* check for being on screen */
         pDraw->x + stuff->x < 0 ||
 	 pDraw->x + stuff->x + (int)stuff->width > pDraw->pScreen->width ||
         pDraw->y + stuff->y < 0 ||
         pDraw->y + stuff->y + (int)stuff->height > pDraw->pScreen->height ||
          /* check for being inside of border */
         stuff->x < - wBorderWidth((WindowPtr)pDraw) ||
         stuff->x + (int)stuff->width >
		wBorderWidth((WindowPtr)pDraw) + (int)pDraw->width ||
         stuff->y < -wBorderWidth((WindowPtr)pDraw) ||
         stuff->y + (int)stuff->height >
		wBorderWidth((WindowPtr)pDraw) + (int)pDraw->height
        )
	    return(BadMatch);
	xgi.visual = wVisual(((WindowPtr)pDraw));
    }
    else
    {
	if (stuff->x < 0 ||
	    stuff->x+(int)stuff->width > pDraw->width ||
	    stuff->y < 0 ||
	    stuff->y+(int)stuff->height > pDraw->height
	    )
	    return(BadMatch);
	xgi.visual = None;
    }
    xgi.type = X_Reply;
    xgi.length = 0;
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if(stuff->format == ZPixmap)
    {
        widthBytesLine = PixmapBytePad(stuff->width, pDraw->depth);
	length = widthBytesLine * stuff->height;

	widthBytesLineProto = PixmapBytePadProto(stuff->width, pDraw->depth);
	lengthProto = widthBytesLineProto * stuff->height;
    }
    else 
    {
	widthBytesLine = PixmapBytePad(stuff->width, 1);
	plane = ((Mask)1) << (pDraw->depth - 1);
	/* only planes asked for */
	length = widthBytesLine * stuff->height *
	  Ones(stuff->planeMask & (plane | (plane - 1)));
	
	widthBytesLineProto = PixmapBytePadProto(stuff->width, 1);
	lengthProto = widthBytesLineProto * stuff->height *
	  Ones(stuff->planeMask & (plane | (plane - 1)));
    }

    VERIFY_SHMSIZE(shmdesc, stuff->offset, lengthProto, client);
    xgi.size = length;

    if (length == 0)
    {
	/* nothing to do */
    }
    else if (stuff->format == ZPixmap)    {
        /* 
	 * check for protocol/server padding differences  and 
	 * if shm address is not aligned to 8 bytes. 
	 */
        if (( widthBytesLine != widthBytesLineProto ) ||
	    ((unsigned long)shmdesc->addr + stuff->offset & (sizeof(long)-1))) 
	{
	    /* temp stuff for 64 bit alignment stuff */
	    register char * bufPtr, * protoPtr;
	    register int i;

	    if(!(tmpImage = (char *) ALLOCATE_LOCAL(length))) 
	      return (BadAlloc);
	    tmpAlloced = 1;
	    
	    (*pDraw->pScreen->GetImage)(pDraw, stuff->x, stuff->y,
					stuff->width, stuff->height,
					stuff->format, stuff->planeMask,
					tmpImage);
	    
	    /* for 64-bit server, convert image to pad to 32 bits */
	    bzero(shmdesc->addr + stuff->offset,lengthProto);
	    
	    for ( i=0,bufPtr=tmpImage,protoPtr=shmdesc->addr + stuff->offset; 
		 i < stuff->height;
		 bufPtr += widthBytesLine,protoPtr += widthBytesLineProto, 
		 i++)
	      bcopy(bufPtr,protoPtr,widthBytesLineProto);
	}
	else {			/* no diff between protocol and local size */
	    (*pDraw->pScreen->GetImage)(pDraw, stuff->x, stuff->y,
					stuff->width, stuff->height,
					stuff->format, stuff->planeMask,
					shmdesc->addr + stuff->offset);
	}
    }
    else
    {
		  
	/* 
	 * check for protocol/server padding differences  and 
	 * if shm address is not aligned to 8 bytes. 
	 *
	 * If so, allocate tmporary space to do the GetImage. 
	 */
	if (( widthBytesLine != widthBytesLineProto ) ||
	    ((unsigned long)shmdesc->addr + stuff->offset & 
	     (sizeof(long)-1))) 
	{
	    if(!(tmpImage = (char *) ALLOCATE_LOCAL(length)))
	      return (BadAlloc);
	    tmpAlloced = 1;
	}

	length = stuff->offset;
        for (; plane; plane >>= 1)
	{
	    if (stuff->planeMask & plane)
	    {
		if (( widthBytesLine != widthBytesLineProto ) ||
		    ((unsigned long)shmdesc->addr + stuff->offset & 
		     (sizeof(long)-1))) 
		{
		    /* 
		     * get image for each plane. 
		     */
		    (*pDraw->pScreen->GetImage)(pDraw,
						stuff->x, stuff->y,
						stuff->width, stuff->height,
						stuff->format, plane,
						tmpImage);
		    
		    /* for 64-bit server, convert image to pad to 32 bits */
		    bzero(shmdesc->addr+length, widthBytesLine);
		    bcopy(tmpImage, shmdesc->addr+length, widthBytesLineProto);
		    /* increment length */
		    length += widthBytesLineProto * stuff->height;
		}
		else 
		{	/* no diff between protocol and local size */
		    (*pDraw->pScreen->GetImage)(pDraw,
						stuff->x, stuff->y,
						stuff->width, stuff->height,
						stuff->format, plane,
						shmdesc->addr + length);
		    length += widthBytesLine * stuff->height;
		}
	    }
	}
    }
    

    if (client->swapped) {
    	swaps(&xgi.sequenceNumber, n);
    	swapl(&xgi.length, n);
	swapl(&xgi.visual, n);
	swapl(&xgi.size, n);
    }
    WriteToClient(client, sizeof(xShmGetImageReply), (char *)&xgi);

    if ( tmpAlloced )
      DEALLOCATE_LOCAL(tmpImage);

    return(client->noClientException);
}    
    

static PixmapPtr
fbShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    char	*addr;
{
    register PixmapPtr pPixmap;

    pPixmap = (PixmapPtr)xalloc(sizeof(PixmapRec));
    if (!pPixmap)
	return NullPixmap;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = depth;
    pPixmap->drawable.id = 0;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->devKind = PixmapBytePad(width, depth);
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = (pointer)addr;
    return pPixmap;
}

static int
ProcShmCreatePixmap(client)
    register ClientPtr client;
{
    PixmapPtr pMap;
    register DrawablePtr pDraw;
    DepthPtr pDepth;
    register int i;
    ShmDescPtr shmdesc;
    REQUEST(xShmCreatePixmapReq);

    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    client->errorValue = stuff->pid;
    if (!sharedPixmaps)
	return BadImplementation;
    LEGAL_NEW_RESOURCE(stuff->pid, client);
    VERIFY_GEOMETRABLE(pDraw, stuff->drawable, client);
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);
    if (!stuff->width || !stuff->height)
    {
	client->errorValue = 0;
        return BadValue;
    }
    if (stuff->depth != 1)
    {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i=0; i<pDraw->pScreen->numDepths; i++, pDepth++)
	   if (pDepth->depth == stuff->depth)
               goto CreatePmap;
	client->errorValue = stuff->depth;
        return BadValue;
    }
CreatePmap:
    VERIFY_SHMSIZE(shmdesc, stuff->offset,
		   PixmapBytePad(stuff->width, stuff->depth) * stuff->height,
		   client);
    pMap = (*shmFuncs[pDraw->pScreen->myNum]->CreatePixmap)(
			    pDraw->pScreen, stuff->width,
			    stuff->height, stuff->depth,
			    shmdesc->addr + stuff->offset);
    if (pMap)
    {
	pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	pMap->drawable.id = stuff->pid;
	if (AddResource(stuff->pid, RT_PIXMAP, (pointer)pMap))
	{
	    shmdesc->refcnt++;
	    if (AddResource(stuff->pid, ShmPixType, (pointer)shmdesc))
		return(client->noClientException);
	    FreeResource(stuff->pid, RT_NONE);
	}
    }
    return (BadAlloc);
}

static int
ProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_ShmQueryVersion:
	return ProcShmQueryVersion(client);
    case X_ShmAttach:
	return ProcShmAttach(client);
    case X_ShmDetach:
	return ProcShmDetach(client);
    case X_ShmPutImage:
	return ProcShmPutImage(client);
    case X_ShmGetImage:
	return ProcShmGetImage(client);
    case X_ShmCreatePixmap:
	return ProcShmCreatePixmap(client);
    default:
	return BadRequest;
    }
}

static void
SShmCompletionEvent(from, to)
    xShmCompletionEvent *from, *to;
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->drawable, to->drawable);
    cpswaps(from->minorEvent, to->minorEvent);
    to->majorEvent = from->majorEvent;
    cpswapl(from->shmseg, to->shmseg);
    cpswapl(from->offset, to->offset);
}

static int
SProcShmQueryVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xShmQueryVersionReq);

    swaps(&stuff->length, n);
    return ProcShmQueryVersion(client);
}

static int
SProcShmAttach(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xShmAttachReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xShmAttachReq);
    swapl(&stuff->shmseg, n);
    swapl(&stuff->shmid, n);
    return ProcShmAttach(client);
}

static int
SProcShmDetach(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xShmDetachReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xShmDetachReq);
    swapl(&stuff->shmseg, n);
    return ProcShmDetach(client);
}

static int
SProcShmPutImage(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xShmPutImageReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xShmPutImageReq);
    swapl(&stuff->drawable, n);
    swapl(&stuff->gc, n);
    swaps(&stuff->totalWidth, n);
    swaps(&stuff->totalHeight, n);
    swaps(&stuff->srcX, n);
    swaps(&stuff->srcY, n);
    swaps(&stuff->srcWidth, n);
    swaps(&stuff->srcHeight, n);
    swaps(&stuff->dstX, n);
    swaps(&stuff->dstY, n);
    swapl(&stuff->shmseg, n);
    swapl(&stuff->offset, n);
    return ProcShmPutImage(client);
}

static int
SProcShmGetImage(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xShmGetImageReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xShmGetImageReq);
    swapl(&stuff->drawable, n);
    swaps(&stuff->x, n);
    swaps(&stuff->y, n);
    swaps(&stuff->width, n);
    swaps(&stuff->height, n);
    swapl(&stuff->planeMask, n);
    swapl(&stuff->shmseg, n);
    swapl(&stuff->offset, n);
    return ProcShmGetImage(client);
}

static int
SProcShmCreatePixmap(client)
    ClientPtr client;
{
    register int n;
    REQUEST(xShmCreatePixmapReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    swapl(&stuff->drawable, n);
    swaps(&stuff->width, n);
    swaps(&stuff->height, n);
    swapl(&stuff->shmseg, n);
    swapl(&stuff->offset, n);
    return ProcShmCreatePixmap(client);
}

static int
SProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_ShmQueryVersion:
	return SProcShmQueryVersion(client);
    case X_ShmAttach:
	return SProcShmAttach(client);
    case X_ShmDetach:
	return SProcShmDetach(client);
    case X_ShmPutImage:
	return SProcShmPutImage(client);
    case X_ShmGetImage:
	return SProcShmGetImage(client);
    case X_ShmCreatePixmap:
	return SProcShmCreatePixmap(client);
    default:
	return BadRequest;
    }
}
