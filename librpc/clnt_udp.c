/*********************************************************************
 * RPC for the Windows NT Operating System
 * 1993 by Martin F. Gergeleit
 * Users may use, copy or modify Sun RPC for the Windows NT Operating 
 * System according to the Sun copyright below.
 *
 * RPC for the Windows NT Operating System COMES WITH ABSOLUTELY NO 
 * WARRANTY, NOR WILL I BE LIABLE FOR ANY DAMAGES INCURRED FROM THE 
 * USE OF. USE ENTIRELY AT YOUR OWN RISK!!!
 *********************************************************************/

/* @(#)clnt_udp.c	2.2 88/08/01 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_udp.c 1.39 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * clnt_udp.c, Implements a UDP/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/time.h>

#include "bindresv.h"
#include "gettimeofday.h"

#include <rpc/rpc.h>
#ifdef WIN32
#include <errno.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>

extern int errno;
#endif

#include <rpc/pmap_clnt.h>

/*
 * UDP bases client side rpc operations
 */
static enum clnt_stat	clntudp_call();
static void		clntudp_abort();
static void		clntudp_geterr();
static bool_t		clntudp_freeres();
static bool_t           clntudp_control();
static void		clntudp_destroy();

static struct clnt_ops udp_ops = {
	clntudp_call,
	clntudp_abort,
	clntudp_geterr,
	clntudp_freeres,
	clntudp_destroy,
	clntudp_control
};

/*
 * Private data kept per client handle
 */
struct cu_data {
	int		   cu_sock;
	bool_t		   cu_closeit;
	struct sockaddr_in cu_raddr;
	int		   cu_rlen;
	struct timeval	   cu_wait;
	struct timeval     cu_total;
	struct rpc_err	   cu_error;
	XDR		   cu_outxdrs;
	u_int		   cu_xdrpos;
	u_int		   cu_sendsz;
	char		   *cu_outbuf;
	u_int		   cu_recvsz;
	char		   cu_inbuf[1];
};

/*
 * Create a UDP based client handle.
 * If *sockp<0, *sockp is set to a newly created UPD socket.
 * If raddr->sin_port is 0 a binder on the remote machine
 * is consulted for the correct port number.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is initialized to null authentication.
 *     Caller may wish to set this something more useful.
 *
 * wait is the amount of time used between retransmitting a call if
 * no response has been heard;  retransmition occurs until the actual
 * rpc call times out.
 *
 * sendsz and recvsz are the maximum allowable packet sizes that can be
 * sent and received.
 */
CLIENT *
clntudp_bufcreate(raddr, program, version, wait, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl;
	register struct cu_data *cu;
	struct timeval now;
	struct rpc_msg call_msg;

	cl = (CLIENT *)mem_alloc(sizeof(CLIENT));
	if (cl == NULL) {
#ifdef WIN32
		nt_rpc_report("clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = ENOMEM;
#else
		(void) fprintf(stderr, "clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
#endif
		goto fooy;
	}
	sendsz = ((sendsz + 3) / 4) * 4;
	recvsz = ((recvsz + 3) / 4) * 4;
	cu = (struct cu_data *)mem_alloc(sizeof(*cu) + sendsz + recvsz);
	if (cu == NULL) {
#ifdef WIN32
		nt_rpc_report("clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = ENOMEM;
#else
		(void) fprintf(stderr, "clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
#endif
		goto fooy;
	}
	cu->cu_outbuf = &cu->cu_inbuf[recvsz];

	(void)gettimeofday(&now, (struct timezone *)0);
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port =
		    pmap_getport(raddr, program, version, IPPROTO_UDP)) == 0) {
			goto fooy;
		}
		raddr->sin_port = htons(port);
	}
	cl->cl_ops = &udp_ops;
	cl->cl_private = (caddr_t)cu;
	cu->cu_raddr = *raddr;
	cu->cu_rlen = sizeof (cu->cu_raddr);
	cu->cu_wait = wait;
	cu->cu_total.tv_sec = -1;
	cu->cu_total.tv_usec = -1;
	cu->cu_sendsz = sendsz;
	cu->cu_recvsz = recvsz;
	call_msg.rm_xid = getpid() ^ now.tv_sec ^ now.tv_usec;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = program;
	call_msg.rm_call.cb_vers = version;
	xdrmem_create(&(cu->cu_outxdrs), cu->cu_outbuf,
	    sendsz, XDR_ENCODE);
	if (! xdr_callhdr(&(cu->cu_outxdrs), &call_msg)) {
		goto fooy;
	}
	cu->cu_xdrpos = XDR_GETPOS(&(cu->cu_outxdrs));
	if (*sockp < 0) {
		int dontblock = 1;

		*sockp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef WIN32
		if (*sockp == INVALID_SOCKET) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = WSAerrno;
#else
		if (*sockp < 0) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
#endif
			goto fooy;
		}
		/* attempt to bind to prov port */
		(void)bindresvport(*sockp, (struct sockaddr_in *)0);
		/* the sockets rpc controls are non-blocking */
#ifdef WIN32
		(void)ioctlsocket(*sockp, FIONBIO, (void *) &dontblock);
#else
		(void)ioctl(*sockp, FIONBIO, (void *) &dontblock);
#endif
		cu->cu_closeit = TRUE;
	} else {
		cu->cu_closeit = FALSE;
	}
	cu->cu_sock = *sockp;
	cl->cl_auth = authnone_create();
	return (cl);
fooy:
	if (cu)
		mem_free((caddr_t)cu, sizeof(*cu) + sendsz + recvsz);
	if (cl)
		mem_free((caddr_t)cl, sizeof(CLIENT));
	return ((CLIENT *)NULL);
}

