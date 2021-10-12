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
static char     *sccsid = "@(#)$RCSfile: ntpdc.c,v $ $Revision: 4.3.3.3 $ (DEC) $Date: 1992/07/07 16:11:45 $";
#endif
/*
 */

/*
 * xntpdc - control and monitor your xntpd daemon
 */
#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ntp/ntp_fp.h>
#include <ntp/ntp.h>
#include <ntp/ntp_request.h>
#include <ntp/ntpdc.h>

#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

/*
 * Because we now potentially understand a lot of commands (and
 * it requires a lot of commands to talk to xntpd) we will run
 * interactive if connected to a terminal.
 */
int interactive = 0;		/* set to 1 when we should prompt */
char *prompt = "xntpdc> ";	/* prompt to ask him about */


/*
 * Keyid used for authenticated requests.  Obtained on the fly.
 */
u_int info_auth_keyid;


/*
 * Built in command handler declarations
 */
void help(), timeout(), host(), poll(), hostnames();
void setdebug(), quit(), version();
#ifdef DES_OK
void keyid(), passwd(), delay();
#endif /* DES_OK */

/*
 * Built-in commands we understand
 */
struct xcmd builtins[] = {
	{ "?",		help,		{  OPT|STR, NO, NO, NO },
					{ "command", "", "", "" },
			"tell the use and syntax of commands" },
	{ "help",	help,		{  OPT|STR, NO, NO, NO },
					{ "command", "", "", "" },
			"tell the use and syntax of commands" },
	{ "timeout",	timeout,	{ OPT|UINT, NO, NO, NO },
					{ "msec", "", "", "" },
			"set the primary receive time out" },
#ifdef DES_OK
	{ "delay",	delay,		{ OPT|INT, NO, NO, NO },
					{ "msec", "", "", "" },
			"set the delay added to encryption time stamps" },
#endif /* DES_OK */
	{ "host",	host,		{ OPT|STR, NO, NO, NO },
					{ "hostname", "", "", "" },
			"specify the host whose NTP server we talk to" },
#ifdef DES_OK
	{ "poll",	poll,		{ OPT|UINT, OPT|STR, NO, NO },
					{ "n", "verbose", "", "" },
			"poll an NTP server in client mode `n' times" },
	{ "passwd",	passwd,		{ NO, NO, NO, NO },
					{ "", "", "", "" },
			"specify a password to use for authenticated requests"},
#endif /* DES_OK */
	{ "hostnames",	hostnames,	{ OPT|STR, NO, NO, NO },
					{ "yes|no", "", "", "" },
			"specify whether hostnames or net numbers are printed"},
	{ "debug",	setdebug,	{ OPT|STR, NO, NO, NO },
					{ "no|more|less", "", "", "" },
			"set/change debugging level" },
	{ "quit",	quit,		{ NO, NO, NO, NO },
					{ "", "", "", "" },
			"exit xntpdc" },
#ifdef DES_OK
	{ "keyid",	keyid,		{ OPT|UINT, NO, NO, NO },
					{ "key#", "", "", "" },
			"set keyid to use for authenticated requests" },
#endif /* DES_OK */
	{ "version",	version,	{ NO, NO, NO, NO },
					{ "", "", "", "" },
			"print version number" },
	{ 0,		0,		{ NO, NO, NO, NO },
					{ "", "", "", "" }, "" }
};


/*
 * Default values we use.
 */
#define	DEFTIMEOUT	(5)		/* 5 second time out */
#define	DEFSTIMEOUT	(2)		/* 2 second time out after first */
#define	DEFDELAY	0x51EB852	/* 20 milliseconds, l_fp fraction */
#define	DEFHOST		"localhost"	/* default host name */
#define	LENHOSTNAME	256		/* host name is 256 characters long */
#define	MAXCMDS		100		/* maximum commands on cmd line */
#define	MAXHOSTS	100		/* maximum hosts on cmd line */
#define	MAXLINE		512		/* maximum line length */
#define	MAXTOKENS	(1+MAXARGS+2)	/* maximum number of usable tokens */

/*
 * Some variables used and manipulated locally
 */
struct timeval tvout = { DEFTIMEOUT, 0 };	/* time out for reads */
struct timeval tvsout = { DEFSTIMEOUT, 0 };	/* secondary time out */
l_fp delay_time;				/* delay time */
char currenthost[LENHOSTNAME];			/* current host name */
struct sockaddr_in hostaddr = { 0 };		/* host address */
int showhostnames = 1;				/* show host names by default */

int sockfd;					/* fd socket is openned on */
int havehost = 0;				/* set to 1 when host open */
struct servent *server_entry = NULL;		/* server entry for ntp */

/*
 * Holds data returned from queries.  We allocate INITDATASIZE
 * octets to begin with, increasing this as we need to.
 */
