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
static char	*sccsid = "@(#)$RCSfile: kn01.c,v $ $Revision: 1.2.3.6 $ (DEC) $Date: 1992/10/13 12:08:51 $";
#endif 
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
/* 
 * derived from kn01.c	4.4	(ULTRIX)  11/14/90";
 */


/*
 * Modification History: kn01.c
 *
 * 27-Jun-91 Scott Cranston
 *      Changed optional compilation of binary error logging support startegy.
 *         - added #include <uerf.h>
 *         - changed instances of '#ifdef _UERF' to '#if UERF'
 *
 * 01-Mar-91	Mark Parenti
 *	Modified to use new I/O data structures and configuration.
 *
 * 01-Nov-90	Donald Dutile
 *	Modified isolatepar() to use kseg1 addresses when writing
 *	parity test pattern(s) to suspected bad memory/parity location.
 *	Kseg1 addr used to prevent TLB MOD exception when page being
 *	written to is shared memory (not write enabled in TLB entry). 
 *	Using kseg1 works since main memory < 512MB.
 *
 * 17-Aug-90	Randall Brown
 *	Changed call to ib_config_dev() to configure "fb" device instead 
 *	of "pm" device. 
 *
 * 30-Apr-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 29-Mar-90 -- gmm
 *	Removed splhigh() and splx() from kn01memintr(). The routine gets
 *	entered at splmem() anyway.
 *
 * 02-Dec-89 -- afd
 *	Set fixtick to the number of microsec's that the clock looses per
 *	second.  This is used in hardclock() to fix the time.
 *
 *	Fix a few bugs in parity error handling.
 *
 * 13-Oct-89    gmm
 *	SMP changes - use nofault as a per cpu variable (in cpudata)
 *
 * 22-Sept-89   jls
 *      replaced the direct call to siiprobe will a call to ib_config_cont
 *
 * 21-Sept-89	burns (merge from ISIS pool)
 *    12-sep-89 afd
 *	Block network interrupts around lnprobe to avoid a race condition
 *	while we are switching network interrupts from kn01lnstray to netintr. 
 *
 *	Assign clock address in init routine (since 3max shares same clock
 *	routines but at a different address).
 *
 *	Call cnprobe with the virt address of the dc7085 chip.  Again, 3max
 *	shares the same driver but at a different address.
 *
 *    14-June-89	afd
 *	Add kn01init routine for splm, intr_vectors, iplmask and hz setup.
 *	Handle Lance "stray" interrupts that occur before lnprobe is done.
 *	(It is a pmax console firmware bug that allows early lance interrupts)
 *
 *    27-Apr-89	afd
 *	Change startup msg from "DECStation 3100" to "KN01 processor".
 *
 *    18-July-89 afd
 *	Handle Lance "stray" interrupts that occur before lnprobe is done.
 *	(It is a pmax console firmware bug that allows early lance interrupts)
 *
 * 14-Jun-89	darrell
 *	Added kn01init() -- called from startup().
 *
 * 27-Apr-89	afd
 *	Change startup msg from "DECStation 3100" to "KN01 processor".
 *
 * 07-Apr-89	afd
 *	Created this file which contains routines specific to PMAX
 *	systems (KN01).  Much of this code came from autoconf.c & trap.c.
 */

#include <machine/cpu.h>

#include <sys/systm.h>
#include <sys/proc.h>
#include <mach/machine.h>
#include <sys/vmmac.h>
#include <sys/syslog.h>

#include <dec/binlog/errlog.h>

#include <sys/table.h>

#include <machine/reg.h>
#include <hal/mc146818clock.h>
#include <hal/cpuconf.h>
#include <io/common/devdriver.h>

#define	KN01LANCE_ADDR 0x18000000	/* physical addr of lance registers */
#define KN01SII_ADDR   0x1a000000       /* physical addr of sii registers */
#define	KN01CLOCK_ADDR 0x1d000000		/* phys addr of clock chip */
#define	KN01DC_ADDR 0x1c000000			/* phys addr of dc7085 (dz) */
#define KN01FRAME_BUF_ADDR 0x0fc00000		/* phys addr of frame buffer */
#define KN01_DELAY_MULT	8

#define ESRPKT 1
#define MEMPKT 2


/*
 * PMAX control and status register
 *
 */

