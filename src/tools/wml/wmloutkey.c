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
#ifdef REV_INFO
#ifndef lint
static char SCCSID[] = "OSF/Motif: %W% %E%";
#endif /* lint */
#endif /* REV_INFO */
/******************************************************************************
*******************************************************************************
*
*  (c) Copyright 1989, 1990, OPEN SOFTWARE FOUNDATION, INC.
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS.
*  ALL RIGHTS RESERVED
*  
*  	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED
*  AND COPIED ONLY IN ACCORDANCE WITH THE TERMS OF SUCH LICENSE AND
*  WITH THE INCLUSION OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE OR
*  ANY OTHER COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE
*  AVAILABLE TO ANY OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF THE
*  SOFTWARE IS HEREBY TRANSFERRED.
*  
*  	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
*  NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY OPEN SOFTWARE
*  FOUNDATION, INC. OR ITS THIRD PARTY SUPPLIERS  
*  
*  	OPEN SOFTWARE FOUNDATION, INC. AND ITS THIRD PARTY SUPPLIERS,
*  ASSUME NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE ANY OF ITS
*  SOFTWARE .   OSF SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
*  KIND, AND OSF EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES, INCLUDING
*  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
*  FITNESS FOR A PARTICULAR PURPOSE.
*  
*  Notice:  Notwithstanding any other lease or license that may pertain to,
*  or accompany the delivery of, this computer software, the rights of the
*  Government regarding its use, reproduction and disclosure are as set
*  forth in Section 52.227-19 of the FARS Computer Software-Restricted
*  Rights clause.
*  
*  (c) Copyright 1989, 1990, Open Software Foundation, Inc.  Unpublished - all
*  rights reserved under the Copyright laws of the United States.
*  
*  RESTRICTED RIGHTS NOTICE:  Use, duplication, or disclosure by the
*  Government is subject to the restrictions as set forth in subparagraph
*  (c)(1)(ii) of the Rights in Technical Data and Computer Software clause
*  at DFARS 52.227-7013.
*  
*  Open Software Foundation, Inc.
*  11 Cambridge Center
*  Cambridge, MA   02142
*  (617)621-8700
*  
*  RESTRICTED RIGHTS LEGEND:  This computer software is submitted with
*  "restricted rights."  Use, duplication or disclosure is subject to the
*  restrictions as set forth in NASA FAR SUP 18-52.227-79 (April 1985)
*  "Commercial Computer Software- Restricted Rights (April 1985)."  Open
*  Software Foundation, Inc., 11 Cambridge Center, Cambridge, MA  02142.  If
*  the contract contains the Clause at 18-52.227-74 "Rights in Data General"
*  then the "Alternate III" clause applies.
*  
*  (c) Copyright 1989, 1990, Open Software Foundation, Inc.
*  ALL RIGHTS RESERVED 
*  
*  
* Open Software Foundation is a trademark of The Open Software Foundation, Inc.
* OSF is a trademark of Open Software Foundation, Inc.
* OSF/Motif is a trademark of Open Software Foundation, Inc.
* Motif is a trademark of Open Software Foundation, Inc.
* DEC is a registered trademark of Digital Equipment Corporation
* DIGITAL is a registered trademark of Digital Equipment Corporation
* X Window System is a trademark of the Massachusetts Institute of Technology
*
*******************************************************************************
******************************************************************************/

/*
 * This module contains routines responsible for writing the .h files which
 * define the UIL lexer's keyword (token) tables. All files are written
 * into the current directory.
 *
 * Input:
 *	The resolved objects
 *	.dat files required to specify objects defined in Uil.y:
 *		keyword.dat
 *		reserved.dat
 *
 * Output:
 *	UilKeyTab.h
 *
 */


#include "wml.h"

#ifdef NULL
#undef NULL
#endif
#include <stdio.h>


/*
 * Routines used only in this module
 */
void wmlKeyWBuildTables ();
void wmlKeyWClassTokens ();
void wmlKeyWGrammarTokens ();
int wmlKeyWGrammarClass ();
void wmlKeyWArgTokens ();
void wmlKeyWReasonTokens ();
void wmlKeyWCharsetTokens ();
void wmlKeyWEnumvalTokens ();
void wmlKeyWMakeTokens ();
void wmlOutputUilKeyTab ();
void wmlOutputUilKeyTabBody ();
void wmlTokenClassString ();
void wmlTokenSymKString ();
void wmlTokenTokenString ();
void wmlOutputUilTokName ();

