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
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: __sscans.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1993/06/12 20:26:37 $";
#endif
/*
 * HISTORY
 */
/*
 */ 
/*** "__sscans.c	1.6  com/lib/curses,3.1,8943 10/16/89 22:52:56"; ***/
/*
 * COMPONENT_NAME: (LIBCURSE) Curses Library
 *
 * FUNCTIONS:   __sscans
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1988
 * All Rights Reserved
 * Licensed Material - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

# include	"cursesext.h"
# include	<varargs.h>

/*
 * NAME:        __sscans
 *
 * FUNCTION:    executes the scanf from the window.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This code calls vsscanf, which is like sscanf except
 *      that it takes a va_list as an argument pointer instead
 *      of the argument list itself.  We provide one until
 *      such a routine becomes available.
 */

#define	MAX_BUF	256

__sscans(win, fmt, ap)
WINDOW	*win;
char	*fmt;
va_list	ap;
{
	char	buf[MAX_BUF*4];
	wchar_t	wbuf[MAX_BUF];

	if (wgetwstr(win, wbuf) == ERR)
		return ERR;
	if (wcstombs(buf, wbuf, (MAX_BUF*4)) == -1)
		return ERR;
	return vsscanf(buf, fmt, ap);
}
