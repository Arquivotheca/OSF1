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
static char	*sccsid = "@(#)$RCSfile: termstat.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 06:32:15 $";
#endif 
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
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint

#endif /* not lint */

#include "telnetd.h"

/*
 * local variables
 */
#ifdef	LINEMODE
static int _terminit = 0;
static int def_tspeed = -1, def_rspeed = -1;
#ifdef	TIOCSWINSZ
static int def_row = 0, def_col = 0;
#endif
#endif	LINEMODE


#ifdef	LINEMODE
/*
 * localstat
 *
 * This function handles all management of linemode.
 *
 * Linemode allows the client to do the local editing of data
 * and send only complete lines to the server.  Linemode state is
 * based on the state of the pty driver.  If the pty is set for
 * external processing, then we can use linemode.  Further, if we
 * can use real linemode, then we can look at the edit control bits
 * in the pty to determine what editing the client should do.
 *
 * Linemode support uses the following state flags to keep track of
 * current and desired linemode state.
 *	alwayslinemode : true if -l was specified on the telnetd
 * 	command line.  It means to have linemode on as much as
 *	possible.
 *
 * 	lmodetype: signifies whether the client can
 *	handle real linemode, or if use of kludgeomatic linemode
 *	is preferred.  It will be set to one of the following:
 *		REAL_LINEMODE : use linemode option
 *		KLUDGE_LINEMODE : use kludge linemode
 *		NO_LINEMODE : client is ignorant of linemode
 *
 *	linemode, uselinemode : linemode is true if linemode
 *	is currently on, uselinemode is the state that we wish
 *	to be in.  If another function wishes to turn linemode
 *	on or off, it sets or clears uselinemode.
 *
 *	editmode, useeditmode : like linemode/uselinemode, but
 *	these contain the edit mode states (edit and trapsig).
 *
 * The state variables correspond to some of the state information
 * in the pty.
 *	linemode:
 *		In real linemode, this corresponds to whether the pty
 *		expects external processing of incoming data.
 *		In kludge linemode, this more closely corresponds to the
 *		whether normal processing is on or not.  (ICANON in
 *		system V, or COOKED mode in BSD.)
 *		If the -l option was specified (alwayslinemode), then
 *		an attempt is made to force external processing on at
 *		all times.
 *
 * The following heuristics are applied to determine linemode
 * handling within the server.
 *	1) Early on in starting up the server, an attempt is made
 *	   to negotiate the linemode option.  If this succeeds
 *	   then lmodetype is set to REAL_LINEMODE and all linemode
 *	   processing occurs in the context of the linemode option.
 *	2) If the attempt to negotiate the linemode option failed,
 *	   then we try to use kludge linemode.  We test for this
 *	   capability by sending "do Timing Mark".  If a positive
 *	   response comes back, then we assume that the client
 *	   understands kludge linemode (ech!) and the
 *	   lmodetype flag is set to KLUDGE_LINEMODE.
 *	3) Otherwise, linemode is not supported at all and
 *	   lmodetype remains set to NO_LINEMODE (which happens
 *	   to be 0 for convenience).
 *	4) At any time a command arrives that implies a higher
 *	   state of linemode support in the client, we move to that
 *	   linemode support.
 *
 * A short explanation of kludge linemode is in order here.
 *	1) The heuristic to determine support for kludge linemode
 *	   is to send a do timing mark.  We assume that a client
 *	   that supports timing marks also supports kludge linemode.
 *	   A risky proposition at best.
 *	2) Further negotiation of linemode is done by changing the
 *	   the server's state regarding SGA.  If server will SGA,
 *	   then linemode is off, if server won't SGA, then linemode
 *	   is on.
 */
