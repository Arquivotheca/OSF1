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
 *	@(#)$RCSfile: tt.h,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:11:53 $
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
 * Copyright (c) 1983 Regents of the University of California.
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
 *
 *	tt.h	3.20 (Berkeley) 6/29/88
 */

/*
 * Interface structure for the terminal drivers.
 */
struct tt {
		/* startup and cleanup */
	int (*tt_init)();
	int (*tt_end)();

		/* terminal functions */
	int (*tt_move)();
	int (*tt_insline)();
	int (*tt_delline)();
	int (*tt_delchar)();
	int (*tt_write)();		/* write a whole block */
	int (*tt_putc)();		/* write one character */
	int (*tt_clreol)();
	int (*tt_clreos)();
	int (*tt_clear)();
	int (*tt_scroll_down)();
	int (*tt_scroll_up)();
	int (*tt_setscroll)();		/* set scrolling region */
	int (*tt_setinsert)();		/* set insert mode */
	int (*tt_setmodes)();		/* set display modes */

		/* internal variables */
	char tt_modes;			/* the current display modes */
	char tt_nmodes;			/* the new modes for next write */
	char tt_insert;			/* currently in insert mode */
	char tt_ninsert;		/* insert mode on next write */
	int tt_row;			/* cursor row */
	int tt_col;			/* cursor column */
	int tt_scroll_top;		/* top of scrolling region */
	int tt_scroll_bot;		/* bottom of scrolling region */

		/* terminal info */
	int tt_nrow;			/* number of display rows */
	int tt_ncol;			/* number of display columns */
	char tt_hasinsert;		/* has insert character */
	char tt_availmodes;		/* the display modes supported */
	char tt_wrap;			/* has auto wrap around */
	char tt_retain;			/* can retain below (db flag) */

		/* the frame characters */
	short *tt_frame;
};
struct tt tt;

/*
 * List of terminal drivers.
 */
struct tt_tab {
	char *tt_name;
	int tt_len;
	int (*tt_func)();
};
extern struct tt_tab tt_tab[];

/*
 * Clean interface to termcap routines.
 * Too may t's.
 */
char tt_strings[1024];		/* string buffer */
char *tt_strp;			/* pointer for it */

struct tt_str {
	char *ts_str;
	int ts_n;
};

struct tt_str *tttgetstr();
struct tt_str *ttxgetstr();	/* tgetstr() and expand delays */

int tttputc();
#define tttputs(s, n)	tputs((s)->ts_str, (n), tttputc)
#define ttxputs(s)	ttwrite((s)->ts_str, (s)->ts_n)

/*
 * Buffered output without stdio.
 * These variables have different meanings from the ww_ob* variabels.
 * But I'm too lazy to think up different names.
 */
char tt_ob[512];
char *tt_obp;
char *tt_obe;
#define ttputc(c)	(tt_obp < tt_obe ? (*tt_obp++ = (c)) \
				: (ttflush(), *tt_obp++ = (c)))
