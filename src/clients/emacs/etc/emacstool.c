/* Copyright (C) 1986, 1988, 1990, 1991 Free Software Foundation, Inc.

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

/* For Emacs in SunView/Sun-Windows: (supported by Sun Unix v3.2 or greater)
 * Insert a notifier filter-function to convert all useful input 
 * to "key" sequences that emacs can understand.  See: Emacstool(1).
 *
 * Author: Jeff Peck, Sun Microsystems, Inc. <peck@eng.sun.com>
 *
 * Original Idea: Ian Batten
 * Updated 15-Mar-88, Jeff Peck: set IN_EMACSTOOL, TERM, TERMCAP
 * Updated 10-Sep-88, Jeff Peck: add XVIEW and JLE support
 * Updated  8-Oct-90, Jeff Peck: add Meta-bit for Xview
 * Updated  6-Mar-91, Jeff Peck: Hack to detect -Wt invocation
 *	[note, TTYSW limitation means you must Click-To-Type in Openwin]
 *	[fixed in OW3 or use local/tty.o]
 *	for better results, this should move to using TERMSW.
 * Updated 30-Mar-91, Jeff Peck, et al: Enable using TERMSW (-DTTERM)
 *	TERMSW understands point-to-type, even in OW2.
 * Updated  5-Jun-91, Jeff Peck: Meta-key support fixed for OWv3
 *				 Better event diagnostics for DEBUGEMACSTOOL
 *
 * 	[note: xvetool should be started with the "-nw" flag for emacs!]
 */

#ifdef XVIEW
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/attr.h>
#include <xview/tty.h>
#include <xview/ttysw.h>		/* private defines */
#include <xview/termsw.h>		/* -DTTERM */
#include <xview/font.h>			/* for testing */
#else
#include <suntool/sunview.h>
#include <suntool/tty.h>
#include <suntool/ttysw.h>
#endif /* XVIEW */

#ifdef JLE
# include <locale.h>
#endif /* JLE */

#include <stdio.h>
#include <sys/file.h>

#define BUFFER_SIZE 128               	/* Size of all the buffers */

/* define WANT_CAPS_LOCK to make f-key T1 (aka F1) behave as CapsLock */
#define WANT_CAPS_LOCK
#ifdef WANT_CAPS_LOCK
int caps_lock;		/* toggle indicater for f-key T1 caps lock */
static char *Caps = "[CAPS] ";		/* Caps Lock prefix string */
#define CAPS_LEN 7			/* strlen (Caps) */
#endif

static char *mouse_prefix = "\030\000";	/* C-x C-@ */
static int   m_prefix_length = 2;       /* mouse_prefix length */

static char *key_prefix = "\030*";  	/* C-x *   */
static int   k_prefix_length = 2;       /* key_prefix length */

#ifdef JLE
static char *emacs_name = "nemacs";	/* default run command */
static char *title = "NEmacstool - ";	/* initial title */
#else
static char *emacs_name = "emacs";	/* default run command */
static char *title = "Emacstool - ";	/* initial title */
#endif /* JLE */

static char buffer[BUFFER_SIZE];	/* send to ttysw_input */
static char *bold_name = 0;	 	/* for -bold option */

Frame frame;                            /* Base frame for system */

#ifndef TTERM
#define SWTYPE TTY
Tty tty_win;				/* Where emacs is reading */
#else
#define SWTYPE TERMSW
Termsw tty_win;				/* Termsw does follow-mouse */
#endif /* TTERM */

#ifdef XVIEW
Xv_Window tty_view;			/* Where the events are in Xview*/
#else
Tty tty_view;				/* SunView place filler */
#endif /* XVIEW */

int font_width, font_height;            /* For translating pixels to chars */
int left_margin = 0;		/* default window -- frame offset */

int console_fd = 0;		/* for debugging: setenv DEBUGEMACSTOOL */
FILE *console;			/* for debugging: setenv DEBUGEMACSTOOL */

Icon frame_icon;
/* make an icon_image for the default frame_icon */
static short default_image[258] = 
{
#include <images/terminal.icon>
};
mpr_static(icon_image, 64, 64, 1, default_image);

