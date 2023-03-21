////////////////////
//  process.h   
///////////////////

#include <fdefine.h>
#ifndef process_h
#define process_h

void userkeyreq( int, char*, int) ;
void userauthreq( int, char*, int) ;
void kickout(int, char*, int) ;

void userconfirmreq( int, char*, int) ;
void duserconfirmreq( int, char*, int) ;
void systemparamreq(int, char*, int) ;

void ptzraw(int, char*, int) ;
void sdalarmopen(int, char*, int) ;


void systemcfgreq(int, char*, int) ;
void systimecfgreq(int, char*, int) ;
void netchange(void) ;
void cfgchange(void) ;
void reboot(int, char*, int) ;
void svrinfocfgreq(int, char*, int) ;
void slimitcfgreq(int, char*, int) ;

void svrportparamreq(int, char*, int) ;
void svrportcfgreq(int, char*, int) ;
void idnparamreq(int, char*, int) ;
void idncfgreq(int, char*, int) ;
void iframesettingparamreq(int, char*, int) ;
void iframesettingcfgreq(int, char*, int) ;

void strheaderreq( int, char*, int) ;
void vstreamstart( int, char*, int) ;
void astreamstart( int, char*, int) ;

void mstreamstart( int, char*, int) ;
void pstreamstart( int, char*, int) ;
int delay_func(int, int) ;

void videoparamreq( int, char*, int) ;
void videocfgreq( int, char*, int) ;
void audioparamreq( int, char*, int) ;
void audiocfgreq( int, char*, int) ;
void bitrateparamreq( int, char*, int) ;
void bitratecfgreq( int, char*, int) ;
void serverparamreq( int, char*, int) ;
void userparamreq( int, char*, int) ;
void usercfgreq( int, char*, int) ;

void resolutionparamreq( int, char*, int) ;
void resolutioncfgreq( int, char*, int) ;

void framerateparamreq(int, char*, int) ;
void frameratecfgreq( int, char*, int) ;
void macchange(int, char*, int) ;

void ptzctrlparamreq(int channel, char *data, int len);
void ptzctrlcfgreq(int channel, char *data, int len);
//void ptzinfoparamreq(int channel, char *data, int len);
//void ptzinfocfgreq(int channel, char *data, int len);

void playbackdatereq(int, char*, int) ;
void playbacktimereq(int, char*, int) ;
void multipbstartreq(int, char*, int) ;
void multipbstopreq(int, char*, int) ;
void multipbpausereq(int, char*, int) ;
void multipbplayfwdreq(int, char*, int) ;

#endif // process_h
 
