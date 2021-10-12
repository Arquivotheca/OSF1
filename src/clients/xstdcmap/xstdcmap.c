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
 * $XConsortium: xstdcmap.c,v 1.6 89/07/24 11:06:26 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
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
 * Author:  Donna Converse, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdCmap.h>

extern void exit();

#define REPLACE		1
#define DO_NOT_REPLACE  0
#define RETAIN		1
#define DO_NOT_RETAIN	0

static char		*display_name = NULL;
static char		*program_name = NULL;
static Bool		all = 0;
static Bool		help = 0;
static Bool 		verbose = 0;
static Display		*dpy = NULL;

typedef struct
{
    Bool	create;
    Bool	delete;
    Atom	property;
    char	*name;
    char	*nickname;
} colormap_property;

static colormap_property propertyTable[]=
{
{0,	0,	XA_RGB_DEFAULT_MAP,	"RGB_DEFAULT_MAP",	"default"},
{0,	0,	XA_RGB_BEST_MAP,	"RGB_BEST_MAP",		"best"},
{0,	0,	XA_RGB_GRAY_MAP,	"RGB_GRAY_MAP",		"gray"},
{0,	0,	XA_RGB_RED_MAP,		"RGB_RED_MAP",		"red"},
{0,	0,	XA_RGB_GREEN_MAP,	"RGB_GREEN_MAP",	"green"},
{0,	0,	XA_RGB_BLUE_MAP,	"RGB_BLUE_MAP",		"blue"},
};
#define NPROPERTIES (sizeof propertyTable / sizeof propertyTable[0])

#define DEFAULT	0
#define BEST	1
#define GRAY	2
#define RED	3
#define GREEN	4
#define BLUE	5

static char	*usage_message[]=
{
"    -all               make all standard colormaps for the display",
"    -best              make the RGB_BEST_MAP",
"    -blue              make the RGB_BLUE_MAP",
"    -default           make the RGB_DEFAULT_MAP",
"    -delete name       remove a standard colormap",
"    -display dpy       X server to use",
"    -gray              make the RGB_GRAY_MAP",
"    -green             make the RGB_GREEN_MAP",
"    -red               make the RGB_RED_MAP",
"    -verbose           turn on logging",
"",
NULL };

static XrmOptionDescRec optionTable[]=
{
{"-all",	".all",		XrmoptionNoArg,		(caddr_t) "on"},
{"-best",	".best",	XrmoptionNoArg,		(caddr_t) "on"},
{"-blue",	".blue",	XrmoptionNoArg,		(caddr_t) "on"},
{"-default",	".default",	XrmoptionNoArg,		(caddr_t) "on"},
{"-delete",	".delete",	XrmoptionSepArg,	(caddr_t) NULL},
{"-display",	".display", 	XrmoptionSepArg,	(caddr_t) NULL},
{"-gray",	".gray",	XrmoptionNoArg,		(caddr_t) "on"},
{"-green",	".green",	XrmoptionNoArg,		(caddr_t) "on"},
{"-help",	".help",        XrmoptionNoArg,		(caddr_t) "on"},
{"-red",	".red",		XrmoptionNoArg,		(caddr_t) "on"},
{"-verbose",	".verbose",	XrmoptionNoArg,		(caddr_t) "on"},
};
#define NOPTIONS (sizeof optionTable / sizeof optionTable[0])

static void parse(argc, argv)
    int		argc;
    char	**argv;
{
    XrmDatabase		database = NULL;
    char		*type;
    XrmValue		value;
    char		option[512];

    if (argc == 1)
	usage(0);

    XrmInitialize();
    XrmParseCommand(&database, optionTable, NOPTIONS, program_name, &argc,
		    argv);
    if (--argc)
	usage(1);

    (void) sprintf(option, "%s%s", program_name, ".all");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
    	all++;

    (void) sprintf(option, "%s%s", program_name, ".best");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
    	propertyTable[BEST].create++;

    (void) sprintf(option, "%s%s", program_name, ".blue");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	propertyTable[BLUE].create++;

    (void) sprintf(option, "%s%s", program_name, ".default");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	propertyTable[DEFAULT].create++;

    (void) sprintf(option, "%s%s", program_name, ".delete");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value)) {
	register int i;
	for (i=0; i < NPROPERTIES; i++) 
	    if (strncmp((char *) value.addr, propertyTable[i].nickname,
			(int) value.size) == 0) {
		propertyTable[i].delete++;
		break;
	    }
    }
		
    (void) sprintf(option, "%s%s", program_name, ".display");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	display_name = value.addr;

    (void) sprintf(option, "%s%s", program_name, ".gray");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	propertyTable[GRAY].create++;

    (void) sprintf(option, "%s%s", program_name, ".green");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	propertyTable[GREEN].create++;

    (void) sprintf(option, "%s%s", program_name, ".help");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	help++;

    (void) sprintf(option, "%s%s", program_name, ".red");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	propertyTable[RED].create++;

    (void) sprintf(option, "%s%s", program_name, ".verbose");
    if (XrmGetResource(database, option, (char *) NULL, &type, &value))
	verbose++;
}

Exit(status)
    Status	status;
{
    if (dpy)
	XCloseDisplay(dpy);
    exit(status);
}

usage(status)
    Status		status;
{
    register char	**i;
    (void) fprintf(stderr, "usage:  %s [-options]\n\n", program_name);
    (void) fprintf(stderr, "where options include:\n");
    for (i = usage_message; *i != NULL; i++)
	(void) fprintf(stderr, "%s\n", *i);
    Exit(status);
}

