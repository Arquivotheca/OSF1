/* X Communication module for terminals which understand the X protocol.
   Copyright (C) 1988, 1990 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Yakim Martillo, mods and things by Robert Krawitz  */
/* Redone for X11 by Robert French */
/* Thanks to Mark Biggers for all of the Window Manager support */

/*
 *	$Source: /mit/emacs/src/RCS/11xterm.c,v $
 *	$Author: rfrench $
 *	$Locker:  $
 *	$Header: x11term.c,v 1.12 88/02/29 14:11:07 rfrench Exp $
 */

#ifndef lint
static char *rcsid_xterm_c = "$Header: x11term.c,v 1.12 88/02/29 14:11:07 rfrench Exp $";
#endif	lint

/* On 4.3 this loses if it comes after x11term.h.
   On hp-ux it loses if it comes after config.h.  */
#include <signal.h>
#include <sys/ioctl.h>

/* Load sys/types.h if not already loaded.
   In some systems loading it twice is suicidal.  */
#ifndef makedev
#include <sys/types.h>
#endif

#include "config.h"

#ifdef HAVE_X_WINDOWS

/* Get FIONREAD, if it is available.
   It would be logical to include <sys/ioctl.h> here,
   but it was moved up above to avoid problems.  */
#ifdef USG
#include <termio.h>
#endif /* USG */
#include <fcntl.h>

#include "lisp.h"
#undef NULL

/* Allow m- file to inhibit use of interrupt-driven input.  */
#ifdef BROKEN_FIONREAD
#undef FIONREAD
#endif

/* We are unable to use interrupts if FIONREAD is not available,
   so flush SIGIO so we won't try.  */
#ifndef FIONREAD
#ifdef SIGIO
#undef SIGIO
#endif
#endif

/* This may include sys/types.h, and that somehow loses
   if this is not done before the other system files.
   However, perhaps the problem has been avoided by loading types.h above.  */

#include "x11term.h"

#include "X11/Xresource.h"

/* Allow the config file to specify whether we can assume X11R4.  */
#ifdef SPECIFY_X11R4
#if SPECIFY_X11R4 > 0
#define X11R4
#endif
#else /* not SPECIFY_X11R4 */
/* Try to guess whether this is release 4 or newer.  */
#ifdef PBaseSize
#define X11R4
#endif
#endif /* not SPECIFY_X11R4 */

#ifdef HAVE_SOCKETS
#include <sys/socket.h>		/* Must be done before gettime.h.  */
#endif
/* Include time.h, sys/time.h, or both.  */
#include "gettime.h"

#ifdef AIX
static KeySym XMOD_Alt[] = { XK_Alt_L };
static KeySym XMOD_Shift[] = { XK_Shift_L };
static KeySym XMOD_ShiftAlt[] = { XK_Alt_L, XK_Shift_L };
static KeySym XMOD_CtrlAlt[] = { XK_Control_L, XK_Alt_L };
static KeySym XMOD_Ctrl[] = { XK_Control_L };
static KeySym XMOD_CtrlShift[] = { XK_Control_L, XK_Shift_L };
static KeySym XMOD_ShiftCtrlAlt[] = { XK_Control_L, XK_Alt_L, XK_Shift_L };
#endif

#if 0 /* On some machines, stdio.h doesn't define NULL
	 if stddef.h has been included already!  */
#ifdef NULL  /* Sometimes various definitions conflict here.  */
#undef NULL
#endif
#endif
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#ifdef BSD
#include <strings.h>
#endif
#include <sys/stat.h>

#ifndef NULL
#define NULL 0L
#endif

#include "dispextern.h"
#include "termhooks.h"
#include "termopts.h"
#include "termchar.h"

#include "sink11.h"
#include "sink11mask.h"
#include "proto.h"

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))

extern int errno;

#define sigunblockx(sig) sigblock (SIGEMPTYMASK)
#define sigblockx(sig) sigblock (sigmask ((sig)))

#define METABIT 0200
#define MINWIDTH 12	/* In pixels */
#define MINHEIGHT 5	/* In pixels */
#define MAXHEIGHT 300	/* In lines */

int pixelwidth,pixelheight;
char *progname;

XEvent *XXm_queue[XMOUSEBUFSIZE];
int XXm_queue_num, XXm_queue_in, XXm_queue_out;

char *XXcurrentfont;
XFontStruct *fontinfo;
Font XXfid;
int XXfontw, XXfonth, XXbase, XXisColor;

/* Nonzero means Emacs has explicit keyboard focus.  */
int x_focus_flag;

Colormap XXColorMap;
char *default_window;
extern int initialized;
extern int screen_width, screen_height;
extern int delayed_size_change;

/* Function for init_keyboard to call with no args (if nonzero).  */
extern void (*keyboard_init_hook) ();

extern char *alternate_display;
extern int xargc;
extern char **xargv;

int XXdebug;
int XXpid;

int WindowMapped;

char  *XXidentity;	/* Resource name of this invocation of Emacs */
static char  *XXicon_name;	/* user-supplied icon info */
static int   XXicon_usebitmap;	/* Use bitmap or not */
static char  *XXheader;		/* user-supplied window header info */

static int flexlines;  		/* last line affected by dellines or
				 * inslines functions */
int VisibleX, VisibleY;		/* genuine location of cursor on screen
				 * if it is there */

/* Last cursor position specified by move_cursor.
   During an update, this does not display a cursor on the screen;
   But it controls the position that is output.  */
static int local_cursor_hpos;
static int local_cursor_vpos;

static int SavedX, SavedY;	/* Where the cursor was before update
				 * started */


int CursorExists;		/* during updates cursor is turned off */
int CursorOutline; 	    	/* when the pointer is not in the Emacs
    	    	    	    	 * widow the cursor should be drawn in
    	    	    	         * outline form a la xterm */
static int InUpdate;		/* many of functions here may be invoked
				 * even if no update in progress; when
				 * no update is in progress the action
				 * can be slightly different */

Display *XXdisplay;
int XXscreen;
Window XXwindow;
GC XXgc_norm,XXgc_rev,XXgc_curs,XXgc_temp,XXgc_curs_rev;
XGCValues XXgcv;
Cursor EmacsCursor;
Pixmap SinkPixmap, SinkMaskPixmap;

static XrmDatabase db, db2;

char *fore_color;	/* Variables to store color names */
char *back_color;
char *brdr_color;
char *curs_color;
char *mous_color;

unsigned long fore;	/* Variables to store pixel values */
unsigned long back;
unsigned long brdr;
unsigned long curs;

char *desiredwindow;

int CurHL;			/* Current Highlighting (ala mode line) */

int XXborder;			/* Window border width */
int XXInternalBorder;		/* Internal border width */
int updated[MAXHEIGHT];

static char  *temp_font;                /* needed because of loading hacks */
static char  *temp_reverseVideo;
static char  *temp_borderWidth;
static char  *temp_internalBorder;
static char  *temp_useBitmap;

struct _xdeftab 
{
  char *iname;			/* instance name */
  char *cname;			/* class name (fake it) */
  char **varp;			/* variable to set */
};


static struct _xdeftab xDefaultsValueTable[]
 = {
     { "reverseVideo",	"ReverseVideo",		&temp_reverseVideo },
     { "borderWidth",	"BorderWidth",		&temp_borderWidth },
     { "internalBorder","BorderWidth",		&temp_internalBorder },
     { "bitmapIcon",	"BitmapIcon",		&temp_useBitmap },
     { "borderColor",	"BorderColor",		&brdr_color },
     { "background",	"Background",		&back_color },
     { "foreground",	"Foreground",		&fore_color },
     { "pointerColor",	"Foreground",		&mous_color },
     { "cursorColor",	"Foreground",		&curs_color },
     { "font",		"Font",			&temp_font },
     { "geometry",	"Geometry",		&desiredwindow },
     { "title",		"Title",		&XXheader },
     { "iconName",	"Title",		&XXicon_name },
     { NULL,		NULL,			NULL }
   };

#ifdef _NO_EMACS_PROTO
int (*handler)();
#else /* _NO_EMACS_PROTO */
int (*handler)();
#endif /* _NO_EMACS_PROTO */

extern char *malloc();

/* HLmode -- Changes the GX function for output strings.  Could be used to
 * change font.  Check an XText library function call.
 */
#ifdef _NO_EMACS_PROTO
HLmode (new)
     int new;
#else /* _NO_EMACS_PROTO */
void
HLmode ( int new )
#endif /* _NO_EMACS_PROTO */
{
	extern int inverse_video;
	
	CurHL = new;
}

/* External interface to control of standout mode.
   Call this when about to modify line at position VPOS
   and not change whether it is highlighted.  */

#ifdef _NO_EMACS_PROTO
XTreassert_line_highlight (highlight, vpos)
     int highlight, vpos;
#else /* _NO_EMACS_PROTO */
void
XTreassert_line_highlight ( int highlight, int vpos )
#endif /* _NO_EMACS_PROTO */
{
	HLmode (highlight);
}

/* Call this when about to modify line at position VPOS
   and change whether it is highlighted.  */

#ifdef _NO_EMACS_PROTO
XTchange_line_highlight (new_highlight, vpos, first_unused_hpos)
     int new_highlight, vpos, first_unused_hpos;
#else /* _NO_EMACS_PROTO */
void
XTchange_line_highlight (int new_highlight, int vpos, int first_unused_hpos)
#endif /* _NO_EMACS_PROTO */
{
	HLmode (new_highlight);
	XTmove_cursor (vpos, 0);
	x_clear_end_of_line (0);
}


/* Used for starting or restarting (after suspension) the X window.  Puts the
 * cursor in a known place, update does not begin with this routine but only
 * with a call to redisplay.
 */

void
XTset_terminal_modes ()
{
	int stuffpending;
#ifdef XDEBUG
	fprintf (stderr, "XTset_terminal_modes\n");
#endif

	InUpdate = 0;
	stuffpending = 0;
	if (!initialized) {
		CursorExists = 0;
		CursorOutline = 1;
		VisibleX = 0;
		VisibleY = 0;
	}
	XTclear_screen ();
}

/* XTmove_cursor moves the cursor to the correct location and checks whether an
 * update is in progress in order to toggle it on.
 */

#ifdef _NO_EMACS_PROTO
XTmove_cursor (row, col)
     register int row, col;
