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
/* $XConsortium: miscfuncs.c,v 1.6 91/07/13 17:52:59 gildea Exp $ */

#include <X11/Xos.h>

#ifndef X_NOT_POSIX
#include <dirent.h>
#else
#ifdef SYSV
#include <dirent.h>
#else
#ifdef USG
#include <dirent.h>
#else
#include <sys/dir.h>
#ifndef dirent
#define dirent direct
#endif
#endif
#endif
#endif

char *malloc();
char *realloc();



#if defined(SYSV) && (defined(SYSV386) || defined(MOTOROLA))

/* These systems don't have the ftruncate() system call, so we emulate it.
 * This emulation can only shorten, not lengthen.
 * For convenience, we pass in the name of the file, even though the
 * real ftruncate doesn't.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define CHUNKSIZE 1024

int ftruncate_emu(fd, length, name)
    int fd;
    off_t length;
    char *name;
{
    char            tmp_file[15];
    int             new_fid, bytes_left, i;
    unsigned char   buffer[CHUNKSIZE];
    struct stat     stat_val;

    /* Open a temp file. */
    sprintf(tmp_file, ".xmhtmp%d~", getpid());
    (void) unlink(tmp_file);
    new_fid = open(tmp_file, O_RDWR | O_CREAT);
    lseek(fd, (off_t)0, 0);
	
    /* Copy original file to temp file. */
    for (i = 0; i < length / CHUNKSIZE; i++) {
	if (read(fd, buffer, CHUNKSIZE) != CHUNKSIZE) {
	    (void)fprintf(stderr, "xmh: read error in ftruncate emulation\n");
	    return -1;
	}
	else if (write(new_fid, buffer, CHUNKSIZE) != CHUNKSIZE) {
	    (void)fprintf(stderr, "xmh: write error in ftruncate emulation\n");
	    return -1;
	}
    }
    bytes_left = length % CHUNKSIZE;
    if (read(fd, buffer, bytes_left) != bytes_left) {
	(void)fprintf(stderr, "xmh: read error in ftruncate() emulation\n");
	return -1;
    }
    else if (write(new_fid, buffer, bytes_left) != bytes_left) {
	(void)fprintf(stderr, "xmh: write error in ftruncate() emulation\n");
	return -1;
    }

    /* Set mode of temp file to that of original file. */
    (void) fstat(fd, &stat_val);
    (void) chmod(tmp_file, stat_val.st_mode);

    /* Close files, delete original, rename temp file to original. */
    (void) myclose(new_fid);
    (void) myclose(fd);
    (void) unlink(name);	/* remove original */
    (void) rename(tmp_file, name); /* rename temp file */

    /* If we weren't going to close the file right away in the one
       place this is called from, we'd have to do something like this:
    new_fid = myopen(name, O_RDWR, 0666);
    if (new_fid != fd) {
	dup2(new_fid, fd);
	close(new_fid);
    }
       but the file does get closed, so we don't bother. */

    return 0;
}
#endif /* SYSV variant that needs ftruncate emulation */


/*
**  This code is by Rich Salz (rsalz@bbn.com), and ported to SVR4
**  by David Elliott (dce@smsc.sony.com).  No copyrights were found
**  in the original.  Subsequently modified by Bob Scheifler.
*/

/* A convenient shorthand. */
typedef struct dirent	 ENTRY;

/* Initial guess at directory size. */
#define INITIAL_SIZE	20

static int StrCmp(a, b)
    char **a, **b;
{
    return strcmp(*a, *b);
}

int
ScanDir(Name, List, Selector)
    char		  *Name;
    char		***List;
    int			 (*Selector)();
{
    register char	 **names;
    register ENTRY	  *E;
    register DIR	  *Dp;
    register int	   i;
    register int	   size;

    /* Get initial list space and open directory. */
    size = INITIAL_SIZE;
    if (!(names = (char **)malloc(size * sizeof(char *))) ||
	!(Dp = opendir(Name)))
	return(-1);

    /* Read entries in the directory. */
    for (i = 0; E = readdir(Dp); )
	if (!Selector || (*Selector)(E->d_name)) {
	    /* User wants them all, or he wants this one. */
	    if (++i >= size) {
		size <<= 1;
		names = (char**)realloc((char *)names, size * sizeof(char*));
		if (!names) {
		    closedir(Dp);
		    return(-1);
		}
	    }

	    /* Copy the entry. */
	    if (!(names[i - 1] = (char *)malloc(strlen(E->d_name) + 1))) {
		closedir(Dp);
		return(-1);
	    }
	    (void)strcpy(names[i - 1], E->d_name);
	}

    /* Close things off. */
    names[i] = (char *)0;
    *List = names;
    closedir(Dp);

    /* Sort? */
    if (i)
	qsort((char *)names, i, sizeof(char *), StrCmp);

    return(i);
}
