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
static char *sccsid = "@(#)$RCSfile: etherxdr.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1993/05/18 20:55:34 $";
#endif
/*
 */
/*
 * OSF/1 Release 1.0
 */

/* 
 * Copyright (c) 1988,1990 by Sun Microsystems, Inc.
 * @(#) from SUN 1.9
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/ether.h>

xdr_etherstat(xdrsp, ep)
	XDR *xdrsp;
	struct etherstat *ep;
{
	int i;
	
	if (!xdr_timeval(xdrsp, &ep->e_time))
		return (0);
	if (!xdr_u_long(xdrsp, (u_long *) &ep->e_bytes))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_packets))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bcast))
		return (0);
	for (i = 0; i < NBUCKETS; i++)
		if (!xdr_u_long(xdrsp, &ep->e_size[i]))
			return (0);
	for (i = 0; i < NPROTOS; i++)
		if (!xdr_u_long(xdrsp, &ep->e_proto[i]))
			return (0);
	return (1);
}


xdr_etheraddrs(xdrsp, ep)
	register XDR *xdrsp;
	register struct etheraddrs *ep;
{
	if (!xdr_timeval(xdrsp, &ep->e_time))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bytes))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_packets))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bcast))
		return (0);
	if (!xdr_reference(xdrsp, (char **) &ep->e_addrs,
	    HASHSIZE*sizeof(struct etherhmem *), xdr_etherhtable))
		return (0);
	return (1);
}

xdr_etherhtable(xdrs, hp)
	register XDR *xdrs;
	register struct etherhmem **hp;
{
	int i;
	
	for (i = 0; i < HASHSIZE; i++)
		if (!xdr_etherhmem(xdrs, &hp[i]))
			return (0);
	return (1);
}

xdr_etherhmem(xdrs, hp)
	register XDR *xdrs;
	register struct etherhmem **hp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	int more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct etherhmem **nxt;

	for (;;) {
		more_elements = (*hp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the nxt object
		 * before we free the current object ...
		 */
		if (freeing)
			nxt = &((*hp)->ht_nxt); 
		if (! xdr_reference(xdrs, (char **) hp, sizeof(struct etherhmem),
		    xdr_etherhbody))
			return (FALSE);
		hp = (freeing) ? nxt : &((*hp)->ht_nxt);
	}
}

/* 
 * body of an etherhmem
 */
bool_t
xdr_etherhbody(xdrs, hp)
	XDR *xdrs;
	struct etherhmem *hp;
{
	if (!xdr_int(xdrs, (int *) &hp->ht_addr))
		return FALSE;
	if (!xdr_u_int(xdrs, (u_int *) &hp->ht_cnt))
		return FALSE;
	return(TRUE);
}

xdr_addrmask(xdrs, ap)
	XDR *xdrs;
	struct addrmask *ap;
{
	if (xdr_int(xdrs, &ap->a_addr) == 0)
		return 0;
	if (xdr_int(xdrs, &ap->a_mask) == 0)
		return 0;
	return (1);
}
