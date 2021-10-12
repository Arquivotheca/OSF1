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
static char	*sccsid = "@(#)$RCSfile: syslogd.c,v $ $Revision: 4.2.7.2 $ (DEC) $Date: 1993/05/10 02:19:17 $";
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
 * syslogd.c
 *
 *	Modification History:
 *
 * 20-Jun-91    Mary Walker
 *       -put in OSF 1.0.1 bug fix
 *
 * 05-Jun-91	Scott Cranston
 *       - Added binary error logging support
 *       - Added msgbuf recover from dump file
 *
 * 01-Apr-91	Fred Canter
 *	MIPS C 2.20+, changes for -std
 *
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

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 * more extensive changes by Eric Allman (again)
 */

#define	MAXLINE		1024		/* maximum line length */
#define	MAXSVLINE	120		/* maximum saved line length */
#define DEFUPRI		(LOG_USER|LOG_NOTICE)
#define DEFSPRI		(LOG_KERN|LOG_CRIT)
#define TIMERINTVL	30		/* interval for checking flush, mark */

#include <sys/secdefines.h>

#include <stdio.h>
#include <utmp.h>
#include <ctype.h>
#include <strings.h>
#include <setjmp.h>

#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/msgbuf.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>

#include <netinet/in.h>
#include <netdb.h>


char mbsp[MAXLINE];                      /* msgbuf save path name */

char	*LogName = "/dev/log";
char	*ConfFile = "/etc/syslog.conf";
char	*PidFile = "/var/run/syslog.pid";
char	ctty[] = "/dev/console";
char	klog[] = "/dev/klog";
char	kcon[] = "/dev/kcon";

#define FDMASK(fd)	(1 << (fd))

#define	dprintf		if (Debug) printf

#define UNAMESZ		8	/* length of a login name */
#define MAXUNAMES	20	/* maximum number of user names */
#define MAXFNAME	200	/* max file pathname length */

#define NOPRI		0x10	/* the "no priority" priority */
#define	LOG_MARK	LOG_MAKEPRI(LOG_NFACILITIES, 0)	/* mark "facility" */

/*
 * Flags to logmsg().
 */

#define IGN_CONS	0x001	/* don't print on console */
#define SYNC_FILE	0x002	/* do fsync on file after printing */
#define ADDDATE		0x004	/* add a date to the message */
#define MARK		0x008	/* this message is a mark */

/*
 * This structure represents the files that will have log
 * copies printed.
 */

struct filed {
	struct	filed *f_next;		/* next in linked list */
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	union {
		char	f_uname[MAXUNAMES][UNAMESZ+1];
		struct {
			char	f_hname[MAXHOSTNAMELEN+1];
			struct sockaddr_in	f_addr;
		} f_forw;		/* forwarding address */
		char	f_fname[MAXFNAME];
	} f_un;
	char	f_prevline[MAXSVLINE];		/* last message logged */
	char	f_lasttime[16];			/* time of last occurrence */
	char	f_prevhost[MAXHOSTNAMELEN+1];	/* host from which recd. */
	int	f_prevpri;			/* pri of f_prevline */
	int	f_prevlen;			/* length of f_prevline */
	int	f_prevcount;			/* repetition cnt of prevline */
	int	f_repeatcount;			/* number of "repeated" msgs */
};

/*
 * Intervals at which we flush out "message repeated" messages,
 * in seconds after previous message is logged.  After each flush,
 * we move to the next interval until we reach the largest.
 */
int	repeatinterval[] = { 30, 120, 600 };	/* # of secs before flush */
#define	MAXREPEAT ((sizeof(repeatinterval) / sizeof(repeatinterval[0])) - 1)
#define	REPEATTIME(f)	((f)->f_time + repeatinterval[(f)->f_repeatcount])
#define	BACKOFF(f)	{ if (++(f)->f_repeatcount > MAXREPEAT) \
				 (f)->f_repeatcount = MAXREPEAT; \
			}

/* values for f_type */
#define F_UNUSED	0		/* unused entry */
#define F_FILE		1		/* regular file */
#define F_TTY		2		/* terminal */
#define F_CONSOLE	3		/* console terminal */
#define F_FORW		4		/* remote machine */
#define F_USERS		5		/* list of users */
#define F_WALL		6		/* everyone logged on */

char	*TypeNames[7] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL"
};

struct	filed *Files;
struct	filed consfile;

int	Debug;			/* debug flag */
char	LocalHostName[MAXHOSTNAMELEN+1];	/* our hostname */
char	*LocalDomain;		/* our local domain name */
int	InetInuse = 0;		/* non-zero if INET sockets are being used */
int	finet;			/* Internet datagram socket */
int	LogPort;		/* port number for INET connections */
int	Initialized = 0;	/* set when we have initialized ourselves */
int	MarkInterval = 20 * 60;	/* interval between marks in seconds */
int	MarkSeq = 0;		/* mark sequence number */

