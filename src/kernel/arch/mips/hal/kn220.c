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
static char *rcsid = "@(#)$RCSfile: kn220.c,v $ $Revision: 1.1.4.6 $ (DEC) $Date: 1992/10/13 12:15:34 $";
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
 * derived from kn230.c	4.5       (ULTRIX)  11/14/90";
 */

/*
 * Debug HINTS ( some of the following console commands are not
 *	supported but will be in the console.)
 * 
 * 1) Setting bootmode to d is intended for use ONLY with reset, NEVER
 * across power-ups.  The reason is that the DS5500 has ECC memory.  The
 * memory is indeterminate on powerup and must be initialized to avoid
 * exceptions (machine checks) from ECC errors.
 *
 * If you power the machine off, you MAY be able to fix the problem by
 * depositing into the nvram location that stores the bootmode variable
 * as follows:
 *              >>>d 28000040 0 0 0 0
 * Then power the machine off and on again. Normal powerup diagnostics
 * will be run.
 *
 * CAUTION::: If you do anything else before depositing into the bootmode
 * variable, you will probably have to zero nvram and LOSE PRESTO DATA in
 * order to resume useful operation.
 *     REMEMBER, NEVER POWER OFF WHEN BOOTMODE IS SET TO d !!!!!!!!!!!
 *
 * 2) Using continue. 
 * Pressing Break or the Halt button causes the systems state to be saved
 * in the halt state memory block.  When you enter the continue command,
 * the system state is reloaded and execution continued.
 * 
 * 3) Using d -H reg_name val
 * The -H parameter specifies that the data is to be deposited to a
 * register in the halt state memory block.  This memory location is
 * where all the R3000 internal registers are saved when the system is
 * halted.  The reg_name parameter specifies the name of the particular
 * R3000 internal register for which you want the data written.
 *
 * 4) Using dump -H
 * The -H parameter displays the contents of the halt state memory block.  
 * All the R3000 internal registers are stored in the halt state memory block 
 * when the systems is halted.
 * 
 * The above descriptions (2-4) were taken directly from the commands section
 * of the DECsystem 5500 Operation Manual (Order Number EK 332AA-OP-001)
 * 
 * Summary
 * Pressing break/halt saves the system state (all registers) in a block
 * of NVRAM called the halt state memory block.  To examine the halt
 * state, use dump -H.  To modify the halt state (for example the pc
 * register), use d -H reg_name val.  To continue execution, use the
 * continue command.
 * 
 * NOTE: The go command can't be used to for halt restart because it
 * doesn't reload the system state.  The go command transfers control the
 * entry point address.
 *
 * Revision History:
 *
 * 18 Dec 91 -- jaa
 *      port to OSF
 *
 * 19 Aug 90 -- chet
 *	Changes for presto interface.
 *
 * 07-Sep-90	sekhar
 *	Fixed to log write timeout packet only when crashing.
 *
 * 13-Aug-90	sekhar
 *      changes to support memory mapped devices:
 *      1. added kn220consinfo_t kn220log_errinfo_t to capture error information.
 *      2. added new functions kn220_print_consinfo() and kn220_log_errinfo
 *	   exported through cpusw to print information to console and to log
 *	   information in the error log buffer. 
 *      3. modified bus timeout code to capture error information and
 *         post softnet() interrupt.
 *	4. integrated kn220_print_consinfo() and kn220consprint()
 *	5. integrated kn220logesrpkt and kn220_log_errinfo(()
 *
 * 04-Aug-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 01-Aug-90	robin
 *	Changed memory intrrupt routine to clear the interrupt early,  the
 *	code as it was caused panic to loop because the interrupt was
 *	still pending and panic path lowers IPL with interrupts enabled;
 *	which causes memoery interrupt to be re-entered. (sigh)
 *
 * 09-Aug-90	Robin
 *	Fixed bug in battery test routine that read the iopres status
 *	register from an un-init var on the stack.
 *
 *	Fixed the memory interrupt routine to not try to use the "ep"
 *	because its out of context.  
 *
 *	I also changed the kn220consprint routine to return and not break
 *	out of a bad format packet type.
 *
 * 15-Jun-90    map (Mark Parenti)
 *      Add hooks to call VME configuration routine.
 *
 * 29 May 90 -- chet
 *	Add call to presto NVRAM pseudo driver initialization routine.
 *
 * 05-15-90	Robin
 *	Changed the VME present bit to be a option present bit.  We read
 *	locations on the VME or the FDDI to know whats there.  We don't have
 *	hardware yet so its to early to know if it will work.
 *
 * 04-05-90	Robin
 *	Added code to test NVRAM Battery voltage and get go values. the old
 *	code never worked.
 *
 * 11-22-1989	Robin
 *	File Created
 *
 *
 */
#include <sys/presto.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <dec/binlog/errlog.h>
#include <machine/machparam.h>
#include <machine/cpu.h>
#include <mach/machine.h>
#include <machine/ssc.h>
#include <hal/cpuconf.h>
#include <io/common/devdriver.h>
#include <machine/reg.h>
#include <hal/kn220.h>
#include <io/dec/uba/ubavar.h>
#include <sys/table.h>

#define MEG (1024*1024)

/* Read location 1610005c to clear memory interrupts.
 */
#define CLEAR_MEM_INTR	0x1610005c
#define BIT29 		0x20000000
#define BIT28 		0x10000000
#define GOOD_ADDR	0x00000002
#define ITS_A_QBUS	1

/*
 * QBUS mapping info for Non-Volatile Ram (NVRAM) which
 * is used by Prestoserve to speed up synchronous filesystem writes.
 *
 * The size is 8K smaller than it really is and the start address
 * is 8K higher than it should be.  The first 8K of NVRAM is
 * reserved for use by the console micro code and we can't
 * use it.
 */
#define NVRAM_SIZE	(0x80000 - 0x2000)	/* (512K - 8K)		*/
#define NVRAM_ADDR	(0x98000000 + 0x2000)	/* start phys 0x18000000+8K */

#define ESRPKT 1
#define MEMPKT 2

#define CVAX_TO_R3000   0x10000000
#define CVAX_TO_K1SEG   (K1BASE - CVAX_TO_R3000)
#define CVAX_TABLE_PTR	(0x20040024)
#define CVAX_TABLE      ((*(int *)(CVAX_TABLE_PTR+CVAX_TO_K1SEG)) \
				+ CVAX_TO_K1SEG)

#define NVRAM_STATUS_PTR_OFFSET   (9 * 4)

#define NVRAM_STATUS_ADDR ((*(int *)(CVAX_TABLE + NVRAM_STATUS_PTR_OFFSET)) \
                               + CVAX_TO_K1SEG)

#define NVRAMSTATUS 	(*(char *)(NVRAM_STATUS_ADDR))

extern int hz;

extern u_int printstate;

extern softclock(), softintr2(), kn220hardintr0(),
	kn220hardintr1(), kn220hardintr2(), kn220memintr(), kn220haltintr(),
	fp_intr(), xviaconf();


/* This is an array which holds address values on ECC single
 * bit errors.  When a single bit error is seen an error
 * log is generated, if and only if, its address is not in this
 * array.  If its in the array nothing is logged but if its
 * not the error is loged and the address is stored here.
 * At some later time a routine will zero this array and any
 * single bit error seen will be loged and the process starts over.
 */
unsigned int Sbit_error[SBIT_SIZE];

