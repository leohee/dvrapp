#include <avserver.h>
#include <stream.h>
//#include <encode.h>

//extern GlobalData gbl;

int AUDIO_streamShmCopy(int streamId, OSA_BufInfo *pBufInfo)
{
  int  status=OSA_SOK;
    
 
  status = stream_write(
        pBufInfo->virtAddr, 
        pBufInfo->size, 
        AUDIO_FRAME,
        STREAM_AUDIO_1 + streamId,
        pBufInfo->timestamp,
        stream_get_handle()
      );
      
  if(status!=OSA_SOK) {
    OSA_ERROR("stream_write(%d, %d, %d, %u)\n",
        pBufInfo->size, 
        AUDIO_FRAME,
        STREAM_AUDIO_1 + streamId,
        pBufInfo->timestamp
      );      
  }
  
  return status;
}

