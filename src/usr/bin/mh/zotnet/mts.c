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
/* mts.c - definitions for the mail transport system */
#ifndef	lint
static char ident[] = "@(#)$RCSfile: mts.c,v $ $Revision: 4.2.5.3 $ (DEC) $Date: 1993/12/21 21:41:06 $ devrcs Exp Locker: devbld $";
#endif	lint

/* LINTLIBRARY */

#undef	NETWORK
#if	defined(BSD41A) || defined(BSD42) || defined(SOCKETS)
#define	NETWORK
#endif	not (defined(BSD41A) || defined(BSD42) || defined(SOCKETS))

#include "../h/strings.h"
#include <ctype.h>
#include <stdio.h>
#include "mts.h"
#ifdef	NETWORK
#if	defined(BSD42) || defined(SOCKETS)
#include <netdb.h>
#endif	BSD42 or SOCKETS
#else	not NETWORK
#ifndef SYS5
#include <whoami.h>
#else SYS5
#include <sys/utsname.h>
#endif SYS5
#endif	not NETWORK
#include <pwd.h>


#define	NOTOK	(-1)
#define	OK	0

#define	NULLCP	((char *) 0)

extern int  errno;

static char   *tailor_value ();


#ifdef	SYS5
#define	index	strchr
#define	rindex	strrchr
#endif	SYS5

#ifdef OSF  /* use malloc declarations from stdlib.h */
char   *index (), *mktemp (), *rindex (), *strcpy ();
#else
char   *index (), *malloc (), *mktemp (), *rindex (), *strcpy ();
#endif

struct passwd  *getpwuid ();

/*  */

/*
   *mmdfldir and *uucpldir are the maildrop directories.  If maildrops
   are kept in the user's home directory, then these should be empty
   strings.  In this case, the appropriate ...lfil array should contain
   the name of the file in the user's home directory.  Usually, this is
   something like ".mail".
 */

static char *mtstailor = "/usr/lib/mh/mtstailor";

static char    *localname = "";
static char    *systemname = "";
#ifdef	MF
static char    *UUCPchan = "";
#endif	MF
char    *mmdfldir = "/usr/spool/mail";
char    *mmdflfil = "";
char    *uucpldir = "/usr/spool/mail";
char    *uucplfil = "";


char    *mmdlm1 = "\001\001\001\001\n";
char    *mmdlm2 = "\001\001\001\001\n";


static int  MMailids = 0;
static char *mmailid = "0";


#ifdef	MF
char   *umincproc = "/usr/lib/mh/uminc";
#else	MF
char   *umincproc = NULL;
#endif	MF


#if defined DOUBLE_LK
/* they are set within mts_init */
int	lockstyle;
static char *lkstyle = NULL;
#else
int	lockstyle = LOK_UNIX;
static char *lkstyle = "0";
#endif
char   *lockldir = "";

/*  */

/* MTS specific variables */

#ifdef	MHMTS
char   *Mailqdir = "/usr/spool/netmail";
char   *TMailqdir = "/usr/tmp";
int     Syscpy = 1;
static char *syscpy = "1";
char   *Overseer = "root";
char   *Mailer = "root";
char   *Fromtmp = "/tmp/rml.f.XXXXXX";
char   *Msgtmp = "/tmp/rml.m.XXXXXX";
char   *Errtmp = "/tmp/rml.e.XXXXXX";
int     Tmpmode = 0600;
static char *tmpmode = "0600";
char   *Okhosts = "/usr/lib/mh/Rmail.OkHosts";
char   *Okdests = "/usr/lib/mh/Rmail.OkDests";
#endif	MHMTS

#ifdef	MMDFMTS
#endif	MMDFMTS

#ifdef	SENDMTS
char   *hostable = "/usr/lib/mh/hosts";
char   *sendmail = "/usr/lib/sendmail";
#endif	SENDMTS


/* SMTP/POP stuff */

char   *servers = "localhost \01localnet";
char   *pophost = "";
char   *x400host = "";


/* BBoards-specific variables */

char   *bb_domain = "";


/* POP BBoards-specific variables */

#ifdef	BPOP
char    *popbbhost = "";
char    *popbbuser = "";
char    *popbblist = "/usr/lib/mh/hosts.popbb";
#endif	BPOP


/* MailDelivery */

char   *maildelivery = "/usr/lib/mh/maildelivery";


/* Aliasing Facility (doesn't belong here) */

