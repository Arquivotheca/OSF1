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
static char	*sccsid = "@(#)$RCSfile: wwwrite.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:14:57 $";
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
 * wwwrite.c	3.28 (Berkeley) 6/29/88
 */


#include "ww.h"
#include "tt.h"
#include "char.h"

#define UPDATE() \
	if (!w->ww_noupdate && w->ww_cur.r >= 0 && w->ww_cur.r < wwnrow && \
	    wwtouched[w->ww_cur.r]) \
		wwupdate1(w->ww_cur.r, w->ww_cur.r + 1)

/*
 * To support control character expansion, we save the old
 * p and q values in r and s, and point p at the beginning
 * of the expanded string, and q at some safe place beyond it
 * (p + 10).  At strategic points in the loops, we check
 * for (r && !*p) and restore the saved values back into
 * p and q.  Essentially, we implement a stack of depth 2,
 * to avoid recursion, which might be a better idea.
 */
wwwrite(w, p, n)
register struct ww *w;
register char *p;
int n;
{
	char hascursor;
	char *savep = p;
	char *q = p + n;
	char *r = 0;
	char *s;

#ifdef lint
	s = 0;			/* define it before possible use */
#endif
	if (hascursor = w->ww_hascursor)
		wwcursor(w, 0);
	while (p < q && !w->ww_stopped && (!wwinterrupt() || w->ww_nointr)) {
		if (r && !*p) {
			p = r;
			q = s;
			r = 0;
			continue;
		}
		if (w->ww_wstate == 0 &&
		    (isprt(*p) || w->ww_unctrl && isunctrl(*p))) {
			register i;
			register union ww_char *bp;
			int col, col1;

			if (w->ww_insert) {	/* this is very slow */
				if (*p == '\t') {
					p++;
					w->ww_cur.c += 8 -
						(w->ww_cur.c - w->ww_w.l & 7);
					goto chklf;
				}
				if (!isprt(*p)) {
					r = p + 1;
					s = q;
					p = unctrl(*p);
					q = p + 10;
				}
				wwinschar(w, w->ww_cur.r, w->ww_cur.c,
					*p++ | w->ww_modes << WWC_MSHIFT);
				goto right;
			}

			bp = &w->ww_buf[w->ww_cur.r][w->ww_cur.c];
			i = w->ww_cur.c;
			while (i < w->ww_w.r && p < q)
				if (!*p && r) {
					p = r;
					q = s;
					r = 0;
				} else if (*p == '\t') {
					register tmp = 8 - (i - w->ww_w.l & 7);
					p++;
					i += tmp;
					bp += tmp;
				} else if (isprt(*p)) {
					bp++->c_w = *p++
						| w->ww_modes << WWC_MSHIFT;
					i++;
				} else if (w->ww_unctrl && isunctrl(*p)) {
					r = p + 1;
					s = q;
					p = unctrl(*p);
					q = p + 10;
				} else
					break;
			col = MAX(w->ww_cur.c, w->ww_i.l);
			col1 = MIN(i, w->ww_i.r);
			w->ww_cur.c = i;
			if (w->ww_cur.r >= w->ww_i.t
			    && w->ww_cur.r < w->ww_i.b) {
				register union ww_char *ns = wwns[w->ww_cur.r];
				register char *smap = &wwsmap[w->ww_cur.r][col];
				register char *win = w->ww_win[w->ww_cur.r];
				int nchanged = 0;

				bp = w->ww_buf[w->ww_cur.r];
				for (i = col; i < col1; i++)
					if (*smap++ == w->ww_index) {
						nchanged++;
						ns[i].c_w = bp[i].c_w
							^ win[i] << WWC_MSHIFT;
					}
				if (nchanged > 0)
					wwtouched[w->ww_cur.r] |= WWU_TOUCHED;
			}
		chklf:
			if (w->ww_cur.c >= w->ww_w.r)
				goto crlf;
		} else switch (w->ww_wstate) {
		case 0:
			switch (*p++) {
			case '\n':
				if (w->ww_mapnl)
		crlf:
					w->ww_cur.c = w->ww_w.l;
		lf:
				UPDATE();
				if (++w->ww_cur.r >= w->ww_w.b) {
					w->ww_cur.r = w->ww_w.b - 1;
					if (w->ww_w.b < w->ww_b.b) {
						(void) wwscroll1(w, w->ww_i.t,
							w->ww_i.b, 1, 0);
						w->ww_buf++;
						w->ww_b.t--;
						w->ww_b.b--;
					} else
						wwdelline(w, w->ww_b.t);
				}
				break;
			case '\b':
				if (--w->ww_cur.c < w->ww_w.l) {
					w->ww_cur.c = w->ww_w.r - 1;
					goto up;
				}
				break;
			case '\r':
				w->ww_cur.c = w->ww_w.l;
				break;
			case ctrl('g'):
				ttputc(ctrl('g'));
				break;
			case ctrl('['):
				w->ww_wstate = 1;
				break;
			}
			break;
		case 1:
			w->ww_wstate = 0;
			switch (*p++) {
			case '@':
				w->ww_insert = 1;
				break;
			case 'A':
		up:
				UPDATE();
				if (--w->ww_cur.r < w->ww_w.t) {
					w->ww_cur.r = w->ww_w.t;
					if (w->ww_w.t > w->ww_b.t) {
						(void) wwscroll1(w, w->ww_i.t,
							w->ww_i.b, -1, 0);
						w->ww_buf--;
						w->ww_b.t++;
						w->ww_b.b++;
					} else
						wwinsline(w, w->ww_b.t);
				}
				break;
			case 'B':
				goto lf;
			case 'C':
		right:
				w->ww_cur.c++;
				goto chklf;
			case 'E':
				w->ww_buf -= w->ww_w.t - w->ww_b.t;
				w->ww_b.t = w->ww_w.t;
				w->ww_b.b = w->ww_b.t + w->ww_b.nr;
				w->ww_cur.r = w->ww_w.t;
				w->ww_cur.c = w->ww_w.l;
				wwclreos(w, w->ww_w.t, w->ww_w.l);
				break;
			case 'H':
				UPDATE();
				w->ww_cur.r = w->ww_w.t;
				w->ww_cur.c = w->ww_w.l;
				break;
			case 'J':
				wwclreos(w, w->ww_cur.r, w->ww_cur.c);
				break;
			case 'K':
				wwclreol(w, w->ww_cur.r, w->ww_cur.c);
				break;
			case 'L':
				UPDATE();
				wwinsline(w, w->ww_cur.r);
				break;
			case 'M':
				wwdelline(w, w->ww_cur.r);
				break;
			case 'N':
				wwdelchar(w, w->ww_cur.r, w->ww_cur.c);
				break;
			case 'O':
				w->ww_insert = 0;
				break;
			case 'Y':
				UPDATE();
				w->ww_wstate = 2;
				break;
			case 's':
				w->ww_wstate = 4;
				break;
			case 'r':
				w->ww_wstate = 5;
				break;
			}
			break;
		case 2:
			w->ww_cur.r = w->ww_w.t +
				(unsigned)(*p++ - ' ') % w->ww_w.nr;
			w->ww_wstate = 3;
			break;
		case 3:
			w->ww_cur.c = w->ww_w.l +
				(unsigned)(*p++ - ' ') % w->ww_w.nc;
			w->ww_wstate = 0;
			break;
		case 4:
			w->ww_modes |= *p++ & wwavailmodes;
			w->ww_wstate = 0;
			break;
		case 5:
			w->ww_modes &= ~*p++;
			w->ww_wstate = 0;
			break;
		}
	}
	if (hascursor)
		wwcursor(w, 1);
	wwnwwr++;
	wwnwwra += n;
	n = p - savep;
	wwnwwrc += n;
	return n;
}
