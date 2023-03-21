#ifndef _AV_SERVER_H_
#define _AV_SERVER_H_

#include <avserver_config.h>
#include <avserver_thr.h>
#include <video.h>
#include <audio.h>

#include <avserver_api.h>


// from AVSERVER Main to other TSKs
#define AVSERVER_CMD_CREATE    (0x300)
#define AVSERVER_CMD_DELETE    (0x301)
#define AVSERVER_CMD_START     (0x302)
#define AVSERVER_CMD_NEW_VDATA  (0x303)
#define AVSERVER_CMD_NEW_ADATA  (0x304)

// from UI thread to AVSERVER Main
#define AVSERVER_MAIN_CMD_START  (0x400)
#define AVSERVER_MAIN_CMD_STOP   (0x401)

// AVSERVER Main State's
#define AVSERVER_MAIN_STATE_IDLE       (0)
#define AVSERVER_MAIN_STATE_RUNNING    (1)

#define ALG_VID_CODEC_H264     0
#define ALG_VID_CODEC_MPEG4    1
#define ALG_VID_CODEC_MJPEG    2
#define ALG_VID_CODEC_VNF      3
#define ALG_VID_CODEC_NONE     (-1)

#define ALG_AUD_CODEC_G711_RTSP     0
#define WIS_STREAMER "./open_bin/wis-streamer &"

//#define AVSERVER_DEBUG_API
//#define AVSERVER_DEBUG_MAIN_THR
//#define AVSERVER_DEBUG_VIDEO_STREAM_THR


typedef struct {

  OSA_TskHndl   mainTsk;
  OSA_MbxHndl   uiMbx;
  OSA_MutexHndl lockMutex;
  
} AVSERVER_Ctrl;


typedef struct {
	int type ;
	int chId ;
} AVSERVER_prm ;


extern AVSERVER_Ctrl gAVSERVER_ctrl;
extern AVSERVER_Config gAVSERVER_config;
extern VIDEO_Ctrl gVIDEO_ctrl;
extern AUDIO_Ctrl gAUDIO_ctrl;

int AVSERVER_bufGetNextTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl, OSA_TskHndl **pTskHndl) ;
int AVSERVER_AudbufGetNextTskInfo(int tskId, int streamId, OSA_BufHndl **pBufHndl, OSA_TskHndl **pTskHndl) ;

OSA_BufInfo *AVSERVER_bufGetEmpty(int tskId, int streamId, int *bufId, int timeout);
OSA_BufInfo *AVSERVER_bufPutFull(int tskId, int streamId, int bufId);

OSA_BufInfo *AVSERVER_bufGetFull(int tskId, int streamId, int *bufId, int timeout);
OSA_BufInfo *AVSERVER_bufPutEmpty(int tskId, int streamId, int bufId);


OSA_BufInfo *AVSERVER_AudbufGetEmpty(int tskId, int streamId, int *bufId, int timeout);
OSA_BufInfo *AVSERVER_AudbufPutFull(int tskId, int streamId, int bufId);

OSA_BufInfo *AVSERVER_AudbufGetFull(int tskId, int streamId, int *bufId, int timeout);
OSA_BufInfo *AVSERVER_AudbufPutEmpty(int tskId, int streamId, int bufId);

int AVSERVER_bufAlloc();
int AVSERVER_bufFree();

int AVSERVER_mainCreate();
int AVSERVER_mainDelete();

#endif  /*  _AV_SERVER_H_  */

