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
static char rcsid[] = "@(#)$RCSfile: t6.c,v $ $Revision: 4.2.6.2 $ (DEC) $Date: 1993/06/10 18:04:41 $";
#endif
/*
 * HISTORY
 */
/*
static	char	*sccsid = "@(#)t6.c	8.1	(Japanese ULTRIX)  2/19/91";
static char sccsid[] = "@(#)t6.c	4.2 8/11/83";
*/

 /* t6.c: compute tab stops */

/***********************************************************************
 *  Edit History
 *
 * 16-Nov-88	Tim Newhouse
 * 		Changed the *s dereferences back to just plain old 's'
 *		Whoever wrote this code uses the .col field of the
 *              colstr table as both pointer and character.  Look at
 *              t5.c and tg.c if you are confused.  This does make cc
 *              give warnings during compiles.
 **********************************************************************/


# define tx(a) ((a)>0 && (a)<128)
# include "t..c"
maktab()
{
# define FN(i,c) font[stynum[i]][c]
# define SZ(i,c) csize[stynum[i]][c]
	/* define the tab stops of the table */
	int icol, ilin, tsep, k, ik, vforml, il, text;
	int doubled[MAXCOL], acase[MAXCOL];
	wchar_t *s;
	char *cp;

	for(icol=0; icol <ncol; icol++)
	{
		doubled[icol] = acase[icol] = 0;
		fprintf(tabout, ".nr %d 0\n", icol+CRIGHT);
		for(text=0; text<2; text++)
		{
			if (text)
				fprintf(tabout, ".%02d\n.rm %02d\n", icol+80, icol+80);
			for(ilin=0; ilin<nlin; ilin++)
			{
				if (instead[ilin]|| fullbot[ilin]) continue;
				vforml=ilin;
				for(il=prev(ilin); il>=0 && vspen(table[il][icol].col); il=prev(il))
					vforml=il;
				if (fspan(vforml,icol)) continue;
				if (filler(table[ilin][icol].col)) continue;
				switch(ctype(vforml,icol))
				{
				case 'a':
					acase[icol]=1;
					s = table[ilin][icol].col;
					if((int)s>0 && (int)s<128 && text)
					{
						if (doubled[icol]==0)
							fprintf(tabout, ".nr %d 0\n.nr %d 0\n",S1,S2);
						doubled[icol]=1;
						fprintf(tabout, ".if \\n(%c->\\n(%d .nr %d \\n(%c-\n",s,S2,S2,s);
					}
				case 'n':
					if (table[ilin][icol].rcol!=0)
					{
						if (doubled[icol]==0 && text==0)
							fprintf(tabout, ".nr %d 0\n.nr %d 0\n", S1, S2);
						doubled[icol]=1;
						if (real(s=table[ilin][icol].col) && !vspen(s))
						{
							if(tx((int)s) != text) continue;
							fprintf(tabout, ".nr %d ", TMP);
							wide(s, FN(vforml,icol), SZ(vforml,icol)); 
							fprintf(tabout, "\n");
							fprintf(tabout, ".if \\n(%d<\\n(%d .nr %d \\n(%d\n", S1, TMP, S1, TMP);
						}
						if (text==0 && real(s=table[ilin][icol].rcol) && !vspen(s) && !barent(s))
						{
							fprintf(tabout, ".nr %d \\w%c%s%c\n",TMP, F1, cp = Tombs(s), F1);
							XFREE(cp);
							fprintf(tabout, ".if \\n(%d<\\n(%d .nr %d \\n(%d\n",S2,TMP,S2,TMP);
						}
						continue;
					}
				case 'r':
				case 'c':
				case 'l':
					if (real(s=table[ilin][icol].col) && !vspen(s))
					{
						if(tx((int)s) != text) continue;
						fprintf(tabout, ".nr %d ", TMP);
						wide(s, FN(vforml,icol), SZ(vforml,icol)); 
						fprintf(tabout, "\n");
						fprintf(tabout, ".if \\n(%d<\\n(%d .nr %d \\n(%d\n", icol+CRIGHT, TMP, icol+CRIGHT, TMP);
					}
				}
			}
		}
		if (acase[icol])
		{
			fprintf(tabout, ".if \\n(%d>=\\n(%d .nr %d \\n(%du+2n\n",S2,icol+CRIGHT,icol+CRIGHT,S2);
		}
		if (doubled[icol])
		{
			fprintf(tabout, ".nr %d \\n(%d\n", icol+CMID, S1);
			fprintf(tabout, ".nr %d \\n(%d+\\n(%d\n",TMP,icol+CMID,S2);
			fprintf(tabout, ".if \\n(%d>\\n(%d .nr %d \\n(%d\n",TMP,icol+CRIGHT,icol+CRIGHT,TMP);
			fprintf(tabout, ".if \\n(%d<\\n(%d .nr %d +(\\n(%d-\\n(%d)/2\n",TMP,icol+CRIGHT,icol+CMID,icol+CRIGHT,TMP);
		}
		if (cll[icol][0])
		{
			fprintf(tabout, ".nr %d %sn\n", TMP, cll[icol]);
			fprintf(tabout, ".if \\n(%d<\\n(%d .nr %d \\n(%d\n",icol+CRIGHT, TMP, icol+CRIGHT, TMP);
		}
		for(ilin=0; ilin<nlin; ilin++)
			if (k=lspan(ilin, icol))
			{
				s=table[ilin][icol-k].col;
				if (!real(s) || barent(s) || vspen(s) ) continue;
				fprintf(tabout, ".nr %d ", TMP);
				wide(table[ilin][icol-k].col, FN(ilin,icol-k), SZ(ilin,icol-k));
				for(ik=k; ik>=0; ik--)
				{
					fprintf(tabout, "-\\n(%d",CRIGHT+icol-ik);
					if (!expflg && ik>0) fprintf(tabout, "-%dn", sep[icol-ik]);
				}
				fprintf(tabout, "\n");
				fprintf(tabout, ".if \\n(%d>0 .nr %d \\n(%d/%d\n", TMP, TMP, TMP, k);
				fprintf(tabout, ".if \\n(%d<0 .nr %d 0\n", TMP, TMP);
				for(ik=1; ik<=k; ik++)
				{
					if (doubled[icol-k+ik])
						fprintf(tabout, ".nr %d +\\n(%d/2\n", icol-k+ik+CMID, TMP);
					fprintf(tabout, ".nr %d +\\n(%d\n", icol-k+ik+CRIGHT, TMP);
				}
			}
	}
	if (textflg) untext();
	/* if even requested, make all columns widest width */
# define TMP1 S1
# define TMP2 S2
	if (evenflg)
	{
		fprintf(tabout, ".nr %d 0\n", TMP);
		for(icol=0; icol<ncol; icol++)
		{
			if (evenup[icol]==0) continue;
			fprintf(tabout, ".if \\n(%d>\\n(%d .nr %d \\n(%d\n",
			icol+CRIGHT, TMP, TMP, icol+CRIGHT);
		}
		for(icol=0; icol<ncol; icol++)
		{
			if (evenup[icol]==0)
				/* if column not evened just retain old interval */
				continue;
			if (doubled[icol])
				fprintf(tabout, ".nr %d (100*\\n(%d/\\n(%d)*\\n(%d/100\n",
				icol+CMID, icol+CMID, icol+CRIGHT, TMP);
			/* that nonsense with the 100's and parens tries
							   to avoid overflow while proportionally shifting
							   the middle of the number */
			fprintf(tabout, ".nr %d \\n(%d\n", icol+CRIGHT, TMP);
		}
	}
	/* now adjust for total table width */
	for(tsep=icol=0; icol<ncol; icol++)
		tsep+= sep[icol];
	if (expflg)
	{
		fprintf(tabout, ".nr %d 0", TMP);
		for(icol=0; icol<ncol; icol++)
			fprintf(tabout, "+\\n(%d", icol+CRIGHT);
		fprintf(tabout, "\n");
		fprintf(tabout, ".nr %d \\n(.l-\\n(%d\n", TMP, TMP);
		if (boxflg || dboxflg || allflg)
			tsep += 1;
		else
			tsep -= sep[ncol-1];
		fprintf(tabout, ".nr %d \\n(%d/%d\n", TMP, TMP,  tsep);
		fprintf(tabout, ".if \\n(%d<0 .nr %d 0\n", TMP, TMP);
	}
	else
		fprintf(tabout, ".nr %d 1n\n", TMP);
	fprintf(tabout, ".nr %d 0\n",CRIGHT-1);
	tsep= (boxflg || allflg || dboxflg || left1flg) ? 1 : 0;
	for(icol=0; icol<ncol; icol++)
	{
		fprintf(tabout, ".nr %d \\n(%d+(%d*\\n(%d)\n",icol+CLEFT, icol+CRIGHT-1, tsep, TMP);
		fprintf(tabout, ".nr %d +\\n(%d\n",icol+CRIGHT, icol+CLEFT);
		if (doubled[icol])
		{
			/* the next line is last-ditch effort to avoid zero field width */
			/*fprintf(tabout, ".if \\n(%d=0 .nr %d 1\n",icol+CMID, icol+CMID);*/
			fprintf(tabout, ".nr %d +\\n(%d\n", icol+CMID, icol+CLEFT);
			/*  fprintf(tabout, ".if n .if \\n(%d%%24>0 .nr %d +12u\n",icol+CMID, icol+CMID); */
		}
		tsep=sep[icol];
	}
	if (rightl)
		fprintf(tabout, ".nr %d (\\n(%d+\\n(%d)/2\n",ncol+CRIGHT-1, ncol+CLEFT-1, ncol+CRIGHT-2);
	fprintf(tabout, ".nr TW \\n(%d\n", ncol+CRIGHT-1);
	if (boxflg || allflg || dboxflg)
		fprintf(tabout, ".nr TW +%d*\\n(%d\n", sep[ncol-1], TMP);
	fprintf(tabout,
	".if t .if \\n(TW>\\n(.li .tm Table at line %d file %s is too wide - \\n(TW units\n", iline-1, ifile);
	return;
}
wide(s, fn, size)
char *size, *fn;
wchar_t *s;
{
	register char *cp;

	if (point(s))
	{
		fprintf(tabout, "\\w%c", F1);
		if (*fn>0) putfont(fn);
		if (*size) putsize(size);
		fprintf(tabout, "%s", cp = Tombs(s));
		XFREE(cp);
		if (*fn>0) putfont("P");
		if (*size) putsize("0");
		fprintf(tabout, "%c",F1);
	}
	else
		fprintf(tabout, "\\n(%c-", s);
}
filler(s)
wchar_t *s;
{
	return (point(s) && s[0]=='\\' && s[1] == 'R');
}
