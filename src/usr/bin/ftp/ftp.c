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
static char	*sccsid = "@(#)$RCSfile: ftp.c,v $ $Revision: 4.2.10.2 $ (DEC) $Date: 1993/07/22 17:00:34 $";
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
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * ftp.c
 *
 *	Revision History:
 *
 * 12-Mar-91	Fred Canter
 *	MIPS C 2.20+
 *
 * 16-Apr-91    Mary Walker
 *      print out message on send/receive of zero length file
 */

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <varargs.h>

#include "ftp_var.h"

struct	sockaddr_in hisctladdr;
struct	sockaddr_in data_addr;
int	data = -1;
int	abrtflag = 0;
int	ptflag = 0;
struct	sockaddr_in myctladdr;
uid_t	getuid();
sig_t	lostpeer();
off_t	restart_point = 0;

extern	char *strerror();
extern	int connected, errno;

FILE	*cin, *cout;
FILE	*dataconn();

#ifdef IP_TOS
#define MAXALIASES	35 
#define BSIZ		512

static char TOSDB[] = "/etc/iptos" ;
static FILE *tosf = NULL;
struct tosent {
        char    *t_name;        /* name */
        char    **t_aliases;    /* alias list */
        char    *t_proto;       /* protocol */
        int     t_tos;          /* Type Of Service bits */
};


struct tosent *tp ;

static char * getcommon_any();
static struct tosent *interpret_local();
static struct tosent *gettosbyname(); 
static struct tosent *gettosent_local(); 
static int stayopen;

struct tosent *
gettosbyname_default(name,proto)
char *name, *proto;
{
	struct tosent tos;
	char *aliasp = 0;

	tos.t_name = name;
	tos.t_proto = proto;
	tos.t_aliases = &aliasp;
	if (strcmp(name, "ftp-data") == 0)
		tos.t_tos = IPTOS_THROUGHPUT;
	else
		tos.t_tos = IPTOS_LOWDELAY;
	return(&tos);
}

struct tosent *	
gettosbyname(name,proto)

   char *name, *proto ;

{
	register struct tosent *p;
	register char **cp;
	
	settosent_local(0);
	while(p = gettosent_local()) {
        if (strcmp(name,p->t_name) == 0)
		goto gotname ;
	for (cp = p->t_aliases; *cp; cp++)
		if (strcmp(name, *cp) == 0)
			goto gotname ;
	continue ;
gotname:
		if ((proto == 0) || (*p->t_proto == '*') ||
		    (strcmp(p->t_proto, proto) == 0))
		         break ;
       }
       endtosent_local();
       return(p) ;
}
struct tosent *
gettosent_local()

{

	static char line1[BSIZ+1];
 
        if (tosf == NULL && (tosf = fopen(TOSDB,"r")) == NULL)
		return(NULL);
	if (fgets(line1, BSIZ, tosf) == NULL)
		return (NULL) ;
	return interpret_local(line1, strlen(line1));
}

static struct tosent *
interpret_local(val,len)
	char *val;
	int len;
{
	static char *tos_aliases[MAXALIASES];
	static struct tosent tos ;
	static char line[BSIZ+1];
	char *p = NULL;
	register char *cp, **q;

	strncpy(line, val, len) ;
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return(gettosent_local());
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		return(gettosent_local());
	*cp = '\0' ;
	tos.t_name = p;			/* get application name */
	p = getcommon_any(p, " \t") ;
	if (p == NULL)
		return(gettosent_local());
	*p++ = '\0' ;
	while (*p == ' ' || *p == '\t')
		p++ ;
        tos.t_proto = p;		/* get protocol */
        p = getcommon_any(p, " \t") ;
	if (p == NULL)
		return (gettosent_local());
        *p++ = '\0' ;
	while (*p == ' ' || *p == '\t')
		p++ ;
	tos.t_tos = strtol(p,NULL,0);		/* get TOS bits */
	q = tos.t_aliases = tos_aliases ;
	cp = getcommon_any(p, " \t") ;
	if (cp != NULL)
		*cp++ = '\0' ;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++ ;
			continue;
		}
		if (q < &tos_aliases[MAXALIASES - 1])
			*q++ = cp ;		/* get aliases */
		cp = getcommon_any(cp, " \t") ;
		if (cp != NULL) 
			*cp++ = '\0';
	}
	*q = NULL;
	return(&tos) ;
}