#define PM_CSR_ADDR	((volatile short *)PHYS_TO_K1(0x1e000000))
#define PM_CSR_MNFMOD	0x8000
#define PM_CSR_STATUS	0x4000
/* empty */
#define PM_CSR_CRSRTST	0x1000
#define PM_CSR_MONO	0x0800
#define PM_CSR_MEMERR	0x0400
#define PM_CSR_VINT	0x0200
#define PM_CSR_TXDIS	0x0100
/* empty */
#define PM_CSR_VBGTRG	0x0004
#define PM_CSR_VRGTRG	0x0002
#define PM_CSR_VRGTRB	0x0001

/*
 * These defines, macros, and variables are for memory parity errors.
 * Format of "memreg" for logging memory parity errors.
 */
#define SIMMOFF 28
#define TYPEOFF 26
#define BYTEOFF 25
#define DPOFF 24
#define TCOUNTOFF 16
#define SCOUNTOFF 8
#define HCOUNTOFF 0
#define MEMREGFMT(simm, type, byte, dp, tcount, scount, hcount) \
(simm << SIMMOFF | type << TYPEOFF | byte << BYTEOFF | dp << DPOFF | \
tcount << TCOUNTOFF | scount << SCOUNTOFF | hcount << HCOUNTOFF)

#define TRANSINTVL (60*15)	/* time delta to enable parity log - 15 mins */
int kn01transintvl = TRANSINTVL;/* global var so we can change it */
int kn01translog = 1;		/* is trans logging currently enabled */

struct trans_errcnt {             /* trans parity errors */
        long   trans_last;	/* time of most recent trans err */
        long   trans_prev;	/* time of previous trans err */
} trans_errcnt = { 0, 0 };

#define TRANSPAR 0x1
#define SOFTPAR 0x2
#define HARDPAR 0x3
#define MAXSIMM 16
int tcount[MAXSIMM];			/* # of transient parity errs per simm*/
int scount[MAXSIMM];			/* # of soft errs on each simm */
int hcount[MAXSIMM];			/* # of hard errs on each simm */
int parityerr = 0;			/* flag for parity err isolation */
caddr_t vatophys();			/* typedef of functions */
unsigned isolatepar();
int kn01transenable();

extern u_int printstate;	/* how to print to the console */
extern char *rt_clock_addr;	/* addr of the mc146818clock chip */
extern struct timeval time;	/* the system time */
extern int hz;
extern int tick;
extern int tickadj;
extern int fixtick;		/* set to the number of microsec's that the
				   clock looses per second */

/*
 * Interrupt handlers for the 6 hardware interrupts and 2 software
 * interrupts for PMAX.
 */
extern softclock(),softintr2(),cn_intr(),
	kn_hardclock(), fp_intr(),memintr();
biointr();
netintr();

kn01lnstray();

/* The routines that correspond to each of the 8 interrupt lines */
int (*kn01intr_vec[IPLSIZE])() = {
	/* softint 1 */		softclock,	/* AST */
	/* softint 2 */		softintr2,	/* NETWORK or unused */
	/* hardint 3 */		biointr,	/* SCSI */
	/* hardint 4 */		kn01lnstray,	/* LANCE */
	/* hardint 5 */		cn_intr,	/* CONSOLE */
	/* hardint 6 */		kn_hardclock,	/* CLOCK */
	/* hardint 7 */		memintr,	/* PARITY or VIDEO */
	/* hardint 8 */		fp_intr		/* FPU */
};

/*
 * Define mapping of interrupt lines with the type of interrupt.
 * This is basically taken from the table kn01intr_vec declared
 * immediately about this.
 */
static int KN01_interrupt_type[INTR_MAX_LEVEL] = {
		INTR_TYPE_SOFTCLK,
		INTR_TYPE_SOFTCLK,
		INTR_TYPE_DEVICE,
		INTR_TYPE_STRAY,
		INTR_TYPE_DEVICE,
		INTR_TYPE_HARDCLK,
		INTR_TYPE_OTHER,
		INTR_TYPE_OTHER
};

/*
 * Interrupt table types
 */
