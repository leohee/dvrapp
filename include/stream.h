/** ===========================================================================
* @file stream.h
*
* @path $(IPNCPATH)\multimedia\encode_stream\stream
*
* @desc
* .
* Copyright (c) Appro Photoelectron Inc.  2008
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied
*
* =========================================================================== */

#ifndef _STREAM_H_
#define _STREAM_H_


#include <rendezvous.h>
#include <mem_mng.h>
#include <Msg_Def.h>
#include <cache_mng.h>
#include <gop_lock.h>
#include <semaphore_util.h>

#include "global.h"

enum{
	STREAM_FAILURE   = -1,
	STREAM_SUCCESS   = 0
};

enum{
	STREAM_H264_1 = 0,
	STREAM_H264_2,
	STREAM_H264_3,
	STREAM_H264_4,
 //RTSP_EXT_CHANNEL	
	STREAM_H264_5,
	STREAM_H264_6,
 	STREAM_H264_7,
	STREAM_H264_8,

	STREAM_H264_9,
	STREAM_H264_10,
	STREAM_H264_11,
	STREAM_H264_12,
	STREAM_H264_13,
	STREAM_H264_14,
	STREAM_H264_15,
	STREAM_H264_16,
	
	STREAM_AUDIO_1,
	STREAM_AUDIO_2,
	STREAM_AUDIO_3,
	STREAM_AUDIO_4,
	STREAM_AUDIO_5,
	STREAM_AUDIO_6,
	STREAM_AUDIO_7,
	STREAM_AUDIO_8,
	STREAM_AUDIO_9,
	STREAM_AUDIO_10,
	STREAM_AUDIO_11,
	STREAM_AUDIO_12,
	STREAM_AUDIO_13,
	STREAM_AUDIO_14,
	STREAM_AUDIO_15,
	STREAM_AUDIO_16,
};

enum{
	STREAM_SEM_VIDEO_CH1 = 0,
	STREAM_SEM_VIDEO_CH2,
	STREAM_SEM_VIDEO_CH3,
	STREAM_SEM_VIDEO_CH4,
 //RTSP_EXT_CHANNEL	
	STREAM_SEM_VIDEO_CH5,
	STREAM_SEM_VIDEO_CH6,
	STREAM_SEM_VIDEO_CH7,
	STREAM_SEM_VIDEO_CH8,

	STREAM_SEM_VIDEO_CH9,
	STREAM_SEM_VIDEO_CH10,
	STREAM_SEM_VIDEO_CH11,
	STREAM_SEM_VIDEO_CH12,
	STREAM_SEM_VIDEO_CH13,
	STREAM_SEM_VIDEO_CH14,
	STREAM_SEM_VIDEO_CH15,
	STREAM_SEM_VIDEO_CH16,
	
	STREAM_SEM_AUDIO_1,
	STREAM_SEM_AUDIO_2,
	STREAM_SEM_AUDIO_3,
	STREAM_SEM_AUDIO_4,
	STREAM_SEM_AUDIO_5,
	STREAM_SEM_AUDIO_6,
	STREAM_SEM_AUDIO_7,
	STREAM_SEM_AUDIO_8,
	STREAM_SEM_AUDIO_9,
	STREAM_SEM_AUDIO_10,
	STREAM_SEM_AUDIO_11,
	STREAM_SEM_AUDIO_12,
	STREAM_SEM_AUDIO_13,
	STREAM_SEM_AUDIO_14,
	STREAM_SEM_AUDIO_15,
	STREAM_SEM_AUDIO_16,
	STREAM_SEM_MEMCPY,
	STREAM_SEM_CACHECPY,
	STREAM_SEM_GOP,
	STREAM_SEM_NUM
};

