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
static char rcsid[] = "@(#)$RCSfile: filetype.c,v $ $Revision: 4.2.6.3 $ (DEC) $Date: 1993/08/03 10:26:42 $";
#endif
/*
 * HISTORY
 */

/**/
/*
 *
 *	File name:
 *
 *	    filetype.c
 *
 *	Source file description:
 *
 *		This file contains logic to determine the
 *		general type of file specified by the caller.
 *		ie. Is it binary, text, library, etc..
 *
 *		(further documentation below)
 *
 *		It is used by the Labeled Tape Facility (LTF)
 *		in order to determine the type that the Unix
 *		file should take on whence it becomes an
 *		ANSI tape file.
 *
 *
 *	Functions:
 *
 *	ascom()		...
 *	ccom()		...
 *	english()	...
 *	Filetype()	Top level logic to determine file type.
 *	lookupe()	...
 *	troffint()	Check for  troff intermediate file
 *
 *
 *	Compile:
 *
 *	    cc -O -c filetype.c		 <- For Ultrix-32/32m
 *
 *	    cc CFLAGS=-DU11-O filetype.c <- For Ultrix-11
 *
 *
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	  01.1		23-August-89	Sam Lambert
 *			Modified Filetype() to handle non-null terminated
 *			"ascii" files (U32_SPR #00095).
 *
 *	  01.0		09-April-85	Ray Glaser
 *			Create orginal version.
 *	
 */
/**/
/*
 * ->	Local includes
 */

#include "ltfdefs.h"	/* Common LTF definitions */

/*
 * ->	Local defines  "required for / defined by"  this module
 */

#define	MBUFSIZ	1024	/* Chunk of file to read for "typing" */
#define NL	012	/* Newline character */
#define	NLM	123	/* Used to check for 'c' code */

/*
 * ->	Globals for this module only
 */

char	*as[] = {
	"globl","byte","align","text","data","comm",0};

char	*asc[] = {
	"chmk","mov","tst","clr","jmp",0};

char	buf[MBUFSIZ];

char	*c[] = {
	"int","char","float","double","struct","extern",0};

char	*com[] = { "alias",
	"date","cc","xref","pr","pr50","CFLAGS","lpr","rm","FILES",
	"tar","mdtar","sync","make","/etc","/bin","for","case",
	"echo","do","umask","set","stty","setenv",
	00 };

int	errno;
char	*fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};

int	i  = 0;
FILE	*ifile;
int	in;
char	*troff[] = {	/* new troff intermediate lang */
	"x","T","res","init","font","202","V0","p1",0};

int	Type;	/* Tentative file type value */
/**/
/*
 *
 * Function:
 *
 *	Filetype
 *
 * Function Description:
 *
 *	This function attempts to determine the type of Unix disk file.
 *	It returns a generalized file type indication as defined in
 *	the include file "filetypes.h".
 *
 * Arguments:
 *
 *
 *	char	*file		Points to a null terminated string
 *				assumed to be the desired file name
 *				to be typed.
 *
 *	char	*caller		Points to a string defining the desired
 *				routine name for messages to stderr.
 *
 * Return value(s):
 *
 *	int	value
 *
 *	Zero if the file type could not be determined, else:
 *	one of the file types defined in "filetypes.h".
 *	Additionally, the character string variables "Tftypes"
 *	is filled with a 3 character representation of the
 *	true Ultrix disk file type. (see README.1 file).
 *	
 * 
 * Side Effects:
 *
 *	This function outputs error messages to stderr if any problems
 *	occur during the attempt to access/type the given file.
 *
 */

/**/
/*
 *	FILETYPE	Determination thereof..
 */

Filetype(file,caller)

	char	*caller;
	char	*file;

