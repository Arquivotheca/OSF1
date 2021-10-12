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
static char	*sccsid = "@(#)$RCSfile: lcmd1.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:11:04 $";
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
 * 	lcmd1.c	3.33 (Berkeley) 6/29/88";
 */


#include "defs.h"
#include "string.h"
#include "value.h"
#include "lcmd.h"
#include "var.h"

struct lcmd_arg arg_window[] = {
	{ "row",	1,	ARG_NUM },
	{ "column",	1,	ARG_NUM },
	{ "nrows",	2,	ARG_NUM },
	{ "ncols",	2,	ARG_NUM },
	{ "nlines",	2,	ARG_NUM },
	{ "label",	1,	ARG_STR },
	{ "pty",	1,	ARG_ANY },
	{ "frame",	1,	ARG_ANY },
	{ "mapnl",	1,	ARG_ANY },
	{ "keepopen",	1,	ARG_ANY },
	{ "smooth",	1,	ARG_ANY },
	{ "shell",	1,	ARG_STR|ARG_LIST },
	0
};

l_window(v, a)
struct value *v;
register struct value *a;
{
	struct ww *w;
	int col, row, ncol, nrow, id, nline;
	char *label;
	int haspty, hasframe, mapnl, keepopen, smooth;
	char *shf, **sh;
	char *argv[sizeof shell / sizeof *shell];
	register char **pp;

	if ((id = findid()) < 0)
		return;
	row = a->v_type == V_ERR ? 1 : a->v_num;
	a++;
	col = a->v_type == V_ERR ? 0 : a->v_num;
	a++;
	nrow = a->v_type == V_ERR ? wwnrow - row : a->v_num;
	a++;
	ncol = a->v_type == V_ERR ? wwncol - col : a->v_num;
	a++;
	nline = a->v_type == V_ERR ? nbufline : a->v_num;
	a++;
	label = a->v_type == V_ERR ? 0 : a->v_str;
	if ((haspty = vtobool(++a, 1, -1)) < 0)
		return;
	if ((hasframe = vtobool(++a, 1, -1)) < 0)
		return;
	if ((mapnl = vtobool(++a, !haspty, -1)) < 0)
		return;
	if ((keepopen = vtobool(++a, 0, -1)) < 0)
		return;
	if ((smooth = vtobool(++a, 1, -1)) < 0)
		return;
	if ((++a)->v_type != V_ERR) {
		for (pp = argv; a->v_type != V_ERR &&
		     pp < &argv[sizeof argv/sizeof *argv-1]; pp++, a++)
			*pp = a->v_str;
		*pp = 0;
		shf = *(sh = argv);
		if (*sh = rindex(shf, '/'))
			(*sh)++;
		else
			*sh = shf;
	} else {
		sh = shell;
		shf = shellfile;
	}
	if ((w = openwin(id, row, col, nrow, ncol, nline, label, haspty,
	    hasframe, shf, sh)) == 0)
		return;
	w->ww_mapnl = mapnl;
	w->ww_keepopen = keepopen;
	w->ww_noupdate = !smooth;
	v->v_type = V_NUM;
	v->v_num = id + 1;
}

struct lcmd_arg arg_nline[] = {
	{ "nlines",	1,	ARG_NUM },
	0
};

l_nline(v, a)
register struct value *v, *a;
{
	v->v_num = nbufline;
	v->v_type = V_NUM;
	if (a->v_type != V_ERR)
		nbufline = a->v_num;
}

struct lcmd_arg arg_smooth[] = {
	{ "window",	1,	ARG_NUM },
	{ "flag",	1,	ARG_ANY },
	0
};

l_smooth(v, a)
register struct value *v, *a;
{
	struct ww *w;

	v->v_type = V_NUM;
	v->v_num = 0;
	if ((w = vtowin(a++, selwin)) == 0)
		return;
	v->v_num = !w->ww_noupdate;
	w->ww_noupdate = !vtobool(a, v->v_num, v->v_num);
}

struct lcmd_arg arg_select[] = {
	{ "window",	1,	ARG_NUM },
	0
};

l_select(v, a)
register struct value *v, *a;
{
	struct ww *w;

	v->v_type = V_NUM;
	v->v_num = selwin ? selwin->ww_id + 1 : -1;
	if (a->v_type == V_ERR)
		return;
	if ((w = vtowin(a, (struct ww *)0)) == 0)
		return;
	setselwin(w);
}

struct lcmd_arg arg_debug[] = {
	{ "flag",	1,	ARG_ANY },
	0
};

l_debug(v, a)
register struct value *v, *a;
{
	v->v_type = V_NUM;
	v->v_num = debug;
	debug = vtobool(a, debug, debug);
}

struct lcmd_arg arg_escape[] = {
	{ "escapec",	1,	ARG_STR },
	0
};

l_escape(v, a)
register struct value *v, *a;
{
	char buf[2];

	buf[0] = escapec;
	buf[1] = 0;
	if ((v->v_str = str_cpy(buf)) == 0) {
		error(MSGSTR(OUTOFMEM, "Out of memory."));
		return;
	}
	v->v_type = V_STR;
	if (a->v_type != V_ERR)
		setescape(a->v_str);
}

