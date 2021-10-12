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
 * $XConsortium: Graphics.c,v 1.16 91/11/18 17:30:51 gildea Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
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
 * Author:  Davor Matic, MIT X Consortium
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xfuncs.h>
#include "BitmapP.h"
    
#include <stdio.h>
#include <math.h>

#ifndef abs
#define abs(x)                        (((x) > 0) ? (x) : -(x))
#endif
#define min(x, y)                     (((int)(x) < (int)(y)) ? (x) : (y))
#define max(x, y)                     (((int)(x) > (int)(y)) ? (x) : (y))
#ifndef rint
#define rint(x)                       floor(x + 0.5)
#endif

/*****************************************************************************\
 *                                   Graphics                                *
\*****************************************************************************/

#define GetBit(image, x, y)\
    ((bit)((*(image->data + (x) / 8 + (y) * image->bytes_per_line) &\
	    (1 << ((x) % 8))) ? 1 : 0))


bit BWGetBit(w, x, y)
    Widget  w;
     Position      x, y;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QueryInBitmap(BW, x, y))
	return GetBit(BW->bitmap.image, x, y);
    else
	return NotSet;
}


#define InvertBit(image, x, y)\
    (*(image->data + (x) / 8 + (y) * image->bytes_per_line) ^=\
     (bit) (1 << ((x) % 8)))


#define SetBit(image, x, y)\
    (*(image->data + (x) / 8 + (y) * image->bytes_per_line) |=\
     (bit) (1 << ((x) % 8)))

#define ClearBit(image, x, y)\
    (*(image->data + (x) / 8 + (y) * image->bytes_per_line) &=\
     (bit)~(1 << ((x) % 8)))


#define HighlightSquare(BW, x, y)\
    XFillRectangle(XtDisplay(BW), XtWindow(BW),\
                   BW->bitmap.highlighting_gc,\
		   InWindowX(BW, x), InWindowY(BW, y),\
                   BW->bitmap.squareW, BW->bitmap.squareH)
/*
void HighlightSquare(BW, x, y)
    BitmapWidget BW;
    Position x, y;
{
    XFillRectangle(XtDisplay(BW), XtWindow(BW),
                   BW->bitmap.highlighting_gc,
		   InWindowX(BW, x), InWindowY(BW, y),
                   BW->bitmap.squareW, BW->bitmap.squareH);
}
*/

#define DrawSquare(BW, x, y)\
    XFillRectangle(XtDisplay(BW), XtWindow(BW),\
                   BW->bitmap.drawing_gc,\
		   InWindowX(BW, x), InWindowY(BW, y),\
                   BW->bitmap.squareW, BW->bitmap.squareH) 

/*
void DrawSquare(BW, x, y)
    BitmapWidget BW;
    Position x, y;
{
    XFillRectangle(XtDisplay(BW), XtWindow(BW),
                   BW->bitmap.drawing_gc,
		   InWindowX(BW, x), InWindowY(BW, y),
                   BW->bitmap.squareW, BW->bitmap.squareH);
}
*/

#define InvertPoint(BW, x, y)\
    {InvertBit(BW->bitmap.image, x, y); DrawSquare(BW, x, y);}

#define DrawPoint(BW, x, y, value)\
    if (GetBit(BW->bitmap.image, x, y) != value)\
       InvertPoint(BW, x, y)

void BWDrawPoint(w, x, y, value)
    Widget  w;
    Position      x, y;
    bit           value;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QueryInBitmap(BW, x, y)) {
	if (value == Highlight)
	    HighlightSquare(BW, x, y);
	else
	    DrawPoint(BW, x, y, value);
    }
}

XPoint *HotSpotShape(BW, x ,y)
    BitmapWidget BW;
    Position     x, y;
{
    static XPoint points[5];
  
    points[0].x = InWindowX(BW, x);
    points[0].y = InWindowY(BW, y + 1.0/2);
    points[1].x = InWindowX(BW, x + 1.0/2);
    points[1].y = InWindowY(BW, y + 1);
    points[2].x = InWindowX(BW, x + 1);
    points[2].y = InWindowY(BW, y + 1.0/2);
    points[3].x = InWindowX(BW, x + 1.0/2);
    points[3].y = InWindowY(BW, y);
    points[4].x = InWindowX(BW, x);
    points[4].y = InWindowY(BW, y + 1.0/2);
    
    return points;
}

#define DrawHotSpot(BW, x, y)\
  XFillPolygon(XtDisplay(BW), XtWindow(BW), BW->bitmap.drawing_gc,\
	       HotSpotShape(BW, x, y), 5, Convex, CoordModeOrigin)

#define HighlightHotSpot(BW, x, y)\
  XFillPolygon(XtDisplay(BW), XtWindow(BW), BW->bitmap.highlighting_gc,\
	       HotSpotShape(BW, x, y), 5, Convex, CoordModeOrigin)

XImage *CreateBitmapImage();
void DestroyBitmapImage();

void BWRedrawHotSpot(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;

    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y))
	DrawHotSpot(BW, BW->bitmap.hot.x, BW->bitmap.hot.y);
}

void BWClearHotSpot(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)) {
      DrawHotSpot(BW, BW->bitmap.hot.x, BW->bitmap.hot.y);
      BW->bitmap.hot.x = BW->bitmap.hot.y = NotSet;
    }
}

void BWDrawHotSpot(w, x, y, value)
    Widget w;
    Position x, y;
    int  value;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QueryInBitmap(BW, x, y)) {
	if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y) &&
	    ((BW->bitmap.hot.x == x) && (BW->bitmap.hot.y == y))) {
	    if ((value == Clear) || (value == Invert)) {
		BWClearHotSpot(w);
	    }
	}
	else
	    if ((value == Set) || (value == Invert)) {
		BWClearHotSpot(w);
		DrawHotSpot(BW, x, y);
		BW->bitmap.hot.x = x;
		BW->bitmap.hot.y = y;
	    }
	
	if (value == Highlight)
	    HighlightHotSpot(BW, x, y); 
    }
}

