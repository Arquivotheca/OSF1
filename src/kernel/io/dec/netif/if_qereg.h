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
#ifndef _IF_QEREG_H_
#define _IF_QEREG_H_
/*
 * Digital Q-BUS to NI Adapter 
 */
struct qedevice {
	u_short	qe_sta_addr[2]; 	/* Station address (actually 6 	*/
	u_short	qe_rcvlist_lo; 		/* Recieve list lo address 	*/
	u_short	qe_rcvlist_hi; 		/* Recieve list hi address 	*/
	u_short	qe_xmtlist_lo;		/* Transmit list lo address 	*/
	u_short	qe_xmtlist_hi;		/* Transmit list hi address 	*/
	u_short	qe_vector;		/* Interrupt vector 		*/
	u_short	qe_csr;			/* Command and Status Register 	*/
};

/*
 * Command and status bits (csr)
 */
#define QE_RCV_ENABLE	0x0001		/* Receiver enable		*/
#define QE_RESET	0x0002		/* Software reset		*/
#define QE_NEX_MEM_INT	0x0004		/* Non existant mem interrupt	*/
#define QE_LOAD_ROM	0x0008		/* Load boot/diag from rom	*/
#define QE_XL_INVALID	0x0010		/* Transmit list invalid	*/
#define QE_RL_INVALID	0x0020		/* Receive list invalid		*/
#define QE_INT_ENABLE	0x0040		/* Interrupt enable		*/
#define QE_XMIT_INT	0x0080		/* Transmit interrupt		*/
#define QE_ILOOP 	0x0100		/* Internal loopback		*/
#define QE_ELOOP	0x0200		/* External loopback		*/
#define QE_STIM_ENABLE	0x0400		/* Sanity timer enable		*/
#define QE_POWERUP	0x1000		/* Tranceiver power on		*/
#define QE_CARRIER	0x2000		/* Carrier detect		*/
#define QE_RCV_INT	0x8000		/* Receiver interrupt		*/

/*
 * Transmit and receive ring discriptor ---------------------------
 *
 * The QNA uses the flag, status1 and the valid bit as a handshake/semiphore
 * mechinism. 
 * 
 * The flag word is written on ( bits 15,15 set to 1 ) when it reads the
 * descriptor. If the valid bit is set it considers the address to be valid.
 * When it uses the buffer pointed to by the valid address it sets status word
 * one.
 */
struct qe_ring	{
	u_short qe_flag;		/* Buffer utilization flags	*/
	u_short qe_addr_hi:6,		/* Hi order bits of buffer addr	*/
	      qe_odd_begin:1,		/* Odd byte begin and end (xmit)*/
	      qe_odd_end:1,
	      qe_fill1:4,
	      qe_setup:1,		/* Setup packet			*/
	      qe_eomsg:1,		/* End of message flag		*/
	      qe_chain:1,		/* Chain address instead of buf */
	      qe_valid:1;		/* Address field is valid	*/
	u_short qe_addr_lo;		/* Low order bits of address	*/
	short qe_buf_len;		/* Negative buffer length	*/
	u_short qe_status1;		/* Status word one		*/
	u_short qe_status2;		/* Status word two		*/
};

/*
 * Status word definations (receive)
 *	word1
 */
#define QE_OVF			0x0001	/* Receiver overflow		*/
#define QE_CRCERR		0x0002	/* CRC error			*/
#define QE_FRAME		0x0004	/* Framing alignment error	*/
#define QE_SHORT		0x0008	/* Packet size < 10 bytes	*/
#define QE_RBL_HI		0x0700	/* Hi bits of receive len	*/
#define QE_RUNT			0x0800	/* Runt packet			*/
#define QE_DISCARD		0x1000	/* Discard the packet		*/
#define QE_ESETUP		0x2000	/* Looped back setup or eloop	*/
#define QE_ERROR		0x4000 	/* Receiver error		*/
#define QE_LASTNOT		0x8000	/* Not the last in the packet	*/
/*	word2								*/
#define QE_RBL_LO		0x00ff	/* Low bits of receive len	*/

/*
 * Status word definations (transmit)
 *	word1
 */
#define QE_CCNT			0x00f0	/* Collision count this packet	*/
#define QE_FAIL			0x0100	/* Heart beat check failure	*/
#define QE_ABORT		0x0200	/* Transmission abort		*/
#define QE_STE16		0x0400	/* Sanity timer default on	*/
#define QE_NOCAR		0x0800	/* No carrier			*/
#define QE_LOSS			0x1000	/* Loss of carrier while xmit	*/
#define QE_TERROR		0x4000  /* Xmit error */
/*	word2								*/
#define QE_TDR			0x3fff	/* Time domain reflectometry	*/

/*
 * General constant definations
 */
#define QEALLOC 		0	/* Allocate an mbuf		*/
#define QENOALLOC		1	/* No mbuf allocation		*/
#define QEDEALLOC		2	/* Release an mbuf chain	*/

#define QE_NOTYET		0x8000	/* Descriptor not in use yet	*/
#define QE_INUSE		0x4000	/* Descriptor being used by QNA	*/
#define QE_MASK			0xc000	/* Lastnot/error/used mask	*/

/*
 * Setup packet definitions byte count field
 */
#define QE_MULTICAST		0x0001	/* Recieve all multicast packets */
#define QE_PROMISCUOUS		0x0002	/* Recieve all packets */
#define QE_LED1			0x0004	/* Turns off led #1 */
#define QE_LED2			0x0008	/* Turns off led #2 */
#define QE_LED3			0x000c	/* Turns off led #3 */


#endif
