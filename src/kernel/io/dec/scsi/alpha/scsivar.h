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
#ifndef	SCSIVAR_INCLUDE
#define	SCSIVAR_INCLUDE	1

/************************************************************************
 * scsivar.h	02/27/89
 *
 * Modification History
 *
 * 15-Oct-91	Farrell Woods
 *	Put Alpha and Alpha ADU support back in (ADU support will go
 *	away though)
 *
 * 05-Jun-91	Tom Tierney
 *	Temporarily added partition table definitions.  This was removed
 *	from OSF/1's fudge.scsi.h and is added here temporarily until it
 *	is either defined by new file system code or dropped once support
 *	for old-style ULTRIX partitioning is dropped completely (currently
 *	for OSF/1 ULTRIX partitioned disks will be read-only).
 *
 * 29-Jan-91	Robin Miller
 *   o	Moved pseudo SCSI commands to group 3 range (0x60-0x7f) to
 *	avoid conflict with CD-ROM audio commands.
 *   o	Added CD-ROM audio commands.
 *
 * 20-Nov-90	Robin Miller
 *   o	Added support for 10-byte READ/WRITE CDB's.
 *   o	Added SCSI Group definitions for use by sz_cdb_length() funtion.
 *   o	Removed all CD-ROM audio commands (conflict with psuedo commands).
 *
 * 16-Aug-90	Robin Miller
 *	Added commands to support audio portion of CD-ROM's.
 *
 * 17-Jul-90    Mitch McConnell
 *	Added error states for scsi state machine to handle SZ_IP
 *	return code from port_start (for 3max/rdat problems).
 *
 * 06-Jun-90	Bill Dallas
 *	Got rid of some of the mbz's for device option table
 *	support. - 
 *
 * 23-Oct-89    Janet L. Schank
 *      Added the define DEFAULT_SCSIID which is used as the system's
 *      default scsi id.
 *
 * 16-Jul-89	Fred Canter
 *	Add sz_rzmodsns_dt to datfmt structure. So mode sense can
 *	be used to get disk geometry.
 *
 * 15-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 * 13-Jul-89	Fred Canter
 *	Remove pf bit from mode sense command structure. Adding it
 *	was an error.
 *
 * 06/27/89	John A. Gallant
 *	Added read/write long structure definitions
 *
 * 24-May-89	Fred Canter
 *	Added PF bit to mode select structure (for new rzdisk).
 *
 * 02/27/89	John A. Gallant
 *	Added the pseuso commands for load/unload and start/stop unit.
 *
 * 11/09/88	John A. Gallant
 *	Started the merge with the V3.0 source level.  Due to time constraints
 *	only the changes for the new IOCTL support will be merged.  Others
 *	changes will hopefully occur as time permits.
 *   COMMENTS from V3.0:
 *   03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA, and VERIFY DATA.
 *	Changed values of the pseudo opcodes because they conflicted
 *	with READ DEFECT DATA.
 *
 * 25-Aug-88   Ricky Palmer
 *      Ifdef'ed again for vax and mips. This time it is based on
 *      my August 1, 1988 ifdef's of the original scsi.c "glob" driver.
 *
 * 16-Aug-88	Fred Canter
 *	Merge PVAX and FIREFOX SCSI drivers.
 *
 * 08-Aug-88	Fred Canter
 *	Add SZ_BUSY status code.
 *
 * 18-Jul-88	Fred Canter
 *	Added extended message handling.
 *
 * 28-Jun-88	Fred Canter
 *	Add new error code for improved ??command() error handling.
 *
 * 18-Jun-88	Fred Canter
 *	Changed max request sense data length from 11 to 20 to
 *	allow receiving all available sense data.
 *
 * 06-Jun-88	Fred Canter
 *	Created this header file for scsi.c from stcvar.h.
 *
 ************************************************************************/

#ifndef PT_MAGIC
/* 
 * OSF/1 NOTE: (Temporary hackery)
 * ULTRIX style partitions will be read-only with OSF.  These are normally
 * defined in /fs/ufs/fs.h.  When the file system code is added to OSF/1,
 * this information should probably be put back in fs.h for compatibility
 * purposes.  This was moved here to get rid of fudge.scsi.h.
 */
#define PT_MAGIC        0x032957        /* Partition magic number */
#define PT_VALID        1               /* Indicates if struct is valid */
  /*
 * Structure that is used to determine the partitioning of the disk.
 * It's location is at the end of the superblock area.
 * The reason for both the cylinder offset and block offset
 * is that some of the disk drivers (most notably the uda
 * driver) require the block offset rather than the cyl.
 * offset.
 */
struct pt {
        int    pt_magic;       /* magic no. indicating part. info exits */
        int     pt_valid;       /* set by driver if pt is current */
        struct  pt_info {
                int     pi_nblocks;     /* no. of sectors for the partition */
               daddr_t pi_blkoff;      /* block offset for start of part. */
        } pt_part[8];
};
/* End Temporary Hackery */
#endif

#ifndef __ERRLOG__

/*
 * Some commands are common to disks and tapes, some are not.
 * Disk unique commands begin with "d_", tape commands with "t_".
 * Common commands have no "_" before the name.
 */
struct sz_cmdfmt {
    u_char opcode;			/* SCSI comand opcode		      */
    /*
     * Overlaid portion of commands
     */
    union {
	/* 
	 * TEST UNIT READY Command
	 * READ BLOCK LIMITS Command
	 */
	struct sz_tur_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:4;		/* Flag				      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}tur;
	/*
	 * REWIND Command
	 */
	struct sz_rewind_cm {
	    u_char	immed:1;	/* Immediate			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}rwd;
	/*
	 * REQUEST SENSE Command
	 * MODE SENSE Command
	 * INQUIRY Command
	 */
	struct sz_sense_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	pgcode:6;	/* Page code			      */
	    u_char	pcf:2;		/* PCF 			              */
	    u_char		:8;	/* Reserved			      */
	    u_char	alclen;		/* Allocation Length		      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}sense;
	/*
	 * TAPE specific:
	 * 	READ Command
	 * 	WRITE Command
	 * 	RECOVER BUFFERED DATA Command
	 */
	struct sz_trw_cm {
	    u_char	fixed:1;	/* Fixed			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	xferlen2;	/* Transfer Length (MSB)	      */
	    u_char	xferlen1;	/* Transfer Length		      */
	    u_char	xferlen0;	/* Transfer Length (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}t_rw;
	/*
	 * DISK specific:
	 * 	READ Command
	 * 	WRITE Command
	 * 	RECOVER BUFFERED DATA Command
	 *	READ DEFECT DATA Command
	 *	FORMAT UNIT Command
	 *	REASSIGN BLOCK Command
	 *	VERIFY DATA Command
	 */
	struct sz_drw_cm {
	    u_char	lbaddr2:5;	/* Logical Block Address (MSB)	      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	lbaddr1;	/* Logical Block Address	      */
	    u_char	lbaddr0;	/* Logical Block Address (LSB)	      */
	    u_char	xferlen;	/* Transfer Length (# of LBNs)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero (TODO1: VU)	      */
	    u_char	pad[16];	/* Pad				      */
	}d_rw;

