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
/* $XConsortium: Eyes.c,v 1.26 91/08/23 12:26:40 gildea Exp $ */
/*
 * Copyright 1991 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Eyes.c
 *
 * a widget which follows the mouse around
 */

# include <X11/Xos.h>
# include <stdio.h>
# include <X11/IntrinsicP.h>
# include <X11/StringDefs.h>
# include <X11/Xmu/Converters.h>
# include "EyesP.h"
# include <math.h>
# include <X11/extensions/shape.h>

#if (defined(SVR4) || defined(SYSV) && defined(SYSV386)) && __STDC__
extern double hypot(double, double);
#endif

#define offset(field) XtOffsetOf(EyesRec, eyes.field)
#define goffset(field) XtOffsetOf(WidgetRec, core.field)

static XtResource resources[] = {
    {XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
	goffset(width), XtRImmediate, (XtPointer) 150},
    {XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
	goffset(height), XtRImmediate, (XtPointer) 100},
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(puppixel), XtRString, XtDefaultForeground},
    {XtNoutline, XtCForeground, XtRPixel, sizeof(Pixel),
        offset(outline), XtRString, XtDefaultForeground},
    {XtNcenterColor, XtCBackground, XtRPixel, sizeof (Pixel),
    	offset(center), XtRString, XtDefaultBackground},
    {XtNreverseVideo, XtCReverseVideo, XtRBoolean, sizeof (Boolean),
	offset (reverse_video), XtRImmediate, (XtPointer) FALSE},
    {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof (int),
    	offset (backing_store), XtRString, "default"},
    {XtNshapeWindow, XtCShapeWindow, XtRBoolean, sizeof (Boolean),
	offset (shape_window), XtRImmediate, (XtPointer) TRUE},
};

#undef offset
#undef goffset

# define NUM_EYES	2
# define EYE_X(n)	((n) * 2.0)
# define EYE_Y(n)	(0.0)
# define EYE_OFFSET	(0.1)	/* padding between eyes */
# define EYE_THICK	(0.175)	/* thickness of eye rim */
# define BALL_WIDTH	(0.3)
# define BALL_PAD	(0.05)
# define EYE_WIDTH	(2.0 - (EYE_THICK + EYE_OFFSET) * 2)
# define EYE_HEIGHT	EYE_WIDTH
# define EYE_HWIDTH	(EYE_WIDTH / 2.0)
# define EYE_HHEIGHT	(EYE_HEIGHT / 2.0)
# define BALL_HEIGHT	BALL_WIDTH
# define BALL_DIST	((EYE_WIDTH - BALL_WIDTH) / 2.0 - BALL_PAD)
# define W_MIN_X	(-1.0 + EYE_OFFSET)
# define W_MAX_X	(3.0 - EYE_OFFSET)
# define W_MIN_Y	(-1.0 + EYE_OFFSET)
# define W_MAX_Y	(1.0 - EYE_OFFSET)

# define TPointEqual(a, b)  ((a).x == (b).x && (a).y == (b).y)
# define XPointEqual(a, b)  ((a).x == (b).x && (a).y == (b).y)

static int delays[] = { 50, 100, 200, 400, 0 };

static void Initialize(), Realize(), Destroy();
static void Redisplay(), Resize ();
static Boolean SetValues();
static int repaint_window();
static void draw_it ();

static void ClassInitialize();

