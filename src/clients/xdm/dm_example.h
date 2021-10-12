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
 * xdm - display manager daemon
 *
 * $XConsortium: dm.h,v 1.52 91/10/31 09:45:48 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * dm.h
 *
 * public interfaces for greet/verify functionality
 */

# include	<X11/Xlib.h>
# include	<X11/Xos.h>
# include	<X11/Xfuncs.h>
# include	<X11/Xmd.h>
# include	<X11/Xauth.h>

#if !defined(X_NOT_POSIX) && !defined(_POSIX_SOURCE)
#define _POSIX_SOURCE
# include	<setjmp.h>
#undef _POSIX_SOURCE
#else
# include	<setjmp.h>
#endif

/* If XDMCP symbol defined, compile to run XDMCP protocol */

#define XDMCP

#ifdef XDMCP
# include	<X11/Xdmcp.h>
#endif

#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
# include	<limits.h>
#else
#define _POSIX_SOURCE
# include	<limits.h>
#undef _POSIX_SOURCE
#endif
#endif
#ifndef NGROUPS_MAX
# include	<sys/param.h>
#ifndef NGROUPS_MAX
#ifdef NGROUPS
#define NGROUPS_MAX NGROUPS
#endif
#endif
#endif

#include <pwd.h>

typedef enum displayStatus { running, notRunning, zombie, phoenix } DisplayStatus;

#ifndef FD_ZERO
typedef	struct	my_fd_set { int fds_bits[1]; } my_fd_set;
# define FD_ZERO(fdp)	bzero ((fdp), sizeof (*(fdp)))
# define FD_SET(f,fdp)	((fdp)->fds_bits[(f) / (sizeof (int) * 8)] |=  (1 << ((f) % (sizeof (int) * 8))))
# define FD_CLR(f,fdp)	((fdp)->fds_bits[(f) / (sizeof (int) * 8)] &= ~(1 << ((f) % (sizeof (int) * 8))))
# define FD_ISSET(f,fdp)	((fdp)->fds_bits[(f) / (sizeof (int) * 8)] & (1 << ((f) % (sizeof (int) * 8))))
# define FD_TYPE	my_fd_set
#else
# define FD_TYPE	fd_set
#endif

/*
 * local     - server runs on local host
 * foreign   - server runs on remote host
 * permanent - session restarted when it exits
 * transient - session not restarted when it exits
 * fromFile  - started via entry in servers file
 * fromXDMCP - started with XDMCP
 */

typedef struct displayType {
	unsigned int	location:1;
	unsigned int	lifetime:1;
	unsigned int	origin:1;
} DisplayType;

# define Local		1
# define Foreign	0

# define Permanent	1
# define Transient	0

# define FromFile	1
# define FromXDMCP	0

extern DisplayType parseDisplayType ();

typedef enum fileState { NewEntry, OldEntry, MissingEntry } FileState;

struct display {
	struct display	*next;
	/* Xservers file / XDMCP information */
	char		*name;		/* DISPLAY name */
	char		*class;		/* display class (may be NULL) */
	DisplayType	displayType;	/* method to handle with */
	char		**argv;		/* program name and arguments */

	/* display state */
	DisplayStatus	status;		/* current status */
	int		pid;		/* process id of child */
	int		serverPid;	/* process id of server (-1 if none) */
	FileState	state;		/* state during HUP processing */
	int		startTries;	/* current start try */

#ifdef XDMCP
	/* XDMCP state */
	CARD32		sessionID;	/* ID of active session */
	XdmcpNetaddr    peer;		/* display peer address */
	int		peerlen;	/* length of peer address */
	XdmcpNetaddr    from;		/* XDMCP port of display */
	int		fromlen;
	CARD16		displayNumber;
	int		useChooser;	/* Run the chooser for this display */
	ARRAY8		clientAddr;	/* for chooser picking */
	CARD16		connectionType;	/* ... */
#endif
	/* server management resources */
	int		serverAttempts;	/* number of attempts at running X */
	int		openDelay;	/* open delay time */
	int		openRepeat;	/* open attempts to make */
	int		openTimeout;	/* abort open attempt timeout */
	int		startAttempts;	/* number of attempts at starting */
	int		pingInterval;	/* interval between XSync */
	int		pingTimeout;	/* timeout for XSync */
	int		terminateServer;/* restart for each session */
	int		grabServer;	/* keep server grabbed for Login */
	int		grabTimeout;	/* time to wait for grab */
	int		resetSignal;	/* signal to reset server */
	int		termSignal;	/* signal to terminate server */
	int		resetForAuth;	/* server reads auth file at reset */
	char		*keymaps;	/* keymaps file */
	char		*greeterLib;	/* greeter shareable lib name */

	/* session resources */
	char		*resources;	/* resource file */
	char		*xrdb;		/* xrdb program */
	char		*setup;		/* Xsetup program */
	char		*startup;	/* Xstartup program */
	char		*reset;		/* Xreset program */
	char		*session;	/* Xsession program */
	char		*userPath;	/* path set for session */
	char		*systemPath;	/* path set for startup/reset */
	char		*systemShell;	/* interpreter for startup/reset */
	char		*failsafeClient;/* a client to start when the session fails */
	char		*chooser;	/* chooser program */

