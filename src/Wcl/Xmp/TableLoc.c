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
/*LINTLIBRARY*/
/*
 * SCCS_data:    @(#) TableLoc.c	1.5 92/06/10 06:14:02
 */
#include <X11/Xmp/COPY>
#include <X11/IntrinsicP.h>
#include <X11/Xmp/TableP.h>

/* XmpTableLoc methods
**========================**
   An XmpTable widget keeps its default_layout and real_layout as two
   XmpTableLoc's, which are pointers to the first element of a null
   terminated array of XmpTableLocRecs.

   Each XmpTableLocRec defines a XmpTable widget child's location information
   (the row, col, spans, and layout options like justification and sizing
   controls).  The default_layout describes where widgets may be placed
   if and when they become managed.  The real_layout describes where managed
   children are actually placed.

   The default_layout differs slightly from the real_layout:  Initially,
   it knows only the names of widgets (their quarks) as specified by the
   layout resource.  As time goes on and widgets become managed or are
   placed, then the default_layout may also learn the actual identities of
   widgets.  Note that it is possible and sometimes very useful to have
   multiple children with the same name, but different positions and/or
   options and/or sizes.  If a table has multiple children of the same
   name, then there may be multiple default_layout and real_layout locs
   with the same quark but different widgets.

   The default_layout is created from a string by the XmpTableLocParse
   function which is usually called via the converter.  It is changed due
   to positioning and XtSetValues calls.  The real_layout is calculated by
   the table's layout method, triggered by its ChangeManaged method and
   by the Positioning or SetValues methods.
*/

/* Allocate, Grow, and Free Arrays of XmpTableLocRec's
**=====================================================**
*/
XmpTableLoc XmpTableLocNew( n )
    int n;
{
    return (XmpTableLoc)XtCalloc( n+1, sizeof(XmpTableLocRec) );
}

XmpTableLoc XmpTableLocGrow( loc )
    XmpTableLoc loc;
{
    static XmpTableLocRec nullLoc;	/* all zeros, as if XtCalloc'd */

    int len = XmpTableLocLen( loc );

    /* XtRealloc leaves additional loc + terminating loc as garbage...
    */
    XmpTableLoc new = (XmpTableLoc)XtRealloc( (char*)loc,
						(len+2)*sizeof(XmpTableLocRec) );

    /* ... so initialize the additional and terminating loc to NULLs
    */
    new[len] = nullLoc;
    new[len+1] = nullLoc;

    /* Now new[XmpTableLocLen(new)] is the additional location
    */
    return new;
}

XmpTableLoc XmpTableLocCopy( loc )
    XmpTableLoc loc;
{
    int len = XmpTableLocLen( loc );
    XmpTableLoc copy = XmpTableLocNew( len );
    while (len--)
	copy[len] = loc[len];
    return copy;
}

void XmpTableLocFree( loc )
    XmpTableLoc loc;
{
    XtFree( (char*)loc );
}


/* Parse Layout String
**=====================**
Parse a layout string, allocating and setting the values.  Return
pointer to first element, or NULL if parse fails for any reason.
The result is suitable for use in a call to XtSetValues().

A layout is a list of location specifications separated by semi-colons.
Each location specification has the form:

	widget_name  column  row  col_span  row_span  opt_list

where the meaning of each field is:

	widget_name	Name of the widget as given to XtCreateWidget().
	column		Integer >= 0 descibing column in array
	row		Row >= 0 describing row in array
	col_span	Integer >= 1 describing horizontal widget span
	row_span	Integer >= 1 describing vertical widget span
	opt_list	Series of characters each representing an option:
				l:  TBL_LEFT
				r:  TBL_RIGHT
				t:  TBL_TOP
				b:  TBL_BOTTOM
				w:  TBL_LK_WIDTH
				h:  TBL_LK_HEIGHT
				W:  TBL_SM_WIDTH
				H:  TBL_SM_HEIGHT

The options are interpreted in the XmpTableChildPosition() method.
*/

