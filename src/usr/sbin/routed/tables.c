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
static char	*sccsid = "@(#)$RCSfile: tables.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1992/03/09 10:55:39 $";
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

*/
/* 
 * COMPONENT_NAME: TCPIP tables.c
 * 
 * FUNCTIONS: MSGSTR, rtadd, rtchange, rtdefault, rtdelete, 
 *            rtdeleteall, rtfind, rtinit, rtlookup 
 *
 * ORIGINS: 10  26  27 
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1983, 1988 Regents of the University of California.
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
 */
/*
#ifndef lint
static char sccsid[] = "tables.c	5.17 (Berkeley) 6/1/90";
#endif  not lint */

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/syslog.h>

#ifndef DEBUG
#define	DEBUG	0
#endif

#ifdef RTM_ADD
#define FIXLEN(s) {if ((s)->sa_len == 0) (s)->sa_len = sizeof *(s);}
#else
#define FIXLEN(s) { }
#endif
int	install = !DEBUG;		/* if 1 call kernel */

void insque();
void remque();

/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *
rtlookup(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int doinghost = 1;

	if (dst->sa_family >= af_max)
		return (0);
	(*afswitch[dst->sa_family].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (equal(&rt->rt_dst, dst))
			return (rt);
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		goto again;
	}
	return (0);
}

struct sockaddr wildcard;	/* zero valued cookie for wildcard searches */

/*
 * Find a route to dst as the kernel would.
 */
struct rt_entry *
rtfind(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int af = dst->sa_family;
	int doinghost = 1, (*match)();

	if (af >= af_max)
		return (0);
	(*afswitch[af].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (doinghost) {
			if (equal(&rt->rt_dst, dst))
				return (rt);
		} else {
			if (rt->rt_dst.sa_family == af &&
			    (*match)(&rt->rt_dst, dst))
				return (rt);
		}
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		match = afswitch[af].af_netmatch;
		goto again;
	}
#ifdef notyet
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
#endif
	return (0);
}

