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
static char *rcsid = "@(#)$RCSfile: clnt_kudp.c,v $ $Revision: 1.1.12.2 $ (DEC) $Date: 1993/03/15 12:03:23 $";
#endif
#ifdef	KERNEL
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = 	"@(#)clnt_kudp.c	1.8 90/07/19 4.1NFSSRC Copyr 1990 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * 1.59 88/02/16
 */


/*
 * clnt_kudp.c
 * Implements a kernel UPD/IP based, client side RPC.
 */

#include <rpc/rpc.h>
#include <sys/mbuf.h>


struct mbuf	*ku_recvfrom();
int		ckuwakeup();

enum clnt_stat	clntkudp_callit();
void		clntkudp_abort();
void		clntkudp_error();
bool_t		clntkudp_freeres();
bool_t		clntkudp_control();
void		clntkudp_destroy();

void		xdrmbuf_init();

/*
 * Operations vector for UDP/IP based RPC
 */
/* static */ struct clnt_ops udp_ops = {
	clntkudp_callit,	/* do rpc call */
	clntkudp_abort,		/* abort call */
	clntkudp_error,		/* return error status */
	clntkudp_freeres,	/* free results */
	clntkudp_destroy,	/* destroy rpc handle */
	clntkudp_control	/* the ioctl() of rpc */
};

struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
	int	rcbadverfs;
	int	rctimers;
} rcstat;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010

/* Times to retry */
#define	RECVTRIES	2
#define	SNDTRIES	4

u_int	clntxid;	/* transaction id used by all clients */

static
noop(addr, size, p)
	caddr_t addr;
	u_int	size;
	struct cku_private *p;
{
}

static
buffree(addr, size, p)
	caddr_t addr;
	u_int	size;
	struct cku_private *p;
{
	register int s;

	s = splimp();
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		p->cku_flags &= ~CKU_BUFWANTED;
		wakeup((caddr_t)&p->cku_outbuf);
	}
	splx(s);
}

/*
 * Create an rpc handle for a udp rpc connection.
 * Allocates space for the handle structure and the private data, and
 * opens a socket.  Note sockets and handles are one to one.
 *
 * A note on clntxid initialization:
 * In order to avoid accidentally reusing XIDs over reboots, we try to
 * set clntxid to something higher than what had last been used.  Since
 * we can't do much more than 1000 NFS calls per second, multiplying by
 * 4096 ought to be safe.  That gives us a 12 day window before we wrap
 * around.  Old code tried a lousy attempt as a random number, this has
 * a better, albeit imperfect, chance of being right.
 */
CLIENT *
clntkudp_create(addr, pgm, vers, retrys, cred)
	struct sockaddr_in *addr;
	u_int pgm;
	u_int vers;
	int retrys;
	struct ucred *cred;
{
	register CLIENT *h;
	struct cku_private *p;
	int error = 0;
	struct rpc_msg call_msg;
	struct mbuf *m, *mclgetx();
	extern int nfs_portmon;
	static int xid_initted;

	p = (struct cku_private *)kmem_alloc(sizeof(struct cku_private));
	if (p == (struct cku_private *)0)
		panic("clntkudp_create: can't alloc cku_private");
	bzero((caddr_t)p, sizeof(struct cku_private));

	h = ptoh(p);

	if (!xid_initted) {
		xid_initted = 1; /* Just once! */
		clntxid = time.tv_sec << 12;
	}

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clntkudp_init(h, addr, retrys, cred);
	p->cku_outbuf = (char *)kmem_alloc((u_int)UDPMSGSIZE);
	m = mclgetx(noop, (caddr_t)NULL, p->cku_outbuf, UDPMSGSIZE, M_WAIT);
	if (m == NULL)
		goto bad;
	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		printf("clntkudp_create - Fatal header serialization error.");
		(void) m_freem(m);
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	(void) m_free(m);

