// Process.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <linux/fb.h>

#include <fstruct.h>
#include <fcommand.h>
#include <fdefine.h>
#include <process.h>
#include <msghandler.h>
#include <rawsockio.h> 
#include <encrypt.h>
//#include <systemsignal.h>
#include "networkcfg.h"
#include <osa.h>
//#include <libdvr365.h>
#include <define.h>
//#include <capture.h>
//#include <setparam.h>
//#include <basket.h>
//#include <writer.h>
#include <basket_remoteplayback.h>
#include "lib_dvr_app.h"
#include "remoteplay.h"
#include "app_ptz_if.h"

NETSETTINGCHANGE Netsettingchange ;
SYSTEM_INFO SystemInfo ;
SYSTEMREBOOT Systemreboot ;
SYSTEMPARAMRES Systemparamres ;
USERINFOCFG Userinfocfg[MAXCONNECT] ;
NETWORKCFG Networkcfg ;
SYSINFOCFG Sysinfocfg ;
IPSERVNETWORKCFG Ipservnetworkcfg ;
//extern SETPARAM_Info gSETPARAM_info;

VSTREAMERROR Vstreamerror ;

char m_SendBuffer[MAXUSER][HALFBUFFSIZE] ;


#define USERKEYRES_SIZE 14
const int USERAUTHRES_SIZE = sizeof(USERAUTHRES) ;
const int USERPARAMRES_SIZE = sizeof(USERPARAMRES) ;
const int SERVERPARAMRES_SIZE = sizeof(SERVERPARAMRES) ;
const int BITRATEPARAMRES_SIZE = sizeof(BITRATEPARAMRES) ;
const int VIDEOPARAMRES_SIZE = sizeof(VIDEOPARAMRES) ;
const int AUDIOPARAMRES_SIZE = sizeof(AUDIOPARAMRES) ;
const int CFGCHANGE_SIZE = sizeof(CFGCHANGE) ;
const int REBOOTRES_SIZE = sizeof(REBOOTRES) ;
const int IDNPARAMRES_SIZE = sizeof(IDNPARAMRES) ;
const int RESOLUTIONPARAMRES_SIZE = sizeof(RESOLUTIONPARAMRES) ; 
const int STRHEADERRES_SIZE = sizeof(STRHEADERRES) ;



////////////////////////////////////////////
//  delay_func     
//////////////////////////////////////////////

int delay_func(int sec, int usec)
{

    struct  timeval  wait;
    fd_set  readfds;
    int     rc;


    FD_ZERO(&readfds);
    wait.tv_sec = sec;
    wait.tv_usec = usec;
    rc = 0;
    rc = select(1 , (fd_set *)&readfds, (fd_set *)0, (fd_set *)0, (struct timeval *)&wait);
    return rc ;
}


void userkeyreq(int channel, char* data, int len)
{
    USERKEYREQ  *Userkeyreq ;
    USERKEYRES Userkeyres ;

    int sendlen = 0 ;
    unsigned long privateKey, publicKey, modNumber;

#ifdef NETWORK_DEBUG 
    DEBUG_PRI("userkeyreq packet receive length = %d\n",len) ;
#endif
    
    Userkeyreq = (USERKEYREQ *)data ;

    if (createKeys(&privateKey, &publicKey, &modNumber) == ERRNO)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("createkey function fail\n") ;
#endif
    }
    else
    {
        SystemInfo.Privatekey[channel] = (unsigned long)privateKey ;
        SystemInfo.Publickey[channel] = (unsigned long)publicKey ;
        SystemInfo.Modnumber[channel] = (unsigned long)modNumber ;
    }

    Userkeyres.identifier = htons(IDENTIFIER) ;
    Userkeyres.cmd = htons(CMD_USERKEY_RES) ;
    Userkeyres.length = htons(USERKEYRES_SIZE) ;
    Userkeyres.publickey1 = htonl(publicKey) ;
    Userkeyres.publickey2 = htonl(modNumber) ;

    memcpy(m_SendBuffer[channel], &Userkeyres, USERKEYRES_SIZE) ;

    sendlen = send(SystemInfo.L_Channel[channel], m_SendBuffer[channel], USERKEYRES_SIZE, 0) ;
    memset(m_SendBuffer[channel], HALFBUFFSIZE, 0) ;
#ifdef NETWORK_DEBUG 
    DEBUG_PRI("userkeyres packet sendlen = %d\n",sendlen) ;
#endif 
}


void userauthreq(int channel, char *data, int len) 
{
    //SERVERBUSY Serverbusy ;
    USERAUTHREQ *Userauthreq ;
    USERAUTHRES Userauthres ;

    int sendlen = 0;//, selectno = 0, idlength = 0, passwdlength = 0 ;
   // unsigned long outputidlength = 0, outputpasswdlength = 0 ;
    unsigned char id[100],  passwd[100] ;
    unsigned char outputid[100], outputpasswd[100] ;

    memset(id, 0, 100) ;
    memset(passwd, 0, 100) ;
    memset(outputid, 0, 100) ;
    memset(outputpasswd, 0, 100) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("userauthreq packet receive length = %d\n",len) ;
#endif
    Userauthreq = (USERAUTHREQ *)data ;
printf("selectno=0x%x,idlength=0x%x\n",Userauthreq->selectno,Userauthreq->idlength);
/*
    selectno = ntohs(Userauthreq->selectno) ;
    idlength = ntohs(Userauthreq->idlength) ;
    passwdlength = ntohs(Userauthreq->passwdlength) ;

    memcpy(id, &Userauthreq->id, idlength) ;
    memcpy(passwd, &data[6+idlength], passwdlength) ;
    
    outputidlength = ss_decrypt(SystemInfo.Privatekey[channel], SystemInfo.Modnumber[channel], (unsigned char*)id, 
    (unsigned long)idlength, outputid, (unsigned long)100) ;

    outputpasswdlength = ss_decrypt(SystemInfo.Privatekey[channel], SystemInfo.Modnumber[channel], (unsigned char*)passwd,
    (unsigned long)passwdlength, outputpasswd, (unsigned long)100) ;
    

    cmpvalue = strncmp(outputid, "admin", 5) ;
    if(cmpvalue != 0)
    {
        cmpvalue = strncmp(outputid, "NetUser",7) ;
        if(cmpvalue == 0)
        {
            idmatch = 1 ;
            passwdmatch = 1 ;

            Userauthres.identifier = htons(IDENTIFIER) ;
            Userauthres.cmd = htons(CMD_USERAUTH_RES) ;
            Userauthres.length = htons(USERAUTHRES_SIZE) ;
            Userauthres.result = htons(ACCEPTED) ;
            Userauthres.chaccess = Userinfocfg[i].channelaccess ; 
        }
        else
        {
            if(SystemInfo.headerrecv[2] == 2)
            {
                Serverbusy.identifier = htons(IDENTIFIER) ;
                Serverbusy.cmd = htons(CMD_SERVER_BUSY) ;
                Serverbusy.length = htons(6) ;
//              strncpy(Serverbusy.reserved, "aaaaaaaaaa", 10) ; 

                sendlen = send(SystemInfo.L_Channel[channel], &Serverbusy, 6, 0) ;
#ifdef NETWORK_DEBUG
                DEBUG_PRI("server busy..sendlen = %d\n",sendlen) ;
#endif
                CloseNowChannel(channel, LSOCKFLAG) ;
            }
            else
            {
                retvalue = netaddr(USERMODIFYCFG) ;
                for(i = 0; i < retvalue; i++)
                {
                    if(outputidlength != strlen(Userinfocfg[i].Id))
                    {
                        continue ;
                    }
                    else
                    {
                        cmpvalue = strncmp(outputid, Userinfocfg[i].Id, strlen(Userinfocfg[i].Id)) ;

                        if(cmpvalue == 0)
                        {
#ifdef NETWORK_DEBUG
                            DEBUG_PRI("find a same id\n") ;
#endif
                            idmatch = 1 ;

                            if(outputpasswdlength != strlen(Userinfocfg[i].Passwd))
                            {
                                continue ;
                            }
                            else
                            {
                                cmpvalue = strncmp(outputpasswd, Userinfocfg[i].Passwd, strlen(Userinfocfg[i].Passwd)) ;
                                if(cmpvalue == 0)
                                { 
#ifdef NETWORK_DEBUG
                                    DEBUG_PRI("find a same passwd %s\n",outputpasswd) ;
#endif
                                    passwdmatch = 1 ;
                                    Userauthres.identifier = htons(IDENTIFIER) ;
                                    Userauthres.cmd = htons(CMD_USERAUTH_RES) ;
                                    Userauthres.length = htons(USERAUTHRES_SIZE) ;
                                    Userauthres.result = htons(ACCEPTED) ;
                                    Userauthres.chaccess = Userinfocfg[i].channelaccess ; 
                                }
                                else
                                {
                                    passwdmatch = 0 ;
#ifdef NETWORK_DEBUG
                                    DEBUG_PRI("find a different passwd %s\n",outputpasswd) ;
#endif
                                    break ;
                                }
                            } 
                        }
                    }
                }
            }   
        }       
        if(idmatch != TRUE || passwdmatch != TRUE)
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("auth..fail...\n") ;
#endif
            Userauthres.result = htons(REJECTED) ;
            Userauthres.chaccess = htons(0xffff) ;
            Userauthres.authvalue = htons(1) ;
            Userauthres.conaccess = htonl(0x00ffffff) ;
        }

        memcpy(m_SendBuffer[channel], &Userauthres, USERAUTHRES_SIZE) ;
        sendlen = send(SystemInfo.L_Channel[channel], m_SendBuffer[channel], USERAUTHRES_SIZE, 0) ;
        memset(m_SendBuffer[channel], HALFBUFFSIZE, 0) ;
#ifdef NETWORK_DEBUG
        DEBUG_PRI("userauthres packet sendlen = %d\n",sendlen) ;
#endif
    }
    else
    {
        retvalue = netaddr(USERMODIFYCFG) ;
        for(i = 0; i < retvalue; i++)
        {
            if(outputidlength != strlen(Userinfocfg[i].Id))
            {
                continue ;
            }
            else
            {
                cmpvalue = strncmp(outputid, Userinfocfg[i].Id, strlen(Userinfocfg[i].Id)) ;
                if(cmpvalue == 0)
                {
#ifdef NETWORK_DEBUG
                    DEBUG_PRI("find a admin id\n") ;
#endif
                    idmatch = 1 ;

                    if(outputpasswdlength != strlen(Userinfocfg[i].Passwd))
                    {
                        continue ;
                    }
                    else
                    {
                        cmpvalue = strncmp(outputpasswd, Userinfocfg[i].Passwd, strlen(Userinfocfg[i].Passwd)) ;
                        if(cmpvalue == 0)
                        { 
                            passwdmatch = 1 ;
                            Userauthres.identifier = htons(IDENTIFIER) ;
                            Userauthres.cmd = htons(CMD_USERAUTH_RES) ;
                            Userauthres.length = htons(USERAUTHRES_SIZE) ;
                            Userauthres.result = htons(ACCEPTED) ;
                            Userauthres.chaccess = Userinfocfg[i].channelaccess ;
                        }
                        else
                        {
                            passwdmatch = 0;
                            break ;
                        }
                    } 
                }
            }
        }  
        if(SystemInfo.headerrecv[2] == 2)
        {
            Serverbusy.identifier = htons(IDENTIFIER) ;
            Serverbusy.cmd = htons(CMD_SERVER_BUSY) ;
            Serverbusy.length = htons(6) ; 
//            strncpy(Serverbusy.reserved, "aaaaaaaaaa", 10) ; 

            sendlen = send(SystemInfo.L_Channel[channel], &Serverbusy, 6, 0) ;
#ifdef NETWORK_DEBUG
            DEBUG_PRI("server busy..sendlen = %d\n",sendlen) ;
#endif
            CloseNowChannel(channel, LSOCKFLAG) ;
        }

        if(idmatch != TRUE || passwdmatch != TRUE)
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("admin auth..fail...\n") ;
#endif
            Userauthres.result = htons(REJECTED) ;
            Userauthres.chaccess = htons(0xffff) ;
            Userauthres.authvalue = htons(1) ;
        }
    }
*/
        Userauthres.identifier = htons(IDENTIFIER) ;
        Userauthres.cmd = htons(CMD_USERAUTH_RES) ;
        Userauthres.length = htons(USERAUTHRES_SIZE) ;
        Userauthres.result = htons(ACCEPTED) ;
        Userauthres.chaccess = 255 ;

        memcpy(m_SendBuffer[channel], &Userauthres, USERAUTHRES_SIZE) ;
        sendlen = send(SystemInfo.L_Channel[channel], m_SendBuffer[channel], USERAUTHRES_SIZE, 0) ;
#ifdef NETWORK_DEBUG
        DEBUG_PRI("userauthres packet sendlen = %d\n",sendlen) ;
#endif
        memset(m_SendBuffer[channel], HALFBUFFSIZE, 0) ;
}