static char *
getcommon_any(cp,match) 
	register char *cp;
	char *match;
{
        register char *mp, c;

        while (c = *cp) {
                for (mp = match; *mp; mp++)
                        if (*mp == c)
                                return (cp);
                cp++;
        }
        return ((char *)0);
}

settosent_local(f)
        int f;
{
        if (tosf == NULL)
                tosf = fopen(TOSDB, "r");
        else
                rewind(tosf);
        stayopen |= f;
}
endtosent_local()
{
        if (tosf && !stayopen) {
                fclose(tosf);
                tosf = NULL;
        }
}
#endif /* IP_TOS */

char *
hookup(host, port)
	char *host;
	int port;
{
	register struct hostent *hp = 0;
	int s,len;
	static char hostnamebuf[MAXHOSTNAMELEN];
#ifdef IP_TOS
	struct tosent *tos;
#endif

	bzero((char *)&hisctladdr, sizeof (hisctladdr));
	hisctladdr.sin_addr.s_addr = inet_addr(host);
	if (hisctladdr.sin_addr.s_addr != -1) {
		hisctladdr.sin_family = AF_INET;
		(void) strncpy(hostnamebuf, host, sizeof (hostnamebuf));
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, 
				MSGSTR(UNKNOWNHOST,"ftp: Unknown Host %s\n"), 
				host);
			herror((char *)NULL);
			code = -1;
			return((char *) 0);
		}
		hisctladdr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0],
		    (caddr_t)&hisctladdr.sin_addr, hp->h_length);
		(void) strncpy(hostnamebuf, hp->h_name, sizeof(hostnamebuf));
	}
	hostname = hostnamebuf;
	s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		perror("ftp: socket");
		code = -1;
		return (0);
	}
	hisctladdr.sin_port = port;
	while (connect(s, (struct sockaddr *)&hisctladdr, sizeof(hisctladdr)) < 0) {
		if (hp && hp->h_addr_list[1]) {
			int oerrno = errno;
			extern	char *inet_ntoa();

			fprintf(stderr, MSGSTR(CONNECT, "ftp: connect to address %s: "),
				inet_ntoa(hisctladdr.sin_addr));
			errno = oerrno;
			perror((char *) 0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0],
			     (caddr_t)&hisctladdr.sin_addr, hp->h_length);
			fprintf(stdout, MSGSTR(TRYING, "Trying %s...\n"),
				inet_ntoa(hisctladdr.sin_addr));
			(void) close(s);
			s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
			if (s < 0) {
				perror("ftp: socket");
				code = -1;
				return (0);
			}
			continue;
		}
		perror("ftp: connect");
		code = -1;
		goto bad;
	}
	len = sizeof (myctladdr);
	if (getsockname(s, (struct sockaddr *)&myctladdr, &len) < 0) {
		perror("ftp: getsockname");
		code = -1;
		goto bad;
	}
#ifdef IP_TOS
	if ((tos = gettosbyname("ftp-control", "tcp")) == NULL)
		tos = gettosbyname_default("ftp-control", "tcp");
        if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos->t_tos, sizeof(int)) < 0)
                perror("ftp: setsockopt TOS (ignored)");
#endif
	cin = fdopen(s, "r");
	cout = fdopen(s, "w");
	if (cin == NULL || cout == NULL) {
		fprintf(stderr, MSGSTR(FDOPEN_FAIL, "ftp: fdopen failed.\n"));
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
	if (verbose)
		printf(MSGSTR(CONNECTED, "Connected to %s.\n"), hostname);
	if (getreply(0) > 2) { 	/* read startup message from server */
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on))
		< 0 && debug) {
			perror("ftp: setsockopt");
		}
	}