#else /* _NO_EMACS_PROTO */
void
XTmove_cursor ( int row, int col )
#endif /* _NO_EMACS_PROTO */
{
	BLOCK_INPUT_DECLARE ();
#ifdef XDEBUG
	fprintf (stderr, "XTmove_cursor (X %d, Y %d)\n",col,row);
#endif

	BLOCK_INPUT ();
	local_cursor_hpos = col;
	local_cursor_vpos = row;

	if (InUpdate) {
		if (CursorExists)
			CursorToggle ();
		UNBLOCK_INPUT ();
		return;
		/* Generally, XTmove_cursor will be invoked */
		/* when InUpdate with !CursorExists */
		/* so that wasteful XFlush is not called */
	}
	if ((row == VisibleY) && (col == VisibleX)) {
		if (!CursorExists)
			CursorToggle ();
		XFlush (XXdisplay);
		UNBLOCK_INPUT ();
		return;
	}
	if (CursorExists)
		CursorToggle ();
	VisibleX = col;
	VisibleY = row;
	if (!CursorExists)
		CursorToggle ();
	XFlush (XXdisplay);
	UNBLOCK_INPUT ();
}

/* Used to get the terminal back to a known state after resets.  Usually
 * used when restarting suspended or waiting emacs
 */

void
cleanup ()
{
	inverse_video = 0;
	HLmode (0);
}

/* Erase current line from current column to column END.
   Leave cursor at END.  */

#ifdef _NO_EMACS_PROTO
XTclear_end_of_line (end)
     register int end;
#else /* _NO_EMACS_PROTO */
void
XTclear_end_of_line ( register int end )
#endif /* _NO_EMACS_PROTO */
{
  register int numcols;
  BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
  fprintf (stderr, "XTclear_end_of_line (to %d)\n",end);
#endif

  if (local_cursor_vpos < 0 || local_cursor_vpos >= screen_height)
    return;

  if (end <= local_cursor_hpos)
    return;
  if (end >= screen_width)
    end = screen_width;

  numcols = end - local_cursor_hpos;
  BLOCK_INPUT ();
  if (local_cursor_vpos == VisibleY && VisibleX >= local_cursor_hpos && VisibleX < end)
    if (CursorExists) CursorToggle ();
  if (CurHL)
    XFillRectangle (XXdisplay, XXwindow, XXgc_norm,
		    local_cursor_hpos*XXfontw+XXInternalBorder,
		    local_cursor_vpos*XXfonth+XXInternalBorder,
		    XXfontw*numcols,
		    XXfonth);
  else
    XClearArea (XXdisplay, XXwindow,
		local_cursor_hpos*XXfontw+XXInternalBorder,
		local_cursor_vpos*XXfonth+XXInternalBorder,
		XXfontw*numcols,
		XXfonth,
		0);
  XTmove_cursor (local_cursor_vpos, end);
  UNBLOCK_INPUT ();
}

/* Erase current line from column START to right margin.
   Leave cursor at START.  */

#ifdef _NO_EMACS_PROTO
x_clear_end_of_line (start)
     register int start;
#else /* _NO_EMACS_PROTO */
void 
x_clear_end_of_line ( register int start )
#endif /* _NO_EMACS_PROTO */
{
  register int numcols;
  BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
  fprintf (stderr, "x_clear_end_of_line (start %d)\n", start);
#endif

  if (local_cursor_vpos < 0 || local_cursor_vpos >= screen_height)
    return;

  if (start >= screen_width)
    return;
  if (start < 0)
    start = 0;

  numcols = screen_width - start;
  BLOCK_INPUT ();
  if (local_cursor_vpos == VisibleY && VisibleX >= start)
    if (CursorExists) CursorToggle ();
  if (CurHL)
    XFillRectangle (XXdisplay, XXwindow, XXgc_norm,
		    start*XXfontw+XXInternalBorder,
		    local_cursor_vpos*XXfonth+XXInternalBorder,
		    XXfontw*numcols,
		    XXfonth);
  else
    XClearArea (XXdisplay, XXwindow,
		start*XXfontw+XXInternalBorder,
		local_cursor_vpos*XXfonth+XXInternalBorder,
		XXfontw*numcols,
		XXfonth,
		0);
  XTmove_cursor (local_cursor_vpos, start);
  UNBLOCK_INPUT ();
}

void
XTreset_terminal_modes ()
{
#ifdef XDEBUG
	fprintf (stderr, "XTreset_terminal_modes\n");
#endif

	XTclear_screen ();
}

void
XTclear_screen ()
{
	BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
	fprintf (stderr, "XTclear_screen\n");
#endif

	BLOCK_INPUT ();
	HLmode (0);
	CursorExists = 0;

	local_cursor_hpos = 0;
	local_cursor_vpos = 0;
	SavedX = 0;
	SavedY = 0;
	VisibleX = 0;
	VisibleY = 0;
	XClearWindow(XXdisplay, XXwindow);
	CursorToggle ();
	if (!InUpdate)
		XFlush (XXdisplay);
	UNBLOCK_INPUT ();
}

/* used by dumprectangle which is usually invoked upon Expose
 * events which come from bit blt's or moving an obscuring opaque window
 */


#ifdef _NO_EMACS_PROTO
dumpchars (active_screen, numcols, tempX, tempY, tempHL)
     register struct matrix *active_screen;
     register int numcols;
     register int tempX, tempY, tempHL;
#else /* _NO_EMACS_PROTO */
void
dumpchars (
     register struct matrix *active_screen,
     register int numcols,
     register int tempX, int tempY, int tempHL )
#endif /* _NO_EMACS_PROTO */
{
	if (numcols <= 0)
		return;

	if (numcols-1+tempX > screen_width)
		numcols = screen_width-tempX+1;

	if (tempX < 0 || tempX >= screen_width ||
	    tempY < 0 || tempY >= screen_height)
		return;

	XDrawImageString(XXdisplay, XXwindow, tempHL ? XXgc_rev : XXgc_norm,
			 tempX*XXfontw+XXInternalBorder,
			 tempY*XXfonth+XXInternalBorder+XXbase,
			 (char *) &active_screen->contents[tempY][tempX],
			 numcols);
}

/* When a line has been changed this function is called.  Due to various
 * bits of braindamage on the parts of both X11 and Emacs, the new
 * version of the line is simply output if this function is invoked while
 * in UpDate.  Sometimes writechars can be invoked when not in update if
 * text is to be output at the end of the line.  In this case the whole
 * line is not output.  Simply the new text at the current cursor
 * position given by VisibleX,Y.  The cursor is moved to the end of the
 * new text.
 */

#ifdef _NO_EMACS_PROTO
updateline (first)
	int first;
#else /* _NO_EMACS_PROTO */
void
updateline ( int first )
#endif /* _NO_EMACS_PROTO */
{
	register int temp_length;
	BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
	fprintf(stderr, "updateline\n");
#endif /* XDEBUG */

	BLOCK_INPUT ();
	if ((local_cursor_vpos < 0) || (local_cursor_vpos >= screen_height)
	    || updated[local_cursor_vpos]) {
		UNBLOCK_INPUT ();
		return;
	}
	if (!first)
		updated[local_cursor_vpos] = 1;
	if (CursorExists)
		CursorToggle ();
	if (new_screen->enable[local_cursor_vpos])
		temp_length = new_screen->used[local_cursor_vpos]-first;
	else
		temp_length = 0;
	if (temp_length > 0) {
		XDrawImageString (XXdisplay, XXwindow,
				  CurHL ? XXgc_rev : XXgc_norm,
				  first*XXfontw+XXInternalBorder,
				  local_cursor_vpos*XXfonth+XXInternalBorder+XXbase,
				  (char *) &new_screen->contents[local_cursor_vpos][first],
				  temp_length);
		if (temp_length < screen_width)
			x_clear_end_of_line (temp_length);
		XTmove_cursor (local_cursor_vpos, temp_length);
	}
	else {
		x_clear_end_of_line (0);
		XTmove_cursor (local_cursor_vpos, 0);
	}
	UNBLOCK_INPUT ();
}

#ifdef _NO_EMACS_PROTO
writechars (start, end)
	register char *start, *end;
#else /* _NO_EMACS_PROTO */
void 
writechars ( register char *start, register char *end )
#endif /* _NO_EMACS_PROTO */
{
  BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
  fprintf(stderr, "writechars (local_cursor_hpos %d temp_len %d InUpd %d)\n",
	  local_cursor_hpos, end-start+1, InUpdate);
#endif /* XDEBUG */

  BLOCK_INPUT ();

  if ((local_cursor_vpos < 0) || (local_cursor_vpos >= screen_height))
    {
      UNBLOCK_INPUT ();
      return;
    }

  if (CursorExists)
    CursorToggle ();


  if (InUpdate)
    {
      XDrawImageString (XXdisplay, XXwindow,
			CurHL ? XXgc_rev : XXgc_norm,
			local_cursor_hpos*XXfontw+XXInternalBorder,
			local_cursor_vpos*XXfonth+XXInternalBorder+XXbase,
			start,
			(end - start) + 1);
      XTmove_cursor (local_cursor_vpos, (end - start) + 1);

      UNBLOCK_INPUT ();
      return;
    }

  if ((VisibleX < 0) || (VisibleX >= screen_width)) {
    UNBLOCK_INPUT ();
    return;
  }
  if ((VisibleY < 0) || (VisibleY >= screen_height)) {
    UNBLOCK_INPUT ();
    return;
  }
  if (((end - start) + VisibleX) >= screen_width)
    end = start + (screen_width - (VisibleX + 1));
  if (end >= start) {
    XDrawImageString (XXdisplay, XXwindow,
		      CurHL ? XXgc_rev : XXgc_norm,
		      (VisibleX * XXfontw+XXInternalBorder),
		      VisibleY * XXfonth+XXInternalBorder+XXbase,
		      start,
		      ((end - start) + 1));
    VisibleX = VisibleX + (end - start) + 1;
  }
  if (!CursorExists)
    CursorToggle ();
  UNBLOCK_INPUT ();
}

#ifdef _NO_EMACS_PROTO
static
XToutput_chars (start, len)
     register char *start;
     register int len;
#else /* _NO_EMACS_PROTO */
static void
XToutput_chars (
     register char *start,
     register int len )
#endif /* _NO_EMACS_PROTO */
{
#ifdef XDEBUG
	fprintf (stderr, "XToutput_chars (len %d)\n",len);
#endif

	writechars (start, start+len-1);
}

