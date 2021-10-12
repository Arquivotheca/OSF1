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
/* folder(s).c - report on folders */
#ifndef	lint
static char ident[] = "@(#)$RCSfile: folder.c,v $ $Revision: 4.2.5.2 $ (DEC) $Date: 1993/11/22 13:38:09 $ devrcs Exp Locker: devbld $";
#endif	lint

#include "../h/mh.h"
#include "../h/local.h"
#include <errno.h>
#include <stdio.h>

static		dodir(), addir(), addfold(), dother();
static int	pfold(), sfold(), compare();
/*  */

static struct swit switches[] = {
#define	ALLSW	0
    "all", 0,

#define	FASTSW	1
    "fast", 0,
#define	NFASTSW	2
    "nofast", 0,

#define	HDRSW	3
    "header", 0,
#define	NHDRSW	4
    "noheader", 0,

#define	PACKSW	5
    "pack", 0,
#define	NPACKSW	6
    "nopack", 0,
#define	VERBSW	7
    "verbose", 0,
#define	NVERBSW	8
    "noverbose", 0,

#define	RECURSW	9
    "recurse", 0,
#define	NRECRSW	10
    "norecurse", 0,

#define	TOTALSW	11
    "total", 0,
#define	NTOTLSW	12
    "nototal", 0,

#define	PRNTSW	13
    "print", 0,
#define	NPRNTSW	14
    "noprint", 0,
#define	LISTSW	15
    "list", 0,
#define	NLISTSW	16
    "nolist", 0,
#define	PUSHSW	17
    "push", 0,
#define	POPSW	18
    "pop", 0,

#define	HELPSW	19
    "help", 4,

    NULL, NULL
};

/*  */

extern int errno;

static int  fshort = 0;
static int  fpack = 0;
static int  fverb = 0;
static int  fheader = 0;
static int  frecurse = 0;
static int  ftotonly = 0;
static int  msgtot = 0;
static int  foldtot = 0;
static int  start = 0;
static int  foldp = 0;
/* PJS: Added for max number of folders. */
static int  nfolders = NFOLDERS;

static char *mhdir;
static char *stack = "Folder-Stack";
static char folder[BUFSIZ];
/* PJS: Changed to use 'nfolders'.
static char *folds[NFOLDERS + 1];
*/
static char **folds = NULL;

struct msgs *tfold ();

/*  */

/* ARGSUSED */

main (argc, argv)
char   *argv[];
{
    int     all = 0,
            printsw = 0,
            listsw = 0,
            pushsw = 0,
            popsw = 0;
    char   *cp,
           *dp,
           *msg = NULL,
           *argfolder = NULL,
          **ap,
          **argp,
            buf[100],
           *arguments[MAXARGS];
    struct stat st;

    invo_name = r1bindex (argv[0], '/');
    if (argv[0][strlen (argv[0]) - 1] == 's')
	all++;
    if ((cp = m_find (invo_name)) != NULL) {
	ap = brkstring (cp = getcpy (cp), " ", "\n");
	ap = copyip (ap, arguments);
    }
    else
	ap = arguments;
    (void) copyip (argv + 1, ap);
    argp = arguments;

/*  */

    while (cp = *argp++) {
	if (*cp == '-')
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    adios (NULLCP, "-%s unknown", cp);
		case HELPSW: 
		    (void) sprintf (buf, "%s [+folder] [msg] [switches]",
			    invo_name);
		    help (buf, switches);
		    done (1);

		case ALLSW: 
		    all++;
		    continue;

		case FASTSW: 
		    fshort++;
		    continue;
		case NFASTSW: 
		    fshort = 0;
		    continue;

		case HDRSW: 
		    fheader = -1;
		    continue;
		case NHDRSW: 
		    fheader++;
		    continue;

		case PACKSW: 
		    fpack++;
		    continue;
		case NPACKSW: 
		    fpack = 0;
		    continue;

		case VERBSW:
		    fverb++;
		    continue;
		case NVERBSW:
		    fverb = 0;
		    continue;

		case RECURSW: 
		    frecurse++;
		    continue;
		case NRECRSW: 
		    frecurse = 0;
		    continue;

		case TOTALSW: 
		    all++;
		    ftotonly++;
		    continue;
		case NTOTLSW: 
		    if (ftotonly)
			all = 0;
		    ftotonly = 0;
		    continue;

		case PRNTSW: 
		    printsw++;
		    continue;
		case NPRNTSW: 
		    printsw = 0;
		    continue;

		case LISTSW: 
		    listsw++;
		    continue;
		case NLISTSW: 
		    listsw = 0;
		    continue;

		case PUSHSW: 
		    pushsw++;
		    popsw = 0;
		    continue;
		case POPSW: 
		    popsw++;
		    pushsw = 0;
		    continue;
	    }
	if (*cp == '+' || *cp == '@')
	    if (argfolder)
		adios (NULLCP, "only one folder at a time!");
	    else
		argfolder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	else
	    if (msg)
		adios (NULLCP, "only one (current) message at a time!");
	    else
		msg = cp;
    }