/* Used only for dated directories */
static char *MonthTab[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

#define SYSLOGDIR	"/var/adm/syslog.dated"

#define DATESIZE	13
short MadeDatedDir = 0;
char CurDateDir[DATESIZE];

#define	CHECKDIRINTVL	(5 * 60)

int checkdirseq = 0;

extern daemon();		/* from libutil */

extern	int errno, sys_nerr;
extern	char *sys_errlist[];
#ifdef NO_PROTO
extern	char *ctime(), *index(), *calloc();
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *p;
	int nfds, readfds, len;
	int fklog, fkcon, funix;
	int klogm, kconm, unixm, inetm;
	struct sockaddr_un sunx, fromunix;
	struct sockaddr_in sin, frominet;
	struct stat sbuf;
	FILE *fp;
	char line[MSG_BSIZE + 1];
	extern void die(), domark(), init(), reapchild();

	while (--argc > 0) {
		p = *++argv;
		if (p[0] != '-')
			usage();
		switch (p[1]) {
		case 'f':		/* configuration file */
			if (p[2] != '\0')
				ConfFile = &p[2];
			break;

		case 'd':		/* debug */
			Debug++;
			break;

		case 'p':		/* path */
			if (p[2] != '\0')
				LogName = &p[2];
			break;

		case 'm':		/* mark interval */
			if (p[2] != '\0')
				MarkInterval = atoi(&p[2]) * 60;
			if (MarkInterval <= 0)
				MarkInterval = 20 * 60; /* as before */
			break;

		default:
			usage();
		}
	}

	if (!Debug) 
		daemon(0,0);	/* start our own session */
	else
		setlinebuf(stdout);

	consfile.f_type = F_CONSOLE;
	(void) strcpy(consfile.f_un.f_fname, ctty);
	(void) gethostname(LocalHostName, sizeof LocalHostName);
	if (p = index(LocalHostName, '.')) {
		*p++ = '\0';
		LocalDomain = p;
	}
	else
		LocalDomain = "";
	(void) signal(SIGTERM, die);
	if (Debug) {
		(void) signal(SIGINT, die);
		(void) signal(SIGQUIT, die);
	} else {
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
	}
	(void) signal(SIGCHLD, reapchild);
	(void) signal(SIGALRM, domark);
	(void) alarm(TIMERINTVL);
	(void) unlink(LogName);

	unixm = inetm = klogm = kconm = 0;
	sunx.sun_family = AF_UNIX;
	(void) strncpy(sunx.sun_path, LogName, sizeof sunx.sun_path);
#if BSD44
	/* sunx.sun_len = 0;
	   don't know if we really should do this or if its
	   done by the kernel
	   sunx.sun_len = sizeof(sunx.sun_len) + sizeof(sunx.sun_family)
		       + sizeof(sunx.sun_path) + 1;
	*/
#endif
	funix = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (funix < 0 || bind(funix, (struct sockaddr *) &sunx,
	    sizeof(sunx.sun_family)
#if BSD44
        /*    + sizeof(sunx.sun_len) */
#endif
	    + strlen(sunx.sun_path)) < 0 ||
#if SEC_MAC
            chslabel(LogName, NULL) < 0 ||      /* set WILDCARD mac label */
#endif
	    chmod(LogName, 0666) < 0) {
		(void) sprintf(line, "cannot create %s", LogName);
		logerror(line);
		dprintf("cannot create %s (%d)\n", LogName, errno);
		die(0);
	}
	unixm = FDMASK(funix);

	finet = socket(AF_INET, SOCK_DGRAM, 0);
	if (finet >= 0) {
		struct servent *sp;

		sp = getservbyname("syslog", "udp");
		if (sp == NULL) {
			errno = 0;
			logerror("syslog/udp: unknown service");
			die(0);
		}
		sin.sin_family = AF_INET;
		sin.sin_port = LogPort = sp->s_port;
                sin.sin_addr.s_addr = INADDR_ANY;
		if (bind(finet, &sin, sizeof(sin)) < 0) {
			logerror("bind");
			if (!Debug)
				die(0);
		} else {
			inetm = FDMASK(finet);
			InetInuse = 1;
		}
	}

	if ((fklog = open(klog, O_RDONLY)) >= 0) {
		klogm = FDMASK(fklog);
	} else {
		dprintf("can't open %s (%d)\n", klog, errno);
	}

	if ((fkcon = open(kcon, O_RDONLY)) >= 0) {
		kconm = FDMASK(fkcon);
	} else if (errno != ENOENT) {
		dprintf("can't open %s (%d)\n", kcon, errno);
	} else if (stat(klog, &sbuf) < 0) {
		dprintf("stat of %s failed (%d)\n", klog, errno);
	} else if (!S_ISCHR(sbuf.st_mode)) {
		dprintf("%s is not a character special file\n", klog);
	} else if (mknod(kcon, sbuf.st_mode, sbuf.st_rdev + 1) < 0) {
		dprintf("can't create %s (%d)\n", kcon, errno);
	} else if ((fkcon = open(kcon, O_RDONLY)) < 0) {
		dprintf("can't open %s (%d)\n", kcon, errno);
	} else {
		kconm = FDMASK(fkcon);
	}

	/* tuck my process id away */
	fp = fopen(PidFile, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	dprintf("off & running....\n");

	init();

        read_dump();   /* process msgbuf saved from a dump */

	(void) signal(SIGHUP, init);

	for (;;) {
		errno = 0;
		readfds = klogm | kconm | unixm | inetm;
		dprintf("readfds = %#x\n", readfds);
		nfds = select(20, (fd_set *) &readfds, (fd_set *) NULL,
				  (fd_set *) NULL, (struct timeval *) NULL);
		if (nfds == 0)
			continue;
		if (nfds < 0) {
			if (errno != EINTR)
				logerror("select");
			continue;
		}
		dprintf("got a message (%d, %#x)\n", nfds, readfds);
		if (readfds & klogm) {
			i = read(fklog, line, sizeof(line) - 1);
			if (i > 0) {
				line[i] = '\0';
				printsys(line);
			} else if (i < 0 && errno != EINTR) {
				close(fklog);
				logerror("klog");
				fklog = -1;
				klogm = 0;
			}
		}
		if (readfds & kconm) {
			i = read(fkcon, line, sizeof(line) - 1);
			if (i > 0) {
				printcons(line, i);
			} else if (i < 0 && errno != EINTR) {
				close(fkcon);
				logerror("kcon");
				fkcon = -1;
				kconm = 0;
			}
		}
		if (readfds & unixm) {
			len = sizeof fromunix;
			i = recvfrom(funix, line, MAXLINE, 0,
				     (struct sockaddr *) &fromunix, &len);
			if (i > 0) {
				line[i] = '\0';
				printline(LocalHostName, line);
			} else if (i < 0 && errno != EINTR)
				logerror("recvfrom unix");
		}
		if (readfds & inetm) {
			len = sizeof frominet;
			i = recvfrom(finet, line, MAXLINE, 0, &frominet, &len);
			if (i > 0) {
				extern char *cvthname();

				line[i] = '\0';
				printline(cvthname(&frominet), line);
			} else if (i < 0 && errno != EINTR)
				logerror("recvfrom inet");
		} 
	}
}

usage()
{
	fprintf(stderr, "usage: syslogd [-d] [-mmarkinterval] [-ppath] [-fconffile]\n");
	exit(1);
}

untty()
{
	int i;

	if (!Debug) {
		i = open("/dev/tty", O_RDWR);
		if (i >= 0) {
			(void) ioctl(i, (int) TIOCNOTTY, (char *)0);
			(void) close(i);
		}
	}
}

/*
 * Write a raw line from the /dev/kcon pseudo-device to the system console.
 */

printcons(buf, len)
	char *buf;
	int len;
{
	static int consfd = -1;

	if (consfd >= 0 || (consfd = open(ctty, O_WRONLY)) >= 0) {
		if (write(consfd, buf, len) < 0) {
			/* here for a second try after console logout */
			close(consfd);
			if ((consfd = open(ctty, O_WRONLY)) >= 0) {
				if (write(consfd, buf, len) < 0) {
					close(consfd);
					consfd = -1;
				}
			}
		}
	}
}

/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */

printline(hname, msg)
	char *hname;
	char *msg;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri;

	/* test for special codes */
	pri = DEFUPRI;
	p = msg;
	if (*p == '<') {
		pri = 0;
		while (isdigit(*++p))
			pri = 10 * pri + (*p - '0');
		if (*p == '>')
			++p;
	}

	if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFUPRI;

	/* don't allow users to log kernel messages */
	if (LOG_FAC(pri) == LOG_KERN)
		pri = LOG_MAKEPRI(LOG_USER, LOG_PRI(pri));

	q = line;

	while ((c = *p++ & 0177) != '\0' && c != '\n' &&
	    q < &line[sizeof(line) - 1]) {
		if (iscntrl(c)) {
			*q++ = '^';
			*q++ = c ^ 0100;
		} else
			*q++ = c;
	}
	*q = '\0';

	logmsg(pri, line, hname, 0);
}

