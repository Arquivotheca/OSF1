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
static char	*sccsid = "@(#)$RCSfile: trpt.c,v $ $Revision: 4.2.3.2 $ (DEC) $Date: 1993/10/08 16:13:25 $";
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
/*
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
#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983, 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef OSF
#include <machine/pte.h>
#endif

#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#define PRUREQUESTS
#include <sys/protosw.h>
#include <sys/file.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_var.h>
#define	TANAMES
#include <netinet/tcp_debug.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <nlist.h>
#include <paths.h>

#include <nl_types.h>
#include <locale.h>
#include "trpt_msg.h"
#define MSGSTR(Num,Str) catgets(catd,MS_TRPT,Num,Str)
nl_catd catd;           /* message catalog file descriptor */

struct nlist nl[] = {
#define	N_TCP_DEBUG	0
	{ "_tcp_debug" },
#define	N_TCP_DEBX	1
	{ "_tcp_debx" },
#define	N_SYSMAP	2
	{ "_Sysmap" },
#define	N_SYSSIZE	3
	{ "_Syssize" },
	{ "" },
};

#ifndef OSF
static struct pte *Sysmap;
#endif
static  struct  tcp_debug tcp_debug[TCP_NDEBUG];
static  int     tcp_debx;
static caddr_t tcp_pcbs[TCP_NDEBUG];
static n_time ntime;
static int aflag, kflag, memf, follow, sflag, tflag;

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	int ch, i, jflag, npcbs, numeric();
	char *system, *core, *malloc();
	off_t lseek();

	setlocale(LC_ALL, "");
	catd = NLcatopen( MF_TRPT, NL_CAT_LOCALE);

	jflag = npcbs = 0;
	while ((ch = getopt(argc, argv, "afjp:st")) != EOF)
		switch((char)ch) {
		case 'a':
			++aflag;
			break;
		case 'f':
			++follow;
			setlinebuf(stdout);
			break;
		case 'j':
			++jflag;
			break;
		case 'p':
			if (npcbs >= TCP_NDEBUG) {
				fputs("trpt: too many pcb's specified\n",
				    stderr);
				exit(1);
			}
			(void)sscanf(optarg, "%x", (int *)&tcp_pcbs[npcbs++]);
			break;
		case 's':
			++sflag;
			break;
		case 't':
			++tflag;
			break;
		case '?':
		default:
			(void)fprintf(stderr, MSGSTR(TRPT_USAGE, 
"usage: trpt [-afjst] [-p hex-address] [system [core]]\n"));
			exit(1);
		}
	argc -= optind;
	argv += optind;

	core = _PATH_KMEM;
	if (argc > 0) {
		system = *argv;
		argc--, argv++;
		if (argc > 0) {
			core = *argv;
			argc--, argv++;
			++kflag;
		}
	}
	else
		system = _PATH_UNIX;

	if (nlist(system, nl) < 0 || !nl[0].n_value) {
		fprintf(stderr,  MSGSTR(TRPT_NONAME,  
			"trpt: %s: no namelist\n"), system);
		exit(1);
	}
	if ((memf = open(core, O_RDONLY)) < 0) {
		perror(core);
		exit(2);
	}
#ifndef OSF
	if (kflag) {
		off_t off;

		Sysmap = (struct pte *)
		   malloc((u_int)(nl[N_SYSSIZE].n_value * sizeof(struct pte)));
		if (!Sysmap) {
			fputs("arp: can't get memory for Sysmap.\n", stderr);
			exit(1);
		}
		off = nl[N_SYSMAP].n_value & ~KERNBASE;
		(void)lseek(memf, off, L_SET);
		(void)read(memf, (char *)Sysmap,
		    (int)(nl[N_SYSSIZE].n_value * sizeof(struct pte)));
	}
#endif
	(void)klseek(memf, (off_t)nl[N_TCP_DEBX].n_value, L_SET);
	if (read(memf, (char *)&tcp_debx, sizeof(tcp_debx)) !=
	    sizeof(tcp_debx)) {
		perror("trpt: tcp_debx");
		exit(3);
	}
	(void)klseek(memf, (off_t)nl[N_TCP_DEBUG].n_value, L_SET);
	if (read(memf, (char *)tcp_debug, sizeof(tcp_debug)) !=
	    sizeof(tcp_debug)) {
		perror("trpt: tcp_debug");
		exit(3);
	}
	/*
	 * If no control blocks have been specified, figure
	 * out how many distinct one we have and summarize
	 * them in tcp_pcbs for sorting the trace records
	 * below.
	 */
	if (!npcbs) {
		for (i = 0; i < TCP_NDEBUG; i++) {
			register struct tcp_debug *td = &tcp_debug[i];
			register int j;

			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbs[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbs[npcbs++] = td->td_tcb;
		}
		if (!npcbs)
			exit(0);
	}
	qsort(tcp_pcbs, npcbs, sizeof(caddr_t), numeric);
	if (jflag) {
		for (i = 0;;) {
			printf( MSGSTR(TRPT_STR_X, "%x"), (int)tcp_pcbs[i]);
			if (++i == npcbs)
				break;
			fputs(", ", stdout);
		}
		putchar('\n');
	}
	else for (i = 0; i < npcbs; i++) {
		printf( MSGSTR(TRPT_FMT1, "\n%x:\n"), (int)tcp_pcbs[i]);
		dotrace(tcp_pcbs[i]);
	}
	exit(0);
}

