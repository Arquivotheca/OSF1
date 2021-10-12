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
static char	*sccsid = "@(#)$RCSfile: trace.c,v $ $Revision: 4.2.2.3 $ (DEC) $Date: 1992/06/03 18:24:53 $";
#endif 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 Release 1.0
 */ 
#if !defined(lint) && !defined(_NOIDENT)

#endif
/*

*/
/* 
 * COMPONENT_NAME: TCPIP trace.c
 * 
 * FUNCTIONS: MSGSTR, bumploglevel, dumpif, dumppacket, dumptrace, 
 *            iftraceinit, sigtrace, trace, traceaction, traceinit, 
 *            tracenewmetric, traceoff, traceon 
 *
 * ORIGINS: 10  26  27 
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1983, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
#ifndef lint
static char sccsid[] = "trace.c	5.10 (Berkeley) 6/1/90";
#endif  not lint */

/*
 * Routing Table Management Daemon
 */
#define	RIPCMDS
#include "defs.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include "pathnames.h"

#define	NRECORDS	50	     /* size of circular trace buffer */
/*
 * Packet tracing stuff.
 */
int tracepackets = 0 ;               /* watch packets as they go by */
int tracecontents = 0 ;              /* watch packet contents as they go by */
int traceactions = 0 ;               /* on/off */
int tracehistory = 0 ;               /* on/off */
FILE *ftrace = stdout ;              /* output trace file */

static	struct timeval lastlog;
static	char *savetracename;
static	iftraceinit();

traceinit(ifp)
	register struct interface *ifp;
{

	if (iftraceinit(ifp, &ifp->int_input) &&
	    iftraceinit(ifp, &ifp->int_output))
		return;
	tracehistory = 0;
	fprintf(stderr, MSGSTR(TRINI, "traceinit: can't init %s\n"), ifp->int_name); /*MSG*/

}

static
iftraceinit(ifp, ifd)
	struct interface *ifp;
	register struct ifdebug *ifd;
{
	register struct iftrace *t;

	ifd->ifd_records =
	  (struct iftrace *)malloc(NRECORDS * sizeof (struct iftrace));
	if (ifd->ifd_records == 0)
		return (0);
	ifd->ifd_front = ifd->ifd_records;
	ifd->ifd_count = 0;
	for (t = ifd->ifd_records; t < ifd->ifd_records + NRECORDS; t++) {
		t->ift_size = 0;
		t->ift_packet = 0;
	}
	ifd->ifd_if = ifp;
	return (1);
}

traceon(file)
	char *file;
{
	struct stat stbuf;

	if (ftrace != NULL)
                return;
        if (stat(file, &stbuf) >= 0 && (stbuf.st_mode & S_IFMT) != S_IFREG)
                return;
        savetracename = file;
	(void) gettimeofday(&now, (struct timezone *)NULL);
	ftrace = fopen(file, "a");
        if (ftrace == NULL)
                return;
	dup2(fileno(ftrace), 1);
	dup2(fileno(ftrace), 2);
	traceactions = 1;
	fprintf(ftrace, "Tracing enabled %s\n", ctime((time_t *)&now.tv_sec));
}

traceoff()
{
	if (!traceactions)
		return;
	if (ftrace != NULL) {
		int fd = open(_PATH_DEVNULL, O_RDWR);

		fprintf(ftrace, "Tracing disabled %s\n",
		    ctime((time_t *)&now.tv_sec));
		fflush(ftrace);
		(void) dup2(fd, 1);
		(void) dup2(fd, 2);
		(void) close(fd);
		fclose(ftrace);
		ftrace = NULL;
	}
	traceactions = 0;
	tracehistory = 0;
	tracepackets = 0;
	tracecontents = 0;
}

sigtrace(s)
	int s;
{

	if (s == SIGUSR2)
		traceoff();
	else if (ftrace == NULL && savetracename)
		traceon(savetracename);
	else
		bumploglevel();
}

/*
 * Move to next higher level of tracing when -t option processed or
 * SIGUSR1 is received.  Successive levels are:
 *	traceactions
 *	traceactions + tracepackets
 *	traceactions + tracehistory (packets and contents after change)
 *	traceactions + tracepackets + tracecontents
 */