/*
 * globals
 */

static char		*canned_warn =
"/*\n\
**\tThis file is automatically generated.  Do not edit it by hand.\n\
**/\n\n";

#define	GrTokenMax		200		/* limit on grammar tokens */
static int			grtok_max_val = 0;
static WmlGrammarTokenPtr	grtok_vec[GrTokenMax];



/*
 * Output control routine
 */

void wmlOutputKeyWordFiles ()

{

wmlKeyWBuildTables ();
wmlOutputUilKeyTab ();
wmlOutputUilTokName ();

}



/*
 * Routine to construct token tables for building UilKeyTab.h
 *
 * This routine constructs token entries for each token class which appears
 * in UilKeyTab.h (the token classes are defined in UilKeyDef.h). These
 * tokens are defined both by WML objects in object vectors, and by
 * tokens defined in Uil.y.
 */

void wmlKeyWBuildTables ()

{

int		ndx;


/*
 * Initialize the token vectors
 */
wmlInitHList (wml_tok_sens_ptr, 1000, TRUE);
wmlInitHList (wml_tok_insens_ptr, 1000, TRUE);
for ( ndx=0 ; ndx<GrTokenMax ; ndx++ )
    grtok_vec[ndx] = NULL;

/*
 * Read and enter the tokens from Uil.y (via tokens.dat)
 */
wmlKeyWGrammarTokens ();

/*
 * Enter the class, argument, reason, charset, and enumval tokens
 */
wmlKeyWClassTokens ();
wmlKeyWArgTokens ();
wmlKeyWReasonTokens ();
wmlKeyWCharsetTokens ();
wmlKeyWEnumvalTokens ();

}



/*
 * Routine to read and enter tokens defined in Uil.y into the token tables.
 */

void wmlKeyWGrammarTokens ()

{

FILE			*infil;		/* input file (tokens.dat) */
int			scanres;	/* result of fscanf */
int			lineno = 1;	/* current line number */
char			token[100];	/* current token */
char			class[100];	/* current class */
int			tokval;		/* current token id (as value) */
WmlGrammarTokenPtr	grtok;		/* new grammar token */
int			ndx;
char			sens_name[100];	/* for case-insensitive name */


/*
 * Read tokens.dat. Recognize and save all tokens. Some are saved in the
 * global tokens vector so they will be put into UilKeyTab.h. All are
 * saved in an ordered vector to write UilTokName.h
 *
 * Special handling is required for tokens whose yacc definition conflicts
 * with common literals. In these cases, the token literal does not map
 * directly to its keyword in the language, and must be mapped as a special
 * case:
 *	UILTRUE		-> true/TRUE
 *	UILFALSE	-> false/FALSE
 *	UILfile	 	-> file/FILE
 *	UILeof	 	-> eof/EOF
 */
#ifdef VMS
infil = fopen ("SYS$LIBRARY:DECW$WML_TOKENS.DAT", "r");
#else
infil = fopen ("tokens.dat", "r");
#endif
if ( infil == NULL )
    {
#ifdef VMS
    printf ("\"Couldn't open DECW$WML_TOKENS.DAT");
#else
    printf ("\"Couldn't open tokens.dat");
#endif
    return;
    }

while ( TRUE )
    {
    scanres = fscanf (infil, "%s %d %s", token, &tokval, class);
    if ( scanres == EOF ) break;
    if ( scanres != 3 )
	{
#ifdef VMS
	printf ("\nBadly formatted at line %d in DECW$WML_TOKENS.DAT", lineno);
#else
	printf ("\nBadly formatted at line %d in tokens.dat", lineno);
#endif
	continue;
	}
    lineno += 1;

/*
 * Convert the token class, and construct a grammar token.
 */
    grtok = (WmlGrammarTokenPtr) malloc(sizeof(WmlGrammarToken));
    grtok->class = wmlKeyWGrammarClass (class);
    grtok->token = wmlAllocateString (token);
    grtok->val = tokval;

/*
 * Save the token in the grammar token vector, indexed by its value
 * (for UilTokName.h)
 */
if ( grtok->val < GrTokenMax )
    {
    grtok_vec[grtok->val] = grtok;
    if ( grtok->val > grtok_max_val )
	grtok_max_val = grtok->val;
    }
else
    printf ("\nToken id %d for %s exceed GrTokenMax",
	    grtok->val, grtok->token);

/*
 * Enter tokens which appear in the keyword tables as keyword tokens.
 * These have their lower case names entered as the case-insensitive
 * keyword token string. Do special token literal mapping.
 */
    switch ( grtok->class )
	{
	case WmlTokenClassKeyword:
	case WmlTokenClassReserved:
	    strcpy (sens_name, grtok->token);
	    for ( ndx=0 ; ndx<strlen(sens_name) ; ndx++ )
		sens_name[ndx] = _lower(sens_name[ndx]);
	    if ( strcmp(sens_name,"uiltrue") == 0 )
		strcpy (sens_name, "true");
	    if ( strcmp(sens_name,"uilfalse") == 0 )
		strcpy (sens_name, "false");
	    if ( strcmp(sens_name,"uilfile") == 0 )
		strcpy (sens_name, "file");
	    if ( strcmp(sens_name,"uileof") == 0 )
		strcpy (sens_name, "eof");
	    wmlKeyWMakeTokens (sens_name, grtok->class, (ObjectPtr)grtok);
	}
    }

fclose (infil);

}