void
XTflash ()
{
#ifdef HAVE_TIMEVAL
#ifdef HAVE_SELECT
	XGCValues gcv_temp;
	struct timeval wakeup, now;
	BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
	fprintf (stderr, "XTflash\n");
#endif

	BLOCK_INPUT ();
	XXgc_temp = XCreateGC(XXdisplay, XXwindow, 0, &gcv_temp);
	XSetState(XXdisplay, XXgc_temp, WhitePixel (XXdisplay, XXscreen),
		  BlackPixel(XXdisplay, XXscreen), GXinvert,
		  AllPlanes);

	/* For speed, flash just 1/4 of the window's area, a rectangle in
	   the center.  */
	XFillRectangle (XXdisplay, XXwindow, XXgc_temp,
			screen_width*XXfontw/4, screen_height*XXfonth/4,
			screen_width*XXfontw/2, screen_height*XXfonth/2);
	XFlush (XXdisplay);

	UNBLOCK_INPUT ();

	gettimeofday (&wakeup, (struct timezone *)0);

	/* Compute time to wait until, propagating carry from usecs.  */
	wakeup.tv_usec += 150000;
	wakeup.tv_sec += (wakeup.tv_usec / 1000000);
	wakeup.tv_usec %= 1000000;

	/* Keep waiting until past the time wakeup.  */
	while (1)
	  {
	    struct timeval timeout;

	    gettimeofday (&timeout, (struct timezone *)0);

	    /* In effect, timeout = wakeup - timeout.
	       Break if result would be negative.  */
	    if (timeval_subtract (&timeout, wakeup, timeout))
	      break;

	    /* Try to wait that long--but we might wake up sooner.  */
	    select (0, 0, 0, 0, &timeout);
	  }
	
	BLOCK_INPUT ();

	XFillRectangle (XXdisplay, XXwindow, XXgc_temp,
			screen_width*XXfontw/4, screen_height*XXfonth/4,
			screen_width*XXfontw/2, screen_height*XXfonth/2);

	XFreeGC(XXdisplay, XXgc_temp);
	XFlush (XXdisplay);

	UNBLOCK_INPUT ();
#endif
#endif
}	

void
XTfeep ()
{
	BLOCK_INPUT_DECLARE ();
#ifdef XDEBUG
	fprintf (stderr, "XTfeep\n");
#endif
	BLOCK_INPUT ();
	XBell (XXdisplay, 0);
	XFlush (XXdisplay);
	UNBLOCK_INPUT ();
}

/* Artificially creating a cursor is hard, the actual position on the
 * screen (either where it is or last was) is tracked with VisibleX,Y.
 * Gnu Emacs code tends to assume a cursor exists in hardward at local_cursor_hpos,Y
 * and that output text will appear there.  During updates, the cursor is
 * supposed to be blinked out and will only reappear after the update
 * finishes.
 */

int
CursorToggle ()
{
	register struct matrix *active_screen;

	if (!WindowMapped) {
		CursorExists = 0;
		CursorOutline = 1;
		return 0;
		/* Currently the return values are not */
		/* used, but I could anticipate using */
		/* them in the future. */
	}
	
	if (VisibleX < 0 || VisibleX >= screen_width ||
	    VisibleY < 0 || VisibleY >= screen_height) {
		/* Not much can be done */
		XFlush (XXdisplay);
		CursorExists = 0;
		return 0;
	}

	active_screen = current_screen;

	if (active_screen->highlight[VisibleY])
	  /* If the cursor is in the modeline, it means display was preempted.
	     Don't actually display the cursor there, just say we did.
	     The code below doesn't display it right, and nobody wants
	     to see it anyway.  */
	  ;
	else if (active_screen->enable[VisibleY]
		 && VisibleX < active_screen->used[VisibleY]) {
		if (CursorExists)
			XDrawImageString(XXdisplay, XXwindow, XXgc_norm,
				    VisibleX*XXfontw+XXInternalBorder,
				    VisibleY*XXfonth+XXInternalBorder+XXbase,
				    (char *) &active_screen->contents[VisibleY][VisibleX],
				    1);
		else if (CursorOutline) {
			XDrawImageString(XXdisplay, XXwindow, XXgc_norm,
				    VisibleX*XXfontw+XXInternalBorder,
				    VisibleY*XXfonth+XXInternalBorder+XXbase,
				    (char *) &active_screen->contents[VisibleY][VisibleX],
				    1);
			XDrawRectangle (XXdisplay, XXwindow, XXgc_curs_rev,
					VisibleX*XXfontw+XXInternalBorder,
					VisibleY*XXfonth+XXInternalBorder,
					XXfontw - 1, XXfonth - 1);
		} else
			XDrawImageString(XXdisplay, XXwindow, XXgc_curs,
				    VisibleX*XXfontw+XXInternalBorder,
				    VisibleY*XXfonth+XXInternalBorder+XXbase,
				    (char *) &active_screen->contents[VisibleY][VisibleX],
				    1);
	      }
	else {
		if (CursorExists)
			XClearArea (XXdisplay, XXwindow,
				    VisibleX*XXfontw+XXInternalBorder,
				    VisibleY*XXfonth+XXInternalBorder,
				    XXfontw, XXfonth, 0);
		else if (CursorOutline)
			XDrawRectangle (XXdisplay, XXwindow, XXgc_curs_rev,
 					VisibleX*XXfontw+XXInternalBorder,
					VisibleY*XXfonth+XXInternalBorder,
					XXfontw - 1, XXfonth - 1);
		else
			XDrawImageString(XXdisplay, XXwindow, XXgc_curs,
				    VisibleX*XXfontw+XXInternalBorder,
				    VisibleY*XXfonth+XXInternalBorder+XXbase,
				    " ", 1);
	      }

	CursorExists = !CursorExists;

	if (!InUpdate)
		XFlush (XXdisplay);

	return 1;
}

/* This routine is used by routines which are called to paint regions */
/* designated by Expose events.  If the cursor may be in the exposed */
/* region, this routine makes sure it is gone so that dumprectangle can */
/* toggle it back into existance if dumprectangle is invoked when not in */
/* the midst of a screen update. */


static void 
ClearCursor ()
{
	BLOCK_INPUT_DECLARE ();

	BLOCK_INPUT ();
	if (!WindowMapped) {
		CursorExists = 0;
		CursorOutline = 1;
		UNBLOCK_INPUT ();
		return;
	}
	
	if (VisibleX < 0 || VisibleX >= screen_width ||
	    VisibleY < 0 || VisibleY >= screen_height) {
		/* Not much can be done */
		CursorExists = 0;
		UNBLOCK_INPUT ();
		return;
	}

	XClearArea (XXdisplay, XXwindow,
		    VisibleX*XXfontw+XXInternalBorder,
		    VisibleY*XXfonth+XXInternalBorder,
		    XXfontw, XXfonth, 0);

	CursorExists = 0;
	UNBLOCK_INPUT ();
}

void
XTupdate_begin ()
{
	BLOCK_INPUT_DECLARE ();
	register int i;
	
#ifdef XDEBUG
	fprintf (stderr, "XTupdate_begin\n");
#endif

	BLOCK_INPUT ();
	InUpdate = 1;
	if (CursorExists)
		CursorToggle ();

	for (i=0;i<MAXHEIGHT;i++)
		updated[i] = 0;
	
	SavedX = local_cursor_hpos;
	SavedY = local_cursor_vpos;
	/* Thw initial "hardware" cursor position is */
	/* saved because that is where gnu emacs */
	/* expects the cursor to be at the end of */
	/* the update */

	UNBLOCK_INPUT ();
}

void
XTupdate_end ()
{	
	BLOCK_INPUT_DECLARE ();

#ifdef XDEBUG
	fprintf (stderr, "XTupdate_end\n");
#endif

	BLOCK_INPUT ();
	if (CursorExists)
		CursorToggle ();

	InUpdate = 0;
	/* Display cursor at last place requested.  */
	XTmove_cursor (local_cursor_vpos, local_cursor_hpos);
	XFlush (XXdisplay);
	UNBLOCK_INPUT ();
}

/* Used for Expose events.  Have to get the text
 * back into the newly blank areas.
 */

#ifdef _NO_EMACS_PROTO
dumprectangle (top, left, rows, cols)
     register int top, left, rows, cols;
#else /* _NO_EMACS_PROTO */
void 
dumprectangle (
     register int top, 
     register int left, 
     register int rows, 
     register int cols)
#endif /* _NO_EMACS_PROTO */
{
	register struct matrix *active_screen;
	register int ourindex;
	int localX, localY, localHL;

 	if (top < 0)
		top = 0;
	if (left < 0)
		left = 0;
	rows += top;
	cols += left;
	top /= XXfonth;
	/* Get row and col containing up and */
	/* left borders of exposed region -- */
	/* round down here*/
	left /= XXfontw;
	rows += XXfonth-1;
	cols += XXfontw-1;
	rows /= XXfonth;
	/* Get row and col containing bottom and */
	/* right borders -- round up here */
	rows -= top;
	cols /= XXfontw;
	cols -= left;

	if (rows < 0)
		return;
	if (cols < 0)
		return;
	if (top > screen_height - 1)
		return;
	if (left > screen_width - 1)
		return;
	if (VisibleX >= left && VisibleX < left + cols &&
	    VisibleY >= top && VisibleY < top + rows)
		ClearCursor ();

	if (InUpdate)
		active_screen = new_screen;
	else
		/* When queue is dumped in update this */
		active_screen = current_screen;
	
	for (localY = top, ourindex = 0;
	     ourindex < rows && localY < screen_height;
	     ++ourindex, ++localY) {
		if (localY < 0 || localY >= screen_height ||
		    !active_screen->enable[localY] ||
		    left+1 > active_screen->used[localY])
			continue;
		localX = left;
		localHL = active_screen->highlight[localY];
		dumpchars (active_screen,
			   min (cols,
				active_screen->used[localY]-localX),
			   localX, localY, localHL);
	}
	if (!InUpdate && !CursorExists)
		CursorToggle ();
	/* Routine usually called */
	/* when not in update */
}

/* What sections of the window will be modified from the UpdateDisplay
 * routine is totally under software control.  Any line with Y coordinate
 * greater than flexlines will not change during an update.  This is really
 * used only during dellines and inslines routines (scraplines and stufflines)
 */

#ifdef _NO_EMACS_PROTO
XTset_terminal_window (n)
     register int n;
#else /* _NO_EMACS_PROTO */
void 
XTset_terminal_window ( register int n )
#endif /* _NO_EMACS_PROTO */
{
#ifdef XDEBUG
	fprintf (stderr, "XTset_terminal_window\n");
#endif

	if (n <= 0 || n > screen_height)
		flexlines = screen_height;
	else
		flexlines = n;
}

