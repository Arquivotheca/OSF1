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
static char	*sccsid = "@(#)$RCSfile: string.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:11:45 $";
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
 * 	string.c	3.11 (Berkeley) 6/29/88
 */


#include "string.h"

char *malloc();

char *
str_cpy(s)
register char *s;
{
	char *str;
	register char *p;

	str = p = str_alloc(strlen(s) + 1);
	if (p == 0)
		return 0;
	while (*p++ = *s++)
		;
	return str;
}

char *
str_ncpy(s, n)
register char *s;
register n;
{
	int l = strlen(s);
	char *str;
	register char *p;

	if (n > l)
		n = l;
	str = p = str_alloc(n + 1);
	if (p == 0)
		return 0;
	while (--n >= 0)
		*p++ = *s++;
	*p = 0;
	return str;
}

char *
str_itoa(i)
int i;
{
	char buf[30];

	(void) sprintf(buf, "%d", i);
	return str_cpy(buf);
}

char *
str_cat(s1, s2)
char *s1, *s2;
{
	char *str;
	register char *p, *q;

	str = p = str_alloc(strlen(s1) + strlen(s2) + 1);
	if (p == 0)
		return 0;
	for (q = s1; *p++ = *q++;)
		;
	for (q = s2, p--; *p++ = *q++;)
		;
	return str;
}

/*
 * match s against p.
 * s can be a prefix of p with at least min characters.
 */
str_match(s, p, min)
register char *s, *p;
register min;
{
	for (; *s && *p && *s == *p; s++, p++, min--)
		;
	return *s == *p || *s == 0 && min <= 0;
}

#ifdef STR_DEBUG
char *
str_alloc(l)
int l;
{
	register struct string *s;

	s = (struct string *) malloc((unsigned)l + str_offset);
	if (s == 0)
		return 0;
	if (str_head.s_forw == 0)
		str_head.s_forw = str_head.s_back = &str_head;
	s->s_forw = str_head.s_forw;
	s->s_back = &str_head;
	str_head.s_forw = s;
	s->s_forw->s_back = s;
	return s->s_data;
}

str_free(str)
char *str;
{
	register struct string *s;

	for (s = str_head.s_forw; s != &str_head && s->s_data != str;
	     s = s->s_forw)
		;
	if (s == &str_head)
		abort();
	s->s_back->s_forw = s->s_forw;
	s->s_forw->s_back = s->s_back;
	free((char *)s);
}
#endif
