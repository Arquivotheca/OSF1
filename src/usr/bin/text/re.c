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
static	char	*sccsid = "@(#)$RCSfile: re.c,v $ $Revision: 4.3 $ (DEC) $Date: 1991/09/21 17:11:46 $";
#endif lint
/*
 */
/*
 *
 *   File name: re.c
 *
 *   Source file description:
 *	Performs all of the regular expression handling for the extract
 *	programs. Reads in the PATTERN file and compiles each line using
 *	regexp(3C). Subsequently the function matchre() can be called
 *	to see if the text matches a MATCH argument but is not in the
 *	REJECT list.
 *
 *   Functions:
 *	loadre()
 *	    docompile()
 *	matchre()
 *
 *   Modification history:
 *   ~~~~~~~~~~~~~~~~~~~~~
 * 001	Tom Woodburn, 22 March 1991
 *	- Replaced strings with calls to catgets(3).
 *	- Changed copyright notice to DIGITAL_COPYRIGHT macro.
 *
 *	Andy Gadsby, 16-Dec-1986.
 *		Created.
 *
 */

#include <stdio.h>
#include "defs.h"
#include "extract_msg.h"

extern nl_catd catd;

extern char *progname;			/* name of caller		*/

#define  INIT 	   register char *sp = instring;
#define  GETC()	   (*sp++)
#define  PEEKC()   (*sp)
#define  UNGETC(c) (--sp)
#define  RETURN(c) return (c);
#undef   ERROR
#define  ERROR(v)  { fprintf(stderr,catgets(catd, MS_RE, M_RE_1, "%s: regexp error %d in %s\n"),progname,v,instring);exit(1);}

#include <regexp.h>

#define EXPSIZE		512		/* size of expression buffer	*/

struct  reglist {
	struct reglist *next;		/* forward link pointer		*/
	char  *mesg;			/* pointer to a useful message  */
	int    circf;			/* to help regexp out!!         */
	char   exp[1];			/* start of compiled expression	*/
};

static struct  reglist *matchtail  = (struct reglist *)NULL;
static struct  reglist *matchhead  = (struct reglist *)NULL;
static struct  reglist *rejecttail = (struct reglist *)NULL;
static struct  reglist *rejecthead = (struct reglist *)NULL;

char *refile = (char *)NULL;		/* can be modified via argv	*/
char rewstring[REWLEN] = "";		/* how we rewrite a message     */
int  rewlen = 0;			/* length of rewrite string	*/
char cathead[REWLEN] = "$quote \"\n";	/* pointer to header of msgcat	*/
char *init_str = (char *)0;		/* initial string, if any 	*/

char src1head[REWLEN];			/* pointer to header of 1. source*/
char src2head[REWLEN];			/* pointer to header of 2.. source*/

/*
 * loadre()
 * 	Read in the refile and setup all the regular expressions.
 */

static char *reserve[] =
	{"REWRITE", "MATCH", "REJECT", "ERROR", "CATHEAD",
	 "SRCHEAD1", "SRCHEAD2", (char *)NULL };

#define REWSTATE 	1
#define MATCHSTATE	2
#define REJECTSTATE	3
#define ERRORSTATE	4
#define CATHEADSTATE	5
#define SRC1HEADSTATE	6
#define SRC2HEADSTATE	7

