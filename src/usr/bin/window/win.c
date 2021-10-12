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
static char	*sccsid = "@(#)$RCSfile: win.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:12:39 $";
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
#if !defined(lint) && !defined(_NOIDENT)

#endif
/*
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
 * 
 * 	win.c	3.20 (Berkeley) 6/29/88
 */


#include "defs.h"
#include "char.h"
#ifdef POSIX_TTY
#include <sys/ioctl.h>
#endif

/*
 * Higher level routines for dealing with windows.
 *
 * There are two types of windows: user window, and information window.
 * User windows are the ones with a pty and shell.  Information windows
 * are for displaying error messages, and other information.
 *
 * The windows are doubly linked in overlapping order and divided into
 * two groups: foreground and normal.  Information
 * windows are always foreground.  User windows can be either.
 * Addwin() adds a window to the list at the top of one of the two groups.
 * Deletewin() deletes a window.  Front() moves a window to the front
 * of its group.  Wwopen(), wwadd(), and wwdelete() should never be called
 * directly.
 */

/*
 * Open a user window.
 */
struct ww *
openwin(id, row, col, nrow, ncol, nline, label, haspty, hasframe, shf, sh)
char *label;
char haspty, hasframe;
char *shf, **sh;
{
	register struct ww *w;

	if (id < 0 && (id = findid()) < 0)
		return 0;
	if (row + nrow <= 0 || row > wwnrow - 1
	    || col + ncol <= 0 || col > wwncol - 1) {
		error(MSGSTR(POSERR, "Illegal window position."));
		return 0;
	}
	w = wwopen(haspty ? WWO_PTY : WWO_SOCKET, nrow, ncol, row, col, nline);
	if (w == 0) {
		error(MSGSTR(CANTOPENWIN, "Can't open window: %s."), wwerror());
		return 0;
	}
	w->ww_id = id;
	window[id] = w;
	w->ww_hasframe = hasframe;
	w->ww_alt = w->ww_w;
	if (label != 0 && setlabel(w, label) < 0)
		error(MSGSTR(NOMEM, "No memory for label."));
	wwcursor(w, 1);
	/*
	 * We have to do this little maneuver to make sure
	 * addwin() puts w at the top, so we don't waste an
	 * insert and delete operation.
	 */
	setselwin((struct ww *)0);
	addwin(w, 0);
	setselwin(w);
	wwupdate();
	wwflush();
	if (wwspawn(w, shf, sh) < 0) {
		error(MSGSTR(CANTEXE, "Can't execute %s: %s."), shf, wwerror());
		closewin(w);
		return 0;
	}
	return w;
}

findid()
{
	register i;

	for (i = 0; i < NWINDOW && window[i] != 0; i++)
		;
	if (i >= NWINDOW) {
		error(MSGSTR(TOOMANYWIN, "Too many windows."));
		return -1;
	}
	return i;
}

struct ww *
findselwin()
{
	register struct ww *w, *s = 0;
	register i;

	for (i = 0; i < NWINDOW; i++)
		if ((w = window[i]) != 0 && w != selwin &&
		    (s == 0 ||
		     !isfg(w) && (w->ww_order < s->ww_order || isfg(s))))
			s = w;
	return s;
}

/*
 * Close a user window.  Close all if w == 0.
 */
closewin(w)
register struct ww *w;
{
	char didit = 0;
	register i;

	if (w != 0) {
		closewin1(w);
		didit++;
	} else
		for (i = 0; i < NWINDOW; i++) {
			if ((w = window[i]) == 0)
				continue;
			closewin1(w);
			didit++;
		}
	if (didit) {
		if (selwin == 0)
			if (lastselwin != 0) {
				setselwin(lastselwin);
				lastselwin = 0;
			} else if (w = findselwin())
				setselwin(w);
		if (lastselwin == 0 && selwin)
			if (w = findselwin())
				lastselwin = w;
		reframe();
	}
}

/*
 * Open an information (display) window.
 */
struct ww *
openiwin(nrow, label)
char *label;
{
	register struct ww *w;

	if ((w = wwopen(0, nrow, wwncol, 2, 0, 0)) == 0)
		return 0;
	w->ww_mapnl = 1;
	w->ww_hasframe = 1;
	w->ww_nointr = 1;
	w->ww_noupdate = 1;
	w->ww_unctrl = 1;
	w->ww_id = -1;
	w->ww_center = 1;
	(void) setlabel(w, label);
	addwin(w, 1);
	reframe();
	wwupdate();
	return w;
}

/*
 * Close an information window.
 */
