
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <sys/statvfs.h>// For statvfs()
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "global.h"
#include "app_util.h"
#include "app_manager.h"
#include "avserver.h"
#include "app_writer.h"

#include "app_audio.h"
#include "app_audio_priv.h"
#include "app_motion.h"
#include "lib_dvr_app.h"


#if PREREC_ENABLE
#include "prerec.h" // BKKIM
#endif

//#define AV_SYNC_ENABLE
//#define WRITER_MONITOR_TSK

//#define WRITER_DEBUG
#define WRITER_RECORDFXN_TSK_PRI                (1)
#define WRITER_VIDEOFXN_TSK_PRI                 (2)
#define WRITER_AUDIOFXN_TSK_PRI                 (2)

#define WRITER_AUDIOFXN_TSK_STACK_SIZE          (0) /* 0 means system default will be used */
#define WRITER_VIDEOFXN_TSK_STACK_SIZE          (0) /* 0 means system default will be used */

#define WRITER_INFO_PRINT_INTERVAL              (1000)

#define WRITER_DEFAULT_WIDTH                    (720)
#define WRITER_DEFAULT_HEIGHT                   (576)

//#define WRITER_GET_BUF_SIZE(width,height)		(( (width) * (height) * (2) )/2)
#define WRITER_GET_BUF_SIZE(width,height)		(( (width) * (height))/2)

#define WRITER_MAX_PENDING_RECV_SEM_COUNT		(10)


typedef struct WRITER_CtrlThrObj {
    OSA_ThrHndl thrHandleVideo;
    OSA_ThrHndl thrHandleAudio;
    OSA_ThrHndl thrHandleRecord;
    OSA_ThrHndl thrHandleStreaming;
    OSA_ThrHndl thrHandleAudioPlay; //sk_aud
    OSA_QueHndl bufQFullBufs;
    OSA_QueHndl bufQFreeBufs;
    OSA_QueHndl bufQFullBufsAud;
    OSA_QueHndl bufQFreeBufsAud;
    OSA_QueHndl bufQFullBufsAudStream;
    OSA_QueHndl bufQFreeBufsAudStream;
    OSA_QueHndl bufQFullBufsVidStream;
    OSA_QueHndl bufQFreeBufsVidStream;
    OSA_SemHndl bitsInNotifySem;
    volatile Bool exitVideoThread;
    volatile Bool exitAudioThread;
    volatile Bool exitAudioPlayThread;  //sk_aud
    volatile Bool exitRecordThread;
    volatile Bool exitStreamingThread;
} WRITER_CtrlThrObj;


typedef struct WRITER_IpcBitsCtrl {
    WRITER_CtrlThrObj  thrObj;
} WRITER_IpcBitsCtrl;


static Int32    writerInitDone = 0;
static Uint8    audioCaptureBuf[AUDIO_SAMPLES_TO_READ_PER_CHANNEL
                                    * AUDIO_MAX_CHANNELS * AUDIO_SAMPLE_LEN];
//static Uint8 	audioRecordBuf[AUDIO_SAMPLES_TO_READ_PER_CHANNEL
//									* AUDIO_MAX_CHANNELS * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER];
static Uint8 audioRecordBuf[AUDIO_MAX_CHANNELS][AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER];
extern PROFILE gInitSettings;
extern ST_CAPTURE_INFO gCaptureInfo;


BKT_Mount_Info      gbkt_mount_info;    //MULTI-HDD SKSUNG

WRITER_IpcBitsCtrl  gWRITER_ipcBitsCtrl;
WRITER_Ctrl         gWRITER_ctrl;

WRITER_bufInfoTbl   g_bufInfoTblVid;
WRITER_bufInfoTbl   g_bufInfoTblAud;
WRITER_bufInfoTbl   g_bufInfoTblVidStm;
WRITER_bufInfoTbl   g_bufInfoTblAudStm;

//sk_aud
WRITER_bufInfoTbl_AudPlay g_bufInfoTblAudPlay;
/////////

AUDIO_STATE gAudio_state ; // hwjun


typedef struct ST_WRITER_STATUS
{
    OSA_SemHndl     sem_id;

    OSA_ThrHndl     thrHandleMonitor;
    volatile Bool   exitMonitorThread;

    int             arRecordBps[MAX_DVR_CHANNELS];
    int             arRecordFps[MAX_DVR_CHANNELS];
    int             elapsed_msec[MAX_DVR_CHANNELS];
} ST_WRITER_STATUS;


ST_WRITER_STATUS gStatusHDD;
    
int count = 1;


//////////////////////////////////

static Void WRITER_bufInit()
{
    Int status,i;
    AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun

    status = OSA_mutexCreate(&g_bufInfoTblVid.mutex);
    OSA_assert(status == OSA_SOK);

    OSA_mutexLock(&g_bufInfoTblVid.mutex);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVid.tbl) - 1);i++)
    {
        g_bufInfoTblVid.tbl[i].nextFreeIndex = (i + 1);
        g_bufInfoTblVid.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(WRITER_GET_BUF_SIZE(WRITER_DEFAULT_WIDTH,WRITER_DEFAULT_HEIGHT));
    }

    g_bufInfoTblVid.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblVid.freeIndex = 0;

    OSA_mutexUnlock(&g_bufInfoTblVid.mutex);


// video-streaming


    status = OSA_mutexCreate(&g_bufInfoTblVidStm.mutex);
    OSA_assert(status == OSA_SOK);
    
    OSA_mutexLock(&g_bufInfoTblVidStm.mutex);
    
    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVidStm.tbl) - 1);i++)
    {
        g_bufInfoTblVidStm.tbl[i].nextFreeIndex = (i + 1);
        g_bufInfoTblVidStm.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(WRITER_GET_BUF_SIZE(WRITER_DEFAULT_WIDTH,WRITER_DEFAULT_HEIGHT)/4);
    }
    
    g_bufInfoTblVidStm.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblVidStm.freeIndex = 0;
    
    OSA_mutexUnlock(&g_bufInfoTblVidStm.mutex);


// audio-streaming


    status = OSA_mutexCreate(&g_bufInfoTblAudStm.mutex);

    OSA_assert(status == OSA_SOK);
    
    OSA_mutexLock(&g_bufInfoTblAudStm.mutex);
    
    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAudStm.tbl) - 1);i++)
    {
        g_bufInfoTblAudStm.tbl[i].nextFreeIndex = (i + 1);
        g_bufInfoTblAudStm.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(16*1024);   //16Kb
//      g_bufInfoTblAudStm.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(WRITER_GET_BUF_SIZE(WRITER_DEFAULT_WIDTH,WRITER_DEFAULT_HEIGHT)/4);
    }
    
    g_bufInfoTblAudStm.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAudStm.freeIndex = 0;
    
    OSA_mutexUnlock(&g_bufInfoTblAudStm.mutex);


#ifdef AUDIO_STREAM_ENABLE

    status = OSA_mutexCreate(&g_bufInfoTblAud.mutex);
    OSA_assert(status == OSA_SOK);

    OSA_mutexLock(&g_bufInfoTblAud.mutex);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAud.tbl) - 1);i++)
    {
        g_bufInfoTblAud.tbl[i].nextFreeIndex = (i + 1);
        g_bufInfoTblAud.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(16*1024);  //16Kb
//      g_bufInfoTblAud.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(WRITER_GET_BUF_SIZE(WRITER_DEFAULT_WIDTH,WRITER_DEFAULT_HEIGHT)/4);
    }

    g_bufInfoTblAud.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAud.freeIndex = 0;
    
    OSA_mutexUnlock(&g_bufInfoTblAud.mutex);

	pAudstate->state = LIB_LIVE_MODE ; // hwjun	