/*
 * Assign a value to a set of keys
 */
int
button_value (event)
     Event *event;
{
  int retval = 0;
  /*
   * Code up the current situation:
   *
   * 1 = MS_LEFT;
   * 2 = MS_MIDDLE;
   * 4 = MS_RIGHT;
   * 8 = SHIFT;
   * 16 = CONTROL;
   * 32 = META;
   * 64 = DOUBLE;
   * 128 = UP;
   */

  if (MS_LEFT   == (event_id (event))) retval = 1;
  if (MS_MIDDLE == (event_id (event))) retval = 2;
  if (MS_RIGHT  == (event_id (event))) retval = 4;

  if (event_shift_is_down (event)) retval += 8;
  if (event_ctrl_is_down  (event)) retval += 16;
  if (event_meta_is_down  (event)) retval += 32;
  if (event_is_up         (event)) retval += 128;
  return retval;
}

/*
 *  Variables to store the time of the previous mouse event that was
 *  sent to emacs.
 *
 *  The theory is that to time double clicks while ignoreing UP buttons,
 *  we must keep track of the accumulated time.
 *
 *  If someone writes a SUN-SET-INPUT-MASK for emacstool,
 *  That could be used to selectively disable UP events, 
 *  and then this cruft wouldn't be necessary.
 */
static long prev_event_sec = 0;
static long prev_event_usec = 0;

/*
 *  Give the time difference in milliseconds, where one second
 *  is considered infinite.
 */
int
time_delta (now_sec, now_usec, prev_sec, prev_usec)
     long now_sec, now_usec, prev_sec, prev_usec;
{
  long sec_delta = now_sec - prev_sec;
  long usec_delta = now_usec - prev_usec;
  
  if (usec_delta < 0) {		/* "borrow" a second */
    usec_delta += 1000000;
    --sec_delta;
  }
  
  if (sec_delta >= 10) 
    return (9999);		/* Infinity */
  else
    return ((sec_delta * 1000) + (usec_delta / 1000));
}


/*
 * Filter function to translate selected input events for emacs
 * Mouse button events become ^X^@(button x-col y-line time-delta) .
 * Function keys: ESC-*{c}{lrt} l,r,t for Left, Right, Top; 
 * {c} encodes the keynumber as a character [a-o]
 */
static Notify_value
input_event_filter_function (window, event, arg, type)
#ifdef XVIEW
     Xv_Window window;
#else /* not XVIEW */
     Window window;