/* Determine the visual of greatest depth in a given visual class.
 * If no such visual exists, return NULL.  
 */
static XVisualInfo *getDeepestVisual(visual_class, vinfo, nvisuals)
    int		visual_class;	/* specifies the desired visual class */
    XVisualInfo	*vinfo;		/* specifies all visuals for a screen */
    int		nvisuals;	/* specifies number of visuals in the list */
{
    register int	i;
    unsigned int	maxdepth = 0;
    XVisualInfo		*v = NULL;
    
    for (i=0; i < nvisuals; i++, vinfo++)
	if (vinfo->class == visual_class && vinfo->depth > maxdepth)
	{
	    maxdepth = vinfo->depth;
	    v = vinfo;
	}
    return(v);
}

/* Determine the ``best'' visual of the screen for a standard colormap
 * property.  Return NULL if no visual is appropriate.
 */
static XVisualInfo *getBestVisual(property, vinfo, nvisuals)
    Atom	property;	/* specifies the standard colormap */
    XVisualInfo *vinfo;		/* specifies all visuals of the screen */
    int		nvisuals;	/* specifies number of visuals of screen */
{	
    XVisualInfo	*v1 = NULL, *v2 = NULL;

    if (vinfo == NULL)		 /* unexpected: a screen with no visuals */
	return v1;
    v1 = getDeepestVisual(DirectColor, vinfo, nvisuals);
    v2 = getDeepestVisual(PseudoColor, vinfo, nvisuals);
    if (v2 && (!v1 || (v2->colormap_size >=
		       ((v1->red_mask | v1->green_mask | v1->blue_mask) + 1))))
	return v2;
    else if (v1)
	return v1;
    if (property == XA_RGB_BEST_MAP)
	if (((v1 = getDeepestVisual(TrueColor, vinfo, nvisuals)) != NULL) ||
	    ((v1 = getDeepestVisual(StaticColor, vinfo, nvisuals)) != NULL))
	    return v1;
    if (property == XA_RGB_GRAY_MAP)
	if (((v1 = getDeepestVisual(GrayScale, vinfo, nvisuals)) != NULL) ||
	    ((v1 = getDeepestVisual(StaticGray, vinfo, nvisuals)) != NULL))
	    return v1;
    return v1;

}

static char *visualStringFromClass(class)
    int	class;
{
    switch (class) {
      case PseudoColor: return "PseudoColor";
      case DirectColor: return "DirectColor";
      case GrayScale: return "GrayScale";
      case StaticColor: return "StaticColor";
      case TrueColor: return "TrueColor";
      case StaticGray: return "StaticGray";
    }
    return "unknown visual class";
}

static int doIndividualColormaps()
{
    int			i, screen, nvisuals;
    Status		status;
    XVisualInfo		*vinfo = NULL, *v = NULL, template;
    
    screen = DefaultScreen(dpy);
    template.screen = screen;
    vinfo = XGetVisualInfo(dpy, VisualScreenMask, &template, &nvisuals);

    /* check for individual standard colormap requests */
    for (i=0; i < NPROPERTIES; i++) {

	if (propertyTable[i].delete) {
	    XmuDeleteStandardColormap(dpy, screen, propertyTable[i].property);
	    if (verbose)
		fprintf(stderr, "%s: %s was deleted or did not exist.\n",
			program_name, propertyTable[i].name);
	}

	if (! propertyTable[i].create)	
	    continue;
	
	/* which visual is best for this property? */
	v = getBestVisual(propertyTable[i].property, vinfo, nvisuals);
	if (v == NULL) {
	    if (verbose)
		(void) fprintf(stderr,
		       "%s: no visual appropriate for %s on screen %d.\n",
			program_name, propertyTable[i].name, screen);
	    continue;
	}


	if (verbose)
	    (void) fprintf(stderr,
			   "%s: making %s on a %s visual of depth %u.\n",
			   program_name, propertyTable[i].name,
			   visualStringFromClass(v->class), v->depth);
	
	status = XmuLookupStandardColormap(dpy, screen, v->visualid,
					   v->depth,
					   propertyTable[i].property,
					   DO_NOT_REPLACE, RETAIN);
	if (verbose)
	    (void) fprintf(stderr,
			   "%s: %s standard colormap %s.\n", program_name,
			   propertyTable[i].name, (status)
			   ? "was created or already exists"
			   : "cannot be defined");
	if (!status)
	    break;
    }
    XFree((char *) vinfo);
    return status;
}

/* Bare bones standard colormap generation utility */
main(argc, argv)
    int		argc;
    char	**argv;
{
    Status	status = 0;

    if (program_name = rindex(*argv, '/'))
	program_name++;
    else
	program_name = *argv;

    parse(argc, argv);

    if ((dpy = XOpenDisplay(display_name)) == NULL) {
	(void) fprintf(stderr, "%s: cannot open display \"%s\".\n",
		       program_name, XDisplayName(display_name));
	exit(1);
    }

    if (help) {
	usage(0);
	Exit(0);
    }

    if (all) {
	if (verbose)
	    (void) fprintf(stderr,
			   "%s: making all appropriate standard colormaps...",
			   program_name);
	status = XmuAllStandardColormaps(dpy);
	if (verbose)
	    (void) fprintf(stderr,
			   "\n%s!\n", (status) ? "success" : "failure");
    }
    else {
	status = doIndividualColormaps();
	if (!status && verbose)
	    (void) fprintf(stderr, 
		    "Not all new colormap definitions will be retained.\n");
    }
    Exit((status == 0) ? 1 : 0);
}
