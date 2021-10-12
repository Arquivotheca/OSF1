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
static char	*sccsid = "@(#)$RCSfile: pass1.c,v $ $Revision: 4.3.4.3 $ (DEC) $Date: 1992/07/28 17:02:50 $";
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
/*
 * Copyright (c) 1980, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint

#endif /* not lint */

#include <sys/param.h>
#include <ufs/dinode.h>
#include <ufs/fs.h>
#include <stdlib.h>
#include <string.h>
#include "fsck.h"

static daddr_t badblk;
static daddr_t dupblk;
int pass1check();
struct dinode *getnextinode();

pass1()
{
	register int c, i, j;
	register struct dinode *dp;
	struct zlncnt *zlnp;
	int ndb, cgd;
	struct inodesc idesc;
	ino_t inumber;

#ifdef DEBUG
	printf ("Starting Pass1\n");
#endif
	/*
	 * Set file system reserved blocks in used block map.
	 */
	for (c = 0; c < sblock.fs_ncg; c++) {
		cgd = cgdmin(&sblock, c);
		if (c == 0) {
			i = cgbase(&sblock, c);
			cgd += howmany(sblock.fs_cssize, sblock.fs_fsize);
		} else
			i = cgsblock(&sblock, c);
		for (; i < cgd; i++)
			setbmap(i);
	}
	/*
	 * Find all allocated blocks.
	 */
	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	inumber = 0;
	n_files = n_blks = 0;
	all_dups = 1;
	resetinodebuf();
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			if (inumber < ROOTINO)
				continue;
			dp = getnextinode(inumber);
			if ((dp->di_mode & IFMT) == 0) {
				if (bcmp((char *)dp->di_db, (char *)zino.di_db,
					NDADDR * sizeof(daddr_t)) ||
				    bcmp((char *)dp->di_ib, (char *)zino.di_ib,
					NIADDR * sizeof(daddr_t)) ||
				    dp->di_mode || dp->di_size) {
					pfatal("PARTIALLY ALLOCATED INODE I=%lu",
						inumber);
					if (reply("CLEAR") == 1) {
						dp = ginode(inumber);
						clearinode(dp);
						inodirty();
					}
				}
				statemap[inumber] = USTATE;
				continue;
			}
			lastino = inumber;
			if (/* dp->di_size < 0 || */
			    dp->di_size + sblock.fs_bsize - 1 < 0) {
				if (debug)
					printf("bad size %lu:", 
						(uint_t)dp->di_size);
				goto unknown;
			}
			if (!preen && (dp->di_mode & IFMT) == IFMT &&
			    reply("HOLD BAD BLOCK") == 1) {
				dp = ginode(inumber);
				dp->di_size = (off_t)sblock.fs_fsize;
				dp->di_mode = IFREG|0600;
				dp->di_flags = 0;
				inodirty();
			}
			ndb = howmany(dp->di_size, sblock.fs_bsize);
			if (ndb < 0) {
				if (debug)
					printf("bad size %lu ndb %d:",
						(uint_t)dp->di_size, ndb);
				goto unknown;
			}
			if ((dp->di_mode & IFMT) == IFBLK ||
			    (dp->di_mode & IFMT) == IFCHR ||
			    (dp->di_mode & IFMT) == IFSOCK ||
			    (dp->di_mode & IFMT) == IFIFO) {
				ndb++;
				if (dp->di_size != 0) {
					pwarn("DEVICE HAS NON ZERO SIZE I=%u",
					      inumber);
					if (preen)
						printf(" (CORRECTED)\n");
					else if (reply("CORRECT") == 0)
						break;
					dp = ginode(inumber);
					dp->di_size = 0;
					inodirty();
					if ((dp->di_mode & IFMT) & (IFSOCK|IFIFO))
						ndb = 0;
					else
						ndb = 1;
				}
			}
			if (((dp->di_mode & IFMT) != IFLNK) &&
			    (dp->di_flags & IC_FASTLINK)) {
				pwarn("UNKNOWN INODE FLAG I=%u", inumber);
				if (preen)
					printf(" (CORRECTED)\n");
				else if (reply("CORRECT") == 0)
					errexit("");
				dp = ginode(inumber);
				dp->di_flags = 0;
				inodirty();
			}
			/*
			 * Skip disk block checks for fast symbolic links
			 */
			if ((dp->di_flags & IC_FASTLINK) == 0) {
				for (j = ndb; j < NDADDR; j++)
					if (dp->di_db[j] != 0) {
					    if (debug)
						printf("bad direct addr: %ld\n",
							dp->di_db[j]);
					    goto unknown;
					}
				for (j = 0, ndb -= NDADDR; ndb > 0; j++)
					ndb /= NINDIR(&sblock);
				for (; j < NIADDR; j++)
					if (dp->di_ib[j] != 0) {
					    if (debug)
					       printf("bad indirect addr: %ld\n",
						       dp->di_ib[j]);
					    goto unknown;
					}
			} else if (dp->di_size > MAX_FASTLINK_SIZE) {
				if (debug)
					printf("inode %d (fast link) bad size\n",
					       inumber);
				goto unknown;
			}
			if (ftypeok(dp) == 0)
				goto unknown;
			n_files++;
			lncntp[inumber] = dp->di_nlink;
			if (dp->di_nlink <= 0) {
				zlnp = (struct zlncnt *)malloc(sizeof *zlnp);
				if (zlnp == NULL) {
					pfatal("LINK COUNT TABLE OVERFLOW");
					if (reply("CONTINUE") == 0)
						errexit("");
				} else {
					zlnp->zlncnt = inumber;
					zlnp->next = zlnhead;
					zlnhead = zlnp;
				}
			}
			if ((dp->di_mode & IFMT) == IFDIR) {
				if (dp->di_size == 0)
					statemap[inumber] = DCLEAR;
				else
					statemap[inumber] = DSTATE;
				cacheino(dp, inumber);
			} else
				statemap[inumber] = FSTATE;
			badblk = dupblk = 0;
			idesc.id_number = inumber;
			(void)ckinode(dp, &idesc);
			idesc.id_entryno *= btodb(sblock.fs_fsize);
			if (dp->di_blocks != idesc.id_entryno) {
				pwarn("INCORRECT BLOCK COUNT I=%lu (%ld should be %ld)",
				    inumber, dp->di_blocks, idesc.id_entryno);
				if (preen)
					printf(" (CORRECTED)\n");
				else if (reply("CORRECT") == 0)
					continue;
				dp = ginode(inumber);
				dp->di_blocks = idesc.id_entryno;
				inodirty();
			}
			continue;
	unknown:
			pfatal("UNKNOWN FILE TYPE I=%lu", inumber);
			statemap[inumber] = FCLEAR;
			if (reply("CLEAR") == 1) {
				statemap[inumber] = USTATE;
				dp = ginode(inumber);
				clearinode(dp);
				inodirty();
			}
		}
	}
	freeinodebuf();
}