////sk_aud
    status = OSA_mutexCreate(&g_bufInfoTblAudPlay.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAudPlay.tbl) - 1);i++)
    {
        g_bufInfoTblAudPlay.tbl[i].nextFreeIndex = (i + 1);
        g_bufInfoTblAudPlay.tbl[i].bitBuf.bufVirtAddr = (Void *)OSA_memAlloc(32*1024);  //16Kb
    }

    g_bufInfoTblAudPlay.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAudPlay.freeIndex = 0;  
#endif
}


static Void WRITER_bufDeInit()
{
    Int status,i;

    status = OSA_mutexDelete(&g_bufInfoTblVid.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVid.tbl) - 1);i++)
    {
        g_bufInfoTblVid.tbl[i].nextFreeIndex = (i + 1);
        OSA_memFree( (Void *)g_bufInfoTblVid.tbl[i].bitBuf.bufVirtAddr );
    }
    g_bufInfoTblVid.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblVid.freeIndex = 0;


#ifdef AUDIO_STREAM_ENABLE

    status = OSA_mutexDelete(&g_bufInfoTblAud.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAud.tbl) - 1);i++)
    {
        g_bufInfoTblAud.tbl[i].nextFreeIndex = (i + 1);
        OSA_memFree( (Void *)g_bufInfoTblAud.tbl[i].bitBuf.bufVirtAddr );
    }
    g_bufInfoTblAud.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAud.freeIndex = 0;

////sk_aud
    status = OSA_mutexDelete(&g_bufInfoTblAudPlay.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAudPlay.tbl) - 1);i++)
    {
        g_bufInfoTblAudPlay.tbl[i].nextFreeIndex = (i + 1);
        OSA_memFree( (Void *)g_bufInfoTblAudPlay.tbl[i].bitBuf.bufVirtAddr );
    }
    g_bufInfoTblAudPlay.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAudPlay.freeIndex = 0;  
#endif
    
    status = OSA_mutexDelete(&g_bufInfoTblAudStm.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAudStm.tbl) - 1);i++)
    {
        g_bufInfoTblAudStm.tbl[i].nextFreeIndex = (i + 1);
        OSA_memFree( (Void *)g_bufInfoTblAudStm.tbl[i].bitBuf.bufVirtAddr );
    }
    g_bufInfoTblAudStm.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblAudStm.freeIndex = 0;


    status = OSA_mutexDelete(&g_bufInfoTblVidStm.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVidStm.tbl) - 1);i++)
    {
        g_bufInfoTblVidStm.tbl[i].nextFreeIndex = (i + 1);
        OSA_memFree( (Void *)g_bufInfoTblVidStm.tbl[i].bitBuf.bufVirtAddr );
    }
    g_bufInfoTblVidStm.tbl[i].nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    g_bufInfoTblVidStm.freeIndex = 0;
}

static VCODEC_BITSBUF_S * WRITER_bufAlloc(Int strmType)
{
    VCODEC_BITSBUF_S *freeBufInfo = NULL;

    struct WRITER_bufInfoEntry * entry = NULL;

    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVid;
    if(strmType == AUDIO)
        pTbl = &g_bufInfoTblAud;
    
    OSA_mutexLock(&pTbl->mutex);

    OSA_assert((pTbl->freeIndex != WRITER_BUFINFO_INVALIDID) &&
               (pTbl->freeIndex < OSA_ARRAYSIZE(pTbl->tbl)));

    entry = &pTbl->tbl[pTbl->freeIndex];
    pTbl->freeIndex = entry->nextFreeIndex;
    entry->nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    freeBufInfo = &entry->bitBuf;
    
    OSA_mutexUnlock(&pTbl->mutex);

    return freeBufInfo;
}

////sk_aud
static VCODEC_BITSBUF_S * WRITER_bufAllocAudPlay()
{
    VCODEC_BITSBUF_S *freeBufInfo = NULL;

    struct WRITER_bufInfoEntryAudPlay * entry = NULL;

    WRITER_bufInfoTbl_AudPlay *pTbl;

    pTbl = &g_bufInfoTblAudPlay;
    
    OSA_mutexLock(&pTbl->mutex);

    OSA_assert((pTbl->freeIndex != WRITER_BUFINFO_INVALIDID) &&
               (pTbl->freeIndex < OSA_ARRAYSIZE(pTbl->tbl)));

    entry = &pTbl->tbl[pTbl->freeIndex];
    pTbl->freeIndex = entry->nextFreeIndex;
    entry->nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    freeBufInfo = &entry->bitBuf;
    
    OSA_mutexUnlock(&pTbl->mutex);

    return freeBufInfo;
}


static Int WRITER_bufGetIndex(VCODEC_BITSBUF_S * bufInfo, Int strmType)
{
    Int index;
    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVid;
    if(strmType == AUDIO)
       pTbl = &g_bufInfoTblAud;
    
    struct WRITER_bufInfoEntry *entry = (struct WRITER_bufInfoEntry *)bufInfo;

    OSA_COMPILETIME_ASSERT(offsetof(struct WRITER_bufInfoEntry,bitBuf) == 0);

    index = entry - &(pTbl->tbl[0]);
    return index;
}

/////sk_aud
static Int WRITER_bufGetIndexAudPlay(VCODEC_BITSBUF_S * bufInfo)
{
    Int index;
    WRITER_bufInfoTbl_AudPlay *pTbl;

   pTbl = &g_bufInfoTblAudPlay;
    
    struct WRITER_bufInfoEntryAudPlay *entry = (struct WRITER_bufInfoEntryAudPlay *)bufInfo;

    OSA_COMPILETIME_ASSERT(offsetof(struct WRITER_bufInfoEntryAudPlay, bitBuf) == 0);

    index = entry - &(pTbl->tbl[0]);
    return index;
}


static Void WRITER_bufFree(VCODEC_BITSBUF_S * bitBufInfo, Int strmType)
{
    Int entryIndex;
    struct WRITER_bufInfoEntry * entry = NULL;

    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVid;
    if(strmType == AUDIO)
        pTbl = &g_bufInfoTblAud;

    OSA_mutexLock(&pTbl->mutex);
    
    entryIndex = WRITER_bufGetIndex(bitBufInfo, strmType);
    OSA_assert((entryIndex >= 0) && (entryIndex < OSA_ARRAYSIZE(pTbl->tbl)));
    
    entry = &pTbl->tbl[entryIndex];
    entry->nextFreeIndex = pTbl->freeIndex;
    pTbl->freeIndex = entryIndex;
    
    OSA_mutexUnlock(&pTbl->mutex);
}

////sk_aud
static Void WRITER_bufFreeAudPlay(VCODEC_BITSBUF_S * bitBufInfo)
{
    Int entryIndex;
    struct WRITER_bufInfoEntryAudPlay * entry = NULL;

    WRITER_bufInfoTbl_AudPlay *pTbl;

    pTbl = &g_bufInfoTblAudPlay;

    OSA_mutexLock(&pTbl->mutex);
    
    entryIndex = WRITER_bufGetIndexAudPlay(bitBufInfo);
    OSA_assert((entryIndex >= 0) && (entryIndex < OSA_ARRAYSIZE(pTbl->tbl)));
    
    entry = &pTbl->tbl[entryIndex];
    entry->nextFreeIndex = pTbl->freeIndex;
    pTbl->freeIndex = entryIndex;
    
    OSA_mutexUnlock(&pTbl->mutex);
}


static VCODEC_BITSBUF_S * WRITER_bufAllocStream(Int strmType)
{
    VCODEC_BITSBUF_S *freeBufInfo = NULL;

    struct WRITER_bufInfoEntry * entry = NULL;
    
    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVidStm ;
    if(strmType == AUDIO_STREAM)
        pTbl = &g_bufInfoTblAudStm ;

    OSA_mutexLock(&pTbl->mutex);
    
    OSA_assert((pTbl->freeIndex != WRITER_BUFINFO_INVALIDID) &&
               (pTbl->freeIndex < OSA_ARRAYSIZE(pTbl->tbl)));
    
    entry = &pTbl->tbl[pTbl->freeIndex];
    pTbl->freeIndex = entry->nextFreeIndex;
    entry->nextFreeIndex = WRITER_BUFINFO_INVALIDID;
    freeBufInfo = &entry->bitBuf;
    OSA_mutexUnlock(&pTbl->mutex);

    return freeBufInfo;
}

