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
static char	*sccsid = "@(#)$RCSfile: db_update.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1992/08/31 10:20:50 $";
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
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Copyright (c) 1986 Regents of the University of California.
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

#endif  not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

extern struct timeval	tt;
extern FILE *ddt;
extern struct sockaddr_in from_addr;	/* Source addr of last packet */
extern int needs_prime_cache;

int	max_cache_ttl = (7*24*60*60);	/* ONE_WEEK maximum ttl */
int	min_cache_ttl = (5*60);		/* 5 minute minimum ttl */

/*
 * Update data base. Flags control the action.
 * Inverse query tables modified.
 */
db_update(name, odp, newdp, flags, htp)
	char name[];
	struct databuf *odp, *newdp;
	int flags;
	struct hashbuf *htp;
{
	register struct namebuf *np;
	register struct databuf *dp, *pdp;
	char *fname;
        int foundRR = 0;

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"db_update(%s, 0x%lx, 0x%lx, 0%o, 0x%lx)%s\n",
		    name, odp, newdp, flags, htp,
		    (odp && (odp->d_flags&DB_F_HINT)) ? " hint":"" );
#endif
	np = nlookup(name, &htp, &fname, newdp != NULL);
	if (np == NULL || fname != name)
		return (NONAME);

        /* Reflect certain updates in hint cache also... */
	/* Don't stick data we are authoritative for in hints. */
        if (!(flags & DB_NOHINTS) && (odp != NULL) &&
	    (odp->d_zone <= 0) && !(odp->d_flags & DB_F_HINT) &&
            ((name[0] == '\0' && odp->d_type == T_NS) ||
	     (odp->d_type == T_A)))
        {
		register struct databuf *dp;
#ifdef DEBUG
		if (debug >= 3)
			fprintf(ddt,"db_update: hint '%s' %d\n",
				name, odp->d_ttl);
#endif
		dp = savedata(odp->d_class, odp->d_type, odp->d_ttl,
			odp->d_data, odp->d_size);
		dp->d_zone = DB_Z_CACHE;
		dp->d_flags = DB_F_HINT;
		if (db_update(name, dp,dp, (flags|DB_NOHINTS), fcachetab) != OK) {
#ifdef DEBUG		
			if (debug > 2)
				fprintf(ddt, "db_update: hint %lx freed\n", dp);
#endif
			(void) free((char *)dp);
		}
        }

	if (odp != NULL) {
		pdp = NULL;
		for (dp = np->n_data; dp != NULL; ) {
			if (!match(dp, odp->d_class, odp->d_type)) {
				if ((dp->d_type == T_CNAME ||
				    odp->d_type == T_CNAME) &&
				    odp->d_mark == dp->d_mark &&
				    zones[odp->d_zone].z_type != Z_CACHE) {
					syslog(LOG_ERR,
				MSGSTR(ILLDATA, "%s has CNAME and other data (illegal)\n"), /*MSG*/
					    name);
#ifdef DEBUG
					if (debug)
					    fprintf(ddt,
				"db_update: %s: CNAME and more (%d, %d)\n",
						name, odp->d_type, dp->d_type);
#endif
				}
				goto skip;
			}
#ifdef DEBUG
			if (debug >= 5)
				fprintf(ddt,"db_update: flags = %#x, sizes = %d, %d (%d)\n",
				    flags, odp->d_size, dp->d_size,
				    db_cmp(dp, odp));
#endif
			if (flags & DB_NOTAUTH && dp->d_zone) {
#ifdef DEBUG
				if (debug)
					fprintf(ddt,
					"%s attempted update to auth zone %d '%s'\n",
				    inet_ntoa(from_addr.sin_addr),
				    dp->d_zone, zones[dp->d_zone].z_origin);
#endif
				return (AUTH);
			}
			if ((flags & DB_NODATA) && !db_cmp(dp, odp)) {
				/* refresh ttl if cache entry */
				if (dp->d_zone == 0) {
					if (odp->d_zone != 0) { /* XXX */
                                                /* changing cache->auth */
                                                dp->d_zone = odp->d_zone;
                                                dp->d_ttl = odp->d_ttl;
#ifdef DEBUG
                      				if (debug > 3)
							 fprintf(ddt,
                                "db_update: cache entry now in auth zone\n");
#endif
                                                return (DATAEXISTS);
                                        }
					fixttl(odp);
					if (odp->d_ttl > dp->d_ttl)
						dp->d_ttl = odp->d_ttl;
#ifdef DEBUG
				        if (debug >= 3)
				        fprintf(ddt,"db_update: new ttl %d, +%d\n",
				                dp->d_ttl, dp->d_ttl - tt.tv_sec);
#endif
				}
				return (DATAEXISTS);
			}
			/*
			 * If the old databuf has some data, check that the
			 * data matches that in the new databuf (so UPDATED
			 * will delete only the matching RR)
			 */
			if (odp->d_size > 0) {
				if (db_cmp(dp, odp))
					goto skip;
			}
			foundRR = 1;
			if (flags & DB_DELETE)
				dp = rm_datum(dp, np, pdp);
			else {
skip:				pdp = dp;
				dp = dp->d_next;
			}
		}
                if (!foundRR) {
			if (flags & DB_DELETE)
				return(NODATA);
			if (flags & DB_MEXIST)
				return(NODATA);
		}
	}
	if (newdp == NULL)
		return (OK);
	fixttl(newdp);
