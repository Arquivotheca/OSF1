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
static char rcsid[] = "@(#)$RCSfile: m_addch.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1993/06/12 21:50:00 $";
#endif
/*
 * HISTORY
 */
/*
 */ 
/*** "m_addch.c  1.5  com/lib/curses,3.1,8943 10/16/89 23:25:01"; ***/
/*
 * COMPONENT_NAME: (LIBCURSE) Curses Library
 *
 * FUNCTIONS:   m_addch
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1988
 * All Rights Reserved
 * Licensed Material - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

# include	"cursesext.h"
# include	<signal.h>

/*
 * NAME:        m_addch
 *
 * EXECUTION ENVIRONMENT:
 *
 *      mini.c contains versions of curses routines for minicurses.
 *      They work just like their non-mini counterparts but draw on
 *      std_body rather than stdscr.  This cuts down on overhead but
 *      restricts what you are allowed to do - you can't get stuff back
 *      from the screen and you can't use multiple windows or things
 *      like insert/delete line (the logical ones that affect the screen).
 */

m_addch(c)
register chtype		c;
{
	register int		x, y;
	char *uctr;
	register char rawc = c & A_CHARTEXT;

#ifdef DEBUG
	if (outf) fprintf(outf,
		"m_addch: [(%d,%d)] ", stdscr->_cury, stdscr->_curx);
#endif
	x = stdscr->_curx;
	y = stdscr->_cury;
# ifdef DEBUG
	if (c == rawc)
		if(outf) fprintf(outf, "'%c'", rawc);
	else
		if(outf) fprintf(outf, "'%c' %o, raw %o", c, c, rawc);
# endif
	if (y >= stdscr->_maxy || x >= stdscr->_maxx || y < 0 || x < 0) {
		return ERR;
	}
	switch (rawc) {
	  case '\t':
	  {
		register int newx;

		for (newx = x + (8 - (x & 07)); x < newx; x++)
			if (m_addch(' ') == ERR)
				return ERR;
		return OK;
	  }
	  default:
		/*
		 * If its an ascii control char, do this, otherwise
		 * just put it to the screen
		 */
		if (rawc < ' ' || rawc == 0x7f) {
			uctr = unctrl(rawc);
			m_addch((chtype)uctr[0]|(c&A_ATTRIBUTES));
			m_addch((chtype)uctr[1]|(c&A_ATTRIBUTES));
			return OK;
		}
		if (stdscr->_attrs) {
#ifdef DEBUG
if (outf) fprintf(outf,
	"(attrs %o, %o=>%o)", stdscr->_attrs, c, c | stdscr->_attrs);
#endif
			c |= stdscr->_attrs;;
		}
		/* This line actually puts it out. */
		SP->virt_x++;
		*(SP->curptr++) = c;
		if (x >= stdscr->_maxx) {
			x = 0;
new_line:
			if (++y >= stdscr->_maxy)
				if (stdscr->_scroll) {
					_ll_refresh(stdscr->_use_idl);
					_scrdown();
					--y;
				}
				else {
# ifdef DEBUG
			int i;
			if(outf) fprintf(outf,
			    "ERR because (%d,%d) > (%d,%d)\n",
			    x, y, stdscr->_maxx, stdscr->_maxy);
			if(outf) fprintf(outf, "line: '");
			if(outf) for (i=0; i<stdscr->_maxy; i++)
				fprintf(outf, "%c", stdscr->_y[y-1][i]);
			if(outf) fprintf(outf, "'\n");
# endif
					return ERR;
				}
			_ll_move(y, x);
		}
# ifdef FULLDEBUG
		if(outf) fprintf(outf,
		"ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n",
		y, x, stdscr->_firstch[y], stdscr->_lastch[y]);
# endif
		break;
	  case '\n':
# ifdef DEBUG
		if (outf) fprintf(outf,
		"newline, y %d, lengths %d->%d, %d->%d, %d->%d\n",
		y, y, SP->std_body[y]->length, y+1,
		SP->std_body[y+1]->length, y+2, SP->std_body[y+2]->length);
# endif
		if (SP->std_body[y+1])
			SP->std_body[y+1]->length = x;
		x = 0;
		goto new_line;
	  case '\r':
		x = 0;
		break;
	  case '\b':
		if (--x < 0)
			x = 0;
		break;
	}
	stdscr->_curx = x;
	stdscr->_cury = y;
#ifdef DEBUG
	if (outf) fprintf(outf,
		" => (%d,%d)]\n", stdscr->_cury, stdscr->_curx);
#endif
	return OK;
}
