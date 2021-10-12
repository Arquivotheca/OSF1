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
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991 MIPS Computer Systems, Inc.            |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 52.227-7013.   |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Drive                                |
 * |         Sunnyvale, CA 94086                               |
 * |-----------------------------------------------------------|
 */
/* $Header: /usr/sde/osf1/rcs/os/src/usr/include/alpha/symconst.h,v 1.2.4.3 1993/08/30 17:28:44 Jon_Reeves Exp $ */

/* (C) Copyright 1984 by Third Eye Software, Inc.
 *
 * Third Eye Software, Inc. grants reproduction and use rights to
 * all parties, PROVIDED that this comment is maintained in the copy.
 *
 * Third Eye makes no claims about the applicability of this
 * symbol table to a particular use.
 */

#ifndef _SYMCONST_H
#define _SYMCONST_H

/* glevels for field in FDR */
#define GLEVEL_0	2
#define GLEVEL_1	1
#define GLEVEL_2	0	/* for upward compat reasons. */
#define GLEVEL_3	3

/* magic number fo symheader */
#if defined(__alpha) || defined(__mips64)
#define magicSym	0x1992
#define	magicSym_cia	0x1957	/* uld need to preserve symbols inside cia's */
#else
#define magicSym	0x07009
#define	magicSym_cia	0x07109	/* uld need to preserve symbols inside cia's */
#endif

/* Language codes */
#define langC		0	
#define langPascal	1
#define langFortran	2
#define	langAssembler	3	/* one Assembley inst might map to many mach */
#define langMachine	4
#define langNil		5
#define langAda		6
#define langPl1		7
#define langCobol	8
#define langStdc	9
#define langMIPSCxx     10	/* MIPS version of CXX symtab */
#define langDECCxx      11	/* DEC version of CXX symtab */
#define langCxx         12	/* Joint plan for CXX symtab */
#define langFortran90	13
#define langBliss	14
#define langMax         32      /* maximum allowed 32 -- 5 bits */

/* The following are value definitions for the fields in the SYMR */

/*
 * Storage Classes
 */

#define scNil		0
#define scText		1	/* text symbol */
#define scData		2	/* initialized data symbol */
#define scBss		3	/* un-initialized data symbol */
#define scRegister	4	/* value of symbol is register number */
#define scAbs		5	/* value of symbol is absolute */
#define scUndefined	6	/* who knows? */
#define scCdbLocal	7	/* variable's value is IN se->va.?? */
#define scBits		8	/* this is a bit field */
#define scCdbSystem	9	/* variable's value is IN CDB's address space */
#define scDbx		9	/* overlap dbx internal use */
#define scRegImage	10	/* register value saved on stack */
#define scInfo		11	/* symbol contains debugger information */
#define scUserStruct	12	/* address in struct user for current process */
#define scSData		13	/* load time only small data */
#define scSBss		14	/* load time only small common */
#define scRData		15	/* load time only read only data */
#define scVar		16	/* Var parameter (fortran,pascal) */
#define scCommon	17	/* common variable */
#define scSCommon	18	/* small common */
#define scVarRegister	19	/* Var parameter in a register */
#define scVariant	20	/* Variant record */
#define scFileDesc	20	/* COBOL: File Descriptor (FD) */
#define scSUndefined	21	/* small undefined(external) data */
#define scInit		22	/* .init section symbol */
#define scBasedVar	23	/* Fortran or PL/1 ptr based var */ 
#define scReportDesc	23	/* COBOL: Report Descriptor (RD) */
#define scXData         24      /* exception handling data */
#define scPData         25      /* Procedure section */
#define scFini          26      /* .fini section */
#define scMax		32


/*
 *   Symbol Types
 */

#define stNil		0	/* Nuthin' special */
#define stGlobal	1	/* external symbol */
#define stStatic	2	/* static */
#define stParam		3	/* procedure argument */
#define stLocal		4	/* local variable */
#define stLabel		5	/* label */
#define stProc		6	/*     "      "	 Procedure */
#define stBlock		7	/* beginnning of block */
#define stEnd		8	/* end (of anything) */
#define stMember	9	/* member (of anything	- struct/union/enum */
#define stTypedef	10	/* type definition */
#define stFile		11	/* file name */
#define stRegReloc	12	/* register relocation */
#define stForward	13	/* forwarding address */
#define stStaticProc	14	/* load time only static procs */
#define stConstant	15	/* const */
#define stStaParam	16	/* Fortran static parameters */
#define stBase          17      /* DEC C++ base class */
#define stVirtBase      18      /* DEC C++ virtual base class */
#define stTag           19      /* DEC C++ tag */
#define stInter         20      /* DEC C++ interlude */

    /* Psuedo-symbols - internal to debugger */