int     kn01c0vec_tbl_type[NC0VECS] = {
	/* softint 1 */		INTR_SOFTCLK,	/* AST */
	/* softint 2 */		INTR_SOFTCLK,	/* NETWORK or unused */
	/* hardint 3 */		INTR_NOTCLOCK,	/* SCSI */
	/* hardint 4 */		INTR_NOTCLOCK,	/* LANCE */
	/* hardint 5 */		INTR_NOTCLOCK,	/* CONSOLE */
	/* hardint 6 */		INTR_HARDCLK,	/* CLOCK */
	/* hardint 7 */		INTR_NOTCLOCK,	/* PARITY or VIDEO */
	/* hardint 8 */		INTR_NOTCLOCK	/* FPU */
};


/*
 * The masks to use to look at each of the 8 interrupt lines.
 *
 * Due to the different priorities of TTY, BIO and IMP on PMAX from
 * the norm all are effectivly wired into the same level.
 */
int kn01iplmask[IPLSIZE] = {
	SR_IMASK1|SR_IEC,
	SR_IMASK2|SR_IEC,
	SR_IMASK3|SR_IEC,
	SR_IMASK4|SR_IEC,
	SR_IMASK5|SR_IEC,
	SR_IMASK6|SR_IEC,
	SR_IMASK7|SR_IEC,
	SR_IMASK8|SR_IEC
};

/*
 * The SR reg masks for splxxx usage. See the defines in cpu.h.
 */
int kn01splm[SPLMSIZE] = {
	SR_IEC | SR_IMASK0,		/* 0 SPLNONE			*/
	SR_IEC | SR_IMASK1,		/* 1 SPLSOFTC			*/
	SR_IEC | SR_IMASK2,		/* 2 SPLNET			*/
	SR_IEC | SR_IMASK3,		/* 3 SPLBIO 			*/
	SR_IEC | SR_IMASK4,		/* 4 SPLIMP			*/
	SR_IEC | SR_IMASK5,		/* 5 SPLTTY			*/
	SR_IEC | SR_IMASK6,		/* 6 SPLCLOCK			*/
	SR_IEC | SR_IMASK7,		/* 7 SPLMEM			*/
	SR_IEC | SR_IMASK8		/* 8 SPLFPU			*/
};

/*
 * Initialization routine for kn01 processor (pmax).
 */
kn01init()
{
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int c0vec_tbl_type_size;
	extern int c0vec_tbl_type[];
	extern int iplmask[];
	extern int splm[];
	extern int kn_delay_mult;
	extern struct cpusw *cpup;
	extern volatile int *system_intr_cnts_type_transl;

	/*
	 * Setup global interrupt type array to use our definitions.
	 */
	system_intr_cnts_type_transl = KN01_interrupt_type;

	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the interrupt type table.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt (or spl) is allowed.
	 */
	bcopy((int *)kn01intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn01c0vec_tbl_type, c0vec_tbl_type, c0vec_tbl_type_size);
	bcopy(kn01iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn01splm, splm, (SPLMSIZE) * sizeof(int));

	/* initialize the dealy loop multiplier */
	kn_delay_mult = KN01_DELAY_MULT;


	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
	 */
	hz = cpup->HZ;
	tick = 1000000 / hz;
	tickadj = 240000 / (60 * hz);
	fixtick = 1000000 - (tick * hz);

	/*
	 * Assign the rt_clock_addr for this processor
	 */
	rt_clock_addr = (char *)PHYS_TO_K1(KN01CLOCK_ADDR);

	return (0);
}

/*
 * Configuration routine for kn01 processor (pmax).
 */
