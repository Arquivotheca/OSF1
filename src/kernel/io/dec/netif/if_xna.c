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
static char *rcsid = "@(#)$RCSfile: if_xna.c,v $ $Revision: 1.1.12.8 $ (DEC) $Date: 1993/11/10 14:33:30 $";
#endif
/*
 * derived from:
 * static char *sccsid = "@(#)if_xna.c	5.1      (ULTRIX)        3/30/91";
 *
 * Revision History:
 *
 *	12/91	 - Laurie Murray
 *		   Port to OSF and Alpha for DEMNA support on Laser/Ruby.
 *		   Note: an attempt was made to leave mips support in place
 * 		   where possible, but this has not been tested or even
 *		   compiled for OSF/Mips, just left in tact from Ultrix.
 *
 *	02/24/91 - jsd
 *		   Allow loopback packets if COPYALL mode is set
 *
 *	10/19/89 - Uttam Shikarpur
 *		   Added "if_version" support to return H/W type
 *		   Added counters for multicast packets and bytes sent.
 *
 *	07/11/89 - Fred L. Templin
 *		   Changed port register addressing to work with DEMNA.
 *
 *	07/11/89 - Ali Rafieymehr
 *		   Added XMI support for device interrupts and configuration.
 *		   Also, moved xnadriver initialization here from bi_data.c.
 *
 *	06/14/89 - Fred L. Templin
 *		   Packet filter support integrated.
 *
 *	06/12/89 - Fred L. Templin
 *		   Integrated error logging support for XNA port fatal
 *		   errors.
 *
 *	05/10/89 - Fred L. Templin
 *		   Re-architected "wait-for-command-completion" strategy.
 *		   Kick off timers instead of calls to sleep.
 *
 *	4/11/88  - Fred L. Templin
 *		   Added MIPS-to-VAX physical addressing mode for MIPS
 *
 *	04/07/89 - Fred L. Templin
 *		   Added locks to make the XNA driver SMP-Safe
 *
 *	12/12/88 - Fred L. Templin
 *		   Code cleanup for initial baselevel submit
 *
 *	08/29/88 - Fred L. Templin
 *		   Created the if_xna.c module
 */

/* xna.h is generated at config time to #define NXNA (between 0 and 8) */
#include "xna.h"
#if NXNA > 0 || defined(BINARY)

#ifdef SMP_READY
#define lockinit(x, y) lockinit(x, y)
#define	smp_lock(x, y) smp_lock(x, y)
#define	smp_unlock(x) smp_unlock(x)
#else
#define lockinit(x, y)
#define smp_lock(x, y)
#define smp_unlock(x)
#endif

#include <data/if_xna_data.c>

extern	int ether_output();
extern	struct timeval	time;
extern	struct xmidata *get_xmi();
int	xnaattach(), xnaintr(), xnaprobe();
int	xnainit(),xnastart(),xnaioctl(),xnareset(), xnawatch();
struct	mbuf *xnamkparam(), *xnamkuser();
u_char  xna_notset[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u_int	xna_devtype;

#ifdef BI_SUPPORTED
extern	struct bidata bidata[];
#endif

#define XNAMCLGET(m) { \
	MGETHDR((m), M_DONTWAIT, MT_DATA); \
	if ((m)) { \
		MCLGET2((m), M_DONTWAIT); \
		if (((m)->m_flags & M_EXT) == 0) { \
			m_freem((m)); \
			(m) = (struct mbuf *)NULL; \
		} else\
			m->m_len = MCL2KBYTES; \
	} \
}

#define	XNATIMEOUT(xcmd, val) { \
	register int s = splnet(); \
	register int sec0 = time.tv_sec; \
	while (((xcmd)->opcode == (val)) && ((time.tv_sec - sec0) < 3)); \
	if ((xcmd)->opcode == (val)) \
		(xcmd)->opcode = CMD_NOP; \
	splx(s); \
}

/*
 * Probe the XNA; set up the register mappings with "nxv" giving the
 * base address.
 * Return 0 for failure, non-zero if adapter is alive.
 */
xnaprobe(nxv,ctlr)
	caddr_t nxv;
	struct controller *ctlr;
{
	register struct	xna_softc *sc = &xna_softc[ctlr->ctlr_num];
	register struct	xnadevice *addr = &sc->xregs;
	register struct xnapdb *xpdb;
	register int delay;
	struct xnarecv_ring *rp;
	struct xnacmd_ring *tp;
	struct xmidata *xmidata;
	struct mbuf *m;
	int unit = ctlr->ctlr_num;
	caddr_t dpa;
	vm_offset_t tmp_addr;
	int i;
	u_int xpd1,xpd2;

	/*
	 * Determine device type (DEBNI, DEMNA). Map register blocks
	 * and check for power-up errors.
	 */
#ifdef __alpha
	addr->xnabase = nxv;
	xna_devtype = RDCSR(LONG_32, ctlr, nxv);
#else /* __mips */
	addr->xnabase = (struct xnabase *)nxv;
	xna_devtype = addr->xna_dtype;
#endif
	switch (xna_devtype & XMIDTYPE_TYPE) {
		case XNADEMNA:
#ifdef __mips
			addr->_xpd1 = (struct xnareg *)(nxv + XNAPD1_XMI);
			addr->_xpd2 = (struct xnareg *)(nxv + XNAPD2_XMI);
			addr->_xpst = (struct xnareg *)(nxv + XNAPST_XMI);
			addr->_xpud = (struct xnareg *)(nxv + XNAPUD_XMI);
			addr->_xpci = (struct xnareg *)(nxv + XNAPCI_XMI);
			addr->_xpcp = (struct xnareg *)(nxv + XNAPCP_XMI);
			addr->_xpcs = (struct xnareg *)(nxv + XNAPCS_XMI);
#else /* __alpha */
			/*  set up addresses of the XMI registers  */
			addr->xber_xmi = nxv + XMI_XBER;
			addr->xfadr_xmi = nxv + XMI_XFADR;
			addr->xfaer_xmi = nxv + XMI_XFAER;
			addr->xcomm_xmi = nxv + XMI_XCOMM;
	
			/*  set up addresses of the DEMNA registers  */
			addr->xnapd1 = nxv + XNAPD1_XMI;
			addr->xnapd2 = nxv + XNAPD2_XMI;
			addr->xnapst = nxv + XNAPST_XMI;
			addr->xnapud = nxv + XNAPUD_XMI;
			addr->xnapci = nxv + XNAPCI_XMI;
			addr->xnapcp = nxv + XNAPCP_XMI;
			addr->xnapcs = nxv + XNAPCS_XMI;
#endif
			/*
		 	* Wait up to 10 seconds for XPST_UNDEF state
		 	* to appear.
		 	*/
#ifdef __mips
			for (delay = 1000;
			     (delay && (addr->xnapst != XPST_UNDEF)); delay--)
				DELAY(10000);
			if ((addr->xber_xmi & XMI_STF) ||
				(addr->xnapst != XPST_UNDEF)) {
				printf ("xna%d: port self test failed: xber = 0x%x, xpst = 0x%x, xpd1 = 0x%x, xpd2 = 0x%x, xpud = 0x%x\n",
					unit, addr->xber_xmi, addr->xnapst, addr->xnapd1, addr->xnapd2, addr->xnapud);
				return(0);
			}
#else /* __alpha */
			for (delay = 1000;
			     (delay && (RDCSR(LONG_32, ctlr, addr->xnapst) != XPST_UNDEF));
		     		delay--)
				DELAY(10000);
			if ((RDCSR(LONG_32, ctlr, addr->xber_xmi) & XMI_STF) ||
		    	    (RDCSR(LONG_32, ctlr, addr->xnapst) != XPST_UNDEF)) {
				printf("xna%d: port self test failed\n", unit);
				xnaprintregs(ctlr);
				return(0);
			}
#endif
			break;
#ifdef BI_SUPPORTED
		case XNADEBNI:
			addr->_xpst = (struct xnareg *)(nxv + XNAPST_BI);
			addr->_xpd1 = (struct xnareg *)(nxv + XNAPD1_BI);
			addr->_xpd2 = (struct xnareg *)(nxv + XNAPD2_BI);
			addr->_xpud = (struct xnareg *)(nxv + XNAPUD_BI);
			addr->_xpci = (struct xnareg *)(nxv + XNAPCI_BI);
			addr->_xpcp = (struct xnareg *)(nxv + XNAPCP_BI);
			addr->_xpcs = (struct xnareg *)(nxv + XNAPCS_BI);

			/*
			 * Same as for DEMNA. Wait up to 10 seconds. If the
			 * BROKE bit is set, or we're in the wrong state,
			 * perform hard node restart.
			 */
			for (delay = 1000;
			     (delay && (addr->xnapst != XPST_UNDEF));
			     delay--)
				DELAY(10000);
			if ((addr->xctrl_bi & BICTRL_BROKE) ||
			     (addr->xnapst != XPST_UNDEF)) {
				printf ("xna%d: port self test failed: bi_ctrl = 0x%x, bi_err = 0x%x, xpst = 0x%x, xpd1 = 0x%x, xpd2 = 0x%x, xpud = 0x%x\n",
					unit, addr->xctrl_bi, addr->xber_bi, addr->xnapst, addr->xnapd1, addr->xnapd2, addr->xnapud);
				return(0);
			}
			break;
#endif
		default:
			return (0);
	}

