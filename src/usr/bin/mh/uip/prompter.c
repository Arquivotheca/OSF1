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
/* prompter.c - prompting editor front-end */

/**Revision History

Oct 29, 1991  Aju John DEC ULTRIX/OSF Engineering   AJ01

This program used to give an unligned access error and dump core
because the varible char *drft was incorrectly tested as a boolean 
flag. To preserve the logic, a boolean flag (int dr_flag) is used
that has a value 0 when drft is not set, 1 when it is set. This
flag is now tested instead of drft

**/

#ifndef	lint
static char ident[] = "@(#)$RCSfile: prompter.c,v $ $Revision: 4.2.3.2 $ (DEC) $Date: 1992/01/29 17:51:43 $ devrcs Exp Locker: devbld $";
#endif	/* lint */

#include "../h/mh.h"
#include <stdio.h>
#include <errno.h>
#ifndef	SYS5
#include <sgtty.h>
#else	SYS5
#include <sys/types.h>
#include <termio.h>
#ifndef	NOIOCTLH
#include <sys/ioctl.h>
#endif	NOIOCTLH
#endif	SYS5
#ifdef	BSD42
#include <setjmp.h>
#endif	BSD42
#include <signal.h>


#define	QUOTE	'\\'
#ifndef	CKILL
#define CKILL   '@'
#endif	not CKILL
#ifndef	CERASE
#define CERASE  '#'
#endif	not CERASE

/*  */

static struct swit switches[] = {
#define	ERASESW	0
    "erase chr", 0,
#define	KILLSW	1
    "kill chr", 0,

#define	PREPSW	2
    "prepend", 0,	
#define	NPREPSW	3
    "noprepend", 0,	

#define	RAPDSW	4
    "rapid", 0,	
#define	NRAPDSW	5
    "norapid", 0,	

#define	BODYSW	6
    "body", -4,
#define	NBODYSW	7
    "nobody", -6,

#define	DOTSW	8
    "doteof", 0,
#define	NDOTSW	9
    "nodoteof", 0,

#define	HELPSW	10
    "help", 4,		

#ifdef DDIF
#define DDIFSW 9
    "ddif", 4,
#endif DDIF

    NULL, NULL
};

/*  */

extern int  errno;


#ifndef	SYS5
#define	ERASE	sg.sg_erase
#define	KILL	sg.sg_kill
static struct sgttyb    sg;

#define	INTR	tc.t_intrc
static struct tchars    tc;
#else	SYS5
#define	ERASE	sg.c_cc[VERASE]
#define	KILL	sg.c_cc[VKILL]
#define	INTR	sg.c_cc[VINTR]
static struct termio    sg;
#endif	SYS5


static void	intrser ();

static int  wtuser = 0;
static int  sigint = 0;

#ifdef	BSD42
static jmp_buf sigenv;
#endif	BSD42

/*  */

/* ARGSUSED */