int	Everyone = NOTOK;
static char *everyone = "-1";
char   *NoShell = "";

/*  */

/* customize the MTS settings for MH by reading /usr/lib/mh/mtstailor */

static  struct bind {
    char   *keyword;
    char  **value;
}       binds[] = {
    "localname", &localname,
    "systemname", &systemname,
#ifdef	MF
    "uucpchan", &UUCPchan,
#endif	MF
    "mmdfldir", &mmdfldir,
    "mmdflfil", &mmdflfil,
    "uucpldir", &uucpldir,
    "uucplfil", &uucplfil,
    "mmdelim1", &mmdlm1,
    "mmdelim2", &mmdlm2,
    "mmailid", &mmailid,
    "umincproc", &umincproc,
    "lockstyle", &lkstyle,
    "lockldir", &lockldir,

#ifdef	MHMTS
    "mailqdir", &Mailqdir,
    "tmailqdir", &TMailqdir,
    "syscpy", &syscpy,
    "overseer", &Overseer,
    "mailer", &Mailer,
    "fromtmp", &Fromtmp,
    "msgtmp", &Msgtmp,
    "errtmp", &Errtmp,
    "tmpmode", &tmpmode,
    "okhosts", &Okhosts,
    "okdests", &Okdests,
#endif	MHMTS

#ifdef	MMDFMTS
#endif	MMDFMTS

#ifdef	SENDMTS
    "hostable", &hostable,
    "sendmail", &sendmail,
#endif	SENDMTS

    "servers", &servers,
    "pophost", &pophost,
    "x400host", &x400host,

    "bbdomain", &bb_domain,

#ifdef	BPOP
    "popbbhost", &popbbhost,
    "popbbuser", &popbbuser,
    "popbblist", &popbblist,
#endif	BPOP
#ifdef	NNTP
    "nntphost", &popbbhost,
#endif	NNTP

    "maildelivery", &maildelivery,

    "everyone", &everyone,
    "noshell", &NoShell,

    NULL
};

/*  */

/* I'd like to use m_getfld() here, but not all programs loading mts.o may be
   MH-style programs... */

/* ARGSUSED */

mts_init (name)
char    *name;
{
    register char  *bp,
                   *cp;
    char    buffer[BUFSIZ];
    register struct bind   *b;
    register    FILE *fp;
    static int  inited = 0;
    char *np;

    if (inited++ || (fp = fopen (mtstailor, "r")) == NULL)
	return;

    while (fgets (buffer, sizeof buffer, fp)) {
	if ((cp = index (buffer, '\n')) == NULL)
	    break;
	*cp = NULL;
	if (*buffer == '#' || *buffer == NULL)
	    continue;
	if ((bp = index (buffer, ':')) == NULL)
	    break;
	*bp++ = NULL;
	while (isspace (*bp))
	    *bp++ = NULL;

	for (b = binds; b -> keyword; b++)
	    if (strcmp (buffer, b -> keyword) == 0)
		break;
	if (b -> keyword && (cp = tailor_value (bp)))
	    *b -> value = cp;
    }

    (void) fclose (fp);

    MMailids = atoi (mmailid);

    /*  Get the value from lkstyle */
#if defined DOUBLE_LK
# if defined OSF1 || defined OSF
    /*	atoi() returns 0 for any errors.  Here, we make sure that the
    **	value in mtstailor is a reasonable number.
    */
    lockstyle = strtol(lkstyle, &np, 0);
    if (!LOK_VALID(lockstyle) || lkstyle == np)
	lockstyle = LOK_DFLT;
# else
    lockstyle = atoi (lkstyle);
    if (!LOK_VALID(lockstyle))
	lockstyle = LOK_DFLT;
# endif /* OSF1 */
    if (lockstyle == LOK_UNIX)
	lockstyle = LOK_KERNEL;
#else
    if ((lockstyle = atoi (lkstyle)) < LOK_UNIX || lockstyle > LOK_MMDF)
	lockstyle = LOK_UNIX;
#endif	/* DOUBLE_LK */

#ifdef	MHMTS
    Syscpy = atoi (syscpy);
    (void) sscanf (tmpmode, "0%o", &Tmpmode);
#endif	MHMTS
    Everyone = atoi (everyone);
}

/*  */

#define	QUOTE	'\\'

