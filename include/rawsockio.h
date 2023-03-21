/**************
* rawsockio.h 
*************/

#ifndef rawsockio_h
#define rawsockio_h

char *ToHex( char *pBuffer, int nBufLen, int nSeqNo);

int MainSocketListen(int), InitVar(void) ; 
void ProcessSocket(int, int) ;
void ChangeNonblock(int) ;
void CloseNowChannel(int, int) ;
void CloseAllChannel(void) ;
int IpCamRead(void);
void broadreceive(void) ;
void ipserv(void) ;
void updateserv(void) ;
void invalidserv(void) ;
int ReadSocketData(int, int), ReadSocketQue(int, int, int), GetQue(int, int) ;
int GetGlobalMacAddr(char *) ;

#define NETWORK_DEBUG

#ifdef NETWORK_DEBUG
#define DEBUG_PRI(msg, args...) printf("[NETWORK] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define DEBUG_PRI(msg, args...) ((void)0)
#endif

#endif // rawsockio_h
