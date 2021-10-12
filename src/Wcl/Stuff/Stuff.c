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
/* SCCS_data: @(#) Stuff.c 1.1 92/03/18 10:55:05
*/

#include <X11/Intrinsic.h>
#include <X11/Wc/WcCreate.h>
#include <stdio.h>

void PrintCB( w, args, data )
    Widget	w;
    char*	args;
    XtPointer	data;
{
    fprintf( stderr, "PrintCB( %s, %s, (??)%d )\n", XtName(w), args, (int)data);
}

void PrintACT( w, event, params, num_params )
    Widget	w;
    XEvent*	event;
    char**	params;
    Cardinal*	num_params;
{
    int num = *num_params;

    fprintf( stderr, "PrintACT( %s, ?event?, (", XtName(w) );
    if ( num-- )
	fprintf( stderr, "%s", *params++ );
    while ( num-- )
	fprintf( stderr, ", %s", *params++ );
    fprintf( stderr, "), %d )\n", *num_params);
}