localstat()
{
	void netflush();

	/*
	 * Check for state of BINARY options.
	 */
	if (tty_isbinaryin()) {
		if (hiswants[TELOPT_BINARY] == OPT_NO)
			send_do(TELOPT_BINARY, 1);
	} else {
		if (hiswants[TELOPT_BINARY] == OPT_YES)
			send_dont(TELOPT_BINARY, 1);
	}

	if (tty_isbinaryout()) {
		if (mywants[TELOPT_BINARY] == OPT_NO)
			send_will(TELOPT_BINARY, 1);
	} else {
		if (mywants[TELOPT_BINARY] == OPT_YES)
			send_wont(TELOPT_BINARY, 1);
	}

	/*
	 * Check for changes to flow control if client supports it.
	 */
	if (hisopts[TELOPT_LFLOW] == OPT_YES) {
		if (tty_flowmode() != flowmode) {
			flowmode = tty_flowmode();
			(void) sprintf(nfrontp, "%c%c%c%c%c%c", IAC, SB,
				TELOPT_LFLOW, flowmode, IAC, SE);
			nfrontp += 6;
		}
	}

	/*
	 * Check linemode on/off state
	 */
	uselinemode = tty_linemode();

	/*
	 * If alwayslinemode is on, and pty is changing to turn it off, then
	 * force linemode back on.
	 */
	if (alwayslinemode && linemode && !uselinemode) {
		uselinemode = 1;
		tty_setlinemode(uselinemode);
	}

# ifdef	KLUDGELINEMODE
	/*
	 * If using kludge linemode and linemode is desired, it can't
	 * be done if normal line editing is not available on the
	 * pty.  This becomes the test for linemode on/off when
	 * using kludge linemode.
	 */
	if (lmodetype == KLUDGE_LINEMODE && uselinemode && tty_israw()) {
		uselinemode = 0;
		tty_setlinemode(uselinemode);
	}
# endif	/* KLUDGELINEMODE */

	/*
	 * Do echo mode handling as soon as we know what the
	 * linemode is going to be.
	 * If the pty has echo turned off, then tell the client that
	 * the server will echo.  If echo is on, then the server
	 * will echo if in character mode, but in linemode the
	 * client should do local echoing.  The state machine will
	 * not send anything if it is unnecessary, so don't worry
	 * about that here.
	 */
	if (tty_isecho() && uselinemode)
		send_wont(TELOPT_ECHO, 1);
	else
		send_will(TELOPT_ECHO, 1);

	/*
	 * If linemode is being turned off, send appropriate
	 * command and then we're all done.
	 */
	 if (!uselinemode && linemode) {
# ifdef	KLUDGELINEMODE
		if (lmodetype == REAL_LINEMODE)
# endif	/* KLUDGELINEMODE */
			send_dont(TELOPT_LINEMODE, 1);
# ifdef	KLUDGELINEMODE
		else if (lmodetype == KLUDGE_LINEMODE)
			send_will(TELOPT_SGA, 1);
# endif	/* KLUDGELINEMODE */
		linemode = uselinemode;
		goto done;
	}

# ifdef	KLUDGELINEMODE
	/*
	 * If using real linemode check edit modes for possible later use.
	 */
	if (lmodetype == REAL_LINEMODE) {
# endif	/* KLUDGELINEMODE */
		useeditmode = 0;
		if (tty_isediting())
			useeditmode |= MODE_EDIT;
		if (tty_istrapsig())
			useeditmode |= MODE_TRAPSIG;
# ifdef	KLUDGELINEMODE
	}
# endif	/* KLUDGELINEMODE */

	/*
	 * Negotiate linemode on if pty state has changed to turn it on.
	 * Send appropriate command and send along edit mode, then all done.
	 */
	if (uselinemode && !linemode) {
# ifdef	KLUDGELINEMODE
		if (lmodetype == KLUDGE_LINEMODE) {
			send_wont(TELOPT_SGA, 1);
		} else if (lmodetype == REAL_LINEMODE) {
# endif	/* KLUDGELINEMODE */
			send_do(TELOPT_LINEMODE, 1);
			/* send along edit modes */
			(void) sprintf(nfrontp, "%c%c%c%c%c%c%c", IAC, SB,
				TELOPT_LINEMODE, LM_MODE, useeditmode,
				IAC, SE);
			nfrontp += 7;
			editmode = useeditmode;
# ifdef	KLUDGELINEMODE
		}
# endif	/* KLUDGELINEMODE */
		linemode = uselinemode;
		goto done;
	}

# ifdef	KLUDGELINEMODE
	/*
	 * None of what follows is of any value if not using
	 * real linemode.
	 */
	if (lmodetype < REAL_LINEMODE)
		goto done;
# endif	/* KLUDGELINEMODE */

	if (linemode) {
		/*
		 * If edit mode changed, send edit mode.
		 */
		 if (useeditmode != editmode) {
			/*
			 * Send along appropriate edit mode mask.
			 */
			(void) sprintf(nfrontp, "%c%c%c%c%c%c%c", IAC, SB,
				TELOPT_LINEMODE, LM_MODE, useeditmode,
				IAC, SE);
			nfrontp += 7;
			editmode = useeditmode;
		}
							

		/*
		 * Check for changes to special characters in use.
		 */
		start_slc(0);
		check_slc();
		end_slc(0);
	}

done:
	/*
	 * Some things should be deferred until after the pty state has
	 * been set by the local process.  Do those things that have been
	 * deferred now.  This only happens once.
	 */
	if (_terminit == 0) {
		_terminit = 1;
		defer_terminit();
	}

	netflush();
	set_termbuf();
	return;

}  /* end of localstat */
#endif	/* LINEMODE */


/*
 * clientstat
 *
 * Process linemode related requests from the client.
 * Client can request a change to only one of linemode, editmode or slc's
 * at a time, and if using kludge linemode, then only linemode may be
 * affected.
 */