loadre()
{	FILE *fp;
	FILE *sopen();
	char  lbuf[LINESIZE];
	char *malloc(), *index();
	char *cp;
	char **sp;
	int   state = 0;
	char *currmessage = (char *)NULL;
	int   len;
	int   str_len;

	fp = refile ? fopen(refile, "r") : sopen(PATTERN_FILE, "r");
	if (fp == (FILE *)NULL) {
		fprintf(stderr, catgets(catd, MS_RE, M_RE_2, "%s: cannot open pattern file\n"), progname);
		exit(1);
	}
top:
	while (fgets(lbuf, LINESIZE, fp)) {
					/* zap any newlines		*/
		if (cp = index(lbuf, '\n'))
			*cp = '\0';

					/* ignore comment lines		*/
		if (*lbuf == '\0' || *lbuf == '#')
			continue;

					/* look for reserved words	*/
		if (*lbuf == '$') {
		    if (cp = index(lbuf, ' '))
			*cp++ = '\0';
		    for (sp = reserve; *sp; sp++) 
			if (strcmp(*sp, &lbuf[1]) == 0) {
				state = sp - reserve + 1;
				if (cp) {
					if ((currmessage = malloc(strlen(cp) + 1)) == (char *)NULL) {
						fprintf(stderr, catgets(catd, MS_RE, M_RE_3, "%s: malloc failed\n"), progname);	
						exit(1);
					}
					strcpy(currmessage, cp);
				}
				str_len = 0;
				goto top;
			}
		}
					/* deal with escape character   */
		cp = (*lbuf == '\\') ? &lbuf[1] : lbuf;
#ifdef DEBUG
		printf("string :%s: state %d\n", cp, state);
#endif
		switch (state) {
		case 0:			/* no reserved words yet	*/
			fprintf(stderr, catgets(catd, MS_RE, M_RE_4, "%s: missing reserved word in %s\n"), progname, refile ? refile : PATTERN_FILE);
			exit(1);

		case REWSTATE:		/* have got a rewrite string	*/
			len = strlen(cp);
			if (rewlen + len + 1 >= REWLEN) {
			    fprintf(stderr, catgets(catd, MS_RE, M_RE_5, "%s: rewrite too long\n"), progname);
			    exit(1);
			}
			/*
			 * simply append the string to previous one
			 * user must insert line control using %N or %T
			 */
			strcpy(&rewstring[rewlen], cp);
			rewlen += len;
			break;

		case MATCHSTATE:	/* have got a string to match on */
			docompile(cp, (char *)0, &matchhead, &matchtail);
			break;

		case REJECTSTATE:	/* must reject this pattern	*/
			docompile(cp, (char *)0, &rejecthead, &rejecttail);
			break;

		case ERRORSTATE:	/* issue warning if encountered */
			docompile(cp, currmessage, &rejecthead, &rejecttail);
			break;
		case CATHEADSTATE:
			len = strlen(cp);
			if (str_len + len + 1 >= REWLEN) {
			    fprintf(stderr, catgets(catd, MS_RE, M_RE_6, "%s: CATHEAD too long\n"), progname);
			    exit(1);
			}
			strcpy(&cathead[str_len], cp);
			str_len += len;
			strcpy(&cathead[str_len++], "\n");
			break;
		case SRC1HEADSTATE:
			len = strlen(cp);
			if (str_len + len + 1 >= REWLEN) {
			    fprintf(stderr, catgets(catd, MS_RE, M_RE_7, "%s: SRCHEAD1 too long\n"),progname);
			    exit(1);
			}
			strcpy(&src1head[str_len], cp);
			str_len += len;
			strcpy(&src1head[str_len++], "\n");
			break;
		case SRC2HEADSTATE:
			len = strlen(cp);
			if (str_len + len + 1 >= REWLEN) {
			    fprintf(stderr, catgets(catd, MS_RE, M_RE_8, "%s: SRCHEAD2 too long\n"),progname);
			    exit(1);
			}
			strcpy(&src2head[str_len], cp);
			str_len += len;
			strcpy(&src2head[str_len++], "\n");
			break;
		}
	}
	fclose(fp);
	if (init_str == (char *)0)
	    init_str = &cathead[0];
}

/*
 * docompile()
 *	Compile the given expression and place on the appropriate list
 */

docompile(exp, mesg, headptr, tailptr)
char *exp;
char *mesg;
struct reglist **headptr;		/* pointer to head pointer	*/
struct reglist **tailptr;		/* pointer to tail pointer	*/
{	char expbuf[EXPSIZE], *end;
	int  len;
	struct reglist *tp;

	end = compile(exp, expbuf, expbuf + EXPSIZE, 0);
	len = end - expbuf;
	if ((tp = (struct reglist *)malloc(sizeof(struct reglist) + len)) == (struct reglist *)NULL) {
		fprintf(stderr, catgets(catd, MS_RE, M_RE_3, "%s: malloc failed\n"), progname);
		exit(1);
	}
					/* now link at end of list	*/
	tp->next = (struct reglist *)NULL;
	tp->circf = circf;
	tp->mesg = mesg;
	if (*headptr == (struct reglist *)NULL)
		*headptr = tp;
	if (*tailptr)
		(*tailptr)->next = tp;
	*tailptr = tp;
	
#ifdef ULTRIX
	bcopy(expbuf, tp->exp, len);
#else
	memcpy(tp->exp, expbuf, len);
#endif
}

/*
 * matchre()
 *	For each line passed, firstly try matching it against the match
 *	list. If a match is found then check against the reject list
 * 	if it appears on the reject list then fail the match. If a match
 *	is found then a pointer to the start of the matching string
 *	is returned, NULL is returned on failure. 
 *	NOTE: Care is taken to deal correctly with start of lines
 */

char *
matchre(buf, startofline, len, mesg)
char *buf;
int  startofline;
int  *len;
char **mesg;
{	struct reglist *rp;		/* to walk list with		*/
	char  *start, *end;		/* pointer to matching string	*/
	
	*mesg = (char *)NULL;		/* default return message	*/
loop:
	if (*buf == '\0')
		return (char *)0;

	for (rp = matchhead; rp; rp = rp->next) {
		circf = rp->circf;	/* set for regexp(3c)		*/
		if (circf && ! startofline)
			continue;
		if (step(buf, rp->exp)) {
			start = loc1;
			end   = loc2;
			break;
		}
	}

	if (rp == (struct reglist *)NULL)
		return (char *)NULL;	/* no match at all 		*/

					/* got a match so scan rejects  */
	for (rp = rejecthead; rp; rp = rp->next) {
		circf = rp->circf;
		if (circf && ! startofline)
			continue;
		if (step(buf, rp->exp) && start >= loc1 && end <= loc2) {
					/* string was within reject	*/
				if (rp->mesg) {
					/* found an error, return mesg  */
					*mesg = rp->mesg;
					break;
				}
				buf = loc2;
				goto loop;
			}
	}
	*len = end - start; 		/* got a valid match		*/
	return start;
}
