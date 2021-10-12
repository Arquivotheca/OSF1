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
static char	*sccsid = "@(#)$RCSfile: var.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:12:32 $";
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
 * 	var.c	3.10 (Berkeley) 6/29/88
 */


#include "value.h"
#include "var.h"
#include "string.h"

char *malloc();

struct var *
var_set1(head, name, v)
struct var **head;
char *name;
struct value *v;
{
	register struct var **p;
	register struct var *r;
	struct value val;

	/* do this first, easier to recover */
	val = *v;
	if (val.v_type == V_STR && val.v_str != 0 &&
	    (val.v_str = str_cpy(val.v_str)) == 0)
		return 0;
	if (*(p = var_lookup1(head, name)) == 0) {
		r = (struct var *) malloc(sizeof (struct var));
		if (r == 0) {
			val_free(val);
			return 0;
		}
		if ((r->r_name = str_cpy(name)) == 0) {
			val_free(val);
			free((char *) r);
			return 0;
		}
		r->r_left = r->r_right = 0;
		*p = r;
	} else {
		r = *p;
		val_free(r->r_val);
	}
	r->r_val = val;
	return r;
}

struct var *
var_setstr1(head, name, str)
struct var **head;
char *name;
char *str;
{
	struct value v;

	v.v_type = V_STR;
	v.v_str = str;
	return var_set1(head, name, &v);
}

struct var *
var_setnum1(head, name, num)
struct var **head;
char *name;
int num;
{
	struct value v;

	v.v_type = V_NUM;
	v.v_num = num;
	return var_set1(head, name, &v);
}

var_unset1(head, name)
struct var **head;
char *name;
{
	register struct var **p;
	register struct var *r;

	if (*(p = var_lookup1(head, name)) == 0)
		return -1;
	r = *p;
	*p = r->r_left;
	while (*p != 0)
		p = &(*p)->r_right;
	*p = r->r_right;
	val_free(r->r_val);
	str_free(r->r_name);
	free((char *) r);
	return 0;
}

struct var **
var_lookup1(p, name)
register struct var **p;
register char *name;
{
	register cmp;

	while (*p != 0) {
		if ((cmp = strcmp(name, (*p)->r_name)) < 0)
			p = &(*p)->r_left;
		else if (cmp > 0)
			p = &(*p)->r_right;
		else
			break;
	}
	return p;
}

var_walk1(r, func, a)
register struct var *r;
int (*func)();
{
	if (r == 0)
		return 0;
	if (var_walk1(r->r_left, func, a) < 0 || (*func)(a, r) < 0
	    || var_walk1(r->r_right, func, a) < 0)
		return -1;
	return 0;
}
