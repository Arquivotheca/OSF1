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
static char	*sccsid = "@(#)$RCSfile: jctype_.c,v $ $Revision: 4.2.5.2 $ (DEC) $Date: 1993/06/07 23:19:51 $";
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
 * COMPONENT_NAME: (LIBCGEN/KJI) Standard C Library Conversion Functions
 *
 * FUNCTIONS: none.
 *
 * ORIGINS: 10
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */                                                                   
/* jctype_.c	1.1  com/lib/c/gen/KJI,3.1,9021 10/19/89 17:25:00 */

/*	jctype_ - character classification tables for KJI
 *	Two-level indexing (based on first, second bytes of character)
 *	is used to reduce data requirement.  This scheme relies on the
 *	fact that for most characters, the upper byte determines the
 *	characteristics.
 *						rcd  12-Apr-88
 *      These structures are isolated in include files so that they
 *	can be used by kernel extensions.  Bill Kennedy 19-Oct-89 
 */
/* name space pollution clean up */
#ifdef _NAME_SPACE_WEAK_STRONG
#include "pollution.h"
#endif
#ifdef KJI
#include <NLctype.h>
#include <sys/jctype0.h>
#include <sys/jctype1.h>
#endif KJI
