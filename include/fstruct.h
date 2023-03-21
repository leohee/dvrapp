//
// fstruct.h
//
//       data structures and type definitions.
//
/////////////////////////////////
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fdefine.h>
#include <time.h>
//#include <app_manager.h>
//typedef int SOCKETTYPE ;
//typedef void Sigfunc(int) ;

#define MAX_DVR_CHANNELS 16
typedef struct TAG_Vreader {
    int    cmd ;
    int    vchannel ;
    time_t Rectime ;
    pthread_mutex_t mutex ;
} Vreader ;

extern Vreader vreader ;

typedef struct TAG_DECODESET {
    int    cmd ;
    int    channel ;
    time_t Rectime ;
} DECODESET ;

extern DECODESET Decodeset ;

typedef struct TAG_Writer {
    int writerflag ;
    pthread_mutex_t mutex ;
} Writer ;

typedef struct TAG_Intimeheader
{
    unsigned short Ch ;
    unsigned short Type ;
    unsigned long FrameSize ;
} Intimeheader ;


#pragma pack(1)
typedef struct TAG_SYSTEM_INFO
{
        int L_Channel[MAXUSER];
        int C_Channel[MAXUSER] ;
        int D_Channel[MAXUSER] ;
        int encodingtype[MAXUSER] ;
        int headerrecv[MAXUSER] ;
        int Firstpacket[MAXUSER] ;
        int videochannel[MAXUSER] ;
        int audiochannel[MAXUSER] ;
        int StreamEnable[MAXUSER] ;
        int LQposition[MAXUSER] ;
        int CQposition[MAXUSER] ;
        int DQposition[MAXUSER] ;
        char LQBUFF[MAXUSER][NORMALBUFFSIZE] ;
        char CQBUFF[MAXUSER][NORMALBUFFSIZE] ;
        char DQBUFF[MAXUSER][NORMALBUFFSIZE] ;
        char Nosignal[MAX_DVR_CHANNELS] ;       
        int resolution ;
        int framerate ;
        int bitrate ;
        int input ;
        int tvtype ;  // air or catv
        int tvorcompchannel ;
        int datachannel ;
        int serial ;
        int alternative ;
        unsigned long Privatekey[MAXUSER] ;
        unsigned long Publickey[MAXUSER] ;
        unsigned long Modnumber[MAXUSER] ;
        char ptzdata[1200] ;
        pthread_mutex_t mutex ;
} SYSTEM_INFO;
#pragma pack()


#pragma pack(1)
typedef struct TAG_CONNECTON
{
        unsigned short connect ;
} CONNECTON ;
#pragma pack()       

#pragma pack(1)
typedef struct TAG_USERKEYREQ
{
        unsigned short channel ;
        unsigned short selectno ;
} USERKEYREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_USERKEYRES
{
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned long publickey1 ;
        unsigned long publickey2 ;
} USERKEYRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_USERAUTHREQ
{
        unsigned short selectno ;
        unsigned short idlength ;
        unsigned short passwdlength ;
        char *id ;
        char *passwd ;
} USERAUTHREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_TUSERAUTHREQ
{
        unsigned short channel ;
        unsigned short selectno ;
        unsigned short idlength ;
        unsigned short passwdlength ;
        char *id ;
        char *passwd ;
} TUSERAUTHREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_USERAUTHRES
{
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short result ;
        unsigned short chaccess ;
        unsigned short authvalue ;
        unsigned long conaccess ;
} USERAUTHRES ;
#pragma pack()

typedef struct TAG_USERCONFIRMREQ
{
        unsigned short authvalue ;
//        unsigned short selectno ;
} USERCONFIRMREQ ;

#pragma pack(1)
typedef struct TAG_USERCONFIRMRES
{
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short result ;
} USERCONFIRMRES ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SYSTEMPARAMREQ
{
        unsigned short channel ;
} SYSTEMPARAMREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_TSYSTEMPARAMREQ
{
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
} TSYSTEMPARAMREQ ;
#pragma pack()
/*
#pragma pack(1)
typedef struct TAG_SYSTEMPARAMRES
{
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
} SYSTEMPARAMRES ;
#pragma pack()
*/