void BWSetHotSpot(w, x, y)
    Widget w;
    Position x, y;
{
    if (QuerySet(x, y))
	BWDrawHotSpot(w, x, y, Set);
    else
	BWClearHotSpot(w);
}

/* high level procedures */

void BWRedrawSquares(w, x, y, width, height)
    Widget  w;
    register Position x, y;
    Dimension width, height;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position from_x = InBitmapX(BW, x);
    Position from_y = InBitmapY(BW, y);
    Position to_x = InBitmapX(BW, x + width);
    Position to_y = InBitmapY(BW, y + height);

    QuerySwap(from_x, to_x);
    QuerySwap(from_y, to_y);
    from_x = max(0, from_x);
    from_y = max(0, from_y);
    to_x = min(BW->bitmap.image->width - 1, to_x);
    to_y = min(BW->bitmap.image->height - 1, to_y);
  
    for (x = from_x; x <= to_x; x++)
	for (y = from_y; y <= to_y; y++)
	    if (GetBit(BW->bitmap.image, x, y)) DrawSquare(BW, x, y);
}

void BWDrawGrid(w, from_x, from_y, to_x, to_y)
     Widget w;
     Position from_x, from_y,
              to_x, to_y;
{
    BitmapWidget BW = (BitmapWidget) w;
    int i;
  
    QuerySwap(from_x, to_x);
    QuerySwap(from_y, to_y);
    from_x = max(0, from_x);
    from_y = max(0, from_y);
    to_x = min(BW->bitmap.image->width - 1, to_x);
    to_y = min(BW->bitmap.image->height - 1, to_y);
  
    for(i = from_x + (from_x == 0); i <= to_x; i++)
	XDrawLine(XtDisplay(BW), XtWindow(BW), 
		  BW->bitmap.frame_gc,
		  InWindowX(BW, i), InWindowY(BW, from_y),
		  InWindowX(BW, i), InWindowY(BW, to_y + 1));
  
    for(i = from_y + (from_y == 0); i <= to_y; i++)
	XDrawLine(XtDisplay(BW), XtWindow(BW), 
		  BW->bitmap.frame_gc,
		  InWindowX(BW, from_x), InWindowY(BW, i),
		  InWindowX(BW, to_x + 1), InWindowY(BW, i));
}


void BWRedrawGrid(w, x, y, width, height)
    Widget w;
    Position x, y;
    Dimension width, height;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position from_x = InBitmapX(BW, x);
    Position from_y = InBitmapY(BW, y);
    Position to_x = InBitmapX(BW, x + width);
    Position to_y = InBitmapY(BW, y + height);

    if (BW->bitmap.grid)
	BWDrawGrid(w, from_x, from_y, to_x, to_y);
}

void BWDrawLine(w, from_x, from_y, to_x, to_y, value)
    Widget  w;
    Position      from_x, from_y,
                  to_x, to_y;
    int           value;
{
    Position i;
    register double x, y;
    double dx, dy, delta;

    dx = to_x - from_x;
    dy = to_y - from_y;
    x = from_x + 0.5;
    y = from_y + 0.5;
    delta = max(abs(dx), abs(dy));
    if (delta > 0) {
	dx /= delta;
	dy /= delta;
	for(i = 0; i <= delta; i++) {
	    BWDrawPoint(w, (Position) x, (Position) y, value);
	    x += dx;
	    y += dy;
	}
    }
    else
	BWDrawPoint(w, from_x, from_y, value);
}

void BWBlindLine(w, from_x, from_y, to_x, to_y, value)
    Widget  w;
    Position      from_x, from_y,
                  to_x, to_y;
    int           value;
{
    Position i;
    register double x, y;
    double dx, dy, delta;

    dx = to_x - from_x;
    dy = to_y - from_y;
    x = from_x + 0.5;
    y = from_y + 0.5;
    delta = max(abs(dx), abs(dy));
    if (delta > 0) {
	dx /= delta;
	dy /= delta;
	x += dx;
	y += dy;
	for(i = 1; i <= delta; i++) {
	    BWDrawPoint(w, (Position) x, (Position) y, value);
	    x += dx;
	    y += dy;
	}
    }
    else
	BWDrawPoint(w, from_x, from_y, value);
}

void BWDrawRectangle(w, from_x, from_y, to_x, to_y, value)
    Widget w;
    Position     from_x, from_y,
	to_x, to_y;
    int          value;
{
    register Position i;
    Dimension delta, width, height;
    
    QuerySwap(from_x, to_x);
    QuerySwap(from_y, to_y);
    
    width = to_x - from_x;
    height = to_y - from_y;

    delta = max(width, height);
    
    if (!QueryZero(width, height)) {
	for (i = 0; (int)i < (int)delta; i++) {
	    if ((int)i < (int)width) {
		BWDrawPoint(w, from_x + i, from_y, value);
		BWDrawPoint(w, to_x - i, to_y, value);
	    }
	    if ((int)i < (int)height) {
		BWDrawPoint(w, from_x, to_y - i, value);
		BWDrawPoint(w, to_x, from_y + i, value);
	    }
	}
    }
    else
	BWDrawLine(w, 
		   from_x, from_y, 
		   to_x, to_y, value);
}
  
void BWDrawFilledRectangle(w, from_x, from_y, to_x, to_y, value)
    Widget   w;
    Position from_x, from_y,
	     to_x, to_y;
    int      value;
{
    register Position x, y;

    QuerySwap(from_x, to_x);
    QuerySwap(from_y, to_y);
    
    for (x = from_x; x <= to_x; x++)
	for (y = from_y; y <= to_y; y++)
	    BWDrawPoint(w, x, y, value);
}