	/*
	 * We are now guaranteed that the port has succesfully completed
	 * self test and is in the "UNINITIALIZED" state. At this point,
	 * we can grab the default physical address from the port data
	 * registers.
	 */
#ifdef __mips
	dpa = (caddr_t)&addr->xnapd1;
	for (i = 0; i < 4; i++)
		sc->is_dpaddr[i] = *dpa++;
	dpa = (caddr_t)&addr->xnapd2;
	for (; i < 6; i++)
		sc->is_dpaddr[i] = *dpa++;
#else /* __alpha */
	xpd1 = RDCSR(LONG_32, ctlr, addr->xnapd1);
	xpd2 = RDCSR(LONG_32, ctlr, addr->xnapd2);
	dpa = (caddr_t)&xpd1;
	for (i = 0; i < 4; i++)
		sc->is_dpaddr[i] = *dpa++;
	dpa = (caddr_t)&xpd2;
	for (; i < 6; i++)
		sc->is_dpaddr[i] = *dpa++;
#endif
	/*
	 * Device actual physical address not set yet
	 */
	bcopy(xna_notset, sc->is_addr, 6);

	/*
	 * allocate data structures, fill out port data block, and init device.
	 */
	if ( (sc->rring = (struct xnarecv_ring *)kalloc(1024)) == 0) {
		printf ("xna%d: couldn't allocate receive ring\n", unit);
		return (0);
	}
	bzero(sc->rring,1024);
	if ( (sc->tring = (struct xnacmd_ring *)kalloc(1024)) == 0) {
		printf ("xna%d: couldn't allocate command ring\n", unit);
		kfree(sc->rring, 1024);
		return (0);
	}
	bzero(sc->tring,1024);
	if ( (sc->xpdb = (struct xnapdb *)kalloc(512)) == 0) {
		printf ("xna%d: couldn't allocate port data block\n", unit);
		kfree(sc->rring, 1024);
		kfree(sc->tring, 1024);
		return (0);
	}
	bzero(sc->xpdb,512);
	xpdb = sc->xpdb;

#ifdef XNADEBUG
	printf("XNA: pdb at %x\n", xpdb);
	printf("XNA: command ring at %x\n", sc->tring);
	printf("XNA: receive ring at %x\n", sc->rring);
	{
	extern struct mbox_debug mbox_debug[];
	extern active_mbox;
	svatophys(mbox_debug, &tmp_addr);
	printf("XNA: phsysaddr of mbox_debug = %x\n", tmp_addr);
	svatophys(&active_mbox, &tmp_addr);
	printf("XNA: phsysaddr of active_mbox = %x\n", tmp_addr);
	}
#endif
	/*
	 * Init first sc->nactv receive descriptors with buffer allocation
	 * side-effects. (use nNXNAACTV as initial value). xnainitdesc()
	 * will turn off ownership bit to give the descriptor to the port.
	 * (XNAMCLGET() CAN fail in the case that there are no mbuf's
	 * available, but the "while" loop in the receive interrupt handler
	 * will initialize more as mbufs become available.) Set ownership of
	 * left-over descriptors to the port driver.
	 */
	sc->nactv = nNXNAACTV;
	for (i = 0, rp = &sc->rring[0]; (i < sc->nactv); i++, rp++) {
		rp->mbufindx = i;
		XNAMCLGET(m);
		if (m)
			xnainitdesc(rp, m, sc);
		else
			break;
	}
	sc->nrecv = i;

	for (; i < nNXNARCV; i++, rp++) {
		rp->mbufindx = i;
		rp->status = ST_ROWN;
	}

	for (i = 0, tp = &sc->tring[0]; i < nNXNACMD; i++, tp++) {
		tp->mbufindx = i;
		tp->status = ST_TOWN;
	}

	/*
	 * sc->tindex == index of next transmit/command desc. to activate
	 * sc->tlast  == index of last transmit/command processed
	 * sc->nxmit  == # of transmit/commands pending
	 * sc->rindex == index of next recv. descriptor to activate
	 * sc->rlast  == index of last receive processed
	 * sc->nrecv  == # of receive descriptors currently active
	 * sc->nproc  == running count of receives and transmits processed
	 */
	sc->tindex = sc->nxmit = 0;
	sc->rindex = sc->nrecv;
	sc->rlast = sc->tlast = sc->nproc = -1;

	xpdb->addr_mode = AM_PHYS40;	/* 40 bit, non-interlocked physical */
	if ((svatophys(sc->tring,&tmp_addr)) != KERN_SUCCESS) {
		printf ("xna%d: svatophys failed for tring\n", unit);
		kfree(sc->rring, 1024);
		kfree(sc->tring, 1024);
		kfree(sc->xpdb, 512);
		return(0);
	}
#ifdef __mips
	xpdb->cmd.xaddr_lo = tmp_addr;
	xpdb->cmd.xaddr_hi = 0;
#else /* __alpha */
	xpdb->cmd.xaddr_lo = (u_int)tmp_addr;
	xpdb->cmd.xaddr_hi = (tmp_addr >> 32);
#endif
	if ((svatophys(sc->rring,&tmp_addr)) != KERN_SUCCESS) {
		printf ("xna%d: svatophys failed for rring \n", unit);
		kfree(sc->rring, 1024);
		kfree(sc->tring, 1024);
		kfree(sc->xpdb, 512);
		return(0);
	}
#ifdef __mips
	xpdb->recv.xaddr_lo = tmp_addr;
	xpdb->recv.xaddr_hi = xpdb->recv.xmbz = 0;
#else /* __alpha */
	xpdb->recv.xaddr_lo = (u_int)tmp_addr;
	xpdb->recv.xaddr_hi = (u_int)(tmp_addr >> 32);
#endif

	/*
	 * Set up user buffer unavailable counter
	 */
	if ((svatophys(&sc->xna_ubuacnt,&tmp_addr)) != KERN_SUCCESS) {
		printf ("xna%d: svatophys failed for ubuacnt\n", unit);
		kfree(sc->rring, 1024);
		kfree(sc->tring, 1024);
		kfree(sc->xpdb, 512);
		return(0);
	}
#ifdef __mips
	xpdb->ubua.xaddr_lo = tmp_addr;
	xpdb->ubua.xaddr_hi = 0;
#else /* __alpha */
	xpdb->ubua.xaddr_lo = (u_int)tmp_addr;
	xpdb->ubua.xaddr_hi = (u_int)(tmp_addr >> 32);
#endif
	xpdb->cmd.xlen = sizeof(struct xnacmd_ring); /* cmd ring entry size */
	xpdb->ubua.xlen = sizeof(struct xnactr_ent);

	/*
	 * set up interrupt and error interrupt vectors
	 */
	switch (xna_devtype & XMIDTYPE_TYPE) {
		case XNADEMNA:
			xmidata = get_xmi(ctlr->bus_num);
			xpdb->ivec.level = xpdb->evec.level = 1;
			xpdb->ivec.vector = xpdb->evec.vector = (short)
			   (SCB_XMI_VEC_ADDR(xmidata,ctlr->bus_num,ctlr->slot,LEVEL15)) >> 2;
			xpdb->ivec.nid_mask = xpdb->evec.nid_mask =
			   xmidata->xmiintr_dst;
			break;
#ifdef BI_SUPPORTED
		case XNADEBNI:
			xpdb->ivec.level = xpdb->evec.level = 1;
			xpdb->ivec.vector = xpdb->evec.vector =
			   SCB_BI_VEC_ADDR(ctlr->bus_num,ctlr->slot,LEVEL15) -
			   &scb.scb_stray;
			xpdb->ivec.nid_mask = xpdb->evec.nid_mask =
			   bidata[ctlr->bus_num].biintr_dst;
			break;
#endif
	}

	/*
	 * Hit device init, and wait for XPST_INIT state to appear (up to
	 * one second).
	 */
	if ((svatophys(xpdb, &tmp_addr)) != KERN_SUCCESS){
		kfree(sc->rring, 1024);
		kfree(sc->tring, 1024);
		kfree(sc->xpdb, 512);
		return(0);
	}
#ifdef __mips
	addr->xnapd1 = tmp_addr; addr->xnapd2 = 0;
	addr->xnapci = XPCI_INIT;
	for (delay = 100; (delay && (addr->xnapst != XPST_INIT)); delay--)
		DELAY(10000);
	if ((addr->xnapst & XPST_MASK) != XPST_INIT) {
		printf ("xna%d: port probe failed: xpst=0x%x, xpd1=0x%x, xpd2=0x%x, xpud=0x%x\n",
		  unit, addr->xnapst, addr->xnapd1, addr->xnapd2, addr->xnapud);
		return (0);
	}
#else /* __alpha */
	WRTCSR(LONG_32, ctlr, addr->xnapd1, (u_int)tmp_addr);
	WRTCSR(LONG_32, ctlr, addr->xnapd2, (char)(tmp_addr >> 32));
	mb();
	WRTCSR(LONG_32, ctlr, addr->xnapci, XPCI_INIT);
	for (delay = 100; (delay && (RDCSR(LONG_32, ctlr, addr->xnapst) != XPST_INIT)); delay--)
		DELAY(10000);
	if ((RDCSR(LONG_32, ctlr, addr->xnapst) & XPST_MASK) != XPST_INIT) {
		printf ("xna%d: port probe failed\n", unit);
		xnaprintregs(ctlr);
		return (0);
	}
#endif

	/*
	 * Get the initial value of the "potential system buffer unavailable"
	 * counter.
	 */
	sc->xna_sbuacnt = xpdb->p_sbua;
	return (sizeof(struct xna_softc));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
xnaattach(ctlr)
	struct controller *ctlr;
{
	register struct xna_softc *sc = &xna_softc[ctlr->ctlr_num];
	register struct ifnet *ifp = &sc->is_if;
	struct sockaddr_in *sin;
	register int i;

