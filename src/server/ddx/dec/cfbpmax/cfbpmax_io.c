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
/***********************************************************
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
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $Header: /usr/sde/x11/rcs/x11/src/./server/ddx/dec/cfbpmax/cfbpmax_io.c,v 1.2 91/12/15 12:42:16 devrcs Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <errno.h>
#include <sys/devio.h>
#include <io/tc/pmioctl.h>
#include <io/tc/dc7085reg.h>

#include "misc.h"
#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "scrnintstr.h"
#include "pixmap.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "resource.h"
#include "dixstruct.h"

#include "cfbpmaxcolor.h"
#include "mi.h"
#include "mfb.h"

void cfbpmaxQueryBestSize();
Bool cfbpmaxRealizeCursor();
Bool cfbpmaxUnrealizeCursor();
Bool cfbpmaxDisplayCursor();
void cfbpmaxRecolorCursor();
Bool cfbpmaxSetCursorPosition();
void cfbpmaxCursorLimits();
void cfbpmaxConstrainCursor();
void cfbpmaxPointerNonInterestBox();
void cfbpmaxChangeKeyboardControl();
void cfbpmaxChangePointerControl();
void cfbpmaxClick();

extern Bool cfbScreenInit();
extern void miRecolorCursor();
extern void NoopDDA();

/* XXX warning following symbol is copied from pmreg.h - this will
 * be fixed when the driver header files get straightened out. PK
 * 17 Aug 88
 */
#define MOTION_BUFFER_SIZE 100

/*
 * These "statics" imply there will never be more than one "pm" display and
 * mouse attached to a server; otherwise, they would be in SCREEN private info.
 *
 * This is probably the case since the Ultrix driver does not allow one to
 * open the display without also getting at the same file descriptor that
 * handles the mouse and the keyboard. Most of it is merely to pass information
 * to the keyboard and pointer devices. This could be done more cleanly by
 * repeating the ioctl to get the PM_Info.
 */

int			fdPM;

static PM_Info		*info;
static pmEventQueue	*queue;
static pmBox		*mbox;
static pmCursor		*mouse;
static int		qLimit;
static int		lastEventTime;
static DevicePtr	pmKeyboard;
static DevicePtr	pmPointer;
static int		hotX, hotY;
static BoxRec		cursorRange, cursorConstraint;
static int              dpix = -1, dpiy = -1, dpi = -1;
static char             *blackValue = NULL, *whiteValue = NULL;
static int		class = PseudoColor;
static unsigned	        cursColors[6]; /* 0-2 bg 3-5 fg */

#define MAX_LED 3	/* only 3 LED's can be set by user; Lock LED
			   is controlled by server */

#define FRAMEWIDTH      1024
#define FRAMEHEIGHT     864
#define DEPTH           8
#define BITS_PER_CHAR   8

#ifdef VSYNCFIXED
#define CURRENT_TIME	queue->timestamp_ms
#else
#define CURRENT_TIME	GetTimeInMillis()
#endif

#define NoSuchClass -1

static int
ParseClass(className)
    char *	className;
{
    static char *names[] = {
	"StaticGray", "GrayScale", "StaticColor",
	"PseudoColor", "TrueColor"};
    /* only the ones we support and must be in order from X.h, since
     * return value depends on index into array.
     */
    int i;
    for (i = 0; i < sizeof(names)/sizeof(char *); i++)
    {
	if (strcmp(names[i], className) == 0)
	    return i;
    }
    return NoSuchClass;
}

static Bool
commandLineMatch( argc, argv, pat, pmatch)
    int         argc;           /* may NOT be changed */
    char *      argv[];         /* may NOT be changed */
    char *      pat;
{
    int         ic;

    for ( ic=0; ic<argc; ic++)
        if ( strcmp( argv[ic], pat) == 0)
            return TRUE;
    return FALSE;
}

