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
 *	@(#)$RCSfile: saio.h,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/19 22:15:16 $
 */ 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */

/* 
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * OSF/1 Release 1.0
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *

 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>

/*
 * Header file for standalone package
 */

/*
 * Io block: includes an
 * inode, cells for the use of seek, etc,
 * and a buffer.
 */
struct	iob {
	int	i_flgs;		/* see F_ below */
	struct	inode i_ino;	/* inode, if file */
	int	i_unit;		/* pseudo device unit */
	daddr_t	i_boff;		/* block offset on device */
	daddr_t	i_cyloff;	/* cylinder offset on device */
	off_t	i_offset;	/* seek offset in file */
	daddr_t	i_bn;		/* 1st block # of next read */
	char	*i_ma;		/* memory address of i/o buffer */
	int	i_cc;		/* character count of transfer */
	int	i_error;	/* error # return */
	int	i_errcnt;	/* error count for driver retries */
	int	i_errblk;	/* block # in error for error reporting */
	char	*i_buf;		/* i/o buffer */
	struct fs *i_fs;	/* file system super block info */
};
#define NULL 0

#define F_READ		0x1	/* file opened for reading */
#define F_WRITE		0x2	/* file opened for writing */
#define F_ALLOC		0x4	/* buffer allocated */
#define F_FILE		0x8	/* file instead of device */
#define F_NBSF		0x10	/* no bad sector forwarding */
#define F_SSI		0x40	/* set skip sector inhibit */
/* io types */
#define F_RDDATA	0x0100	/* read data */
#define F_WRDATA	0x0200	/* write data */
#define F_HDR		0x0400	/* include header on next i/o */
#define F_CHECK		0x0800	/* perform check of data read/write */
#define F_HCHECK	0x1000	/* perform check of header and data */

#define F_TYPEMASK	0xff00

/*
 * Device switch.
 */
struct devsw {
	char	*dv_name;
	int	(*dv_strategy)();
	int	(*dv_open)();
	int	(*dv_close)();
	int	(*dv_ioctl)();
};

struct devsw devsw[];

/*
 * Drive description table.
 * Returned from SAIODEVDATA call.
 */
struct st {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/surfaces/heads */
	short	nspc;		/* # sectors/cylinder */
	short	ncyl;		/* # cylinders */
	short	*off;		/* partition offset table (cylinders) */
};

/*
 * Request codes. Must be the same a F_XXX above
 */
#define READ	1
#define WRITE	2

#define NBUFS	4

char	*b[NBUFS];
daddr_t	blknos[NBUFS];

#define NFILES	4
struct	iob iob[NFILES];

extern	int errno;	/* just like unix */

/* error codes */
#define EBADF	1	/* bad file descriptor */
#define EOFFSET	2	/* relative seek not supported */
#define EDEV	3	/* improper device specification on open */
#define ENXIO	4	/* unknown device specified */
#define EUNIT	5	/* improper unit specification */
#define ESRCH	6	/* directory search for file failed */
#define EIO	7	/* generic error */
#define ECMD	10	/* undefined driver command */
#define EBSE	11	/* bad sector error */
#define EWCK	12	/* write check error */
#define EECC	13	/* uncorrectable ecc error */
#define EHER	14	/* hard error */

/* ioctl's -- for disks just now */
#define SAIOHDR		(('d'<<8)|1)	/* next i/o includes header */
#define SAIOCHECK	(('d'<<8)|2)	/* next i/o checks data */
#define SAIOHCHECK	(('d'<<8)|3)	/* next i/o checks header & data */
#define SAIONOBAD	(('d'<<8)|4)	/* inhibit bad sector forwarding */
#define SAIODOBAD	(('d'<<8)|5)	/* enable bad sector forwarding */
#define SAIOECCLIM	(('d'<<8)|6)	/* set limit to ecc correction, bits */
#define SAIORETRIES	(('d'<<8)|7)	/* set retry count for unit */
#define SAIODEVDATA	(('d'<<8)|8)	/* get device data */
#define SAIOSSI		(('d'<<8)|9)	/* set skip sector inhibit */
#define SAIONOSSI	(('d'<<8)|10)	/* inhibit skip sector handling */
#define SAIOSSDEV	(('d'<<8)|11)	/* is device skip sector type? */
#define SAIODEBUG	(('d'<<8)|12)	/* enable/disable debugging */
#define SAIOGBADINFO	(('d'<<8)|13)	/* get bad-sector table */

#define	BIOS_DEV_FLOPPY	0x0
#define	BIOS_DEV_WIN	0x80

#define	DEV_HD		0x00
#define	DEV_FLOPPY	0x01
