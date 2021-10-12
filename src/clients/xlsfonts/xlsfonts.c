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
 * $XConsortium: xlsfonts.c,v 1.32 91/06/24 11:09:12 gildea Exp $
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
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "dsimple.h"

#define N_START 1000  /* Maximum # of fonts to start with */

int max_output_line_width = 79;
int output_line_padding = 3;
int columns = 0;

#define L_SHORT 0
#define L_MEDIUM 1
#define L_LONG 2
#define L_VERYLONG 3

Bool	sort_output = True;
Bool	open_instead_of_list = False;
int	long_list = L_SHORT;
int	nnames = N_START;
int	font_cnt;
int	min_max;
typedef struct {
	char		*name;
	XFontStruct	*info;
} FontList;
FontList	*font_list;


usage()
{
	fprintf (stderr,"usage:  %s [-options] [-fn pattern]\n", program_name);
	fprintf (stderr, "where options include:\n");
	fprintf (stderr,
	"    -l[l[l]]                 give long info about each font\n");
	fprintf (stderr,
	"    -m                       give character min and max bounds\n");
	fprintf (stderr,
	"    -C                       force columns\n");
	fprintf (stderr,
	"    -1                       force single column\n");
	fprintf (stderr,
	"    -u                       keep output unsorted\n");
	fprintf (stderr,
	"    -o                       use OpenFont/QueryFont instead of ListFonts\n");
	fprintf (stderr,
	"    -w width                 maximum width for multiple columns\n");
	fprintf (stderr,
	"    -n columns               number of columns if multi column\n");
	fprintf (stderr,
	"    -display displayname     X server to contact\n");
	fprintf (stderr,
	    "\n");
	exit(1);
}

main(argc, argv)
int argc;
char **argv;    
{
	int	argcnt = 0, i;

	INIT_NAME;

	/* Handle command line arguments, open display */
	Setup_Display_And_Screen(&argc, argv);
	for (argv++, argc--; argc; argv++, argc--) {
		if (argv[0][0] == '-') {
			if (argcnt > 0) usage ();
			for (i=1; argv[0][i]; i++)
				switch(argv[0][i]) {
				case 'l':
					long_list++;
					break;
				case 'm':
					min_max++;
					break;
				case 'C':
					columns = 0;
					break;
				case '1':
					columns = 1;
					break;
				case 'f':
					if (--argc <= 0) usage ();
					argcnt++;
					argv++;
					get_list(argv[0]);
					goto next;
				case 'w':
					if (--argc <= 0) usage ();
					argv++;
					max_output_line_width = atoi(argv[0]);
					goto next;
				case 'n':
					if (--argc <= 0) usage ();
					argv++;
					columns = atoi(argv[0]);
					goto next;
				case 'o':
					open_instead_of_list = True;
					break;
				case 'u':
					sort_output = False;
					break;
				default:
					usage();
					break;
				}
			if (i == 1)
				usage();
		} else {
			argcnt++;
			get_list(argv[0]);
		}
	  next: ;
	}
	if (argcnt == 0)
		get_list("*");
	show_fonts();
	exit(0);
}

get_list(pattern)
	char	*pattern;
{
	int	available = nnames+1,
		i;
	char	**fonts;
	XFontStruct	*info;

	/* Get list of fonts matching pattern */
	for (;;) {
		if (open_instead_of_list) {
		    info = XLoadQueryFont (dpy, pattern);

		    if (info) {
			fonts = &pattern;
			available = 1;
			XUnloadFont (dpy, info->fid);
		    } else {
			fonts = NULL;
		    }
		    break;
		}
		    
		if (long_list == L_MEDIUM)
			fonts = XListFontsWithInfo(dpy,
				pattern, nnames, &available, &info);
		else
			fonts = XListFonts(dpy, pattern, nnames, &available);
		if (fonts == NULL || available < nnames)
			break;
		if (long_list == L_MEDIUM)
			XFreeFontInfo(fonts, info, available);
		else
			XFreeFontNames(fonts);
		nnames = available * 2;
	}

	if (fonts == NULL) {
		fprintf(stderr, "%s: pattern \"%s\" unmatched\n",
			program_name, pattern);
		return;
	}

	font_list = (FontList *)Realloc((char *)font_list,
		(font_cnt + available) * sizeof(FontList));
	for (i=0; i<available; i++) {
		font_list[font_cnt].name = fonts[i];
		if (long_list == L_MEDIUM)
			font_list[font_cnt].info = info + i;
		else
			font_list[font_cnt].info = NULL;
		font_cnt++;
	}
}