main (argc, argv)
int     argc;
char   *argv[];
{
    int     body = 1,
	    prepend = 1,
	    rapid = 0,
#ifdef DDIF
            ddif=0,		/* DDIF Support. */
#endif DDIF
	    doteof = 0,
	    fdi,
	    fdo,
            i,
	    isdf=0,		/* DDIF Support. */
            state;
    char   *cp,
           *drft = NULL,
           drtmp[BUFSIZ],	/* DDIF Support. */
           *erasep = NULL,
           *killp = NULL,
            name[NAMESZ],
            field[BUFSIZ],
            buffer[BUFSIZ],
            tmpfil[BUFSIZ],
          **ap,
           *arguments[MAXARGS],
          **argp;
    FILE *in, *out;
    int  in1,out1;		/* DDIF Support. */
    int  dr_flag;               /* draft indicator flag AJO1 */

    dr_flag = 0;                /* draft flag set to false. */
    invo_name = r1bindex (argv[0], '/');
    if ((cp = m_find (invo_name)) != NULL) {
	ap = brkstring (cp = getcpy (cp), " ", "\n");
	ap = copyip (ap, arguments);
    }
    else
	ap = arguments;
    (void) copyip (argv + 1, ap);
    argp = arguments;

/*  */

    while (cp = *argp++)
	if (*cp == '-')
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    adios (NULLCP, "-%s unknown", cp);
		case HELPSW: 
		    (void) sprintf (buffer, "%s [switches] file", invo_name);
		    help (buffer, switches);
		    done (1);

		case ERASESW: 
		    if (!(erasep = *argp++) || *erasep == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case KILLSW: 
		    if (!(killp = *argp++) || *killp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;

		case PREPSW: 
		    prepend++;
		    continue;
		case NPREPSW: 
		    prepend = 0;
		    continue;

		case RAPDSW: 
		    rapid++;
		    continue;
		case NRAPDSW: 
		    rapid = 0;
		    continue;

		case BODYSW: 
		    body++;
		    continue;
		case NBODYSW: 
		    body = 0;
		    continue;

		case DOTSW: 
		    doteof++;
		    continue;
		case NDOTSW: 
		    doteof = 0;
		    continue;
	    }
	else
	  if (!dr_flag)  /* AJ01 */
	    {
	      drft = cp;
	      if (cp)
		dr_flag = 1; /* flag is set true if drft changes */
	    }
/*  */

#ifdef DDIF
    if(!dr_flag && ddif){
	if((in1 = open(libpath(components),0)) == NOTOK)
		adios(NULLCP,"unable to open default component file %s ",libpath(components));

	(void)strcpy(drtmp,m_draft(NULLCP,NULLCP,0,&isdf));
	if((out1 = creat(drtmp,m_gmprot())) == NOTOK)
		adios(NULLCP,"unable to createfile %s ",drtmp);

	cpydata(in1,out1,components,drtmp);
	(void)close(in1);
	(void)close(out1);
	drft = drtmp;
	if (drtmp)
	  dr_flag = 1; /* draft flag set true. AJ01 */
    }
#endif DDIF
/* down to here. */
	
/*  */

    if (!dr_flag) /* AJ01. It used to crash at this point */
	adios (NULLCP, "usage: %s [switches] file", invo_name);
    if ((in = fopen (drft, "r")) == NULL)
	adios (drft, "unable to open");

    (void) strcpy (tmpfil, m_tmpfil (invo_name));
    if ((out = fopen (tmpfil, "w")) == NULL)
	adios (tmpfil, "unable to create");
    (void) chmod (tmpfil, 0600);

    if (killp || erasep) {
#ifndef	SYS5
	int    serase,
	       skill;
#else	SYS5
	char   serase,
	       skill;
#endif	SYS5

#ifndef	SYS5
	(void) ioctl (0, TIOCGETP, (char *) &sg);
	(void) ioctl (0, TIOCGETC, (char *) &tc);
#else	SYS5
	(void) ioctl(0, TCGETA, &sg);
#endif	SYS5
	skill = KILL;
	serase = ERASE;
	KILL = killp ? chrcnv (killp) : skill;
	ERASE = erasep ? chrcnv (erasep) : serase;
#ifndef	SYS5
	(void) ioctl (0, TIOCSETN, (char *) &sg);
#else	SYS5
	(void) ioctl(0, TCSETAW, &sg);
#endif	SYS5

	chrdsp ("erase", ERASE);
	chrdsp (", kill", KILL);
	chrdsp (", intr", INTR);
	(void) putchar ('\n');
	(void) fflush (stdout);

	KILL = skill;
	ERASE = serase;
    }

/*  */

    sigint = 0;
    setsig (SIGINT, intrser);

    for (state = FLD;;) {
	switch (state = m_getfld (state, name, field, sizeof field, in)) {
	    case FLD: 
	    case FLDEOF: 
	    case FLDPLUS: 
		for (cp = field; *cp; cp++)
		    if (*cp != ' ' && *cp != '\t')
			break;
		if (*cp++ != '\n' || *cp != NULL) {
		    printf ("%s:%s", name, field);
		    fprintf (out, "%s:%s", name, field);
		    while (state == FLDPLUS) {
			state =
			    m_getfld (state, name, field, sizeof field, in);
			printf ("%s", field);
			fprintf (out, "%s", field);
		    }
		}
		else {
		    printf ("%s: ", name);
		    (void) fflush (stdout);
		    i = getln (field, sizeof field);
		    if (i == -1) {
abort: ;
			if (killp || erasep)
#ifndef	SYS5
			    (void) ioctl (0, TIOCSETN, (char *) &sg);
#else	SYS5
			    (void) ioctl (0, TCSETA, &sg);
#endif	SYS5
			(void) unlink (tmpfil);
			done (1);
		    }
		    if (i != 0 || (field[0] != '\n' && field[0] != NULL)) {
			fprintf (out, "%s:", name);
			do {
			    if (field[0] != ' ' && field[0] != '\t')
				(void) putc (' ', out);
			    fprintf (out, "%s", field);
			} while (i == 1
				    && (i = getln (field, sizeof field)) >= 0);
			if (i == -1)
			    goto abort;
		    }
		}
		if (state == FLDEOF) {/* moby hack */
		    fprintf (out, "--------\n");
		    printf ("--------\n");
		    if (!body)
			break;
		    goto no_body;
		}
		continue;

	    case BODY: 
	    case BODYEOF:
	    case FILEEOF: 
	        if (!body)
	            break;
#ifdef DDIF
                if(ddif){
                        docapsarbit(out,tmpfil);
                        goto outofloop;
                }
#endif DDIF
		fprintf (out, "--------\n");
		if (field[0] == NULL || !prepend)
		    printf ("--------\n");
		if (field[0]) {
		    if (prepend && body) {
			printf ("\n--------Enter initial text\n\n");
			(void) fflush (stdout);
			for (;;) {
			    (void) getln (buffer, sizeof buffer);
			    if (doteof && buffer[0] == '.' && buffer[1] == '\n')
				break;
			    if (buffer[0] == NULL)
				break;
			    fprintf (out, "%s", buffer);
			}
		    }

		    do {
			fprintf (out, "%s", field);
			if (!rapid && !sigint)
			    printf ("%s", field);
		    } while (state == BODY &&
			    (state = m_getfld (state, name, field, sizeof field, in)));
		    if (prepend || !body)
			break;
		    else
			printf ("\n--------Enter additional text\n\n");
		}
no_body: ;
		(void) fflush (stdout);
		for (;;) {
		    (void) getln (field, sizeof field);
		    if (doteof && field[0] == '.' && field[1] == '\n')
			break;
		    if (field[0] == NULL)
			break;
 		    fprintf (out, "%s", field);
		}
		break;

	    default: 
		adios (NULLCP, "skeleton is poorly formatted");
	}
	break;
    }

    if (body)
	printf ("--------\n");
outofloop:      ;		/* DDIF Support. */
    (void) fflush (stdout);

    (void) fclose (in);
    (void) fclose (out);

    (void) signal (SIGINT, SIG_IGN);

/*  */

    if (killp || erasep)
#ifndef	SYS5
	(void) ioctl (0, TIOCSETN, (char *) &sg);
#else	SYS5
	(void) ioctl (0, TCSETAW, &sg);
#endif	SYS5

    if ((fdi = open (tmpfil, 0)) == NOTOK)
	adios (tmpfil, "unable to re-open");
    if ((fdo = creat (drft, m_gmprot ())) == NOTOK)
	adios (drft, "unable to write");
    cpydata (fdi, fdo, tmpfil, drft);
    (void) close (fdi);
    (void) close (fdo);
    (void) unlink (tmpfil);

    m_update ();

    done (0);
}

/*  */

getln (buffer, n)
char   *buffer;
int     n;
{
    int     c;
    char   *cp;

    cp = buffer;
    *cp = NULL;

#ifndef	BSD42
    wtuser = 1;
#else	BSD42
    switch (setjmp (sigenv)) {
	case OK: 
	    wtuser = 1;
	    break;

	case DONE: 
	    wtuser = 0;
	    return 0;

	default: 
	    wtuser = 0;
	    return NOTOK;
    }
#endif	BSD42

    for (;;)
	switch (c = getchar ()) {
	    case EOF: 
#ifndef BSD42
		wtuser = 0;
		return (errno != EINTR ? 0 : NOTOK);
#else	BSD42
		clearerr (stdin);
		longjmp (sigenv, DONE);
#endif	BSD42

	    case '\n': 
		if (cp[-1] == QUOTE) {
		    cp[-1] = c;
		    wtuser = 0;
		    return 1;
		}
		*cp++ = c;
		*cp = NULL;
		wtuser = 0;
		return 0;

	    default: 
		if (cp < buffer + n)
		    *cp++ = c;
		*cp = NULL;
	}
}

/*  */

/* ARGSUSED */

static	void intrser (i)
int    i;
{
#ifndef	BSD42
    (void) signal (SIGINT, intrser);
    if (!wtuser)
	sigint++;
#else	BSD42
    if (wtuser)
	longjmp (sigenv, NOTOK);
    sigint++;
#endif	BSD42
}


chrcnv (cp)
register char   *cp;
{
    return (*cp != QUOTE ? *cp : m_atoi (++cp));
}


chrdsp (s, c)
char   *s,
	c;
{
    printf ("%s ", s);
    if (c < ' ' || c == 0177)
	printf ("^%c", c ^ 0100);
    else
	printf ("%c", c);
}

#ifdef DDIF

#undef NOTOK
#undef OK
#undef DONE
#undef NULLCP
#undef NULLVP

#include <capsar.h>
#include <sys/file.h>

docapsarbit(out,file)
FILE	*out;
char	*file;
{
	int	i;
	int	fieldtag[80];
	int	field[BUFSIZ];
	MM	*m;
	char	*cp;

	printf("DDIF/DOTS file : ");
	(void) fflush (stdout);
	scanf("%s",field);
	

	if(access(field,F_OK) == NOTOK)
		adios(NULLCP," Sorry %s does not exist ",field);

	strcpy(fieldtag,field);

	m = capsar_create(field,fieldtag);

	if(m!=NULL && m->dataptr != NULL){
		fprintf(out,"\n\n--------%s\n",m->start);
		if(fwrite(m->dataptr,1,m->size,out) == NULL)
			adios(NULLCP,"write error on DDIF/DOTS data",NULL);
		
		fprintf(out,"\n\n--------%s\n",m->stop);
		return(OK);
	}
	return(NOTOK);
}
#endif DDIF