clientstat(code, parm1, parm2)
register int code, parm1, parm2;
{
	void netflush();

	/*
	 * Get a copy of terminal characteristics.
	 */
	init_termbuf();

	/*
	 * Process request from client. code tells what it is.
	 */
	switch (code) {
#ifdef	LINEMODE
	case TELOPT_LINEMODE:
		/*
		 * Don't do anything unless client is asking us to change
		 * modes.
		 */
		uselinemode = (parm1 == WILL);
		if (uselinemode != linemode) {
# ifdef	KLUDGELINEMODE
			/*
			 * If using kludge linemode, make sure that
			 * we can do what the client asks.
			 * We can not turn off linemode if alwayslinemode
			 * and the ICANON bit is set.
			 */
			if (lmodetype == KLUDGE_LINEMODE) {
				if (alwayslinemode && tty_isediting()) {
					uselinemode = 1;
				}
			}
		
			/*
			 * Quit now if we can't do it.
			 */
			if (uselinemode == linemode)
				return;

			/*
			 * If using real linemode and linemode is being
			 * turned on, send along the edit mode mask.
			 */
			if (lmodetype == REAL_LINEMODE && uselinemode)
# else	/* KLUDGELINEMODE */
			if (uselinemode)
# endif	/* KLUDGELINEMODE */
			{
				useeditmode = 0;
				if (tty_isediting())
					useeditmode |= MODE_EDIT;
				if (tty_istrapsig)
					useeditmode |= MODE_TRAPSIG;
				(void) sprintf(nfrontp, "%c%c%c%c%c%c%c", IAC,
					SB, TELOPT_LINEMODE, LM_MODE,
							useeditmode, IAC, SE);
				nfrontp += 7;
				editmode = useeditmode;
			}


			tty_setlinemode(uselinemode);

			linemode = uselinemode;

		}
		break;
	
	case LM_MODE:
	    {
		register int mode, sig, ack;

		/*
		 * Client has sent along a mode mask.  If it agrees with
		 * what we are currently doing, ignore it; if not, it could
		 * be viewed as a request to change.  Note that the server
		 * will change to the modes in an ack if it is different from
		 * what we currently have, but we will not ack the ack.
		 */
		 useeditmode &= MODE_MASK;
		 ack = (useeditmode & MODE_ACK);
		 useeditmode &= ~MODE_ACK;

		 if (useeditmode != editmode) {
			mode = (useeditmode & MODE_EDIT);
			sig = (useeditmode & MODE_TRAPSIG);

			if (mode != (editmode & LM_MODE)) {
				tty_setedit(mode);
			}
			if (sig != (editmode & MODE_TRAPSIG)) {
				tty_setsig(sig);
			}

			set_termbuf();

 			if (!ack) {
 				(void) sprintf(nfrontp, "%c%c%c%c%c%c%c", IAC,
					SB, TELOPT_LINEMODE, LM_MODE,
 					useeditmode|MODE_ACK,
 					IAC, SE);
 				nfrontp += 7;
 			}
 		
			editmode = useeditmode;
		}

		break;

	    }  /* end of case LM_MODE */
#endif	/* LINEMODE */

	case TELOPT_NAWS:
#ifdef	TIOCSWINSZ
	    {
		struct winsize ws;

#ifdef	LINEMODE
		/*
		 * Defer changing window size until after terminal is
		 * initialized.
		 */
		if (terminit() == 0) {
			def_col = parm1;
			def_row = parm2;
			return;
		}
#endif	/* LINEMODE */

		/*
		 * Change window size as requested by client.
		 */

		ws.ws_col = parm1;
		ws.ws_row = parm2;
		(void) ioctl(pty, TIOCSWINSZ, (char *)&ws);
	    }
#endif	/* TIOCSWINSZ */
		
		break;
	
	case TELOPT_TSPEED:
	    {
#ifdef	LINEMODE
		/*
		 * Defer changing the terminal speed.
		 */
		if (terminit() == 0) {
			def_tspeed = parm1;
			def_rspeed = parm2;
			return;
		}
#endif	/* LINEMODE */
		/*
		 * Change terminal speed as requested by client.
		 */
		tty_tspeed(parm1);
		tty_rspeed(parm2);
		set_termbuf();

		break;

	    }  /* end of case TELOPT_TSPEED */

	default:
		/* What? */
		break;
	}  /* end of switch */

	netflush();

}  /* end of clientstat */

#ifdef	LINEMODE
/*
 * defer_terminit
 *
 * Some things should not be done until after the login process has started
 * and all the pty modes are set to what they are supposed to be.  This
 * function is called when the pty state has been processed for the first time. 
 * It calls other functions that do things that were deferred in each module.
 */
defer_terminit()
{

	/*
	 * local stuff that got deferred.
	 */
	if (def_tspeed != -1) {
		clientstat(TELOPT_TSPEED, def_tspeed, def_rspeed);
		def_tspeed = def_rspeed = 0;
	}

#ifdef	TIOCSWINSZ
	if (def_col || def_row) {
		struct winsize ws;

		ws.ws_col = def_col;
		ws.ws_row = def_row;
		(void) ioctl(pty, TIOCSWINSZ, (char *)&ws);
	}
#endif

	/*
	 * The only other module that currently defers anything.
	 */
	deferslc();

}  /* end of defer_terminit */

/*
 * terminit
 *
 * Returns true if the pty state has been processed yet.
 */
int terminit()
{
	return _terminit;

}  /* end of terminit */
#endif	/* LINEMODE */