#define	INITDATASIZE	(sizeof(struct resp_pkt) * 16)
#define	INCDATASIZE	(sizeof(struct resp_pkt) * 8)

char *pktdata;
int pktdatasize;

/*
 * For commands typed on the command line (with the -c option)
 */
int numcmds = 0;
char *ccmds[MAXCMDS];
#define	ADDCMD(cp)	if (numcmds < MAXCMDS) ccmds[numcmds++] = (cp)

/*
 * When multiple hosts are specified.
 */
int numhosts = 0;
char *chosts[MAXHOSTS];
#define	ADDHOST(cp)	if (numhosts < MAXHOSTS) chosts[numhosts++] = (cp)

/*
 * Error codes for internal use
 */
#define	ERR_INCOMPLETE		16
#define	ERR_TIMEOUT		17

/*
 * Macro definitions we use
 */
#define	ISSPACE(c)	((c) == ' ' || (c) == '\t')
#define	ISEOL(c)	((c) == '\n' || (c) == '\r' || (c) == '\0')
#define	STREQ(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

/*
 * For converting time stamps to dates
 */
#define	JAN_1970	2208988800	/* 1970 - 1900 in seconds */

/*
 * Jump buffer for longjumping back to the command level
 */
jmp_buf interrupt_buf;

/*
 * Pointer to current output unit
 */
FILE *current_output;

/*
 * Command table imported from ntpdc_ops.c
 */
extern struct xcmd opcmds[];

char *progname;
int debug;

/*
 * main - parse arguments and handle options
 */
main(argc, argv)
int argc;
char *argv[];
{
	int c;
	int errflg = 0;
	extern int optind;
	extern char *optarg;
	void getcmds();
	void docmd();
	extern char *malloc();

	delay_time.l_ui = 0;
	delay_time.l_uf = DEFDELAY;

	progname = argv[0];
	while ((c = getopt(argc, argv, "c:dilnps")) != EOF)
		switch (c) {
		case 'c':
			ADDCMD(optarg);
			break;
		case 'd':
			++debug;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'l':
			ADDCMD("listpeers");
			break;
		case 'n':
			showhostnames = 0;
			break;
		case 'p':
			ADDCMD("peers");
			break;
		case 's':
			ADDCMD("dmpeers");
			break;
		default:
			errflg++;
			break;
		}
	if (errflg) {
		(void) fprintf(stderr,
		    "usage: %s [-dilnps] [-c cmd] host ...\n",
		    progname);
		exit(2);
	}
	if (optind == argc) {
		ADDHOST(DEFHOST);
	} else {
		for (; optind < argc; optind++)
			ADDHOST(argv[optind]);
	}

	if (numcmds == 0 && interactive == 0
	    && isatty(fileno(stdin)) && isatty(fileno(stderr))) {
		interactive = 1;
	}

	/*
	 * Initialize the packet data buffer
	 */
	pktdata = malloc(INITDATASIZE);
	if (pktdata == NULL) {
		(void) fprintf(stderr, "%s: malloc() failed!\n", progname);
		exit(1);
	}
	pktdatasize = INITDATASIZE;

	if (numcmds == 0) {
		(void) openhost(chosts[0]);
		getcmds();
	} else {
		int ihost;
		int icmd;

		for (ihost = 0; ihost < numhosts; ihost++) {
			if (openhost(chosts[ihost]))
				for (icmd = 0; icmd < numcmds; icmd++)
					docmd(ccmds[icmd]);
		}
	}
	exit(0);
}


/*
 * openhost - open a socket to a host
 */
int
openhost(hname)
	char *hname;
{
	u_int netnum;
#ifdef SO_RCVBUF
	int rbufsize;
#endif
	char temphost[LENHOSTNAME];
	int getnetnum();

	if (server_entry == NULL) {
		server_entry = getservbyname("ntp", "udp");
		if (server_entry == NULL) {
			(void) fprintf(stderr, "%s: ntp/udp: unknown service\n",
			    progname);
			exit(1);
		}
		if (debug > 2)
			printf("Got ntp/udp service entry\n");
	}

	if (!getnetnum(hname, &netnum, temphost))
		return 0;
	
	if (debug > 2)
		printf("Opening host %s\n", temphost);

	if (havehost == 1) {
		if (debug > 2)
			printf("Closing old host %s\n", currenthost);
		(void) close(sockfd);
		havehost = 0;
	}
	(void) strcpy(currenthost, temphost);

	hostaddr.sin_family = AF_INET;
	hostaddr.sin_port = server_entry->s_port;
	hostaddr.sin_addr.s_addr = netnum;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
		error("socket", "", "");
	
#ifdef SO_RCVBUF
	rbufsize = INITDATASIZE + 2048;	/* 2K for slop */
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
	    &rbufsize, sizeof(int)) == -1)
		error("setsockopt", "", "");
#endif

	if (connect(sockfd, (char *)&hostaddr, sizeof(hostaddr)) == -1)
		error("connect", "", "");
	
	havehost = 1;
	return 1;
}