#ifdef _NO_EMACS_PROTO
XTins_del_lines (vpos, n)
     int vpos, n;
#else /* _NO_EMACS_PROTO */
void
XTins_del_lines ( int vpos, int n )
#endif /* _NO_EMACS_PROTO */
{
#ifdef XDEBUG
	fprintf (stderr, "XTins_del_lines\n");
#endif

	XTmove_cursor (vpos, 0);
	if (n >= 0)
		stufflines (n);
	else
		scraplines (-n);
}

/* Estimate the cost of scrolling as equal to drawing one fifth
   of the character cells copied if black and white,
   or half of those characters if color.  */

#ifdef _NO_EMACS_PROTO
static
XTcalculate_costs (extra, costvec, ncostvec)
     int extra;
     int *costvec, *ncostvec;
#else /* _NO_EMACS_PROTO */
static void 
XTcalculate_costs ( int extra, int *costvec, int *ncostvec)
#endif /* _NO_EMACS_PROTO */
{
  int color_p = DisplayCells (XXdisplay, XXscreen) > 2;

  CalcLID (0, screen_width / (color_p ? 2 : 5), 0, 0, costvec, ncostvec);
}

#ifdef _NO_EMACS_PROTO
static
XTinsert_chars (start, len)
     register char *start;
     register int len;
#else /* _NO_EMACS_PROTO */
static void 
XTinsert_chars (
     register char *start,
     register int len )
#endif /* _NO_EMACS_PROTO */
{
#ifdef XDEBUG
	fprintf (stderr, "XTinsert_chars\n");
#endif

	updateline (0);
}

#ifdef _NO_EMACS_PROTO
static
XTdelete_chars (n)
     register int n;
#else /* _NO_EMACS_PROTO */
static void
XTdelete_chars ( register int n )
#endif /* _NO_EMACS_PROTO */
{
	char *msg = "Major foobars!  This shouldn't show up!";
	
#ifdef XDEBUG
	fprintf (stderr, "XTdelete_chars (num %d local_cursor_hpos %d)\n",n,local_cursor_hpos);
#endif

	updateline (0);
}

#ifdef _NO_EMACS_PROTO
stufflines (n)
     register int n;
#else /* _NO_EMACS_PROTO */
void 
stufflines ( register int n )
#endif /* _NO_EMACS_PROTO */
{
	register int topregion, bottomregion;
	register int length, newtop;
	BLOCK_INPUT_DECLARE ();

	if (local_cursor_vpos >= flexlines)
		return;

	BLOCK_INPUT ();

	if (CursorExists)
		CursorToggle ();

	topregion = local_cursor_vpos;
	bottomregion = flexlines-(n+1);
	newtop = local_cursor_vpos+n;
	length = bottomregion-topregion+1;

	if (length > 0 && newtop <= flexlines) {
		XCopyArea (XXdisplay, XXwindow, XXwindow, XXgc_norm,
			   XXInternalBorder,
			   topregion*XXfonth+XXInternalBorder,
			   screen_width*XXfontw,
			   length*XXfonth,
			   XXInternalBorder, newtop*XXfonth+XXInternalBorder);
	}

	newtop = min (newtop, flexlines-1);
	length = newtop-topregion;
	if (length > 0)
		XClearArea (XXdisplay, XXwindow,
			    XXInternalBorder,
			    topregion*XXfonth+XXInternalBorder,
			    screen_width*XXfontw,
			    n*XXfonth, 0);
	UNBLOCK_INPUT ();
}

#ifdef _NO_EMACS_PROTO
scraplines (n)
     register int n;
#else /* _NO_EMACS_PROTO */
void
scraplines ( register int n )
#endif /* _NO_EMACS_PROTO */
{
	BLOCK_INPUT_DECLARE ();

	if (local_cursor_vpos >= flexlines)
		return;

	BLOCK_INPUT ();

	if (CursorExists)
		CursorToggle ();

	if (local_cursor_vpos+n >= flexlines) {
		if (flexlines >= (local_cursor_vpos + 1))
			XClearArea (XXdisplay, XXwindow,
				    XXInternalBorder,
				    local_cursor_vpos*XXfonth+XXInternalBorder,
				    screen_width*XXfontw,
				    (flexlines-local_cursor_vpos) * XXfonth,
				    0);
		UNBLOCK_INPUT ();
	}
	else {
		XCopyArea (XXdisplay, XXwindow, XXwindow, XXgc_norm,
			   XXInternalBorder,
			   (local_cursor_vpos+n)*XXfonth+XXInternalBorder,
			   screen_width*XXfontw,
			   (flexlines-local_cursor_vpos-n)*XXfonth,
			   XXInternalBorder, local_cursor_vpos*XXfonth+XXInternalBorder);

		XClearArea (XXdisplay, XXwindow, XXInternalBorder,
			    (flexlines-n)*XXfonth+XXInternalBorder,
			    screen_width*XXfontw,
			    n*XXfonth, 0);
		UNBLOCK_INPUT ();
	}
}
	
/* Substitutes for standard read routine.  Under X not interested in individual
 * bytes but rather individual packets.
 */

#ifdef _NO_EMACS_PROTO
XTread_socket (sd, bufp, numchars)
     register int sd;
     register char *bufp;
     register int numchars;
#else /* _NO_EMACS_PROTO */
int
XTread_socket (
     register int sd,
     register char *bufp,
     register int numchars )
#endif /* _NO_EMACS_PROTO */
{
#ifdef XDEBUG
	fprintf(stderr,"XTread_socket\n");
#endif

	return (internal_socket_read ((unsigned char *)bufp, numchars));
}

/*
 * Interpreting incoming keycodes. Should have table modifiable as needed
 * from elisp.
 */

#ifdef sun
char *stringFuncVal( KeySym keycode )
{
	switch (keycode) {
	case XK_L1:
		return("192");
	case XK_L2:
		return("193");
	case XK_L3:
		return("194");
	case XK_L4:
		return("195");
	case XK_L5:
		return("196");
	case XK_L6:
		return("197");
	case XK_L7:
		return("198");
	case XK_L8:
		return("199");
	case XK_L9:
		return("200");
	case XK_L10:
		return("201");

	case XK_R1:
		return("208");
	case XK_R2:
		return("209");
	case XK_R3:
		return("210");
	case XK_R4:
		return("211");
	case XK_R5:
		return("212");
	case XK_R6:
		return("213");
	case XK_R7:
		return("214");
	case XK_R8:
		return("215");
	case XK_R9:
		return("216");
	case XK_R10:
		return("217");
	case XK_R11:
		return("218");
	case XK_R12:
		return("219");
	case XK_R13:
		return("220");
	case XK_R14:
		return("221");
	case XK_R15:
		return("222");

	case XK_Break:			/* Sun3 "Alternate" key */
		return("223");

	case XK_F1:
		return("224");
	case XK_F2:
		return("225");
	case XK_F3:
		return("226");
	case XK_F4:
		return("227");
	case XK_F5:
		return("228");
	case XK_F6:
		return("229");
	case XK_F7:
		return("230");
	case XK_F8:
		return("231");
	case XK_F9:
		return("232");
	case XK_F10:
		return("233");

	default:
		return("-1");
	}
}
#else
#ifndef AIX
#ifdef _NO_EMACS_PROTO
char *stringFuncVal(keycode)
	KeySym keycode;
#else /* _NO_EMACS_PROTO */
char *stringFuncVal( KeySym keycode )
#endif /* _NO_EMACS_PROTO */
{
	switch (keycode) {
	case XK_F1:
		return("11");
	case XK_F2:
		return("12");
	case XK_F3:
		return("13");
	case XK_F4:
		return("14");
	case XK_F5:
		return("15");
	case XK_F6:
		return("17");
	case XK_F7:
		return("18");
	case XK_F8:
		return("19");
	case XK_F9:
		return("20");
	case XK_F10:
		return("21");
	case XK_F11:
		return("23");
	case XK_F12:
		return("24");
	case XK_F13:
		return("25");
	case XK_F14:
		return("26");
	case XK_F15:
		return("28");
	case XK_Help:
		return("28");
	case XK_F16:
		return("29");
	case XK_Menu:
		return("29");
	case XK_F17:
		return("31");
	case XK_F18:
		return("32");
	case XK_F19:
		return("33");
	case XK_F20:
		return("34");
	
	case XK_Find :
		return("1");
	case XK_Insert:
		return("2");
	case XK_Delete:
		return("3");
	case XK_Select:
		return("4");
	case XK_Prior:
		return("5");
	case XK_Next:
		return("6");
	default:
		return("-1");
	}
}
#endif /* not AIX */
#endif /* not sun */

#ifdef _NO_EMACS_PROTO
internal_socket_read(bufp, numchars)
	register unsigned char *bufp;
	register int numchars;
#else /* _NO_EMACS_PROTO */
int
internal_socket_read ( register unsigned char *bufp, 
	register int numchars )
