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
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * $Header: /usr/sde/x11/rcs/x11/src/./server/ddx/x386/x386Io.c,v 1.2 91/12/15 12:42:16 devrcs Exp $
 */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "x386Procs.h"
#include "x386OSD.h"
#include "atKeynames.h"


/*
 * x386KbdBell --
 *	Ring the terminal/keyboard bell for an amount of time proportional to
 *      "loudness".
 */

static void
x386KbdBell(loudness, pKeyboard)
     int           loudness;         /* Percentage of full volume */
     DeviceIntPtr  pKeyboard;        /* Keyboard to ring */
{
  /*
   * KDMKTONE is not supported on SCO. To make sound we use KIOCSOUND. The
   * sound must be explicitly turned on and off. The timeout needs to have
   * millisecond precision.
   *
   * And now some notes to KIOCSOUND from the manual:
   * >  This call starts the sound generation.  It turns on
   * >  sound.  The argument is the inverse frequency desired.
   * >  A value of 0 turns off the sound.
   *
   * Interpretion: The value passed is just the reload value for the timer. An
   *               AT386 uses for the times a quarz with 1193180 Hz.
   *
   * NOTE:      ctrl.bell ist not supported.
   */

  if (loudness)
    {
      ioctl(x386Info.consoleFd, KIOCSOUND, 1193180 / x386Info.bell_pitch);
      usleep(x386Info.bell_duration * loudness * 20);
      ioctl(x386Info.consoleFd, KIOCSOUND, 0);
    }
}



/*
 * x386KbdLeds --
 *      Update the keyboards leds.
 */

void
x386KbdLeds ()
{

#ifdef KBIO_SETMODE
  /*
   * We must deal here with the LED's. But there is one problem: If
   * the keyboard is in XT-scancode-mode the LEDs can't be set.
   * Thus we switch here temporarily to AT-scancode-mode.
   *
   * TODO: This way of doing things is NOT portable between
   *       SVR3s. It is standard for SVR4. Is there a common
   *       solution ?
   */

  char leds = 0;

  if (x386Info.capsLock && !(x386Info.xleds & XLED1))
    leds |= LED_CAP;

  if (x386Info.numLock && !(x386Info.xleds & XLED2))
    leds |= LED_NUM;

  if ((x386Info.scrollLock || x386Info.modeSwitchLock) && 
      !(x386Info.xleds & XLED3))
    leds |= LED_SCR;

  if ((x386Info.leds & x386Info.xleds) & XLED1) leds |= LED_CAP;
  if ((x386Info.leds & x386Info.xleds) & XLED2) leds |= LED_NUM;
  if ((x386Info.leds & x386Info.xleds) & XLED3) leds |= LED_SCR;

  ioctl(x386Info.consoleFd, KBIO_SETMODE, KBM_AT);
  ioctl(x386Info.consoleFd, KDSETLED, leds );
  ioctl(x386Info.consoleFd, KBIO_SETMODE, KBM_XT);

#endif
}



/*
 * x386KbdCtrl --
 *      Alter some of the keyboard control parameters. All special protocol
 *      values are handled by dix (ProgChangeKeyboardControl)
 */

static void
x386KbdCtrl (pKeyboard, ctrl)
     DevicePtr     pKeyboard;        /* Keyboard to alter */
     KeybdCtrl     *ctrl;
{
  x386Info.bell_pitch    = ctrl->bell_pitch;
  x386Info.bell_duration = ctrl->bell_duration;
  x386Info.autoRepeat    = ctrl->autoRepeat;
  x386Info.leds          = (ctrl->leds & ~(XCAPS | XNUM | XSCR));

  x386KbdLeds();
}



/*
 * x386InitKBD --
 *      Reinitialize the keyboard. Only set Lockkeys accrding to ours leds.
 *      Depress all other keys.
 */