void kickout(int channel, char*data, int len)
{
#ifdef NETWORK_DEBUG
    DEBUG_PRI("kick out packet receive...\n") ;
#endif
     
    CloseAllChannel() ;
}
void userconfirmreq(int channel, char*data, int len)
{
    int sendlen = 0 ;

    USERCONFIRMREQ  *Userconfirmreq ;
    COMRESPACKET Userconfirmres ;

    //gSETPARAM_info.remoteConnectOn = TRUE ; 
    Userconfirmreq = (USERCONFIRMREQ *)data ;

    Userconfirmres.identifier = htons(IDENTIFIER) ;
    Userconfirmres.cmd = htons(CMD_USERCONFIRM_RES) ;
    Userconfirmres.length = htons(8) ;
    Userconfirmres.result = htons(TRUE) ;

    memcpy(m_SendBuffer[channel], &Userconfirmres, 8) ;

    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], 8, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("contorl user confirmres.. packet sendlen = %d\n",sendlen) ;        
#endif

}

void ptzraw(int channel, char *data, int len)
{
#ifdef NETWORK_DEBUG
    DEBUG_PRI("ptzraw packet receive....\n") ;
#endif

//    pthread_mutex_lock (&SystemInfo.mutex);
    memcpy(SystemInfo.ptzdata, data, len) ;
    SystemInfo.serial = len ;
//    pthread_mutex_unlock (&SystemInfo.mutex);


}



void systemparamreq(int channel, char *data, int len)
{
#ifdef NETWORK_DEBUG
    DEBUG_PRI("systemparamreq packet receive\n") ;
#endif

    int sendlen = 0, retvalue = 0 ;
    time_t stime ;
    struct tm tm1;
    time(&stime) ;
    localtime_r(&stime, &tm1) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("control socket vssystemparamreq ................\n");
#endif

  //  retvalue = netaddr(NETWORKMODIFYCFG) ;
    //retvalue = netaddr(SYSINFOMODIFYCFG) ;

    Systemparamres.identifier = htons(IDENTIFIER) ;
    Systemparamres.cmd = htons(CMD_SYSTEMPARAM_RES) ;
    Systemparamres.length = htons(168) ;
    Systemparamres.boardtype = htons(0x0001) ;
    Systemparamres.ipaddress = Networkcfg.ipaddress.s_addr ;
    Systemparamres.subnetmask = Networkcfg.subnetmask.s_addr ;
    Systemparamres.gateway = Networkcfg.gateway.s_addr ;
    Systemparamres.year = htons(tm1.tm_year + 1900) ;
    Systemparamres.month = htons(tm1.tm_mon + 1) ;
    Systemparamres.day = htons(tm1.tm_mday) ;
    Systemparamres.hour = htons(tm1.tm_hour) ;
    Systemparamres.minute = htons(tm1.tm_min) ;
    Systemparamres.second = htons(tm1.tm_sec) ;

    Systemparamres.serialnumber = Sysinfocfg.serial ;
    Systemparamres.firmwareversion1 = htons(VERSIONMAJOR) ;
    Systemparamres.firmwareversion2 = VERSIONMINOR ;
    Systemparamres.firmwareversion3 = VERSIONTEXT ;
    strncpy(Systemparamres.boooomboxname, Sysinfocfg.boooomboxname, 32) ;
    strncpy(Systemparamres.composite1, Sysinfocfg.composite1, 32) ;
    strncpy(Systemparamres.composite2, Sysinfocfg.composite2, 32) ;
    strncpy(Systemparamres.svideo, Sysinfocfg.svideo, 32) ;

    memcpy(m_SendBuffer[channel], &Systemparamres, 168) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], 168, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("systemparamreq sendlen = %d\n",sendlen) ;
#endif



}


void systemcfgreq(int channel, char*data, int len)
{
	SYSTEMCFGREQ *Systemcfgreq ;
    	//SYSINFOCFG Sysinfocfg ;
   	 COMRESPACKET Systemcfgres ;
    	FILE *fp = NULL;
    	int sendlen = 0;//, retvalue = 0;
    	char bfound = 0;

#ifdef NETWORK_DEBUG
    	DEBUG_PRI("systemcfgreq packet receive! length = %d\n",len) ;
#endif

  //  retvalue = netaddr(SYSINFOMODIFYCFG) ;
  	Systemcfgreq = (SYSTEMCFGREQ *)data ;

   	 if((ntohl(Systemcfgreq->serialnumber) != 0) && (Systemcfgreq->serialnumber != Sysinfocfg.serial))
    	{
    		Sysinfocfg.serial = Systemcfgreq->serialnumber;
       	bfound = 1;
    	}
   
    	if((Systemcfgreq->boooomboxname != NULL) && (Systemcfgreq->boooomboxname != Sysinfocfg.boooomboxname))
    	{
		strcpy(Sysinfocfg.boooomboxname,Systemcfgreq->boooomboxname);
		bfound = 1;
    	}

  	if((Systemcfgreq->composite1 != NULL) && (Systemcfgreq->composite1 != Sysinfocfg.composite1))
   	{
   		strcpy(Sysinfocfg.composite1,Systemcfgreq->composite1);
		bfound = 1;
   	}
    	if((Systemcfgreq->composite2 != NULL) && (Systemcfgreq->composite2 != Sysinfocfg.composite2))
    	{
	   	strcpy(Sysinfocfg.composite2,Systemcfgreq->composite2);
		bfound = 1;
   	}	
    	if((Systemcfgreq->svideo != NULL) && (Systemcfgreq->svideo != Sysinfocfg.svideo))
   	{
	   	strcpy(Sysinfocfg.svideo,Systemcfgreq->svideo);
		bfound = 1;
   	}	
	
   	if(bfound)
  	{
    		if(( fp = fopen("/mnt/settings/sysinfo", "w")) == NULL)
    		{
#ifdef NETWORK_DEBUG
        		DEBUG_PRI("sysinfo file open error\n") ;
#endif
    		}
    		else
    		{
    			fprintf(fp,"SERIALNUMBER = %d\n",ntohl(Sysinfocfg.serial)) ;
       		fprintf(fp,"BOOOOMBOXNAME = \"%s\"\n",Sysinfocfg.boooomboxname) ;
        		fprintf(fp,"COMPOSITE1 = \"%s\"\n",Sysinfocfg.composite1) ;
        		fprintf(fp,"COMPOSITE2 = \"%s\"\n",Sysinfocfg.composite2) ;
        		fprintf(fp,"SVIDEO = \"%s\"\n",Sysinfocfg.svideo) ;
	  		fclose(fp) ;
    		}
   	}
     
    
    
    	Systemcfgres.identifier = htons(IDENTIFIER) ;
    	Systemcfgres.cmd = htons(CMD_SYSTEMCFG_RES) ;
    	Systemcfgres.length = htons(RESULT_SIZE) ;
    	Systemcfgres.result = htons(0x3001) ;

    	memcpy(m_SendBuffer[channel], &Systemcfgres, RESULT_SIZE) ;
    	sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    	DEBUG_PRI("systemcfgres packet sendlen = %d\n",sendlen) ;
#endif
}

void systimecfgreq(int channel, char *data, int len)
{
    SYSTIMECFGREQ *Systimecfgreq ;
    COMRESPACKET Systimecfgres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("systimecfgreq packet receive!\n") ;
#endif

    Systimecfgreq = (SYSTIMECFGREQ*)data ;

    Systimecfgres.identifier = htons(IDENTIFIER) ;
    Systimecfgres.cmd = htons(CMD_SYSTIMECFG_RES) ;
    Systimecfgres.length = htons(RESULT_SIZE) ;
    Systimecfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Systimecfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("systimecfgres packet sendlen = %d\n",sendlen) ;
#endif
}

