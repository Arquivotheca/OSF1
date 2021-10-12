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
 * @(#)$RCSfile: swap_wd_bytes.s,v $ $Revision: 4.1.3.2 $ (DEC) $Date: 1992/01/28 17:29:55 $
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

 /*
  * Modification History:
  *
  *     
  * 	11-Oct-1991	Stuart Hollander
  *		Converted header comments and include paths for OSF-based version
  *     
  * 	12-May-1990	Paul Grist
  *		Created this file, and the sw byte swap routine for
  *          	user level VME support.  
  */

#include <mips/asm.h>
#include <mips/regdef.h>


/***************************************************************************
 *
 *   swap__word_bytes(buffer);
 *   unsigned long buffer;
 *
 *   RETURNS: resulting byte swapped value
 *
 ***************************************************************************
 *
 *   swap word bytes: will operate on one unsigned 32 bit quantity
 *
 *
 *  			31       |       0
 *			 +---+---+---+---+
 *	start with:	 | a | b | c | d |
 *			 +---+---+---+---+
 *                               |
 *
 *
 *			31       |       0
 *	end with:        +---+---+---+---+
 *			 | b | a | d | c |
 *			 +---+---+---+---+
 *                               |
 *
 ***************************************************************************/


LEAF(swap_word_bytes)
#ifdef MIPSEL
	# a0 has the input, unsigned 32 bit quanity - abcd
	sll	v0,a0,8		# shift left 8      - bcd0  in v0
	and	v0,0xff00ff00   # mask out b,d      - b0d0  in v0
	srl	v1,a0,8		# shift right 8     - 0abc  in v1
	and	v1,0x00ff00ff   # mask out a,c	    - 0a0c  in v1
	or	v0,v1		# v0 <- v0 | v1	    - badc  in v0
#else
	move	v0,a0		# move v0 into ret value
#endif
	j	ra		# return
.end swap_word_bytes

