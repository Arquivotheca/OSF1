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
static char	*sccsid = "@(#)$RCSfile: ping.c,v $ $Revision: 4.2.4.3 $ (DEC) $Date: 1993/10/08 15:46:58 $";
#endif 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
 *
 * Revision 2.4  90/06/22  22:24:51  devrcs
 *      modify rcsid
 *      [90/06/08  09:56:16  jrieden]
 *
 *      removed commented out setlinebuf
 *      added rcsid
 *      [90/06/08  09:43:47  jrieden]
 *
 * Revision 2.3  90/05/24  21:58:48  devrcs
 *      initial version
 *      [90/05/14  21:07:25  jrieden]
 *
 */
/*
 * ping.c
 *
 *	Modification History:
 *
 * 01-Apr-91	Fred Canter
 *	MIPS C 2.20+, changes for -std
 *
 */
/*
 * IBM CONFIDENTIAL
 * Copyright International Business Machines Corp. 1989
 * Unpublished Work
 * All Rights Reserved
 * Licensed Material - Property of IBM
 */
/*
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */


/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */
#include <sys/secdefines.h>
#if SEC_BASE
#include <sys/security.h>
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - 8)/* max packet size */
#define	MAXWAIT		10		/* max seconds to wait for response */
#define	NROUTES		9		/* number of record route slots */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* various options */
int options;
#define	F_FLOOD		0x001
#define	F_INTERVAL	0x002
#define	F_NUMERIC	0x004
#define	F_PINGFILLED	0x008
#define	F_QUIET		0x010
#define	F_RROUTE	0x020
#define	F_SO_DEBUG	0x040
#define	F_SO_DONTROUTE	0x080
#define	F_VERBOSE	0x100

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

struct sockaddr whereto;	/* who to ping */
int datalen = DEFDATALEN;
int s;				/* socket file descriptor */
u_char outpack[MAXPACKET];
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
char *hostname;
int ident;			/* process id to identify our packets */

/* counters */
long npackets;			/* max packets to transmit */
long nreceived;			/* # of packets we got back */
long nrepeats;			/* number of duplicates */
long ntransmitted;		/* sequence # for outbound packets = #sent */
int interval = 1;		/* interval between packets */

/* timing */
int timing;			/* flag to do timing */
long tmin = LONG_MAX;		/* minimum round trip time */
long tmax;			/* maximum round trip time */
u_long tsum;			/* sum of all times, for doing average */

u_int inet_addr();
char *inet_ntoa(), *pr_addr();
void catcher(), finish();

#include <nl_types.h>
#include <locale.h>
#include "ping_msg.h"
#define MSGSTR(n,s) catgets(catd,MS_PING,n,s)
nl_catd catd;

main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno, optind;
	extern char *optarg;
	struct timeval timeout;
	struct hostent *hp;
	struct sockaddr_in *to;
	struct protoent *proto;
	register int i;
	int ch, fdmask, hold, packlen, preload;
	u_char *datap, *packet;
	char *target, hnamebuf[MAXHOSTNAMELEN], *malloc();
#ifdef IP_OPTIONS
	char rspace[3 + 4 * NROUTES + 1];	/* record route space */
#endif
	setlocale(LC_ALL, "");
        catd = NLcatopen(MF_PING,NL_CAT_LOCALE);
#if SEC_BASE
        set_auth_parameters(argc, argv);
        initprivs();

        if (!authorized_user("ping")) {
                fprintf(stderr, MSGSTR(PING_AUTH, "%s: need ping authorization\n"),
                        "ping");
                exit(1);
        }
        if (!forcepriv(SEC_REMOTE)) {
                fprintf(stderr, MSGSTR(PING_PRIV, "%s: insufficient privileges\n"),
                        "ping");
                exit(1);
        }