EyesClassRec eyesClassRec = {
    { /* core fields */
    /* superclass		*/	&widgetClassRec,
    /* class_name		*/	"Eyes",
    /* size			*/	sizeof(EyesRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	Resize,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	NULL,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry		*/	XtInheritQueryGeometry,
    }
};

static void ClassInitialize()
{
    XtAddConverter( XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
		    NULL, 0 );
}

WidgetClass eyesWidgetClass = (WidgetClass) &eyesClassRec;

/* ARGSUSED */
static void Initialize (greq, gnew)
    Widget greq, gnew;
{
    EyesWidget w = (EyesWidget)gnew;
    XtGCMask	valuemask;
    XGCValues	myXGCV;
    int shape_event_base, shape_error_base;

    /*
     * set the colors if reverse video; these are the colors used:
     *
     *     background - paper		white
     *     foreground - text, ticks	black
     *     border - border		black (foreground)
     *
     * This doesn't completely work since the parent has already made up a 
     * border.  Sigh.
     */
    if (w->eyes.reverse_video) {
	Pixel fg = w->eyes.puppixel;
	Pixel bg = w->core.background_pixel;

	if (w->core.border_pixel == fg)
 	    w->core.border_pixel = bg;
	if (w->eyes.outline == fg)
	    w->eyes.outline = bg;
	if (w->eyes.center == bg)
	    w->eyes.center = fg;
	w->eyes.puppixel = bg;
	w->core.background_pixel = fg;
    }

    myXGCV.foreground = w->eyes.puppixel;
    myXGCV.background = w->core.background_pixel;
    valuemask = GCForeground | GCBackground;
    w->eyes.pupGC = XtGetGC(gnew, valuemask, &myXGCV);

    myXGCV.foreground = w->eyes.outline;
    valuemask = GCForeground | GCBackground;
    w->eyes.outGC = XtGetGC(gnew, valuemask, &myXGCV);

    myXGCV.foreground = w->eyes.center;
    myXGCV.background = w->eyes.puppixel;
    valuemask = GCForeground | GCBackground;
    w->eyes.centerGC = XtGetGC(gnew, valuemask, &myXGCV);

    w->eyes.update = 0;
    /* wait for Realize to add the timeout */
    w->eyes.interval_id = 0;

    w->eyes.pupil[0].x = w->eyes.pupil[1].x = -1000;
    w->eyes.pupil[0].y = w->eyes.pupil[1].y = -1000;

    if (w->eyes.shape_window && !XShapeQueryExtension (XtDisplay (w),
						       &shape_event_base,
						       &shape_error_base))
	w->eyes.shape_window = False;
    w->eyes.shape_mask = 0;
    w->eyes.shapeGC = 0;
}

static void Resize (gw)
    Widget	gw;
{
    EyesWidget	w = (EyesWidget) gw;
    XGCValues	xgcv;
    Widget	parent;
    int		x, y;

    if (XtIsRealized (gw))
    {
    	XClearWindow (XtDisplay (w), XtWindow (w));
    	SetTransform (&w->eyes.t,
		    	0, w->core.width,
 		    	w->core.height, 0,
		    	W_MIN_X, W_MAX_X,
		    	W_MIN_Y, W_MAX_Y);
    	if (w->eyes.shape_window) {
    	    w->eyes.shape_mask = XCreatePixmap (XtDisplay (w), XtWindow (w),
	    	    w->core.width, w->core.height, 1);
    	    if (!w->eyes.shapeGC)
            	w->eyes.shapeGC = XCreateGC (XtDisplay (w), w->eyes.shape_mask, 0, &xgcv);
    	    XSetForeground (XtDisplay (w), w->eyes.shapeGC, 0);
    	    XFillRectangle (XtDisplay (w), w->eyes.shape_mask, w->eyes.shapeGC, 0, 0,
	    	w->core.width, w->core.height);
    	    XSetForeground (XtDisplay (w), w->eyes.shapeGC, 1);
    	    eyeLiner (w, w->eyes.shape_mask, w->eyes.shapeGC, (GC) 0, 0);
    	    eyeLiner (w, w->eyes.shape_mask, w->eyes.shapeGC, (GC) 0, 1);
	    x = y = 0;
	    for (parent = (Widget) w; XtParent (parent); parent = XtParent (parent)) {
	    	x += parent->core.x + parent->core.border_width;
	    	x += parent->core.y + parent->core.border_width;
	    }
    	    XShapeCombineMask (XtDisplay (parent), XtWindow (parent), ShapeBounding,
		       	       x, y, w->eyes.shape_mask, ShapeSet);
    	    XFreePixmap (XtDisplay (w), w->eyes.shape_mask);
    	}
    }
}

static void Realize (gw, valueMask, attrs)
     Widget gw;
     XtValueMask *valueMask;
     XSetWindowAttributes *attrs;
{
    EyesWidget	w = (EyesWidget)gw;

    if (w->eyes.backing_store != Always + WhenMapped + NotUseful) {
     	attrs->backing_store = w->eyes.backing_store;
	*valueMask |= CWBackingStore;
    }
    XtCreateWindow( gw, (unsigned)InputOutput, (Visual *)CopyFromParent,
		     *valueMask, attrs );
    Resize (gw);
    w->eyes.interval_id =
	XtAppAddTimeOut(XtWidgetToApplicationContext(gw),
			delays[w->eyes.update], draw_it, (XtPointer)gw);
}

static void Destroy (gw)
     Widget gw;
{
     EyesWidget w = (EyesWidget)gw;

     if (w->eyes.interval_id)
	XtRemoveTimeOut (w->eyes.interval_id);
     XtReleaseGC(gw, w->eyes.pupGC);
     XtReleaseGC(gw, w->eyes.outGC);
     XtReleaseGC(gw, w->eyes.centerGC);
}

/* ARGSUSED */
static void Redisplay(gw, event, region)
     Widget gw;
     XEvent *event;
     Region region;
{
    EyesWidget	w;

    w = (EyesWidget) gw;
    w->eyes.pupil[0].x = -1000;
    w->eyes.pupil[0].y = -1000;
    w->eyes.pupil[1].x = -1000;
    w->eyes.pupil[1].y = -1000;
    (void) repaint_window ((EyesWidget)gw);
}

static TPoint computePupil (num, mouse)
    int		num;
    TPoint	mouse;
{
	double	cx, cy;
	double	dist;
	double	angle;
	double	x, y;
	double	h;
	double	dx, dy;
	double	cosa, sina;
	TPoint	ret;

	dx = mouse.x - EYE_X(num);
	dy = mouse.y - EYE_Y(num);
	if (dx == 0 && dy == 0) {
		cx = EYE_X(num);
		cy = EYE_Y(num);
	} else {
		angle = atan2 ((double) dy, (double) dx);
		cosa = cos (angle);
		sina = sin (angle);
		h = hypot (EYE_HHEIGHT * cosa, EYE_HWIDTH * sina);
		x = (EYE_HWIDTH * EYE_HHEIGHT) * cosa / h;
		y = (EYE_HWIDTH * EYE_HHEIGHT) * sina / h;
		dist = BALL_DIST * hypot (x, y);
		if (dist > hypot ((double) dx, (double) dy)) {
			cx = dx + EYE_X(num);
			cy = dy + EYE_Y(num);
		} else {
			cx = dist * cosa + EYE_X(num);
			cy = dist * sina + EYE_Y(num);
		}
	}
	ret.x = cx;
	ret.y = cy;
	return ret;
}

static void computePupils (mouse, pupils)
    TPoint	mouse;
    TPoint	pupils[2];
{
    pupils[0] = computePupil (0, mouse);
    pupils[1] = computePupil (1, mouse);
}

/* ARGSUSED */
static void draw_it(client_data, id)
     XtPointer client_data;
     XtIntervalId *id;		/* unused */
{
        EyesWidget	w = (EyesWidget)client_data;
	Window		rep_root, rep_child;
	int		rep_rootx, rep_rooty;
	unsigned int	rep_mask;
	int		dx, dy;
	TPoint		mouse;
	Display		*dpy = XtDisplay (w);
	Window		win = XtWindow (w);
	TPoint		newpupil[2];
	XPoint		xnewpupil, xpupil;

	if (XtIsRealized((Widget)w)) {
    		XQueryPointer (dpy, win, &rep_root, &rep_child,
 			&rep_rootx, &rep_rooty, &dx, &dy, &rep_mask);
		mouse.x = Tx(dx, dy, &w->eyes.t);
		mouse.y = Ty(dx, dy, &w->eyes.t);
		if (!TPointEqual (mouse, w->eyes.mouse)) {
			computePupils (mouse, newpupil);
			xpupil.x = Xx(w->eyes.pupil[0].x, w->eyes.pupil[0].y, &w->eyes.t);
			xpupil.y = Xy(w->eyes.pupil[0].x, w->eyes.pupil[0].y, &w->eyes.t);
			xnewpupil.x =  Xx(newpupil[0].x, newpupil[0].y, &w->eyes.t);
			xnewpupil.y =  Xy(newpupil[0].x, newpupil[0].y, &w->eyes.t);
			if (!XPointEqual (xpupil, xnewpupil)) {
			    if (w->eyes.pupil[0].x != -1000 || w->eyes.pupil[0].y != -1000)
				eyeBall (w, w->eyes.centerGC, 0);
			    w->eyes.pupil[0] = newpupil[0];
			    eyeBall (w, w->eyes.pupGC, 0);
			}
			xpupil.x = Xx(w->eyes.pupil[1].x, w->eyes.pupil[1].y, &w->eyes.t);
			xpupil.y = Xy(w->eyes.pupil[1].x, w->eyes.pupil[1].y, &w->eyes.t);
			xnewpupil.x =  Xx(newpupil[1].x, newpupil[1].y, &w->eyes.t);
			xnewpupil.y =  Xy(newpupil[1].x, newpupil[1].y, &w->eyes.t);
			if (!XPointEqual (xpupil, xnewpupil)) {
			    if (w->eyes.pupil[1].x != -1 || w->eyes.pupil[1].y != -1)
				eyeBall (w, w->eyes.centerGC, 1);
			    w->eyes.pupil[1] = newpupil[1];
			    eyeBall (w, w->eyes.pupGC, 1);
			}
			w->eyes.mouse = mouse;
			w->eyes.update = 0;
		} else {
			if (delays[w->eyes.update + 1] != 0)
				++w->eyes.update;
		}
	}
	w->eyes.interval_id =
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) w),
				delays[w->eyes.update], draw_it, (XtPointer)w);
} /* draw_it */