	/*
	 * Disk Read / Write 10-byte CDB.
	 */
	struct sz_drw10_cm {
	    u_char reladr	: 1;	/* Relative Address bit		[1] */
	    u_char		: 4;	/* Reserved			    */
	    u_char lun		: 3;	/* Logical Unit Number		    */
	    u_char lbaddr3;		/* Logical Block Address	[2] */
	    u_char lbaddr2;		/* Logical Block Address	[3] */
	    u_char lbaddr1;		/* Logical Block Address	[4] */
	    u_char lbaddr0;		/* Logical Block Address	[5] */
	    u_char		: 8;	/* Reserved			[6] */
	    u_char xferlen1;		/* Transfer Length    		[7] */
	    u_char xferlen0;		/* Transfer Length    		[8] */
	    u_char link		: 1;	/* Link				[9] */
	    u_char flag		: 1;	/* Flag				    */
	    u_char		: 4;	/* Reserved			    */
	    u_char mbz		: 2;	/* Must be Zero			    */
	    u_char pad[12];		/* Pad				    */
	} d_rw10;

	struct sz_drdd_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	dlf:3;		/* Defect List Format  		      */
	    u_char	g:1;		/* Grown Defect List Bit	      */
	    u_char	m:1;		/* Manufacturers Defect List Bit      */
	    u_char		:3;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	alclen1;	/* Allocation Length  		      */
	    u_char	alclen0;	/* Allocation Length  		      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[12];	/* Pad				      */
	}d_rdd;
	struct sz_dfu_cm {
	    u_char	dlf:3;		/* Defect List Format  		      */
	    u_char	cmplst:1;	/* Complete List Bit		      */
	    u_char	fmtdat:1;	/* Format Data Bit		      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	pattern;	/* Format Data Pattern 		      */
	    u_char	interleave1;	/* Interleave Factor 		      */
	    u_char	interleave0;	/* Interleave Factor 		      */
	    u_char	control;	/* Control Byte  		      */
	    u_char	pad[16];	/* Pad				      */
	}d_fu;
	struct sz_drb_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	control;	/* Control Byte  		      */
	    u_char	pad[16];	/* Pad				      */
	}d_rb;
	struct sz_dvd_cm {
	    u_char	reladr:1;	/* Relative Address bit		      */
	    u_char	bytchk:1;	/* Byte Check			      */
	    u_char		:3;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	lbaddr3;	/* Logical Block Address	      */
	    u_char	lbaddr2;	/* Logical Block Address	      */
	    u_char	lbaddr1;	/* Logical Block Address	      */
	    u_char	lbaddr0;	/* Logical Block Address	      */
	    u_char		:8;	/* Reserved			      */
	    u_char	verflen1;	/* Verification Length		      */
	    u_char	verflen0;	/* Verification Length		      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[12];	/* Pad				      */
	}d_vd;

	/*
	 * WRITE FILEMARKS Command
	 */
	struct sz_wfm_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	numoffmks2;	/* Number of Filemarks (MSB)	      */
	    u_char	numoffmks1;	/* Number of Filemarks		      */
	    u_char	numoffmks0;	/* Number of Filemarks (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}wfm;
	/*
	 * SPACE Command
	 */
	struct sz_space_cm {
	    u_char	code:2;		/* Fixed			      */
	    u_char		:3;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	count2;		/* Count (MSB)			      */
	    u_char	count1;		/* Count			      */
	    u_char	count0;		/* Count (LSB)			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}space;
	/*
	 * VERIFY Command
	 */
	struct sz_vfy_cm {
	    u_char	fixed:1;	/* Fixed			      */
	    u_char	bytcmp:1;	/* Byte Compare			      */
	    u_char		:3;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	verflen2;	/* Verification Length (MSB)	      */
	    u_char	verflen1;	/* Verification Length		      */
	    u_char	verflen0;	/* Verification Length (LSB)	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}vfy;
	/*
	 * ERASE Command
	 */
	struct sz_era_cm {
	    u_char	longbit:1;	/* Long				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}erase;
	/*
	 * LOAD/UNLOAD Command
	 */
	struct sz_ld_cm {
	    u_char	immed:1;	/* Immediate			      */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	load:1;		/* Load				      */
	    u_char	reten:1;	/* Retention			      */
	    u_char		:6;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}ld;
	/*
	 * SEND DIAGNOSTIC Command
	 */
	struct sz_snd_diag_cm {
	    u_char control	: 5;	/* Diagnostic control bits.	[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char param_len1;		/* Allocation length (MSB).	[3] */
	    u_char param_len0;		/* Allocation length (LSB).	[4] */
	    u_char link		: 1;	/* Link.			[5] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[16];		/* Pad.				    */
	} diag;
	/*
	 * RECEIVE DIAGNOSTIC RESULT Command
	 */
	struct sz_recdiag_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char alloc_len1;		/* Allocation length (MSB).	[3] */
	    u_char alloc_len0;		/* Allocation length (LSB).	[4] */
	    u_char link		: 1;	/* Link.			[5] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[16];		/* Pad.				    */
	} recdiag;
	/*
	 * MODE SELECT Command
	 */
	struct sz_modsel_cm {
	    u_char	sp:1;		/* Save Parameters (DISK only)	      */
	    u_char	mbz0:3;		/* Reserved (all)		      */
	    u_char	pf:1;		/* Page Format			      */
	    u_char	lun:3;		/* Logical Unit Number (all)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	pll;		/* Parameter List Length	      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	/* Parameter List Header	*/
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	speed:4;	/* was mbz now speed 		      */
	    u_char	bufmode:3;	/* Buffered Mode		      */
	    u_char		:1;	/* Reserved			      */
	    u_char	rdeclen;	/* Record Descriptor Length	      */
	/* Parameter List Block Descriptor */
	    u_char	density;	/* Was nbz2 now its the density code  */
	    u_char	numofrec2;	/* Number of Records (MSB)	      */
	    u_char	numofrec1;	/* Number of Records 		      */
	    u_char	numofrec0;	/* Number of Records (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char	reclen2;	/* Record Length (MSB)		      */
	    u_char	reclen1;	/* Record Length 		      */
	    u_char	reclen0;	/* Record Length (LSB)		      */
	    u_char	vulen;		/* Vendor Unique Length		      */
	    u_char	nof:3;		/* Enable Fillers		      */
	    u_char	mbz3:3;		/* Must be Zero			      */
	    u_char	notimo:1;	/* Disable Reselect timeouts	      */
	    u_char	vu7:1;		/* Direct Track Access		      */
	    u_char	pad[2];		/* Pad				      */
	}modsel;
	/*
	 * Track SELECT Command
	 */
	struct sz_trksel_cm {
	    u_char		:5;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	trkval;		/* Track Value			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}trksel;
	/*
	 * RESERVE UNIT Command
	 * RELEASE UNIT Command
	 */
	 struct sz_runit_cm {
	    u_char		:1;	/* Reserved			      */
	    u_char	thrdpdev:3;	/* Third Party Device		      */
	    u_char	thrdp:1;	/* Third Party			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz:2;		/* Must be Zero			      */
	    u_char	pad[16];	/* Pad				      */
	}runit;
	/*
	 * PREVENT/ALLOW MEDIUM REMOVAL Command
	 */
	 struct sz_medremoval_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char prevent	: 1;	/* Prevent = 1, Allow = 0.	[4] */
	    u_char		: 7;	/* Reserved.			    */
	    u_char link		: 1;	/* Link.			[5] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz		: 2;	/* Must be Zero.		    */
	    u_char pad[16];		/* Pad.				    */
	} mr;