compare(f1, f2)
	FontList	*f1, *f2;
{
	char	*p1 = f1->name,
		*p2 = f2->name;

	while (*p1 && *p2 && *p1 == *p2)
		p1++, p2++;
	return(*p1 - *p2);
}

show_fonts()
{
	int	i;

	if (font_cnt == 0)
		return;

	/* first sort the output */
	if (sort_output) qsort(font_list, font_cnt, sizeof(FontList), compare);

	if (long_list > L_MEDIUM) {
	    for (i = 0; i < font_cnt; i++) {
		do_query_font (dpy, font_list[i].name);
	    }
	    return;
	}

	if (long_list == L_MEDIUM) {
		XFontStruct	*pfi;
		char		*string;

		printf("DIR  ");
		printf("MIN  ");
		printf("MAX ");
		printf("EXIST ");
		printf("DFLT ");
		printf("PROP ");
		printf("ASC ");
		printf("DESC ");
		printf("NAME");
		printf("\n");
		for (i=0; i<font_cnt; i++) {
			pfi = font_list[i].info;
			if (!pfi) {
			    fprintf (stderr, 
		    	     "%s:  no font information for font \"%s\".\n",
				     program_name, 
				     font_list[i].name ? 
				     font_list[i].name : "");
			    continue;
			}
			switch(pfi->direction) {
			case FontLeftToRight: string = "-->"; break;
			case FontRightToLeft: string = "<--"; break;
			default:	      string = "???"; break;
			}
			printf("%-4s", string);
			if (pfi->min_byte1 == 0
			 && pfi->max_byte1 == 0) {
				printf(" %3d ", pfi->min_char_or_byte2);
				printf(" %3d ", pfi->max_char_or_byte2);
			} else {
				printf("*%3d ", pfi->min_byte1);
				printf("*%3d ", pfi->max_byte1);
			}
			printf("%5s ", pfi->all_chars_exist ? "all" : "some");
			printf("%4d ", pfi->default_char);
			printf("%4d ", pfi->n_properties);
			printf("%3d ", pfi->ascent);
			printf("%4d ", pfi->descent);
			printf("%s\n", font_list[i].name);
			if (min_max) {
				char	min[ BUFSIZ ],
					max[ BUFSIZ ];
				char	*pmax = max,
					*pmin = min;

				strcpy(pmin, "     min(l,r,w,a,d) = (");
				strcpy(pmax, "     max(l,r,w,a,d) = (");
				pmin += strlen(pmin);
				pmax += strlen(pmax);

				copy_number(&pmin, &pmax,
					pfi->min_bounds.lbearing,
					pfi->max_bounds.lbearing);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.rbearing,
					pfi->max_bounds.rbearing);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.width,
					pfi->max_bounds.width);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.ascent,
					pfi->max_bounds.ascent);
				*pmin++ = *pmax++ = ',';
				copy_number(&pmin, &pmax,
					pfi->min_bounds.descent,
					pfi->max_bounds.descent);
				*pmin++ = *pmax++ = ')';
				*pmin = *pmax = '\0';
				printf("%s\n", min);
				printf("%s\n", max);
			}
		}
		return;
	}

	if ((columns == 0 && isatty(1)) || columns > 1) {
		int	width,
			max_width = 0,
			lines_per_column,
			j,
			index;

		for (i=0; i<font_cnt; i++) {
			width = strlen(font_list[i].name);
			if (width > max_width)
				max_width = width;
		}
		if (max_width == 0)
			Fatal_Error("all %d fontnames listed are zero length",
				    font_cnt);

		if (columns == 0) {
		    if ((max_width * 2) + output_line_padding >
			max_output_line_width) {
			columns = 1;
		    } else {
			max_width += output_line_padding;
			columns = ((max_output_line_width +
				    output_line_padding) / max_width);
		    }
		} else {
		    max_width += output_line_padding;
		}
		if (columns <= 1) goto single_column;

		if (font_cnt < columns)
			columns = font_cnt;
		lines_per_column = (font_cnt + columns - 1) / columns;

		for (i=0; i<lines_per_column; i++) {
			for (j=0; j<columns; j++) {
				index = j * lines_per_column + i;
				if (index >= font_cnt)
					break;
				if (j+1 == columns)
					printf("%s", font_list[ index ].name);
				else
					printf("%-*s",
						max_width, 
						font_list[ index ].name);
			}
			printf("\n");
		}
		return;
	}

      single_column:
	for (i=0; i<font_cnt; i++)
		printf("%s\n", font_list[i].name);
}