struct esr {
	u_long esr_dser;	/* DMA System Error Reg */
	u_long esr_qbear;	/* QBus Error Address Reg */
	u_long esr_dear;	/* DMA Error Address Reg */
	u_long esr_cbtcr;	/* CDAL Bus Timeout Control Reg */
	u_long esr_isr;		/* Interrupt Status Reg	*/
	u_long esr_ipcr;	/* Inter Process Communication Reg. */
	u_long esr_mesr;	/* Memory Error Syndrome Reg. */
	u_long esr_mear;	/* Memory Error Address Reg. */
} esr[1];

/* The routines that correspond to each of the 8 interrupt lines */
int (*kn220intr_vec[IPLSIZE])() = {
	softclock,		/* softint 1	*/
	softintr2,		/* softint 2	*/
	kn220hardintr0,		/* hardint 1	*/
	kn220hardintr1,		/* hardint 2	*/
	kn220hardintr2,		/* hardint 3	*/
	kn220memintr,		/* hardint 4	*/
	kn220haltintr,		/* hardint 5	*/
	fp_intr			/* hardint 6	*/
};

/*
 * Define mapping of interrupt lines with the type of interrupt.
 * This is basically taken from the table kn220intr_vec declared
 * immediately about this.
 */
static int KN220_interrupt_type[INTR_MAX_LEVEL] = {
		INTR_TYPE_SOFTCLK,
		INTR_TYPE_SOFTCLK,
		INTR_TYPE_DEVICE,
		INTR_TYPE_DEVICE,
		INTR_TYPE_HARDCLK,
		INTR_TYPE_OTHER,
		INTR_TYPE_OTHER,
		INTR_TYPE_OTHER
};

/* The masks to use to look at each of the 8 interrupt lines */
int kn220iplmask[IPLSIZE] = {
	SR_IMASK1|SR_IEC,
	SR_IMASK2|SR_IEC,
	SR_IMASK3|SR_IEC,
	SR_IMASK4|SR_IEC,
	SR_IMASK5|SR_IEC,
	SR_IMASK6|SR_IEC,
	SR_IMASK7|SR_IEC,
	SR_IMASK8|SR_IEC
};

/* The status register masks for splxxx usage */
int kn220splm[SPLMSIZE] = {
#define SR_HALT	0x4000			/* Halt interrupt */
        SR_IEC | SR_IMASK0 | SR_HALT,   /* 0 SPLNONE                    */
        SR_IEC | SR_IMASK1 | SR_HALT,   /* 1 SPLSOFTC                   */
        SR_IEC | SR_IMASK2 | SR_HALT,   /* 2 SPLNET                     */
        SR_IEC | SR_IMASK4 | SR_HALT,   /* 3 SPLBIO qbus 5,4 uq & ni    */
        SR_IEC | SR_IMASK4 | SR_HALT,   /* 4 SPLIMP ibus, qbus 6 ni,sii,dssi */
        SR_IEC | SR_IMASK4 | SR_HALT,   /* 5 SPLTTY                     */
        SR_IEC | SR_IMASK5 | SR_HALT,   /* 6 SPLCLOCK                   */
        SR_IEC | SR_IMASK6 | SR_HALT,   /* 7 SPLMEM                     */
        SR_IEC | SR_IMASK8 | SR_HALT,   /* 8 SPLFPU                     */
};

/* A record of each memory board size in each of four slots on the system
 */
int memory_slot[4];

int kn220_usenvram = 1;
int kn220_battdelay = 1000;
int kn220_cache_nvram = 1;
int kn220_nvram_mapped = 0;
/*
 *	The structure kn220consinfo_t can be used to store information
 *	to be printed on a console. This information can then be printed
 *	by kn220_print_consinfo() which is exported  through the cpusw.
 *
 *	Currently this is used only by the bus timeout code(although it 
 *	is a general pupose mechanism).
 */
 
/* error information to be dumped on a MEMPKT and ESRPKT type */

struct kn220consinfo_mem_t {
	u_int	cause;	/* from the exception frame */
	u_int	epc;	
	u_int	sr;	/* from the exception frame */
	u_int	badvaddr;
	u_int	pa;	/* physical address */
};

struct kn220consinfo_t {
	int	pkt_type;		/* pkt type */
	int 	qbus;			/* 1 if qbus addr; 0 otherwise */
	struct	kn220consinfo_mem_t pkt;
} kn220consinfo;

struct kn220log_errinfo_t {
	int 	pkt_type;
	u_int	cause;		/* from exception frame */
	u_int	epc;		/* from exception frame */
	u_int	sr;		/* from exception frame */
	u_int	badvaddr;	/* from exception frame */
	u_int	sp;		/* from exception frame */
	struct  esr logesr;	/* from the esr registers */
} kn220log_errinfo;

unsigned int     qmem; 

/*
 * Initialization routine for kn220 processor (MIPSFAIR2).
 */
kn220init()
{
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int iplmask[];
	extern int splm[];
	extern int tick;
	extern int tickadj;
	extern struct cpusw *cpup;
	extern volatile int *system_intr_cnts_type_transl;

	/*
	 * Setup global interrupt type array to use our definitions.
	 */
	system_intr_cnts_type_transl = KN220_interrupt_type;

	/* 
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done before any interrupt is allowed.
	 */
	bcopy((int *)kn220intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn220iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn220splm, splm, (SPLMSIZE) * sizeof(int));


	/* 
	 * Change over from console to kernel trap handling; this
	 * is done by changing the BEV bit in the Status Register.
	 */
	clear_bev();

	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
	 */
	hz	= cpup->HZ;
	tick	= 1000000 / hz;
	tickadj = 240000 / (60 * hz);

	return (0);
}

/*
 * Configuration routine.
 */
