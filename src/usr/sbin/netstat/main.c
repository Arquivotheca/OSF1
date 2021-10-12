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
static char	*sccsid = "@(#)$RCSfile: main.c,v $ $Revision: 4.2.5.5 $ (DEC) $Date: 1993/08/16 19:54:50 $";
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
 * Copyright (c) 1983, 1988 Regents of the University of California.
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
"@(#) Copyright (c) 1983, 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/socket.h>
#include <sys/file.h>
#if	!MACH
#include <machine/pte.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>
#include <paths.h>

#define nl netstatnl
struct nlist nl[] = {
#define	N_MBSTAT	0
	{ "_mbstat" },
#define	N_IPSTAT	1
	{ "_ipstat" },
#define	N_TCB		2
	{ "_tcb" },
#define	N_TCPSTAT	3
	{ "_tcpstat" },
#define	N_UDB		4
	{ "_udb" },
#define	N_UDPSTAT	5
	{ "_udpstat" },
#define	N_RAWCB		6
	{ "_rawcb" },
#if	MACH
	{ "_rawcb" },	/* not used */
	{ "_rawcb" },	/* not used */
#else	/* UNIX */
#define	N_SYSMAP	7
	{ "_Sysmap" },
#define	N_SYSSIZE	8
	{ "_Syssize" },
#endif
#define	N_IFNET		9
	{ "_ifnet" },
#define	N_IMP		10
	{ "_imp_softc" },
#define	N_RTHOST	11
	{ "_rthost" },
#define	N_RTNET		12
	{ "_rtnet" },
#define	N_ICMPSTAT	13
	{ "_icmpstat" },
#define	N_RTSTAT	14
	{ "_rtstat" },
#define	N_NFILE		15
	{ "_nfile" },
#define	N_FILE		16
	{ "_file" },
#define	N_UNIXSW	17
	{ "_unixsw" },
#define N_RTHASHSIZE	18
	{ "_rthashsize" },
#define N_IDP		19
	{ "_nspcb"},
#define N_IDPSTAT	20
	{ "_idpstat"},
#define N_SPPSTAT	21
	{ "_spp_istat"},
#define N_NSERR		22
	{ "_ns_errstat"},
#define	N_CLNPSTAT	23
	{ "_clnp_stat"},
#define	IN_TP		24
	{ "_tp_inpcb" },
#define	ISO_TP		25
	{ "_tp_isopcb" },
#define	ISO_X25		26
	{ /*"_x25_isopcb"*/ "_file"}, /* fast gross hack to speed up */
#define	N_TPSTAT	27
	{ "_tp_stat" },
#define	N_X25STAT	28
	{ /*"_x25_isopcb"*/ "_file"},
#define	N_ESISSTAT	29
	{ "_esis_stat"},
#define	N_NIMP		30
	{ "_nimp"},
#define	N_RTREE		31
	{ "_radix_node_head"},
#define	N_CLTP		32
	{ "_cltb"},
#define	N_CLTPSTAT	33
	{ "_cltpstat"},
#define N_IGMPSTAT	34
	{ "_igmpstat"},
#define N_MRTPROTO	35
	{ "_ip_mrtproto" },
#define N_MRTSTAT	36
	{ "_mrtstat" },
#define N_MRTTABLE	37
	{ "_mrttable" },
#define N_VIFTABLE	38
	{ "_viftable" },
#define N_KMEMTYPES	39
	{ "_kmemtypes"},
#if	MACH
#define	N_ARPTAB	40	/* Following only under OSF/MACH */
	{ "_arptab" },
#define	N_ARPTAB_BSIZ	41
	{ "_arptab_bsiz" },
#define	N_ARPTAB_NB	42
	{ "_arptab_nb" },
#define	N_MCLBYTES	43
	{ "_mclbytes" },
#endif
	"",
};

/* internet protocols */
extern	int protopr();
extern	int tcp_stats(), udp_stats(), ip_stats(), icmp_stats(), igmp_stats();
/* ns protocols */
extern	int nsprotopr();
extern	int spp_stats(), idp_stats(), nserr_stats();
/* iso protocols */
extern	int iso_protopr();
extern	int tp_stats(), esis_stats(), clnp_stats(), cltp_stats();

