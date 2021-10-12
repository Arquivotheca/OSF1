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
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * $Header: /usr/sde/x11/rcs/x11/src/./server/ddx/x386/etc/inetemul.c,v 1.2 91/12/15 12:42:16 devrcs Exp $
 */


/*
 * This is a set of dummy-routines to make shared libs that were compiled for
 * sockets linkable without libinet.a
 *
 * They all return with an error-condition !!!
 */
#include <stdio.h> /* NULL */
#include <errno.h>


unsigned long inet_addr(cp)
     char *cp;
{
  return(-1);
}


void *gethostbyname(name)
     char *name;
{
  return (NULL);
}

int socket(domain, type, protocol)
     int domain, type, protocol;
{
  /* errno here, since they are only defined for TCP/IP ... */
  return(-1);
}



int setsockopt(s, level, optname, optval, optlen)
     int s, level, optname;
     char optval;
     int optlen;
{
  errno = EBADF;
  return (-1);
}     



int connect(s, name, namelen)
     int s;
     void *name;
     int namelen;
{
  errno = EBADF;
  return (-1);
}


int bind(s, name, namelen)
     int s;
     void *name;
     int namelen;
{
  errno = EBADF;
  return (-1);
}



int listen(s, backlog)
     int s, backlog;
{
  errno = EBADF;
  return (-1);
}



int accept(s, addr, addrlen)
     int s;
     void *addr;
     int *addrlen;
{
  errno = EBADF;
  return (-1);
}



int getpeername(s, name, namelen)
     int s;
     void *name;
     int namelen;
{
  errno = EBADF;
  return (-1);
}



int recvfrom(s, buf, len, flags, from, fromlen)
     int s;
     char buf;
     int len, flags;
     void *from;
     int fromlen;
{
  errno = EBADF;
  return (-1);
}



int sendto(s, msg, len, flags, to, tolen)
     int s;
     char msg;
     int len, flags;
     void *to;
     int tolen;
{
  errno = EBADF;
  return (-1);
}