XmpTableLoc XmpTableLocParse( layout )
    char*		layout;
{
#ifndef CHILD_NAME_LEN
#define CHILD_NAME_LEN 127
#endif
    char	buf[CHILD_NAME_LEN+1];
    int		numLocs;
    XmpTableLoc	locs;		/* array XtCalloc'd and returned	*/
    XmpTableLoc	loc;		/* current location being parsed	*/
    int		thisLoc;	/* index of current loc			*/
    int		i;
    char*	cp;

    if ( layout == NULL || *layout == '\0' )
	return (XmpTableLoc)NULL;

    /* Figure out how many location specification there are in the layout.
    ** Each location specifier may be semi-colon SEPARATED, so we may
    ** have one more than the number of semi-colons.  Space for null
    ** termination is provided by XmpTableLocNew().
    */
    for ( numLocs = 1, cp = layout ; *cp ; cp++ )
	if (*cp == ';') numLocs++;

    /* XmpTableLocNew() provides additional NULL location so we do not
    ** need logic to null terminate array.
    */
    locs = XmpTableLocNew( numLocs );

    loc = locs;
    thisLoc = 0;
    cp = layout;

#define EAT_WHITESPACE(cp) while (*cp && *cp <= ' ') cp++;

    EAT_WHITESPACE(cp)

    while ( *cp && ++thisLoc <= numLocs )
    {
	/* Parse a location specification from the layout string
	*/

	for (i = 0 ; ' ' < *cp && i < CHILD_NAME_LEN ; i++, cp++ )
	    buf[i] = *cp;
	buf[i] = '\0';

	if ( i )
	    loc->w_quark = XrmStringToQuark(buf);	/* widget name */

	EAT_WHITESPACE(cp)

	while ('0' <= *cp && *cp <= '9')
	    loc->col = loc->col * 10 + *cp++ - '0';

	EAT_WHITESPACE(cp)

	while ('0' <= *cp && *cp <= '9')
	    loc->row = loc->row * 10 + *cp++ - '0';

	EAT_WHITESPACE(cp)

	while ('0' <= *cp && *cp <= '9')
	    loc->col_span = loc->col_span * 10 + *cp++ - '0';
	if (loc->col_span == 0)
	    loc->col_span = 1;		/* default span */

	EAT_WHITESPACE(cp)

	while ('0' <= *cp && *cp <= '9')
	    loc->row_span = loc->row_span * 10 + *cp++ - '0';
	if (loc->row_span == 0)
	    loc->row_span = 1;		/* default span */

	EAT_WHITESPACE(cp)

	i = 0;
	while ( *cp && i < CHILD_NAME_LEN &&
		*cp == 'l' || *cp == 'r' || *cp == 't' || *cp == 'b' ||
		*cp == 'w' || *cp == 'h' || *cp == 'W' || *cp == 'H' )
	    buf[i++] = *cp++;
	buf[i] = '\0';
	if ( i )
	    loc->options = XmpTableOptsParse(buf);

	while (*cp && *cp <= ' ' || *cp == ';' ) cp++;

	loc++;
    }

    if (*cp )
    {
	/* Something went wrong.
	*/
	XmpTableLocFree( locs );
	locs = (XmpTableLoc)NULL;
    }
    return locs;
}


int XmpTableLocLen(loc)
    XmpTableLoc loc;
{
    int i = 0;

    for ( i = 0 ;  loc && loc->w_quark != NULLQUARK  ; loc++ )
	i++;
    return i;
}

/* Find things in TableLocs
**==============================**
   Linear search of TableLoc array looking for various parameters
*/

XmpTableLoc XmpTableLocFind( loc, w )
    XmpTableLoc	loc;		/* Table Locations to examine	*/
    Widget	w;		/* Widget to find		*/
{
    if ( loc && w )
    {
	for ( ;  loc->w_quark != NULLQUARK  ; loc++ )
	{
	    if ( loc->w_quark == w->core.xrm_name &&
		 loc->w == w )
		return loc;
	}
    }
    return (XmpTableLoc)0;
}