void BWDrawCircle(w, origin_x, origin_y, point_x, point_y, value)
    Widget  w;
    Position      origin_x, origin_y,
	point_x, point_y;
    int           value;
{
    register Position i, delta;
    Dimension dx, dy, half;
    double radius;
    
    dx = abs(point_x - origin_x);
    dy = abs(point_y - origin_y);
    radius = sqrt((double) ((int)dx * (int)dx + (int)dy * (int)dy));
    if (radius < 1.0) {
	BWDrawPoint(w, origin_x, origin_y, value);
    }
    else {
	BWDrawPoint(w, origin_x - (Position) floor(radius), origin_y, value);
	BWDrawPoint(w, origin_x + (Position) floor(radius), origin_y, value);
	BWDrawPoint(w, origin_x, origin_y - (Position) floor(radius), value);
	BWDrawPoint(w, origin_x, origin_y + (Position) floor(radius), value);
    }
    half = radius / sqrt(2.0);
    for(i = 1; (int)i <= (int)half; i++) {
	delta = sqrt(radius * radius - i * i);
	BWDrawPoint(w, origin_x - delta, origin_y - i, value);
	BWDrawPoint(w, origin_x - delta, origin_y + i, value);
	BWDrawPoint(w, origin_x + delta, origin_y - i, value);
	BWDrawPoint(w, origin_x + delta, origin_y + i, value);
	if (i != delta) {
	    BWDrawPoint(w, origin_x - i, origin_y - delta, value);
	    BWDrawPoint(w, origin_x - i, origin_y + delta, value);
	    BWDrawPoint(w, origin_x + i, origin_y - delta, value);
	    BWDrawPoint(w, origin_x + i, origin_y + delta, value);
	}
    }
}

void BWDrawFilledCircle(w, origin_x, origin_y, point_x, point_y, value)
    Widget  w;
    Position      origin_x, origin_y,
	          point_x, point_y;
    int           value;
{
    register Position i, j, delta;
    Dimension dx, dy;
    double radius;
    
    dx = abs(point_x - origin_x);
    dy = abs(point_y - origin_y);
    radius = sqrt((double) ((int)dx * (int)dx + (int)dy * (int)dy));
    for(j = origin_x - (Position) floor(radius); 
	j <= origin_x + (Position) floor(radius); j++)
	BWDrawPoint(w, j, origin_y, value);
    for(i = 1; i <= (Position) floor(radius); i++) {
	delta = sqrt(radius * radius - i * i);
	for(j = origin_x - delta; j <= origin_x + delta; j++) {
	    BWDrawPoint(w, j, origin_y - i, value);
	    BWDrawPoint(w, j, origin_y + i, value);
	}
    }
}

#define QueryFlood(BW, x, y, value)\
    ((GetBit(BW->bitmap.image, x, y) !=\
      (value & 1)) && QueryInBitmap(BW, x, y))

#define Flood(BW, x, y, value)\
    {if (value == Highlight) HighlightSquare(BW, x, y);\
     else InvertPoint(BW, x, y);}

/*
void FloodLoop(BW, x, y, value)
    BitmapWidget  BW;
    Position      x, y;
    int           value;
{
    if (QueryFlood(BW, x, y, value)) {
	Flood(BW, x, y, value);
	FloodLoop(BW, x, y - 1, value);
	FloodLoop(BW, x - 1, y, value);
	FloodLoop(BW, x, y + 1, value);
	FloodLoop(BW, x + 1, y, value);
    }
}
*/

void FloodLoop(BW, x, y, value)
    BitmapWidget BW;
    Position     x, y;
    int          value;
{
    Position save_x, save_y, x_left, x_right;
    
    if (QueryFlood(BW, x, y, value)) 
	Flood(BW, x, y, value)


    save_x = x;
    save_y = y;

    x++;
    while (QueryFlood(BW, x, y, value)) {
	Flood(BW, x, y, value);
	x++;
    }
    x_right = --x;

    x = save_x;
    x--;
    while (QueryFlood(BW, x, y, value)) {
	Flood(BW, x, y, value);
	x--;
    }
    x_left = ++x;


    x = x_left;
    y = save_y;
    y++;
    
    while (x <= x_right) {
	Boolean flag = False;
	Position x_enter;
	
	while (QueryFlood(BW, x, y, value) && (x <= x_right)) {
	    flag = True;
	    x++;
	}
	
	if (flag) {
	    if ((x == x_right) && QueryFlood(BW, x, y, value))
		FloodLoop(BW, x, y, value);
	    else
		FloodLoop(BW, x - 1, y, value);
	}
	
	x_enter = x;
	
	while (!QueryFlood(BW, x, y, value) && (x < x_right))
	    x++;
	
	if (x == x_enter) x++;
    }

    x = x_left;
    y = save_y;
    y--;

    while (x <= x_right) {
	Boolean flag = False;
	Position x_enter;
	
	while (QueryFlood(BW, x, y, value) && (x <= x_right)) {
	    flag = True;
	    x++;
	}
	
	if (flag) {
	    if ((x == x_right) && QueryFlood(BW, x, y, value))
		FloodLoop(BW, x, y, value);
	    else
		FloodLoop(BW, x - 1, y, value);
	}
	
	x_enter = x;
	
	while (!QueryFlood(BW, x, y, value) && (x < x_right))
	    x++;
	
	if (x == x_enter) x++;
    }
}

void BWFloodFill(w, x, y, value)
    Widget  w;
    Position      x, y;
    int           value;
{
    BitmapWidget BW = (BitmapWidget) w;
    int pixel;

    pixel = GetBit(BW->bitmap.image, x, y);

    if (value == Invert)
	FloodLoop(BW, x, y, (pixel ? Clear : Set));
    else if (value != pixel)
	FloodLoop(BW, x, y, value); 
}

