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
static char	*sccsid = "@(#)$RCSfile: tth29.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 01:12:06 $";
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
 * 	tth29.c	3.6 (Berkeley) 6/29/88
 */


#include "ww.h"
#include "tt.h"

/*
 * H29 Driver
 *
 * WWM_USR mode is alternate character set.
 *
kC|h29|heath-29|z29|zenith-29:\
	:am:bc=\ED:bt=\E-:cr=^M:do=^J:nl=^J:bl=^G:\
	:al=\EL:le=^H:bs:cd=\EJ:ce=\EK:cl=\EE:cm=\EY%+ %+ :co#80:dc=\EN:\
	:dl=1*\EM:do=\EB:ei=\EO:ho=\EH:im=\E@:li#24:mi:nd=\EC:as=\EF:ae=\EG:\
	:ms:ta=^I:pt:sr=\EI:se=\Eq:so=\Ep:up=\EA:vs=\Ex4:ve=\Ey4:\
	:kb=^H:ku=\EA:kd=\EB:kl=\ED:kr=\EC:kh=\EH:kn#1:k0=\E~:l0=HOME:\
	:k1=\ES:k2=\ET:k3=\EU:k4=\EV:k5=\EW:k6=\EP:k7=\EQ:k8=\ER:k9=\E01:\
	:es:hs:ts=\Ej\Ex5\Ex1\EY8%+ \Eo:fs=\Ek\Ey5:ds=\Ey1:us=\Es8:ue=\Es0:
 *
 */

#define pc(c)	ttputc('c')
#define esc()	pc(\033)

h29_setmodes(new)
register new;
{
	register modes = '0';

	if (new & WWM_REV)
		modes += 0x01;
	if (new & WWM_BLK)
		modes += 0x02;
	if (new & WWM_DIM)
		modes += 0x04;
	if (new & WWM_UL)
		modes += 0x08;
	if (new & WWM_USR)
		modes += 0x10;
	esc();
	pc(s);
	ttputc(modes);
	if (new & WWM_GRP) {
		if ((tt.tt_modes & WWM_GRP) == 0)
			esc(), pc(F);
	} else
		if (tt.tt_modes & WWM_GRP)
			esc(), pc(G);
	tt.tt_modes = new;
}

tt_h29()
{
	if (tt_h19() < 0)
		return -1;
	tt.tt_setmodes = h29_setmodes;
	tt.tt_availmodes |= WWM_BLK|WWM_UL|WWM_DIM|WWM_USR;
	return 0;
}
