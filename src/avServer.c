
#include <avserver.h>
#include <osa_buf.h>
//#include <osa_cmem.h>

AVSERVER_Config gAVSERVER_config;
VIDEO_Ctrl      gVIDEO_ctrl ;
AUDIO_Ctrl      gAUDIO_ctrl ;

int AVSERVER_bufAlloc()
{
  	int status=OSA_SOK;

  	OSA_BufHndl *pBufHndl, *pAudBufHndl;
  	OSA_BufInfo *pBufInfo, *pAudBufInfo ;

//	AUDIO_BufInfo *pAudBufInfo ;
  	OSA_BufCreate *pBufCreatePrm, *pAudCreatePrm;
  	int bufId, i, k;
  	VIDEO_EncodeStream *pEncodeStream ;
	AUDIO_EncodeStream *pAudEncodeStream ;	

  	Uint32 maxbufSize = 0;
  	Uint32 bufSize;

#ifdef AVSERVER_DEBUG_MAIN_THR
  	OSA_printf(" AVSERVER MAIN: Allocating buffers channels[%d] ...\n",gAVSERVER_config.numEncodeStream);
#endif

  	for(i=0; i<gAVSERVER_config.numEncodeStream; i++) // 4
  	{
    	pEncodeStream = &gVIDEO_ctrl.encodeStream[i];
  
    	bufSize = gAVSERVER_config.encodeConfig[i].cropWidth*gAVSERVER_config.encodeConfig[i].cropHeight;//704*576
    
    	switch(gAVSERVER_config.encodeConfig[i].codecType) 
		{
      		default:
      		case ALG_VID_CODEC_MJPEG:
        		bufSize /= 1;
        		break;
        
      		case ALG_VID_CODEC_H264:
      		case ALG_VID_CODEC_MPEG4:
        		bufSize /= 1;      
        	break;
    	}
    
    	bufSize += VIDEO_BUF_HEADER_SIZE;
    
   		bufSize = OSA_align(bufSize, KB);
  
    	pBufHndl      = &pEncodeStream->bufStreamIn;
    	pBufCreatePrm = &pEncodeStream->bufStreamInCreatePrm;    

#ifdef AVSERVER_DEBUG_MAIN_THR
    	OSA_printf(" AVSERVER MAIN: Stream %d: Allocating Stream buffers[%x]: %d of size %d bytes\n", i, pBufHndl, pBufCreatePrm->numBuf, bufSize);
#endif
    
    	for(k=0; k<pBufCreatePrm->numBuf; k++)  // 3
		{
      		pBufCreatePrm->bufVirtAddr[k] = OSA_memAlloc(bufSize);
      
      		if(pBufCreatePrm->bufVirtAddr[k]==NULL/*||pBufCreatePrm->bufPhysAddr[k]==NULL*/) 
	  		{
          		OSA_ERROR("OSA_cmemAlloc()\n");      
          		return OSA_EFAIL;
      		}
    	}
    
    	if(pBufCreatePrm->numBuf)
      		status = OSA_bufCreate(pBufHndl, pBufCreatePrm);
  	
		if(status!=OSA_SOK)  
    		OSA_ERROR("OSA_bufCreate()\n");  
// audio
	
		pAudEncodeStream = &gAUDIO_ctrl.encodeStream[i];

		bufSize = gAVSERVER_config.audioConfig.samplingRate*100 ; 

		bufSize = OSA_align(bufSize, KB) ;

		pAudBufHndl		= &pAudEncodeStream->bufHndl ;
		pAudCreatePrm	= &pAudEncodeStream->CreatePrm ;

		for(k=0; k<pAudCreatePrm->numBuf; k++)  // 3
        {
            pAudCreatePrm->bufVirtAddr[k] = OSA_memAlloc(bufSize);

            if(pAudCreatePrm->bufVirtAddr[k]==NULL/*||pAudCreatePrm->bufPhysAddr[k]==NULL*/)
            {
                OSA_ERROR("OSA_cmemAlloc()\n");
                return OSA_EFAIL;
            }
        }
    	if(pAudCreatePrm->numBuf)
      		status = OSA_bufCreate(pAudBufHndl, pAudCreatePrm);

  		if(status!=OSA_SOK)  
    		OSA_ERROR("OSA_bufCreate()\n");  
  	}  


#ifdef AVSERVER_DEBUG_MAIN_THR
  	OSA_printf(" AVSERVER MAIN: Allocating buffers ...DONE\n");
#endif
    
  	return status;
}

