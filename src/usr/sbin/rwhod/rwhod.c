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
static char	*sccsid = "@(#)$RCSfile: rwhod.c,v $ $Revision: 4.2.8.2 $ (DEC) $Date: 1993/10/08 16:01:31 $";
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
 */
/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */
/*
 * rwhod.c
 *
 *	Revision History:
 *
 * 16-Apr-91    Mary Walker
 *      added in -b and -l flags
 *      stripped domain name from incoming packets
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/table.h>

#include <net/if.h>
#include <netinet/in.h>

#include <errno.h>
#include <utmp.h>
#include <ctype.h>
#include <netdb.h>
#include <syslog.h>
#include <protocols/rwhod.h>
#include <stdio.h>
#include <paths.h>
/*MESSAGE CATALOGS*/
#include <nl_types.h>
#include <locale.h>
#include "rwhod_msg.h"
#define MSGSTR(Num,Str) catgets(catd,MS_RWHOD,Num,Str)
nl_catd catd;           /* message catalog file descriptor */

/*
 * Alarm interval. Don't forget to change the down time check in ruptime
 * if this is changed.
 */
#define AL_INTERVAL (3 * 60)
#define _PATH_UTMP	UTMP_FILE
#define NETLONG_MAX	0x7fffffff	/* 32-bit network long max */

struct	sockaddr_in sin;

char	myname[MAXHOSTNAMELEN];


/*
 * We communicate with each neighbor in
 * a list constructed at the time we're
 * started up.  Neighbors are currently
 * directly connected via a hardware interface.
 */
struct	neighbor {
	struct	neighbor *n_next;
	char	*n_name;		/* interface name */
	char	*n_addr;		/* who to send to */
	int	n_addrlen;		/* size of address */
	int	n_flags;		/* should forward?, interface flags */
};

struct	neighbor *neighbors;
struct	whod mywd;
struct	servent *sp;
int	s, utmpf, kmemf = -1;

#define	WHDRSIZE	(sizeof (mywd) - sizeof (mywd.wd_we))