kn01conf()
{
	extern unsigned cpu_systype;
	extern int cpu;			/* Ultrix internal System type */
	register struct bus *sysbus;

/* TODO:	cpu_subtype_t subtype; */

	/* 
	 * Report what system we are on
	 */
	printf("KN01 processor - system rev %d\n", (GETHRDREV(cpu_systype)));

	coproc_find();
	/*
	 * Below is some stuff from generic osf 'configue' routine.
	 * burns.
	 */
	master_cpu = 0;
#if	NCPUS > 1
	printf("Master cpu at slot %d.\n", master_cpu);
#endif	/* NCPUS > 1 */
	machine_slot[master_cpu].is_cpu = TRUE;
	machine_slot[master_cpu].cpu_type = CPU_TYPE_MIPS;
/* TODO:	machine_slot[master_cpu].cpu_subtype = subtype; */
	machine_slot[master_cpu].cpu_subtype = CPU_SUBTYPE_MIPS_R2000a;
	machine_slot[master_cpu].running = TRUE;
	machine_slot[master_cpu].clock_freq = hz;

	config_delay();

	/*
	 * Get the system bus structure and call the bus configuration code
	 * system_bus is global pointer used by loadable code when walking
	 * the hardware topology tree.  It is extern'd via devdriver.h.
	 */
	system_bus = sysbus = get_sys_bus("ibus");
	if (sysbus == 0) 
		panic("No system bus configured");
	(*sysbus->confl1)(-1, 0, sysbus); /* call level one configuration */
	(*sysbus->confl2)(-1, 0, sysbus); /* call level two configuration */

	timeout (kn01transenable, (caddr_t) 0, kn01transintvl * hz);
	return(0);
}

/*
 * Handle stray interrupts that occur before the lance is probed & attached.
 * This only occurs if the console doesn't initialize the lance properly.
 */

#define LN_CSR0 0x0000			/* CSR 0 */
#define LN_STOP 0x0004			/* Reset firmware (stop) */

kn01lnstray()
{
	volatile unsigned short *addr;	/* 16 bit lance registers */

	addr = (volatile unsigned short *)PHYS_TO_K1(KN01LANCE_ADDR);
	*addr = LN_CSR0;
	/*
	 * The lance registers are long word alligned, so increment by 2 shorts
	 */
	addr += 2;
	*addr = LN_STOP;
	return;
}


/*
 * biointr & netintr used for DECstation 2100/3100 to quickly 
 * get to intr routines with the proper argument.
 */
biointr(ep)
u_int   *ep;
{
        sii_intr(0);
}

netintr(ep)
u_int   *ep;
{
        lnintr(0);
}


/*
 * Strategy here is to probe all I/O chips/controllers in one place
 *   and set the appropriate states/flags for further use.
 */
kn01config_devices(bus) 
struct bus *bus;
{
	register struct driver *drp;
	register struct controller *ctlr;
	int s, i;

	/*
	 * Probe the system console subsystem;
	 *   set the printstate flag to CONSPRINT if successful.
	 */
	if (ib_config_cont(PHYS_TO_K1(KN01DC_ADDR), KN01DC_ADDR, -1, "dc", bus, 0)) {
	        printstate = printstate | CONSPRINT;
	}

	/*
	 * Probe the frame buffer graphics driver;
	 *   both color and mono bitmaps use the same driver
	 */
	if (!bbadaddr(PHYS_TO_K1(KN01FRAME_BUF_ADDR), 1, 0))
	    ib_config_cont(PHYS_TO_K1(KN01FRAME_BUF_ADDR), KN01FRAME_BUF_ADDR, -1, "fb", bus, 0);

	/*
	 * Probe the sii based scsi bus.
	 */
	ib_config_cont(PHYS_TO_K1(KN01SII_ADDR),KN01SII_ADDR,-1,"sii",bus,0);

	/*
	 * Config the Lance.
	 * Note: We must block Lance interrupts until after the lnprobe
	 *       is done and interrupts are vectored to the "netintr" routine.
	 *	 Otherwise there is a race condition: when lnprobe turns the
	 *	 Lance on, the first interrupt could go to kn01lnstray and
	 *	 turn the lance off!
	 */
	s = splimp();
	if (ib_config_cont(PHYS_TO_K1(KN01LANCE_ADDR),KN01LANCE_ADDR,-1,"ln",bus,0)) {
	        c0vec_tbl[3] = netintr;
	} else {
		printf("ln0 not probed\n");
	}
	splx(s);
	return(0);
}

/*
 * Enable transient parity memory error logging
 */
kn01transenable()
{
	kn01translog = 1;
	timeout (kn01transenable, (caddr_t) 0, kn01transintvl * hz);
}


