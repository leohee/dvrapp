/*******************************************************************************
*                             INCLUDE FILES
*******************************************************************************/
#ifndef _OMX_COMMON_DEFINE_H_
#define _OMX_COMMON_DEFINE_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "app_manager.h"
#include "global.h"
#include "ti_media_std.h"

#define OMX_TEST_SLEEPDURN_PER_ITERATION (1000)

#define NUM_DISPLAYS			(2)
#define OMX_NUM_DEIS			(2)
#define OMX_NUM_SINKS			(0)
#define OMX_NUM_SRCS			(0)
#define ENCODE_CHANNEL			(16)
#define DECODE_CHANNEL			(16)
#define NUM_CHANNEL				(16)
#define DEIM_CHANNEL			(8)
#define DEIH_CHANNEL			(8)
#define SC_CHANNEL				(16)

typedef struct ST_CAPTURE_INFO
{
	Int chCount;
	SOURCE_CH_STATUS_S chStatus[MAX_CH];
}ST_CAPTURE_INFO;



// ==================================================================================================================================================================
// /* Other Function Prototypes */
// ==================================================================================================================================================================

void platform_init();
void platform_deinit();


// ==================================================================================================================================================================
// /* DVR Function Prototypes */
// ==================================================================================================================================================================

/* DVR_Init() will Initialize OMX_core, Load the M3 binaries, Cretae RCM servers, Initialize frameQ structures */
void DVR_Init();

/* DVR_Deinit() will Deinitialize OMX_core*/
void DVR_Deinit();


// ===================================================
// /* DVR setting initialize */
// ===================================================
//void DVR_InitDisplayProperty(ST_DISPLAY_PROPERTY *pDspProperty);
int DVR_SetBrightness(int nChannelIndex, int ibrightness);
int DVR_SetContrast(int nChannelIndex, int icontrast);
int DVR_SetSaturation(int nChannelIndex, int isaturation);
int DVR_SetHue(int nChannelIndex, int ihue);
int DVR_SetSharpness(int nChannelIndex, int isharpness);


int DVR_GetBrightness(int nChannelIndex, int *ibrightness);
int DVR_GetContrast(int nChannelIndex, int *icontrast);
int DVR_GetSaturation(int nChannelIndex, int *isaturation);
int DVR_GetHue(int nChannelIndex, int *ihue);
int DVR_GetSharpness(int nChannelIndex, int *isharpness);
int DVR_GetNoiseReduction(int nChannelIndex, int *inoisereduction);
int DVR_GetDeInterlace(int nChannelIndex, int *ideinterlace);

int DVR_SetCameraEnable(int chId, int bEnable);
int DVR_SetCameraLayout(int iOutput, int nStartChannelIndex, int iLayoutMode, int m_iCameraSelectId);
int DVR_SetCovert(int nChannelIndex, int bEnable);
int DVR_GetSourceStatus();
int DVR_SetBitrateType(int chId, int bitrateType);	//VBR:0 CBR:1
int DVR_SetDisplayRes(int displayId, int resType);
int DVR_SetSpotChannel(int chId);
int DVR_SwitchDisplay(int mainDspId, int subDspId, int spotDspId, int mainResolution, int subResolution, int spotResolution);
int DVR_displaySwitchChn(UInt32 devId, UInt32 startChId);

int DVR_setPlaybackChnMap(Bool bSet);

int DVR_setCaptureResolution(int chId, int resId);

int DVR_setAudioInputParams(int iSampleRate, int iVolume);
int DVR_setAudioAIC3volume(int iVolume);
int DVR_setAudioOutputDev(int devId) ;

unsigned int DVR_GetVideoLossStatus();

/* Create Components, Tunnel them, Move state to Idle then execute. DVR_Init() has to be called before calling DVR_StartSystem */
int DVR_StartSystem(int isNTSC, int main_vout, int sub_vout, int venc_mode);

/* Move components back to Idle, then Loaded. Free component handles */
int DVR_StopSystem();


//====================================================
// /* DVR Recording */
//===================================================