#endif /* _NO_EMACS_PROTO */
{
  /* Number of keyboard chars we have produced so far.  */
  int count = 0;
  int nbytes;
  char mapping_buf[20];
  BLOCK_INPUT_DECLARE ();
  XEvent event;
  /* Must be static since data is saved between calls.  */
  static XComposeStatus status;
  KeySym keysym;
  SIGMASKTYPE oldmask;

  BLOCK_INPUT ();
#ifdef BSD
#ifndef BSD4_1
  oldmask = sigblock (sigmask (SIGALRM));
#endif
#endif
#ifdef FIOSNBIO
  /* If available, Xlib uses FIOSNBIO to make the socket
     non-blocking, and then looks for EWOULDBLOCK.  If O_NDELAY is set, FIOSNBIO is
     ignored, and instead of signalling EWOULDBLOCK, a read returns
     0, which Xlib interprets as equivalent to EPIPE. */
  fcntl (fileno (stdin), F_SETFL, 0);
#endif
#ifndef HAVE_SELECT
  if (! (fcntl (fileno (stdin), F_GETFL, 0) & O_NDELAY))
    {
      extern int read_alarm_should_throw;
      read_alarm_should_throw = 1;
      XPeekEvent (XXdisplay,&event);
      read_alarm_should_throw = 0;
    }
#endif
  while (XPending (XXdisplay)) {
    XNextEvent (XXdisplay,&event);
    event.type &= 0177;		/* Mask out XSendEvent indication */

    switch (event.type) {

    default:
      break;

    case MappingNotify:
      XRefreshKeyboardMapping(&event.xmapping);
      break;

    case MapNotify:
      WindowMapped = 1;
      break;

    case UnmapNotify:
      WindowMapped = 0;
      break;
			
    case ConfigureNotify:
      if (abs(pixelheight-event.xconfigure.height) >= XXfonth
	  || abs(pixelwidth-event.xconfigure.width) >= XXfontw)
	{
	  int rows, cols;

	  rows = (event.xconfigure.height-2*XXInternalBorder)/XXfonth;
	  cols = (event.xconfigure.width-2*XXInternalBorder)/XXfontw;
	  pixelwidth = cols*XXfontw+2*XXInternalBorder;
	  pixelheight = rows*XXfonth+2*XXInternalBorder;

	  /* This is absolutely, amazingly gross.  However, without
	     it, emacs will core dump if the window gets too small.
	     And uwm is too brain-damaged to handle large minimum size
	     windows.  */
	  if (cols > 11 && rows > 4)
	    /* Delay the change unless Emacs is waiting for input.  */
	    change_screen_size (rows, cols, 0, !waiting_for_input, 1);
	}
      break;

    case Expose:
      if (!delayed_size_change)
	dumprectangle (event.xexpose.y-XXInternalBorder,
		       event.xexpose.x-XXInternalBorder,
		       event.xexpose.height,
		       event.xexpose.width);
      break;

    case GraphicsExpose:
      if (!delayed_size_change)
	dumprectangle (event.xgraphicsexpose.y-XXInternalBorder,
		       event.xgraphicsexpose.x-XXInternalBorder,
		       event.xgraphicsexpose.height,
		       event.xgraphicsexpose.width);
      break;

    case NoExpose:
      break;

#if 0
    case FocusIn:
    case FocusOut:
    case EnterNotify:
    case LeaveNotify:
      {
	int cursor = -1;	/* 0=on, 1=off, -1=no change */

	if (event.type == FocusIn) cursor = 0;
	else if (event.type == FocusOut) cursor = 1;
	else
	  {
	    Window focus;
	    int revert_to;	/* dummy return val */
	    XGetInputFocus (XXdisplay, &focus, &revert_to);
	    if (focus == PointerRoot)
	      cursor = (event.type == LeaveNotify) ? 1 : 0;
	  }
	if (cursor == -1) break;
	CursorToggle();
	CursorOutline = cursor;
	CursorToggle();
      }
#endif
    case FocusIn:
      /* kenc@viewlogic.com says adding the following line fixes the
	 problems with grabbing and twm.  */
      if (event.xfocus.mode == NotifyNormal)
	x_focus_flag = 1;
    case EnterNotify:
      if (event.type == FocusIn || (!x_focus_flag && event.xcrossing.focus))
	{
	  CursorToggle ();
	  CursorOutline = 0;
	  CursorToggle ();
	}
      break;

    case FocusOut:
      x_focus_flag = 0;
    case LeaveNotify:
      if (event.type == FocusOut || (!x_focus_flag && event.xcrossing.focus))
	{
	  CursorToggle ();
	  CursorOutline = 1;
	  CursorToggle ();
	}
      break;

    case KeyPress:
      nbytes = XLookupString (&event.xkey,
			      mapping_buf, 20, &keysym,
			      0);

#ifndef AIX
      /* Someday this will be unnecessary as we will
	 be able to use XRebindKeysym so XLookupString
	 will have already given us the string we want. */
      if (IsFunctionKey(keysym)
	  || IsMiscFunctionKey(keysym)
	  || keysym == XK_Prior
	  || keysym == XK_Next) {
	strcpy(mapping_buf,"[");
	strcat(mapping_buf,stringFuncVal(keysym));
#ifdef sun
	strcat(mapping_buf,"z");
#else
	strcat(mapping_buf,"~");
#endif /* sun */
	nbytes = strlen(mapping_buf);
      }
      else {
#endif /* not AIX */
	switch (keysym) {
	case XK_Left:
	  strcpy(mapping_buf,"\002");
	  nbytes = 1;
	  break;
	case XK_Right:
	  strcpy(mapping_buf,"\006");
	  nbytes = 1;
	  break;
	case XK_Up:
	  strcpy(mapping_buf,"\020");
	  nbytes = 1;
	  break;
	case XK_Down:
	  strcpy(mapping_buf,"\016");
	  nbytes = 1;
	  break;
	}
#ifndef AIX
      }
#endif  /* not AIX */
      if (nbytes) {
	if ((nbytes == 1) && (event.xkey.state & Mod1Mask))
	  *mapping_buf |= METABIT;
	if ((nbytes == 1) && (event.xkey.state & ControlMask))
	  *mapping_buf &= 0x9F;		/* mask off bits 1 and 2 */
	if (numchars-nbytes > 0) {
	  bcopy (mapping_buf, bufp, nbytes);
	  bufp += nbytes;
	  count += nbytes;
	  numchars -= nbytes;
	}
      }
      break;

    case ButtonPress:
    case ButtonRelease:
      *bufp++ = (char) 'X' & 037;
      ++count;
      --numchars;
      *bufp++ = (char) '@' & 037;
      ++count;
      --numchars;
      if (XXm_queue_num == XMOUSEBUFSIZE)
	break;
      XXm_queue[XXm_queue_in] = (XEvent *) malloc (sizeof(XEvent));
      *XXm_queue[XXm_queue_in] = event;
      XXm_queue_num++;
      XXm_queue_in = (XXm_queue_in + 1) % XMOUSEBUFSIZE;
      break;
    }
  }

  if (CursorExists)
    xfixscreen ();

#ifdef BSD
#ifndef BSD4_1
  sigsetmask (oldmask);
#endif
#endif
  UNBLOCK_INPUT ();
  return count;
}

/* Exit gracefully from gnuemacs, doing an autosave and giving a status.
 */

int
XExitGracefully ()
{
	XCleanUp();
	exit (70);
}

int
XIgnoreError ()
{
	return 0;
}

static int server_ping_timer;

void
xfixscreen ()
{
	BLOCK_INPUT_DECLARE ();

	if (server_ping_timer > 0)
	  server_ping_timer--;
	else
	  {
	    server_ping_timer = 100;

	    /* Yes, this is really what I mean -- Check to see if we've
	     * lost our connection */

	    BLOCK_INPUT ();
	    XSetErrorHandler(0);
	    XSetIOErrorHandler(0);
	    XNoOp (XXdisplay);
	    XFlush (XXdisplay);
	    XSetErrorHandler(handler);
	    XSetIOErrorHandler(handler);
	    if (!InUpdate && !CursorExists)
		    CursorToggle ();

	    UNBLOCK_INPUT ();
	  }
}
	

/* ------------------------------------------------------------
 */
static int  reversevideo;

#ifdef _NO_EMACS_PROTO
static int
XT_GetDefaults (class)
    char *class;
#else /* _NO_EMACS_PROTO */
static int 
XT_GetDefaults ( char *class )
#endif /* _NO_EMACS_PROTO */
{
  register struct _xdeftab *entry;
  char *iname, *cname;

  char *disp = 0, *scrn = 0;

  int len = strlen (CLASS);
  if (strlen (class) > len)
    len = strlen (class);

  /* 100 is bigger than any of the resource names in our fixed set.  */
  iname = (char *) alloca (len + 100);
  cname = (char *) alloca (len + 100);

  XrmInitialize ();

  /* Merge all databases on root window.  */

  disp = XResourceManagerString (XXdisplay);
  if (disp) db = XrmGetStringDatabase (disp);

#if (XlibSpecificationRelease >= 5)
  scrn = XScreenResourceString (ScreenOfDisplay (XXdisplay, XXscreen));
  if (scrn) db2 = XrmGetStringDatabase (scrn);

  /* Screen database takes precedence over global database.  */
  XrmMergeDatabases (db2, &db);
#endif

  /*
   * Walk the table reading in the resources.  Instance names supersede
   * class names.
   */

  for (entry = xDefaultsValueTable; entry->iname; entry++)
    {
      /* Build the instance name and class name of resource.  */
      XrmValue value;
      char *dummy;
      register char  *option;

      strcpy (iname, class);
      strcat (iname, ".");
      strcat (iname, entry->iname);
      strcpy (cname, CLASS);
      strcat (cname, ".");
      strcat (cname, entry->cname);

      if (XrmGetResource (db, iname, cname, &dummy, &value))
	{
	  if (entry->varp)
	    *entry->varp = (char*)value.addr;
	}
      else
	{
#ifdef XBACKWARDS
	  if (!(option = XGetDefault (XXdisplay, entry->iname, class)))
	    if (!(option = XGetDefault (XXdisplay, entry->iname, CLASS)))
	      if (!(option = XGetDefault (XXdisplay, entry->cname, class)))
		option = XGetDefault (XXdisplay, entry->cname, CLASS);
#else
	  if (!(option = XGetDefault (XXdisplay, class, entry->iname)))
	    if (!(option = XGetDefault (XXdisplay, CLASS, entry->iname)))
	      if (!(option = XGetDefault (XXdisplay, class, entry->cname)))
		option = XGetDefault (XXdisplay, CLASS, entry->cname);
#endif
	  if (option && entry->varp)
	    *entry->varp = option;
	}
    }

  /*
   * Now set global variables that aren't character strings; yes it would
   * be nice to do this automatically as part of the scanning step, but this
   * is less likely to screw up.  The real answer is to use the resource
   * manager.
   */

  if (temp_reverseVideo)
    {
      if (strcmp (temp_reverseVideo, "on") == 0)
	reversevideo = 1;
      else if (strcmp (temp_reverseVideo, "off") == 0)
	reversevideo = 0;
    }

  if (temp_borderWidth) 
    XXborder = atoi (temp_borderWidth);

  if (temp_internalBorder)
    XXInternalBorder = atoi (temp_internalBorder);

  if (temp_useBitmap)
    {
      if (strcmp (temp_useBitmap, "on") == 0)
	XXicon_usebitmap = 1;
      else if (strcmp (temp_useBitmap, "off") == 0)
	XXicon_usebitmap = 0;
    }

  return 0;
}

#ifdef _NO_EMACS_PROTO
x_error_handler (disp, event)
     Display *disp;
     XErrorEvent *event;
#else /* _NO_EMACS_PROTO */
x_error_handler ( Display *disp, XErrorEvent *event )
#endif /* _NO_EMACS_PROTO */
{
  char msg[200];
  XGetErrorText (disp, event->error_code, msg, 200);
  fprintf (stderr, "Fatal X-windows error: %s\n", msg);
  Fkill_emacs (make_number (70));
}