#ifdef DEBUG
        if (debug >= 3)
	        fprintf(ddt,"db_update: adding%s %lx\n",
			(newdp->d_flags&DB_F_HINT) ? " hint":"", newdp);
#endif
	if (!(newdp->d_flags & DB_F_HINT))
		addinv(np, newdp);	/* modify inverse query tables */

	/* Add to end of list, generally preserving order */
	newdp->d_next = NULL;
	if ((dp = np->n_data) == NULL)  {
		np->n_data = newdp;
		return (OK);
	}
	/* XXX: need to check for duplicate WKS records and flag error */
	while (dp->d_next != NULL) {
		if ((flags & DB_NODATA) && !db_cmp(dp, newdp))
			return (DATAEXISTS);
		dp = dp->d_next;
	}
	if ((flags & DB_NODATA) && !db_cmp(dp, newdp))
		return (DATAEXISTS);
	dp->d_next = newdp;
	return (OK);
}

fixttl(dp)
register struct databuf *dp;
{
	if (dp->d_zone == 0 && !(dp->d_flags & DB_F_HINT)) {
		if (dp->d_ttl <= tt.tv_sec)
			return;
		else if (dp->d_ttl < tt.tv_sec+min_cache_ttl)
			dp->d_ttl = tt.tv_sec+min_cache_ttl;
		else if (dp->d_ttl > tt.tv_sec+max_cache_ttl)
			dp->d_ttl = tt.tv_sec+max_cache_ttl;
	}
	return;
}

struct invbuf *invtab[INVHASHSZ];	/* Inverse query hash table */

/*
 * Add data 'dp' to inverse query tables for name 'np'.
 */
addinv(np, dp)
	struct namebuf *np;
	struct databuf *dp;
{
	register struct invbuf *ip;
	register int hval, i;

	switch (dp->d_type) {
	case T_A:
	case T_UID:
	case T_GID:
		break;

	default:
		return;
	}

	hval = dhash(dp->d_data, dp->d_size);
	for (ip = invtab[hval]; ip != NULL; ip = ip->i_next)
		for (i = 0; i < INVBLKSZ; i++)
			if (ip->i_dname[i] == NULL) {
				ip->i_dname[i] = np;
				return;
			}
	ip = saveinv();
	ip->i_next = invtab[hval];
	invtab[hval] = ip;
	ip->i_dname[0] = np;
}