static void
x386InitKBD()
{
  char            leds,rad;
  unsigned int    i;
  xEvent          kevent;
  DevicePtr       pKeyboard = x386Info.pKeyboard;
  KeyClassRec     *keyc = ((DeviceIntPtr)x386Info.pKeyboard)->key;
  KeySym          *map = keyc->curKeySyms.map;

  kevent.u.keyButtonPointer.time = GetTimeInMillis();
  kevent.u.keyButtonPointer.rootX = 0;
  kevent.u.keyButtonPointer.rootY = 0;

  /*
   * Hmm... here is the biggest hack of every time !
   * It may be possible that a switch-vt procedure has finished BEFORE
   * you released all keys neccessary to do this. That peculiar behavior
   * can fool the X-server pretty much, cause it assumes that some keys
   * were not released. TWM may stuck alsmost completly....
   * OK, what we are doing here is after returning from the vt-switch
   * exeplicitely unrelease all keyboard keys before the input-devices
   * are reenabled.
   */
  for (i = keyc->curKeySyms.minKeyCode; i < keyc->curKeySyms.maxKeyCode; i++)
    if (KeyPressed(i))
      {
	kevent.u.u.detail = i;
	kevent.u.u.type = KeyRelease;
	(* pKeyboard->processInputProc)(&kevent, pKeyboard, 1);
      }
  
  /*
   * we must deal here with the fact, that on some cases the numlock or
   * capslock key are enabled BEFORE the server is started up. So look
   * here at the state on the according LEDS to determine whether a
   * lock-key is allready set.
   */

  x386Info.scanPrefix      = 0;
  x386Info.capsLock        = FALSE;
  x386Info.numLock         = FALSE;
  x386Info.scrollLock      = FALSE;
  x386Info.modeSwitchLock  = FALSE;

  (void) ioctl(x386Info.consoleFd, KDGETLED, &leds );

  for (i = keyc->curKeySyms.minKeyCode;
       i < keyc->curKeySyms.maxKeyCode;
       i++, map += keyc->curKeySyms.mapWidth)

    switch(*map) {

    case XK_Caps_Lock:
    case XK_Shift_Lock:
      if (leds & LED_CAP) 
	{
	  kevent.u.u.detail = i;
	  kevent.u.u.type = KeyPress;
	  (* pKeyboard->processInputProc)(&kevent, pKeyboard, 1);
	  x386Info.capsLock = TRUE;
	}
      break;

    case XK_Num_Lock:
      if (leds & LED_NUM)
	{
	  kevent.u.u.detail = i;
	  kevent.u.u.type = KeyPress;
	  (* pKeyboard->processInputProc)(&kevent, pKeyboard, 1);
	  x386Info.numLock = TRUE;
	}
      break;

    case XK_Scroll_Lock:
    case XK_Kana_Lock:
      if (leds & LED_SCR)
	{
	  kevent.u.u.detail = i;
	  kevent.u.u.type = KeyPress;
	  (* pKeyboard->processInputProc)(&kevent, pKeyboard, 1);
	  x386Info.scrollLock = TRUE;
	}
      break;
    }
  x386KbdLeds();

#ifdef KDSETRAD
  /*
   * TODO: check whether kdbRate is correctly translated ..
   */
  if      (x386Info.kbdDelay <= 375) rad = 0x00;
  else if (x386Info.kbdDelay <= 625) rad = 0x20;
  else if (x386Info.kbdDelay <= 875) rad = 0x40;
  else                               rad = 0x60;

  if      (x386Info.kbdRate <=  2)   rad |= 0x1F;
  else if (x386Info.kbdRate >= 30)   rad |= 0x00;
  else                               rad |= ((58 / x386Info.kbdRate) - 2);

  (void) ioctl(x386Info.consoleFd, KDSETRAD, rad );
#endif  
}