/*  */

    if (!m_find ("path"))
	free (path ("./", TFOLDER));
    mhdir = concat (m_maildir (""), "/", NULLCP);

/* PJS: Dynamic allocation of folders. */
    folds = (char **)malloc((unsigned)(nfolders * sizeof(char *)));
    if (folds == (char **)NULL)
	adios (NULLCP, "no space for folders");

    if (pushsw == 0 && popsw == 0 && listsw == 0)
	printsw++;
    if (pushsw) {
	if (!argfolder) {
	    if ((cp = m_find (stack)) == NULL
		    || (ap = brkstring (dp = getcpy (cp), " ", "\n")) == NULL
		    || (argfolder = *ap++) == NULL)
		adios (NULLCP, "no other folder");
	    for (cp = getcpy (m_getfolder ()); *ap; ap++)
		cp = add (*ap, add (" ", cp));
	    free (dp);
	    m_replace (stack, cp);
	}
	else
	    m_replace (stack,
		    (cp = m_find (stack))
		    ? concat (m_getfolder (), " ", cp, NULLCP)
		    : getcpy (m_getfolder ()));
    }
    if (popsw) {
	if (argfolder)
	    adios (NULLCP, "sorry, no folders allowed with -pop");
	if ((cp = m_find (stack)) == NULL
		|| (ap = brkstring (dp = getcpy (cp), " ", "\n")) == NULL
		|| (argfolder = *ap++) == NULL)
	    adios (NULLCP, "folder stack empty");
	for (cp = NULL; *ap; ap++)
	    cp = cp ? add (*ap, add (" ", cp)) : getcpy (*ap);
	free (dp);
	if (cp)
	    m_replace (stack, cp);
	else
	    (void) m_delete (stack);
    }
    if (pushsw || popsw) {
	if (access (cp = m_maildir (argfolder), 0) == NOTOK)
	    adios (cp, "unable to find folder");
	m_replace (pfolder, argfolder);
	m_update ();
	argfolder = NULL;
    }
    if (pushsw || popsw || listsw) {
	printf ("%s", argfolder ? argfolder : m_getfolder ());
	if (cp = m_find (stack)) {
	    for (ap = brkstring (dp = getcpy (cp), " ", "\n"); *ap; ap++)
		printf (" %s", *ap);
	    free (dp);
	}
	printf ("\n");

	if (!printsw)
	    done (0);
    }

/*  */

    if (all) {
	fheader = 0;
	if (argfolder || msg) {
	    (void) strcpy (folder, argfolder ? argfolder : m_getfolder ());

/* PJS: Changed to use 'folder' rather than 'argfolder' in pfold
 */
	    if (argfolder && pfold (folder, msg)) {
		m_replace (pfolder, argfolder);
		m_update ();
	    }
	    if (!frecurse)	/* counter-intuitive */
		dodir (folder);
	}
	else {
	    dother ();

	    (void) strcpy (folder, (cp = m_find (pfolder)) ? cp : "");
	    dodir (".");
	}

	if (!fshort) {
	    if (!ftotonly)
		printf ("\n\t\t     ");
	    printf ("TOTAL= %*d message%c in %d folder%s.\n",
		    DMAXFOLDER, msgtot, msgtot != 1 ? 's' : ' ',
		    foldtot, foldtot != 1 ? "s" : "");
	}
    }
    else {
	fheader++;

	(void) strcpy (folder, argfolder ? argfolder : m_getfolder ());
	if (stat (strcpy (buf, m_maildir (folder)), &st) == NOTOK) {
	    if (errno != ENOENT)
		adios (buf, "error on folder");
	    cp = concat ("Create folder \"", buf, "\"? ", NULLCP);
	    if (!getanswer (cp))
		done (1);
	    free (cp);
	    if (!makedir (buf))
		adios (NULLCP, "unable to create folder %s", buf);
	}

	if (pfold (folder, msg) && argfolder)
	    m_replace (pfolder, argfolder);
    }

    m_update ();

    done (0);
}

