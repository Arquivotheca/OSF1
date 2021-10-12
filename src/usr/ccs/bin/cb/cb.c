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
static char	*sccsid = "@(#)$RCSfile: cb.c,v $ $Revision: 4.2.4.2 $ (DEC) $Date: 1993/10/07 19:22:07 $";
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
 * COMPONENT_NAME: (CMDPROG) Programming Utilites
 *
 * FUNCTIONS: main, checkif, clearif, comment, copy, getch, getnext, getnl,
 *	      gotdo, gotelse, gotif, gotop, gotstruct, gottype, keep, lookup,
 *	      outs, ptabs, putch, putspace, puttmp, resetdo, unget, work
 *
 * ORIGINS: 00 03 10 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 * 
 * cb.c	1.10  com/cmd/prog/cb,3.1,8951 11/28/89 15:00:46"; 
 */
#include <stdio.h>

#ifdef KJI
#include <NLchar.h>
#endif
#include <locale.h>	/* 001 */
#include <ctype.h>	/* 001 */

#include "cb_msg.h"
nl_catd catd;
#define MSGSTR(Num, Str) catgets(catd, MS_CB, Num, Str)

/* character type flags */
/****** 001 BEGIN Comment Out. Char type flags are now defined in ctype.h
#define _U      01
#define _L      02
#define _N      04
#define _S      010
#define _P      020
#define _C      040
#define _X      0100
#define _O      0200
******* 001 END Comment Out. **********************/

/* 001 Force use of is*() functions instead of macros.
 *     The functions handle 8-bit international chars.
 */
/****** 001 BEGIN Comment Out. *******************
extern  char    _ctype_[];

#ifdef KJI
#define isop(c) 	((c) <_O && (_ctype_+1)[c]&_O)
#else
#define isop(c) 	((_ctype_+1)[c]&_O)
#endif

#define isalpha(c)      ((_ctype_+1)[c]&(_U|_L))
#define isupper(c)      ((_ctype_+1)[c]&_U)
#define islower(c)      ((_ctype_+1)[c]&_L)
#define isdigit(c)      ((_ctype_+1)[c]&_N)
#define isxdigit(c)     ((_ctype_+1)[c]&(_N|_X))
#define isspace(c)      ((_ctype_+1)[c]&_S)
#define ispunct(c)      ((_ctype_+1)[c]&_P)
#define isalnum(c)      ((_ctype_+1)[c]&(_U|_L|_N))
#define isprint(c)      ((_ctype_+1)[c]&(_P|_U|_L|_N))
#define iscntrl(c)      ((_ctype_+1)[c]&_C)
#define isascii(c)      ((unsigned)(c)<=0177)
#define toupper(c)      ((c)-'a'+'A')
#define tolower(c)      ((c)-'A'+'a')
#define toascii(c)      ((c)&0177)
char _ctype_[] = {
	0,
	_C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
	_C,     _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C,     _C,
	_C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
	_C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
	_S,     _P|_O,  _P,     _P,     _P,     _P|_O,  _P|_O,  _P,
	_P,     _P,     _P|_O,  _P|_O,  _P,     _P|_O,  _P,     _P|_O,
	_N,     _N,     _N,     _N,     _N,     _N,     _N,     _N,
	_N,     _N,     _P,     _P,     _P|_O,  _P|_O,  _P|_O,  _P,
	_P,     _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U,
	_U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
	_U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
	_U,     _U,     _U,     _P,     _P,     _P,     _P|_O,  _P|_L,
	_P,     _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L,
	_L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
	_L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
	_L,     _L,     _L,     _P,     _P|_O,  _P,     _P,     _C
};
******* 001 END Comment Out. **********************/

/*
 * 001 Define macro, isop(), to classify operation chars, which also
 *     handles 8 bit chars. No libc function is provided for this.
 */
#define CB_O      0200
#define isop(c)	(((unsigned char)(c)) <CB_O && (_optable_+1)[c]&CB_O)

char _optable_[] = {
	0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     CB_O,  0,     0,     0,     CB_O,  CB_O,  0,
	0,     0,     CB_O,  CB_O,  0,     CB_O,  0,     CB_O,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     CB_O,  CB_O,  CB_O,  0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     CB_O,  0,   
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     CB_O,  0,     0,     0
};


#define IF      1
#define ELSE    2
#define CASE    3
#define TYPE    4
#define DO      5
#define STRUCT  6
#define OTHER   7

#define ALWAYS  01
#define NEVER   02
#define SOMETIMES       04