#define QueryHotInMark(BW)\
    ((BW->bitmap.hot.x == max(BW->bitmap.mark.from_x,\
			      min(BW->bitmap.hot.x, BW->bitmap.mark.to_x)))\
     &&\
     (BW->bitmap.hot.y == max(BW->bitmap.mark.from_y,\
			      min(BW->bitmap.hot.y, BW->bitmap.mark.to_y))))

void BWUp(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    bit first, up, down;
    Position from_x, from_y, to_x, to_y;

    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }

    if ((to_y - from_y) == 0)
	return;
    
    for(x = from_x; x <= to_x; x++) {
	first = up = GetBit(BW->bitmap.image, x, to_y);
	for(y = to_y - 1; y >= from_y; y--) {
	    down = GetBit(BW->bitmap.image, x, y);
	    if (up != down) 
		InvertPoint(BW, x, y);
	    up =down;
	}
	if(first != down)
	    InvertPoint(BW, x, to_y);
    }

    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     BW->bitmap.hot.x,
		     (BW->bitmap.hot.y - 1 + BW->bitmap.image->height) % 
		     BW->bitmap.image->height);

}

void BWDown(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    bit first, down, up;
    Position from_x, from_y, to_x, to_y;

    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }

    if ((to_y - from_y) == 0)
	return;

    for(x = from_x; x <= to_x; x++) {
	first = down = GetBit(BW->bitmap.image, x, from_y);
	for(y = from_y + 1; y <= to_y; y++) {
	    up = GetBit(BW->bitmap.image, x, y);
	    if (down != up)
		InvertPoint(BW, x, y);
	    down = up;
	}
	if(first != up) 
	    InvertPoint(BW, x, from_y);
    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     BW->bitmap.hot.x,
		     (BW->bitmap.hot.y + 1) % BW->bitmap.image->height);
}

void BWLeft(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    bit first, left, right;
    Position from_x, from_y, to_x, to_y;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }

    if ((to_x - from_x) == 0)
	return;
    
    for(y = from_y; y <= to_y; y++) {
	first = left = GetBit(BW->bitmap.image, to_x, y);
	for(x = to_x - 1; x >= from_x; x--) {
	    right = GetBit(BW->bitmap.image, x, y);
	    if (left != right)
		InvertPoint(BW, x, y);
	    left = right;
	}
	if(first != right)
	    InvertPoint(BW, to_x, y);
    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     (BW->bitmap.hot.x - 1 + BW->bitmap.image->width) % 
		     BW->bitmap.image->width,
		     BW->bitmap.hot.y);
}

void BWRight(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    bit first, right, left;
    Position from_x, from_y, to_x, to_y;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }
    
    if ((to_x - from_x) == 0)
	return;
    
    for(y = from_y; y <= to_y; y++) {
	first = right = GetBit(BW->bitmap.image, from_x, y);
	for(x = from_x + 1; x <= to_x; x++) {
	    left = GetBit(BW->bitmap.image, x, y);
	    if (right != left)
		InvertPoint(BW, x, y);
	    right = left;
	}
	if(first != left)
	    InvertPoint(BW, from_x, y);
    }

    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     (BW->bitmap.hot.x + 1) % BW->bitmap.image->width,
		     BW->bitmap.hot.y);
}

void TransferImageData();

void BWFold(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position x, y, new_x, new_y;
    Dimension horiz, vert;
    char *storage_data;
    XImage *storage;
	
    storage_data = CreateCleanData(Length(BW->bitmap.image->width, 
					  BW->bitmap.image->height));

    storage = CreateBitmapImage(BW, storage_data, 
				(Dimension) BW->bitmap.image->width, 
				(Dimension) BW->bitmap.image->height);

    TransferImageData(BW->bitmap.image, storage);

    BW->bitmap.fold ^= True;
    horiz = (BW->bitmap.image->width + BW->bitmap.fold) / 2;
    vert = (BW->bitmap.image->height + BW->bitmap.fold) / 2;
    
    for (x = 0; x < BW->bitmap.image->width; x++)
	for (y = 0; y < BW->bitmap.image->height; y++) {
	    new_x = (int)(x + horiz) % (int)BW->bitmap.image->width;
	    new_y = (int)(y + vert) % (int)BW->bitmap.image->height;
	    if(GetBit(BW->bitmap.image, new_x, new_y) != 
	       GetBit(storage, x, y))
		InvertPoint(BW, new_x, new_y);
	}
    
    DestroyBitmapImage(&storage);

    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y))
      BWSetHotSpot(w, 
		   (Position) 
		   ((int)(BW->bitmap.hot.x+horiz)
		    %(int)BW->bitmap.image->width),
		   (Position)
		   ((int)(BW->bitmap.hot.y+vert)
		    %(int)BW->bitmap.image->height)
		   );
}


void BWClear(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    int i, length;

    length = Length(BW->bitmap.image->width, BW->bitmap.image->height);

    for (x = 0; x < BW->bitmap.image->width; x++)
	for (y = 0; y < BW->bitmap.image->height; y++)
	    if (GetBit(BW->bitmap.image, x, y))
		DrawSquare(BW, x, y);
    
    for (i = 0; i < length; i++)
	BW->bitmap.image->data[i] = 0;

}

void BWSet(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    int i, length;
    
    length = Length(BW->bitmap.image->width, BW->bitmap.image->height);
    
    for (x = 0; x < BW->bitmap.image->width; x++)
	for (y = 0; y < BW->bitmap.image->height; y++)
	    if (!GetBit(BW->bitmap.image, x, y))
		DrawSquare(BW, x, y);
    
    for (i = 0; i < length; i++)
	BW->bitmap.image->data[i] = 255;

}
 