static Bool
commandLinePairMatch( argc, argv, pat, pmatch)
    int         argc;           /* may NOT be changed */
    char *      argv[];         /* may NOT be changed */
    char *      pat;
    char **     pmatch;         /* RETURN */
{
    int         ic;

    for ( ic=0; ic<argc; ic++)
        if ( strcmp( argv[ic], pat) == 0)
        {
            *pmatch = argv[ ic+1];
            return TRUE;
	}
    return FALSE;
}

static void
colorNameToColor( pname, pred, pgreen, pblue)
    char *      pname;
    u_int *     pred;
    u_int *     pgreen;
    u_int *     pblue;
{
    if ( *pname == '#')
    {
        pname++;                /* skip over # */
        sscanf( pname, "%2x", pred);
        *pred <<= 8;

        pname += 2;
        sscanf( pname, "%2x", pgreen);
        *pgreen <<= 8;

        pname += 2;
        sscanf( pname, "%2x", pblue);
        *pblue <<= 8;
    }
    else /* named color */
    {
        *pred = *pgreen = *pblue = 0; /*OsLookupColor thinks these are shorts*/
        OsLookupColor( 0 /*"screen", not used*/, pname, strlen( pname),
                pred, pgreen, pblue);
    }
}

/* SaveScreen does blanking, so no need to worry about the interval timer */

