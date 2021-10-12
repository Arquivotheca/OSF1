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
 *	@(#)$RCSfile: type.h,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 04:23:00 $
 */ 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * OSF/1 Release 1.0
 */

#ifndef	_TYPE_H_
#define	_TYPE_H_

#define	EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <sys/types.h>
#include "string.h"

typedef u_int ipc_flags_t;

#define	flNone		(0)
#define	flLong		(01)	/* IsLong specified */
#define	flNotLong	(02)	/* NotIsLong specified */
#define	flDealloc	(04)	/* Dealloc specified */
#define	flNotDealloc	(010)	/* NotDealloc specified */

/*
 * itName and itNext are internal fields (not used for code generation).
 * They are only meaningful for types entered into the symbol table.
 * The symbol table is a simple self-organizing linked list.
 *
 * The function itCheckDecl checks & fills in computed information.
 * Every type actually used (pointed at by argType) is so processed.
 *
 * The itInName, itOutName, itSize, itNumber, itInLine, itLongForm,
 * and itDeallocate fields correspond directly to msg_type_t fields.
 * For out-of-line variable sized types, itNumber is zero.  For
 * in-line variable sized types, itNumber is the maximum size of the
 * array.  itInName is the msg_type_name value supplied to the kernel,
 * and itOutName is the msg_type_name value received from the kernel.
 * Either or both may be MSG_TYPE_POLYMORPHIC, indicating a
 * "polymorphic" msg_type_name.  For itInName, this means the user
 * supplies the value with an argument.  For itOutName, this means the
 * the value is returned in an argument.
 *
 * The itInNameStr and itOutNameStr fields contain "printing" versions
 * of the itInName and itOutName values.  The mapping from number->string
 * is not into (eg, MSG_TYPE_UNSTRUCTURED/MSG_TYPE_BOOLEAN/MSG_TYPE_BIT).
 * These fields are used for code-generation and pretty-printing.  Mostly
 * they just help make the generated code more readable, but in the
 * case of MSG_TYPE_INTERNAL_MEMORY they are crucial: the correct symbolic
 * form must be generated for this value, because the symbolic definition
 * is in fact conditional.
 *
 * itFlags contains the user's requests for itLongForm and itDeallocate
 * values.  itCheckDecl takes it into account when setting itLongForm
 * and itDeallocate, but they can be overridden (with a warning message).
 *
 * itTypeSize is the calculated size of the C type, in bytes.
 * itPadSize is the size of any padded needed after the data field.
 * itMinTypeSize is the minimum size fo the data field, including padding.
 * For variable-length inline data, it is zero.
 *
 * itUserType, itServerType, itTransType are the C types used in
 * code generation.  itUserType is the C type passed to the user-side stub
 * and used for msg declarations in the user-side stub.  itServerType
 * is the C type used for msg declarations in the server-side stub.
 * itTransType is the C type passed to the server function by the
 * server-side stub.  Normally it differs from itServerType only when
 * translation functions are defined.
 *
 * itInTrans and itOutTrans are translation functions.  itInTrans
 * takes itServerType values and returns itTransType values.  itOutTrans
 * takes itTransType vaulues and returns itServerType values.
 * itDestructor is a finalization function applied to In arguments
 * after the server-side stub calls the server function.  It takes
 * itTransType values.  Any combination of these may be defined.
 *
 * The following type specification syntax modifies these values:
 *	type new = old
 *		ctype: name		// itUserType and itServerType
 *		cusertype: itUserType
 *		cservertype: itServerType
 *		intran: itTransType itInTrans(itServerType)
 *		outtran: itServerType itOutTrans(itTransType)
 *		destructor: itDestructor(itTransType);
 *
 * At most one of itStruct and itString should be TRUE.  If both are
 * false, then this is assumed to be an array type (msg data is passed
 * by reference).  If itStruct is TRUE, then msg data is passed by value
 * and can be assigned with =.  If itString is TRUE, then the msg_data
 * is a null-terminated string, assigned with strncpy.  The itNumber
 * value is a maximum length for the string; the msg field always
 * takes up this much space.
 *
 * itVarArray means this is a variable-sized array.  If it is inline,
 * then itStruct and itString are FALSE.  If it is out-of-line, then
 * itStruct is TRUE (because pointers can be assigned).
 *
 * itElement points to any substructure that the type may have.
 * It is only used with variable-sized array types.
 */

typedef struct ipc_type
{
    identifier_t itName;	/* Mig's name for this type */
    struct ipc_type *itNext;	/* next type in symbol table */

    u_int itTypeSize;		/* size of the C type */
    u_int itPadSize;		/* amount of padding after data */
    u_int itMinTypeSize;	/* minimal amount of space occupied by data */

    u_int itInName;		/* name supplied to kernel in sent msg */
    u_int itOutName;		/* name in received msg */
    u_int itSize;
    u_int itNumber;
    boolean_t itInLine;
    boolean_t itLongForm;
    boolean_t itDeallocate;

    string_t itInNameStr;	/* string form of itInName */
    string_t itOutNameStr;	/* string form of itOutName */

    /* what the user wants, not necessarily what he gets */
    ipc_flags_t itFlags;

    boolean_t itStruct;
    boolean_t itString;
    boolean_t itVarArray;

    struct ipc_type *itElement;	/* may be NULL */

    identifier_t itUserType;
    identifier_t itServerType;
    identifier_t itTransType;

    identifier_t itInTrans;	/* may be NULL */
    identifier_t itOutTrans;	/* may be NULL */
    identifier_t itDestructor;	/* may be NULL */
} ipc_type_t;

#define	itNULL		((ipc_type_t *) 0)

extern ipc_type_t *itLookUp(/* identifier_t name */);
extern void itInsert(/* identifier_t name, ipc_type_t *it */);
extern void itTypeDecl(/* identifier_t name, ipc_type_t *it */);

extern ipc_type_t *itShortDecl(/* u_int inname, string_t instr,
				  u_int outname, string_t outstr,
				  u_int default */);
extern ipc_type_t *itLongDecl(/* u_int inname, string_t instr,
				 u_int outname, string_t outstr,
				 u_int default,
				 u_int size, ipc_flags_t flags */);
extern ipc_type_t *itPrevDecl(/* identifier_t name */);
extern ipc_type_t *itResetType(/* ipc_type_t *it */);
extern ipc_type_t *itVarArrayDecl(/* u_int number, ipc_type_t *it */);
extern ipc_type_t *itArrayDecl(/* u_int number, ipc_type_t *it */);
extern ipc_type_t *itPtrDecl(/* ipc_type_t *it */);
extern ipc_type_t *itStructDecl(/* u_int number, ipc_type_t *it */);

extern ipc_type_t *itRetCodeType;
extern ipc_type_t *itDummyType;
extern ipc_type_t *itTidType;
extern ipc_type_t *itPortType;
extern ipc_type_t *itWaitTimeType;
extern ipc_type_t *itMsgTypeType;
extern ipc_type_t *itMakeCountType();
extern ipc_type_t *itMakePolyType();

extern void init_type();

extern void itCheckReturnType(/* identifier_t name, ipc_type_t *it */);
extern void itCheckPortType(/* identifier_t name, ipc_type_t *it */);
extern void itCheckIntType(/* identifier_t name, ipc_type_t *it */);

#endif	/* _TYPE_H_ */
