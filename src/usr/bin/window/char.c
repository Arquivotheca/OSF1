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
static char	*sccsid = "@(#)$RCSfile: char.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:10:17 $";
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
#if !defined(lint) && !defined(_NOIDENT)

#endif
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
 * 	char.c	3.5 (Berkeley) 6/29/88
 * 
 */

#include "char.h"

char _cmap[] = {
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^@ - ^C */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^D - ^G */
	_C,		_C|_P,		_C,		_C|_U,	/* ^H - ^K */
	_C|_U,		_C,		_C|_U,		_C|_U,	/* ^L - ^O */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^P - ^S */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^T - ^W */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^U - ^[ */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^\ - ^_ */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,

	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,

	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_P|_U,
	_P|_U,		_P|_U,		_P|_U,		_C|_U,

	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,

	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,

	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,

	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U,
	_C|_U,		_C|_U,		_C|_U,		_C|_U
};

char *_unctrl[] = {
	"^@",	"^A",	"^B",	"^C",	"^D",	"^E",	"^F",	"^G",
	"^H",	"^I",	"^J",	"^K",	"^L",	"^M",	"^N",	"^O",
	"^P",	"^Q",	"^R",	"^S",	"^T",	"^U",	"^V",	"^W",
	"^X",	"^Y",	"^Z",	"^[",	"^\\",	"^]",	"^^",	"^_",
	" ",	"!",	"\"",	"#",	"$",	"%",	"&",	"'",
	"(",	")",	"*",	"+",	",",	"-",	".",	"/",
	"0",	"1",	"2",	"3",	"4",	"5",	"6",	"7",
	"8",	"9",	":",	";",	"<",	"=",	">",	"?",
	"@",	"A",	"B",	"C",	"D",	"E",	"F",	"G",
	"H",	"I",	"J",	"K",	"L",	"M",	"N",	"O",
	"P",	"Q",	"R",	"S",	"T",	"U",	"V",	"W",
	"X",	"Y",	"Z",	"[",	"\\",	"]",	"^",	"_",
	"`",	"a",	"b",	"c",	"d",	"e",	"f",	"g",
	"h",	"i",	"j",	"k",	"l",	"m",	"n",	"o",
	"p",	"q",	"r",	"s",	"t",	"u",	"v",	"w",
	"x",	"y",	"z",	"{",	"|",	"}",	"~",	"^?",
	"\\200","\\201","\\202","\\203","\\204","\\205","\\206","\\207",
	"\\210","\\211","\\212","\\213","\\214","\\215","\\216","\\217",
	"\\220","\\221","\\222","\\223","\\224","\\225","\\226","\\227",
	"\\230","\\231","\\232","\\233","\\234","\\235","\\236","\\237",
	"\\240","\\241","\\242","\\243","\\244","\\245","\\246","\\247",
	"\\250","\\251","\\252","\\253","\\254","\\255","\\256","\\257",
	"\\260","\\261","\\262","\\263","\\264","\\265","\\266","\\267",
	"\\270","\\271","\\272","\\273","\\274","\\275","\\276","\\277",
	"\\300","\\301","\\302","\\303","\\304","\\305","\\306","\\307",
	"\\310","\\311","\\312","\\313","\\314","\\315","\\316","\\317",
	"\\320","\\321","\\322","\\323","\\324","\\325","\\326","\\327",
	"\\330","\\331","\\332","\\333","\\334","\\335","\\336","\\337",
	"\\340","\\341","\\342","\\343","\\344","\\345","\\346","\\347",
	"\\350","\\351","\\352","\\353","\\354","\\355","\\356","\\357",
	"\\360","\\361","\\362","\\363","\\364","\\365","\\366","\\367",
	"\\370","\\371","\\372","\\373","\\374","\\375","\\376","\\377"
};