	/*
	 * START/STOP UNIT Command
	 */
	 struct sz_ssu_cm {
	    u_char immed	: 1;	/* Immediate 1 = True		[1] */
	    u_char		: 4;	/* Reserved.			    */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char start	: 1;	/* Start = 1, Stop = 0.		[4] */
	    u_char loej		: 1;	/* Load/Eject = 1, 0 = No Affect.   */
	    u_char		: 6;	/* Reserved.			    */
	    u_char link		: 1;	/* Link.			[5] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz		: 2;	/* Must be Zero.		    */
	    u_char pad[16];		/* Pad.				    */
	} ssu;

	/*
	 * DISK specific:
	 *	READ CAPACITY Command
	 *	Read/Write Long
	 */
	 struct sz_rdcap_cm {
	    u_char	reladr:1;	/* RelAdr (TODO1: spec don't say!)   */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	lbaddr3;	/* Logical Block Address (MSB)	      */
	    u_char	lbaddr2;	/* Logical Block Address	      */
	    u_char	lbaddr1;	/* Logical Block Address	      */
	    u_char	lbaddr0;	/* Logical Block Address (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	pmi:1;		/* Partial Media Indicator	      */
	    u_char		:5;	/* Reserved			      */
	    u_char	mbz1:2;		/* Vendor Unique		      */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz2:2;		/* Vendor Unique		      */
	    u_char	pad[12];	/* Pad				      */
	}rdcap;
	 struct sz_rwl_cm {
	    u_char	reladr:1;	/* RelAdr (NOTE: spec dose not say!)  */
	    u_char		:4;	/* Reserved			      */
	    u_char	lun:3;		/* Logical Unit Number		      */
	    u_char	lbaddr3:7;	/* Logical Block Address (MSB)	      */
	    u_char	phad:1;		/* Physical Address Flag              */
	    u_char	lbaddr2;	/* Logical Block Address	      */
	    u_char	lbaddr1;	/* Logical Block Address	      */
	    u_char	lbaddr0;	/* Logical Block Address (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	dspec:8; 	/* Drive specific byte 55:0 22/23:1   */
	    u_char	link:1;		/* Link				      */
	    u_char	flag:1;		/* Flag				      */
	    u_char		:4;	/* Reserved			      */
	    u_char	mbz2:2;		/* Vendor Unique		      */
	    u_char	pad[12];	/* Pad				      */
	}rwl;

	/*
	 * CD-ROM: Play Audio MSF Command.
	 */
	 struct sz_play_audio_msf_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char starting_M_unit;	/* Starting M-unit.		[3] */
	    u_char starting_S_unit;	/* Starting S-unit.		[4] */
	    u_char starting_F_unit;	/* Starting F-unit.		[5] */
	    u_char ending_M_unit;	/* Ending M-unit.		[6] */
	    u_char ending_S_unit;	/* Ending S-unit.		[7] */
	    u_char ending_F_unit;	/* Ending F-unit.		[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} msf;

	/*
	 * CD-ROM: Play Audio Track/Index Command.
	 */
	 struct sz_play_audio_ti_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char starting_track;	/* Starting Track.		[4] */
	    u_char starting_index;	/* Starting Index.		[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char ending_track;	/* Ending Track.		[7] */
	    u_char ending_index;	/* Ending Index			[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} ti;

	/*
	 * CD-ROM: Play Audio Track Relative Command.
	 */
	 struct sz_play_audio_tr_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char lbaddr3;		/* Logical Block Address	[2] */
	    u_char lbaddr2;		/* Logical Block Address.	[3] */
	    u_char lbaddr1;		/* Logical Block Address.	[4] */
	    u_char lbaddr0;		/* Logical Block Address.	[5] */
	    u_char starting_track;	/* Starting Track.		[6] */
	    u_char xfer_len1;		/* Transfer Length    		[7] */
	    u_char xfer_len0;		/* Transfer Length    		[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} tr;

	/*
	 * CD-ROM: Pause/Resume Command.
	 */
	 struct sz_pause_resume_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char 		: 8;	/* Reserved.			[4] */
	    u_char 		: 8;	/* Reserved.			[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char 		: 8;	/* Reserved.			[7] */
	    u_char resume	: 1;	/* Resume = 1, Pause = 0.	[8] */
	    u_char 		: 7;	/* Reserved.			    */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} pr;