	/* open udp socket */
	error = socreate(AF_INET, &p->cku_sock, SOCK_DGRAM, IPPROTO_UDP);
	if (error) {
		printf("clntkudp_create: socket creation problem, %d", error);
		goto bad;
	}
	if (error = bindresvport(p->cku_sock)) {
		printf("clntkudp_create: socket bind problem, %d", error);
		goto bad;
	}
	return (h);

bad:
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, (u_int)sizeof (struct cku_private));
	return ((CLIENT *)NULL);
}

clntkudp_init(h, addr, retrys, cred)
	CLIENT *h;
	struct sockaddr_in *addr;
	int retrys;
	struct ucred *cred;
{
	struct cku_private *p = htop(h);

	p->cku_retrys = retrys;
	p->cku_addr = *addr;
	p->cku_cred = cred;
	p->cku_xid = 0;
	p->cku_flags &= (CKU_BUFBUSY | CKU_BUFWANTED);
}

/*
 * set the timers.  Return current retransmission timeout.
 */
clntkudp_settimers(h, t, all, minimum, feedback, arg, xid)
	CLIENT *h;
	struct rpc_timers *t, *all;
	unsigned int minimum;
	void (*feedback)();
	caddr_t arg;
	u_int xid;
{
	struct cku_private *p = htop(h);
	int value;

	p->cku_feedback = feedback;
	p->cku_feedarg = arg;
	p->cku_timers = t;
	p->cku_timeall = all;
	if (xid)
		p->cku_xid = xid;
	else
		p->cku_xid = alloc_xid();
	value = all->rt_rtxcur;
	value += t->rt_rtxcur;
	if (value < minimum)
		return (minimum);
	rcstat.rctimers++;
	return (value);
}

/*
 * Time out back off function. tim is in hz
 */
#define	MAXTIMO	(20 * hz)
#define	backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * Call remote procedure.
 * Most of the work of rpc is done here.  We serialize what is left
 * of the header (some was pre-serialized in the handle), serialize
 * the arguments, and send it off.  We wait for a reply or a time out.
 * Timeout causes an immediate return, other packet problems may cause
 * a retry on the receive.  When a good packet is received we deserialize
 * it, and check verification.  A bad reply code will cause one retry
 * with full (longhand) credentials.
 * If "ignorebad" is true, rpc replies with remote errors are ignored.
 */
enum clnt_stat
clntkudp_callit_addr(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait,
		sin, ignorebad)
	register CLIENT	*h;
	u_int		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
	struct sockaddr_in *sin;
	int	ignorebad;
{
	register struct cku_private *p = htop(h);
	register XDR		   *xdrs;
	register struct socket	   *so = p->cku_sock;
	int			   rtries;
	int			   stries = p->cku_retrys;
	int			   s;
	struct ucred		   *tmpcred;
	struct mbuf		   *m;
	int timohz;
	u_int xid;
	u_int rempos = 0;
	int refreshes = 2;	/* number of times to refresh credential */
	int round_trip;		/* time the RPC */
	int interrupted;	/* return from sleep() */
	sigset_t smask;		/* saved signal mask */
	struct proc *pp = u.u_procp;

# define time_in_hz (time.tv_sec*hz + time.tv_usec/(1000000/hz))

	rcstat.rccalls++;

	while (p->cku_flags & CKU_BUSY) {
		rcstat.rcwaits++;
		p->cku_flags |= CKU_WANTED;
		(void) sleep((caddr_t)h, PZERO-2);
	}
	p->cku_flags |= CKU_BUSY;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_cred;
	u.u_cred = p->cku_cred;

	if (p->cku_xid == 0)
		xid = alloc_xid();
	else
		xid = p->cku_xid;

	/*
	 * This is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * hz + (wait.tv_usec * hz) / 1000000;

call_again:
	/*
	 * Wait til buffer gets freed then make a type 2 mbuf point at it
	 * The buffree routine clears CKU_BUFBUSY and does a wakeup when
	 * the mbuf gets freed.
	 */
	s = splimp();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * loop-back call.  The client can block wainting for
		 * the server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf.  Avoid this
		 * by flushing the input queue every once in a while while
		 * we are waiting.
		 */
		timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
		(void) sleep((caddr_t)&p->cku_outbuf, PZERO-3);
		if (!(p->cku_flags & CKU_BUFBUSY)) {
			/* probably woke up from buffree */
			untimeout(wakeup, (caddr_t)&p->cku_outbuf);
		}
		sbflush(&so->so_rcv);
	}
	p->cku_flags |= CKU_BUFBUSY;
	(void) splx(s);
	m = mclgetx(buffree, p, p->cku_outbuf, UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		p->cku_err.re_status = RPC_SYSTEMERROR;
		p->cku_err.re_errno = ENOBUFS;
		buffree(p->cku_outbuf, UDPMSGSIZE, p);
		goto done;
	}
	xdrs = &p->cku_outxdr;
	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	(*(u_int *)(p->cku_outbuf)) = xid;

	xdrmbuf_init(xdrs, m, XDR_ENCODE);

	if (rempos != 0) {
		XDR_SETPOS(xdrs, rempos);
	} else {
		/*
		 * Serialize dynamic stuff into the output buffer.
		 */
		XDR_SETPOS(xdrs, p->cku_outpos);
		if ((! XDR_PUTLONG(xdrs, (int *)&procnum)) ||
		    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
		    (! (*xdr_args)(xdrs, argsp))) {
			p->cku_err.re_status = RPC_CANTENCODEARGS;
			p->cku_err.re_errno = EIO;
			(void) m_freem(m);
			goto done;
		}
		rempos = XDR_GETPOS(xdrs);
	}
	m->m_pkthdr.len = m->m_len = rempos;

	round_trip = time_in_hz;
	so->so_error = 0;
	if ((p->cku_err.re_errno = ku_sendto_mbuf(so, m, &p->cku_addr)) != 0) {
		p->cku_err.re_status = RPC_CANTSEND;
		p->cku_err.re_errno = EIO;
		goto done;
	}

