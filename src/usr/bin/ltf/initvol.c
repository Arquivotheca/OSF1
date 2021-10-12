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
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: initvol.c,v $ $Revision: 4.2.4.3 $ (DEC) $Date: 1993/06/10 22:22:46 $";
#endif
/*
 * HISTORY
 */

/**/
/*
 *
 *	File name:
 *
 *		initvol.c
 *
 *	Source file description:
 *
 *		This file contains the functions pertaining to
 *		the initialization of a new volume for output.
 *
 *	Functions:
 *
 *		initvol()	Top-level logic for volume creation.
 *
 *		tree()		Changes current working directory
 *				(if required) and calls putfile to
 *				place a file on the output volume.
 *
 *	Usage:
 *
 *		n/a
 *
 *	Compile:
 *
 *	    cc -O -c initvol.c		  <- For Ultrix-32/32m
 *
 *	    cc CFLAGS=-DU11-O initvol.c  <- For Ultrix-11
 *
 *
 *	Modification History:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	  01.0		30-Apr-85	Ray Glaser
 *			Create original version
 *
 *	  01.1		19-Sep-85	Suzanne Logcher
 *			Add logic to check the validity of all file
 *			names, either on-line arguments, stdin, or 
 *			a file of file names.  If on-line arguments,
 *			removes unknown files and prints warning
 *			message.  If stdin, checks after input, and
 *			rejects unknown file.  If by file, checks for
 *			unknown files, and if any exist, asks user to
 *			either abort or skip over unknown files.
 *
 *	  01.2		26-Mar-86	Suzanne Logcher
 *			Call statchk for non Ultrix-11, see statchk.c
 *			Change tape routine to use a stat call
 */

/*
 * +--> Local Includes
 */

#include "ltfdefs.h"	/* Common Global Defs % structs */

/**/
/*
 *
 * Function:
 *
 *	initvol
 *
 * Function Description:
 *
 *	This function is the top-level entry routine for the
 *	creation of an entirely new output volume. ie. Any and
 *	all data previously recorded on the volume is lost.
 *	It is entered from the main() function of module "ltf.c".
 *
 * Arguments:
 *
 *	int	num	 	Number of arguments to process
 *	char	*args[]	 	Array of pointers to arguments
 *	int	iflag	 	-I option flag (if arguments are being
 *				being passed from an input file)
 *	char	*inputfile	Name of the possible input file of args.
 *
 *
 * Return values:
 *
 *	none
 *
 * Side Effects:
 *
 *	This function never returns to the caller. Good or bad,
 *	an exit is always taken to the system.
 */

initvol(num, args, iflag, inputfile)
	int	num;		/* number of arguments to process */
	char	*args[];	/* array of pointers to arguments */
	int	iflag;		/* -I option */
	char	*inputfile;
{
/*
 * +--> Local Variables
 */

char	dummy[MAXPATHLEN+1];
int	dumpflag = 0;
struct	FILESTAT *fstat;
FILE	*ifp;
struct	passwd *pw_entry;
char	line[512];	/* line from passwd file */
char	oowner[15];	/* orignal owner name string */
int	ret;
int	ret1;
int	ret2;
FILE	*statchk();
int	stnum;
char	*temp;
struct	stat tinode;
struct	stat tinodes;
int	tnum;
int	ttnum;
int	uid;		/* uid of user */
int     bytes;

/*------*\
   Code
\*------*/

/* Check file list for nonexistent files and print warning
 * then bump file name off list */

if (num > 0) {
    stnum = num;
    for (tnum = stnum, i = 0; tnum > 0; tnum--, i++) {
	ret1 = lstat(args[i], &tinode);
	ret2 = stat(args[i], &tinodes);
	if (ret1 < 0 || ret2 < 0) {
	    if ((tinode.st_mode & S_IFMT) == S_IFLNK && ret2 < 0)
		PERROR "%s: %s %s%c\n", Progname, CANTSTS, args[i], BELL);
	    else	
		PERROR "%s: %s %s%c\n", Progname, CANTSTW, args[i], BELL);
	    perror(Progname);
	    stnum--;
	    tnum--;
	    for (ttnum = tnum, j = i; ttnum > 0; ttnum--, j++) {
		temp = args[j];
		args[j] = args[j+1];
		args[j+1] = temp;
	    }
	    tnum++;
	    i--;
	}
    }/* tnum > 0 */
    if (stnum == 0) {
	PERROR "\n%s: %s %c\n", Progname, NOVALFI, BELL);
	exit(FAIL);
    }
    if (stnum != num)
	printf("\n");
    for (tnum = 0; tnum < stnum; tnum++)
	rec_args(args[tnum], dumpflag);
}/* num > 0 */