XmpTableLoc XmpTableLocFindDefault( loc, w )
    XmpTableLoc	loc;		/* Table Locations to examine   */
    Widget	w;		/* Widget to find		*/
{
    if ( loc && w )
    {
	for ( ;  loc->w_quark != NULLQUARK  ; loc++ )
	{
	    if ( loc->w_quark == w->core.xrm_name && loc->w == (Widget)0 )
		return loc;
	}
    }
    return (XmpTableLoc)0;
}

XmpTableLoc XmpTableLocFindAtPosition( loc, col, row )
    XmpTableLoc	loc;		/* Table Locations to examine	*/
    int		col, row;	/* position of widget to find	*/
{
    if ( loc &&  (0 <= col) && (0 <= row) )
    {
	for ( ;  loc->w_quark != NULLQUARK  ; loc++ )
	{
	    if ( loc->col == col && loc->row == row )
		return loc;
	}
    }
    return (XmpTableLoc)0;
}	


/* Preferred size determination:
**==============================**
   If a widget is very simple (does not have a query_geometry method),
   XtQueryGeometry() returns the current size.  This has the unfortunate
   effect of causing the XmpTable to never shrink such very simple
   children.

   A new optional resource "shrinkSimple" (which can be set to False in
   order to mimic the behavior of old versions of XmpTable) causes the
   XmpTable to figure that very simple widgets will prefer their original
   size: i.e., they will shrink if all the more complex widgets in a row
   or column shrink.
*/

int XmpTableLocPreferredWidth( loc, shrink_simple )
    XmpTableLoc	loc;		/* preferred size of widget in this loc	*/
    int		shrink_simple;	/* from parent XmpTableWidget		*/
{
    if ( shrink_simple &&  XtClass(loc->w)->core_class.query_geometry == NULL )
    {
	return loc->orig_width;
    }
    else
    {
	XtWidgetGeometry	child;
	int			width	= loc->w->core.width;
	int			border	= loc->w->core.border_width;

	(void)XtQueryGeometry(loc->w, (XtWidgetGeometry*)NULL, &child);

	if (child.request_mode & CWWidth)	width  = child.width;
	if (child.request_mode & CWBorderWidth) border = child.border_width;

	return width + 2*border;
    }
}

int XmpTableLocPreferredHeight( loc, shrink_simple )
    XmpTableLoc	loc;		/* preferred size of widget in this loc	*/
    int		shrink_simple;	/* from parent XmpTableWidget		*/
{
    if ( shrink_simple &&  XtClass(loc->w)->core_class.query_geometry == NULL )
    {
	return loc->orig_height;
    }
    else
    {
	XtWidgetGeometry	child;
	int			height	= loc->w->core.height;
	int			border	= loc->w->core.border_width;

	(void)XtQueryGeometry(loc->w, (XtWidgetGeometry*)NULL, &child);

	if (child.request_mode & CWHeight)	height = child.height;
	if (child.request_mode & CWBorderWidth) border = child.border_width;

	return height + 2*border;
    }
}

int XmpTableLocNumCols( loc )
    XmpTableLoc loc;
{
    int cols;
    for ( cols = 0  ;  loc && loc->w_quark != NULLQUARK  ; loc++ )
	if ( cols < (loc->col + loc->col_span) )
	    cols = loc->col + loc->col_span;
    return cols;
}

int XmpTableLocNumRows( loc )
    XmpTableLoc loc;
{
    int rows;
    for ( rows = 0  ;  loc && loc->w_quark != NULLQUARK  ;  loc++)
        if ( rows < (loc->row + loc->row_span) )
            rows = loc->row + loc->row_span;
    return rows;
}

/* Used by qsort when the real_layout table is sorted by
** span before doing distribution of space to rows or columns.
*/
int XmpTableLocCompareColSpan( loc1, loc2 )
    XmpTableLoc loc1, loc2;
{
    return loc1->col_span - loc2->col_span;
}

int XmpTableLocCompareRowSpan( loc1, loc2 )
    XmpTableLoc loc1, loc2;
{
    return loc1->row_span - loc2->row_span;
}
