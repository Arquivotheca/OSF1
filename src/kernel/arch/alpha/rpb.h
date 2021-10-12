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
 *	"@(#)rpb.h	9.2	(ULTRIX/OSF)	10/30/91"
 */
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification History: dec/machine/alpha/rpb.h
 *
 * 14-Oct-91 -- ald
 *	Fixed structures to match console revision 3.2
 *
 * 26-Jun-91 -- jmartin
 *	Fix RPB_END macro to conform to current console spec.
 *
 * 23-Aug-90 -- afd
 *	Created this file with definition of Alpha HWRPB
 *
 */

#ifndef _RPB_H_
#define _RPB_H_

/*
 * Defines for the architected startup addresses.
 */

#define HWRPB_ADDR	0x10000000	/* 256 MB */
#define BOOT_ADDR	0x20000000	/* 512 MB */
#define PGTBL_ADDR	0x40000000	/*   1 GB */

/*
 * Values for the "haltcode" field in the per-cpu portion of the HWRPB
 *
 * Bit defines for the "sysvar" field in the HWRPB.
 * Each platform has different values for SYSBOARD and IOBOARD bits.
 */
#define HALT_PWRUP	0		/* power up */
#define HALT_OPR	1		/* operator issued halt cmd */
#define HALT_KSTK	2		/* kernel stack not valid */
#define HALT_SCBB	3		/* invalid SCBB */
#define HALT_PTBR	4		/* invalid PTBR */
#define HALT_EXE	5		/* kernel executed halt instruction */
#define HALT_DBLE	6		/* double error abort */

/*
 * Bit defines for the "state" field in the per-cpu portion of the HWRPB
 */
#define STATE_BIP	0x00000001	/* bootstrap in progress */
#define STATE_RC	0x00000002	/* restart capable */
#define STATE_PA	0x00000004	/* processor available to OS */
#define STATE_PP	0x00000008	/* processor present */
#define STATE_OH	0x00000010	/* operator halted */
#define STATE_CV	0x00000020	/* context valid */
#define STATE_PV	0x00000040	/* PALcode valid */
#define STATE_PMV	0x00000080	/* PALcode memory valid */
#define STATE_PL	0x00000100	/* PALcode loaded */
#define STATE_HALT_MASK	0x00ff0000	/* Mask for Halt Requested field */
#define STATE_DEFAULT	0x00000000	/* Default (no specific action) */
#define STATE_SVRS_TERM	0x00010000	/* SAVE_TERM/RESTORE_TERM Exit */
#define STATE_COLD_BOOT	0x00020000	/* Cold Bootstrap Requested */
#define STATE_WARM_BOOT	0x00030000	/* Warm Bootstrap Requested */
#define STATE_HALT	0x00040000	/* Remain halted (no restart) */


#define SV_PF_RSVD	0x00000000	/* RESERVED */
#define SV_RESERVED	0x00000000	/* All STS bits; zero for backward compat. */
#define SV_MPCAP	0x00000001	/* MP capable */
#define SV_PF_UNITED	0x00000020	/* United */
#define SV_PF_SEPARATE	0x00000040	/* Separate */
#define SV_PF_FULLBB	0x00000060	/* Full battery backup */
#define SV_POWERFAIL	0x000000e0	/* Powerfail implementation */
#define SV_PF_RESTART	0x00000100	/* Powerfail restart */

#define SV_GRAPHICS	0x00000200	/* Embedded graphics processor */

#define SV_STS_MASK	0x0000fc00	/* STS bits - system and I/O board   */
#define SV_SANDPIPER	0x00000400	/* others define system platforms.   */
#define SV_FLAMINGO	0x00000800	/* STS BIT SETTINGS */
#define SV_HOTPINK	0x00000c00	/* STS BIT SETTINGS */
#define SV_FLAMINGOPLUS	0x00001000	/* STS BIT SETTINGS */
#define SV_ULTRA	0x00001400	/* STS BIT SETTINGS */
#define SV_SANDPLUS	0x00001800	/* STS BIT SETTINGS */
#define SV_SANDPIPER45	0x00001c00	/* STS BIT SETTINGS */
#define SV_FLAMINGO45	0x00002000	/* STS BIT SETTINGS */