static Int WRITER_bufGetIndexStream(VCODEC_BITSBUF_S * bufInfo, Int strmType)
{
    Int index;
    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVidStm ;
    if(strmType == AUDIO_STREAM)
        pTbl = &g_bufInfoTblAudStm ;

    struct WRITER_bufInfoEntry *entry = (struct WRITER_bufInfoEntry *)bufInfo;

    OSA_COMPILETIME_ASSERT(offsetof(struct WRITER_bufInfoEntry,bitBuf) == 0);

    index = entry - &(pTbl->tbl[0]);
    return index;
}


static Void WRITER_bufFreeStream(VCODEC_BITSBUF_S * bitBufInfo, Int strmType)
{
    Int entryIndex;
    struct WRITER_bufInfoEntry * entry = NULL;

    WRITER_bufInfoTbl *pTbl;

    pTbl = &g_bufInfoTblVidStm ;
    if(strmType == AUDIO_STREAM)
        pTbl = &g_bufInfoTblAudStm ;

    OSA_mutexLock(&pTbl->mutex);
    
    entryIndex = WRITER_bufGetIndexStream(bitBufInfo, strmType);
    OSA_assert((entryIndex >= 0) && (entryIndex < OSA_ARRAYSIZE(pTbl->tbl)));

    entry = &pTbl->tbl[entryIndex];
    entry->nextFreeIndex = pTbl->freeIndex;
    pTbl->freeIndex = entryIndex;

    OSA_mutexUnlock(&pTbl->mutex);
}


static Void WRITER_videoCopyBufInfo (VCODEC_BITSBUF_S *dst, const VCODEC_BITSBUF_S *src)
{
    dst->chnId          = src->chnId;
    dst->codecType      = src->codecType;
    dst->filledBufSize  = src->filledBufSize;
    dst->frameType      = src->frameType;
    dst->timestamp      = src->timestamp;
    dst->upperTimeStamp = src->upperTimeStamp;
    dst->lowerTimeStamp = src->lowerTimeStamp;
    dst->frameWidth     = src->frameWidth;
    dst->frameHeight    = src->frameHeight;
}


static Void WRITER_videoCopyBufDataMem2Mem(VCODEC_BITSBUF_S *dstBuf, VCODEC_BITSBUF_S *srcBuf)
{

    memcpy(dstBuf->bufVirtAddr,srcBuf->bufVirtAddr,srcBuf->filledBufSize);
//  dstBuf->bufVirtAddr = srcBuf->bufVirtAddr;
    OSA_assert(srcBuf->filledBufSize < WRITER_GET_BUF_SIZE(WRITER_DEFAULT_WIDTH,WRITER_DEFAULT_HEIGHT));
}

//MULTI-HDD SKSUNG//
int WRITER_sendMsgPipe(int which, int data, void *pData)
{
    WRITER_Ctrl* pCtrl = &gWRITER_ctrl;
    DVR_MSG_T   msg;

    if (pCtrl->msg_pipe_id == NULL)
        return OSA_EFAIL;

    msg.which = which;
    msg.data = data;
    msg.pData = pData;

    OSA_WriteToPipe(pCtrl->msg_pipe_id, &msg, sizeof(DVR_MSG_T), OSA_SUSPEND);

    return OSA_SOK;
}


#if PREREC_ENABLE
T_TS gTs[MAX_DVR_CHANNELS];
static int WRITER_prePushframe(int type, int chId, VCODEC_BITSBUF_S *pBufInfo)
{
    Uint64 captime;
    struct timeval tv;
    WRITER_ChInfo *pChInfo = &gWRITER_ctrl.chInfo[chId];

    // prerecord enable
    if( pChInfo->enableChSave || pChInfo->presec > 0) 
    {
#ifdef AV_SYNC_ENABLE   
        captime = (Uint64)pBufInfo->upperTimeStamp << 32 | pBufInfo->lowerTimeStamp ;
#endif

        gettimeofday(&tv, NULL);


        if (type == VIDEO)
        {
            T_VIDEO_REC_PARAM vp;

            vp.ch         = chId;
            vp.framesize  = pBufInfo->filledBufSize;
            vp.framerate  = 30;
            vp.event      = pChInfo->event_mode;
            vp.frametype  = pBufInfo->frameType;
            vp.width      = pBufInfo->frameWidth;
            vp.height     = pBufInfo->frameHeight;
            vp.buff       = pBufInfo->bufVirtAddr;
#ifdef AV_SYNC_ENABLE
            vp.ts.sec     = (long)((UInt64)captime/1000) ;
            vp.ts.usec    = ((pBufInfo->lowerTimeStamp%1000)>240)?(long)(((pBufInfo->lowerTimeStamp%1000)-240)*1000):(long)(((pBufInfo->lowerTimeStamp%1000)-240 + 1000)*1000);


            if(chId == 2)
            {
                if((count%1000) == 0)
                {
                    printf("APP_WRITER: tv.tv_sec: %ld tv.tv_usec: %ld \n", tv.tv_sec, tv.tv_usec);
                    printf("APP_WRITER: vp.ts.sec: %ld vp.ts.usec :%ld \n", vp.ts.sec, vp.ts.usec);
                    printf("APP_WRITER: vp.ts.usec: %ld\n", (long)pBufInfo->lowerTimeStamp);
                    printf("CAPTIME: %lld\n", captime);
                    count = 1;
                }
                count++;
            }  

            if(vp.ts.sec <= gTs[vp.ch].sec)
            {
                if(vp.ts.usec <= gTs[vp.ch].usec)
                {
                    /* If there is no overflow in microsecond value */
                    if((gTs[vp.ch].usec + 33000)/1000000 == 0)
                    {
                        vp.ts.sec     = gTs[vp.ch].sec;
                        vp.ts.usec    = gTs[vp.ch].usec + 33000;
                    }
                    else /* If microsecond vlaue overflows to sec */
                    {
                        vp.ts.sec     = gTs[vp.ch].sec + 1;
                        vp.ts.usec    = gTs[vp.ch].usec + 33000 - 1000000;
                    }
                    
                }
            }
            /* update timestamp to keep up to current rate */
            gTs[vp.ch].sec = vp.ts.sec;
            gTs[vp.ch].usec = vp.ts.usec;
            
 
            

#else
            vp.ts.sec     = tv.tv_sec ;
            vp.ts.usec    = tv.tv_usec;
#endif
            vp.audioONOFF = pChInfo->enableAudio;
            strncpy(vp.camName, pChInfo->camName, 16);

            PRELIST_add_tail_vdo(&vp);
        }
        else { // audio

//			if(pChInfo->enableAudio)
            {
                T_AUDIO_REC_PARAM ap;

                ap.ch               = chId;
                ap.framesize        = pBufInfo->filledBufSize;
                ap.buff             = pBufInfo->bufVirtAddr;
                //ap.buff           = pBufInfo->bufVirtAddr+chId*1024;
                ap.achannel         = 1;
                ap.samplingrate     = pBufInfo->fieldId;
                //ap.bitpersample     = 0;

                ap.ts.sec     = tv.tv_sec ;
                ap.ts.usec    = tv.tv_usec;


                PRELIST_add_tail_ado(&ap);
            }

        } // audio

    } //if( 0 != pChInfo->presec) 

    return OSA_SOK;
}
#endif


