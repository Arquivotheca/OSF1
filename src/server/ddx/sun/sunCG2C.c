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
/*-
 * sunCG2C.c --
 *	Functions to support the sun CG2 board as a memory frame buffer.
 */

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef	lint
static char sccsid[] = "@(#)sunCG2C.c 2.8 87/06/05 Copyright 1987 Sun Micro";
#endif

#include    "sun.h"

#include    <sys/mman.h>
#include    <struct.h>
#include    <pixrect/memreg.h>
#include    <pixrect/cg2reg.h>
#include    "colormap.h"
#include    "colormapst.h"
#include    "resource.h"

extern Bool sunCG2MProbe();
#ifndef _MAP_NEW
extern caddr_t valloc();
#else
extern caddr_t mmap();
#endif	_MAP_NEW

/*-
 * The cg2 frame buffer is divided into several pieces.
 *	1) a stack of 8 monochrome bitplanes
 *	2) an array of 8-bit pixels
 *	3) a union of these two where modifications are done via RasterOp
 *	    chips
 *	4) various control registers
 *	5) a shadow colormap.
 *
 * Each of these things is at a given offset from the base of the 4Mb devoted
 * to each color board. In addition, the mmap() system call insists on the
 * address and the length to be mapped being aligned on 8K boundaries.
 * 
 * XXX This could be made a lot cleaner with proper use of structs and 
 * sizeof()'s.
 */

struct cg2c_reg {
    /*
     * The status register is at 0x309000.  This isn't on an 8K
     * boundary, so we have to put a 4K (0x1000) pad in front of it and
     * map it here at 0x308000.
     */
    char csr_base[4096];
    union {
	struct cg2statusreg csr;
	char csr_pad[4096];
    } u_csr;
};

struct cg2c_ppmask {
    /* per-plane mask, offset = 0x30A000, size = 8K */
    union {
	unsigned short	    ppmask;
	char	  	    ppm_pad[8192];
    } u_ppmask;
};

struct cg2c_cmap {
    /* colormap, offset = 0x310000, size = 8K */
    union {
	struct {  	/* Shouldn't these be u_char's??? */
	    u_short	    	redmap[256];	/* Red-component map */
	    u_short	    	greenmap[256];	/* Green-component map */
	    u_short	    	bluemap[256];	/* Blue-component map */
	}   	  	    cmap;
	char	  	    cmap_pad[8192];
    } u_cmap;
};

typedef struct cg2c {
    union byteplane *image;		/* the 8-bit memory */
    struct cg2c_reg *u_csr;		/* the status register */
    struct cg2c_ppmask *u_ppmask;	/* The plane mask register */
    struct cg2c_cmap *u_cmap;		/* the colormap */
} CG2C, CG2CRec, *CG2CPtr;

#define CG2C_IMAGE(fb)	    ((caddr_t)((fb).image))
#define CG2C_IMAGEOFF	    ((off_t)0x00100000)
#define CG2C_IMAGELEN	    (sizeof(union byteplane))
#define CG2C_REG(fb)	    ((caddr_t)((fb).u_csr))
#define CG2C_REGOFF	    ((off_t)0x00308000)
#define CG2C_REGLEN	    (0x2000)
#define CG2C_MASK(fb)	    ((caddr_t)((fb).u_ppmask))
#define CG2C_MASKOFF	    ((off_t)0x0030A000)
#define CG2C_MASKLEN	    (0x2000)
#define CG2C_CMAP(fb)	    ((caddr_t)((fb).u_cmap))
#define CG2C_CMAPOFF	    ((off_t)0x00310000)
#define CG2C_CMAPLEN	    (0x2000)

extern int TellLostMap(), TellGainedMap();