{
/*
 * ->	Local variables
 */

int	nl;
struct stat	mbuf;

/*------*\
   Code
\*------*/

strcpy(Tftypes,"???");	/* Assume we can't determine file type */
mbuf = Inode;
/* Check if this is a file that should not be open
*/
switch (mbuf.st_mode & S_IFMT) {
	case S_IFCHR:	/* Charater special */
		strcpy(Tftypes,"csp");
		return(CHCTRSP);

	case S_IFBLK:	/* Block special */
		strcpy(Tftypes,"bsp");
		return(BLKSP);

	case S_IFIFO:	/* Fifo - pipe */
		strcpy(Tftypes,"pip");
		return(CHCTRSP);
#ifndef U11
	case S_IFSOCK:	/* Socket ! */
		strcpy(Tftypes,"soc");
		return(SOCKET);
#endif
}/*E switch mbuf.st_mode & S_IFMT */
if((ifile = fopen(file, "r")) == NULL) {
	PERROR "\n%s: %s %s\n", caller, CANTOPW, file);
	perror(caller);
	return(EOF);
}
switch (mbuf.st_mode & S_IFMT) {

	case S_IFLNK:	/* Symbolic link */
		strcpy(Tftypes,"sym");
		fclose(ifile);
		return(SYMLNK);

	case S_IFDIR:	/* Directory */
		strcpy(Tftypes,"dir");
		fclose(ifile);
		return(DIRECT);

}/*E switch mbuf.st_mode & S_IFMT */

/*
 *	Read in a MBUFSIZ amount of data from the file for
 *	further examination.
 */
if ((in = read(fileno(ifile), buf, MBUFSIZ)) <= 0)
    if (in < 0) {
	PERROR "\n%s: %s %s\n", caller, CANTRD, file);
	perror(caller);
	exit(FAIL);
    }
    else { 
	fclose(ifile);
	strcpy(Tftypes,"nul");
	return(EMPTY);	/* File appears to be empty */
    }
/*
 *	This check is looking for files that are used as output
 *	of ltf instead of using tape.  NOTE:  This is bogus as
 *	the rest of all these checks are.  This is the bare 
 *	minimum of checking.  Should check not only for VOL1,
 *	but also HDR1, etc., tape mark, then EOF1, etc., tape
 *	mark.  Thus this checks for VOL1 in the first 4 characters,
 *	and makes the file type binary.
 */
if (!strncmp(buf, "VOL1", 4)) {
	fclose(ifile);
	strcpy(Tftypes,"bin");
	return(BINARY);	/* Data file of some type */
}
switch(*(int *)buf) {

	case 0407:	/* Old impure Format on Ultrix-11 should
			 * be PDP-11 normal (I space only).
			 * On Ultrix-32 - seems to indicate
			 * a  .o  (object) file. */

		fclose(ifile);
#ifdef U11
		/* check to see if relocation info is stripped */
		if (((int *)buf)[7])
		    strcpy(Tftypes,"exe");
		else
#endif
		    strcpy(Tftypes,"bin");
		return(BINARY);

	case 0410:	/* Read-only/shared text */
	case 0413:	/* Demand Load Format */
#ifdef U11
	case 0401:	/* PDP-11 Standalone executable */
	case 0411:	/* PDP-11 Separated I & D */
	case 0430:	/* PDP-11 7 Overlays */
	case 0431:	/* PDP-11 7 Overlays */
	case 0450:	/* PDP-11 15 Overlays */
	case 0451:	/* PDP-11 15 Overlays */
#endif
		fclose(ifile);
		strcpy(Tftypes,"exe");
		return(BINARY);	

	case 0177555:	/* Very old archive */
	case 0177545:	/* Old archive */
		fclose(ifile);
		strcpy(Tftypes,"oar");
		return(BINARY);	

	case 070707:	/* CPIO data */
		fclose(ifile);
		strcpy(Tftypes,"cpi");
		return(CPIO);

}/*E switch(*(int *)buf) */

if(strncmp(buf, "!<arch>\n__.SYMDEF", 17) == 0 ) {

	fclose(ifile);
	strcpy(Tftypes,"arl");
	return(BINARY);	/* Archive Random Library */
}
if (strncmp(buf, "!<arch>\n", 8)==0) {

	fclose(ifile);
	strcpy(Tftypes,"arc");
	return(BINARY);	/* Archive */
}
if (mbuf.st_size % 512 == 0) {	/* it may be a PRESS file */
	lseek(fileno(ifile), -512L, 2);	/* last block */
	if ((in = read(fileno(ifile), buf, MBUFSIZ)) <= 0) {
	    if (in < 0) {
		PERROR "\n%s: %s %s\n", caller, CANTRD, file);
		perror(caller);
		exit(FAIL);
	    }
	} 
	if (in > 0 && *(int *)buf == 12138) {
		fclose(ifile);
		strcpy(Tftypes,"cmp");
		return(BINARY);	/* Press file ..*/
	}
}/*E if mbuf.st_size ..*/

/*
 * See if it looks like a command file.
 */