#endif

	preload = 0;
	datap = &outpack[8 + sizeof(struct timeval)];
	while ((ch = getopt(argc, argv, "Rc:dfh:i:l:np:qrs:v")) != EOF)
		switch(ch) {
		case 'c':
			npackets = atoi(optarg);
			if (npackets <= 0) {
				fprintf(stderr,MSGSTR( PING_BAD_NUM, 
				    "ping: bad number of packets to transmit.\n"));
				exit(1);
			}
			break;
		case 'd':
			options |= F_SO_DEBUG;
			break;
		case 'f':
			if (getuid()) {
				(void)fprintf(stderr, MSGSTR( PING_ROOT, 
				    "ping: you must be root to use either -f or -l option.\n"));
				exit(1);
			}
			options |= F_FLOOD;
			setbuf(stdout, (char *)NULL);
			break;
		case 'i':		/* wait between sending packets */
			interval = atoi(optarg);
			if (interval <= 0) {
				(void)fprintf(stderr, MSGSTR( PING_TIMING,
				    "ping: bad timing interval.\n"));
				exit(1);
			}
			options |= F_INTERVAL;
			break;
		case 'l':
			if (getuid()) {
				(void)fprintf(stderr, MSGSTR( PING_ROOT, 
				    "ping: you must be root to use either -f or -l option.\n"));
				exit(1);
			}
			preload = atoi(optarg);
			if (preload < 0) {
				(void)fprintf(stderr, MSGSTR(PING_BAD_PRELOAD,
				    "ping: bad preload value.\n"));
				exit(1);
			}
			break;
		case 'n':
			options |= F_NUMERIC;
			break;
		case 'p':		/* fill buffer with user pattern */
			options |= F_PINGFILLED;
			fill((char *)datap, optarg);
				break;
		case 'q':
			options |= F_QUIET;
			break;
		case 'R':
			options |= F_RROUTE;
			break;
		case 'r':
			options |= F_SO_DONTROUTE;
			break;
		case 's':		/* size of packet to send */
			datalen = atoi(optarg);
			if (datalen > MAXPACKET) {
				(void)fprintf(stderr, MSGSTR( PKTSZERR,
				    "ping: packet size too large.\n"));
				exit(1);
			}
			if (datalen <= 0) {
				(void)fprintf(stderr, MSGSTR( PING_ILLEGAL_PACK,
				    "ping: illegal packet size.\n"));
				exit(1);
			}
			break;
		case 'v':
			options |= F_VERBOSE;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();
	target = *argv;

	bzero((char *)&whereto, sizeof(struct sockaddr));
	to = (struct sockaddr_in *)&whereto;
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(target);
	if (to->sin_addr.s_addr != (u_int)-1)
		hostname = target;
	else {
		hp = gethostbyname(target);
		if (!hp) {
			(void)fprintf(stderr, MSGSTR( PING_UNK_HOST,
			    "ping: unknown host %s\n"), target);
			exit(1);
		}
		to->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
		(void)strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
		hostname = hnamebuf;
	}

	if (options & F_FLOOD && options & F_INTERVAL) {
		(void)fprintf(stderr, MSGSTR( PING_INCOMP_OPTIONS,
		    "ping: -f and -i incompatible options.\n"));
		exit(1);
	}

	if (datalen >= sizeof(struct timeval))	/* can we time transfer */
		timing = 1;
	packlen = datalen + MAXIPLEN + MAXICMPLEN;
	if (!(packet = (u_char *)malloc((u_int)packlen))) {
		(void)fprintf(stderr, MSGSTR( PING_NO_MEM, 
			"ping: out of memory.\n"));
		exit(1);
	}
	if (!(options & F_PINGFILLED))
		for (i = 8; i < datalen; ++i)
			*datap++ = i;

	ident = getpid() & 0xFFFF;

	if (!(proto = getprotobyname("icmp"))) {
		(void)fprintf(stderr, MSGSTR( PING_UNKN_PROTO, 
			"ping: unknown protocol icmp.\n"));
		exit(1);
	}
	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("ping: socket");
		exit(1);
	}
	hold = 1;
	if (options & F_SO_DEBUG)
		(void)setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&hold,
		    sizeof(hold));
	if (options & F_SO_DONTROUTE)
		(void)setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)&hold,
		    sizeof(hold));

	/* record route option */
	if (options & F_RROUTE) {
#ifdef IP_OPTIONS
		rspace[IPOPT_OPTVAL] = IPOPT_RR;
		rspace[IPOPT_OLEN] = sizeof(rspace)-1;
		rspace[IPOPT_OFFSET] = IPOPT_MINOFF;
		if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, rspace,
		    sizeof(rspace)) < 0) {
			perror("ping: record route");
			exit(1);
		}
#else
		(void)fprintf(stderr, MSGSTR( PING_RECORD_RTE,
		  "ping: record route not available in this implementation.\n"));
		exit(1);
