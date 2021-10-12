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
	.rdata
	.asciiz "@(#)$RCSfile: pal_lib.s,v $ $Revision: 1.2.17.2 $ (DEC) $Date: 1993/09/27 14:18:37 $"
	.text

/*
 * Modification History: /sys/machine/alpha/pal_lib.s
 *
 * 13-Sep-90 -- rjl
 *	Created this file for Alpha support.
 *
 *	This file contains C callable assembly jacket routines for all
 *	of the pal code routines and machine language instructions not
 *	generated directly from C.
 */

#include <machine/pal.h>
#include <machine/regdef.h>
#include <machine/asm.h>


/*
 * write internal processor register, C callable jacket
 */
IPRFRAME=	(3*8)+8		/* 3 registers and ra	*/

#define	rduniq		NOT_USED
#define	wruniq		NOT_USED
#define mfpr_sysval	NOT_USED
#define mtpr_sysval	NOT_USED
#define mtpr_kgp	NO_C_CALL
#define mtpr_vptptr	NO_C_CALL

/**********************************************************************
 *                                                                    *
 *  Routines and instructions common to both versions of the palcode  *
 *                                                                    *
 **********************************************************************/

/*
 * The following are special instructions (non-pal_code) not normally
 * generated by the C compiler.  This should only be used from C or
 * other highlevel code.
 */
LEAF(trapb)
	trapb
	ret	zero,(ra)
END(trapb)
	
LEAF(mb)
XLEAF(wbflush)
	mb
	ret	zero,(ra)
	.set reorder
END(mb)
	
LEAF(kdebug_mb)
	mb
	ret	zero,(ra)
	.set reorder
END(kdebug_mb)

LEAF(imb)
	call_pal PAL_imb
	ret	zero,(ra)
END(imb)

LEAF(kdebug_imb)
	call_pal PAL_imb
	ret	zero,(ra)
END(kdebug_imb)

#ifndef	rduniq
LEAF(rduniq)
	call_pal PAL_rduniq
	ret	zero,(ra)
END(rduniq)
#endif	/* rduniq */

#ifndef	wruniq
LEAF(wruniq)
	call_pal PAL_wruniq
	ret	zero,(ra)
END(wruniq)
#endif	/* wruniq */

LEAF(draina)
	call_pal PAL_draina
	ret	zero,(ra)
END(draina)

LEAF(stqc)
	.set noreorder
	mb
	stq_c	a1,(a0)
	bis	a1,zero,v0
	ret	zero,(ra)
	.set reorder
END(stqc)

/*
 *	cobratt - control cobra uart through pal calls.
 *	for details see cobra_cons.c
 */
LEAF(cobra_tt)
	call_pal PAL_cobratt
	ret zero,(ra)
END(init_cobra_cons)

LEAF(kdebug_cobra_tt)
	call_pal PAL_cobratt
	ret zero,(ra)
END(kdebug_cobra_tt)

/*
 * Read the system cycle counter
 */
LEAF(scc)
	rpcc	v0
	ret	zero,(ra)
END(scc)

/*
 * Ensure a page is flushed from all caches to system memory.
 * For memory mapped presto RAM
 */
LEAF(cflush)
	call_pal PAL_cflush
	ret	zero,(ra)
END(cflush)

/**********************************************************************
 *                                                                    *
 *  Routines and instructions for the OSF style of palcode            *
 *                                                                    *
 **********************************************************************/
/*
 * Read system value register
 */
#ifndef	mfpr_sysval
LEAF(mfpr_sysval)
	call_pal PAL_rdval
	ret	zero,(ra)
END(mfpr_sysval)
#endif	/* mfpr_sysval */

/*
 * Write system value register
 */
#ifndef	mtpr_sysval
LEAF(mtpr_sysval)
	call_pal PAL_wrval
	ret	zero,(ra)
END(mtpr_sysval)
#endif	/* mtpr_sysval */

/*
 * Read user stack pointer
 */
LEAF(mfpr_usp)
	call_pal PAL_rdusp
	ret	zero,(ra)
END(mfpr_usp)

/*
 * Write user stack pointer
 */
LEAF(mtpr_usp)
	call_pal PAL_wrusp
	ret	zero,(ra)
END(mtpr_usp)

/*
 * Write kernel global pointer
 */
#ifndef	mtpr_kgp
LEAF(mtpr_kgp)
	call_pal PAL_wrkgp
	ret	zero,(ra)
END(mtpr_kgp)
#endif	/* mtpr_kgp */

/*
 * Write the virtual page table base pointer
 */
#ifndef	mtpr_vptptr
LEAF(mtpr_vptptr)
	call_pal PAL_wrvptptr
	ret	zero,(ra)
END(mtpr_vptptr)
#endif	/* mtpr_vptptr */

/*
 * Read the who am i register
 */
LEAF(mfpr_whami)
	call_pal PAL_whami
	ret	zero,(ra)
END(mfpr_whami)

/*
 * Write the floating point enable
 */
LEAF(mtpr_fen)
	call_pal PAL_wrfen
	ret	zero,(ra)
END(mtpr_fen)

/*
 * Write the MCES register
 */
LEAF(mtpr_mces)
	call_pal PAL_mtpr_mces
	ret	zero,(ra)
END(mtpr_mces)


/*
 * mtpr_tbi: use the macros in ./pmap.h
 */
LEAF(mtpr_tbi)
	call_pal PAL_tbi
	ret	zero,(ra)
END(mtpr_tbi)

/*
 * enable the performance counters
 */
LEAF(wrperfmon)
	call_pal PAL_wrperfmon
	ret	zero,(ra)
END(wrperfmon)

/*
 * Read the process cycle counter
 */
LEAF(rpcc)
	rpcc	v0
	ret	zero,(ra)
END(rcc)