enum{
	STREAM_FEATURE_BIT_RATE1 = 0,
	STREAM_FEATURE_BIT_RATE2,
	STREAM_FEATURE_JPG_QUALITY,
	STREAM_FEATURE_M41_FRAMERATE,
	STREAM_FEATURE_M42_FRAMERATE,
	STREAM_FEATURE_JPG_FRAMERATE,
	STREAM_FEATURE_BLC,
	STREAM_FEATURE_SATURATION,
	STREAM_FEATURE_AWB_MODE,
	STREAM_FEATURE_AE_MODE,
	STREAM_FEATURE_BRIGHTNESS,
	STREAM_FEATURE_CONTRAST,
	STREAM_FEATURE_SHARPNESS,
	STREAM_FEATURE_AEW_TYPE,
	STREAM_FEATURE_ENV_50_60HZ,
	STREAM_FEATURE_BINNING_SKIP,
	STREAM_FEATURE_TSTAMPENABLE,
	STREAM_FEATURE_PTZ,
	STREAM_FEATURE_MOTION,
	STREAM_FEATURE_M41_ADV_FEATURE,
	STREAM_FEATURE_M42_ADV_FEATURE,
	STREAM_FEATURE_JPG_ADV_FEATURE,
	STREAM_FEATURE_ROICFG,
	STREAM_FEATURE_OSDTEXT_EN,
	STREAM_FEATURE_HIST_EN,
	STREAM_FEATURE_GBCE_EN,
	STREAM_FEATURE_EVALMSG_EN,
	STREAM_FEATURE_OSDLOGO_EN,
	STREAM_FEATURE_OSDTEXT,
	STREAM_FEATURE_FACE_SETUP,
	STREAM_FEATURE_NUM
};

enum{
	STREAM_VIDEO_1 = 0,
	STREAM_VIDEO_2,
	STREAM_VIDEO_3,
	STREAM_VIDEO_4,
 //RTSP_EXT_CHANNEL	
	STREAM_VIDEO_5,
	STREAM_VIDEO_6,
	STREAM_VIDEO_7,
 	STREAM_VIDEO_8,

	STREAM_VIDEO_9,
	STREAM_VIDEO_10,
	STREAM_VIDEO_11,
	STREAM_VIDEO_12,
	STREAM_VIDEO_13,
	STREAM_VIDEO_14,
	STREAM_VIDEO_15,
	STREAM_VIDEO_16,
	
	STREAM_VIDEO_NUM
};

enum{
	STREAM_EXT_MP4_CIF=0,
	STREAM_EXT_JPG,
	STREAM_EXT_NUM
};

typedef struct _ApproMotionPrm{
	int bMotionEnable;
	int bMotionCEnale;
	int MotionLevel;
	int MotionCValue;
	int MotionBlock;
}ApproMotionPrm;

typedef struct _STREAM_SET{
	int				ImageWidth[STREAM_VIDEO_NUM];
	int				ImageHeight[STREAM_VIDEO_NUM];
	int				ImageWidth_Ext[STREAM_EXT_NUM];
	int				ImageHeight_Ext[STREAM_EXT_NUM];
	int				JpgQuality;
	int				Mpeg4Quality[STREAM_VIDEO_NUM];
	int				Mem_layout;
}	STREAM_SET;


typedef struct _STREAM_PARM{
	MEM_MNG_INFO 	MemInfo;
	int 			MemMngSemId[STREAM_SEM_NUM];
	pthread_t 		threadControl;

	Rendezvous_Obj	objRv[GOP_INDEX_NUM];

	int 			checkNewFrame[GOP_INDEX_NUM];
	int 			lockNewFrame[GOP_INDEX_NUM];
	int				IsQuit;

	int				qid;

	int				ImageWidth[STREAM_VIDEO_NUM];
	int				ImageHeight[STREAM_VIDEO_NUM];
	int				ImageWidth_Ext[STREAM_EXT_NUM];
	int				ImageHeight_Ext[STREAM_EXT_NUM];
	int				JpgQuality;
	int				Mpeg4Quality[STREAM_VIDEO_NUM];
}	STREAM_PARM;
STREAM_PARM *stream_get_handle(void);
int stream_init( STREAM_PARM *pParm , STREAM_SET *pSet);
int stream_write(void *pAddr, int size, int frame_type ,int stream_type, unsigned int timestamp ,STREAM_PARM *pParm);
int stream_end(STREAM_PARM *pParm);

void Motion_setparm(ApproMotionPrm* pMotionPrm);


#endif