/*
 * sendpkt - send a packet to the remote host
 */
int
sendpkt(xdata, xdatalen)
	char *xdata;
	int xdatalen;
{
	if (write(sockfd, xdata, xdatalen) == -1) {
		warning("write to %s failed", currenthost, "");
		return -1;
	}

	return 0;
}


/*
 * growpktdata - grow the packet data area
 */
void
growpktdata()
{
	extern char *realloc();

	pktdatasize += INCDATASIZE;
	pktdata = realloc(pktdata, pktdatasize);
	if (pktdata == 0) {
		(void) fprintf(stderr, "%s: realloc() failed!\n", progname);
		exit(1);
	}
}


/*
 * getresponse - get a (series of) response packet(s) and return the data
 */
int
getresponse(implcode, reqcode, ritems, rsize, rdata)
	int implcode;
	int reqcode;
	int *ritems;
	int *rsize;
	char **rdata;
{
	struct resp_pkt rpkt;
	struct timeval tvo;
	int items;
	int size;
	int datasize;
	char *datap;
	char haveseq[MAXSEQ+1];
	int firstpkt;
	int lastseq;
	int numrecv;
	int seq;
	fd_set fds;
	int n;

	/*
	 * This is pretty tricky.  We may get between 1 and many packets
	 * back in response to the request.  We peel the data out of
	 * each packet and collect it in one long block.  When the last
	 * packet in the sequence is received we'll know how many we
	 * should have had.  Note we use one long time out, should reconsider.
	 */
	*ritems = 0;
	*rsize = 0;
	firstpkt = 1;
	numrecv = 0;
	*rdata = datap = pktdata;
	lastseq = 999;	/* too big to be a sequence number */
	bzero(haveseq, sizeof(haveseq));
	FD_ZERO(&fds);

again:
	if (firstpkt)
		tvo = tvout;
	else
		tvo = tvsout;
	
	FD_SET(sockfd, &fds);
	n = select(sockfd+1, &fds, (fd_set *)0, (fd_set *)0, &tvo);

	if (n == -1) {
		warning("select fails", "", "");
		return -1;
	}
	if (n == 0) {
		/*
		 * Timed out.  Return what we have
		 */
		if (firstpkt) {
			(void) fprintf(stderr,
			    "%s: timed out, nothing received\n", currenthost);
			return ERR_TIMEOUT;
		} else {
			(void) fprintf(stderr,
			    "%s: timed out with incomplete data\n",
			    currenthost);
			if (debug) {
				printf("Received sequence numbers");
				for (n = 0; n <= MAXSEQ; n++)
					if (haveseq[n])
						printf(" %d,", n);
				if (lastseq != 999)
					printf(" last frame received\n");
				else
					printf(" last frame not received\n");
			}
			return ERR_INCOMPLETE;
		}
	}

	n = read(sockfd, (char *)&rpkt, sizeof(rpkt));
	if (n == -1) {
		warning("read", "", "");
		return -1;
	}


	/*
	 * Check for format errors.  Bug proofing.
	 */
	if (n < RESP_HEADER_SIZE) {
		if (debug)
			printf("Short (%d byte) packet received\n", n);
		goto again;
	}
	if (INFO_VERSION(rpkt.rm_vn_mode) != NTP_VERSION) {
		if (debug)
			printf("Packet received with version %d\n",
			    INFO_VERSION(rpkt.rm_vn_mode));
		goto again;
	}
	if (INFO_MODE(rpkt.rm_vn_mode) != MODE_PRIVATE) {
		if (debug)
			printf("Packet received with mode %d\n",
			    INFO_MODE(rpkt.rm_vn_mode));
		goto again;
	}
	if (INFO_IS_AUTH(rpkt.auth_seq)) {
		if (debug)
			printf("Encrypted packet received\n");
		goto again;
	}
	if (!ISRESPONSE(rpkt.rm_vn_mode)) {
		if (debug)
			printf("Received request packet, wanted response\n");
		goto again;
	}
	if (INFO_MBZ(rpkt.mbz_itemsize) != 0) {
		if (debug)
			printf("Received packet with nonzero MBZ field!\n");
		goto again;
	}

	/*
	 * Check implementation/request.  Could be old data getting to us.
	 */
	if (rpkt.implementation != implcode || rpkt.request != reqcode) {
		if (debug)
			printf(
		"Received implementation/request of %d/%d, wanted %d/%d",
			    rpkt.implementation, rpkt.request,
			    implcode, reqcode);
		goto again;
	}

	/*
	 * Check the error code.  If non-zero, return it.
	 */
	if (INFO_ERR(rpkt.err_nitems) != INFO_OKAY) {
		if (debug && ISMORE(rpkt.rm_vn_mode)) {
			printf("Error code %d received on not-final packet\n",
			    INFO_ERR(rpkt.err_nitems));
		}
		return (int)INFO_ERR(rpkt.err_nitems);
	}


	/*
	 * Collect items and size.  Make sure they make sense.
	 */
	items = INFO_NITEMS(rpkt.err_nitems);
	size = INFO_ITEMSIZE(rpkt.mbz_itemsize);

	if ((datasize = items*size) > (n-RESP_HEADER_SIZE)) {
		if (debug)
			printf(
		"Received items %d, size %d (total %d), data in packet is %d\n",
			    items, size, datasize, n-RESP_HEADER_SIZE);
		goto again;
	}

	/*
	 * If this isn't our first packet, make sure the size matches
	 * the other ones.
	 */
	if (!firstpkt && size != *rsize) {
		if (debug)
			printf("Received itemsize %d, previous %d\n",
			    size, *rsize);
		goto again;
	}

	/*
	 * If we've received this before, toss it
	 */
	seq = INFO_SEQ(rpkt.auth_seq);
	if (haveseq[seq]) {
		if (debug)
			printf("Received duplicate sequence number %d\n", seq);
		goto again;
	}
	haveseq[seq] = 1;

	/*
	 * If this is the last in the sequence, record that.
	 */
	if (!ISMORE(rpkt.rm_vn_mode)) {
		if (lastseq != 999) {
			printf("Received second end sequence packet\n");
			goto again;
		}
		lastseq = seq;
	}

	/*
	 * So far, so good.  Copy this data into the output array.
	 */
	if ((datap + datasize) > (pktdata + pktdatasize))
		growpktdata();
	bcopy((char *)rpkt.data, datap, datasize);
	datap += datasize;
	if (firstpkt) {
		firstpkt = 0;
		*rsize = size;
	}
	*ritems += items;

	/*
	 * Finally, check the count of received packets.  If we've got them
	 * all, return
	 */
	++numrecv;
	if (numrecv <= lastseq)
		goto again;
	return INFO_OKAY;
}


