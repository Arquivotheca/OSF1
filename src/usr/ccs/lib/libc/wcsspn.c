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
#ifndef lint
static char	*sccsid = "@(#)$RCSfile: wcsspn.c,v $ $Revision: 4.2.5.2 $ (DEC) $Date: 1993/06/08 00:08:55 $";
#endif 
/*
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * OSF/1 1.2
 */
/*
 * COMPONENT_NAME: (LIBCNLS) Standard C Library National Language Support
 *
 * FUNCTIONS: wcsspn
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1989 
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1984 AT&T	
 * All Rights Reserved  
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	
 * The copyright notice above does not evidence any   
 * actual or intended publication of such source code.
 *
 * wcsspn.c	1.4  com/lib/c/nls,3.1,9013 2/27/90 21:45:24
 */

/* name space pollution clean up */
#ifdef _NAME_SPACE_WEAK_STRONG
#include "pollution.h"
#if !defined(_THREAD_SAFE)
#pragma weak wcsspn = __wcsspn
#endif
#endif
#include <sys/types.h>

/*
 * NAME: wcsspn
 *
 * FUNCTION: Compute the number of wchar_t characters in the initial
 *  segment of the string pointed to by string1, which consists entirely of
 *  wchar_t characters from the string pointed to by string2.
 *
 * PARAMETERS:
 *	wchar_t *string1	-	the wide character string
 *	wchar_t *string2	-	the wide character string
 *
 * RETURN VALUE DESCRIPTION: the number of wchar_t in the segment.
 */
size_t 
#ifdef _NO_PROTO
wcsspn(string1, string2)
wchar_t *string1,*string2;
#else
wcsspn(const wchar_t *string1,const wchar_t *string2)
#endif
{
	register const wchar_t *q;
	register const wchar_t *p;

	for(q=string1; *q != '\0'; q++) {
		for (p = string2; *p != 0 && *p != *q; ++p)
			;
		if(*p == 0)
			break;
	}
	return(q - string1);
}