#endif /* IP_OPTIONS */
	}

	/*
	 * When pinging the broadcast address, you can get a lot of answers.
	 * Doing something so evil is useful if you are trying to stress the
	 * ethernet, or just want to fill the arp cache to get some stuff for
	 * /etc/ethers.
	 */
	hold = 48 * 1024;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold,
	    sizeof(hold));

	if (to->sin_family == AF_INET)
		(void)printf(MSGSTR( DATBYTS1,
			"PING %s (%s): %d data bytes\n"), hostname,
		    inet_ntoa(*(struct in_addr *)&to->sin_addr.s_addr),
		    datalen);
	else
		(void)printf(MSGSTR( DATBYTS2,
			"PING %s: %d data bytes\n"), hostname, datalen);

	(void)signal(SIGINT, finish);
	(void)signal(SIGALRM, catcher);

	while (preload--)		/* fire off them quickies */
		pinger();

	if ((options & F_FLOOD) == 0)
		catcher();		/* start things going */

	for (;;) {
		struct sockaddr_in from;
		register int cc;
		int fromlen;

		if (options & F_FLOOD) {
			pinger();
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;
			fdmask = 1 << s;
			if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
			    (fd_set *)NULL, &timeout) < 1)
				continue;
		}
		fromlen = sizeof(from);
		if ((cc = recvfrom(s, (char *)packet, packlen, 0,
		    (struct sockaddr *)&from, &fromlen)) < 0) {
			if (errno == EINTR)
				continue;
			perror("ping: recvfrom");
			continue;
		}
		pr_pack((char *)packet, cc, &from);
		if (npackets && nreceived >= npackets)
			break;
	}
	finish();
	/* NOTREACHED */
}

/*
 * catcher --
 *	This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * bug --
 *	Our sense of time will slowly skew (i.e., packets will not be
 * launched exactly at 1-second intervals).  This does not affect the
 * quality of the delay and loss statistics.
 */
void
catcher()
{
	void catcher();

	int waittime;

	pinger();
	(void)signal(SIGALRM, catcher);
	if (!npackets || ntransmitted < npackets)
		alarm((u_int)interval);
	else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (!waittime)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		(void)signal(SIGALRM, finish);
		(void)alarm((u_int)waittime);
	}
}

/*
 * pinger --
 * 	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
pinger()
{
	register struct icmp *icp;
	register int cc;
	int i;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;			/* ID */

	CLR(icp->icmp_seq % mx_dup_ck);

	if (timing)
		(void)gettimeofday((struct timeval *)&outpack[8],
		    (struct timezone *)NULL);

	cc = datalen + 8;			/* skips ICMP portion */

	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	i = sendto(s, (char *)outpack, cc, 0, &whereto,
	    sizeof(struct sockaddr));

	if (i < 0 || i != cc)  {
		if (i < 0)
			perror("ping: sendto");
		(void)printf(MSGSTR( WRTBYTS, 
			"ping: wrote %s %d chars, ret=%d\n"),
		    hostname, cc, i);
	}
	if (options & F_FLOOD)
		(void)write(STDOUT_FILENO, &DOT, 1);
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
pr_pack(buf, cc, from)
	char *buf;
	int cc;
	struct sockaddr_in *from;
{
	register struct icmp *icp;
	register u_int l;
	register int i, j;
	register u_char *cp,*dp;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
	struct ip *ip;
	struct timeval tv, *tp;
	long triptime;
	int hlen, dupflag;