i = 0;
while(buf[i] == ' ' || buf[i] == '#' || buf[i] == '!') {
	while(buf[i++] != '\n')
		if(i >= in)	goto notcom;

}/*E while buf[i] ..*/

#if 0 /*for debugging*/
printf("\ngoing to lookupe \n");
for (x=i;x<i+80;x++)
  printf("%c",buf[x]);
#endif 0

if (lookupe(com)==1) {
	Type = TEXT;
	strcpy(Tftypes,"com");
	goto outa;
}

notcom:
i = 0;
if (!ccom()) {
	goto notc;
}

while(buf[i] == '#') {
	j = i;
	while(buf[i++] != '\n') {
		if(i - j > 255) {
			fclose(ifile);
			strcpy(Tftypes,"adf");
			return(BINARY);	/* Data file of some type */
		}
		if(i >= in) {
			goto notc;
		}
	}/* while buf[i++] ...*/

	if(!ccom()) {
		goto notc;
	}
}/*E while buf[i] ..*/

check:

if(lookupe(c) == 1) {
	while((ch = buf[i++]) != ';' && ch != '{') if(i >= in) {
		goto notc;
	}
	strcpy(Tftypes,"cc ");
	Type = TEXT;	/* 'C' program text */
	goto outa;

}/*E if lookupe(c) */

/* Look for newline or null character */
if (!strchr(buf, '\n')) {
	lseek(fileno(ifile), -512L, L_XTND);  /* last block */
	if ((in = read(fileno(ifile), buf, MBUFSIZ)) < 0) {
		PERROR "\n%s: %s %s\n", caller, CANTRD, file);
		perror(caller);
		exit(FAIL);
	} 
	if ((!strchr(buf, '\n')) || (!strchr(buf, '\0'))) {
		fclose(ifile);
		strcpy(Tftypes,"bin");
		return(BINARY);
	}
	else {
		/* Re-read the first bit of the file for the following tests */
		lseek(fileno(ifile), 0L, L_SET);  /* beginning of file */
		if ((in = read(fileno(ifile), buf, MBUFSIZ)) < 0) {
			PERROR "\n%s: %s %s\n", caller, CANTRD, file);
			perror(caller);
			exit(FAIL);
		}
	}
	
}

nl = 0;

while(buf[i] != '(') {
	if(buf[i] <= 0)
		goto notas;
	if(buf[i] == ';') {
		i++; 
		goto check; 
	}
	if(buf[i++] == '\n') {
		if(nl++ > NLM) {
			goto notc;
		}
		if(i >= in) {
			goto notc;
		}
 	}
}/*E while(buf[i] ..*/

while(buf[i] != ')') {
	if(buf[i++] == '\n')
		if(nl++ > NLM) {
			goto notc;
		}
		if(i >= in) {
			goto notc;
		}

}/*E while buf[i] ..*/

while(buf[i] != '{') {
	if(buf[i++] == '\n')
		if(nl++ > NLM) {
			goto notc;
		}
		if(i >= in) {
			goto notc;
		}

}/*E while buf[i] ..*/

strcpy(Tftypes,"cc ");
Type = TEXT;	/* 'C' program text */
goto outa;

notc:

i = 0;
while(buf[i] == 'c' || buf[i] == '#') {

	while(buf[i++] != '\n')
		if(i >= in)	goto notfort;

}/*E while buf[i] ..*/

if(lookupe(fort) == 1) {
	strcpy(Tftypes,"for");
	Type = TEXT;	/* Fortran program text */
	goto outa;

}/*E if lookupe fort */

notfort:
i=0;
if (!ascom()) goto notas;

j = i-1;
if (buf[i] == '.') {
	i++;
	if(lookupe(as) == 1) {
		strcpy(Tftypes,"asm");
		Type = TEXT;	/* Assembler program text */
		goto outa;

	}/*T if lookupe as .. */
	else	if(buf[j] == '\n' && isalpha(buf[j+2])) {

			strcpy(Tftypes,"rof");
			Type = TEXT;	/* roff, nroff or eqn
					 * input text.
					 */
			goto outa;
		}
}/*E if buf[i] ..*/

