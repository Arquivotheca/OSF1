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
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: C.c,v $ $Revision: 4.2.4.4 $ (OSF) $Date: 1993/12/10 20:42:27 $";
#endif
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 * 
 * C.c	5.4 (Berkeley) 6/1/90
 */

#include <stdio.h>
#include "ctags.h"

static	int 	func_entry(void);
static	void	hash_entry(void);
static	int	str_entry(int);
void		skip_comment(void);

/*
 * c_entries --
 *	read .c and .h files and call appropriate routines
 */
void
c_entries(void)
{
	extern int	tflag;		/* -t: create tags for typedefs */
	register int	c,		/* current character */
			level;		/* brace level */
	register char	*sp;		/* buffer pointer */
	int	token,			/* if reading a token */
		t_def,			/* if reading a typedef */
		t_level;		/* typedef's brace level */
	char	tok[MAXTOKEN];		/* token buffer */

	lineftell = ftell(inf);
	sp = tok; token = t_def = NO; t_level = -1; level = 0; lineno = 1;
	while (GETC(!=,EOF)) {

	switch ((char)c) {
		/*
		 * Here's where it DOESN'T handle:
		 *	foo(a)
		 *	{
		 *	#ifdef notdef
		 *		}
		 *	#endif
		 *		if (a)
		 *			puts("hello, world");
		 *	}
		 */
		case '{':
			++level;
			goto endtok;
		case '}':
			/*
			 * if level goes below zero, try and fix
			 * it, even though we've already messed up
			 */
			if (--level < 0)
				level = 0;
			goto endtok;

		case '\n':
			SETLINE;
			/*
			 * the above 3 cases are similar in that they
			 * are special characters that also end tokens.
			 */
endtok:			if (sp > tok) {
				*sp = EOS;
				token = YES;
				sp = tok;
			}
			else
				token = NO;
			continue;

		/* we ignore quoted strings and comments in their entirety */
		case '"':
		case '\'':
			(void)skip_key(c);
			break;

		/*
		 * comments can be fun; note the state is unchanged after
		 * return, in case we found:
		 *	"foo() XX comment XX { int bar; }"
		 */
		case '/':
			if (GETC(==,'*')) {
				skip_comment();
				continue;
			}
			(void)ungetc(c,inf);
			c = '/';
			goto storec;

		/* hash marks flag #define's. */
		case '#':
			if (sp == tok) {
				hash_entry();
				break;
			}
			goto storec;

		/*
	 	 * if we have a current token, parenthesis on
		 * level zero indicates a function.
		 */
		case '(':
			if (!level && token) {
				int	curline;

				if (sp != tok)
					*sp = EOS;
				/*
				 * grab the line immediately, we may
				 * already be wrong, for example,
				 *	foo\n
				 *	(arg1,
				 */
				getline();
				curline = lineno;
				if (func_entry()) {
					++level;
					pfnote(tok,curline);
				}
				break;
			}
			goto storec;

		/*
		 * semi-colons indicate the end of a typedef; if we find a
		 * typedef we search for the next semi-colon of the same
		 * level as the typedef.  Ignoring "structs", they are
		 * tricky, since you can find:
		 *
		 *	"typedef long time_t;"
		 *	"typedef unsigned int u_int;"
		 *	"typedef unsigned int u_int [10];"
		 *
		 * If looking at a typedef, we save a copy of the last token
		 * found.  Then, when we find the ';' we take the current
		 * token if it starts with a valid token name, else we take
		 * the one we saved.  There's probably some reasonable
		 * alternative to this...
		 */
		case ';':
			if (t_def && level == t_level) {
				t_def = NO;
				getline();
				if (sp != tok)
					*sp = EOS;
				pfnote(tok,lineno);
				break;
			}
			goto storec;

		/*
		 * store characters until one that can't be part of a token
		 * comes along; check the current token against certain
		 * reserved words.
		 */
		default:
storec:			if (!intoken(c)) {
				if (sp == tok)
					break;
				*sp = EOS;
				if (tflag) {
					/* no typedefs inside typedefs */
					if (!t_def && !bcmp(tok,"typedef",8)) {
						t_def = YES;
						t_level = level;
						break;
					}
					/* catch "typedef struct" */
					if ((!t_def || t_level < level)
					    && (!bcmp(tok,"struct",7)
					    || !bcmp(tok,"union",6)
					    || !bcmp(tok,"enum",5))) {
						/*
						 * get line immediately;
						 * may change before '{'
						 */
						getline();
						if (str_entry(c))
							++level;
						break;
					}
				}
				sp = tok;
			}
			else if (sp != tok || begtoken(c)) {
				*sp++ = c;
				token = YES;
			}
			continue;
		}
		sp = tok;
		token = NO;
	}
}

/*
 * func_entry --
 *	handle a function reference
 */