#pragma pack(1)
typedef struct TAG_DUSERCONFIRMREQ
{
        unsigned short cmd ;
} DUSERCONFIRMREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_PACKETHEADER
{
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
} PACKETHEADER ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_STRHEADERREQ {
        unsigned short channel ;
} STRHEADERREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_HEADER
{
        unsigned short channel ;
} HEADER ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_THEADER
{
        unsigned short channel ;
        unsigned short headchannel ;
} THEADER ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_STRHEADERRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short channel ;
	unsigned char  Encodingtype ;
        unsigned char  Picturesize ;
        unsigned char  Framerate ;
        unsigned char  Reserved ;
        unsigned short Gmt ;
        unsigned short Year ;
        unsigned short Month ;
        unsigned short Day ;
        unsigned short Hour ;
        unsigned short Minute ;
        unsigned short Second ;
        unsigned short DayofWeek ;
        unsigned short Bias ;
//        char           Tvtitle[32] ;
        char           Cameratitle[32] ;
        unsigned short AudioEncodingtype ;
        unsigned short Inputchannel ; 
        unsigned short SamplingFrequency ;
        unsigned short OutputBitrate ;
} STRHEADERRES ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_VSTREAMSTART {
        unsigned short streamchannel ;
        unsigned short priority ;
} VSTREAMSTART ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_TVSTREAMSTART {
        unsigned short channel ;
        unsigned short streamchannel ;
        unsigned short priority ;
} TVSTREAMSTART ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_TIMESTRUCT {
	unsigned short Hour ;
	unsigned short Minute ;
	unsigned short Second ;
} TIMESTRUCT ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_VSTREAMDATA {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned char  FragmentNo ;
        unsigned char  FragmentIdx ;
        unsigned long  Timestamp ;
        unsigned short channel ;
} VSTREAMDATA ;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_ASTREAMSTART {
        unsigned short streamchannel ;
        unsigned short priority ;
} ASTREAMSTART ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_TASTREAMSTART {
        unsigned short channel ;
        unsigned short streamchannel ;
        unsigned short priority ;
} TASTREAMSTART ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_ASTREAMDATA {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned char  FragmentNo ;
        unsigned char  FragmentIdx ;
        unsigned long  Timestamp ;
        unsigned short channel ;
} ASTREAMDATA ;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_PVSTREAMDATA {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned char  FragmentNo ;
        unsigned char  FragmentIdx ;
        unsigned long  Timestamp ;
        unsigned short channel ;
        unsigned short resolution ;
} PVSTREAMDATA ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_FRAMECHANGE {
        unsigned short type ;
        unsigned long FrameSize ;
} FRAMECHANGE ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_SYSTEMPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short boardtype ;
        unsigned long  ipaddress ;
        unsigned long  subnetmask ;
        unsigned long  gateway ;
        unsigned short year ;
        unsigned short month ;
        unsigned short day ;
        unsigned short hour ;
        unsigned short minute ;
        unsigned short second ;
        unsigned long serialnumber ;
        unsigned short firmwareversion1 ;
        unsigned char firmwareversion2 ;
        unsigned char firmwareversion3 ;
        char          boooomboxname[32] ;
        char          composite1[32] ;
        char          composite2[32] ;
        char          svideo[32] ;
} SYSTEMPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_SYSINFOCFG {
        unsigned long serial ;
        char          boooomboxname[32] ;
        char          composite1[32] ;
        char          composite2[32] ;
        char          svideo[32] ;
} SYSINFOCFG ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SYSTEMCFGREQ {
        unsigned long serialnumber ;
        char          boooomboxname[32] ;
        char          composite1[32] ;
        char          composite2[32] ;
        char          svideo[32] ;
} SYSTEMCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SYSTIMECFGREQ {
        unsigned short year ;
        unsigned short month ;
        unsigned short day ;
        unsigned short hour ;
        unsigned short minute ;
        unsigned short second ;
} SYSTIMECFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_USERPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
} USERPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_USERCFGREQ {
        char UserId[12] ;
        char UserPasswd[12] ;
        unsigned short channelaccess ;
        unsigned long controlaccess ;
} USERCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_USERINFOCFG {
        char Id[12] ;
        char Passwd[12] ;
        unsigned short channelaccess ;
        unsigned long controlaccess ;
} USERINFOCFG ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_VIDEOPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        char           channelgroup ;
        char           encodingtype ;
        char 	       encodingstandard ;
        char           picturesize ;
        char           framerate ;
        char           pictureencodingtype ;
        char           ipictureinterval ;
        char           channelenable ;
        char           currentchannel ;
        char           reserved ;
} VIDEOPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_VIDEOCFGREQ {
        char           channelgroup ;
        char           encodingtype ;
        char           encodingstandard ;
        char           picturesize ;
        char           framerate ;
        char           pictureencodingtype ;
        char           ipictureinterval ;
        char           channelenable ;
        char           currentchannel ;
        char           reserved ;
} VIDEOCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_AUDIOPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        char           channelgroup ;
        char           samplingfrequency ;
        char           inputchannel ;
        char           outputbitrate ;
        char           encodingtype ;
        char           channelenable ;
} AUDIOPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_AUDIOCFGREQ {
        char           channelgroup ;
        char           samplingfrequency ;
        char           inputchannel ;
        char           outputbitrate ;
        char           encodingtype ;
        char           channelenable ;
} AUDIOCFGREQ ;
#pragma pack()