while (!lookupe(asc)) {
	if (!ascom()) goto notas;

	while(buf[i] != '\n' && buf[i++] != ':')
		if(i >= in) goto notas;

	while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t'
		|| buf[i] == '\f' || buf[i] == NL)
		if(i++ >= in) goto notas;

	j = i-1;
	if(buf[i] == '.') {
		i++;
		if(lookupe(as) == 1) {

			strcpy(Tftypes,"asm");
			Type = TEXT; /* ASM program text */
			goto outa;
		}
		else if(buf[j] == '\n' && isalpha(buf[j+2])) {

				strcpy(Tftypes,"rof");
				Type = TEXT;	/* roff, nroff, or
						 * eqn input text.
						 */
				goto outa;
		}
	}/*E if buf[i] == . */
}/* while lookupe(asc) ..*/

strcpy(Tftypes,"asm");
Type = TEXT;	/* Assembler program text */	
goto outa;

notas:

for (i=0; i < in; i++)
	if (buf[i] & 0200) {
		if (((int)buf[0]==(int)'\100') && ((int)buf[1]==(int)'\357')) {
			fclose(ifile);
			strcpy(Tftypes,"rof");
			return(BINARY);	/* troff (CAT) output */
		}
		else if (valid_codeset(ifile,Tftypes)) {
			fclose(ifile);
			return(TEXT);
		}
	fclose(ifile);
	strcpy(Tftypes,"bin");
	return(BINARY);	/* Data file of some type */

}/*E for i=0 ..*/

if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6))) {

	strcpy(Tftypes,"com");
	Type = TEXT;	/* Commands text */
	goto outa;
}
	else if (troffint(in)) {

		strcpy(Tftypes,"rof");
		Type = BINARY; /* troff intermediate output text */
		}
	else if (english(buf, in)) {

		strcpy(Tftypes,"eng");
		Type = ENGLISH;	/* English text */
		}
	else	{
		strcpy(Tftypes,"asc");
		Type = TEXT;	/* Ascii text */
		}
outa:

while(i < in)
	if((!buf[i]) || ((buf[i++] & 0377) > 0176)) {
		if (valid_codeset(ifile,Tftypes)) {
			fclose(ifile);
			return(TEXT);
		}
		/* With garbage, assume binary
		 */
		fclose(ifile);
		strcpy(Tftypes,"bin");
		return(BINARY);
	}

if (((Type == TEXT) || (Type == ENGLISH))) {
	/*
	 *	We don't want to be mislead. If we think this is
	 *	a 'text' file... Make certain of it..
	 */
	while((in = read(fileno(ifile),buf,MBUFSIZ)) > 0) {

		for(i = 0; i < in; i++)
			if((!buf[i]) || ((buf[i] & 0377) > 0176)) {
				if (valid_codeset(ifile,Tftypes)) {
					fclose(ifile);
					return(TEXT);
				}
				/* With garbage, assume binary
				 */
				fclose(ifile);
				strcpy(Tftypes,"bin");
				return(BINARY);
			}
		/* If it doesn't meet the English frequency test,
		 * again assume it is TEXT of some kind.
		 */
		if (Type == ENGLISH)
			if (!(english(buf, in))) {
				/*
				 * If it fails English freq test,
				 * switch over to TEXT and continue
				 * checking for BINARY data.
				 */
				strcpy(Tftypes,"asc");
				Type = TEXT;
			}
	}/*E while in .. */
	if (in < 0) {
	    PERROR "\n%s: %s %s\n", caller, CANTRD, file);
	    perror(caller);
	    exit(FAIL);
	}

}/*E if Type == TEXT */

	/* If the above logic didn't change our mind,
	 * return the tentative file type as the actual.
	 * Convert tentative specifics to generalized types
	 * as/if required.
	 */
if (Type == ENGLISH) {
	fclose(ifile);
	return(TEXT);
}
else {
	fclose(ifile);
	return(Type);
}
}/*E Filetype() */
/**/
/*
 *
 * Function:
 *
 *	ascom
 *
 * Function Description:
 *
 *	
 *
 * Arguments:
 *
 *	none
 *
 * Return values:
 *
 *	Indication of what value(s) are returned.
 *
 *
 * Side Effects:
 *
 */

ascom()
{

while(buf[i] == '/') { 
	i++;

	while(buf[i++] != '\n')if(i >= in)
		return(0);

	while(buf[i] == '\n')if(i++ >= in)
		return(0);

}/*E while buf[i] ..*/

return(1);

}/*E ascom */
/**/
/*
 *
 * Function:
 *
 *	ccom
 *
 * Function Description:
 *
 *	This section will provide a description of the function.
 *
 * Arguments:
 *
 *	none
 *
 * Return values:
 *
 *	Indication of what value(s) are returned.
 *
 *
 * Side Effects:
 *
 *	
 */