void duserconfirmreq(int channel, char*data, int len)
{
    int sendlen ;

    USERCONFIRMREQ  *Userconfirmreq ;
    COMRESPACKET Userconfirmres ;

    Userconfirmreq = (USERCONFIRMREQ *)data ;

    Userconfirmres.identifier = htons(IDENTIFIER) ;
    Userconfirmres.cmd = htons(CMD_USERCONFIRM_RES) ;
    Userconfirmres.length = htons(8) ;
    Userconfirmres.result = htons(TRUE) ;

    memcpy(m_SendBuffer[channel], &Userconfirmres, 8) ;
    sendlen = send(SystemInfo.D_Channel[channel], m_SendBuffer[channel], 8, 0) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("Date user confirmres.. packet sendlen = %d\n",sendlen) ;
#endif

}


void strheaderreq(int channel, char *data, int len)
{   
    int sendlen = 0;//, retvalue = 0, i = 0 ;
    STRHEADERREQ *Strheaderreq ;
    STRHEADERRES Strheaderres ;

    time_t stime ;
    struct tm tm1;
    time(&stime) ;
    localtime_r(&stime, &tm1) ;

    Strheaderreq = (STRHEADERREQ *)data ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("tstrheaderreq packet receive..\n") ;
#endif

    Strheaderres.identifier = htons(IDENTIFIER) ;
    Strheaderres.cmd = htons(CMD_STRHEADER_RES) ;
    Strheaderres.length = htons(STRHEADERRES_SIZE) ;
    Strheaderres.channel = htons(channel) ;
    Strheaderres.Encodingtype = 0x03 ;  // 0x01 video only, 0x03 video + audio

    Strheaderres.Picturesize = SystemInfo.resolution;
    Strheaderres.Framerate = FRAMERATE1 ;

    Strheaderres.Reserved = 0x00 ;

    Strheaderres.Gmt = htons(0x00) ;
    Strheaderres.Year = htons(tm1.tm_year + 1900) ;
    Strheaderres.Month = htons(tm1.tm_mon + 1) ;
    Strheaderres.Day = htons(tm1.tm_mday) ;
    Strheaderres.Hour = htons(tm1.tm_hour) ;
    Strheaderres.Minute = htons(tm1.tm_min) ;
    Strheaderres.Second = htons(tm1.tm_sec) ;
    Strheaderres.DayofWeek = htons(0x00) ;
    Strheaderres.Bias = htons(0x00) ;
   // strncpy(Strheaderres.Cameratitle, gSETPARAM_info.sysInfo[ntohs(Strheaderreq->channel)].CameraTitle, 16) ;

    Strheaderres.AudioEncodingtype = htons(0x01) ; // u-law PCM(g711)
    Strheaderres.Inputchannel = htons(0x00) ;
    Strheaderres.SamplingFrequency = htons(0x00) ;
    Strheaderres.OutputBitrate = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Strheaderres, STRHEADERRES_SIZE) ;
    sendlen = send(SystemInfo.D_Channel[channel], m_SendBuffer[channel], STRHEADERRES_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("streamheader packet sendtoclient sendlen = %d, channel = %d\n",sendlen, channel) ;
    DEBUG_PRI("streamheaderreq receive camera channel = %d\n",ntohs(Strheaderreq->channel)) ;
#endif
}


void vstreamstart(int channel, char *data, int len) 
{
    VSTREAMSTART *Vstreamstart ;
    int sendlen = 0, vchannel = 0,  command = 0;//,status = 0, captureMode = 0, mode = 0 ;
    int chNoSignal = 1;
    Vstreamstart = (VSTREAMSTART *)data ;	
    vchannel = ntohs(Vstreamstart->streamchannel) ;
    set_rtsp_status(TRUE);
    SystemInfo.alternative = 1 ;
   // VSTREAM_enableCh(vchannel) ;
   SystemInfo.StreamEnable[vchannel] = TRUE;

    SystemInfo.headerrecv[channel] = 1 ;
    SystemInfo.encodingtype[channel] = 0 ;
    SystemInfo.datachannel = channel ;  // no signal A©ø¢¬¢ç¢¬| A¡×CN ¨¬I¨¬¨¢
    SystemInfo.videochannel[channel] = vchannel + 1 ;

#if 0
   // status = DIO_getNoSignalStatus(chLockStatus);
	chNoSignal = DVR_GetVideoLossStatus();
   if(chNoSignal & (1<<vchannel))
   {
   // if(status == 0)
   // {
//#ifdef NETWORK_DEBUG
//    DEBUG_PRI("vstreamstart chLockStatus[%d] = %d\n",vchannel, chLockStatus[vchannel]) ;
//#endif
       // if(chLockStatus[vchannel] == 0) 
       // {
            SystemInfo.Nosignal[vchannel] = TRUE ;
            command = CMD_NOSIGNAL ;
        }
        else
        {
            SystemInfo.Nosignal[vchannel] = FALSE ;
        }
   // }   
#endif
   /* captureMode = CAPTURE_getCurResolution();
    mode = 4;
    
    if(captureMode == RESOLUTION_8CH_CIF)
    {
        mode = 8 ;
    }

    if(captureMode == RESOLUTION_2CH_D1 || captureMode == RESOLUTION_2CH_D1_PLUS_2CH_CIF)
    {
        mode = 2 ;
    }

    if( mode <= vchannel)
        command = CMD_INVALID_DEVICE ; 
*/
    if(SystemInfo.Nosignal[vchannel] == TRUE || command != 0)
    {
        Vstreamerror.identifier = htons(IDENTIFIER) ;
        Vstreamerror.cmd = htons(CMD_VSTREAM_ERROR) ;
        Vstreamerror.length = htons(10) ;
        Vstreamerror.channel = htons(1) ;
        Vstreamerror.errcode = htons(command) ;
        sendlen = send(SystemInfo.D_Channel[channel], &Vstreamerror, 10,0) ;

#ifdef NETWORK_DEBUG
        DEBUG_PRI("system video nosignal packet send to client sendlen = %d\n",sendlen) ;
#endif
    }


#ifdef NETWORK_DEBUG
    DEBUG_PRI("vstreamstart receive channel = %d\n",vchannel) ;
#endif

}

void astreamstart(int channel, char *data, int len)
{
    ASTREAMSTART *Astreamstart ; 
    int  achannel = 0 ;
	set_rtsp_status(TRUE);
    Astreamstart = (ASTREAMSTART *)data ;
    achannel = ntohs(Astreamstart->streamchannel) ; 
    SystemInfo.headerrecv[channel] = 1 ;
    SystemInfo.encodingtype[channel] = 1 ;
    SystemInfo.audiochannel[channel] = achannel + 1 ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("audio stream start packet receive..\n") ;
#endif

}


void videoparamreq(int channel, char *data, int len)
{
    VIDEOPARAMRES Videoparamres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("video paramreq packet receive\n") ;
    DEBUG_PRI("VIDEOPARAMRES_SIZE = %d\n",VIDEOPARAMRES_SIZE) ;    
#endif

    Videoparamres.identifier = htons(IDENTIFIER) ;
    Videoparamres.cmd = htons(CMD_VIDEOPARAM_RES) ;
    Videoparamres.length = htons(VIDEOPARAMRES_SIZE) ;
    Videoparamres.channelgroup = 0x01 ;
    Videoparamres.encodingtype = 0x01 ;
    Videoparamres.encodingstandard = 0x01 ;
    Videoparamres.picturesize = 0x01 ;
    Videoparamres.framerate = 0x00 ;
    Videoparamres.pictureencodingtype = 0x00 ;
    Videoparamres.ipictureinterval = 0x01 ;
    Videoparamres.channelenable = 0x01 ;
    Videoparamres.currentchannel = 0x01 ;
    Videoparamres.reserved = 0x01 ;

    memcpy(m_SendBuffer[channel], &Videoparamres, VIDEOPARAMRES_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], VIDEOPARAMRES_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("videoparamres packet sendlen = %d\n",sendlen) ;
#endif

}

void videocfgreq(int channel, char *data, int len)
{
    VIDEOCFGREQ *Videocfgreq ;
    COMRESPACKET Videocfgres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("videocfgreq packet receive\n") ;
#endif

    Videocfgreq = (VIDEOCFGREQ*)data ;

    Videocfgres.identifier = htons(IDENTIFIER) ;
    Videocfgres.cmd = htons(CMD_VIDEOCFG_RES) ;
    Videocfgres.length = htons(RESULT_SIZE) ;
    Videocfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Videocfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("videocfgres packet sendlen = %d\n",sendlen) ;
#endif

}

void audioparamreq(int channel, char *data, int len)
{
    AUDIOPARAMRES Audioparamres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("audio paramreq packet receive\n") ;
#endif
    Audioparamres.identifier = htons(IDENTIFIER) ;
    Audioparamres.cmd = htons(CMD_AUDIOPARAM_RES) ;
    Audioparamres.length = htons(AUDIOPARAMRES_SIZE) ;
    Audioparamres.channelgroup = 0x01 ;
    Audioparamres.samplingfrequency = 0x01 ;
    Audioparamres.inputchannel = 0x01 ;
    Audioparamres.outputbitrate = 0x01 ;
    Audioparamres.encodingtype = 0x01 ;
    Audioparamres.channelenable = 0x01 ;

    memcpy(m_SendBuffer[channel], &Audioparamres, AUDIOPARAMRES_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], AUDIOPARAMRES_SIZE, 0) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("audioparamres packet sendlen = %d\n",sendlen) ;
#endif
}

void audiocfgreq(int channel, char *data, int len)
{
    AUDIOCFGREQ *Audiocfgreq ;
    COMRESPACKET Audiocfgres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("audiocfgreq packet receive!\n");
#endif

    Audiocfgreq = (AUDIOCFGREQ*)data ;

    Audiocfgres.identifier = htons(IDENTIFIER) ;
    Audiocfgres.cmd = htons(CMD_AUDIOCFG_RES) ;
    Audiocfgres.length = htons(RESULT_SIZE) ;
    Audiocfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Audiocfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Audiocfgres packet sendlen = %d\n",sendlen) ;
#endif

}