	sc->ctlr = ctlr;
	ifp->if_unit = ctlr->ctlr_num;
	ifp->if_name = "xna";
#ifdef SMP_READY
	ifp->lk_softc = &sc->lk_xna_softc;
#endif
	ifp->if_mtu = ETHERMTU;
	ifp->if_mediamtu = ETHERMTU;
	ifp->if_type = IFT_ETHER;
	ifp->if_flags |= IFF_BROADCAST|IFF_MULTICAST|IFF_NOTRAILERS|IFF_SIMPLEX;
	((struct arpcom *)ifp)->ac_ipaddr.s_addr = 0;
	sc->is_ac.ac_bcastaddr = (u_char *)etherbroadcastaddr;
	sc->is_ac.ac_arphrd = ARPHRD_ETHER;	/*  Ethernet addr  */
	ifp->if_addrlen = 6;			/*  Media address len */
	ifp->if_hdrlen = sizeof(struct ether_header) + 8;  /* for SNAP? */

	/*
	 * Initialize multicast address table
	 */
	for (i = 0; i < NMULTI; i++) {
		bcopy(etherbroadcastaddr, sc->is_multi[i], 6);
		sc->is_muse[i] = 0;
	}

	/*
	 * Set maxlen for the command queue; attach interface
	 */
	lockinit(&sc->lk_xna_softc, &lock_device15_d);
	sc->if_cmd.ifq_maxlen = IFQ_MAXLEN;
	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	ifp->if_init = xnainit;
	ifp->if_output = ether_output;
	ifp->if_start = xnastart;
	ifp->if_ioctl = xnaioctl;
	ifp->if_reset = xnareset;
	ifp->if_watchdog = xnawatch;
	ifp->if_timer = 1;
#ifdef SMP_READY
	ifp->d_affinity = ALLCPU;
#endif
	ifp->if_baudrate = ETHER_BANDWIDTH_10MB;

	/*
	 * Hook up the device interrupt vector to begin allowing interrupts.
	 * We know that the device has been successfully initialized at this
	 * point. Initialize if_version string.
	 */
	switch (xna_devtype & XMIDTYPE_TYPE) {
		case XNADEMNA:
			xmidev_vec(ctlr->bus_num, ctlr->slot, LEVEL15, ctlr);
			ifp->if_version = "DEC DEMNA Ethernet Interface";
			break;
#ifdef BI_SUPPORTED
		case XNADEBNI:
			bidev_vec(ctlr->ui_adpt, ctlr->ui_nexus, LEVEL15, ctlr);
			bcopy("DEC DEBNI Ethernet Interface", ifp->if_version, 28);
			ifp->if_version[28] = '\0';
			break;
#endif
	}

	printf("xna%d: %s, hardware address %s\n", ifp->if_unit,
	       ifp->if_version, ether_sprintf(sc->is_dpaddr));

	attachpfilter(&(sc->is_ed));