extern int errno;
void	onalrm();
char	*strcpy(), *malloc();
long	lseek();
void    getkmem();
struct	in_addr inet_makeaddr();
int listenmode = 0;	/* listen to incoming traffic only */
int broadcastmode = 0;	/* broadcast only, do not listen */

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	struct sockaddr_in from;
	struct stat st;
	char path[64];
	int on = 1, ch;
	char *cp, *index(), *strerror();
	void usage();

        setlocale(LC_ALL,"");
	catd = NLcatopen(MF_RWHOD, NL_CAT_LOCALE);

	if (getuid()) {
		fprintf(stderr, MSGSTR( RWHOD_SUPERUSER, 
			"rwhod: not super user\n"));
		exit(1);
	}
	while ((ch = getopt(argc, argv, "lb")) != EOF) {
		switch((char)ch) {
		case 'l':
		 	listenmode = 1;
			break;
		case 'b':
		 	broadcastmode = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argv[0] != NULL) {
		fprintf(stderr, MSGSTR(RWHOD_UNKN_ARG,
			"rwhod: unknown argument -- %s\n"), argv[0]);
		usage();
	}

	if (listenmode && broadcastmode) {
		 listenmode = 0;
		 broadcastmode = 0;
	}
	sp = getservbyname("who", "udp");
	if (sp == 0) {
		fprintf(stderr, MSGSTR( RHWOD_UNKN_SERVICE,
			"rwhod: udp/who: unknown service\n"));
		exit(1);
	}
#ifndef DEBUG
	daemon(1, 0);
#endif
	(void) signal(SIGHUP, getkmem);
	openlog("rwhod", LOG_PID, LOG_DAEMON);
	if (chdir(_PATH_RWHODIR) < 0) {
		syslog(LOG_ERR, MSGSTR( RWHOD_DIR_ERROR, 
			"Can't change dir to  %s\n"), _PATH_RWHODIR);
		exit(1);
	}
	/*
	 * Establish host name as returned by system.
	 */
	if (gethostname(myname, sizeof (myname) - 1) < 0) {
		syslog(LOG_ERR, "gethostname: %m");
		exit(1);
	}
	if ((cp = index(myname, '.')) != NULL)
		*cp = '\0';
	strncpy(mywd.wd_hostname, myname, sizeof (myname) - 1);
	utmpf = open(_PATH_UTMP, O_RDONLY|O_CREAT, 0644);
	if (utmpf < 0) {
		syslog(LOG_ERR, "%s: %m", _PATH_UTMP);
		exit(1);
	}
	getkmem();
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		syslog(LOG_ERR, "setsockopt SO_BROADCAST: %m");
		exit(1);
	}
	sin.sin_family = AF_INET;
	sin.sin_port = sp->s_port;
	if (bind(s, &sin, sizeof (sin)) < 0) {
		syslog(LOG_ERR, "bind: %m");
		exit(1);
	}
	if (!configure(s))
		exit(1);

	/*
	 * If not in listen-only mode, call signal(onalrm) to sleep
	 * until awakened by alarm.  Make first onalrm call.
	 */
	if (! listenmode ) {
	        signal(SIGALRM, onalrm);
		onalrm();
	}

	for (;;) {
		struct whod wd;
		int cc, whod, len = sizeof (from);

		cc = recvfrom(s, (char *)&wd, sizeof (struct whod), 0,
			&from, &len);
		/*
		 * When -b is specified do not save info in /usr/spool/rwho
		 * The read of the socket is still needed so UDP counters
		 * don't show lots of dropped packets.
		 */
		if (broadcastmode)
			continue;

		if (cc <= 0) {
			if (cc < 0 && errno != EINTR)
				syslog(LOG_WARNING, "recv: %m");
			continue;
		}
		if (from.sin_port != sp->s_port) {
			syslog(LOG_WARNING, "%d: bad from port",
				ntohs(from.sin_port));
			continue;
		}
		if (wd.wd_vers != WHODVERSION)
			continue;
		if (wd.wd_type != WHODTYPE_STATUS)
			continue;
		/* strip domain name from incoming packets */
		if ((cp = index(wd.wd_hostname, '.')) != NULL)
			*cp = '\0';
		if (!verify(wd.wd_hostname)) {
			syslog(LOG_WARNING, "malformed host name from %x",
				from.sin_addr);
			continue;
		}
		(void) sprintf(path, "whod.%s", wd.wd_hostname);
		/*
		 * Rather than truncating and growing the file each time,
		 * use ftruncate if size is less than previous size.
		 */
		whod = open(path, O_WRONLY | O_CREAT, 0644);
		if (whod < 0) {
			syslog(LOG_WARNING, "%s: %m", path);
			continue;
		}
#if ENDIAN != BIG_ENDIAN
		{
			int i, n = (cc - WHDRSIZE)/sizeof(struct whoent);
			struct whoent *we;

			/* undo header byte swapping before writing to file */
			wd.wd_sendtime = ntohl(wd.wd_sendtime);
			for (i = 0; i < 3; i++)
				wd.wd_loadav[i] = ntohl(wd.wd_loadav[i]);
			wd.wd_boottime = ntohl(wd.wd_boottime);
			we = wd.wd_we;
			for (i = 0; i < n; i++) {
				we->we_idle = ntohl(we->we_idle);
				we->we_utmp.out_time =
				    ntohl(we->we_utmp.out_time);
				we++;
			}
		}
#endif
		(void) time((time_t *)&wd.wd_recvtime);
		(void) write(whod, (char *)&wd, cc);
		if (fstat(whod, &st) < 0 || st.st_size > cc)
			ftruncate(whod, cc);
		(void) close(whod);
	}
}

/*
 * Check out host name for unprintables
 * and other funnies before allowing a file
 * to be created.  Sorry, but blanks aren't allowed.
 */
verify(name)
	register char *name;
{
	register int size = 0;

	while (*name) {
		if (!isascii(*name) || !(isalnum(*name) || ispunct(*name)))
			return (0);
		if (isupper(*name))
			*name = tolower(*name);
		name++, size++;
	}
	return (size > 0);
}

int	utmptime;
int	utmpent;
int	utmpsize = 0;
struct	utmp *utmp;
int	alarmcount;