pass1check(idesc)
	register struct inodesc *idesc;
{
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;
	register struct dups *dlp;
	struct dups *new;

	if ((anyout = chkrange(blkno, idesc->id_numfrags)) != 0) {
		blkerror(idesc->id_number, "BAD", blkno);
		if (++badblk >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%lu",
				idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				errexit("");
			return (STOP);
		}
	}
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (anyout && chkrange(blkno, 1)) {
			res = SKIP;
		} else if (!testbmap(blkno)) {
			n_blks++;
			setbmap(blkno);
		} else {
			blkerror(idesc->id_number, "DUP", blkno);
			if (++dupblk >= MAXDUP) {
				all_dups = 0;
				pwarn("EXCESSIVE DUP BLKS I=%lu",
					idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			new = (struct dups *)malloc(sizeof(struct dups));
			if (new == NULL) {
				all_dups = 0;
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			new->dup = blkno;
			if (muldup == 0) {
				duplist = muldup = new;
				new->next = 0;
			} else {
				new->next = muldup->next;
				muldup->next = new;
			}
			for (dlp = duplist; dlp != muldup; dlp = dlp->next)
				if (dlp->dup == blkno)
					break;
			if (dlp == muldup && dlp->dup != blkno)
				muldup = new;
		}
		/*
		 * count the number of blocks found in id_entryno
		 */
		idesc->id_entryno++;
	}
	return (res);
}
