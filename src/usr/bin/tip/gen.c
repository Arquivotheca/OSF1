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
static char *rcsid = "@(#)$RCSfile: gen.c,v $ $Revision: 1.1.5.2 $ (DEC) $Date: 1993/09/07 15:37:24 $";
#endif
/*
 *      Generic dialer routines (mainly the termcap part here). The
 *      Actual dial, abort & disconnect routines are in aculib/generic.c
 *
 *                                              Larry Palmer
 *                                              lp@decvax
 */

#include <sys/time.h>

typeahead(masker, secns, usecns)
int masker;
int secns, usecns;
{
	int nbits=16, *writemask=0, *exceptmask=0, found;
	struct timeval timeqr, *timerp;
	int *rdmsk, mask;

	timeqr.tv_sec = secns;
	timeqr.tv_usec = usecns;

	mask = masker;
	rdmsk = &mask;
	timerp = &timeqr;
/*
 *      Select returns some positive value if things in typeahead
 *      otherwise 0.
 */
	found = select(nbits, rdmsk, writemask, exceptmask, timerp);
	return (found > 0 ? 1 : 0);
}


#define DEFDELAY 1
#define GENBUF 512
#define MAXSTR 128

extern  char *syncstr, *respstr, *dialstr, *respdial, *abostr, *onlstr,
		*disconstr, charup, *compstr, hangup, dtr, stupidi,
		*respsync, *dialterm, *replacestr, *speedstr, dtr_delay;
extern int syncdel, dialdel, cdelay, compdel, dspeed, cspeed;

char *acustrngs[] = { "ss", "sr", "di", "dr", "dt", "ab",
			"ds", "cs", "os", "rs", "co", "is", "xs", 0};

static char null = '\0'; /* used as a null string */

extern int FDD;

gen_setup(gbuf, FD)
char *gbuf;
int FD;
{
	char **p, *temp;
	char *rgetstr();
	extern char *tbuf;
	extern char lsleep, debugn;
	extern int dialack;

	/* get strings out */

	FDD = FD;
	tbuf = gbuf;

	for (p = acustrngs; *p != 0; p++) {
		temp = (char *) malloc(MAXSTR);
		if (!strcmp("ss", *p))
			syncstr = rgetstr(*p, &temp);
		else if (!strncmp("co", *p)) 
			speedstr = rgetstr(*p, &temp);
		else if (!strcmp("sr", *p))
			respsync = rgetstr(*p, &temp);
		else if (!strcmp("di", *p))
			dialstr = rgetstr(*p, &temp);
		else if (!strcmp("dr", *p))
			respdial = rgetstr(*p, &temp);
		else if (!strcmp("dt", *p))
			dialterm = rgetstr(*p, &temp);
		else if (!strcmp("ab", *p))
			abostr = rgetstr(*p, &temp);
		else if (!strcmp("ds", *p))
			disconstr = rgetstr(*p, &temp);
		else if (!strcmp("cs", *p))
			compstr = rgetstr(*p, &temp);
		else if (!strcmp("os", *p))
			onlstr = rgetstr(*p, &temp);
		else if (!strcmp("rs", *p))
			replacestr = rgetstr(*p, &temp);
		else
		       cfree(temp);
	}

	/* Check for null pointers
	 *
	 * If null, assign them equal to 'null'
	 */
	if (!syncstr)
		syncstr = &null;
	if (!speedstr)
		speedstr = &null;
	if (!respsync)
		respsync = &null;
	if (!dialstr)
		dialstr = &null;
	if (!respdial)
		respdial = &null;
	if (!dialterm)
		dialterm = &null;
	if (!abostr)
		abostr = &null;
	if (!disconstr)
		disconstr = &null;
	if (!compstr)
		compstr = &null;
	if (!onlstr)
		onlstr = &null;
	if (!replacestr)
		replacestr = &null;


	/*
	 * Get booleans
	 */

	charup = rgetflag("cr");
	hangup = rgetflag("hu");
	dtr = rgetflag("re");
	dtr_delay = rgetflag("rd");
	lsleep = rgetflag("ls");
	debugn = rgetflag("db");
	stupidi = rgetflag("si");

	/*
	 * dspeed - speed to initialize modem at.
 	 * cspeed - second speed, used for modem conversation.
 	 */
	dspeed = rgetnum("is"); 
	cspeed = rgetnum("xs"); 

	/*
	 * Get delay numbers
	 */

	syncdel = rgetnum("sd");
	dialdel = rgetnum("dd");
	compdel = rgetnum("cd");
	cdelay = rgetnum("fd");
	dialack = rgetnum("da");
	if (debugn)
		printf("gen.c: cspeed=%d, dspeed=%d\n",cspeed, dspeed);

}

#define ACULIST         "/etc/acucap"

#ifndef BUFSIZ
#define BUFSIZ          1024
#endif

agetent(bp, name)
	char *bp, *name;
{
	extern char *tbuf;
	char lbuf[BUFSIZ], *cp, *p;
	int rc1, rc2;
	cp = ACULIST;
	if (cp == (char *)0 || strcmp(cp, ACULIST) == 0) {
		cp = ACULIST;
		return (getent(bp, name, cp));
	} else {
		if ((rc1 = getent(bp, name, cp)) != 1)
			*bp = '\0';
		cp = ACULIST;
		rc2 = getent(lbuf, name, cp);
		if (rc1 != 1 && rc2 != 1)
			return (rc2);
		if (rc2 == 1) {
			p = lbuf;
			if (rc1 == 1)
				while (*p++ != ':')
					;
			if (strlen(bp) + strlen(p) > BUFSIZ) {
				write(2, "Acucap entry too long\n", 23);
				return (-1);
			}
			strcat(bp, p);
		}
		tbuf = bp;
		return (1);
	}
}