/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 */
kn01trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr */
	register u_int code;		/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
	int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */	
	int epc;			/* the EPC of the error */	
	unsigned memreg;		/* memory parity error info */
	int vaddr;			/* virt addr of error */
	register struct proc *p;	/* ptr to current proc struct */
	long currtime;			/* current time value */

	p = u.u_procp;
	if (USERMODE(sr)) {
		/*
		 * If address of bus error is in physical memory, then its
		 * a parity memory error.  Gather additional info in "memreg",
		 * for the error log & to determine how to recover.
		 * If its a transient error then continue the user process.
		 * If its a hard or soft parity error:
		 *    a) on a private process page, terminate the process
		 *	 (by setting signo = SIGBUS)
		 *    b) on a shared page, crash the system.
		 * TBD: on a non-modified page, re-read the page (page fault),
		 *	and continue the process.
		 * TBD: on a shared page terminate all proc's sharing the page,
		 *	instead of crash system.
		 * TBD: on hard errors map out the page.
		 */
		pa = vatophys(ep[EF_BADVADDR]);
		if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
			/*
			 * Note that we must save anything "interesting"
			 * from the exception frame, since isolatepar()
			 * may cause additional bus errors which will
			 * stomp on the exception frame in locore.
			 */
			vaddr = ep[EF_BADVADDR];
			epc = ep[EF_EPC];
			memreg = isolatepar(pa, vaddr);
			ep[EF_BADVADDR] = vaddr;
			ep[EF_EPC] = epc;
			/*
			 * If we get 3 or more in 1 second then disable logging
			 * them for 15 minutes.  The variable "kn01translog"
			 * is set by the kn01transenable routine.
			 */
			if (((memreg >> TYPEOFF) & TRANSPAR) == TRANSPAR) {
			    if (kn01translog) {
				currtime = time.tv_sec;
				if (currtime == trans_errcnt.trans_prev) {
					kn01translog = 0;
					printf("High rate of transient parity memory errors, logging disabled for 15 minutes\n");
					trans_errcnt.trans_last = 0;
					currtime = 0;
				}
			        pmaxlogmempkt(EL_PRIHIGH, ep, memreg, pa);
				trans_errcnt.trans_prev = trans_errcnt.trans_last;
				trans_errcnt.trans_last = currtime;
			    }
			    return(0);
			}
/* TODO:  ULTRIX kn01trap_error Shared page code is out....
			if (SHAREDPG(pa)) {	****/
				pmaxlogmempkt(EL_PRISEVERE, ep, memreg, pa);
				pmaxconsprint(MEMPKT, ep, memreg, pa);
				panic("memory parity error");
/**** TODO: 		} else {
				pmaxlogmempkt(EL_PRIHIGH, ep, memreg, pa);
 				printf("pid %d (%s) was killed on memory parity error\n", 
					p->p_pid, u.u_comm);
				uprintf("pid %d (%s) was killed on memory parity error\n",
					p->p_pid, u.u_comm);
 TODO: 			} */
		} else {
			uprintf("pid %d (%s) was killed on bus error\n",
				p->p_pid, u.u_comm);
		}
	} else {
		/*
		 * Kernel mode errors.
		 * They all panic, its just a matter of what we log
		 * and what panic message we issue.
		 */
		switch (code) {

		case EXC_DBE:
		case EXC_IBE:
			/*
			 * Figure out if its a memory parity error
			 *     or a read bus timeout error
			 */
			pa = vatophys(ep[EF_BADVADDR]);
			if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
				/*
				 * Note that we must save anything "interesting"
				 * from the exception frame, since isolatepar()
				 * may cause additional bus errors which will
				 * stomp on the exception frame in locore.
				 */
				vaddr = ep[EF_BADVADDR];
				epc = ep[EF_EPC];
				memreg = isolatepar(pa, vaddr);
				ep[EF_BADVADDR] = vaddr;
				ep[EF_EPC] = epc;
				pmaxlogmempkt(EL_PRISEVERE, ep, memreg, pa);
				pmaxconsprint(MEMPKT, ep, memreg, pa);
				panic("memory parity error in kernel mode");
			} else {
				pmaxlogesrpkt(ep, EL_PRISEVERE);
				pmaxconsprint(ESRPKT, ep, 0, 0);
				panic("bus timeout");
			}
			break;
		case EXC_CPU:
			pmaxlogesrpkt(ep, EL_PRISEVERE);
			pmaxconsprint(ESRPKT, ep, 0, 0);
			panic("coprocessor unusable");
			break;
		case EXC_RADE:
		case EXC_WADE:
			pmaxlogesrpkt(ep, EL_PRISEVERE);
			pmaxconsprint(ESRPKT, ep, 0, 0);
			panic("unaligned access");
			break;
		default:
			pmaxlogesrpkt(ep, EL_PRISEVERE);
			pmaxconsprint(ESRPKT, ep, 0, 0);
			panic("trap");
			break;
		}
	}
	/*
	 * Default user-mode action is to terminate the process
	 */
	*signo = SIGBUS;
	return(0);
}

