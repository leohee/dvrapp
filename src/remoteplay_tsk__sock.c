#include <sys/ioctl.h>
#include <errno.h>
#include <remoteplay_priv.h>
//#include <capture_priv.h>
#include <fstruct.h>
//#include <setparam.h>
//#include <capture.h>
#include <fdefine.h>
#include <fcommand.h>
#include <rawsockio.h>
#include <encrypt.h>

//#include <basket.h>
#include <basket_remoteplayback.h>
#include "basket_muldec.h"
#include "ti_media_common_def.h"
#include "app_writer.h"
REMOTEPLAY_Ctrl gREMOTEPLAY_ctrl;
extern SYSTEM_INFO SystemInfo ;
//CAPTURE_Ctrl gCAPTURE_ctrl;
static int Blockcnt = 0 ;
extern int  BKTMD_getTargetDisk(long sec);
extern T_BKTMD_CTRL gBKTMD_CtrlV;
int REMOTEPLAY_tskCreate()
{
  int status ;

  status = OSA_mutexCreate(&gREMOTEPLAY_ctrl.mutexLock);
  if(status!=OSA_SOK) {
    OSA_ERROR("OSA_mutexCreate()\n");
  }
  
  return OSA_SOK;
  
}

int REMOTEPLAY_tskDelete()
{
  int status=0;
  
  OSA_mutexDelete(&gREMOTEPLAY_ctrl.mutexLock);
  
  return status;
}