static int
func_entry(void)
{
	register int	c;		/* current character */

        /*
         * The skip_key function will come back positive
         * if some illegal C syntax is encountered.
         */
        if (skip_key((int)')'))
          return(NO);                           /*GA002*/

	/*
	 * we assume that the character after a function's right paren
	 * is a token character if it's a function and a non-token
	 * character if it's a declaration.  Comments don't count...
	 */
	for (;;) {
		while (GETC(!=,EOF) && iswhite(c))
			if (c == (int)'\n')
				SETLINE;
		if (intoken(c) || c == (int)'{')
			break;
		if (c == (int)'/' && GETC(==,'*'))
			skip_comment();
		else {				/* don't ever "read" '/' */
			(void)ungetc(c,inf);
			return(NO);
		}
        }
	if (c != (int)'{')
                (void)skip_key((int)'{');	
	return(YES);			    /* Now we go looking for     */
}					    /* the body of the function. */

/*
 * hash_entry --
 *	handle a line starting with a '#'
 */
static void
hash_entry(void)
{
	extern int	dflag;		/* -d: non-macro defines */
	register int	c,		/* character read */
			curline;	/* line started on */
	register char	*sp;		/* buffer pointer */
	char	tok[MAXTOKEN];		/* storage buffer */

	curline = lineno;
	for (sp = tok;;) {		/* get next token */
		if (GETC(==,EOF))
			return;
		if (iswhite(c))
			break;
		*sp++ = c;
	}
	*sp = EOS;
	if (bcmp(tok,"define",6))	/* only interested in #define's */
		goto skip;
	for (;;) {			/* this doesn't handle "#define \n" */
		if (GETC(==,EOF))
			return;
		if (!iswhite(c))
			break;
	}
	for (sp = tok;;) {		/* get next token */
		*sp++ = c;
		if (GETC(==,EOF))
			return;
		/*
		 * this is where it DOESN'T handle
		 * "#define \n"
		 */
		if (!intoken(c))
			break;
	}
	*sp = EOS;
	if (dflag || c == (int)'(') {	/* only want macros */
		getline();
		pfnote(tok,curline);
	}
skip:	if (c == (int)'\n') {		/* get rid of rest of define */
		SETLINE
		if (*(sp - 1) != '\\')
			return;
	}
	(void)skip_key((int)'\n');
}

/*
 * str_entry --
 *	handle a struct, union or enum entry
 * c is current character 
 */
static int
str_entry(int c)
{
	register char	*sp;		/* buffer pointer */
	int	curline;		/* line started on */
	char	tok[BUFSIZ];		/* storage buffer */

	curline = lineno;
	while (iswhite(c))
		if (GETC(==,EOF))
			return(NO);
	if (c == (int)'{')		/* it was "struct {" */
		return(YES);
	for (sp = tok;;) {		/* get next token */
		*sp++ = c;
		if (GETC(==,EOF))
			return(NO);
		if (!intoken(c))
			break;
	}
	switch ((char)c) {
		case '{':		/* it was "struct foo{" */
			--sp;
			break;
		case '\n':		/* it was "struct foo\n" */
			SETLINE;
			/*FALLTHROUGH*/
		default:		/* probably "struct foo " */
			while (GETC(!=,EOF))
				if (!iswhite(c))
					break;
			if (c != (int)'{') {
				(void)ungetc(c, inf);
				return(NO);
			}
	}
	*sp = EOS;
	pfnote(tok,curline);
	return(YES);
}

/*
 * skip_comment --
 *	skip over comment
 */
void
skip_comment(void)
{
	register int	c,		/* character read */
			star;		/* '*' flag */

	for (star = 0;GETC(!=,EOF);)
		switch((char)c) {
			/* comments don't nest, nor can they be escaped. */
			case '*':
				star = YES;
				break;
			case '/':
				if (star)
					return;
				break;
			case '\n':
				SETLINE;
				/*FALLTHROUGH*/
			default:
				star = NO;
		}
}

/*
 * skip_key --
 *	skip to next char "key"
 */
int
skip_key(int key)
{
	register int	c,
			skip,
			retval,
			paren = 0,
			counter = 0;

	for (skip = retval = NO; GETC(!=,EOF); ++counter)
		switch((char)c) {
                case ',':                  /* Trap for illegal C        */
                        if (counter == 0)  /* syntax which might be     */
                          retval = YES;    /* an unexpanded macro.      */
                        break;             /* Fix for QAR11597. (GA002) */

		case '\\':		/* a backslash escapes anything */
			skip = !skip;	/* we toggle in case it's "\\" */
			break;
		case ';':		/* special case for yacc; if one */
		case '|':		/* of these chars occurs, we may */
			retval = YES;	/* have moved out of the rule */
			break;		/* not used by C */
		case '(':
			paren++;	/* watch out for nested parens */
			break;
		case ')':
			if (key == ')' && paren == 0)
				return(retval);	/* found key, return */
			else
				paren--;	/* end of nested parens */
			break;
		case '\n':
			SETLINE;
			/*FALLTHROUGH*/
		default:
			if (c == key && !skip)
				return(retval);
			skip = NO;
		}
	return(retval);
}
