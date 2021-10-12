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
 *	@(#)$RCSfile: emulate_instr.s,v $ $Revision: 1.2.3.3 $ (DEC) $Date: 1992/03/18 15:03:20 $
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
 * derived from emulate_instr.s	2.7      (ULTRIX)        12/7/89
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/************************************************************************
 *
 *	Modification History: emulate_instr.s
 *
 *   8-Apr-91 -- Ron Widyono
 *	Do not allow inclusion of sys/preempt.h from machine/pmap.h.
 *
 *   11-Nov-90 -- dutile
 *	fixed bneq v0,zero,1f typo to bne v0,zero,1f (bneq nonexist. instr.)
 *
 */

/************************************************************************
 *
 *	Modification History: locore.s
 *
 *   5-Dec-89 -- jmartin
 *	Fix flush_ea to flush the entire EMULATE_AREA icache.
 *
 *   4-Dec-89 -- jmartin
 *	Fix function flush_ea to use new PTE format.
 *
 */
/*
 * emulate_instr.s -- general instruction emulation/execution
 */

#include <rt_preempt.h>

#if	RT_PREEMPT
#define _SKIP_PREEMPT_H_
#endif

#include <machine/machparam.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/regdef.h>
#include <machine/asm.h>
#include <machine/pmap.h>
#include <machine/softfp.h>
#include <machine/vmparam.h>
#include <machine/thread.h>
#include <sys/signal.h>
#include <hal/cpuconf.h>
#include <assym.s>
#include <cpus.h>

IMPORT(exception_exit,0)
IMPORT(cpu,4)
IMPORT(kn5800_wbflush_addr,4)

/*
 * The emulate_instr() is a routine with a locore.s interface which emulates
 * or executes an instruction and changes the state of the user's registers
 * where they reside at the locore.s interface level.
 *
 * This routine is called via jal emulate_instr and eventually gets back to
 * the return address in ra.
 *
 * input parameters:
 *	a0 -- the current exception frame
 *	a1 -- the instruction to emulate
 *	a2 -- the epc for the instruction to emulate (also in the pcb)
 *	ra -- the return address
 * input parameters in the process's pcb (valid on return)
 *	PCB_BD_EPC -- the epc for the instruction to emulate
 *	PCB_BD_CAUSE -- the cause register with the branch delay bit set
 *			correctly with respect to the epc above
 * return state:
 *	a3 -- the resulting program counter after the emulated instruction
 *	a0 -- the new or existing exception frame
 *      user's register values as modified by emulated instruction in either
 *	the register or in the exception frame (as per the locore.s interface)
 *
 * In the cases that this instruction is going to be executed on the user's
 * stack the return address (from the place in the kernel emulate_instr() was
 * called) is saved in the pcb at PCB_BD_RA.  The value on PCB_BD_RA is zero
 * when not doing this emulation so that spurious reserved break instructions 
 * used for this emulation can be detected (in VEC_breakpoint).  PCB_BD_RA is
 * normally used on the return path from the executed instruction via the break
 * instruction that will get executed after it.  If the instruction causes an
 * error the trap() routine looks for a non-zero value of PCB_BD_RA so it
 * will know the real values for the epc and cause register for this error are
 * in PCB_BD_EPC and PCB_BD_CAUSE.
 */

LEAF(emulate_instr)
	bne	a1,zero,1f	# is this a nop?
	addu	a3,a2,4		# the next pc is just the next instr (epc)+4
	j	ra		# return
	
1:
	# Check for instructions which unconditionally transfer control to
	# a program counter to other than just the next instruction.

	srl	s0,a1,IMMED_SHIFT	# is this an unconditional branch instr?
	bne	s0,OPCODE_BEQ<<(OPCODE_SHIFT-16),2f
	sll	a1,IMMED_SHIFT		# sign extend the immediate offset
	sra	a1,IMMED_SHIFT-2
	addu	a3,a2,a1		# add the epc (a2) to the offset (a1)
	addu	a3,4
	j	ra			# return
2:
	# Note that s0 is assumed to hold the opcode until it is determined
	# how the instruction is to be executed or emulated.
	srl	s0,a1,OPCODE_SHIFT

	bne	s0,OPCODE_J,2f		# is this a jump instruction?
	and	a3,a2,PC_JMP_MASK	# get the high bits from the epc (a2)
	and	a1,TARGET_MASK		# get the target from the instr (a1)
	sll	a1,2			# shift the target left two bits
	or	a3,a1			# combine the target and epc bits
	j	ra			# return
2:
	bne	s0,OPCODE_JAL,3f	# is this a jump and link instr?
	addu	s0,a2,8			# epc (a2) + 8 value of the link reg
	sw	s0,EF_RA*4(a0)		# store value of the link in user's RA
	and	a3,a2,PC_JMP_MASK	# get the high bits from the epc (a2)
	and	a1,TARGET_MASK		# get the target from the instr (a1)
	sll	a1,2			# shift the target left two bits
	or	a3,a1			# combine the target and epc bits
	j	ra			# return
3:
	# Check for jump register and jump and link register instructions
	bne	s0,OPCODE_SPECIAL,5f	# check for SPECIAL opcode
	and	a3,a1,FUNC_MASK
	beq	a3,FUNC_JR,4f		# is this a jump reg instr?
	bne	a3,FUNC_JALR,5f		# is this a jump and link reg instr?
	addu	a2,8			# epc (a2) + 8 value of the link reg
	sw	a2,EF_RA*4(a0)		# store value of the link in user's RA
4:	# get the register for the target address of the jump instruction
	# then just return
	srl	a1,BASE_SHIFT-2
	and	a1,BASE_MASK<<2
	lw	a1,jreg_tbl(a1)
	j	a1

jreg_tbl:
	.word	jreg_zero:1, jreg_AT:1, jreg_v0:1, jreg_v1:1, jreg_a0:1
	.word	jreg_a1:1, jreg_a2:1, jreg_a3:1, jreg_t0:1, jreg_t1:1
	.word	jreg_t2:1, jreg_t3:1, jreg_t4:1, jreg_t5:1, jreg_t6:1
	.word	jreg_t7:1, jreg_s0:1, jreg_s1:1, jreg_s2:1, jreg_s3:1
	.word	jreg_s4:1, jreg_s5:1, jreg_s6:1, jreg_s7:1, jreg_t8:1
	.word	jreg_t9:1, jreg_k0:1, jreg_k1:1, jreg_gp:1, jreg_sp:1
	.word	jreg_fp:1, jreg_ra:1

jreg_zero:
	move	a3,zero;	j	ra
jreg_AT:
	lw	a3,EF_AT*4(a0);	j	ra
jreg_v0:
	move	a3,v0;		j	ra
jreg_v1:
	move	a3,v1;		j	ra
jreg_a0:
	lw	a3,EF_A0*4(a0);	j	ra
jreg_a1:
	lw	a3,EF_A1*4(a0);	j	ra
jreg_a2:
	lw	a3,EF_A2*4(a0);	j	ra
jreg_a3:
	lw	a3,EF_A3*4(a0);	j	ra
jreg_t0:
	move	a3,t0;		j	ra
jreg_t1:
	move	a3,t1;		j	ra
jreg_t2:
	move	a3,t2;		j	ra
jreg_t3:
	move	a3,t3;		j	ra
jreg_t4:
	move	a3,t4;		j	ra
jreg_t5:
	move	a3,t5;		j	ra
jreg_t6:
	move	a3,t6;		j	ra
jreg_t7:
	move	a3,t7;		j	ra
jreg_s0:
	lw	a3,EF_S0*4(a0);	j	ra
jreg_s1:
	move	a3,s1;		j	ra
jreg_s2:
	move	a3,s2;		j	ra
jreg_s3:
	move	a3,s3;		j	ra
jreg_s4:
	move	a3,s4;		j	ra
jreg_s5:
	move	a3,s5;		j	ra
jreg_s6:
	move	a3,s6;		j	ra
jreg_s7:
	move	a3,s7;		j	ra
jreg_t8:
	move	a3,t8;		j	ra
jreg_t9:
	move	a3,t9;		j	ra
jreg_k0:
	move	a3,k0;		j	ra
jreg_k1:
	lw	a3,EF_K1*4(a0);	j	ra
jreg_gp:
	lw	a3,EF_GP*4(a0);	j	ra
jreg_sp:
	lw	a3,EF_SP*4(a0);	j	ra
jreg_fp:
	move	a3,fp;		j	ra
jreg_ra:
	lw	a3,EF_RA*4(a0);	j	ra
5:
/*
 * At this point all the simple instructions have been emulated so
 * the instruction will be copied into the user area and executed
 * there.  Branch instructions are handled differently from the
 * rest of the instructions.  For branch instructions the branch
 * with a modified offset is executed.  The offset is modified to
 * branch over the first break instruction to the second break
 * instruction.
 *
 *	branch (offset)---
 *	nop              |
 *	break A          |
 *	break B  <--------
 *
 * For all other instructions (non-branch) the instruction is just
 * executed followed by a break.
 *
 *	instr
 *	break A
 */
	beq	s0,OPCODE_BCOND,1f	# is this a BCOND branch?

	and	a3,s0,BRANCH_MASK	# is this a general branch?
	beq	a3,OPCODE_BRANCHES,1f

	and	a3,s0,COPN_MASK		# is this a coprocessor op?
	bne	a3,OPCODE_COPN,2f
	srl	a3,a1,COPN_BCSHIFT	# is this a coprocessor branch?
	and	a3,COPN_BCMASK
	bne	a3,COPN_BC,2f
1:
/*
 * The instruction is now known to be a branch so save the instruction
 * for the return trip (to determine the target) and build a branch with
 * a modified offset.  Then write out the instruction, a nop, a break A
 * and a break B.  Clean-up the caches and return from the exception to
 * the modified branch.
 */
	li	a2,PCB_WIRED_ADDRESS
	sw	ra,PCB_BD_RA(a2)	# save emulate_instr caller's ra
	sw	a1,PCB_BD_INSTR(a2)	# save the branch instruction
	li	a2,EMULATE_AREA		# load address to use for emulation area
	sw	a2,EF_EPC*4(a0)		# use it for the user's return pc
	srl	a1,IMMED_SHIFT		# remove the offset from the branch
	sll	a1,IMMED_SHIFT
	or	a1,8>>2			# set the modified offset
	sw	a1,0(a2)		# store the modified branch instruction
	sw	zero,4(a2)		# store the nop instruction
	lw	a3,break_A		# get a break A instruction
	sw	a3,8(a2)		# store the break A instruction
	lw	a3,break_B		# get a break B instruction
	sw	a3,12(a2)		# store the break B instruction
	j	flush_ea		# flush emulate area from i cache

2:
	li	a2,PCB_WIRED_ADDRESS
	sw	ra,PCB_BD_RA(a2)	# save emulate_instr caller's ra
	li	a2,EMULATE_AREA		# load address to use for emulation area
	sw	a2,EF_EPC*4(a0)		# use it for the user's return pc
	sw	a1,0(a2)		# store the instruction
	lw	a3,break_A		# get a break A instruction
	sw	a3,4(a2)		# store the break A instruction
	j	flush_ea		# flush emulate area from i cache

break_A:
	break	BRK_BD_NOTTAKEN
break_B:
	break	BRK_BD_TAKEN
	END(emulate_instr)


/*
 * flush i-cache for emulate area
 */
#define	FRAME_SIZE	20
#define	A0_OFF		FRAME_SIZE+4*0
#define	A1_OFF		FRAME_SIZE+4*1
#define	A2_OFF		FRAME_SIZE+4*2
#define	A3_OFF		FRAME_SIZE+4*3
#define RA_OFF		FRAME_SIZE-4*1
NESTED(flush_ea, FRAME_SIZE, ra)
	.mask	0x80000000, -(FRAME_SIZE - 4*4)
	subu	sp,FRAME_SIZE
	sw	ra,RA_OFF(sp)
	sw	a0,A0_OFF(sp)			# preserve exc frame ptr
#if NCPUS > 1
        li      a0,PCB_WIRED_ADDRESS
        lw      a1,PCB_CPU_NUMBER(a0)
        la      a0,active_pmap
        sll    	a1,2 
        addu    a1,a1,a0
        lw      a0,0(a1)                # assert new pmap
#else
	lw	a0,active_pmap
#endif
	li	a1,EMULATE_AREA
	jal	pmap_extract
#ifdef	ASSERTIONS
	bne	v0,zero,1f
	PANIC("Emulate: pmap_extract")
1:
#endif
	lw	a0,A0_OFF(sp)
	and	a1,v0,(VA_PAGEMASK & (MAXCACHE-1))
	or	a1,K0BASE		# alias for cache page
	addu	a1,NBPG-EA_SIZE		# offset to emulate area
	.set noreorder
	mtc0	zero,C0_SR		# interrupts off
	.set reorder
	/*
	 * DECsystem 58xx systems need to make sure that the write buffer
	 * is empty to avoid an R3000 chip bug. So we use a3, which is about to
	 * to be reloaded, to accomplish this.
	 */
	lw	a3, cpu			# Get cpu (really system) type
	bne	a3, DS_5800, 1f		# If not DECsystem 5800 skip the flush
	lw	a3, kn5800_wbflush_addr	# Get address to read to cause a wbflush
	lw	a3, (a3)		# do the write buffer flush
1:

	la	a3,1f
	or	a3,K1BASE
	j	a3			# run uncached
	.set	noreorder
1:	nop
	nop
	nop
	li	a3,SR_ISC|SR_SWC
	mtc0	a3,C0_SR		# isolate and swap
	nop
	nop
	.set	reorder
	sb	zero,0(a1)		# flush emulate area
	sb	zero,4(a1)
	sb	zero,8(a1)
	sb	zero,12(a1)
	.set	noreorder
	nop
	nop
	mtc0	zero,C0_SR		# unisolate and unswap
	nop
	nop
	.set	reorder
	lw	ra,RA_OFF(sp)
	addu	sp,FRAME_SIZE
	lw	s0,EF_SR*4(a0)		# load a0 with the SR at the time of 
					#  the exception.
	.set noreorder
	mtc0	s0,C0_SR		# replace the SR to disable interrupts
	.set reorder
	j	exception_exit		# branch to exit the exception code
	END(flush_ea)