/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */

printsys(msg)
	char *msg;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri, flags;
	char *lp;

	(void) sprintf(line, "vmunix: ");
	lp = line + strlen(line);
	for (p = msg; *p != '\0'; ) {
	    flags = SYNC_FILE | ADDDATE;	/* fsync file after write */
	    pri = DEFSPRI;
	    if (*p == '<') {
		   pri = 0;
		   while (isdigit(*++p))
			pri = 10 * pri + (*p - '0');
		   if (*p == '>')
		        ++p;
	     } else {
		   /* kernel printf's come out on console */
		   flags |= IGN_CONS;
	     }

	       if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
			pri = DEFSPRI;
	       q = lp;
	       while (*p != '\0' && (c = *p++) != '\n' &&
		    q < &line[MAXLINE])
			*q++ = c;
	       *q = '\0';
	       logmsg(pri, line, LocalHostName, flags);
	}
}

time_t	now;

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */

logmsg(pri, msg, from, flags)
	int pri;
	char *msg, *from;
	int flags;
{
	register struct filed *f;
	int fac, prilev;
	int omask, msglen;
	char *timestamp;

	dprintf("logmsg: pri %o, flags %x, from %s, msg %s\n", pri, flags, from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	/*
	 * Check to see if msg looks non-standard.
	 */
	msglen = strlen(msg);
	if (msglen < 16 || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	(void) time(&now);
	if (flags & ADDDATE)
		timestamp = ctime(&now) + 4;
	else {
		timestamp = msg;
		msg += 16;
		msglen -= 16;
	}

	/* extract facility and priority level */
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	else
		fac = LOG_FAC(pri);
	prilev = LOG_PRI(pri);

	/* log the message to the particular outputs */
	if (!Initialized) {
		f = &consfile;
		f->f_file = open(ctty, O_WRONLY);

		if (f->f_file >= 0) {
			untty();
			fprintlogmsg(f, flags, msg, from,
				     msglen, pri, timestamp);
			(void) close(f->f_file);
		}
		(void) sigsetmask(omask);
		return;
	}
	for (f = Files; f; f = f->f_next) {
		/* skip messages that are incorrect priority */
		if (f->f_pmask[fac] < prilev || f->f_pmask[fac] == NOPRI)
			continue;

		if (f->f_type == F_CONSOLE && (flags & IGN_CONS))
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < MarkInterval / 2)
			continue;

		/*
		 * suppress duplicate lines to this file
		 */
		if ((flags & MARK) == 0 && msglen == f->f_prevlen &&
		    !strcmp(msg, f->f_prevline) &&
		    !strcmp(from, f->f_prevhost)) {
			(void) strncpy(f->f_lasttime, timestamp, 15);
			f->f_prevcount++;
			dprintf("msg repeated %d times, %d sec of %d\n",
			    f->f_prevcount, now - f->f_time,
			    repeatinterval[f->f_repeatcount]);
			/*
			 * If domark would have logged this by now,
			 * flush it now (so we don't hold isolated messages),
			 * but back off so we'll flush less often
			 * in the future.
			 */
			if (now > REPEATTIME(f)) {
				fprintlog(f, flags, (char *)NULL);
				BACKOFF(f);
			}
		} else {
			/* new line, save it */
			if (f->f_prevcount)
				fprintlog(f, 0, (char *)NULL);
			fprintlogmsg(f, flags, msg, from,
				     msglen, pri, timestamp);
		}
	}
	(void) sigsetmask(omask);
}





fprintlogmsg(f, flags, msg, from, msglen, pri, timestamp)
	register struct filed *f;
	int flags;
	char *msg, *from;
	int msglen, pri;
	char *timestamp;
{
	f->f_repeatcount = 0;
	(void) strncpy(f->f_lasttime, timestamp, 15);
	(void) strncpy(f->f_prevhost, from,
			sizeof(f->f_prevhost));
	if (msglen < MAXSVLINE) {
		f->f_prevlen = msglen;
		f->f_prevpri = pri;
		(void) strcpy(f->f_prevline, msg);
		fprintlog(f, flags, (char *)NULL);
	} else {
		f->f_prevline[0] = 0;
		f->f_prevlen = 0;
		fprintlog(f, flags, msg);
	}
}

fprintlog(f, flags, msg)
	register struct filed *f;
	int flags;
	char *msg;
{
	struct iovec iov[6];
	register struct iovec *v = iov;
	register int l;
	char line[MAXLINE + 1];
	char repbuf[80];

	v->iov_base = f->f_lasttime;
	v->iov_len = 15;
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	v->iov_base = f->f_prevhost;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	if (msg) {
		v->iov_base = msg;
		v->iov_len = strlen(msg);
	} else if (f->f_prevcount > 1) {
		(void) sprintf(repbuf, "last message repeated %d times",
		    f->f_prevcount);
		v->iov_base = repbuf;
		v->iov_len = strlen(repbuf);
	} else {
		v->iov_base = f->f_prevline;
		v->iov_len = f->f_prevlen;
	}
	v++;

	dprintf("Logging to %s", TypeNames[f->f_type]);
	f->f_time = now;

	switch (f->f_type) {
	case F_UNUSED:
		dprintf("\n");
		break;

	case F_FORW:
		dprintf(" %s\n", f->f_un.f_forw.f_hname);
		(void) sprintf(line, "<%d>%.15s %s", f->f_prevpri,
			iov[0].iov_base, iov[4].iov_base);
		l = strlen(line);
		if (l > MAXLINE)
			l = MAXLINE;
		if (sendto(finet, line, l, 0, &f->f_un.f_forw.f_addr,
		    sizeof f->f_un.f_forw.f_addr) != l) {
			int e = errno;
			(void) close(f->f_file);
			f->f_type = F_UNUSED;
			errno = e;
			logerror("sendto");
		}
		break;

	case F_CONSOLE:
		if (flags & IGN_CONS) {
			dprintf(" (ignored)\n");
			break;
		}
		/* FALLTHROUGH */

	case F_TTY:
	case F_FILE:
		dprintf(" %s\n", f->f_un.f_fname);
		if (f->f_type != F_FILE) {
			v->iov_base = "\r\n";
			v->iov_len = 2;
		} else {
			v->iov_base = "\n";
			v->iov_len = 1;
		}
	again:
		if (writev(f->f_file, iov, 6) < 0) {
			int e = errno;
			(void) close(f->f_file);
			/*
			 * Check for EBADF on TTY's due to vhangup() XXX
			 */
			if (e == EBADF && f->f_type != F_FILE) {
				f->f_file = open(f->f_un.f_fname, O_WRONLY|O_APPEND);
				if (f->f_file < 0) {
					f->f_type = F_UNUSED;
					logerror(f->f_un.f_fname);
				} else {
					untty();
					goto again;
				}
			} else {
				f->f_type = F_UNUSED;
				errno = e;
				logerror(f->f_un.f_fname);
			}
		} else if (flags & SYNC_FILE)
			(void) fsync(f->f_file);
		break;

	case F_USERS:
	case F_WALL:
		dprintf("\n");
		v->iov_base = "\r\n";
		v->iov_len = 2;
		wallmsg(f, iov);
		break;
	}
	f->f_prevcount = 0;
}

jmp_buf ttybuf;

void
endtty()
{
	longjmp(ttybuf, 1);
}

/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */

wallmsg(f, iov)
	register struct filed *f;
	struct iovec *iov;
{
	void endtty();
	register char *p;
	register int i;
	int ttyf, len;
	FILE *uf;
	static int reenter = 0;
	struct utmp ut;
	char greetings[200];

	if (reenter++)
		return;

	/* open the user login file */
	if ((uf = fopen("/var/adm/utmp", "r")) == NULL) {
		logerror("/var/adm/utmp");
		reenter = 0;
		return;
	}

	/*
	 * Might as well fork instead of using nonblocking I/O
	 * and doing notty().
	 */
	if (fork() == 0) {
		(void) signal(SIGTERM, SIG_DFL);
		(void) alarm(0);
		(void) signal(SIGALRM, endtty);
		(void) signal(SIGTTOU, SIG_IGN);
		(void) sigsetmask(0);
		(void) sprintf(greetings,
		    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
			iov[2].iov_base, ctime(&now));
		len = strlen(greetings);

		/* scan the user login file */
		while (fread((char *) &ut, sizeof ut, 1, uf) == 1) {
			/* is this slot used? */
			if (ut.ut_name[0] == '\0')
				continue;

			/* should we send the message to this user? */
			if (f->f_type == F_USERS) {
				for (i = 0; i < MAXUNAMES; i++) {
					if (!f->f_un.f_uname[i][0]) {
						i = MAXUNAMES;
						break;
					}
					if (strncmp(f->f_un.f_uname[i],
					    ut.ut_name, UNAMESZ) == 0)
						break;
				}
				if (i >= MAXUNAMES)
					continue;
			}

			/* compute the device name */
			p = "/dev/12345678";
			strncpy(&p[5], ut.ut_line, UNAMESZ);

			if (f->f_type == F_WALL) {
				iov[0].iov_base = greetings;
				iov[0].iov_len = len;
				iov[1].iov_len = 0;
			}
			if (setjmp(ttybuf) == 0) {
				(void) alarm(15);
				/* open the terminal */
				ttyf = open(p, O_WRONLY);
				if (ttyf >= 0) {
					struct stat statb;

					if (fstat(ttyf, &statb) == 0 &&
					    (statb.st_mode & S_IWRITE))
						(void) writev(ttyf, iov, 6);
					close(ttyf);
					ttyf = -1;
				}
			}
			(void) alarm(0);
		}
		exit(0);
	}
	/* close the user login file */
	(void) fclose(uf);
	reenter = 0;
}

void
reapchild()
{
	int status;

	while (wait3((union wait *)&status, WNOHANG, (struct rusage *) NULL) > 0)
		;
}

/*
 * Return a printable representation of a host address.
 */
char *
cvthname(f)
	struct sockaddr_in *f;
{
	struct hostent *hp;
	register char *p;
	extern char *inet_ntoa();

	dprintf("cvthname(%s)\n", inet_ntoa(f->sin_addr));

	if (f->sin_family != AF_INET) {
		dprintf("Malformed from address\n");
		return ("???");
	}
	hp = gethostbyaddr((char *)&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0) {
		dprintf("Host name for your address (%s) unknown\n",
			inet_ntoa(f->sin_addr));
		return (inet_ntoa(f->sin_addr));
	}
	if ((p = index(hp->h_name, '.')) && strcmp(p + 1, LocalDomain) == 0)
		*p = '\0';
	return (hp->h_name);
}

void
domark()
{
	register struct filed *f;

	now = time(0);
	MarkSeq += TIMERINTVL;
	if (MarkSeq >= MarkInterval) {
		logmsg(LOG_INFO, "-- MARK --", LocalHostName, ADDDATE|MARK);
		MarkSeq = 0;
	}

	for (f = Files; f; f = f->f_next) {
		if (f->f_prevcount && now >= REPEATTIME(f)) {
			dprintf("flush %s: repeated %d times, %d sec.\n",
			    TypeNames[f->f_type], f->f_prevcount,
			    repeatinterval[f->f_repeatcount]);
			fprintlog(f, 0, (char *)NULL);
			BACKOFF(f);
		}
	}
	checkdirseq += TIMERINTVL;
	if ((checkdirseq >= CHECKDIRINTVL) && MadeDatedDir) {
		CheckDateDir();
		checkdirseq = 0;
	}
	(void) alarm(TIMERINTVL);
}

/*
 * Print syslogd errors some place.
 */
logerror(type)
	char *type;
{
	char buf[100];

	if (errno == 0)
		(void) sprintf(buf, "syslogd: %s", type);
	else if ((unsigned) errno > sys_nerr)
		(void) sprintf(buf, "syslogd: %s: error %d", type, errno);
	else
		(void) sprintf(buf, "syslogd: %s: %s", type, sys_errlist[errno]);
	errno = 0;
	dprintf("%s\n", buf);
	logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
}

void
die(sig)
{
	register struct filed *f;
	char buf[100];

	for (f = Files; f != NULL; f = f->f_next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);
	}
	if (sig) {
		dprintf("syslogd: exiting on signal %d\n", sig);
		(void) sprintf(buf, "exiting on signal %d", sig);
		errno = 0;
		logerror(buf);
	}
	(void) unlink(LogName);
	exit(0);
}

