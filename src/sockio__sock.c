#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include <fdefine.h>
#include <fstruct.h>
#include <message.h>
#include <cmessage.h>
#include <dmessage.h>
#include <msghandler.h>
#include <sockio.h>
#include <rawsockio.h>
//#include <systemsignal.h>
#include <osa.h>
//#include <setparam.h>
#include <fcommand.h>

NETSETTINGCHANGE Netsettingchange ;
//extern SETPARAM_Info gSETPARAM_info;
extern SYSTEM_INFO SystemInfo ;
//VSTREAMERROR Vstreamerror ;
#define MSG_TRANS(msg)	(((msg)>>4) & 0xf00) + ((msg) & 0xff)
char m_SendBuffer[HALFBUFFSIZE] ;

void Lmsg_process(MessageManager* manager)
{
    Message* msg = NULL ;
    int i = 0 ;
    unsigned short cmd  = 0 ;

    for (i=0; i<manager->cnt; i++)
    {
        msg = get_message(manager, i) ;
        cmd = htons((msg->buff[0]&0xff) | ((msg->buff[1]<<8)&0xff00)) ;
//      cmd = msg->buff[1] ;
#ifdef NETWORK_DEBUG
      DEBUG_PRI("cmd = 0x%x,0x%x\n", cmd,MSG_TRANS(cmd)) ;
#endif
        if (msgfunc[MSG_TRANS(cmd)])
        {
            (*msgfunc[MSG_TRANS(cmd)])(msg->channel, msg->buff+4, msg->len-4) ;
        }

    }
    zero_message_manager(manager) ;
}

void Cmsg_process(CMessageManager* Cmanager)
{
    CMessage* Cmsg = NULL ;
    int i = 0 ;
    unsigned short cmd  = 0 ;

    for (i=0; i<Cmanager->cnt; i++)
    {
        Cmsg = Cget_message(Cmanager, i) ;
        cmd = htons((Cmsg->buff[0]&0xff) | ((Cmsg->buff[1]<<8)&0xff00)) ;
#ifdef NETWORK_DEBUG
          DEBUG_PRI("cmd = 0x%x,0x%x\n", cmd,MSG_TRANS(cmd)) ;
#endif
        if (msgfunc[MSG_TRANS(cmd)])
        {
            (*msgfunc[MSG_TRANS(cmd)])(Cmsg->channel, Cmsg->buff+4, Cmsg->len-4) ;
        }
    }
    Czero_message_manager(Cmanager) ;
}

void Dmsg_process(DMessageManager* Dmanager)
{
    DMessage* Dmsg = NULL ;
    int i = 0 ;
    unsigned short cmd  = 0 ;

    for (i=0; i<Dmanager->cnt; i++)
    {
        Dmsg = Dget_message(Dmanager, i) ;
        cmd = htons((Dmsg->buff[0]&0xff) | ((Dmsg->buff[1]<<8)&0xff00)) ;
#ifdef NETWORK_DEBUG
       DEBUG_PRI("cmd = 0x%x,0x%x\n", cmd,MSG_TRANS(cmd)) ;
#endif
        if (msgfunc[MSG_TRANS(cmd)])
        {
            (*msgfunc[MSG_TRANS(cmd)])(Dmsg->channel, Dmsg->buff+4, Dmsg->len-4) ;
        }
    }
    Dzero_message_manager(Dmanager) ;
}

void Lrefresh_message()
{
    Lmsg_process(gMessageManager) ;
}

void Crefresh_message()
{
    Cmsg_process(CgMessageManager) ;
}

void Drefresh_message()
{
    Dmsg_process(DgMessageManager) ;
}

void TcpServerInit()
{
    int retvalue = 0;

    //SetupSignalRoutine();
    msginit () ;
    retvalue = InitVar ();
#ifdef NETWORK_DEBUG
    DEBUG_PRI("TcpServerInit............\n") ;
#endif
}