#endif /* XVIEW */
     Event *event;
     Notify_arg arg;
     Notify_event_type type;
{
  struct timeval time_stamp;

  /* if DEBUGEMACSTOOL is set, printout event information */
  if (console_fd) {
      fprintf(console, "Event: %s%s%c %d %d\n",
	      (event_ctrl_is_down(event) ? "C-" : "  "),
	      (event_meta_is_down(event) ? "M-" : "  "),
	      (((event_is_button    (event)) ? 'M' :
		((event_is_key_left  (event)) ? 'L' : 
		 ((event_is_key_right (event)) ? 'R' : 
		  ((event_is_key_top   (event)) ? 'T' :
		   (((ASCII_FIRST <= event_id(event))
		     && (event_id(event) <= ASCII_LAST)) ? 
		    (((127 & event_id(event)) < 32) ?
		     ((127 & event_id(event)) + 64) :
		     ((127 & event_id(event)))) : '?')))))),
	      event_id(event),
	      event_action(event));
  }
  /* UP L1 is the STOP key */
  if (event_id(event) == WIN_STOP) {
      ttysw_input(tty_win, "\007\007\007\007\007\007\007", 7);
      return NOTIFY_IGNORED;
  }

  /* UP L5 & L7 is Expose & Open, let them pass to sunview */
  if (event_id(event) == KEY_LEFT(5) || event_id(event) == KEY_LEFT(7)) 
      if (event_is_up (event)) 
	  return notify_next_event_func (window, event, arg, type);
      else 
	  return NOTIFY_IGNORED;


  { /* Do the mouse == button events */
      if (event_is_button (event)) { /* do Mouse Button events */
	  time_stamp = event_time (event);
	  ttysw_input (tty_win, mouse_prefix, m_prefix_length);
	  sprintf (buffer, "(%d %d %d %d)\015", 
		   button_value (event),
		   (event_x (event) - left_margin) / font_width,
		   event_y (event) / font_height,
		   time_delta (time_stamp.tv_sec, time_stamp.tv_usec,
			       prev_event_sec, prev_event_usec)
		   );
	  ttysw_input (tty_win, buffer, strlen(buffer));
	  prev_event_sec = time_stamp.tv_sec;
	  prev_event_usec = time_stamp.tv_usec;
	  return NOTIFY_IGNORED; 
      }
  }
  { /* Do the function key events */
      int d;
      char c = (char) 0;
      if ((event_is_key_left  (event)) ?
	  ((d = event_id(event) - KEY_LEFT(1)   + 'a'), c='l') : 
	  ((event_is_key_right (event)) ?
	   ((d = event_id(event) - KEY_RIGHT(1) + 'a'), c='r') : 
	   ((event_is_key_top   (event)) ?
	    ((d = event_id(event) - KEY_TOP(1)  + 'a'), c='t') : 0))) {
	  if (event_is_up(event)) return NOTIFY_IGNORED;
	  if (event_shift_is_down (event)) c = c -  32;
	  /* this will give a non-{lrt} for unshifted keys */
	  if (event_ctrl_is_down  (event)) c = c -  64;
	  if (event_meta_is_down  (event)) c = c + 128;
#ifdef WANT_CAPS_LOCK
	  /* set a toggle and relabel window so T1 can act like caps-lock */
	  if (event_id(event) == KEY_TOP(1)) {
	      /* make a frame label with and without CAPS */
	      strcpy (buffer, Caps); 
	      title = &buffer[CAPS_LEN];
	      strncpy (title, (char *)window_get (frame, FRAME_LABEL),
		       BUFFER_SIZE - CAPS_LEN);
	      buffer[BUFFER_SIZE] = (char) 0;	
	      if (strncmp (title, Caps, CAPS_LEN) == 0)
		  title += CAPS_LEN; /* already Caps */
	      caps_lock =  (caps_lock ? 0 : CAPS_LEN);
	      window_set(frame, FRAME_LABEL, (title -= caps_lock), 0);
	      return NOTIFY_IGNORED;
	  }
#endif /* WANT_CAPS_LOCK */
	  ttysw_input (tty_win, key_prefix, k_prefix_length);
	  sprintf (buffer, "%c%c", d, c);
	  ttysw_input(tty_win, buffer, strlen(buffer));

	  return NOTIFY_IGNORED;
      }
  }

  /* handle ascii keyboard events
   * extract "event_is_ascii(event)" from the event_id, not the event_action
   */
  if ((ASCII_FIRST <= event_id(event)) && (event_id(event) <= ASCII_LAST)) {
      /* ignore key-up events (button events have already been handled) */
      if (event_is_up(event)) return NOTIFY_IGNORED;
#ifdef WANT_CAPS_LOCK
      /* shift alpha chars to upper case if toggle is set */
      if ((caps_lock) && (event_id(event) >= 'a') && (event_id(event) <= 'z'))
	  event_set_id(event, (event_id(event) - 32));
#endif /* WANT_CAPS_LOCK */
      
#ifndef NO_META_BIT
      /* under Openwindows/X, the meta bit is not set in the key event,
       * emacs expects this so we add it in here:
       */
      if (event_meta_is_down(event))
	  event_set_id(event, 128 | event_id(event));
#endif /* NO_META_BIT */
  }
  return notify_next_event_func (window, event, arg, type);
}

