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

#ifndef __BINDRESV_HEADER__
#define __BINDRESV_HEADER__

#include <winsock2.h>

int bindresvport(int sd, struct sockaddr_in *sin);

#endif /* ndef __BINDRESV_HEADER__ */