void
onalrm()
{
	register struct neighbor *np;
	register struct whoent *we = mywd.wd_we, *wlast;
	register int i;
	struct stat stb;
	int cc;
	struct tbl_loadavg load;
	time_t now = time((time_t *)NULL);
	char *strerror();

	if (alarmcount % 10 == 0)
		getkmem();
	alarmcount++;
	(void) fstat(utmpf, &stb);
	if ((stb.st_mtime != utmptime) || (stb.st_size > utmpsize)) {
		utmptime = stb.st_mtime;
		if (stb.st_size > utmpsize) {
			utmpsize = stb.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (! utmp) {
				fprintf(stderr, MSGSTR( RWHOD_MALLOC_FAIL,
					"rwhod: malloc failed\n"));
				utmpsize = 0;
				goto done;
			}
		}
		(void) lseek(utmpf, (off_t)0, L_SET);
		cc = read(utmpf, (char *)utmp, stb.st_size);
		if (cc < 0) {
			fprintf(stderr, MSGSTR( RWHOD_UTMP_ERR, 
				"rwhod: %s: %s\n"), _PATH_UTMP, strerror(errno));
			goto done;
		}
		wlast = &mywd.wd_we[1024 / sizeof (struct whoent) - 1];
		utmpent = cc / sizeof (struct utmp);
		for (i = 0; i < utmpent; i++)
			if (utmp[i].ut_name[0] && (utmp[i].ut_type == USER_PROCESS) && (strcmp(utmp[i].ut_name,"LOGIN") != 0)) {
				bcopy(utmp[i].ut_line, we->we_utmp.out_line,
				   sizeof (utmp[i].ut_line));
				bcopy(utmp[i].ut_name, we->we_utmp.out_name,
				   sizeof (utmp[i].ut_name));
				we->we_utmp.out_time = htonl(utmp[i].ut_time);
				if (we >= wlast)
					break;
				we++;
			}
		utmpent = we - mywd.wd_we;
	}

	/*
	 * The test on utmpent looks silly---after all, if no one is
	 * logged on, why worry about efficiency?---but is useful on
	 * (e.g.) compute servers.
	 */
	if (utmpent && chdir(_PATH_DEV)) {
		syslog(LOG_ERR, "chdir(%s): %m", _PATH_DEV);
		exit(1);
	}
	we = mywd.wd_we;
	for (i = 0; i < utmpent; i++) {
		if (stat(we->we_utmp.out_line, &stb) >= 0)
			we->we_idle = htonl(now - stb.st_atime);
		we++;
	}
	if (table(TBL_LOADAVG, 0, (char *)&load, 1, sizeof(load)) != -1) {
		long    longvar;
                union avenrun {
                        long    l[3];
                        double  d[3];
                } *ave;

		ave = (union avenrun *)&load.tl_avenrun;
		for (i = 0; i < 3; i++) {
			if (load.tl_lscale != 0) {
				longvar = ((float)ave->l[i] / (float)load.tl_lscale) * 100;
				longvar &= NETLONG_MAX;
				mywd.wd_loadav[i] = htonl(longvar);
			} else {
				longvar = ave->d[i] * 100;
				longvar &= NETLONG_MAX;
				mywd.wd_loadav[i] = htonl(longvar);
			}
		}
	}
	cc = (char *)we - (char *)&mywd;
	mywd.wd_sendtime = htonl(time(0));
	mywd.wd_vers = WHODVERSION;
	mywd.wd_type = WHODTYPE_STATUS;
	for (np = neighbors; np != NULL; np = np->n_next)
		(void) sendto(s, (char *)&mywd, cc, 0,
			np->n_addr, np->n_addrlen);
	if (utmpent && chdir(_PATH_RWHODIR)) {
		syslog(LOG_ERR, "chdir(%s): %m", _PATH_RWHODIR);
		exit(1);
	}
done:
	(void) alarm(AL_INTERVAL);
}

void
getkmem()
{
	struct utmp     boot_time;

	boot_time.ut_type = BOOT_TIME;
	if ((utmp = getutid(&boot_time)) == NULL) {
        	printf(MSGSTR( RWHOD_NO_BOOT, "No boot time in utmp file\n"));
                exit(1);
        }
	mywd.wd_boottime = htonl( utmp->ut_time);
}

/*
 * Figure out device configuration and select
 * networks which deserve status information.
 */
configure(s)
	int s;
{
	char buf[BUFSIZ], *cp, *cplim;
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	struct sockaddr_in *sin;
	register struct neighbor *np;

	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		syslog(LOG_ERR, "ioctl (get interface configuration)");
		return (0);
	}
	ifr = ifc.ifc_req;