int REMOTEPLAY_tskRun()
{

    int status=OSA_SOK, channel, sockchannel, resolution, retval, hour, min, sec, pause = 0, i = 0, stop = 0, sendlen = 0, done = 0 ;
    unsigned long rectime, RecordedTimestamp[8] = {0,0,0,0,0,0,0,0}, RecordedOldTimestamp[8] = {0,0,0,0,0,0,0,0},RecordedTimegap = 0,Timestamp[8] = {0,0,0,0,0,0,0,0} , OldTimestamp[8] = {0,0,0,0,0,0,0,0}, Timegap = 0, gap = 0;
    char vbuffer[1024*100], basketOC[8] ;
    unsigned short abuffer[1024*2] ; 

    struct tm tm1;
    struct timeval tv ;

    T_BKTRMP_PARAM dec_param;
     VCODEC_BITSBUF_S *pFullBuf;
     char streamtype;

    memset(&dec_param, 0, sizeof(dec_param));

    rectime = gREMOTEPLAY_ctrl.rectime ;
    sockchannel = gREMOTEPLAY_ctrl.sockchannel ;
    channel = gREMOTEPLAY_ctrl.channel ;

    T_BKTRMP_CTRL rmpCtrl;
    strcpy(rmpCtrl.target_path, "/dvr/data/sdda");

    dec_param.vbuffer = vbuffer;
    dec_param.abuffer = abuffer;
    dec_param.ch = channel ;
	
	//BKTMD_getTargetDisk	(rectime);	
	//rmpCtrl.target_path = gBKTMD_CtrlV.target_path;//compile error
    if( BKT_ERR == BKTRMP_Open(&rmpCtrl, rectime))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("BASKET OPEN FAIL....\n") ;
#endif
        gREMOTEPLAY_ctrl.runflag = 0 ;
        gREMOTEPLAY_ctrl.stop = FALSE ;
        return OSA_EFAIL;
    }

    while(!done)
    {
        OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
        pause = gREMOTEPLAY_ctrl.pause ;
        stop =  gREMOTEPLAY_ctrl.stop ;
        OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

        if(stop == TRUE)
        {
            gREMOTEPLAY_ctrl.stop = FALSE ;
            gREMOTEPLAY_ctrl.runflag = 0 ;
            break ;
        }

        if(pause == TRUE)
        {
            usleep(1000) ;
            continue ;
        }
        else
        { 
            long fposcur = BKTRMP_ReadNextFrame(&rmpCtrl, &dec_param);
            if(fposcur == BKT_ERR)
            {
#ifdef NETWORK_DEBUG
                DEBUG_PRI("Remote Playback end........\n") ;
#endif
                gREMOTEPLAY_ctrl.runflag = 0 ;
                gREMOTEPLAY_ctrl.stop = FALSE ;
 
                status = -1 ;
                done = 1 ;
            }

		
	      gettimeofday(&tv, NULL) ;
            Timestamp[dec_param.ch] = tv.tv_sec*1000 + tv.tv_usec/1000 ;

	
            pFullBuf->timestamp = Timestamp[dec_param.ch];
            pFullBuf->filledBufSize = dec_param.framesize;
            pFullBuf->frameType = dec_param.frametype;
            pFullBuf->bufVirtAddr =  (dec_param.streamtype == ST_VIDEO) ? (dec_param.vbuffer) : (dec_param.abuffer);

	      streamtype = (dec_param.streamtype == ST_VIDEO) ? VIDEO_STREAM : AUDIO_STREAM;	
	      Stream_SendRun(streamtype, dec_param.ch, pFullBuf) ;
#if 0
            if(dec_param.streamtype == ST_VIDEO)
            {
               	if(dec_param.frameWidth == 704)
               	{
                    if(dec_param.frameHeight == 480)
                    {
                      	resolution = NTSC720480 ;
                    }
                    else
              	    {
                      	resolution = PAL720576 ;
              	    }
              	}
                else
               	{
              	    if(dec_param.frameHeight == 240)
                    {
                        resolution =  NTSC352240 ;
                    } 
                    else
              	    {
                        resolution = PAL360288 ;
                    }
                }

                if(SystemInfo.D_Channel[sockchannel] == 0)   // ?
                {
                    gREMOTEPLAY_ctrl.runflag = 0 ;
                    break ;  
                }

               	localtime_r(&dec_param.ts.sec, &tm1);

                RecordedTimestamp[dec_param.ch] = ((tm1.tm_hour*3600 + tm1.tm_min*60 + tm1.tm_sec)*1000 + dec_param.ts.usec/1000) ;

//                if(RecordedOldTimestamp[dec_param.ch] != 0 && dec_param.streamtype == ST_VIDEO)
                if(RecordedOldTimestamp[dec_param.ch] != 0)
                    RecordedTimegap = RecordedTimestamp[dec_param.ch] - RecordedOldTimestamp[dec_param.ch] ;

                RecordedOldTimestamp[dec_param.ch] = RecordedTimestamp[dec_param.ch] ;

                gettimeofday(&tv, NULL) ;
                Timestamp[dec_param.ch] = tv.tv_sec*1000 + tv.tv_usec/1000 ;
                if(OldTimestamp[dec_param.ch] != 0)
                    Timegap = Timestamp[dec_param.ch] - OldTimestamp[dec_param.ch] ;

                OldTimestamp[dec_param.ch] = Timestamp[dec_param.ch] ;
                   
                gap = RecordedTimegap - Timegap ;  
                if(gap < 0 || gap > 2000)
                   gap = 0 ;
//fprintf(stderr, "Timegap = %d, Recorded Timegap = %d\n",Timegap, RecordedTimegap) ;
                usleep(gap*1000) ;               

                retval = RemotePlayback(dec_param.ch, dec_param.streamtype, dec_param.frametype, vbuffer, dec_param.framesize, Timestamp[dec_param.ch], resolution) ;
            }
            else
            {
                retval = RemotePlayback(dec_param.ch, dec_param.streamtype, 0, abuffer, dec_param.framesize, Timestamp[dec_param.ch], 0) ;
            }
#endif
            if(retval == -1)
            {
                status = retval ;
                break ;
            }
        }

    }

    return status;
}