/*
 * This routine translates a string identifying a token class into
 * its matching internal literal.
 */
int wmlKeyWGrammarClass (token)
    char		*token;

{

if ( strcmp(token,"argument") == 0 )
    return WmlTokenClassArgument;
if ( strcmp(token,"charset") == 0 )
    return WmlTokenClassCharset;
if ( strcmp(token,"color") == 0 )
    return WmlTokenClassColor;
if ( strcmp(token,"enumval") == 0 )
    return WmlTokenClassEnumval;
if ( strcmp(token,"font") == 0 )
    return WmlTokenClassFont;
if ( strcmp(token,"identifier") == 0 )
    return WmlTokenClassIdentifier;
if ( strcmp(token,"keyword") == 0 )
    return WmlTokenClassKeyword;
if ( strcmp(token,"literal") == 0 )
    return WmlTokenClassLiteral;
if ( strcmp(token,"reason") == 0 )
    return WmlTokenClassReason;
if ( strcmp(token,"reserved") == 0 )
    return WmlTokenClassReserved;
if ( strcmp(token,"special") == 0 )
    return WmlTokenClassSpecial;
if ( strcmp(token,"unused") == 0 )
    return WmlTokenClassUnused;
if ( strcmp(token,"class") == 0 )
    return WmlTokenClassClass;

printf ("\nUnrecognized token class %s", token);
return 0;

}



/*
 * Routine to process the class objects and enter them in the token tables.
 * Aliases are also entered, under their own names.
 */

void wmlKeyWClassTokens ()

{

int			ndx;		/* loop index */
WmlClassDefPtr		clsobj;		/* class object */
WmlSynClassDefPtr	synobj;		/* syntactic object */


/*
 * Make tokens for all class entries
 */
for ( ndx=0 ; ndx<wml_obj_class_ptr->cnt ; ndx++ )
    {
    clsobj = (WmlClassDefPtr) wml_obj_class_ptr->hvec[ndx].objptr;
    synobj = clsobj->syndef;
    wmlKeyWMakeTokens (synobj->name, WmlTokenClassClass, (ObjectPtr)clsobj);
    }

}



/*
 * Routine to process the argument objects and enter them in the token tables.
 * Aliases are also entered, under their own names.
 */

void wmlKeyWArgTokens ()

{

int			ndx;		/* loop index */
WmlResourceDefPtr	resobj;		/* resource object */
WmlSynResourceDefPtr	synobj;		/* syntactic object */
int			alias_ndx;	/* alias loop index */


/*
 * Make tokens for all argument entries
 */
for ( ndx=0 ; ndx<wml_obj_arg_ptr->cnt ; ndx++ )
    {
    resobj = (WmlResourceDefPtr) wml_obj_arg_ptr->hvec[ndx].objptr;
    synobj = resobj->syndef;
    wmlKeyWMakeTokens (synobj->name, WmlTokenClassArgument, (ObjectPtr)resobj);
    for ( alias_ndx=0 ; alias_ndx<synobj->alias_cnt ; alias_ndx++ )
	wmlKeyWMakeTokens (synobj->alias_list[alias_ndx],
			   WmlTokenClassArgument,
			   (ObjectPtr)resobj);
    }

}



