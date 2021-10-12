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
 * @(#)$RCSfile: ciscs.h,v $ $Revision: 1.1.7.2 $ (DEC) $Date: 1993/06/29 20:28:34 $
 */
/*
 * derived from ciscs.h	4.2	(ULTRIX)	10/16/90
 */
/*
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		constants and data structure definitions visible to SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	April 22, 1985
 *
 *   Modification History:
 *
 *   16-Oct-1990	Pete Keilty
 *	1. Added cidevice_lk lock to the cipccb structure.
 *	2. Added 3 new macros for cidevice locking Lock_cidevice, 
 *	   Unlock_cidevice, Init_cidevice_lock;
 *	The above where added because of a CIXCD XMOV bug to synchronize
 *	register access.
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Moved interconnect specific registers to the adapter structure.
 *	2. Add port data packet multiple to cipccb structure for use with
 *	   4K packet eco.
 *
 *   19-Sep-1989	Pete Keilty
 *	Added CI/CIPORT ECO's 
 *
 *   18-Jan-1989	Todd M. Katz		TMK0004
 *	Add padding when it is necessary to keep longword alignment.  While
 *	some space is wasted such alignment is essential for ports of SCA to
 *	hardware platforms which require field alignment and access type to
 *	match( ie- only longword aligned entities may be longword accessed ).
 *
 *   23-Apr-1988	Todd M. Katz		TMK0003
 *	1. Create a single unified hierarchical set of naming conventions for
 *	   use within the CI port driver and describe them within ciport.h.
 *	   Apply these conventions to all names( routine, macro, constant, and
 *	   data structure ) used within the driver.  Restructure the driver to
 *	   segregate most CI family and port type specific code into separate
 *	   routines.  Such restructuring requires creation of new PCCB fields:
 *		1) To contain information which varies by family or port type(
 *		   mrltab, dg_cache, msg_cache, max_fn_level, max_rom_level ).
 *		2) To allow transparent indirect invocation of specific
 *		   routines appropriate to family or port type( disable_port,
 *		   start_port, load_ucode ).
 *	2. Add support for the CIXCB hardware port type by adding XMI register
 *	   pointers( structure xmi ) to the interconnect specific register
 *	   pointers( union ic ) of structure definition CIPCCB.
 *	3. Remove structure pointer rpmrltab and BIIC error interrupt control
 *	   register pointer( bierr_int ) from CIPCCB.
 *	4. Remove cleanup and fkip as CIPCCB local port status flags.
 *
 *   02-Apr-1988	Todd M. Katz		TMK0002
 *	Add support for onboard CI port microcode.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, and added SMP support.
 */
#ifndef _CISCS_H_
#define _CISCS_H_

/* CI Constants.
 */
#define	LBDSIZE			48	/* Size of loopback data	     */

/* CI Data Structure Definitions.
 */
typedef struct _ciregptrs {		/* Port Control Register Pointers */
    union	{
	struct	{
    volatile unsigned long *cnfr;	/* Configuration register	     */
    volatile unsigned long *madr;	/* Maintenance address register	     */
    volatile unsigned long *mdatr;	/* Maintenance data register	     */
	} old;
	struct	{
    volatile unsigned long *open;	/* register	     */
    volatile unsigned long *xpcpser;	/* XPC port specific error register*/
    volatile unsigned long *xpcpstat;	/* XPC port status register*/
	} kmf;
    } type;
    volatile unsigned long *pmcsr;	/* Port maintenance cntl & status reg*/
    volatile unsigned long *psr;	/* Port status register		     */
    volatile unsigned long *pqbbase;	/* PQB base register		     */
    volatile unsigned long *pcq0cr;	/* Port command queue 0 control reg  */
    volatile unsigned long *pcq1cr;	/* Port command queue 1 control reg  */
    volatile unsigned long *pcq2cr;	/* Port command queue 2 control reg  */
    volatile unsigned long *pcq3cr;	/* Port command queue 3 control reg  */
    volatile unsigned long *psrcr;	/* Port status release control reg   */
    volatile unsigned long *pecr;	/* Port enable control register      */
    volatile unsigned long *pdcr;	/* Port disable control register     */
    volatile unsigned long *picr;	/* Port initialization control reg   */
    volatile unsigned long *pdfqcr;	/* Port dg free queue control reg    */
    volatile unsigned long *pmfqcr;	/* Port msg free queue control reg   */
    volatile unsigned long *pmtcr;	/* Port maintenance timer control reg*/
    volatile unsigned long *pfar;	/* Port failing address register     */
    volatile unsigned long *pesr;	/* Port error status register	     */
    volatile unsigned long *ppr;	/* Port parameter register	     */
    volatile unsigned long *pper;	/* Port parameter ext. register	     */
} CIREGPTRS;

typedef	struct _cipccb	{		/* CI Specific Fields of PCCB	     */
    struct _gvpbq	dfreeq;		/* Datagram free queue head	     */
    struct _gvpbq	mfreeq;		/* Message free queue head	     */
    struct _ciadap	*ciadap;	/* Adapter Interface Block pointer   */
    struct _isr		*ciisr;		/* Interupt Service  Block pointer   */
    struct _ciregptrs	ciregptrs;	/* Port control register pointers    */
    void		( *disable_port )();/* Disable a local CI port	     */
    u_long		( *start_port )();  /* Start a local CI port	     */
    u_long		( *load_ucode )();  /* Load fn microcode( optional ) */
    struct	{			/* Local port status flags	     */
	u_int	init		:  1;	/*  First time initialization 	     */
	u_int	power		:  1;	/*  Port has power		     */
	u_int	mapped		:  1;	/*  Adapter space is mapped	     */
	u_int	mtimer		:  1;	/*  Maintenance timer is operational */
	u_int	connectivity	:  1;	/*  Port connectivity established    */
	u_int	onboard		:  1;	/*  Port microcode is onboard	     */
	u_int	adapt		:  1;	/*  This PCCB "ONLY" touchs adapter  */
	u_int			: 25;
    } lpstatus;
    struct _mrltab	*mrltab;	/* Microcode revision level table ptr*/
    u_int		lbcrc;		/* Loopback CRC			     */
    u_short		pkt_size;	/* Size of port command packet	     */
    u_short		reinit_tries;	/* Number consecutive re-inits left  */
    u_int		pkt_mult;	/* Port packet data multiple	     */
    struct	{			/* Loopback status flags	     */
	u_char	cable0_prev	:  1;	/*  Cable 0 prev status( Bad == 1 )  */
	u_char	cable0_curr	:  1;	/*  Cable 0 current status( Bad == 1)*/
	u_char	cable0_test	:  1;	/*  Cable 0 loopback tested	     */
	u_char	cable1_prev	:  1;	/*  Cable 1 prev status( Bad == 1 )  */
	u_char	cable1_curr	:  1;	/*  Cable 1 current status( Bad == 1)*/
	u_char	cable1_test	:  1;	/*  Cable 1 loopback tested	     */
	u_char			:  2;
    } lbstatus;
    u_char		interconnect;	/* Interconnect type	 	     */
    u_char		fn_level;	/* Functional ucode revision level   */
    u_char		rom_level;	/* PROM/Self-test ucode rev level    */
    u_char		lbdata[ LBDSIZE ];/* Loopback data		     */
    union ci_dattnopt	devattn;	/* Device attention information	     */
					/* Family/port specific information  */
    u_char		dg_cache;	/* Size of datagram cache	     */
    u_char		msg_cache;	/* Size of message cache	     */
    u_char		max_fn_level;	/* Max functional ucode rev level    */
    u_char		max_rom_level;	/* Max PROM/Self-test ucode rev lev  */
    u_char              *asb;		/* Adapter State Base Address        */
    struct slock 	cidevice_lk;	/* Lock_t for CIXCD hardware problem */
} CIPCCB;

typedef struct _cipqb	{		/* CI Specific Fields of PQB	     */
    struct _gvpbq  *dfreeq_hdr;		/* Datagram free queue head pointer  */
    struct _gvpbq  *mfreeq_hdr; 	/* Message free queue head pointer   */
    u_int	   dqe_len	: 12;	/* Datagram queue entry length	     */
    u_int			: 20;	/* MBZ				     */
    u_int	   mqe_len	: 12;	/* Message queue entry length	     */
    u_int			: 20;	/* MBZ				     */
    struct _gvppqb *vpqb_base;		/* PQB system virtual address	     */
    struct _gvpbd  *bdt_base;		/* BDT system virtual address	     */
    u_short	   bdt_len;		/* BDT octaword length		     */
    u_short			: 16;	/* MBZ				     */
    struct pte	   *spt_base;		/* System page table physical address*/
    u_int	   spt_len	: 22;	/* SPT longword length		     */
    u_int			: 10;	/* MBZ				     */
    struct pte	   *gpt_base;		/* Global page table physical address*/
    u_int	   gpt_len	: 22;	/* GPT longword length		     */
    u_int			: 10;	/* MBZ				     */
    u_int	   keepalive;		/* Variable maintenance tmr interval */
    u_int	   func_mask	: 3;	/* Memory management mode	     */
    u_int	   		: 29;	/* MBZ				     */
    u_char	   reserved1[ 16 ];	/* Reserved			     */
    u_int	   spt_base_ext;	/* SPT base ext bit 0 & 1 used 34bit */
    u_int	   asb_base;		/* Adapter state block base          */
    u_int	   asb_len;		/* Adapter state block lenght        */
    u_char	   reserved2[ 128 ];	/* Reserved			     */
    struct _gvph   *dqe_logout[ CI_NLOG ];/* Datagram queue entry logout area*/
    struct _gvph   *mqe_logout[ CI_NLOG ];/* Message queue entry logout area */
    u_char	   reserved3[ 128 ];	/* Port maintenance logout area	     */
} CIPQB;

typedef struct _cipb	{		/* CI Specific Fields of PB	     */
    struct _gvph *scpkt;		/* Set circuit off command packet    */
    struct _gvph *invtcpkt;		/* Invalidate translation cache pkt  */
    struct	{			/* Path status flags		     */
	u_int	cable0		:  1;	/*  Cable 0 status( Bad == 1 )	     */
	u_int	cable1		:  1;	/*  Cable 1 status( Bad == 1 )	     */
	u_int	cables_crossed	:  1;	/*  Cables crossed		     */
	u_int			: 29;
    } pstatus;
} CIPB;

#ifndef REMOVE_DSA_SMP
#define	Init_cidevice_lock( pccb ) {					\
    lockinit( &(( pccb )->pd.gvp.type.ci.cidevice_lk ), &lock_cidevice_d );\
}
#define	Lock_cidevice( pccb ) {						\
    smp_lock( &(( pccb )->pd.gvp.type.ci.cidevice_lk ), LK_RETRY );	\
}
#define	Unlock_cidevice( pccb ) {					\
    smp_unlock( &(( pccb )->pd.gvp.type.ci.cidevice_lk ));		\
}
#else
#define	Init_cidevice_lock( pccb ) {					\
    ;\
}
#define	Lock_cidevice( pccb ) {						\
    ;	\
}
#define	Unlock_cidevice( pccb ) {					\
    ;		\
}
#endif

#endif