#endif /* SO_OOBINLINE */

	return (hostname);
bad:
	(void) close(s);
	return ((char *)0);
}

login(host)
	char *host;
{
	char tmp[80];
	char *user, *pass, *acct, *getlogin(), *mygetpass();
	int n, aflag = 0;

	user = pass = acct = 0;
		if (ruserpass(host, &user, &pass, &acct) < 0) {
			code = -1;
			return(0);
		}
	while (user == NULL) {
		char *myname = getlogin();

		if (myname == NULL || *myname == NULL) {
			struct passwd *pp = getpwuid(getuid());

			if (pp != NULL)
				myname = pp->pw_name;
		}
		if (myname)
			printf(MSGSTR(FTP_NAME, "Name (%s:%s): "), host, myname);
		else
			printf(MSGSTR(FTP_NAME1, "Name (%s): "), host);
			
		(void) fgets(tmp, sizeof(tmp) - 1, stdin);
		tmp[strlen(tmp) - 1] = '\0';
		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = mygetpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		acct = mygetpass("Account:");
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		fprintf(stderr, MSGSTR(LOGIN_FAIL, "Login failed.\n"));
		return (0);
	}
	if (!aflag && acct != NULL)
		(void) command("ACCT %s", acct);
	if (proxy)
		return(1);
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

void
cmdabort()
{
	extern jmp_buf ptabort;

	printf("\n");
	(void) fflush(stdout);
	abrtflag++;
	if (ptflag)
		longjmp(ptabort,1);
}

/*VARARGS1*/
command(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;
	int r; 
	sig_t oldintr;
	void cmdabort();

	abrtflag = 0;
	if (debug) {
		printf("---> ");
		va_start(ap);
                fmt = va_arg(ap, char *);
                if (strncmp("PASS ", fmt, 5) == 0)
                        printf("PASS XXXX");
                else
                        vfprintf(stdout, fmt, ap);
                va_end(ap);
                printf("\n");
		(void) fflush(stdout);
	}
	if (cout == NULL) {
		perror (MSGSTR(NO_CONTROL, "No control connection for command"));
		code = -1;
		return (0);
	}
	oldintr = signal(SIGINT, cmdabort);
	va_start(ap);
        fmt = va_arg(ap, char *);
        vfprintf(cout, fmt, ap);
        va_end(ap);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)();
	(void) signal(SIGINT, oldintr);
	return(r);
}

char reply_string[BUFSIZ];

#include <ctype.h>

getreply(expecteof)
	int expecteof;
{
	register int c, n;
	register int dig;
	register char *cp;
	int originalcode = 0, continuation = 0; 
	sig_t oldintr;
	int pflag = 0;
	char *pt = pasv;
	void cmdabort();

	oldintr = signal(SIGINT, cmdabort);
	for (;;) {
		dig = n = code = 0;
		cp = reply_string;
		while ((c = getc(cin)) != '\n') {
			if (c == IAC) {     /* handle telnet commands */
				switch (c = getc(cin)) {
				case WILL:
				case WONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,WONT,c);
					(void) fflush(cout);
					break;
				case DO:
				case DONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,DONT,c);
					(void) fflush(cout);
					break;
				default:
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
					(void) signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (verbose) {
					printf(MSGSTR(SERVICE_421, "421 Service not available, remote server has closed connection\n"));
					(void) fflush(stdout);
				}
				code = 421;
				return(4);
			}
			if (c != '\r' && (verbose > 0 ||
			    (verbose > -1 && n == '5' && dig > 4))) {
				if (proxflag &&
				   (dig == 1 || dig == 5 && verbose == 0))
					printf("%s:",hostname);
				(void) putchar(c);
			}
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (n == 0)
				n = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		}
		if (verbose > 0 || verbose > -1 && n == '5') {
			(void) putchar(c);
			(void) fflush (stdout);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (n != '1')
			cpend = 0;
		(void) signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabort && oldintr != SIG_IGN)
			(*oldintr)();
		return (n - '0');
	}
}

empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (int) sec;
	t.tv_usec = 0;
	return(select(32, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	sendabort;

void
abortsend()
{

	mflag = 0;
	abrtflag = 0;
	printf(MSGSTR(NSEND_ABORT, "\nsend aborted\n"));
	(void) fflush(stdout);
	longjmp(sendabort, 1);
}

sendrequest(cmd, local, remote, printnames)
	char *cmd, *local, *remote;
	int printnames;
{
	struct stat st;
	struct timeval start, stop;
	register int c, d;
	FILE *fin, *dout = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	sig_t oldintr, oldintp;
	int bytes = 0, hashbytes = HASHBYTES;
	char *lmode, buf[BUFSIZ], *bufp;
	void abortsend();


	if (verbose && printnames) {
                if (local && *local != '-')
                        printf(MSGSTR( LOCAL, "local: %s "), local);
                if (remote)
                        printf( MSGSTR( REMOTE, "remote: %s\n"), remote);
	}
	if (proxy) {
		proxtrans(cmd, local, remote);
		return;
	}
	if (curtype != type)
                changetype(type, 0);
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	lmode = "w";
	if (setjmp(sendabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT,oldintr);
		if (oldintp)
			(void) signal(SIGPIPE,oldintp);
		code = -1;
		return;
	}
	oldintr = signal(SIGINT, abortsend);
	if (strcmp(local, "-") == 0)
		fin = stdin;
	else if (*local == '|') {
		oldintp = signal(SIGPIPE,SIG_IGN);
                fin = popen(local + 1, "r");
		if (fin == NULL) {
			perror(local + 1);
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGPIPE, oldintp);
			code = -1;
			return;
		}
		closefunc = pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			perror(local);
			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stdout, MSGSTR(PLAIN_FILE, "%s: not a plain file.\n"), local);
			(void) signal(SIGINT, oldintr);
			(void) fclose(fin);
			code = -1;
			return;
		}
	}
	if (initconn()) {
		(void) signal(SIGINT, oldintr);
		if (oldintp)
			(void) signal(SIGPIPE, oldintp);
		code = -1;
		if (closefunc != NULL)
		        (*closefunc) (fin);
		return;
	}
	if (setjmp(sendabort))
		goto abort;

	if (restart_point &&
            (strcmp(cmd, "STOR") == 0 || strcmp(cmd, "APPE") == 0)) {
                if (fseek(fin, (long) restart_point, 0) < 0) {
                        fprintf(stderr, "local: %s: %s\n", local,
                                strerror(errno));
			restart_point = 0;
                        if (closefunc != NULL)
                                (*closefunc)(fin);
                        return;
                }
		if (command("REST %ld", (long) restart_point)
                        != CONTINUE) {
                        restart_point = 0;
                        if (closefunc != NULL)
                                (*closefunc)(fin);
                        return;
		 }
                restart_point = 0;
                lmode = "r+w";
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
                                (*closefunc)(fin);
			return;
		}
	} else
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
                        (*closefunc)(fin);
			return;
		}
	dout = dataconn(lmode);
	if (dout == NULL)
		goto abort;
	(void) gettimeofday(&start, (struct timezone *)0);
	oldintp = signal(SIGPIPE, SIG_IGN);
	switch (curtype) {

	case TYPE_I:
	case TYPE_L:
		errno = d = 0;
		while ((c = read(fileno(fin), buf, sizeof (buf))) > 0) {
			bytes += c;
			for (bufp = buf; c > 0; c -= d, bufp += d)
				if ((d = write(fileno(dout), bufp, c)) <= 0)
					break;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			perror(local);
		if (d <= 0) {
			if (d == 0)
				fprintf(stderr, MSGSTR( NETOUT,"netout: write returned 0?\n"));
			else if (errno != EPIPE) 
				perror("netout");
			bytes = -1;
		}
		break;

	case TYPE_A:
		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
				if (ferror(dout))
					break;
				(void) putc('\r', dout);
				bytes++;
			}
			(void) putc(c, dout);
			bytes++;
	/*		if (c == '\r') {			  	*/
	/*		(void)	putc('\0', dout);  /* this violates rfc */
	/*			bytes++;				*/
	/*		}                          			*/	
		}
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(fin))
			fprintf(stderr, "local: %s: %s\n", local,
                                strerror(errno));
		if (ferror(dout)) {
			if (errno != EPIPE)
				perror("netout");
			bytes = -1;
		}
		break;
	}
	(void) gettimeofday(&stop, (struct timezone *)0);
	if (closefunc != NULL)
		(*closefunc)(fin);
	(void) fclose(dout);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (bytes > 0)
		ptransfer("sent", bytes, &start, &stop, local, remote);
	return;