/*
 * Values for the "console type" field in the CTB portion of the HWRPB
 */
#define CONS_NONE	0		/* no console present */
#define CONS_SRVC	1		/* console is service processor */
#define CONS_DZ		2		/* console is dz/dl VT device */
#define CONS_GRPH	3		/* cons is graphics dev w/ dz/dl keybd*/
#define CONS_REM	4		/* cons is remote, protocal enet/MOP */

/*
 * PALcode variants that we're interested in.
 * Used as indices into the "palrev_avail" array in the per-cpu portion
 * of the HWRPB.
 */
#define PALvar_reserved	0
#define PALvar_OpenVMS	1
#define PALvar_OSF1	2

#include <sys/types.h>
/*
 * The Alpha restart parameter block, which is a page or 2 in low memory
 */
#ifdef __LANGUAGE_C__
struct rpb {
    struct rpb *rpb_selfref;	/* 000: physical self-reference */
    long   rpb_string;		/* 008: contains string "HWRPB" */
    long   rpb_vers;		/* 010: HWRPB version number */
    u_long rpb_size;		/* 018: bytes in RPB perCPU CTB CRB MEMDSC */
    u_long rpb_cpuid;		/* 020: primary cpu id */
    u_long rpb_pagesize;	/* 028: page size in bytes */
    u_long rpb_addrbits;	/* 030: number of phys addr bits */
    u_long rpb_maxasn;		/* 038: max valid ASN */
    char   rpb_ssn[16];		/* 040: system serial num: 10 ascii chars */
    u_long rpb_systype;		/* 050: system type */
    long   rpb_sysvar;		/* 058: system variation */
    long   rpb_sysrev;		/* 060: system revision */
    u_long rpb_clock;		/* 068: scaled interval clock intr freq */
    u_long rpb_counter;		/* 070: cycle counter frequency */
    u_long rpb_vptb;		/* 078: virtual page table base */
    long   rpb_res1;		/* 080: reserved */
    u_long rpb_trans_off;	/* 088: offset to translation buffer hint */
    u_long rpb_numprocs;	/* 090: number of processor slots */
    u_long rpb_slotsize;	/* 098: per-cpu slot size */
    u_long rpb_percpu_off;	/* 0A0: offset to per_cpu slots */
    u_long rpb_num_ctb;		/* 0A8: number of CTBs */
    u_long rpb_ctb_size;	/* 0B0: bytes in largest CTB */
    u_long rpb_ctb_off;		/* 0B8: offset to CTB (cons term block) */
    u_long rpb_crb_off;		/* 0C0: offset to CRB (cons routine block) */
    u_long rpb_mdt_off;		/* 0C8: offset to memory descriptor table */
    u_long rpb_config_off;	/* 0D0: offset to config data block */
    u_long rpb_fru_off;		/* 0D8: offset to FRU table */
    void   (*rpb_saveterm)();	/* 0E0: virt addr of save term routine */
    long   rpb_saveterm_pv;	/* 0E8: proc value for save term routine */
    void   (*rpb_rstrterm)();	/* 0F0: virt addr of restore term routine */
    long   rpb_rstrterm_pv;	/* 0F8: proc value for restore term routine */
    void   (*rpb_restart)();	/* 100: virt addr of CPU restart routine */
    long   rpb_restart_pv;	/* 108: proc value for CPU restart routine */
    long   rpb_software;	/* 110: used to determine presence of kdebug */
    long   rpb_hardware;	/* 118: reserved for hardware */
    long   rpb_checksum;	/* 120: checksum of prior entries in rpb */
    long   rpb_rxrdy;		/* 128: receive ready bitmask */
    long   rpb_txrdy;		/* 130: transmit ready bitmask */
    u_long rpb_dsr_off;		/* 138: Dynamic System Recog. offset */
};
#endif /* __LANGUAGE_C__ */

#define rpb_kdebug rpb_software

#define OSF_HWRPB_ADDR	((vm_offset_t)(-1L << 23))

