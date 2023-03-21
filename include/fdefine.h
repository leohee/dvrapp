//
// fdefine.h
//
// 	각종 수치 정의	

#ifndef _FDEFINE_H_
#define _FDEFINE_H_

//#define TRUE            1
//#define FALSE           0 
#define ERRNO           -1
#define OFF             0
#define ON              1

#define IFRAME          0
#define PFRAME          1

#define CMD_START       1
#define CMD_STOP        2

#define VIDEO           0
#define AUDIO           1

#define SOCKWAIT        500

#define BOOOOM_REBOOT                              _IO('P',2)

#define MAXCONNECT      5
#define IRCOUNT         100 
#define MAXCFG_LENGTH   1024 
#define TITLESIZE       32

#define VERSIONMAJOR    2
#define VERSIONMINOR    0
#define VERSIONTEXT     'A'

#define LSOCKFLAG	0
#define CSOCKFLAG	1
#define DSOCKFLAG	2
#define Iframetype      1     // penta. intime = 0
#define Pframetype      2     // penta. intime = 1

#define IDENTIFIER      0x2818
#define SESSIONLIMIT    20
#define REUSEOK         1
#define HALFBUFFSIZE    512
#define NORMALBUFFSIZE  1024
#define SENDBUFFSIZE    6000
#define MAXBUFFSIZE     65536
#define PACKETSIZE      4096
#define UPDATEBUFFSIZE  4096


#define MAXIMTELIST 24*60*8  // 8 == MAXCHANNEL 
#define RESULT_SIZE     8

#define VSTREAMDATA_SIZE 14
#define PVSTREAMDATA_SIZE 16
#define ASTREAMDATA_SIZE 14

#define DELAY           10000  // micro
#define BAUDRATE B9600

#define CBR             1
#define VBR             2

#define MAXBUFF         512  // MSG HANDLER
//#define MAXCOMMANDS     100000
#define MAXCOMMANDS    0x1000//10000
#define MAXUSER         10
#define MAXCHANNEL      16


#define ACCEPTED        0x0001
#define REJECTED        0xffff


#define FRAMERATE1     0x00
#define FRAMERATE2     0x01
#define FRAMERATE3     0x02

#define NTSC176144      0x0003
#define NTSC320240      0x0000
#define NTSC640480      0x0001
#define NTSC720480      0x0002
#define NTSC640240      0x0009
#define NTSC352240      0x000A
#define NTSC720240      0x000B

#define PAL176144       0x0013
#define PAL320288       0x0004
#define PAL352288       0x0005
#define PAL704576       0x0006
#define PAL720576       0x0007
#define PAL360288       0x000C
#define PAL720288       0x000D


#define NETWORKMODIFYCFG        0
#define IPSERVMODIFYNETWORKCFG  1
#define USERMODIFYCFG           2
#define RESOLUTIONMODIFYCFG     3
#define SYSINFOMODIFYCFG        5

#define RESOLUTION      1
#define BITRATE         2 
#define FRAMERATE       3
#define GROUPOFPICTURE  4
#define IFRAMEONLYTYPE  5

#define INTROREQUEST    0x0001
#define INTRORESULT     0x0002
#define STATICINSERT    0x0004



typedef int SOCKETTYPE ;
typedef void Sigfunc(int) ;

#define INVALID_SOCKET  -1

#endif // _F_DEFINE_