recv_again:
	for (rtries = RECVTRIES; rtries; rtries--) {
		s = splnet();
		while (so->so_rcv.sb_cc == 0) {
			/*
			 * Set timeout then wait for input, timeout
			 * or interrupt.
			 */
			timeout(ckuwakeup, (caddr_t)p, timohz);
			so->so_rcv.sb_flags |= SB_WAIT;
			interrupted = tsleep((caddr_t)&so->so_rcv.sb_cc,
				PZERO-1, "callit", 0);
			untimeout(ckuwakeup, (caddr_t)p);
			if ((interrupted) || (so->so_error == EINTR)) {
				(void) splx(s);
				p->cku_err.re_status = RPC_INTR;
				p->cku_err.re_errno = EINTR;
				goto done;
			}
			if (p->cku_flags & CKU_TIMEDOUT) {
				p->cku_flags &= ~CKU_TIMEDOUT;
				if (p->cku_flags & CKU_BUFBUSY)
					arphasmbuf(m);
				(void) splx(s);
				p->cku_err.re_status = RPC_TIMEDOUT;
				p->cku_err.re_errno = ETIMEDOUT;
				rcstat.rctimeouts++;
				goto done;
			}
		}

		if (so->so_error) {
			so->so_error = 0;
			(void) splx(s);
			continue;
		}

		{
			/*
			 * Declare this variable here to have smaller
			 * demand for stack space in this procedure.
			 */
			struct sockaddr_in from;

			p->cku_inmbuf = ku_recvfrom(so, &from);
			if (sin) {
				*sin = from;
			}
		}

		(void) splx(s);
		if (p->cku_inmbuf) /* xdr_replymsg() wants at least three XDR_UNITs */
			p->cku_inmbuf = m_pullup(p->cku_inmbuf, 3 * BYTES_PER_XDR_UNIT);
                if (p->cku_inmbuf == NULL) {
                        printf("clntkudp_callit_addr: inmbuf too short\n");
                        continue;
                }

		p->cku_inbuf = mtod(p->cku_inmbuf, char *);
		if (p->cku_inmbuf->m_len < sizeof (u_int)) {
			printf("clntkudp_callit_addr: inmbuf too short %d\n",p->cku_inmbuf->m_len);
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * If reply transaction id matches id sent we have a good packet.
		 * If it doesn't match, then it's probably a late arrival, so
		 * let's not allow rtries to be decremented by the for loop.
		 */
		if (*((u_int *)(p->cku_inbuf))
		    != *((u_int *)(p->cku_outbuf))) {
			rcstat.rcbadxids++;
			m_freem(p->cku_inmbuf);
			rtries++;
			continue;
		}

		break;
	}

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	round_trip = time_in_hz - round_trip;
	/*
	 * Van Jacobson timer algorithm here, only if NOT a retransmission.
	 */
	if (p->cku_timers != (struct rpc_timers *)0 &&
	    (stries == p->cku_retrys) && !ignorebad) {
		register int rt;

		rt = round_trip;
		rt -= (p->cku_timers->rt_srtt >> 3);
		p->cku_timers->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timers->rt_deviate >> 2);
		p->cku_timers->rt_deviate += rt;
		p->cku_timers->rt_rtxcur =
			((p->cku_timers->rt_srtt >> 2) +
			    p->cku_timers->rt_deviate) >> 1;

		rt = round_trip;
		rt -= (p->cku_timeall->rt_srtt >> 3);
		p->cku_timeall->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timeall->rt_deviate >> 2);
		p->cku_timeall->rt_deviate += rt;
		p->cku_timeall->rt_rtxcur =
			((p->cku_timeall->rt_srtt >> 2) +
			    p->cku_timeall->rt_deviate) >> 1;
		if (p->cku_feedback != (void (*)()) 0)
		    (*p->cku_feedback)(FEEDBACK_OK, procnum, p->cku_feedarg);
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmbuf_init(xdrs, p->cku_inmbuf, XDR_DECODE);

	{
		/*
		 * Declare this variable here to have smaller
		 * demand for stack space in this procedure.
		 */
		struct rpc_msg		   reply_msg;

		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = resultsp;
		reply_msg.acpted_rply.ar_results.proc = xdr_results;

		/*
		 * Decode and validate the response.
		 */
		if (xdr_replymsg(xdrs, &reply_msg)) {
			_seterr_reply(&reply_msg, &(p->cku_err));

			if (p->cku_err.re_status == RPC_SUCCESS) {
				/*
				 * Reply is good, check auth.
				 */
				if (! AUTH_VALIDATE(h->cl_auth,
				    &reply_msg.acpted_rply.ar_verf)) {
					p->cku_err.re_status = RPC_AUTHERROR;
					p->cku_err.re_why = AUTH_INVALIDRESP;
					rcstat.rcbadverfs++;
				}
				if (reply_msg.acpted_rply.ar_verf.oa_base !=
				    NULL) {
					/* free auth handle */
					xdrs->x_op = XDR_FREE;
					(void) xdr_opaque_auth(xdrs,
					    &(reply_msg.acpted_rply.ar_verf));
				}
			} else {
				p->cku_err.re_errno = EIO;

				/*
				 * Maybe our credential needs refreshed
				 */
				if (refreshes > 0 && AUTH_REFRESH(h->cl_auth)) {
					refreshes--;
					rcstat.rcnewcreds++;
					rempos = 0;
				}
			}
		} else {
			p->cku_err.re_status = RPC_CANTDECODERES;
			p->cku_err.re_errno = EIO;
		}
	}

	m_freem(p->cku_inmbuf);