/*
 * Remove data 'odp' from inverse query table.
 */
rminv(odp)
	struct databuf *odp;
{
	register struct invbuf *ip;
	register struct databuf *dp;
	struct namebuf *np;
	register int i;

	for (ip = invtab[dhash(odp->d_data, odp->d_size)]; ip != NULL;
	    ip = ip->i_next) {
		for (i = 0; i < INVBLKSZ; i++) {
			if ((np = ip->i_dname[i]) == NULL)
				break;
			for (dp = np->n_data; dp != NULL; dp = dp->d_next) {
				if (!match(dp, odp->d_class, odp->d_type))
					continue;
				if (db_cmp(dp, odp))
					continue;
				while (i < INVBLKSZ-1) {
					ip->i_dname[i] = ip->i_dname[i+1];
					i++;
				}
				ip->i_dname[i] = NULL;
				return;
			}
		}
	}
}

/*
 * Compute hash value from data.
 */
dhash(dp, dlen)
	char *dp;
	int dlen;
{
	register char *cp;
	register unsigned hval;
	register int n;

	n = dlen;
	if (n > 8)
		n = 8;
	hval = 0;
	for (cp = dp; --n >= 0; ) {
		hval <<= 1;
		hval += *cp++;
	}
	return (hval % INVHASHSZ);
}

/*
 * Compare type, class and data from databufs for equivalence.
 * Must be case insensitive for some domain names.
 * Return 0 if equivalent, nonzero otherwise.
 */

db_cmp(dp1, dp2)
	register struct databuf *dp1, *dp2;

{
	register char *cp1, *cp2;
	int len;

	if (dp1->d_type != dp2->d_type || dp1->d_class != dp2->d_class)
                return(1);
	if (dp1->d_size != dp2->d_size)
		return(1);
	if (dp1->d_mark != dp2->d_mark)
		return(1);		/* old and new RR's are distinct */
	switch (dp1->d_type) {

	case T_A:
	case T_UID:
	case T_GID:
	case T_WKS:
	case T_NULL:
#ifdef ALLOW_T_UNSPEC
        case T_UNSPEC:
#endif ALLOW_T_UNSPEC
		return(bcmp(dp1->d_data, dp2->d_data, dp1->d_size));

	case T_NS:
	case T_CNAME:
	case T_PTR:
	case T_MB:
	case T_MG:
	case T_MR:
	case T_UINFO:
		return(strcasecmp(dp1->d_data, dp2->d_data));

	case T_HINFO:
		cp1 = dp1->d_data;
		cp2 = dp2->d_data;
		len = *cp1;
		if (strncasecmp(++cp1, ++cp2, len))
			return(1);
		cp1 += len;
		cp2 += len;
		len = *cp1;
		return(strncasecmp(++cp1, ++cp2, len));

	case T_SOA:
	case T_MINFO:
		if (strcasecmp(dp1->d_data, dp2->d_data))
			return(1);
		cp1 = dp1->d_data + strlen(dp1->d_data) + 1;
		cp2 = dp2->d_data + strlen(dp2->d_data) + 1;
		if (dp1->d_type != T_SOA)
			return(strcasecmp(cp1, cp2));
		if (strcasecmp(cp1, cp2))
			return(1);
		cp1 += strlen(cp1) + 1;
		cp2 += strlen(cp2) + 1;
		return(bcmp(cp1, cp2, sizeof(u_int) * 5));
	
	case T_MX:
		cp1 = dp1->d_data;
		cp2 = dp2->d_data;
		if (*cp1++ != *cp2++ || *cp1++ != *cp2++)	/* cmp prio */
			return(1);
		return(strcasecmp(cp1, cp2));

	case T_TXT:
                if (dp1->d_size != dp2->d_size)
                        return(1);
                return(bcmp(dp1->d_data, dp2->d_data, dp1->d_size));

	default:
		return (1);
	}
}