#if 0
int RemotePlayback(int channel, int streamtype, int vtype, void *buffer, int framesize, unsigned long Timestamp, int resolution) 
{
    PVSTREAMDATA PVstreamdata, PAstreamdata ;
    unsigned long offset = 0 ;
    int sendlen = 0, i = 0, j = 0, totalfrag = 0, lastfragsize = 0, framereadsize = 0, StreamToClient = 0, retval = TRUE; 

    char Streamtempbuff[MAXCHANNEL][MAXBUFFSIZE] ;
    char vp_SendBuffer[MAXCHANNEL][SENDBUFFSIZE] ;
    char ap_SendBuffer[MAXCHANNEL][NORMALBUFFSIZE*2] ;

    Intimeheader Intime ;

    if(streamtype == ST_VIDEO)
    {
        if(framesize != 0)
        {
            Intime.Ch = htons(0) ;
            Intime.Type = htons(vtype) ;
            Intime.FrameSize = htonl(framesize) ;

            if(vtype == IFRAME) // Iframe
            {
                totalfrag = ((framesize + 8)/PACKETSIZE) + 1 ; //
                lastfragsize = (framesize + 8) % PACKETSIZE ;

                memcpy(Streamtempbuff[channel], &Intime, 8) ;
                memcpy(&Streamtempbuff[channel][8], (char *)buffer, framesize) ;

                offset = 0 ;

                for(i = 0; i < totalfrag ; i++)
                { 
                    if(i != totalfrag -1)
                    {
                        framereadsize = PACKETSIZE ;
                    }
                    else
                    {
                        framereadsize = lastfragsize ;
                    }

                    PVstreamdata.identifier = htons(IDENTIFIER) ;
                    PVstreamdata.cmd = htons(CMD_SINGLE_PB_VIDEO_DATA) ;
                    PVstreamdata.length = htons(PVSTREAMDATA_SIZE + framereadsize) ;
                    PVstreamdata.FragmentNo = totalfrag;
                    PVstreamdata.FragmentIdx = i;
                    PVstreamdata.Timestamp = htonl(Timestamp) ;
                    PVstreamdata.channel = htons(channel) ;
                    PVstreamdata.resolution = htons(resolution) ;

                    memcpy(vp_SendBuffer[channel], &PVstreamdata, PVSTREAMDATA_SIZE) ;
                    memcpy(&vp_SendBuffer[channel][PVSTREAMDATA_SIZE], &Streamtempbuff[channel][offset], framereadsize) ;
                    for(j = 1; j < MAXUSER; j++)
                    {
//                        if(SystemInfo.D_Channel[j] != 0 && SystemInfo.encodingtype[j] == 2 && SystemInfo.videochannel[j] == channel )
                        if(SystemInfo.D_Channel[j] != 0 && SystemInfo.encodingtype[j] == 2 )
                        {
                            if(i == 0)
                            {
                                SystemInfo.Firstpacket[j] = 1;
                            }
                            if(SystemInfo.Firstpacket[j] == 1)
                            {
                                errno = 0 ;
                                sendlen = send(SystemInfo.D_Channel[j], vp_SendBuffer[channel], PVSTREAMDATA_SIZE + framereadsize ,0) ;
                                if(errno == EWOULDBLOCK)
                                {
                                    if(SystemInfo.D_Channel[j] != 0)
                                    {
                                        CloseNowChannel(j, DSOCKFLAG) ;
                                        SystemInfo.D_Channel[j] = 0 ;
                                    }
                                    if(SystemInfo.C_Channel[j] != 0)
                                    {
                                        CloseNowChannel(j, CSOCKFLAG) ;
                                        SystemInfo.C_Channel[j] = 0 ;
                                    }
                                    gREMOTEPLAY_ctrl.runflag = 0 ;
                                    retval = -1 ;
                                    break ;
                                }
                                if(errno == ECONNRESET )   // for timeout....
                                {
#ifdef NETWORK_DEBUG
                                    DEBUG_PRI("I frame ECONNRESET....channel = %d\n",j) ;
#endif

                                    if(SystemInfo.D_Channel[j] != 0)
                                    {
                                        CloseNowChannel(j, DSOCKFLAG) ;
                                        SystemInfo.D_Channel[j] = 0 ;
                                    }
                                    if(SystemInfo.C_Channel[j] != 0)
                                    {
                                        CloseNowChannel(j, CSOCKFLAG) ;
                                        SystemInfo.C_Channel[j] = 0 ;
                                    }
                                    gREMOTEPLAY_ctrl.runflag = 0 ;
                                    retval = -1 ;
                                    break ;
                                }
                                errno = 0 ;
                            }
                        }
                    }
                    memset(vp_SendBuffer[channel], SENDBUFFSIZE, 0);
                    offset += framereadsize ;
                }
                memset(Streamtempbuff[channel], MAXBUFFSIZE, 0) ;
            }
            else // PFRAME
            {
                totalfrag = ((framesize + 8)/PACKETSIZE) + 1 ;
                lastfragsize = (framesize + 8) % PACKETSIZE ;
                Intime.FrameSize = htonl(framesize) ;

                memcpy(Streamtempbuff[channel], &Intime, 8) ;
                memcpy(&Streamtempbuff[channel][8], (char *)buffer, framesize) ;
 
                offset = 0 ;
                for(i = 0; i < totalfrag; i++)
                {
                    if(i != totalfrag - 1)
                    {
                        framereadsize = PACKETSIZE ;
                    }
                    else
                    {
                        framereadsize = lastfragsize ;
                    }

                    PVstreamdata.identifier = htons(IDENTIFIER) ;
                    PVstreamdata.cmd = htons(CMD_SINGLE_PB_VIDEO_DATA) ;
                    PVstreamdata.length = htons(PVSTREAMDATA_SIZE + framereadsize) ;
                    PVstreamdata.FragmentNo = totalfrag ;
                    PVstreamdata.FragmentIdx = i;
                    PVstreamdata.Timestamp = htonl(Timestamp) ;
                    PVstreamdata.channel = htons(channel) ;
                    PVstreamdata.resolution = htons(resolution) ;

                    memcpy(vp_SendBuffer[channel], &PVstreamdata, PVSTREAMDATA_SIZE) ;
                    memcpy(&vp_SendBuffer[channel][PVSTREAMDATA_SIZE], &Streamtempbuff[channel][offset], framereadsize) ;
                    for(j = 1 ; j < MAXUSER ; j++)
                    {
//                        if(SystemInfo.D_Channel[j] != 0 && SystemInfo.Firstpacket[j] == 1 && SystemInfo.encodingtype[j] == 2 && SystemInfo.videochannel[j] == channel )
                        if(SystemInfo.D_Channel[j] != 0 && SystemInfo.Firstpacket[j] == 1 && SystemInfo.encodingtype[j] == 2)
                        {
                            errno = 0 ;
                            sendlen = send(SystemInfo.D_Channel[j], vp_SendBuffer[channel], PVSTREAMDATA_SIZE + framereadsize ,0) ;
                            if(errno == EWOULDBLOCK)
                            {
                                SystemInfo.Firstpacket[j] = 0 ;
                                if(SystemInfo.D_Channel[j] != 0)
                                {
                                    CloseNowChannel(j, DSOCKFLAG) ;
                                    SystemInfo.D_Channel[j] = 0 ;
                                }
                                if(SystemInfo.C_Channel[j] != 0)
                                {
                                    CloseNowChannel(j, CSOCKFLAG) ;
                                    SystemInfo.C_Channel[j] = 0 ;
                                }
                                gREMOTEPLAY_ctrl.runflag = 0 ;
                                retval = -1 ;
                                break ;
                            }
                            if(errno == ECONNRESET )
                            {
#ifdef NETWORK_DEBUG
                                DEBUG_PRI("P frame ECONNRESET.... channel = %d\n",j) ;
#endif
                                if(SystemInfo.D_Channel[j] != 0)
                                {
                                    CloseNowChannel(j, DSOCKFLAG) ;
                                    SystemInfo.D_Channel[j] = 0 ;
                                }
                                if(SystemInfo.C_Channel[j] != 0)
                                {
                                    CloseNowChannel(j, CSOCKFLAG) ;
                                    SystemInfo.C_Channel[j] = 0 ;
                                }
                                gREMOTEPLAY_ctrl.runflag = 0 ;
                                retval = -1 ;
                                break ;
                            }
                        }
                        
                    }
                    memset(vp_SendBuffer[channel], SENDBUFFSIZE, 0) ;
                    offset += framereadsize ;
                }
                memset(Streamtempbuff[channel], MAXBUFFSIZE, 0) ;
            }
        }
    }
    else
    {
        if(framesize != 0)
        {
            framereadsize = 1024 ;
         
            PAstreamdata.identifier = htons(IDENTIFIER) ;
            PAstreamdata.cmd = htons(CMD_SINGLE_PB_AUDIO_DATA) ;
            PAstreamdata.length = htons(PVSTREAMDATA_SIZE + framereadsize) ;
            PAstreamdata.FragmentNo = 1;
            PAstreamdata.FragmentIdx = 0;
            PAstreamdata.Timestamp = htonl(Timestamp) ;
            PAstreamdata.channel = htons(channel) ;
            PAstreamdata.resolution = htons(0x01) ; //audio type u_law PCM

            memcpy(ap_SendBuffer[channel], &PAstreamdata, PVSTREAMDATA_SIZE) ;
            memcpy(&ap_SendBuffer[channel][PVSTREAMDATA_SIZE], (char *)buffer, framereadsize) ;

            for(j = 1 ; j < MAXUSER ; j++)
            {
//                if(SystemInfo.D_Channel[j] != 0 && SystemInfo.headerrecv[j] == 1 && SystemInfo.encodingtype[j] == 1  && SystemInfo.videochannel[j] == channel )
                if(SystemInfo.D_Channel[j] != 0 && SystemInfo.encodingtype[j] == 2 )
                {
                   // if(i == 0)
                   // {
                        SystemInfo.Firstpacket[j] = 1 ;
                   // }
                }
                if(SystemInfo.Firstpacket[j] != 0 )
                {
                    errno = 0 ;
                    sendlen = send(SystemInfo.D_Channel[j], ap_SendBuffer[channel], PVSTREAMDATA_SIZE + framereadsize ,0) ;
                    if(errno == EWOULDBLOCK)
                    {
                        SystemInfo.Firstpacket[j] = 0 ;
                        if(SystemInfo.D_Channel[j] != 0)
                        {
                            CloseNowChannel(j, DSOCKFLAG) ;
                            SystemInfo.D_Channel[j] = 0 ;
                        }
                        if(SystemInfo.C_Channel[j] != 0)
                        {
                            CloseNowChannel(j, CSOCKFLAG) ;
                            SystemInfo.C_Channel[j] = 0 ;
                        }
                        gREMOTEPLAY_ctrl.runflag = 0 ;
                        retval = -1 ;
                        break ; 
                    }
                    if(errno == ECONNRESET )   // for timeout....
                    {
                        if(SystemInfo.D_Channel[j] != 0)
                        {
                            CloseNowChannel(j, DSOCKFLAG) ;
                            SystemInfo.D_Channel[j] = 0 ;
                        }
                        if(SystemInfo.C_Channel[j] != 0)
                        {
                            CloseNowChannel(j, CSOCKFLAG) ;
                            SystemInfo.C_Channel[j] = 0 ;
                        }
                        gREMOTEPLAY_ctrl.runflag = 0 ;
                        retval = -1 ;
                        break ;
                    }
                    errno = 0 ;
                }
            }
            memset(ap_SendBuffer[channel], NORMALBUFFSIZE*2, 0) ;
            offset += framereadsize ;
        }
    }
    return retval ;
}
#endif 


