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
static char *rcsid = "@(#)$RCSfile: print-ntp.c,v $ $Revision: 1.1.2.2 $ (DEC) $Date: 1993/06/22 18:45:54 $";
#endif
/*
 * Copyright (c) 1988-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Format and print ntp packets.
 *	By Jeffrey Mogul/DECWRL
 *	loosely based on print-bootp.c
 */

/*
 * Based on:
 * static char rcsid[] = "print-ntp.c,v 1.7 92/01/04 01:45:16 leres Exp $ (LBL)";
 */

#include <stdio.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <strings.h>
#include <ctype.h>

#include "interface.h"
#include "addrtoname.h"
#include "ntp.h"

/*
 * Print ntp requests
 */
void
ntp_print(bp, length)
	register struct ntpdata *bp;
	int length;
{
	u_char *ep;
	int mode, version, leapind;
	static char rclock[5];

#define TCHECK(var, l) if ((u_char *)&(var) > ep - l) goto trunc

	/* Note funny sized packets */
	if (length != sizeof(struct ntpdata))
		(void)printf(" [len=%d]", length);

	/* 'ep' points to the end of avaible data. */
	ep = (u_char *)snapend;

	TCHECK(bp->status, sizeof(bp->status));

	version = (bp->status & VERSIONMASK) >> 3;
	printf(" v%d", version);

	leapind = bp->status & LEAPMASK;
	switch (leapind) {

	case NO_WARNING:
		break;

	case PLUS_SEC:
		fputs(" +1s", stdout);
		break;

	case MINUS_SEC:
		fputs(" -1s", stdout);
		break;
	}

	mode = bp->status & MODEMASK;
	switch (mode) {

	case MODE_UNSPEC:	/* unspecified */
		fputs(" unspec", stdout);
		break;

	case MODE_SYM_ACT:	/* symmetric active */
		fputs(" sym_act", stdout);
		break;

	case MODE_SYM_PAS:	/* symmetric passive */
		fputs(" sym_pas", stdout);
		break;

	case MODE_CLIENT:	/* client */
		fputs(" client", stdout);
		break;

	case MODE_SERVER:	/* server */
		fputs(" server", stdout);
		break;

	case MODE_BROADCAST:	/* broadcast */
		fputs(" bcast", stdout);
		break;

	case MODE_RES1:		/* reserved */
		fputs(" res1", stdout);
		break;

	case MODE_RES2:		/* reserved */
		fputs(" res2", stdout);
		break;

	}

	TCHECK(bp->stratum, sizeof(bp->stratum));
	printf(" strat %d", bp->stratum);

	TCHECK(bp->ppoll, sizeof(bp->ppoll));
	printf(" poll %d", bp->ppoll);

	/* Can't TCHECK bp->precision bitfield so bp->distance + 0 instead */
	TCHECK(bp->distance, 0);
	printf(" prec %d", bp->precision);

	if (!vflag)
		return;

	TCHECK(bp->distance, sizeof(bp->distance));
	fputs(" dist ", stdout);
	p_sfix(&bp->distance);

	TCHECK(bp->dispersion, sizeof(bp->dispersion));
	fputs(" disp ", stdout);
	p_sfix(&bp->dispersion);

	TCHECK(bp->refid, sizeof(bp->refid));
	fputs(" ref ", stdout);
	/* Interpretation depends on stratum */
	switch (bp->stratum) {

	case UNSPECIFIED:
	case PRIM_REF:
		strncpy(rclock, (char *)&(bp->refid), 4);
		rclock[4] = '\0';
		fputs(rclock, stdout);
		break;

	case INFO_QUERY:
		printf("%s INFO_QUERY", ipaddr_string(&(bp->refid)));
		/* this doesn't have more content */
		return;

	case INFO_REPLY:
		printf("%s INFO_REPLY", ipaddr_string(&(bp->refid)));
		/* this is too complex to be worth printing */
		return;

	default:
		printf("%s", ipaddr_string(&(bp->refid)));
		break;
	}

	TCHECK(bp->reftime, sizeof(bp->reftime));
	putchar('@');
	p_ntp_time(&(bp->reftime));

	TCHECK(bp->org, sizeof(bp->org));
	fputs(" orig ", stdout);
	p_ntp_time(&(bp->org));

	TCHECK(bp->rec, sizeof(bp->rec));
	fputs(" rec ", stdout);
	p_ntp_delta(&(bp->org), &(bp->rec));

	TCHECK(bp->xmt, sizeof(bp->xmt));
	fputs(" xmt ", stdout);
	p_ntp_delta(&(bp->org), &(bp->xmt));

	return;

trunc:
	fputs(" [|ntp]", stdout);
#undef TCHECK
}

p_sfix(sfp)
	register struct s_fixedpt *sfp;
{
	register int i;
	register int f;
	register float ff;

	i = ntohs(sfp->int_part);
	f = ntohs(sfp->fraction);
	ff = f / 65536.0;	/* shift radix point by 16 bits */
	f = ff * 1000000.0;	/* Treat fraction as parts per million */
	printf("%d.%06d", i, f);
}

#define	FMAXINT	(4294967296.0)	/* floating point rep. of MAXINT */

p_ntp_time(lfp)
	register struct l_fixedpt *lfp;
{
	register long i;
	register unsigned long uf;
	register unsigned long f;
	register float ff;

	i = ntohl(lfp->int_part);
	uf = ntohl(lfp->fraction);
	ff = uf;
	if (ff < 0.0)		/* some compilers are buggy */
		ff += FMAXINT;
	ff = ff / FMAXINT;	/* shift radix point by 32 bits */
	f = ff * 1000000000.0;	/* treat fraction as parts per billion */
	printf("%lu.%09d", i, f);
}

/* Prints time difference between *lfp and *olfp */
p_ntp_delta(olfp, lfp)
	register struct l_fixedpt *olfp;
	register struct l_fixedpt *lfp;
{
	register long i;
	register unsigned long uf;
	register unsigned long ouf;
	register unsigned long f;
	register float ff;
	int signbit;

	i = ntohl(lfp->int_part) - ntohl(olfp->int_part);

	uf = ntohl(lfp->fraction);
	ouf = ntohl(olfp->fraction);

	if (i > 0) {		/* new is definitely greater than old */
		signbit = 0;
		f = uf - ouf;
		if (ouf > uf)	/* must borrow from high-order bits */
			i -= 1;
	} else if (i < 0) {	/* new is definitely less than old */
		signbit = 1;
		f = ouf - uf;
		if (uf > ouf)	/* must carry into the high-order bits */
			i += 1;
		i = -i;
	} else {		/* int_part is zero */
		if (uf > ouf) {
			signbit = 0;
			f = uf - ouf;
		} else {
			signbit = 1;
			f = ouf - uf;
		}
	}

	ff = f;
	if (ff < 0.0)		/* some compilers are buggy */
		ff += FMAXINT;
	ff = ff / FMAXINT;	/* shift radix point by 32 bits */
	f = ff * 1000000000.0;	/* treat fraction as parts per billion */
	if (signbit)
		putchar('-');
	else
		putchar('+');
	printf("%d.%09d", i, f);
}