/*
 * Routine to process the reason objects and enter them in the token tables.
 */

void wmlKeyWReasonTokens ()

{

int			ndx;		/* loop index */
WmlResourceDefPtr	resobj;		/* resource object */
WmlSynResourceDefPtr	synobj;		/* syntactic object */
int			alias_ndx;	/* alias loop index */


/*
 * Make tokens for all reason entries
 */
for ( ndx=0 ; ndx<wml_obj_reason_ptr->cnt ; ndx++ )
    {
    resobj = (WmlResourceDefPtr) wml_obj_reason_ptr->hvec[ndx].objptr;
    synobj = resobj->syndef;
    wmlKeyWMakeTokens (synobj->name, WmlTokenClassReason, (ObjectPtr)resobj);
    for ( alias_ndx=0 ; alias_ndx<synobj->alias_cnt ; alias_ndx++ )
	wmlKeyWMakeTokens (synobj->alias_list[alias_ndx],
			   WmlTokenClassReason,
			   (ObjectPtr)resobj);
    }

}



/*
 * Routine to process the charset objects and enter them in the token tables.
 */

void wmlKeyWCharsetTokens ()

{

int			ndx;		/* loop index */
WmlCharSetDefPtr	csobj;		/* character set object */
WmlSynCharSetDefPtr	synobj;		/* syntactic object */
int			alias_ndx;	/* alias loop index */


/*
 * Make tokens for all charset entries
 */
for ( ndx=0 ; ndx<wml_obj_charset_ptr->cnt ; ndx++ )
    {
    csobj = (WmlCharSetDefPtr) wml_obj_charset_ptr->hvec[ndx].objptr;
    synobj = csobj->syndef;
    wmlKeyWMakeTokens (synobj->name, WmlTokenClassCharset, (ObjectPtr)csobj);
    for ( alias_ndx=0 ; alias_ndx<synobj->alias_cnt ; alias_ndx++ )
	wmlKeyWMakeTokens (synobj->alias_list[alias_ndx],
			   WmlTokenClassCharset,
			   (ObjectPtr)csobj);
    }

}



/*
 * Routine to process the enumval objects and enter them in the token tables.
 */

void wmlKeyWEnumvalTokens ()

{

int			ndx;		/* loop index */
WmlEnumValueDefPtr	esobj;		/* enumeration value object */
WmlSynEnumValueDefPtr	synobj;		/* syntactic object */


/*
 * Make tokens for all enumval entries
 */
for ( ndx=0 ; ndx<wml_obj_enumval_ptr->cnt ; ndx++ )
    {
    esobj = (WmlEnumValueDefPtr) wml_obj_enumval_ptr->hvec[ndx].objptr;
    synobj = esobj->syndef;
    wmlKeyWMakeTokens (synobj->name, WmlTokenClassEnumval, (ObjectPtr)esobj);
    }

}



/*
 * Routine to create tokens and enter them in the token list.
 *
 * This routine constructs a case-sensitive and a case-insensitive token
 * and enters them the token vectors.
 */
void wmlKeyWMakeTokens (sens_name, class, obj)
    char		*sens_name;
    int			class;
    ObjectPtr		obj;

{

WmlKeyWTokenPtr		senstok;	/* case-sensitive token */
WmlKeyWTokenPtr		insenstok;	/* case-insensitive token */
char			insens_name[100];
int			ndx;


/*
 * Create both tokens, with one having an upper-case name. The names are
 * entered only in the order vector, not in the token itself.
 */
senstok = (WmlKeyWTokenPtr) malloc (sizeof(WmlKeyWToken));
insenstok = (WmlKeyWTokenPtr) malloc (sizeof(WmlKeyWToken));
senstok->class = class;
senstok->objdef = obj;
insenstok->class = class;
insenstok->objdef = obj;

strcpy (insens_name, sens_name);
for ( ndx=0 ; ndx<strlen(insens_name) ; ndx++ )
    insens_name[ndx] = _upper (insens_name[ndx]);

wmlInsertInHList (wml_tok_sens_ptr, sens_name, senstok);
wmlInsertInHList (wml_tok_insens_ptr, insens_name, insenstok);

}