void bitrateparamreq(int channel, char *data, int len)
{
    BITRATEPARAMRES Bitrateparamres ;
    int sendlen = 0,  i = 0, videoMode=0, bitrate;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("bitrate paramreq packet receive\n") ;
#endif
   
    //videoMode = LIBDVR365_getResolution(); //no idea how to replace it

    Bitrateparamres.identifier = htons(IDENTIFIER) ;
    Bitrateparamres.cmd = htons(CMD_BITRATEPARAM_RES) ;
    Bitrateparamres.length = htons(sizeof(BITRATEPARAMRES)) ;
    Bitrateparamres.videoMode = htons(videoMode) ;
    Bitrateparamres.initialquant = (0x03) ;
    Bitrateparamres.bitratecontrol = (0x00) ; // 0x00 vbr, 0x01 cbr, 0x02 hybridvbr
 
    /*if(videoMode == RESOLUTION_2CH_D1 || videoMode == RESOLUTION_2CH_D1_PLUS_2CH_CIF)	//sk_cif
    {
       Maxchannel = 2 ;
    }
    else if(videoMode == RESOLUTION_8CH_CIF)
    {
       Maxchannel = 8 ;
    }
*/
    for(i = 0; i < 16; i++)
    { 
    		DVR_get_encoder_bps(i,&bitrate);
		if(bitrate == 3000000)
			Bitrateparamres.bitrate[i] = htons(BITRATE_ARG_HIGHEST);
		else if(bitrate == 2000000)
			Bitrateparamres.bitrate[i] = htons(BITRATE_ARG_HIGH);
		else if(bitrate == 1000000)
			Bitrateparamres.bitrate[i] = htons(BITRATE_ARG_MEDIUM);
		else //if(bitrate == 256000)
			Bitrateparamres.bitrate[i] = htons(BITRATE_ARG_LOW);
#ifdef NETWORK_DEBUG
           // DEBUG_PRI("bitrateparamreq DVR_get_encoder_bps(%d) = %d\n",i, bitrate) ;
#endif
      }

    memcpy(m_SendBuffer[channel], &Bitrateparamres, sizeof(BITRATEPARAMRES)) ;

    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], sizeof(BITRATEPARAMRES), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("bitrateparamres packet sendlen = %d\n",sendlen) ;
#endif
    
}

void framerateparamreq(int channel, char *data, int len)
{
    FRAMERATEPARAMRES Framerateparamres ;
    int sendlen = 0,ch, videoMode=0, framerate;

   // videoMode = LIBDVR365_getResolution();

    Framerateparamres.identifier = htons(IDENTIFIER) ;
    Framerateparamres.cmd = htons(CMD_FRAMERATEPARAM_RES) ;
    Framerateparamres.length = htons(sizeof(FRAMERATEPARAMRES)) ;
    Framerateparamres.videoMode = htons(videoMode) ;


	for (ch = 0; ch < MAX_CH; ch++)
	{

		//if(ch < )    //must know the number of video channel in each usecase
		DVR_get_encoder_fps(ch, &framerate);
		if((framerate == 30) || (framerate == 25))
			Framerateparamres.framerate[ch] = htons(FRAMERATE_30_25);
		if((framerate == 15) || (framerate == 13))
			Framerateparamres.framerate[ch] = htons(FRAMERATE_15_13);
		if((framerate == 8) || (framerate == 6))
			Framerateparamres.framerate[ch] = htons(FRAMERATE_08_06);
		if((framerate == 4) || (framerate == 3))
			Framerateparamres.framerate[ch] = htons(FRAMERATE_04_03);

#ifdef NETWORK_DEBUG
         //   DEBUG_PRI("framerateparamreq DVR_get_encoder_fps(%d) = %d\n",ch, framerate) ;
#endif
		
	}
				
    memcpy(m_SendBuffer[channel], &Framerateparamres, sizeof(FRAMERATEPARAMRES)) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel],sizeof(FRAMERATEPARAMRES), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("framerateparamres packet sendlen = %d\n",sendlen) ;
#endif
    
 
    
}


void frameratecfgreq(int channel, char *data, int len)
{
    FRAMERATECFGREQ *Frameratecfgreq;
    COMRESPACKET Frameratecfgres ;
    int sendlen = 0, framerate = 0, videoMode, Maxchannel = 0;
   // int resMode = 0, frameSkipMask = 0;
	int i;
    //SETPARAM_Info* pSetPrmInfo;
    
    Frameratecfgreq = (FRAMERATECFGREQ *)data ;

  //  pSetPrmInfo	= SETPARAM_getInfo();
    //resMode	= CAPTURE_getCurResolution();
    videoMode 	= ntohs(Frameratecfgreq->videoMode) ;
    framerate 	= ntohs(Frameratecfgreq->framerate[0]) ;

    Maxchannel = 16 ;
   /* if(videoMode == RESOLUTION_2CH_D1 || videoMode == RESOLUTION_2CH_D1_PLUS_2CH_CIF) //sk_cif
    {
       Maxchannel = 2 ;
    }
    else if(videoMode == RESOLUTION_8CH_CIF)
    {
       Maxchannel = 8 ;
    }
*/
   /* if(videoMode != resMode)
    {
	switch(framerate) 
        {
	    case FRAMERATE_8:
		frameSkipMask = FRAME_RATE_08_MASK ;
		break;			
	    case FRAMERATE_15:
		frameSkipMask = FRAME_RATE_15_MASK ;
		break;
	    case FRAMERATE_30:
	        frameSkipMask = FRAME_RATE_30_MASK ;
		break;				
	    default:
		frameSkipMask = FRAME_RATE_15_MASK ;
		break;
	}			
			
	for(i = 0; i < Maxchannel ; i++)
	{
	    pSetPrmInfo->codecSetPrm[i].frameSkipMask = frameSkipMask;
	}			
    }
    else
    {*/
        if (1)//(!gSETPARAM_info.useUIsettingMode)
        {
            for(i = 0; i < Maxchannel ; i++)
            {
            		switch(ntohs(Frameratecfgreq->framerate[i]))
        		{
	    			case FRAMERATE_30_25:
					framerate = 30 ;
					break;			
	   			case FRAMERATE_15_13:
					framerate = 15 ;
					break;
	   			case FRAMERATE_08_06:
	        			framerate = 8 ;
					break;		
				case FRAMERATE_04_03:
					framerate = 4;
	    			default:
					framerate = 15 ;
					break;
            		} 
                	//DVR_set_encoder_fps(i, framerate);
                	printf("Set %d channel framerate: %d\n",i,ntohs(Frameratecfgreq->framerate[i]));
            	}
        }
    //}

    Frameratecfgres.identifier = htons(IDENTIFIER) ;
    Frameratecfgres.cmd = htons(CMD_FRAMERATECFG_RES) ;
    Frameratecfgres.length = htons(RESULT_SIZE) ;

    if (1)//(!gSETPARAM_info.useUIsettingMode)
    {
        Frameratecfgres.result = htons(0x3001) ;
    }
    else 
    {
        Frameratecfgres.result = htons(0x3040) ;
    }
    memcpy(m_SendBuffer[channel], &Frameratecfgres, RESULT_SIZE) ;

    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("frameratecfgres packet sendlen = %d\n",sendlen) ;
#endif
}

void iframesettingparamreq(int channel, char *data, int len)
{
    IFRAMESETTINGPARAMRES Iframesettingparamres ;
    int sendlen = 0, videoMode, i, Maxchannel = 0,gop=0;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Iframesetting param packet receive..\n") ;
#endif

    //videoMode = LIBDVR365_getResolution();

    Iframesettingparamres.identifier = htons(IDENTIFIER) ;
    Iframesettingparamres.cmd = htons(CMD_IFRAMESETTINGPARAM_RES) ;
    Iframesettingparamres.length = htons(sizeof(IFRAMESETTINGPARAMRES)) ;
    Iframesettingparamres.videoMode = htons(videoMode) ;

    Maxchannel = 16 ;
    /*if(videoMode == RESOLUTION_2CH_D1 || videoMode == RESOLUTION_2CH_D1_PLUS_2CH_CIF) //sk_cif
    {
       Maxchannel = 2 ;
    }
    else if(videoMode == RESOLUTION_8CH_CIF)
    {
       Maxchannel = 8 ;
    }
*/
    for(i = 0; i < MAX_CH; i++)
    {
	DVR_get_encoder_iframeinterval( i, &gop);//(LIBDVR365_getChannelIframeInterval(i));
			switch(gop)
        		{
	    			case 1:
					Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_1) ;
					break;			
	   			case 5:
					Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_5);
					break;
	   			case 10:
	        			Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_10);
					break;		
				case 15:
					Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_15);
					break;
				case 30:
					Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_30);
					break;
	    			default:
					Iframesettingparamres.gop[i] = htons(IFRAME_INTERVAL_30);
					break;
            		} 
//#ifdef NETWORK_DEBUG
 //           DEBUG_PRI("iframesettingparamreq DVR_get_encoder_iframeinterval(%d) = %d\n",i, gop) ;
//#endif
    }

    memcpy(m_SendBuffer[channel], &Iframesettingparamres, sizeof(IFRAMESETTINGPARAMRES)) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], sizeof(IFRAMESETTINGPARAMRES), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("iframesettingparamres packet sendlen = %d\n",sendlen) ;
#endif
}