CLIENT *
clntudp_create(raddr, program, version, wait, sockp)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
{

	return(clntudp_bufcreate(raddr, program, version, wait, sockp,
	    UDPMSGSIZE, UDPMSGSIZE));
}

static enum clnt_stat
clntudp_call(cl, proc, xargs, argsp, xresults, resultsp, utimeout)
	register CLIENT	*cl;		/* client handle */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	struct timeval	utimeout;	/* seconds to wait before giving up */
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs;
	register int outlen;
	register int inlen;
	int fromlen;
#ifdef FD_SETSIZE
	fd_set readfds;
	fd_set mask;
#else
	int readfds;
	register int mask;
#endif /* def FD_SETSIZE */
	struct sockaddr_in from;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	struct timeval time_waited;
	bool_t ok;
	int nrefreshes = 2;	/* number of times to refresh cred */
	struct timeval timeout;

	if (cu->cu_total.tv_usec == -1) {
		timeout = utimeout;     /* use supplied timeout */
	} else {
		timeout = cu->cu_total; /* use default timeout */
	}

	time_waited.tv_sec = 0;
	time_waited.tv_usec = 0;
call_again:
	xdrs = &(cu->cu_outxdrs);
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, cu->cu_xdrpos);
	/*
	 * the transaction is the first thing in the out buffer
	 */
	(*(u_short *)(cu->cu_outbuf))++;

	if ((! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp)))
		return (cu->cu_error.re_status = RPC_CANTENCODEARGS);

	outlen = (int)XDR_GETPOS(xdrs);

send_again:
	if (sendto(cu->cu_sock, cu->cu_outbuf, outlen, 0,
	    (struct sockaddr *)&(cu->cu_raddr), cu->cu_rlen)
	    != outlen) {
#ifdef WIN32
		cu->cu_error.re_errno = WSAerrno;
#else
		cu->cu_error.re_errno = errno;
#endif
		return (cu->cu_error.re_status = RPC_CANTSEND);
	}

	/*
	 * Hack to provide rpc-based message passing
	 */
	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		return (cu->cu_error.re_status = RPC_TIMEDOUT);
	}
	/*
	 * sub-optimal code appears here because we have
	 * some clock time to spare while the packets are in flight.
	 * (We assume that this is actually only executed once.)
	 */
	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xresults;
#ifdef FD_SETSIZE
	FD_ZERO(&mask);
	FD_SET(cu->cu_sock, &mask);
#else
	mask = 1 << cu->cu_sock;