/*
 * sendrequest - format and send a request packet
 */
int
sendrequest(implcode, reqcode, auth, qitems, qsize, qdata)
	int implcode;
	int reqcode;
	int qitems;
	int qsize;
	char *qdata;
	int auth;
{
	struct req_pkt qpkt;
	int datasize;
	extern void auth1crypt();
	extern void auth2crypt();
	extern void authusekey();
	extern int auth_havekey();
	extern char *getpass();
	u_int getkeyid();

	bzero((char *)&qpkt, sizeof qpkt);

	qpkt.rm_vn_mode = RM_VN_MODE(0, 0);
	qpkt.implementation = (u_char)implcode;
	qpkt.request = (u_char)reqcode;

	datasize = qitems * qsize;
	if (datasize != 0 && qdata != NULL) {
		bcopy(qdata, (char *)qpkt.data, datasize);
		qpkt.err_nitems = ERR_NITEMS(0, qitems);
		qpkt.mbz_itemsize = MBZ_ITEMSIZE(qsize);
	} else {
		qpkt.err_nitems = ERR_NITEMS(0, 0);
		qpkt.mbz_itemsize = MBZ_ITEMSIZE(0);
	}
#ifdef DES_OK
	if (!auth) {
#endif /* DES_OK */
		qpkt.auth_seq = AUTH_SEQ(0, 0);
		return sendpkt((char *)&qpkt, REQ_LEN_NOMAC);
#ifdef DES_OK
	} else {
		l_fp ts;
		char *pass;

		if (info_auth_keyid == 0) {
			info_auth_keyid = getkeyid("Keyid: ");
			if (info_auth_keyid == 0) {
				(void) fprintf(stderr,
				   "Keyid must be defined, request not sent\n");
				return 1;
			}
		}
		if (!auth_havekey(info_auth_keyid)) {
			pass = getpass("Password: ");
			if (*pass != '\0')
				authusekey(info_auth_keyid, 3, pass);
		}
		if (auth_havekey(info_auth_keyid)) {
			qpkt.auth_seq = AUTH_SEQ(1, 0);
			qpkt.keyid = htonl(info_auth_keyid);
			auth1crypt(info_auth_keyid, (u_int *)&qpkt,
			    REQ_LEN_NOMAC);
			gettstamp(&ts);
			L_ADD(&ts, &delay_time);
			HTONL_FP(&ts, &qpkt.tstamp);
			auth2crypt(info_auth_keyid, (u_int *)&qpkt,
			    REQ_LEN_NOMAC);
			return sendpkt((char *)&qpkt, REQ_LEN_MAC);
		} else {
			(void) fprintf(stderr,
			    "No password, request not sent\n");
			return 1;
		}
	      }
#endif /* DES_OK */
	/*NOTREACHED*/
}


/*
 * doquery - send a request and process the response
 */