/*
 * x386KbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
x386KbdProc (pKeyboard, what)
     DevicePtr pKeyboard;	/* Keyboard to manipulate */
     int       what;	    	/* What to do to it */
{
  static char          *emap;
  static struct termio tty;
  static int           trans;

  KeySymsRec           keySyms;
  CARD8                modMap[MAP_LENGTH];
  struct termio        nTty;

  switch (what) {

  case DEVICE_INIT:

    /*
     * First open and find the current state of the keyboard.
     */
    ioctl (x386Info.consoleFd, KDGKBMODE, &trans);
    ioctl (x386Info.consoleFd, TCGETA, &tty); 
    
    /*
     * now handle the the line discipline 0 extended mapping
     *
     * IDEA: After we read the mapping table, we could use the compose -
     *       table for initializing the X-server's one.
     */
    emap = (char *)xalloc(E_TABSZ);
    if (ioctl(x386Info.consoleFd, LDGMAP, emap) < 0)
      {
	xfree(emap);
	emap = NULL;
      }
    
    x386KbdGetMapping(&keySyms, modMap);
    
    /*
     * Get also the initial led settings
     */
    ioctl(x386Info.consoleFd, KDGETLED, &x386Info.leds);
    
    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */
    pKeyboard->on = FALSE;


    InitKeyboardDeviceStruct(x386Info.pKeyboard,
			     &keySyms,
			     modMap,
			     x386KbdBell,
			     x386KbdCtrl);
    
    break;
    
  case DEVICE_ON:
    /*
     * Set the keyboard into "direct" mode and turn on
     * event translation.
     */

    ioctl(x386Info.consoleFd, KDSKBMODE, K_RAW);
    ioctl(x386Info.consoleFd, LDNMAP, 0); /* disable mapping completely */
    nTty = tty;
    nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    nTty.c_oflag = 0;
    nTty.c_cflag = CREAD | CS8 | B9600;
    nTty.c_lflag = 0;
    nTty.c_cc[VTIME]=0; 
    nTty.c_cc[VMIN]=1;
    ioctl(x386Info.consoleFd, TCSETA, &nTty);

    AddEnabledDevice(x386Info.consoleFd);
    pKeyboard->on = TRUE;
    x386InitKBD();
    break;
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    /*
     * Restore original keyboard directness and translation.
     */

    if (emap) ioctl(x386Info.consoleFd, LDSMAP, emap);
    ioctl(x386Info.consoleFd, KDSKBMODE, trans);
    ioctl(x386Info.consoleFd, TCSETA, &(tty));

    RemoveEnabledDevice(x386Info.consoleFd); 
    pKeyboard->on = FALSE;
    break;
    
  }

  return (Success);
}



/*
 * x386KbdEvents --
 *      Read the new events from the device, and pass them to the eventhandler.
 */

void
x386KbdEvents()
{
  unchar rBuf[64];
  int    nBytes,i;

  if ((nBytes = read( x386Info.consoleFd, (char *)rBuf, sizeof(rBuf))) >= 1)
    
    for (i = 0; i < nBytes; i++) x386PostKbdEvent(rBuf[i]);
}



/*
 * x386MseCtrl --
 *      Alter the control parameters for the mouse. Note that all special
 *      protocol values are handled by dix.
 */

static void
x386MseCtrl(pPointer, ctrl)
     DevicePtr pPointer;
     PtrCtrl   *ctrl;
{
  x386Info.num       = ctrl->num;
  x386Info.den       = ctrl->num;
  x386Info.threshold = ctrl->threshold;
}



/*
 * GetMotionEvents --
 *      Return the (number of) motion events in the "motion history
 *      buffer" (snicker) between the given times.
 */

int
GetMotionEvents (buff, start, stop, pScreen)
     CARD32 start, stop;
     xTimecoord *buff;
     ScreenPtr pScreen;
{
  return 0;
}


static void
x386SetSpeed(old, new, cflag)
     int old, new;
     unsigned short cflag;
{
  struct termio  tty;
  char           *c;

  (void) ioctl(x386Info.mseFd, TCGETA, &tty);
  tty.c_iflag = IGNBRK | IGNPAR ;     
  tty.c_oflag = 0;           
  tty.c_lflag = 0;
  tty.c_line = 0;
  tty.c_cc[VTIME]=0; 
  tty.c_cc[VMIN]=1;
  
  switch (old) {
  case 9600:  tty.c_cflag = cflag | B9600;  break;
  case 4800:  tty.c_cflag = cflag | B4800;  break;
  case 2400:  tty.c_cflag = cflag | B2400;  break;
  case 1200:
  default:    tty.c_cflag = cflag | B1200;  break;
  }
  (void) ioctl(x386Info.mseFd, TCSETAW, &tty);  
  
  switch(new) {
  case 9600:  c = "*q";  tty.c_cflag = cflag | B9600;  break;
  case 4800:  c = "*p";  tty.c_cflag = cflag | B4800;  break;
  case 2400:  c = "*o";  tty.c_cflag = cflag | B2400;  break;
  case 1200:
  default:    c = "*n";  tty.c_cflag = cflag | B1200;  break;
  }

  write(x386Info.mseFd, c, 2);  
  usleep(100000);

  (void) ioctl(x386Info.mseFd, TCSETAW, &tty);  
#ifdef TCMOUSE
  (void) ioctl(x386Info.mseFd, TCMOUSE, 1);  
#endif
}