static char *tailor_value (s)
register char   *s;
{
    register int    i,
                    r;
    register char  *bp;
    char    buffer[BUFSIZ];

    for (bp = buffer; *s; bp++, s++)
	if (*s != QUOTE)
	    *bp = *s;
	else
	    switch (*++s) {
#define	grot(y,z) case y: *bp = z; break;
		grot ('b', '\b');
		grot ('f', '\f');
		grot ('n', '\n');
		grot ('t', '\t');
#undef	grot

		case NULL: s--;
		case QUOTE: 
		    *bp = QUOTE;
		    break;

		default: 
		    if (!isdigit (*s)) {
			*bp++ = QUOTE;
			*bp = *s;
		    }
		    r = *s != '0' ? 10 : 8;
		    for (i = 0; isdigit (*s); s++)
			i = i * r + *s - '0';
		    s--;
		    *bp = toascii (i);
		    break;
	    }
    *bp = NULL;

    bp = malloc ((unsigned) (strlen (buffer) + 1));
    if (bp != NULL)
	(void) strcpy (bp, buffer);

    return bp;
}

/*  */

char   *LocalName () {
#ifdef	BSD41A
    char  *myname;
#endif	BSD41A
#if	defined(BSD42) || defined(SOCKETS)
    register struct hostent *hp;
#endif	BSD42 or SOCKETS
#if	defined(SYS5) && !defined(NETWORK)
    struct utsname name;
#endif	SYS5 and not NETWORK
    static char buffer[BUFSIZ] = "";

    if (buffer[0])
	return buffer;

    mts_init ("mts");
    if (*localname)
	return strcpy (buffer, localname);

#ifdef	locname
    (void) strcpy (buffer, locname);
#else	not locname
#ifdef	NETWORK
#ifdef	BSD41A
    myname = "myname";
    if (rhost (&myname) == -1)
	(void) gethostname (buffer, sizeof buffer);
    else {
	(void) strcpy (buffer, myname);
	free (myname);
    }
#endif	BSD41A
#if	defined(BSD42) || defined(SOCKETS)
    (void) gethostname (buffer, sizeof buffer);
#ifndef	BIND
    sethostent (1);
#endif
    if (hp = gethostbyname (buffer))
	(void) strcpy (buffer, hp -> h_name);
#endif	BSD42 or SOCKETS
#else	not NETWORK
#ifndef	SYS5
    (void) strcpy (buffer, SystemName ());
#else	SYS5
    (void) uname (&name);
    (void) strcpy (buffer, name.nodename);
#endif	SYS5
#endif	not NETWORK
#endif	not locname

    return buffer;
}

/*  */

char *SystemName () {
#if	defined(SYS5) && !defined(NETWORK)
    struct utsname name;
#endif	SYS5 and not NETWORK
    static char buffer[BUFSIZ] = "";

    if (buffer[0])
	return buffer;

    mts_init ("mts");
    if (*systemname)
	return strcpy (buffer, systemname);

#ifdef	sysname
    (void) strcpy (buffer, sysname);
#else	sysname
#if	!defined(SYS5) || defined(NETWORK)
    (void) gethostname (buffer, sizeof buffer);
#else	SYS5 and not NETWORK
#ifdef	SYS5
    (void) uname (&name);
    (void) strcpy (buffer, name.nodename);
#endif  SYS5
#endif  SYS5 and not NETWORK
#endif	sysname

    return buffer;
}

/*  */

char   *UucpChan () {
#ifdef	MF
    static char buffer[BUFSIZ] = "";
#endif	MF

#ifndef	MF
    return NULL;
#else	MF
    if (buffer[0])
	return buffer;

    mts_init ("mts");
    if (*UUCPchan)
	return strcpy (buffer, UUCPchan);

#ifdef	uucpchan
    (void) strcpy (buffer, uucpchan);
#else	uucpchan
    (void) strcpy (buffer, "uucp");
#endif	uucpchan
    return buffer;
#endif	MF
}

/*  */

#ifdef	ALTOS
gethostname (name, len)
register char   *name;
register int     len;
{
    register char  *cp;
    register FILE  *fp;

    if (fp = fopen ("/etc/systemid", "r")) {
	if (fgets (name, len, fp)) {
	    if (cp = index (name, '\n'))
		*cp = NULL;
	    (void) fclose (fp);
	    return OK;
	}
	(void) fclose (fp);
    }
    (void) strncpy (name, "altos", len);

    return OK;
}
#endif	ALTOS

/*  */

static char username[BUFSIZ] = "";
static char fullname[BUFSIZ] = "";