	/* authorization resources */
	int		authorize;	/* enable authorization */
	char		**authNames;	/* authorization protocol names */
	unsigned short	*authNameLens;	/* authorization protocol name lens */
	char		*clientAuthFile;/* client specified auth file */
	char		*userAuthDir;	/* backup directory for tickets */
	int		authComplain;	/* complain when no auth for XDMCP */

	/* information potentially derived from resources */
	int		authNameNum;	/* number of protocol names */
	Xauth		**authorizations;/* authorization data */
	int		authNum;	/* number of authorizations */
	char		*authFile;	/* file to store authorization in */
};

#ifdef XDMCP

#define PROTO_TIMEOUT	(30 * 60)   /* 30 minutes should be long enough */

struct protoDisplay {
	struct protoDisplay	*next;
	XdmcpNetaddr		address;   /* UDP address */
	int			addrlen;    /* UDP address length */
	unsigned long		date;	    /* creation date */
	CARD16			displayNumber;
	CARD16			connectionType;
	ARRAY8			connectionAddress;
	CARD32			sessionID;
	Xauth			*fileAuthorization;
	Xauth			*xdmcpAuthorization;
	ARRAY8			authenticationName;
	ARRAY8			authenticationData;
	XdmAuthKeyRec		key;
};
#endif /* XDMCP */

struct greet_info {
	char		*name;		/* user name */
	char		*password;	/* user password */
	char		*string;	/* random string */
	struct passwd	*pwd;		/* password information */
};

/* setgroups is not covered by POSIX, arg type varies */
#if defined(SYSV) || defined(SVR4) || defined(__OSF1__)
#define GID_T gid_t
#else
#define GID_T int
#endif

struct verify_info {
	int		uid;		/* user id */
	GID_T		groups[NGROUPS_MAX];/* group list */
	int		ngroups;	/* number of elements in groups */
	char		**argv;		/* arguments to session */
	char		**userEnviron;	/* environment for session */
	char		**systemEnviron;/* environment for startup/reset */
};

/* verify return values */
#define VERIFY_SUCCESSFUL		0
#define VERIFY_FAILED			1
#define VERIFY_CAPACITY_LIMIT_EXCEEDED	2

/* display manager exit status definitions */

# define OBEYSESS_DISPLAY	0	/* obey multipleSessions resource */
# define REMANAGE_DISPLAY	1	/* force remanage */
# define UNMANAGE_DISPLAY	2	/* force deletion */
# define RESERVER_DISPLAY	3	/* force server termination */
# define OPENFAILED_DISPLAY	4	/* XOpenDisplay failed, retry */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

extern char	*config;

extern char	*servers;
extern int	request_port;
extern int	debugLevel;
extern char	*errorLogFile;
extern int	daemonMode;
extern char	*pidFile;
extern int	lockPidFile;
extern char	*authDir;
extern int	autoRescan;
extern int	removeDomainname;
extern char	*keyFile;
extern char	*accessFile;
extern char	**exportList;
extern char	*randomFile;
extern char	*greeterLib;
extern int	choiceTimeout;	/* chooser choice timeout */

extern struct display	*FindDisplayByName (),
			*FindDisplayBySessionID (),
			*FindDisplayByAddress (),
			*FindDisplayByPid (),
			*FindDisplayByServerPid (),
			*NewDisplay ();

extern struct protoDisplay	*FindProtoDisplay (),
				*NewProtoDisplay ();

extern char		*localHostname ();

int GreetUser(struct display	*d,
	    Display		**dpy,
	    struct verify_info	*verify,
	    struct greet_info	*greet);

typedef int (*GreetUserProc)( struct display *, Display **,
			    struct verify_info *, struct greet_info *);

/*
 * return values for GreetUser();
 */

#define SESSION_OVER	0
#define GREET_SUCCESS	1
#define GREET_FAILURE	-1

/*
 * CloseOnFork flags
 */

# define CLOSE_ALWAYS	    0
# define LEAVE_FOR_DISPLAY  1

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
char *malloc(), *realloc();
#endif
#if defined(macII) && !defined(__STDC__)  /* stdlib.h fails to define these */
char *malloc(), *realloc();
#endif /* macII */

#if defined(X_NOT_POSIX) && defined(SIGNALRETURNSINT)
#define SIGVAL int
#else
#define SIGVAL void
#endif

#if defined(X_NOT_POSIX) || defined(__alpha)
#ifdef SYSV
#define SIGNALS_RESET_WHEN_CAUGHT
#define UNRELIABLE_SIGNALS
#endif
#define Setjmp(e)	setjmp(e)
#define Longjmp(e,v)	longjmp(e,v)
#define Jmp_buf		jmp_buf
#else
#define Setjmp(e)   sigsetjmp(e,1)
#define Longjmp(e,v)	siglongjmp(e,v)
#define Jmp_buf		sigjmp_buf
#endif

SIGVAL (*Signal())();