void iframesettingcfgreq(int channel, char *data, int len)
{
    IFRAMESETTINGCFGREQ *Iframesettingcfgreq ;
    COMRESPACKET Iframesettingcfgres ;
    int sendlen = 0, videoMode=0, i, Maxchannel = 0,gop;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("iframesettingcfgreq..packet receive..\n") ;
#endif

    Iframesettingcfgreq = (IFRAMESETTINGCFGREQ *)data ;
    videoMode = ntohs(Iframesettingcfgreq->videoMode) ;

    Maxchannel = 16 ;

    Iframesettingcfgres.identifier = htons(IDENTIFIER) ;
    Iframesettingcfgres.cmd = htons(CMD_IFRAMESETTINGCFG_RES) ;
    Iframesettingcfgres.length = htons(RESULT_SIZE) ; 

    if (1)//(!gSETPARAM_info.useUIsettingMode)
    {
        for(i = 0; i < Maxchannel; i++)
        {
		switch(ntohs(Iframesettingcfgreq->gop[i]))
        	{
	    		case IFRAME_INTERVAL_1:
				gop = 1 ;
				break;			
	   		case IFRAME_INTERVAL_5:
				gop = 5;
				break;
	   		case IFRAME_INTERVAL_10:
	        		gop = 10;
				break;		
			case IFRAME_INTERVAL_15:
				gop = 15;
				break;
			case IFRAME_INTERVAL_30:
				gop = 30;
				break;
	    		default:
				gop = 30;
				break;
            	} 
		//DVR_set_encoder_iframeinterval(i, gop) ;
		printf("Set %d channel gop: %d\n",i,ntohs(Iframesettingcfgreq->gop[i]));
        }

        Iframesettingcfgres.result = htons(0x3001) ;
    }
    else
    {
        Iframesettingcfgres.result = htons(0x3040) ;
    }
    memcpy(m_SendBuffer[channel], &Iframesettingcfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("iframesettingcfgres packet sendlen = %d\n",sendlen) ;
#endif

}   


void bitratecfgreq(int channel, char *data, int len)
{
    COMRESPACKET Bitratecfgres ;
    BITRATECFGREQ *Bitratecfgreq ;

    int sendlen = 0, Maxchannel = 0 ;
    int i = 0, initialquant = 0, bitratecontrol = 0, videoMode, bitrate;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("bitratecfgreq packet receive! length = %d\n",len) ;
#endif
    Bitratecfgreq = (BITRATECFGREQ *)data ;

    videoMode = ntohs(Bitratecfgreq->videoMode) ;
    initialquant = Bitratecfgreq->initialquant ;
    bitratecontrol = Bitratecfgreq->bitratecontrol ;

    Maxchannel = 16 ;
    /*if(videoMode == RESOLUTION_2CH_D1 || videoMode == RESOLUTION_2CH_D1_PLUS_2CH_CIF) //sk_cif
    {
       Maxchannel = 2 ;
    }
    else if(videoMode == RESOLUTION_8CH_CIF)
    {
       Maxchannel = 8 ;
    }
*/
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Maxchannel = %d\n",Maxchannel) ;
#endif

    Bitratecfgres.identifier = htons(IDENTIFIER) ;
    Bitratecfgres.cmd = htons(CMD_BITRATECFG_RES) ;
    Bitratecfgres.length = htons(RESULT_SIZE) ;

    if (1)//(!gSETPARAM_info.useUIsettingMode)
    {
        for(i = 0; i < Maxchannel; i++)
        {
		switch(ntohs(Bitratecfgreq->bitrate[i]))
		{
			case BITRATE_ARG_HIGHEST:
				bitrate = 3000000;
				break;
			case BITRATE_ARG_HIGH:
				bitrate = 2000000;
				break;
			case BITRATE_ARG_MEDIUM:
				bitrate = 1000000;
				break;
			case BITRATE_ARG_LOW:
				bitrate = 256000;
				break;
			default:
				bitrate = 3000000;
				break;
		}
		//DVR_set_encoder_bps(i, bitrate) ;
		printf("Set %d channel bitrate: %d\n",i,ntohs(Bitratecfgreq->bitrate[i]));
        }
        Bitratecfgres.result = htons(0x3001) ;
    }
    else
    {
        Bitratecfgres.result = htons(0x3040) ;
    }
    memcpy(m_SendBuffer[channel], &Bitratecfgres, RESULT_SIZE) ;

    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("bitratecfgres packet sendlen = %d\n",sendlen) ;
    DEBUG_PRI("customize variable initialize...\n") ;
#endif
}

void userparamreq(int channel, char *data, int len)
{
    USERPARAMRES Userparamres ;
    int sendlen = 0, retvalue = 0, i = 0 ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("user paramreq packet receive\n") ;
#endif

  //  retvalue = netaddr(USERMODIFYCFG) ;

    Userparamres.identifier = htons(IDENTIFIER) ;
    Userparamres.cmd = htons(CMD_USERPARAM_RES);
    Userparamres.length = htons(6 + (30*retvalue)) ; 
 
    memcpy(m_SendBuffer[channel], &Userparamres, 6) ;
    for(i = 0; i < retvalue; i++)
    {
        memcpy(&m_SendBuffer[channel][6+(30*i)], &Userinfocfg[i], 30) ;
    }
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], 6 +(30*retvalue), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("userparamres packet sendlen = %d\n",sendlen) ;
#endif

    
}

void usercfgreq(int channel, char *data, int len)
{
    USERCFGREQ *Usercfgreq ;
    COMRESPACKET Usercfgres ;
    FILE *fp = NULL;
    int sendlen = 0, usercount = 0, i = 0;//, retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("usercfgreq packet receive! length = %d\n",len) ;
#endif
    
    Usercfgreq = (USERCFGREQ *)data ;
    
    if(len > 36)
    {
        usercount = ((len - 6) / 30) ;
    }

    //retvalue = netaddr(USERMODIFYCFG) ;

    if(( fp = fopen("/mnt/settings/userinfo", "w")) == NULL)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("userinfo file open error\n") ;
#endif
    }
    for(i = 0; i < usercount + 1; i++)
    {
        Usercfgreq = (USERCFGREQ *)&data[i*30] ;  
 
        if(Usercfgreq->UserId != NULL)
        {
            fprintf(fp,"USERID=\"%s\"\n",Usercfgreq->UserId);
        }
        else
        {
            fprintf(fp,"USERID=\"%s\"\n",Userinfocfg[0].Id);
        }            
        if(Usercfgreq->UserPasswd != NULL)
        {
            fprintf(fp,"USERPASSWD=\"%s\"\n",Usercfgreq->UserPasswd);
        }
        else
        {
            fprintf(fp,"USERPASSWD=\"%s\"\n",Userinfocfg[0].Passwd);
        }
        if(Usercfgreq->channelaccess != 0)
        {
            fprintf(fp,"CHACCESS=%d\n",ntohs(Usercfgreq->channelaccess)) ;
        }
        else
        { 
            fprintf(fp,"CHACCESS=%d\n",ntohs(Userinfocfg[0].channelaccess)) ;
        }
        if(Usercfgreq->controlaccess != 0)
        {
            fprintf(fp,"CONTROLACCESS=%d\n",ntohl(Usercfgreq->controlaccess)) ;
        }
        else
        {
            fprintf(fp,"CONTROLACCESS=%d\n",ntohl(Userinfocfg[0].controlaccess)) ;
        }
    }

    fclose(fp) ;

    Usercfgres.identifier = htons(IDENTIFIER) ;
    Usercfgres.cmd = htons(CMD_USERCFG_RES) ;
    Usercfgres.length = htons(RESULT_SIZE) ;
    Usercfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Usercfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("usercfgres packet sendlen = %d\n",sendlen) ; 
#endif

}

void serverparamreq(int channel, char *data, int len) 
{
    SERVERPARAMRES Serverparamres ;
    int sendlen = 0,retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("serverparamreq..packet receive !\n") ;
#endif
    
    //retvalue = netaddr(NETWORKMODIFYCFG) ;

    Serverparamres.identifier = htons(IDENTIFIER) ;
    Serverparamres.cmd = htons(CMD_SERVERPARAM_RES) ;
    Serverparamres.length = htons(SERVERPARAMRES_SIZE) ;
    Serverparamres.tcplogonport = htons(Networkcfg.port) ;
    Serverparamres.tcpcontrolport = htons(Networkcfg.port+1) ;
    Serverparamres.tcpdataport = htons(Networkcfg.port + 2) ;
    Serverparamres.udplogonport = htons(Networkcfg.port) ;
    Serverparamres.udpcontrolport = htons(Networkcfg.port+1) ;
    Serverparamres.udpdataport = htons(Networkcfg.port+2) ;
    Serverparamres.tcpsessionlimit = htons(SESSIONLIMIT) ;
    Serverparamres.udpsessionlimit = htons(SESSIONLIMIT) ;
    strncpy(Serverparamres.servertitle, "testserver",11) ;
    strncpy(Serverparamres.serverlocation, "udworks office",15) ;
    strncpy(Serverparamres.serverdescription, "test",5) ;

    memcpy(m_SendBuffer[channel], &Serverparamres, SERVERPARAMRES_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], SERVERPARAMRES_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("serverparamres packet sendlen = %d\n",sendlen) ;
#endif

}