	/*
	 * CD-ROM: Read TOC Command.
	 */
	 struct sz_read_toc_cm {
	    u_char		: 1;	/* Reserved.			[1] */
	    u_char msf		: 1;	/* Report address in MSF format.    */
	    u_char		: 3;	/* Reserved.			    */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char 		: 8;	/* Reserved.			[4] */
	    u_char 		: 8;	/* Reserved.			[5] */
	    u_char starting_track;	/* Reserved.			[6] */
	    u_char alloc_len1;		/* Allocation length (MSB).	[7] */
	    u_char alloc_len0;		/* Allocation length (LSB).	[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} toc;

	/*
	 * CD-ROM: Read Sub-Channel Command.
	 */
	 struct sz_read_subc_cm {
	    u_char		: 1;	/* Reserved.			[1] */
	    u_char msf		: 1;	/* Report address in MSF format.    */
	    u_char		: 3;	/* Reserved.			    */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 6;	/* Reserved.			[2] */
	    u_char subQ		: 1;	/* Sub-Q Channel Data.		    */
	    u_char		: 1;	/* Reserved.			    */
	    u_char data_format;		/* Reserved.			[3] */
	    u_char 		: 8;	/* Reserved.			[4] */
	    u_char 		: 8;	/* Reserved.			[5] */
	    u_char track_number;	/* Reserved.			[6] */
	    u_char alloc_len1;		/* Allocation length (MSB).	[7] */
	    u_char alloc_len0;		/* Allocation length (LSB).	[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} sch;

	/*
	 * CD-ROM: Read Header Command/
	 */
	struct sz_read_header_cm {
	    u_char		: 1;	/* Reserved.			[1] */
	    u_char msf		: 1;	/* Report address in MSF format.    */
	    u_char		: 3;	/* Reserved.			    */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char lbaddr3;		/* Logical Block Address	[2] */
	    u_char lbaddr2;		/* Logical Block Address.	[3] */
	    u_char lbaddr1;		/* Logical Block Address.	[4] */
	    u_char lbaddr0;		/* Logical Block Address.	[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char alloc_len1;		/* Allocation Length MSB.	[7] */
	    u_char alloc_len0;		/* Allocation Length LSB.	[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz		: 2;	/* Must be Zero.		    */
	    u_char pad[12];		/* Pad.				    */
	} rh;

	/*
	 * CD-ROM: Play Track Command.
	 */
	 struct sz_play_track_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char starting_track;	/* Starting track.		[4] */
	    u_char starting_index;	/* Starting index.		[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char		: 8;	/* Reserved.			[7] */
	    u_char number_indexes;	/* Number of indexes.		[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} pt;

	/*
	 * CD-ROM: Playback Control/Status Command.
	 */
	 struct sz_playback_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char 		: 8;	/* Reserved.			[4] */
	    u_char 		: 8;	/* Reserved.			[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char alloc_len1;		/* Allocation length (MSB).	[7] */
	    u_char alloc_len0;		/* Allocation length (LSB).	[8] */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} pb;