static
repaint_window (w)
    EyesWidget	w;
{
	if (XtIsRealized ((Widget) w)) {
		eyeLiner (w, XtWindow (w), w->eyes.outGC, w->eyes.centerGC, 0);
		eyeLiner (w, XtWindow (w), w->eyes.outGC, w->eyes.centerGC, 1);
		computePupils (w->eyes.mouse, w->eyes.pupil);
		eyeBall (w, w->eyes.pupGC, 0);
		eyeBall (w, w->eyes.pupGC, 1);
	}
}
    
/* ARGSUSED */
static Boolean SetValues (current, request, new)
    Widget current, request, new;
{
    return( FALSE );
}

eyeLiner (w, d, outgc, centergc, num)
EyesWidget	w;
Drawable	d;
GC		outgc, centergc;
int		num;
{
	Display *dpy = XtDisplay(w);

	TFillArc (dpy, d, outgc, &w->eyes.t,
		  EYE_X(num) - EYE_HWIDTH - EYE_THICK,
 		  EYE_Y(num) - EYE_HHEIGHT - EYE_THICK,
		  EYE_WIDTH + EYE_THICK * 2.0,
 		  EYE_HEIGHT + EYE_THICK * 2.0,
 		  90 * 64, 360 * 64);
	if (centergc) {
    	    TFillArc (dpy, d, centergc, &w->eyes.t,
		  EYE_X(num) - EYE_HWIDTH,
 		  EYE_Y(num) - EYE_HHEIGHT,
		  EYE_WIDTH, EYE_HEIGHT,
		  90 * 64, 360 * 64);
	}
}

eyeBall (w, gc, num)
EyesWidget	w;
GC	gc;
int	num;
{
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);

	TFillArc (dpy, win, gc, &w->eyes.t,
		   w->eyes.pupil[num].x - BALL_WIDTH / 2.0,
		   w->eyes.pupil[num].y - BALL_HEIGHT / 2.0,
		   BALL_WIDTH, BALL_HEIGHT,
		  90 * 64, 360 * 64);
}