int AVSERVER_bufFree()
{
  	int i, k, status=OSA_SOK;
  	VIDEO_EncodeStream *pEncodeStream;
  	AUDIO_EncodeStream *pAudEncodeStream ;

  	for(i=0; i<gAVSERVER_config.numEncodeStream; i++) 
	{
    	pEncodeStream = &gVIDEO_ctrl.encodeStream[i];
    	pAudEncodeStream = &gAUDIO_ctrl.encodeStream[i];
  
    	for(k=0; k<pEncodeStream->bufStreamInCreatePrm.numBuf; k++)
		{
      		OSA_memFree(pEncodeStream->bufStreamInCreatePrm.bufVirtAddr[k]);
      		OSA_memFree(pAudEncodeStream->CreatePrm.bufVirtAddr[k]);
		}
    	if(pEncodeStream->bufStreamInCreatePrm.numBuf)
      		OSA_bufDelete(&pEncodeStream->bufStreamIn);
		if(pAudEncodeStream->CreatePrm.numBuf)
      		OSA_bufDelete(&pAudEncodeStream->bufHndl);
  	}  

  	return status;
}


int AVSERVER_tskConnectInit()
{
  	int i, status;
  	VIDEO_EncodeStream *pEncodeStream;
  	AUDIO_EncodeStream *pAudEncodeStream ;
  

  	for(i=0; i<gAVSERVER_config.numEncodeStream; i++) 
	{
    	pEncodeStream = &gVIDEO_ctrl.encodeStream[i];
    	pAudEncodeStream = &gAUDIO_ctrl.encodeStream[i];
    
    	pEncodeStream->bufStreamInCreatePrm.numBuf = VIDEO_NUM_BUF;      
    	pAudEncodeStream->CreatePrm.numBuf = VIDEO_NUM_BUF;    // 3  
  	}

  	status = AVSERVER_tskConnectReset();

  	return status;
}

int AVSERVER_tskConnectExit()
{
  int status;
  
  status = AVSERVER_bufFree();
  
  return status;
}


int AVSERVER_tskConnectReset()
{
  	int i, status=OSA_SOK;

  	VIDEO_EncodeStream *pEncodeStream;
  	AUDIO_EncodeStream *pAudEncodeStream ;

  	VIDEO_EncodeConfig *pEncodeConfig;  
    
  	for(i=0; i<gAVSERVER_config.numEncodeStream; i++) 
	{
    	pEncodeStream = &gVIDEO_ctrl.encodeStream[i];
    	pAudEncodeStream = &gAUDIO_ctrl.encodeStream[i];
    	pEncodeConfig = &gAVSERVER_config.encodeConfig[i];    
    
    	pEncodeStream->encodeNextTsk = VIDEO_TSK_STREAM;
    	pAudEncodeStream->encodeNextTsk = AUDIO_TSK_STREAM;
  	}

  	return status;
}

// get hndl....  
int AVSERVER_bufGetNextTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl, OSA_TskHndl **pTskHndl)
{
  	VIDEO_EncodeStream  *pEncodeStream=NULL;

  	int nextTskId;
  
  	*pTskHndl=NULL;
  	*pBufHndl=NULL;
 
  		if(streamId<0 || streamId >= AVSERVER_MAX_STREAMS) 
		{
    		OSA_ERROR("Incorrect streamId (%d)\n", streamId);  
    		return OSA_EFAIL;
  		}

  		if(tskId <=VIDEO_TSK_NONE || tskId >= VIDEO_TSK_STREAM) 
		{
    		OSA_ERROR("Incorrect tskId (%d)\n", tskId);    
    		return OSA_EFAIL;
  		}


    	pEncodeStream  = &gVIDEO_ctrl.encodeStream[streamId];

    	nextTskId = VIDEO_TSK_NONE;
    
    	if(tskId==VIDEO_TSK_ENCODE) 
		{
    		nextTskId = pEncodeStream->encodeNextTsk;
	
    	} 
		else if(tskId==VIDEO_TSK_ENCRYPT) 
		{    
      		nextTskId = VIDEO_TSK_STREAM;
    	}	 
    
    	if(nextTskId==VIDEO_TSK_NONE) 
		{
    		OSA_ERROR("Incorrect nextTskId (%d)\n", nextTskId);          
      		return OSA_EFAIL;
    	}
    
    	if(nextTskId==VIDEO_TSK_STREAM) 
		{    
      		*pBufHndl = &pEncodeStream->bufStreamIn;
      		*pTskHndl = &gAVSERVER_ctrl.mainTsk;//gVIDEO_ctrl.streamTsk;                        
    	} 
		else 
		{
      		OSA_ERROR("Incorrect nextTskId (%d)\n", nextTskId);      
      		return OSA_EFAIL;    
    	}
  	return OSA_SOK;
}

