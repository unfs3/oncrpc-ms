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

/* @(#)xdr_array.c	2.1 88/07/29 4.0 RPCSRC */
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
static char sccsid[] = "@(#)xdr_array.c 1.10 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * xdr_array.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * arrays.  See xdr.h for more info on the interface to xdr.
 */

#include <stdio.h>

#include <rpc/rpc.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#define LASTUNSIGNED	((u_int)0-1)


/*
 * XDR an array of arbitrary elements
 * *addrp is a pointer to the array, *sizep is the number of elements.
 * If addrp is NULL (*sizep * elsize) bytes are allocated.
 * elsize is the size (in bytes) of each element, and elproc is the
 * xdr procedure to call to handle each element of the array.
 */
bool_t
xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
	register XDR *xdrs;
	caddr_t *addrp;		/* array pointer */
	u_int *sizep;		/* number of elements */
	u_int maxsize;		/* max numberof elements */
	u_int elsize;		/* size in bytes of each element */
	xdrproc_t elproc;	/* xdr routine to handle each element */
{
	register u_int i;
	register caddr_t target = *addrp;
	register u_int c;  /* the actual element count */
	register bool_t stat = TRUE;
	register u_int nodesize;

	/* like strings, arrays are really counted arrays */
	if (! xdr_u_int(xdrs, sizep)) {
		return (FALSE);
	}
	c = *sizep;
	if ((c > maxsize) && (xdrs->x_op != XDR_FREE)) {
		return (FALSE);
	}
	nodesize = c * elsize;

	/*
	 * if we are deserializing, we may need to allocate an array.
	 * We also save time by checking for a null array if we are freeing.
	 */
	if (target == NULL)
		switch (xdrs->x_op) {
		case XDR_DECODE:
			if (c == 0)
				return (TRUE);
			*addrp = target = mem_alloc(nodesize);
			if (target == NULL) {
#ifdef WIN32
				nt_rpc_report(
#else
				(void) fprintf(stderr, 
#endif
					"xdr_array: out of memory\n");
				return (FALSE);
			}
			bzero(target, nodesize);
			break;

		case XDR_FREE:
			return (TRUE);
	}

	/*
	 * now we xdr each element of array
	 */
	for (i = 0; (i < c) && stat; i++) {
		stat = (*elproc)(xdrs, target, LASTUNSIGNED);
		target += elsize;
	}

	/*
	 * the array may need freeing
	 */
	if (xdrs->x_op == XDR_FREE) {
		mem_free(*addrp, nodesize);
		*addrp = NULL;
	}
	return (stat);
}

/*
 * xdr_vector():
 *
 * XDR a fixed length array. Unlike variable-length arrays,
 * the storage of fixed length arrays is static and unfreeable.
 * > basep: base of the array
 * > size: size of the array
 * > elemsize: size of each element
 * > xdr_elem: routine to XDR each element
 */
bool_t
xdr_vector(xdrs, basep, nelem, elemsize, xdr_elem)
	register XDR *xdrs;
	register char *basep;
	register u_int nelem;
	register u_int elemsize;
	register xdrproc_t xdr_elem;
{
	register u_int i;
	register char *elptr;

	elptr = basep;
	for (i = 0; i < nelem; i++) {
		if (! (*xdr_elem)(xdrs, elptr, LASTUNSIGNED)) {
			return(FALSE);
		}
		elptr += elemsize;
	}
	return(TRUE);
}