static int WRITER_fileSaveRun(int type, int chId, VCODEC_BITSBUF_S *pBufInfo)
{
    Uint64 captime ;
    struct timeval tv;

    int  fileSaveState;
    int  status=OSA_SOK;
    WRITER_ChInfo *pChInfo;
    T_VIDEO_REC_PARAM vp;
    T_AUDIO_REC_PARAM ap;

    pChInfo = &gWRITER_ctrl.chInfo[chId];
    fileSaveState = pChInfo->fileSaveState;

    if(pChInfo->enableChSave)
    {
        if(fileSaveState==WRITER_FILE_SAVE_STATE_IDLE)
            fileSaveState=WRITER_FILE_SAVE_STATE_OPEN;
    }
    else
    {
        if(fileSaveState==WRITER_FILE_SAVE_STATE_OPEN)
            fileSaveState=WRITER_FILE_SAVE_STATE_IDLE;
        else
        {
            if(fileSaveState==WRITER_FILE_SAVE_STATE_WRITE)
                fileSaveState=WRITER_FILE_SAVE_STATE_CLOSE;
        }
    }

    if(fileSaveState==WRITER_FILE_SAVE_STATE_OPEN)
    {
        if(pBufInfo->frameType == FRAME_TYPE_I)
            fileSaveState = WRITER_FILE_SAVE_STATE_WRITE;
    }

    if(fileSaveState==WRITER_FILE_SAVE_STATE_WRITE)
    {
        if(pBufInfo->filledBufSize > 0)
        {
#ifdef AV_SYNC_ENABLE       
            captime = (Uint64)pBufInfo->upperTimeStamp << 32 | pBufInfo->lowerTimeStamp ;
#endif
            gettimeofday(&tv, NULL);


            if(type == VIDEO)
            {
                if(pChInfo->firstKeyframe == FALSE && pBufInfo->frameType == FRAME_TYPE_I)
                    pChInfo->firstKeyframe = TRUE;

                if(pChInfo->firstKeyframe == TRUE)
                {
                    vp.ch           = chId;
                    vp.framesize    = pBufInfo->filledBufSize;
					vp.framerate 	= gInitSettings.camera[chId].camrec.iFps;
                    vp.event        = pChInfo->event_mode;
                    vp.frametype    = pBufInfo->frameType;
                    vp.width        = pBufInfo->frameWidth;
                    vp.height       = pBufInfo->frameHeight;
                    vp.buff         = pBufInfo->bufVirtAddr;
#ifdef AV_SYNC_ENABLE
                    vp.ts.sec       = (long)captime/1000 ;
                    vp.ts.usec      = (long)(pBufInfo->lowerTimeStamp%1000)*1000;
                   
#else                   
                    vp.ts.sec       = tv.tv_sec ;
                    vp.ts.usec      = tv.tv_usec;
#endif
                    vp.audioONOFF   = pChInfo->enableAudio;
                    strncpy(vp.camName, pChInfo->camName, 16);

                    status = BKTREC_WriteVideoStream(&vp);
                    if(status == BKT_ERR){
                        pChInfo->enableChSave = FALSE;
                        fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
                        WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_REC_FAIL, NULL);
                        eprintf("failed write video Ch %d \n", chId);
                    }
                    else if(status == BKT_FULL){
                        pChInfo->enableChSave = FALSE;
                        fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
                        WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_HDD_FULL, NULL);
                        eprintf("failed write video Ch %d \n", chId);
                    }
                }
            }
            else // audio
            {
//              if(pChInfo->enableAudio)
                {
                    ap.ch               = chId;
                    ap.framesize        = pBufInfo->filledBufSize;
                    ap.buff             = pBufInfo->bufVirtAddr;
                    ap.samplingrate     = pBufInfo->fieldId;
                    ap.achannel         = 1;
                  
                    ap.ts.sec           = tv.tv_sec ;
                    ap.ts.usec          = tv.tv_usec;
                 
                    //ap.bitpersample     = 0;

                    if(BKTREC_WriteAudioStream(&ap) == BKT_ERR) //bk.. except case full basket.caution...091106
                    {
                        pChInfo->enableChSave = FALSE;
                        fileSaveState=WRITER_FILE_SAVE_STATE_CLOSE;
                        WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_REC_FAIL, NULL);
                    }
                }
            }

        } //end if(pBufInfo->filledBufSize > 0)

    } //end if(fileSaveState==WRITER_FILE_SAVE_STATE_WRITE)

    if(fileSaveState==WRITER_FILE_SAVE_STATE_CLOSE) {

        fileSaveState=WRITER_FILE_SAVE_STATE_IDLE;
        pChInfo->firstKeyframe = FALSE;

		dprintf("IDLE file. CH:%d\n", chId);
    }

    pChInfo->fileSaveState = fileSaveState;

    return status;
}

 int Stream_SendRun(int type, int chId, VCODEC_BITSBUF_S *pBufInfo)
{
    int inBufIdRtsp ;
    OSA_BufInfo *pInBufRtsp ;


    if(type == VIDEO_STREAM)
    {
        pInBufRtsp = AVSERVER_bufGetEmpty( VIDEO_TSK_ENCODE, chId /*- MAX_CH*/, &inBufIdRtsp, OSA_TIMEOUT_NONE);

        if(pInBufRtsp != NULL)
        {
            pInBufRtsp->timestamp   = pBufInfo->timestamp;
            pInBufRtsp->size        = pBufInfo->filledBufSize;
            pInBufRtsp->isKeyFrame  = !pBufInfo->frameType;
            memcpy(pInBufRtsp->virtAddr, pBufInfo->bufVirtAddr, pBufInfo->filledBufSize) ;
            AVSERVER_bufPutFull( VIDEO_TSK_ENCODE, chId/* - MAX_CH*/, inBufIdRtsp);
        }
    }
    else
    {

        pInBufRtsp = AVSERVER_AudbufGetEmpty( AUDIO_TSK_ENCODE, chId, &inBufIdRtsp, OSA_TIMEOUT_NONE);

        if(pInBufRtsp != NULL)
        {

            pInBufRtsp->timestamp   = pBufInfo->timestamp;
            pInBufRtsp->size        = pBufInfo->filledBufSize;
            pInBufRtsp->isKeyFrame  = !pBufInfo->frameType;
            memcpy(pInBufRtsp->virtAddr, pBufInfo->bufVirtAddr, pBufInfo->filledBufSize) ;
            
            AVSERVER_AudbufPutFull( AUDIO_TSK_ENCODE, chId, inBufIdRtsp);

        }

    }

    return 0;
}