int REMOTEPLAY_tskMain(struct OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg, Uint32 curState )
{
	int status;
	Bool done = FALSE, ackMsg=FALSE;;
	Uint16 cmd = OSA_msgGetCmd(pMsg);

	if( cmd != REMOTEPLAY_CMD_CREATE) {
		OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
		return OSA_SOK;
	}
	
	status = REMOTEPLAY_tskCreate();
	
	OSA_tskAckOrFreeMsg(pMsg, status);  
	
	if(status != OSA_SOK)
		return OSA_SOK;
	
	while(!done)
	{
		status = OSA_tskWaitMsg(pTsk, &pMsg);
		
		if(status != OSA_SOK) {
			done = TRUE;
			break;
		}
		
		cmd = OSA_msgGetCmd(pMsg);  
		
		switch(cmd) {

		case REMOTEPLAY_CMD_RUN:
			
			OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);      
			
			REMOTEPLAY_tskRun();      
			
			break;
			
		case REMOTEPLAY_CMD_DELETE:
			done = TRUE;        
			ackMsg = TRUE;
			break;   
			
		default:   
			
			OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);
			
			break;              
		}
	}
	
	REMOTEPLAY_tskDelete();
	if(ackMsg)
		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);          
	
	return OSA_SOK;
}





int REMOTEPLAY_sendCmd(Uint16 cmd, void *prm, Uint16 flags )
{
  return OSA_mbxSendMsg(&gREMOTEPLAY_ctrl.tskHndl.mbxHndl, &gREMOTEPLAY_ctrl.mbxHndl, cmd, prm, flags);
}