#pragma pack(1)
typedef struct TAG_BITRATEPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short videoMode ;
        char           initialquant ;
        char           bitratecontrol ;
        unsigned short bitrate[MAX_DVR_CHANNELS] ;
} BITRATEPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_BITRATECFGREQ {
        char		videoMode ;
        char           	initialquant ;
        char           	bitratecontrol ;
        char 		bitrate[MAX_DVR_CHANNELS] ;
} BITRATECFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SERVERPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short tcplogonport ; 
        unsigned short tcpcontrolport ;
        unsigned short tcpdataport ;
        unsigned short udplogonport ; 
        unsigned short udpcontrolport ;
        unsigned short udpdataport ;
        unsigned short tcpsessionlimit ;
        unsigned short udpsessionlimit ;
        char           servertitle[32] ;
        char           serverlocation[32] ;
        char           serverdescription[32] ;
} SERVERPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_SVRINFOCFGREQ {
        char           servertitle[32] ;
        char           serverlocation[32] ;
        char           serverdescription[32] ;
} SVRINFOCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SLIMITCFGREQ {
        unsigned short tcpsessionlimit ;
        unsigned short udpsessionlimit ;
} SLIMITCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SVRPORTPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short validbit ;
        unsigned short tcplogonport ;
        unsigned short tcpcontrolport ;
        unsigned short tcpdataport ;
        unsigned short udplogonport ;
        unsigned short udpcontrolport ;
        unsigned short udpdataport ;
} SVRPORTPARAMRES ;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_SVRPORTCFGREQ {
        unsigned short validbit ;
        unsigned short tcplogonport ; 
        unsigned short tcpcontrolport ;
        unsigned short tcpdataport ;
        unsigned short udplogonport ; 
        unsigned short udpcontrolport ;
        unsigned short udpdataport ;
} SVRPORTCFGREQ ;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_NETWORKPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ; 
        unsigned short length ; 
        unsigned short bootoption ;
        unsigned long ipaddress ;
        unsigned long subnetmask ;
        unsigned long gateway ;
        unsigned long primarydns ;
        unsigned long secondarydns ;
} NETWORKPARAMRES ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_NETWORKCFGREQ {
        unsigned short bootoption ;
        unsigned long ipaddress ;
        unsigned long subnetmask ;
        unsigned long gateway ;
        unsigned long primarydns ;
        unsigned long secondarydns ;
} NETWORKCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_NETCHANGE {
        unsigned short identifier ;
        unsigned short cmd ; 
        unsigned short length ; 
        unsigned long ipaddress ;
        unsigned long subnetmask ;
        unsigned long gateway ;
} NETCHANGE ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_CFGCHANGE {
        unsigned short identifier ;
        unsigned short cmd ; 
        unsigned short length ; 
} CFGCHANGE ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_REBOOTRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
} REBOOTRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_RESOLUTIONPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short videoMode ;
        unsigned short ssize ;
} RESOLUTIONPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_RESOLUTIONCFGREQ {
        unsigned short videoMode ;
        unsigned short Ssize ;
} RESOLUTIONCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_IDNPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned char  status ;
        unsigned char  protocol ;
        unsigned short interval ;
        unsigned short remoteport ;
        unsigned long ipaddress ;
} IDNPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_IDNCFGREQ {
        unsigned char  status ;
        unsigned char  protocol ;
        unsigned short interval ;
        unsigned short remoteport ;
        unsigned long ipaddress ;
} IDNCFGREQ ;
#pragma pack()
 