int
kn220conf()
{
	extern unsigned cpu_systype;
	extern (*(scb[]))();		/* 1st page of scb vectors */
	extern struct bus *sysbus;
	extern kn220_hardclock();
	
	register int *memid;
	int iopres, B1, B2;
	int slot, mem_mask, memory_descptr;
	extern cnrint(), cnxint();
	extern timeout();
	extern prom_getenv();
	extern kn220_ZSbit();
	extern kn220_nvram_status();
	extern kn220_battery_status();
	extern void bcopy();
	extern void bzero();

	hwconf_init();

	memid = (int *) KN220MIDR;

	/* Fill in the vector in SCB for handling hardclock interrupts. */
	scb[0xc0/4] = kn220_hardclock;	/* hardclock wrapper */

	/* Fill in the vector in SCB for handling console interrupts. */
	scb[0xf8/4] = cnrint;	/* general console receive  interrupt */
	scb[0xfc/4] = cnxint;	/* general console transmit interrupt */

	printf("DECsystem 5500 - system rev %d\n", GETHRDREV(cpu_systype));

	/* 
	 * Read content of IOPRES 
	 * If I/O board isn't present, panic.
	 */
	iopres = *(int *)KN220IOPRES;	
	if(((iopres) & KN220_IOP) == 0)
		panic("kn220conf: DS5500 I/O Board is missing");

	/* 
	 * Init the structure that will point to the correct
	 * hardware dependent functions that the presto code 
	 * depends on.
	 */
	presto_interface0.nvram_status = kn220_nvram_status;
	presto_interface0.nvram_battery_status = kn220_battery_status;
	presto_interface0.nvram_battery_disable = NULL;
	presto_interface0.nvram_battery_enable = NULL;

	/*
	 * presto_interface modified to allow for "programming" of
	 * Presto transfers
	 */
	presto_interface0.nvram_ioreg_read = bcopy ;
	presto_interface0.nvram_ioreg_write = bcopy ;
	presto_interface0.nvram_block_read = bcopy ;
	presto_interface0.nvram_block_write = bcopy ;
	presto_interface0.nvram_ioreg_zero = bzero ;
	presto_interface0.nvram_block_zero = bzero ;

	presto_interface0.nvram_min_ioreg = sizeof(int) ;
	presto_interface0.nvram_ioreg_align = sizeof(int) ;
	presto_interface0.nvram_min_block = sizeof(int) ;
	presto_interface0.nvram_block_align = sizeof(int) ;

	if(prom_getenv("memdescriptor")) {

	/* 
	 * If the memory descriptor is set by the console we use it to
	 * compute which memory board a problem was logged on.  This must
	 * be set by field service when they install memory boards (good luck).
	 * 
	 * Each board get two bits in a byte to recore what is in the slot.
	 * 	00	no memory in slot
	 *	01	32 meg board in slot
	 *	10	64 meg board in slot
	 *	11	128 meg board in slot
	 *
	 * If its not set up at all we log it and ask them to run console test
	 * "T 9A" to establish a variable.  If its there but wrong we will
	 * never know.
	 */
		memory_descptr = xtob(prom_getenv("memdescriptor"));
		for(slot = 0, mem_mask = 3; slot < 4; slot++, (mem_mask << 2)){
			memory_slot[slot] = 32 * MEG * (memory_descptr & mem_mask);
			if((memory_descptr & mem_mask) == 3)
				memory_slot[slot] += (32 * MEG);
		}
	} else
		printf("Run the console test \">>t 9a\" to \nestablish memory size boards in each slot.\n");

	/* 
	 * Now we will check the state of the NVRAM battery
	 * which backs up the presto functions.
	 */
	*memid = KN220BLOAD;                  /* Get old state	*/
	wbflush();
	DELAY(kn220_battdelay);
	iopres = *(int *)KN220IOPRES;         /* Read content of IOPRES */
	B1 = iopres & KN220_BOK;
	*memid = KN220JUMPER & KN220BLOAD;    /* Update state	*/
	wbflush();
	DELAY(kn220_battdelay);
	iopres = *(int *)KN220IOPRES;         /* Read content of IOPRES */
	B2 = iopres & KN220_BOK;
	*memid = 0;                           /* Turn it off battery load */
	wbflush();
	/* check for NVRAM battery low */
	if(B1 == B2) {
		if( B1 != KN220_BOK)
			printf("DS5500 NVRAM lost battery backup\n");
	} else
		printf("DS5500 NVRAM battery backup disabled\n");

	/* Deal with FPU */
	coproc_find();	

	master_cpu = 0;
#if NCPUS > 1
	printf("Master cpu at slot %d.\n", master_cpu);
#endif /* NCPUS > 1 */
	machine_slot[master_cpu].is_cpu = TRUE;
        machine_slot[master_cpu].cpu_type = CPU_TYPE_MIPS;
/* OHMS machine_slot[master_cpu].cpu_subtype = subtype; */
        machine_slot[master_cpu].cpu_subtype = CPU_SUBTYPE_MIPS_R3000a;
	machine_slot[master_cpu].running = TRUE;
        machine_slot[master_cpu].clock_freq = hz;

	/* Turn on interupts */
	splnone();

	/* 
	 * configure the I/O system 
	 * On board devices sit on a virtual bus called the "ibus".
	 * system_bus is global pointer used by loadable code when walking
	 * the hardware topology tree.  It is extern'd via devdriver.h.
	 */
	if((sysbus = get_sys_bus("ibus")) == 0)
		panic("kn220conf: no system bus configured");
	system_bus = sysbus;
	(*sysbus->confl1)(-1, 0, sysbus); 
	(*sysbus->confl2)(-1, 0, sysbus); 

	/* 
	 * Startup the code that clears the Single Bit ECC error
	 * log array every 15 min.
	 */
	timeout(kn220_ZSbit, 0, 15*60*hz);
	return(0);
}

void
kn220config_devices(sbus)
	struct bus *sbus;
{
	extern int nNMSI;
	extern int nNUBA;
	u_int kn220_machineid();

	sbus->bus_type = BUS_IBUS;
	sbus->alive |= ALV_ALIVE;

	/*
	 * This will eventually call unifind which will use 
	 * mapped address space (KSEG2) to touch the Qbus csrs 
	 * It also initialises the i/o data structures to use 
	 * KSEG2 to access the csrs.
	 *
	 * Unifind will set the scb entries to stray catcher 
	 * then load the entries.  Because of this the 
	 * unifind() call needs to be before the 
	 * ib_config_.... calls which load the scb; 
	 * don't want unifind to stomp on the entries 
	 * after they are set.
	 */
	if(nNUBA) {
 		register struct bus *qbus;
		char *sbname = sbus->bus_name;
		int sbnum = sbus->bus_num;

		if((qbus = get_bus("uba", 0, sbname, sbnum)) ||
		   (qbus = get_bus("uba", 0, sbname, -1)) ||
		   (qbus = get_bus("uba", -1, sbname, sbnum)) ||
		   (qbus = get_bus("uba", -1, sbname, -1)) ||
		   (qbus = get_bus("uba", -1, "*", -99))) {

			register struct ub_info qbinfo;
			extern int qmapbase;    /* Qbus map base register */

			uba_hd[0].uba_type = UBAUVII;
			*(int *)PHYS_TO_K1(KN220QMAPBASEREG) = qmapbase;

			printf("Q22 bus\n");
			/* refill in case of wild cards */
			qbus->connect_bus = sbname;
			qbus->connect_num = sbnum;
			qbus->bus_type = BUS_UNIBUS;

			/* set everything up for unifind */
			qbinfo.vubp = PHYS_TO_K1(qbinfo.pubp = KN220QBUSREG);
			qbinfo.pumem = 0x14000000;
			qbinfo.umemsize = 512*8192;
			qbinfo.pdevaddr = 0x10000000;
			qbinfo.haveubasr = 0;
			qbinfo.adpt_num = 0;
			qbinfo.nexus_num = 0;

			if(!(*qbus->confl1)(qbus->bus_type, &qbinfo, qbus))
				panic("kn220config_devices: Qbus confl1");
			if(!(*qbus->confl2)(qbus->bus_type, &qbinfo, qbus))
				panic("kn220config_devices: Qbus confl2");
			qmem = qbinfo.vumem;
			qbus->alive |= ALV_ALIVE;
			conn_bus(sbus, qbus);
		} else
			printf("kn220config_devices: can't configure Qbus");
	} 
	
/* 
 * We need a test nNASC but nNASC is not set up in autoconf_data.c yet
 * NEED FIXING OR fix the alive bit in the SCSI driver.
 */
	/* Configure on-board SCSI */
	if(ib_config_cont(PHYS_TO_K1(KN220_53C94_REG_ADDR), 
			  KN220_53C94_REG_ADDR, 0, "asc", sbus, 
			  (int)scb+SCSI_OFFSET) == 0)
		panic("kn220config_device: can't config on board scsi (asc)");

	/* Configure on-board ethernet */
 	if(ib_config_cont(PHYS_TO_K1(KN220SGEC_ADDR), KN220SGEC_ADDR, 0,
		       "ne", sbus, (int)scb+SGEC_OFFSET) == 0)
		panic("kn220config_device: can't config on board ethernet (ne)");