static Bool
cfbpmaxSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{
    if (on == SCREEN_SAVER_FORCER)
    {
        lastEventTime = CURRENT_TIME;	
    }
    else if (on == SCREEN_SAVER_ON)
    {
	if (ioctl(fdPM, QIOVIDEOOFF) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen off.\n");
    } else {
	if (ioctl(fdPM, QIOVIDEOON) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen on.\n");
    }
    return TRUE;
}

Bool
cfbpmaxScreenClose(index, pScreen)
    int index;
    ScreenPtr pScreen;
{
    /* This routine frees all of the dynamically allocated space associate
	with a screen. */

    /* ||| XXX cfbScreenClose(pScreen); */

/* ||| Just seeing if this lets me shut down and restart clients
    if(close(fdPM))
    {
	ErrorF("Close of fdPM yields %d\n", errno);
	return(FALSE);
    }
*/
    return(TRUE);
}

Bool
cfbpmaxScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;
    char **argv;
{
    register    PixmapPtr pPixmap;
    ColormapPtr pColormap;
    int		i;
    static int  mapOnce = FALSE;

/* for initializing color map entries */
    u_int blackred      = 0x0000;
    u_int blackgreen    = 0x0000;
    u_int blackblue     = 0x0000;

    u_int whitered      = 0xffff;
    u_int whitegreen    = 0xffff;
    u_int whiteblue     = 0xffff;

    if (mapOnce)
    {
	/*
	 * The reason you need to do this is so that when the
	 * the server recycles and the system is in video off
	 * state you need something to turn the video on.  Note
	 * that unlike the vaxstar you don't do a fresh open 
	 * and close of the device and thus the video remains
	 * off if you don't explicitly turn it on.
	 */
	if (ioctl(fdPM, QIOVIDEOON) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen on.\n");
    }

    if (! mapOnce) {
	if ((fdPM = open("/dev/mouse", O_RDWR | O_NDELAY, 0)) < 0)
	{
	    ErrorF("couldn't open /dev/mouse \n");
	    return FALSE;
	}

	if (ioctl(fdPM, QIOCGINFO, &info) < 0)
	{
	    extern int errno;
	    int en = errno;
	    fprintf(stderr, "errno = %d, ", en);
	    ErrorF("error getting address of pmax screen\n");
	    close(fdPM);
	    return FALSE;
	}
	ioctl(fdPM, QIOKERNLOOP);

	mapOnce = TRUE;
    }

    queue = &info->qe;
    mouse = &info->mouse;
    qLimit = queue->eSize - 1;
    lastEventTime = CURRENT_TIME;

    /* discard all the current input events */
    queue->eHead = queue->eTail;

    /*
     * set keyclick, mouse acceleration and threshold
     */
    {
        int	    clicklevel;
        char *      clickvolume;
        char *      mouseAcceleration;
        int         ma = 4;
        char *      mouseThreshold;
        int         mt = 4;
        PtrCtrl     ctrl;

        if ( commandLinePairMatch( argc, argv, "c", &clickvolume))
	{
		sscanf( clickvolume, "%d", &clicklevel);
		cfbpmaxClick(clicklevel);
	}

        if ( commandLineMatch( argc, argv, "-c"))
	{
		cfbpmaxClick(0);
	}
	    
        /*
         * calling cfbpmaxChangePointerControl here may be unclean       XXX
         */
	if ( commandLinePairMatch( argc, argv, "-a", &mouseAcceleration))
	    sscanf( mouseAcceleration, "%d", &ma);
	if ( commandLinePairMatch( argc, argv, "-t", &mouseThreshold))
	    sscanf( mouseThreshold, "%d", &mt);
	ctrl.num = ma;
	ctrl.den = 1;
	ctrl.threshold = mt;
	cfbpmaxChangePointerControl( (DevicePtr) NULL, &ctrl);
    }


    cursorRange.x1 = -15;
    cursorRange.x2 = info->max_x - 1;
    cursorRange.y1 = -15;
    cursorRange.y2 = info->max_y - 1;

/* XXX FRAMEWIDTH is bogus.  Don't we get this with initial IOCTL?? */

    if (dpi == -1) /* dpi has not been set */
    {
        if (dpix == -1) /* ie dpix has not been set */
        {
	    if (dpiy == -1)
	    {
	        dpix = 78;
	        dpiy = 78;
	    }
	    else
	        dpix = dpiy;
        }
        else
        { 
	    if (dpiy == -1)
	        dpiy = dpix;
        }
    }
    else
    {
	dpix = dpi;
	dpiy = dpi;
    }

    pScreen->CloseScreen = cfbpmaxScreenClose;
    pScreen->SaveScreen = cfbpmaxSaveScreen;
    pScreen->RealizeCursor = cfbpmaxRealizeCursor;
    pScreen->UnrealizeCursor = cfbpmaxUnrealizeCursor;
    pScreen->DisplayCursor = cfbpmaxDisplayCursor;
    pScreen->SetCursorPosition = cfbpmaxSetCursorPosition;
    pScreen->CursorLimits = cfbpmaxCursorLimits;
    pScreen->PointerNonInterestBox = cfbpmaxPointerNonInterestBox;
    pScreen->ConstrainCursor = cfbpmaxConstrainCursor;
    pScreen->RecolorCursor = cfbpmaxRecolorCursor;
    pScreen->QueryBestSize = cfbpmaxQueryBestSize;

    pScreen->StoreColors = cfbpmaxStoreColors;
    pScreen->InstallColormap = cfbpmaxInstallColormap;
    pScreen->UninstallColormap = cfbpmaxUninstallColormap;
    pScreen->ListInstalledColormaps = cfbpmaxListInstalledColormaps;

    if (!cfbScreenInit(pScreen, info->bitmap, FRAMEWIDTH,
			FRAMEHEIGHT, dpix, dpiy, FRAMEWIDTH, class, FRAMEWIDTH) ||
	!cfbCreateDefColormap (pScreen))
    {
	close (fdPM);
	return FALSE;
    }
    return TRUE;
}

cfbpmaxPixelError(index)
int index;
{
    ErrorF("Only 0 through 255 are acceptable pixels for device %d\n", index);
}
/* Totally bogus.  This procedure and data should be in the screen structure,
   but devPriv is used extensively for the pointer to the screen bits.  There
   is no screenPriv field. ||| */

unsigned int
cfbpmaxSetPlaneMask(planemask)
    unsigned int planemask;
{
    static unsigned int currentmask = ~0;
    unsigned int result;

    result = currentmask;
    currentmask = *(info->planemask) = planemask;
    return result;
}

static void
ChangeLED(led, on)
    int led;
    Bool on ;
{
    struct pm_kpcmd ioc;

    switch (led) {
       case 1:
          ioc.par[0] = LED_1;
          break;
       case 2:
          ioc.par[0] = LED_2;
          break;
       case 3:
          /* the keyboard's LED_3 is the Lock LED, which the server owns.
             So the user's LED #3 maps to the keyboard's LED_4. */
          ioc.par[0] = LED_4;
          break;
       default:
          return;   /* out-of-range LED value */
          }

    ioc.cmd = on ? LK_LED_ENABLE : LK_LED_DISABLE;
    ioc.par[1] = 0;
    ioc.nbytes = 1;
    ioctl(fdPM, QIOCKPCMD, &ioc);
}


static void
cfbpmaxClick(click)
    int         click;
{
#define LK_ENABLE_CLICK 0x1b	/* enable keyclick / set volume	*/
#define LK_DISABLE_CLICK 0x99	/* disable keyclick entirely	*/
#define LK_ENABLE_BELL 0x23	/* enable bell / set volume 	*/

    struct pm_kpcmd ioc;

    if (click == 0)    /* turn click off */
    {
	ioc.nbytes = 0;
	ioc.cmd = LK_DISABLE_CLICK;
	ioctl(fdPM, QIOCKPCMD, &ioc);
    }
    else 
    {
        int volume;

        volume = 7 - ((click / 14) & 7);
	ioc.nbytes = 1;
	ioc.cmd = LK_ENABLE_CLICK;
	ioc.par[0] = volume;
	ioctl(fdPM, QIOCKPCMD, &ioc);
    }
#undef LK_ENABLE_CLICK 
#undef LK_DISABLE_CLICK
#undef LK_ENABLE_BELL 

}

static void
cfbpmaxChangeKeyboardControl(device, ctrl)
    DevicePtr device;
    KeybdCtrl *ctrl;
{
    int i;

    cfbpmaxClick(ctrl->click);

    /* ctrl->bell: the DIX layer handles the base volume for the bell */
    
    /* ctrl->bell_pitch: as far as I can tell, you can't set this on lk201 */

    /* ctrl->bell_duration: as far as I can tell, you can't set this  */

    /* LEDs */
    for (i=1; i<=MAX_LED; i++)
        ChangeLED(i, (ctrl->leds & (1 << (i-1))));

    /* ctrl->autoRepeat: I'm turning it all on or all off.  */

    SetLKAutoRepeat(ctrl->autoRepeat);
}

static void
cfbpmaxBell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{
    struct pm_kpcmd ioc;

/* the lk201 volume is between 7 (quiet but audible) and 0 (loud) */
    loud = 7 - ((loud / 14) & 7);
    ioc.nbytes = 1;
    ioc.cmd = LK_BELL_ENABLE;
    ioc.par[0] = loud;
    ioctl(fdPM, QIOCKPCMD, &ioc);

    ioc.nbytes = 0;
    ioc.cmd = LK_RING_BELL;
    ioctl(fdPM, QIOCKPCMD, &ioc);
}

/*
 * These serve protocol requests, setting/getting acceleration and threshold.
 * X10 analog is "SetMouseCharacteristics".
 */
static void
cfbpmaxChangePointerControl(device, ctrl)
    DevicePtr device;
    PtrCtrl   *ctrl;
{
    info->mthreshold = ctrl->threshold;
    if (!(info->mscale = ctrl->num / ctrl->den))
	info->mscale = 1;	/* watch for den > num */
}

static int
cfbpmaxGetMotionEvents(pDevice, buff, start, stop)
    CARD32 start, stop;
    DevicePtr pDevice;
    xTimecoord *buff;
{
    int     count = 0;
    int     tcFirst = queue->tcNext;
    int     tcLast = (tcFirst) ? tcFirst - 1 : queue->tcSize - 1;
    register    pmTimeCoord * tcs;
    int     i;

    for (i = tcFirst;; i++) {
	if (i = queue->tcSize)
	    i = 0;
	tcs = &queue->tcs[i];
	if ((start <= tcs->time) && (tcs->time <= stop)) {
	    buff[count].time = tcs->time;
	    buff[count].x = tcs->x;
	    buff[count].y = tcs->y;
	    count++;
	}
	if (i == tcLast)
	    return count;
    }
}

int
cfbpmaxMouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    int     i;
    BYTE    map[4];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    pmPointer = pDev;
	    pDev->devicePrivate = (pointer) &queue;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pmPointer, map, 3, cfbpmaxGetMotionEvents,
		cfbpmaxChangePointerControl, MOTION_BUFFER_SIZE);
	    SetInputCheck(&queue->eHead, &queue->eTail);
	    hotX = hotY = 0;	
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdPM); 
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    RemoveEnabledDevice(fdPM);
	    break;
	case DEVICE_CLOSE:
	    break;
    }
    return Success;
}