int REMOTEPLAY_create()
{
  int status;
 
  memset(&gREMOTEPLAY_ctrl, 0, sizeof(gREMOTEPLAY_ctrl));
   
  status = OSA_tskCreate( &gREMOTEPLAY_ctrl.tskHndl, REMOTEPLAY_tskMain, REMOTEPLAY_THR_PRI, REMOTEPLAY_STACK_SIZE, 0,&gREMOTEPLAY_ctrl);

  OSA_assertSuccess(status);
    
  status = OSA_mbxCreate( &gREMOTEPLAY_ctrl.mbxHndl);  
  
  OSA_assertSuccess(status);  

  status = REMOTEPLAY_sendCmd(REMOTEPLAY_CMD_CREATE, NULL, OSA_MBX_WAIT_ACK );
  
  return status;  
}

int REMOTEPLAY_delete()
{
  int status;
  
  status = REMOTEPLAY_sendCmd(REMOTEPLAY_CMD_DELETE, NULL, OSA_MBX_WAIT_ACK );
  
  status = OSA_tskDelete( &gREMOTEPLAY_ctrl.tskHndl );

  OSA_assertSuccess(status);
    
  status = OSA_mbxDelete( &gREMOTEPLAY_ctrl.mbxHndl);  
  
  OSA_assertSuccess(status);  
  
  return status;
}