static Void WRITER_videoProcessFullBufs(WRITER_CtrlThrObj *thrObj)
{
    int status=OSA_SOK, outBufId_Motion, i ;

    VCODEC_BITSBUF_LIST_S fullBufList;

    VCODEC_BITSBUF_S *pFullBuf;
    VCODEC_BITSBUF_S *pEmptyBuf;

    VCODEC_BITSBUF_S *pEmptyBufStm;

    OSA_BufInfo *pOutBuf_Motion = NULL;

    ST_WRITER_STATUS *d = &gStatusHDD;
    

#ifdef WRITER_DEBUG
    static Int printStatsInterval = 0;

    if ((printStatsInterval % WRITER_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("MCFW_IPCBITS:%s:INFO: periodic print..",__func__);
    }
    printStatsInterval++;
#endif

    Venc_getBitstreamBuffer(&fullBufList, 0);

    for (i = 0; i < fullBufList.numBufs; i++)
    {

        pFullBuf = &fullBufList.bitsBuf[i];
        if(pFullBuf->chnId < MAX_CH)
        {
            status = OSA_queGet(&thrObj->bufQFreeBufs,(Int32 *)(&pEmptyBuf), OSA_TIMEOUT_FOREVER);
            if(status != OSA_EFAIL)
            {
                WRITER_videoCopyBufInfo(pEmptyBuf, pFullBuf);
                WRITER_videoCopyBufDataMem2Mem(pEmptyBuf, pFullBuf);

                OSA_quePut(&thrObj->bufQFullBufs, (Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
                
                d->arRecordBps[pFullBuf->chnId] += pFullBuf->filledBufSize;
                d->arRecordFps[pFullBuf->chnId] ++;
            }
        }
        
        if((pFullBuf->chnId < MAX_CH))
        {
            pOutBuf_Motion = VIDEOMOTION_getEmptyBuf(&outBufId_Motion, OSA_TIMEOUT_NONE);
            if(pOutBuf_Motion !=  NULL)
            {
                memcpy(pOutBuf_Motion->virtAddr, pFullBuf->bufVirtAddr + pFullBuf->mvDataOffset, pFullBuf->mvDataFilledSize) ;
                pOutBuf_Motion->count      = (pFullBuf->frameType==0)?1:0;
                pOutBuf_Motion->size       =  pFullBuf->mvDataFilledSize;
                pOutBuf_Motion->timestamp  =  pFullBuf->timestamp;
                pOutBuf_Motion->width      =  pFullBuf->frameWidth;
                pOutBuf_Motion->height     =  pFullBuf->frameHeight;
                pOutBuf_Motion->isKeyFrame = (pFullBuf->frameType==0)?1:0;
                pOutBuf_Motion->codecType  =  pFullBuf->codecType;
                pOutBuf_Motion->flags      =  pFullBuf->chnId ;           
                VIDEOMOTION_putFullBuf(pFullBuf->chnId, pOutBuf_Motion, outBufId_Motion);
            }
        }
        
        
#ifdef RTSP_STREAM_ENABLE
      //  if(pFullBuf->chnId >= MAX_CH && pFullBuf->chnId < MAX_CH + AVSERVER_MAX_STREAMS)
       if(pFullBuf->chnId < MAX_CH)
        {

            status = OSA_queGet(&thrObj->bufQFreeBufsVidStream,(Int32 *)(&pEmptyBufStm), OSA_TIMEOUT_FOREVER);
            if(status != OSA_EFAIL)
            {
                WRITER_videoCopyBufInfo(pEmptyBufStm, pFullBuf);
                WRITER_videoCopyBufDataMem2Mem(pEmptyBufStm, pFullBuf);

                OSA_quePut(&thrObj->bufQFullBufsVidStream, (Int32)pEmptyBufStm, OSA_TIMEOUT_NONE);
            }
        }
#endif

//		OSA_waitMsecs(1);
    }

    Venc_releaseBitstreamBuffer(&fullBufList);
}


static Void *WRITER_videoRecvFxn(Void * prm)
{
    WRITER_CtrlThrObj *thrObj = (WRITER_CtrlThrObj *) prm;

#ifdef WRITER_DEBUG
    static Int printStats = 0;
#endif

    OSA_printf("APP_WRITER:%s:Entered...",__func__);
    while (FALSE == thrObj->exitVideoThread)
    {
        OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        WRITER_videoProcessFullBufs(thrObj);

#ifdef WRITER_DEBUG
        if ((printStats % WRITER_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("APP_WRITER:%s:INFO: periodic print..",__func__);
        }
        printStats++;
#endif
    }

    OSA_printf("APP_WRITER:%s:Leaving...",__func__);
    return NULL;
}

///////sk_aud/////
static Void WRITER_audioPlayFxn(Void * prm)
{
    Int status;
    WRITER_CtrlThrObj *thrObj = (WRITER_CtrlThrObj *) prm;
	WRITER_bufInfoTbl_AudPlay *pAudPlay = &g_bufInfoTblAudPlay;

    VCODEC_BITSBUF_S *pEmptyBuf;
    VCODEC_BITSBUF_S *pFullBuf;

    while (FALSE == thrObj->exitAudioPlayThread)
    {
        status = OSA_queGet(&pAudPlay->bufQFullBufsAudPlay,(Int32 *)(&pFullBuf), OSA_TIMEOUT_NONE);
        if(status != OSA_EFAIL)
        {
			if(gInitSettings.audioout.iAudioOutOn == TRUE)
			{
            if (Audio_playIsStart(AUDIO_PLAY_AIC3x)) {
                Audio_playAudio(AUDIO_PLAY_AIC3x, pFullBuf->bufVirtAddr, pFullBuf->filledBufSize);
            }
            
            if (Audio_playIsStart(AUDIO_PLAY_ONCHIP_HDMI)) {
                Audio_playAudio(AUDIO_PLAY_ONCHIP_HDMI, pFullBuf->bufVirtAddr, pFullBuf->filledBufSize);
            }
			}

            WRITER_bufFreeAudPlay(pFullBuf);
            
            pEmptyBuf = WRITER_bufAllocAudPlay();
			OSA_quePut(&pAudPlay->bufQFreeBufsAudPlay,(Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
        }   

        OSA_waitMsecs(1);
    }

}

static Void WRITER_audioRecvFxn(Void * prm)
{
    Int32 i, err, len, rcount;
    Int32 frameLen, sampleLen;
    Int32 frameCnt = 0;
    
    Uint32 stored, offset;
    
    Uint8 *tmp, *tmp1;
    Uint8 buf[AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER];
    Uint8 buf_g711_encode[AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER]; /* Added Support for G711 Codec */

    WRITER_CtrlThrObj *thrObj = (WRITER_CtrlThrObj *) prm;
	WRITER_bufInfoTbl_AudPlay *pAudPlay = &g_bufInfoTblAudPlay;

    VCODEC_BITSBUF_S *pEmptyBuf, *pAudEmptyBuf, *pEmptyBufAudPlay;  //sk_aud    
    AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun
    
    while (FALSE == thrObj->exitAudioThread)
    {
        if (Audio_captureIsStart())
        {
            len = AUDIO_SAMPLES_TO_READ_PER_CHANNEL;
			
            rcount = Audio_captureAudio(audioCaptureBuf, len);
            if (rcount > 0) {
                frameLen  = Audio_captureGetFrameLength();
                sampleLen = Audio_captureGetSampleLength();
                
                /* capture buffer relocation */
                for (i = 0; i < AUDIO_MAX_CHANNELS; i++) 
                {
                    stored = 0;
					if(gInitSettings.audioin.audioinch[i].iAudioInOn == TRUE)
					{
                    tmp = audioCaptureBuf;
                    tmp += (Audio_captureGetIndexMap(i) * sampleLen);
                    
                    /* receiving frames */
                    while (stored < (rcount * sampleLen)) {
                        memcpy(buf + stored, tmp, sampleLen);
                        stored += sampleLen;
                        tmp += frameLen;
                    }
                    
                    /* live playback hwjun */
						if (pAudstate->state == LIB_LIVE_MODE)
                    {
						if (pAudstate->livechannel == i)
                        {
                            ///////sk_aud///////
								err = OSA_queGet(&pAudPlay->bufQFreeBufsAudPlay, (Int32*)(&pEmptyBufAudPlay), OSA_TIMEOUT_NONE);
                            if (err == 0) {
                                pEmptyBufAudPlay->chnId = i;
                                pEmptyBufAudPlay->filledBufSize = stored;
                                memcpy(pEmptyBufAudPlay->bufVirtAddr, (Void*)buf, stored);
        
									OSA_quePut(&pAudPlay->bufQFullBufsAudPlay, (Int32)pEmptyBufAudPlay, OSA_TIMEOUT_NONE);
                            } 
                            /////////////////////
                        }
                    }
                    /* copy to audioRecordBuf(64 periods stored) */
                    offset = AUDIO_PLAYBACK_SAMPLE_MULTIPLIER * AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN;
                    
						tmp1 = audioRecordBuf[i];
						tmp1 += (frameCnt * stored); 
                    memcpy(tmp1, buf, stored);
                }
				}
                /* count frames */
                frameCnt++; 
#if 1				
				if (frameCnt >= AUDIO_PLAYBACK_SAMPLE_MULTIPLIER)
				{
					frameCnt = 0;
					
					/* audio recording */
					offset = 0;
					for (i = 0; i < AUDIO_MAX_CHANNELS; i++)
					{
						stored = AUDIO_PLAYBACK_SAMPLE_MULTIPLIER * AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_SAMPLE_LEN;
						
						if(gInitSettings.audioin.audioinch[i].iAudioInOn == TRUE)
						{
							AUDIO_audioEncode( 1,  							/* audio codec id */ 
											  (short *)buf_g711_encode, 	/* dst buffer */ 
											  (short *)audioRecordBuf[i], 	/* src buffer */
											  stored); 						/* buffer size*/
							
							err = OSA_queGet(&thrObj->bufQFreeBufsAud,(Int32 *)(&pEmptyBuf), OSA_TIMEOUT_FOREVER);
							if (err != OSA_EFAIL) {
								pEmptyBuf->chnId 			= i;
								pEmptyBuf->codecType 		= VCODEC_TYPE_MAX;
								pEmptyBuf->filledBufSize	= stored/2;
								pEmptyBuf->fieldId			= AUDIO_SAMPLE_RATE_DEFAULT;
								memcpy(pEmptyBuf->bufVirtAddr, (Void*)buf_g711_encode, pEmptyBuf->filledBufSize);
			
						        OSA_quePut(&thrObj->bufQFullBufsAud, (Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
							}
							
							if (i < AVSERVER_MAX_STREAMS) {
					        	err = OSA_queGet(&thrObj->bufQFreeBufsAudStream,(Int32 *)(&pAudEmptyBuf), OSA_TIMEOUT_FOREVER);
					        	if (err != OSA_EFAIL) {
									pAudEmptyBuf->chnId 			= i;
									pAudEmptyBuf->codecType 		= VCODEC_TYPE_MAX;
									pAudEmptyBuf->filledBufSize		= stored/2;
									pAudEmptyBuf->fieldId			= AUDIO_SAMPLE_RATE_DEFAULT;
									memcpy(pAudEmptyBuf->bufVirtAddr, (Void*)buf_g711_encode, pAudEmptyBuf->filledBufSize);
								
						        	OSA_quePut(&thrObj->bufQFullBufsAudStream, (Int32)pAudEmptyBuf, OSA_TIMEOUT_NONE);
				        		}
							}
						}
					}
				} 
#endif				
			} //# if (err > 0) {
            else {          
                AUDIO_ERROR ("AUDIO >> CAPTURE ERROR %s, capture wont continue...\n", 
                                                                    snd_strerror(Audio_captureGetLastErr()));
                Audio_captureStop();
                Audio_captureStart();
            }
        }
        OSA_waitMsecs(1);
    }
    //return NULL;  // BKKIM, unused
}

static Void *WRITER_recordFxn(void * prm)
{
    Int status, streamType;
    WRITER_CtrlThrObj *thrObj = (WRITER_CtrlThrObj *) prm;

    VCODEC_BITSBUF_S *pEmptyBuf;
    VCODEC_BITSBUF_S *pFullBuf;

    OSA_printf("APP_WRITER:%s:Entered...",__func__);
    while (FALSE == thrObj->exitRecordThread)
    {
        status = OSA_queGet(&thrObj->bufQFullBufs,(Int32 *)(&pFullBuf), OSA_TIMEOUT_NONE);
        if(status != OSA_EFAIL)
        {
            streamType = VIDEO;

            if (BKTREC_IsOpenBasket()/* && writerInitDone == 1*/) {
#if PREREC_ENABLE
                WRITER_prePushframe(streamType, pFullBuf->chnId, pFullBuf);
#else
                WRITER_fileSaveRun(streamType, pFullBuf->chnId, pFullBuf);
#endif
            }

            WRITER_bufFree(pFullBuf, VIDEO);

            pEmptyBuf = WRITER_bufAlloc(VIDEO);
            OSA_quePut(&thrObj->bufQFreeBufs,(Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
        }

#ifdef AUDIO_STREAM_ENABLE
        status = OSA_queGet(&thrObj->bufQFullBufsAud,(Int32 *)(&pFullBuf), OSA_TIMEOUT_NONE);
        if(status != OSA_EFAIL)
        {
            streamType = AUDIO;

            if (BKTREC_IsOpenBasket()){
#if PREREC_ENABLE
                WRITER_prePushframe(streamType, pFullBuf->chnId, pFullBuf);
#else
                WRITER_fileSaveRun(streamType, pFullBuf->chnId, pFullBuf);
#endif
            }

            WRITER_bufFree(pFullBuf, AUDIO);
            
            pEmptyBuf = WRITER_bufAlloc(AUDIO);
            OSA_quePut(&thrObj->bufQFreeBufsAud,(Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
        }
#endif

        OSA_waitMsecs(1);
    }
    
    OSA_printf("APP_WRITER:%s:Leaving...",__func__);
    return NULL;
}


static Void *SEND_streamFxn(void * prm)
{
    Int status, streamType ;
    WRITER_CtrlThrObj *thrObj = (WRITER_CtrlThrObj *) prm;

    VCODEC_BITSBUF_S *pEmptyBuf, *pAudEmptyBuf;
    VCODEC_BITSBUF_S *pFullBuf, *pAudFullBuf;

    OSA_printf("APP_WRITER:%s:Entered...",__func__);
    while (FALSE == thrObj->exitStreamingThread)
    {
        status = OSA_queGet(&thrObj->bufQFullBufsVidStream,(Int32 *)(&pFullBuf), OSA_TIMEOUT_NONE);
        if(status != OSA_EFAIL)
        {
            streamType = VIDEO_STREAM;
            
            if(get_rtsp_status())
                Stream_SendRun(streamType, pFullBuf->chnId, pFullBuf) ;

            WRITER_bufFreeStream(pFullBuf, VIDEO_STREAM);
            pEmptyBuf = WRITER_bufAllocStream(VIDEO_STREAM);

            OSA_quePut(&thrObj->bufQFreeBufsVidStream,(Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
        }

#ifdef AUDIO_STREAM_ENABLE
        status = OSA_queGet(&thrObj->bufQFullBufsAudStream,(Int32 *)(&pAudFullBuf), OSA_TIMEOUT_NONE);
        if(status != OSA_EFAIL)
        {
            streamType = AUDIO_STREAM;

            if(get_rtsp_status())
                Stream_SendRun(streamType, pAudFullBuf->chnId, pAudFullBuf) ;

            WRITER_bufFreeStream(pAudFullBuf, AUDIO_STREAM);

            pAudEmptyBuf = WRITER_bufAllocStream(AUDIO_STREAM);
            OSA_quePut(&thrObj->bufQFreeBufsAudStream,(Int32)pAudEmptyBuf, OSA_TIMEOUT_NONE);
        }
#endif

        OSA_waitMsecs(1);
    }

    OSA_printf("APP_WRITER:%s:Leaving...",__func__);
    return NULL;
}

static void dbgSystemTime(void)
{
    T_BKT_TM tm1;
    time_t time_now = time(NULL);

    BKT_GetLocalTime(time_now, &tm1);
    dprintf("(TIME) - %04d/%02d/%02d~%02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
}

static Void *monitorTskEntry(void * prm)
{
    ST_WRITER_STATUS *d = &gStatusHDD;
	UInt32 msec = 30000;
    int i;
    
    OSA_semWait(&d->sem_id, OSA_TIMEOUT_FOREVER);
    
    while(d->exitMonitorThread == FALSE)
    {
/*
		printf("\n");
		dbgSystemTime();

		printf("(W) - ");
		for (i = 0; i < MAX_DVR_CHANNELS; i++)
			printf("%4u ",d->arRecordBps[i] / (msec/1000) * 8 / 1024);
		printf(", ");
		for (i = 0; i < MAX_DVR_CHANNELS; i++)
			printf("%2u ",d->arRecordFps[i] / (msec/1000));
		printf("\n");
*/
		Vsys_printDetailedStatistics() ;

        memset(d, 0x00, sizeof(ST_WRITER_STATUS));

        OSA_waitMsecs(msec);
    }
}

static void app_writer_dgb_start(void)
{
    ST_WRITER_STATUS *pStatus = &gStatusHDD;
    OSA_semSignal(&pStatus->sem_id);
}

static Void WRITER_initThrObj(WRITER_CtrlThrObj *thrObj)
{
    Int i, status;
    VCODEC_BITSBUF_S *pBuf;
	WRITER_bufInfoTbl_AudPlay *pAudPlay = &g_bufInfoTblAudPlay;	
    ST_WRITER_STATUS *pStatus = &gStatusHDD;
    memset(pStatus, 0, sizeof(ST_WRITER_STATUS));
    
    OSA_semCreate(&pStatus->sem_id, WRITER_MAX_PENDING_RECV_SEM_COUNT,0);
    OSA_semCreate(&thrObj->bitsInNotifySem, WRITER_MAX_PENDING_RECV_SEM_COUNT,0);

    pStatus->exitMonitorThread = FALSE;
    
    thrObj->exitVideoThread     = FALSE;
    thrObj->exitAudioThread     = FALSE;
    thrObj->exitAudioPlayThread     = FALSE;    //sk_aud
    thrObj->exitRecordThread    = FALSE;
    thrObj->exitStreamingThread = FALSE;
    
    OSA_queCreate(&thrObj->bufQFreeBufs,WRITER_FREE_QUE_MAX_LEN);
    OSA_queCreate(&thrObj->bufQFullBufs,WRITER_FULL_QUE_MAX_LEN);

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVid.tbl) - 1);i++)
    {
        pBuf = WRITER_bufAlloc(VIDEO);
        OSA_assert(pBuf != NULL);
        status = OSA_quePut(&thrObj->bufQFreeBufs,(Int32)pBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);        
    }   

#ifdef AUDIO_STREAM_ENABLE	
    OSA_queCreate(&thrObj->bufQFreeBufsAud,WRITER_FREE_QUE_MAX_LEN);
    OSA_queCreate(&thrObj->bufQFullBufsAud,WRITER_FULL_QUE_MAX_LEN);    

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAud.tbl) - 1);i++)
    {
        pBuf = WRITER_bufAlloc(AUDIO);
        OSA_assert(pBuf != NULL);
        status = OSA_quePut(&thrObj->bufQFreeBufsAud,(Int32)pBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);        
    }   

    OSA_thrCreate(&thrObj->thrHandleAudio,
              WRITER_audioRecvFxn,
              AUDIO_CAPTURE_TSK_PRI,/*1,*/
              AUDIO_CAPTURE_TSK_STACK_SIZE,
              thrObj);

////sk_aud
	OSA_queCreate(&pAudPlay->bufQFreeBufsAudPlay,WRITER_AUDIO_PLAY_MAX_LEN);
	OSA_queCreate(&pAudPlay->bufQFullBufsAudPlay,WRITER_AUDIO_PLAY_MAX_LEN);	

	for (i = 0; i < (OSA_ARRAYSIZE(pAudPlay->tbl) - 1);i++)
    {
        pBuf = WRITER_bufAllocAudPlay();
        OSA_assert(pBuf != NULL);
		status = OSA_quePut(&pAudPlay->bufQFreeBufsAudPlay,(Int32)pBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);        
    }   

    OSA_thrCreate(&thrObj->thrHandleAudioPlay,
              WRITER_audioPlayFxn,
              AUDIO_CAPTURE_TSK_PRI,/*1,*/
              AUDIO_CAPTURE_TSK_STACK_SIZE,
              thrObj);
    
#endif


    OSA_queCreate(&thrObj->bufQFreeBufsVidStream, WRITER_FREE_QUE_MAX_LEN) ;    
    OSA_queCreate(&thrObj->bufQFullBufsVidStream, WRITER_FREE_QUE_MAX_LEN) ;    

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblVidStm.tbl) - 1);i++)
    {
        pBuf = WRITER_bufAllocStream(VIDEO_STREAM);
        OSA_assert(pBuf != NULL);
        status = OSA_quePut(&thrObj->bufQFreeBufsVidStream,(Int32)pBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);        
    }

    OSA_thrCreate(&thrObj->thrHandleStreaming,
                  SEND_streamFxn,
                  WRITER_VIDEOFXN_TSK_PRI,
                  WRITER_VIDEOFXN_TSK_STACK_SIZE,
                  thrObj);

    
    OSA_queCreate(&thrObj->bufQFreeBufsAudStream,WRITER_FREE_QUE_MAX_LEN);
    OSA_queCreate(&thrObj->bufQFullBufsAudStream,WRITER_FULL_QUE_MAX_LEN);  

    for (i = 0; i < (OSA_ARRAYSIZE(g_bufInfoTblAudStm.tbl) - 1);i++)
    {
        pBuf = WRITER_bufAllocStream(AUDIO_STREAM);
        OSA_assert(pBuf != NULL);
        status = OSA_quePut(&thrObj->bufQFreeBufsAudStream,(Int32)pBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);        
    }   


    OSA_thrCreate(&thrObj->thrHandleVideo,
                  WRITER_videoRecvFxn,
                  WRITER_VIDEOFXN_TSK_PRI,
                  WRITER_VIDEOFXN_TSK_STACK_SIZE,
                  thrObj);

    OSA_thrCreate(&thrObj->thrHandleRecord,
                  WRITER_recordFxn,
                  WRITER_VIDEOFXN_TSK_PRI,
                  WRITER_VIDEOFXN_TSK_STACK_SIZE,
                  thrObj);

    OSA_thrCreate(&pStatus->thrHandleMonitor,
              monitorTskEntry,
              10,
              10*1024,
              &pStatus->thrHandleMonitor);

#ifdef WRITER_MONITOR_TSK
    app_writer_dgb_start();
#endif

}

static Void WRITER_deInitThrObj(WRITER_CtrlThrObj *thrObj)
{
    ST_WRITER_STATUS *pStatus = &gStatusHDD;
	WRITER_bufInfoTbl_AudPlay *pAudPlay = &g_bufInfoTblAudPlay; 

    pStatus->exitMonitorThread = TRUE;

    thrObj->exitVideoThread     = TRUE;
    thrObj->exitAudioThread     = TRUE;
    thrObj->exitAudioPlayThread     = TRUE; //sk_aud
    thrObj->exitRecordThread    = TRUE;
    thrObj->exitStreamingThread = TRUE;


    OSA_queDelete(&thrObj->bufQFreeBufs);
    OSA_queDelete(&thrObj->bufQFullBufs);

    OSA_queDelete(&thrObj->bufQFreeBufsVidStream);
    OSA_queDelete(&thrObj->bufQFullBufsVidStream);

#ifdef AUDIO_STREAM_ENABLE	
    OSA_queDelete(&thrObj->bufQFreeBufsAud);
    OSA_queDelete(&thrObj->bufQFullBufsAud);
//sk_aud
	OSA_queDelete(&pAudPlay->bufQFreeBufsAudPlay);
	OSA_queDelete(&pAudPlay->bufQFullBufsAudPlay);	

    OSA_queDelete(&thrObj->bufQFreeBufsAudStream);
    OSA_queDelete(&thrObj->bufQFullBufsAudStream);
    OSA_thrDelete(&thrObj->thrHandleAudio);
    OSA_thrDelete(&thrObj->thrHandleAudioPlay); //sk_aud    

#endif

    OSA_thrDelete(&thrObj->thrHandleVideo);
    OSA_thrDelete(&thrObj->thrHandleRecord);
    OSA_thrDelete(&thrObj->thrHandleStreaming);
    OSA_semDelete(&thrObj->bitsInNotifySem);
    
    OSA_semDelete(&pStatus->sem_id);
}


Void WRITER_videoEncCbFxn (Ptr cbCtx)
{
    WRITER_IpcBitsCtrl *app_ipcBitsCtrl;
    
#ifdef WRITER_DEBUG 
    static Int printInterval = 0;
#endif

    OSA_assert(cbCtx = &gWRITER_ipcBitsCtrl);
    app_ipcBitsCtrl = cbCtx;

    OSA_semSignal(&app_ipcBitsCtrl->thrObj.bitsInNotifySem);
    
#ifdef WRITER_DEBUG 
    if ((printInterval % WRITER_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("MCFW_IPCBITS: Callback function:%s",__func__);
    }
    printInterval++;    
#endif  

}

int WRITER_create()
{
    VENC_CALLBACK_S callback;

    memset(&gWRITER_ctrl, 0, sizeof(gWRITER_ctrl));

#if PREREC_ENABLE
    // BKKIM pre-record
    PRELIST_init_instance(PREVRECORDDURATION_SEC_DEFAULT*1000);
#endif

    gWRITER_ctrl.createPrm.numCh = MAX_DVR_CHANNELS;

    callback.newDataAvailableCb = WRITER_videoEncCbFxn;
    
    /* Register call back with encoder */
    Venc_registerCallback(&callback, (Ptr)&gWRITER_ipcBitsCtrl);    

    WRITER_bufInit();
    WRITER_initThrObj(&gWRITER_ipcBitsCtrl.thrObj);

    return OSA_SOK;
}

void WRITER_stop()
{
    gWRITER_ipcBitsCtrl.thrObj.exitVideoThread  = TRUE;
    gWRITER_ipcBitsCtrl.thrObj.exitAudioThread  = TRUE;
    gWRITER_ipcBitsCtrl.thrObj.exitRecordThread = TRUE;
    gWRITER_ipcBitsCtrl.thrObj.exitStreamingThread = TRUE;
}

int WRITER_delete()
{
    OSA_printf("Entered:%s...",__func__);

    WRITER_bufDeInit();
    Audio_captureStop();
    
    if (Audio_playIsStart(AUDIO_PLAY_AIC3x))
        Audio_playStop(AUDIO_PLAY_AIC3x);
    
    if (Audio_playIsStart(AUDIO_PLAY_ONCHIP_HDMI))
        Audio_playStop(AUDIO_PLAY_ONCHIP_HDMI);

	WRITER_deInitThrObj(&gWRITER_ipcBitsCtrl.thrObj);

    OSA_printf("Leaving:%s...",__func__);
    
    return OSA_SOK;
}


int WRITER_isBasketOpen()
{
    return BKTREC_IsOpenBasket();
}

int WRITER_enableChSave(int chId, int enable, int event_mode, int addAudio, char *camName)
{
    int status = 0;

    /*---------------------------------------*
     *     Open basket process               *
     *---------------------------------------*/
    if((enable == 1) && (!gWRITER_ctrl.is_basket_opened))
    {
        WRITER_updateMountInfo();//MULTI-HDD SKSUNG 
        
        if(gbkt_mount_info.hddCnt > 0)
        {
            int curIdx = gbkt_mount_info.curHddIdx;
            status = BKTREC_open(gbkt_mount_info.hddInfo[curIdx].mountDiskPath);
            if(status == BKT_ERR)
            {
                gWRITER_ctrl.is_basket_opened = 0;
                WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_OPEN_FAIL, NULL);   
                eprintf("Error open BASKET-DB(\"%s\") for Record !!\n", gbkt_mount_info.hddInfo[curIdx].mountDiskPath);

            }
            else if(status == BKT_FULL)
            {
                gWRITER_ctrl.is_basket_opened = 0;
                WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_HDD_FULL, NULL);    
                eprintf("HDD FULL BASKET-DB(\"%s\") for Record !!\n", gbkt_mount_info.hddInfo[curIdx].mountDiskPath);
            
            }
            else
            {
                gWRITER_ctrl.is_basket_opened = 1;
                status = 1;
                dprintf("recorder_start_rec... basket open success..\n") ;
            }
        }
        else
        {
            WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_NOT_MOUNTED, NULL);
        }
    }

    if(gWRITER_ctrl.is_basket_opened)
    {
        dprintf("WRITER_enableChSave() ch[%02d] enable[%d]====\n", chId, enable);
        gWRITER_ctrl.chInfo[chId].enableChSave  = enable;
        gWRITER_ctrl.chInfo[chId].event_mode    = event_mode;
        gWRITER_ctrl.chInfo[chId].enableAudio   = addAudio;
        if(strlen(camName))
        {
            strncpy(gWRITER_ctrl.chInfo[chId].camName, camName, 16);
        }

        status = 1;

#if PREREC_ENABLE // BKKIM
        {
            int c, pre_chk_cnt=0;
            for(c=0;c<MAX_CH;c++){
                if( gWRITER_ctrl.chInfo[c].presec > 0 ) {
                    gWRITER_ctrl.bPreEnabled = TRUE;
                    break;
                }
                pre_chk_cnt++;
            }

            if(pre_chk_cnt == MAX_CH) {
                gWRITER_ctrl.bPreEnabled = FALSE;
                PRELIST_set_max_duration(0);
            }
        }
#endif

    }

    return status;
}

int WRITER_fileSaveExit()
{
    int i;
    for(i = 0; i < BKT_MAX_CHANNEL; i++)
    {
        gWRITER_ctrl.chInfo[i].fileSaveState = WRITER_FILE_SAVE_STATE_IDLE;
        gWRITER_ctrl.chInfo[i].firstKeyframe = FALSE;
    }

    if( BKTREC_exit(SF_STOP, BKT_REC_CLOSE_BUF_FLUSH) == BKT_ERR)
    {
        eprintf("BKTREC_exit() Error !!!!\n");
    }
    else
        gWRITER_ctrl.is_basket_opened = 0;

    return 1;
}

int WRITER_changeCamName(int chId, char* camName)
{
    if(chId >= 0 && chId < BKT_MAX_CHANNEL)
    {
        if(strlen(camName))
        {
            strncpy(gWRITER_ctrl.chInfo[chId].camName, camName, 16);
        }
    }

    return 1;
}

void WRITER_updateMountInfo()
{
    BKTREC_getTargetDisk();
}

void WRITER_setMsgPipe(void* msg_pipe_id)
{
    WRITER_Ctrl* pCtrl = &gWRITER_ctrl;

    if(msg_pipe_id == NULL)
        return;

    pCtrl->msg_pipe_id = msg_pipe_id;
}

void WRITER_setInitDone()
{
    if(gCaptureInfo.chStatus[0].frameWidth == 0 || gCaptureInfo.chStatus[0].frameHeight == 0)
    {
        gWRITER_ctrl.createPrm.frameWidth   = 704;
        gWRITER_ctrl.createPrm.frameHeight  = 480;
    }
    else
    {
        gWRITER_ctrl.createPrm.frameWidth   = gCaptureInfo.chStatus[0].frameWidth;
        gWRITER_ctrl.createPrm.frameHeight  = gCaptureInfo.chStatus[0].frameHeight;
    }
    
    writerInitDone = 1;
}

#if PREREC_ENABLE
// BKKIM, set pre record time
void WRITER_setPreRecSec(int ch, int presec)
{
    printf("%s -- ch:%02d, presec:%d\n", __FUNCTION__, ch, presec);

    gWRITER_ctrl.chInfo[ch].presec = presec; // enable(1~10) or disable(0)
    PRELIST_set_max_duration(presec*1000);

    // check all channel disable
    PRELIST_check_enable(presec);
}
#endif

// EOF app_writer.c