done:
	/*
	 * Weed out remote rpc error replies.
	 */
	if (ignorebad &&
		((p->cku_err.re_status == RPC_VERSMISMATCH) ||
		    (p->cku_err.re_status == RPC_AUTHERROR) ||
		    (p->cku_err.re_status == RPC_PROGUNAVAIL) ||
		    (p->cku_err.re_status == RPC_PROGVERSMISMATCH) ||
		    (p->cku_err.re_status == RPC_PROCUNAVAIL) ||
		    (p->cku_err.re_status == RPC_CANTDECODEARGS) ||
		    (p->cku_err.re_status == RPC_SYSTEMERROR)))
		goto recv_again;

	/*
	 * Flush the rest of the stuff on the input queue
	 * for the socket.
	 */
	s = splnet();
	sbflush(&so->so_rcv);
	(void) splx(s);

	if ((p->cku_err.re_status != RPC_SUCCESS) &&
	    (p->cku_err.re_status != RPC_INTR) &&
	    (p->cku_err.re_status != RPC_CANTENCODEARGS)) {
		if (p->cku_feedback != (void (*)()) 0 &&
		    stries == p->cku_retrys)
			(*p->cku_feedback)(FEEDBACK_REXMIT1,
				procnum, p->cku_feedarg);
		timohz = backoff(timohz);
		if (p->cku_timeall != (struct rpc_timers *)0)
			p->cku_timeall->rt_rtxcur = timohz;
		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack o resources, wait a bit
			 * and try again.
			 */
			(void) sleep((caddr_t)&lbolt, PZERO-4);
		}
		if (--stries > 0) {
			rcstat.rcretrans++;
			goto call_again;
		}
	}
	u.u_cred = tmpcred;
	/*
	 * Insure that buffer is not busy prior to releasing client handle.
	 */
	s = splimp();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
		(void) sleep((caddr_t)&p->cku_outbuf, PZERO-3);
		if (!(p->cku_flags & CKU_BUFBUSY)) {
			/* probably woke up from buffree */
			untimeout(wakeup, (caddr_t)&p->cku_outbuf);
		}
		sbflush(&so->so_rcv);
	}
	(void) splx(s);
	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
		wakeup((caddr_t)h);
	}
	if (p->cku_err.re_status != RPC_SUCCESS) {
		rcstat.rcbadcalls++;
	}
	return (p->cku_err.re_status);
}