void BWRedraw(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;

    XClearArea(XtDisplay(BW), XtWindow(BW),
	       0, 0, BW->core.width, BW->core.height,
	       True);
}

void BWInvert(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    int i, length;

    length = Length(BW->bitmap.image->width, BW->bitmap.image->height);

    XFillRectangle(XtDisplay(BW), XtWindow(BW),
		   BW->bitmap.drawing_gc,
		   InWindowX(BW, 0), InWindowY(BW, 0),
		   InWindowX(BW, BW->bitmap.image->width) - InWindowX(BW, 0),
		   InWindowY(BW, BW->bitmap.image->height) - InWindowY(BW, 0));
    
    for (i = 0; i < length; i++)
	BW->bitmap.image->data[i] ^= 255;
}

void BWFlipHoriz(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    Position from_x, from_y, to_x, to_y;
    float half;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }
    half = (float) (to_y - from_y) / 2.0 + 0.5;
    
    if (half == 0.0)
	return;
    
    for (x = from_x; x <= to_x; x++) 
	for (y = 0; y <  half; y++)
	    if (GetBit(BW->bitmap.image, x, from_y + y) != 
		GetBit(BW->bitmap.image, x, to_y - y)) {
		InvertPoint(BW, x, from_y + y);
		InvertPoint(BW, x, to_y - y);
	    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     BW->bitmap.hot.x,
		     BW->bitmap.image->height - 1 - BW->bitmap.hot.y);
}

void BWFlipVert(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    register Position x, y;
    Position from_x, from_y, to_x, to_y;
    float half;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }
    half = (float) (to_x - from_x) / 2.0 + 0.5;

    if (half == 0)
	return;

    for (y = from_y; y <= to_y; y++)
	for (x = 0; x < half; x++)
	    if (GetBit(BW->bitmap.image, from_x + x, y) != 
		GetBit(BW->bitmap.image, to_x - x, y)) {
		InvertPoint(BW, from_x + x, y);
		InvertPoint(BW, to_x - x, y);
	    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w))
	BWSetHotSpot(w,
		     BW->bitmap.image->width - 1 - BW->bitmap.hot.x,
		     BW->bitmap.hot.y);
}


void BWRotateRight(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position x, y, delta, shift, tmp;
    Position half_width, half_height;
    XPoint hot;
    bit quad1, quad2, quad3, quad4;
    Position from_x, from_y, to_x, to_y;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }

    half_width = floor((to_x - from_x) / 2.0 + 0.5);
    half_height = floor((to_y - from_y ) / 2.0 + 0.5);
    shift = min((Position)(to_x - from_x), (Position)(to_y - from_y )) % 2;
    delta = min((Position) half_width, (Position) half_height) - shift;
    
    for (x = 0; x <= delta; x++) {
	for (y = 1 - shift; y <= delta; y++) {
	    quad1 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width + x, 
			   from_y + (Position)half_height + y);
	    quad2 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width + y, 
			   from_y + (Position)half_height - shift - x);
	    quad3 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width - shift - x, 
			   from_y + (Position)half_height - shift - y);
	    quad4 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width - shift - y, 
			   from_y + (Position)half_height + x);

	    if (quad1 != quad2)
		InvertPoint(BW, 
			    from_x + (Position)half_width + x, 
			    from_y + (Position)half_height + y);
	    if (quad2 != quad3)
		InvertPoint(BW, 
			    from_x + (Position)half_width + y, 
			    from_y + (Position)half_height - shift - x);
	    if (quad3 != quad4)
		InvertPoint(BW, 
			    from_x + (Position)half_width - shift - x,
			    from_y + (Position)half_height - shift - y);
	    if (quad4 != quad1)
		InvertPoint(BW, 
			    from_x + (Position)half_width - shift - y, 
			    from_y + (Position)half_height + x);
	}
    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w)) {
	hot.x = BW->bitmap.hot.x - half_width;
	hot.y = BW->bitmap.hot.y - half_height;
	if (hot.x >= 0) hot.x += shift;
	if (hot.y >= 0) hot.y += shift;
	tmp = hot.x;
	hot.x = - hot.y;
	hot.y = tmp;
	if (hot.x > 0) hot.x -= shift;
	if (hot.y > 0) hot.y -= shift;
	hot.x += half_width;
	hot.y += half_height;
	if (QueryInBitmap(BW, hot.x, hot.y))
	    BWSetHotSpot(w, hot.x, hot.y);
    }
    
}

