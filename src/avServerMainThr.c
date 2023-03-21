#include <avserver.h>
#include <audio.h>

//#include <osa_cmem.h>

int AVSERVER_tskStart(OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg)
{
  int status;
  
  #ifdef AVSERVER_DEBUG_MAIN_THR
  OSA_printf(" AVSERVER MAIN: AVSERVER_tskStart() ...\n");
  #endif
  
  status = AVSERVER_tskConnectInit();

  if(status!=OSA_SOK) 
    return status;

  status = AVSERVER_bufAlloc();
  
  if(status!=OSA_SOK) {
    OSA_ERROR("AVSERVER_bufAlloc()\n");
    return status;
  }

  VIDEO_streamSysInit();
	
  OSA_tskSetState(pTsk, AVSERVER_MAIN_STATE_RUNNING);        

  #ifdef AVSERVER_DEBUG_MAIN_THR
  OSA_printf(" AVSERVER MAIN: Start DONE\n");
  #endif

  return status;
}

int AVSERVER_tskStop(OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg)
{
  int status=OSA_SOK;

  status = AVSERVER_tskConnectExit();
	VIDEO_streamSysExit();
	
  OSA_tskSetState(pTsk, AVSERVER_MAIN_STATE_IDLE);        

  if(status!=OSA_SOK) {
    OSA_ERROR("\n");
    return status;
  }

  #ifdef AVSERVER_DEBUG_MAIN_THR
  OSA_printf(" AVSERVER MAIN: Stop...DONE\n");
  #endif

  return status;
}

int AVSERVER_tskRun(int streamType, int streamId)
{
  	int status=OSA_EFAIL, inBufId, inAudBufId;
  	OSA_BufInfo *pInBufInfo, *pAudInBufInfo;
  

#ifdef AVSERVER_DEBUG_VIDEO_STREAM_THR
      	OSA_printf(" STREAM: Stream %d InBuf %d, Frame size %d bytes, Key frame %d\n", streamId, inBufId, pInBufInfo->size, pInBufInfo->isKeyFrame);
#endif 
 
  	if(streamType == AUDIO_TSK_ENCODE)
	{
  		pAudInBufInfo = AVSERVER_AudbufGetFull( AUDIO_TSK_STREAM, streamId, &inAudBufId, OSA_TIMEOUT_FOREVER);
//  		OSA_assert(pAudInBufInfo!=NULL);

  		if(pAudInBufInfo!=NULL) 
		{
			if(streamType == AUDIO_TSK_ENCODE)
			{ 
    			if(pAudInBufInfo->size > 0) 
				{
//					printf("AUDIO_streamShmCopy.......reached. pAudInBufInfo->Size = %d\n",pAudInBufInfo->size) ; 
	    			AUDIO_streamShmCopy(streamId, pAudInBufInfo) ;
    				AVSERVER_AudbufPutEmpty( AUDIO_TSK_STREAM, streamId, inAudBufId);  
				}
			}

		}
//    	AVSERVER_AudbufPutEmpty( AUDIO_TSK_STREAM, streamId, inAudBufId);  

	}
  	else
	{
		pInBufInfo = AVSERVER_bufGetFull( VIDEO_TSK_STREAM, streamId, &inBufId, OSA_TIMEOUT_FOREVER);
//  		OSA_assert(pInBufInfo!=NULL);

  		if(pInBufInfo!=NULL) 
		{
    		if(pInBufInfo->size > 0) 
       		VIDEO_streamShmCopy(streamId, pInBufInfo);

    		AVSERVER_bufPutEmpty( VIDEO_TSK_STREAM, streamId, inBufId);  
		}
//    	AVSERVER_bufPutEmpty( VIDEO_TSK_STREAM, streamId, inBufId);  

  	}
  	return status;
}

int AVSERVER_tskMain(struct OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg, Uint32 curState )
{
  int status=OSA_SOK, streamId, streamType;
  Bool done=FALSE, ackMsg = FALSE;
  Uint16 cmd = OSA_msgGetCmd(pMsg);
  
  AVSERVER_prm *av_prm ;

  #ifdef AVSERVER_DEBUG_MAIN_THR
  OSA_printf(" AVSERVER MAIN: Recevied CMD = 0x%04x, state = 0x%04x\n", cmd, curState);
  #endif
  
    if(cmd!=AVSERVER_MAIN_CMD_START) {
    	OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);
    	return OSA_SOK;
  	}

	if(curState==AVSERVER_MAIN_STATE_IDLE) {
		status = AVSERVER_tskStart(pTsk, pMsg);
	}
	
	if(status!=OSA_SOK) {
  		AVSERVER_tskStop(pTsk, pMsg);
  		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);
  		return status;
  	}
  	
	OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);	
	
	while(!done){
		
		status = OSA_tskWaitMsg(pTsk, &pMsg);
			
		if(status!=OSA_SOK) 
    		break;
    		
		cmd = OSA_msgGetCmd(pMsg);
		
		switch(cmd) {    		

      		case AVSERVER_CMD_NEW_VDATA:
        		streamId = (int)OSA_msgGetPrm(pMsg);
	
        
        		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);



        		AVSERVER_tskRun(VIDEO_TSK_ENCODE, streamId);		  	
	       		break;
			
			case AVSERVER_CMD_NEW_ADATA:

        		streamId = (int)OSA_msgGetPrm(pMsg);
	
        
        		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);



        		AVSERVER_tskRun(AUDIO_TSK_ENCODE, streamId);		  	
	       		break;
        		
			case AVSERVER_MAIN_CMD_STOP:
		  		  
		        done = TRUE;
        		ackMsg = TRUE;

		  		break;
		}		
	}
	
	if(curState==AVSERVER_MAIN_STATE_RUNNING) {

		AVSERVER_tskStop(pTsk, pMsg);
	}
	
	if(ackMsg)
		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);

  return OSA_SOK;
}

int AVSERVER_mainCreate()
{
  int status;
  
//  CMEM_init();
  
  status = OSA_mbxCreate(&gAVSERVER_ctrl.uiMbx);
  if(status!=OSA_SOK) {
    OSA_ERROR("OSA_mbxCreate()\n");
    return status;
  }
  
  status = OSA_tskCreate( &gAVSERVER_ctrl.mainTsk, AVSERVER_tskMain, AVSERVER_MAIN_THR_PRI, AVSERVER_MAIN_STACK_SIZE, AVSERVER_MAIN_STATE_IDLE, &gAVSERVER_ctrl);
  if(status!=OSA_SOK) {
    OSA_ERROR("OSA_tskCreate()\n");
    return status;
  }
  
  return status;
}

int AVSERVER_mainDelete()
{
  int status;
  
//  CMEM_exit();
  
  status = OSA_tskDelete(&gAVSERVER_ctrl.mainTsk);
  status |= OSA_mbxDelete(&gAVSERVER_ctrl.uiMbx);
  
  if(status!=OSA_SOK)
    OSA_ERROR("\n");
  
  return status;
}