int
doquery(implcode, reqcode, auth, qitems, qsize, qdata, ritems, rsize, rdata)
	int implcode;
	int reqcode;
	int auth;
	int qitems;
	int qsize;
	char *qdata;
	int *ritems;
	int *rsize;
	char **rdata;
{
	int res;
	char junk[512];
	fd_set fds;
	struct timeval tvzero;

	/*
	 * Check to make sure host is open
	 */
	if (!havehost) {
		(void) fprintf(stderr, "***No host open, use `host' command\n");
		return -1;
	}

	/*
	 * Poll the socket and clear out any pending data
	 */
	do {
		tvzero.tv_sec = tvzero.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		res = select(sockfd+1, &fds, (fd_set *)0, (fd_set *)0, &tvzero);

		if (res == -1) {
			warning("polling select", "", "");
			return -1;
		} else if (res > 0)
			(void) read(sockfd, junk, sizeof junk);
	} while (res > 0);


	/*
	 * send a request
	 */
	res = sendrequest(implcode, reqcode, auth, qitems, qsize, qdata);
	if (res != 0)
		return res;
	
	/*
	 * Get the response.  If we got a standard error, print a message
	 */
	res = getresponse(implcode, reqcode, ritems, rsize, rdata);

	if (res > 0) {
		switch(res) {
		case INFO_ERR_IMPL:
			(void) fprintf(stderr,
		     "***Server implementation incompatable with our own\n");
			break;
		case INFO_ERR_REQ:
			(void) fprintf(stderr,
			     "***Server doesn't implement this request\n");
		case INFO_ERR_FMT:
			(void) fprintf(stderr,
"***Server reports a format error in the received packet (shouldn't happen)\n");
			break;
		case INFO_ERR_NODATA:
			(void) fprintf(stderr,
			    "***Server reports data not found\n");
			break;
		case INFO_ERR_AUTH:
			(void) fprintf(stderr, "***Permission denied\n");
			break;
		case ERR_TIMEOUT:
			(void) fprintf(stderr, "***Request timed out\n");
			break;
		case ERR_INCOMPLETE:
			(void) fprintf(stderr,
			    "***Response from server was incomplete\n");
			break;
		default:
			(void) fprintf(stderr,
			    "***Server returns unknown error code %d\n", res);
			break;
		}
	}
	return res;
}


/*
 * getcmds - read commands from the standard input and execute them
 */
void
getcmds()
{
	char line[MAXLINE];
	void docmd();

	for (;;) {
		if (interactive) {
			(void) fputs(prompt, stderr);
			(void) fflush(stderr);
		}

		if (fgets(line, sizeof line, stdin) == NULL)
			return;

		docmd(line);
	}
}


/*
 * abortcmd - catch interrupts and abort the current command
 */
abortcmd()
{
	if (current_output == stdout)
		(void) fflush(stdout);
	putc('\n', stderr);
	(void) fflush(stderr);
	longjmp(interrupt_buf, 1);
}


/*
 * docmd - decode the command line and execute a command
 */
void
docmd(cmdline)
	char *cmdline;
{
	char *tokens[1+MAXARGS+2];
	struct parse pcmd;
	int ntok;
	int i;
	int (*oldintr)();
	struct xcmd *xcmd;
	int findcmd();
	void tokenize();
	void printusage();

	/*
	 * Tokenize the command line.  If nothing on it, return.
	 */
	tokenize(cmdline, tokens, &ntok);
	if (ntok == 0)
		return;
	
	/*
	 * Find the appropriate command description.
	 */
	i = findcmd(tokens[0], builtins, opcmds, &xcmd);
	if (i == 0) {
		(void) fprintf(stderr, "***Command `%s' unknown\n",
		    tokens[0]);
		return;
	} else if (i >= 2) {
		(void) fprintf(stderr, "***Command `%s' ambiguous\n",
		    tokens[0]);
		return;
	}
	
	/*
	 * Save the keyword, then walk through the arguments, interpreting
	 * as we go.
	 */
	pcmd.keyword = tokens[0];
	pcmd.nargs = 0;
	for (i = 0; i < MAXARGS && xcmd->arg[i] != NO; i++) {
		if ((i+1) >= ntok) {
			if (!(xcmd->arg[i] & OPT)) {
				printusage(xcmd, stderr);
				return;
			}
			break;
		}
		if ((xcmd->arg[i] & OPT) && (*tokens[i+1] == '>'))
			break;
		if (!getarg(tokens[i+1], (int)xcmd->arg[i], &pcmd.argval[i]))
			return;
		pcmd.nargs++;
	}

	i++;
	if (i < ntok && *tokens[i] == '>') {
		char *fname;

		if (*(tokens[i]+1) != '\0')
			fname = tokens[i]+1;
		else if ((i+1) < ntok)
			fname = tokens[i+1];
		else {
			(void) fprintf(stderr, "***No file for redirect\n");
			return;
		}

		current_output = fopen(fname, "w");
		if (current_output == NULL) {
			(void) fprintf(stderr, "***Error opening %s: ", fname);
			perror("");
			return;
		}
		i = 1;		/* flag we need a close */
	} else {
		current_output = stdout;
		i = 0;		/* flag no close */
	}

	if (interactive) {
		if (!setjmp(interrupt_buf)) {
			oldintr = (void*)signal(SIGINT, (void (*)(int))abortcmd);
			(xcmd->handler)(&pcmd, current_output);
		}
		(void) signal(SIGINT, (void (*)(int))oldintr);
	} else {
		(xcmd->handler)(&pcmd, current_output);
	}
	if (i) (void) fclose(current_output);
}


