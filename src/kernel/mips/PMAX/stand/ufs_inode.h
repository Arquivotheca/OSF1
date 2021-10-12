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
 *	@(#)$RCSfile: ufs_inode.h,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/19 22:38:57 $
 */ 
/*
 */

/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * OSF/1 Release 1.0
 */


/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
***********************************************************************/

/* ---------------------------------------------------------------------
 * Modification History: /sys/h/inode.h
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 * 
 * 19 Jul 85 -- depp
 *	Removed #ifdef NPIPE.  
 *
 * 4  April 85 -- Larry Cohen
 *	Add GINUSE flag to support open block if in use capability
 *
 * 15 Mar 85 -- funding
 *	Added named pipe support (re. System V named pipes)
 *
 * 23 Oct 84 -- jrs
 *	Add definitions for nami cacheing
 *
 * 17 Jul 84 -- jmcg
 *	Insert code to keep track of lockers and unlockers as a debugging
 *	aid.  Conditionally compiled with option RECINODELOCKS.
 *
 * 17 Jul 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		inode.h	6.1	83/07/29
 *
 * ---------------------------------------------------------------------
 */

#include <sys/secdefines.h>
#if SEC_FSCHANGE
#include <sys/security.h>
#endif

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent inode on volume.
 */

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */


/*
 *	this is the on disk format for an ultrix inode.  this
 *	structure also appears withing the gnode structure
 */


struct	ufs_inode {
	union {
		char	trash[128];
		struct {
			struct gnode_common gn_gc;
			daddr_t	dg_db[NDADDR];	/* 40: disk block addresses */
			daddr_t	dg_ib[NIADDR];	/* 88: indirect blocks */
			long	dg_flags;	/* 100: status, currently unused */
			long	dg_blocks;	/* 104: blocks actually held */
			u_long	dg_gennum; 	/* 108: incarnation of inode */
			long	dg_spare[4];	/* 108: reserved, currently unused */
		} di_gcom;
	} ufs_iu;
};

#define di_ic		ufs_iu.di_gcom
#define	di_mode		di_ic.gn_gc.gc_mode
#define	di_nlink	di_ic.gn_gc.gc_nlink
#define	di_uid		di_ic.gn_gc.gc_uid
#define	di_gid		di_ic.gn_gc.gc_gid
#define di_db		di_ic.dg_db
#define di_ib		di_ic.dg_ib
#define	di_flags	di_ic.dg_flags
#define di_blocks	di_ic.dg_blocks
#define di_gennum	di_ic.dg_gennum
#define di_spare	di_ic.dg_spare
#if defined(vax) || defined(mips)
#define	di_size		di_ic.gn_gc.gc_size.val[0]
#endif
#define	di_atime	di_ic.gn_gc.gc_atime
#define	di_mtime	di_ic.gn_gc.gc_mtime
#define	di_ctime	di_ic.gn_gc.gc_ctime
#define	di_rdev		di_ic.dg_db[0]

#ifndef KERNEL
#define dinode ufs_inode
#endif

#if SEC_FSCHANGE

/*
 * Security extensions to the on-disk inode format:
 */

struct dinode_sec {
	priv_t	di_gpriv[2];	/* granted privilege vector */
	priv_t	di_ppriv[2];	/* potential privilege vector */
	tag_t	di_tag[SEC_TAG_COUNT];	/* security policy tags */
	ino_t	di_parent;	/* inode number of parent of MLD child */
	ushort	di_type_flags;	/* security type flags */
};

/*
 * On-disk inode format for a secure filesystem:
 */

struct sec_dinode {
	struct ufs_inode	di_node;
	struct dinode_sec	di_sec;
};

#endif /* SEC_FSCHANGE */

#define	G_TO_I(x)	((struct ufs_inode *)(x)->g_in.pad)

#define	UFS_GLOCK(gp) { \
	while ((gp)->g_flag & GLOCKED) { \
		(gp)->g_flag |= GWANT; \
		sleep((caddr_t)(gp), PINOD); \
	} \
	(gp)->g_flag |= GLOCKED; \
}

#define	UFS_GUNLOCK(gp) { \
	(gp)->g_flag &= ~GLOCKED; \
	if ((gp)->g_flag&GWANT) { \
		(gp)->g_flag &= ~GWANT; \
		wakeup((caddr_t)(gp)); \
	} \
}

#define	UFS_GUPDAT(gp, t1, t2, waitfor) { \
	if (gp->g_flag&(GUPD|GACC|GCHG|GMOD)) \
		gfs_update(gp, t1, t2, waitfor); \
}

#define	UFS_GTIMES(gp, t1, t2) { \
	if ((gp)->g_flag&(GUPD|GACC|GCHG)) { \
		(gp)->g_flag |= GMOD; \
		if ((gp)->g_flag&GACC) { \
			(gp)->g_atime.tv_sec = (t1)->tv_sec; \
			(gp)->g_atime.tv_usec = (t1)->tv_usec; \
		} \
		if ((gp)->g_flag&GUPD) { \
			(gp)->g_mtime.tv_sec = (t2)->tv_sec; \
			(gp)->g_mtime.tv_usec = (t2)->tv_usec; \
		} \
		if ((gp)->g_flag&GCHG) { \
			(gp)->g_ctime.tv_sec = time.tv_sec; \
			(gp)->g_ctime.tv_usec = time.tv_usec; \
		} \
		(gp)->g_flag &= ~(GACC|GUPD|GCHG); \
	} \
}

struct	gnode	*ufs_gget();
struct	mount	*ufs_mount();