ccom()
{

/*------*\
   Code
\*------*/

while((ch = buf[i]) == ' ' || ch == '\t' || ch == '\n'
	|| ch == '\f' || ch == NL)
	if(i++ >= in)
	{
	return(0);
}

if(buf[i] == '/' && buf[i+1] == '*') {
	i += 2;
	while(buf[i] != '*' || buf[i+1] != '/') {
		if(buf[i] == '\\')
			i += 2;
		else
			i++;

		if(i >= in) {
			return(0);
		}

	}/* while buf[i] ..*/

	if((i += 2) >= in) {
		return(0);
	}
}/*E if buf[i] ..*/

if(buf[i] == '\n')
	if(ccom() == 0) {
		return(0);
	}
return(1);

}/*E ccom() */
/**/
/*
 *
 * Function:
 *
 *	english
 *
 * Function Description:
 *
 *	This routine attempts to determine if the file contains
 *	english text based on the frequency (or lack thereof)
 *	of key letter usage in the English language.	
 *
 * Arguments:
 *
 *	char	*bp	Pointer to the buffer of text
 *	int	n	Number of characters in the buffer.
 *
 * Return values:
 *
 *	Zero	if the buffer doesn't look like english text.
 *	Non-zero if buffer appears to be English.
 *
 *
 * Side Effects:
 *
 *	none	
 */

english(bp, n)
	char	*bp;
	int	n;
{
/* Local variables
 */

int	ct[NASC], freq, rare, vow;
int	badpun = 0, punct = 0;

/*-*/

if (n<50)
	return(0); /* no point in statistics on squibs */

for(j=0; j<NASC; j++)
	ct[j]=0;

for(j=0; j<n; j++) {
	if (bp[j]<NASC)
		ct[bp[j]|040]++;

	switch (bp[j]) {

		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if ( j < n-1 &&
			    bp[j+1] != ' '  &&
			    bp[j+1] != '\f' &&
			    bp[j+1] != '\t' &&
			    bp[j+1] != NL   &&
			    bp[j+1] != '\n')
				badpun++;

	}/*E switch bp[j] */
}/*E for j=0 ..*/

if (badpun*5 > punct)
	return(0);

vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];

if (2*ct[';'] > ct['e'])
	return(0);

if ( (ct['>']+ct['<']+ct['/'])>ct['e'])
	return(0); /* shell file test */

return (vow*5 >= n-ct[' '] && freq >= 10*rare);

}/*E english() */
/**/
/*
 *
 * Function:
 *
 *	lookupe
 *
 * Function Description:
 *
 *	This section will provide a description of the function.
 *
 * Arguments:
 *
 *	char	*tab	??
 *
 * Return values:
 *
 *	Indication of what value(s) are returned.
 *
 *
 * Side Effects:
 *
 *	
 */

lookupe(tab)
	char	*tab[];
{
/* Local variables
 */

int	k, l;

/*-*/

while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n'
	|| buf[i] == '\f' || buf[i] == NL)
	i++;

for(j=0; tab[j] != 0; j++) {
	l=0;
	for(k=i; ((ch=tab[j][l++]) == buf[k] && ch != '\0');k++)
		;
	if(ch == '\0')
		if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
		    || buf[k] == '\f' || buf[k] == '{' ||
			 buf[k] == NL || buf[k] == '/') {
			i=k;
			return(1);

		}/*E if buf[k] ..*/
}/*E for j=0 ..*/

return(0);

}/*E lookupe() */
/**/
/*
 *
 * Function:
 *
 *	troffint
 *
 * Function Description:
 *
 *	See if the file appears to be  troff  intermediate text
 *
 * Arguments:
 *
 *	int	n	?
 *
 * Return values:
 *
 *	Indication of what value(s) are returned.
 *
 *
 * Side Effects:
 *
 *	
 */

troffint(n)
	int	n;
{

i = 0;
for (j = 0; j < 6; j++) {
	if (lookupe(troff) == 0)
		return(0);
	if (lookupe(troff) == 0)
		return(0);
	while (i < n && buf[i] != '\n')
		i++;
	if (i++ >= n)
		return(0);

}/*E for j=0 .. */

return(1);

}/*E troffint() */

