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
#ifndef _FontGridP_h_
#define _FontGridP_h_

#include "fontgrid.h"

typedef struct _FontGridClassPart { int dummy; } FontGridClassPart;

typedef struct _FontGridClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    FontGridClassPart	grid_class;
} FontGridClassRec;
extern FontGridClassRec fontgridClassRec;

typedef struct _FontGridPart {
    XFontStruct *	text_font;		/* font to display */
    int			cell_cols, cell_rows;  /* number of cells */
    int			cell_width, cell_height;  /* size of cell */
    Pixel		foreground_pixel;	/* color of text */
    Pixel		box_pixel;	/* for box_chars */
    Boolean		center_chars;	/* center characters in grid */
    Boolean		box_chars;	/* put box around logical width */
    XtCallbackList	callbacks;	/* for notifying caller */
    int			internal_pad;	/* extra padding inside grid */
    Dimension		start_char;	/* first character of grid */
    int			grid_width;	/* width of grid lines */
    /* private data */
    GC			text_gc;	/* printing text */
    GC			box_gc;		/* for box_chars */
    int			xoff, yoff;	/* extra offsets within grid */
} FontGridPart;

typedef struct _FontGridRec {
    CorePart		core;
    SimplePart		simple;
    FontGridPart	fontgrid;
} FontGridRec;

#define DefaultCellWidth(fgw) (((fgw)->fontgrid.text_font->max_bounds.width) \
			       + ((fgw)->fontgrid.internal_pad * 2))
#define DefaultCellHeight(fgw) ((fgw)->fontgrid.text_font->ascent + \
				(fgw)->fontgrid.text_font->descent + \
				((fgw)->fontgrid.internal_pad * 2))


#define CellWidth(fgw) (((int)(fgw)->core.width - (fgw)->fontgrid.grid_width) \
			/ (fgw)->fontgrid.cell_cols \
			- (fgw)->fontgrid.grid_width)
#define CellHeight(fgw) (((int)(fgw)->core.height - (fgw)->fontgrid.grid_width)\
			 / (fgw)->fontgrid.cell_rows \
			 - (fgw)->fontgrid.grid_width)

#endif /* !_FontGridP_h_ */
