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
static char	*sccsid = "@(#)$RCSfile: lcmd2.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1992/04/14 13:14:49 $";
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
 * 	lcmd2.c	3.15 (Berkeley) 6/29/88";
 */


#include "defs.h"
#include "string.h"
#include "value.h"
#include "var.h"
#include "lcmd.h"
#include <sys/resource.h>
#include "alias.h"

/*ARGSUSED*/
l_iostat(v, a)
struct value *v, *a;
{
	register struct ww *w;

	if ((w = openiwin(14, MSGSTR(IOSTAT,"IO Statistics"))) == 0) {
		error(MSGSTR(CANTOPENSTAT, "Can't open statistics window: %s."), wwerror());
		return;
	}
	wwprintf(w, "ttflush\twrite\terror\tzero\tchar\n");
	wwprintf(w, "%d\t%d\t%d\t%d\t%d\n",
		wwnflush, wwnwr, wwnwre, wwnwrz, wwnwrc);
	wwprintf(w, "wwwrite\tattempt\tchar\n");
	wwprintf(w, "%d\t%d\t%d\n",
		wwnwwr, wwnwwra, wwnwwrc);
	wwprintf(w, "wwupdat\tline\tmiss\tmajor\tmiss\n");
	wwprintf(w, "%d\t%d\t%d\t%d\t%d\n",
		wwnupdate, wwnupdline, wwnupdmiss, wwnmajline, wwnmajmiss);
	wwprintf(w, "select\terror\tzero\n");
	wwprintf(w, "%d\t%d\t%d\n",
		wwnselect, wwnselecte, wwnselectz);
	wwprintf(w, "read\terror\tzero\tchar\n");
	wwprintf(w, "%d\t%d\t%d\t%d\n",
		wwnread, wwnreade, wwnreadz, wwnreadc);
	wwprintf(w, "ptyread\terror\tzero\tcontrol\tdata\tchar\n");
	wwprintf(w, "%d\t%d\t%d\t%d\t%d\t%d\n",
		wwnwread, wwnwreade, wwnwreadz,
		wwnwreadp, wwnwreadd, wwnwreadc);
	waitnl(w);
	closeiwin(w);
}

struct lcmd_arg arg_time[] = {
	{ "who",	1,	ARG_STR },
	0
};

/*ARGSUSED*/
l_time(v, a)
struct value *v;
register struct value *a;
{
	register struct ww *w;
	struct rusage rusage;
	struct timeval timeval;
	char *strtime();

	if ((w = openiwin(6, MSGSTR(TRUSAGE, "Timing and Resource Usage"))) == 0) {
		error(MSGSTR(COTIME, "Can't open time window: %s."), wwerror());
		return;
	}

	(void) gettimeofday(&timeval, (struct timezone *)0);
	timeval.tv_sec -= starttime.tv_sec;
	if ((timeval.tv_usec -= starttime.tv_usec) < 0) {
		timeval.tv_sec--;
		timeval.tv_usec += 1000000;
	}
	(void) getrusage(a->v_type == V_STR
			&& str_match(a->v_str, "children", 1)
		? RUSAGE_CHILDREN : RUSAGE_SELF, &rusage);

	wwprintf(w, "time\t\tutime\t\tstime\t\tmaxrss\tixrss\tidrss\tisrss\n");
	wwprintf(w, "%-16s", strtime(&timeval));
	wwprintf(w, "%-16s", strtime(&rusage.ru_utime));
	wwprintf(w, "%-16s", strtime(&rusage.ru_stime));
	wwprintf(w, "%ld\t%ld\t%ld\t%ld\n",
		rusage.ru_maxrss, rusage.ru_ixrss,
		rusage.ru_idrss, rusage.ru_isrss);
	wwprintf(w, "minflt\tmajflt\tnswap\tinblk\toublk\tmsgsnd\tmsgrcv\tnsigs\tnvcsw\tnivcsw\n");
	wwprintf(w, "%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\n",
		rusage.ru_minflt, rusage.ru_majflt, rusage.ru_nswap,
		rusage.ru_inblock, rusage.ru_oublock,
		rusage.ru_msgsnd, rusage.ru_msgrcv, rusage.ru_nsignals,
		rusage.ru_nvcsw, rusage.ru_nivcsw);

	waitnl(w);
	closeiwin(w);
}

char *
strtime(t)
register struct timeval *t;
{
	char fill = 0;
	static char buf[20];
	register char *p = buf;

	if (t->tv_sec > 60*60) {
		(void) sprintf(p, "%ld:", t->tv_sec / (60*60));
		while (*p++)
			;
		p--;
		t->tv_sec %= 60*60;
		fill++;
	}
	if (t->tv_sec > 60) {
		(void) sprintf(p, fill ? "%02ld:" : "%ld:", t->tv_sec / 60);
		while (*p++)
			;
		p--;
		t->tv_sec %= 60;
		fill++;
	}
	(void) sprintf(p, fill ? "%02ld.%02d" : "%ld.%02ld",
		t->tv_sec, t->tv_usec / 10000);
	return buf;
}

/*ARGSUSED*/
l_list(v, a)
struct value *v, *a;
{
	register struct ww *w, *wp;
	register i;
	int n;

