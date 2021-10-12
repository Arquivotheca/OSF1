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
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: ex_get.c,v $ $Revision: 4.2.5.2 $ (DEC) $Date: 1993/06/10 23:11:58 $";
#endif
/*
 * HISTORY
 */
/*
 * OSF/1 1.2
 */
/*
 * COMPONENT_NAME: (CMDEDIT) ex_get.c
 *
 * FUNCTION: checkjunk, getach, getcd, ex_getchar, gettty, ignchar, peekcd,
 * peekchar, setin, smunch
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * 1.9  com/cmd/edit/vi/ex_get.c, , bos320, 9134320 8/11/91 12:28:59
 * 
 */
/* Copyright (c) 1981 Regents of the University of California */

#include "ex.h"
#include "ex_tty.h"
#include <wchar.h>

static void checkjunk(wchar_t);
static int getach(void);

/*
 * Input routines for command mode.
 * Since we translate the end of reads into the implied ^D's
 * we have different flavors of routines which do/don't return such.
 */
int	lastc = '\n';			/* must fit EOF or wchar_t */

void
ignchar(void)
{
	ignore(ex_getchar());
}

int
ex_getchar(void)
{
	register int c;

	do
		c = getcd();
	while (!globp && c == Ctrl('D'));
	return (c);
}

int
getcd(void)
{
	register int c;
	extern short slevel;

again:
	c = getach();
	if (c == EOF)
		return (c);
	if (!inopen && slevel==0)
		if (!globp && c == Ctrl('D'))
			setlastchar('\n');
		else if (junk(c)) {
			checkjunk((wchar_t)c);
			goto again;
		}
	return (c);
}

int
peekchar(void)
{

	if (peekc == 0)
		peekc = ex_getchar();
	return (peekc);
}

int
peekcd(void)
{
	if (peekc == 0)
		peekc = getcd();
	return (peekc);
}


static int getach(void)
{
	register int c;
	char in_buf[512];
	static wchar_t in_line[512];
	int	count;
	wchar_t	widc;

	c = peekc;
	if (c != 0) {
		peekc = 0;
		return (c);
	}
	if (globp) {
		if (*globp)
			return ((int)*globp++);
		globp = 0;
		return (lastc = WEOF);
	}
top:
	if (input) {
		if (c = (int)*input++) {
			return (lastc = c);
		}
		input = 0;
	}
	flush();
	if (intty) {
		c = read(0, in_buf, sizeof in_buf - 4);
		if (c < 0)
			return (lastc = WEOF);
		if (c == 0 || in_buf[c-1] != '\n')
			in_buf[c++] = Ctrl('D');
		if (in_buf[c-1] == '\n')
			noteinp();
		in_buf[c] = 0;
		if (mbstowcs(in_line,in_buf,WCSIZE(in_line)) == -1)
			error(MSGSTR(M_650, "Invalid multibyte character string encountered, conversion failed."), DUMMY_INT);
		input = in_line;
		goto top;
	}

        /* this section of code reads in a single character but from a file
        descriptor not a FILE pointer. The code loops until a complete char
        has been read in signified by a successful conversion to process
        code. If the read fails or if a valid char is not read then the
        characters read in are invalid and are dumped. In this case the
        function will return EOF
        */

        lastc = WEOF;
        for( count = 0; count < MB_CUR_MAX; count++){
                if(read(0, &in_buf[count], 1) != 1){
                        break;
                }
                if((mbtowc(&widc, in_buf, count +1)) == count +1){
                        lastc = widc;
                        break;
                }
        }
        return ((int)lastc);
}

/*
 * Input routine for insert/append/change in command mode.
 * Most work here is in handling autoindent.
 */
static	int	lastin;

int
gettty(void)
{
	register int c = 0;
	register wchar_t *cp = genbuf;
	wchar_t hadup = 0;
	int offset = (Pline == numbline ? 8 : 0);
	int ch;

	if (intty && !inglobal) {
		if (offset) {
			holdcm = 1;
			ex_printf("  %4d  ", lineDOT() + 1);
			flush();
			holdcm = 0;
		}
		if (value(AUTOINDENT) ^ aiflag) {
			holdcm = 1;
			if (value(LISP))
				lastin = lindent(dot + 1);
			gotab(lastin + offset);
			while ((c = getcd()) == Ctrl('D')) {
				if (lastin == 0 && isatty(0) == -1) {
					holdcm = 0;
					return (EOF);
				}
				lastin = backtab(lastin);
				gotab(lastin + offset);
			}
			switch (c) {

			case '^':
			case '0':
				ch = getcd();
				if (ch == Ctrl('D')) {
					if (c == '0')
						lastin = 0;
					if (!over_strike) {
						ex_putchar(QUOTE_BSP);
					}
					gotab(offset);
					hadup = 1;
					c = ex_getchar();
				} else
					ungetchar(ch);
				break;

			case '.':
				if (peekchar() == '\n') {
					ignchar();
					noteinp();
					holdcm = 0;
					return (EOF);
				}
				break;

			case '\n':
				hadup = 1;
				break;
			}
		}
		flush();
		holdcm = 0;
	}
	if (c == 0)
		c = ex_getchar();
	while (c != EOF && c != '\n') {
		if (cp > &genbuf[LBSIZE])
			error(MSGSTR(M_067, "An input line cannot be longer than %d characters."), LBSIZE);
		*cp++ = c;
		c = ex_getchar();
	}
	if (c == EOF) {
		if (inglobal)
			ungetchar(EOF);
		return (EOF);
	}
	*cp = 0;
	cp = linebuf;
	if ((value(AUTOINDENT) ^ aiflag) && hadup == 0 && intty && !inglobal) {
		lastin = c = smunch(lastin, genbuf);
		for (c = lastin; c >= value(TABSTOP); c -= value(TABSTOP))
			*cp++ = '\t';
		for (; c > 0; c--)
			*cp++ = ' ';
	}
	CP(cp, genbuf);
	if (linebuf[0] == '.' && linebuf[1] == 0)
		return (EOF);
	return (0);
}

/*
 * Crunch the indent.
 * Hard thing here is that in command mode some of the indent
 * is only implicit, so we must seed the column counter.
 * This should really be done differently so as to use the whitecnt routine
 * and also to modify indenting for LISP.
 */
int
smunch(register int col, wchar_t *ocp)
{
	register wchar_t *cp;

	cp = ocp;
	for (;;)
		switch (*cp++) {

		case ' ':
			col++;
			continue;

		case '\t':
			col += value(TABSTOP) - (col % value(TABSTOP));
			continue;

		default:
			cp--;
			CP(ocp, cp);
			return (col);
		}
}

static void checkjunk(wchar_t c)
{
	static	short junkbs;

	if (!junkbs && c == '\b') {
		register char *txt = MSGSTR(M_068, "^H discarded\n");
		write(2, txt, strlen(txt));
		junkbs = TRUE;
	}
}

line *
setin(line *addr)
{

	if (addr == zero)
		lastin = 0;
	else
		getline(*addr), lastin = smunch(0, linebuf);
}