dotrace(tcpcb)
	register caddr_t tcpcb;
{
	register struct tcp_debug *td;
	register int i;
	int prev_debx = tcp_debx;

again:	if (--tcp_debx < 0)
		tcp_debx = TCP_NDEBUG - 1;
	for (i = prev_debx % TCP_NDEBUG; i < TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
		if (i == tcp_debx)
			goto done;
	}
	for (i = 0; i <= tcp_debx % TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
	}
done:	if (follow) {
		prev_debx = tcp_debx + 1;
		if (prev_debx >= TCP_NDEBUG)
			prev_debx = 0;
		do {
			sleep(1);
			(void)klseek(memf, (off_t)nl[N_TCP_DEBX].n_value, L_SET);
			if (read(memf, (char *)&tcp_debx, sizeof(tcp_debx)) !=
			    sizeof(tcp_debx)) {
				perror("trpt: tcp_debx");
				exit(3);
			}
		} while (tcp_debx == prev_debx);
		(void)klseek(memf, (off_t)nl[N_TCP_DEBUG].n_value, L_SET);
		if (read(memf, (char *)tcp_debug, sizeof(tcp_debug)) !=
		    sizeof(tcp_debug)) {
			perror("trpt: tcp_debug");
			exit(3);
		}
		goto again;
	}
}

/*
 * Tcp debug routines
 */
/*ARGSUSED*/
tcp_trace(act, ostate, atp, tp, ti, req)
	short act, ostate;
	struct tcpcb *atp, *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int flags, len, win, timer;

	printf( MSGSTR(TRPT_FMT2, "%03ld %s:%s "),(ntime/10) % 1000, 
		tcpstates[ostate], tanames[act]);
	switch (act) {
	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (aflag) {
			printf( MSGSTR(TRPT_SRC,  "(src=%s,%u, "),
			    inet_ntoa(ti->ti_src), ntohs(ti->ti_sport));
			printf( MSGSTR(TRPT_DST,  "dst=%s,%u)"),
			    inet_ntoa(ti->ti_dst), ntohs(ti->ti_dport));
		}
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		win = ti->ti_win;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs(len);
			win = ntohs(win);
		}
		if (act == TA_OUTPUT)
			len -= sizeof(struct tcphdr);
		if (len)
			printf( MSGSTR(TRPT_FMT3, "[%lx..%lx)"), seq, seq + len);
		else
			printf( MSGSTR(TRPT_FMT4,  "%lx"), seq);
		printf( MSGSTR(TRPT_FMT9, "@%lx"), ack);
		if (win)
			printf( MSGSTR(TRPT_WIN, "(win=%x)"), win);
		flags = ti->ti_flags;
		if (flags) {
			register char *cp = "<";
#define	pf(flag, string) { \
	if (ti->ti_flags&flag) { \
		(void)printf( MSGSTR(TRPT_FMT5, "%s%s"), cp, string); \
		cp = ","; \
	} \
}
			pf(TH_SYN, "SYN");
			pf(TH_ACK, "ACK");
			pf(TH_FIN, "FIN");
			pf(TH_RST, "RST");
			pf(TH_PUSH, "PUSH");
			pf(TH_URG, "URG");
			printf( MSGSTR(TRPT_ARROW, ">"));
		}
		break;
	case TA_USER:
		timer = req >> 8;
		req &= 0xff;
		printf( MSGSTR(TRPT_FMT6, "%s"), prurequests[req]);
		if (req == PRU_SLOWTIMO || req == PRU_FASTTIMO)
			printf( MSGSTR(TRPT_FMT7, "<%s>"), tcptimers[timer]);
		break;
	}
	printf( MSGSTR(TRPT_FMT10, " -> %s"), tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf( MSGSTR(TRPT_NEWLINE,  "\n"));
	if (sflag) {
		printf( MSGSTR(TRPT_SFLAG1,
			"\trcv_nxt %lx rcv_wnd %x snd_una %lx snd_nxt %lx snd_max %lx\n"),
		    	tp->rcv_nxt, tp->rcv_wnd, tp->snd_una, tp->snd_nxt,
		    	tp->snd_max);
		printf( MSGSTR(TRPT_SFLAG2,
			"\tsnd_wl1 %lx snd_wl2 %lx snd_wnd %x\n"), tp->snd_wl1,
		    	tp->snd_wl2, tp->snd_wnd);
	}
	/* print out timers? */
	if (tflag) {
		register char *cp = "\t";
		register int i;

		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] == 0)
				continue;
			printf( MSGSTR(TRPT_FMT8, "%s%s=%d"), 
				cp, tcptimers[i], tp->t_timer[i]);
			if (i == TCPT_REXMT)
				printf( MSGSTR(TRPT_RXTSHFT, 
					" (t_rxtshft=%d)"), tp->t_rxtshift);
			cp = ", ";
		}
		if (*cp != '\t')
			putchar('\n');
	}
}

numeric(c1, c2)
	caddr_t *c1, *c2;
{
	return(*c1 - *c2);
}

klseek(fd, base, off)
	int fd, off;
	off_t base;
{
	off_t lseek();

#ifndef OSF
	if (kflag) {	/* get kernel pte */
		base &= ~KERNBASE;
		base = ctob(Sysmap[btop(base)].pg_pfnum) + (base & PGOFSET);
	}
#endif
	(void)lseek(fd, base, off);
}