#define LK_REPEAT_ON  0xe3
#define LK_REPEAT_OFF 0xe1

int
SetLKAutoRepeat (onoff)
    Bool onoff;
{
    extern char *AutoRepeatLKMode();
    extern char *UpDownLKMode();
    
    struct pm_kpcmd ioc;
    register char  *divsets;
    divsets = onoff ? (char *) AutoRepeatLKMode() : (char *) UpDownLKMode();
    ioc.nbytes = 0;
    while (ioc.cmd = *divsets++)
	ioctl(fdPM, QIOCKPCMD, &ioc);
    ioc.cmd = ((onoff > 0) ? LK_REPEAT_ON : LK_REPEAT_OFF);
    return(ioctl(fdPM, QIOCKPCMD, &ioc));
}

int
cfbpmaxKeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];

    switch (onoff)
    {
	case DEVICE_INIT: 
/* Note that keyclick is off by default.  The QDSS MIT server sets it
   to 20 */	
	    pmKeyboard = pDev;
	    GetLK201Mappings( &keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    pmKeyboard, &keySyms, modMap, cfbpmaxBell,
		    cfbpmaxChangeKeyboardControl);
            cfbpmaxClick(0);     /* turn key click off */	    
	    /* Free the key sym mapping space allocated by GetLK201Mappings. */
	    Xfree(keySyms.map);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdPM); 
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    RemoveEnabledDevice(fdPM);
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}
/*
 * The driver has been set up to put events in the queue that are identical
 * in shape to the events that the DDX layer has to deliver to ProcessInput
 * in DIX.
 */