#if defined(AF_LINK) && defined(notyet)
#define max(a, b) (a > b ? a : b)
#define size(p)	max((p).sa_len, sizeof(p))
#else
#define size(p) (sizeof (p))
#endif
	cplim = buf + ifc.ifc_len; /*skip over if's with big ifr_addr's */
	for (cp = buf; cp < cplim;
			cp += sizeof (ifr->ifr_name) + size(ifr->ifr_addr)) {
		ifr = (struct ifreq *)cp;
		for (np = neighbors; np != NULL; np = np->n_next)
			if (np->n_name &&
			    strcmp(ifr->ifr_name, np->n_name) == 0)
				break;
		if (np != NULL)
			continue;
		ifreq = *ifr;
		np = (struct neighbor *)malloc(sizeof (*np));
		if (np == NULL)
			continue;
		np->n_name = malloc(strlen(ifr->ifr_name) + 1);
		if (np->n_name == NULL) {
			free((char *)np);
			continue;
		}
		strcpy(np->n_name, ifr->ifr_name);
		np->n_addrlen = sizeof (ifr->ifr_addr);
		np->n_addr = malloc(np->n_addrlen);
		if (np->n_addr == NULL) {
			free(np->n_name);
			free((char *)np);
			continue;
		}
		bcopy((char *)&ifr->ifr_addr, np->n_addr, np->n_addrlen);
		if (ioctl(s, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
			syslog(LOG_ERR, "ioctl (get interface flags)");
			free((char *)np);
			continue;
		}
		if ((ifreq.ifr_flags & IFF_UP) == 0 ||
		    (ifreq.ifr_flags & (IFF_BROADCAST|IFF_POINTOPOINT)) == 0) {
			free((char *)np);
			continue;
		}
		np->n_flags = ifreq.ifr_flags;
		if (np->n_flags & IFF_POINTOPOINT) {
			if (ioctl(s, SIOCGIFDSTADDR, (char *)&ifreq) < 0) {
				syslog(LOG_ERR, "ioctl (get dstaddr)");
				free((char *)np);
				continue;
			}
			/* we assume addresses are all the same size */
			bcopy((char *)&ifreq.ifr_dstaddr,
			  np->n_addr, np->n_addrlen);
		}
		if (np->n_flags & IFF_BROADCAST) {
			if (ioctl(s, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
				syslog(LOG_ERR, "ioctl (get broadaddr)");
				free((char *)np);
				continue;
			}
			/* we assume addresses are all the same size */
			bcopy((char *)&ifreq.ifr_broadaddr,
			  np->n_addr, np->n_addrlen);
		}
		/* gag, wish we could get rid of Internet dependencies */
		sin = (struct sockaddr_in *)np->n_addr;
		sin->sin_port = sp->s_port;
		np->n_next = neighbors;
		neighbors = np;
	}
	return (1);
}

#ifdef DEBUG
sendto(s, buf, cc, flags, to, tolen)
	int s;
	char *buf;
	int cc, flags;
	char *to;
	int tolen;
{
	register struct whod *w = (struct whod *)buf;
	register struct whoent *we;
	struct sockaddr_in *sin = (struct sockaddr_in *)to;
	char *interval();

	printf(MSGSTR( RWHOD_SENDTO,
		 "sendto %x.%d\n"), ntohl(sin->sin_addr), ntohs(sin->sin_port));
	printf(MSGSTR( RWHOD_HOSTNAME, "hostname %s %s\n"), w->wd_hostname,
	   interval(ntohl(w->wd_sendtime) - ntohl(w->wd_boottime), "  up"));
	printf(MSGSTR( RWHOD_LOAD, "load %4.2f, %4.2f, %4.2f\n"),
	    ntohl(w->wd_loadav[0]) / 100.0, ntohl(w->wd_loadav[1]) / 100.0,
	    ntohl(w->wd_loadav[2]) / 100.0);
	cc -= WHDRSIZE;
	for (we = w->wd_we, cc /= sizeof (struct whoent); cc > 0; cc--, we++) {
		time_t t = ntohl(we->we_utmp.out_time);
		printf(MSGSTR( RWHOD_FMT1, "%-8.8s %s:%s %.12s"),
			we->we_utmp.out_name,
			w->wd_hostname, we->we_utmp.out_line,
			ctime(&t)+4);
		we->we_idle = ntohl(we->we_idle) / 60;
		if (we->we_idle) {
			if (we->we_idle >= 100*60)
				we->we_idle = 100*60 - 1;
			if (we->we_idle >= 60)
				printf(MSGSTR( RWHOD_FMT2,
					 " %2d"), we->we_idle / 60);
			else
				printf(MSGSTR( RWHOD_SPACE, "   "));
			printf(MSGSTR( RWHOD_FMT3, ":%02d"), we->we_idle % 60);
		}
		printf(MSGSTR( RWHOD_NEWLINE, "\n"));
	}
}

char *
interval(time, updown)
	int time;
	char *updown;
{
	static char resbuf[32];
	int days, hours, minutes;

	if (time < 0 || time > 3*30*24*60*60) {
		(void) sprintf(resbuf, "   %s ??:??", updown);
		return (resbuf);
	}
	minutes = (time + 59) / 60;		/* round to minutes */
	hours = minutes / 60; minutes %= 60;
	days = hours / 24; hours %= 24;
	if (days)
		(void) sprintf(resbuf, "%s %2d+%02d:%02d",
		    updown, days, hours, minutes);
	else
		(void) sprintf(resbuf, "%s    %2d:%02d",
		    updown, hours, minutes);
	return (resbuf);
}
#endif

void usage()
{
	fprintf(stderr, MSGSTR(RWHOD_USAGE, "Usage: rwhod [-b | -l]\n"));
	exit(1);
}
