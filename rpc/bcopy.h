/*********************************************************************
 * RPC for the Windows NT Operating System
 * 1993 by Martin F. Gergeleit
 *
 * RPC for the Windows NT Operating System COMES WITH ABSOLUTELY NO 
 * WARRANTY, NOR WILL I BE LIABLE FOR ANY DAMAGES INCURRED FROM THE 
 * USE OF. USE ENTIRELY AT YOUR OWN RISK!!!
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void bcopy(void *,void *,int);
void bcopy_nf(void *,void *,int);
void bcopy_fn(void *,void *,int);
void bcopy_ff(void *,void *,int);
void bzero(void* ,int);
#ifdef __cplusplus
};
#endif