#ifdef _NO_EMACS_PROTO
x_io_error_handler ( *disp ) 
	Display *disp;
#else /* _NO_EMACS_PROTO */
int 
x_io_error_handler ( Display *disp)
#endif /* _NO_EMACS_PROTO */
{
  int save_errno = errno;
  if (errno == EPIPE)
    kill (0, SIGHUP);
  Fdo_auto_save (Qnil);
  errno = save_errno;
  perror ("Fatal X-windows I/O error");
  kill (0, SIGILL);
}

void
x_term_init ()
{
	register char *vardisplay;
	register int xxargc;
	register char **xxargv;
	char *ptr;
	XColor cdef;

	extern char *getenv ();
	extern XTinterrupt_signal ();
	extern Lisp_Object Vxterm, Vxterm1, Qt;
	extern int XIgnoreError();
	int  ix;
	

	vardisplay = (alternate_display ? alternate_display : "");
	if (!vardisplay) {
		fprintf (stderr, "DISPLAY environment variable must be set\n");
		exit (-200);
	}

	XXdisplay = XOpenDisplay (vardisplay);
	if (XXdisplay == (Display *) 0) {
		fprintf (stderr, "X server not responding.  Check your DISPLAY environment variable.\n");
		exit (-99);	
	}

	XXscreen = DefaultScreen (XXdisplay);
	XXisColor = DisplayCells (XXdisplay, XXscreen) > 2;
	XXColorMap = DefaultColormap (XXdisplay, XXscreen);

	XSetErrorHandler (x_error_handler);
	XSetIOErrorHandler (x_io_error_handler);
	signal (SIGPIPE, x_io_error_handler);

	WindowMapped = 0;
	baud_rate = 9600;
	min_padding_speed = 10000;
	must_write_spaces = 1;
	meta_key = 1;
	visible_bell = 1;
	inverse_video = 0;
	
	fix_screen_hook = xfixscreen;
	clear_screen_hook = XTclear_screen;
	clear_end_of_line_hook = XTclear_end_of_line;
	ins_del_lines_hook = XTins_del_lines;
	change_line_highlight_hook = XTchange_line_highlight;
	insert_chars_hook = XTinsert_chars;
	output_chars_hook = XToutput_chars;
	delete_chars_hook = XTdelete_chars;
	ring_bell_hook = XTfeep;
	reset_terminal_modes_hook = XTreset_terminal_modes;
	set_terminal_modes_hook = XTset_terminal_modes;
	update_begin_hook = XTupdate_begin;
	update_end_hook = XTupdate_end;
	set_terminal_window_hook = XTset_terminal_window;
	read_socket_hook = XTread_socket;
	move_cursor_hook = XTmove_cursor;
	reassert_line_highlight_hook = XTreassert_line_highlight;
	scroll_region_ok = 1;	/* we'll scroll partial screens */
	char_ins_del_ok = 0;
	line_ins_del_ok = 1;	/* we'll just blt 'em */
	fast_clear_end_of_line = 1; /* X does this well */
	memory_below_screen = 0; /* we don't remember what scrolls
				  * 		off the bottom */
	dont_calculate_costs = 1;
	calculate_costs_hook = XTcalculate_costs;

	/* New options section */
	XXborder = 1;
	XXInternalBorder = 1;
	screen_width = 80;
	screen_height = 66;
	
	reversevideo = 0;

	XXdebug = 0;
	XXm_queue_num = 0;
	XXm_queue_in = 0;
	XXm_queue_out = 0;

#if 0
	handler = XIgnoreError;
	XSetErrorHandler (handler);
	XSetIOErrorHandler (handler);
#endif

	desiredwindow =
	XXcurrentfont =
	XXidentity =
	XXicon_name =
	XXheader = (char *) NULL;

	XXicon_usebitmap = 0;
	
#ifdef X_DEFAULT_FONT
	temp_font = X_DEFAULT_FONT;
#else
	temp_font = "fixed";
#endif
	progname = xargv[0];
	if (ptr = rindex(progname, '/'))
	  progname = ptr+1;
	XXpid = getpid ();
	default_window = "=80x24+0+0";

#if 0
	handler = XIgnoreError;
	XSetErrorHandler (handler);
	XSetIOErrorHandler (handler);
#endif

	/*  Get resource name and its defaults, it it exists...
	 */
	for (ix = 1; ix < xargc && xargv[ix][0] == '-'; ix++)
	{
	    int  valx;
	
	    if (strcmp(xargv[ix], "-rn") == 0 &&
		(valx = ix + 1) < xargc)
	    {
		XXidentity = (char *) xmalloc (strlen(xargv[valx]) + 1 );
		(void) strcpy(XXidentity, xargv[valx]);

		break;
	    }
	}

	if (!XXidentity)
	{
	    char  *t;

	    if ( (t = getenv("WM_RES_NAME")) != (char *) NULL )
		XXidentity = t;

	    if (!XXidentity)
	    {
		XXidentity = progname;
	    }
	}

	if (XXidentity)
	    XT_GetDefaults(XXidentity);
	else
	    XT_GetDefaults(CLASS);

	XXpid = getpid ();
	default_window = "=80x24+0+0";

	/* Process X command line args...*/
	xxargc = xargc;
	xxargv = xargv;
	xxargv++;
	xxargc--;
	while (xxargc) {
		int sargc;
		sargc = xxargc;
		if (xxargc && !strcmp (*xxargv, "-r")) {
			reversevideo = !reversevideo;
			xxargc--;
			xxargv++;
		}
		if ((xxargc > 1) && (!strcmp (*xxargv, "-font") ||
				     !strcmp (*xxargv, "-fn"))) {
			xxargc--;
			xxargv++;
			if (XXcurrentfont != NULL)
				free(XXcurrentfont);
			XXcurrentfont = (char *) xmalloc (strlen (*xxargv)+1);
			strcpy (XXcurrentfont, *xxargv);
			xxargc--;
			xxargv++;
		}
		if ((xxargc > 1) && !strcmp (*xxargv, "-wn")) {
			xxargc--;
			xxargv++;
			XXheader = (char *) xmalloc (strlen (*xxargv)+1);
			strcpy (XXheader, *xxargv);
			xxargc--;
			xxargv++;
		}
		if ((xxargc > 1) && !strcmp (*xxargv, "-in")) {
			xxargc--;
			xxargv++;
			XXicon_name = (char *) xmalloc (strlen (*xxargv)+1);
			strcpy (XXicon_name, *xxargv);
			xxargc--;
			xxargv++;
		}
		if (xxargc && !strcmp (*xxargv, "-i")) {
			xxargc--;
			xxargv++;
			XXicon_usebitmap = 1;
		}
		if ((xxargc > 1) && !strcmp (*xxargv, "-b")) {
			xxargc--;
			xxargv++;
			XXborder = atoi (*xxargv);
			xxargc--;
			xxargv++;
		}
		if ((xxargc > 1) && !strcmp (*xxargv, "-ib")) {
			xxargc--;
			xxargv++;
			XXInternalBorder = atoi (*xxargv);
			xxargc--;
			xxargv++;
		}
		if ((xxargc > 1) && (!strcmp (*xxargv, "-w") ||
				     !strcmp (*xxargv, "-geometry"))) {
			xxargc--;
			xxargv++;
			desiredwindow = (char *) xmalloc (strlen (*xxargv)+1);
			strcpy (desiredwindow, *xxargv);
			xxargc--;
			xxargv++;
		}
		if (XXisColor) {
			if ((xxargc > 1 && !strcmp (*xxargv, "-fg"))) {
			        xxargc--;
			        xxargv++;

			        fore_color =
				    (char *) xmalloc (strlen (*xxargv)+1);
			        strcpy (fore_color, *xxargv);

				xxargc--;
				xxargv++;
			}
			if ((xxargc > 1 && !strcmp (*xxargv, "-bg"))) {
			        xxargc--;
			        xxargv++;

				back_color =
				    (char *) xmalloc (strlen (*xxargv)+1);
				strcpy (back_color, *xxargv);

				xxargc--;
				xxargv++;
			}
			if ((xxargc > 1 && !strcmp (*xxargv, "-bd"))) {
				xxargc--;
				xxargv++;

				brdr_color =
				    (char *) xmalloc (strlen (*xxargv)+1);
				strcpy (brdr_color, *xxargv);

				xxargc--;
				xxargv++;
			}
			if ((xxargc > 1 && !strcmp (*xxargv, "-cr"))) {
				xxargc--;
				xxargv++;

				curs_color =
				    (char *) xmalloc (strlen (*xxargv)+1);
				strcpy (curs_color, *xxargv);

				xxargc--;
				xxargv++;
			}
			if ((xxargc > 1 && !strcmp (*xxargv, "-ms"))) {
				xxargc--;
				xxargv++;

				mous_color =
				    (char *) xmalloc (strlen (*xxargv)+1);
				strcpy (mous_color, *xxargv);

				xxargc--;
				xxargv++;
			}
		}
		if (sargc == xxargc) {
			xxargc--;
			xxargv++;
		}
	}

	/*  Now, actually Parse and Set colors...
	 */
	if (XXisColor) {
	  if (fore_color || back_color)
	    reversevideo = 0;
	  if (fore_color &&
	      XParseColor (XXdisplay, XXColorMap, fore_color, &cdef) &&
	      XAllocColor (XXdisplay, XXColorMap, &cdef))
	    fore = cdef.pixel;
	  else {
	    fore_color = black_color;
	    fore = BlackPixel (XXdisplay, XXscreen);
	  }

	  if (back_color &&
	      XParseColor (XXdisplay, XXColorMap, back_color, &cdef) &&
	      XAllocColor (XXdisplay, XXColorMap, &cdef))
	    back = cdef.pixel;
	  else {
	    back_color = white_color;
	    back = WhitePixel (XXdisplay, XXscreen);
	  }

	  if (curs_color &&
	      XParseColor (XXdisplay, XXColorMap, curs_color, &cdef) &&
	      XAllocColor (XXdisplay, XXColorMap, &cdef))
	    curs = cdef.pixel;
	  else {
	    curs_color = black_color;
	    curs = BlackPixel (XXdisplay, XXscreen);
	  }

	  if (mous_color &&
	      XParseColor (XXdisplay, XXColorMap, mous_color, &cdef) &&
	      XAllocColor (XXdisplay, XXColorMap, &cdef))
	    ;
	  else mous_color = black_color;

	  if (brdr_color &&
	      XParseColor (XXdisplay, XXColorMap, brdr_color, &cdef) &&
	      XAllocColor (XXdisplay, XXColorMap, &cdef))
	    brdr = cdef.pixel;
	  else {
	    brdr_color = black_color;
	    brdr = BlackPixel (XXdisplay, XXscreen);
	  }
	}
	else {
		fore_color  = curs_color = mous_color = brdr_color
		  = black_color;
		fore = curs = brdr = BlackPixel (XXdisplay, XXscreen);
		back_color = white_color;
		back = WhitePixel (XXdisplay, XXscreen);
	}


	if (reversevideo) {
		unsigned long tempcolor;
		char *tempname;
		brdr = back;
		brdr_color = back_color;
		tempcolor = fore;
		fore = back;
		back = tempcolor;
		tempname = fore_color;
		fore_color = back_color;
		back_color = tempname;
		if (!strcmp (mous_color, back_color))
		  mous_color = fore_color;
		else if (!strcmp (mous_color, fore_color))
		  mous_color = back_color;
		if (!strcmp (curs_color, back_color))
		  curs_color = fore_color, curs = fore;
		else if (!strcmp (curs_color, fore_color))
		  curs_color = back_color, curs = back;
	}


	if (!XXcurrentfont)
	{
	    XXcurrentfont = (char *) xmalloc (strlen (temp_font) + 1);
	
	    if (!XXcurrentfont) {
		fprintf (stderr, "Memory allocation failure.\n");
		exit (-150);
	    }

	    strcpy (XXcurrentfont, temp_font);
	}
	


	signal (SIGPIPE, XExitGracefully);

#ifndef CANNOT_DUMP
	if (initialized)
#endif /* CANNOT_DUMP */
		Vxterm = Qt;

	Fset (intern ("window-system-version"), make_number (11));

	XInitWindow ();

	keyboard_init_hook = x_init_1;
}

