#ifndef _APP_WRITER_H_
#define _APP_WRITER_H_
///////////////////////////////////////////////////////////////////////////////

#include <osa.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include <osa_pipe.h>

#include "basket_rec.h"

#include "ti_venc.h"

#define WRITER_MAX_NUM_FREE_BUFS_PER_CHANNEL	(4)
#define WRITER_FREE_QUE_MAX_LEN					(MAX_DVR_CHANNELS*WRITER_MAX_NUM_FREE_BUFS_PER_CHANNEL)
#define WRITER_FULL_QUE_MAX_LEN					(WRITER_FREE_QUE_MAX_LEN)
#define WRITER_AUDIO_PLAY_MAX_LEN				(4)

#define WRITER_BUFINFO_TBL_SIZE					(WRITER_FREE_QUE_MAX_LEN)
#define WRITER_BUFINFO_INVALIDID				(~0u)

typedef struct WRITER_bufInfoTbl
{
    OSA_MutexHndl mutex;
    UInt32        freeIndex;
	
    struct WRITER_bufInfoEntry
    {
        VCODEC_BITSBUF_S bitBuf;
        UInt32           nextFreeIndex;
	   	UInt32			 streamType;			/* Video / Audio */
    } tbl[WRITER_FREE_QUE_MAX_LEN];
} WRITER_bufInfoTbl;

//sk_aud
typedef struct WRITER_bufInfoTbl_AudPlay
{
    OSA_MutexHndl mutex;
    UInt32        freeIndex;
    OSA_QueHndl bufQFullBufsAudPlay;
    OSA_QueHndl bufQFreeBufsAudPlay;
	
    struct WRITER_bufInfoEntryAudPlay
    {
        VCODEC_BITSBUF_S bitBuf;
        UInt32           nextFreeIndex;
	   	UInt32			 streamType;			/* Video / Audio */
    } tbl[WRITER_AUDIO_PLAY_MAX_LEN];
} WRITER_bufInfoTbl_AudPlay;



typedef struct {

	int numCh;
	int frameWidth;
	int frameHeight;

} WRITER_CreatePrm;


typedef struct {

	int event_mode;
	int enableChSave;
	int enableAudio;
	int	presec;         ///< pre recording time ...if 0 , disabled. BKKIM
	int	postsec;        ///< post recording time ...if 0 , disabled

	int firstKeyframe;
	int fileSaveState;
	
	char camName[16];

} WRITER_ChInfo;


typedef struct {
	WRITER_CreatePrm 	createPrm;
	WRITER_ChInfo  		chInfo[BKT_MAX_CHANNEL];

	// BKKIM
	int                 bPreEnabled; ///< pre-record enable

	Uint16 	bufNum;
	Uint8  *bufVirtAddr;
	Uint32 	bufSize;

	int 	run_flag;
	int 	state;
	int		is_basket_opened;
	int 	bufPutCount;

	void*	msg_pipe_id;	//MULTI-HDD SKSUNG
	
} WRITER_Ctrl;


#define WRITER_FILE_SAVE_STATE_IDLE   (0)
#define WRITER_FILE_SAVE_STATE_OPEN   (1)
#define WRITER_FILE_SAVE_STATE_WRITE  (2)  
#define WRITER_FILE_SAVE_STATE_CLOSE  (3) 

#define VIDEO           0
#define AUDIO           1
#define VIDEO_STREAM    2
#define AUDIO_STREAM    3

int WRITER_create();
void WRITER_stop();
int WRITER_delete();

int WRITER_enableChSave(int chId, int enable, int event_mode, int addAudio, char *camName);

int WRITER_fileSaveExit();
int WRITER_isBasketOpen();
int WRITER_changeCamName(int chId, char* camName);

//MULTI-HDD SKSUNG///
void WRITER_updateMountInfo();
void WRITER_setMsgPipe(void* msg_pipe_id);
void WRITER_setInitDone();

//Void WRITER_ipcBitsInCbFxn(Ptr cbCtx);
Void WRITER_videoEncCbFxn(Ptr cbCtx);

extern WRITER_Ctrl gWRITER_ctrl;


#if PREREC_ENABLE
/**
 @author BKKIM
 @brief set pre-event record time(seconds)
 @param[in] ch channel
 @param[in] sec seconds, if 0, disabled. 1:min, 10:max
 */
void WRITER_setPreRecSec(int ch, int presec);
#endif
int Stream_SendRun(int type, int chId, VCODEC_BITSBUF_S *pBufInfo);
int WRITER_sendMsgPipe(int which, int data, void *pData);
///////////////////////////////////////////////////////////////////////////////
#endif//_APP_WRITER_H_
///////////////////////////////////////////////////////////////////////////////