#define NULLPROTOX	((struct protox *) 0)
struct protox {
	u_char	pr_index;		/* index into nlist of cb head */
	u_char	pr_sindex;		/* index into nlist of stat block */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	int	(*pr_cblocks)();	/* control blocks printing routine */
	int	(*pr_stats)();		/* statistics printing routine */
	char	*pr_name;		/* well-known name */
	int	pr_get;			/* getprotoent */
} protox[] = {
	{ N_TCB,	N_TCPSTAT,	1,	protopr,
	  tcp_stats,	"tcp",	0 },
	{ N_UDB,	N_UDPSTAT,	1,	protopr,
	  udp_stats,	"udp",	0 },
	{ IN_TP,	N_TPSTAT,	1,	protopr,
	  tp_stats,	"tpip",	0 },
	{ -1,		N_IPSTAT,	1,	0,
	  ip_stats,	"ip",	0 },
	{ -1,		N_ICMPSTAT,	1,	0,
	  icmp_stats,	"icmp",	0 },
	{ -1,		N_IGMPSTAT,	1,	0,
	  igmp_stats,	"igmp",	0 },
	{ -1,		-1,		0,	0,
	  0,		0,	0 }
};

struct protox nsprotox[] = {
	{ N_IDP,	N_IDPSTAT,	1,	nsprotopr,
	  idp_stats,	"idp" },
	{ N_IDP,	N_SPPSTAT,	1,	nsprotopr,
	  spp_stats,	"spp" },
	{ -1,		N_NSERR,	1,	0,
	  nserr_stats,	"ns_err" },
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct protox isoprotox[] = {
	{ ISO_TP,	N_TPSTAT,	1,	iso_protopr,
	  tp_stats,	"tp" },
	{ N_CLTP,	N_CLTPSTAT,	1,	iso_protopr,
	  cltp_stats,	"cltp" },
#ifdef notdef
	{ ISO_X25,	N_X25STAT,	1,	x25_protopr,
	  x25_stats,	"x25" },
#endif
	{ -1,		N_CLNPSTAT,	1,	 0,
	  clnp_stats,	"clnp"},
	{ -1,		N_ESISSTAT,	1,	 0,
	  esis_stats,	"esis"},
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct protox *protoprotox[] = { protox, nsprotox, isoprotox, NULLPROTOX };

#if	!MACH
struct	pte *Sysmap;
#endif

char	*system = _PATH_UNIX;
char	*kmemf = _PATH_KMEM;
int	kmem;
int	kflag;
int	Aflag;
int	Mflag;
int	aflag;
#if	MACH
int	Hflag;
#endif
int	iflag;
int	mflag;
int	nflag;
int	pflag;
int	rflag;
int	sflag;
int	tflag;
int	dflag;
int	interval;
char	*interface;
int	unit;

int	af = AF_INET;		/* See code at -f option */

extern	char *malloc();
extern	off_t lseek();

main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	register struct protoent *p;
	register struct protox *tp;	/* for printing cblocks & stats */
	struct protox *name2protox();	/* for -p */
	int ch;

#if	MACH
	while ((ch = getopt(argc, argv, "AHI:Maf:himnp:drstu")) != EOF)
#else
	while ((ch = getopt(argc, argv, "AI:Maf:himnp:drstu")) != EOF)
#endif
		switch((char)ch) {
		case 'A':
			Aflag++;
			break;
		case 'M':
			Mflag++;
			break;
#if	MACH
		case 'H':
			Hflag++;
			break;

#endif
		case 'I': {
			char *cp;

			iflag++;
			for (cp = interface = optarg; isalpha(*cp); cp++);
			unit = atoi(cp);
			*cp = '\0';
			break;
		}
		case 'a':
			aflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'f':
			if (strcmp(optarg, "ns") == 0)
				af = AF_NS;
			else if (strcmp(optarg, "inet") == 0)
				af = AF_INET;
			else if (strcmp(optarg, "unix") == 0)
				af = AF_UNIX;
			else if (strcmp(optarg, "iso") == 0)
				af = AF_ISO;
			else if (strcmp(optarg, "route") == 0)
				rflag++;
			else if (strcmp(optarg, "any") == 0 ||
				 strcmp(optarg, "all") == 0)
				af = AF_UNSPEC;
			else {
				fprintf(stderr, "%s: unknown address family\n", optarg);
				exit(10);
			}
			break;
		case 'i':
			iflag++;
			break;
		case 'm':
			mflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'p':
			if ((tp = name2protox(optarg)) == NULLPROTOX) {
				fprintf(stderr, "%s: unknown or uninstrumented protocol\n", optarg);
				exit(10);
			}
			pflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'u':
			af = AF_UNIX;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc > 0) {
		if (isdigit(argv[0][0])) {
			interval = atoi(argv[0]);
			if (interval <= 0)
				usage();
			argv++, argc--;
			iflag++;
		}
		if (argc > 0) {
			system = *argv;
			argv++, argc--;
			if (argc > 0) {
				kmemf = *argv;
				kflag++;
			}
		}
	}
#if	1
	if ((vmnlist(system, nl) && nlist(system, nl) < 0) ||
	    nl[0].n_type == 0) {
#else
	if (nlist(system, nl) < 0 || nl[0].n_type == 0) {
#endif
		fprintf(stderr, "%s: no namelist\n", system);
		exit(1);
	}
	kmem = open(kmemf, O_RDONLY);
	if (kmem < 0) {
		perror(kmemf);
		exit(1);
	}
#if	!MACH
	if (kflag) {
		off_t off;

		Sysmap = (struct pte *)
		   malloc((u_int)(nl[N_SYSSIZE].n_value * sizeof(struct pte)));
		if (!Sysmap) {
			fputs("netstat: can't get memory for Sysmap.\n", stderr);
			exit(1);
		}
		off = nl[N_SYSMAP].n_value & ~KERNBASE;
		(void)lseek(kmem, off, L_SET);
		(void)read(kmem, (char *)Sysmap,
		    (int)(nl[N_SYSSIZE].n_value * sizeof(struct pte)));
	}
#endif
	if (mflag) {
		mbpr((off_t)nl[N_MBSTAT].n_value,
		     (off_t)nl[N_MCLBYTES].n_value,
		     (off_t)nl[N_KMEMTYPES].n_value);
		exit(0);
	}
	if (pflag) {
		if (tp->pr_stats)
			(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
				tp->pr_name);
		else
			printf("%s: no stats routine\n", tp->pr_name);
		exit(0);
	}
	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	sethostent(1);
	setnetent(1);
	if (iflag) {
		if(sflag)
			if_cntrs();
		else
			intpr(interval, nl[N_IFNET].n_value);
		exit(0);
	}
#if	MACH
	if (Hflag) {
		arppr((off_t)nl[N_ARPTAB].n_value,
		      (off_t)nl[N_ARPTAB_BSIZ].n_value,
		      (off_t)nl[N_ARPTAB_NB].n_value);
		exit(0);
	}
#endif
	if (rflag || af == AF_UNSPEC) {
		if (sflag)
			rt_stats((off_t)nl[N_RTSTAT].n_value);
		else
			routepr((off_t)nl[N_RTHOST].n_value, 
				(off_t)nl[N_RTNET].n_value,
				(off_t)nl[N_RTHASHSIZE].n_value,
				(off_t)nl[N_RTREE].n_value);
		if (af != AF_UNSPEC)
			exit(0);
	}
	if (Mflag) {
		if (sflag)
			mrt_stats((off_t)nl[N_MRTPROTO].n_value,
				  (off_t)nl[N_MRTSTAT].n_value);
		else
			mroutepr((off_t)nl[N_MRTPROTO].n_value,
				 (off_t)nl[N_MRTTABLE].n_value,
				 (off_t)nl[N_VIFTABLE].n_value);
		exit(0);
	}
    if (af == AF_INET || af == AF_UNSPEC) {
	setprotoent(1);
	setservent(1);
	while (p = getprotoent()) {
		for (tp = protox; tp->pr_name; tp++)
			if (strcmp(tp->pr_name, p->p_name) == 0) {
				tp->pr_get++;
				break;
			}
		if (tp->pr_name == 0 || tp->pr_wanted == 0 || tp->pr_get > 1)
			continue;
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					p->p_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					p->p_name);
	}
	endprotoent();
    }
    if (af == AF_NS || af == AF_UNSPEC) {
	for (tp = nsprotox; tp->pr_name; tp++) {
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					tp->pr_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					tp->pr_name);
	}
    }
    if (af == AF_ISO || af == AF_UNSPEC) {
	for (tp = isoprotox; tp->pr_name; tp++) {
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					tp->pr_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					tp->pr_name);
	}
    }
    if ((af == AF_UNIX || af == AF_UNSPEC) && !sflag)
	    unixpr((off_t)nl[N_NFILE].n_value, (off_t)nl[N_FILE].n_value,
		(struct protosw *)nl[N_UNIXSW].n_value);
    exit(0);
}