/* Initialize for keyboard input using X.
   This is called by init_keyboard via keyboard_init_hook.  */

static void
x_init_1 ()
{
#ifdef F_SETOWN
	extern int old_fcntl_owner;
#endif

	dup2 (ConnectionNumber(XXdisplay), 0);
#if 0 /* code below breaks local:0 shared memory transport of the Xserver */
#ifndef SYSV_STREAMS
	/* Streams somehow keeps track of which descriptor number
	   is being used to talk to X.  So it is not safe to substitute
	   descriptor 0.  But it is safe to make descriptor 0 a copy of it.  */
	close (ConnectionNumber(XXdisplay));
	ConnectionNumber(XXdisplay) = 0;	/* Looks a little strange?
						 * check the def of the macro;
						 * it is a genuine lvalue */
#endif
#endif /* 0 */

#ifdef USG
	setpgrp ();		/* No arguments but equivalent in this case */
#else
	setpgrp (0, getpid ());
#endif /* USG */
	
#ifdef F_SETOWN
	old_fcntl_owner = fcntl (0, F_GETOWN, 0);
#ifdef F_SETOWN_SOCK_NEG
	fcntl (0, F_SETOWN, -getpid ());	/* stdin is a socket here */
#else
	fcntl (0, F_SETOWN, getpid ());
#endif /* F_SETOWN_SOCK_NEG */
#endif /* F_SETOWN */

	/* Enable interrupt_input because otherwise we cannot asynchronously
	   detect C-g sent as a keystroke event from the X server.  */
	Fset_input_mode (Qt, Qnil, Qnil);
}

void
XSetFlash ()
{
	ring_bell_hook = XTflash;
}

void
XSetFeep ()
{
	ring_bell_hook = XTfeep;
}


/* ------------------------------------------------------------
 *  Load a font by name.  Return the font pointer, or NULL if
 *  it can't be loaded.  Do all appropriate calculations.
 */
#ifdef _NO_EMACS_PROTO
static XFontStruct *
XT_CalcForFont(fontname)
    char  *fontname;
#else /* _NO_EMACS_PROTO */
static XFontStruct *
XT_CalcForFont( char  *fontname )
#endif /* _NO_EMACS_PROTO */
{
    XFontStruct  *fontp;


    if ( (fontp = XLoadQueryFont(XXdisplay, fontname)) == (XFontStruct *) 0 )
    {
	return  (XFontStruct *) NULL;
    }

    XXfid = fontp->fid;
    XXfonth = fontp->ascent + fontp->descent;
    XXfontw = fontp->max_bounds.width;
    XXbase = fontp->ascent;

    return  fontp;
}


/* ------------------------------------------------------------
 */
#ifdef _NO_EMACS_PROTO
XNewFont (newname)
     register char *newname;
#else /* _NO_EMACS_PROTO */
int
XNewFont ( register char *newname )
#endif /* _NO_EMACS_PROTO */
{
	XFontStruct *temp;
	BLOCK_INPUT_DECLARE ();

	BLOCK_INPUT ();
	XFlush (XXdisplay);


	temp = XT_CalcForFont(newname);

	if (temp == (XFontStruct *) NULL)
	{
	    UNBLOCK_INPUT ();
	    return  -1;
	}

	XSetFont (XXdisplay, XXgc_norm, XXfid);
	XSetFont (XXdisplay, XXgc_rev, XXfid);
	XSetFont (XXdisplay, XXgc_curs, XXfid);
	XSetFont (XXdisplay, XXgc_curs_rev, XXfid);

	XFreeFont (XXdisplay, fontinfo);
	fontinfo = temp;


	XSetWindowSize(screen_height, screen_width);


	UNBLOCK_INPUT ();
	return 0;
}

/* Flip foreground/background colors */
void
XFlipColor ()
{
	long tempcolor;
	char *tempname;
	XColor forec, backc;
	BLOCK_INPUT_DECLARE ();

	BLOCK_INPUT ();
	CursorToggle ();
	XSetWindowBackground(XXdisplay, XXwindow, fore);
	if (XXborder)
		XSetWindowBorder(XXdisplay, XXwindow, back);

	brdr = back;
	brdr_color = back_color;
	tempcolor = fore;
	fore = back;
	back = tempcolor;
	tempname = fore_color;
	fore_color = back_color;
	back_color = tempname;
	XClearArea (XXdisplay, XXwindow, 0, 0,
		    screen_width*XXfontw+2*XXInternalBorder,
		    screen_height*XXfonth+2*XXInternalBorder, 0);

	XXgc_temp = XXgc_norm;
	XXgc_norm = XXgc_rev;
	XXgc_rev = XXgc_temp;

	if (!strcmp (mous_color, back_color))
	  mous_color = fore_color;
	else if (!strcmp (mous_color, fore_color))
	  mous_color = back_color;

	x_set_cursor_colors ();

	XRedrawDisplay ();
	if (!strcmp (curs_color, back_color))
	  curs_color = fore_color, curs = fore;
	else if (!strcmp (curs_color, fore_color))
	  curs_color = back_color, curs = back;

	XSetState (XXdisplay, XXgc_curs, back, curs, GXcopy, AllPlanes);
	XSetState (XXdisplay, XXgc_curs_rev, curs, back, GXcopy, AllPlanes);

	CursorToggle ();
	XFlush (XXdisplay);
	UNBLOCK_INPUT ();
}

/* ------------------------------------------------------------
 */

#define NO_MANAGER  1


/* ------------------------------------------------------------
 */
static XClassHint  class_hint;