/*  */

static	dodir (dir)
register char   *dir;
{
    int     i;
    int     os = start;
    int     of = foldp;
    char    buffer[BUFSIZ];

    start = foldp;
    if (chdir (mhdir) == NOTOK)
	adios (mhdir, "unable to change directory to");

    addir (strcpy (buffer, dir));
    for (i = start; i < foldp; i++)
	(void) pfold (folds[i], NULLCP), (void) fflush (stdout);

    start = os;
    foldp = of;
}

/*  */

static int  pfold (fold, msg)
register char   *fold,
		*msg;
{
    int	    hack,
	    others,
            retval = 1;
    register char *mailfile;
    register struct msgs   *mp = NULL;

    mailfile = m_maildir (fold);
    if (chdir (mailfile) == NOTOK) {
	if (errno != EACCES)
	    admonish (mailfile, "unable to change directory to");
	else
	    printf ("%22s%c unreadable\n",
		    fold, strcmp (folder, fold) ? ' ' : '+');
	return 0;
    }

    if (fshort) {
	printf ("%s\n", fold);

	if (!msg && !fpack) {
	    if (frecurse)
		dodir (fold);
	    return retval;
	}
    }

    if (!(mp = m_gmsg (fold))) {
	admonish (NULLCP, "unable to read folder %s", fold);
	return 0;
    }

    if (msg && !sfold (mp, msg))
	retval = 0;
    if (fpack)
	mp = tfold (mp);

    if (fshort)
	goto out;
    foldtot++;
    msgtot += mp -> nummsg;

    if (ftotonly)
	goto out;

    if (!fheader++)
	printf ("\t\tFolder  %*s# of messages (%*srange%*s); cur%*smsg  (other files)\n",
	    DMAXFOLDER, "", DMAXFOLDER - 2, "", DMAXFOLDER - 2, "",
	    DMAXFOLDER - 2, "");

    printf ("%22s%c ", fold, strcmp (folder, fold) ? ' ' : '+');

    hack = 0;
    if (mp -> hghmsg == 0)
	printf ("has   no messages%*s",
		mp -> msgflags & OTHERS ? DMAXFOLDER * 2 + 4 : 0, "");
    else {
	printf ("has %*d message%s (%*d-%*d)",
		DMAXFOLDER, mp -> nummsg, (mp -> nummsg == 1) ? " " : "s",
		DMAXFOLDER, mp -> lowmsg, DMAXFOLDER, mp -> hghmsg);
	if (mp -> curmsg >= mp -> lowmsg && mp -> curmsg <= mp -> hghmsg)
	    printf ("; cur=%*d", DMAXFOLDER, hack = mp -> curmsg);
    }

    if (mp -> msgflags & OTHERS)
	printf (";%*s (others)", hack ? 0 : DMAXFOLDER + 6, "");
    printf (".\n");

out: ;
    others = mp -> msgflags & OTHERS;
    m_fmsg (mp);

    if (frecurse && others)
	dodir (fold);

    return retval;
}

/*  */

static int  sfold (mp, msg)
register struct msgs   *mp;
char   *msg;
{
    if (!m_convert (mp, msg))
	return 0;

    if (mp -> numsel > 1) {
	admonish (NULLCP, "only one message at a time!");
	return 0;
    }
    m_setseq (mp);
    m_setcur (mp, mp -> lowsel);
    m_sync (mp);
    m_update ();

    return 1;
}


