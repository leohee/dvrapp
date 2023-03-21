////////////////////////////
//  networkcfg.h 
//////////////////////////

#include <fdefine.h>
#ifndef networkcfg_h 
#define networkcfg_h

int netaddr(int) ;
#if 0
int GetIPtype(), GetPort() ;
char* GetMacAddr(); 
char* GetIPaddr() ;
char* GetNetMask(); 
char* GetGateWay() ;
void SetIPtype(int type),SetMacInfo(char *macaddress), SetIPInfo(char* ipaddr, char* submask, char* gateway),
SetPort(int port) ;
#endif
#endif //  networkcfg_h