#ifdef _NO_EMACS_PROTO
static int
XT_Set_Class_Hints(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Class_Hints( Window  w )
#endif /* _NO_EMACS_PROTO */
{
    extern char  *getenv();


    if (XXidentity == (char *) NULL)
	XXidentity = "";	/* XSCH() doesn't like NULL pointers! */

    class_hint.res_name = XXidentity;
    class_hint.res_class = CLASS;

	
    XSetClassHint(XXdisplay, w, &class_hint);
}


/* ------------------------------------------------------------
 */
#ifdef _NO_EMACS_PROTO
static int
XT_Set_Command_Line(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Command_Line( Window  w )
#endif /* _NO_EMACS_PROTO */
{

    XSetCommand(XXdisplay, w, xargv, xargc);
}


/* ------------------------------------------------------------
 */
static char  hostname[100];

#ifdef _NO_EMACS_PROTO
static int
XT_Set_Host(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Host( Window  w )
#endif /* _NO_EMACS_PROTO */
{

    gethostname(hostname, 100);
    hostname[99] = '\0';

    XChangeProperty(XXdisplay, w, XA_WM_CLIENT_MACHINE, XA_STRING, 8,
		    PropModeReplace,
		    (unsigned char *) hostname, strlen(hostname));
}


/* ------------------------------------------------------------
 *  Set header title to window-name (from '-wn'), or if none,
 *  "optional-id: class-of-appl @ host"
 */
#ifdef _NO_EMACS_PROTO
static int
XT_Set_Title(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Title( Window  w )
#endif /* _NO_EMACS_PROTO */
{
    char  header_info[200];


    if (XXheader != (char *) NULL)
    {
	    strcpy(header_info, XXheader);
    }
    else
    {
	char  *next;

	next = header_info;
	
	if (strlen(class_hint.res_name) != 0)
	{
	    sprintf(header_info, "%s: ",
		    class_hint.res_name);
	
	    next += strlen(header_info);
	}
	
	sprintf(next, "%s @ %s",
		class_hint.res_class,
		hostname);
    }


    XStoreName(XXdisplay, w, header_info);
}


/* ------------------------------------------------------------
 *  Set icon title to icon-name (from '-in'),
 *  or if none, to "invocation-or-class @ host".
 *
 */
#ifdef _NO_EMACS_PROTO
static int
XT_Set_Icon_Title(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Icon_Title( Window  w )
#endif /* _NO_EMACS_PROTO */
{
    char  title_info[100];

    if (XXicon_name != (char *) NULL)
    {
	    strcpy(title_info, XXicon_name);
    }
    else
    {
	if (strlen(class_hint.res_name) != 0)
	{
	    sprintf(title_info, "%s@", class_hint.res_name);
	}
	else
	{
	    sprintf(title_info, "%s@", class_hint.res_class);
	}

	strcat(title_info, hostname);
    }


    XChangeProperty(XXdisplay, w, XA_WM_ICON_NAME, XA_STRING, 8,
		    PropModeReplace,
		    (unsigned char *) title_info, strlen(title_info));
}


/* Arg PR carries value returned by XGeometry at startup, or 0.  */

#ifdef _NO_EMACS_PROTO
static int
XT_Set_Size_Hints(w, x, y, width, height, do_resize, pr)
    int  x, y;			/* only used at Startup: do_resize == FALSE */
    int  width, height;
    Window  w;
    Bool  do_resize;
    int pr;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Size_Hints(
    Window  w,
    int  x, int y,	/* only used at Startup: do_resize == FALSE */
    int  width, int height,
    Bool  do_resize,
    int pr )
#endif /* _NO_EMACS_PROTO */
{
#ifndef X11R4
    XSizeHints  sizehints;

    sizehints.flags = (pr & (WidthValue | HeightValue)) ? USSize : PSize;

    if (!do_resize)
      sizehints.flags |= (pr & (XValue | YValue)) ? USPosition : PPosition;

    sizehints.flags |= PResizeInc|PMinSize;


    sizehints.x = x;
    sizehints.y = y;
    sizehints.width = width*XXfontw + 2 * XXInternalBorder;
    sizehints.height = height*XXfonth + 2 * XXInternalBorder;

    pixelwidth = sizehints.width;
    pixelheight = sizehints.height;
    flexlines = height;


    change_screen_size (height, width, 0 - (do_resize == False), 0, 0);

    sizehints.width_inc = XXfontw;
    sizehints.height_inc = XXfonth;

    sizehints.min_width = XXfontw*MINWIDTH+2*XXInternalBorder;
    sizehints.min_height = XXfonth*MINHEIGHT+2*XXInternalBorder;

#if 0
    /* old, broken versions */
    sizehints.min_width = 3 * XXInternalBorder;
    sizehints.min_height = 3 * XXInternalBorder;
#endif

    XSetNormalHints(XXdisplay, w, &sizehints);

    if (do_resize)
    {
	XResizeWindow(XXdisplay, XXwindow, pixelwidth, pixelheight);
	XFlush(XXdisplay);
    }
#else /* not X11R4 */
    XSizeHints sizehints;
    XWindowChanges changes;
    unsigned int change_mask = 0;
    
    sizehints.flags = 0;
    
    if (! do_resize) {
	 changes.x = x;
	 changes.y = y;
	 sizehints.flags |= (pr & (XValue | YValue)) ? USPosition : PPosition;
	 change_mask |= CWX | CWY;
    }

    sizehints.base_width = 2 * XXInternalBorder;
    sizehints.base_height = 2 * XXInternalBorder;
    changes.width = sizehints.base_width + width * XXfontw;
    changes.height = sizehints.base_height + height * XXfonth;
    sizehints.flags |= ((pr & (WidthValue | HeightValue)) ? USSize : PSize) |
	 PBaseSize;

    /* If user has specified precise position, ... */
    if ((pr & XValue) || (pr & YValue))
      {
	sizehints.flags |= USSize | USPosition | PWinGravity;
	/* Tell window manager which corner to keep fixed.  */
	switch (pr & (XNegative | YNegative))
	  {
	  case 0:
	    sizehints.win_gravity = NorthWestGravity;
	    break;
	  case XNegative:
	    sizehints.win_gravity = NorthEastGravity;
	    break;
	  case YNegative:
	    sizehints.win_gravity = SouthWestGravity;
	    break;
	  default:
	    sizehints.win_gravity = SouthEastGravity;
	    break;
	  }
      }

    change_mask |= CWWidth | CWHeight;
    
    /*
     * NOTE: The sizehints.x, sizehints.y, sizehints.width and
     * sizehints.height fields are OBSOLETE according to the ICCC, and
     * no window manager should be considering them, even if USSize/PSize
     * and/or USPosition/PPosition are set.  Unfortunately, many
     * window managers consider them anyway, and programs like xprop
     * display their values when fetching the normal hints property
     * from the window.  Therefore, I set them here just to make
     * things a little bit more robust.
     */
    if (! do_resize) {
	 sizehints.x = x;
	 sizehints.y = y;
    }
    sizehints.width = changes.width;
    sizehints.height = changes.height;

    pixelwidth = sizehints.base_width;
    pixelheight = sizehints.base_height;
    flexlines = height;

    change_screen_size (height, width, 0 - (do_resize == False), 0, 0);

    sizehints.min_width = XXfontw * MINWIDTH + 2 * XXInternalBorder;
    sizehints.min_height = XXfonth * MINHEIGHT + 2 * XXInternalBorder;
    sizehints.flags |= PMinSize;

    sizehints.width_inc = XXfontw;
    sizehints.height_inc = XXfonth;
    sizehints.flags |= PResizeInc;

    XSetWMNormalHints(XXdisplay, w, &sizehints);
    XConfigureWindow(XXdisplay, w, change_mask, &changes);
#endif /* not X11R4 */
}


/* ------------------------------------------------------------
 */
/*ARGSUSED*/
#ifdef _NO_EMACS_PROTO
static int
XT_Set_Zoom_Sizes(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_Zoom_Sizes( Window  w )
#endif /* _NO_EMACS_PROTO */
{
}


/* ------------------------------------------------------------
 *  Set our state and icon parameters.
 */
#ifdef _NO_EMACS_PROTO
static int
XT_Set_WM_Hints(w)
    Window  w;
#else /* _NO_EMACS_PROTO */
static int
XT_Set_WM_Hints( Window  w )
#endif /* _NO_EMACS_PROTO */
{
    XWMHints  wmhints;

    wmhints.flags = InputHint | StateHint;
    if (XXicon_usebitmap)
	    wmhints.flags |= IconPixmapHint | IconMaskHint;

    wmhints.input = True;
    wmhints.initial_state = NormalState;

    SinkPixmap = XCreateBitmapFromData (XXdisplay, w,
					(char *)sink_bits, sink_width,
					sink_height);

    SinkMaskPixmap = XCreateBitmapFromData (XXdisplay, w,
					    (char *)sink_mask_bits,
					    sink_mask_width,
					    sink_mask_height);

    if (XXicon_usebitmap) {
	    wmhints.icon_pixmap = SinkPixmap;
	    wmhints.icon_mask = SinkMaskPixmap;
    }
    else {
	    wmhints.icon_pixmap = 0;
	    wmhints.icon_mask = 0;
    }

    XSetWMHints(XXdisplay, w, &wmhints);
}


/* ------------------------------------------------------------
 *  Change just the size of the window.
 */
#ifdef _NO_EMACS_PROTO
XSetWindowSize(rows, cols)
    int rows, cols;
#else /* _NO_EMACS_PROTO */
void
XSetWindowSize( int rows, int cols )
#endif /* _NO_EMACS_PROTO */
{
    XT_Set_Size_Hints(XXwindow, 0, 0, cols, rows, NO_MANAGER, 0);
}


/* ------------------------------------------------------------
 */
static int
XInitWindow ()
{
    extern int xargc;
    extern char **xargv;
    int x, y, width, height, pr;
    char  *dp;
    Window  desktop;
    XColor forec, backc;

 retry:
  fontinfo = XT_CalcForFont(XXcurrentfont);
  if (fontinfo == (XFontStruct *) NULL)
    {
      if (strcmp (XXcurrentfont, "fixed"))
	{
	  free (XXcurrentfont);
	  XXcurrentfont = (char *) xmalloc (6);
	  strcpy (XXcurrentfont, "fixed");
	  goto retry;
	}
      fatal ("X server unable to find requested font `%s'\n",
	     (XXcurrentfont == NULL) ? "(null)" :  XXcurrentfont);
    }

  pr = XGeometry (XXdisplay, 0, desiredwindow, default_window,
		  XXborder, XXfontw, XXfonth,
		  XXInternalBorder*2, XXInternalBorder*2,
		  &x, &y, &width, &height);

  /*  Which desktop do we start up on?
   */
  if ( (dp = getenv("WM_DESKTOP")) != (char *) NULL )
    {
      desktop = atoi(dp);
    }
  else
    {
      desktop = RootWindow(XXdisplay, DefaultScreen(XXdisplay));
    }

  XXwindow = XCreateSimpleWindow(XXdisplay, desktop,
				 x, y,
				 width*XXfontw + 2*XXInternalBorder,
				 height*XXfonth + 2*XXInternalBorder,
				 XXborder, brdr, back);
  if (!XXwindow)
    {
      fprintf (stderr, "Could not create X window!\n");
      fflush (stderr);
      exit (-97);
    }

  XXgcv.font = XXfid;
  XXgcv.foreground = fore;
  XXgcv.background = back;
  XXgc_norm = XCreateGC(XXdisplay, XXwindow,
			GCFont|GCForeground|GCBackground,
			&XXgcv);
  XXgcv.foreground = back;
  XXgcv.background = fore;
  XXgc_rev = XCreateGC(XXdisplay, XXwindow,
		       GCFont|GCForeground|GCBackground,
		       &XXgcv);
  XXgcv.foreground = back;
  XXgcv.background = curs;
  XXgc_curs = XCreateGC(XXdisplay, XXwindow,
			GCFont|GCForeground|GCBackground,
			&XXgcv);
  XXgcv.foreground = curs;
  XXgcv.background = back;
  XXgc_curs_rev = XCreateGC(XXdisplay, XXwindow,
			    GCFont|GCForeground|GCBackground,
			    &XXgcv);

  EmacsCursor = XCreateFontCursor(XXdisplay, XC_left_ptr);

  x_set_cursor_colors ();

  XDefineCursor (XXdisplay, XXwindow, EmacsCursor);

  CursorExists = 0;
  CursorOutline = 1;
  VisibleX = 0;
  VisibleY = 0;


  XT_Set_Class_Hints(XXwindow);
  XT_Set_Command_Line(XXwindow);
  XT_Set_Host(XXwindow);
  XT_Set_Title(XXwindow);
  XT_Set_Icon_Title(XXwindow);
  XT_Set_Size_Hints(XXwindow, x, y, width, height, False, pr);
  XT_Set_Zoom_Sizes(XXwindow);
  XT_Set_WM_Hints(XXwindow);

  XSelectInput(XXdisplay, XXwindow, KeyPressMask |
	       ExposureMask | ButtonPressMask | ButtonReleaseMask |
	       EnterWindowMask | LeaveWindowMask | FocusChangeMask |
	       StructureNotifyMask);

  XMapWindow (XXdisplay, XXwindow);
  XFlush (XXdisplay);

#ifdef AIX          
#include "xkeys-aix.h"
#endif				/* AIX */

  /* Free XrmGetStringDatabase */

#ifndef NO_X_DESTROY_DATABASE
  XrmDestroyDatabase (db);
#if (XlibSpecificationRelease >= 5)
  XrmDestroyDatabase (db2);
#endif
#endif
}

#endif /* HAVE_X_WINDOWS */

/*#include "xundebug.h"*/