	if_attach(ifp);
}

/*
 * Initialize interface. May be called by a user process or a software
 * interrupt scheduled by xnareset().
 */
xnainit(unit)
	int unit;
{
	register struct xna_softc *sc = &xna_softc[unit];
	register struct xnadevice *addr = &sc->xregs;
	register struct xnacmd_buf *xcmd;
	register struct mbuf *m;
	register int delay;
	struct ifnet *ifp = &sc->is_if;
	int s, t;

	/* not yet, if address still unknown */

	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
	if (ifp->if_flags & IFF_RUNNING)
		return;

	if (xna_firsttime[unit] == 0) {
		int i;
		struct xnarecv_ring *rp;
		xna_firsttime[unit] = 1;
		for (i = 0, rp = &sc->rring[0]; (i < sc->nactv); i++, rp++) {
			XNAMCLGET(m);
			if (m)
				xnainitdesc(rp, m, sc);
			else
				break;
		}
		sc->rindex = sc->nrecv = i;
	}

	/*
	 * Lock the softc to ensure coherency of softc data needed to fill
	 * out the command buffers by "xnamkparam()" and "xnamkuser()", and
	 * to lock access to the if_cmd queue by "xnacmd()". (xnaintr writes
	 * command completion info. into the first longword of the command
	 * buffer; XNATIMEOUT flags command timeouts. Need the splnet() if
	 * we're called from softclock() by xnareset.
	 */
	s = splnet(); t = splimp();
	smp_lock(&sc->lk_xna_softc, LK_RETRY);

	/*
	 * Issue a "PARAM" command to fill in actual physical address (from
	 * sc->is_addr) and other system parameters. Wait for completion,
	 * which will be signaled by the interrupt handler.
	 */
	if (m = xnamkparam(sc, ifp)) {
		xnacmd(sc, m);
		smp_unlock(&sc->lk_xna_softc);
		splx(t);
		xcmd = mtod(m, struct xnacmd_buf *);
		XNATIMEOUT(xcmd, CMD_PARAM);	/* Wait */
		t = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		switch (xcmd->opcode) {
			case CMD_COMPLETE:
				/*
				 * Set actual physical address
				 */
				bcopy(&xcmd->xnaparam.apa, sc->is_addr, 6);
				m_freem(m);
				break;
			case CMD_INVAL:
				m_freem(m);
			case CMD_NOP:
			default:
				/*
				 * Don't free m if adapter appears to be hung
				 */
				printf("xna%d: port init failed (param)\n",unit);
				goto done;
		}
	} else {
		printf ("xna%d: port init failed (no mbuf)\n", unit);
		log(LOG_ERR,"xna%d: can't get mbuf for xnamkparam\n",unit);
		goto done;
	}

	/*
	 * Issue a "USTART" command for the Ethernet user. (Will initialize
	 * all multicast addresses here.)
	 */
	if (m = xnamkuser(sc, XNA_ETHERU, CMD_USTART)) {
		xcmd = mtod(m, struct xnacmd_buf *);
		xnacmd(sc, m);
		smp_unlock(&sc->lk_xna_softc);
		splx(t);
		xcmd = mtod(m, struct xnacmd_buf *);
		XNATIMEOUT(xcmd, CMD_USTART);	/* Wait */
		t = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		switch (xcmd->opcode) {
			case CMD_COMPLETE:
				m_freem(m);
				break;
			case CMD_INVAL:
				m_freem(m);
			case CMD_NOP:
			default:
				/*
				 * Don't free m if adapter appears to be hung
				 */
				printf("xna%d: port init failed (ustart)\n",unit);
				goto done;
		}
	}
	else {
		printf ("xna%d: port init failed (no mbuf)\n", unit);
		log(LOG_ERR,"xna%d: can't get mbuf for xnamkuser\n",unit);
		goto done;
	}

	/*
	 * Clear reset, mark interface up and running; start output
	 * on device.
	 */
	sc->flags &= ~XNA_RFLAG;
	sc->is_if.if_flags |= IFF_UP|IFF_RUNNING;
	sc->is_if.if_flags &= ~IFF_OACTIVE;
	sc->ztime = time.tv_sec;
	if (sc->is_if.if_snd.ifq_head)
		xnastart(ifp);			/* queue output packets */
done:
	/*
	 * Relinqusih softc lock, drop IPL.
	 */
	smp_unlock(&sc->lk_xna_softc);
	splx(t); splx(s);
	return;
}

/*
 * Perform a node halt/reset (SOFT reset) due to state change interrupt.
 * Issue a timeout() on xnainit, but give up if we're being reset
 * constantly. xnainit() will set IFF_RUNNING if the reset succedes.
 */
xnareset(unit)
	int unit;
{
	register struct xna_softc *sc = &xna_softc[unit];
	register struct xnadevice *addr = &sc->xregs;
	register struct xnarecv_ring *rp;
	register struct xnacmd_ring *tp;
	struct xnapdb *xpdb = sc->xpdb;
	struct ifnet *ifp = &sc->is_if;
	struct mbuf *mp;
	int i, delay;
	vm_offset_t tmp_addr;

	/*
	 * Start receive and command queues from scratch. Clean off the
	 * receive and transmit rings, then initialize the first nNXNAACTV
	 * receive descriptors at the head of the receive ring.
	 */
	for (i = 0, rp = &sc->rring[0]; i < nNXNARCV; i++, rp++) {
		rp->status = ST_ROWN;
		mb();
		mp = sc->mbuf_recv[i];
		if (mp) {
			m_freem(mp);
			sc->mbuf_recv[i] = 0L;
		}
	}

	for (i = 0, tp = &sc->tring[0]; i < nNXNACMD; i++, tp++) {
		tp->status = ST_TOWN;
		mb();
		if (mp = sc->mbuf_tofree[i]) {
			if (tp->status & ST_CMD) {
				switch ((mtod(mp, struct xnacmd_buf *))->opcode) {
					case CMD_RDCNTR:
					case CMD_RCCNTR:
						mp->m_data = mp->m_dat;
						mp->m_len = MLEN;
						break;
					default:
						break;
				}
			}
			m_freem(mp);
			sc->mbuf_tofree[i] = 0L;
		}
	}

	sc->nactv = nNXNAACTV;
	for (i = 0, rp = &sc->rring[0]; (i < sc->nactv); i++, rp++) {
		XNAMCLGET(mp);
		if (mp)
			xnainitdesc(rp, mp, sc);
		else
			break;
	}
	sc->nrecv = i;

	sc->tindex = sc->nxmit = 0;
	sc->rindex = sc->nrecv;
	sc->rlast = sc->tlast = sc->nproc =  -1;

	/*
	 * Re-init the device; wait for XPST_INIT state to appear
	 */
	if ((svatophys(xpdb, &tmp_addr)) != KERN_SUCCESS){
		printf("xna%d: xnareset: svatophys failed\n",unit);
		printf("xna%d: port reset failed\n",unit);
		return(0);
	}
#ifdef __mips
	addr->xnapd1 = tmp_addr; addr->xnapd2 = 0;
	addr->xnapci = XPCI_INIT;
	for (delay = 100; (delay && (addr->xnapst != XPST_INIT)); delay--)
		DELAY(10000);
	if ((addr->xnapst & XPST_MASK) != XPST_INIT) {
		printf ("xna%d: port reset failed: xpst=0x%x, xpd1=0x%x, xpd2=0x%x, xpud=0x%x\n",
			unit, addr->xnapst, addr->xnapd1, addr->xnapd2, addr->xnapud);
		return;
	}
#else /* __alpha */
	WRTCSR(LONG_32, sc->ctlr, addr->xnapd1, (u_int)tmp_addr);
	WRTCSR(LONG_32, sc->ctlr, addr->xnapd2, (u_int)(tmp_addr >> 32));
	mb();
	WRTCSR(LONG_32, sc->ctlr, addr->xnapci, XPCI_INIT);
	for (delay = 100; (delay && (RDCSR(LONG_32, sc->ctlr, addr->xnapst) != XPST_INIT)); delay--)
		DELAY(10000);
	if ((RDCSR(LONG_32, sc->ctlr, addr->xnapst) & XPST_MASK) != XPST_INIT) {
		printf ("xna%d: port reset failed\n", unit);
		xnaprintregs(sc->ctlr);
		return;
	}
#endif

	/*
	 * Get the initial value of the "potential system buffer unavailable"
	 * counter. Schedule an "xnainit" at SPLNET if device was running.
	 */
	sc->xna_sbuacnt = xpdb->p_sbua;

	if (ifp->if_flags & IFF_RUNNING) {
		ifp->if_flags &= ~IFF_RUNNING;
		timeout(xnainit, unit, 1);
	}
}

/*
 * XNA start routine. Strings output packets and commands onto the command
 * ring. Start routine is called at splimp() WITH the softc locked.
 */
xnastart(ifp)
register struct ifnet *ifp;
{
	register struct mbuf *m, *mprev;
	register struct xnacmd_ring *tp;
	register int index, i;
	struct xna_softc *sc = &xna_softc[ifp->if_unit];
	struct xnadevice *addr = &sc->xregs;
	struct mbuf *m0;
	vm_offset_t tmp_addr;
	register int page_cross;

	/*
	 * Process the transmit and command queues, giving priority to
	 * commands. Done when we either run out of requests, or command
	 * ring entries to string them on.
	 */
	for (index = sc->tindex, tp = &sc->tring[index]; sc->nxmit < nNXNACMD;
	     index = ++index % nNXNACMD, tp = &sc->tring[index]) {
next_m:
		if (sc->if_cmd.ifq_head) {
			IF_DEQUEUE(&sc->if_cmd, m0);
			tp->status |= ST_CMD;
		} else {
			if (sc->is_if.if_snd.ifq_head) {
				IF_DEQUEUE(&sc->is_if.if_snd, m0);
				tp->status &= ~ST_CMD;
			} else {
				sc->tindex = index;
				return;
			}
		}

		/*
		 * String the mbuf chain onto the command ring entry's
		 * buffer segment addresses.
		 */
                for (m=m0, i=0; (m && i<XNA_XMIT_NBUFS); mprev=m, m=m->m_next) {
                        register int len = m->m_len;
                        register u_long vadr = mtod(m, u_long);
                        register int clsize;

                        if (len == 0)
                                continue;

                        if (svatophys(vadr, &tmp_addr) != KERN_SUCCESS)
                                panic("xnastart: svatophys failed");

                        tp->bseg[i].xaddr_lo = (u_int)tmp_addr;
                        tp->bseg[i].xaddr_hi = (u_short)(tmp_addr>>32);

                        /*
                         * If there was a page-crossing in this mbuf, we'll need
                         * to setup another entry.
                         */
                        page_cross = 0;
                        clsize = MCLBYTES - (vadr & (MCLBYTES -1));
                        if (clsize >= len)
                                tp->bseg[i++].xlen = len;
                        else {
                                page_cross = 1;
                                tp->bseg[i++].xlen = clsize;

                                /*
                                 * If we've reached the end, then have to quit.
                                 * It's important to do so from up above.
                                 */
                                if (i == XNA_XMIT_NBUFS) {
                                        i++;
                                        continue;
                                }

                                vadr += clsize;
                                if (svatophys(vadr, &tmp_addr) != KERN_SUCCESS)
                                        panic("xnaintr: svatophys failed");

                                tp->bseg[i].xaddr_lo = (u_int)tmp_addr;
                                tp->bseg[i].xaddr_hi = (u_short)(tmp_addr>>32);
                                tp->bseg[i++].xlen = len - clsize;
                        }
                }

                /*
                 * If we ran out of slots, then must copy current and
                 * subsequent buffers into a cluster mbuf.
                 */
                if (m) {
                        register int off = 0;
                        register struct mbuf *n;

                        XNAMCLGET(n);
                        if (!n) {
                                m_freem(m0);
                                goto next_m;
                        }

                        for (m=mprev; m; off+=m->m_len, mprev=m, m=m->m_next)
                                bcopy(mtod(m, vm_offset_t),
                                      mtod(n, vm_offset_t)+off,
                                      (unsigned)m->m_len);

                        /* Keep track of this mbuf so we can free it later */
                        n->m_len = off; n->m_next = 0;
                        mprev->m_next = n;

                    	if ((svatophys(mtod(n, vm_offset_t), &tmp_addr)) != KERN_SUCCESS)
                                panic("xnastart: svatophys failed");

                        /* Make sure we point to the right slot */
                        i--;
                        if (page_cross)
                                i--;

                        tp->bseg[i].xaddr_lo = (u_int)tmp_addr;
                        tp->bseg[i].xaddr_hi = (u_short)(tmp_addr >>32);
                        tp->bseg[i++].xlen = n->m_len;
                }

		sc->mbuf_tofree[tp->mbufindx] = m0;
		tp->usr_index = XNA_ETHERU;
		tp->nbufs = i;
		mb();
		tp->status &= ~ST_TOWN;
		sc->nxmit++;

		/*
		 * Advise the port that a new transmit packet is pending
		 */
#ifdef __mips
		addr->xnapcp = XPCP_POLL;
#else /* __alpha */
		WRTCSR(LONG_32, sc->ctlr, addr->xnapcp, XPCP_POLL);
#endif
	}  /* endfor */
	sc->tindex = index;

        /* If the ring is full, inform the network-layer about it. */
        if (sc->nxmit == nNXNACMD)
                ifp->if_flags |= IFF_OACTIVE;
}

/*
 * XNA command start routine. Wraps a command buffer in an mbuf (for queuing
 * purposes ONLY), then enqueues the request to xnastart(). Command start
 * routine is called at splimp() WITH the softc locked.
 */
xnacmd(sc, m)
	register struct xna_softc *sc;
	register struct mbuf *m;
{
	register struct xnacmd_buf *xcmd = mtod(m, struct xnacmd_buf *);

	if (IF_QFULL(&sc->if_cmd)) {
		/*
		 * Couldn't enqueue command request; 
		 * invalidate opcode to signal error.
		 */
		xcmd->opcode = CMD_INVAL;
		return;
	}

	/*
	 * Place command request on command queue
	 */
	IF_ENQUEUE(&sc->if_cmd, m);
	xnastart(&(sc->is_if));
}

/*
 * XNA device interrupt handler
 */
#ifdef __mips
xnaintr(unit)
	int unit;
{
	register struct xna_softc *sc = &xna_softc[unit];
#else /* __alpha */
xnaintr(ctlr)
struct controller *ctlr;
{
	register struct xna_softc *sc = &xna_softc[ctlr->ctlr_num];
	int unit = ctlr->ctlr_num;
#endif
	register struct xnadevice *addr = &sc->xregs;
	int s = splimp();
	mb();
	/*
	 * Lock softc, since we will be updating the per-unit ring pointers
	 * and active ring entry counts frequently.
	 */
	smp_lock(&sc->lk_xna_softc, LK_RETRY);

	/*
	 * See if we got here due to a port state change interrupt. If so,
	 * need to log error, reset interface, and re-init.
	 */
#ifdef __mips
	if ((addr->xnapst & XPST_MASK) != XPST_INIT) {
#else /* __alpha */
	if ((RDCSR(LONG_32, ctlr, addr->xnapst) & XPST_MASK) != XPST_INIT) {
#endif
		struct el_rec *elrp;
#ifdef XNADEBUG
		printf("XNA: error interrupt\n");
#endif
		log(LOG_ERR,"xna%d: error interrupt; logging to error log\n",unit);
		if((elrp = ealloc(sizeof(struct el_xna), EL_PRILOW))) {
			struct el_xna *elbod = &elrp->el_body.elxna;

			switch (xna_devtype & XMIDTYPE_TYPE) {
				case XNADEMNA:
					bcopy (sc->xpdb->port_err,
					&elbod->xnatype.xnaxmi.xnaxmi_fatal,
					sizeof(struct xna_xmi_fatal));
					LSUBID(elrp,ELCT_DCNTL,ELXMI_XNA,
					       0,sc->xpdb->ivec.nid_mask,unit,
					       XNA_FATAL);
					break;
#ifdef BI_SUPPORTED
				case XNADEBNI:
					bcopy (sc->xpdb->port_err,
					&elbod->xnatype.xnabi.xnabi_fatal,
					sizeof(struct xna_bi_fatal));
					LSUBID(elrp,ELCT_DCNTL,ELBI_XNA,
					       0,sc->xpdb->ivec.nid_mask,unit,
					       XNA_FATAL);
					break;
#endif
			}
			EVALID(elrp);
		}
		if (!(sc->flags & XNA_RFLAG)) {
			sc->flags |= XNA_RFLAG;
			xnareset(unit);
		} else {
#ifdef __mips
			printf ("xna%d: port reset failed: xpst=0x%x, xpd1=0x%x, xpd2=0x%x, xpud=0x%x\n",
				unit, addr->xnapst, addr->xnapd1,
				addr->xnapd2, addr->xnapud);
#else /* __alpha */
			printf ("xna%d: port reset failed\n", unit);
			xnaprintregs(ctlr);
#endif
		}
		smp_unlock(&sc->lk_xna_softc);
		splx(s);
		return;
	}

	/*
	 * Process receive ring
	 */
	{
	register int index;
	register struct xnarecv_ring *rp;
	int orindex = sc->rindex;
	register struct xnarecv_ring *nrp = &sc->rring[orindex];
	struct mbuf *mp;

	/*
	 * Process all incoming packets on the receive ring. Stop if
	 * we get to the current receive index to avoid locking out
	 * the system, but give back one descriptor for each one we
	 * process to keep the device busy.
	 */
	for (index = ((sc->rlast+1)%nNXNARCV), rp = &sc->rring[index];
	     ((index != orindex) && (rp->status & ST_ROWN));
	     index = ++index % nNXNARCV, rp = &sc->rring[index]) {
		mb();

		/*
		 * Init the next descriptor(s) in line right away to make
		 * sure we always have buffers to string up. If we DON'T
		 * have mbuf's, we drop the current receive right here by
		 * re-posting it's mbuf. (This is to save our necks during
		 * burst-mode reception)
		 */
		sc->nrecv--;
		while (sc->nrecv < sc->nactv) {
			XNAMCLGET(mp);
			if (mp) {
				xnainitdesc(nrp, mp, sc);
				sc->nrecv++;
				sc->rindex = ++sc->rindex % nNXNARCV;
				nrp = &sc->rring[sc->rindex];
			}
			else {
#ifdef XNADEBUG
			     printf("XNA: no mbufs, dropping receive packet\n");
#endif
				/*
				 * The following line is a patch to avoid
				 * bumping up m_data a second time!
				 * (goes with NFS patch in xnainitdesc)
				 */
				(sc->mbuf_recv[rp->mbufindx])->m_data -= 2;
				xnainitdesc(nrp, sc->mbuf_recv[rp->mbufindx], sc);
				sc->nrecv++;
				sc->rindex = ++sc->rindex % nNXNARCV;
				nrp = &sc->rring[sc->rindex];
				sc->mbuf_recv[rp->mbufindx] = 0L;
				goto drop;
			}
		}
		
		/*
		 * Process current receive
		 */
#ifdef XNADEBUG
		printf("XNA: processing rentry %x\n",rp->mbufindx);
#endif
		if (rp->status & ST_RERR) {
#ifdef XNADEBUG
			printf("XNA: rp->status = ST_RERR (%x), dropping packet\n",rp->status);
#endif
			sc->is_if.if_ierrors++;
			m_freem(sc->mbuf_recv[rp->mbufindx]);
			sc->mbuf_recv[rp->mbufindx] = 0L;
		}
		else {
			/*
			 * Hand recv to upper levels
			 */
			sc->is_if.if_ipackets++;
			xnaread (sc, sc->mbuf_recv[rp->mbufindx], rp->len);
			sc->mbuf_recv[rp->mbufindx] = 0L;
		}
drop:
		sc->rlast = (++sc->rlast)%nNXNARCV;
		sc->nproc++;
	} /* endfor */
	}

	/*
	 * Process transmit/command ring
	 */
	{
	register struct xnacmd_ring *tp;
	register struct mbuf *mp;
	register int index;

	/*
	 * Process all outstanding transmits and commands completed by
	 * the port. Wakeup anyone waiting for command completions.
	 */
	for (index = ((sc->tlast+1)%nNXNACMD), tp = &sc->tring[index];
	     (sc->nxmit > 0 && (tp->status & ST_TOWN));
	     index = ++index % nNXNACMD, tp = &sc->tring[index]) {
		mb();
		/*
		 * Process cmd/xmit descriptor
		 */
#ifdef XNADEBUG
		printf("XNA: processing tentry %x\n",tp->mbufindx);
#endif
		mp = sc->mbuf_tofree[tp->mbufindx];
		sc->mbuf_tofree[tp->mbufindx] = 0L;
		if (tp->status & ST_CMD) {		/* Command */
			struct xnacmd_buf *xcmd;

			/*
			 * Grab command buffer address from mbuf wrapper.
			 * Process which posted command will free both the
			 * wrapper and the command buffer.
			 */
			xcmd = mtod(mp, struct xnacmd_buf *);
			switch (xcmd->opcode) {
				case CMD_RCCNTR:
					if (!(tp->status & ST_TERR)) {
						bzero(&sc->ctrblk,
					           sizeof(struct xnacmd_buf));
						sc->ztime = time.tv_sec;
					}
				case CMD_RDCNTR:
					/*
					 * Make certain we don't attempt to
					 * free the ctrblk region of the softc
					 */
					mp->m_data = mp->m_dat;
					mp->m_len = MLEN;
					m_freem(mp);
					break;
				case CMD_NOP:
					/*
					 * XNATIMEOUT timer went off before this
					 * command was processed; simply free
					 * the mbuf, since the person who
					 * issued this command has gone away.
					 */
#ifdef XNADEBUG
					printf ("xna%d: command timed out\n");
#endif
					m_freem(mp);
					break;
				case CMD_PARAM:
				case CMD_SYSID:
				case CMD_USTART:
				case CMD_UCHANGE:
				case CMD_USTOP:
					/*
		 			 * On command failure, alert caller
					 * by invalidating the command opcode.
					 */
					if (tp->status & ST_TERR) {
						printf ("xna%d: command failed, error code: 0x%x\n", unit, tp->error);
						xcmd->opcode = CMD_INVAL;
					} else
						xcmd->opcode = CMD_COMPLETE;
					break;
				default:
					xcmd->opcode = CMD_INVAL;
					break;
			}
		} else {				/* Transmit */
			if (tp->status & ST_TERR) {
#ifdef XNADEBUG
				printf("XNA: tp->status is ST_TERR\n");
#endif
				sc->is_if.if_oerrors++;
				m_freem(mp);
			} else {
				sc->is_if.if_opackets++;
				if (mp) m_freem(mp);
			}
		}
		sc->tlast = index;	/* Last cmd/xmit processed */
		sc->nxmit--;
		sc->nproc++;
	} /* endfor */
	}

	/*
	 * Dequeue next transmit request if interface is no longer busy.
	 */
	if (sc->nxmit <= 0) {
		sc->is_if.if_flags &= ~IFF_OACTIVE;
		xnastart( &(sc->is_if) );
	}

	/*
	 * Ring Release function. Tell port how many ring entries we've
	 * processed. Drop softc lock and return from interrupt.
	 */
#ifdef __mips
	addr->xnapd2 = sc->nproc;
#else /* __alpha */
	WRTCSR(LONG_32, ctlr, addr->xnapd2, sc->nproc);
#endif
	smp_unlock(&sc->lk_xna_softc);
	splx(s);
}

/*
 * XNA read routine. Pass input packets up to higher levels.
 */
xnaread (sc, m, len)
	register struct xna_softc *sc;
	struct mbuf *m;
	register int len;
{
	register struct ether_header *eptr;
	register int off, resid;
	struct ether_header eh;
	struct protosw *pr;
	struct ifqueue *inq;

	/*
	 * Deal with trailer protocol: if type is INET trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	m->m_len = len -= 4;	/* subtract 4 bytes CRC */
	eptr = mtod(m, struct ether_header *);
	eh = *eptr;
	eptr = &eh;

	eptr->ether_type = ntohs((u_short)eptr->ether_type);
	if ((eptr->ether_type >= ETHERTYPE_TRAIL &&
	    eptr->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)) {
		off = (eptr->ether_type - ETHERTYPE_TRAIL) * 512 +
		      sizeof(struct ether_header);
		if (off >= ETHERMTU)
			return;		/* sanity */
		eptr->ether_type =
			ntohs(*(short *)(mtod(m, vm_offset_t) + off));
		resid = ntohs(*(short *)(mtod(m, vm_offset_t) + off +2));
		if (off + resid > m->m_len)
			return;		/* sanity */
	} else {
		off = 0;
	}

	/*
	 * Pull packet off interface. (In the case of the XNA, only need
	 * to shuffle trailer data around, then hand up the address of the
	 * mbuf we had tied to the receive descriptor.) Off is nonzero if
	 * packet has trailing header; need to force this header information
	 * to be at the front, but we still have to drop the type and
	 * length which are at the front of any trailer data.
	 */
	{
		struct mbuf *m0 = m;

		m->m_pkthdr.rcvif = &sc->is_if;
		m->m_pkthdr.len = len;

		if (off) {
			int nbytes;
			int cnt = resid;
			struct mbuf *n;
			struct mbuf **mp = &m;

			while (cnt > 0) {
				MGET(n, M_DONTWAIT, MT_DATA);
				if (n == 0) {
					m_freem(m);
					return;
				}
				nbytes = MIN(MLEN, cnt);
				bcopy((mtod(m, vm_offset_t) + off),
				       mtod(n, vm_offset_t), nbytes);
				n->m_len = nbytes;
				off += nbytes;
				cnt -= nbytes;
				*mp = n;
				mp = &n->m_next;
			}

			/*
			 * Done hoisting trailer data; tie m0 onto the
			 * end of the chain, and drop trailer data length
			 */
			*mp = m0;
			m0->m_len -= resid;

			/*
			 * Adjust head of chain for trailer header
			 */
			m->m_data += 2 * sizeof (u_short);
			m->m_len -= 2 * sizeof (u_short);
		}

		/*
		 * Trim off ethernet header
		 */
		m0->m_data += sizeof (struct ether_header);
		m0->m_len -= sizeof (struct ether_header);
	}

	/*
	 * Subtract length of header from len
	 */
	m->m_pkthdr.len = len -= sizeof (struct ether_header);

        /*
         * If we can fit the incoming data into a small mbuf, swap
         * the data, and free the cluster mbuf.
         */
        if (off == 0 && len <= MINCLSIZE) {
                struct mbuf *nh, *n;
                int nhlen=0, nlen=0;

                /* Get an mbuf with a packet-header first */
                MGETHDR(nh, M_DONTWAIT, MT_DATA);

                if (nh) {
                        /* If one mbuf isn't enough, get another. */
                        if (len > MHLEN) {
                                MGET(n, M_DONTWAIT, MT_DATA);

                                if (n) {
                                        nhlen = MHLEN;
                                        nlen = len - nhlen;
                                } else
                                        m_freem(nh);
                        } else
                                nhlen = len;

                        /* Copy from the big mbufs to the small ones. */
                        if (nhlen) {
                                nh->m_pkthdr = m->m_pkthdr;
                                bcopy(mtod(m, vm_offset_t),
                                      mtod(nh, vm_offset_t), nhlen);
                                nh->m_len = nhlen;

                                if (nlen) {
                                        nh->m_next = n;
                                        bcopy(mtod(m, vm_offset_t) + nhlen,
                                              mtod(n, vm_offset_t), nlen);
                                        n->m_len = nlen;
                                }

                                /*
                                 * Let go off of the big one and setup
                                 * for the hand-off below.
                                 */
                                m_freem(m);
                                m = nh;
                        }
                }
        }

	/* Dispatch this packet */
	ether_input(&(sc->is_ed), (struct ether_header *)eptr, m, (off != 0));
}

/*
 * Process an ioctl request.
 */
xnaioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_int cmd;
	vm_offset_t data;
{
	register struct xna_softc *sc = &xna_softc[ifp->if_unit];
	register struct xnadevice *addr = &sc->xregs;
	register struct xnacmd_buf *xcmd;
	struct protosw *pr;
	struct mbuf *m;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	int s, error = 0;

	switch (cmd) {

        case SIOCENABLBACK:
        case SIOCDISABLBACK:
		if (cmd == SIOCENABLBACK)
                	ifp->if_flags |= IFF_LOOPBACK;
		else
                	ifp->if_flags &= ~IFF_LOOPBACK;
		if (ifp->if_flags & IFF_RUNNING) {

			/*
			 * Lock softc. Same comments as for "xnainit()"
			 */
			s = splimp();
			smp_lock(&sc->lk_xna_softc, LK_RETRY);
			if (m = xnamkparam(sc, ifp)) {
				xnacmd(sc, m);
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				xcmd = mtod(m, struct xnacmd_buf *);
				XNATIMEOUT(xcmd, CMD_PARAM); /* Wait */
				switch (xcmd->opcode) {
					case CMD_COMPLETE:
						m_freem(m);
						break;
					case CMD_INVAL:
						m_freem(m);
					case CMD_NOP:
					default:
						error = EINVAL;
						break;
				}
			} else {
				smp_unlock(&sc->lk_xna_softc);
				error = ENOBUFS;
				splx(s);
			}
		}
                break;
 
        case SIOCRPHYSADDR: 
                /*
                 * read default hardware address. Lock softc while accessing
		 * per-unit physical address info.
                 */
		s = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		bcopy(sc->is_dpaddr, ifd->default_pa, 6);
		bcopy(sc->is_addr, ifd->current_pa, 6);
		smp_unlock(&sc->lk_xna_softc);
		splx(s);
                break;
 
	case SIOCSPHYSADDR: 
		/* 
		 * Set physaddr. Lock softc while updating per-unit physical
		 * address, and for command processing as in "xnainit()".
		 */
		s = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		bcopy(ifr->ifr_addr.sa_data, sc->is_addr, 6);

		pfilt_newaddress(sc->is_ed.ess_enetunit, sc->is_addr);

		if (ifp->if_flags & IFF_RUNNING) {
			if (m = xnamkparam(sc, ifp)) {
				xnacmd(sc, m);
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				xcmd = mtod(m, struct xnacmd_buf *);
				XNATIMEOUT(xcmd, CMD_PARAM); /* Wait */
				switch (xcmd->opcode) {
					case CMD_COMPLETE:
						m_freem(m);
						break;
					case CMD_INVAL:
						m_freem(m);
					case CMD_NOP:
					default:
						error = EINVAL;
						break;
				}
			} else {
				smp_unlock(&sc->lk_xna_softc);
				error = ENOBUFS;
				splx(s);
			}
		} else {
			smp_unlock(&sc->lk_xna_softc);
			splx(s);
                	xnainit(ifp->if_unit);
		}
		/* If an IP address has been configured then an ARP packet
		 * must be broadcast to tell all hosts which currently have
		 * our address in their ARP tables to update their information.
		 */
#ifdef INET
		if(((struct arpcom *)ifp)->ac_ipaddr.s_addr)
			rearpwhohas((struct arpcom *)ifp,
				  &((struct arpcom *)ifp)->ac_ipaddr);
#endif
		break;

	case SIOCDELMULTI: 
	case SIOCADDMULTI: 
		/*
		 * Lock softc while updating per-unit multicast address
		 * list and for command processing as in "xnainit()".
		 */
		s = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		if (cmd == SIOCDELMULTI) {
			/*
			 * If we're deleting a multicast address, decrement
			 * the is_muse count and invalidate the address if
			 * count goes to zero.
			 */
			int i;
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(sc->is_multi[i],
				    ifr->ifr_addr.sa_data,6) == 0)
					break;
			}
			if ((i < NMULTI) && (--sc->is_muse[i] == 0))
				bcopy(etherbroadcastaddr,sc->is_multi[i],6);
			else {
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				goto done;
			}
		} else {
			/*
			 * If we're adding a multicat address, increment the
			 * is_muse count if it's already in our table, and
			 * return. Otherwise, add it to the table or return
			 * ENOBUFS if we're out of entries.
			 */
			int i, j = -1;
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(sc->is_multi[i],
				    ifr->ifr_addr.sa_data,6) == 0) {
					sc->is_muse[i]++;
					smp_unlock(&sc->lk_xna_softc);
					splx(s);
					goto done;
				}
				if ((j < 0) && (bcmp(sc->is_multi[i],
				    etherbroadcastaddr,6) == 0))
					j = i;
			}
			if (j < 0) {
				printf("xna%d: addmulti failed, multicast list full: %d\n",
					ifp->if_unit, NMULTI);
				smp_unlock(&sc->lk_xna_softc);
				error = ENOBUFS;
				splx(s);
				goto done;
			} else {
		    		bcopy(ifr->ifr_addr.sa_data,
				      sc->is_multi[j], 6);
		    		sc->is_muse[j]++;
			}
		}
		if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * If we've successfully init'ed the interface,
			 * issue a UCHANGE command to update the ethernet
			 * user's multicast address list. Otherwise, the
			 * list will be initialized upon the first call
			 * to "xnainit()".
			 */
			if (m = xnamkuser(sc, XNA_ETHERU, CMD_UCHANGE)) {
				xnacmd(sc, m);
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				xcmd = mtod(m, struct xnacmd_buf *);
				XNATIMEOUT(xcmd, CMD_UCHANGE);	/* Wait */
				switch (xcmd->opcode) {
					case CMD_COMPLETE:
						m_freem(m);
						break;
					case CMD_INVAL:
						m_freem(m);
					case CMD_NOP:
					default:
						error = EINVAL;
						break;
				}
			} else {
				smp_unlock(&sc->lk_xna_softc);
				error = ENOBUFS;
				splx(s);
			}
		}
		else {
			smp_unlock(&sc->lk_xna_softc);
			splx(s);
		}
		break;

	case SIOCRDZCTRS:
		/*
		 * Schedule a read-and-clear counters cmd, and report
		 * the most recent version of the counter block for
		 * this unit. xnaintr() will free m.
		 */
		if (ifp->if_flags & IFF_RUNNING) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m) {
				/*
				 * Point the data region of m to the
				 * ctrblk in the softc for this unit.
				 */
				m->m_data = (caddr_t)&sc->ctrblk;
				m->m_len = sizeof(sc->ctrblk.opcode) +
					   sizeof(struct _xnactrs);

				/*
				 * Lock softc while setting the opcode
				 * to prevent xnawatch() from blowing
				 * away the "CMD_RCCNTR" opcode.
				 */
				s = splimp();
				smp_lock(&sc->lk_xna_softc, LK_RETRY);
				sc->ctrblk.opcode = CMD_RCCNTR;
				xnacmd(sc, m);
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				xnagetctrs (sc, &ctr->ctr_ether, &sc->ctrblk);
				ctr->ctr_type = CTR_ETHER;
			} else
				error = ENOBUFS;
		}
		break;

	case SIOCRDCTRS:
		/*
		 * Copyin most recent contents of unit's counter
		 * block.
		 */
		if (ifp->if_flags & IFF_RUNNING) {
			xnagetctrs (sc, &ctr->ctr_ether, &sc->ctrblk);
			ctr->ctr_type = CTR_ETHER;
		}
		break;

	case SIOCSIFADDR:
		/*
		 * Init the interface if its not already running
		 */
		xnainit(ifp->if_unit);
		switch(ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			s = splimp();
			smp_lock(&lk_ifnet, LK_RETRY);
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			smp_unlock(&lk_ifnet);
			splx(s);
			break;
#endif
		default:
#ifdef XNADEBUG
			printf("XNA: xnaioctl: unsupported family\n");
#endif
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * If we've successfully init'ed the interface,
			 * issue a UCHANGE command to update the ethernet
			 * user's promiscuous bit based upon the interface
			 * flags.
			 */
			s = splimp();
			smp_lock(&sc->lk_xna_softc, LK_RETRY);
			if (m = xnamkuser(sc, XNA_ETHERU, CMD_UCHANGE)) {
				xnacmd(sc, m);
				smp_unlock(&sc->lk_xna_softc);
				splx(s);
				xcmd = mtod(m, struct xnacmd_buf *);
				XNATIMEOUT(xcmd, CMD_UCHANGE);	/* Wait */
				switch (xcmd->opcode) {
					case CMD_COMPLETE:
						m_freem(m);
						break;
					case CMD_INVAL:
						m_freem(m);
					case CMD_NOP:
					default:
						error = EINVAL;
						break;
				}
			} else {
				smp_unlock(&sc->lk_xna_softc);
				error = ENOBUFS;
				splx(s);
			}
		}
		break;

	case SIOCSIPMTU:
		{
		u_short ifmtu;

		bcopy(ifr->ifr_data, (u_char *)(&ifmtu), sizeof(u_short));	

		if (ifmtu > ETHERMTU || ifmtu < IP_MINMTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifmtu;
		}
		break;

	default:
		error = EINVAL;
	}