/*
 * Routine to output UilKeyTab.h
 *
 * This routine dumps the tokens defined in the token tables into
 * UilKeyTab.h. Both the case-sensitive and case-insensitive token
 * lists are used.
 */

void wmlOutputUilKeyTab ()

{

char			*canned1 =
"\n/*    case sensitive keyword table    */\n\
static key_keytable_entry_type key_table_vec[] =\n\
  {\n";

char			*canned2 =
"  };\n\
externaldef(uil_sym_glbl) key_keytable_entry_type *key_table =\n\
\t\tkey_table_vec;\n\n\
/*    Maximum length of a keyword, and table size    */\n\
externaldef(uil_sym_glbl) int key_k_keyword_max_length = %d;\n\
externaldef(uil_sym_glbl) int key_k_keyword_count = %d;\n\n\
/*    case insensitive keyword table    */\n\
static key_keytable_entry_type key_table_case_ins_vec[] =\n\
  {\n";

char			*canned3 =
"  };\n\
externaldef(uil_sym_glbl) key_keytable_entry_type *key_table_case_ins =\n\
\t\tkey_table_case_ins_vec;\n";


FILE			*outfil;	/* output file */
int			ndx;		/* loop index */
int			maxlen = 0;	/* max keyword length */
int			maxkey = 0;	/* # entries in keyword table */


/*
 * Open the output file.
 */
outfil = fopen ("UilKeyTab.h", "w");
fprintf (outfil, canned_warn);

/*
 * Print the case sensitive and insensitive tables
 */
fprintf (outfil, canned1);
wmlOutputUilKeyTabBody (outfil, wml_tok_sens_ptr, &maxlen, &maxkey);
fprintf (outfil, canned2, maxlen, maxkey);
wmlOutputUilKeyTabBody (outfil, wml_tok_insens_ptr, &maxlen, &maxkey);
fprintf (outfil, canned3);

/*
 * close the output file
 */
printf ("\nCreated UilKeyTab.h");
fclose (outfil);

}



/*
 * Routine to output the body of a keyword table
 */
void wmlOutputUilKeyTabBody (outfil, tokvec, maxlen, maxkey)
    FILE			*outfil;
    DynamicHandleListDefPtr	tokvec;
    int				*maxlen;
    int				*maxkey;

{

int			ndx;		/* loop index */
WmlKeyWTokenPtr		tok;		/* current token */
char			*tokstg;	/* string for token (keyword) */
char			tkclass[100];	/* token class string */
char			tksym[100];	/* token sym_k string */
char			tktoken[100];	/* token tkn_k_num string */


/*
 * Loop over all tokens, and put out an entry for each.
 */
for ( ndx=0 ; ndx<tokvec->cnt ; ndx++ )
    {
    tok = (WmlKeyWTokenPtr) tokvec->hvec[ndx].objptr;
    tokstg = tokvec->hvec[ndx].objname;
    wmlTokenClassString (tkclass, tok);
    wmlTokenSymKString (tksym, tok);
    wmlTokenTokenString (tktoken, tok);
    fprintf (outfil, "    {%s, %s, %d, %s, \"%s\"},\n",
	     tkclass,
	     tksym,
	     strlen(tokstg),
	     tktoken,
	     tokstg);
    if ( strlen(tokstg) > *maxlen )
	*maxlen = strlen (tokstg);
    *maxkey += 1;
    }

}



/*
 * Routine to return the string for a token class, tkn_k_class_...
 */
void wmlTokenClassString (dststg, tok)
    char		*dststg;
    WmlKeyWTokenPtr	tok;

{

switch ( tok->class )
    {
    case WmlTokenClassArgument:
        strcpy (dststg, "tkn_k_class_argument");
	return;
    case WmlTokenClassCharset:
        strcpy (dststg, "tkn_k_class_charset");
	return;
    case WmlTokenClassEnumval:
        strcpy (dststg, "tkn_k_class_enumval");
	return;
    case WmlTokenClassKeyword:
        strcpy (dststg, "tkn_k_class_keyword");
	return;
    case WmlTokenClassReason:
        strcpy (dststg, "tkn_k_class_reason");
	return;
    case WmlTokenClassReserved:
        strcpy (dststg, "tkn_k_class_reserved");
	return;
    case WmlTokenClassClass:
        strcpy (dststg, "tkn_k_class_class");
	return;
    }

}