void BWRotateLeft(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position x, y,delta, shift, tmp;
    Position half_width, half_height;
    XPoint hot;
    bit quad1, quad2, quad3, quad4;
    Position from_x, from_y, to_x, to_y;
    
    if (BWQueryMarked(w)) {
	from_x = BW->bitmap.mark.from_x;
	from_y = BW->bitmap.mark.from_y;
	to_x = BW->bitmap.mark.to_x;
	to_y = BW->bitmap.mark.to_y;
    }
    else {
	from_x = 0;
	from_y = 0;
	to_x = BW->bitmap.width - 1;
	to_y = BW->bitmap.height - 1;
    }

    half_width = floor((to_x - from_x) / 2.0 + 0.5);
    half_height = floor((to_y - from_y ) / 2.0 + 0.5);
    shift = min((Position)(to_x - from_x), (Position)(to_y - from_y )) % 2;
    delta = min((Position) half_width, (Position) half_height) - shift;
    
    for (x = 0; x <= delta; x++) {
	for (y = 1 - shift; y <= delta; y++) {
	    quad1 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width + x, 
			   from_y + (Position)half_height + y);
	    quad2 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width + y, 
			   from_y + (Position)half_height - shift - x);
	    quad3 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width - shift - x, 
			   from_y + (Position)half_height - shift - y);
	    quad4 = GetBit(BW->bitmap.image, 
			   from_x + (Position)half_width - shift - y, 
			   from_y + (Position)half_height + x);

	    if (quad1 != quad4)
		InvertPoint(BW, 
			    from_x + (Position)half_width + x, 
			    from_y + (Position)half_height + y);
	    if (quad2 != quad1)
		InvertPoint(BW, 
			    from_x + (Position)half_width + y, 
			    from_y + (Position)half_height - shift - x);
	    if (quad3 != quad2)
		InvertPoint(BW, 
			    from_x + (Position)half_width - shift - x,
			    from_y + (Position)half_height - shift - y);
	    if (quad4 != quad3)
		InvertPoint(BW, 
			    from_x + (Position)half_width - shift - y, 
			    from_y + (Position)half_height + x);
	}
    }
    
    if (QuerySet(BW->bitmap.hot.x, BW->bitmap.hot.y)
	&&
	!BWQueryMarked(w)) {
	hot.x = BW->bitmap.hot.x - half_width;
	hot.y = BW->bitmap.hot.y - half_height;
	if (hot.x >= 0) hot.x += shift;
	if (hot.y >= 0) hot.y += shift;
	tmp = hot.x;
	hot.x = hot.y;
	hot.y = - tmp;
	if (hot.x > 0) hot.x -= shift;
	if (hot.y > 0) hot.y -= shift;
	hot.x += half_width;
	hot.y += half_height;
	if (QueryInBitmap(BW, hot.x, hot.y))
	    BWSetHotSpot(w, hot.x, hot.y);
    }
}


void CopyImageData(source, destination, from_x, from_y, to_x, to_y, at_x, at_y)
    XImage *source, *destination;
    Position  from_x, from_y, to_x, to_y, at_x, at_y;
{
    Position x, y, delta_x, delta_y;
    
    delta_x = to_x - from_x + 1;
    delta_y = to_y - from_y + 1;
    
    for (x = 0; x < delta_x; x++)
	for (y = 0; y < delta_y; y++)
	    if (GetBit(source, from_x + x, from_y + y))
		SetBit(destination, at_x + x, at_y + y);
	    else
		ClearBit(destination, at_x + x, at_y + y);
}
      
XImage *ConvertToBitmapImage(BW, image)
    BitmapWidget BW;
    XImage *image;
{
    XImage *bitmap_image;
    char   *data;
    Position x, y;
    
    data = CreateCleanData(Length(image->width, image->height));
    bitmap_image = CreateBitmapImage(BW, data, 
				     (Dimension) image->width, 
				     (Dimension) image->height);

    for (x = 0; x < min(image->width, bitmap_image->width); x++)
	for (y = 0; y < min(image->height, bitmap_image->height); y++)
	    if ((XGetPixel(image, x, y) != 0) != GetBit(bitmap_image, x, y))
		InvertBit(bitmap_image, x, y);

    return bitmap_image;
}

void TransferImageData(source, destination)
    XImage *source, *destination;
{
    Position x, y;
    
    for (x = 0; x < min(source->width, destination->width); x++)
	for (y = 0; y < min(source->height, destination->height); y++)
	    if (GetBit(source, x, y) != GetBit(destination, x, y))
		InvertBit(destination, x, y);
}

void BWStore(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    Dimension width, height;
    char *storage_data;

    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y)) {

	DestroyBitmapImage(&BW->bitmap.storage);

	width = BW->bitmap.mark.to_x - BW->bitmap.mark.from_x + 1;
	height = BW->bitmap.mark.to_y - BW->bitmap.mark.from_y + 1;
	
	storage_data = CreateCleanData(Length(width, height));

	BW->bitmap.storage = CreateBitmapImage(BW,
					       storage_data,
					       width, height);

	CopyImageData(BW->bitmap.image, BW->bitmap.storage,
		      BW->bitmap.mark.from_x,  BW->bitmap.mark.from_y,
		      BW->bitmap.mark.to_x,  BW->bitmap.mark.to_y,
		      0, 0);
    }
}

void BWClearMarked(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y))
	BWDrawFilledRectangle(w,
			      BW->bitmap.mark.from_x,
			      BW->bitmap.mark.from_y,
			      BW->bitmap.mark.to_x,
			      BW->bitmap.mark.to_y,
			      Clear);
}


void BWDragMarked(w, at_x, at_y)
    Widget w;
    Position     at_x, at_y;
{
    BitmapWidget BW = (BitmapWidget) w;

    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y))
	BWDrawRectangle(w, 
			at_x, at_y, 
			at_x + BW->bitmap.mark.to_x - BW->bitmap.mark.from_x,
			at_y + BW->bitmap.mark.to_y - BW->bitmap.mark.from_y,
			Highlight);
}

void BWDragStored(w, at_x, at_y)
    Widget       w;
    Position     at_x, at_y;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (BW->bitmap.storage)
	BWDrawRectangle(w, 
			at_x, at_y,
			at_x + BW->bitmap.storage->width - 1,
			at_y + BW->bitmap.storage->height - 1,
			Highlight);
}

void DrawImageData(BW, image, at_x, at_y, value)
    BitmapWidget BW;
    XImage       *image;
    Position     at_x, at_y;
    int          value;
{
    Position x, y;
    Boolean  C, S, I, H;
    bit      A, B;

    C = value == Clear;
    S = value == Set;
    I = value == Invert;
    H = value == Highlight;

    for (x = 0; x < image->width; x++) 
	for (y = 0; y < image->height; y++) {
	    A = GetBit(image, x, y);
	    B = GetBit(BW->bitmap.image, at_x + x, at_y + y);
	    if (A & C | (A | B) & S | (A ^ B) & I | (A | B) & H)
		value = (A & H) ? Highlight : Set;
	    else
		value = Clear;
	    BWDrawPoint((Widget) BW, 
			 at_x + x, at_y + y, 
			 value);
	}
}

