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

/* @(#)svc_run.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_run.c 1.1 87/10/13 Copyr 1984 Sun Micro";
#endif

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

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include <rpc/rpc.h>
#ifdef WIN32
#include <errno.h>
#else
#include <sys/errno.h>
#endif

void
svc_run()
{
#ifdef FD_SETSIZE
	fd_set readfds;
#else
      int readfds;
#endif /* def FD_SETSIZE */
#ifndef WIN32
	extern int errno;
#endif

	for (;;) {
#ifdef FD_SETSIZE
		readfds = svc_fdset;
#else
		readfds = svc_fds;
#endif /* def FD_SETSIZE */
#ifdef WIN32
		switch (select(0 /* unused in winsock */, &readfds, NULL, NULL,
#else
		switch (select(_rpc_dtablesize(), &readfds, NULL, NULL,
#endif
			       NULL)) {
		case -1:
#ifdef WIN32
			if (WSAerrno == EINTR) {
#else
			if (errno == EINTR) {
#endif
				continue;
			}
			perror("svc_run: - select failed");
			return;
		case 0:
			continue;
		default:
			svc_getreqset(&readfds);
		}
	}
}