/*
 * tokenize - turn a command line into tokens
 */
void
tokenize(line, tokens, ntok)
	char *line;
	char **tokens;
	int *ntok;
{
	register char *cp;
	register char *sp;
	static char tspace[MAXLINE];

	sp = tspace;
	cp = line;
	for (*ntok = 0; *ntok < MAXTOKENS; (*ntok)++) {
		tokens[*ntok] = sp;
		while (ISSPACE(*cp))
			cp++;
		if (ISEOL(*cp))
			break;
		do {
			*sp++ = *cp++;
		} while (!ISSPACE(*cp) && !ISEOL(*cp));

		*sp++ = '\0';
	}
}



/*
 * findcmd - find a command in a command description table
 */
int
findcmd(str, clist1, clist2, cmd)
	register char *str;
	struct xcmd *clist1;
	struct xcmd *clist2;
	struct xcmd **cmd;
{
	register struct xcmd *cl;
	register int clen;
	int nmatch;
	struct xcmd *nearmatch;
	struct xcmd *clist;

	clen = strlen(str);
	nmatch = 0;
	if (clist1 != 0)
		clist = clist1;
	else if (clist2 != 0)
		clist = clist2;
	else
		return 0;

again:
	for (cl = clist; cl->keyword != 0; cl++) {
		/* do a first character check, for efficiency */
		if (*str != *(cl->keyword))
			continue;
		if (strncmp(str, cl->keyword, clen) == 0) {
			/*
			 * Could be extact match, could be approximate.
			 * Is exact if the length of the keyword is the
			 * same as the str.
			 */
			if (*((cl->keyword) + clen) == '\0') {
				*cmd = cl;
				return 1;
			}
			nmatch++;
			nearmatch = cl;
		}
	}

	/*
	 * See if there is more to do.  If so, go again.  Sorry about the
	 * goto, too much looking at BSD sources...
	 */
	if (clist == clist1 && clist2 != 0) {
		clist = clist2;
		goto again;
	}

	/*
	 * If we got extactly 1 near match, use it, else return number
	 * of matches.
	 */
	if (nmatch == 1) {
		*cmd = nearmatch;
		return 1;
	}
	return nmatch;
}


/*
 * getarg - interpret an argument token
 */
int
getarg(str, code, argp)
	char *str;
	int code;
	arg_v *argp;
{
	int isneg;
	char *cp, *np;
	int getnetnum();
	static char *digits = "0123456789";

	switch (code & ~OPT) {
	case STR:
		argp->string = str;
		break;
	case ADD:
		if (!getnetnum(str, &(argp->netnum), (char *)0)) {
			return 0;
		}
		break;
	case INT:
	case UINT:
		isneg = 0;
		np = str;
		if (*np == '-') {
			np++;
			isneg = 1;
		}

		argp->uval = 0;
		do {
			cp = index(digits, *np);
			if (cp == NULL) {
				(void) fprintf(stderr,
				    "***Illegal integer value %s\n", str);
				return 0;
			}
			argp->uval *= 10;
			argp->uval += (cp - digits);
		} while (*(++np) != '\0');

		if (isneg) {
			if ((code & ~OPT) == UINT) {
				(void) fprintf(stderr,
				    "***Value %s should be unsigned\n", str);
				return 0;
			}
			argp->ival = -argp->ival;
		}
		break;
	}

	return 1;
}


/*
 * getnetnum - given a host name, return its net number
 *	       and (optional) full name
 */
int
getnetnum(host, num, fullhost)
	char *host;
	u_int *num;
	char *fullhost;
{
	struct hostent *hp;
	int decodenetnum();

	if (decodenetnum(host, num)) {
		if (fullhost != 0) {
			(void) sprintf(fullhost,
			    "%d.%d.%d.%d", ((htonl(*num)>>24)&0xff),
			    ((htonl(*num)>>16)&0xff), ((htonl(*num)>>8)&0xff),
			    (htonl(*num)&0xff));
		}
		return 1;
	} else if ((hp = gethostbyname(host)) != 0) {
		bcopy(hp->h_addr, (char *)num, sizeof(u_int));
		if (fullhost != 0)
			(void) strcpy(fullhost, hp->h_name);
		return 1;
	} else {
		(void) fprintf(stderr, "***Can't find host %s\n", host);
		return 0;
	}
	/*NOTREACHED*/
}

