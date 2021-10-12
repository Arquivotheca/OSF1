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
 *	@(#)$RCSfile: setjmp.h,v $ $Revision: 1.2 $ (DEC) $Date: 1992/01/15 01:17:45 $
 */ 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
 */
/*
 * setjmp.h
 *
 *	Modification History:
 *
 * 28-Apr-91	Fred Canter
 *	Change LANGUAGE_* to __LANGUAGE_*__ for MIPS ANSI C.
 *
 */

/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * jmp_buf offsets
 * This should really just be a struct sigcontext, but for historical
 * reasons ....
 * NOTE: THIS MUST MATCH the initial portion of struct sigcontext,
 *	sc_onsigstk, sc_sigmask, sc_pc, sc_regs, sc_mdlo, sc_mdhi,
 *	fpregs, and fpc_csr
 * must lie at offset equal to the corresponding entries in the jmp_buf
 * since longjmp performs a sigcleanup.
 * See libc routines setjmp/longjmp/sigvec, and kernel routines
 * sendsig/sigcleanup.
 */
#define	JB_ONSIGSTK	0		/* onsigstack flag */
#define	JB_SIGMASK	1		/* signal mask */
#define	JB_PC		2		/* program counter */
#define	JB_REGS		3		/* registers */
#define	JB_ZERO		(JB_REGS+0)	/* register zero */
#define	JB_MAGIC	(JB_ZERO)	/* magic number saved at reg 0 */
#define	JB_AT		(JB_REGS+1)	/* AT */
#define	JB_V0		(JB_REGS+2)	/* function result regs */
#define	JB_V1		(JB_REGS+3)
#define	JB_A0		(JB_REGS+4)	/* argument regs */
#define	JB_A1		(JB_REGS+5)
#define	JB_A2		(JB_REGS+6)
#define	JB_A3		(JB_REGS+7)
#define	JB_T0		(JB_REGS+8)	/* caller saved regs */
#define	JB_T1		(JB_REGS+9)
#define	JB_T2		(JB_REGS+10)
#define	JB_T3		(JB_REGS+11)
#define	JB_T4		(JB_REGS+12)
#define	JB_T5		(JB_REGS+13)
#define	JB_T6		(JB_REGS+14)
#define	JB_T7		(JB_REGS+15)
#define	JB_S0		(JB_REGS+16)	/* callee saved regs */
#define	JB_S1		(JB_REGS+17)
#define	JB_S2		(JB_REGS+18)
#define	JB_S3		(JB_REGS+19)
#define	JB_S4		(JB_REGS+20)
#define	JB_S5		(JB_REGS+21)
#define	JB_S6		(JB_REGS+22)
#define	JB_S7		(JB_REGS+23)
#define	JB_T8		(JB_REGS+24)	/* temps */
#define	JB_T9		(JB_REGS+25)
#define	JB_K0		(JB_REGS+26)	/* kernel regs */
#define	JB_K1		(JB_REGS+27)
#define	JB_GP		(JB_REGS+28)	/* frame pointer */
#define	JB_SP		(JB_REGS+29)	/* stack pointer */
#define	JB_FP		(JB_REGS+30)	/* frame pointer */
#define	JB_RA		(JB_REGS+31)	/* return address */

#define	JB_FREGS	38		/* floating-point registers */
#define	JB_F0		(JB_FREGS+0)	/* function result regs */
#define	JB_F1		(JB_FREGS+1)
#define	JB_F2		(JB_FREGS+2)
#define	JB_F3		(JB_FREGS+3)
#define	JB_F4		(JB_FREGS+4)	/* caller save regs */
#define	JB_F5		(JB_FREGS+5)
#define	JB_F6		(JB_FREGS+6)
#define	JB_F7		(JB_FREGS+7)
#define	JB_F8		(JB_FREGS+8)
#define	JB_F9		(JB_FREGS+9)
#define	JB_F10		(JB_FREGS+10)
#define	JB_F11		(JB_FREGS+11)
#define	JB_F12		(JB_FREGS+12)	/* argument regs */
#define	JB_F13		(JB_FREGS+13)
#define	JB_F14		(JB_FREGS+14)
#define	JB_F15		(JB_FREGS+15)
#define	JB_F16		(JB_FREGS+16)	/* caller save regs */
#define	JB_F17		(JB_FREGS+17)
#define	JB_F18		(JB_FREGS+18)
#define	JB_F19		(JB_FREGS+19)
#define	JB_F20		(JB_FREGS+20)	/* callee save regs */
#define	JB_F21		(JB_FREGS+21)
#define	JB_F22		(JB_FREGS+22)
#define	JB_F23		(JB_FREGS+23)
#define	JB_F24		(JB_FREGS+24)
#define	JB_F25		(JB_FREGS+25)
#define	JB_F26		(JB_FREGS+26)
#define	JB_F27		(JB_FREGS+27)
#define	JB_F28		(JB_FREGS+28)
#define	JB_F29		(JB_FREGS+29)
#define	JB_F30		(JB_FREGS+30)
#define	JB_F31		(JB_FREGS+31)
#define JB_FPC_CSR	(JB_FREGS+32)	/* fp control and status register */

/*
 * WARNING: a jmp_buf must be as large as a sigcontext since
 * longjmp uses one to perform a sigreturn
 */
#define	SIGCONTEXT_PAD	48
#define	NJBREGS		(JB_RA+1+SIGCONTEXT_PAD)

/*
 * These are not part of a jmpbuf, but are part of the sigcontext
 * and are referenced from the signal trampoline code in sigvec.s
 */
#define	SC_MDLO		(JB_REGS+32)
#define	SC_MDHI		(JB_REGS+33)

#define	JBMAGIC		0xacedbade

#ifdef __LANGUAGE_C__
#ifndef LOCORE
typedef	int	jmp_buf[NJBREGS];
#endif /* !LOCORE */
#endif /* __LANGUAGE_C__ */