main (argc, argv)
     int argc;
     char **argv;
{
  int error_code;	/* Error codes */
  
#ifdef JLE
  setlocale(LC_ALL, "");
#endif /* JLE */

  if(getenv("DEBUGEMACSTOOL"))
    console = fdopen (console_fd = open("/dev/console",O_WRONLY), "w");

  putenv("IN_EMACSTOOL=t");	/* notify subprocess that it is in emacstool */

  if (putenv("TERM=sun") != 0)	/* TTY_WIN will be a TERM=sun window */
    {fprintf (stderr, "%s: Could not set TERM=sun, using `%s'\n",
	     argv[0], (char *)getenv("TERM")) ;};
  /*
   * If TERMCAP starts with a slash, it is the pathname of the
   * termcap file, not an entry extracted from it, so KEEP it!
   * Otherwise, it may not relate to the new TERM, so Nuke-It.
   * If there is no TERMCAP environment variable, don't make one.
   */
  {
    char *termcap ;	/* Current TERMCAP value */
    termcap = (char *)getenv("TERMCAP") ;
    if (termcap && (*termcap != '/'))
      {
	if (putenv("TERMCAP=") != 0)
	  {fprintf (stderr, "%s: Could not clear TERMCAP\n", argv[0]) ;} ;
      } ;
  } ;
  
  /* find command to run as subprocess in window */
  if (!(argv[0] = (char *)getenv("EMACSTOOL")))	/*  Set emacs command name */
      argv[0] = emacs_name;			
  /* Emacstool recognizes two special args: -rc <file> and -bold <bold-name> */
  for (argc = 1; argv[argc]; argc++)		/* Use last one on line */
    {
      if(!(strcmp ("-rc", argv[argc])))		/* Override if -rc given */
	{int i = argc;
	 argv[argc--]=0;		/* kill the -rc argument */
	 if (argv[i+1]) {	/* move to argv[0] and squeeze the rest */
	   argv[0]=argv[i+1];
	   for (; argv[i+2]; (argv[i]=argv[i+2],argv[++i]=0));
	 }
       }

      if (!(strcmp ("-bold", argv[argc]))) 
	  {int i = argc;
	   argv[argc--]=0;		/* kill the -bold argument */
	   if (argv[i+1]) {	/* move to bold_name and squeeze the rest */
	       bold_name = argv[i+1];
	       for (; argv[i+2]; (argv[i]=argv[i+2],argv[++i]=0));
	   }
       }
  };

  strcpy (buffer, title);
  strncat (buffer, argv[0],		 /* append run command name */
	   (BUFFER_SIZE - (strlen (buffer)) - (strlen (argv[0]))) - 1);

  error_code = interpose_on_window(argc,argv);
  if (error_code != 0) {		/* Barf */
      fprintf (stderr, "notify_interpose_event_func returns %d.\n", error_code);
      exit (1);
  }

#ifdef XVIEW
  xv_main_loop (frame);                  /* And away we go */
#else
  window_main_loop (frame);
#endif /* XVIEW */
}

#ifdef XVIEW
int interpose_on_window(argc,argv)
    int argc;
    char **argv;
{
#ifndef TTERM
#ifdef FONT_WIDTH_ADJUST
    int i, font_width_adjust = 1; /* hackery, and hueristics */
    /* If -Wt is not supplied, OWv2 font defaults as lucidasans-12 (width=8)
     * rather than the lucidasanstypewriter (width=7) actually used by ttysw.
     * This hack attempts to workaround it.
     */
    for (i = 1; argv[i]; i++) {
	if (!(strcmp ("-Wt", argv[i])) || !(strcmp ("-font", argv[i])))
	    {font_width_adjust = 0;
	     if (console_fd) fprintf(console, "-Wt = %d\n", font_width_adjust);
	     break;}
    }
#endif /* FONT_WIDTH_ADJUST */
#endif /* not TTERM */
    /* initialize Xview, and strip window args */
    xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);

    /* do this first, so arglist can override it */
    frame_icon = icon_create (ICON_LABEL, "Emacstool",
			      ICON_IMAGE, &icon_image,
			      0);

    /* Build a frame to run in */
    frame = xv_create ((Xv_Window)NULL, FRAME,
		       FRAME_LABEL, buffer,
		       FRAME_ICON, frame_icon,
		       0);

    /* Create a tty with emacs in it */
    tty_win = xv_create (frame, SWTYPE, WIN_IS_CLIENT_PANE,
			 TTY_QUIT_ON_CHILD_DEATH, TRUE, 
			 TTY_BOLDSTYLE, TTYSW_BOLD_INVERT,
			 TTY_ARGV, argv, 
			 0);

    if (bold_name) {
	(void)xv_set(tty_win, TTY_BOLDSTYLE_NAME, bold_name, 0);
    }

    {
	Xv_font font;		/* declare temp font variable */
	font = (Xv_font)xv_get (tty_win, XV_FONT);
	font_height = (int)xv_get (font, FONT_DEFAULT_CHAR_HEIGHT);
	font_width  = (int)xv_get (font, FONT_DEFAULT_CHAR_WIDTH);
    }
    if (console_fd) fprintf(console, "Width = %d\n", font_width);

