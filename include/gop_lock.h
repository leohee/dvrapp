/** ===========================================================================
* @file gop_lock.h
*
* @path $(IPNCPATH)\multimedia\encode_stream\stream
*
* @desc 
* .
*
* =========================================================================== */

#ifndef __GOP_LOCK_H__
#define __GOP_LOCK_H__

#include <mem_mng.h>

typedef enum{ 
	GOP_FAIL = -104,
	GOP_INCOMPLETE,
	GOP_INVALID_PRM,
	GOP_ERROR_OP,
	GOP_NOMEM,
	GOP_OVERWRITTEN,
	GOP_NOT_READY,
	GOP_COMPLETE = 0
}GopRet_t;

enum{
	GOP_INDEX_VIDEO_1 = 0,
	GOP_INDEX_VIDEO_2,
	GOP_INDEX_VIDEO_3,
	GOP_INDEX_VIDEO_4,
 //RTSP_EXT_CHANNEL	
	GOP_INDEX_VIDEO_5,
	GOP_INDEX_VIDEO_6,

	GOP_INDEX_VIDEO_7,

 	GOP_INDEX_VIDEO_8,
	GOP_INDEX_VIDEO_9,
	GOP_INDEX_VIDEO_10,
	GOP_INDEX_VIDEO_11,
	GOP_INDEX_VIDEO_12,
	GOP_INDEX_VIDEO_13,
	GOP_INDEX_VIDEO_14,
	GOP_INDEX_VIDEO_15,
	GOP_INDEX_VIDEO_16,
	
	GOP_INDEX_AUDIO_1,
	GOP_INDEX_AUDIO_2,
	GOP_INDEX_AUDIO_3,
	GOP_INDEX_AUDIO_4,
	GOP_INDEX_AUDIO_5,
	GOP_INDEX_AUDIO_6,
	GOP_INDEX_AUDIO_7,
	GOP_INDEX_AUDIO_8,
	GOP_INDEX_AUDIO_9,
	GOP_INDEX_AUDIO_10,
	GOP_INDEX_AUDIO_11,
	GOP_INDEX_AUDIO_12,
	GOP_INDEX_AUDIO_13,
	GOP_INDEX_AUDIO_14,
	GOP_INDEX_AUDIO_15,
	GOP_INDEX_AUDIO_16,
	GOP_INDEX_NUM	
};


void 	GopInit(void);


GopRet_t LockGopBySerial(int serial, VIDEO_BLK_INFO *pVidInfo,int index);
GopRet_t UnlockGopBySerial(int serial, VIDEO_BLK_INFO *pVidInfo,int index);
void GopCleanup(VIDEO_BLK_INFO *pVidInfo,int index );
GopRet_t LockCurrentGopP(VIDEO_BLK_INFO *pVidInfo, int index);
#endif