	/* 
	 * Initialize NVRAM pseudo-driver
	 * 3rd arg is: mapped?; 4th arg is cached?
	 * Use un-mapped, cached NVRAM 
	 */
	if(kn220_usenvram)
		presto_init(NVRAM_ADDR, NVRAM_SIZE, kn220_nvram_mapped,
			    kn220_cache_nvram, kn220_machineid());

	/* Configure on-board DSSI */
	if(nNMSI) {
		extern int nummsi;
		extern int (*msiint0[])();
		struct bus *msibus;
		char *sbname = sbus->bus_name;
		int sbnum = sbus->bus_num;
		
		if((msibus = get_bus("msi", 0, sbname, sbnum)) ||
		   (msibus = get_bus("msi", 0, sbname, -1)) ||
		   (msibus = get_bus("msi", -1, sbname, sbnum)) ||
		   (msibus = get_bus("msi", -1, sbname, -1)) ||
		   (msibus = get_bus("msi", -1, "*", -99))) {
			
			/* refill in case of wild cards */
			msibus->connect_bus = sbname;
			msibus->connect_num = sbnum;
			msibus->bus_type = BUS_MSI;
			msibus->bus_num = nummsi;
			
			/* Stuff the SCB  */
			scb[MSI_OFFSET/4] = msiint0[nummsi];
			
			/* pass in info needed for msi_probe() call  */
			msibus->private[0] = (caddr_t)nummsi;
			msibus->private[1] = 
					(caddr_t)PHYS_TO_K1(KN220MSIREG_ADDR);
			msibus->private[2] = 
					(caddr_t)PHYS_TO_K1(KN220SIIBUF_ADDR);
			if(!(*msibus->confl1)(msibus->bus_type, msibus,msibus))
				panic("kn220config_devices: msibus confl1");
			if(!(*msibus->confl2)(msibus->bus_type, msibus,msibus))
				panic("kn220config_devices: msibus confl2");
			msibus->alive |= ALV_ALIVE;
			conn_bus(sbus, msibus);
			printf("msi%d at %s%d\n", nummsi, sbname, sbnum );
		} else 
			printf("kn220config_devices: can't configure msibus");

		nummsi++;
	}
}

/*
 * Routine to start the 10mS hardclock running.
 * This is simply to start up the 10mS interval timer so
 * that it interrupts every 10mS.  The interval timer
 * on KN220 interrupts the R3000 cpu at hardware line 2 (0 being
 * lowest priority, 5 being highest).  When the interrupt handler
 * reads the vector, the interval timer returns a vector of 0xc0.
 */
kn220startrtclock()
{
	int *tcsr;

	tcsr	= (int *)TCSR;
	*tcsr	= TCSR_IE;
	return(0);
}

/*
 * Routine to stop the 10mS hardclock from interrupting.
 */
kn220stopclocks()
{
	int *tcsr;

	tcsr	= (int *)TCSR;
	*tcsr	= 0;
	return(0);
}

/*
 * The MIPSFAIR2 special register in physical address 0x10084000
 * can be read to determine which error condition occurred.  Format
 * of ISR:
 *	bit<3> set if halt requset is posted,
 *	bit<2> set if powerfail,
 *	bit<1> set if CQBIC posts a memory error.
 *	bit<0> Always reads as 1.
 *
 * The ISR must have the proper bit cleared to enable subsequent interrupts.
 * A halt condition is received.  This is an interrupt
 * handler.
 *
 */
kn220haltintr(ep)
	int *ep;
{
	prom_halt(ep);
}

/*
 *	1. Memory error by CQBIC or MEM_CTL.
 *
 *
 * Note that multiple bits can be set.  For now, arbitrarily handle
 * the error with the following priority (from high to low):
 *	CQBIC & memory error.
 *
 * Parameter:
 *	ep		Pointer to exception frame.
 *
 */
kn220memintr(ep)
	int	*ep;
{

	/* To clear the memory interrupt we need to write to
	 * locoation 1610005c.  The logic on what to do based
	 * on the kind of error is in the general memory error
	 * handler.
	 */

	*(int *)PHYS_TO_K1(CLEAR_MEM_INTR) = 0;
	*(int *)ISR = 0;	/* Acknowledge all error 	*/
	wbflush();
	kn220memerr(ep);	/* Clear ISR after returning here */

}

/*
 * Pending powerfail, currently ULTRIX has no handling
 * strategy on VAX and MIPS.  Further, there is no restart
 * parameter block on MIPS.  
 *
 */
kn220powerfail_intr(ep)
	int *ep;
{
	printf("Power fail..\n");
	DELAY(1000000);	/* Delay 1 second, if we are still alive, continue */
}

/*
 * Interrupts from Qbus level 7 or 10mS clock.  
 * Level 7 is reserved for Qbus real-time devices.  
 * Currently not used in ULTRIX.
 */
kn220hardintr2(ep)
	int *ep;
{
	extern int (*(scb[]))();
	register int vrr;
	int	isr;

	isr = *(int *)ISR;		/* Read content of ISR 		*/
	if(isr & ISR_CERR)  		/* Test for CQBIC mem error	*/
		kn220_qbus_memerr(ep);

	vrr = read_nofault(VRR3);	/* Read interrupt vector 	*/
					/* 0 = passive release		*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep, vrr);	/* Call the interrupt handler 	*/
}

/*
 * Interrupts from NI, DSSI, or Qbus level 6.
 */
kn220hardintr1(ep)
	int *ep;
{
	extern int (*(scb[]))();
	register int vrr;
	int	isr;

	isr = *(int *)ISR;		/* Read content of ISR 		*/
	if(isr & ISR_CERR) 		/* Test for CQBIC mem error	*/
		kn220_qbus_memerr(ep);

	vrr = read_nofault(VRR2);	/* Read interrupt vector 	*/
					/* 0 = passive release		*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep, vrr);	/* Call the interrupt handler 	*/
}


/*
 * Interrupts from the Qbus level 5 and 4,
 * and from the SSC chip (programmable timers, console).
 *
 * VRR0 contains vector for Qbus level 4 & 5, don't read VRR1 for
 * reason I don't understand.
 */
kn220hardintr0(ep)
	int *ep;
{
	extern int (*(scb[]))();
	register int vrr;
	int	isr;			

	isr = *(int *)ISR;		/* Read content of ISR 		*/
	if(isr & ISR_CERR) 		/* Test for CQBIC mem errors	*/
		kn220_qbus_memerr(ep);

	vrr = read_nofault(VRR0);	/* Read interrupt vector 	*/
					/* 0 = passive release		*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep, vrr);	/* Call the interrupt handler 	*/
}



/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 * 
 * A read to non-existent memory will generate an
 * exception which we need to handle.  A write to non-existent 
 * memory location will generate an interupt which we 
 * will need to handle in the interupt routine.
 */
