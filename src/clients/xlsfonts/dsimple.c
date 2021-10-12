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
/* $XConsortium: dsimple.c,v 1.12 91/05/11 21:01:05 gildea Exp $ */
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

unsigned long Resolve_Color();
Pixmap Bitmap_To_Pixmap();
Window Select_Window();
void out();
void blip();
Window Window_With_Name();
void Fatal_Error();

/*
 * Just_display: A group of routines designed to make the writting of simple
 *               X11 applications which open a display but do not open
 *               any windows much faster and easier.  Unless a routine says
 *               otherwise, it may be assumed to require program_name, dpy,
 *               and screen already defined on entry.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */


/* This stuff is defined in the calling program by just_display.h */
extern char *program_name;
extern Display *dpy;
extern int screen;


/*
 * Malloc: like malloc but handles out of memory using Fatal_Error.
 */
char *Malloc(size)
     unsigned size;
{
	char *data, *malloc();

	if (!(data = malloc(size)))
	  Fatal_Error("Out of memory!");

	return(data);
}
	

/*
 * Realloc: like Malloc except for realloc, handles NULL using Malloc.
 */
char *Realloc(ptr, size)
        char *ptr;
        int size;
{
	char *new_ptr, *realloc();

	if (!ptr)
	  return(Malloc(size));

	if (!(new_ptr = realloc(ptr, size)))
	  Fatal_Error("Out of memory!");

	return(new_ptr);
}


/*
 * Get_Display_Name (argc, argv) Look for -display, -d, or host:dpy (obselete)
 * If found, remove it from command line.  Don't go past a lone -.
 */
char *Get_Display_Name(pargc, argv)
    int *pargc;  /* MODIFIED */
    char **argv; /* MODIFIED */
{
    int argc = *pargc;
    char **pargv = argv+1;
    char *displayname = NULL;
    int i;

    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (!strcmp (arg, "-display") || !strcmp (arg, "-d")) {
	    if (++i >= argc) usage ();

	    displayname = argv[i];
	    *pargc -= 2;
	    continue;
	}
	if (!strcmp(arg,"-")) {
		while (i<argc)
			*pargv++ = argv[i++];
		break;
	}
	*pargv++ = arg;
    }

    *pargv = NULL;
    return (displayname);
}


/*
 * Open_Display: Routine to open a display with correct error handling.
 *               Does not require dpy or screen defined on entry.
 */
Display *Open_Display(display_name)
char *display_name;
{
	Display *d;

	d = XOpenDisplay(display_name);
	if (d == NULL) {
	    fprintf (stderr, "%s:  unable to open display '%s'\n",
		     program_name, XDisplayName (display_name));
	    usage ();
	    /* doesn't return */
	}

	return(d);
}


/*
 * Setup_Display_And_Screen: This routine opens up the correct display (i.e.,
 *                           it calls Get_Display_Name) and then stores a
 *                           pointer to it in dpy.  The default screen
 *                           for this display is then stored in screen.
 *                           Does not require dpy or screen defined.
 */
void Setup_Display_And_Screen(argc, argv)
int *argc;      /* MODIFIED */
char **argv;    /* MODIFIED */
{
	dpy = Open_Display (Get_Display_Name(argc, argv));
	screen = DefaultScreen(dpy);
}


/*
 * Open_Font: This routine opens a font with error handling.
 */
XFontStruct *Open_Font(name)
char *name;
{
	XFontStruct *font;

	if (!(font=XLoadQueryFont(dpy, name)))
	  Fatal_Error("Unable to open font %s!", name);

	return(font);
}


/*
 * Beep: Routine to beep the display.
 */
void Beep()
{
	XBell(dpy, 50);
}


/*
 * ReadBitmapFile: same as XReadBitmapFile except it returns the bitmap
 *                 directly and handles errors using Fatal_Error.
 */
static void _bitmap_error(status, filename)
     int status;
     char *filename;
{
  if (status == BitmapOpenFailed)
    Fatal_Error("Can't open file %s!", filename);
  else if (status == BitmapFileInvalid)
    Fatal_Error("file %s: Bad bitmap format.", filename);
  else
    Fatal_Error("Out of memory!");
}