/* Check for non-existent files named in input file.  If found, 
 * ask user if they want to quit (to edit input file) or to just
 * skip the unknown file upon creation */

if (iflag == 1) {
    if ((ifp = fopen(inputfile, "r")) == NULL) {
	PERROR "\n%s: %s %s%c\n\n", Progname, CANTOPEN, inputfile, BELL);
	perror(Progname);
	exit(FAIL);
    }
    stnum = 0;
    while ((ret = rec_file(ifp, iflag, dummy)) != EOF) {
	if (ret) {
	    ret1 = lstat(dummy, &tinode);
	    ret2 = stat(dummy, &tinodes);
	    if (ret1 < 0 || ret2 < 0) {
	        if ((tinode.st_mode & S_IFMT) == S_IFLNK && ret2 < 0)
		    PERROR "%s: %s %s%c\n", Progname, CANTSTS, dummy, BELL);
		else	
		    PERROR "%s: %s %s%c\n", Progname, CANTSTW, dummy, BELL);
		perror(Progname);
		PROMPT "%s: %s ", Progname, STOPCRIN); 
		gets(Labelbuf);
		if (Labelbuf[0] == 'y')
		   exit(FAIL); 
		else
		   skip = TRUE;
	    }
	    else {
		rec_args(dummy, dumpflag);
		stnum++;
	    }
	}/*T ret */
    }/*E while ret ..*/
    if (stnum == 0) {
	PERROR "\n%s: %s %c\n", Progname, NOVALFI, BELL);
	exit(FAIL);
    }
    fclose(ifp);
}/*T iflag == 1 */

/* Check for non-existent files as they are inputted by the stdin.
 * If file is unknown, reject it, and ask for another.  All valid
 * file names are stored on a linked list, fstat. */

if (iflag == -1) {
    stnum = 0;
    while ((ret = rec_file(stdin, iflag, dummy)) != EOF) {
	if (ret) {
	    ret1 = lstat(dummy, &tinode);
	    ret2 = stat(dummy, &tinodes);
	    if (ret1 < 0 || ret2 < 0) {
	        if ((tinode.st_mode & S_IFMT) == S_IFLNK && ret2 < 0)
		    PERROR "%s: %s %s%c\n", Progname, CANTSTS, dummy, BELL);
		else	
		    PERROR "%s: %s %s%c\n", Progname, CANTSTW, dummy, BELL);
		perror(Progname);
	    }
	    else {
		rec_args(dummy, dumpflag);
		stnum++;
	    }
	}
    }
    if (stnum == 0) {
	PERROR "\n%s: %s %c\n", Progname, NOVALFI, BELL);
	exit(FAIL);
    }
    printf("\n");
}
/**/
/* Try to open device.  If error, print message and exit */

#ifdef U11
if (!(Magtfp = fopen(Magtdev, "w"))) {
#else
if (!(Magtfp = statchk(Magtdev, "w"))) {
#endif
	PERROR "\n%s: %s %s%c\n", Progname, CANTOPEN, Magtdev, BELL);
	perror(Progname);
	exit(FAIL);
}
/*
 *	As the default owner identification of this volume, get
 *	the user's name from the /etc/passwd file based on 
 *	the current real  uid ..
 */
uid = getuid();

if ((pw_entry=getpwuid(uid))==NULL) {
	PERROR "\n%s: %s %d%c\n", Progname, CANTFPW, uid, BELL);
	strcpy(line, "          ");
}
else {
	strcpy(line,pw_entry->pw_name);
}
/*
 * Owner id field in VOL1 label is maximum of 14 chtrs (38-51)
 */
for (j=0; j < 14 && line[j] && line[j] != ':'; j++)
	Owner[j] = line[j];

/*
 * Ensure owner id field is truncated to 14 (or less) chctrs.
 */
Owner[j] = 0;

/*
 * Ultrix permits non-"A"-character login names.
 * Save a copy of the orginal version for error message
 * reporting and check to see if this is the case.
 */
strcpy(oowner,Owner);

if (!(filter_to_a(Owner,REPORT_ERRORS))) {
	PERROR "\n%s: %s %s %c", Progname, INVOWN, oowner, BELL);
	PERROR "\n%s: %s %s\n\n", Progname, INVVID2, Owner);
	exit(FAIL);
}

#ifdef U11
ret = ghostname(Hostname, 21);
#else
ret = gethostname(Hostname, 21);
#endif