	/*
	 * CD-ROM: Set Address Format Command.
	 */
	 struct sz_set_address_format_cm {
	    u_char		: 5;	/* Reserved.			[1] */
	    u_char lun		: 3;	/* Logical Unit Number.		    */
	    u_char		: 8;	/* Reserved.			[2] */
	    u_char		: 8;	/* Reserved.			[3] */
	    u_char 		: 8;	/* Reserved.			[4] */
	    u_char 		: 8;	/* Reserved.			[5] */
	    u_char		: 8;	/* Reserved.			[6] */
	    u_char		: 8;	/* Reserved.			[7] */
	    u_char lbamsf	: 1;	/* Address Format 0/1 = LBA/MSF	[8] */
	    u_char		: 7;	/* Reserved.			    */
	    u_char link		: 1;	/* Link.			[9] */
	    u_char flag		: 1;	/* Flag.			    */
	    u_char		: 4;	/* Reserved.			    */
	    u_char mbz2		: 2;	/* Vendor Unique.		    */
	    u_char pad[12];		/* Pad.				    */
	} saf;

    }cmd;
};

#endif /* __ERRLOG__ */

/*
 * Data Format Structure
 */
struct sz_datfmt {
    union {
#ifdef __alpha
        /*
         * For alpha it is required that the size of this structure be
         * a multiple of 32 bytes in length.  This is because the buffer will be
         * used as a sink for a DATAI phase (since the TVBus interface can only
         * write in 32-byte blocks).  Since the size of this structure was 44
         * bytes the following pad field is used to insure that the actual
         * size of the data structure will be at least 64 bytes.
         */
        u_char  sz_alpha_pad[64];
#endif /* alpha */
	/*
	 * Extended Sense Data Format
	 */
	struct sz_exsns_dt {
	    u_char	errcode:4;	/* Error Code			      */
	    u_char	errclass:3;	/* Error Class			      */
	    u_char	valid:1;	/* Valid			      */
	    u_char	segnum;		/* Segment Number		      */
	    u_char	snskey:4;	/* Sense Key			      */
	    u_char		:1;	/* Reserved			      */
	    u_char	ili:1;		/* Illegal Length Indicator	      */
	    u_char	eom:1;		/* End of Medium		      */
	    u_char	filmrk:1;	/* Filemark			      */
	    u_char	infobyte3;	/* Information Byte (MSB)	      */
	    u_char	infobyte2;	/* Information Byte 		      */
	    u_char	infobyte1;	/* Information Byte 		      */
	    u_char	infobyte0;	/* Information Byte (LSB)	      */
	    u_char	asl;		/* Additional Sense Length	      */
	    union {			/* Additional sense bytes 	      */
		struct {		/* TAPE specific bytes		      */
		    u_char ctlr;	/* Controller internal error code     */
		    u_char drv0;	/* Drive error byte		      */
		    u_char drv1;	/* Drive error byte		      */
		    u_char pad[9];	/* Pad				      */
		} tz_asb;
		struct {		/* DISK specific bytes		      */
		    u_char rb1[4];	/* Reserved bytes		      */
		    u_char asc;		/* Additional Sense Code	      */
		    u_char rb2[5];	/* Reserved bytes		      */
		    u_char pad[2];	/* Pad				      */
		} rz_asb;
		struct {		/* CD-ROM specific bytes		     */
		    u_char rb1[4];	/* Reserved bytes		      */
		    u_char asc;		/* Additional Sense Code	      */
		    u_char rb2;		/* Reserved byte		      */
		    u_char frufld;	/* Field Replacable Unit failed	      */
		    u_char bitp:3;	/* Bit Pointer			      */
		    u_char bpv:1;	/* Bit Pointer Valid		      */
		    u_char vu:2;	/* Vendor Unique		      */
		    u_char cd:1;	/* Command / Data		      */
		    u_char fpv:1;	/* Field Pointer Valid		      */
		    u_char fpmsb;	/* Field Pointer (MSB)		      */
		    u_char fplsb;	/* Field Pointer (LSB)		      */
		    u_char pad[2];	/* Pad				      */
		} cd_asb;
	    } asb;
	    u_char	pad1[24];
	}exsns;
	/*
	 * READ BLOCK LIMITS Data
	 */
	struct sz_rd_blklim_dt {
	    u_char		:8;	/* Reserved			      */
	    u_char	maxreclen2;	/* Maximum Record Length (MSB)	      */
	    u_char	maxreclen1;	/* Maximum Record Length 	      */
	    u_char	maxreclen0;	/* Maximum Record Length (LSB)	      */
	    u_char	minreclen1;	/* Minimum Record Length (MSB)	      */
	    u_char	minreclen0;	/* Minimum Record Length (LSB)	      */
	    u_char	pad[14];	/* Pad				      */
	    u_char	pad1[24];
	}rdblim;
	/*
	 * DISK specific:
	 *	READ CAPACITY Data
	 */
	struct sz_rdcap_dt {
	    u_char	lbaddr3;	/* Logical block Address (MSB)	      */
	    u_char	lbaddr2;	/* Logical block Address	      */
	    u_char	lbaddr1;	/* Logical block Address	      */
	    u_char	lbaddr0;	/* Logical block Address (LSB)	      */
	    u_char	blklen3;	/* Block Length (MSB)		      */
	    u_char	blklen2;	/* Block Length			      */
	    u_char	blklen1;	/* Block Length			      */
	    u_char	blklen0;	/* Block Length (LSB)		      */
	    u_char	pad[12];	/* Pad				      */
	    u_char	pad1[24];
	}rdcap;
	/*
	 * INQUIRY Data
	 */
	struct sz_inq_dt {
	    u_char	perfdt;		/* Peripheral Deice Type	      */
	    u_char	devtq:7;	/* Device Type Qualifier	      */
	    u_char	rmb:1;		/* Removable Media Bit		      */
	    u_char	version;	/* Version			      */
	    u_char		:8;	/* Reserved			      */
	    u_char	addlen;		/* Additional Length		      */
	    u_char	rsvd[3];	/* Reserved			      */
	    u_char	vndrid[8];	/* Vendor ID (ASCII)		      */
	    u_char	prodid[16];	/* Product ID (ASCII)		      */
	    u_char	revlvl[4];	/* Revision level (ASCII)	      */
	    u_char	revdata[8];	/* Revision data (ASCII)	      */
	}inq;
	/*
	 * MODE SENSE Data
	 */
	struct sz_modsns_dt {
	/* Parameter List Header	*/
	    u_char	sdlen;		/* Sense Data Length		      */
	    u_char	mbz1:8;		/* Must be Zero			      */
	    u_char	speed:4;	/* was mbz2 now speed 		      */
	    u_char	bufmode:3;	/* Buffered Mode		      */
	    u_char	wp:1;		/* Write Protect		      */
	    u_char	bdeclen;	/* Block Descriptor Length	      */
	/* Parameter List Block Descriptor */
	    u_char	density;	/* WAS mbz3 now density(tapes)        */
	    u_char	numofblk2;	/* Number of Blocks (MSB)	      */
	    u_char	numofblk1;	/* Number of Blocks 		      */
	    u_char	numofblk0;	/* Number of Blocks (LSB)	      */
	    u_char		:8;	/* Reserved			      */
	    u_char	blklen2;	/* Block Length (MSB)		      */
	    u_char	blklen1;	/* Block Length 		      */
	    u_char	blklen0;	/* Block Length (LSB)		      */
	    u_char	vulen;		/* Vendor Unique Length		      */
	    u_char	nof:2;		/* Enable Fillers		      */
	    u_char	mbz4:5;		/* Must be Zero			      */
	    u_char	vu7:1;		/* Direct Track Access		      */
	    u_char	pad[6];		/* Pad				      */
	    u_char	pad1[24];
	}modsns;
	/*
	 * MODE SENSE Data for DISKS
	 */
	struct sz_rzmodsns_dt {
	/* Parameter List Header	*/
	    u_char	sdlen;		/* Sense Data Length		      */
	    u_char	medtyp;		/* Medium Type			      */
	    u_char		: 7;	/* Reserved			      */
	    u_char	wp	: 1;	/* Write Protect		      */
	    u_char	bdlen;		/* Block Descriptor Length	      */
	/* Parameter List Block Descriptor */
	    u_char	dencode;	/* Density Code			      */
	    u_char	nblks2;		/* Number of Blocks (MSB)	      */
	    u_char	nblks1;		/* Number of Blocks 		      */
	    u_char	nblks0;		/* Number of Blocks (LSB)	      */
	    u_char		: 8;	/* Reserved			      */
	    u_char	blklen2;	/* Block Length (MSB)		      */
	    u_char	blklen1;	/* Block Length 		      */
	    u_char	blklen0;	/* Block Length (LSB)		      */
	/* Parameter List Page Data (32 bytes, one page only) */
	    u_char	mspage[32];	/* Page header and data		      */
	}rzmodsns;
	/*
	 * RECEIVE DIAGNOSTIC RESULT Data
	 */
	struct sz_recdiag_dt {
	    u_char	ctlr_hd_rev;	/* Controller hardware revision level */
	    u_char	ctlr_fw_rev;	/* Controller firmware revision level */
	    u_char	ctlr_selftest;	/* Controller selftest  0 = Passed    */
					/*                      1 = Failed    */
	    u_char	drv_hw_rev;	/* Drive hardware revision level      */
	    u_char	drv_fw_rev;	/* Drive firmware revision level      */
	    u_char	drv_selftest;	/* Drive selftest  0 = Passed	      */
					/*		  xx = Failure Code   */
	/* TODO1: why no pad? */
	}recdiag;
    }dat;
};

#ifndef __ERRLOG__

/*
 * Define Masks for SCSI Group Codes.
 */
#define	SCSI_GROUP_0		0x00	/* SCSI Group Code 0.		*/
#define SCSI_GROUP_1		0x20	/* SCSI Group Code 1.		*/
#define SCSI_GROUP_2		0x40	/* SCSI Group Code 2.		*/
#define SCSI_GROUP_3		0x60	/* SCSI Group Code 3.		*/
#define SCSI_GROUP_4		0x80	/* SCSI Group Code 4.		*/
#define SCSI_GROUP_5		0xA0	/* SCSI Group Code 5.		*/
#define SCSI_GROUP_6		0xC0	/* SCSI Group Code 6.		*/
#define SCSI_GROUP_7		0xE0	/* SCSI Group Code 7.		*/
#define SCSI_GROUP_MASK		0xE0	/* SCSI Group Code mask.	*/

/*
 * SCSI Command Opcodes:
 */
#define	SZ_TUR		0x00		/* TEST UNIT READY Command	      */
#define SZ_REWIND	0x01		/* REWIND Command		      */
#define SZ_RQSNS	0x03		/* REQUEST SENSE Command	      */
#define SZ_RBL		0x05		/* READ BLOCK LIMITS Command	      */
#define	SZ_READ		0x08		/* READ Command			      */
#define SZ_WRITE	0x0a		/* WRITE Command		      */
#define SZ_TRKSEL	0x0b		/* TRACK SELECT Comand		      */
#define SZ_RESUNIT	0x16		/* RESERVE UNIT Command		      */
#define SZ_WFM		0x10		/* WRITE FILEMARKS Command	      */
#define SZ_SPACE	0x11		/* SPACE Command		      */
#define SZ_INQ		0x12		/* INQUIRY Command		      */
#define SZ_VFY		0x13		/* VERIFY Command		      */
#define SZ_RBD		0x14		/* RECOVER BUFFERED DATA Command      */
#define SZ_MODSEL	0x15		/* MODE SELECT Command		      */
#define SZ_RELUNIT	0x17		/* RELEASE UNIT Command		      */
#define SZ_ERASE	0x19		/* ERASE Command		      */
#define SZ_MODSNS	0x1a		/* MODE SENSE Command		      */
#define	SZ_SSLU		0x1b		/* DISK: START/STOP UNIT Command      */
#define SZ_RECDIAG	0x1c		/* RECEIVE DIAGNOSTIC RESULT Command  */
#define SZ_SNDDIAG	0x1d		/* SEND DIAGNOSTIC Command	      */
#define SZ_MEDREMOVAL	0x1e		/* PREVENT/ALLOW MEDIUM REMOVAL Cmd     */
#define	SZ_RDCAP	0x25		/* DISK: READ CAPACITY		      */
#define SZ_FORMAT	0x04		/* DISK: FORMAT UNIT Command	      */
#define SZ_REASSIGN	0x07		/* DISK: REASSIGN BLOCK Command	      */
#define SZ_READ_10	0x28		/* DISK: READ 10-byte Command.	      */
#define SZ_WRITE_10	0x2A		/* DISK: WRITE 10-byte Command.	      */
#define SZ_SEEK_10	0x2B		/* DISK: SEEK 10-byte Command.	      */
#define SZ_VFY_DATA	0x2f		/* DISK: VERIFY DATA Command	      */
#define SZ_RDD		0x37		/* DISK: READ DEFECT DATA Command     */
#define SZ_READL	0x3E		/* DISK: READ LONG Command            */
#define SZ_WRITEL	0x3F		/* DISK: WRITE LONG Command           */
#define RZSPECIAL	0x8000		/* DISK: set for special disk commands*/

/*
 * SCSI Commands for CD-ROM.
 */
/* Group 1 Commands (Diagnostic) */
#define SZ_WRITE_BUFFER		0x3B	/* CDROM: WRITE BUFFER Command.	   */
#define SZ_READ_BUFFER		0x3C	/* CDROM: READ BUFFER Command.	   */

/* Group 2 Commands */
#define SZ_CHANGE_DEFINITION	0x40	/* CDROM: CHANGE DEFINITION Cmd.    */
#define SZ_READ_SUBCHAN		0x42	/* CDROM: READ SUB-CHANNEL Cmd.	    */
#define SZ_READ_TOC		0x43	/* CDROM: READ TOC Command.	    */
#define SZ_READ_HEADER		0x44	/* CDROM: READ HEADER Command.	    */
#define SZ_PLAY_AUDIO		0x45	/* CDROM: PLAY AUDIO Command.	    */
#define SZ_PLAY_AUDIO_MSF	0x47	/* CDROM: PLAY AUDIO MSF Command.   */
#define SZ_PLAY_AUDIO_TI	0x48	/* CDROM: PLAY AUDIO TRACK/INDEX.   */
#define SZ_PLAY_TRACK_REL	0x49	/* CDROM: PLAY TRACK RELATIVE Cmd.  */
#define SZ_PAUSE_RESUME		0x4B	/* CDROM: PAUSE/RESUME Command.	    */

/* Group 5 Commands */
#define SZ_PLAY_AUDIO_12	0xA5	/* CDROM: PLAY AUDIO Cmd (12 byte). */
#define SZ_PLAY_TRACK_REL_12	0xA6	/* CDROM: PLAY TRACK RELATIVE Cmd.  */

/* Group 6 Commands (Vendor Unique) */
#define SZ_SET_ADDRESS_FORMAT	0xC0	/* CDROM: SET ADDRESS FORMAT Cmd.   */
#define SZ_PLAYBACK_STATUS	0xC4	/* CDROM: PLAYBACK STATUS Command.  */
#define SZ_PLAY_TRACK		0xC6	/* CDROM: PLAY TRACK Command.	    */
#define SZ_PLAY_MSF		0xC7	/* CDROM: PLAY MSF Command.	    */
#define SZ_PLAY_VAUDIO		0xC8	/* CDROM: PLAY AUDIO Command.	    */
#define SZ_PLAYBACK_CONTROL	0xC9	/* CDROM: PLAYBACK CONTROL Command. */

/*
 * Group 3 (normally reserved) - Pseudo SCSI Commands.
 */
#define SZ_P_FSPACER	0x60		/* Psuedo opcode for space record     */
#define SZ_P_FSPACEF	0x61		/* Psuedo opcode for space file	      */
#define SZ_P_BSPACER	0x62		/* Psuedo opcode for backspace record */
#define SZ_P_BSPACEF	0x63		/* Psuedo opcode for backspace file   */
#define SZ_P_CACHE	0x64		/* Psuedo opcode for buffered mode    */
#define SZ_P_NOCACHE	0x65		/* Psuedo opcode for no buffered mode */
#define SZ_P_LOAD	0x66		/* Pseudo opcode for load (not used)  */
#define SZ_P_UNLOAD	0x67		/* Pseudo opcode for unload           */
#define SZ_P_SSUNIT	0x68		/* Pseudo opcode for start/stop unit  */
#define SZ_P_RETENSION	0x69		/* Psuedo opcode for retension	      */
#define SZ_P_EJECT	0x6a		/* Psuedo opcode for eject caddy.   */

/*
 * SCSI Command Data structure names shortened
 */
#define szdt_exsns	sz_datfmt.dat.exsns	/* EXTENDED SENSE Data	      */
#define szdt_rbl	sz_datfmt.dat.rbl	/* READ BLOCK LIMITS Data     */
#define	szdt_rdcap	sz_datfmt.dat.rdcap	/* DISK: READ CAPACITY Data   */
#define	szdt_inq	sz_datfmt.dat.inq	/* INQUIRY Data		      */
#define szdt_modsel	sz_datfmt.dat.modsel	/* MODE SELECT Data	      */
#define szdt_modsns	sz_datfmt.dat.modsns	/* MODE SENSE Data	      */
/*
 * SCSI STATUS Byte
 */
#define	SZ_GOOD		0x00	/* Good					      */
#define	SZ_CHKCND	0x02	/* Check Condition			      */
#define	SZ_BUSY		0x08	/* Device cannot accept a command (busy)      */
#define SZ_INTRM	0x10	/* Intermediate				      */
#define SZ_RESCNF	0x18	/* Reservation Conflict			      */
/*
 * Fake SCSI STATUS Byte.
 */
#define	SZ_BAD		0xff	/* Fatal error command (couldn't do RQSNS)    */
/*
 * SCSI Message Protocols
 */
#define	SZ_CMDCPT	0x00	/* Command Complete			      */
#define	SZ_EXTMSG	0x01	/* Extended message			      */
#define SZ_SDP		0x02	/* Save Data Pointers			      */
#define SZ_RDP		0x03	/* Restore Data Pointers (DISK: new)	      */
#define SZ_DISCON	0x04	/* Disconnect				      */
#define SZ_ABT		0x06	/* Abort				      */
#define SZ_MSGREJ	0x07	/* Message Reject			      */
#define SZ_NOP		0x08	/* No Operation				      */
#define SZ_MSGPE	0x09	/* Message Parity Error			      */
#define SZ_DEVRST	0x0c	/* Bus Device Reset			      */
#define SZ_LNKCMP	0x0a	/* Linked Command Complete		      */
#define SZ_LNKCMPF	0x0b	/* Linked Command Complete with Flag	      */
#define SZ_ID_NODIS	0x80	/* IDENTIFY wo/ disconnect capability,	      */
#define SZ_ID_DIS	0xc0	/* IDENTIFY w/ disconnect capability,	      */
/*
 * SCSI Extended Message Codes
 */
#define	SZ_SYNC_XFER	0x01	/* Synchronous Data Transfer Request          */
/*
 * Command Data allocation constants
 */
#define	SZ_INQ_MAXLEN	44	/* number of bytes for INQUIRY data (MAX)     */
#define	SZ_INQ_MINLEN	 5	/* number of bytes for INQUIRY data (MIN)     */
				/* next 3 must match sz_inq_dt struct below   */
#define	SZ_VID_LEN	 8	/* INQUIRY data vendor ID field length	      */
#define	SZ_PID_LEN	16	/* INQUIRY data product ID field length       */
#define	SZ_REV_LEN	 4	/* INQUIRY data revision level field length   */

#define SZ_RECDIAG_LEN	 6	/* number of bytes for RECEIVE DIAG data      */
#define	SZ_RDCAP_LEN	 8	/* number of bytes for READ CAPACITY data     */
#define SZ_RQSNS_LEN	20	/* number of bytes for REQUEST SENSE data     */
#define SZ_MODSNS_LEN	14	/* number of bytes for MODE SENSE data	      */
#define SZ_MODSEL_LEN	14	/* number of bytes for MODE SELECT data	      */
				/* TODO1: DATAO: subtracts 2 for disk/cdrom */
#define SZ_MAX_DT_LEN	44	/* Size of sz_datfmt structure		      */

/*
 * Command Packet Length constants
 */
#define SZ_MAX_CMD_LEN		22	/* maximum command length in bytes    */
#define SZ_MODSEL_CMD_LEN	20	/* maximum length of modsel command   */
#define	SZ_RDCAP_CMD_LEN	10	/* length of DISK read capacity cmd   */
#define SZ_CMD_LEN		 6	/* length of commands in bytes	      */

/*
 * Define Maximum Logical Block Address (LBA) for a 6-byte CDB.
 */
#define SZ_MAX_LBA		0x1fffff /* Maximum 6-byte CDB LBA.	*/

/*
 * Pause/Resume Audio Playback Defines
 */
#define SZ_AUDIO_PAUSE		0	/* Pause audio playback.	*/
#define SZ_AUDIO_RESUME		1	/* Resume audio playback.	*/

/*
 * Allow/Prevent Medium Removal Defines
 */
#define SZ_ALLOW_REMOVAL	0	/* Allow medium removal.	*/
#define SZ_PREVENT_REMOVAL	1	/* Prevent medium removal.	*/

/*
 * Start/Stop Unit Defines
 */
#define SZ_STOP_UNIT		0	/* Stop the unit.		*/
#define SZ_START_UNIT		1	/* Start the unit.		*/

/*
 * Miscellaneous Constants
 */
#define	SZ_DEFAULT_FILLS	7	/* Default number of fills that can   */
					/* be written to keep tape streaming. */
#ifdef ALPHAFLAMINGO
#define DEFAULT_SCSIID          6       /* Default system scsi id             */
#else
#define DEFAULT_SCSIID          7       /* Default system scsi id             */
#endif

/*
 * MACRO for converting a longword to a byte
 *	args - the longword
 *	     - which byte in the longword you want
 */
#define	LTOB(a,b)	((a>>(b*8))&0xff)

/* MACROS for timing out spin loops.
 *
 *	desc:	Waits while expression is true.
 *
 *	args:	expr 		- expression to spin on
 *		spincount 	- amount of time to wait in microseconds
 *		retval		- return value of this macro
 */
#define	SZWAIT_WHILE(expr,spincount,retval) {				\
		for (retval = 0; ((retval < 100) && (expr)); retval++);	\
		while ((expr) && ( retval < spincount)) {		\
			DELAY(100);					\
			retval++;					\
		}							\
	}

/*
 * 	desc: Waits unitl expression is true.
 */
#define SZWAIT_UNTIL(expr,spincount,retval) {				\
		SZWAIT_WHILE((!expr),spincount,retval);			\
	}