int REMOTEPLAY_start(int channel, unsigned long rectime, int sockchannel)
{
    int disable = FALSE ;
    int runflag = 0;

    OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
    runflag = gREMOTEPLAY_ctrl.runflag;
    OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

    if(runflag == 0)
    {
        OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
        gREMOTEPLAY_ctrl.channel = channel ;
        gREMOTEPLAY_ctrl.rectime = rectime ;
        gREMOTEPLAY_ctrl.sockchannel = sockchannel ;
        gREMOTEPLAY_ctrl.runflag = 1 ;
        gREMOTEPLAY_ctrl.stop = disable ;
        OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

        return REMOTEPLAY_sendCmd(REMOTEPLAY_CMD_RUN, NULL, 0 );
    }
    else
    {
        return 0 ;
    }

}

int PLAYBACK_pauseCh(int sockch)
{
    int enable = TRUE, runflag = 0, sockchannel = 0 ;
  
    OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
    runflag = gREMOTEPLAY_ctrl.runflag;
    sockchannel = gREMOTEPLAY_ctrl.sockchannel ;
    OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

    if(runflag == 1 && sockchannel == sockch)
    {
        OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
        gREMOTEPLAY_ctrl.pause = enable ;
        OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);
    }
    return OSA_SOK;
}

int PLAYBACK_streamCh(int sockch)
{
    int disable = FALSE ;
    
    OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
    gREMOTEPLAY_ctrl.pause = disable ;
    OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

    return OSA_SOK;
}

int PLAYBACK_stopCh(int sockch)
{
    int enable = TRUE, runflag = 0, sockchannel = 0 ;
 
   
    OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
    runflag = gREMOTEPLAY_ctrl.runflag;
    sockchannel = gREMOTEPLAY_ctrl.sockchannel ;
    OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

    gREMOTEPLAY_ctrl.runflag = 0;
    if(runflag == 1 && sockchannel == sockch)
    {
        OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
        gREMOTEPLAY_ctrl.stop = enable ;
        gREMOTEPLAY_ctrl.runflag = 0;
        OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);
    }

    return OSA_SOK ;
}

int PLAYBACK_status()
{
    int runflag = 0;

    OSA_mutexLock(&gREMOTEPLAY_ctrl.mutexLock);
    runflag = gREMOTEPLAY_ctrl.runflag;
    OSA_mutexUnlock(&gREMOTEPLAY_ctrl.mutexLock);

    return runflag ;
}


	