int AVSERVER_AudbufGetNextTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl, OSA_TskHndl **pTskHndl)
{
	AUDIO_EncodeStream  *pAudEncodeStream=NULL;

  	int nextTskId;
  
  	*pTskHndl=NULL;
  	*pBufHndl=NULL;
 
		pAudEncodeStream = &gAUDIO_ctrl.encodeStream[streamId];
		
		nextTskId = AUDIO_TSK_NONE ;
		
		if(tskId == AUDIO_TSK_ENCODE)
		{
			nextTskId = pAudEncodeStream->encodeNextTsk ;

		}
		if(nextTskId==AUDIO_TSK_NONE)
        {
            OSA_ERROR("Incorrect nextTskId (%d)\n", nextTskId);
            return OSA_EFAIL;
        }

        if(nextTskId==AUDIO_TSK_STREAM)
        {
            *pBufHndl = &pAudEncodeStream->bufHndl;
            *pTskHndl = &gAVSERVER_ctrl.mainTsk;//gVIDEO_ctrl.streamTsk;
        }
        else
        {
            OSA_ERROR("Incorrect nextTskId (%d)\n", nextTskId);
            return OSA_EFAIL;
        }		

  	return OSA_SOK;
}

int AVSERVER_bufGetCurTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl)
{
	VIDEO_EncodeStream  *pEncodeStream=NULL;

	*pBufHndl=NULL;
  
		if(streamId<0 || streamId >= AVSERVER_MAX_STREAMS) 
		{
    		OSA_ERROR("Incorrect streamId (%d)\n", streamId);
    		return OSA_EFAIL;
  		}

  		if(tskId <= VIDEO_TSK_CAPTURE || tskId > AUDIO_TSK_STREAM) 
		{
    		OSA_ERROR("Incorrect tskId (%d)\n", tskId);  
    		return OSA_EFAIL;
  		}

 		pEncodeStream  = &gVIDEO_ctrl.encodeStream[streamId];
    
    	if(tskId==VIDEO_TSK_STREAM) 
		{   
    		*pBufHndl = &pEncodeStream->bufStreamIn;
    	} 
		else 
		{
      		OSA_ERROR("Incorrect tskId (%d)\n", tskId);    
      		return OSA_EFAIL;
    	}
  	return OSA_SOK;
}

int AVSERVER_AudbufGetCurTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl)
{
	AUDIO_EncodeStream  *pAudEncodeStream=NULL ;  

	*pBufHndl=NULL;
	
	pAudEncodeStream = &gAUDIO_ctrl.encodeStream[streamId] ;
	if(tskId == AUDIO_TSK_STREAM)
	{

		*pBufHndl = &pAudEncodeStream->bufHndl ;
	}
	else
	{
		OSA_ERROR("Incorrect tskId (%d)\n", tskId);
        return OSA_EFAIL;
	}
		
  	return OSA_SOK;
}

OSA_BufInfo *AVSERVER_bufGetEmpty(int tskId, int streamId, int *bufId, int timeout)
{
  	OSA_BufHndl *pBufHndl;
  	OSA_TskHndl *pTskHndl;
  	int status;
    
  	*bufId = -1;
 
  	status = AVSERVER_bufGetNextTskInfo(tskId, streamId, &pBufHndl, &pTskHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_bufGetNextTskInfo(%d, %d)\n", tskId, streamId);
    	return NULL;
  	}
    
  	status = OSA_bufGetEmpty(pBufHndl, bufId, timeout);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufGetEmpty(%d, %d, %d)\n", tskId, streamId, *bufId);  
    	return NULL;
  	}
//  return status ;
    
  	return OSA_bufGetBufInfo(pBufHndl, *bufId);
}


OSA_BufInfo *AVSERVER_bufPutFull(int tskId, int streamId, int bufId)
{
  	OSA_BufHndl *pBufHndl;
  	OSA_TskHndl *pTskHndl;
  	int status;

  	status = AVSERVER_bufGetNextTskInfo(tskId, streamId, &pBufHndl, &pTskHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_bufGetNextTskInfo(%d, %d)\n", tskId, streamId);  
    	return NULL;
  	}
    
  	status = OSA_bufPutFull(pBufHndl, bufId);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufPutFull(%d, %d, %d)\n", tskId, streamId, bufId);    
    	return NULL;
  	}
    
    status = OSA_tskSendMsg(pTskHndl, NULL, AVSERVER_CMD_NEW_VDATA, (void*)streamId, 0);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_tskSendMsg(AVSERVER_CMD_NEW_VDATA, %d, %d)\n", tskId, streamId);  
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, bufId);
}

