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
static char *rcsid = "@(#)$RCSfile: frag.c,v $ $Revision: 1.1.3.3 $ (DEC) $Date: 1992/08/14 12:25:07 $";
#endif

/*
 * frag.c
 * Keep a cache of recent "fragment leaders" to allow us
 * to do the right thing with later fragments
 *
 * 28 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 *	Copyright 1989, 1990 Digital Equipment Corporation
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "screentab.h"

extern int debug;

/*
 * Whenever we get the first fragment of a packet, we record its
 * identity and the unpacked value.  Subsequent fragments are
 * matched with cache entries and, if present, the unpacked value
 * is returned.
 *
 * The database is a simple hash table.  Entries are removed when
 * their TTL expires (but we limit the TTL to a brief distance
 * into the future, since we should not be seeing too much delay
 * skew).
 *
 * "Identification" is a triple: (src, dst, id) since a sender
 * is allowed to use the same fragment ID for different dests
 * (they probably don't, but better safe than sorry).
 */

#define	TTL_MAX		5	/* seconds before trashing entry */

struct frag_entry {
	struct in_addr src;	/* IP source */
	struct in_addr dst;	/* IP destination */
	u_short	id;		/* IP datagram ID */
	struct unpacked_hdrs unpacking;	/* cached value */
	int expiration;	        /* time (tv_sec) when this entry is stale */
	struct frag_entry *next;	/* free (or hash) list chain */
};

#define	HASH_SLOTS	(1<<8)
#define	HASH_MASK	(HASH_SLOTS - 1)
#define	HASH_FUNC(id)	(((id)&HASH_MASK)^(((id)/HASH_SLOTS)&HASH_MASK))

struct frag_entry *frag_hashtab[HASH_SLOTS];
			/* ASSUMPTION: initialized to all NULLs */

struct frag_entry *frag_freelist = NULL;

InitFragCache()
{
	register struct frag_entry *fstore;
	register int count;
	
	count = HASH_SLOTS * 2;		/* arbitrary */
	
	fstore = (struct frag_entry *)malloc(sizeof(*fstore) * count);
	if (fstore == NULL) {
	    perror("InitFragCache/malloc");
	    exit(1);
	}
	
	/* Break up the storage and put it on the free list */
	while (count-- > 0) {
	    fstore->next = frag_freelist;
	    frag_freelist = fstore;
	    fstore++;
	}
}

RecordFragLeader(ipp, unpp)
register struct unpacked_hdrs *unpp;
register struct ip *ipp;
{
	register struct frag_entry *fep;
	register int i;
	
	if (frag_freelist == NULL) {
	    PurgeFragCache();	/* see if there is some space */
	    if (frag_freelist == NULL) {
		if (debug) {
		    fprintf(stderr, "all fragment entries busy\n");
		}
		return;		/* still no space, tough luck */
	    }
	}
	fep = frag_freelist;
	frag_freelist = fep->next;
	
	fep->src = ipp->ip_src;
	fep->dst = ipp->ip_dst;
	fep->id = ipp->ip_id;
	fep->unpacking = *unpp;
	
	i = ipp->ip_ttl;
	if (i > TTL_MAX)
		i = TTL_MAX;
	fep->expiration = time(0) + i + 1;	/* add 1 for roundoff error */
	
	i = HASH_FUNC(fep->id);
	/* insert into that hash chain */
	fep->next = frag_hashtab[i];
	frag_hashtab[i] = fep;
	
	if (debug) {
	    fprintf(stderr, "FragLeader (hash %d) id %x [%s",
			i, fep->id, inet_ntoa(fep->src));
	    fprintf(stderr, "->%s] life %d\n",
			inet_ntoa(fep->dst), fep->expiration - time(0));
	}
}


FindFragLeader(ipp, unpp)
register struct unpacked_hdrs *unpp;
register struct ip *ipp;
{
	register int i;
	register struct frag_entry *fep;
	
	i = HASH_FUNC(ipp->ip_id);
	
	if (debug) {
	    fprintf(stderr, "Fragment id %x [%s",
			ipp->ip_id, inet_ntoa(ipp->ip_src));
	    fprintf(stderr, "->%s]\n", inet_ntoa(ipp->ip_dst));
	}

	fep = frag_hashtab[i];
	while (fep) {
	    if (debug) {
		fprintf(stderr, "\tChecking #%d id %x [%s",
			    i, fep->id, inet_ntoa(fep->src));
		fprintf(stderr, "->%s]\n", inet_ntoa(fep->dst));
	    }
	    if ((fep->id == ipp->ip_id)
			&& (fep->src.s_addr == ipp->ip_src.s_addr)
			&& (fep->dst.s_addr == ipp->ip_dst.s_addr)) {
		/* match */
		*unpp = fep->unpacking;
		if (debug)
		    fprintf(stderr, "match\n");
		return(1);
	    }
	    /* no match yet */
	    fep = fep->next;
	}
	/* no match at all */
	return(0);
}

PurgeFragCache()
{
	register struct frag_entry *fep;
	register int now;
	register int i;
	struct frag_entry *PurgeFragEntry();
	
	now = time(0);
	
	if (debug) {
	    fprintf(stderr, "Purging fragment cache:\n");
	}
	
	for (i = 0; i < HASH_SLOTS; i++) {
	    fep = frag_hashtab[i];
	    while (fep) {
		if (debug) {
		    fprintf(stderr, "slot %d id %x life %d\n",
				i, fep->id, fep->expiration - now);
		}
		if (fep->expiration < now) {
		    fep = PurgeFragEntry(i, fep);
		}
		else
		    fep = fep->next;
	    }
	}
}

struct frag_entry *
PurgeFragEntry(slot, fep)
register int slot;
register struct frag_entry *fep;
{
	register struct frag_entry *targp;
	register struct frag_entry *nextp = fep->next;
	
	if (debug) {
	    fprintf(stderr, "purging %x from slot %d\n", fep, slot);
	}

	targp = frag_hashtab[slot];
	
	if (targp == fep) {
	    /* unlink from head of hash chain */
	    frag_hashtab[slot] = nextp;
	    /* link into free list */
	    fep->next = frag_freelist;
	    frag_freelist = fep;
	    return(nextp);
	}

	/* must be past front of chain */
	while (targp) {
	    if (targp->next == fep) {
		/* unlink from hash chain */
		targp->next = nextp;
		/* link into free list */
		fep->next = frag_freelist;
		frag_freelist = fep;
		return(nextp);
	    }
	    targp = targp->next;
	}
	
	/* Didn't find it, but that is "impossible" */
	fprintf(stderr, "PurgeFragEntry: entry not found\n");
	exit(1);
}