void *Lsock(void)
{
    int LogonMainsock = 0 ;
    int year = 2010 ;
    int month = 3 ;
    int day = 2 ;


#ifdef NETWORK_DEBUG
    DEBUG_PRI("FIRMWARE VERSION %d.%d%c %d.%d.%d\n",VERSIONMAJOR, VERSIONMINOR, VERSIONTEXT, year, month, day) ;
    DEBUG_PRI("logon thread... run...\n") ;
#endif

    gMessageManager = init_message_manager() ;
    zero_message_manager(gMessageManager) ;
    LogonMainsock = MainSocketListen(LSOCKFLAG) ;
    while(TRUE)
    {
/*
        if(Netsettingchange.setting == 1)
        {
            Netsettingchange.setting = 2 ;
#ifdef NETWORK_DEBUG
       	DEBUG_PRI("EXIT Lsock !!!!!!!!!!!! \n");
#endif
            close(LogonMainsock) ;
            pthread_exit(NULL) ;
            
        }*/
        usleep(SOCKWAIT) ;
        ProcessSocket(LogonMainsock, LSOCKFLAG) ;  // select
        Lrefresh_message() ;
    }
}

void *Csock(void)
{
    int ControlMainsock = 0, chLockStatus[MAXCHANNEL], chNoSignal = 0, count = 0,  i = 0, j = 0, sendlen = 0;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("control thread... run...\n") ;
#endif
    CgMessageManager = Cinit_message_manager() ;
    Czero_message_manager(CgMessageManager) ;
    ControlMainsock = MainSocketListen(CSOCKFLAG) ;

    while(TRUE)
    {
    #if 0
        if(count > 1000)
        {
            count = 0 ;
      //    status = DIO_getNoSignalStatus(chLockStatus);
		chNoSignal = DVR_GetVideoLossStatus();
   		for(i = 0; i < MAXCHANNEL; i++)
   		{
   			if(chNoSignal & (1<<i))
   			{
            			SystemInfo.Nosignal[i] = TRUE ;
        		}
        		else
        		{
            			SystemInfo.Nosignal[i] = FALSE ;
        		}
   		}
       /*     if(status == 0)
            {
                for(i = 0; i < MAXCHANNEL; i++)
                {
                    if(chLockStatus[i] == 0)
     		        SystemInfo.Nosignal[i] = TRUE ;
                    else
                        SystemInfo.Nosignal[i] = FALSE ;
                }
            }*/
        }   
        count++ ;
#endif
	
  /*      if(Netsettingchange.setting == 2)
        {
            Netsettingchange.setting = 3 ;
#ifdef NETWORK_DEBUG
       	DEBUG_PRI("EXIT Csock !!!!!!!!!!!! \n");
#endif
            close(ControlMainsock) ;
            pthread_exit(NULL) ;
        }*/
        usleep(SOCKWAIT) ;
        ProcessSocket(ControlMainsock, CSOCKFLAG);
        Crefresh_message() ;
    }
}

void *Dsock(void)
{
    int DataMainsock = 0 ;
    char *boot=NULL ;

#ifdef NETWORK_DEBUG
    DEBUG_PRI("data thread... run...\n") ;
#endif

    DgMessageManager = Dinit_message_manager() ;
    Dzero_message_manager(DgMessageManager) ;
    DataMainsock = MainSocketListen(DSOCKFLAG) ;

    while(TRUE)
    {
        if(Netsettingchange.setting == 3)
        {
            Netsettingchange.setting = 4 ;
#ifdef NETWORK_DEBUG
       	DEBUG_PRI("EXIT Dsock !!!!!!!!!!!! \n");
#endif
            close(DataMainsock) ;
            pthread_exit(NULL) ;
        }

        usleep(1000);//(SOCKWAIT) ;
        ProcessSocket(DataMainsock, DSOCKFLAG) ;
        Drefresh_message() ;
    }

}

void *Ipset(void)
{
#ifdef NETWORK_DEBUG
    DEBUG_PRI("localcast & ip setting...thread...run\n") ;
#endif
    broadreceive() ;
    return(0) ;
}

