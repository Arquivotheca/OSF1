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
static char	*sccsid = "@(#)$RCSfile: reject.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 04:05:17 $";
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
 * COMPONENT_NAME: (CMDLANG) Language Utilities
 *
 * FUNCTIONS: yyracc, yyreject
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
# include <stdio.h>
extern FILE *yyout, *yyin;
extern int yyprevious , *yyfnd;
extern char yyextra[];
extern char yytext[];
extern int yyleng;
extern struct {
	int *yyaa, *yybb;
	int *yystops;
} *yylstate [], **yylsp, **yyolsp;
yyreject ()
{
for( ; yylsp < yyolsp; yylsp++)
	yytext[yyleng++] = yyinput();
if (*yyfnd > 0)
	return(yyracc(*yyfnd++));
while (yylsp-- > yylstate)
	{
	yyunput(yytext[yyleng-1]);
	yytext[--yyleng] = 0;
	if (*yylsp != 0 && (yyfnd= (*yylsp)->yystops) && *yyfnd > 0)
		return(yyracc(*yyfnd++));
	}
if (yytext[0] == 0)
	return(0);
yyoutput(yyprevious = yyinput());
yyleng=0;
return(-1);
}
yyracc(m)
{
yyolsp = yylsp;
if (yyextra[m])
	{
	while (yyback((*yylsp)->yystops, -m) != 1 && yylsp>yylstate)
		{
		yylsp--;
		yyunput(yytext[--yyleng]);
		}
	}
yyprevious = yytext[yyleng-1];
yytext[yyleng] = 0;
return(m);
}
