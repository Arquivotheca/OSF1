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
 *	@(#)$RCSfile: fpu.h,v $ $Revision: 1.2 $ (DEC) $Date: 1992/01/15 01:08:49 $
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
 * fpu.h
 *
 *	Modification History:
 *
 * 28-Apr-91	Fred Canter
 *	Change LANGUAGE_* to __LANGUAGE_*__ for MIPS ANSI C.
 *
 */
/* 
 * derived from fpu.h	2.1	(ULTRIX/OSF)	12/3/90
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * fpu.h -- fpu specific defines
 */


#ifdef __LANGUAGE_C__
/*
 * Structure and constant definisions for floating-point instructions.
 */
union fpu_instr {
	unsigned long instr;

	struct {
#if	BYTE_MSF
		unsigned op : 6;
		unsigned base : 5;
		unsigned rt : 5;
		signed immed : 16;
#else	/* BYTE_MSF */
		signed immed : 16;
		unsigned rt : 5;
		unsigned base : 5;
		unsigned op : 6;
#endif	/* BYTE_MSF */
	} itype;

	struct {
#if	BYTE_MSF
		unsigned op : 6;
		unsigned sub : 5;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned : 11;
#else	/* BYTE_MSF */
		unsigned : 11;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned sub : 5;
		unsigned op : 6;
#endif	/* BYTE_MSF */
	} mtype;

	struct {
#if	BYTE_MSF
		unsigned op : 6;
		unsigned one : 1;
		unsigned fmt : 4;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned rd : 5;
		unsigned func : 6;
#else	/* BYTE_MSF */
		unsigned func : 6;
		unsigned rd : 5;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned fmt : 4;
		unsigned one : 1;
		unsigned op : 6;
#endif	/* BYTE_MSF */
	} rtype;
};
#endif /* __LANGUAGE_C__ */

#define FMT_SINGLE	0
#define FMT_DOUBLE	1
#define FMT_EXTENDED	2
#define FMT_QUAD	3
#define FMT_WORD	4
#define FMT_MAX		4

#define FUNC_ADD	0
#define FUNC_SUB	1
#define FUNC_MUL	2
#define FUNC_DIV	3

#define	FUNC_SQRT	4
#define	FUNC_ABS	5
#define	FUNC_MOV	6
#define FUNC_NEG	7

#define FUNC_CVTS	0x20
#define FUNC_CVTD	0x21
#define FUNC_CVTE	0x22
#define FUNC_CVTQ	0x23
#define FUNC_CVTW	0x24

/*
 * The func field of floating-point compare opcodes are FUNC_FC with
 * some combination of conditions (COND_*).
 */
#define FUNC_FC		0x30
#define COND_UN_MASK	0x1
#define COND_EQ_MASK	0x2
#define COND_LT_MASK	0x4
#define COND_IN_MASK	0x8

/*
 * The _MASK's are used to get a the specified field after it has been
 * shifted by _SHIFT and then bit patterns above can be used to test
 * the field.  These are useful in assembly code.
 */
#define	FPU_BASE_SHIFT	21
#define	FPU_BASE_MASK	0x1f
#define	FPU_FMT_SHIFT	21
#define	FPU_FMT_MASK	0xf
#define	FPU_RT_SHIFT	16
#define	FPU_RT_MASK	0x1f
#define	FPU_RT_FPRMASK	0x1e
#define	FPU_RS_SHIFT	11
#define	FPU_RS_MASK	0x1f
#define	FPU_RS_FPRMASK	0x1e
#define	FPU_RD_SHIFT	6
#define	FPU_RD_MASK	0x1f
#define	FPU_RD_FPRMASK	0x1e

#ifdef __LANGUAGE_C__
/*
 * These functions are used to get and set the floating-point control
 * registers.  They are defined in the library module fp_control.s
 */
extern unsigned long get_fpc_csr();
extern unsigned long set_fpc_csr();
extern unsigned long get_fpc_irr();
extern void set_fpc_led();
extern unsigned long get_fpc_eir();

/*
 * Structure and constant definisions for the floating-point control
 * implementation and revision register (fpc_irr).
 */
union fpc_irr {
	unsigned long fi_word;
	struct {
#if	BYTE_MSF
		unsigned reserved : 16;
		unsigned implementation : 8;
		unsigned revision : 8;
#else	/* BYTE_MSF */
		unsigned revision : 8;
		unsigned implementation : 8;
		unsigned reserved : 16;
#endif	/* BYTE_MSF */
	} fi_struct;
};
#endif /* __LANGUAGE_C__ */

/*
 * Constants for the implementation fieid of the fpc_irr structure for the
 * known implementations.
 */
#define IMPLEMENTATION_NONE	0	/* software */
#define IMPLEMENTATION_R2360	1	/* board */
#define IMPLEMENTATION_R2010	2	/* chip */
#define IMPLEMENTATION_R2010a	3	/* chip */

/*
 * The constant IRR_IMP_MASK is to get at the implementation fieid of the
 * implementation and revision register.  The other constants (IRR_IMP_*)
 * are constants for the known implementations.
 */
#define IRR_IMP_MASK		0x0000ff00

#define IRR_IMP_NONE		0x00000000
#define IRR_IMP_R2360		0x00000100
#define IRR_IMP_R2010		0x00000200
#define IRR_IMP_R2010a		0x00000300

/*
 * Structure and constant definitions for the floating-point control
 * control and status register (fpc_csr).
 */
#ifdef __LANGUAGE_C__
union fpc_csr {
	unsigned long fc_word;
	struct {
#if	BYTE_MSF
		unsigned reserved0 : 8;
		unsigned condition : 1;
		unsigned reserved1 : 5;

		unsigned ex_unimplemented : 1;
		unsigned ex_invalid : 1;
		unsigned ex_divide0 : 1;
		unsigned ex_overflow : 1;
		unsigned ex_underflow : 1;
		unsigned ex_inexact : 1;

		unsigned en_invalid : 1;
		unsigned en_divide0 : 1;
		unsigned en_overflow : 1;
		unsigned en_underflow : 1;
		unsigned en_inexact : 1;

		unsigned se_invalid : 1;
		unsigned se_divide0 : 1;
		unsigned se_overflow : 1;
		unsigned se_underflow : 1;
		unsigned se_inexact : 1;

		unsigned rounding_mode : 2;
#else	/* BYTE_MSF */
		unsigned rounding_mode : 2;

		unsigned se_inexact : 1;
		unsigned se_underflow : 1;
		unsigned se_overflow : 1;
		unsigned se_divide0 : 1;
		unsigned se_invalid : 1;

		unsigned en_inexact : 1;
		unsigned en_underflow : 1;
		unsigned en_overflow : 1;
		unsigned en_divide0 : 1;
		unsigned en_invalid : 1;

		unsigned ex_inexact : 1;
		unsigned ex_underflow : 1;
		unsigned ex_overflow : 1;
		unsigned ex_divide0 : 1;
		unsigned ex_invalid : 1;
		unsigned ex_unimplemented : 1;

		unsigned reserved1 : 5;
		unsigned condition : 1;
		unsigned reserved0 : 8;
#endif	/* BYTE_MSF */
	} fc_struct;
};
#endif /* __LANGUAGE_C__ */

/*
 * Constants for the rounding_mode field of the fpc_csr
 */
#define ROUND_TO_NEAREST	0
#define ROUND_TO_ZERO		1
#define ROUND_TO_PLUS_INFINITY	2
#define ROUND_TO_MINUS_INFINITY	3

/* Masks to get at the unimplemented exception */
#define	FPCSR_UNIMP		0x00020000
#define	FPCSR_EXCEPTIONS	0x0003f000	/* all exceptions */
#define	FPCSR_ENABLES		0x00000f80

#ifdef __LANGUAGE_ASSEMBLY__
/*
 * Assembly register names for the floating-point control registers.
 */
#define fpc_irr	$0
#define fpc_led	$0
#define fpc_eir	$30
#define fpc_csr	$31
#endif /* __LANGUAGE_ASSEMBLY__ */