extern int screenIsSaved;

void
ProcessInputEvents()
{
#define DEVICE_KEYBOARD 2
    xEvent x;
    pmEvent e;
    register int    i;

    i = queue->eHead;
    while (i != queue->eTail)
    {
	e = *((pmEvent *)&queue->events[i]);
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	x.u.keyButtonPointer.rootX = e.x + hotX;
	x.u.keyButtonPointer.rootY = e.y + hotY;
	x.u.keyButtonPointer.time = lastEventTime = e.time;
	x.u.u.detail = e.key;

	if (e.device == DEVICE_KEYBOARD)
	{
	    switch (e.type)
	    {
		case BUTTON_DOWN_TYPE: 
		    x.u.u.type = KeyPress;
		    (*pmKeyboard->processInputProc) (&x, pmKeyboard, 1);
		    break;
		case BUTTON_UP_TYPE: 
		    x.u.u.type = KeyRelease;
		    (*pmKeyboard->processInputProc) (&x, pmKeyboard, 1);
		    break;
		default: 	       /* hopefully BUTTON_RAW_TYPE */
		    ProcessLK201Input(&x, pmKeyboard);
	    }
	}
	else
	{
	    switch (e.type)
	    {
		case BUTTON_DOWN_TYPE: 
		    x.u.u.type = ButtonPress;
		    break;
		case BUTTON_UP_TYPE: 
		    x.u.u.type = ButtonRelease;
		    break;
		case MOTION_TYPE: 
		    x.u.u.type = MotionNotify;
		    break;
		default: 
		    printf("Unknown input event = %d\n",e.type);
		    continue;
	    }
	    (*pmPointer->processInputProc) (&x, pmPointer, 1);
	}
	if (i == qLimit)
	    i = queue->eHead = 0;
	else
	    i = ++queue->eHead;
    }
#undef DEVICE_KEYBOARD
}

TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
	lastEventTime = CURRENT_TIME;
    return CURRENT_TIME -  lastEventTime;
}

/*
 * set the bounds in the device for this particular cursor
 */
static void
cfbpmaxConstrainCursor( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    cursorConstraint = *pBox;
    if (info)
    {
	info->max_cur_x = pBox->x2 - hotX - 1;
	info->max_cur_y = pBox->y2 - hotY - 1;
	info->min_cur_x = pBox->x1 - hotX;
	info->min_cur_y = pBox->y1 - hotY;
    }
}

Bool
cfbpmaxSetCursorPosition( pScr, newx, newy, generateEvent)
    ScreenPtr	pScr;
    unsigned int	newx;
    unsigned int	newy;
    Bool		generateEvent;
{
    pmCursor pmCPos;
    xEvent motion;

    pmCPos.x = newx - hotX;
    pmCPos.y = newy - hotY;

    if (ioctl (fdPM, QIOCPMSTATE, &pmCPos) < 0) {
	ErrorF ("error warping cursor\n");
	return FALSE;
    }

    if (generateEvent) {
	if (queue->eHead != queue->eTail)
	    ProcessInputEvents ();
	motion.u.keyButtonPointer.rootX = newx;
	motion.u.keyButtonPointer.rootY = newy;
	motion.u.keyButtonPointer.time = currentTime.milliseconds;
	motion.u.u.type = MotionNotify;
	(pmPointer->processInputProc)(&motion, pmPointer, 1);
    }
    return TRUE;
}

static Bool
cfbpmaxDisplayCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    register    int		i;
    int		x, y;
    unsigned    *pColors;

    /*
     * load the cursor
     */
    if ((hotX != pCurs->bits->xhot) || (hotY != pCurs->bits->yhot))
    {
	x = mouse->x + hotX;
	y = mouse->y + hotY;
	hotX = pCurs->bits->xhot;
	hotY = pCurs->bits->yhot;
	cfbpmaxSetCursorPosition(pScr, x, y, FALSE);
	cfbpmaxConstrainCursor(pScr, &cursorConstraint);
		/* to update constraints in driver */
    }
    if ( ioctl( fdPM, QIOWCURSOR, pCurs->devPriv[ pScr->myNum]) < 0)
    {
	ErrorF( "error loading bits of new cursor\n");
        return FALSE;
    }


    /* pick the background color */
    cursColors[3] = pCurs->backRed;
    cursColors[4] = pCurs->backGreen;
    cursColors[5] = pCurs->backBlue;

    /* pick the foreground color */
    cursColors[0] = pCurs->foreRed;
    cursColors[1] = pCurs->foreGreen;
    cursColors[2] = pCurs->foreBlue;

    /*
    for (i=0;i<6;i++)
	printf ("colors[%d]= %d\n", i, cursColors[i]);
    */
    pColors = &cursColors[0];
    if ( ioctl( fdPM, QIOWCURSORCOLOR, cursColors) < 0)
    {
	ErrorF( "error writing colors of new cursor\n");
        return FALSE;
    }

    return (TRUE);

}

static void
cfbpmaxRecolorCursor (pScr, pCurs, displayed)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
    Bool	displayed;
{
    unsigned    *pColors;

    if (!displayed)
	return;

    /* pick the background color */
    cursColors[3] = pCurs->backRed;
    cursColors[4] = pCurs->backGreen;
    cursColors[5] = pCurs->backBlue;

    /* pick the foreground color */
    cursColors[0] = pCurs->foreRed;
    cursColors[1] = pCurs->foreGreen;
    cursColors[2] = pCurs->foreBlue;

    /*
    for (i=0;i<6;i++)
	printf ("colors[%d]= %d\n", i, cursColors[i]);
    */
    pColors = &cursColors[0];
    if ( ioctl( fdPM, QIOWCURSORCOLOR, &pColors) < 0)
    {
	ErrorF( "error writing colors of new cursor\n");
        return;
    }

    return;
}