max(i, j)
	int	i, j;
{
	if (i > j)
		return (i);
	return(j);
}

copy_number(pp1, pp2, n1, n2)
	char	**pp1, **pp2;
	int	n1, n2;
{
	char	*p1 = *pp1;
	char	*p2 = *pp2;
	int	w;

	sprintf(p1, "%d", n1);
	sprintf(p2, "%d", n2);
	w = max(strlen(p1), strlen(p2));
	sprintf(p1, "%*d", w, n1);
	sprintf(p2, "%*d", w, n2);
	p1 += strlen(p1);
	p2 += strlen(p2);
	*pp1 = p1;
	*pp2 = p2;
}



/* ARGSUSED */
static int IgnoreError(disp, event)
    Display *disp;
    XErrorEvent *event;
{
    return 0;
}

static char *bounds_metrics_title =
                      "width left  right  asc  desc   attr   keysym\n";
static char *char_metrics_fmt = 
"\t0x%02x%02x (%u)\t%4d  %4d  %4d  %4d  %4d  0x%04x  %s\n";
static char *bounds_metrics_fmt =
"\t%3s\t\t%4d  %4d  %4d  %4d  %4d  0x%04x\n";


#define PrintBounds(_what,_ptr) \
{   register XCharStruct *p = (_ptr); \
    printf (bounds_metrics_fmt, _what, p->width, p->lbearing, \
	  p->rbearing, p->ascent, p->descent, p->attributes); }


static char* stringValued [] = { /* values are atoms */
    "FAMILY_NAME",
    "FOUNDRY",
    "STYLE",
    "MONOSPACED",
    "RELATIVE_WEIGHT",
    "RELATIVE_SET",
    "CLASSIFICATION",
    "CHARSET_REGISTRY",
    "CHARSET_ENCODING",
    "QUALITY",
    "CHARSET_COLLECTIONS",
    "FULL_NAME",
    "COPYRIGHT",
    "WEIGHT_NAME",
    "SETWIDTH_NAME",
    "SLANT",
    "SPACING",
    "ADD_STYLE_NAME",
    "FONTNAME_REGISTRY",
    "FONT",
    "DEVICE_FONT_NAME",
     NULL
    };

static void PrintProperty (prop)
    XFontProp *prop;
{
    char *atom, *value;
    char nosuch[40];
    int i;
    int (*oldhandler)() = XSetErrorHandler (IgnoreError);

    atom = XGetAtomName(dpy, prop->name);
    if (!atom) {
	atom = nosuch;
	nosuch[0] = '\0';
	(void)sprintf (atom, "No such atom = %d", prop->name);
    }
    printf ("      %s", atom);
    for (i = strlen(atom); i < 22; i++) printf (" ");
    for (i = 0; ; i++) {
	if (stringValued[i] == NULL) {
	    printf ("%d\n", prop->card32);
	    break;
	}
	if (strcmp(stringValued[i], atom) == 0) {
	    value = XGetAtomName(dpy, prop->card32);
	    if (value == NULL)
		printf ("%d (expected string value)\n", prop->card32);
	    else {
		printf ("%s\n", value);
		XFree (value);
	    }
	    break;
	}
    } 
    if (atom != nosuch) XFree (atom);
    XSetErrorHandler (oldhandler);
}