/* 
 * macro for converting the density selection in the
 * minor number out of the buf struct into a tape option
 * struct index.
*/
#define DENS_IDX(bp)	((((minor(bp->b_dev))&DENS_MASK)>>3))

/*
 * Request Sense Sense Key Codes.
 * Sense keys without comments are unsupported
 * becasue DEC devices don't use them.
 * TODO: comments are tape specific should be general?
 */
#define SZ_NOSENSE	0x00	/* Successful cmd or EOT, ILI, FILEMARK	      */
#define SZ_RECOVERR	0x01	/* Successful cmd with controller recovery    */
#define SZ_NOTREADY	0x02	/* Device present but not ready		      */
#define	SZ_MEDIUMERR	0x03	/* Cmd terminated with media flaw	      */
#define SZ_HARDWARE	0x04	/* Controller or drive hardware error	      */
#define SZ_ILLEGALREQ	0x05	/* Illegal command or command parameters      */
#define SZ_UNITATTEN	0x06	/* Unit Attention			      */
#define SZ_DATAPROTECT	0x07	/* Write attempted to write protected media   */
#define SZ_BLANKCHK	0x08	/* Read zero length record (EOD)	      */
#define	SZ_VNDRUNIQUE	0x09
#define	SZ_COPYABORTD	0x0a
#define SZ_ABORTEDCMD	0x0b	/* Cmd aborted that may retry successfully    */
#define	SZ_EQUAL	0x0c
#define SZ_VOLUMEOVFL	0x0d	/* Buffer data left over after EOM	      */
#define SZ_MISCOMPARE	0x0e	/* Miscompare on Verify command		      */
#define	SZ_RESERVED	0x0f