if (ret) {
	PERROR "\n%s: %s\n", Progname, HOSTF);
	perror(Progname);
	exit(FAIL);
}

/******\
 	Output  Volume Label 	-->	VOL1
\******/

/*
 * This area will require an "update" when we add  VERSION 4 logic !
 */
sprintf(Labelbuf, "VOL1%-6.6s %-13.13s%-13.13s%-14.14s%28.28s%c",
	Volid, Spaces,
	(Ansiv == '4') ? IMPID : Spaces,
	Owner, Spaces, Ansiv);

if ((bytes = write(fileno(Magtfp), Labelbuf, BUFSIZE)) != BUFSIZE) {
	PERROR "\n%s: %s %s%c\n\n",
		Progname, CANTWVL, Magtdev, BELL);
	if (bytes == -1)
		perror(Magtdev);
	ceot();
}

if (Verbose) {
	PROMPT "%s: %s %s %s %c\n", Progname, VOLIS, Volid, ANSIV, Ansiv); 
	PROMPT "%s: %s %s\n", Progname, OWNRID, Owner);
	PROMPT "%s: %s  %s\n", Progname, IMPIDM, IMPID);
	if (Ultrixvol && !Noheader3) 
		PERROR "%s: %s  %s\n", Progname, VOLCRE, Hostname);
    	printf("\n");
}
Fsecno = Fseqno = 1;

/**/
/*
 *	If arguments on F_head list, process
 */

if (F_head) {
    for (; stnum; stnum--) {

	/* Get names off fstat from the end first */

	fstat = F_head;
	if (stnum > 1)
	    for (i = 1; i < stnum; i++, fstat = fstat->f_next)
		;
	/*
	 * Reset global default/user designated file
	 * type for output.
	 */
	Dfiletype = 0;
	/* 
	 * Go change directories (if required) and
	 * put the file onto the output volume.
	 */
	tree(fstat->f_src, iflag);
    }/*E for (; stnum; stnum--) */
}/*T if (F_head) */
else {
    PERROR "\n%s: %s %c\n", Progname, NOVALFI, BELL);
    exit(FAIL);
}
/*
 * Write final tape mark to signify end of all data
 * on this volume.
 * ie. A double set of tape marks should indicate EOV ..
 *
 */
weof();
printf("\n");
exit(SUCCEED);

}/*E initvol() */
/**/
/*
 *
 * Function:
 *
 *	tree
 *
 * Function Description:
 *
 *	This function changes the current working directory
 *	(if required) before calling the "putfile" logic to
 *	place the designated file on the output volume.
 *	Working directory is changed back to the orginal
 *	on completion of the "putfile".
 *
 * Arguments:
 *
 *	char	*pathname	Pointer to the pathname (file)
 *				to be written to the output vol.
 *
 * Return values:
 *
 *	none
 *
 * Side Effects:
 *
 *	If the routine cannot successfully change directories,
 *	a message is output to "stderr" and the function
 *	exits to system control.
 *	
 */

tree(pathname, iflag)
	char	*pathname;
	int	iflag;
{
/*
 * +--> Local Variables
 */

char	wdir[MAXPATHLEN];	/* Save working directory */
int	ret = TRUE;		/* Check if putfile good */

/*------*\
   Code
\*------*/

if (! getwd(wdir)) {
	PERROR "\n%s: %s %s%c\n\n", Progname, GETWDF, wdir, BELL);
	perror(Progname);
	exit(FAIL);
}
cp2 = pathname;
/*
 * Check if special case pathname = "/"
 */
if (!strcmp(pathname, "/") && (strlen(pathname) == 1)) {
    cp2 = pathname;
    pathname = '\0';
}
else {
    for (cp = pathname; *cp; cp++)
	if (*cp == '/')
	    cp2 = cp;
    if (*cp2 == '/')
	cp2++;
}
#ifdef DEBUG
printf("initvol: pathname = %s, cp2 = %s\n", pathname, cp2);
#endif

/*
 * Go put file(s) onto the output volume.
 */

ret = putfile(pathname, cp2, iflag, wdir);
if (chdir(wdir) < 0) {
	PERROR "\n%s: %s %s%c\n\n",
		Progname, CANTCHD, wdir, BELL);
	perror(Progname);
	exit(FAIL);
}
if (ret == EOF)
   return(EOF);
return(TRUE);
}/*E tree() */

/**\\**\\**\\**\\**\\**  EOM  initvol.c  **\\**\\**\\**\\**\\*/
/**\\**\\**\\**\\**\\**  EOM  initvol.c  **\\**\\**\\**\\**\\*/