done:
	return (error);
}

/*
 * XNA watchdog timer (runs once per second). Schedule a "read counters"
 * command to update the per-unit counter block. Look at the "p_sbua"
 * counter in the port data block to determine if we should bump up
 * (or reduce) the number of active receive descriptors. Descriptors
 * will be activated/deactivated on the next interrupt service.
 */
xnawatch (unit)
	int unit;
{
	register struct xna_softc *sc = &xna_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	register struct xnapdb *xpdb = sc->xpdb;
	struct mbuf *m;
	static int callno = 0;
	int s;

	if (ifp->if_flags & IFF_RUNNING) {

		/*
		 * Lock softc while writing opcode and updating sbua count
		 */
		s = splimp();
		smp_lock(&sc->lk_xna_softc, LK_RETRY);
		/*
		 * Schedule a read counters cmd to update the counter
		 * block for this unit. xnaintr() will free m.
		 */
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m) {
			m->m_data = (caddr_t)&sc->ctrblk;
			m->m_len = sizeof(sc->ctrblk.opcode) +
				   sizeof(struct _xnactrs);
			sc->ctrblk.opcode = CMD_RDCNTR;
			xnacmd(sc, m);
		}

		/*
		 * Tweak sc->nactv based upon the "potential sbua"
		 * count provided by the adapter. We bump up the number
		 * of active descriptors if the p_sbua count has gone up
		 * during the last second, but bump it down if it has
		 * stayed the same for the last 60 seconds.
		 */
		if (*(u_int *)xpdb->p_sbua.lo >
		    *(u_int *)sc->xna_sbuacnt.lo) {
			if (sc->nactv < XNANMAX) {
				sc->nactv++;
				callno = 0;
			}
		} else {
			if ((++callno > 60) && (sc->nactv > XNANMIN)) {
				sc->nactv--;
				callno = 0;
			}
		}
		sc->xna_sbuacnt = xpdb->p_sbua;
		smp_unlock(&sc->lk_xna_softc);
		splx(s);

		/*
		 * Get collision count for ifnet readers (i.e. netstat, mon)
		 */
		ifp->if_collisions = *(u_int *)sc->ctrblk.xnactrs.single.lo +
			(*(u_int *)sc->ctrblk.xnactrs.multiple.lo * 2);
	}
	ifp->if_timer = 1;
}