#define YES     1
#define NO      0

#define KEYWORD 1
#define DATADEF 2
#define SINIT   3

#define CLEVEL  20
#define IFLEVEL 10
#define DOLEVEL 10
#define OPLENGTH        10
#define LINE    256
#define LINELENG        120
#define MAXTABS 8
#define TABLENG 8
#define TEMP    1024

#define OUT     outs(clev->tabs); putchar('\n');opflag = lbegin = 1; count = 0
#define OUTK    OUT; keyflag = 0;
#define BUMP    clev->tabs++; clev->pdepth++
#define UNBUMP  clev->tabs -= clev->pdepth; clev->pdepth = 0
#define eatspace()      while((cc=getch()) == ' ' || cc == '\t'); unget(cc)
#define eatallsp()      while((cc=getch()) == ' ' || cc == '\t' || cc == '\n'); unget(cc)

struct indent {         /* one for each level of { } */
	int tabs;
	int pdepth;
	int iflev;
	int ifc[IFLEVEL];
	int spdepth[IFLEVEL];
} ind[CLEVEL];
struct indent *clev = ind;
struct keyw {
	char    *name;
	char    punc;
	char    type;
} key[] = {
	"switch", ' ', OTHER,
	"do", ' ', DO,
	"while", ' ', OTHER,
	"if", ' ', IF,
	"for", ' ', OTHER,
	"else", ' ', ELSE,
	"case", ' ', CASE,
	"default", ' ', CASE,
	"char", '\t', TYPE,
	"int", '\t', TYPE,
	"short", '\t', TYPE,
	"long", '\t', TYPE,
	"unsigned", '\t', TYPE,
	"float", '\t', TYPE,
	"double", '\t', TYPE,
	"struct", ' ', STRUCT,
	"union", ' ', STRUCT,
	"extern", ' ', TYPE,
	"register", ' ', TYPE,
	"static", ' ', TYPE,
	"typedef", ' ', TYPE,
	"const", ' ', TYPE,
	"enum", ' ', TYPE,
	"signed",' ', TYPE,
	"void",'\t',TYPE,
	"volatile", ' ', TYPE,
	0, 0, 0
};
struct keyw *lookup();
struct op {
	char    *name;
	char    blanks;
	char    setop;
} op[] = {
	"+=",   ALWAYS,  YES,
	"-=",   ALWAYS,  YES,
	"*=",   ALWAYS,  YES,
	"/=",   ALWAYS,  YES,
	"%=",   ALWAYS,  YES,
	">>=",  ALWAYS,  YES,
	"<<=",  ALWAYS,  YES,
	"&=",   ALWAYS,  YES,
	"^=",   ALWAYS,  YES,
	"|=",   ALWAYS,  YES,
	">>",   ALWAYS,  YES,
	"<<",   ALWAYS,  YES,
	"<=",   ALWAYS,  YES,
	">=",   ALWAYS,  YES,
	"==",   ALWAYS,  YES,
	"!=",   ALWAYS,  YES,
	"=",    ALWAYS,  YES,
	"&&",   ALWAYS, YES,
	"||",   ALWAYS, YES,
	"++",   NEVER, NO,
	"--",   NEVER, NO,
	"->",   NEVER, NO,
	"<",    ALWAYS, YES,
	">",    ALWAYS, YES,
	"+",    ALWAYS, YES,
	"/",    ALWAYS, YES,
	"%",    ALWAYS, YES,
	"^",    ALWAYS, YES,
	"|",    ALWAYS, YES,
	"!",    NEVER, YES,
	"~",    NEVER, YES,
	"*",    SOMETIMES, YES,
	"&",    SOMETIMES, YES,
	"-",    SOMETIMES, YES,
	"?",    ALWAYS,YES,
	":",    ALWAYS,YES,
	0,      0,0
};
FILE *input = stdin;
char *getnext();
char puttmp();
int     strict = 0;
int     join    = 0;
int     opflag = 1;
int     keyflag = 0;
int     paren    = 0;
int     split    = 0;
int     folded  = 0;
int     dolevel =0;
int     dotabs[DOLEVEL];
int     docurly[DOLEVEL];
int     dopdepth[DOLEVEL];
int     structlev = 0;
int     question         = 0;
char    string[LINE];
char    *lastlook;
char    *p = string;
char *temp;		/* Dynamic buffer allocation */
char *tp;
int err = 0;
char *lastplace;
char *tptr;
int maxleng     = LINELENG;
int maxtabs     = MAXTABS;
int count       = 0;
char next = '\0';
int     inswitch        =0;
int     lbegin   = 1;
int	temp_size = TEMP;	/* size of current dynamic buffer */
main(argc, argv) int    argc;
char    *argv[];
{
	char *num_ptr;


        setlocale(LC_ALL, "");	/* 001 Use system defaults for char types */
catd = catopen(MF_CB, NL_CAT_LOCALE);

	while (--argc > 0 && (*++argv)[0] == '-'){
		switch ((*argv)[1]){
		case 's':
			strict = 1;
			continue;
		case 'j':
			join = 1;
			continue;
		case 'l':
			if ( (*argv)[2] == '\0' && argc > 1) { 
				num_ptr = *++argv;
				do {
					if ( isdigit(*num_ptr) ) num_ptr++;
					else {
						fprintf(stderr, MSGSTR(NONUM,
						    "The -l flag must be followed by a number.\n"));
						exit(1);
					}
				} while (*num_ptr);
				maxleng = atoi(*argv);
				argc--;
			}
			else if ( isdigit((*argv)[2]) ) {
				num_ptr = &((*argv)[3]);
				do {
					if ( isdigit(*num_ptr) ) num_ptr++;
					else {
						fprintf(stderr, MSGSTR(NONUM,
						    "The -l flag must be followed by a number.\n"));
						exit(1);
					}
				} while (*num_ptr);
				maxleng = atoi(&(*argv)[2]);
			}
			else {
				fprintf(stderr, MSGSTR(NONUM,
				    "The -l flag must be followed by a number.\n"));
				exit(1);
			}
			maxtabs = maxleng/TABLENG - 2;
                        maxleng -= maxleng/10;
			continue;
		default:
			fprintf(stderr, MSGSTR(BADOPTION,
				"cb: illegal option %c\n"), (*argv)[1]);
			exit(1);
		}
	}
	if (argc <= 0)work();
	else {
		while (argc-- > 0){
			if ((input = fopen( *argv, "r")) == NULL){
				fprintf(stderr, MSGSTR(BADOPEN,
				  "cb: cannot open input file %s\n"), *argv);
				exit(1);
			}
			work();
			argv++;
		}
	}
	return(0);
}
work(){
	register int c;
	register struct keyw *lptr;
	char *pt;
	char cc;
	int ct, save_paren, save_split;

	/* initialize/allocate dynamic buffer and pointers */
	temp = (char *) malloc((unsigned) temp_size);
	lastplace = tptr = temp;

	while ((c = getch()) != EOF){
		switch (c){
		case '{':
			if ((lptr = lookup(lastlook,p)) != 0){
				if (lptr->type == ELSE)gotelse();
				else if(lptr->type == DO)gotdo();
				else if(lptr->type == STRUCT)structlev++;
			}
			if(++clev >= &ind[CLEVEL-1]){
				fprintf(stderr,MSGSTR(CURLY,
				      "too many levels of curly brackets\n"));
				clev = &ind[CLEVEL-1];
			}
			clev->pdepth = 0;
			clev->tabs = (clev-1)->tabs;
			clearif(clev);
			if(strict && clev->tabs > 0)
				putspace(' ',NO);
			putch(c,NO);
			getnl();
			if(keyflag == DATADEF){
				OUT;
			}
			else {
				OUTK;
			}
			clev->tabs++;
			pt = getnext(0);
			/* to handle initialized structures */
			if(*pt == '{'){         /* hide one level of {} */
				while((c=getch()) != '{');
				putch(c,NO);
				if(strict){
					putch(' ',NO);
					eatspace();
				}
				keyflag = SINIT;
			}
			continue;
		case '}':
			pt = getnext(0);
			/* to handle initialized structures */
			if(*pt == ','){
				if(strict){
					putspace(' ',NO);
					eatspace();
				}
				putch(c,NO);
				putch(*pt,NO);
				*pt = '\0';
				ct = getnl();
				pt = getnext(0);
				if(*pt == '{'){
					OUT;
					while((cc = getch()) != '{');
					putch(cc,NO);
					if(strict){
						putch(' ',NO);
						eatspace();
					}
					pt = getnext(0);
					continue;
				}
				else if(strict || ct){
					OUT;
				}
				continue;
			}
			else if(keyflag == SINIT && *pt == '}'){
				if(strict)
					putspace(' ',NO);
				putch(c,NO);
				getnl();
				OUT;
				keyflag = DATADEF;
				*pt = '\0';
				pt = getnext(0);
			}
			outs(clev->tabs);
			if(--clev < ind)clev = ind;
			ptabs(clev->tabs);
			putch(c,NO);
			lbegin = 0;
			lptr=lookup(pt,lastplace+1);
			c = *pt;
			if(*pt == ';' || *pt == ','){
				putch(*pt,NO);
				*pt = '\0';
				lastplace=pt;
			}
			ct = getnl();
			if((dolevel && clev->tabs <= dotabs[dolevel]) ||
				(structlev ) || (lptr != 0 &&
				lptr->type == ELSE&& clev->pdepth == 0)){
				if(c == ';'){
					OUTK;
				}
				else if(strict || (lptr != 0 &&
					lptr->type == ELSE && ct == 0)){
					putspace(' ',NO);
					eatspace();
				}
				else if(lptr != 0 && lptr->type == ELSE){
					OUTK;
				}
				if(structlev){
					structlev--;
					keyflag = DATADEF;
				}
			}
			else {
				OUTK;
				if(strict && clev->tabs == 0){
					if((c=getch()) != '\n'){
						putchar('\n');
						putchar('\n');
						unget(c);
					}
					else {
						putchar('\n');
						if((c=getch()) != '\n')
							unget(c);
						putchar('\n');
					}
				}
			}
			if(lptr != 0 && lptr->type == ELSE &&
				clev->pdepth != 0){
				UNBUMP;
			}
			if(lptr == 0 || lptr->type != ELSE){
				clev->iflev = 0;
				if(dolevel && docurly[dolevel] == NO &&
					clev->tabs == dotabs[dolevel]+1)
					clev->tabs--;
				else if(clev->pdepth != 0){
					UNBUMP;
				}
			}
			continue;
		case '(':
			paren++;
			if ((lptr = lookup(lastlook,p)) != 0){
				if(!(lptr->type == TYPE ||
					lptr->type == STRUCT))keyflag=KEYWORD;
				if (strict){
					putspace(lptr->punc,NO);
					opflag = 1;
				}
				putch(c,NO);
				if (lptr->type == IF)gotif();
			}
			else {
				putch(c,NO);
				lastlook = p;
				opflag = 1;
			}
			continue;
		case ')':
			if(--paren < 0)paren = 0;
			putch(c,NO);
			if((lptr = lookup(lastlook,p)) != 0){
				if(lptr->type == TYPE || lptr->type == STRUCT)
					opflag = 1;
			}
			else if(keyflag == DATADEF)opflag = 1;
			else opflag = 0;
			outs(clev->tabs);
			pt = getnext(1);
			if ((ct = getnl()) == 1 && !strict){
				if(dolevel && clev->tabs <= dotabs[dolevel])
					resetdo();
				if(clev->tabs > 0 && (paren != 0 ||
					keyflag == 0)){
					if(join){
						eatspace();
						putch(' ',YES);
						continue;
					} else {
						OUT;
						split = 1;
						continue;
					}
				}
				else if(clev->tabs > 0 && *pt != '{'){
					BUMP;
				}
				OUTK;
			}
			else if(strict){
				if(clev->tabs == 0){
					if(*pt != ';' && *pt != ',' &&
						*pt != '(' && *pt != '['){
						OUTK;
					}
				}
				else {
					if(keyflag == KEYWORD && paren == 0){
						if(dolevel && clev->tabs
							<= dotabs[dolevel]){
							resetdo();
							eatspace();
							continue;
						}
						if(*pt != '{'){
							BUMP;
							OUTK;
						}
						else {
							*pt='\0';
							eatspace();
							unget('{');
						}
					}
					else if(ct){
						if(paren){
							if(join){
								eatspace();
							} else {
								split = 1;
								OUT;
							}
						}
						else {
							OUTK;
						}
					}
				}
			}
			else if(dolevel && clev->tabs <= dotabs[dolevel])
				resetdo();
			continue;
		case ' ':
		case '\t':
			if ((lptr = lookup(lastlook,p)) != 0){
				if(!(lptr->type==TYPE||lptr->type==STRUCT))
					keyflag = KEYWORD;
				else if(paren == 0)keyflag = DATADEF;
				if(strict){
					if(lptr->type != ELSE){
						if(lptr->type == TYPE){
							if(paren != 0)
								putch(' ',YES);
						}
						else
							putch(lptr->punc,NO);
						eatspace();
					}
				}
				else putch(c,YES);
				switch(lptr->type){
				case CASE:
					outs(clev->tabs-1);
					continue;
				case ELSE:
					pt = getnext(1);
					eatspace();
					if((cc = getch()) == '\n' && !strict){
						unget(cc);
					}
					else {
						unget(cc);
						if(checkif(pt))continue;
					}
					gotelse();
					if(strict) unget(c);
					if(getnl() == 1 && !strict){
						OUTK;
						if(*pt != '{'){
							BUMP;
						}
					}
					else if(strict){
						if(*pt != '{'){
							OUTK;
							BUMP;
						}
					}
					continue;
				case IF:
					gotif();
					continue;
				case DO:
					gotdo();
					pt = getnext(1);
					if(*pt != '{'){
						eatallsp();
						OUTK;
						docurly[dolevel] = NO;
						dopdepth[dolevel]=clev->pdepth;
						clev->pdepth = 0;
						clev->tabs++;
					}
					continue;
				case TYPE:
					if(paren)continue;
					if(!strict)continue;
					gottype(lptr);
					continue;
				case STRUCT:
					gotstruct();
					continue;
				}
			}
			else if (lbegin == 0 || p > string)
				if(strict)
					putch(c,NO);
				else putch(c,YES);
			continue;
		case ';':
			putch(c,NO);
			if(paren != 0){
				if(strict){
					putch(' ',YES);
					eatspace();
				}
				opflag = 1;
				continue;
			}
			outs(clev->tabs);
			pt = getnext(0);
			lptr=lookup(pt,lastplace+1);
			if(lptr == 0 || lptr->type != ELSE){
				clev->iflev = 0;
				if(clev->pdepth != 0){
					UNBUMP;
				}
				if(dolevel && docurly[dolevel] == NO &&
					clev->tabs <= dotabs[dolevel]+1)
					clev->tabs--;
/*
				else if(clev->pdepth != 0){
					UNBUMP;
				}
*/
			}
			getnl();
			OUTK;
			continue;
		case '\n':
			if ((lptr = lookup(lastlook,p)) != 0){
				pt = getnext(1);
				if (lptr->type == ELSE){
					if(strict)
						if(checkif(pt))continue;
					gotelse();
					OUTK;
					if(*pt != '{'){
						BUMP;
					}
				}
				else if(lptr->type == DO){
					OUTK;
					gotdo();
					if(*pt != '{'){
						docurly[dolevel] = NO;
						dopdepth[dolevel]=clev->pdepth;
						clev->pdepth = 0;
						clev->tabs++;
					}
				}
				else {
					OUTK;
					if(lptr->type == STRUCT)gotstruct();
				}
			}
			else if(p == string)putchar('\n');
			else {
				if(clev->tabs > 0 &&
					(paren != 0 || keyflag == 0)){
					if(join){
						putch(' ',YES);
						eatspace();
						continue;
					} else {
						OUT;
						split = 1;
						continue;
					}
				}
				else if(keyflag == KEYWORD){
					OUTK;
					continue;
				}
				OUT;
			}
			continue;
		case '"':
		case '\'':
			putch(c,NO);
			while ((cc = getch()) != c){
				putch(cc,NO);
#ifdef KJI
				if (NCisshift(cc)) {
					putch(getch(),NO);
					continue;
				}
#endif
				if (cc == '\\'){
#ifdef KJI
					cc = getch();
					putch(cc,NO);
					if (NCisshift(cc)) {
						putch(getch(),NO);
						continue;
					}
#else
					putch(getch(),NO);
#endif
				}
				if (cc == '\n'){
					outs(clev->tabs);
					lbegin = 1;
					count = 0;
				}
			}
			putch(cc,NO);
			opflag=0;
			if (getnl() == 1){
				unget('\n');
			}
			continue;
		case '\\':
			putch(c,NO);
			putch(getch(),NO);
			continue;
		case '?':
			question = 1;
			gotop(c);
			continue;
		case ':':
			if (question == 1){
				question = 0;
				gotop(c);
				continue;
			}
			putch(c,NO);
			if(structlev)continue;
			if ((lptr = lookup(lastlook,p)) != 0){
				if (lptr->type == CASE)outs(clev->tabs - 1);
			}
			else {
				lbegin = 0;
				outs(clev->tabs);
			}
			getnl();
			OUTK;
			continue;
		case '/':
			if ((cc = getch()) != '*'){
				unget(cc);
				gotop(c);
				continue;
			}
			putch(c,NO);
			putch(cc,NO);
			cc = comment(YES);
			if(getnl() == 1){
				if(cc == 0){
					OUT;
				}
				else {
					outs(0);
					putchar('\n');
					lbegin = 1;
					count = 0;
				}
				lastlook = 0;
			}
			continue;
		case '[':
			putch(c,NO);
			ct = 0;
			while((c = getch()) != ']' || ct > 0){
				putch(c,NO);
				if(c == '[')ct++;
				if(c == ']')ct--;
			}
			putch(c,NO);
			continue;
		case '#':
			putch(c,NO);
			while ((cc = getch()) != '\n'){
#ifdef KJI
				if (NCisshift(cc)) {
					putch(cc,NO);
					putch(getch(),NO);
					continue;
				}
#endif
				if (cc == '\\'){
					putch(cc,NO);
					cc = getch();
				}
				putch(cc,NO);
			}
			putch(cc,NO);
			if (strict)	/* If K+R style, then preprocessing
					 * directives should start at first
					 * column */
				lbegin = 0;
			save_paren = paren;
			paren = 0;
			save_split = split;
			split = 0;
			outs(clev->tabs);
			paren = save_paren;
			split = save_split;
			lbegin = 1;
			count = 0;
			continue;
		default:
			if (c == ','){
				opflag = 1;
				putch(c,YES);
				if (strict){
					if ((cc = getch()) != ' ')unget(cc);
					if(cc != '\n')putch(' ',YES);
				}
			}
			else if(isop(c))gotop(c);
			else {
				if(isalnum(c) && lastlook == 0)lastlook = p;
				putch(c,NO);
				if(keyflag != DATADEF)opflag = 0;
			}
		}
	}
}