unsigned sbe_addr;
int memintr_cnt = 0;

/*
 * Bus timeout on write.
 * Caused by memory failure, or write to a non-existent address
 *
 * This does not happen synchronously (buffered write),
 *    therefor we are not in process context and cannot terminate
 *    a user process.  We must crash the system.
 *
 * Video interrupt is on the same interrupt line so it comes here too.
 */
kn01memintr(ep)
	u_int *ep;		/* exception frame ptr */
{
	register volatile short *pm_csr;
	register short pmcsr;
	caddr_t pa;			/* the physical addr of error */

	pm_csr = PM_CSR_ADDR;
	pmcsr = *pm_csr;
	memintr_cnt++;
	if (pmcsr & PM_CSR_MEMERR) {

		/*
		 * clear the pending bus error and save the address away 
		 * for post-mortems.
		 */
		*pm_csr = PM_CSR_MEMERR|pmcsr|0x00ff;
		sbe_addr = *(volatile unsigned *)PHYS_TO_K1(SBE_ADDR);
		/*
		 * Figure out if its a failed write to memory.
		 * or a write to a bad address.
		 */
		pa = vatophys(ep[EF_BADVADDR]);
		if ((int)pa != -1 && (btop((int)pa) < physmem) ) {
			pmaxlogesrpkt(ep, EL_PRISEVERE);
			pmaxconsprint(ESRPKT, ep, 0, 0);
			panic("memintr, memory failure");
		} else {
			pmaxlogesrpkt(ep, EL_PRISEVERE);
			pmaxconsprint(ESRPKT, ep, 0, 0);
			panic("memintr, write timeout");
		}
	}
	else {
		*pm_csr = PM_CSR_VINT|pmcsr|0x00ff;
	}
	return(0);
}

/*
 * Log Error & Status Registers to the error log buffer
 */
pmaxlogesrpkt(ep, priority)
	register u_int *ep;	/* exception frame ptr */
	int priority;		/* for pkt priority */
{
	struct el_rec *elrp;

	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
	      LSUBID(elrp,ELCT_MCK,ELESR_kn01,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	      elrp->el_body.elesr.elesr.el_esrkn01.esr_cause = ep[EF_CAUSE];
	      elrp->el_body.elesr.elesr.el_esrkn01.esr_epc = ep[EF_EPC];
	      elrp->el_body.elesr.elesr.el_esrkn01.esr_status = ep[EF_SR];
	      elrp->el_body.elesr.elesr.el_esrkn01.esr_badva = ep[EF_BADVADDR];
	      elrp->el_body.elesr.elesr.el_esrkn01.esr_sp = ep[EF_SP];
	      EVALID(elrp);
	}

	log(LOG_ERR, "kn01 error: Cause 0x%x PC 0x%x Status 0x%x Bad VA 0x%x Sp 0x%x\n",
	    ep[EF_CAUSE], ep[EF_EPC], ep[EF_SR], ep[EF_BADVADDR], ep[EF_SP]);
}

/*
 * Log a memory error packet, so uerf can find it as a main memory error.
 */
pmaxlogmempkt(priority, ep, memreg, pa)
	int priority;		/* pkt priority: panic: severe; else: high */
	register u_int *ep;	/* exception frame ptr */
	unsigned memreg;	/* assorted parity error info */
	int pa;			/* physical addr where memory err occured */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;

	elrp = ealloc(EL_MEMSIZE, priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_PMAX,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		mrp->elmemerr.type = ELMETYP_PAR;
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = memreg;
		mrp->elmemerr.regs[1] = pa;
		mrp->elmemerr.regs[2] = ep[EF_EPC];;
		mrp->elmemerr.regs[3] = ep[EF_BADVADDR];;
		EVALID(elrp);
	}

	log(LOG_ERR, "kn01 memory parity error: Mem reg 0x%x Pa 0x%x PC 0x%x Bad VA 0x%x\n",
	    memreg, pa, ep[EF_EPC], ep[EF_BADVADDR]);
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 *
 * Note: side-effect.
 *	If console is a graphics device, printstate is changed  to force
 *	kernel printfs directly to the screen.
 */