#pragma pack(1)
typedef struct TAG_CHANGEBITRATE {
        unsigned long chbit ;
        unsigned short type ;
        unsigned short curtype ;
        pthread_mutex_t mutex ;
} CHANGEBITRATE ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_TIMECHECK {
        unsigned short year ;
        unsigned short month ;
        unsigned short day;
        unsigned short hour ;
        unsigned short minute;
        unsigned short second ;
} TIMECHECK ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_IPCONFIRM {
        unsigned short identifier ;
        unsigned short cmd ;
        char macaddress[6] ;
        unsigned short valid ;
        struct in_addr ipaddress ;
        struct in_addr subnetmask ;
        struct in_addr gateway ;
        unsigned long reserved1 ;
        unsigned short reserved2 ;
        unsigned short reserved3 ;
        unsigned short Tlogonport ;
        unsigned short Tcontrolport ;
        unsigned short Tdataport ;
        unsigned short Ulogonport ;
        unsigned short Ucontrolport ;
        unsigned short Udataport ;
        char servertitle[32] ;
        char serverlocation[32] ;
        char serverdescription[32] ;
} IPCONFIRM ;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_INTROPACKET {
        unsigned short identifier ;
        unsigned short cmd ;
        char macaddress[6] ;
        unsigned short reserved1 ;
        struct in_addr ipaddress ;
        struct in_addr subnetmask ;
        struct in_addr gateway ;
        unsigned short versionmajor ;
        unsigned char versionminor ;
        unsigned char versiontext ;
        unsigned short boardtype ;
        unsigned short reserved2 ;
        unsigned short Tlogonport ;
        unsigned short Tcontrolport ;
        unsigned short Tdataport ;
        unsigned short Ulogonport ;
        unsigned short Ucontrolport ;
        unsigned short Udataport ;
        char servertitle[32] ;
        char serverlocation[32] ;
        char serverdescription[32] ;
        char cameratitle1[32] ;
        char cameratitle2[32] ;
        char cameratitle3[32] ;
        char cameratitle4[32] ;
} INTROPACKET;
#pragma pack() 

#pragma pack(1)
typedef struct TAG_NETWORKCFG {
        unsigned short dhcp ;
        char macaddress[20] ;
        struct in_addr ipaddress ;
        struct in_addr subnetmask ;
        struct in_addr gateway ;
        struct in_addr broadcast ;
        unsigned short port ;
} NETWORKCFG ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_IPSERVNETWORKCFG {
        unsigned char status ;
        unsigned char protocoltype ;
        unsigned short interval ;
        unsigned short port ;
        struct in_addr ipaddress ;
} IPSERVNETWORKCFG ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_RESOLUTIONCFG {
        unsigned long bitrate ;
        unsigned short resolution ;
        unsigned short framerate ;
        unsigned short IframeInterval ;
        unsigned short IframeOnly ;
} RESOLUTIONCFG ;
#pragma pack()

typedef struct TAG_CFG {
        struct TAG_CFG     *next ;
        char               *key ;
        char               *strval ;
        int                intval ;
        int                type ;
} KEY_LIST ;