ComputeFontType (fs)
    XFontStruct *fs;
{
    int i;
    Bool char_cell = True;
    char *reason = NULL;
    XCharStruct *cs;
    Atom awatom = XInternAtom (dpy, "AVERAGE_WIDTH", False);

    printf ("  font type:\t\t");
    if (fs->min_bounds.width != fs->max_bounds.width) {
	printf ("Proportional (min and max widths not equal)\n");
	return;
    }

    if (awatom) {
	for (i = 0; i < fs->n_properties; i++) {
	    if (fs->properties[i].name == awatom &&
		(fs->max_bounds.width * 10) != fs->properties[i].card32) {
		char_cell = False;
		reason = "font width not equal to AVERAGE_WIDTH";
		break;
	    }
	}
    }

    if (fs->per_char) {
	for (i = fs->min_char_or_byte2, cs = fs->per_char;
	     i <= fs->max_char_or_byte2; i++, cs++) {
	    if (cs->width == 0) continue;
	    if (cs->width != fs->max_bounds.width) {
		/* this shouldn't happen since we checked above */
		printf ("Proportional (characters not all the same width)\n");
		return;
	    }
	    if (char_cell) {
		if (cs->width < 0) {
		    if (!(cs->width <= cs->lbearing &&
			  cs->lbearing <= cs->rbearing &&
			  cs->rbearing <= 0)) {
			char_cell = False;
			reason = "ink outside bounding box";
		    }
		} else {
		    if (!(0 <= cs->lbearing &&
			  cs->lbearing <= cs->rbearing &&
			  cs->rbearing <= cs->width)) {
			char_cell = False;
			reason = "ink outside bounding box";
		    }
		}
		if (!(cs->ascent <= fs->ascent &&
		      cs->descent <= fs->descent)) {
		    char_cell  = False;
		    reason = "characters not all same ascent or descent";
		}
	    }
	}
    }

    printf ("%s", char_cell ? "Character Cell" : "Monospaced");
    if (reason) printf (" (%s)", reason);
    printf ("\n");
	
    return;
}


print_character_metrics (info)
    register XFontStruct *info;
{
    register XCharStruct *pc = info->per_char;
    register int i, j;
    unsigned n, saven;

    printf ("  character metrics:\n");
    saven = ((info->min_byte1 << 8) | info->min_char_or_byte2);
    for (j = info->min_byte1; j <= info->max_byte1; j++) {
	n = saven;
	for (i = info->min_char_or_byte2; i <= info->max_char_or_byte2; i++) {
	    char *s = XKeysymToString ((KeySym) n);
	    printf (char_metrics_fmt, j, i, n, pc->width, pc->lbearing,
		    pc->rbearing, pc->ascent, pc->descent, pc->attributes,
		    s ? s : ".");
	    pc++;
	    n++;
	}
	saven += 256;
    }
}


do_query_font (dpy, name)
    Display *dpy;
    char *name;
{
    register int i;
    register XFontStruct *info = XLoadQueryFont (dpy, name);

    if (!info) {
	fprintf (stderr, "%s:  unable to get info about font \"%s\"\n",
		 program_name, name);
	return;
    }
    printf ("name:  %s\n", name ? name : "(nil)");
    printf ("  direction:\t\t%s\n", ((info->direction == FontLeftToRight)
				     ? "left to right" : "right to left"));
    printf ("  indexing:\t\t%s\n", 
	    ((info->min_byte1 == 0 && info->max_byte1 == 0) ? "linear" :
	     "matrix"));
    printf ("  rows:\t\t\t0x%02x thru 0x%02x (%d thru %d)\n",
	    info->min_byte1, info->max_byte1,
	    info->min_byte1, info->max_byte1);
    printf ("  columns:\t\t0x%02x thru 0x%02x (%d thru %d)\n",
	    info->min_char_or_byte2, info->max_char_or_byte2,
	    info->min_char_or_byte2, info->max_char_or_byte2);
    printf ("  all chars exist:\t%s\n",
	(info->all_chars_exist) ? "yes" : "no");
    printf ("  default char:\t\t0x%04x (%d)\n",
	    info->default_char, info->default_char);
    printf ("  ascent:\t\t%d\n", info->ascent);
    printf ("  descent:\t\t%d\n", info->descent);
    ComputeFontType (info);
    printf ("  bounds:\t\t%s", bounds_metrics_title);
    PrintBounds ("min", &info->min_bounds);
    PrintBounds ("max", &info->max_bounds);
    if (info->per_char && long_list >= L_VERYLONG) 
      print_character_metrics (info);
    printf ("  properties:\t\t%d\n", info->n_properties);
    for (i = 0; i < info->n_properties; i++)
	PrintProperty (&info->properties[i]);
    printf ("\n");

    XFreeFontInfo (NULL, info, 1);
}
