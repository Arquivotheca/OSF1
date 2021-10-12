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
static char *rcsid = "@(#)$RCSfile: dosys.c,v $ $Revision: 1.1.2.2 $ (DEC) $Date: 1992/11/20 17:24:52 $";
#endif

/* static char	*sccsid = "@(#)dosys.c	4.2	(ULTRIX)	9/20/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1986,1987,1988,1989 by		*
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
/*
 *
 *   Modification History:
 *
 *	18-Nov-92	Robin Miller
 *	Changed variable name from waitpid to wpid to avoid a conflict
 *	with the waitpid() shared library entry.  Results in the error:
 *	    waitpid: common but defined as text in a shared object
 *
 *	20-Sep-90   Lee R. Miller
 *
 *	14-Apr-89	Tim N
 *		Added posix handling of 'SHELL'.  In posix mode the command
 *		will always be passed to the shell for execution.
 *
 */

# include "defs"
# include <sys/types.h>
# include <sys/stat.h>

extern char Makecall;

dosys(comstring, nohalt)
register CHARSTAR comstring;
int nohalt;
{
	register CHARSTAR p;
	register int i;
	int status;

	p = comstring;
	while(	*p == BLANK ||
		*p == TAB) p++;
	if(!*p)
		return(-1);

	if(IS_ON(NOEX) && Makecall == NO)
		return(0);

	if( (sysvmode == POSIX_ON) || metas(comstring))
		status = doshell(comstring,nohalt);
	else
		status = doexec(comstring);

	return(status);
}



metas(s)   /* Are there are any  Shell meta-characters? */
register CHARSTAR s;
{
	while(*s)
		if( funny[*s++] & META)
			return(YES);

	return(NO);
}

doshell(comstring,nohalt)
register CHARSTAR comstring;
register int nohalt;
{
	register CHARSTAR shell;

	if((wpid = vfork()) == (pid_t) 0)
	{
		enbint(0);
		doclose();

		setenv();
		shell = varptr("SHELL")->varval;
		if(shell == 0 || shell[0] == CNULL || sysvmode==0)
			shell = SHELLCOM;
		execl(shell, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
		fatal("Couldn't load Shell");
	}

	return( await() );
}




await()
{
	int intrupt();
	int status;
	pid_t pid;

	enbint(intrupt);
	while( (pid = wait(&status)) != wpid)
		if(pid == (pid_t) -1)
			fatal("bad wait code");
	wpid = (pid_t) 0;
	return(status);
}






doclose()	/* Close open directory files before exec'ing */
{
	register DIRHDR od;

	for (od = firstdir; od != 0; od = od->nextdirhdr)
		if (od->dirfc != NULL)
			/* can't do closedir since we are vforking */
			/* so we close the fd for the child */
			/* closedir(od->dirfc); */
			(void) close(od->dirfc->dd_fd);
}





doexec(str)
register CHARSTAR str;
{
	register CHARSTAR t;
	register CHARSTAR *p;
	CHARSTAR argv[MAXEXECARGS];	/* 2048 args allowed!!! */
	int status, cntr = 0;

	while( *str==BLANK || *str==TAB )
		++str;
	if( *str == CNULL )
		return(-1);	/* no command */

	p = argv;
	for(t = str ; *t ; )
	{
		*p++ = t;
		if(++cntr > MAXEXECARGS - 1)
			fatal1(
			  "Maximum arguments (%d) allowed on an exec() has been exceeded",
						MAXEXECARGS - 1);
		while(*t!=BLANK && *t!=TAB && *t!=CNULL)
			++t;
		if(*t)
			for( *t++ = CNULL ; *t==BLANK || *t==TAB  ; ++t);
	}

	*p = NULL;

	if((wpid = vfork()) == (pid_t) 0)
	{
		enbint(0);
		doclose();
		setenv();
		execvp(str, argv);
		fatal1("Cannot load %s",str);
	}

	return( await() );
}

touch(force, name)
register int force;
register char *name;
{
        struct stat stbuff;
        char junk[1];
        int fd;

        if( stat(name,&stbuff) < 0)
                if(force)
                        goto create;
                else
                {
                        fprintf(stderr,"touch: file %s does not exist.\n",name);
                        return;
                }
        if(stbuff.st_size == 0)
                goto create;
        if( (fd = open(name, 2)) < 0)
                goto bad;
        if( read(fd, junk, 1) < 1)
        {
                close(fd);
                goto bad;
        }
        lseek(fd, (off_t) 0, 0);
        if( write(fd, junk, 1) < 1 )
        {
                close(fd);
                goto bad;
        }
        close(fd);
        return;
bad:
        fprintf(stderr, "Cannot touch %s\n", name);
        return;
create:
        if( (fd = creat(name, 0666)) < 0)
                goto bad;
        close(fd);
}