/*
 *  INIT -- Initialize syslogd from configuration table
 */

void
init()
{
	register int i;
	register FILE *cf;
	register struct filed *f, *next, **nextp;
	register char *p;
	char cline[BUFSIZ];

	dprintf("init\n");

	/*
	 *  Close all open log files.
	 */
	MadeDatedDir = 0;
	Initialized = 0;
	for (f = Files; f != NULL; f = next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);

		switch (f->f_type) {
		  case F_FILE:
		  case F_TTY:
		  case F_CONSOLE:
			(void) close(f->f_file);
			break;
		}
		next = f->f_next;
		free((char *) f);
	}
	Files = NULL;
	nextp = &Files;

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL) {
		dprintf("cannot open %s\n", ConfFile);
		*nextp = (struct filed *)calloc(1, sizeof(*f));
		cfline("*.ERR\t/dev/console", *nextp);
		(*nextp)->f_next = (struct filed *)calloc(1, sizeof(*f));
		cfline("*.PANIC\t*", (*nextp)->f_next);
		Initialized = 1;
		return;
	}


	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = NULL;
	while (fgets(cline, sizeof cline, cf) != NULL) {
		/*
		 * check for end-of-section, comments, strip off trailing
		 * spaces and newline character.
		 */
		for (p = cline; isspace(*p); ++p);
		if (*p == NULL || *p == '#')
			continue;
		for (p = index(cline, '\0'); isspace(*--p););
		*++p = '\0';
		f = (struct filed *)calloc(1, sizeof(*f));
		*nextp = f;
		nextp = &f->f_next;
		cfline(cline, f);
	}

	/* close the configuration file */
	(void) fclose(cf);

	Initialized = 1;

	if (Debug) {
		for (f = Files; f; f = f->f_next) {
			for (i = 0; i <= LOG_NFACILITIES; i++)
				if (f->f_pmask[i] == NOPRI)
					printf("X ");
				else
					printf("%d ", f->f_pmask[i]);
			printf("%s: ", TypeNames[f->f_type]);
			switch (f->f_type) {
			case F_FILE:
			case F_TTY:
			case F_CONSOLE:
				printf("%s", f->f_un.f_fname);
				break;

			case F_FORW:
				printf("%s", f->f_un.f_forw.f_hname);
				break;

			case F_USERS:
				for (i = 0; i < MAXUNAMES && *f->f_un.f_uname[i]; i++)
					printf("%s, ", f->f_un.f_uname[i]);
				break;
			}
			printf("\n");
		}
	}

	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
}

