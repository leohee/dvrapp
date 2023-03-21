
#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <osa_tsk.h>
#include <osa_buf.h>
#include <osa_mutex.h>
#include <avserver_config.h>
//#include <alg_g711.h>

#define AUDIO_TSK_NONE 		(0x1000)
#define AUDIO_TSK_ENCODE	(0x1001)
#define AUDIO_TSK_STREAM	(0x1002) 


typedef struct {
  void *physAddr;
  void *virtAddr;
  Uint32 timestamp;
  Uint32 encFrameSize;

} AUDIO_BufInfo;

typedef struct {

	int encodeNextTsk;
    OSA_BufHndl     bufHndl;
	OSA_BufCreate   CreatePrm;

} AUDIO_EncodeStream ;

typedef struct {
	OSA_TskHndl 		audioTsk;
	int 				streamId;
	short 				*encodedBuffer;
	int					encodeBufSize;
	unsigned short 		*inputBuffer;
	int					inputBufSize;
	AUDIO_EncodeStream  encodeStream[AVSERVER_MAX_STREAMS];
} AUDIO_Ctrl;

int AUDIO_streamShmCopy(int streamId, OSA_BufInfo *pBufInfo);


#endif  /*  _AUDIO_H_ */

