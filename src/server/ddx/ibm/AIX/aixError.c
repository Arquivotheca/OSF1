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
 * $XConsortium: aixError.c,v 1.2 91/07/16 12:59:01 jap Exp $
 *
 * Copyright IBM Corporation 1987,1988,1989,1990,1991
 *
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS, AND 
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/

#include <stdio.h>
#include "aixError.h"

int	(*errExitFunc)()=	NULL;
int	errAllow=	ERR_INFO|ERR_WARNING|ERR_ERROR|ERR_FATAL|ERR_WSGO;
int	errThisLevel=	ERR_NONE;

/***====================================================================***/

/*VARARGS*/
void
errMsg(msg,arg1,arg2,arg3)
char	*msg;
char	*arg1,*arg2,*arg3;
{
int	 format;

   if (errThisLevel==ERR_NONE)	return;
   if (errThisLevel&errAllow) {
	if	(errThisLevel==ERR_INFO)	/* do nothing */;
	else if	(errThisLevel==ERR_ACTION)	ErrorF("            ");
	else if	(errThisLevel==ERR_WARNING)	ErrorF("Warning!    ");
	else if	(errThisLevel==ERR_ERROR)	ErrorF("Error!      ");
	else if	(errThisLevel==ERR_FATAL)	ErrorF("Fatal!      ");
	else if	(errThisLevel==ERR_WSGO)	ErrorF("WSGO!       ");
	else					ErrorF("Meta-error! ");
	ErrorF(msg,arg1,arg2,arg3);
	ErrorF("\n");
	if (errThisLevel&ERR_FATAL) 
	    ErrorF("            Exiting.\n");
   }

   if (errThisLevel&ERR_FATAL) {
       if (errExitFunc)
          (*errExitFunc)();
       abort();
   }
   errThisLevel= ERR_NONE;
}