/* PJS: Indicates which username to return: login or ORname. */
#ifdef X400
char	need_x400_me = 0;

char *
get_x400_orname()
{
int		pid, len;
int		pipefds[2];
static char	ptr[BUFSIZ];

/* Open a pipe channel for retrieving the output of x400_whois. */
    if (pipe(pipefds) != 0) {
	fprintf(stderr,"Can't get pipes for x400_whois!\n");
	return ((char *)NULL);
    }

    pid = fork();
    if (pid == -1)
	return((char *)NULL);

    if (pid > 0) {	/* Parent process. */
	if (wait(NULL) != pid)
	    fprintf(stderr,"Wait returned without pid\n");
    }
    else {		/* Child process. */
	close(1);
	dup(pipefds[1]);
	close(2);
	dup(pipefds[1]);
	execl("/usr/bin/x400_whois", "x400_whois", (char *)NULL);
	execlp("x400_whois", "x400_whois", (char *)NULL);
	fprintf(stderr,"Can't find 'x400_whois'...\n");
	exit(1);
    }

    len = read(pipefds[0], ptr, BUFSIZ-1);
    if (len < 0)
	fprintf(stderr,"Failed to read anything from x400_whois...\n");
    else {
	ptr[len] = '\0';

/* If the output of x400_whois does not begin with a '/' then it is not an
 * ORaddress... Ignore any of the error output.
 */
	if (*ptr != '/')
	    *ptr = '\0';
	else
	    if (ptr[len-1] == '\n')
		ptr[len - 1] = '\0';
    }
    close(pipefds[0]);
    close(pipefds[1]);
    return(ptr);
}
#endif X400

char   *getusr () {
    register char  *cp,
                   *np;
    register struct passwd *pw;

#ifdef X400
    static char	   *x400_name = NULL;
    char           *get_x400_orname();

    if (need_x400_me == 1) {
	if (x400_name == (char *)NULL)
	    x400_name = get_x400_orname();
	if (x400_name && *x400_name)
	    return(x400_name);
    }
#endif X400

    if (username[0])
	return username;

    if ((pw = getpwuid (getuid ())) == NULL
	    || pw -> pw_name == NULL
	    || *pw -> pw_name == NULL) {
	(void) strcpy (username, "unknown");
	(void) sprintf (fullname, "The Unknown User-ID (%d)", getuid ());
	return username;
    }

    if (MMailids) {
	np = pw -> pw_gecos;
	for (cp = fullname; *np && *np != '<'; *cp++ = *np++)
	    continue;
	*cp = NULL;
	if (*np)
	    np++;
	for (cp = username; *np && *np != '>'; *cp++ = *np++)
	    continue;
	*cp = NULL;
    }
    if (MMailids == 0 || *np == NULL) {
	(void) strcpy (username, pw -> pw_name);
	fullname[0] = NULL;
    }
    if ((cp = getenv ("SIGNATURE")) && *cp)
	(void) strcpy (fullname, cp);
    if (index(fullname, '.')) {		/*  quote any .'s */
	  char tmp[BUFSIZ];
      sprintf (tmp, "\"%s\"", fullname);
      strcpy (fullname, tmp);
    }

    return username;
}


char   *getfullname () {
    if (username[0] == NULL)
	(void) getusr ();

    return fullname;
}

/*  */

#ifdef	SYS5
#ifndef	notdef			/* Supposedly this works, I prefer the
				   recursive solution... */

#include <fcntl.h>

int     dup2 (d1, d2)
register int    d1,
                d2;
{
    int     d;

    if (d1 == d2)
	return OK;

    (void) close (d2);
    if ((d = fcntl (d1, F_DUPFD, d2)) == NOTOK)
	return NOTOK;
    if (d == d2)
	return OK;

    errno = 0;
    return NOTOK;
}

#else	notdef
int     dup2 (d1, d2)
register int    d1,
                d2;
{
    if (d1 == d2)
	return OK;

    (void) close (d2);
    return dup2aux (d1, d2);
}


static int  dup2aux (d1, d2)
register int    d1,
                d2;
{
    int     d,
            i,
            eindex;

    if ((d = dup (d1)) == NOTOK)
	return NOTOK;
    if (d == d2)
	return OK;

    i = dup2aux (d1, d2);
    eindex = errno;
    (void) close (d);
    errno = eindex;
    return i;
}
#endif	notdef
#endif	SYS5
