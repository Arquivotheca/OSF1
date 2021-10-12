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
static char	*sccsid = "@(#)$RCSfile: railmag.c,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 06:09:23 $";
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

/*
 * tell vcat which fonts are loaded on the "typesetter"
 */

#define MAGIC_NUMBER 0436
#define RAILMAG_FILE "/usr/lib/vfont/railmag"

char	*concat();
int	rmfd;
char	*rm[4];
char	tbuf[256];

main(argc, argv)
	int argc;
	char *argv[];
{
	register int fnum;
	char cbuf[4][50];

	readrm();
	if (argc <= 1) {
		printrm();
		exit(0);
	}
	while (--argc) {
		argv++;
		fnum = argv[0][0] - '0';
		if (fnum < 1 || fnum > 4)
			error("Invalid font number");
		checkfont(argv[1]);
		if (argv[1][0] == '/')
			rm[fnum-1] = argv[1];
		else
			rm[fnum-1] = concat(cbuf[fnum-1], "/usr/lib/vfont/", argv[1]);
		argv++;	argc--;
	}
	writerm();
}

error(str)
	char *str;
{
	write(2, "Railmag: ", 9);
	write(2, str, strlen(str));
	write(2, "\n", 1);
	exit();
}

checkfont(file)
	char *file;
{
	register int fd;
	char cbuf[80];
	char cbuf2[80];
	short word;

	if ((fd = open(concat(cbuf, file, ".10"), 0)) < 0)
		if ((fd = open(concat(cbuf2, "/usr/lib/vfont/", cbuf), 0)) < 0)
			error("cant open font");
	if (read(fd, &word, 2) != 2)
		error("cant read font");
	if (word != MAGIC_NUMBER)
		error("font has no magic number");
	close(fd);
}

readrm()
{
	register int i;
	register char *cp;
	char c;

	if ((rmfd = open(RAILMAG_FILE, 0)) < 0)
		error("No railmag file");
	cp = tbuf;
	for (i = 0; i < 4; i++) {
		rm[i] = cp;
		while (read(rmfd, &c, 1) == 1 && c != '\n')
			*cp++ = c;
		*cp++ = '\0';
	}
}

printrm()
{
	register int i;

	for (i = 0; i < 4; i++)
		printf("%s on %d\n", rm[i], i+1);
}

writerm()
{
	register int i;
	register char *cp;

	unlink(RAILMAG_FILE);
	if ((rmfd = creat(RAILMAG_FILE, 0644)) < 0)
		error("cant recreate railmag file");
	for (i = 0; i < 4; i++) {
		cp = rm[i];
		while (*cp != '\0')
			write(rmfd, cp++, 1);
		write(rmfd, "\n", 1);
	}
}

char *
concat(outbuf, in1, in2)
	register char *outbuf, *in1, *in2;
{
	char *save;

	save = outbuf;
	while (*in1)
		*outbuf++ = *in1++;
	while (*in2)
		*outbuf++ = *in2++;
	return(save);
}