/*
 * x386MseProc --
 *      Handle the initialization, etc. of a mouse
 */

int
x386MseProc(pPointer, what)
     DevicePtr	pPointer;
     int        what;
{
  unchar                map[4];

  static unsigned short cflag[5] =
    {
      (CS7                   | CREAD | CLOCAL | HUPCL ),   /* MicroSoft */
      (CS8 | CSTOPB          | CREAD | CLOCAL | HUPCL ),   /* MouseSystems */
      (CS8 | PARENB | PARODD | CREAD | CLOCAL | HUPCL ),   /* MMSeries */
      (CS8 | CSTOPB          | CREAD | CLOCAL | HUPCL ),   /* Logitech */
      0,                                                   /* BusMouse */
    };
  
  switch (what)
    {
    case DEVICE_INIT: 
      pPointer->on = FALSE;
 
      map[1] = 3;
      map[2] = 2;
      map[3] = 1;
      InitPointerDeviceStruct(pPointer, 
			      map, 
			      3, 
			      GetMotionEvents, 
			      x386MseCtrl, 
			      0);

      break;
      
    case DEVICE_ON:
      if ((x386Info.mseFd = open(x386Info.mseDevice, O_RDWR | O_NDELAY)) < 0)
	{
	  Error ("Cannot open mouse");
	  return (!Success);
	}

      if (x386Info.mseType != P_BM) 
	{
	  x386SetSpeed(9600, x386Info.baudRate, cflag[x386Info.mseType]);
	  x386SetSpeed(4800, x386Info.baudRate, cflag[x386Info.mseType]);
	  x386SetSpeed(2400, x386Info.baudRate, cflag[x386Info.mseType]);
	  x386SetSpeed(1200, x386Info.baudRate, cflag[x386Info.mseType]);

	  if (x386Info.mseType == P_LOGI)
	    {
	      write(x386Info.mseFd, "S", 1);
	      x386SetSpeed(x386Info.baudRate, x386Info.baudRate, cflag[P_MM]);
	    }

	  if      (x386Info.sampleRate <=   0)  write(x386Info.mseFd, "O", 1);
	  else if (x386Info.sampleRate <=  15)  write(x386Info.mseFd, "J", 1);
	  else if (x386Info.sampleRate <=  27)  write(x386Info.mseFd, "K", 1);
	  else if (x386Info.sampleRate <=  42)  write(x386Info.mseFd, "L", 1);
	  else if (x386Info.sampleRate <=  60)  write(x386Info.mseFd, "R", 1);
	  else if (x386Info.sampleRate <=  85)  write(x386Info.mseFd, "M", 1);
	  else if (x386Info.sampleRate <= 125)  write(x386Info.mseFd, "Q", 1);
	  else                                  write(x386Info.mseFd, "N", 1);
	}
      
      AddEnabledDevice(x386Info.mseFd);

      x386Info.lastButtons = 0;
      x386Info.emulateState = 0;
      pPointer->on = TRUE;
      break;
      
    case DEVICE_OFF:
    case DEVICE_CLOSE:
      if (x386Info.mseType == P_LOGI)
	{
	  write(x386Info.mseFd, "U", 1);
	  x386SetSpeed(x386Info.baudRate, 1200, cflag[P_LOGI]);
	}
      close(x386Info.mseFd);
      RemoveEnabledDevice(x386Info.mseFd);
      pPointer->on = FALSE;
      break;
      
    }

  return Success;
}



/*
 * x386MseEvents --
 *      Read the new events from the device, and pass them to the eventhandler.
 */

