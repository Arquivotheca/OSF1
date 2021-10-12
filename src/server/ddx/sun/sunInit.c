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
/* $XConsortium: sunInit.c,v 5.24 91/08/23 16:14:53 keith Exp $ */
/*-
 * sunInit.c --
 *	Initialization functions for screen/keyboard/mouse, etc.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
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
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif

#include    "sun.h"
#include    <servermd.h>
#include    "dixstruct.h"
#include    "dix.h"
#include    "opaque.h"
#include    "mipointer.h"

extern int sunMouseProc();
extern int sunKbdProc();
extern Bool sunBW2Probe(), sunBW2Create();
#ifndef MONO_ONLY
extern Bool sunCG2CProbe(), sunCG2CCreate();
extern Bool sunCG3CProbe(), sunCG3CCreate();
extern Bool sunCG4CProbe(), sunCG4CCreate();
#ifdef FBTYPE_SUNFAST_COLOR /* doesn't exist in sunos3.x */
extern Bool sunCG6CProbe(), sunCG6CCreate();
#endif
#endif
extern void ProcessInputEvents();

extern void SetInputCheck();
extern char *strncpy();
extern GCPtr CreateScratchGC();

#define	XDEVICE	"XDEVICE"
#define	PARENT	"WINDOW_GFX"

static int autoRepeatHandlersInstalled;	/* FALSE each time InitOutput called */

static Bool sunDevsProbed = FALSE;
Bool sunSupportsDepth8 = FALSE;
unsigned long sunGeneration = 0;
int sunScreenIndex;
Bool FlipPixels = FALSE;


/*-
 *-----------------------------------------------------------------------
 * SigIOHandler --
 *	Signal handler for SIGIO - input is available.
 *
 * Results:
 *	sunSigIO is set - ProcessInputEvents() will be called soon.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
SigIOHandler(sig, code, scp)
    int		code;
    int		sig;
    struct sigcontext *scp;
{
    sunEnqueueEvents ();
}

sunFbDataRec sunFbData[] = {
#ifndef MONO_ONLY
    sunBW2Probe,  	"/dev/bwtwo0",	    sunBW2Create,
    sunCG3CProbe,  	"/dev/cgthree0",    sunCG3CCreate,
#ifdef FBTYPE_SUNFAST_COLOR
    sunCG6CProbe,	"/dev/cgsix0",	    sunCG6CCreate,
#endif
    sunCG2CProbe,  	"/dev/cgtwo0",	    sunCG2CCreate,
    sunCG4CProbe,  	"/dev/cgfour0",	    sunCG4CCreate,
#endif
    sunBW2Probe,  	"/dev/bwtwo0",	    sunBW2Create,
};

#ifdef MONO_ONLY
#define DEV_START   0
#define DEV_END	    0
#else
#define DEV_START   1
#define DEV_END	    1
#endif

/*
 * NUMSCREENS is the number of supported frame buffers (i.e. the number of
 * structures in sunFbData which have an actual probeProc).
 */
#define NUMSCREENS (sizeof(sunFbData)/sizeof(sunFbData[0]))
#define NUMDEVICES  2

fbFd sunFbs[NUMDEVICES];

static PixmapFormatRec	formats[] = {
    1, 1, BITMAP_SCANLINE_PAD,	/* 1-bit deep */
    8, 8, BITMAP_SCANLINE_PAD,	/* 8-bit deep */
};
#define NUMFORMATS	(sizeof formats)/(sizeof formats[0])

/*-
 *-----------------------------------------------------------------------
 * sunNonBlockConsoleOff --
 *	Turn non-blocking mode on the console off, so you don't get logged
 *	out when the server exits.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
sunNonBlockConsoleOff(arg)
    char	*arg;
{
    register int i;

    i = fcntl(2, F_GETFL, 0);
    if (i >= 0)
	(void) fcntl(2, F_SETFL, i & ~FNDELAY);
}

/*-
 *-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *	The
 *
 * Results:
 *	screenInfo init proc field set
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */

InitOutput(pScreenInfo, argc, argv)
    ScreenInfo 	  *pScreenInfo;
    int     	  argc;
    char    	  **argv;
{
    int     	  i, n, dev;
    int		  nonBlockConsole = 0;
    static int	  setup_on_exit = 0;
    int		  devStart = DEV_START;
    extern Bool	  RunFromSmartParent;

    if (!monitorResolution)
	monitorResolution = 90;
    if (RunFromSmartParent)
	nonBlockConsole = 1;
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i],"-debug"))
	    nonBlockConsole = 0;
	else if (!strcmp(argv[i],"-mono"))
	    devStart = 0;
    }
    /*
     *	Writes to /dev/console can block - causing an
     *	excess of error messages to hang the server in
     *	deadlock.  So.......
     */
    if (nonBlockConsole) {
	if (!setup_on_exit) {
	    if (on_exit(sunNonBlockConsoleOff, (char *)0))
		ErrorF("InitOutput: can't register NBIO exit handler\n");
	    setup_on_exit = 1;
	}
	i = fcntl(2, F_GETFL, 0);
	if (i >= 0)
	    i = fcntl(2, F_SETFL, i | FNDELAY);
	if (i < 0) {
	    perror("fcntl");
	    ErrorF("InitOutput: can't put stderr in non-block mode\n");
	}
    }
    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
        pScreenInfo->formats[i] = formats[i];

    autoRepeatHandlersInstalled = FALSE;

    if (!sunDevsProbed)
    {
	n = 0;
	for (i = NUMSCREENS, dev = devStart; --i >= DEV_END; dev++) {
	    if ((*sunFbData[dev].probeProc)(pScreenInfo, n, dev, argc, argv))
		n++;
	    else
		sunFbData[dev].createProc = NULL;
	}
	sunDevsProbed = TRUE;
	if (n == 0)
	    return;
    }
    if (!sunSupportsDepth8)
	pScreenInfo->numPixmapFormats--;
    for (i = NUMSCREENS, dev = devStart; --i >= DEV_END; dev++) {
	if (sunFbData[dev].createProc)
	    (*sunFbData[dev].createProc)(pScreenInfo, argc, argv);
    }
    sunInitCursor();
    signal(SIGWINCH, SIG_IGN);
}

/*-
 *-----------------------------------------------------------------------
 * InitInput --
 *	Initialize all supported input devices...what else is there
 *	besides pointer and keyboard?
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Two DeviceRec's are allocated and registered as the system pointer
 *	and keyboard devices.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
InitInput(argc, argv)
    int     	  argc;
    char    	  **argv;
{
    DevicePtr p, k;
    static int  zero = 0;
    
    p = AddInputDevice(sunMouseProc, TRUE);
    k = AddInputDevice(sunKbdProc, TRUE);
    if (!p || !k)
	FatalError("failed to create input devices in InitInput");

    SetTimeSinceLastInputEvent();
    RegisterPointerDevice(p);
    RegisterKeyboardDevice(k);
    miRegisterPointerDevice(screenInfo.screens[0], p);
    if (!mieqInit (k, p))
	return FALSE;
    signal(SIGIO, SigIOHandler);
}


static Bool
sunCloseScreen (i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    SetupScreen(pScreen);
    Bool    ret;

    signal (SIGIO, SIG_IGN);
    sunDisableCursor (pScreen);
    pScreen->CloseScreen = pPrivate->CloseScreen;
    ret = (*pScreen->CloseScreen) (i, pScreen);
    (void) (*pScreen->SaveScreen) (pScreen, SCREEN_SAVER_OFF);
    xfree ((pointer) pPrivate);
    return ret;
}

Bool
sunSaveScreen (pScreen, on)
    ScreenPtr	pScreen;
    int		on;
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
	(void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);
    }
    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunScreenInit --
 *	Things which must be done for all types of frame buffers...
 *	Should be called last of all.
 *
 * Results:
 *	TRUE if successful, else FALSE
 *
 * Side Effects:
 *	Both a BlockHandler and a WakeupHandler are installed for the
 *	first screen.  Together, these handlers implement autorepeat
 *	keystrokes on the Sun.
 *
 *-----------------------------------------------------------------------
 */

Bool
sunScreenAllocate (pScreen)
    ScreenPtr	pScreen;
{
    sunScreenPtr    pPrivate;

    if (sunGeneration != serverGeneration)
    {
	sunScreenIndex = AllocateScreenPrivateIndex();
	if (sunScreenIndex < 0)
	    return FALSE;
	sunGeneration = serverGeneration;
    }
    pPrivate = (sunScreenPtr) xalloc (sizeof (sunScreenRec));
    if (!pPrivate)
	return FALSE;

    pScreen->devPrivates[sunScreenIndex].ptr = (pointer) pPrivate;
    return TRUE;
}