static Bool
cfbpmaxRealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;	/* a SERVER-DEPENDENT cursor */
{
    int	forecolor = 0;
    int	backcolor = ~forecolor;
    register short	*a, *b;	/* vaxstar-defined */
    register int *	mask;	/* server-defined */
    register int *	src;	/* server-defined */
    int		i;
    int		cursorBytes = 32*sizeof(short);
    int		lastRow = ((pCurs->bits->height < 16) ? pCurs->bits->height : 16);
    register unsigned short widthmask = (1<<pCurs->bits->width)-1;
				/* used to mask off beyond the edge of the
				   real mask and source bits
				*/

    pCurs->devPriv[ pScr->myNum] = (pointer)Xalloc(cursorBytes);
    bzero((char *)pCurs->devPriv[ pScr->myNum], cursorBytes);

    /*
     * munge the SERVER-DEPENDENT, device-independent cursor bits into
     * what the device wants, which is 32 contiguous shorts.
     *
     * cursor hardware has "A" and "B" bitmaps
     * logic table is:
     *
     *		A	B	cursor
     *
     *		0	0	transparent
     *		1	0	xor (not used)
     *		0	1	black
     *		1	1	white
     */

    /*
     * "a" bitmap = image 
     */
    /*
     * "b" bitmap can be same as "mask", providing "a" is never on when
     *  "b" is off.
     */
    for ( i=0,
	  a = (short *)pCurs->devPriv[pScr->myNum],
	  b = ((short *)pCurs->devPriv[pScr->myNum]) + 16,
	/* XXX assumes server bitmap pad is size of int, 
	   and cursor is < 32 bits wide */
	  src = (int *)pCurs->bits->source,
	  mask = (int *)pCurs->bits->mask;

	  i < lastRow;

	  i++, a++, b++, src++, mask++)
    {
	*a = ((*src & forecolor) | (~*src & backcolor)) & *mask;
	*b = *mask;
	*a &= widthmask;
	*b &= widthmask;
    }
    return TRUE;
}

static Bool
cfbpmaxUnrealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    Xfree( pCurs->devPriv[ pScr->myNum]);
    return TRUE;
}

/*
 * pm cursor top-left corner can now go to negative coordinates
 */
static void
cfbpmaxCursorLimits( pScr, pCurs, pHotBox, pPhysBox)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
    BoxPtr	pHotBox;
    BoxPtr	pPhysBox;	/* return value */
{
    pPhysBox->x1 = max( pHotBox->x1, cursorRange.x1 + (int) pCurs->bits->xhot);
    pPhysBox->y1 = max( pHotBox->y1, cursorRange.y1 + (int) pCurs->bits->yhot);
    pPhysBox->x2 = min( pHotBox->x2, cursorRange.x2 + 1);
    pPhysBox->y2 = min( pHotBox->y2, cursorRange.y2 + 1);
}

static void
cfbpmaxPointerNonInterestBox( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    info->mbox.bottom = pBox->y2;
    info->mbox.top = pBox->y1;
    info->mbox.left = pBox->x1;
    info->mbox.right = pBox->x2;
}

static void
cfbpmaxQueryBestSize(class, pwidth, pheight)
    int class;
    short *pwidth;
    short *pheight;
{
    unsigned width, test;

    if (*pwidth > 0)
    {
      switch(class)
      {
        case CursorShape:
	  *pwidth = 16;
	  *pheight = 16;
	  break;
        case TileShape:
        case StippleShape:
	  width = *pwidth;
	  /* Return the closes power of two not less than what they gave me */
	  test = 0x80000000;
	  /* Find the highest 1 bit in the width given */
	  while(!(test & width))
	     test >>= 1;
	  /* If their number is greater than that, bump up to the next
	   *  power of two */
	  if((test - 1) & width)
	     test <<= 1;
	  *pwidth = test;
	  /* We don't care what height they use */
	  break;
       }
    }
}