#ifdef __LANGUAGE_C__

/*
 * This is the format for the boot/restart HWPCB.  It must match the
 * initial fields of the pcb structure as defined in pcb.h, but must
 * additionally contain the appropriate amount of padding to line up
 * with formats used by other palcode types.
 */
struct bootpcb {
    long   rpb_ksp;		/* 000: kernel stack pointer */
    long   rpb_usp;		/* 008: user stack pointer */
    long   rpb_ptbr;		/* 010: page table base register */
    int    rpb_cc;		/* 018: cycle counter */
    int    rpb_asn;		/* 01C: address space number */
    long   rpb_proc_uniq;	/* 020: proc/thread unique value */
    long   rpb_fen;		/* 028: floating point enable */
    long   rpb_palscr[2];	/* 030: pal scratch area */
    long   rpb_pcbpad[8];	/* 040: padding for fixed size */
};

/*
 * The per-cpu portion of the Alpha HWRPB.
 * Note that the main portion of the HWRPB is of variable size,
 * hence this must be a separate structure.
 *
 */
struct rpb_percpu {
    struct bootpcb rpb_pcb;	/* 000: boot/restart HWPCB */
    long   rpb_state;		/* 080: per-cpu state bits */
    long   rpb_palmem;		/* 088: palcode memory length */
    long   rpb_palscratch;	/* 090: palcode scratch length */
    long   rpb_palmem_addr;	/* 098: phys addr of palcode mem space */
    long   rpb_palscratch_addr;	/* 0A0: phys addr of palcode scratch space */
    long   rpb_palrev;		/* 0A8: PALcode rev required */
    long   rpb_proctype;	/* 0B0: processor type */
    long   rpb_procvar;		/* 0B8: processor variation */
    long   rpb_procrev;		/* 0C0: processor revision */
    char   rpb_procsn[16];	/* 0C8: proc serial num: 10 ascii chars */
    long   rpb_logout;		/* 0D8: phys addr of logout area */
    long   rpb_logout_len;	/* 0E0: length in bytes of logout area */
    long   rpb_haltpb;		/* 0E8: halt pcb base */
    long   rpb_haltpc;		/* 0F0: halt pc */
    long   rpb_haltps;		/* 0F8: halt ps */
    long   rpb_haltal;		/* 100: halt arg list (R25) */
    long   rpb_haltra;		/* 108: halt return address (R26) */
    long   rpb_haltpv;		/* 110: halt procedure value (R27) */
    long   rpb_haltcode;	/* 118: reason for halt */
    long   rpb_software;	/* 120: for software */
    long   rpb_buffer[21];	/* 128: inter-console buffer area */
    long   rpb_palrev_avail[16];/* 1D0: PALcode revs available */
    long   rpb_pcrsvd[6];	/* 250: reserved for arch use */
/* the dump stack grows from the end of the rpb page not to reach here */
};

/* The firmware revision is in the (unused) first entry of palrevs available */
#define rpb_firmrev rpb_palrev_avail[0]

/*
 * The memory cluster descriptor.
 */
struct rpb_cluster {
    long   rpb_pfn;		/* 000: starting PFN of this cluster */
    long   rpb_pfncount;	/* 008: count of PFNs in this cluster */
    long   rpb_pfntested;	/* 010: count of tested PFNs in cluster */
    long   rpb_va;		/* 018: va of bitmap */
    long   rpb_pa;		/* 020: pa of bitmap */
    long   rpb_checksum;	/* 028: checksum of bitmap */
    long   rpb_usage;		/* 030: usage of cluster */
};
#endif /* __LANGUAGE_C__ */
#define CLUSTER_USAGE_OS	((long)0)
#define CLUSTER_USAGE_PAL	((long)1)
#define CLUSTER_USAGE_NVRAM	((long)2)

/*
 * The "memory descriptor table" portion of the HWRPB.
 * Note that the main portion of the HWRPB is of variable size and there is a
 * variable number of per-cpu slots, hence this must be a separate structure.
 * Also note that the memory descriptor table contains a fixed portion plus
 * a variable number of "memory cluster descriptors" (one for each "cluster"
 * of memory).
 */