Pixmap ReadBitmapFile(d, filename, width, height, x_hot, y_hot)
     Drawable d;
     char *filename;
     int *width, *height, *x_hot, *y_hot;
{
  Pixmap bitmap;
  int status;

  status = XReadBitmapFile(dpy, RootWindow(dpy, screen), filename,
			   (unsigned int *)width, (unsigned int *)height,
			   &bitmap, x_hot, y_hot);
  if (status != BitmapSuccess)
    _bitmap_error(status, filename);

  return(bitmap);
}


/*
 * WriteBitmapFile: same as XWriteBitmapFile except it handles errors
 *                  using Fatal_Error.
 */
void WriteBitmapFile(filename, bitmap, width, height, x_hot, y_hot)
     char *filename;
     Pixmap bitmap;
     int width, height, x_hot, y_hot;
{
  int status;

  status= XWriteBitmapFile(dpy, filename, bitmap, width, height, x_hot,
			   y_hot);
  if (status != BitmapSuccess)
    _bitmap_error(status, filename);
}


/*
 * Select_Window_Args: a rountine to provide a common interface for
 *                     applications that need to allow the user to select one
 *                     window on the screen for special consideration.
 *                     This routine implements the following command line
 *                     arguments:
 *
 *                       -root            Selects the root window.
 *                       -id <id>         Selects window with id <id>. <id> may
 *                                        be either in decimal or hex.
 *                       -name <name>     Selects the window with name <name>.
 *
 *                     Call as Select_Window_Args(&argc, argv) in main before
 *                     parsing any of your program's command line arguments.
 *                     Select_Window_Args will remove its arguments so that
 *                     your program does not have to worry about them.
 *                     The window returned is the window selected or 0 if
 *                     none of the above arguments was present.  If 0 is
 *                     returned, Select_Window should probably be called after
 *                     all command line arguments, and other setup is done.
 *                     For examples of usage, see xwininfo, xwd, or xprop.
 */
Window Select_Window_Args(rargc, argv)
     int *rargc;
     char **argv;
#define ARGC (*rargc)
{
	int nargc=1;
	int argc;
	char **nargv;
	Window w=0;

	nargv = argv+1; argc = ARGC;
#define OPTION argv[0]
#define NXTOPTP ++argv, --argc>0
#define NXTOPT if (++argv, --argc==0) usage()
#define COPYOPT nargv++[0]=OPTION; nargc++

	while (NXTOPTP) {
		if (!strcmp(OPTION, "-")) {
			COPYOPT;
			while (NXTOPTP)
			  COPYOPT;
			break;
		}
		if (!strcmp(OPTION, "-root")) {
			w=RootWindow(dpy, screen);
			continue;
		}
		if (!strcmp(OPTION, "-name")) {
			NXTOPT;
			w = Window_With_Name(dpy, RootWindow(dpy, screen),
					     OPTION);
			if (!w)
			  Fatal_Error("No window with name %s exists!",OPTION);
			continue;
		}
		if (!strcmp(OPTION, "-id")) {
			NXTOPT;
			w=0;
			sscanf(OPTION, "0x%lx", &w);
			if (!w)
			  sscanf(OPTION, "%ld", &w);
			if (!w)
			  Fatal_Error("Invalid window id format: %s.", OPTION);
			continue;
		}
		COPYOPT;
	}
	ARGC = nargc;
	
	return(w);
}

/*
 * Other_stuff: A group of routines which do common X11 tasks.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

extern Display *dpy;
extern int screen;

/*
 * Resolve_Color: This routine takes a color name and returns the pixel #
 *                that when used in the window w will be of color name.
 *                (WARNING:  The colormap of w MAY be modified! )
 *                If colors are run out of, only the first n colors will be
 *                as correct as the hardware can make them where n depends
 *                on the display.  This routine does not require wind to
 *                be defined.
 */
