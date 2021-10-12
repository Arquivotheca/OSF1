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
static char	*sccsid = "@(#)$RCSfile: lcmd.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:10:57 $";
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
 * 	lcmd.c	3.28 (Berkeley) 6/29/88";
 */


#include "defs.h"
#include "value.h"
#include "lcmd.h"

int l_alias();
int l_close();
int l_cursormodes();
int l_debug();
int l_echo();
int l_escape();
int l_foreground();
int l_iostat();
int l_label();
int l_list();
int l_nline();
int l_select();
int l_shell();
int l_smooth();
int l_source();
int l_terse();
int l_time();
int l_unalias();
int l_unset();
int l_variable();
int l_window();
int l_write();

extern struct lcmd_arg arg_alias[];
extern struct lcmd_arg arg_cursormodes[];
extern struct lcmd_arg arg_debug[];
extern struct lcmd_arg arg_echo[];
extern struct lcmd_arg arg_escape[];
extern struct lcmd_arg arg_foreground[];
extern struct lcmd_arg arg_label[];
extern struct lcmd_arg arg_nline[];
extern struct lcmd_arg arg_close[];
extern struct lcmd_arg arg_select[];
extern struct lcmd_arg arg_shell[];
extern struct lcmd_arg arg_smooth[];
extern struct lcmd_arg arg_source[];
extern struct lcmd_arg arg_terse[];
extern struct lcmd_arg arg_time[];
extern struct lcmd_arg arg_unalias[];
extern struct lcmd_arg arg_unset[];
extern struct lcmd_arg arg_window[];
extern struct lcmd_arg arg_write[];
struct lcmd_arg arg_null[] = {
	0,	0,	0
};

struct lcmd_tab lcmd_tab[] = {
	"alias",	1,	l_alias,	arg_alias,
	"close",	2,	l_close,	arg_close,
	"cursormodes",	2,	l_cursormodes,	arg_cursormodes,
	"debug",	1,	l_debug,	arg_debug,
	"echo",		2,	l_echo,		arg_echo,
	"escape",	2,	l_escape,	arg_escape,
	"foreground",	1,	l_foreground,	arg_foreground,
	"iostat",	1,	l_iostat,	arg_null,
	"label",	2,	l_label,	arg_label,
	"list",		2,	l_list,		arg_null,
	"nlines",	1,	l_nline,	arg_nline,
	"select",	2,	l_select,	arg_select,
	"shell",	2,	l_shell,	arg_shell,
	"smooth",	2,	l_smooth,	arg_smooth,
	"source",	2,	l_source,	arg_source,
	"terse",	2,	l_terse,	arg_terse,
	"time",		2,	l_time,		arg_time,
	"unalias",	3,	l_unalias,	arg_unalias,
	"unset",	3,	l_unset,	arg_unset,
	"variable",	1,	l_variable,	arg_null,
	"window",	2,	l_window,	arg_window,
	"write",	2,	l_write,	arg_write,
	0
};

struct lcmd_tab *
lcmd_lookup(name)
char *name;
{
	register struct lcmd_tab *p;

	for (p = lcmd_tab; p->lc_name != 0; p++)
		if (str_match(name, p->lc_name, p->lc_minlen))
			return p;
	return 0;
}

dosource(filename)
char *filename;
{
	if (cx_beginfile(filename) < 0)
		return -1;
	p_start();
	err_end();
	cx_end();
	return 0;
}

dolongcmd(buffer, arg, narg)
char *buffer;
struct value *arg;
int narg;
{
	if (cx_beginbuf(buffer, arg, narg) < 0)
		return -1;
	p_start();
	err_end();
	cx_end();
	return 0;
}