void BWRestore(w, at_x, at_y, value)
    Widget       w;
    Position     at_x, at_y;
    int          value;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (BW->bitmap.storage) {
      DrawImageData(BW, BW->bitmap.storage, at_x, at_y, value);
      /*DestroyBitmapImage(&BW->bitmap.storage);*/
    }
}

void BWCopy(w, at_x, at_y, value)
    Widget       w;
    Position     at_x, at_y;
    int          value;
{
    BitmapWidget BW = (BitmapWidget) w;
    XImage *storage;
    char *storage_data;
    Dimension width, height;

    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y)) {

	width = BW->bitmap.mark.to_x - BW->bitmap.mark.from_x + 1;
	height = BW->bitmap.mark.to_y - BW->bitmap.mark.from_y + 1;
	
	storage_data = CreateCleanData(Length(width, height));

	storage = CreateBitmapImage(BW, storage_data, width, height);

	CopyImageData(BW->bitmap.image, storage,
		      BW->bitmap.mark.from_x,  BW->bitmap.mark.from_y,
		      BW->bitmap.mark.to_x,  BW->bitmap.mark.to_y,
		      0, 0);

	DrawImageData(BW, storage, at_x, at_y, value);

	DestroyBitmapImage(&storage);
    }
}

void BWMark();

void BWMove(w, at_x, at_y, value)
    Widget   w;
    Position at_x, at_y;
    int      value;
{
    BitmapWidget BW = (BitmapWidget) w;
    XImage *storage;
    char *storage_data;
    Dimension width, height;

    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y)) {

	width = BW->bitmap.mark.to_x - BW->bitmap.mark.from_x + 1;
	height = BW->bitmap.mark.to_y - BW->bitmap.mark.from_y + 1;
	
	storage_data = CreateCleanData(Length(width, height));

	storage = CreateBitmapImage(BW, storage_data, width, height);

	CopyImageData(BW->bitmap.image, storage,
		      BW->bitmap.mark.from_x,  BW->bitmap.mark.from_y,
		      BW->bitmap.mark.to_x,  BW->bitmap.mark.to_y,
		      0, 0);

	BWDrawFilledRectangle(w,
			      BW->bitmap.mark.from_x, BW->bitmap.mark.from_y,
			      BW->bitmap.mark.to_x, BW->bitmap.mark.to_y,
			      Clear);

	DrawImageData(BW, storage, at_x, at_y, value);

	BWMark(w, at_x, at_y,
	     at_x + BW->bitmap.mark.to_x - BW->bitmap.mark.from_x,
	     at_y + BW->bitmap.mark.to_y - BW->bitmap.mark.from_y);

	DestroyBitmapImage(&storage);
    }
}

void BWRedrawMark(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y)) 
	XFillRectangle(XtDisplay(BW), XtWindow(BW), BW->bitmap.highlighting_gc,
		       InWindowX(BW, BW->bitmap.mark.from_x), 
		       InWindowY(BW, BW->bitmap.mark.from_y), 
		       InWindowX(BW, BW->bitmap.mark.to_x + 1) - 
		       InWindowX(BW, BW->bitmap.mark.from_x),
		       InWindowY(BW, BW->bitmap.mark.to_y + 1) -
		       InWindowY(BW, BW->bitmap.mark.from_y));
}

void BWStoreToBuffer(w)
     Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    
    bcopy(BW->bitmap.image->data, BW->bitmap.buffer->data,
	  Length(BW->bitmap.image->width, BW->bitmap.image->height));

    BW->bitmap.buffer_hot = BW->bitmap.hot;
    BW->bitmap.buffer_mark = BW->bitmap.mark;
}

void BWUnmark(w)
     Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;

    BW->bitmap.buffer_mark = BW->bitmap.mark;

    if (QuerySet(BW->bitmap.mark.from_x, BW->bitmap.mark.from_y)) {
	XFillRectangle(XtDisplay(BW), XtWindow(BW), BW->bitmap.highlighting_gc,
		       InWindowX(BW, BW->bitmap.mark.from_x), 
		       InWindowY(BW, BW->bitmap.mark.from_y), 
		       InWindowX(BW, BW->bitmap.mark.to_x + 1) - 
		       InWindowX(BW, BW->bitmap.mark.from_x),
		       InWindowY(BW, BW->bitmap.mark.to_y + 1) -
		       InWindowY(BW, BW->bitmap.mark.from_y));
    
	BW->bitmap.mark.from_x = BW->bitmap.mark.from_y = NotSet;
	BW->bitmap.mark.to_x = BW->bitmap.mark.to_y = NotSet;
    }
}

void BWMark(w, from_x, from_y, to_x, to_y)
    Widget w;
    Position from_x, from_y,
	     to_x, to_y;
{
    BitmapWidget BW = (BitmapWidget) w;

    BWUnmark(w);
    
    if (QuerySet(from_x, from_y)) {
	if ((from_x == to_x) && (from_y == to_y)) {
	    /*
	      BW->bitmap.mark.from_x = 0;
	      BW->bitmap.mark.from_y = 0;
	      BW->bitmap.mark.to_x = BW->bitmap.image->width - 1;
	      BW->bitmap.mark.to_y = BW->bitmap.image->height - 1;
	      */
	    return;
	}
	else {
	    QuerySwap(from_x, to_x);
	    QuerySwap(from_y, to_y);
	    from_x = max(0, from_x);
	    from_y = max(0, from_y);
	    to_x = min(BW->bitmap.image->width - 1, to_x);
	    to_y = min(BW->bitmap.image->height - 1, to_y);
	    
	    BW->bitmap.mark.from_x = from_x;
	    BW->bitmap.mark.from_y = from_y;
	    BW->bitmap.mark.to_x = to_x;
	    BW->bitmap.mark.to_y = to_y;
	}
	
	XFillRectangle(XtDisplay(BW), XtWindow(BW), BW->bitmap.highlighting_gc,
		       InWindowX(BW, BW->bitmap.mark.from_x),
		       InWindowY(BW, BW->bitmap.mark.from_y), 
		       InWindowX(BW, BW->bitmap.mark.to_x + 1) -
		       InWindowX(BW, BW->bitmap.mark.from_x),
		       InWindowY(BW, BW->bitmap.mark.to_y +1) - 
		       InWindowY(BW, BW->bitmap.mark.from_y));
    }
}