/**/
/*
 *
 * Function:
 *
 *	valid_codeset	
 *
 * Function Description:
 *
 *	This routine is to determine if the input file is a text file
 *	(contains only printable and spacing characters) under
 *	current locale setting.
 *
 * Arguments:
 *
 *	FILE	*fp;		input file
 *	char	*Tftypes;	return true file type (3 characters code)
 *
 * Return values:
 *
 *	Zero	if the input file contains wide character which is not
 *		printable and not spacing character under current
 *		locale setting.
 *	One	if the input file only contains wide characters which
 *		are printable or spacing characters under current
 *		locale setting.
 *
 * Side Effects:
 *
 *	This function changes the position specified by the file pointer
 *	as side effects of read operations.
 *	
 */

int valid_codeset(FILE *fp, char *Tftypes)
{
	unsigned char	tmpbuf[MBUFSIZ+4];
	wchar_t		wcbuf[1];
	int		numc, numc2, valid_code;
	int		mb, i, j, ip, sflag;
	char		current_codeset[20];
	nl_item 	item[1]={CODESET};

	/* list of valid control codes for multi-byte text file; the
	   trailing zero is the terminator for the list.
	*/
	unsigned char	valid_cntr_code[]={0x07,0x08,0x0e,0x0f,0x1b,0};

			/* supported codesets */
	char		codeset_tbl[][20]={
				"ISO8859-1",
				"deckanji","sdeckanji","eucJP","SJIS",
				"dechanzi",
				"dechanyu","eucTW","big5",
				"deckorean","KSC5601",
				"TACTIS",
				""
			};

			/* file type codes for the supported codesets */
	char		ftype_tbl[][4]={
				"ISO",
				"knj","skj","eJP","SJS",
				"hzi",
				"hyu","eTW","bg5",
				"kor","eKR",
				"TTS",
				""
			};

	setlocale(LC_ALL, "");
	lseek (fileno(fp), 0L, 0);
	ip=0;
	while((numc=read(fileno(fp),&tmpbuf[ip],MBUFSIZ)) > 0) {
		/* true buffer size of tmpbuf for examination */
		numc2 = numc+ip;
		i = 0;
		while((i+MB_CUR_MAX <= numc2) ||
		      ((numc<MBUFSIZ) && (i<numc2))) {
			if ((mb=mbtowc(wcbuf,&tmpbuf[i],MB_CUR_MAX)) <= 0) {
				return(0);
			}
			else {
				if ((iswprint(wcbuf[0]) == 0) &&
				    (iswspace(wcbuf[0])==0)) {
					if (mb != 1) return(0);
					valid_code = FALSE;
					for (j=0;valid_cntr_code[j]!=0;j++) 
						if (tmpbuf[i]==valid_cntr_code[j]) {
							valid_code = TRUE;
							break;
						}
					if (!valid_code) return(0);
				}
			}
			i += mb;
		} /* end of while - processing buffer */
		/* move the unprocessed multibytes to the beginning
		   of buffer */
		if ((numc2-i) > 0) {
			for (j=0;j<(numc2-i);j++)
				tmpbuf[j]=tmpbuf[i+j];
			ip=j;
		}
		else if ((numc2-i) == 0) {
			ip=0;
		}
		else { /* should never go there */
			printf("filetype: program logic error ! \n");
			exit(FAIL);
		}
	} /* end of while - read input file */

	if (numc < 0) {
		printf("filetype: error in reading input file \n");
		exit(FAIL);
	}

	/* assign file type code to the current supported codeset */
	strcpy(current_codeset,nl_langinfo(item[0]));
	i=0;
	sflag=0;
	while(strcmp(codeset_tbl[i],"")!=0) {
		if (strcmp(current_codeset,codeset_tbl[i])==0) {
			strcpy(Tftypes,ftype_tbl[i]);
			sflag=1;
			break;
		}
		i++;
	}
	/* if current codeset is not supported by the codeset_tbl,
	   assign "mbT" for multibyte text and "sbT" for single
	   byte text */
	if (sflag==0) {
		if (MB_CUR_MAX > 1) 
			strcpy(Tftypes,"mbT");
		else
			strcpy(Tftypes,"sbT");
	}
	return(1);
}
/**\\**\\**\\**\\**\\**  EOM  filetype.c  **\\**\\**\\**\\**\\*/
/**\\**\\**\\**\\**\\**  EOM  filetype.c  **\\**\\**\\**\\**\\*/