/* Additional sence code values for extended sence data. */

#define	SZ_ASC_RRETRY	0x17
#define	SZ_ASC_RERROR	0x18

/*
 * Top Level State Machine
 */
/* Status/Positioning States */
#define	SZ_NEXT		 0	/* Process next request from queue	      */
#define SZ_SP_START	 1	/* Start a Status or Positioning Operation    */
#define SZ_SP_CONT	 2	/* Continue SZ_SP_SETUP:		      */
/* Read/Write (DMA) States */
#define	SZ_RW_START	 3	/* Setup a Read or Write Operation	      */
#define	SZ_RW_CONT	 4	/* Start/Continue a Read or Write Operation   */
#define SZ_R_STDMA	 5	/* Start DMA READ			      */
#define	SZ_W_STDMA	 6	/* Start DMA WRITE			      */
#define SZ_R_DMA	 7	/* Read DMA processing			      */
#define SZ_W_DMA	 8	/* Write DMA processing			      */
#define SZ_R_COPY	 9	/* Copy READ data to memory		      */
#define SZ_ERR		10	/* DMA Error				      */
#define SZ_SP_START_ERR	11	/* Handle RQSNS SZ_IP rtncode for SP_START    */
#define SZ_SP_CONT_ERR	12	/* Handle RQSNS SZ_IP rtncode for SP_CONT     */
#define SZ_RDMA_ERR	13	/* Handle RQSNS SZ_IP rtncode for RDMA	      */
#define SZ_WDMA_ERR	14	/* Handle RQSNS SZ_IP rtncode for WDMA	      */