xnainitdesc(rp, m, sc)
	register struct xnarecv_ring *rp;
	register struct mbuf *m;
	struct xna_softc *sc;
{
	vm_offset_t addr;

	/*
	 * Tie cluster mbuf to ring descriptor.
	 */

	/*
	 * Bump up m_data by 2 bytes
	 * to satisfy NFS, which needs longword-aligned data.
	 * (Otherwise, the 14 byte ethernet header would place the
	 * data at a non-longword alignment.)
	 */
	m->m_data += 2;
	if (svatophys(mtod(m, vm_offset_t), &addr) != KERN_SUCCESS) {
		panic("xnainitdesc: svatophys failed");
	}
#ifdef	__mips
	rp->bseg.xaddr_lo = addr;
#else /* __alpha */
	rp->bseg.xaddr_lo = (u_int)addr;
	rp->bseg.xaddr_hi = (u_short)((u_long)addr >> 32);
#endif
	rp->bseg.xmbz = 0;
	sc->mbuf_recv[rp->mbufindx] = m;
	mb();
	rp->status &= ~ST_ROWN;
}
	
struct	mbuf *
xnamkparam(sc, ifp)
	struct xna_softc *sc;
	struct ifnet *ifp;
{
	register struct mbuf *m;
	register struct xnacmd_buf *xparam;
	/*
	 * Allocate PARAM buffer
	 */
	XNAMCLGET(m);
	if (!m)
		return (0);
	xparam = mtod(m, struct xnacmd_buf *);
	m->m_len = sizeof(xparam->opcode) + sizeof(struct _xnaparam);
	bzero(xparam, m->m_len);
	xparam->opcode = CMD_PARAM;
	/*
	 * Copies 0's if the physical address hasn't been set. This signals
	 * the port to use the default physical address.
	 */
	bcopy(sc->is_addr, &xparam->xnaparam.apa, 6);
	bzero(xparam->xnaparam.bvc, 8);
	if (ifp->if_flags & IFF_LOOPBACK)
		xparam->xnaparam.loop_mode = PARAM_ELOOP;
	else
		xparam->xnaparam.loop_mode = PARAM_NOLOOP;
	xparam->xnaparam.flags = 0;