void svrportcfgreq(int channel, char *data, int len)
{
    SVRPORTCFGREQ *Svrportcfgreq ;
    COMRESPACKET Svrportcfgres ;
    int sendlen = 0, logonport = 0, controlport = 0, dataport = 0, validbit = 0, i = 0, retvalue = 0 ;
    
    FILE *fp = NULL ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("svrportcfgreq packet receive ! length = %d\n",len) ;
#endif

    Svrportcfgreq = (SVRPORTCFGREQ *)data ;
    
  //  retvalue = netaddr(NETWORKMODIFYCFG) ;
    validbit = ntohs(Svrportcfgreq->validbit) ;
    logonport= ntohs(Svrportcfgreq->tcplogonport) ; 
    controlport = ntohs(Svrportcfgreq->tcpcontrolport) ;
    dataport = ntohs(Svrportcfgreq->tcpcontrolport) ;
#if 0
    if(( fp = fopen("/etc/network/cfg-network" , "w")) == NULL)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("ipservcfg-network file open error\n") ;
#endif
    }

    fprintf(fp, "DHCP=%d\n",Networkcfg.dhcp) ;
    fprintf(fp, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
    fprintf(fp, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
    fprintf(fp, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
    fprintf(fp, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
    fprintf(fp, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
    if(logonport != 0)
    {
        fprintf(fp, "PORT=%d\n",logonport) ;
    }
    else
    {
        fprintf(fp, "PORT=%d\n",Networkcfg.port) ;
    }
    fprintf(fp, "NETWORK=\"\"\n") ;

    fclose(fp) ;
 #endif   

    Svrportcfgres.identifier = htons(IDENTIFIER) ;
    Svrportcfgres.cmd = htons(CMD_SVRPORTCFG_RES) ;
    Svrportcfgres.length = htons(RESULT_SIZE) ;
    Svrportcfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Svrportcfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Svrportcfgres packet sendlen = %d\n",sendlen );
#endif
 
    for(i = 1; i < MAXUSER; i++)
    {
        if(SystemInfo.C_Channel[i] != 0)
        close(SystemInfo.C_Channel[i]) ;
    }
    Netsettingchange.setting = 1 ;
}

void svrportparamreq(int channel, char *data, int len)
{
    SVRPORTPARAMRES Svrportparamres ;
    int sendlen = 0, retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("svrportparamreq packet receive..\n") ;
#endif
 
    //retvalue = netaddr(NETWORKMODIFYCFG) ;
    
    Svrportparamres.identifier = htons(IDENTIFIER) ;
    Svrportparamres.cmd = htons(CMD_SVRPORTPARAM_RES) ; 
    Svrportparamres.length = htons(20) ;
    Svrportparamres.validbit = htons(1) ;
    Svrportparamres.tcplogonport = htons(Networkcfg.port) ;
    Svrportparamres.tcpcontrolport = htons(Networkcfg.port + 1) ;
    Svrportparamres.tcpdataport = htons(Networkcfg.port + 2) ;
    Svrportparamres.udplogonport = htons(Networkcfg.port) ;
    Svrportparamres.udpcontrolport = htons(Networkcfg.port + 1) ;
    Svrportparamres.udpdataport = htons(Networkcfg.port + 2) ;
    
    memcpy(m_SendBuffer[channel], &Svrportparamres, 20) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], 20, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Svrportparamres packet sendlen = %d\n",sendlen ) ;
#endif
}


void slimitcfgreq(int channel, char *data, int len)
{
    SLIMITCFGREQ *Slimitcfgreq ;
    COMRESPACKET Slimitcfgres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("slimitcfgreq packet receive ! length = %d\n",len) ;
#endif

    Slimitcfgreq = (SLIMITCFGREQ *)data ;
    
    Slimitcfgres.identifier = htons(IDENTIFIER) ;
    Slimitcfgres.cmd = htons(CMD_SLIMITCFG_RES) ;
    Slimitcfgres.length = htons(RESULT_SIZE) ;
    Slimitcfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Slimitcfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Slimitcfgres packet sendlen = %d\n",sendlen );
#endif

}

void svrinfocfgreq(int channel, char *data, int len)
{
    SVRINFOCFGREQ *Svrinfocfgreq ;
    COMRESPACKET Svrinfocfgres ;
    int sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("svrinfocfgreq packet receive ! length = %d\n",len) ;
#endif

    Svrinfocfgreq = (SVRINFOCFGREQ *)data ;
    
    Svrinfocfgres.identifier = htons(IDENTIFIER) ;
    Svrinfocfgres.cmd = htons(CMD_SVRINFOCFG_RES) ;
    Svrinfocfgres.length = htons(RESULT_SIZE) ;
    Svrinfocfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Svrinfocfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Svrinfocfgres packet sendlen = %d\n",sendlen );
#endif

}


void cfgchange(void)
{
    CFGCHANGE Cfgchange ;
    int sendlen = 0, i = 0 ;
    
    Cfgchange.identifier = htons(IDENTIFIER) ;
    Cfgchange.cmd = htons(CMD_CFGCHANGE) ;
    Cfgchange.length = htons(CFGCHANGE_SIZE) ;

    memcpy(m_SendBuffer[0], &Cfgchange, CFGCHANGE_SIZE) ;
    for(i = 1 ; i < MAXUSER; i++)
    {
        if(SystemInfo.C_Channel[i] != 0)
        {
            sendlen = send(SystemInfo.C_Channel[i], m_SendBuffer[0], CFGCHANGE_SIZE, 0) ;
        }
    }
#ifdef NETWORK_DEBUG
    DEBUG_PRI("cfgchange packet sendlen = %d\n",sendlen );
#endif

}

void reboot(int channel, char *data, int len)
{
    REBOOTRES Rebootres ;
    int sendlen = 0, i = 0, rebootfd = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("system reboot packet receive\n") ; 
#endif
 
    Rebootres.identifier = htons(IDENTIFIER) ;
    Rebootres.cmd = htons(CMD_REBOOT_RES) ;
    Rebootres.length = htons(6) ;

    memcpy(m_SendBuffer[channel], &Rebootres, 6) ;
    for(i = 1 ; i < MAXUSER; i++)
    {
        if(SystemInfo.C_Channel[i] != 0)
        {
            sendlen = send(SystemInfo.C_Channel[i], m_SendBuffer[channel], 6, 0) ;
        }
    }

#ifdef NETWORK_DEBUG
    DEBUG_PRI("rebootres packet sendlen = %d\n",sendlen) ;
#endif
    sleep(3) ;

    if(SystemInfo.Nosignal[channel] == 1)
    {
        rebootfd = open("/dev/remote",O_RDWR) ;
        if(rebootfd < 0)
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("reboot device open Error\n") ;
#endif
        }
        else
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("reboot device open Success\n") ;
#endif
        }
        ioctl(rebootfd, BOOOOM_REBOOT) ;
    }
    else
    {
        Systemreboot.reboot = TRUE ;
        Netsettingchange.setting = 1 ;
    }
}


void resolutionparamreq(int channel, char *data, int len)
{
    RESOLUTIONPARAMRES Resolutionparamres ;
    int sendlen = 0, resolution = 0, mode = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("resolutionparamreq packet receive!\n") ;
#endif
 
    //mode = LIBDVR365_getResolution();
    resolution = SystemInfo.resolution ;

    Resolutionparamres.identifier = htons(IDENTIFIER) ;
    Resolutionparamres.cmd = htons(CMD_RESOLUTIONPARAM_RES) ;
    Resolutionparamres.length = htons(RESOLUTIONPARAMRES_SIZE) ;
    Resolutionparamres.videoMode = htons(mode) ;
    Resolutionparamres.ssize = htons(resolution) ;

    memcpy(m_SendBuffer[channel], &Resolutionparamres, RESOLUTIONPARAMRES_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESOLUTIONPARAMRES_SIZE,0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("resolutionparamres packtet sendlen = %d\n", sendlen) ;
#endif
}


void resolutioncfgreq(int channel, char *data, int len)
{
    RESOLUTIONCFGREQ *Resolutioncfgreq ;
    COMRESPACKET Resolutioncfgres ;

    int sendlen = 0, Ssize = 0, videoMode = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("resolutioncfgreq packet receive! \n") ;
#endif

    Resolutioncfgreq = (RESOLUTIONCFGREQ*)data ;

    videoMode = ntohs(Resolutioncfgreq->videoMode) ;
    Ssize = ntohs(Resolutioncfgreq->Ssize) ;

    Resolutioncfgres.identifier = htons(IDENTIFIER) ;
    Resolutioncfgres.cmd = htons(CMD_RESOLUTIONCFG_RES) ;
    Resolutioncfgres.length = htons(RESULT_SIZE) ;

    if (1)//(!gSETPARAM_info.useUIsettingMode)
    {
        Resolutioncfgres.result = htons(0x3001) ;

        memcpy(m_SendBuffer[channel], &Resolutioncfgres, RESULT_SIZE) ;

       // LIBDVR365_setResolution(videoMode) ;
    }
    else
    {
        Resolutioncfgres.result = htons(0x3040) ;
    }
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Resolutioncfgres packet sendlen = %d\n",sendlen) ;
#endif
}



void idnparamreq(int channel, char *data, int len)
{
    
    //IPSERVNETWORKCFG Ipservnetworkcfg ;
    IDNPARAMRES Idnparamres ;
    int sendlen = 0;//, retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("idnparamreq packet receive length = %d\n",len) ;
#endif

//    retvalue = netaddr(IPSERVMODIFYNETWORKCFG) ;

    Idnparamres.identifier = htons(IDENTIFIER) ;
    Idnparamres.cmd = htons(CMD_IDNPARAM_RES) ;
    Idnparamres.length = htons(IDNPARAMRES_SIZE) ;
    Idnparamres.status = Ipservnetworkcfg.status ; // 0x01 enable 0x00 disable
    Idnparamres.protocol = Ipservnetworkcfg.protocoltype ; // 0x01 type1 0x02 type2
    Idnparamres.interval = htons(Ipservnetworkcfg.interval) ;
    Idnparamres.remoteport = htons(Ipservnetworkcfg.port) ;
    Idnparamres.ipaddress = Ipservnetworkcfg.ipaddress.s_addr ;

    memcpy(m_SendBuffer[channel], &Idnparamres, IDNPARAMRES_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], IDNPARAMRES_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Idnparamres packet sendlen = %d\n",sendlen) ;
#endif
    
}

void idncfgreq(int channel, char *data, int len)
{
    IDNCFGREQ *Idncfgreq ;
    COMRESPACKET Idncfgres ;
    int sendlen = 0, status = 0, protocol = 0, interval = 0, remoteport = 0 ;
    FILE *fp ;
    struct sockaddr_in addr ;
    char *str ;

    Idncfgreq = (IDNCFGREQ *)data ;

    status = Idncfgreq->status ;
    protocol = Idncfgreq->protocol ;
    interval = ntohs(Idncfgreq->interval) ;
    remoteport = ntohs(Idncfgreq->remoteport) ;
    addr.sin_addr.s_addr = Idncfgreq->ipaddress ;
    str = inet_ntoa(addr.sin_addr) ;
#if 0
    if(( fp = fopen("/etc/network/cfg-network" , "w")) == NULL)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("ipserv-network file open error\n") ;
#endif
    }
 
    if(status != 0)
    {
        fprintf(fp,"STATUS=%d\n",status) ;
    }
    else
    {
        fprintf(fp,"STATUS=%d\n",Ipservnetworkcfg.status) ;
    }
    if(protocol != 0)
    {
        fprintf(fp,"PROTOCOLTYPE=%d\n",protocol) ;
    }
    else
    {
        fprintf(fp,"PROTOCOLTYPE=%d\n",Ipservnetworkcfg.protocoltype) ;
    }
    if(interval != 0)
    { 
        fprintf(fp,"INTERVAL=%d\n",interval) ;
    }
    else
    {
        fprintf(fp,"INTERVAL=%d\n",Ipservnetworkcfg.interval) ;

    }
    if(remoteport != 0)
    {
        fprintf(fp,"PORT=%d\n",remoteport) ;
    }
    else
    {
        fprintf(fp,"PORT=%d\n",Ipservnetworkcfg.port) ;

    }
    if(str != NULL)
    {
        fprintf(fp,"IPADDRESS=\"%s\"\n",str);
    }
    else
    {
        fprintf(fp,"IPADDRESS=\"%s\"\n",inet_ntoa(Ipservnetworkcfg.ipaddress));

    }
    fclose(fp) ;
#endif
    Idncfgres.identifier = htons(IDENTIFIER);
    Idncfgres.cmd = htons(CMD_IDNCFG_RES) ;
    Idncfgres.length = htons(RESULT_SIZE) ;
    Idncfgres.result = htons(0x00) ;

    memcpy(m_SendBuffer[channel], &Idncfgres, RESULT_SIZE) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("idncfgres packet sendlen = %d\n",sendlen) ;
#endif

}

//It is unceccessary to open MAC to user to modify, so this function should be removed, but other net para should be set
//We should compara para between UI and profile.netinfo, if not same, update profile then call set_network_info to set them.
//GetNetworkdata realize it, we can modify it to use.
void macchange(int channel, char *data, int len)
{
    MACCHANGE *Macchange ;
    COMRESPACKET Macchangeres ;
    int sendlen = 0, retvalue = 0 ;

    FILE *fp, *fd ;
    
    Macchange = (MACCHANGE *)data ;
#if 0
    retvalue = netaddr(NETWORKMODIFYCFG) ;

    if((fp = fopen("/etc/network/cfg-network", "w")) == NULL)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("/mnt/settings/cfg-network file open error for macchange..\n") ;
#endif
    }
    if((fd = fopen("/etc/network/ftdt/cfg-network", "w")) == NULL)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("/etc/network/ftdt/cfg-network file open error for macchange..\n") ;
#endif
    }

    if(Macchange->macaddress != NULL)
    {
        fprintf(fp, "DHCP=%d\n",Networkcfg.dhcp) ;
        fprintf(fp, "MAC=\"%s\"\n",Macchange->macaddress) ;
        fprintf(fp, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
        fprintf(fp, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
        fprintf(fp, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
        fprintf(fp, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
        fprintf(fp, "PORT=%d\n",Networkcfg.port) ;
        fprintf(fp, "NETWORK=\"\"\n") ;

        fprintf(fd, "DHCP=%d\n",Networkcfg.dhcp) ;
        fprintf(fd, "MAC=\"%s\"\n",Macchange->macaddress) ;
        fprintf(fd, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
        fprintf(fd, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
        fprintf(fd, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
        fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
        fprintf(fd, "PORT=%d\n",Networkcfg.port) ;
        fprintf(fd, "NETWORK=\"\"\n") ;
    }
    else
    {
        fprintf(fp, "DHCP=%d\n",Networkcfg.dhcp) ;
        fprintf(fp, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
        fprintf(fp, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
        fprintf(fp, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
        fprintf(fp, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
        fprintf(fp, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
        fprintf(fp, "PORT=%d\n",Networkcfg.port) ;
        fprintf(fp, "NETWORK=\"\"\n") ;

        fprintf(fd, "DHCP=%d\n",Networkcfg.dhcp) ;
        fprintf(fd, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
        fprintf(fd, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
        fprintf(fd, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
        fprintf(fd, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
        fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
        fprintf(fd, "PORT=%d\n",Networkcfg.port) ;
        fprintf(fd, "NETWORK=\"\"\n") ;
    }
    fclose(fp) ;
    fclose(fd) ;
#endif

/*
    if((Macchange->dev_no == 1) && (Macchange->macaddress != profile.net_info1.mac))
    {
		profile.net_info1.mac = Macchange->macaddress;
		set_net_info(Macchange->dev_no, Macchange->macaddress);
    } 
    else if((Macchange->dev_no == 2) && (Macchange->macaddress != profile.net_info2.mac)
    {
		profile.net_info2.mac = Macchange->macaddress;
		set_net_info(Macchange->dev_no, Macchange->macaddress);
    }
 */
    Macchangeres.identifier = htons(IDENTIFIER) ;
    Macchangeres.cmd = htons(CMD_MACCHANGE_RES) ;
    Macchangeres.length = htons(RESULT_SIZE) ;
    Macchangeres.result = htons(0x0001) ;

    sendlen = send(SystemInfo.C_Channel[channel], &Macchangeres, RESULT_SIZE, 0) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("mac change sendlen = %d\n", sendlen) ;
#endif

    Netsettingchange.setting = 1 ;
}

void sdalarmopen(int channel, char *data, int len)
{
#ifdef NETWORK_DEBUG
    DEBUG_PRI("sdalarm open packet receive!\n") ;
#endif
}

void ptzctrlparamreq(int channel, char *data, int len)
{	
	PTZCTRLPARAMRES Ptzctrlparamres;

    char sendlen = 0;//, retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("ptzctrlparamreq packet receive length = %d\n",len) ;
#endif

    Ptzctrlparamres.identifier = htons(IDENTIFIER) ;
    Ptzctrlparamres.cmd = htons(CMD_PTZCTRLPARAM_RES) ;
    Ptzctrlparamres.length = sizeof(PTZCTRLPARAMRES);
    Ptzctrlparamres.protocol = 0x01;//PELCO_D
    Ptzctrlparamres.bandrate = INT_BAUDRATE_2400;
    Ptzctrlparamres.channel = 0x00;
    Ptzctrlparamres.ptzcmd = 0x00;

    memcpy(m_SendBuffer[channel], &Ptzctrlparamres, sizeof(PTZCTRLPARAMRES)) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], sizeof(PTZCTRLPARAMRES), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("ptzctrlparamres packet sendlen = %d\n",sendlen) ;
#endif
}

void ptzctrlcfgreq(int channel, char *data, int len)
{
	char vchannel,vprotocol,vptzcmd,vbaudrate,baudrate,sendlen=0;
	PTZCTRLCFGREQ *Ptzctrlcfgreq;
	//PTZCTRLCFGRES	Ptzctrlparmres;
	COMRESPACKET Ptzctrlcfgres;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("ptzctrlcfgreq packet receive,channel=%d\n",channel) ;
#endif

	Ptzctrlcfgreq = (PTZCTRLCFGREQ *)data;
	vchannel = Ptzctrlcfgreq->channel;
	vprotocol = Ptzctrlcfgreq->protocol;
	vptzcmd = Ptzctrlcfgreq->ptzcmd;
	vbaudrate = Ptzctrlcfgreq->bandrate;

	switch(vbaudrate)
	{
		case INT_BAUDRATE_1200:
			baudrate = 1200;
			break;
		case INT_BAUDRATE_2400:
			baudrate = 2400;
			break;	
		case INT_BAUDRATE_4800:
			baudrate = 4800;
			break;
		case INT_BAUDRATE_9600:
			baudrate = 9600;
			break;
		case INT_BAUDRATE_19200:
			baudrate = 19200;
			break;
		case INT_BAUDRATE_38400:
			baudrate = 28400;
			break;	
		case INT_BAUDRATE_57600:
			baudrate = 57600;
			break;
		case INT_BAUDRATE_115200:
			baudrate = 115200;
			break;
		default:
			baudrate = 9600;
			break;
	}
	DVR_RE_ptz_setInfo(vprotocol,baudrate);
	DVR_RE_ptz_setInternal_Ctrl(vprotocol,vchannel,vptzcmd);

	Ptzctrlcfgres.identifier = htons(IDENTIFIER) ;
	Ptzctrlcfgres.cmd = htons(CMD_PTZCTRLCFG_RES);
	Ptzctrlcfgres.length = htons(sizeof(COMRESPACKET));
	Ptzctrlcfgres.result = 1;

	sendlen = send(SystemInfo.C_Channel[channel], &Ptzctrlcfgres, sizeof(COMRESPACKET), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Ptzctrlcfgres packet sendlen = %d,vchannel=%d\n",sendlen,vchannel) ;
#endif

}

#if 0
void ptzinfoparamreq(int channel, char *data, int len)
{	
	PTZINFOPARAMRES Ptzinfoparamres;

    char sendlen = 0;//, retvalue = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("ptzctrlparamreq packet receive length = %d\n",len) ;
#endif

    Ptzinfoparamres.identifier = htons(IDENTIFIER) ;
    Ptzinfoparamres.cmd = htons(CMD_PTZINFOPARAM_RES) ;
    Ptzinfoparamres.length = sizeof(PTZINFOPARAMRES);
    Ptzinfoparamres.protocol = 0x01;//PELCO_D
    Ptzinfoparamres.targetaddr= 0x00;
    Ptzinfoparamres.bandrate= INT_BAUDRATE_2400;
    Ptzinfoparamres.databit = INT_DATBIT_7;
    Ptzinfoparamres.stopbit = INT_PARITY_NONE;
    Ptzinfoparamres.paritybit = INT_STOPBIT_1;

    memcpy(m_SendBuffer[channel], &Ptzinfoparamres, sizeof(PTZINFOPARAMRES)) ;
    sendlen = send(SystemInfo.C_Channel[channel], m_SendBuffer[channel], sizeof(PTZINFOPARAMRES), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Ptzinfoparamres packet sendlen = %d\n",sendlen) ;
#endif
}

void ptzinfocfgreq(int channel, char *data, int len)
{
	char sendlen=0;
	PTZINFOCFGREQ *Ptzinfocfgreq;
	//PTZCTRLCFGRES	Ptzctrlparmres;
	COMRESPACKET Ptzinfocfgres;
	char ptzProtocol,pztBaudrate,ptzDatabit,pztStopbit,ptzParitybit;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("PTZINFOCFGREQ packet receive\n") ;
#endif

	Ptzinfocfgreq = (PTZINFOCFGREQ *)data;
	ptzProtocol = Ptzinfocfgreq->protocol;
	pztBaudrate = Ptzinfocfgreq->baudrate;
	ptzDatabit = Ptzinfocfgreq->databit;
	pztStopbit = Ptzinfocfgreq->stopbit;
	ptzParitybit = Ptzinfocfgreq->paritybit;
	

	switch(pztBaudrate)
	{
		case INT_BAUDRATE_1200:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 1200;
			break;
		case INT_BAUDRATE_2400:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 2400;
			break;	
		case INT_BAUDRATE_4800:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 4800;
			break;
		case INT_BAUDRATE_9600:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 9600;
			break;
		case INT_BAUDRATE_19200:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 19200;
			break;
		case INT_BAUDRATE_38400:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 28400;
			break;	
		case INT_BAUDRATE_57600:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 57600;
			break;
		case INT_BAUDRATE_115200:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 115200;
			break;
		default:
			gPTZLIST_intList[3+ptzProtocol].ptzBaudrate = 9600;
			break;
	}

	gPTZLIST_intList[3+ptzProtocol].ptzIdx = ptzProtocol;
	gPTZLIST_intList[3+ptzProtocol].ptzDatabit = ptzDatabit;
	gPTZLIST_intList[3+ptzProtocol].ptzStopbit = pztStopbit; 
	gPTZLIST_intList[3+ptzProtocol].ptzParitybit = pztStopbit;
	DVR_RE_ptz_setInfo(3+ptzProtocol);

	Ptzinfocfgres.identifier = htons(IDENTIFIER) ;
	Ptzinfocfgres.cmd = htons(CMD_PTZCTRLCFG_RES);
	Ptzinfocfgres.length = htons(sizeof(COMRESPACKET));
	Ptzinfocfgres.result = 1;

	sendlen = send(SystemInfo.C_Channel[channel], &Ptzinfocfgres, sizeof(COMRESPACKET), 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("Ptzctrlcfgres packet sendlen = %d\n",sendlen) ;
#endif

}
#endif
void playbackdatereq(int channel, char *data, int len)
{

    T_BKTRMP_CTRL ParamCtrl ;
    PLAYBACKDATERES Playbackdateres ;
    PLAYBACKDATEREQ *Playbackdatereq ;
    int Year = 0, Month = 0, sendlen = 0;//, retval = 0 ;
    
#ifdef NETWORK_DEBUG
    DEBUG_PRI("playbackdatereq packet receive\n") ;
#endif
 
    Playbackdatereq = (PLAYBACKDATEREQ *)data ;

    Year = ntohs(Playbackdatereq->pYear) ;
    Month = ntohs(Playbackdatereq->pMonth) ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("playbackdatereq Month = %d\n",Month) ;
#endif

    Playbackdateres.identifier = htons(IDENTIFIER) ;
    Playbackdateres.cmd = htons(CMD_PLAYBACKDATE_RES) ;
    Playbackdateres.length = htons(37) ;

    strcpy(ParamCtrl.target_path, "/dvr/data/sdda") ;

    BKTRMP_GetRecDayData(&ParamCtrl, Year, Month, Playbackdateres.DAY) ;    


    sendlen = send(SystemInfo.C_Channel[channel], &Playbackdateres, 37, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("playbackdateres packet sendlen = %d\n",sendlen) ;
#endif


}

void playbacktimereq(int channel, char *data, int len)
{
    PLAYBACKTIMERES Playbacktimeres ;
    PLAYBACKTIMEREQ *Playbacktimereq ;

    T_BKTRMP_CTRL ParamCtrl ;
    T_BKTRMP_RECHOUR_PARAM Param ;

    int ListType = 0, sendlen = 0, retval = 0;//, StartYear = 0, StartMonth = 0, StartDay = 0, i ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("playbacktimereq packet receive.\n") ;
#endif

    Playbacktimereq = (PLAYBACKTIMEREQ *)data ;

    ListType = ntohs(Playbacktimereq->listType) ;

    Param.year = ntohs(Playbacktimereq->startYear) ;
    Param.mon = ntohs(Playbacktimereq->startMonth) ;
    Param.day = ntohs(Playbacktimereq->startDay) ;
	memset(Playbacktimeres.Min, 0, sizeof(Playbacktimeres.Min));
    Param.pRecHourTBL = Playbacktimeres.Min;
    strcpy(ParamCtrl.target_path, "/dvr/data/sdda") ;

    if(BKT_ERR == BKTRMP_GetRecHourDataUnion(&ParamCtrl, &Param))
    {
        retval = 0x00 ;
    }
    else
    {
        retval = 0x01 ;
    }

    Playbacktimeres.identifier = htons(IDENTIFIER) ;
    Playbacktimeres.cmd = htons(CMD_PLAYBACKTIME_RES) ;
    Playbacktimeres.length = htons(TOTAL_MINUTES_DAY + 10) ;
    Playbacktimeres.channelMask = htons(0xff) ;
    Playbacktimeres.result = htons(retval) ;

    sendlen = send(SystemInfo.C_Channel[channel], &Playbacktimeres, TOTAL_MINUTES_DAY + 10, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("playbacktimeres packet sendlen = %d\n",sendlen) ;
#endif

}

void multipbstartreq(int channel, char *data, int len)
{
    MULTIPBSTARTREQ *Multipbstartreq ;
    COMRESPACKET Multipbstartres ;
    int ConFlag = 0, InitMode = 0, InitPausePosition = 0 ,InitChMask = 0, DataType = 0, PlayRangeType = 0,
        RangeStartYear = 0, RangeStartMonth = 0, RangeStartDay = 0, sendlen = 0,
        RangeStartHour = 0, RangeStartMin = 0, RangeStartSec = 0, RangeEndYear = 0,
        RangeEndMonth = 0, RangeEndDay = 0, RangeEndHour = 0, RangeEndMin = 0, RangeEndSec = 0, retval = 0;

    struct tm tm_ptr;
    time_t the_time;

    long rectime ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbstartreq packet receive. channel = %d, len = %d\n",channel, len) ;
#endif

    Multipbstartreq = (MULTIPBSTARTREQ *)data ;

    ConFlag = ntohs(Multipbstartreq->ContinuousFlag) ;
    InitMode = ntohs(Multipbstartreq->InitialMode) ;
    InitPausePosition = ntohs(Multipbstartreq->InitialPausePosition) ;
    InitChMask = ntohs(Multipbstartreq->InitialChannelMask) ;
    DataType = ntohs(Multipbstartreq->DataType) ;
    PlayRangeType = ntohs(Multipbstartreq->PlayRangeType) ;
    RangeStartYear = ntohs(Multipbstartreq->RangeStartYear) ;
    RangeStartMonth = ntohs(Multipbstartreq->RangeStartMonth) ;
    RangeStartDay = ntohs(Multipbstartreq->RangeStartDay) ;
    RangeStartHour = ntohs(Multipbstartreq->RangeStartHour) ;
    RangeStartMin = ntohs(Multipbstartreq->RangeStartMin) ;
    RangeStartSec = ntohs(Multipbstartreq->RangeStartSec) ;
    RangeEndYear = ntohs(Multipbstartreq->RangeEndYear) ;
    RangeEndMonth = ntohs(Multipbstartreq->RangeEndMonth) ;
    RangeEndDay = ntohs(Multipbstartreq->RangeEndDay) ;
    RangeEndHour = ntohs(Multipbstartreq->RangeEndHour) ;
    RangeEndMin = ntohs(Multipbstartreq->RangeEndMin) ;
    RangeEndSec = ntohs(Multipbstartreq->RangeEndSec) ;

    SystemInfo.encodingtype[channel] = 2 ; // 0 vstream 1 astream 2 playback

#ifdef NETWORK_DEBUG
    DEBUG_PRI("year = %d\n",RangeStartYear) ;
    DEBUG_PRI("month = %d\n",RangeStartMonth) ;
    DEBUG_PRI("day = %d\n",RangeStartDay) ;
    DEBUG_PRI("hour = %d\n",RangeStartHour) ;
    DEBUG_PRI("min = %d\n",RangeStartMin) ;
    DEBUG_PRI("sec = %d\n",RangeStartSec) ;
#endif

    tm_ptr.tm_year  = RangeStartYear - 1900 ;
    tm_ptr.tm_mon  = RangeStartMonth - 1;
    tm_ptr.tm_mday = RangeStartDay ;
    tm_ptr.tm_hour = RangeStartHour ;
    tm_ptr.tm_min  = RangeStartMin ;
    tm_ptr.tm_sec  = RangeStartSec ;

    rectime = mktime(&tm_ptr) ;
    SystemInfo.videochannel[channel] = InitChMask ;
 
    Multipbstartres.identifier = htons(IDENTIFIER) ;
    Multipbstartres.cmd = htons(CMD_MULTI_PB_START_RES) ;
    Multipbstartres.length = htons(RESULT_SIZE) ;

    retval = PLAYBACK_status() ;
    printf("PLAYBACK_status retval = %d\n",retval) ;

    if(retval == 0)
    {
        Multipbstartres.result = htons(0x00) ;  
    }
    else
    {
        Multipbstartres.result = htons(0x01) ; // invalid
    }

    sendlen = send(SystemInfo.D_Channel[channel], &Multipbstartres, RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbstartres sendlen = %d\n",sendlen) ;
#endif

    REMOTEPLAY_start(InitChMask, rectime, channel) ;

}

void multipbstopreq(int channel, char *data, int len)
{
    COMRESPACKET Multipbstopres ;
    int sendlen = 0, retval = 0, chId = 0 ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbstopreq receive..\n") ;
#endif

    retval = PLAYBACK_stopCh(channel) ;

    Multipbstopres.identifier = htons(IDENTIFIER) ;
    Multipbstopres.cmd = htons(CMD_MULTI_PB_STOP_RES) ;
    Multipbstopres.length = htons(RESULT_SIZE) ;

    if(retval == OSA_SOK)
    {
        Multipbstopres.result = htons(0x00) ;
    }
    else
    {
        Multipbstopres.result = htons(0x01) ;
    }
       
    sendlen = send(SystemInfo.D_Channel[channel], &Multipbstopres, RESULT_SIZE, 0) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbstopres sendlen = %d\n",sendlen) ;
#endif

}

void multipbpausereq(int channel, char *data, int len)
{
    COMRESPACKET Pausepacketres ;
    int chId = 0, retval = 0, sendlen = 0 ;
 
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbpausereq receive..\n") ;
#endif
    retval = PLAYBACK_pauseCh(channel) ;  
    
    Pausepacketres.identifier = htons(IDENTIFIER);
    Pausepacketres.cmd = htons(CMD_MULTI_PB_PAUSE_RES) ;
    Pausepacketres.length = htons(RESULT_SIZE) ;

    if(retval == OSA_SOK)
    {
        Pausepacketres.result = htons(0x00) ;
    }
    else
    {
        Pausepacketres.result = htons(0x05) ;
    }

    sendlen = send(SystemInfo.D_Channel[channel], &Pausepacketres, RESULT_SIZE, 0 ) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbpausereq sendlen = %d\n",sendlen) ; 
#endif
       
}

void multipbplayfwdreq(int channel, char *data, int len)
{
      
    COMRESPACKET Pbplayfwdres ;

    int chId = 0, retval = 0, sendlen = 0 ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbplayfwdreq receive..\n") ;
#endif
    retval = PLAYBACK_streamCh(channel) ;

    Pbplayfwdres.identifier = htons(IDENTIFIER) ;
    Pbplayfwdres.cmd = htons(CMD_MULTI_PB_PLAY_FWD_RES) ;
    Pbplayfwdres.length = htons(RESULT_SIZE) ;

    if(retval == OSA_SOK)
    {
        Pbplayfwdres.result = htons(0x00) ;
    }
    else
    {
        Pbplayfwdres.result = htons(0x05) ;
    }
    sendlen = send(SystemInfo.D_Channel[channel], &Pbplayfwdres, RESULT_SIZE, 0 ) ;
#ifdef NETWORK_DEBUG
    DEBUG_PRI("multipbpausereq sendlen = %d\n",sendlen) ; 
#endif

}