void
x386MseEvents()
{
  unchar               rBuf[64];
  int                  i,nBytes, buttons, dx, dy;
  static int           pBufP = 0;
  static unsigned char pBuf[8];

  static unsigned char proto[5][5] = {
    /*  hd_mask hd_id   dp_mask dp_id   nobytes */
    { 	0x40,	0x40,	0x40,	0x00,	3 	},  /* MicroSoft */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* MouseSystems */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* MMSeries */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* Logitech */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* BusMouse */
  };
  
  if (!(nBytes = read(x386Info.mseFd, (char *)rBuf, sizeof(rBuf)))) return;

  for ( i=0; i < nBytes; i++) {
    /*
     * Hack for resyncing: We check here for a package that is:
     *  a) illegal (detected by wrong data-package header)
     *  b) invalid (0x80 == -128 and that might be wrong for MouseSystems)
     *  c) bad header-package
     *
     * NOTE: b) is a voilation of the MouseSystems-Protocol, since values of
     *       -128 are allowed, but since they are very seldom we can easily
     *       use them as package-header with no button pressed.
     */
    if (pBufP != 0 && 
	((rBuf[i] & proto[x386Info.mseType][2]) != proto[x386Info.mseType][3]
	 || rBuf[i] == 0x80))
      {
	pBufP = 0;          /* skip package */
      }

    if (pBufP == 0 &&
	(rBuf[i] & proto[x386Info.mseType][0]) != proto[x386Info.mseType][1])
      {
	/*
	 * Hack for Logitech MouseMan Mouse - Middle button
	 *
	 * Unfortunately this mouse has variable length packets: the standard
	 * Microsoft 3 byte packet plus an optional 4th byte whenever the
	 * middle button status changes.
	 *
	 * We have already processed the standard packet with the movement
	 * and button info.  Now post an event message with the old status
	 * of the left and right buttons and the updated middle button.
	 */
	if (x386Info.mseType == P_MS && (rBuf[i] == 0x20 || rBuf[i] == 0))
	  {
	    buttons = ((int)(rBuf[i] & 0x20) >> 4)
	      | (x386Info.lastButtons & 0x05);
	    x386PostMseEvent(buttons, 0, 0);
	  }

	continue;            /* skip package */
      }


    pBuf[pBufP++] = rBuf[i];
    if (pBufP != proto[x386Info.mseType][4]) continue;

    /*
     * assembly full package
     */
    switch(x386Info.mseType) {
      
    case P_MS:              /* Mircosoft */
      buttons = (x386Info.lastButtons & 2)
	| ((int)(pBuf[0] & 0x20) >> 3)
	  | ((int)(pBuf[0] & 0x10) >> 4);
      dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
      dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
      break;
      
    case P_MSC:             /* Mouse Systems Corp */
      buttons = (~pBuf[0]) & 0x07;
      dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
      dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
      break;
      
    case P_MM:              /* MM Series */
    case P_LOGI:            /* Logitech Mice */
      buttons = pBuf[0] & 0x07;
      dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
      dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
      break;
      
    case P_BM:              /* BusMouse */
      buttons = (~pBuf[0]) & 0x07;
      dx =   (char)pBuf[1];
      dy = - (char)pBuf[2];
      break;
    }

    x386PostMseEvent(buttons, dx, dy);
    pBufP = 0;
  }
}



#ifdef XQUEUE

static xqEventQueue      *XqueQaddr;

/*
 * x386XqueRequest --
 *      Notice an i/o request from the xqueue.
 */

static void
x386XqueRequest()
{
  xqEvent  *XqueEvents = XqueQaddr->xq_events;
  int      XqueHead = XqueQaddr->xq_head;

  while (XqueHead != XqueQaddr->xq_tail)
    {

      switch(XqueEvents[XqueHead].xq_type) {
	
      case XQ_BUTTON:
	x386PostMseEvent(~(XqueEvents[XqueHead].xq_code) & 0x07, 0, 0);
	break;

      case XQ_MOTION:
	x386PostMseEvent(~(XqueEvents[XqueHead].xq_code) & 0x07,
		       XqueEvents[XqueHead].xq_x,
		       XqueEvents[XqueHead].xq_y);
	break;

      case XQ_KEY:
	x386PostKbdEvent(XqueEvents[XqueHead].xq_code);
	break;
	
      default:
	ErrorF("Unknown Xque Event: 0x%02x\n", XqueEvents[XqueHead].xq_type);
      }
      
      if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
    }

  /* reenable the signal-processing */
  x386Info.inputPending = TRUE;
  signal(SIGUSR2, (void (*)()) x386XqueRequest);
  XqueQaddr->xq_head = XqueQaddr->xq_tail;
  XqueQaddr->xq_sigenable = 1; /* UNLOCK */
}