Bool
mfbScreenClose(pScreen)
    register ScreenPtr pScreen;
{

    if (pScreen->allowedDepths)
    {
	if (pScreen->allowedDepths->vids)
		Xfree(pScreen->allowedDepths->vids);
 
	Xfree(pScreen->allowedDepths);
    }

    /*  pScreen->visuals does not need to be freed here, since it is added as
	a resource in "mfbScreenInit" and is freed with the rest of the 
	resources. */

    if(pScreen->devPrivate)
	Xfree(pScreen->devPrivate);

 /* Does pScreen->devPrivate->devPrivate have to be freed since it's a bitmap?
	but I don't know where or how it is allocated. It may be static. */

    return TRUE;
}

void
SetLockLED (on)
    Bool on;
{
    struct pm_kpcmd ioc;
    ioc.cmd = on ? LK_LED_ENABLE : LK_LED_DISABLE;
    ioc.par[0] = LED_3;
    ioc.par[1] = 0;
    ioc.nbytes = 1;
    ioctl(fdPM, QIOCKPCMD, &ioc);
}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */
void
AbortDDX()
{
}

/* Called by GiveUp(). */
void
ddxGiveUp()
{
}

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;
{
    int			argind=i;
    int			skip;
    static int		Once=0;
    void		ddxUseMsg();

    skip = 0;
    if (!Once)
    {
        blackValue = NULL;
        whiteValue = NULL;
	Once = 1;
    }

    if (strcmp( argv[argind], "-dpix") == 0)
    {
	if (++argind < argc)
	{
	    dpix = atoi(argv[argind]);
	    skip = 2;
	}
	else
	    return 0;	/* failed to parse */
    }
    else if (strcmp( argv[argind], "-dpiy") == 0)
    {
	if (++argind < argc)
	{
	    dpiy = atoi(argv[argind]);
	    skip = 2;
	}
	else
	    return 0;
    }
    else if (strcmp( argv[argind], "-dpi") == 0)
    {
	if (++argind < argc)
	{
	    dpi = atoi(argv[argind]);
	    dpix = dpi;
	    dpiy = dpi;
	    skip = 2;
	}
	else
	    return 0;
    }
    else 
    if(strcmp(argv[argind], "-bp") == 0 )
    {
	    if(++argind < argc)
	    {
		blackValue = argv[argind];
		skip = 2;
	    }
	    else
	        return 0;
    }
    else
    if(strcmp(argv[argind], "-wp") == 0 )
    {
	    if(++argind < argc)
	    {
		whiteValue = argv[argind];
		skip = 2;
	    }
	    else
	        return 0;
    }
    else
    if (strcmp(argv[argind], "-class") == 0 )
    {
	    if(++argind < argc)
	    {
		class = ParseClass(argv[argind]);
		if (class == NoSuchClass)
		    return 0;
		skip = 2;
	    }
	    else
	        return 0;
    }
    return skip;
}

void
ddxUseMsg()
{
    ErrorF ("\n");
    ErrorF ("\n");
    ErrorF ("Device Dependent Usage\n");
    ErrorF ("\n");
    ErrorF ("-dpix <n>          Dots per inch, x coordinate\n");
    ErrorF ("-dpiy <n>          Dots per inch, y coordinate\n");
    ErrorF ("-dpi <n>           Dots per inch, x and y coordinates\n");
    ErrorF ("                   (overrides -dpix and -dpiy above)\n");
    ErrorF ("-bp #XXX           color for BlackPixel for screen\n");
    ErrorF ("-wp #XXX           color for WhitePixel for screen\n");
    ErrorF ("-class <classname> type of Visual for root window\n");
    ErrorF ("       one of StaticGray, StaticColor, PseudoColor,\n");
    ErrorF ("       GrayScale, or even TrueColor!\n");
}
