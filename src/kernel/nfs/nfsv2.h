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
 *	@(#)$RCSfile: nfsv2.h,v $ $Revision: 4.3.5.2 $ (DEC) $Date: 1993/06/29 18:53:26 $
 */ 
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
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *

 */

#ifndef _NFSV2_H_
#define _NFSV2_H_
/*
 * nfs definitions as per the version 2 specs
 */

/*
 * Constants as defined in the Sun NFS Version 2 spec.
 * "NFS: Network File System Protocol Specification" RFC1094
 */

#define NFS_PORT	2049
#define	NFS_PROG	100003
#define NFS_VER2	2
#define	NFS_MAXDATA	8192
#define	NFS_MAXPATHLEN	1024
#define	NFS_MAXNAMLEN	255
#define	NFS_FHSIZE	32
#define	NFS_NPROCS	18
#define NFS_FABLKSIZE	512

/* Stat numbers for rpc returns */
#define	NFS_OK		0
#define	NFSERR_PERM	1
#define	NFSERR_NOENT	2
#define	NFSERR_IO	5
#define	NFSERR_NXIO	6
#define	NFSERR_ACCES	13
#define	NFSERR_EXIST	17
#define	NFSERR_NODEV	19
#define	NFSERR_NOTDIR	20
#define	NFSERR_ISDIR	21
#define	NFSERR_FBIG	27
#define	NFSERR_NOSPC	28
#define	NFSERR_ROFS	30
#define	NFSERR_NAMETOOLONG	63
#define	NFSERR_NOTEMPTY	66
#define	NFSERR_DQUOT	69
#define	NFSERR_STALE	70
#define	NFSERR_WFLUSH	99

/* Sizes in bytes of various nfs rpc components */
#define	NFSX_FH		32
#define	NFSX_UNSIGNED	4
#define	NFSX_FATTR	68
#define	NFSX_SATTR	32
#define	NFSX_COOKIE	4
#define NFSX_STATFS	20

/* nfs rpc procedure numbers */
#define	NFSPROC_NULL		0
#define	NFSPROC_GETATTR		1
#define	NFSPROC_SETATTR		2
#define	NFSPROC_ROOT		3		/* Obsolete */
#define	NFSPROC_LOOKUP		4
#define	NFSPROC_READLINK	5
#define	NFSPROC_READ		6
#define	NFSPROC_WRITECACHE	7		/* Obsolete */
#define	NFSPROC_WRITE		8
#define	NFSPROC_CREATE		9
#define	NFSPROC_REMOVE		10
#define	NFSPROC_RENAME		11
#define	NFSPROC_LINK		12
#define	NFSPROC_SYMLINK		13
#define	NFSPROC_MKDIR		14
#define	NFSPROC_RMDIR		15
#define	NFSPROC_READDIR		16
#define	NFSPROC_STATFS		17

/* Conversion macros */
#define	vtonfs_mode(t,m) txdr_unsigned(MAKEIMODE(t,m))
#define	nfstov_mode(a)	(fxdr_unsigned(u_short, (a))&07777)
#define	vtonfs_type(a)	txdr_unsigned(nfs_type[((int)(a))])
#define	nfstov_type(a)	v_type[fxdr_unsigned(u_int, (a))]

/* File types */
typedef enum { NFNON=0, NFREG=1, NFDIR=2, NFBLK=3, NFCHR=4, NFLNK=5,
/* Note: the following are not part of the protocol spec, but NFFIFO
   is used by at least one vendor (HP) */
		NFSOCK=6, NFBAD=7, NFFIFO=8
} nfstype;

/* Structs for common parts of the rpc's */
struct nfsv2_time {
	u_int	tv_sec;
	u_int	tv_usec;
};

struct nfsv2_fattr {
	u_int	fa_type;
	u_int	fa_mode;
	u_int	fa_nlink;
	u_int	fa_uid;
	u_int	fa_gid;
	u_int	fa_size;
	u_int	fa_blocksize;
	u_int	fa_rdev;
	u_int	fa_blocks;
	u_int	fa_fsid;
	u_int	fa_fileid;
	struct nfsv2_time fa_atime;
	struct nfsv2_time fa_mtime;
	struct nfsv2_time fa_ctime;
};

struct nfsv2_sattr {
	u_int	sa_mode;
	u_int	sa_uid;
	u_int	sa_gid;
	u_int	sa_size;
	struct nfsv2_time sa_atime;
	struct nfsv2_time sa_mtime;
};

struct nfsv2_statfs {
	u_int	sf_tsize;
	u_int	sf_bsize;
	u_int	sf_blocks;
	u_int	sf_bfree;
	u_int	sf_bavail;
};

#endif