bumploglevel()
{

	(void) gettimeofday(&now, (struct timezone *)NULL);
	if (traceactions == 0) {
		traceactions++;
		if (ftrace)
			fprintf(ftrace, "Tracing actions started %s\n",
			    ctime((time_t *)&now.tv_sec));
	} else if (tracepackets == 0) {
		tracepackets++;
		tracehistory = 0;
		tracecontents = 0;
		if (ftrace)
			fprintf(ftrace, "Tracing packets started %s\n",
			    ctime((time_t *)&now.tv_sec));
	} else if (tracehistory == 0) {
		tracehistory++;
		if (ftrace)
			fprintf(ftrace, "Tracing history started %s\n",
			    ctime((time_t *)&now.tv_sec));
	} else {
		tracepackets++;
		tracecontents++;
		tracehistory = 0;
		if (ftrace)
			fprintf(ftrace, "Tracing packet contents started %s\n",
			    ctime((time_t *)&now.tv_sec));
	}
	if (ftrace)
		fflush(ftrace);
}

trace(ifd, who, p, len, m)
	register struct ifdebug *ifd;
	struct sockaddr *who;
	char *p;
	int len, m;
{
	register struct iftrace *t;

	if (ifd->ifd_records == 0)
		return;
	t = ifd->ifd_front++;
	if (ifd->ifd_front >= ifd->ifd_records + NRECORDS)
		ifd->ifd_front = ifd->ifd_records;
	if (ifd->ifd_count < NRECORDS)
		ifd->ifd_count++;
	if (t->ift_size > 0 && t->ift_size < len && t->ift_packet) {
		free(t->ift_packet);
		t->ift_packet = 0;
	}
	t->ift_stamp = now;
	t->ift_who = *who;
	if (len > 0 && t->ift_packet == 0) {
		t->ift_packet = malloc(len);
		if (t->ift_packet == 0)
			len = 0;
	}
	if (len > 0)
		bcopy(p, t->ift_packet, len);
	t->ift_size = len;
	t->ift_metric = m;
}

traceaction(fd, action, rt)
	FILE *fd;
	char *action;
	struct rt_entry *rt;
{
	struct sockaddr_in *dst, *gate;
	static struct bits {
		int	t_bits;
		char	*t_name;
	} flagbits[] = {
		{ RTF_UP,	"UP" },
		{ RTF_GATEWAY,	"GATEWAY" },
		{ RTF_HOST,	"HOST" },
		{ 0 }
	}, statebits[] = {
		{ RTS_PASSIVE,	"PASSIVE" },
		{ RTS_REMOTE,	"REMOTE" },
		{ RTS_INTERFACE,"INTERFACE" },
		{ RTS_CHANGED,	"CHANGED" },
		{ RTS_INTERNAL,	"INTERNAL" },
		{ RTS_EXTERNAL,	"EXTERNAL" },
		{ RTS_SUBNET,	"SUBNET" },
		{ 0 }
	};
	register struct bits *p;
	register int first;
	char *cp;
	struct interface *ifp;

	if (fd == NULL)
		return;
	if (lastlog.tv_sec != now.tv_sec || lastlog.tv_usec != now.tv_usec) {
		fprintf(fd, "\n%.19s:\n", ctime((time_t *)&now.tv_sec));
		lastlog = now;
	}
	fprintf(fd, "%s ", action);
	dst = (struct sockaddr_in *)&rt->rt_dst;
	gate = (struct sockaddr_in *)&rt->rt_router;
	fprintf(fd, "dst %s, ", inet_ntoa(dst->sin_addr));
	fprintf(fd, "router %s, metric %d, flags",
	     inet_ntoa(gate->sin_addr), rt->rt_metric);
	cp = " %s";
	for (first = 1, p = flagbits; p->t_bits > 0; p++) {
		if ((rt->rt_flags & p->t_bits) == 0)
			continue;
		fprintf(fd, cp, p->t_name);
		if (first) {
			cp = "|%s";
			first = 0;
		}
	}
	fprintf(fd, " state");
	cp = " %s";
	for (first = 1, p = statebits; p->t_bits > 0; p++) {
		if ((rt->rt_state & p->t_bits) == 0)
			continue;
		fprintf(fd, cp, p->t_name);
		if (first) {
			cp = "|%s";
			first = 0;
		}
	}
	fprintf(fd, " timer %d\n", rt->rt_timer);
	if (tracehistory && !tracepackets &&
	    (rt->rt_state & RTS_PASSIVE) == 0 && rt->rt_ifp)
		dumpif(fd, rt->rt_ifp);
	fflush(fd);
	if (ferror(fd))
		traceoff();
}