unsigned long Resolve_Color(w, name)
     Window w;
     char *name;
{
	XColor c;
	Colormap colormap;
	XWindowAttributes wind_info;

	/*
	 * The following is a hack to insure machines without a rgb table
	 * handle at least white & black right.
	 */
	if (!strcmp(name, "white"))
	  name="#ffffffffffff";
	if (!strcmp(name, "black"))
	  name="#000000000000";

	XGetWindowAttributes(dpy, w, &wind_info);
	colormap = wind_info.colormap;

	if (!XParseColor(dpy, colormap, name, &c))
	  Fatal_Error("Bad color format '%s'.", name);

	if (!XAllocColor(dpy, colormap, &c))
	  Fatal_Error("XAllocColor failed!");

	return(c.pixel);
}


/*
 * Bitmap_To_Pixmap: Convert a bitmap to a 2 colored pixmap.  The colors come
 *                   from the foreground and background colors of the gc.
 *                   Width and height are required solely for efficiency.
 *                   If needed, they can be obtained via. XGetGeometry.
 */
Pixmap Bitmap_To_Pixmap(dpy, d, gc, bitmap, width, height)
     Display *dpy;
     Drawable d;
     GC gc;
     Pixmap bitmap;
     int width, height;
{
  Pixmap pix;
  int x;
  unsigned int i, depth;
  Drawable root;

  if (!XGetGeometry(dpy, d, &root, &x, &x, &i, &i, &i, &depth))
    return(0);

  pix = XCreatePixmap(dpy, d, width, height, (int)depth);

  XCopyPlane(dpy, bitmap, pix, gc, 0, 0, width, height, 0, 0, 1);

  return(pix);
}


/*
 * blip: a debugging routine.  Prints Blip! on stderr with flushing. 
 */
void blip()
{
  outl("blip!");
}


/*
 * Routine to let user select a window using the mouse
 */

Window Select_Window(dpy)
     Display *dpy;
{
  int status;
  Cursor cursor;
  XEvent event;
  Window target_win = None, root = RootWindow(dpy,screen);
  int buttons = 0;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
			ButtonPressMask|ButtonReleaseMask, GrabModeSync,
			GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  while ((target_win == None) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
      if (target_win == None) {
	target_win = event.xbutton.subwindow; /* window selected */
	if (target_win == None) target_win = root;
      }
      buttons++;
      break;
    case ButtonRelease:
      if (buttons > 0) /* there may have been some down before we started */
	buttons--;
       break;
    }
  } 

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

  return(target_win);
}


/*
 * Window_With_Name: routine to locate a window with a given name on a display.
 *                   If no window with the given name is found, 0 is returned.
 *                   If more than one window has the given name, the first
 *                   one found will be returned.  Only top and its subwindows
 *                   are looked at.  Normally, top should be the RootWindow.
 */
Window Window_With_Name(dpy, top, name)
     Display *dpy;
     Window top;
     char *name;
{
	Window *children, dummy;
	unsigned int nchildren;
	int i;
	Window w=0;
	char *window_name;

	if (XFetchName(dpy, top, &window_name) && !strcmp(window_name, name))
	  return(top);

	if (!XQueryTree(dpy, top, &dummy, &dummy, &children, &nchildren))
	  return(0);

	for (i=0; i<nchildren; i++) {
		w = Window_With_Name(dpy, children[i], name);
		if (w)
		  break;
	}
	if (children) XFree ((char *)children);
	return(w);
}

/*
 * outl: a debugging routine.  Flushes stdout then prints a message on stderr
 *       and flushes stderr.  Used to print messages when past certain points
 *       in code so we can tell where we are.  Outl may be invoked like
 *       printf with up to 7 arguments.
 */
/* VARARGS1 */
outl(msg, arg0,arg1,arg2,arg3,arg4,arg5,arg6)
     char *msg;
     char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fflush(stdout);
	fprintf(stderr, msg, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	fprintf(stderr, "\n");
	fflush(stderr);
}


/*
 * Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
/* VARARGS1 */
void Fatal_Error(msg, arg0,arg1,arg2,arg3,arg4,arg5,arg6)
char *msg;
char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "%s: error: ", program_name);
	fprintf(stderr, msg, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	fprintf(stderr, "\n");
	exit(1);
}