/*
 * decodenetnum - return a net number (this is crude, but careful)
 */
int
decodenetnum(num, netnum)
	char *num;
	u_int *netnum;
{
	register char *cp;
	register char *bp;
	register int i;
	register int temp;
	char buf[80];		/* will core dump on really stupid stuff */

	cp = num;
	*netnum = 0;
	for (i = 0; i < 4; i++) {
		bp = buf;
		while (isdigit(*cp))
			*bp++ = *cp++;
		if (bp == buf)
			break;

		if (i < 3) {
			if (*cp++ != '.')
				break;
		} else if (*cp != '\0')
			break;

		*bp = '\0';
		temp = atoi(buf);
		if (temp > 255)
			break;
		*netnum <<= 8;
		*netnum += temp;
	}
	
	if (i < 4)
		return 0;
	*netnum = htonl(*netnum);
	return 1;
}


/*
 * nntohost - convert network number to host name.  This routine enforces
 *	       the showhostnames setting.
 */
char *
nntohost(netnum)
	u_int netnum;
{
	extern char *numtoa();
	extern char *numtohost();

	if (!showhostnames)
		return numtoa(netnum);
	return numtohost(netnum);
}


/*
 * Finally, the built in command handlers
 */

/*
 * help - tell about commands, or details of a particular command
 */
void
help(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	int i;
	int n;
	struct xcmd *xcp;
	char *cmd;
	char *cmdsort[100];
	int length[100];
	int maxlength;
	int numperline;
	int helpsort();
	int findcmd();
	void printusage();
	static char *spaces = "                    ";	/* 20 spaces */

	if (pcmd->nargs == 0) {
		n = 0;
		for (xcp = builtins; xcp->keyword != 0; xcp++) {
			if (*(xcp->keyword) != '?')
				cmdsort[n++] = xcp->keyword;
		}
		for (xcp = opcmds; xcp->keyword != 0; xcp++)
			cmdsort[n++] = xcp->keyword;

		qsort((char *)cmdsort, n, sizeof(char *), helpsort);

		maxlength = 0;
		for (i = 0; i < n; i++) {
			length[i] = strlen(cmdsort[i]);
			if (length[i] > maxlength)
				maxlength = length[i];
		}
		maxlength++;
		numperline = 76 / maxlength;

		(void) fprintf(fp, "Commands available:\n");
		for (i = 0; i < n; i++) {
			if ((i % numperline) == (numperline-1)
			    || i == (n-1))
				(void) fprintf(fp, "%s\n", cmdsort[i]);
			else
				(void) fprintf(fp, "%s%s", cmdsort[i],
				    spaces+20-maxlength+length[i]);
		}
	} else {
		cmd = pcmd->argval[0].string;
		n = findcmd(cmd, builtins, opcmds, &xcp);
		if (n == 0) {
			(void) fprintf(stderr,
			    "Command `%s' is unknown\n", cmd);
			return;
		} else if (n >= 2) {
			(void) fprintf(stderr,
			    "Command `%s' is ambiguous\n", cmd);
			return;
		}
		(void) fprintf(fp, "function: %s\n", xcp->comment);
		printusage(xcp, fp);
	}
}


/*
 * helpsort - do hostname qsort comparisons
 */
int
helpsort(name1, name2)
	char **name1;
	char **name2;
{
	return strcmp(*name1, *name2);
}


/*
 * printusage - print usage information for a command
 */
void
printusage(xcp, fp)
	struct xcmd *xcp;
	FILE *fp;
{
	register int i;

	(void) fprintf(fp, "usage: %s", xcp->keyword);
	for (i = 0; i < MAXARGS && xcp->arg[i] != NO; i++) {
		if (xcp->arg[i] & OPT)
			(void) fprintf(fp, " [ %s ]", xcp->desc[i]);
		else
			(void) fprintf(fp, " %s", xcp->desc[i]);
	}
	(void) fprintf(fp, "\n");
}


/*
 * timeout - set time out time
 */
void
timeout(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	int val;

	if (pcmd->nargs == 0) {
		val = tvout.tv_sec * 1000 + tvout.tv_usec / 1000;
		(void) fprintf(fp, "primary timeout %d ms\n", val);
	} else {
		tvout.tv_sec = pcmd->argval[0].uval / 1000;
		tvout.tv_usec = (pcmd->argval[0].uval - (tvout.tv_sec * 1000))
		    * 1000;
	}
}

#ifdef DES_OK
/*
 * delay - set delay for auth requests
 */