/*
 * Crack a configuration file line
 */

struct code {
	char	*c_name;
	int	c_val;
};

struct code	PriNames[] = {
	"panic",	LOG_EMERG,
	"emerg",	LOG_EMERG,
	"alert",	LOG_ALERT,
	"crit",		LOG_CRIT,
	"err",		LOG_ERR,
	"error",	LOG_ERR,
	"warn",		LOG_WARNING,
	"warning",	LOG_WARNING,
	"notice",	LOG_NOTICE,
	"info",		LOG_INFO,
	"debug",	LOG_DEBUG,
	"none",		NOPRI,
	NULL,		-1
};

struct code	FacNames[] = {
	"kern",		LOG_KERN,
	"user",		LOG_USER,
	"mail",		LOG_MAIL,
	"daemon",	LOG_DAEMON,
	"auth",		LOG_AUTH,
	"security",	LOG_AUTH,
	"mark",		LOG_MARK,
	"syslog",	LOG_SYSLOG,
	"lpr",		LOG_LPR,
	"news",		LOG_NEWS,
	"uucp",		LOG_UUCP,
	"local0",	LOG_LOCAL0,
	"local1",	LOG_LOCAL1,
	"local2",	LOG_LOCAL2,
	"local3",	LOG_LOCAL3,
	"local4",	LOG_LOCAL4,
	"local5",	LOG_LOCAL5,
	"local6",	LOG_LOCAL6,
	"local7",	LOG_LOCAL7,
	"msgbuf",	DUMP_FILE,
	NULL,		-1
};

