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
 * @(#)$RCSfile: job.h,v $ $Revision: 1.1.2.3 $ (DEC) $Date: 1993/10/29 22:02:16 $
 */

/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)job.h	5.3 (Berkeley) 6/1/90
 */

/*-
 * job.h --
 *	Definitions pertaining to the running of jobs in parallel mode.
 *	Exported from job.c for the use of remote-execution modules.
 */

#ifndef _JOB_H_
#define _JOB_H_

#define TMPPAT	"/tmp/makeXXXXX"

/*
 * The SEL_ constants determine the maximum amount of time spent in select
 * before coming out to see if a child has finished. SEL_SEC is the number of
 * seconds and SEL_USEC is the number of micro-seconds 
 */
#define SEL_SEC		0
#define SEL_USEC	500000


/*-
 * Job Table definitions. 
 *
 * Each job has several things associated with it:
 *	1) The process id of the child shell
 *	2) The graph node describing the target being made by this job
 *	3) A LstNode for the first command to be saved after the job
 *	   completes. This is NILLNODE if there was no "..." in the job's
 *	   commands.
 *	4) An FILE* for writing out the commands. This is only
 *	   used before the job is actually started.
 *	5) A union of things used for handling the shell's output. Different
 *	   parts of the union are used based on the value of the usePipes
 *	   flag. If it is true, the output is being caught via a pipe and
 *	   the descriptors of our pipe, an array in which output is line
 *	   buffered and the current position in that buffer are all
 *	   maintained for each job. If, on the other hand, usePipes is false,
 *	   the output is routed to a temporary file and all that is kept
 *	   is the name of the file and the descriptor open to the file.
 *	6) An identifier provided by and for the exclusive use of the
 *	   Rmt module.
 *	7) A word of flags which determine how the module handles errors,
 *	   echoing, etc. for the job 
 *
 * The job "table" is kept as a linked Lst in 'jobs', with the number of
 * active jobs maintained in the 'nJobs' variable. At no time will this
 * exceed the value of 'maxJobs', initialized by the Job_Init function. 
 *
 * When a job is finished, the Make_Update function is called on each of the
 * parents of the node which was just remade. This takes care of the upward
 * traversal of the dependency graph.
 */
#define JOB_BUFSIZE	1024
typedef struct Job {
    int       	pid;	    /* The child's process ID */
    GNode    	*node;      /* The target the child is making */
    LstNode 	tailCmds;   /* The node of the first command to be
			     * saved when the job has been run */
    FILE 	*cmdFILE;   /* When creating the shell script, this is
			     * where the commands go */
    char    	*rmtID;     /* ID returned from Rmt module */
    short      	flags;	    /* Flags to control treatment of job */
#define	JOB_IGNERR	0x001	/* Ignore non-zero exits */
#define	JOB_SILENT	0x002	/* no output */
#define JOB_SPECIAL	0x004	/* Target is a special one. i.e. run it locally
				 * if we can't export it and maxLocal is 0 */
#define JOB_IGNDOTS	0x008  	/* Ignore "..." lines when processing
				 * commands */
#define JOB_REMOTE	0x010	/* Job is running remotely */  
#define JOB_FIRST	0x020	/* Job is first job for the node */
#define JOB_REMIGRATE	0x040	/* Job needs to be remigrated */
#define JOB_RESTART	0x080	/* Job needs to be completely restarted */
#define JOB_RESUME	0x100	/* Job needs to be resumed b/c it stopped,
				 * for some reason */
#define JOB_CONTINUING	0x200	/* We are in the process of resuming this job.
				 * Used to avoid infinite recursion between
				 * JobFinish and JobRestart */
    union {
	struct {
	    int	  	op_inPipe;	/* Input side of pipe associated
					 * with job's output channel */
	    int   	op_outPipe;	/* Output side of pipe associated with
					 * job's output channel */
	    char  	op_outBuf[JOB_BUFSIZE + 1];
	    	  	    	    	/* Buffer for storing the output of the
					 * job, line by line */
	    int   	op_curPos;	/* Current position in op_outBuf */
	}   	    o_pipe;	    /* data used when catching the output via
				     * a pipe */
	struct {
	    char  	of_outFile[sizeof(TMPPAT)+2];
	    	  	    	    	/* Name of file to which shell output
					 * was rerouted */
	    int	    	of_outFd;	/* Stream open to the output
					 * file. Used to funnel all
					 * from a single job to one file
					 * while still allowing
					 * multiple shell invocations */
	}   	    o_file;	    /* Data used when catching the output in
				     * a temporary file */
    }       	output;	    /* Data for tracking a shell's output */
} Job;

#define outPipe	  	output.o_pipe.op_outPipe
#define inPipe	  	output.o_pipe.op_inPipe
#define outBuf		output.o_pipe.op_outBuf
#define curPos		output.o_pipe.op_curPos
#define outFile		output.o_file.of_outFile
#define outFd	  	output.o_file.of_outFd


/*-
 * Shell Specifications:
 * Each shell type has associated with it the following information:
 *	1) The string which must match the last character of the shell name
 *	   for the shell to be considered of this type. The longest match
 *	   wins.
 *	2) A command to issue to turn off echoing of command lines
 *	3) A command to issue to turn echoing back on again
 *	4) What the shell prints, and its length, when given the echo-off
 *	   command. This line will not be printed when received from the shell
 *	5) A boolean to tell if the shell has the ability to control
 *	   error checking for individual commands.
 *	6) The string to turn this checking on.
 *	7) The string to turn it off.
 *	8) The command-flag to give to cause the shell to start echoing
 *	   commands right away.
 *	9) The command-flag to cause the shell to Lib_Exit when an error is
 *	   detected in one of the commands.
 *
 * Some special stuff goes on if a shell doesn't have error control. In such
 * a case, errCheck becomes a printf template for echoing the command,
 * should echoing be on and ignErr becomes another printf template for
 * executing the command while ignoring the return status. If either of these
 * strings is empty when hasErrCtl is FALSE, the command will be executed
 * anyway as is and if it causes an error, so be it.
 */
typedef struct Shell {
    char	  *name;	/* the name of the shell. For Bourne and C
				 * shells, this is used only to find the
				 * shell description when used as the single
				 * source of a .SHELL target. For user-defined
				 * shells, this is the full path of the shell.
				 */
    Boolean 	  hasEchoCtl;	/* True if both echoOff and echoOn defined */
    char          *echoOff;	/* command to turn off echo */
    char          *echoOn;	/* command to turn it back on again */
    char          *noPrint;	/* command to skip when printing output from
				 * shell. This is usually the command which
				 * was executed to turn off echoing */
    int           noPLen;	/* length of noPrint command */
    Boolean	  hasErrCtl;	/* set if can control error checking for
				 * individual commands */
    char	  *errCheck;	/* string to turn error checking on */
    char	  *ignErr;	/* string to turn off error checking */
    /*
     * command-line flags 
     */
    char          *echo;	/* echo commands */
    char          *exit;	/* exit on error */
}               Shell;


#endif /* _JOB_H_ */