#ifdef __LANGUAGE_C__
struct rpb_mdt {
    long   rpb_checksum;	/* 000: checksum of entire mem desc table */
    long   rpb_impaddr;		/* 008: PA of implementation dep info */
    long   rpb_numcl;		/* 010: number of clusters */
    struct rpb_cluster rpb_cluster[1];	/* first instance of a cluster */
};

/*
 * The "Console Terminal Block" portion of the HWRPB, for serial line
 * UART console device. 
 */
struct ctb_tt {
    long   ctb_type;		/* 000: console type */
    long   ctb_unit;		/* 008: console unit */
    long   ctb_resv;		/* 010: reserved */
    long   ctb_length;		/* 018: byte length of device dep */
				/* portion */
    long   ctb_csr;		/* 020: CSR Address */
    long   ctb_tivec;		/* 028: <63>=tie; interrupt vector */
    long   ctb_rivec;		/* 030: <63>=rie; interrupt vector */
    long   ctb_baud;		/* 038: baud rate */
    long   ctb_put_sts;		/* 040: PUTS callback extended status */
    long   ctb_get_sts;		/* 048: GETS callback extended status */
    long   ctb_rsvd[1];		/* 050: reserved for console use */
};

/*
 * The "Console Terminal Block" portion of the HWRPB.
 */
struct rpb_ctb {
    long   rpb_type;		/* 000: console type */
    long   rpb_unit;		/* 008: console unit */
    long   rpb_resv;		/* 010: reserved */
    long   rpb_length;		/* 018: byte length of device dep portion */
    long   rpb_first;		/* 000: first field of device dep portion */
};

/*
 * The physical/virtual map for the console routine block.
 */
struct rpb_map {
	long	rpb_virt;		/* virtual address for map entry */
	long	rpb_phys;		/* phys address for map entry */
	long	rpb_pgcount;		/* page count for map entry */
};

/*
 * The "Console Routine Block" portion of the HWRPB.
 * Note: the "offsets" are all relative to the start of the HWRPB (HWRPB_ADDR).
 */
struct rpb_crb {
	long	rpb_va_disp;		/* va of call-back dispatch rtn */
	long	rpb_pa_disp;		/* pa of call-back dispatch rtn */
	long	rpb_va_fixup;		/* va of call-back fixup rtn */
	long	rpb_pa_fixup;		/* pa of call-back fixup rtn */
	long	rpb_num;		/* number of entries in phys/virt map */
	long	rpb_mapped_pages;	/* Number of pages to be mapped */
	struct	rpb_map rpb_map[1];	/* first instance of a map entry */
};
#endif /* __LANGUAGE_C__ */

/*
 * These macros define where within the HWRPB the CTB and CRB are located.
 */
#define CTB_SETUP ((struct rpb_ctb *) ((long)hwrpb_addr + 	\
			(long)(hwrpb_addr->rpb_ctb_off)))	
#define CRB_SETUP ((struct rpb_crb *) ((long)hwrpb_addr + 	\
			(long)(hwrpb_addr->rpb_crb_off)))	

#ifdef __LANGUAGE_C__
#ifdef KERNEL
extern	struct rpb *rpb;
extern	struct rpb_percpu *percpu;
extern	struct rpb_mdt *mdt;
extern	struct rpb_cluster *cluster;
extern	struct rpb_ctb *ctb;
extern	struct rpb_crb *crb;
extern	struct rpb_map *map;


#endif /* KERNEL */
#endif /* __LANGUAGE_C__ */

/*
 * The "Dynamic System Recognition" portion of the HWRPB.
 * It is used to obtain the platform specific data need to allow
 * the platform define the platform name, the platform SMM and LURT
 * data for software licensing
 */
#ifdef __LANGUAGE_C__
struct rpb_dsr {
	long	rpb_smm;		/* SMM nubber used by LMF	*/
	u_long	rpb_lurt_off;		/* offset to LURT table		*/
	u_long	rpb_sysname_off;	/* offset to sysname char count	*/
};
#endif /* __LANGUAGE_C__ */

#endif /* _RPB_H_ */