/*
 * Routine to return the string for a sym_k_... for some object
 */
void wmlTokenSymKString (dststg, tok)
    char		*dststg;
    WmlKeyWTokenPtr	tok;

{

WmlClassDefPtr		clsobj;		/* class object */
WmlResourceDefPtr	resobj;		/* resource object */
WmlCharSetDefPtr	csobj;		/* character set object */
WmlEnumValueDefPtr	esobj;		/* enumeration value object */


switch ( tok->class )
    {
    case WmlTokenClassArgument:
	resobj = (WmlResourceDefPtr) tok->objdef;
	sprintf (dststg, "sym_k_%s_arg", resobj->tkname);
	return;
    case WmlTokenClassCharset:
	csobj = (WmlCharSetDefPtr) tok->objdef;
	sprintf (dststg, "sym_k_%s_charset", csobj->syndef->name);
	return;
    case WmlTokenClassEnumval:
	esobj = (WmlEnumValueDefPtr) tok->objdef;
	sprintf (dststg, "sym_k_%s_enumval", esobj->syndef->name);
	return;
    case WmlTokenClassKeyword:
        strcpy (dststg, "0");
	return;
    case WmlTokenClassReason:
	resobj = (WmlResourceDefPtr) tok->objdef;
	sprintf (dststg, "sym_k_%s_reason", resobj->tkname);
	return;
    case WmlTokenClassReserved:
        strcpy (dststg, "0");
	return;
    case WmlTokenClassClass:
        clsobj = (WmlClassDefPtr) tok->objdef;
	sprintf (dststg, "sym_k_%s_object", clsobj->tkname);
	return;
    }

}



/*
 * Routine to return the string for a token number, tkn_k_num_...
 */
void wmlTokenTokenString (dststg, tok)
    char		*dststg;
    WmlKeyWTokenPtr	tok;

{

WmlGrammarTokenPtr	grtok;		/* grammar token */


switch ( tok->class )
    {
    case WmlTokenClassArgument:
        strcpy (dststg, "ARGUMENT_NAME");
	return;
    case WmlTokenClassCharset:
        strcpy (dststg, "CHARSET_NAME");
	return;
    case WmlTokenClassEnumval:
        strcpy (dststg, "ENUMVAL_NAME");
	return;
    case WmlTokenClassReason:
        strcpy (dststg, "REASON_NAME");
	return;
    case WmlTokenClassKeyword:
    case WmlTokenClassReserved:
	grtok = (WmlGrammarTokenPtr) tok->objdef;
        strcpy (dststg, grtok->token);
	return;
    case WmlTokenClassClass:
        strcpy (dststg, "CLASS_NAME");
	return;
    }

}



/*
 * routine to output UilTokName.h
 */

void wmlOutputUilTokName ()

{

char			*canned1 =
"/*\tToken name table */\n\
static char *tok_token_name_table_vec[] = \n\
  {\n";

char			*canned2 =
"  };\n\
externaldef(uil_sym_glbl) char **tok_token_name_table =\n\
\t\ttok_token_name_table_vec;\n\n\
/*\tNumber of entries in table */\n\
externaldef(uil_sym_glbl) int tok_num_tokens = %d;\n";


FILE			*outfil;	/* output file */
int			ndx;		/* loop index */
WmlGrammarTokenPtr	grtok;		/* current grammar token */


/*
 * Open the output file.
 */
outfil = fopen ("UilTokName.h", "w");
fprintf (outfil, canned_warn);
fprintf (outfil, canned1);

/*
 * Print the token name entries
 */
for ( ndx=0 ; ndx<grtok_max_val ; ndx++ )
    {
    grtok = grtok_vec[ndx];
    if ( grtok != NULL )
	fprintf (outfil, "    \"%s\",\n", grtok->token);
    else
	fprintf (outfil, "    \"UNKNOWN_TOKEN\",\n");
    }

/*
 * close the output file
 */
fprintf (outfil, canned2, grtok_max_val);
printf ("\nCreated UilTokName.h");
fclose (outfil);

}
