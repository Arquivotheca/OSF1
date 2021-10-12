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
 * @(#)$RCSfile: badaddr.s,v $ $Revision: 1.1.3.3 $ (DEC) $Date: 1992/10/13 12:06:34 $
 */

#include  <machine/regdef.h>
#include  <machine/asm.h>
#include  <machine/cpu.h>
#include  <hal/cpuconf.h>
#include  "assym.s"

/*
 * bbadaddr(addr, len, ptr)
 *	check for bus error on read access to addr
 *	len is length of access (1=byte, 2=short, 4=long)
 *	ptr is a pointer to a bus or controller struct and 
 *	    is used by machines that access io space via mailboxes
 *	    currently Alpha AXP systems only
 */
BADADDRFRM=	(4*4)+4		# 4 arg saves plus a ra
NESTED(bbadaddr, BADADDRFRM, zero)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
	subu	sp,BADADDRFRM
	sw	ra,BADADDRFRM-4(sp)
        li      a2,PCB_WIRED_ADDRESS
#ifdef ASSERTIONS
	lw	v0,PCB_NOFAULT(a2)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif
	.set	noreorder
	li	v0,NF_BADADDR
	sw	v0,PCB_NOFAULT(a2)

	bne	a1,1,1f
	nop
	lb	v0,0(a0)
	b	4f
	nop

1:	bne	a1,2,2f
	nop
	lh	v0,0(a0)
	b	4f
	nop

2:	bne	a1,4,3f
	nop
	lw	v0,0(a0)
	b	4f
	nop

	.set	reorder
3:	PANIC("bbaddaddr")
	.set	noreorder

4:	sw	zero,PCB_NOFAULT(a2)
	nop
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	mtc0	t0,C0_SR		# PE BIT
	move	v0,zero
	j	ra
	nop
	.set	reorder
	END(bbadaddr)

/*
 * wbadaddr(addr, len, ptr)
 *	check for bus error on write access to addr
 *	len is length of access (1=byte, 2=short, 4=long)
 *	ptr is a pointer to a bus or controller struct and 
 *	    is used by machines that access io space via mailboxes
 *	    currently Alpha AXP systems only
 */
NESTED(wbadaddr, BADADDRFRM, zero)
	subu	sp,BADADDRFRM
	sw	ra,BADADDRFRM-4(sp)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
#ifndef SABLE
	lw	zero,SBE_ADDR|K1BASE
#endif
        li      a2,PCB_WIRED_ADDRESS
#ifdef ASSERTIONS
	lw	v0,PCB_NOFAULT(a2)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif
	.set	noreorder
	li	v0,NF_BADADDR
	sw	v0,PCB_NOFAULT(a2)

	bne	a1,1,1f
	nop
	sb	zero,0(a0)
	b	4f
	nop

1:	bne	a1,2,2f
	nop
	b	4f
	sh	zero,0(a0)

2:	bne	a1,4,3f
	nop
	b	4f
	sw	zero,0(a0)

	.set	reorder
3:	PANIC("wbaddaddr")
	.set	noreorder

4:	bc0f	4b			# wait for write buffer empty
	nop
	mfc0	t1,C0_CAUSE
	lw	v0,K1BASE
	and	t1,CAUSE_IP7		# Memory line error
	bne	t1,zero,baerror
	sw	zero,PCB_NOFAULT(a2)	# clear nofault
	mtc0	t0,C0_SR		# PE BIT
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	j	ra
	move	v0,zero
	.set	reorder
	END(wbadaddr)

/*
 * wbadmemaddr(addr)
 *	check for address error on word access to addr
 *	Assumes addr points to RAM since trap is generated by read-back
 */
 NESTED(wbadmemaddr, BADADDRFRM, zero)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
 	subu	sp,BADADDRFRM
 	sw	ra,BADADDRFRM-4(sp)
        li      a1,PCB_WIRED_ADDRESS
#ifdef ASSERTIONS
	lw	v0,PCB_NOFAULT(a1)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif
 	.set	noreorder
 	li	v0,NF_BADADDR		# LDSLOT
 	sw	v0,PCB_NOFAULT(a1)
 	sw	zero,0(a0)		# store first to generate ECC
 	lw	v0,0(a0)		# load can cause sync DBE
 	sw	zero,PCB_NOFAULT(a1)
 	lw	ra,BADADDRFRM-4(sp)
 	addu	sp,BADADDRFRM
	mtc0	t0,C0_SR		# PE BIT
 	j	ra
 	move	v0,zero
 	.set	reorder
	END(wbadmemaddr)

/*
 * trap() nofault code comes here on bus errors when nofault == NF_BADADDR
 */
NESTED(baerror, BADADDRFRM, zero)
#ifndef SABLE
	lw	v0, cpu			# get cpu
	bne	v0, DS_3100, 1f		# only PMAX has SBE
	lw	zero,SBE_ADDR|K1BASE
1:
#endif
	.set noreorder
	nop
	mtc0	t0,C0_SR		# PE BIT
	nop
	.set reorder
	li	v0,1
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	j	ra
	END(baerror)