pmaxconsprint(pkt, ep, memreg, pa)
	int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
	register u_int *ep;	/* exception frame ptr */
	unsigned memreg;	/* For MEMPKT: assorted parity error info */
	unsigned pa;		/* For MEMPKT: physical addr of error */	
{
	register int i;
	int ws_disp;
	int simm;
	int byte;

	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 */
	printstate |= PANICPRINT;

	switch (pkt) {
	case ESRPKT:
		printf("\nException condition\n");
		printf("\tCause reg\t= 0x%x\n", ep[EF_CAUSE]);
		printf("\tException PC\t= 0x%x\n", ep[EF_EPC]);
		printf("\tStatus reg\t= 0x%x\n", ep[EF_SR]);
		printf("\tBad virt addr\t= 0x%x\n", ep[EF_BADVADDR]);
		break;
	case MEMPKT:
		printf("\nMemory Parity Error\n");
		simm =  (memreg >> SIMMOFF) & 0xf;
		printf("\tSIMM (module number)\t= %d\n", simm);
		if (((memreg >> TYPEOFF) & HARDPAR) == HARDPAR)
			printf("\tHard error\t\n");
		else if (((memreg >> TYPEOFF) & SOFTPAR) == SOFTPAR)
			printf("\tSoft error\t\n");
		else printf("\tTransient error\t\n");
		if (simm & 0x1) {
			/* odd simm: low half word */
			if ((memreg >> BYTEOFF) & 0x1)
				byte = 1;
			else
				byte = 0;
		} else {
			/* even simm: high half word */
			if ((memreg >> BYTEOFF) & 0x1)
				byte = 3;
			else
				byte = 2;
		}
		printf("\tByte in error (0-3)\t= %d\n", byte);
		printf("\t%s bit error\n", ((memreg >> DPOFF) & 0x1) ? "Parity" : "Data");
		printf("\tTransient errors for this SIMM\t= %d\n", tcount[simm]);
		printf("\tSoft errors for this SIMM\t= %d\n", scount[simm]);
		printf("\tHard errors for this SIMM\t= %d\n", hcount[simm]);
		printf("\tPhysical address of error\t= 0x%x\n", pa);
		printf("\tException PC\t\t\t= 0x%x\n", ep[EF_EPC]);
		printf("\tVirtual address of error\t= 0x%x\n", ep[EF_BADVADDR]);
		break;
	default:
		printf("bad consprint\n");
		break;
	}
}

/*
 * Isolate a memory parity error to which SIMM is in error.
 * This routine is machine specific, in that it "knows" how the memory
 * is laid out, i.e. how to convert a physical address to a module number.
 *
 * Block faults from occuring while we isolate the parity error by using
 * "nofault" facility thru the bbadaddr routine.
 */