abort:
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (!cpend) {
		code = -1;
		return;
	}
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (dout)
		(void) fclose(dout);
	(void) getreply(0);
	code = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	if (bytes > 0 )
		ptransfer("sent", bytes, &start, &stop, local, remote);
}

jmp_buf	recvabort;

void
abortrecv()
{

	mflag = 0;
	abrtflag = 0;
	printf("\nreceive aborted\nwaiting for remote to finish abort\n");
	(void) fflush(stdout);
	longjmp(recvabort, 1);
}

recvrequest(cmd, local, remote, lmode, printnames)
	char *cmd, *local, *remote, *lmode;
	int printnames;
{
	FILE *fout, *din = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	sig_t oldintr, oldintp; 
	int is_retr, tcrflag, bare_lfs = 0;
	char *gunique();
	static int bufsize;
	static char *buf;
	int bytes = 0, hashbytes = HASHBYTES;
	register int c, d;
	struct timeval start, stop;
	struct stat st;
	off_t	lseek();
	void abortrecv();
	char *malloc();


	is_retr = strcmp(cmd, "RETR") == 0;
	if (is_retr && verbose && printnames) {
                if (local && *local != '-')
                        printf("local: %s ", local);
                if (remote)
                        printf("remote: %s\n", remote);
        }
	if (proxy && is_retr) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	tcrflag = !crflag && is_retr;
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	oldintr = signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|') {
		if (access(local, 2) < 0) {
			char *dir = rindex(local, '/');

			if (errno != ENOENT && errno != EACCES) {
	 			fprintf(stderr, "local: %s: %s\n", local,
                                	strerror(errno));
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (dir != NULL)
                                *dir = 0;
                        d = access(dir ? (*local ? local : "/") : ".", 2);
                        if (dir != NULL)
                                *dir = '/';
			if (d < 0) {
				fprintf(stderr, "local: %s: %s\n", local,
					strerror(errno));
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (!runique && errno == EACCES &&
			    chmod(local,0600) < 0) {
				fprintf(stderr, "local: %s: %s\n", local,
                                        strerror(errno));
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (runique && errno == EACCES &&
			   (local = gunique(local)) == NULL) {
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
	}
	if (!is_retr) {
                if (curtype != TYPE_A)
                        changetype(TYPE_A, 0);
        } else if (curtype != type)
                changetype(type, 0);
	if (initconn()) {
		(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	if (setjmp(recvabort))
		goto abort;
	if (is_retr && restart_point &&
            command("REST %ld", (long) restart_point) != CONTINUE)
                return;
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			return;
		}
	} else {
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	din = dataconn("r");
	if (din == NULL)
		goto abort;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
                fout = popen(local + 1, "w");
		if (fout == NULL) {
			perror(local+1);
			goto abort;
		}
		closefunc = pclose;
	} else {
		fout = fopen(local, lmode);
		if (fout == NULL) {
			fprintf(stderr, "local: %s: %s\n", local,
                                strerror(errno));
			goto abort;
		}
		closefunc = fclose;
	}
	if (fstat(fileno(fout), &st) < 0 || st.st_blksize == 0)
		st.st_blksize = BUFSIZ;
	if (st.st_blksize > bufsize) {
		if (buf)
			(void) free(buf);
		buf = malloc((unsigned)st.st_blksize);
		if (buf == NULL) {
			perror("malloc");
			bufsize = 0;
			goto abort;
		}
		bufsize = st.st_blksize;
	}
	(void) gettimeofday(&start, (struct timezone *)0);
	switch (curtype) {

	case TYPE_I:
	case TYPE_L:
		if (restart_point &&
                    lseek(fileno(fout), (off_t) restart_point, L_SET) < 0) {
                        fprintf(stderr, "local: %s: %s\n", local,
                                strerror(errno));
                        if (closefunc != NULL)
                        	(*closefunc)(fout);
                        return;
                }
		errno = d = 0;
		while ((c = read(fileno(din), buf, bufsize)) > 0) {
			if ((d = write(fileno(fout), buf, c)) != c)
				break;
			bytes += c;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0) {
			if (errno != EPIPE)
				perror("netin");
			bytes = -1;
		}
		if (d < c) {
			if (d < 0)
				fprintf(stderr, "local: %s: %s\n", local,
                                        strerror(errno));
			else
				fprintf(stderr, MSGSTR(SHORTWRITE, "%s: short write\n"), local);
		}
		break;

	case TYPE_A:
		if (restart_point) {
                        register int i, n, ch;

                        if (fseek(fout, 0L, L_SET) < 0)
                                goto done;
                        n = restart_point;
			for (i = 0; i++ < n;) {
                                if ((ch = getc(fout)) == EOF)
                                        goto done;
                                if (ch == '\n')
                                        i++;
			 }
                        if (fseek(fout, 0L, L_INCR) < 0) {
done:
                                fprintf(stderr, "local: %s: %s\n", local,
                                        strerror(errno));

			if (closefunc != NULL)
                                        (*closefunc)(fout);
                                return;
                        }
                }
		while ((c = getc(din)) != EOF) {
			if (c == '\n')
                                bare_lfs++;
			while (c == '\r') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
				bytes++;
				if ((c = getc(din)) != '\n' || tcrflag) {
					if (ferror(fout))
						goto break2;
					(void) putc('\r', fout);
					if (c == '\0') {
                                                bytes++;
                                                goto contin2;
                                        }
                                        if (c == EOF)
                                      		goto contin2;
				}
			}
			(void) putc(c, fout);
			bytes++;
	contin2:	;
		}
break2:
		if (bare_lfs && type == TYPE_A) {
	printf("WARNING! %d bare linefeeds received in ASCII mode\n", bare_lfs);
                        printf("File may not have transferred correctly.\n");
                }
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(din)) {
			if (errno != EPIPE)
				perror("netin");
			bytes = -1;
		}
		if (ferror(fout))
			perror(local);
		break;
	}
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) fclose(din);
	(void) getreply(0);
	if (bytes > 0 && is_retr)
		ptransfer("received", bytes, &start, &stop, local, remote);
	return;
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	(void) gettimeofday(&stop, (struct timezone *)0);
	if (oldintp)
		(void) signal(SIGPIPE, oldintr);
	(void) signal(SIGINT,SIG_IGN);
	if (!cpend) {
		code = -1;
		(void) signal(SIGINT,oldintr);
		return;
	}

	abort_remote(din);

/* send IAC in urgent mode instead of DM because UNIX places oob mark */
/* after urgent byte rather than before as now is protocol            */
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		(void) fclose(din);
	if (bytes > 0 )
		ptransfer("received", bytes, &start, &stop, local, remote);
	(void) signal(SIGINT,oldintr);
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */

initconn()
{
	register char *p, *a;
	int result, len, tmpno = 0;
	int on = 1;
#ifdef IP_TOS
	struct tosent *tos;
#endif

noport:
	data_addr = myctladdr;
	if (sendport)
		data_addr.sin_port = 0;	/* let system pick one */ 
	if (data != -1)
		(void) close(data);
	data = socket(AF_INET, SOCK_STREAM, 0);
	if (data < 0) {
		perror("ftp: socket");
		if (tmpno)
			sendport = 1;
		return (1);
	}
	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			perror(MSGSTR(REUSE_ADDR, "ftp: setsockopt (resuse address)"));
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr, sizeof (data_addr)) < 0) {
		perror("ftp: bind");
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		perror(MSGSTR(IGNORED, "ftp: setsockopt (ignored)"));
	len = sizeof (data_addr);
	if (getsockname(data, (struct sockaddr *)&data_addr, &len) < 0) {
		perror("ftp: getsockname");
		goto bad;
	}
	if (listen(data, 1) < 0)
		perror("ftp: listen");
	if (sendport) {
		a = (char *)&data_addr.sin_addr;
		p = (char *)&data_addr.sin_port;
#define	UC(b)	(((int)b)&0xff)
		result =
		    command("PORT %d,%d,%d,%d,%d,%d",
		      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		      UC(p[0]), UC(p[1]));
		if (result == ERROR && sendport == -1) {
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
		return (result != COMPLETE);
	}
	if (tmpno)
		sendport = 1;
#ifdef IP_TOS
	if ((tos = gettosbyname("ftp-data", "tcp")) == NULL)
		tos = gettosbyname_default("ftp-data", "tcp");
        if (setsockopt(data, IPPROTO_IP, IP_TOS, (char *)&tos->t_tos, sizeof(int)) < 0)
                perror("ftp: setsockopt TOS (ignored)");
#endif
	return (0);
bad:
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

FILE *
dataconn(lmode)
	char *lmode;
{
	struct sockaddr_in from;
	int s, fromlen = sizeof (from);
#ifdef IP_TOS
	struct tosent *tos;
#endif

	s = accept(data, (struct sockaddr *) &from, &fromlen);
	if (s < 0) {
		perror("ftp: accept");
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
#ifdef IP_TOS
	if ((tos = gettosbyname("ftp-data", "tcp")) == NULL)
		tos = gettosbyname_default("ftp-data", "tcp");
        if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos->t_tos, sizeof(int)) < 0)
                perror("ftp: setsockopt TOS (ignored)");
#endif
	return (fdopen(data, lmode));
}

ptransfer(direction, bytes, t0, t1, local, remote)
	char *direction, *local, *remote;
	int bytes;
	struct timeval *t0, *t1;
{
	struct timeval td;
	float s, bs;

	if (verbose) {
	    tvsub(&td, t1, t0);
	    s = td.tv_sec + (td.tv_usec / 1000000.);
#define	nz(x)	((x) == 0 ? 1 : (x))
	    bs = (bytes) / nz(s);
	    printf(MSGSTR(BYTES_SEC,"%ld bytes %s in %.2g seconds (%.2g Kbytes/s)\n"),
		bytes, direction, s, bs / 1024.);
	}
}

/*tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{

	tsum->tv_sec += t0->tv_sec;
	tsum->tv_usec += t0->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
} */

tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

void
psabort()
{
	extern int abrtflag;

	abrtflag++;
}

pswitch(flag)
	int flag;
{
	void psabort();
	extern int proxy, abrtflag;
	sig_t oldintr;
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
		struct sockaddr_in mctl;
		struct sockaddr_in hctl;
		FILE *in;
		FILE *out;
		int tpe;
		int curtpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
	} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	}
	else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	ip->hctl = hisctladdr;
	hisctladdr = op->hctl;
	ip->mctl = myctladdr;
	myctladdr = op->mctl;
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
        type = op->tpe;
	ip->curtpe = curtype;
	curtype = op->curtpe;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
	(void) signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)();
	}
}