#define stStr		60	/* string */
#define stNumber	61	/* pure number (ie. 4 NOR 2+2) */
#define stExpr		62	/* 2+2 vs. 4 */
#define stType		63	/* post-coersion SER */
#define stMax		64

/* definitions for fields in TIR */

/* type qualifiers for ti.tq0 -> ti.(itqMax-1) */
#define tqNil	0	/* bt is what you see */
#define tqPtr	1	/* pointer */
#define tqProc	2	/* procedure */
#define tqArray 3	/* duh */
#define tqFar	4	/* longer addressing - 8086/8 land */
#define tqVol	5	/* volatile */
#define tqConst 6	/* const */
#define tqRef   7       /* reference */
#define tqMax	8

/* basic types as seen in ti.bt */
#define btNil		0	/* undefined */
#define btAdr32		1	/* 32-bit address */
#define btChar		2	/* character */
#define btUChar		3	/* unsigned character */
#define btShort		4	/* short */
#define btUShort	5	/* unsigned short */
#define btInt32		6	/* 32-bit int */
#define btInt		btInt32	/* int */
#define btUInt32	7	/* 32-bit unsigned int */
#define btUInt		btUInt32	/* unsigned int */
#define btLong32	8	/* 32-bit long */
#define btULong32	9	/* 32-bit unsigned long */
#define btFloat		10	/* float (real) */
#define btDouble	11	/* Double (real) */
#define btStruct	12	/* Structure (Record) */
#define btUnion		13	/* Union (variant) */
#define btEnum		14	/* Enumerated */
#define btTypedef	15	/* defined via a typedef, isymRef points */
#define btRange		16	/* subrange of int */
#define btSet		17	/* pascal sets */
#define btComplex	18	/* fortran complex */
#define btDComplex	19	/* fortran double complex */
#define btIndirect	20	/* forward or unnamed typedef */
#define btFixedDec	21	/* Fixed Decimal */
#define btFixedBin	21	/* COBOL: Fixed Binary */
#define btFloatDec	22	/* Float Decimal */
#define btDecimal	22	/* COBOL: packed/unpacked decimal */
#define btString	23	/* Varying Length Character String */
#define btBit		24	/* Aligned Bit String */
#define btPicture	25	/* Picture */
#define btVoid		26	/* void */
#define btPtrMem        27      /* DEC C++:  Pointer to member */
#define btScaledBin	27	/* COBOL: Scaled Binary */
#define btVptr          28      /* DEC C++:  Virtual function table */
#define btArrayDesc	28	/* FORTRAN 90: Array Descriptor */
#define btClass         29      /* DEC C++:  Class (Record) */
#define btLong64        30      /* 64-bit long */
#define btLong		btLong64
#define btULong64       31      /* 64-bit unsigned long */
#define btULong		btULong64
#define btLongLong64    32      /* 64-bit long long */
#define btLongLong      btLongLong64    /* long long */
#define btULongLong64   33      /* 64-bit unsigned long long */
#define btULongLong     btULongLong64   /* unsigned long long */
#define btAdr64         34      /* 64-bit address */
#define btAdr		btAdr64	/* address - same size as pointer */
#define btInt64		35	/* 64-bit int */
#define btUInt64	36	/* 64-bit unsigned int */
#define btLDouble	37	/* long double (real*16) */
#define btMax		64

#if (_MFG == _MIPS)
/* optimization type codes */
#define otNil		0
#define otReg		1	/* move var to reg */
#define otBlock		2	/* begin basic block */
#define	otProc		3	/* procedure */
#define otInline	4	/* inline procedure */
#define otEnd		5	/* whatever you started */
#define otMax		6	/* KEEP UP TO DATE */
#endif  /* (_MFG == _MIPS) */

/* Additional description for btDecimal */
#define decDecimal4		1	/* 4 bits/digit */
#define decDecimal8		2	/* 8 bits/digit */
#define signUnsigned		1	/* No sign present */
#define signLeadingOverpunch	2	/* Sign with leading digit */
#define signTrailingOverpunch	3	/* Sign with final digit */
#define signLeadingSeparate	4	/* Sign in front alone */
#define signTrailingSeparate	5	/* Sign at end alone */

#endif /* _SYMCONST_H */
