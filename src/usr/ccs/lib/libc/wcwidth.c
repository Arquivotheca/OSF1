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
static char *rcsid = "@(#)$RCSfile: wcwidth.c,v $ $Revision: 1.1.6.2 $ (DEC) $Date: 1993/06/08 20:45:59 $";
#endif
/*
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * OSF/1 1.2
 */

/*
 * COMPONENT_NAME: (LIBCCCPPC) LIBC Code-Point/Process-Code Conversion Functions
 *
 * FUNCTIONS: wcwidth
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1991 
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * 1.2  com/lib/c/cppc/wcwidth.c, libccppc, 9130320 7/17/91 15:13:27
 */

/* name space pollution clean up */
#ifdef _NAME_SPACE_WEAK_STRONG
#include "pollution.h"
#if !defined(_THREAD_SAFE)
#pragma weak wcwidth = __wcwidth
#endif
#endif
#include <stdlib.h>
#include <ctype.h>
#include <sys/localedef.h>

/*
  returns the number of characters for a SINGLE-BYTE codeset
*/
int
__wcwidth_latin(wchar_t wc, _LC_charmap_t *hdl)
{
    /**********
      if wc is null, return 0
    **********/
    if (wc == (wchar_t) '\0')
	return(0);

    /**********
      if wc is greater than 255, return -l
    **********/
    if (wc > 255)
	return(-1);

    /**********
      single-display width
    **********/
    return(1);

}


int 
wcwidth(wchar_t wc)
{
    	if (wc == '\0')		/* NUL => zero width */
	    	return (0);
	else if (!iswprint(wc))	/* Nonprintable => ERROR */
	    	return (-1);

	IF_METHOD(__lc_charmap, wcwidth)
		return METHOD(__lc_charmap, wcwidth)( wc, __lc_charmap);
	else
		return __wcwidth_latin(wc, __lc_charmap);
}