jmp_buf ptabort;
int ptabflg;

void
abortpt()
{
	printf("\n");
	(void) fflush(stdout);
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	longjmp(ptabort, 1);
}

proxtrans(cmd, local, remote)
	char *cmd, *local, *remote;
{
	sig_t oldintr;
	int secndflag = 0, prox_type, nfnd;
	extern jmp_buf ptabort;
	char *cmd2;
	struct fd_set mask;
	void abortpt();

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if ((prox_type = type) == 0) {
                if (unix_server && unix_proxy)
                        prox_type = TYPE_I;
                else
                        prox_type = TYPE_A;
        }
        if (curtype != prox_type)
                changetype(prox_type, 1);
	if (command("PASV") != COMPLETE) {
		printf(MSGSTR(PROXY_3RD, "proxy server does not support third part transfers.\n"));
		return;
	}
	pswitch(0);
	if (!connected) {
		printf(MSGSTR(PRIMARY, "No primary connection\n"));
		pswitch(1);
		code = -1;
		return;
	}
	if (curtype != prox_type)
                changetype(prox_type, 1);
        if (command("PORT %s", pasv) != COMPLETE) {
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
	oldintr = signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote) != PRELIM) {
		(void) signal(SIGINT, oldintr);
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	pswitch(1);
	ptflag = 0;
	printf(MSGSTR(LOCAL_REMOTE, "local: %s remote: %s\n"), local, remote);
	return;
abort:
	(void) signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			if (cpend)
                                abort_remote((FILE *) NULL);
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
		(void) signal(SIGINT, oldintr);
		return;
	}
	if (cpend)
		abort_remote((FILE *) NULL);
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			if (cpend)
                		abort_remote((FILE *) NULL);
			pswitch(1);
			if (ptabflg)
				code = -1;
			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend)
		abort_remote((FILE *) NULL);
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	pswitch(1);
	if (ptabflg)
		code = -1;
	(void) signal(SIGINT, oldintr);
}