struct msgs *tfold (mp)
register struct msgs   *mp;
{
    register int    hole,
                    msgnum;
    char    newmsg[BUFSIZ],
            oldmsg[BUFSIZ];

    if (mp -> lowmsg > 1 && (mp = m_remsg (mp, 1, mp -> hghmsg)) == NULL)
	adios (NULLCP, "unable to allocate folder storage");

    for (msgnum = mp -> lowmsg, hole = 1; msgnum <= mp -> hghmsg; msgnum++)
	if (mp -> msgstats[msgnum] & EXISTS) {
	    if (msgnum != hole) {
		(void) strcpy (newmsg, m_name (hole));
		(void) strcpy (oldmsg, m_name (msgnum));
		if (fverb)
		    printf ("message %s becomes %s\n", oldmsg, newmsg);
		if (mhrename (oldmsg, newmsg) == NOTOK)
		    adios (newmsg, "unable to rename %s to", oldmsg);
		if (msgnum == mp -> curmsg)
		    m_setcur (mp, mp -> curmsg = hole);
		mp -> msgstats[hole] = mp -> msgstats[msgnum];
		mp -> msgflags |= SEQMOD;
		if (msgnum == mp -> lowsel)
		    mp -> lowsel = hole;
		if (msgnum == mp -> hghsel)
		    mp -> hghsel = hole;
	    }
	    hole++;
	}
    if (mp -> nummsg > 0) {
	mp -> lowmsg = 1;
	mp -> hghmsg = hole - 1;
    }
    m_sync (mp);
    m_update ();

    return mp;
}

/*  */

static	addir (name)
register char   *name;
{
    register char  *base,
                   *cp;
    struct stat st;
#ifdef SYS5DIR
    register struct dirent *dp;
#else  SYS5DIR
    register struct direct *dp;
#endif SYS5DIR
    register    DIR * dd;

    cp = name + strlen (name);
    *cp++ = '/';
    *cp = NULL;

    base = strcmp (name, "./") ? name : name + 2;/* hack */

    if ((dd = opendir (name)) == NULL) {
	admonish (name, "unable to read directory ");
	return;
    }
    while (dp = readdir (dd))
	if (strcmp (dp -> d_name, ".") && strcmp (dp -> d_name, "..")) {
#ifdef SYS5DIR
	    if (cp + dp -> d_reclen + 2 >= name + BUFSIZ)
#else  SYS5DIR
	    if (cp + strlen (dp -> d_name) + 2 >= name + BUFSIZ)
#endif SYS5DIR
		continue;
	    (void) strcpy (cp, dp -> d_name);
	    if (stat (name, &st) != NOTOK && (st.st_mode & S_IFMT) == S_IFDIR)
		addfold (base);
	}
    closedir (dd);

    *--cp = NULL;
}

/*  */

static	addfold (fold)
register char   *fold;
{
    register int    i,
                    j;
    register char  *cp;

/* PJS: Changed for dynamic storage of folders. */
    if (foldp > nfolders) {
	nfolders += NFOLDERS;
	folds = (char **)realloc(folds, (unsigned)(nfolders * sizeof(char *)));
	if (folds == (char **)NULL)
	    adios (NULLCP, "more than %d folders to report on", nfolders);
    }

    cp = getcpy (fold);
    for (i = start; i < foldp; i++)
	if (compare (cp, folds[i]) < 0) {
	    for (j = foldp - 1; j >= i; j--)
		folds[j + 1] = folds[j];
	    foldp++;
	    folds[i] = cp;
	    return;
	}

    folds[foldp++] = cp;
}

/*  */

static int  compare (s1, s2)
register char   *s1,
		*s2;
{
    register int    i;

    while (*s1 || *s2)
	if (i = *s1++ - *s2++)
	    return i;

    return 0;
}

/*  */

static	dother () {
    int	    atrlen;
    char    atrcur[BUFSIZ];
    register struct node   *np;

    (void) sprintf (atrcur, "atr-%s-", current);
    atrlen = strlen (atrcur);

    m_getdefs ();
    for (np = m_defs; np; np = np -> n_next)
	if (ssequal (atrcur, np -> n_name)
		&& !ssequal (mhdir, np -> n_name + atrlen))
	    (void) pfold (np -> n_name + atrlen, NULLCP);
}