void BWMarkAll(w)
    Widget w;
{
  BitmapWidget BW = (BitmapWidget) w;

  BWMark(w, 0, 0, BW->bitmap.image->width - 1, BW->bitmap.image->height - 1);
}

void BWUndo(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;
    Position x, y;
    char *tmp_data;
    XPoint tmp_hot;
    BWArea tmp_mark;
    
    tmp_data = BW->bitmap.image->data;
    BW->bitmap.image->data = BW->bitmap.buffer->data;
    BW->bitmap.buffer->data = tmp_data;

    tmp_hot = BW->bitmap.hot;
    tmp_mark = BW->bitmap.mark;
    
    for (x = 0; x < BW->bitmap.image->width; x++)
	for (y = 0; y < BW->bitmap.image->height; y++)
	 if (GetBit(BW->bitmap.image, x, y) != GetBit(BW->bitmap.buffer, x, y))
	     DrawSquare(BW, x, y);

    BWSetHotSpot(w, BW->bitmap.buffer_hot.x, BW->bitmap.buffer_hot.y);
/*    
    BWMark(w, BW->bitmap.buffer_mark.from_x, BW->bitmap.buffer_mark.from_y,
	   BW->bitmap.buffer_mark.to_x, BW->bitmap.buffer_mark.to_y);
*/
    BW->bitmap.buffer_hot = tmp_hot;
    BW->bitmap.buffer_mark= tmp_mark;

}

void BWHighlightAxes(w)
    Widget w;
{
    BitmapWidget BW = (BitmapWidget) w;

    XDrawLine(XtDisplay(BW), XtWindow(BW),
	      BW->bitmap.axes_gc,
	      InWindowX(BW, 0), 
	      InWindowY(BW, 0),
	      InWindowX(BW, BW->bitmap.width),
	      InWindowY(BW, BW->bitmap.height));
    XDrawLine(XtDisplay(BW), XtWindow(BW),
	      BW->bitmap.axes_gc,
	      InWindowX(BW, BW->bitmap.width),
	      InWindowY(BW, 0), 
	      InWindowX(BW, 0),
	      InWindowY(BW, BW->bitmap.height));
    XDrawLine(XtDisplay(BW), XtWindow(BW),
	      BW->bitmap.axes_gc,
	      InWindowX(BW, 0),
	      InWindowY(BW, (float)BW->bitmap.height / 2.0),
	      InWindowX(BW, BW->bitmap.width),
	      InWindowY(BW, (float)BW->bitmap.height / 2.0));
    XDrawLine(XtDisplay(BW), XtWindow(BW),
	      BW->bitmap.axes_gc,
	      InWindowX(BW, (float)BW->bitmap.width / 2.0),
	      InWindowY(BW, 0),
	      InWindowX(BW, (float)BW->bitmap.width / 2.0),
	      InWindowY(BW, BW->bitmap.height));
}
    
typedef struct {
    Position *x, *y;
    Dimension *width, *height;
} Table;

XImage *ScaleBitmapImage(BW, src, scale_x, scale_y)
    BitmapWidget BW;
    XImage *src;
    double scale_x, scale_y;
{
    char *data;
    XImage *dst;
    Table table;    
    Position x, y, w, h;
    Dimension width, height;
    bit pixel;

    width = max(rint(scale_x * src->width), 1);
    height = max(rint(scale_y * src->height), 1);

    data = CreateCleanData(Length(width, height));
    dst = CreateBitmapImage(BW, data, width, height);

    /*
     * It would be nice to check if width or height < 1.0 and
     * average the skipped pixels. But, it is slow as it is now.
     */
    if (scale_x == 1.0 && scale_y == 1.0)
	bcopy (src->data, dst->data, Length(width, height));
    else {
	table.x = (Position *) XtMalloc(sizeof(Position) * src->width);
	table.y = (Position *) XtMalloc(sizeof(Position) * src->height);
	table.width = (Dimension *) XtMalloc(sizeof(Dimension) * src->width);
	table.height = (Dimension *) XtMalloc(sizeof(Dimension) * src->height);
	
	for (x = 0; x < src->width; x++) {
	    table.x[x] = rint(scale_x * x);
	    table.width[x] = rint(scale_x * (x + 1)) - rint(scale_x * x);
	}
	for (y = 0; y < src->height; y++) {
	    table.y[y] = rint(scale_y * y);
	    table.height[y] = rint(scale_y * (y + 1)) - rint(scale_y * y);
	}
	
	for (x = 0; x < src->width; x++)
	    for (y = 0; y < src->height; y++) {
	        pixel = GetBit(src, x, y);
		for (w = 0; (int)w < (int)table.width[x]; w++)
		    for (h = 0; (int)h < (int)table.height[y]; h++)
			if (pixel) SetBit(dst, 
					table.x[x] + w, 
					table.y[y] + h);
	    }

	XtFree((char *)table.x);
	XtFree((char *)table.y);
	XtFree((char *)table.width);
	XtFree((char *)table.height);
    }
    
    return (dst);
}

/*****************************************************************************/
