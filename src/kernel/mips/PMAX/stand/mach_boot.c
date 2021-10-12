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
static char	*sccsid = "@(#)$RCSfile: mach_boot.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/19 22:38:25 $";
#endif 
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
/*
 * ultrixboot.c
 */

#ifndef lint

#endif lint

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
 ************************************************************************/
#include "../../../sys/param.h"
#include "../../../sys/reboot.h"

char	vmunix_name[] = "vmunix";
#define printf _prom_printf
#define gets _prom_gets
#define stop _prom_restart
#define getenv prom_getenv

#define INBUFSZ 256
char    ask_name[INBUFSZ];

#define RB_LOADDS RB_INITNAME			/* defined as 0x10 */

char   *imagename;

/*
 * Functional Discription:
 *	main of `ultrixboot' program ... 
 *
 * Inputs:
 *	none although R10 and R11 are preserved
 *
 * Outputs:
 *	none
 *
 */
int ub_argc;
char **ub_argv;
char **ub_envp;
main (argc,argv,envp)
int argc;
char **argv, **envp;
{
	register howto, devtype;		/* howto=r11, devtype=r10 */
	int	io;
	char	fs;
	char	*boot;
	extern char *version;

#if LABELS
	printf("\nOSF boot - %s\n\n", version);
#else
	printf("\nMach boot - %s\n\n", version);
#endif
	/*
	 * If the amount of memory found by VMB exceeds the amount of
	 * contiguous memory sized for Ultrix by 16 pages, warn the
	 * user, otherwise let it slide.  Rememeber, MVAX II reserves two
	 * pages at the top for the console.  Also, leave some slop for
	 * the way the bit map is counted (by byte rather than by bit).
	 * This warning is intended to find large holes in memory and
	 * and answer the question 'Why is Ultrix only seeing x of the y
	 * Meg of memory?'
	 */
	ub_argc = argc;		/* save prom args for kernel */
	ub_argv = argv;		/* save prom args for kernel */
	ub_envp = envp;		/* save prom args for kernel */
	howto = 0;

	imagename = (char *)getenv("boot");
/*	imagename = (char *)getenv("BOOT");*/

	if (devopen(imagename, 0) < 0) {
		printf("can't open channel to boot driver\n");
		stop();
	}

	io = -1;				/* set to start loop */
	while (io < 0) {
	/* 
	 * If we are loading the supervisor, imagename is
	 * already set.  If we are not, then check for RB_ASKNAME.
	 * If RB_ASKNAME is not set, then assume the default 
	 * 'vmunix' name, otherwise ask for a unix file name to
	 * load.  Entering a diagnostic supervisor name will not
	 * work as it is not a unix image.
	 */
		if (howto & RB_ASKNAME) {
			printf ("Enter %s name: ",
				howto & RB_LOADDS ? "supervisor" : "image");
			gets (ask_name);
			if (ask_name[0] == 0)
				continue;
			imagename = ask_name;
		}

		if ((io = open (imagename, 0)) < 0) {
			printf ("can't open %s\n", imagename);
			howto |= RB_ASKNAME;
			continue;
		}
		printf ("Loading %s ...\n", imagename);
		/* 
	 	 * If we are loading the supervisor, call load_ds which
	 	 * is a special load routine, just for the supervisor.
	 	 * load_image will not load the diagnostic supervisor.
	 	 */
		load_image (io);

		howto |= RB_ASKNAME;
		close (io);
		io = -1;
	}
}