void
delay(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	int isneg;
	u_int val;

	if (pcmd->nargs == 0) {
		val = delay_time.l_ui * 1000 + delay_time.l_uf / 4294967;
		(void) fprintf(fp, "delay %d ms\n", val);
	} else {
		if (pcmd->argval[0].ival < 0) {
			isneg = 1;
			val = (u_int)(-pcmd->argval[0].ival);
		} else {
			isneg = 0;
			val = (u_int)pcmd->argval[0].ival;
		}

		delay_time.l_ui = val / 1000;
		val %= 1000;
		delay_time.l_uf = val * 4294967;	/* 2**32/1000 */

		if (isneg)
			L_NEG(&delay_time);
	}
}
#endif /* DES_OK */


/*
 * host - set the host we are dealing with.
 */
void
host(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	if (pcmd->nargs == 0) {
		if (havehost)
			(void) fprintf(fp, "current host is %s\n", currenthost);
		else
			(void) fprintf(fp, "no current host\n");
	} else if (openhost(pcmd->argval[0].string)) {
		(void) fprintf(fp, "current host set to %s\n", currenthost);
	} else {
		if (havehost)
			(void) fprintf(fp,
			    "current host remains %s\n", currenthost);
		else
			(void) fprintf(fp, "still no current host\n");
	}
}

#ifdef DES_OK

/*
 * poll - do one (or more) polls of the host via NTP
 */
void
poll(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	(void) fprintf(fp, "poll not implemented yet\n");
}

/*
 * keyid - get a keyid to use for authenticating requests
 */
void
keyid(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	if (pcmd->nargs == 0) {
		if (info_auth_keyid == 0)
			(void) fprintf(fp, "no keyid defined\n");
		else
			(void) fprintf(fp, "keyid is %u\n", info_auth_keyid);
	} else {
		info_auth_keyid = pcmd->argval[0].uval;
	}
}
#endif /* DES_OK */


#ifdef DES_OK
/*
 * passwd - get an authentication key
 */
void
passwd(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
        char *pass;
	u_int getkeyid();
	extern char *getpass();
	extern void authusekey();

	if (info_auth_keyid == 0) {
		info_auth_keyid = getkeyid("Keyid: ");
		if (info_auth_keyid == 0) {
			(void)fprintf(fp, "Keyid must be defined\n");
			return;
		}
	}
	pass = getpass("Password: ");
	if (*pass == '\0')
		(void) fprintf(fp, "Password unchanged\n");
	else
		authusekey(info_auth_keyid, 3, pass);

}
#endif /* DES_OK */

/*
 * hostnames - set the showhostnames flag
 */
void
hostnames(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	if (pcmd->nargs == 0) {
		if (showhostnames)
			(void) fprintf(fp, "hostnames being shown\n");
		else
			(void) fprintf(fp, "hostnames not being shown\n");
	} else {
		if (STREQ(pcmd->argval[0].string, "yes"))
			showhostnames = 1;
		else if (STREQ(pcmd->argval[0].string, "no"))
			showhostnames = 0;
		else
			(void)fprintf(stderr, "What?\n");
	}
}


/*
 * setdebug - set/change debugging level
 */
void
setdebug(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	if (pcmd->nargs == 0) {
		(void) fprintf(fp, "debug level is %d\n", debug);
		return;
	} else if (STREQ(pcmd->argval[0].string, "no")) {
		debug = 0;
	} else if (STREQ(pcmd->argval[0].string, "more")) {
		debug++;
	} else if (STREQ(pcmd->argval[0].string, "less")) {
		debug--;
	} else {
		(void) fprintf(fp, "What?\n");
		return;
	}
	(void) fprintf(fp, "debug level set to %d\n", debug);
}


/*
 * quit - stop this nonsense
 */
void
quit(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	if (havehost)
		(void) close(sockfd);	/* cleanliness next to godliness */
	exit(0);
}


/*
 * version - print the current version number
 */
void
version(pcmd, fp)
	struct parse *pcmd;
	FILE *fp;
{
	extern char *Version;

	(void) fprintf(fp, "%s\n", Version);
}


/*
 * warning - print a warning message
 */
warning(fmt, st1, st2)
	char *fmt;
	char *st1;
	char *st2;
{
	(void) fprintf(stderr, "%s: ", progname);
	(void) fprintf(stderr, fmt, st1, st2);
	(void) fprintf(stderr, ": ");
	perror("");
}


/*
 * error - print a message and exit
 */
error(fmt, st1, st2)
	char *fmt;
	char *st1;
	char *st2;
{
	warning(fmt, st1, st2);
	exit(1);
}

/*
 * getkeyid - prompt the user for a keyid to use
 */
u_int
getkeyid(prompt)
char *prompt;
{
	register char *p;
	register c;
	FILE *fi;
	char pbuf[20];

	if ((fi = fdopen(open("/dev/tty", 2), "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	fprintf(stderr, "%s", prompt); fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[18])
			*p++ = c;
	}
	*p = '\0';
	if (fi != stdin)
		fclose(fi);
	return (u_int)atoi(pbuf);
}
