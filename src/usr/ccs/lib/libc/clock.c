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
static char	*sccsid = "@(#)$RCSfile: clock.c,v $ $Revision: 4.2.7.2 $ (DEC) $Date: 1993/06/07 22:41:35 $";
#endif 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
 */
#if !defined(lint) && !defined(_NOIDENT)

#endif
/*
 * FUNCTIONS:  clock
 *
 * This module contains IBM CONFIDENTIAL code. -- (IBM
 * Confidential Restricted when combined with the aggregated
 * modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989 
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
 * clock.c	1.12  com/lib/c/gen,3.1,8943 9/8/89 08:39:13
 */

/* name space pollution clean up */
#ifdef _NAME_SPACE_WEAK_STRONG
#include "pollution.h"
#endif
#include <sys/times.h>
#include <time.h>		/* for CLK_TCK (clock ticks per second) */

#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern struct rec_mutex	_clock_rmutex;
#endif

/*
 * TIMES computes full amount of cpu time used including user, sys, child user,
 * and child sys times...
 */
#define TIMES(B)	(B.tms_utime+B.tms_stime+B.tms_cutime+B.tms_cstime)

static clock_t first = (clock_t) -1; /* cpu time used after first call	*/

/*
 * NAME:	clock
 *
 * FUNCTION:	clock - return CPU time used
 *
 * NOTES:	Clock returns the amount of CPU time used (in
 *		microseconds) since the first call to clock.
 *
 * RETURN VALUE DESCRIPTION:	mount of CPU time used (in
 *		microseconds) since the first call to clock
 */

clock_t
clock(void)
{
	struct tms buffer;
#ifdef _THREAD_SAFE
	clock_t	return_val;

	rec_mutex_lock(&_clock_rmutex);
#endif

	/* set first if first time in */
	if((times(&buffer) != (clock_t)(-1)) && (first == (clock_t)(-1)))
		first = TIMES(buffer);

	/*
	 * compute difference, convert to microseconds.
	 * struct tms elements are in CLK_TCK's...
	 */
#ifdef _THREAD_SAFE
	return_val = (TIMES(buffer) - first) * (clock_t)(1000000L/CLK_TCK);
	rec_mutex_unlock(&_clock_rmutex);
	return(return_val);
#else
	return((TIMES(buffer) - first) * (clock_t)(1000000L/CLK_TCK));
#endif
}