static void
sunCG2CUpdateColormap(pScreen, index, count, rmap, gmap,bmap)
    ScreenPtr	pScreen;
    int		  index, count;
    u_char	  *rmap, *gmap, *bmap;
{
    CG2CPtr 	  fb;

    fb = (CG2CPtr) sunFbs[pScreen->myNum].fb;
#ifdef SUN_WINDOWS
    if (sunUseSunWindows()) {
	static Pixwin *pw = 0;

	if (! pw) {
	    if ( ! (pw = pw_open(windowFd)) )
		FatalError( "sunCG2CUpdateColormap: pw_open failed\n" );
	    pw_setcmsname(pw, "X.V11");
	}
	pw_putcolormap(
	    pw, index, count, &rmap[index], &gmap[index], &bmap[index]
	);
    }
#endif SUN_WINDOWS

    fb->u_csr->u_csr.csr.update_cmap = 0;
    while (count--) {
	fb->u_cmap->u_cmap.cmap.redmap[index] = rmap[index];
	fb->u_cmap->u_cmap.cmap.greenmap[index] = gmap[index];
	fb->u_cmap->u_cmap.cmap.bluemap[index] = bmap[index];
	index++;
    }
    fb->u_csr->u_csr.csr.update_cmap = 1;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG2CSaveScreen --
 *	Preserve the color screen by turning on or off the video
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video state is switched
 *
 *-----------------------------------------------------------------------
 */
static Bool
sunCG2CSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    int    	  on;
{
    int		state;

    if (on == SCREEN_SAVER_FORCER)
	SetTimeSinceLastInputEvent();
    else
    {
	if (on == SCREEN_SAVER_ON)
	    state = 0;
	else
	    state = 1;
	((CG2CPtr)sunFbs[pScreen->myNum].fb)->u_csr->u_csr.csr.video_enab = state;
    }
    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunCG2CInit --
 *	Attempt to find and initialize a cg2 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in. Memory is
 *	allocated for the frame buffer and the buffer is mapped. The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG2CInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    sunScreenPtr    pPrivate;
    if (!cfbScreenInit (pScreen,
			(pointer)((CG2CPtr)sunFbs[index].fb)->image,
			sunFbs[index].info.fb_width,
			sunFbs[index].info.fb_height,
			monitorResolution, monitorResolution,
			sunFbs[index].info.fb_width))
	return (FALSE);

    if (!sunScreenAllocate (pScreen))
	return FALSE;

    sunCGScreenInit (pScreen);

    if (!sunScreenInit (pScreen))
	return FALSE;

    pPrivate = GetScreenPrivate (pScreen);

    pScreen->SaveScreen = sunCG2CSaveScreen;
    pPrivate->UpdateColormap = sunCG2CUpdateColormap;

    return cfbCreateDefColormap(pScreen);
}


/*-
 *-----------------------------------------------------------------------
 * sunCG2CProbe --
 *	Attempt to find and initialize a cg2 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped.
 *
 *-----------------------------------------------------------------------
 */
Bool
sunCG2CProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int		i;
    int         fd;
    struct fbtype	fbType;
    static CG2CRec	  CG2Cfb;

    /*
     * See if the user wants this board to be treated as a monochrome
     * display.
     */
    for (i = 0; i < argc; i++) {
	if (strcmp (argv[i], "-mono") == 0) {
	    return sunCG2MProbe (pScreenInfo, index, fbNum, argc, argv);
	}
    }

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUN2COLOR, &fbType,
				 index, fbNum, argc, argv)) < 0)
	return FALSE;

#ifdef	_MAP_NEW
    if ((int)(CG2Cfb.image = (union byteplane *) mmap ((caddr_t) 0, CG2C_IMAGELEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2C_IMAGEOFF)) == -1) {
		  Error ("Mapping cg2c.image");
		  goto bad;
    }
    if ((int)(CG2Cfb.u_csr = (struct cg2c_reg *) mmap ((caddr_t) 0, CG2C_REGLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2C_REGOFF)) == -1) {
		  Error ("Mapping cg2c.reg");
		  goto bad;
    }
    if ((int)(CG2Cfb.u_ppmask = (struct cg2c_ppmask *) mmap ((caddr_t) 0, CG2C_MASKLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2C_MASKOFF)) == -1) {
		  Error ("Mapping cg2c.reg");
		  goto bad;
    }
    if ((int)(CG2Cfb.u_cmap = (struct cg2c_cmap *) mmap ((caddr_t) 0, CG2C_CMAPLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2C_CMAPOFF)) != -1) {
		  goto ok;
    }
    Error ("Mapping cg2c.cmap");
#else
    CG2Cfb.image = (union byteplane *)valloc (CG2C_IMAGELEN + CG2C_REGLEN + CG2C_MASKLEN + CG2C_CMAPLEN);
    CG2Cfb.u_csr = (struct cg2c_reg *) ((char *)CG2Cfb.image + CG2C_IMAGELEN);
    CG2Cfb.u_ppmask = (struct cg2c_ppmask *) ((char *)CG2Cfb.u_csr + CG2C_REGLEN);
    CG2Cfb.u_cmap = (struct cg2c_cmap *) ((char *)CG2Cfb.u_ppmask + CG2C_MASKLEN);
    if (CG2Cfb.image == (union byteplane *) NULL) {
	ErrorF ("Could not allocate room for frame buffer.\n");
	return FALSE;
    }

    if (mmap (CG2C_IMAGE(CG2Cfb), CG2C_IMAGELEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2C_IMAGEOFF) < 0) {
		  Error ("Mapping cg2c.image");
		  goto bad;
    }
    if (mmap (CG2C_REG(CG2Cfb), CG2C_REGLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2C_REGOFF) < 0) {
		  Error ("Mapping cg2c.reg");
		  goto bad;
    }
    if (mmap (CG2C_MASK(CG2Cfb), CG2C_MASKLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2C_MASKOFF) < 0) {
		  Error ("Mapping cg2c.reg");
		  goto bad;
    }
    if (mmap (CG2C_CMAP(CG2Cfb), CG2C_CMAPLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2C_CMAPOFF) >= 0) {
		  goto ok;
    }
    Error ("Mapping cg2c.cmap");
#endif	_MAP_NEW
bad:
    (void) close (fd);
    return FALSE;

ok:
    /*
     * Enable all planes
     */
    CG2Cfb.u_ppmask->u_ppmask.ppmask = 0xFF;

    sunFbs[index].fd = fd;
    sunFbs[index].info = fbType;
    sunFbs[index].fb = (pointer) &CG2Cfb;
    sunFbs[index].EnterLeave = NoopDDA;
    sunSupportsDepth8 = TRUE;
    return TRUE;
}

/*ARGSUSED*/
Bool
sunCG2CCreate(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    return (AddScreen(sunCG2CInit, argc, argv) >= 0);
}