	(void)gettimeofday(&tv, (struct timezone *)NULL);

	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (options & F_VERBOSE)
			(void)fprintf(stderr, MSGSTR( SHRTPKT,
			  "ping: packet too short (%d bytes) from %s\n"), cc,
			  inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr));
		return;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {
		if (icp->icmp_id != ident)
			return;			/* 'Twas not our ECHO */
		++nreceived;
		if (timing) {
#ifndef icmp_data
			tp = (struct timeval *)&icp->icmp_ip;
#else
			tp = (struct timeval *)icp->icmp_data;
#endif
			tvsub(&tv, tp);
			triptime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
			tsum += triptime;
			if (triptime < tmin)
				tmin = triptime;
			if (triptime > tmax)
				tmax = triptime;
		}

		if (TST(icp->icmp_seq % mx_dup_ck)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(icp->icmp_seq % mx_dup_ck);
			dupflag = 0;
		}

		if (options & F_QUIET)
			return;

		if (options & F_FLOOD)
			(void)write(STDOUT_FILENO, &BSPACE, 1);
		else {
			(void)printf(MSGSTR( PING_F_FLOOD, 
				"%d bytes from %s: icmp_seq=%u"), cc,
			   inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr),
			   icp->icmp_seq);
			(void)printf(MSGSTR( PING_TTL, " ttl=%d"), ip->ip_ttl);
			if (timing)
				(void)printf(MSGSTR( PING_TIME, 
					" time=%ld ms"), triptime);
			if (dupflag)
				(void)printf(MSGSTR( PING_DUP, " (DUP!)"));
			/* check the data */
			cp = (u_char*)&icp->icmp_data[8];
			dp = &outpack[8 + sizeof(struct timeval)];
			for (i = 8; i < datalen; ++i, ++cp, ++dp) {
				if (*cp != *dp) {
	(void)printf(MSGSTR( PING_WRONG_DB, 
			"\nwrong data byte #%d should be 0x%x but was 0x%x"),
	    i, *dp, *cp);
					cp = (u_char*)&icp->icmp_data[0];
					for (i = 8; i < datalen; ++i, ++cp) {
						if ((i % 32) == 8)
							(void)printf(MSGSTR(PING_NL_TAB,
								"\n\t"));
						(void)printf(MSGSTR( PING_X,
							"%x "), *cp);
					}
					break;
				}
			}
		}
	} else {
		/* We've got something other than an ECHOREPLY */
		if (!(options & F_VERBOSE))
			return;
		(void)printf(MSGSTR( PING_BYTES_FROM, "%d bytes from %s: "), cc,
		    pr_addr(from->sin_addr.s_addr));
		pr_icmph(icp);
	}

	/* Display any IP options */
	cp = (u_char *)buf + sizeof(struct ip);

	/* ANSI C will force hlen to unsigned! */
	for (; hlen > (signed)sizeof(struct ip); --hlen, ++cp)
		switch (*cp) {
		case IPOPT_EOL:
			hlen = 0;
			break;
		case IPOPT_LSRR:
			(void)printf(MSGSTR( PING_LSRR, "\nLSRR: "));
			hlen -= 2;
			j = *++cp;
			++cp;
			if (j > IPOPT_MINOFF)
				for (;;) {
					l = *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					if (l == 0)
						(void)printf(MSGSTR( PING_ZERO, 
						"\t0.0.0.0"));
				else
					(void)printf(MSGSTR( PING_FMT1, "\t%s"),
						 pr_addr(ntohl(l)));
				hlen -= 4;
				j -= 4;
				if (j <= IPOPT_MINOFF)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			hlen -= 2;
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				continue;
			if (i == old_rrlen
			    && cp == (u_char *)buf + sizeof(struct ip) + 2
			    && !bcmp((char *)cp, old_rr, i)
			    && !(options & F_FLOOD)) {
				(void)printf(MSGSTR( PING_SAME_RTE, 
					"\t(same route)"));
				i = ((i + 3) / 4) * 4;
				hlen -= i;
				cp += i;
				break;
			}
			old_rrlen = i;
			bcopy((char *)cp, old_rr, i);
			(void)printf( MSGSTR( PING_RR, "\nRR: "));
			for (;;) {
				l = *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				if (l == 0)
					(void)printf( MSGSTR( PING_ZERO,
						"\t0.0.0.0"));
				else
					(void)printf( MSGSTR( PING_FMT1,
						"\t%s"), pr_addr(ntohl(l)));
				hlen -= 4;
				i -= 4;
				if (i <= 0)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_NOP:
			(void)printf( MSGSTR( PING_NOP, "\nNOP"));
			break;
		default:
			(void)printf( MSGSTR( PING_UNKN_OPT,
				"\nunknown option %x"), *cp);
			break;
		}
	if (!(options & F_FLOOD)) {
		(void)putchar('\n');
		(void)fflush(stdout);
	}
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

/*
 * tvsub --
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
tvsub(out, in)
	register struct timeval *out, *in;
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 * finish --
 *	Print out statistics, and give up.
 */
void
finish()
{
	(void)signal(SIGINT, SIG_IGN);
	(void)putchar('\n');
	(void)fflush(stdout);
	printf(MSGSTR(PINGHDR, "\n----%s PING Statistics----\n"), hostname );
        printf(MSGSTR(XMTPKTS, "%ld packets transmitted, "), ntransmitted );
        printf(MSGSTR(RECPKTS, "%ld packets received, "), nreceived );
	if (nrepeats)
		(void)printf( MSGSTR( PING_DUPLICATE, 
			"+%ld duplicates, "), nrepeats);
	if (ntransmitted)
		if (nreceived > ntransmitted)
			(void)printf(MSGSTR( PING_SOMEONE, 
				"-- somebody's printing up packets!"));
		else
			(void)printf(MSGSTR(LOSTPKTS, "%d%% packet loss"),
			    (int) (((ntransmitted - nreceived) * 100) /
			    ntransmitted));
	(void)putchar('\n');
	if (nreceived && timing)
		(void)printf( MSGSTR(RDTRIPTIM,
		"round-trip (ms)  min/avg/max = %ld/%lu/%ld ms\n"),
		    tmin, tsum / (nreceived + nrepeats), tmax);
	exit(0);
}

#ifdef notdef
static char *ttab[] = {
	"Echo Reply",		/* ip + seq + udata */
	"Dest Unreachable",	/* net, host, proto, port, frag, sr + IP */
	"Source Quench",	/* IP */
	"Redirect",		/* redirect type, gateway, + IP  */
	"Echo",
	"Time Exceeded",	/* transit, frag reassem + IP */
	"Parameter Problem",	/* pointer + IP */
	"Timestamp",		/* id + seq + three timestamps */
	"Timestamp Reply",	/* " */
	"Info Request",		/* id + sq */
	"Info Reply"		/* " */
};
#endif

/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
pr_icmph(icp)
	struct icmp *icp;
{
	switch(icp->icmp_type) {
	case ICMP_ECHOREPLY:
		(void)printf( MSGSTR( PING_ECHO_REPLY, "Echo Reply\n"));
		/* XXX ID + Seq + Data */
		break;
	case ICMP_UNREACH:
		switch(icp->icmp_code) {
		case ICMP_UNREACH_NET:
			(void)printf( MSGSTR( PING_NET_UNREACH,
				"Destination Net Unreachable\n"));
			break;
		case ICMP_UNREACH_HOST:
			(void)printf( MSGSTR( PING_HOST_UNR,
				"Destination Host Unreachable\n"));
			break;
		case ICMP_UNREACH_PROTOCOL:
			(void)printf( MSGSTR( PING_PROTO_UNR,
				"Destination Protocol Unreachable\n"));
			break;
		case ICMP_UNREACH_PORT:
			(void)printf( MSGSTR( PING_PORT_UNR,
				"Destination Port Unreachable\n"));
			break;
		case ICMP_UNREACH_NEEDFRAG:
			(void)printf( MSGSTR( PING_FRAG_DF,
				"frag needed and DF set\n"));
			break;
		case ICMP_UNREACH_SRCFAIL:
			(void)printf( MSGSTR( PING_RTE_FAIL,
				"Source Route Failed\n"));
			break;
		default:
			(void)printf( MSGSTR( PING_DEST_BCODE,
				"Dest Unreachable, Bad Code: %d\n"),
			    icp->icmp_code);
			break;
		}
		/* Print returned IP header information */
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_SOURCEQUENCH:
		(void)printf( MSGSTR( PING_SRC_QUENCH, "Source Quench\n"));
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_REDIRECT:
		switch(icp->icmp_code) {
		case ICMP_REDIRECT_NET:
			(void)printf( MSGSTR( PING_REDIRECT_NET,
				"Redirect Network"));
			break;
		case ICMP_REDIRECT_HOST:
			(void)printf( MSGSTR( PING_REDIRECT_HST,
				"Redirect Host"));
			break;
		case ICMP_REDIRECT_TOSNET:
			(void)printf( MSGSTR( PING_SERV_NET,
				"Redirect Type of Service and Network"));
			break;
		case ICMP_REDIRECT_TOSHOST:
			(void)printf( MSGSTR( PING_SERV_HOST,
				"Redirect Type of Service and Host"));
			break;
		default:
			(void)printf( MSGSTR( PING_RED_BCODE,
				"Redirect, Bad Code: %d"), icp->icmp_code);
			break;
		}
		(void)printf( MSGSTR( PING_NEW_ADDR,
			"(New addr: 0x%08lx)\n"), icp->icmp_gwaddr.s_addr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_ECHO:
		(void)printf( MSGSTR( PING_ECHO_REQ, "Echo Request\n"));
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIMXCEED:
		switch(icp->icmp_code) {
		case ICMP_TIMXCEED_INTRANS:
			(void)printf( MSGSTR( PING_TTLIVE,
				"Time to live exceeded\n"));
			break;
		case ICMP_TIMXCEED_REASS:
			(void)printf( MSGSTR( PING_FRAG_TIMEOUT,
				"Frag reassembly time exceeded\n"));
			break;
		default:
			(void)printf( MSGSTR( PING_TIME_BCODE,
				"Time exceeded, Bad Code: %d\n"),
			    icp->icmp_code);
			break;
		}
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_PARAMPROB:
		(void)printf( MSGSTR( PING_PARAM_PROB,
			"Parameter problem: pointer = 0x%02x\n"),
		    icp->icmp_hun.ih_pptr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_TSTAMP:
		(void)printf( MSGSTR( PING_TIMESTAMP, "Timestamp\n"));
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TSTAMPREPLY:
		(void)printf( MSGSTR( PING_TIMESTAMP_REPLY, "Timestamp Reply\n"));
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_IREQ:
		(void)printf( MSGSTR( PING_INFO_REQ, "Information Request\n"));
		/* XXX ID + Seq */
		break;
	case ICMP_IREQREPLY:
		(void)printf( MSGSTR( PING_INFO_REPLY, "Information Reply\n"));
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		(void)printf( MSGSTR( PING_ADDR_MASK, "Address Mask Request\n"));
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		(void)printf( MSGSTR( PING_ADDR_REPLY, "Address Mask Reply\n"));
		break;
#endif
	default:
		(void)printf( MSGSTR( PING_BAD_ICMP, "Bad ICMP type: %d\n"), icp->icmp_type);
	}
}

/*
 * pr_iph --
 *	Print an IP header with options.
 */
pr_iph(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	(void)printf( MSGSTR( PING_IP_HDR,
		"Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n"));
	(void)printf( MSGSTR( PING_FMT2, " %1x  %1x  %02x %04x %04x"),
	    ip->ip_v, ip->ip_hl, ip->ip_tos, ip->ip_len, ip->ip_id);
	(void)printf( MSGSTR( PING_FMT3,
		"   %1x %04x"), ((ip->ip_off) & 0xe000) >> 13,
	    (ip->ip_off) & 0x1fff);
	(void)printf( MSGSTR( PING_FMT4,
		"  %02x  %02x %04x"), ip->ip_ttl, ip->ip_p, ip->ip_sum);
	(void)printf( MSGSTR( PING_FMT5, " %s "),
		inet_ntoa(*(struct in_addr *)&ip->ip_src.s_addr));
	(void)printf( MSGSTR( PING_FMT5, " %s "),
		inet_ntoa(*(struct in_addr *)&ip->ip_dst.s_addr));
	/* dump and option bytes */
	while (hlen-- > 20) {
		(void)printf( MSGSTR( PING_FMT6, "%02x"), *cp++);
	}
	(void)putchar('\n');
}

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
char *
pr_addr(l)
	u_int l;
{
	struct hostent *hp;
	static char buf[80];

	if ((options & F_NUMERIC) ||
	    !(hp = gethostbyaddr((char *)&l, 4, AF_INET)))
		(void)sprintf(buf, "%s", inet_ntoa(*(struct in_addr *)&l));
	else
		(void)sprintf(buf, "%s (%s)", hp->h_name,
		    inet_ntoa(*(struct in_addr *)&l));
	return(buf);
}

/*
 * pr_retip --
 *	Dump some info on a returned (via ICMP) IP packet.
 */
pr_retip(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	pr_iph(ip);
	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + hlen;

	if (ip->ip_p == 6)
		(void)printf( MSGSTR( PING_TCP_DUMP,
			"TCP: from port %u, to port %u (decimal)\n"),
		    (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
	else if (ip->ip_p == 17)
		(void)printf( MSGSTR( PING_UDP_DUMP,
			"UDP: from port %u, to port %u (decimal)\n"),
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
}

fill(bp, patp)
	char *bp, *patp;
{
	register int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit(*cp)) {
			(void)fprintf(stderr, MSGSTR( PING_PATTERNS,
			    "ping: patterns must be specified as hex digits.\n"));
			exit(1);
		}
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if (ii > 0)
		for (kk = 0; kk <= MAXPACKET - (8 + ii); kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!(options & F_QUIET)) {
		(void)printf( MSGSTR( PING_PATTERN, "PATTERN: 0x"));
		for (jj = 0; jj < ii; ++jj)
			(void)printf( MSGSTR( PING_FMT6, "%02x"), bp[jj] & 0xFF);
		(void)printf( MSGSTR( PING_NEWLINE, "\n"));
	}
}

usage()
{
	(void)fprintf(stderr, MSGSTR( PING_USAGE,
	    "usage: ping [-Rdfnqrv] [-c count] [-i wait] [-l preload]\n\t[-p pattern] [-s packetsize] host\n"));
	exit(1);
}