#pragma pack(1)
typedef struct TAG_VSTREAMERROR {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short channel ;
        unsigned short errcode ;
} VSTREAMERROR ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SERVERBUSY {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
//        char reserved[10] ;
} SERVERBUSY ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_FLOWCONTROL {
        unsigned long bitrate ;
        unsigned short resolution ;
        unsigned short framerate ;
        unsigned short iframeinterval ;
        unsigned short iframeonly ;
} FLOWCONTROL ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_FRAMERATEPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short videoMode ;
        unsigned short framerate[MAX_DVR_CHANNELS] ;
} FRAMERATEPARAMRES ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_FRAMERATECFGREQ {
        char 		videoMode ;
        char 		framerate[MAX_DVR_CHANNELS] ;
} FRAMERATECFGREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_CHANGEFRAMERATE {
        int framerate ;
} CHANGEFRAMERATE ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_NETSETTINGCHANGE {
        unsigned short setting ;
} NETSETTINGCHANGE ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_IFRAMESETTINGPARAMRES {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short videoMode ;
        unsigned short gop[MAX_DVR_CHANNELS] ;
} IFRAMESETTINGPARAMRES ;
#pragma pack(1)


#pragma pack(1)
typedef struct TAG_IFRAMESETTINGCFGREQ {
        char videoMode ;
        char gop[MAX_DVR_CHANNELS] ;
} IFRAMESETTINGCFGREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SYSTEMREBOOT {
        unsigned short reboot ;
} SYSTEMREBOOT ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_MACCHANGE {
	 char dev_no;
        char macaddress[17] ;
} MACCHANGE ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PLAYBACKDATEREQ {
        unsigned short pYear ;
        unsigned short pMonth ;
} PLAYBACKDATEREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PLAYBACKDATERES {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        char DAY[31] ;
} PLAYBACKDATERES ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_PLAYBACKTIMEREQ {
	unsigned short listType ;
        unsigned short startYear ;
        unsigned short startMonth ;
        unsigned short startDay ;
} PLAYBACKTIMEREQ ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PLAYBACKTIMERES {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
	unsigned short channelMask ;
        unsigned short result ;
//        char Min[11520] ;
        char Min[1440] ;
} PLAYBACKTIMERES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_MULTIPBSTARTREQ {
        unsigned short ContinuousFlag ;
        unsigned short InitialMode ;
        unsigned short InitialPausePosition ;
        unsigned short InitialChannelMask ;
        unsigned short DataType ;
        unsigned short PlayRangeType ;
        unsigned short RangeStartYear ;
        unsigned short RangeStartMonth ;
        unsigned short RangeStartDay ;
        unsigned short RangeStartHour ;
        unsigned short RangeStartMin ;
        unsigned short RangeStartSec ;
        unsigned short RangeEndYear ;
        unsigned short RangeEndMonth ;
        unsigned short RangeEndDay ;
        unsigned short RangeEndHour ;
        unsigned short RangeEndMin ;
        unsigned short RangeEndSec ;
} MULTIPBSTARTREQ ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_SIGLEPBDATA {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short legnth ;
        char FragNo ;
        char FragIdx ;
        unsigned long TimeStamp ;
        unsigned short channel ; 
} SIGLEPBVIDEODATA ;
#pragma pack()


#pragma pack(1)
typedef struct TAG_COMRESPACKET {
        unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
        unsigned short result ;
} COMRESPACKET ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PTZCTRLPARAM_RES {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
	  char protocol ;
	  char bandrate;
        char channel;
        char ptzcmd;
} PTZCTRLPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PTZCTRLCFG_REQ {
	 char protocol ;
	 char bandrate;
        char channel;
        char ptzcmd;
} PTZCTRLCFGREQ ;
#pragma pack()

/*#pragma pack(1)
typedef struct TAG_PTZINFOPARAM_RES {
	unsigned short identifier ;
        unsigned short cmd ;
        unsigned short length ;
	 char protocol ;
        char targetaddr;
        char bandrate;
	  char databit;
	  char stopbit;
	  char paritybit;
} PTZINFOPARAMRES ;
#pragma pack()

#pragma pack(1)
typedef struct TAG_PTZINFOCFG_REQ {
	 char protocol ;
        char targetaddr;
        char baudrate;
	  char databit;
	  char stopbit;
	  char paritybit;
} PTZINFOCFGREQ ;
#pragma pack()
*/