	for (n = 0, i = 0; i < NWINDOW; i++)
		if (window[i] != 0)
			n++;
	if (n == 0) {
		error("No windows.");
		return;
	}
	if ((w = openiwin(n + 2, MSGSTR(WINDOWS, "Windows"))) == 0) {
		error(MSGSTR(COWINDOWS, "Can't open listing window: %s."), wwerror());
		return;
	}
	for (i = 0; i < NWINDOW; i++) {
		if ((wp = window[i]) == 0)
			continue;
		wwprintf(w, "%c %c %-13s %-.*s\n",
			wp == selwin ? '+' : (wp == lastselwin ? '-' : ' '),
			i + '1',
			wp->ww_state == WWS_HASPROC ? "" : MSGSTR(NOPROC,"(No process)"),
			wwncol - 20,
			wp->ww_label ? wp->ww_label : MSGSTR(NOLABEL, "(No label)"));
	}
	waitnl(w);
	closeiwin(w);
}

/*ARGSUSED*/
l_variable(v, a)
struct value *v, *a;
{
	register struct ww *w;
	int printvar();

	if ((w = openiwin(wwnrow - 3, MSGSTR(VARS, "Variables"))) == 0) {
		error(MSGSTR(COVARS, "Can't open variable window: %s."), wwerror());
		return;
	}
	if (var_walk(printvar, (long)w) >= 0)
		waitnl(w);
	closeiwin(w);
}

printvar(w, r)
register struct ww *w;
register struct var *r;
{
	if (more(w, 0) == 2)
		return -1;
	wwprintf(w, "%16s    ", r->r_name);
	switch (r->r_val.v_type) {
	case V_STR:
		wwprintf(w, "%s\n", r->r_val.v_str);
		break;
	case V_NUM:
		wwprintf(w, "%d\n", r->r_val.v_num);
		break;
	case V_ERR:
		wwprintf(w, MSGSTR(ERROR, "ERROR\n"));
		break;
	}
	return 0;
}

struct lcmd_arg arg_shell[] = {
	{ "",	0,		ARG_ANY|ARG_LIST },
	0
};

l_shell(v, a)
	struct value *v, *a;
{
	register char **pp;
	register struct value *vp;

	if (a->v_type == V_ERR) {
		if ((v->v_str = str_cpy(shellfile)) != 0)
			v->v_type = V_STR;
		return;
	}
	if (v->v_str = shellfile) {
		v->v_type = V_STR;
		for (pp = shell + 1; *pp; pp++) {
			str_free(*pp);
			*pp = 0;
		}
	}
	for (pp = shell, vp = a;
	     vp->v_type != V_ERR && pp < &shell[sizeof shell/sizeof *shell-1];
	     pp++, vp++)
		if ((*pp = vp->v_type == V_STR ?
		     str_cpy(vp->v_str) : str_itoa(vp->v_num)) == 0) {
			/* just leave shell[] the way it is */
			p_memerror();
			break;
		}
	if (shellfile = *shell)
		if (*shell = rindex(shellfile, '/'))
			(*shell)++;
		else
			*shell = shellfile;
}

struct lcmd_arg arg_alias[] = {
	{ "",	0,		ARG_STR },
	{ "",	0,		ARG_STR|ARG_LIST },
	0
};

l_alias(v, a)
	struct value *v, *a;
{
	if (a->v_type == V_ERR) {
		register struct ww *w;
		int printalias();

		if ((w = openiwin(wwnrow - 3, MSGSTR(ALIASES, "Aliases"))) == 0) {
			error(MSGSTR(COALIASES, "Can't open alias window: %s."), wwerror());
			return;
		}
		if (alias_walk(printalias, (long)w) >= 0)
			waitnl(w);
		closeiwin(w);
	} else {
		register struct alias *ap = 0;

		if (ap = alias_lookup(a->v_str)) {
			if ((v->v_str = str_cpy(ap->a_buf)) == 0) {
				p_memerror();
				return;
			}
			v->v_type = V_STR;
		}
		if (a[1].v_type == V_STR) {
			register struct value *vp;
			register char *p, *q;
			char *str;
			register n;

			for (n = 0, vp = a + 1; vp->v_type != V_ERR; vp++, n++)
				for (p = vp->v_str; *p; p++, n++)
					;
			if ((str = str_alloc(n)) == 0) {
				p_memerror();
				return;
			}
			for (q = str, vp = a + 1; vp->v_type != V_ERR;
			     vp++, q[-1] = ' ')
				for (p = vp->v_str; *q++ = *p++;)
					;
			q[-1] = 0;
			if ((ap = alias_set(a[0].v_str, (char *)0)) == 0) {
				p_memerror();
				str_free(str);
				return;
			}
			ap->a_buf = str;
		}
	}
}

printalias(w, a)
register struct ww *w;
register struct alias *a;
{
	if (more(w, 0) == 2)
		return -1;
	wwprintf(w, "%16s    %s\n", a->a_name, a->a_buf);
	return 0;
}

struct lcmd_arg arg_unalias[] = {
	{ "alias",	1,	ARG_STR },
	0
};

l_unalias(v, a)
struct value *v, *a;
{
	if (a->v_type == ARG_STR)
		v->v_num = alias_unset(a->v_str);
	v->v_type = V_NUM;
}

struct lcmd_arg arg_echo[] = {
	{ "window",	1,	ARG_NUM },
	{ "",		0,	ARG_ANY|ARG_LIST },
	0
};

/*ARGSUSED*/
l_echo(v, a)
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
			(void) wwwrite(w, buf, strlen(buf));
		} else
			(void) wwwrite(w, a->v_str, strlen(a->v_str));
		if ((++a)->v_type != V_ERR)
			(void) wwwrite(w, " ", 1);
	}
	(void) wwwrite(w, "\r\n", 2);
}