OSA_BufInfo *AVSERVER_bufGetFull(int tskId, int streamId, int *bufId, int timeout)
{
  	OSA_BufHndl *pBufHndl;
  	int status;
    
  	*bufId = -1;
  
	if(streamId > 1024)
		printf("AVSERVER_bufGetFull streamId = %d\n",streamId) ;

  	status = AVSERVER_bufGetCurTskInfo(tskId, streamId, &pBufHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_bufGetCurTskInfo(%d, %d)\n", tskId, streamId);   
    	return NULL;
  	}
  
  	status = OSA_bufGetFull(pBufHndl, bufId, timeout);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufGetFull(%d, %d, %d)\n", tskId, streamId, *bufId);   
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, *bufId);
}

OSA_BufInfo *AVSERVER_bufPutEmpty(int tskId, int streamId, int bufId)
{
  	OSA_BufHndl *pBufHndl;
  	int status;
    
  	status = AVSERVER_bufGetCurTskInfo(tskId, streamId, &pBufHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_bufGetCurTskInfo(%d, %d)\n", tskId, streamId);     
    	return NULL;
  	}
    
  	status = OSA_bufPutEmpty(pBufHndl, bufId);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufPutEmpty(%d, %d, %d)\n", tskId, streamId, bufId);    
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, bufId);
}


OSA_BufInfo *AVSERVER_AudbufGetEmpty(int tskId, int streamId, int *bufId, int timeout)
{
  	OSA_BufHndl *pBufHndl;
  	OSA_TskHndl *pTskHndl;
  	int status;
    
  	*bufId = -1;
 
  	status = AVSERVER_AudbufGetNextTskInfo(tskId, streamId, &pBufHndl, &pTskHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_AudbufGetNextTskInfo(%d, %d)\n", tskId, streamId);
    	return NULL;
  	}
    
  	status = OSA_bufGetEmpty(pBufHndl, bufId, timeout);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufGetEmpty(%d, %d, %d)\n", tskId, streamId, *bufId);  
    	return NULL;
  	}
//  return status ;
    
  	return OSA_bufGetBufInfo(pBufHndl, *bufId);
}


OSA_BufInfo *AVSERVER_AudbufPutFull(int tskId, int streamId, int bufId)
{
  	OSA_BufHndl *pBufHndl;
  	OSA_TskHndl *pTskHndl;
  	int status;

  	status = AVSERVER_AudbufGetNextTskInfo(tskId, streamId, &pBufHndl, &pTskHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_bufGetNextTskInfo(%d, %d)\n", tskId, streamId);  
    	return NULL;
  	}
    
  	status = OSA_bufPutFull(pBufHndl, bufId);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufPutFull(%d, %d, %d)\n", tskId, streamId, bufId);    
    	return NULL;
  	}
    
  	status = OSA_tskSendMsg(pTskHndl, NULL, AVSERVER_CMD_NEW_ADATA, (void*)streamId, 0);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_tskSendMsg(AVSERVER_CMD_NEW_ADATA, %d, %d)\n", tskId, streamId);  
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, bufId);
}

OSA_BufInfo *AVSERVER_AudbufGetFull(int tskId, int streamId, int *bufId, int timeout)
{
  	OSA_BufHndl *pBufHndl;
  	int status;
    
  	*bufId = -1;
  
  	status = AVSERVER_AudbufGetCurTskInfo(tskId, streamId, &pBufHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_AudbufGetCurTskInfo(%d, %d)\n", tskId, streamId);   
    	return NULL;
  	}
  
  	status = OSA_bufGetFull(pBufHndl, bufId, timeout);
  	if(status!=OSA_SOK) 
	{
    //OSA_ERROR("OSA_bufGetFull(%d, %d, %d)\n", tskId, streamId, *bufId);   
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, *bufId);
}

OSA_BufInfo *AVSERVER_AudbufPutEmpty(int tskId, int streamId, int bufId)
{
  	OSA_BufHndl *pBufHndl;
  	int status;
    
  	status = AVSERVER_AudbufGetCurTskInfo(tskId, streamId, &pBufHndl);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("AVSERVER_AudbufGetCurTskInfo(%d, %d)\n", tskId, streamId);     
    	return NULL;
  	}
    
  	status = OSA_bufPutEmpty(pBufHndl, bufId);
  	if(status!=OSA_SOK) 
	{
    	OSA_ERROR("OSA_bufPutEmpty(%d, %d, %d)\n", tskId, streamId, bufId);    
    	return NULL;
  	}
    
  	return OSA_bufGetBufInfo(pBufHndl, bufId);
}