Bool
sunScreenInit (pScreen)
    ScreenPtr	pScreen;
{
    SetupScreen(pScreen);
    extern void   sunBlockHandler();
    extern void   sunWakeupHandler();
    static ScreenPtr autoRepeatScreen;
    extern miPointerScreenFuncRec   sunPointerScreenFuncs;

    pPrivate->installedMap = 0;
    pPrivate->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = sunCloseScreen;
    pScreen->SaveScreen = sunSaveScreen;

    /*
     *	Block/Unblock handlers
     */
    if (autoRepeatHandlersInstalled == FALSE) {
	autoRepeatScreen = pScreen;
	autoRepeatHandlersInstalled = TRUE;
    }

    if (pScreen == autoRepeatScreen) {
        pScreen->BlockHandler = sunBlockHandler;
        pScreen->WakeupHandler = sunWakeupHandler;
    }

    if (!sunCursorInitialize (pScreen))
	miDCInitialize (pScreen, &sunPointerScreenFuncs);

#ifdef SUN_WINDOWS
    /*
     * We need to wrap one of the cursor functions so that
     * we can keep the SunWindows cursor position in sync
     * with X. We still call the mi version to do the real
     * work.
     */
    pPrivate->SetCursorPosition = pScreen->SetCursorPosition;
    pScreen->SetCursorPosition = sunSetCursorPosition;
#endif
    sunInitCursor ();

    return TRUE;
}

extern char *getenv();