/*
 * Seek into the kernel for a value.
 */
off_t
klseek(fd, base, off)
	int fd, off;
	off_t base;
{
#if	!MACH
	if (kflag) {
		/* get kernel pte */
		base &= ~KERNBASE;
		base = ctob(Sysmap[btop(base)].pg_pfnum) + (base & PGOFSET);
	}
#endif
	return (lseek(fd, base, off));
}

char *
plural(n)
	int n;
{

	return (n != 1 ? "s" : "");
}

char *
pluraly(n)
	int n;
{
	return (n != 1 ? "ies" : "y");
}

char *
plurales(n)
	int n;
{
	return (n != 1 ? "es" : "");
}

/*
 * Find the protox for the given "well-known" name.
 */
struct protox *
knownname(name)
	char *name;
{
	struct protox **tpp, *tp;

	for (tpp = protoprotox; *tpp; tpp++)
	    for (tp = *tpp; tp->pr_name; tp++)
		if (strcmp(tp->pr_name, name) == 0)
			return(tp);
	return(NULLPROTOX);
}

/*
 * Find the protox corresponding to name.
 */
struct protox *
name2protox(name)
	char *name;
{
	struct protox *tp;
	char **alias;			/* alias from p->aliases */
	struct protoent *p;

	/*
	 * Try to find the name in the list of "well-known" names. If that
	 * fails, check if name is an alias for an Internet protocol.
	 */
	if (tp = knownname(name))
		return(tp);

	setprotoent(1);			/* make protocol lookup cheaper */
	while (p = getprotoent()) {
		/* assert: name not same as p->name */
		for (alias = p->p_aliases; *alias; alias++)
			if (strcmp(name, *alias) == 0) {
				endprotoent();
				return(knownname(p->p_name));
			}
	}
	endprotoent();
	return(NULLPROTOX);
}