/*
 * Bottom Level State Machine
 */
#define SZ_SP_ARB	 0	/* Arbitrate for the Bus		      */
#define SZ_SP_SEL	 1	/* Select the target device		      */
#define SZ_CMD_PHA	 2	/* Command Phase			      */
#define SZ_DATAO_PHA	 3	/* Data Out Phase			      */
#define SZ_DATAI_PHA	 4	/* Data In Phase			      */
#define SZ_STATUS_PHA	 5	/* Status Phase				      */
#define SZ_MESSI_PHA	 6	/* Message In Phase			      */
#define SZ_MESSO_PHA	 7	/* Message Out Phase			      */
#define SZ_RELBUS	 8	/* Release the Bus			      */

/*
 * Current Bus Status Register  (SCS_CUR_STAT)
 */
#define SCS_RST         0x80    /* reset condition on SCSI bus                */
#define SCS_BSY         0x40    /* busy -- SCSI bus being used                */
#define SCS_REQ         0x20    /* request portion of REQ/ACK handshake       */
#define SCS_MESSI       0x1c    /* Message In Phase                           */
#define SCS_MESSO       0x18    /* Message Out Phase                          */
#define SCS_STATUS      0x0c    /* Status Phase                               */
#define SCS_CMD         0x08    /* Command Phase                              */
#define SCS_DATAI       0x04    /* Data In Phase                              */
#define SCS_IO          0x04    /* I/O                                        */
#define SCS_SEL         0x02    /* select a target or reselect an initiator   */
#define SCS_DBP         0x01    /* data parity                                */
#define SCS_DATAO       0x00    /* Data Out Phase                             */
#define SCS_PHA_MSK     0x1c    /* Mask out the Phase Bits                    */


/*
 * define for fixed block tape units scsi.
 * Since there are no record boundaries we 
 * now must remember if a tape mark is pending
*/
#define TPMARK_PENDING	0X80000000

#endif /* __ERRLOG__ */

#endif /*  SCSIVAR_INCLUDE  */