kn220trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr 		*/
	register u_int code;		/* trap code (trap type) 	*/
	u_int sr, cause;		/* status and cause regs 	*/
	int *signo;			/* set if we want to kill process */
{
	unsigned long pa;		/* the physical addr of the error */	
	int epc;			/* the EPC of the error 	*/	
	int vaddr;			/* virt addr of error 		*/
	register struct proc *p;	/* ptr to current proc struct 	*/
	u_long	dser;
	int i;
	int	isr;			/* Content of ISR */
	isr = *(int *)ISR;		/* Read content of ISR		*/

	/*
	 * Log DMA system error register, error address register, ..etc.
	 */
	esr[0].esr_dser = * (u_long *)KN220DSER;
	esr[0].esr_dear = * (u_long *)KN220DEAR;
	esr[0].esr_qbear = * (u_long *)KN220QBEAR;
	esr[0].esr_cbtcr = * (u_long *)KN220CBTCR;
	esr[0].esr_mesr = * (u_long *)KN220MESR;
	esr[0].esr_mear = * (u_long *)KN220MEAR;

	/* Clear the error bits 	*/
	*(u_long *)KN220DSER = 0xc0bd;
	*(u_long *)KN220CBTCR = 0xc0000000 | esr[0].esr_cbtcr;

	/* Compute the physical address. 
	 * pa that is load by the R3000 in the execption frame will
	 * be the default address to use.
	 */
	pa = vatophys(ep[EF_BADVADDR]);


	/* If the problem was a memeory error problem the address will be
	 * found in the MEAR register.
	 * 
	 * Check the high two bit of the MEAR; if one and only one
	 * of them is set the DESR is invalid but the  MESR is valid.
	 * Set the mesr to a valid value to stop us from processing
	 * bad data.
	 *
	 * bit 28 & 29 value meanings
	 *	00	memory space
	 *	11	memory space
	 *	01	I/O space
	 *	10	I/O space
	 *
	 * Also, if bit 1 is set, the address in MEAR is valid.  This bit is
	 * needed by the hardware to indicate what kind of cycle it was
	 * processing ... if the contents of the reg. is OK or not.
	 */
	if((esr[0].esr_mear & GOOD_ADDR) == 0) {	/* DMA Cycle 	*/
		pa = esr[0].esr_mear;
		if((esr[0].esr_mear & KN220_NXM) == 1)
			esr[0].esr_mesr = 0xffffffff;
	} else {					/* R3000 Cycle	*/
		register f29, f28;

		/* need the single bits for xor */
		f29 = (esr[0].esr_mear & BIT29) ? 1 : 0;
		f28 = (esr[0].esr_mear & BIT28) ? 1 : 0;
		if((f29 ^ f28) || (esr[0].esr_mear & KN220_NXM) == 1)
			esr[0].esr_mesr = 0xffffffff;

		if((((esr[0].esr_mear & BIT29) == 1) && 
		    ((esr[0].esr_mear & BIT28) == 0)) || 
		   (((esr[0].esr_mear & BIT29) == 0) && 
		    ((esr[0].esr_mear & BIT28) == 1))) {
			/* I/O Cycle	*/
			/* Zap mesr to make ECC Ok*/
			esr[0].esr_mesr = 0xffffffff;
		} else {
			/* Memory Cycle	*/
			if((esr[0].esr_mear & KN220_NXM) == 1)
				esr[0].esr_mesr = 0xffffffff;
		}
	}

	if(USERMODE(sr)) {
		p = u.u_procp;
		uprintf("pid %d (%s) was killed on bus read error\n",
			p->p_pid, u.u_comm);
	} else {
		/*
		 * Kernel mode errors.
		 * They all panic, its just a matter of what we log
		 * and what panic message we issue.
		 */
		switch (code) {

		case EXC_DBE:		/* Data Bus Error	 */
		case EXC_IBE:		/* Instruction Bus Error */
			/*
			 * Figure out if its a memory parity error
			 *     or a read bus timeout error
			 */

			/* If the bits in the dser are set its a CQBIC
			 * reported problem.
			 */
			if(isr & ISR_CERR) 
				kn220_qbus_memerr(ep);
			else {	
				/* A memory controller reported
				 * problem.  Trap is caused on multi bit errors
				 * on read or bus timeouts.
				 * Look for Multi bit ECC errors.  If it is a multi bit error
				 * bits (HME or LME) will be zero in the memory error syndrome
				 * reister.
				 */
				if((( esr[0].esr_mesr & KN220_LME ) == 0) ||
				   (( esr[0].esr_mesr & KN220_HME) == 0)) {
					/* Multi bit ECC error */
					kn220logmempkt(EL_PRISEVERE, ep, pa);
					kn220consprint(MEMPKT, ep, pa, 0);
					panic("memory read error in kernel mode");
				}

				/* To get here it was not a parity error
				 * so it must have been a memory timeout.
				 */
 				kn220logesrpkt(ep, esr, EL_PRISEVERE);
				kn220consprint(ESRPKT, ep, pa, 0);
				panic("read bus timeout");
			}
			break;

		case EXC_CPU:		/* CoProcessor Unusable */
 			kn220logesrpkt(ep, esr, EL_PRISEVERE);
			kn220consprint(ESRPKT, ep, 0, 0);
			panic("coprocessor unusable");
			break;

		case EXC_RADE:		/* Read Address Error	*/
		case EXC_WADE:		/* Write Address Error 	*/
 			kn220logesrpkt(ep, esr, EL_PRISEVERE);
			kn220consprint(ESRPKT, ep, 0, 0);
			panic("unaligned access");
			break;

		case SEXC_SEGV: /* This can be because we were in TLBMIS code
				 * and to a SEGV.  That causes us to get to this
				 * routine with a SEXC_SEGV and I want a better
				 * panic message than "trap".
				 */
			kn220logesrpkt(ep, esr, EL_PRISEVERE);
			kn220consprint(ESRPKT, ep, 0, 0);
			panic("segmentation violation");
			break;

		default:
			kn220logesrpkt(ep, esr, EL_PRISEVERE);
			kn220consprint(ESRPKT, ep, 0, 0);
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

#define KN220_LOG_ESRPKT(elrp, cause, epc, sr, badva, sp)		\
	elrp->el_body.elesr.elesr.el_esr5500.esr_cause  = cause;	\
	elrp->el_body.elesr.elesr.el_esr5500.esr_epc    = epc;		\
	elrp->el_body.elesr.elesr.el_esr5500.esr_status = sr;		\
	elrp->el_body.elesr.elesr.el_esr5500.esr_badva  = badva;	\
	elrp->el_body.elesr.elesr.el_esr5500.esr_sp 	= sp;		\

/*
 *	Copy the information from the error registers to the 
 *	the error log buffer area.
 */

kn220copy_esrinfo(elrp, esrp)
	struct el_rec *elrp;
	struct esr *esrp;	/* pointer to esrs */
{
	elrp->el_body.elesr.elesr.el_esr5500.esr_dser  = esrp->esr_dser;
	elrp->el_body.elesr.elesr.el_esr5500.esr_qbear = esrp->esr_qbear;
	elrp->el_body.elesr.elesr.el_esr5500.esr_dear  = esrp->esr_dear;
	elrp->el_body.elesr.elesr.el_esr5500.esr_cbtcr = esrp->esr_cbtcr;
	elrp->el_body.elesr.elesr.el_esr5500.esr_isr   = esrp->esr_isr;
	elrp->el_body.elesr.elesr.el_esr5500.esr_mser  = esrp->esr_mesr;
	elrp->el_body.elesr.elesr.el_esr5500.esr_mear  = esrp->esr_mear;
	elrp->el_body.elesr.elesr.el_esr5500.esr_ipcr  = esrp->esr_ipcr;
}


/*
 * Log Error & Status Registers to the error log buffer.
 */
kn220logesrpkt(ep, ptr, priority)
	register u_int *ep;	/* exception frame ptr */
	struct esr *ptr;	/* pointer to esr */
	int priority;		/* for pkt priority */
{
	struct el_rec *elrp;

	if((elrp = ealloc(sizeof(struct el_esr), priority)) != NULL) {
		LSUBID(elrp, ELCT_MCK, ELESR_5500, EL_UNDEF, EL_UNDEF,
		       EL_UNDEF,EL_UNDEF);
		
		KN220_LOG_ESRPKT(elrp, ep[EF_CAUSE], ep[EF_EPC], 
				 ep[EF_SR], ep[EF_BADVADDR], ep[EF_SP]);
		kn220copy_esrinfo(elrp, ptr);
		EVALID(elrp);
	}
}

/*
 * kn220_log_errinfo:
 *	Exported through the cpusw and used to log error information to 
 *	the error log buffer.
 */

kn220_log_errinfo(p)
	struct kn220log_errinfo_t *p;
{
	struct el_rec *elrp;

	switch (p->pkt_type) {
	
	case ESRPKT:
		if((elrp =ealloc(sizeof(struct el_esr),EL_PRISEVERE))!= NULL) {
			LSUBID(elrp, ELCT_MCK, ELESR_5500, EL_UNDEF,
			       EL_UNDEF,EL_UNDEF,EL_UNDEF);
			KN220_LOG_ESRPKT(elrp, p->cause, p->epc, 
					 p->sr, p->badvaddr, p->sp);
			kn220copy_esrinfo(elrp, (struct esr *) &(p->logesr));
			EVALID(elrp);
		}
		break;
		
	default:
		printf("bad pkt type");
		return;
	}
}
	
	

/*
 * Log a memory error packet, so uerf can find it as a main memory error.
 * Determine the type of memory error by reading MEMCSR16.
 *
 * Side effect: clear error bits in MEMCSR16.
 */
kn220logmempkt(priority, ep, pa)
	int priority;		/* pkt priority: panic: severe; else: high */
	register u_int *ep;	/* exception frame ptr */
	int pa;			/* physical addr where memory err occured */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;
	int slot, sum_addr;
	
	/* compute which memory board the pa is on.
	 * Walk through each of the four slots and find which slot the PA
	 * is on.  Memory_slot[slot] has the size of the memory board so
	 * we loop through each until the pa boundry is found.
	 */
	slot = 0;
	for(; slot < 4; slot++)	{
		if(memory_slot[slot] != 0) {
			if((sum_addr + memory_slot[slot]) < pa) {
				sum_addr = sum_addr + memory_slot[slot];
			} else {
				/* the sum of the address exceeds the pa so
				 * we have found the board!
				 */
				break;
			}
		} else {
			/* An empty slot so we have reached the end of memory
			 * and can stop now.
			 */
			break;
		}
	}
	slot++;	/* because we want to start counting from 1 when we log it */
	
	/* Allocate an error log packet. */
	if((elrp = ealloc(EL_MEMSIZE, priority)) != NULL) {
		LSUBID(elrp, ELCT_MEM, EL_UNDEF, ELMCNTR_5500,
		       EL_UNDEF,EL_UNDEF,EL_UNDEF);
		
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		mrp->elmemerr.numerr = 1;
		
		/* Check for MULTI bit errors in both the 
		 * High and Low ECC checks.  Bits are zero
		 * if an error was detected.
		 * Address of location in error is in esr_mear
		 */
		if( ((esr[0].esr_mesr & KN220_HME) == 0) || 
		   ((esr[0].esr_mesr & KN220_LME) == 0)) {
			mrp->elmemerr.type = ELMETYP_RDS;
		} else 
			/* Check for SINGLE bit errors in both the 
			 * High and Low ECC checks.  Bits are zero
			 * if an error was detected.
			 * Address of location in error is in esr_mear
			 */
			if( ((esr[0].esr_mesr & KN220_HER) == 0) || 
			   ((esr[0].esr_mesr & KN220_LER) == 0)) {
				mrp->elmemerr.type = ELMETYP_CRD;
			} else
				/* Check for PARITY error
				 */
				if(esr[0].esr_dser & DSER_PARITY) {
					mrp->elmemerr.type = ELMETYP_PAR;
				} else	
					/* Check Non-existant memory access */
					if(esr[0].esr_dser & DSER_NXM) {
						mrp->elmemerr.type = ELMETYP_NXM;
					} else {
						mrp->elmemerr.type = 0;
					}
		mrp->elmemerr.regs[0] = esr[0].esr_mesr;
		mrp->elmemerr.regs[1] = pa;
		mrp->elmemerr.regs[2] = ep[EF_EPC];
		mrp->elmemerr.regs[3] = ep[EF_BADVADDR];
		mrp->elmemerr.regs[4] = slot;
		/* 
		 * We are done...validate the error log packet.  This puts
		 * it back on the available list and closes the error log
		 * task.
		 */
		EVALID(elrp);
	}
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 * Uses kn220_print_consinfo to do the actual printing.
 *
 */
kn220consprint(pkt, ep, pa, qbus)
	int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
	register u_int *ep;	/* exception frame ptr */
	unsigned pa;		/* For MEMPKT: physical addr of error */ 
	int	qbus;		/* 1 if pa is a qbus address, 0 otherwise */
{

	struct kn220consinfo_t p;

	p.pkt_type 	= pkt;
	p.pkt.cause	= ep[EF_CAUSE];
	p.pkt.epc	= ep[EF_EPC];
	p.pkt.sr	= ep[EF_SR];
	p.pkt.badvaddr	= ep[EF_BADVADDR];
	p.pkt.pa	= pa;
	p.qbus		= qbus;
	kn220_print_consinfo(&p);
}


/*
 *	This routine is similar to kn220consprint().
 *	This is exported through the cpusw structure. 
 *
 * 	N.B.: side-effect.
 *	If console is a graphics device, printstate is changed  
 *	to force kernel printfs directly to the screen.
 */

kn220_print_consinfo(p)
	struct kn220consinfo_t *p;
{

	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 * Note: DS_5500 currently does not support qdss, 
	 * but just in case...
	 */
	printstate |= PANICPRINT;
	switch (p->pkt_type) {
	case ESRPKT:
		printf("\nException condition\n");
		break;
	case MEMPKT:
 		printf("\nMemory Error\n");
		break;
	default:
 		printf("bad consprint\n");
		return;
	}
	printf("\tCause reg\t= 0x%x\n", p->pkt.cause);
	printf("\tException PC\t= 0x%x\n", p->pkt.epc);
	printf("\tStatus reg\t= 0x%x\n", p->pkt.sr);
	printf("\tBad virt addr\t= 0x%x\n", p->pkt.badvaddr);

	if(p->pkt.pa) {
		if(p->qbus) 
			printf("\tQ22 Bus physical address of error: 0x%x\n",
				p->pkt.pa);
		else 
			printf("\tPhysical address of error: 0x%x\n",
				p->pkt.pa);
	}
	return;
}

/*
 * Log memory errors in kernel buffer
 * We get memory interupts for ECC errors on writes
 * but single bit errors; data is fixed which makes single bit errors
 * generated informational only.
 */
kn220memerr(ep)
	register u_int *ep;	/* exception frame ptr */
{
	register struct proc *p;	/* ptr to current proc struct 	*/
	u_long	dser;
	u_long	pa;	/* Physical address 				*/
	int	isr;	/* Content of ISR 				*/
	int     i;	/* counter for Single bit array scan 		*/
        struct kn220consinfo_t *pcons;    /* pointer to console info */
        struct kn220log_errinfo_t *plog;  /* pointer to log info */
	
	/* Read content of Interrupt Status Register */
	isr = *(int *)ISR;
	
	/*
	 * Log DMA system error register, error address register, ..etc.
	 */
	esr[0].esr_dser  = * (u_long *)KN220DSER;  /* DMA Sys error register*/
	esr[0].esr_qbear = * (u_long *)KN220QBEAR; /* Qbus error addr reg.  */
	esr[0].esr_dear  = * (u_long *)KN220DEAR;  /* DMA error addr reg.   */
	esr[0].esr_cbtcr = * (u_long *)KN220CBTCR; /* CDAL Bus Timeout	    */
	esr[0].esr_mesr  = * (u_long *)KN220MESR;  /* Memory Error Syndrom Reg.*/
	esr[0].esr_mear  = * (u_long *)KN220MEAR;  /* Memory Error Address Reg.*/
	esr[0].esr_isr   = isr;			   /* Interupt Status Reg.  */
	
	/* Clear the error bits  */
	
	/* Compute the physical address. 
	 * pa that is load by the R3000 in the execption frame will
	 * be the default address to use.
	 */
	pa = vatophys(ep[EF_BADVADDR]);
	
	
	/* If the problem was a memeory error problem the address will be
	 * found in the MEAR register.
	 * 
	 * Check the high two bit of the MEAR; if one and only one
	 * of them is set the DESR is invalid but the  MESR is valid.
	 * Set the mesr to a valid value to stop us from processing
	 * bad data.
	 *
	 * bit 28 & 29 value meanings
	 *	00	memory space
	 *	11	memory space
	 *	01	I/O space
	 *	10	I/O space
	 *
	 * Also, if bit 1 is set, the address in MEAR is valid.  This bit is
	 * needed by the hardware to indicate what kind of cycle it was
	 * processing ... if the contents of the reg. is OK or not.
	 */
	if((esr[0].esr_mear & GOOD_ADDR) == 0) {
		/* DMA Cycle 	*/
		pa = esr[0].esr_mear;
		if((esr[0].esr_mear & KN220_NXM) == 1) {
			esr[0].esr_mesr = 0xffffffff;
		}
	} else {					/* R3000 Cycle	*/
		pa = esr[0].esr_mear;
		if((((esr[0].esr_mear & BIT29) == 1) &&
		    ((esr[0].esr_mear & BIT28) == 0)) ||
		   (((esr[0].esr_mear & BIT29) == 0) &&
		    ((esr[0].esr_mear & BIT28) == 1))) {	
			/* I/O Cycle	*/
			/* Zap mesr to make ECC Ok*/
			esr[0].esr_mesr = 0xffffffff;
		} else {
			/* Memory Cycle	*/
			if((esr[0].esr_mear & KN220_NXM) == 1) {
				esr[0].esr_mesr = 0xffffffff;
			}
		}
	}

	/* Its a Kernel space error
	 *
	 * Look in the Memory Error Syndrome Register and if the error
	 * is a single bit error (LER or HER bit set to zero) then
	 * we will not panic.
	 */
	if( (( esr[0].esr_mesr & KN220_LER ) == 0) ||
	   (( esr[0].esr_mesr & KN220_HER) == 0)) {

		/* Single bit error case-
		 * Look in the Sbit_error array to see if the
		 * error has hapened in the last 15 min.  If it
		 * has the address will be in here and we 
		 * will not log the error.
		 */
		for(i = 0; i < SBIT_SIZE; i++) {
			
			/* Return here because it was only a single bit
			 * memory error and we don't want to panic the system.
			 * But first check for multri bit first just 
			 * in case both single and multi were 
			 * set (no a valid case but who knows)
			 */
			if(Sbit_error[i] == esr[0].esr_mear)
				if( (( esr[0].esr_mesr & KN220_LME ) == 1) &&
				(( esr[0].esr_mesr & KN220_HME) == 1)) 
					return;
		}
		
		/* The address is not there so load it and go on
		 * to log the error.
		 */
		for(i = 0; i < SBIT_SIZE; i++) {
			if((Sbit_error[i] != 0) && (i != SBIT_SIZE))
				continue;
			if(i == (SBIT_SIZE - 1))
				break;
			Sbit_error[i] = esr[0].esr_mear;
 			kn220logmempkt(EL_PRISEVERE, ep, pa);
		}
		
		/* Return here because it was only a single bit
		 * memory error and we don't want to panic the system.
		 * But first check for multri bit first just 
		 * in case both single and multi were 
		 * set (no a valid case but who knows)
		 */
		if( (( esr[0].esr_mesr & KN220_LME ) == 1) &&
		   (( esr[0].esr_mesr & KN220_HME) == 1)) 
			return;
	}
	

	/* Look for Multi bit ECC errors.  If it is a 
	 * multi bit error bits (HME or LME) will be 
	 * zero in the memory error syndrome reister.
	 */
	if( (( esr[0].esr_mesr & KN220_LME ) == 0) ||
	   (( esr[0].esr_mesr & KN220_HME) == 0)) {
		/* MULTI BIT error */
 		kn220logesrpkt(ep, esr, EL_PRISEVERE);
		kn220consprint(ESRPKT, ep, pa, 0);
		panic("Memory multi bit parity error");
	}

	/*
	 * if the low bit in the address is high its a NXM
	 */
	if( (esr[0].esr_mear & KN220_NXM) == 1 ){
		kn220logesrpkt(ep, esr, EL_PRISEVERE);
		kn220consprint(MEMPKT, ep, pa, 0);
		panic("nonexistant memory access");
	}
}

/* Every 15 min. we want to "zap" the array that keeps track
 * of single bit ECC memory errors.  We do this so that a location
 * which generates a single bit error does not flood the error log
 * if it stays in memory and get hit many, many times.  This array
 * tries to limit the amount entries.  After this array is zeroed
 * any new single bit errors will get logged.
 */
kn220_ZSbit()
{
	int i;
	extern timeout();

	for(i = 0; i < SBIT_SIZE; i++)
		Sbit_error[i] = 0;

	/* Reschedule this routine to run in 15 min. */
	timeout(kn220_ZSbit, 0, 15*60*hz);
}

/* In the DS_5500 we need to write to a memory location
 * to clear memory interrupts.  This routine seems the best
 * way to do this?
 */
int 
kn220badaddr(addr, len, ptr)
	caddr_t addr;
	int len;
	struct bus_ctlr_common *ptr;       /* dummy on mips */
{
	register int foo, s;	
	
#ifdef lint
	len = len;
#endif lint

	s = spl7();
	*(int *)ISR = 0;
	foo = bbadaddr(addr,len, ptr);
	*(int *)PHYS_TO_K1(CLEAR_MEM_INTR) = 0;
	wbflush();
	splx(s);
	return(foo);
}

/* The CQBIC can generate a memory type error interrupts.  This routine checks
 * for the error and will crash the system.  This routine is called from interrupt
 * if and only if the ISR bit 1 is set.  Bit 1 of the ISR indicates a Qbus
 * posted memory error.  The nofault code must clear this bit.
 */
kn220_qbus_memerr(ep)
	register u_int *ep;
{
	u_long pa;
	int	isr;	/* Content of ISR 				*/
	int 	dser;

	/* Read content of Interrupt Status Register */
	isr = *(int *)ISR;
	*(int *)ISR = 0;			   /* Acknowledge all error */
	wbflush();

	/*
	 * Log DMA system error register, error address register, ..etc.
	 */
	esr[0].esr_dser  = * (u_long *)KN220DSER;  /* DMA Sys error register*/
	esr[0].esr_qbear = * (u_long *)KN220QBEAR; /* Qbus error addr reg.  */
	esr[0].esr_dear  = * (u_long *)KN220DEAR;  /* DMA error addr reg.   */
	esr[0].esr_cbtcr = * (u_long *)KN220CBTCR; /* CDAL Bus Timeout	    */
	esr[0].esr_mesr  = * (u_long *)KN220MESR;  /* Memory Error Syndrom Reg.*/
	esr[0].esr_mear  = * (u_long *)KN220MEAR;  /* Memory Error Address Reg.*/
	esr[0].esr_isr   = isr;			   /* Interupt Status Reg.  */

	/* Clear the error bits  */
	*(u_long *)KN220DSER = 0xc0bd;
	*(u_long *)KN220CBTCR = 0xc0000000 | esr[0].esr_cbtcr;

	/* If the CQBIC reports nogrant or nxm then the address in 
	 * the dear is valid
	 */
	if(isr & ISR_CERR) {
		/* if the CQBIC reports a Q22 bus parity error OR bus time out the
		 * Q22 bus the address is in the QBEAR (but left shift it 9).
		 */
		if( (esr[0].esr_dser & DSER_PARITY ) || 
		   ( esr[0].esr_dser  & DSER_MEMTO ))
			pa = esr[0].esr_qbear << 9;

		if( (esr[0].esr_dser & DSER_NOGRNT ) || 
		   ( esr[0].esr_dser  & DSER_NXM ))
			pa = esr[0].esr_dear << 9;

		/* Log a error log packet */
 		kn220logmempkt(EL_PRISEVERE, ep, pa);
		kn220consprint(MEMPKT, ep, pa, ITS_A_QBUS);

		/*
		 * Q22 bus write cycle timeout after 10uS,
		 * or Q22 bus parity error.
		 * QBEAR contains Qbus physical address.
		 */
		dser = esr[0].esr_dser;

		if( dser & DSER_PARITY ) 
			panic("Q22 bus memory parity error");

		if( dser & DSER_MEMTO ) 
			panic("Read bus timeout");

		if((dser & DSER_NXM) || (dser & DSER_MME)) {

			/*
			 * DMA transfer to non-existent main 
			 * memory location.
			*/
			panic("DMA memory error");	
		}

	       if(dser & DSER_NOGRNT) {
			/*
			 * Q22 Bus does not return a bus grant
			 * within the 10ms of the Bus request for a CPU
			 * read or write cycle.
			 */
			panic("Q22 Bus Grant Timeout");
		}

		/* Default case */
		panic("Qbus Memory error");

	/* END of CQBIC error code 	*/
	}
}

kn220_battery_status()
{
	register int i, *memid;
	int B1, B2;
	int iopres;

	/*
	 * This routine is called by presto software
	 * periodically. 
	 *
	 * See h/presto.h for comments on the data structures involved. 
	 * 
	 * For DS5500 the NVRAM battery is the primary
	 * and the H3602 battery is the secondary.
	 * Both batteries must be operational for normal Prestoserve
	 * operation.
	 *
	 * Get the battery status for the primary on the fly, the console
	 * gives me the secondary status as it was at boot time.
	 */

	memid =(int *) KN220MIDR;
  	nvram_batteries0.nv_nbatteries = 2;	   /* two batteries */
	nvram_batteries0.nv_minimum_ok = 2;	   /* both must be good */
	nvram_batteries0.nv_primary_mandatory = 1; /* primary must be good */
	nvram_batteries0.nv_test_retries = 3;	   /* call this routine 3 times
						    * for each "test" */

	ssc_ptr = (struct ssc_regs *)PHYS_TO_K1(DEFAULT_SSC_PHYS);

	*memid = KN220BLOAD;		/* Get old state */
	wbflush();
	DELAY(kn220_battdelay);

	iopres = *(int *)KN220IOPRES;	/* Read content of IOPRES */
	B1 = iopres & KN220_BOK;
	*memid = KN220JUMPER & KN220BLOAD;	/* Update state	*/
	wbflush();
	DELAY(kn220_battdelay);

	iopres = *(int *)KN220IOPRES;	/* Read content of IOPRES */
	B2 = iopres & KN220_BOK;

  	nvram_batteries0.nv_status[0] = BATT_OK; /* assume OK */
	*memid = 0;			/* Turn it off battery load*/
	wbflush();
	if(B1 == B2) {			/* check for NVRAM battery low */
		if(B1 != KN220_BOK)
			/* "DS5500 NVRAM battery backup disabled" */
			/* battery will not back RAM */
		  	nvram_batteries0.nv_status[0] = BATT_NONE; 
	} else {
		/* "DS5500 NVRAM lost battery backup" */
		/* low battery */
  		nvram_batteries0.nv_status[0] = BATT_ENABLED;
	}

	/* Bit 31 high is low H3602 
	 * battery, zero is battery OK
	 */
  	if(ssc_ptr->ssc_ssccr & 0x80000000)	
	  	nvram_batteries0.nv_status[1] = BATT_ENABLED;
	else
  		nvram_batteries0.nv_status[1] = BATT_OK;
	return(0);
}

int kn220_fakestatus = 0; /* set to 1 for old, bad ROMs */

int
kn220_nvram_status()
{

	int i;

	/* Status of NVRAM diagnostics 
	* NVRAM_BAD	0	 either read/write or read-only diagnostics
	*			 	run unsuccessfully 
	* NVRAM_RDWR	1	 read/write diagnostics run successfully 
	* NVRAM_RDONLY	2	 read-only diagnostics run successfully 

	* If the reply is NVRAM_RDONLY, then presto will attempt recovery, but
	* not allow normal operation. If the reply is NVRAM_BAD, presto will
	* offer the console operator a "halt or continue" choice.
	*
	*/

	if(kn220_fakestatus)
		return(NVRAM_RDWR); /* HACK UNTIL THE CONSOLE ROMS ARE UP TO DATE */
	else {
		i = NVRAMSTATUS;
		return (i);
	}

}

#include <sys/socket.h>
#include <net/if.h>
/*
 * Get the hardware E_net address for on-board ne0 and use it
 * to build a poor man's version of a unique processor ID.
 */
u_int
kn220_machineid()
{
	register struct ifnet *ifp = ifnet;
	struct ifdevea ifr;
	u_int i = 0;
	int error;

	if(ifnet == NULL) {
		printf("kn220: ifnet NULL\n");
		return (i);
	}

	while (ifp != NULL) {
		if(ifp->if_name[0] == 'n' && ifp->if_name[1] == 'e' &&
		    ifp->if_unit == 0) {	/* found ne0 */
			error = (*ifp->if_ioctl)(ifp, SIOCRPHYSADDR, &ifr);
			if(error)
				return (i);
			i = (u_int)ifr.default_pa[2];
			i = i << 8;
			i += (u_int)ifr.default_pa[3];
			i = i << 8;
			i += (u_int)ifr.default_pa[4];
			i = i << 8;
			i += (u_int)ifr.default_pa[5];
			return (i);
		}
		ifp = ifp->if_next;
	}
	return (i);
}

