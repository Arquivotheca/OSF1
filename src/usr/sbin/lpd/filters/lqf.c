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
static char	*sccsid = "@(#)$RCSfile: lqf.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 06:09:09 $";
#endif 
/*
 */
/*
 * OSF/1 Release 1.0
 */
/*
 * This work is derived from work that is 
 * Copyright (c) 1983 Regents of the University of California.
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
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

/*
 *	General output filter for "letter quality" class of printers.
 *	Specifically supports the LQP02, but should work for any DEC
 * 	impact letter printers.  The filter allows the escape characters
 *	and control sequences to be transmitted to the printers.
 *
 * 	The filter reads the output of nroff and converts lines
 *	with ^H's to overwritten lines.  Thus this works like 'ul'
 *	but is much better: it can handle more than 2 overwrites
 *	and it is written with some style.
 *	modified by kls to use register references instead of arrays
 *	to try to gain a little speed.
 *
 *	Note: the LA210 does not have an escape sequence to reset all the
 *	power up default values.  Therefore, if any attributes are changed
 *	by transmission of escape/control sequences, these changes will 
 *	remain in effect unless specifically cancelled by other escape/
 *	control sequences, or until the printer is powered off and on.
 *
 */

#include <stdio.h>
#include <signal.h>

/*************************************************************************/
/* added for escape sequence pass through 				 */

#define ESC	  '\033'	/* escape sequence introducer */
#define BSLH	  '\134'	/* back slash */
#define UCP	  '\120'	/* upper case P */
#define escend(x) ((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))
int   	escflg =  0;		/* escape sequence flag, 1 = in progress */
int	lstchr;		
/*************************************************************************/

#define MAXWIDTH  132
#define MAXREP    10

char	buf[MAXREP][MAXWIDTH];
int	maxcol[MAXREP] = {-1};
int	lineno;
int	width = 80;	/* default line length */
int	length = 66;	/* page length */
int	indent;		/* indentation length */
int	npages = 1;
int	literal;	/* print control characters */
char	*name;		/* user's login name */
char	*host;		/* user's machine name */
char	*acctfile;	/* accounting information file */

main(argc, argv) 
	int argc;
	char *argv[];
{
	register FILE *p = stdin, *o = stdout;
	register int i, col;
	register char *cp;
	int done, linedone, maxrep;
	char ch, *limit;


	while (--argc) {
		if (*(cp = *++argv) == '-') {
			switch (cp[1]) {
			case 'n':		/* collect login name */
				argc--;
				name = *++argv;
				break;

			case 'h':		/* collect host name */
				argc--;
				host = *++argv;
				break;

			case 'w':		/* collect page width */
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH)
					width = i;
				break;

			case 'l':		/* collect page length */	
				length = atoi(&cp[2]);
				break;

			case 'i':		/* collect indent */
				indent = atoi(&cp[2]);
				break;

			case 'c':		/* print control chars */
				literal++;
				break;
			}
		} else
			acctfile = cp;
	}

	for (cp = buf[0], limit = buf[MAXREP]; cp < limit; *cp++ = ' ');
	done = 0;
	
	escflg = 0;		/* is escape/control sequence in progress? */
	while (!done) {
		col = indent;
		maxrep = -1;
		linedone = 0;
		while (!linedone) {
			ch = getc(p);
			if (((escflg==0)&&(ch==ESC))||escflg)
				eschdl(o,ch);	/* deal with escape character */
			else 
				switch (ch) {
				case EOF:
					linedone = done = 1;
					ch = '\n';
					break;
	
				case '\f':		/* new page on form feed */
					lineno = length;
				case '\n':		/* new line */
					if (maxrep < 0)
						maxrep = 0;
					linedone = 1;
					break;
	
				case '\b':		/* backspace */
					if (--col < indent)
						col = indent;
					break;
	
				case '\r':		/* carriage return */
					col = indent;
					break;
	
				case '\t':		/* tab */
					col = ((col - indent) | 07) + indent + 1;
					break;
	
				case '\031':		/* end media */
					/*
				 	* lpd needs to use a different filter to
				 	* print data so stop what we are doing and
				 	* wait for lpd to restart us.
				 	*/
					if ((ch = getchar()) == '\1') {
						fflush(stdout);
						kill(getpid(), SIGSTOP);
						break;
					} else {
						ungetc(ch, stdin);
						ch = '\031';
					}
	
				default:		/* everything else */
					if (col >= width || !literal && ch < ' ') {
						col++;
						break;
					}
					cp = &buf[0][col];
					for (i = 0; i < MAXREP; i++) {
						if (i > maxrep)
							maxrep = i;
						if (*cp == ' ') {
							*cp = ch;
							if (col > maxcol[i])
								maxcol[i] = col;
							break;
						}
						cp += MAXWIDTH;
					}
					col++;
					break;
				}
			}

		/* print out lines */
		for (i = 0; i <= maxrep; i++) {
			for (cp = buf[i], limit = cp+maxcol[i]; cp <= limit;) {
				putc(*cp, o);
				*cp++ = ' ';
			}
			if (i < maxrep)
				putc('\r', o);
			else
				putc(ch, o);
			if (++lineno >= length) {
				npages++;
				lineno = 0;
				if (length != 66)	/* 66 lines fill page just right so no FF */
					putchar('\f'); 
			}
			maxcol[i] = -1;
		}
	}
	if (lineno) {		/* be sure to end on a page boundary */
		putchar('\f');
		npages++;
	}
	fprintf(o,"\033\143"); 	/* reset printer defaults (no effect on la210) */ 
	fflush(o);		/* make sure the reset goes out */
	sleep(10);		/* reset eats lines so wait */
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    freopen(acctfile, "a", stdout) != NULL) {
		printf("%7.2f\t%s:%s\n", (float)npages, host, name);
	}
	exit(0);
}
/****************************************************************/
/*								*/
/*	eschdl - escape sequence handler			*/
/*								*/
/*      This routine intercepts escape sequences for the purpose*/
/*	of pass through.					*/
/*								*/
/****************************************************************/
eschdl(o,c)
int c;
FILE  *o;
{
if(escflg==0)
	{		/* set escflg=1 => ready to receive 2nd seqchar*/
	escflg=1;
	}
else	switch(escflg)
		{
		case 1:		/* second character of escseq 		*/
			switch(c)
				{
  				case UCP:
					escflg=2; /*ctrl str pass thru mode=8 */
					lstchr=c;
					putc(ESC,o);
					putc(c,o);
					break;
				default:
					escflg=3;  /* set seq pass thru mode*/
					putc(ESC,o);
					putc(c,o);
					break;
				}
			break;
		case 2:		/* ctrl string pass through mode       	*/
			if((lstchr==ESC) && (c==BSLH))
				{
				escflg=0;
				lstchr=0;
				}
			else lstchr=c;	/* save it for next pass */
			putc(c,o);
			break;
		case 3:
			if(escend(c))
				escflg=0;/* turn off esc handler if at end  */
			putc(c,o);
			break;
		}
return(0);
}