enum clnt_stat
clntkudp_callit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
	register CLIENT *h;
	u_int		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
{
	return (clntkudp_callit_addr(h, procnum, xdr_args, argsp, xdr_results,
		resultsp, wait, (struct sockaddr_in *)0, 0));
}

/*
 * Wake up client waiting for a reply.
 */
ckuwakeup(p)
	register struct cku_private *p;
{
	register struct socket	*so = p->cku_sock;

	p->cku_flags |= CKU_TIMEDOUT;
	sowakeup(so, &so->so_rcv);
}

/*
 * Return error info on this handle.
 */
void
clntkudp_error(h, err)
	CLIENT *h;
	struct rpc_err *err;
{
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
}

/* static */ bool_t
clntkudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

void
clntkudp_abort()
{
}

bool_t
clntkudp_control()
{
	return (FALSE);
}


/*
 * Destroy rpc handle.
 * Frees the space used for output buffer, private data, and handle
 * structure, and closes the socket for this handle.
 */
void
clntkudp_destroy(h)
	CLIENT *h;
{
	register struct cku_private *p = htop(h);

	(void) soclose(p->cku_sock);
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, sizeof (*p));
}

/*
 * try to bind to a reserved port
 */
bindresvport(so)
	struct socket *so;
{
	struct sockaddr_in *sin;
	struct mbuf *m;
	u_short i;
	int error;
	struct ucred *tmpcred;
	struct ucred *savecred;

#	define MAX_PRIV	(IPPORT_RESERVED-1)
#	define MIN_PRIV	(IPPORT_RESERVED/2)

	m = m_get(M_WAIT, MT_SONAME);
	if (m == NULL) {
		printf("bindresvport: couldn't alloc mbuf");
		return (ENOBUFS);
	}

	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	m->m_len = sizeof (struct sockaddr_in);

	/*
	 * Only root can bind to a privileged port number, so
	 * temporarily change the uid to 0 to do the bind.
	 */
	tmpcred = crdup(u.u_cred);
	savecred = u.u_cred;
	u.u_cred = tmpcred;
	u.u_uid = 0;
	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
		error = sobind(so, m);
	}
	(void) m_freem(m);
	u.u_cred = savecred;
	crfree(tmpcred);
	return (error);
}

#endif	KERNEL