#endif /* def FD_SETSIZE */
	for (;;) {
		readfds = mask;
#ifdef WIN32
		switch (select(0 /* unused in winsock */, &readfds, NULL,
#else
		switch (select(_rpc_dtablesize(), &readfds, NULL,
#endif
			       NULL, &(cu->cu_wait))) {

		case 0:
			time_waited.tv_sec += cu->cu_wait.tv_sec;
			time_waited.tv_usec += cu->cu_wait.tv_usec;
			while (time_waited.tv_usec >= 1000000) {
				time_waited.tv_sec++;
				time_waited.tv_usec -= 1000000;
			}
			if ((time_waited.tv_sec < timeout.tv_sec) ||
				((time_waited.tv_sec == timeout.tv_sec) &&
				(time_waited.tv_usec < timeout.tv_usec)))
				goto send_again;
			return (cu->cu_error.re_status = RPC_TIMEDOUT);

		/*
		 * buggy in other cases because time_waited is not being
		 * updated.
		 */
		case -1:
#ifdef WIN32
			if (WSAerrno == WSAEINTR)
				continue;
			cu->cu_error.re_errno = WSAerrno;
#else
			if (errno == EINTR)
				continue;	
			cu->cu_error.re_errno = errno;
#endif
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}
		do {
			fromlen = sizeof(struct sockaddr);
			inlen = recvfrom(cu->cu_sock, cu->cu_inbuf,
				(int) cu->cu_recvsz, 0,
				(struct sockaddr *)&from, &fromlen);
#ifdef WIN32
		} while (inlen < 0 && WSAerrno == EINTR);
		if (inlen < 0) {
			if (WSAerrno == WSAEWOULDBLOCK)
				continue;
			cu->cu_error.re_errno = WSAerrno;
#else
		} while (inlen < 0 && errno == EINTR);
		if (inlen < 0) {
			if (errno == EWOULDBLOCK)
				continue;	
			cu->cu_error.re_errno = errno;
#endif
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}
		if (inlen < sizeof(u_long))
			continue;
		/* see if reply transaction id matches sent id */
		if (*((u_long *)(cu->cu_inbuf)) != *((u_long *)(cu->cu_outbuf)))
			continue;
		/* we now assume we have the proper reply */
		break;
	}

	/*
	 * now decode and validate the response
	 */
	xdrmem_create(&reply_xdrs, cu->cu_inbuf, (u_int)inlen, XDR_DECODE);
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	/* XDR_DESTROY(&reply_xdrs);  save a few cycles on noop destroy */
	if (ok) {
		_seterr_reply(&reply_msg, &(cu->cu_error));
		if (cu->cu_error.re_status == RPC_SUCCESS) {
			if (! AUTH_VALIDATE(cl->cl_auth,
				&reply_msg.acpted_rply.ar_verf)) {
				cu->cu_error.re_status = RPC_AUTHERROR;
				cu->cu_error.re_why = AUTH_INVALIDRESP;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				xdrs->x_op = XDR_FREE;
				(void)xdr_opaque_auth(xdrs,
				    &(reply_msg.acpted_rply.ar_verf));
			}
		}  /* end successful completion */
		else {
			/* maybe our credentials need to be refreshed ... */
			if (nrefreshes > 0 && AUTH_REFRESH(cl->cl_auth)) {
				nrefreshes--;
				goto call_again;
			}
		}  /* end of unsuccessful completion */
	}  /* end of valid reply message */
	else {
		cu->cu_error.re_status = RPC_CANTDECODERES;
	}
	return (cu->cu_error.re_status);
}

static void
clntudp_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	*errp = cu->cu_error;
}


static bool_t
clntudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs = &(cu->cu_outxdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

static void
clntudp_abort(/*h*/)
	/*CLIENT *h;*/
{
}

static bool_t
clntudp_control(cl, request, info)
	CLIENT *cl;
	int request;
	char *info;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	switch (request) {
	case CLSET_TIMEOUT:
		cu->cu_total = *(struct timeval *)info;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *)info = cu->cu_total;
		break;
	case CLSET_RETRY_TIMEOUT:
		cu->cu_wait = *(struct timeval *)info;
		break;
	case CLGET_RETRY_TIMEOUT:
		*(struct timeval *)info = cu->cu_wait;
		break;
	case CLGET_SERVER_ADDR:
		*(struct sockaddr_in *)info = cu->cu_raddr;
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}

static void
clntudp_destroy(cl)
	CLIENT *cl;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	if (cu->cu_closeit) {
#ifdef WIN32
		(void)closesocket(cu->cu_sock);
#else
		(void)close(cu->cu_sock);
#endif
	}
	XDR_DESTROY(&(cu->cu_outxdrs));
	mem_free((caddr_t)cu, (sizeof(*cu) + cu->cu_sendsz + cu->cu_recvsz));
	mem_free((caddr_t)cl, sizeof(CLIENT));
}