closeiwin(w)
struct ww *w;
{
	closewin1(w);
	reframe();
}

closewin1(w)
register struct ww *w;
{
	if (w == selwin)
		selwin = 0;
	if (w == lastselwin)
		lastselwin = 0;
	if (w->ww_id >= 0 && w->ww_id < NWINDOW)
		window[w->ww_id] = 0;
	if (w->ww_label)
		str_free(w->ww_label);
	deletewin(w);
	wwclose(w);
}

/*
 * Move the window to the top of its group.
 * Don't do it if already fully visible.
 * Wwvisible() doesn't work for tinted windows.
 * But anything to make it faster.
 * Always reframe() if doreframe is true.
 */
front(w, doreframe)
register struct ww *w;
char doreframe;
{
	if (w->ww_back != (isfg(w) ? framewin : fgwin) && !wwvisible(w)) {
		deletewin(w);
		addwin(w, isfg(w));
		doreframe = 1;
	}
	if (doreframe)
		reframe();
}

/*
 * Add a window at the top of normal windows or foreground windows.
 * For normal windows, we put it behind the current window.
 */
addwin(w, fg)
register struct ww *w;
char fg;
{
	if (fg) {
		wwadd(w, framewin);
		if (fgwin == framewin)
			fgwin = w;
	} else
		wwadd(w, selwin != 0 && selwin != w && !isfg(selwin)
				? selwin : fgwin);
}

/*
 * Delete a window.
 */
deletewin(w)
register struct ww *w;
{
	if (fgwin == w)
		fgwin = w->ww_back;
	wwdelete(w);
}

reframe()
{
	register struct ww *w;

	wwunframe(framewin);
	for (w = wwhead.ww_back; w != &wwhead; w = w->ww_back)
		if (w->ww_hasframe) {
			wwframe(w, framewin);
			labelwin(w);
		}
}

labelwin(w)
register struct ww *w;
{
	int mode = w == selwin ? WWM_REV : 0;

	if (!w->ww_hasframe)
		return;
	if (w->ww_id >= 0) {
		char buf[2];

		buf[0] = w->ww_id + '1';
		buf[1] = 0;
		wwlabel(w, framewin, 1, buf, mode);
	}
	if (w->ww_label) {
		int col;

		if (w->ww_center) {
			col = (w->ww_w.nc - strlen(w->ww_label)) / 2;
			col = MAX(3, col);
		} else
			col = 3;
		wwlabel(w, framewin, col, w->ww_label, mode);
	}
}

stopwin(w)
	register struct ww *w;
{
	w->ww_stopped = 1;
	if (w->ww_pty >= 0 && w->ww_ispty)
		(void) ioctl(w->ww_pty, TIOCSTOP, (char *)0);
}

startwin(w)
	register struct ww *w;
{
	w->ww_stopped = 0;
	if (w->ww_pty >= 0 && w->ww_ispty)
		(void) ioctl(w->ww_pty, TIOCSTART, (char *)0);
}

sizewin(w, nrow, ncol)
register struct ww *w;
{
	struct ww *back = w->ww_back;

	w->ww_alt.nr = w->ww_w.nr;
	w->ww_alt.nc = w->ww_w.nc;
	wwdelete(w);
	if (wwsize(w, nrow, ncol) < 0)
		error(MSGSTR(CANTRESIZE, "Can't resize window: %s."), wwerror());
	wwadd(w, back);
	reframe();
}

waitnl(w)
struct ww *w;
{
	(void) waitnl1(w, MSGSTR(HITAKEY, "[Type any key to continue]"));
}

more(w, always)
register struct ww *w;
char always;
{
	int c;
	char uc = w->ww_unctrl;

	if (!always && w->ww_cur.r < w->ww_w.b - 2)
		return 0;
	c = waitnl1(w, MSGSTR(HITAKEY2,"[Type escape to abort, any other key to continue]"));
	w->ww_unctrl = 0;
	wwputs("\033E", w);
	w->ww_unctrl = uc;
	return c == ctrl('[') ? 2 : 1;
}

waitnl1(w, prompt)
register struct ww *w;
char *prompt;
{
	char uc = w->ww_unctrl;

	w->ww_unctrl = 0;
	front(w, 0);
	wwprintf(w, "\033Y%c%c\033sA%s\033rA ",
		w->ww_w.nr - 1 + ' ', ' ', prompt);	/* print on last line */
	wwcurtowin(w);
	while (wwpeekc() < 0)
		wwiomux();
	w->ww_unctrl = uc;
	return wwgetc();
}