	/*
	 * Can't support the VMS-style binary quad-word date format.
	 * Just write zero's here.
	 */
	xparam->xnaparam.sysdate_lo = xparam->xnaparam.sysdate_hi = 0;
	return ((struct mbuf *)m);
}

struct	mbuf *
xnamkuser(sc, index, opcode)
	struct xna_softc *sc;
	int index, opcode;
{
	register struct mbuf *m;
	register struct xnacmd_buf *xuser;
	register int cnt;
	int i;
	/*
	 * Allocate USER buffer
	 */
	XNAMCLGET(m);
	if (!m)
		return (0);
	xuser = mtod(m, struct xnacmd_buf *);
	m->m_len = sizeof(xuser->opcode) + sizeof(struct _xnaustart);
	bzero(xuser, m->m_len);
	xuser->opcode = opcode;
	switch (index) {
		/* 
		 * Only handle the "generic ethernet user" for now
		 */
		case XNA_ETHERU:
			xuser->xnaustart.sap_ptt = ETHERTYPE_IP;
			xuser->xnaustart.mode =
			    (USTART_UNK|USTART_BDC|USTART_BAD);
			bcopy(sc->is_addr,&xuser->xnaustart.user_phys,6);
			xuser->xnaustart.user_phys.xlen =
			    ETHERMTU + sizeof(struct ether_header) + 4;
			xuser->xnaustart.addr_alloc = NMULTI;
			cnt = 0;
			for (i = 0; i < NMULTI; i++) { 
				if (sc->is_muse[i] > 0)
					bcopy(sc->is_multi[i],
					  &xuser->xnaustart.multi_addr[cnt++],6);
			}
			xuser->xnaustart.addr_len = cnt;

			if (sc->is_if.if_flags & IFF_PROMISC)
				xuser->xnaustart.mode |= USTART_PROM;

			if (sc->is_if.if_flags & IFF_ALLMULTI)
                                xuser->xnaustart.mode |= USTART_AMC;
			break;
		default:
			m_freem(xuser);
			return (0);
	}
	return ((struct mbuf *)m);
}