/*
 * x386XqueEnable --
 *      Enable the handling of the Xque
 */

static int
x386XqueEnable()
{
  static struct kd_quemode xqueMode;
  static Bool              was_here = FALSE;

  if (!was_here) {
    if ((x386Info.xqueFd = open("/dev/mouse", O_RDONLY|O_NDELAY)) < 0)
      {
	Error ("Cannot open /dev/mouse");
	return (!Success);
      }
    was_here = TRUE;
  }

  if (x386Info.xqueSema++ == 0) 
    {
      (void) signal(SIGUSR2, (void (*)()) x386XqueRequest);
      xqueMode.qsize = 64;    /* max events */
      xqueMode.signo = SIGUSR2;
      ioctl(x386Info.consoleFd, KDQUEMODE, NULL);
      
      if (ioctl(x386Info.consoleFd, KDQUEMODE, &xqueMode) < 0) {
	Error ("Cannot set KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
      
      XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
      XqueQaddr->xq_sigenable = 1; /* UNLOCK */
    }

  return(Success);
}



/*
 * x386XqueDisable --
 *      disable the handling of the Xque
 */

static int
x386XqueDisable()
{
  if (x386Info.xqueSema-- == 1)
    {
      
      XqueQaddr->xq_sigenable = 0; /* LOCK */
      
      if (ioctl(x386Info.consoleFd, KDQUEMODE, NULL) < 0) {
	Error ("Cannot unset KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
    }

  return(Success);
}



/*
 * x386XqueMseProc --
 *      Handle the initialization, etc. of a mouse
 */

int
x386XqueMseProc(pPointer, what)
     DevicePtr	pPointer;
     int        what;
{
  unchar        map[4];

  switch (what)
    {
    case DEVICE_INIT: 
      
      pPointer->on = FALSE;
      
      map[1] = 3;
      map[2] = 2;
      map[3] = 1;
      InitPointerDeviceStruct(pPointer, 
			      map, 
			      3, 
			      GetMotionEvents, 
			      x386MseCtrl, 
			      0);
      break;
      
    case DEVICE_ON:
      x386Info.lastButtons = 0;
      x386Info.emulateState = 0;
      pPointer->on = TRUE;
      return(x386XqueEnable());
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
      pPointer->on = FALSE;
      return(x386XqueDisable());
    }
  
  return Success;
}



/*
 * x386XqueKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
x386XqueKbdProc (pKeyboard, what)
     DevicePtr pKeyboard;	/* Keyboard to manipulate */
     int       what;	    	/* What to do to it */
{
  KeySymsRec  keySyms;
  CARD8       modMap[MAP_LENGTH];

  switch (what) {
      
  case DEVICE_INIT:
    
    x386KbdGetMapping(&keySyms, modMap);
    
    /*
     * Get also the initial led settings
     */
    ioctl(x386Info.consoleFd, KDGETLED, &x386Info.leds);
    
    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */
    pKeyboard->on = FALSE;
    
    InitKeyboardDeviceStruct(x386Info.pKeyboard,
			     &keySyms,
			     modMap,
			     x386KbdBell,
			     x386KbdCtrl);
    
    break;
    
  case DEVICE_ON:
    pKeyboard->on = TRUE;
    x386InitKBD();
    return(x386XqueEnable());
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    pKeyboard->on = FALSE;
    return(x386XqueDisable());
  }
  
  return (Success);
}


/*
 * x386XqueEvents --
 *      Get some events from our queue. Nothing to do here ...
 */

void
x386XqueEvents()
{
}

#endif /* XQUEUE */



