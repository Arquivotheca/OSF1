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
static char	*sccsid = "@(#)$RCSfile: wwtty.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:14:50 $";
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
 * wwtty.c	3.12 (Berkeley) 6/29/88
 */


#include "ww.h"
#include <fcntl.h>

wwgettty(d, t)
register struct ww_tty *t;
{
#ifndef POSIX_TTY
        if (ioctl(d, TIOCGETP, (char *)&t->ww_sgttyb) < 0)
                goto bad;
        if (ioctl(d, TIOCGETC, (char *)&t->ww_tchars) < 0)
                goto bad;
        if (ioctl(d, TIOCGLTC, (char *)&t->ww_ltchars) < 0)
                goto bad;
        if (ioctl(d, TIOCLGET, (char *)&t->ww_lmode) < 0)
                goto bad;
        if (ioctl(d, TIOCGETD, (char *)&t->ww_ldisc) < 0)
                goto bad;
#else
	if (tcgetattr(d, &t->ww_termios) < 0)
		goto bad;
#endif
	if ((t->ww_fflags = fcntl(d, F_GETFL, 0)) < 0)
		goto bad;
	return 0;
bad:
	wwerrno = WWE_SYS;
	return -1;
}

/*
 * Set the modes of tty 'd' to 't'
 * 'o' is the current modes.  We set the line discipline only if
 * it changes, to avoid unnecessary flushing of typeahead.
 */
wwsettty(d, t, o)
register struct ww_tty *t, *o;
{
#ifndef POSIX_TTY
        /* for buggy tty driver that doesn't wait for output to drain */
        int i;
        while (ioctl(d, TIOCOUTQ, &i) >= 0 && i > 0)
                usleep(100000);
        if (ioctl(d, TIOCSETN, (char *)&t->ww_sgttyb) < 0)
                goto bad;
        if (ioctl(d, TIOCSETC, (char *)&t->ww_tchars) < 0)
                goto bad;
        if (ioctl(d, TIOCSLTC, (char *)&t->ww_ltchars) < 0)
                goto bad;
        if (ioctl(d, TIOCLSET, (char *)&t->ww_lmode) < 0)
                goto bad;
        if ((o == 0 || t->ww_ldisc != o->ww_ldisc) &&
            ioctl(d, TIOCSETD, (char *)&t->ww_ldisc) < 0)
                goto bad;
#else
	if (tcsetattr(d, TCSADRAIN, &t->ww_termios) < 0)
		goto bad;
#endif
	if (fcntl(d, F_SETFL, t->ww_fflags) < 0)
		goto bad;
	return 0;
bad:
	wwerrno = WWE_SYS;
	return -1;
}