/*-
 *-----------------------------------------------------------------------
 * nthdev --
 *	Return the nth device in a colon-separated list of devices.
 *	n is 0-origin.
 *
 * Results:
 *	A pointer to a STATIC string which is the device name.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static char *
nthdev (dList, n)
    register char    *dList;	    /* Colon-separated device names */
    int	    n;	  	    /* Device number wanted */
{
    char *result;
    static char returnstring[100];

    while (n--) {
	while (*dList && *dList != ':') {
	    dList++;
	}
	if (*dList)
	    ++dList;
    }
    if (*dList) {
	register char *cp = dList;

	while (*cp && *cp != ':') {
	    cp++;
	}
	result = returnstring;
	strncpy (result, dList, cp - dList);
	result[cp - dList] = '\0';
    } else {
	result = (char *)0;
    }
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * sunOpenFrameBuffer --
 *	Open a frame buffer according to several rules. If running under
 *	overview and we're set up for it, use the device given in the
 *	PARENT envariable and note that the screen is under overview.
 *	Else find the device to use by looking in the sunFbData table,
 *	an XDEVICE envariable, a -dev switch or using /dev/fb if trying
 *	to open screen 0 and all else has failed.
 *
 * Results:
 *	The fd of the framebuffer.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
sunOpenFrameBuffer(expect, pfbType, index, fbNum, argc, argv)
    int	    	  expect;   	/* The expected type of framebuffer */
    struct fbtype *pfbType; 	/* Place to store the fb info */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  index;    	/* Screen index */
    int	    	  argc;	    	/* Command-line arguments... */
    char	  **argv;   	/* ... */
{
    char       	  *name=(char *)0;
    int           i;	    	/* Index into argument list */
    int           fd = -1;	    	/* Descriptor to device */
    static int	  devFbUsed=FALSE;  /* true if /dev/fb has been used for a */
    	    	  	    	    /* screen already */
    static Bool	  inited = FALSE;
    static char	  *xdevice; 	/* string of devices to use from environ */
    static char	  *devsw;   	/* string of devices from args */
    struct fbgattr  fbattr;
    int		  type;

    sunFbs[index].parent = FALSE;

    if (!inited) {
	xdevice = devsw = (char *)NULL;

	xdevice = getenv (XDEVICE);
	/*
	 * Look for an argument of the form -dev <device-string>
	 * If such a one is found place the <device-string> in devsw.
	 */
	for (i = 1; i < argc; i++) {
	    if ((strcmp(argv[i], "-dev") == 0) && (i + 1 < argc)) {
		devsw = argv[i+1];
		break;
	    }
	}
	inited = TRUE;
    }

    /*
     * Attempt to find a file name for the frame buffer 
     */

    /*
     * First see if any device was given on the command line.
     * If one was and the device is both readable and writeable,
     * set 'name' to it, else set it to NULL.
     */
    if (devsw == (char *)NULL ||
	(name = nthdev (devsw, index)) == (char *)NULL ||
	(access (name, R_OK | W_OK) != 0) ||
	(strcmp(name, sunFbData[fbNum].devName) != 0)) {
	    name = (char *)NULL;
    }
	    
    /*
     * If we still don't have a device for this screen, check the
     * environment variable for one. If one was given, stick its
     * path in name and check its accessibility. If it's not
     * properly accessible, then reset the name to NULL to force the
     * checking of the sunFbData array.
     */
    if (devsw == (char *)NULL && name == (char *)NULL &&
	xdevice != (char *)NULL &&
	(name = nthdev(xdevice, index)) != (char *)NULL &&
	(access (name, R_OK | W_OK) != 0)) {
	    name = (char *)NULL;
    }

    /*
     * Take the device given in the frame buffer description
     * and see if it exists and is accessible. If it does/is,
     * we will use it, as long as no other device was given.
     */
    if (devsw == (char *)NULL && name == (char *)NULL &&
	access(sunFbData[fbNum].devName, (R_OK | W_OK)) == 0) {
	    name = sunFbData[fbNum].devName;
    }

    /*
     * If we still have nothing and have yet to use "/dev/fb" for
     * a screen, default the name to be "/dev/fb"
     */
    if (devsw == (char *)NULL && name == (char *)NULL && !devFbUsed) {
	name = "/dev/fb";
    }


    if (name != (char *) NULL && sunUseSunWindows()) {
#ifdef	SUN_WINDOWS

	/*
	 * Running X in coexistence with SunWindows.
	 *
	 * This section of code enables X to run with SunWindows.  This is 
	 * accomplished by opening up screens and windows in the SunWindows 
	 * style and accepting input events from them.  However, since the X 
	 * graphics libraries want a raw framebuffer, we open the 
	 * framebuffer of the current screen and return that.  So we get 
	 * input from SunWindows but send output to the raw framebuffer.
	 */

	char       *parent = getenv("WINDOW_PARENT");
	struct screen sc;
	int	    winFd;
	int	    parentFd;
	int	    framebufferFd;
	Rect	    r;
	static struct screen newScreen;
	struct inputmask inputMask;
	Bool	    keepParent = FALSE;
	static int  sunFbFound = 0;	/* True if FB found under SunWindows */

	/*
	 * If no device was specified on the command line, open the window 
	 * specified in WINDOW_PARENT.  If a device was specified, open a 
	 * new screen on that device and use it as a parent window.
	 */

	if ( devsw ) {
	    bzero( (caddr_t)&newScreen, sizeof(newScreen) );
	    strcpy( newScreen.scr_fbname, name );
	    newScreen.scr_flags |= SCR_TOGGLEENABLE;
	    if ( (parentFd = win_screennew( &newScreen )) < 0 ) {
		ErrorF( "sunOpenFrameBuffer: Can't open new screen on %s.\n",
		    name );
		return( -1 );
	    }
	    keepParent = TRUE;
	} else {
	    if ((parentFd = open(parent, 2, 0)) < 0) {
		ErrorF("sunOpenFrameBuffer: Can't open parent %s.\n", parent);
		return (-1);
	    }
	}

	if ((winFd = win_getnewwindow()) < 0) {
	    ErrorF("sunOpenFrameBuffer: Can't open a new window.\n");
	    close( parentFd );
	    return (-1);
	}

	/* link the new window into the window hierarchy */
	bzero((caddr_t) & r, sizeof r);
	win_getrect(parentFd, &r);
	win_setrect(winFd, &r);
	win_setlink(winFd, WL_PARENT, win_fdtonumber(parentFd));
	win_setlink(winFd, WL_OLDERSIB, win_getlink(parentFd, WL_TOPCHILD));
	sunFbs[index].parent = TRUE;

	/*
	 * We would like to close the parent window here, since it's no 
	 * longer needed.  However, in the case where the parent is really a 
	 * screen, we have to keep it open, because closing a screen closes 
	 * all the windows on that screen.
	 */

	if ( ! keepParent )
	    close(parentFd);
	
	/*
	 * Express interest in SunView mouse events.  Note: the win_insert 
	 * must be AFTER setting the input mask in order to receive the 
	 * initial LOC_WINENTER event.
	 */

	input_imnull(&inputMask);
	inputMask.im_flags = IM_ASCII | IM_META | IM_NEGEVENT | IM_INTRANSIT;

	win_setinputcodebit(&inputMask, LOC_MOVE);
	win_setinputcodebit(&inputMask, LOC_WINEXIT);
	win_setinputcodebit(&inputMask, LOC_WINENTER);
	win_setinputcodebit(&inputMask, MS_LEFT);
	win_setinputcodebit(&inputMask, MS_MIDDLE);
	win_setinputcodebit(&inputMask, MS_RIGHT);
	win_set_pick_mask(winFd, &inputMask);

	input_imnull(&inputMask);
	win_setinputcodebit(&inputMask, KBD_USE);
	win_setinputcodebit(&inputMask, KBD_DONE);
	win_set_kbd_mask(winFd, &inputMask);

	win_insert(winFd);

	/*
	 * Determine the framebuffer name from the window's screen, then 
	 * open it.  Then do an FBIOGTYPE ioctl to determine its type.
	 */

	win_screenget( winFd, &sc );
	if ((framebufferFd = open(sc.scr_fbname, O_RDWR, 0)) < 0) {
	    ErrorF("sunOpenFrameBuffer: can't open %s\n",sc.scr_fbname);
	    (void) close(winFd);
	    (void) close(parentFd);
	    return (-1);
	}

	if (ioctl(framebufferFd, FBIOGTYPE, pfbType) < 0) {
	    perror("sunOpenFrameBuffer: FBIOGTYPE");
badfb:
	    (void) close(framebufferFd);
	    (void) close(winFd);
	    (void) close(parentFd);
	    return (-1);
	}

	if (pfbType->fb_type != expect) {
	    struct fbgattr fbgattr;
	    int i;

	    /*
	     * On a 3/110 we only want to open one display
	     * on /dev/fb (or /dev/cgfour0) if no device is
	     * specified in the command line.  If not, we won't
	     * be able to slide back and forth between X displays
	     * since only one SunWindows framebuffer is open.
	     * The screen that is opened is /dev/bwtwo0.
	     */
	    if (sunFbFound && (devsw == (char *) NULL))
		goto badfb;

	    if (ioctl(framebufferFd, FBIOGATTR, &fbgattr) < 0)
		goto badfb;
	    
	    for (i=0; i<FB_ATTR_NEMUTYPES; i++)
		if (fbgattr.emu_types[i] == expect)
		    break;
		else if (fbgattr.emu_types[i] == -1)
		    goto badfb;
	}
        /*
	 * NDELAY only applies to "input" fds, or fds that can be
	 * read.  sunwindows fds are read, while frame buffer fds aren't.
	 * That's why this fcntl is in the conditional compilation section.
	 */

	if (fcntl(winFd, F_SETFL, FNDELAY|FASYNC) < 0) {
	    ErrorF("Can't set FNDELAY|FASYNC on %s\n",name);
	    perror("sunOpenFrameBuffer");
	    (void) close(winFd);
	    return (-1);
	}
	
	fd = framebufferFd;
	windowFd = winFd;
	sunFbFound++;
#else
	ErrorF("Not configured to run inside SunWindows\n");
	fd = -1;
#endif	SUN_WINDOWS
    } else if (name) {
	fd = open(name, O_RDWR, 0);
        if (fd < 0) {
	    return (-1);
	} 
	if (ioctl(fd, FBIOGATTR, &fbattr) < 0)
	{
	    if (ioctl(fd, FBIOGTYPE, pfbType) < 0) {
	    	perror("sunOpenFrameBuffer");
	    	(void) close(fd);
	    	return (-1);
	    }
	    type = pfbType->fb_type;
	}
	else
	{
	    int	i;

	    type = fbattr.real_type;
	    *pfbType = fbattr.fbtype;
	    if (expect == FBTYPE_SUN2BW && expect != type)
	    {
	    	for (i = 0; i < FB_ATTR_NEMUTYPES; i++)
	    	{
		    if (expect == fbattr.emu_types[i])
		    {
		    	type = fbattr.emu_types[i];
			break;
		    }
	    	}
	    }
	}
	/* XXX - this is temporary 'cos the CG4 pretends its a BW2 */
	if (strcmp(name, sunFbData[fbNum].devName) != 0 && type != expect)
	{
	    (void) close(fd);
	    return (-1);
	}
    }

    if (name && strcmp (name, "/dev/fb") == 0) {
	devFbUsed = TRUE;
    }

    return (fd);
}