tracenewmetric(fd, rt, newmetric)
	FILE *fd;
	struct rt_entry *rt;
	int newmetric;
{
	struct sockaddr_in *dst, *gate;

	if (fd == NULL)
		return;
	if (lastlog.tv_sec != now.tv_sec || lastlog.tv_usec != now.tv_usec) {
		fprintf(fd, "\n%.19s:\n", ctime((time_t *)&now.tv_sec));
		lastlog = now;
	}
	dst = (struct sockaddr_in *)&rt->rt_dst;
	gate = (struct sockaddr_in *)&rt->rt_router;
	fprintf(fd, "CHANGE metric dst %s, ", inet_ntoa(dst->sin_addr));
	fprintf(fd, "router %s, from %d to %d\n",
	     inet_ntoa(gate->sin_addr), rt->rt_metric, newmetric);
	fflush(fd);
	if (ferror(fd))
		traceoff();
}

dumpif(fd, ifp)
	FILE *fd;
	register struct interface *ifp;
{
	if (ifp->int_input.ifd_count || ifp->int_output.ifd_count) {
		fprintf(fd, "*** Packet history for interface %s ***\n",
			ifp->int_name);
#ifdef notneeded
		dumptrace(fd, "to", &ifp->int_output);
#endif
		dumptrace(fd, "from", &ifp->int_input);
		fprintf(fd, "*** end packet history ***\n");
	}
}

dumptrace(fd, dir, ifd)
	FILE *fd;
	char *dir;
	register struct ifdebug *ifd;
{
	register struct iftrace *t;
	char *cp = !strcmp(dir, "to") ? "Output" : "Input";

	if (ifd->ifd_front == ifd->ifd_records &&
	    ifd->ifd_front->ift_size == 0) {
		fprintf(fd, "%s: no packets.\n", cp);
		fflush(fd);
		return;
	}
	fprintf(fd, "%s trace:\n", cp);
	t = ifd->ifd_front - ifd->ifd_count;
	if (t < ifd->ifd_records)
		t += NRECORDS;
	for ( ; ifd->ifd_count; ifd->ifd_count--, t++) {
		if (t >= ifd->ifd_records + NRECORDS)
			t = ifd->ifd_records;
		if (t->ift_size == 0)
			continue;
	       dumppacket(fd, dir, &t->ift_who, t->ift_packet,(int)t->ift_size,
		    &t->ift_stamp);
	}
}

dumppacket(fd, dir, who, cp, size, stamp)
	FILE *fd;
	struct sockaddr_in *who;		/* should be sockaddr */
	char *dir, *cp;
	register int size;
	struct timeval *stamp;
{
	register struct rip *msg = (struct rip *)cp;
	register struct netinfo *n;

	if (fd == NULL)
		return;
	if (msg->rip_cmd && msg->rip_cmd < RIPCMD_MAX)
		fprintf(fd, "%s %s %s.%d %.19s:\n", ripcmds[msg->rip_cmd],
		    dir, inet_ntoa(who->sin_addr), ntohs(who->sin_port),
		    ctime((time_t *)&stamp->tv_sec));
	else {
		fprintf(fd, "Bad cmd 0x%x %s %x.%d %.19s\n", msg->rip_cmd,
		    dir, inet_ntoa(who->sin_addr), ntohs(who->sin_port));
		fprintf(fd, "size=%d cp=%x packet=%x\n", size, cp, packet,
		    ctime((time_t *)&stamp->tv_sec));
		fflush(fd);
		return;
	}
	if (tracepackets && tracecontents == 0) {
		fflush(fd);
		return;
	}
	switch (msg->rip_cmd) {

	case RIPCMD_REQUEST:
	case RIPCMD_RESPONSE:
		size -= 4 * sizeof (char);
		n = msg->rip_nets;
		for (; size > 0; n++, size -= sizeof (struct netinfo)) {
			if (size < sizeof (struct netinfo)) {
				fprintf(fd, "(truncated record, len %d)\n",
				    size);
				break;
			}
			if (sizeof(n->rip_dst.sa_family) > 1)
			    n->rip_dst.sa_family = ntohs(n->rip_dst.sa_family);

			switch ((int)n->rip_dst.sa_family) {

			case AF_INET:
				fprintf(fd, "\tdst %s metric %d\n",
#define	satosin(sa)	((struct sockaddr_in *)&sa)
				     inet_ntoa(satosin(n->rip_dst)->sin_addr),
				     ntohl(n->rip_metric));
				break;

			default:
				fprintf(fd, "\taf %d? metric %d\n",
				     n->rip_dst.sa_family,
				     ntohl(n->rip_metric));
				break;
			}
		}
		break;

	case RIPCMD_TRACEON:
		fprintf(fd, "\tfile=%*s\n", size, msg->rip_tracefile);
		break;

	case RIPCMD_TRACEOFF:
		break;
	}
	fflush(fd);
	if (ferror(fd))
		traceoff();
}