rtadd(dst, gate, metric, state)
	struct sockaddr *dst, *gate;
	int metric, state;
{
	struct afhash h;
	register struct rt_entry *rt;
	struct rthash *rh;
	int af = dst->sa_family, flags;
	u_int hash;

	if (af >= af_max)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	flags = (*afswitch[af].af_rtflags)(dst);
	/*
	 * Subnet flag isn't visible to kernel, move to state.	XXX
	 */
	FIXLEN(dst);
        FIXLEN(gate);
	if (flags & RTF_SUBNET) {
		state |= RTS_SUBNET;
		flags &= ~RTF_SUBNET;
	}
	if (flags & RTF_HOST) {
		hash = h.afh_hosthash;
		rh = &hosthash[hash & ROUTEHASHMASK];
	} else {
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
	}
	rt = (struct rt_entry *)malloc(sizeof (*rt));
	if (rt == 0)
		return;
	rt->rt_hash = hash;
	rt->rt_dst = *dst;
	rt->rt_router = *gate;
	rt->rt_timer = 0;
	rt->rt_flags = RTF_UP | flags;
	rt->rt_state = state | RTS_CHANGED;
	rt->rt_ifp = if_ifwithdstaddr(&rt->rt_dst);
	if (rt->rt_ifp == 0)
		rt->rt_ifp = if_ifwithnet(&rt->rt_router);
	if ((state & RTS_INTERFACE) == 0)
		rt->rt_flags |= RTF_GATEWAY;
	rt->rt_metric = metric;
	insque(rt, rh);
	TRACE_ACTION("ADD", rt);
	/*
	 * If the ioctl fails because the gateway is unreachable
	 * from this host, discard the entry.  This should only
	 * occur because of an incorrect entry in /etc/gateways.
	 */
	if (install && (rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
	    ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0) {
		if (errno != EEXIST && gate->sa_family < af_max)
			syslog(LOG_ERR,
			MSGSTR(ADDROUTE,"adding route to net/host %s through gateway %s: %m\n"), /*MSG*/
			   (*afswitch[dst->sa_family].af_format)(dst),
			   (*afswitch[gate->sa_family].af_format)(gate));
		perror(MSGSTR(ADDRTE, "SIOCADDRT")); /*MSG*/
		if (errno == ENETUNREACH) {
			TRACE_ACTION("DELETE", rt);
			remque(rt);
			free((char *)rt);
		}
	}
}

rtchange(rt, gate, metric)
	struct rt_entry *rt;
	struct sockaddr *gate;
	short metric;
{
	int add = 0, delete = 0, newgateway = 0;
	struct rtentry oldroute;

	FIXLEN(gate);
        FIXLEN(&(rt->rt_router));
        FIXLEN(&(rt->rt_dst));

	if (!equal(&rt->rt_router, gate)) {
		newgateway++;
		TRACE_ACTION("CHANGE FROM ", rt);
	} else if (metric != rt->rt_metric)
		TRACE_NEWMETRIC(rt, metric);
	if ((rt->rt_state & RTS_INTERNAL) == 0) {
		/*
		 * If changing to different router, we need to add
		 * new route and delete old one if in the kernel.
		 * If the router is the same, we need to delete
		 * the route if has become unreachable, or re-add
		 * it if it had been unreachable.
		 */
		if (newgateway) {
			add++;
			if (rt->rt_metric != HOPCNT_INFINITY)
				delete++;
		} else if (metric == HOPCNT_INFINITY)
			delete++;
		else if (rt->rt_metric == HOPCNT_INFINITY)
			add++;
	}
	if (delete)
		oldroute = rt->rt_rt;
	if ((rt->rt_state & RTS_INTERFACE) && delete) {
		rt->rt_state &= ~RTS_INTERFACE;
		rt->rt_flags |= RTF_GATEWAY;
		if (metric > rt->rt_metric && delete) {
		    if (add)
			syslog(LOG_ERR, MSGSTR(CHGRTE, "changing route from interface %s (timed out)"), /*MSG*/
			    rt->rt_ifp->int_name);
		    else
			syslog(LOG_ERR, MSGSTR(DELIF, "deleting route to interface %s (timed out)"), /*MSG*/
			    rt->rt_ifp->int_name);
		}
	}
	if (add) {
		rt->rt_router = *gate;
		rt->rt_ifp = if_ifwithdstaddr(&rt->rt_router);
		if (rt->rt_ifp == 0)
			rt->rt_ifp = if_ifwithnet(&rt->rt_router);
	}
	rt->rt_metric = metric;
	rt->rt_state |= RTS_CHANGED;
	if (newgateway)
		TRACE_ACTION("CHANGE TO   ", rt);
#ifndef RTM_ADD
	if (add && install)
		if (ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0)
			perror(MSGSTR(ADDRTE, "SIOCADDRT")); /*MSG*/
	if (delete && install)
		if (ioctl(s, SIOCDELRT, (char *)&oldroute) < 0)
			perror(MSGSTR(DELRTE, "SIOCDELRT")); /*MSG*/
#else
        if (delete && install)
                if (ioctl(s, SIOCDELRT, (char *)&oldroute) < 0)
                        perror("SIOCDELRT");
        if (add && install) {
                if (ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0)
                        perror("SIOCADDRT");
        }
#endif
}

rtdelete(rt)
	struct rt_entry *rt;
{

	TRACE_ACTION("DELETE", rt);
	FIXLEN(&(rt->rt_router));
        FIXLEN(&(rt->rt_dst));
	if (rt->rt_metric < HOPCNT_INFINITY) {
	    if ((rt->rt_state & (RTS_INTERFACE|RTS_INTERNAL)) == RTS_INTERFACE)
		syslog(LOG_ERR, MSGSTR(DELIF, "deleting route to interface %s (timed out)"), /*MSG*/
		    rt->rt_ifp->int_name);
	    if (install &&
		(rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
		ioctl(s, SIOCDELRT, (char *)&rt->rt_rt))
		    perror(MSGSTR(DELRTE, "SIOCDELRT")); /*MSG*/
	}
	remque(rt);
	free((char *)rt);
}

void
rtdeleteall(sig)
	int sig;
{
	register struct rthash *rh;
	register struct rt_entry *rt;
	struct rthash *base = hosthash;
	int doinghost = 1;

again:
	for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++) {
		rt = rh->rt_forw;
		for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
			if (rt->rt_state & RTS_INTERFACE ||
			    rt->rt_metric >= HOPCNT_INFINITY)
				continue;
			TRACE_ACTION("DELETE", rt);
			if ((rt->rt_state & (RTS_INTERNAL|RTS_EXTERNAL)) == 0 &&
			    ioctl(s, SIOCDELRT, (char *)&rt->rt_rt))
		    		perror(MSGSTR(DELRTE, "SIOCDELRT")); /*MSG*/
		}
	}
	if (doinghost) {
		doinghost = 0;
		base = nethash;
		goto again;
	}
	exit(sig);
}

/*
 * If we have an interface to the wide, wide world,
 * add an entry for an Internet default route (wildcard) to the internal
 * tables and advertise it.  This route is not added to the kernel routes,
 * but this entry prevents us from listening to other people's defaults
 * and installing them in the kernel here.
 */
rtdefault()
{
	extern struct sockaddr inet_default;

	rtadd(&inet_default, &inet_default, 1,
		RTS_CHANGED | RTS_PASSIVE | RTS_INTERNAL);
}

rtinit()
{
	register struct rthash *rh;

	for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
	for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
}