unsigned
isolatepar(pa, va)
	register caddr_t pa;	/* the phys addr to convert to a SIMM */	
	caddr_t va;		/* the virtual addr of the error */	
{
	register int i;		/* loop index */	
	register char *addr;	/* increment thru the word w/ parity error */
	register char *k1_addr;	/* kseg1 addr. for mem test writes */
	unsigned memreg;	/* collection of memory error info */
	int odd;		/* true if its the odd numbered SIMM */
	int simm;		/* which simm had the error */
	register int allzeros;	/* true if parity err occurs on all 0's write */
	register int allones;	/* true if parity err occurs on all 1's write */
	register int oneone;	/* true if parity err occurs on 1 1 write */
	int dp;			/* 0 for data bit, 1 for parity bit */
	int type;		/* error type: transient, soft, hard */
	int byte;		/* 0 for low byte; 1 for high byte in word */

	/*
	 * Round address down to long word, & clear flags.
	 */
	addr = (char *)((int)va & (~0x3));
	type = 0;
	dp = 0;
	/*
	 * Do badaddr probe on addr (a few times),
	 * to see if it was only a transient.
	 */
	parityerr = 0;
	for (i = 0; i < 4; i++) {
		if (bbadaddr(addr, 4, 0)) {
			parityerr = 1;
			break;
		}
	}
	if (!parityerr) {
		type = TRANSPAR;
		byte = 0;
		odd = 1;
		goto getsimm;
	}
	/*
	 * Isolate the parity error to which SIMM is in error (which byte in
	 * the word) and isolate the type of error: soft or hard, data bit
	 * or parity bit.
	 *
	 * This is done by writing (& reading) each byte in the word first
	 * with all 0's then with all 1's (0xff) then with one 1 (0x1).
	 *
	 * use k1_addr in order not to get TLBMOD exception when writing
	 * shared memory space
	 */
	k1_addr = (char *)(PHYS_TO_K1(((int)pa & (~0x3)))); /* lw addr */
	for (i = 0; i < 4; i++, addr += 1, k1_addr += 1) {
		allzeros = 0;
		*k1_addr = 0x00;
		if (bbadaddr(addr, 1, 0))
			allzeros = 1;
		allones = 0;
		*k1_addr = 0xff;
		if (bbadaddr(addr, 1, 0))
			allones = 1;
		oneone = 0;
		*k1_addr = 0x1;
		if (bbadaddr(addr, 1, 0))
			oneone = 1;
		/*
		 * If all 3 reads caused the error then this is the wrong
		 * byte, go on to the next byte
		 */
		if (allzeros && allones && oneone)
			continue;
		/*
		 * If only one of the allones/allzeros patterns caused a
		 * parity error, then we have a hard data bit stuck to
		 * zero or one.
		 */
		if ((allzeros && !allones && !oneone) ||
		    (allones && !allzeros && !oneone)) {
			type = HARDPAR;
			break;
		}
		/*
		 * If only the "oneone" (0x1) pattern caused a parity error,
		 *   then we have a parity bit stuck to zero.
		 * If only the "oneone" (0x1) pattern did NOT cause a parity
		 *   error then we have a parity bit stuck to one.
		 */
		if ((oneone && !allzeros && !allones) ||
		    (allzeros && allones && !oneone)) {
			type = HARDPAR;
			dp = 1;
			break;
		}
		/*
		 * If no parity error on all 3 patterns then we had a soft
		 * parity error in one of the data bits or in the parity bit
		 * of this byte.
		 */
		if (!allzeros && !allones && !oneone) {
			type = SOFTPAR;
			break;
		}
	}
	/*
	 * If i is 0 or 1, parity error is on the odd SIMM.
	 * If i is 2 or 3, parity error is on the even SIMM.
	 * Also record high or low byte position in half-word.
	 */
	switch (i) {
	case 0:
		byte = 0;
		odd = 1;
		break;
	case 1:
		byte = 1;
		odd = 1;
		break;
	case 2:
		byte = 0;
		odd = 0;
		break;
	case 3:
	default:
		byte = 1;
		odd = 0;
		break;
	}
getsimm:
	/*
	 * Record which SIMM: 4 Mbytes per SIMM
	 */
	if ((int)pa < 4*1024*1024)
		if (odd)
			simm = 1;
		else
			simm = 2;
	else if ((int)pa < 8*1024*1024)
		if (odd)
			simm = 3;
		else
			simm = 4;
	else if ((int)pa < 12*1024*1024)
		if (odd)
			simm = 5;
		else
			simm = 6;
	else if ((int)pa < 16*1024*1024)
		if (odd)
			simm = 7;
		else
			simm = 8;
	else if ((int)pa < 20*1024*1024)
		if (odd)
			simm = 9;
		else
			simm = 10;
	else if (odd)
		simm = 11;
	else
		simm = 12;
	/*
	 * Increment error counts
	 */
	switch (type) {
	case TRANSPAR:
	default:
		tcount[simm]++;
		if (tcount[simm] > 255) {
			printf("Transient parity error count on simm # %d reached 255, reset to zero.\n", simm);
			tcount[simm] = 0;
		}
		break;
	case SOFTPAR:
		scount[simm]++;
		break;
	case HARDPAR:
		hcount[simm]++;
		break;
	}
	memreg = MEMREGFMT(simm, type, byte, dp, tcount[simm], scount[simm], hcount[simm]);
	return(memreg);
}