struct lcmd_arg arg_label[] = {
	{ "window",	1,	ARG_NUM },
	{ "label",	1,	ARG_STR },
	0
};

/*ARGSUSED*/
l_label(v, a)
struct value *v;
register struct value *a;
{
	struct ww *w;

	if ((w = vtowin(a, selwin)) == 0)
		return;
	if ((++a)->v_type != V_ERR && setlabel(w, a->v_str) < 0)
		error("Out of memory.");
	reframe();
}

struct lcmd_arg arg_foreground[] = {
	{ "window",	1,	ARG_NUM },
	{ "flag",	1,	ARG_ANY },
	0
};

l_foreground(v, a)
register struct value *v, *a;
{
	struct ww *w;
	char flag;

	if ((w = vtowin(a, selwin)) == 0)
		return;
	v->v_type = V_NUM;
	v->v_num = isfg(w);
	flag = vtobool(++a, v->v_num, v->v_num);
	if (flag == v->v_num)
		return;
	deletewin(w);
	addwin(w, flag);
	reframe();
}

struct lcmd_arg arg_terse[] = {
	{ "flag",	1,	ARG_ANY },
	0
};

l_terse(v, a)
register struct value *v, *a;
{
	v->v_type = V_NUM;
	v->v_num = terse;
	setterse(vtobool(a, terse, terse));
}

struct lcmd_arg arg_source[] = {
	{ "filename",	1,	ARG_STR },
	0
};

l_source(v, a)
register struct value *v, *a;
{
	v->v_type = V_NUM;
	if (a->v_type != V_ERR && dosource(a->v_str) < 0) {
		error(MSGSTR(CANTOPEN, "Can't open %s."), a->v_str);
		v->v_num = -1;
	} else
		v->v_num = 0;
}

struct lcmd_arg arg_write[] = {
	{ "window",	1,	ARG_NUM },
	{ "",		0,	ARG_ANY|ARG_LIST },
	0
};

/*ARGSUSED*/
l_write(v, a)
struct value *v;
register struct value *a;
{
	char buf[20];
	struct ww *w;

	if ((w = vtowin(a++, selwin)) == 0)
		return;
	while (a->v_type != V_ERR) {
		if (a->v_type == V_NUM) {
			(void) sprintf(buf, "%d", a->v_num);
			(void) write(w->ww_pty, buf, strlen(buf));
		} else
			(void) write(w->ww_pty, a->v_str, strlen(a->v_str));
		if ((++a)->v_type != V_ERR)
			(void) write(w->ww_pty, " ", 1);
	}
}

struct lcmd_arg arg_close[] = {
	{ "window",	1,	ARG_ANY|ARG_LIST },
	0
};

/*ARGSUSED*/
l_close(v, a)
struct value *v;
register struct value *a;
{
	struct ww *w;

	if (a->v_type == V_STR && str_match(a->v_str, "all", 3))
		closewin((struct ww *)0);
	else
		for (; a->v_type != V_ERR; a++)
			if ((w = vtowin(a, (struct ww *)0)) != 0)
				closewin(w);
}

struct lcmd_arg arg_cursormodes[] = {
	{ "modes",	1,	ARG_NUM },
	0
};

l_cursormodes(v, a)
register struct value *v, *a;
{

	v->v_type = V_NUM;
	v->v_num = wwcursormodes;
	if (a->v_type != V_ERR)
		wwsetcursormodes(a->v_num);
}

struct lcmd_arg arg_unset[] = {
	{ "variable",	1,	ARG_ANY },
	0
};

l_unset(v, a)
register struct value *v, *a;
{
	v->v_type = V_NUM;
	switch (a->v_type) {
	case V_ERR:
		v->v_num = -1;
		return;
	case V_NUM:
		if ((a->v_str = str_itoa(a->v_num)) == 0) {
			error(MSGSTR(OUTOFMEM, "Out of memory."));
			v->v_num = -1;
			return;
		}
		a->v_type = V_STR;
		break;
	}
	v->v_num = var_unset(a->v_str);
}

struct ww *
vtowin(v, w)
register struct value *v;
struct ww *w;
{
	switch (v->v_type) {
	case V_ERR:
		if (w != 0)
			return w;
		error(MSGSTR(NOWINSPEC, "No window specified."));
		return 0;
	case V_STR:
		error("%s: No such window.", v->v_str);
		return 0;
	}
	if (v->v_num < 1 || v->v_num > NWINDOW
	    || (w = window[v->v_num - 1]) == 0) {
		error("%d: No such window.", v->v_num);
		return 0;
	}
	return w;
}

vtobool(v, def, err)
register struct value *v;
int def, err;
{
	switch (v->v_type) {
	case V_NUM:
		return v->v_num != 0;
	case V_STR:
		if (str_match(v->v_str, "true", 1)
		    || str_match(v->v_str, "on", 2)
		    || str_match(v->v_str, "yes", 1))
			return 1;
		else if (str_match(v->v_str, "false", 1)
		    || str_match(v->v_str, "off", 2)
		    || str_match(v->v_str, "no", 1))
			return 0;
		else {
			error("%s: Illegal boolean value.", v->v_str);
			return err;
		}
		/*NOTREACHED*/
	case V_ERR:
		return def;
	}
	/*NOTREACHED*/
}