xnagetctrs (sc, ctr, xcmd)
	register struct xna_softc *sc;
	register struct estat *ctr;
	register struct xnacmd_buf *xcmd;
{
	register int seconds;

	/*
	 * Fill out the ethernet counters based upon the information
	 * returned by the CMD_{RDC,RCC}CNTR command. This is pretty
	 * disgusting, but necessary...
	 */
	bzero (ctr, sizeof(struct estat));

	seconds = time.tv_sec - sc->ztime;
	if (seconds & 0xffff0000)
	    ctr->est_seconds = 0xffff;
	else
	    ctr->est_seconds = seconds & 0xffff;
	if (*(u_int *)xcmd->xnactrs.bytercvd.hi)
	    ctr->est_bytercvd = 0xffffffff;
	else
	    ctr->est_bytercvd = *(u_int *)xcmd->xnactrs.bytercvd.lo;
	if (*(u_int *)xcmd->xnactrs.bytesent.hi)
	    ctr->est_bytesent = 0xffffffff;
	else
	    ctr->est_bytesent = *(u_int *)xcmd->xnactrs.bytesent.lo;
	if (*(u_int *)xcmd->xnactrs.blokrcvd.hi)
	    ctr->est_blokrcvd = 0xffffffff;
	else
	    ctr->est_blokrcvd = *(u_int *)xcmd->xnactrs.blokrcvd.lo;
	if (*(u_int *)xcmd->xnactrs.bloksent.hi)
	    ctr->est_bloksent = 0xffffffff;
	else
	    ctr->est_bloksent = *(u_int *)xcmd->xnactrs.bloksent.lo;
	if (*(u_int *)xcmd->xnactrs.mbytercvd.hi)
	    ctr->est_mbytercvd = 0xffffffff;
	else
	    ctr->est_mbytercvd = *(u_int *)xcmd->xnactrs.mbytercvd.lo;
	if (*(u_int *)xcmd->xnactrs.mblokrcvd.hi)
	    ctr->est_mblokrcvd = 0xffffffff;
	else
	    ctr->est_mblokrcvd = *(u_int *)xcmd->xnactrs.mblokrcvd.lo;
	if (*(u_int *)xcmd->xnactrs.mbytesent.hi)
	    ctr->est_mbytesent = 0xffffffff;
	else
	    ctr->est_mbytesent = *(u_int *)xcmd->xnactrs.mbytesent.lo;
	if (*(u_int *)xcmd->xnactrs.mbloksent.hi)
	    ctr->est_mbloksent = 0xffffffff;
	else
	    ctr->est_mbloksent = *(u_int *)xcmd->xnactrs.mbloksent.lo;
	if (*(u_int *)xcmd->xnactrs.deferred.hi)
	    ctr->est_deferred = 0xffffffff;
	else
	    ctr->est_deferred = *(u_int *)xcmd->xnactrs.deferred.lo;
	if (*(u_int *)xcmd->xnactrs.single.hi)
	    ctr->est_single = 0xffffffff;
	else
	    ctr->est_single = *(u_int *)xcmd->xnactrs.single.lo;
	if (*(u_int *)xcmd->xnactrs.multiple.hi)
	    ctr->est_multiple = 0xffffffff;
	else
	    ctr->est_multiple = *(u_int *)xcmd->xnactrs.multiple.lo;
	if ((*(u_int *)xcmd->xnactrs.collis.hi) ||
	    (*(u_int *)xcmd->xnactrs.collis.lo & 0xffff0000))
	    ctr->est_collis = 0xffff;
	else
	    ctr->est_collis = *(u_short *)xcmd->xnactrs.collis.lo;
	if ((*(u_int *)xcmd->xnactrs.unrecog.hi) ||
	    ((*(u_int *)xcmd->xnactrs.unrecog.lo) & 0xffff0000))
	    ctr->est_unrecog = 0xffff;
	else
	    ctr->est_unrecog = *(u_short *)xcmd->xnactrs.unrecog.lo;
	if ((*(u_int *)xcmd->xnactrs.overrun.hi) ||
	    (*(u_int *)xcmd->xnactrs.overrun.lo & 0xffff0000))
	    ctr->est_overrun = 0xffff;
	else
	    ctr->est_overrun = *(u_short *)xcmd->xnactrs.overrun.lo;
	if ((*(u_int *)xcmd->xnactrs.sysbuf.hi) ||
	    (*(u_int *)xcmd->xnactrs.sysbuf.lo & 0xffff0000))
	    ctr->est_sysbuf = 0xffff;
	else
	    ctr->est_sysbuf = *(u_short *)xcmd->xnactrs.sysbuf.lo;
	if ((*(u_int *)xcmd->xnactrs.userbuf.hi) ||
	    (*(u_int *)xcmd->xnactrs.userbuf.lo & 0xffff0000))
	    ctr->est_userbuf = 0xffff;
	else
	    ctr->est_userbuf = *(u_short *)xcmd->xnactrs.userbuf.lo;
	if ((*(u_int *)xcmd->xnactrs.sendfail_retry.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_retry.lo)) {
	    ctr->est_sendfail_bm |= 0x01;
	    ctr->est_sendfail =
	    			*(u_int *)xcmd->xnactrs.sendfail_retry.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.sendfail_carrier.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_carrier.lo)) {
	    ctr->est_sendfail_bm |= 0x02;
	    ctr->est_sendfail +=
	    			*(u_int *)xcmd->xnactrs.sendfail_carrier.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.sendfail_short.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_short.lo)) {
	    ctr->est_sendfail_bm |= 0x04;
	    ctr->est_sendfail +=
	    			*(u_int *)xcmd->xnactrs.sendfail_short.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.sendfail_open.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_open.lo)) {
	    ctr->est_sendfail_bm |= 0x08;
	    ctr->est_sendfail +=
	    			*(u_int *)xcmd->xnactrs.sendfail_open.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.sendfail_long.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_long.lo)) {
	    ctr->est_sendfail_bm |= 0x10;
	    ctr->est_sendfail +=
	    			*(u_int *)xcmd->xnactrs.sendfail_long.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.sendfail_defer.hi) ||
	    (*(u_int *)xcmd->xnactrs.sendfail_defer.lo)) {
	    ctr->est_sendfail_bm |= 0x20;
	    ctr->est_sendfail +=
	    			*(u_int *)xcmd->xnactrs.sendfail_defer.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.recvfail_crc.hi) ||
	    (*(u_int *)xcmd->xnactrs.recvfail_crc.lo)) {
	    ctr->est_recvfail_bm |= 0x01;
	    ctr->est_recvfail =
	    			*(u_int *)xcmd->xnactrs.recvfail_crc.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.recvfail_frame.hi) ||
	    (*(u_int *)xcmd->xnactrs.recvfail_frame.lo)) {
	    ctr->est_recvfail_bm |= 0x02;
	    ctr->est_recvfail +=
	    			*(u_int *)xcmd->xnactrs.recvfail_frame.lo;
	}
	if ((*(u_int *)xcmd->xnactrs.recvfail_long.hi) ||
	    (*(u_int *)xcmd->xnactrs.recvfail_long.lo)) {
	    ctr->est_recvfail_bm |= 0x04;
	    ctr->est_recvfail +=
	    			*(u_int *)xcmd->xnactrs.recvfail_long.lo;
	}
}

xnaprintregs(ctlr)
struct controller *ctlr;
{
	struct	xna_softc *sc = &xna_softc[ctlr->ctlr_num];
	u_int	xber, xpst, xpd1, xpd2, xpud;

	printf("xna%d: register dump\n",ctlr->ctlr_num);
	xber = RDCSR(LONG_32, ctlr, sc->xregs.xber_xmi);
	xpst = RDCSR(LONG_32, ctlr, sc->xregs.xnapst);
	xpd1 = RDCSR(LONG_32, ctlr, sc->xregs.xnapd1);
	xpd2 = RDCSR(LONG_32, ctlr, sc->xregs.xnapd2);
	xpud = RDCSR(LONG_32, ctlr, sc->xregs.xnapud);

	printf("xna%d: xber = 0x%x, xpst = 0x%x\n",ctlr->ctlr_num, xber, xpst);
	printf("      xpd1 = 0x%x, xpd2 = 0x%x, xpud = 0x%x\n",xpd1,xpd2,xpud);
}

#ifdef XNADEBUG
dumpmbuf(m)
struct mbuf *m;
{
	int i;
	u_int *ptr;

	printf("Dumping contents of mbuf 0x%x \n", m);
	printf("m_next = %x  m_nextpkt = %x \n", m->m_next, m->m_nextpkt);
	printf("m_type = 0x%x m_len = 0x%x \n", m->m_type, m->m_len);
	printf("m_flags = 0x%x %s %s %s \n", m->m_flags, 
		(m->m_flags & M_EXT ? "M_EXT" : ""), 
		(m->m_flags & M_PKTHDR ? "M_PKTHDR" : ""), 
		(m->m_flags & M_EOR ? "M_EOR" : "")); 
	DELAY(1000000);
	printf("m_data = 0x%x\n", m->m_data);
	printf("m_ipq[0] = 0x%x, m_ipq[1] = 0x%x\n", m->m_ipq[0], m->m_ipq[1]);
	printf("m_ipq[2] = 0x%x, m_ipq[3] = 0x%x\n", m->m_ipq[2], m->m_ipq[3]);
	DELAY(1000000);
	if (m->m_flags & M_PKTHDR) {
		printf("Packet Header Information and data: \n");
		printf("m_pkthdr.len = 0x%x m_pkthdr.rcvif = 0x%x\n", 
			m->m_pkthdr.len, (u_long)m->m_pkthdr.rcvif);
		DELAY(1000000);
		if (!(m->m_flags & M_EXT)) {
			printf("Packet Header Data (10 longwords): \n");
			ptr = (u_int *)m->m_data;
			for (i = 0; i < 10; i++, ptr++)
				printf(" %d: 0x%x \n", i, *ptr);
			DELAY(1000000);
		}
	}
	if (m->m_flags & M_EXT) {
		printf("External Packet Information and data:\n");
		printf("m_ext.ext_buf = 0x%x, m_ext.ext_free = 0x%x\n",
			m->m_ext.ext_buf, m->m_ext.ext_free);
		printf("m_ext.ext_size = 0x%x m->m_ext.ext_arg = 0x%x\n",
			m->m_ext.ext_size, m->m_ext.ext_arg);
		DELAY(1000000);
		printf("External Packet Data (10 longwords): \n");
		ptr = (u_int *)m->m_data;
		for (i = 0; i < 10; i++, ptr++)
			printf(" %d: 0x%x \n", i, *ptr);
		DELAY(1000000);
	}
	if (!(m->m_flags & M_EXT) && !(m->m_flags & M_PKTHDR)) {
		printf("Local Mbuf Packet Data (10 longwords): \n");
		ptr = (u_int *)m->m_data;
		for (i = 0; i < 10; i++, ptr++)
			printf(" %d: 0x%x \n", i, *ptr);
		DELAY(1000000);
	}
}
#endif

#endif