cfline(line, f)
	char *line;
	register struct filed *f;
{
	register char *p;
	register char *q;
	register int i;
	char *bp;
	int pri;
	struct hostent *hp;
	char buf[MAXLINE];

	dprintf("cfline(%s)\n", line);

	errno = 0;	/* keep sys_errlist stuff out of logerror messages */

	/* clear out file entry */
	bzero((char *) f, sizeof *f);
	for (i = 0; i <= LOG_NFACILITIES; i++)
		f->f_pmask[i] = NOPRI;

	/* scan through the list of selectors */
	for (p = line; *p && *p != '\t';) {

		/* find the end of this facility name list */
		for (q = p; *q && *q != '\t' && *q++ != '.'; )
			continue;

		/* collect priority name */
		for (bp = buf; *q && !index("\t,;", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (index(", ;", *q))
			q++;

		/* decode priority name */
		pri = decode(buf, PriNames);
		if (pri < 0) {
			char xbuf[200];

			(void) sprintf(xbuf, "unknown priority name \"%s\"", buf);
			logerror(xbuf);
			return;
		}

		/* scan facilities */
		while (*p && !index("\t.;", *p)) {
			int i;

			for (bp = buf; *p && !index("\t,;.", *p); )
				*bp++ = *p++;
			*bp = '\0';
			if (*buf == '*')
				for (i = 0; i < LOG_NFACILITIES; i++)
					f->f_pmask[i] = pri;
			else {
				i = decode(buf, FacNames);

                                /* ignore dump file save path lines */
			        if (i == DUMP_FILE)
			            return;

				if (i < 0) {
					char xbuf[200];

					(void) sprintf(xbuf, "unknown facility name \"%s\"", buf);
					logerror(xbuf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	while (*p == '\t')
		p++;

	switch (*p)
	{
	case '@':
		if (!InetInuse)
			break;
		(void) strcpy(f->f_un.f_forw.f_hname, ++p);
		hp = gethostbyname(p);
		if (hp == NULL) {
			char buf[100];

			(void) sprintf(buf, "unknown host %s", p);
			errno = 0;
			logerror(buf);
			break;
		}
		bzero((char *) &f->f_un.f_forw.f_addr,
			 sizeof f->f_un.f_forw.f_addr);
		f->f_un.f_forw.f_addr.sin_family = AF_INET;
		f->f_un.f_forw.f_addr.sin_port = LogPort;
		bcopy(hp->h_addr, (char *) &f->f_un.f_forw.f_addr.sin_addr, hp->h_length);
		f->f_type = F_FORW;
		break;

	case '/':
		if (strncmp(SYSLOGDIR, p, sizeof(SYSLOGDIR)-1) == 0 &&
		    (bp = rindex(p, '/')) == p + sizeof(SYSLOGDIR)-1)
		{
			if ( !MadeDatedDir )
				MakeDateDir();
			*bp = '\0';
			(void) sprintf(f->f_un.f_fname, "%s/%s/%s",
				       p, CurDateDir, bp + 1);
			p = f->f_un.f_fname;
			if (access(p, F_OK)) {
				f->f_file = open(p, O_WRONLY|O_TRUNC|O_CREAT,
						 0640);
				if (f->f_file < 0 || chown(p, 0, -1) < 0) {
					f->f_file = F_UNUSED;
					logerror(p);
					break;
				}
				(void) close(f->f_file);
			}
		} else
			(void) strcpy(f->f_un.f_fname, p);
		if ((f->f_file = open(p, O_WRONLY|O_APPEND|O_CREAT,0640)) < 0)
		{
			f->f_file = F_UNUSED;
			logerror(p);
			break;
		}
		if (isatty(f->f_file)) {
			f->f_type = F_TTY;
			untty();
		}
		else
			f->f_type = F_FILE;
		if (strcmp(p, ctty) == 0)
			f->f_type = F_CONSOLE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		for (i = 0; i < MAXUNAMES && *p; i++) {
			for (q = p; *q && *q != ','; )
				q++;
			(void) strncpy(f->f_un.f_uname[i], p, UNAMESZ);
			if ((q - p) > UNAMESZ)
				f->f_un.f_uname[i][UNAMESZ] = '\0';
			else
				f->f_un.f_uname[i][q - p] = '\0';
			while (*q == ',' || *q == ' ')
				q++;
			p = q;
		}
		f->f_type = F_USERS;
		break;
	}
}


/*
 *  Decode a symbolic name to a numeric value
 */

decode(name, codetab)
	char *name;
	struct code *codetab;
{
	register struct code *c;
	register char *p;
	char buf[40];

	if (isdigit(*name))
		return (atoi(name));

	(void) strcpy(buf, name);
	for (p = buf; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}

time_t	CurDate;

/*
 * Find out which date directory the log files are supposed to be in.
 * The name of the current date directory is kept in DATEDIR, and
 * updated about every twenty-four hours.
 */
MakeDateDir()
{
	struct tm *tm, *localtime();
	char buf[MAXFNAME];

	(void) time((long *)&CurDate);
	tm = localtime(&CurDate);
	(void) sprintf(CurDateDir, "%02d-%s-%02d:%02d",
		tm->tm_mday,
		MonthTab[tm->tm_mon],
		tm->tm_hour,
		tm->tm_min);
	(void) sprintf(buf, "%s/%s", SYSLOGDIR, CurDateDir);
	(void) mkdir(SYSLOGDIR, 0755);
	(void) mkdir(buf, 0775);
	(void) chown(buf, 0, -1);
	MadeDatedDir = 1;
}

CheckDateDir()
{
	time_t curtime;

	(void) time((long *)&curtime);
	if (curtime - CurDate >= 60 * 60 * 24)
		(void) kill(getpid(), SIGHUP);
}



/*
 * get__savepath()
 *
 * This routine searches the /etc/syslog.conf file for a line like:
 *
 *          msgbuf.err		/var/adm/syslog/msgbuf.savecore
 *
 * This specifies the pathname of where to find the syslog kernel message
 * buffer saved from a crash dump.
 */

int get_savepath()
{
   register FILE *cf;
   register char *p;
   register char *q;
   char cline[BUFSIZ];

   /* open the configuration file */
   if ((cf = fopen(ConfFile, "r")) == NULL)
       return(0);     /* error can't open /etc/syslog.conf */

    /* search the file line by line */
    while (fgets(cline, sizeof cline, cf) != NULL) {

	  /* skip blank lines and comment lines */
	  for (p = cline; isspace(*p); ++p);
          if (*p == NULL || *p == '#')
	     continue;         /* try another line */

	  /* clean white space off the end of the line */
	  for (p = index(cline, '\0'); isspace(*--p););
	  *++p = '\0';

	  /* collect just the facility.priority name */
	  p = cline;
	  for (q = mbsp; *p && !index("\t,;", *p); )
	       *q++ = *p++;
	  *q = '\0'; 

	  /* is this the line we want ? */
	  if (strcmp("msgbuf.err", mbsp) != 0)
	     continue;          /* try another line */

	  /* skip to the path specifier */
          while (*p == '\t')
		p++;
          if (*p != '/')        /* must be a file.. can't forward */
	     continue;          /* try another line *

	  /* collect the path name */
	  for (q = mbsp; *p;)
	      *q++ = *p++;

	  return(1);          /* got it, all done */
    }
    return(0);       /* can't find it */
}


/*
 *  read_dump()
 *
 *  This routine check for a file that contains the kernel syslog message
 *  buffer (msgbuf) saved from a crash dump by savecore.  This
 *  file can have several copies of the msgbuf in it if the system kept
 *  crashing before this routine could run. The file is deleted after
 *  being processed.
 *
 *  This routine reads in the msgbuf from the dump save file (one msgbuf
 *  amount at a time) and puts it in a form that printsys() likes.
 */

read_dump()
{
   register i;
   register int  bx;               /* copy of msgbuf.msg_bufx */
   register int  br;               /* copy of msgbuf.msg_bufr */
   struct msgbuf *mb;
   int  ifd;                       /* input file descriptor */ 
   int  cc;                        /* misc char count */
   char *buf;                      /* work buffer */
   char line[MSG_BSIZE + 1];       /* buffer to give to printsys() */


   if (!get_savepath())  {          /* get pathname to the msgbuf dump file */
       dprintf("read_dump: 'msgbuf.err' not in syslog.conf\n");
       return;
   }
   ifd = open(mbsp, O_RDONLY, 0);    /* open the msgbuf dump file */
   if (ifd < 0)  {
      if (errno == ENOENT)  {
	 dprintf("read_dump: no msgbuf dump file found\n");
	 return;
      }
      logerror("read_dump: can't open dump file ");
      return;
   }
   if ((buf = (char *)malloc(sizeof(struct msgbuf))) == 0)  {
       dprintf("read_dump: could not malloc I/O buffer\n");
       close(ifd);
       return;
   }
   
   /*
    * read the file in 'sizeof(struct msgbuf)' chunks, anything
    * less than this ammount indicates a corrupted msgbuf dump file.
    *
    * We could have crashed multiple times before this runs so there could
    * several msgbuf's in the dump file.
    */
   while( (cc = read(ifd, buf, sizeof(struct msgbuf))) > 0 )  {
	if (cc < sizeof(struct msgbuf))  {
	   logerror("read_dump: msgbuf dump file corrupted");
	   break;
	}
        mb = (struct msgbuf *)buf;
	if (mb->msg_magic != MSG_MAGIC)
	    continue;                /* this msgbuf is invalid */

	bx = mb->msg_bufx;           /* logical end of msgbuf.msg_bufc */
	br = mb->msg_bufr;           /* logical begining of msgbuf.msg_bufc */

	/*
	 * we now need to extract the data from the msgbuf into another
	 * contigous buffer so that printsys() will like it.  It is
	 * possible that the data in the msgbuf could have wrapped aound.
	 */
         if (br < bx)  {
	     cc = bx - br;
	     for (i = 0; i < cc;)
		  line[i++] = mb->msg_bufc[br++];
	 } else  {                       /* handle wrapped buffer condition */
             cc = MSG_BSIZE - br;
	     for (i = 0; i < cc;)  
	          line[i++] = mb->msg_bufc[br++];
	     if ( br < 0  ||  br >= MSG_BSIZE )  {
	         br = 0;
	         for (i = 0; i < bx;)
	              line[i++] = mb->msg_bufc[br++];
	     }
	 }
	 dprintf("read_dump: recovering msgbuf from dump file\n");
	 printsys(line);       /* process and log the buffer contents */
   }
   free(buf);
   close(ifd);
   unlink(mbsp);      /*  delete the msgbuf dump file */
}
