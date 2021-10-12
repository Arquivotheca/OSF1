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
static char	*sccsid = "@(#)$RCSfile: wwinit.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:13:49 $";
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
 * wwinit.c	3.31 (Berkeley) 6/29/88
 */


#include "ww.h"
#include "tt.h"
#include <sys/signal.h>
#include <fcntl.h>
#include "char.h"

wwinit()
{
	register i, j;
	char *kp;
	int s;

	wwdtablesize = getdtablesize();
	wwhead.ww_forw = &wwhead;
	wwhead.ww_back = &wwhead;

	s = sigblock(sigmask(SIGIO));
	if (signal(SIGIO, (void (*)(int))wwrint) == BADSIG ||
	    signal(SIGCHLD, (void (*)(int))wwchild) == BADSIG ||
	    signal(SIGPIPE, SIG_IGN) == BADSIG) {
		wwerrno = WWE_SYS;
		return -1;
	}

	if (wwgettty(0, &wwoldtty) < 0)
		return -1;
	wwwintty = wwoldtty;
#ifndef POSIX_TTY
        wwwintty.ww_sgttyb.sg_flags &= ~XTABS;
        wwnewtty.ww_sgttyb = wwoldtty.ww_sgttyb;
        wwnewtty.ww_sgttyb.sg_erase = -1;
        wwnewtty.ww_sgttyb.sg_kill = -1;
        wwnewtty.ww_sgttyb.sg_flags |= CBREAK;
        wwnewtty.ww_sgttyb.sg_flags &= ~(ECHO|CRMOD);
        wwnewtty.ww_tchars.t_intrc = -1;
        wwnewtty.ww_tchars.t_quitc = -1;
        wwnewtty.ww_tchars.t_startc = -1;
        wwnewtty.ww_tchars.t_stopc = -1;
        wwnewtty.ww_tchars.t_eofc = -1;
        wwnewtty.ww_tchars.t_brkc = -1;
        wwnewtty.ww_ltchars.t_suspc = -1;
        wwnewtty.ww_ltchars.t_dsuspc = -1;
        wwnewtty.ww_ltchars.t_rprntc = -1;
        wwnewtty.ww_ltchars.t_flushc = -1;
        wwnewtty.ww_ltchars.t_werasc = -1;
        wwnewtty.ww_ltchars.t_lnextc = -1;
        wwnewtty.ww_lmode = wwoldtty.ww_lmode | LLITOUT;
        wwnewtty.ww_ldisc = wwoldtty.ww_ldisc;
#else
	wwwintty.ww_termios.c_oflag &= ~OXTABS;
	wwnewtty.ww_termios = wwoldtty.ww_termios;
	wwnewtty.ww_termios.c_iflag &=
		~(ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IMAXBEL);
	wwnewtty.ww_termios.c_iflag |= INPCK;
	wwnewtty.ww_termios.c_oflag = 0;
	wwnewtty.ww_termios.c_cflag &= ~(CSIZE | PARENB);
	wwnewtty.ww_termios.c_cflag |= CS8;
	wwnewtty.ww_termios.c_lflag = 0;
	for (i = 0; i < NCCS; i++)
		wwnewtty.ww_termios.c_cc[i] = _POSIX_VDISABLE;
#endif
	wwnewtty.ww_fflags = wwoldtty.ww_fflags | FASYNC;
	if (wwsettty(0, &wwnewtty, &wwoldtty) < 0)
		goto bad;

	if ((wwterm = getenv("TERM")) == 0) {
		wwerrno = WWE_BADTERM;
		goto bad;
	}
	if (tgetent(wwtermcap, wwterm) != 1) {
		wwerrno = WWE_BADTERM;
		goto bad;
	}
#ifndef POSIX_TTY
        wwbaud = wwbaudmap[wwoldtty.ww_sgttyb.sg_ospeed];
#else
#ifdef CBAUD
        wwbaud = wwbaudmap[wwoldtty.ww_termios.c_cflag & CBAUD];
#else
        wwbaud = wwoldtty.ww_termios.c_ospeed;
#endif
#endif

	if (ttinit() < 0)
		goto bad;
	wwnrow = tt.tt_nrow;
	wwncol = tt.tt_ncol;
	wwavailmodes = tt.tt_availmodes;
	wwwrap = tt.tt_wrap;
	(*tt.tt_init)();

	if (wwavailmodes & WWM_REV)
		wwcursormodes = WWM_REV | wwavailmodes & WWM_BLK;
	else if (wwavailmodes & WWM_UL)
		wwcursormodes = WWM_UL;

	if ((wwib = malloc((unsigned) 512)) == 0)
		goto bad;
	wwibe = wwib + 512;
	wwibq = wwibp = wwib;

	if ((wwsmap = wwalloc(0, 0, wwnrow, wwncol, sizeof (char))) == 0)
		goto bad;
	for (i = 0; i < wwnrow; i++)
		for (j = 0; j < wwncol; j++)
			wwsmap[i][j] = WWX_NOBODY;

	wwos = (union ww_char **)
		wwalloc(0, 0, wwnrow, wwncol, sizeof (union ww_char));
	if (wwos == 0)
		goto bad;
	for (i = 0; i < wwnrow; i++)
		for (j = 0; j < wwncol; j++)
			wwos[i][j].c_w = ' ';
	wwns = (union ww_char **)
		wwalloc(0, 0, wwnrow, wwncol, sizeof (union ww_char));
	if (wwns == 0)
		goto bad;
	for (i = 0; i < wwnrow; i++)
		for (j = 0; j < wwncol; j++)
			wwns[i][j].c_w = ' ';

	wwtouched = malloc((unsigned) wwnrow);
	if (wwtouched == 0) {
		wwerrno = WWE_NOMEM;
		goto bad;
	}
	for (i = 0; i < wwnrow; i++)
		wwtouched[i] = 0;

	wwindex[WWX_NOBODY] = &wwnobody;
	wwnobody.ww_order = NWW;

	kp = wwwintermcap;
	if (wwavailmodes & WWM_REV)
		wwaddcap1(WWT_REV, &kp);
	if (wwavailmodes & WWM_BLK)
		wwaddcap1(WWT_BLK, &kp);
	if (wwavailmodes & WWM_UL)
		wwaddcap1(WWT_UL, &kp);
	if (wwavailmodes & WWM_GRP)
		wwaddcap1(WWT_GRP, &kp);
	if (wwavailmodes & WWM_DIM)
		wwaddcap1(WWT_DIM, &kp);
	if (wwavailmodes & WWM_USR)
		wwaddcap1(WWT_USR, &kp);
	if (tt.tt_insline && tt.tt_delline || tt.tt_setscroll)
		wwaddcap1(WWT_ALDL, &kp);
	if (tt.tt_hasinsert)
		wwaddcap1(WWT_IMEI, &kp);
	if (tt.tt_delchar)
		wwaddcap1(WWT_DC, &kp);
	wwaddcap("kb", &kp);
	wwaddcap("ku", &kp);
	wwaddcap("kd", &kp);
	wwaddcap("kl", &kp);
	wwaddcap("kr", &kp);
	wwaddcap("kh", &kp);
	if ((j = tgetnum("kn")) >= 0) {
		char cap[32];

		(void) sprintf(kp, "kn#%d:", j);
		for (; *kp; kp++)
			;
		for (i = 1; i <= j; i++) {
			(void) sprintf(cap, "k%d", i);
			wwaddcap(cap, &kp);
			cap[0] = 'l';
			wwaddcap(cap, &kp);
		}
	}
	/*
	 * It's ok to do this here even if setenv() is destructive
	 * since tt_init() has already made its own copy of it and
	 * wwterm now points to the copy.
	 */
	(void) setenv("TERM", WWT_TERM, 1);

	(void) signal(SIGPIPE, SIG_IGN);
	(void) sigsetmask(s);
	return 0;
bad:
	/*
	 * Don't bother to free storage.  We're supposed
	 * to exit when wwinit fails anyway.
	 */
	(void) wwsettty(0, &wwoldtty, &wwnewtty);
	(void) signal(SIGIO, SIG_DFL);
	(void) sigsetmask(s);
	return -1;
}

wwaddcap(cap, kp)
	register char *cap;
	register char **kp;
{
	char tbuf[512];
	char *tp = tbuf;
	register char *str, *p;

	if ((str = tgetstr(cap, &tp)) != 0) {
		while (*(*kp)++ = *cap++)
			;
		(*kp)[-1] = '=';
		while (*str) {
			for (p = unctrl(*str++); *(*kp)++ = *p++;)
				;
			(*kp)--;
		}
		*(*kp)++ = ':';
		**kp = 0;
	}
}

wwaddcap1(cap, kp)
	register char *cap;
	register char **kp;
{
	while (*(*kp)++ = *cap++)
		;
	(*kp)--;
}