reset()
{
	struct fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,0)) < 0) {
			perror("reset");
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			(void) getreply(0);
		}
	}
}

char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp = rindex(local, '/');
	int d, count=0;
	char ext = '1';

	if (cp)
		*cp = '\0';
	d = access(cp ? local : ".", 2);
	if (cp)
		*cp = '/';
	if (d < 0) {
		fprintf(stderr, "local: %s: %s\n", local, strerror(errno));
		return((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			printf(MSGSTR(RUNIQUE, "runique: can't find unique file name.\n"));
			return((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return(new);
}

abort_remote(din)
FILE *din;
{
        char buf[BUFSIZ];
        int nfnd;
        struct fd_set mask;

	/*
	 * send IAC in urgent mode instead of DM because 4.3BSD places oob
         * mark after urgent byte rather than before as is protocol now
         */
        sprintf(buf, "%c%c%c", IAC, IP, IAC);
        if (send(fileno(cout), buf, 3, MSG_OOB) != 3)
		perror("abort");
        fprintf(cout,"%cABOR\r\n", DM);
        (void) fflush(cout);
        FD_ZERO(&mask);
        FD_SET(fileno(cin), &mask);
	if (din) {
                FD_SET(fileno(din), &mask);
        }
        if ((nfnd = empty(&mask, 10)) <= 0) {
                if (nfnd < 0) {
                        perror("abort");
                }
		if (ptabflg)
                        code = -1;
                lostpeer();
        }
        if (din && FD_ISSET(fileno(din), &mask)) {
                while (read(fileno(din), buf, BUFSIZ) > 0)
                        /* LOOP */;
        }
	if (getreply(0) == ERROR && code == 552) {
                /* 552 needed for nic style abort */
                (void) getreply(0);
        }
        (void) getreply(0);
}