usage()
{
#if	MACH
	fputs("usage: netstat [-Aan] [-f address_family] [system]\n               [-adHimMnrstu] [-f address_family] [-p protocol] [system]\n               [-an] [-I interface] interval [system]\n", stderr);
#else
	fputs("usage: netstat [-Aan] [-f address_family] [system]\n               [-imnrs] [-f address_family] [system]\n               [-n] [-I interface] interval [system]\n", stderr);
#endif
	exit(1);
}

/*
 * quick routine for loading up the short nlist info from vmunix.
 *
 * return zero on success, 1 on failure
 */

#define NLISTFILE "/etc/vmnlist"

#include <sys/stat.h>

vmnlist(nlistf,nl)
char *nlistf;
struct nlist nl[];
{
    register FILE *f;
    struct stat stb,stb2;
    char symnam[100];
    register char *p;
    struct nlist n;
    register struct nlist *t;
    register nlsize;

    if (strcmp (nlistf, "/vmunix"))
	return 1;
    if (stat ("/vmunix", &stb) < 0 || stat (NLISTFILE, &stb2) < 0)
	return 1;
    if (stb.st_mtime >= stb2.st_mtime || stb2.st_size == 0)
	return 1;
    if ((f = fopen (NLISTFILE, "r")) == NULL)
	return 1;
    nlsize = 0;
    for (t=nl; t->n_name != NULL && t->n_name[0] != '\0'; ++t) {
	t->n_type = 0;
	t->n_value = 0;
	nlsize++;
    }
    for(;;) {
	p = symnam;
	while ((*p++ = fgetc(f)) != '\0' && !feof(f))
	    ;
	if (feof(f)) break;
	if (fread(&n, sizeof(struct nlist), 1, f) != 1) break;
	/* find the symbol in nlist */
	for (t=nl; t->n_name != NULL && t->n_name[0] != '\0'; ++t)
	    if (t->n_type == 0 && t->n_value == 0 &&
		strcmp(symnam,t->n_name) == 0) {
		n.n_name = t->n_name;
		*t = n;
		if (--nlsize == 0) {
		    fclose(f);
		    return(0);
		}
		break;
	    }
    }
    fclose(f);
    return(1);
}
