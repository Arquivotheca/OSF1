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
static char	*sccsid = "@(#)$RCSfile: slattach.c,v $ $Revision: 4.2.5.3 $ (DEC) $Date: 1993/06/16 17:19:50 $";
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
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Adams.
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
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#include <sys/param.h>
#include <sgtty.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <paths.h>
#include <signal.h>

/* SLIP interface flag settings */
#define SC_COMPRESS	0x0002		/* compress TCP headers */
#define SC_NOICMP	0x0004		/* suppress ICMP traffic */
#define SC_AUTOCOMP	0x0008		/* auto-enable TCP header compression */

#define DEFAULT_BAUD	9600

#define PIDDIR		"/var/run/"

int	slipdisc = SLIPDISC;

char	devname[32];
char	hostname[MAXHOSTNAMELEN];
char    pidfile[24];

void  terminate();



main(argc, argv)
	int argc;
	char *argv[];
{
	register int fd;
	register char *dev;
	struct sgttyb sgtty;
	char *tmpdev;
        FILE *fp;
	pid_t sl_pid;
	int	speed;
	int	flags;
	int	set_flags = 0;		/* flags to be set */
	int	clear_flags = 0;	/* flags to be cleared */

	if (argc < 2) {
		fprintf(stderr, "usage:  slattach [{+|-} {c|e|i}] ttyname [baudrate]\n");
		exit(1);
	}
	while (argv[1][0] == '-' || argv[1][0] == '+') {
		if (argc <= 2) {
			fprintf(stderr, "usage:  slattach [{+|-} {c|e|i}] ttyname [baudrate]\n");
			exit(1);
		}
		switch (argv[1][1]) {
		case 'c':	/* enable/disable TCP header compression */
			if (argv[1][0] == '-')
				clear_flags |= SC_COMPRESS;
			else
				set_flags |= SC_COMPRESS;
			break;
		case 'e':	/* enable/disable auto-enable of TCP hdr comp.*/
			if (argv[1][0] == '-')
				clear_flags |= SC_AUTOCOMP;
			else
				set_flags |= SC_AUTOCOMP;
			break;
		case 'i':	/* enable/disable ICMP suppression */
			if (argv[1][0] == '-')
				clear_flags |= SC_NOICMP;
			else
				set_flags |= SC_NOICMP;
			break;
		default:
			fprintf(stderr,"%s: unknown flag: `%s'\n", argv[0], argv[1]);
			fprintf(stderr, "usage: slattach [{+|-}{c|e|i}] ttyname [baudrate]\n");
			exit(1);
		}
		--argc;
		++argv;
	}
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: slattach [{+|-}{c|e|i}] ttyname [baudrate]\n");
		exit(1);
	}

	/* */

	(void) signal(SIGTERM,terminate);
	(void) signal(SIGINT,terminate);
	(void) signal(SIGQUIT,terminate);
	(void) signal(SIGKILL,terminate);

	sl_pid = getpid();
	dev = argv[1];
	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) 
	   sprintf(pidfile,"%s%s.pid",PIDDIR,dev);
	else{
	   tmpdev = dev + 5;
	   sprintf(pidfile,"%s%s.pid",PIDDIR,tmpdev);
	}
        fp = fopen(pidfile,"w");
        if (fp != NULL) {
           fprintf(fp,"%d\n",sl_pid);
           (void) fclose(fp);
        }

	speed = argc == 3 ? findspeed(atoi(argv[2])) : findspeed(DEFAULT_BAUD);
	if (speed == 0) {
		fprintf(stderr, "unknown speed: %s\n", argv[2]);
		exit(1);
	}
	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		(void)sprintf(devname, "%s/%s", _PATH_DEV, dev);
		dev = devname;
	}
	if ((fd = open(dev, O_RDWR | O_NDELAY)) < 0) {
		perror(dev);
		exit(1);
	}
	sgtty.sg_flags = RAW | ANYP;
	sgtty.sg_ispeed = sgtty.sg_ospeed = speed;
	if (ioctl(fd, TIOCSETP, &sgtty) < 0) {
		perror("ioctl(TIOCSETP)");
		exit(1);
	}
	if (ioctl(fd, TIOCSETD, &slipdisc) < 0) {
		perror("ioctl(TIOCSETD)");
		exit(1);
	}
	if (clear_flags || set_flags) {
		/* Get current interface flags */
		if (ioctl(fd, SLIOCGFLAGS, &flags) < 0) {
			perror("ioctl(SLIOCGFLAGS)");
			exit(1);
		}
		flags &= ~clear_flags;
		flags |= set_flags;
		/* Set new interface flags */
		if (ioctl(fd, SLIOCSFLAGS, &flags) < 0) {
			perror("ioctl(SLIOCSFLAGS)");
			exit(1);
		}
	}

	if (fork() > 0)
		exit(0);
	for (;;)
		sigpause(0L);
}

struct sg_spds {
	int sp_val, sp_name;
}       spds[] = {
#ifdef B50
	{ 50, B50 },
#endif
#ifdef B75
	{ 75, B75 },
#endif
#ifdef B110
	{ 110, B110 },
#endif
#ifdef B150
	{ 150, B150 },
#endif
#ifdef B200
	{ 200, B200 },
#endif
#ifdef B300
	{ 300, B300 },
#endif
#ifdef B600
	{ 600, B600 },
#endif
#ifdef B1200
	{ 1200, B1200 },
#endif
#ifdef B1800
	{ 1800, B1800 },
#endif
#ifdef B2000
	{ 2000, B2000 },
#endif
#ifdef B2400
	{ 2400, B2400 },
#endif
#ifdef B3600
	{ 3600, B3600 },
#endif
#ifdef B4800
	{ 4800, B4800 },
#endif
#ifdef B7200
	{ 7200, B7200 },
#endif
#ifdef B9600
	{ 9600, B9600 },
#endif
#ifdef EXTA
	{ 19200, EXTA },
#endif
#ifdef EXTB
	{ 38400, EXTB },
#endif
	{ 0, 0 }
};

findspeed(speed)
	register int speed;
{
	register struct sg_spds *sp;

	sp = spds;
	while (sp->sp_val && sp->sp_val != speed)
		sp++;
	return (sp->sp_name);
}

/* Remove the pid file and exit. */

void terminate()
{
	unlink(pidfile);
	exit(0);
}