/* System starts reccording bitstream to SATA */
int DVR_SetRecording(int channel, int enableRec, int event_mode, int addAudio, char* camName);


int DVR_SetRecordingType(int iRecycle) ;
int DVR_changeCamName(int channel, char* camName);
void DVR_closeBasket();

void DVR_setWriterMsgPipe(void* pipe_id); ////MULTI-HDD SKSUNG////


long DVR_GetRecDays(long t, int* pRecDayTBL);
long DVR_GetRecHour(int ch, long t, int* pRecHourTBL) ;
long DVR_GetLastRecTime(void);

/* Getting Playback time in playback */

int DVR_GetCurPlaybackTime(struct tm *tp) ;

int DVR_set_encoder_request_key(/*TunnelTestCtxt *pHandleParam,*/ int ch);
int DVR_set_encoder_resolution(/*TunnelTestCtxt *pHandleParam,*/ int ch, int width, int height);
int DVR_set_encoder_fps(int ch, int fps);
int DVR_set_encoder_bps(int ch, int bps);
int DVR_set_encoder_iframeinterval(int ch, int value);
int DVR_set_encoder_codectype(int ch, int codectype);

int DVR_get_encoder_request_key(/*TunnelTestCtxt *pHandleParam,*/ int ch, int *pIsEnabled);
int DVR_get_encoder_bps(int ch, int *pBps);
int DVR_get_encoder_fps(int ch, int *pFps);
int DVR_get_encoder_iframeinterval(int ch, int *value);
int DVR_get_encoder_codectype(int ch, int *codectype);
int DVR_get_encoder_resolution(/*TunnelTestCtxt *pHandleParam,*/ int ch, int *pWidth, int *pHeight);

/* Switching mosaic layout */
int DVR_SetMosaicLayout(int layoutMode);

/* Switching Playback mosaic layout */
int DVR_SetPlaybackMosaicLayout(int layoutMode);

/* Basket  */
int DVR_Basket_Create(char *diskpath) ;
int DVR_BasketInfo(long *bkt_count, long *bkt_size) ;

//MULTI-HDD SKSUNG//
int DVR_FormatHDD(char* mntpath, char* devpath, int hddstatus);
int DVR_GetDiskInfo(dvr_disk_info_t* idisk);
int DVR_GetRecDiskSize(unsigned long *total, unsigned long *used);

//Cam-Property PIP//
void DVR_startCamProperty(int selectedCh, int startX, int startY, int width, int height);
void DVR_endCamProperty();
void DVR_selCamPropCh(int nCh);


/* Start RTSP Streaming */
void DVR_startRTSP();

/* Stop RTSP Streaming */
void DVR_stopRTSP() ;


void DVR_VdisStartDrv(char displayId);

void DVR_VdisStopDrv(char displayId);


/* Get old playback mosaic info */
void DVR_getPbMosaicInfo(int *numWin, int* chnMap);


/************************************************************/
/*below function is not yet implemeted                      */
/************************************************************/

/* System pause Playback from SATA */
int DVR_PausePlayback();

/* System Resumes Playback from SATA */
int DVR_ResumePlayback();


/* Terminate and Restart IL Client */
int DVR_RestartSystem();

/* Reset to Default parameters */
int DVR_ResetSystem();

///////////////////////////////////////////////////////////////////////////////
#if PREREC_ENABLE // BKKIM pre-record
/**
 @brief settings of pre-record
 @author BKKIM
 @param[in] channel     channel
 @param[in]	presec  pre-record enable. if 0, disabled. 1:min, 10:max
 */
void DVR_SetPreRecDuration(int channel, int presec);
#endif
///////////////////////////////////////////////////////////////////////////////

int DVR_FBStart();
int DVR_FBScale(int devId, UInt32 inWidth, UInt32 inHeight, UInt32 outWidth, UInt32 outHeight);

void DVR_setOff_GRPX(void) ;
///////////////////////////////////////////////////////////////////////////////
#endif//_OMX_COMMON_DEFINE_H_
///////////////////////////////////////////////////////////////////////////////