gotif(){
	outs(clev->tabs);
	if(++clev->iflev >= IFLEVEL-1){
		fprintf(stderr,MSGSTR(IFDEEP,"too many levels of if\n"));
		clev->iflev = IFLEVEL-1;
	}
	clev->ifc[clev->iflev] = clev->tabs;
	clev->spdepth[clev->iflev] = clev->pdepth;
}

gotelse(){
	clev->tabs = clev->ifc[clev->iflev];
	clev->pdepth = clev->spdepth[clev->iflev];
	if(--(clev->iflev) < 0)clev->iflev = 0;
}

checkif(pt)
char *pt;
{
	register struct keyw *lptr;
	int cc;
	if((lptr=lookup(pt,lastplace+1))!= 0){
		if(lptr->type == IF){
			if(strict)putch(' ',YES);
			copy(lptr->name);
			*pt='\0';
			lastplace = pt;
			if(strict){
				putch(lptr->punc,NO);
				eatallsp();
			}
			clev->tabs = clev->ifc[clev->iflev];
			clev->pdepth = clev->spdepth[clev->iflev];
			keyflag = KEYWORD;
			return(1);
		}
	}
	return(0);
}
gotdo(){
	if(++dolevel >= DOLEVEL-1){
		fprintf(stderr,MSGSTR(DODEEP, "too many levels of do\n"));
		dolevel = DOLEVEL-1;
	}
	dotabs[dolevel] = clev->tabs;
	docurly[dolevel] = YES;
}
resetdo(){
	if(docurly[dolevel] == NO)
		clev->pdepth = dopdepth[dolevel];
	if(--dolevel < 0)dolevel = 0;
}
gottype(lptr)
struct keyw *lptr;
{
	char *pt;
	struct keyw *tlptr;
	int c;
	while(1){
		pt = getnext(1);
		if((tlptr=lookup(pt,lastplace+1))!=0){
			putch(' ',YES);
			copy(tlptr->name);
			*pt='\0';
			lastplace = pt;
			if(tlptr->type == STRUCT){
				putch(tlptr->punc,YES);
				gotstruct();
				break;
			}
			lptr=tlptr;
			continue;
		}
		else{
			putch(lptr->punc,NO);
			while((c=getch())== ' ' || c == '\t');
			unget(c);
			break;
		}
	}
}
gotstruct(){
	int c;
	int cc;
	char *pt;
	while((c=getch()) == ' ' || c == '\t')
		if(!strict)putch(c,NO);
	if(c == '{'){
		structlev++;
		unget(c);
		return;
	}
	if(isalpha(c)){
		putch(c,NO);
		while(isalnum(c=getch()))putch(c,NO);
	}
	unget(c);
	pt = getnext(1);
	if(*pt == '{')structlev++;
	if(strict){
		eatallsp();
		putch(' ',NO);
	}
}
gotop(c)
{
	char optmp[OPLENGTH];
	char *op_ptr;
	struct op *s_op;
	char *a, *b;
	op_ptr = optmp;
	*op_ptr++ = c;
	for (*op_ptr = getch(); isop(*op_ptr); *++op_ptr = getch());
	if(!strict)unget(*op_ptr);
	else if (*op_ptr != ' ')unget( *op_ptr);
	*op_ptr = '\0';
	s_op = op;
	b = optmp;
	while ((a = s_op->name) != 0){
		op_ptr = b;
		while ((*op_ptr == *a) && (*op_ptr != '\0')){
			a++;
			op_ptr++;
		}
		if (*a == '\0'){
			keep(s_op);
			opflag = s_op->setop;
			if (*op_ptr != '\0'){
				b = op_ptr;
				s_op = op;
				continue;
			}
			else break;
		}
		else s_op++;
	}
}
keep(o)
struct op *o;
{
	char    *s;
	int ok;
	ok = !strict;
	if (strict && ((o->blanks & ALWAYS)
	    || ((opflag == 0 && o->blanks & SOMETIMES) && clev->tabs != 0)))
		putspace(' ',YES);
	if (strlen(o->name)>0) {
		for(s=o->name; *(s+1) != '\0'; s++)
			putch(*s,NO);
		putch(*s,ok);
	}
	if (strict && ((o->blanks & ALWAYS)
	    || ((opflag == 0 && o->blanks & SOMETIMES) && clev->tabs != 0)))
		putch(' ',YES);
}
getnl(){
	register int ch;
	char *savp;
	int gotcmt;
	gotcmt = 0;
	savp = p;
	while ((ch = getch()) == '\t' || ch == ' ')putch(ch,NO);
	if (ch == '/'){
		if ((ch = getch()) == '*'){
			putch('/',NO);
			putch('*',NO);
			comment(NO);
			ch = getch();
			gotcmt=1;
		}
		else {
			if(inswitch)*(++lastplace) = ch;
			else {
				inswitch = 1;
				*lastplace = ch;
			}
			unget('/');
			return(0);
		}
	}
	if(ch == '\n'){
		if(gotcmt == 0)p=savp;
		return(1);
	}
	unget(ch);
	return(0);
}
ptabs(n){
	int     i;
	int num;
	if(n > maxtabs){
		if(!folded){
			printf(MSGSTR(FOLDED,
				"/* code folded from here */\n"));
			folded = 1;
		}
		num = n-maxtabs;
	}
	else {
		num = n;
		if(folded){
			folded = 0;
			printf(MSGSTR(UNFOLDING, "/* unfolding */\n"));
		}
	}
	for (i = 0; i < num; i++)putchar('\t');
}
outs(n){
	if (p > string){
		if (lbegin){
			ptabs(n);
			lbegin = 0;
			if (split == 1){
				split = 0;
				if (clev->tabs > 0)printf("    ");
			}
		}
		*p = '\0';
		printf("%s", string);
		lastlook = p = string;
	}
	else {
		if (lbegin != 0){
			lbegin = 0;
			split = 0;
		}
	}
}
putch(c,ok)
char c;
{
	register int cc;
	if(p < &string[LINE-1]){
		if(count+TABLENG*clev->tabs >= maxleng && ok && !folded){
			if(c != ' ')*p++ = c;
			OUT;
			split = 1;
			if((cc=getch()) != '\n')unget(cc);
		}
		else {
#ifdef KJI
			/* If we have the first byte of a two byte char
			 * and there is not room for the second byte,
			 * then output what we have and reuse the buffer.
			 */
			if (NCisshift(c) && (p+1 == &string[LINE-1])) {
				outs(clev->tabs);
				*p++ = c;
				count = 0;
			}
			else {
				*p++ = c;
				count++;
			}
#else
			*p++ = c;
			count++;
#endif
		}
	}
	else {
		outs(clev->tabs);
		*p++ = c;
		count = 0;
	}
}
struct keyw *
lookup(first, last)
char *first, *last;
{
	struct keyw *ptr;
	char    *cptr, *ckey, *k;

	if(first == last || first == 0)return(0);
	cptr = first;
	while (*cptr == ' ' || *cptr == '\t')cptr++;
	if(cptr >= last)return(0);
	ptr = key;
	while ((ckey = ptr->name) != 0){
		for (k = cptr; (*ckey == *k && *ckey != '\0'); k++, ckey++);
		if(*ckey=='\0' && (k==last|| (k<last && !isalnum(*k)))){
			opflag = 1;
			lastlook = 0;
			return(ptr);
		}
		ptr++;
	}
	return(0);
}
comment(ok)
{
	register int ch;
	int hitnl;

	hitnl = 0;
	while ((ch  = getch()) != EOF){
		putch(ch, NO);
		if (ch == '*'){
gotstar:
			if ((ch  = getch()) == '/'){
				putch(ch,NO);
				return(hitnl);
			}
			putch(ch,NO);
			if (ch == '*')goto gotstar;
		}
		if (ch == '\n'){
			if(ok && !hitnl){
				outs(clev->tabs);
			}
			else {
				outs(0);
			}
			lbegin = 1;
			count = 0;
			hitnl = 1;
		}
	}
	return(hitnl);
}
putspace(ch,ok)
char    ch;
{
	if(p == string)putch(ch,ok);
	else if (*(p - 1) != ch) putch(ch,ok);
}
getch(){
	register char c;
	if(inswitch){
		if(next != '\0'){
			c=next;
			next = '\0';
			return(c);
		}
		if(tptr <= lastplace){
			if(*tptr != '\0')return(*tptr++);
			else if(++tptr <= lastplace)return(*tptr++);
		}
		inswitch=0;
		lastplace = tptr = temp;
	}
	return(getc(input));
}
unget(c)
{
	if(inswitch){
		if(tptr != temp)
			*(--tptr) = c;
		else next = c;
	}
	else ungetc(c,input);
}
char *
getnext(must){
	int c;
	char *beg;
	int prect,nlct;
	prect = nlct = 0;
	if(tptr > lastplace){
		tptr = lastplace = temp;
		err = 0;
		inswitch = 0;
	}
	tp = beg = lastplace;
	if(inswitch && tptr <= lastplace)
		if (isalnum(*lastplace)||ispunct(*lastplace)||
			isop(*lastplace))return(lastplace);
space:
	while(isspace(c=getc(input)))puttmp(c,1);
	beg = tp;
	puttmp(c,1);
	if(c == '/'){
		if(puttmp((c=getc(input)),1) == '*'){
cont:
			while((c=getc(input)) != '*'){
				puttmp(c,0);
				if(must == 0 && c == '\n')
					if(nlct++ > 2)goto done;
			}
			puttmp(c,1);
	star:
			if(puttmp((c=getc(input)),1) == '/'){
				beg = tp;
				puttmp((c=getc(input)),1);
			}
			else if(c == '*')goto star;
			else goto cont;
		}
		else goto done;
	}
	if(isspace(c))goto space;
	if(c == '#' && tp > temp+1 && *(tp-2) == '\n'){
		if(prect++ > 2)goto done;
		while(puttmp((c=getc(input)),1) != '\n')
			if(c == '\\')puttmp(getc(input),1);
		goto space;
	}
	if(isalnum(c)){
		while(isalnum(c = getc(input)))puttmp(c,1);
		ungetc(c,input);
	}
done:
	puttmp('\0',1);
	lastplace = tp-1;
	inswitch = 1;
	return(beg);
}
copy(s)
char *s;
{
	while(*s != '\0')putch(*s++,NO);
}
clearif(cl)
struct indent *cl;
{
	int i;
	for(i=0;i<IFLEVEL-1;i++)cl->ifc[i] = 0;
}
char puttmp(c,keep)
char c;
{
	char *tmp_buf;

	if(tp < &temp[temp_size-120])
		*tp++ = c;
	else
	/* reach the high water mark of current buffer, expand the buffer */
	{
	    tmp_buf = (char *) realloc(temp, (unsigned) temp_size + TEMP);
	    if (tmp_buf != 0)
	    /* expand buffer successfully, then adjust size and pointers */
	    {
		temp_size += TEMP;
		tptr = tmp_buf + (tptr - temp);
		tp = tmp_buf + (tp - temp);
		lastplace = tmp_buf + (lastplace - temp);
		temp = tmp_buf;
		*tp++ = c;
	    }
	    else
	    /* no more memory available or fail in realloc function */
	    {
		if(keep){
			if(tp >= &temp[temp_size-1]){
				fprintf(stderr,MSGSTR(BIGCOMMENT,
				  "can't look past huge comment - quiting\n"));
				exit(1);
			}
			*tp++ = c;
		}
		else if(err == 0){
			err++;
			fprintf(stderr,MSGSTR(TRUNCATING,
				"truncating long comment\n"));
		}
	    }
	}
	return(c);
}