#ifndef TTERM
#ifdef FONT_WIDTH_ADJUST
    font_width -= font_width_adjust; /* A guess! font cache bug in ttysw*/
#endif /* FONT_WIDTH_ADJUST */
#else
    /* make the termsw act as a tty */
    xv_set(tty_win, TERMSW_MODE, TTYSW_MODE_TYPE, 0);
    /* termsw has variable offset depending on scrollbar size/location */
    left_margin = (int)xv_get (tty_win, TEXTSW_LEFT_MARGIN);
#endif /* TTERM */

    tty_view = (Xv_Window) xv_get (tty_win, OPENWIN_NTH_VIEW, 0);
    xv_set(tty_view,
	   WIN_CONSUME_EVENTS, 
	   WIN_MOUSE_BUTTONS, WIN_UP_EVENTS,
	   ACTION_ADJUST, ACTION_MENU,
	   WIN_ASCII_EVENTS, 
	   WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS,
	   0,
	   0);
    /* Interpose my event function */
    return (int) notify_interpose_event_func 
	(tty_view, input_event_filter_function, NOTIFY_SAFE);
}
#else /* not XVIEW */
int interpose_on_window (argc, argv)
 int argc;
 char **argv;
{
    /* do this first, so arglist can override it */
    frame_icon = icon_create (ICON_LABEL, "Emacstool",
			      ICON_IMAGE, &icon_image,
			      0);

    /* Build a frame to run in */
    frame = window_create ((Window)NULL, FRAME,
			   FRAME_LABEL, buffer,
			   FRAME_ICON, frame_icon,
			   FRAME_ARGC_PTR_ARGV, &argc, argv,
			   0);

    /* Create a tty with emacs in it */
    tty_win = window_create (frame, TTY, 
			     TTY_QUIT_ON_CHILD_DEATH, TRUE, 
			     TTY_BOLDSTYLE, TTYSW_BOLD_INVERT,
			     TTY_ARGV, argv, 
			     0);

    if (bold_name) {
	(void)window_set(tty_win, TTY_BOLDSTYLE_NAME, bold_name, 0);
    }

    /* ttysw uses pf_default, one must set WIN_FONT explicitly */
                       window_set (tty_win, WIN_FONT, pf_default(), 0);
    font_height = (int)window_get (tty_win, WIN_ROW_HEIGHT);
    font_width  = (int)window_get (tty_win, WIN_COLUMN_WIDTH);

    tty_view = tty_win;
    window_set(tty_view,
	       WIN_CONSUME_PICK_EVENTS, 
	       WIN_STOP,
	       WIN_MOUSE_BUTTONS, WIN_UP_EVENTS,
	       /* LOC_WINENTER, LOC_WINEXIT, LOC_MOVE, */
	       0,
	       WIN_CONSUME_KBD_EVENTS, 
	       WIN_STOP,
	       WIN_ASCII_EVENTS, 
	       WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS,
	       /* WIN_UP_ASCII_EVENTS, */
	       0,
	       0);
    /* Interpose my event function */
    return (int) notify_interpose_event_func 
	(tty_view, input_event_filter_function, NOTIFY_SAFE);
}
#endif /* XVIEW */
