#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <osa_pipe.h>

#include "app_manager.h"
#include "lib_dvr_app.h"
#include "app_front_if.h"
#include "app_dio_if.h"
#include "app_ptz_if.h"
#include "app_util.h"
#include "global.h"
#include "app_avibackup.h"
#include "app_bskbackup.h"
#include "app_playback.h"
#include "disk_control.h"
#include "util_func.h"
#include "avi.h"
#include "app_motion.h"
#include "app_audio.h"

#define NORMAL_MODE     0
#define STREAM_MODE     1

#define ENC_BPS_DEFAULT 1200000

static int gRecCount;
PROFILE gInitSettings;

extern ST_CAPTURE_INFO gCaptureInfo;
AUDIO_STATE gAudio_state ; // hwjun

/*-------------------*/
/* Initialize system */
/*-------------------*/

void LIB816x_initDisplayInfo(ST_DISPLAY_PROPERTY *pDspProperty)
{
//  DVR_InitDisplayProperty(pDspProperty);
    return;
}


int LIB816x_startSystem(int disp_main, int disp_sub, int bNtsc, int vmode)
{
    int rv = 0;
    int i;

    VIDEOMOTION_CreatePrm videomotionCreatePrm ;

    videomotionCreatePrm.numCh       = MAX_CH;
    videomotionCreatePrm.frameWidth  = 704 ;
    videomotionCreatePrm.frameHeight = 576;
    if(bNtsc == CAM_NTSC)
    {
        videomotionCreatePrm.frameHeight = 480;
    }
	DVR_SetRecordingType(gInitSettings.storage.iHddRecylceModeOn);

    rv = VIDEO_motionCreate(&videomotionCreatePrm) ;

    (void) app_playback_init();
//	(void) set_rtsp_status(TRUE);
    (void) DVR_StartSystem(bNtsc, disp_main, disp_sub, vmode);

	// BKKIM
	app_tplay_save_layout(gInitSettings.layout.iSecondLayoutMode, 0);

    get_hw_info(gMac0, gMac1, gHWver);
    (void) DVR_Init();
#if 0
    printf("\n\napp: lib816x_statsystem (%s)\n\n",LIB_MAIN_VERSION);

    (void) set_rtsp_status(TRUE);

    printf("app: lib816x_statsystem ... done\n");
#endif
    app_backup_init() ;

    for(i=0;i<MAX_SENSOR; i++)
    {
        LIB816x_setSensor(i,gInitSettings.sensor[i].iSensorOn,gInitSettings.sensor[i].iSensorType);
    }
    for(i=0;i<MAX_ALARM;i++)
    {
        LIB816x_setAlarm(i,gInitSettings.alarm[i].iAlarmOn,gInitSettings.alarm[i].iAlarmType,gInitSettings.alarm[i].iDelayTime);
    }

    for(i=0;i<16;i++)
    {
        LIB816x_setMotion(i,gInitSettings.camera[i].motion.iMotionOn, gInitSettings.camera[i].motion.iSensitivity, gInitSettings.camera[i].motion.motiontable);
		LIB816x_setVideoCodecType(i,gInitSettings.camera[i].camrec.iCodecType);
		LIB816x_setAudioCodecType(i,gInitSettings.camera[i].camrec.iAudioCodecType);
		LIB816x_setRecDuration(i,gInitSettings.camera[i].recordduration.iPrevRecordOn, gInitSettings.camera[i].recordduration.iPrevRecordDuration);
        //audio
//		LIB816x_setAudioInput(i,gInitSettings.audioin.audioinch[i].iAudioInOn);
	}
	LIB816x_setAudioOutputDev(gInitSettings.audioout.iDevId);
//	LIB816x_setAudioInputParams(gInitSettings.audioin.iSampleRate, gInitSettings.audioin.iVolume);
//	LIB816x_setAudioOutput(gInitSettings.audioout.iAudioOutOn, gInitSettings.audioout.iVolume, gInitSettings.audioout.iSelectAudioIn);
    return rv;
}

int LIB816x_stopSystem(void)
{
    int rv = 0;

    (void) DVR_StopSystem();

    return rv;
}
/*-------------------*/
/*  System settings  */
/*-------------------*/
int LIB816x_setDisplayLayout(int mode, int nChannelIndex)
{

	int ret = 0;
	if((gInitSettings.layout.iLayoutMode != mode) || (nChannelIndex ==1))
	{
		ret = DVR_SetMosaicLayout(mode);
		gInitSettings.layout.iLayoutMode = mode;
	}
	return ret;
}

int LIB816x_setPlaybackDisplayLayout(int mode,  int nChannelIndex)
{
	int ret = 0;
	if((gInitSettings.layout.iSecondLayoutMode != mode) || (nChannelIndex == 1))
	{
	// BKKIM
#if 1
	app_tplay_save_layout(mode, 0);
#endif

    	ret = DVR_SetPlaybackMosaicLayout(mode);
		gInitSettings.layout.iSecondLayoutMode = mode;
	}

	return ret;
}

/*-----------------------*/
/*  Record Start & Stop  */
/*-----------------------*/
int LIB816x_setRecChannel(int chId, int enableRec, int eventMode, int addAudio, char* camName)
{
    int ret = 0;

        if(enableRec == 0 && gRecCount > 0)
        {
            gRecCount--;
            if(gRecCount == 0)
            {
                dvr_send_front(RECORD_STOP);
            }
        }
    ret = DVR_SetRecording(chId, enableRec, eventMode, addAudio, camName);

    if(ret)
    {
        if(enableRec == 1 && gRecCount <= MAX_CH)
        {
            gRecCount++;
            if(gRecCount == 1)
            {
                dvr_send_front(RECORD_START);
            }
        }

    }

    return ret;
}

int LIB816x_changeCamName(int chId, char* camName)
{
    return DVR_changeCamName(chId, camName);
}

//  iRecycle=0:Recycle,1:Once
int LIB816x_setRecordingType(int iRecycle)
{
    return DVR_SetRecordingType(iRecycle);
}

/*------------------------*/
/*  Playback Start & Stop */
/*------------------------*/
int LIB816x_initPlayback_x(void)
{
    int rv=0;

    rv = app_playback_init();
    dprintf("app: LIB816x_initPlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_startPlayback_x(int ch_bitmask, struct tm *ptm)
{
    int rv=0;

    time_t t;
    struct tm tv ;

    memcpy(&tv, ptm, sizeof(struct tm));
    tv.tm_year = tv.tm_year - 1900;

	if(gInitSettings.layout.iMainOutput == gInitSettings.layout.iSubOutput)
	{
		//DVR_displaySwitchChn(gInitSettings.layout.iSubOutput, MAX_DVR_CHANNELS);
		//gInitSettings.layout.iSecondLayoutMode = gInitSettings.layout.iLayoutMode;// now set live and play to same mode, should be change later
		//DVR_setPlaybackChnMap(0);
		LIB816x_setPlaybackDisplayLayout(gInitSettings.layout.iSecondLayoutMode,1);  // 1 ---force to change
	}

//	else
//		DVR_setPlaybackChnMap(1);	//chnMap VALID channel num.
	
    t = mktime(&tv);
    rv = app_playback_start(ch_bitmask, t);
	//DVR_setOff_GRPX() ;

    dprintf("app: LIB816x_startPlayback_x ... %d\n",rv);

    return rv;


}

int LIB816x_stopPlayback_x(void)
{
    int rv=0;

 	rv = app_playback_stop();
	if(gInitSettings.layout.iMainOutput == gInitSettings.layout.iSubOutput)
		//DVR_displaySwitchChn(gInitSettings.layout.iMainOutput, 0);	
		LIB816x_setDisplayLayout(gInitSettings.layout.iLayoutMode,1);  // 1 ---force to change
	else
	DVR_setPlaybackChnMap(0);	//chnMap INVALID

   
    dprintf("app: LIB816x_stopPlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_jumpPlayback_x(int ch_bitmask, struct tm *ptm)
{
    int rv=0;

    time_t t;
    struct tm tv ;

    memcpy(&tv, ptm, sizeof(struct tm));
    tv.tm_year = tv.tm_year - 1900;

    t = mktime(&tv);
    rv = app_playback_jump(ch_bitmask, t);
    dprintf("app: LIB816x_jumpPlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_pausePlayback_x(void)
{
    int rv=0;

    rv = app_playback_pause();
    dprintf("app: LIB816x_pausePlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_restartPlayback_x(int ch_bitmask)
{
    int rv=0;

    rv = app_playback_restart(ch_bitmask);
    dprintf("app: LIB816x_restartPlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_fastBackward_x(int ch_bitmask)
{
	int rv=0;

	rv = app_playback_fastbackward(ch_bitmask);
	dprintf("app: LIB816x_fastBackward_x ... %d\n",rv);

	return rv;
}

int LIB816x_fastforward_x(int ch_bitmask)
{
	int rv=0;
	
	rv = app_playback_fastforward(ch_bitmask);
	dprintf("app: LIB816x_fastforward_x ... %d\n",rv);
    return rv;
}

int LIB816x_stepPlayback_x(int ch_bitmask, int bReverse)
{
    int rv=0;

    rv = app_playback_step(ch_bitmask, bReverse);
    dprintf("app: LIB816x_stepPlayback_x ... %d\n",rv);

    return rv;
}

int LIB816x_setPlaybackProperty_x(int cmd, int ch_bitmask, int value, void *pData)
{
    int rv=0;

    rv = app_playback_ctrl(cmd, ch_bitmask, value, pData);
    dprintf("app: LIB816x_setPlaybackProperty_x ... %d\n",rv);

    return rv;
}

int LIB816x_getCurPlaybackTime(struct tm *tp)
{
    int ret = 0 ;
	ret = app_playback_get_cur_sec(tp);
    return ret;
}

/*-------------------------*/
/* Get Recording Date      */
/*-------------------------*/
long LIB816x_GetRecDays(struct tm t, int* pRecDayTBL)
{

    struct tm tv ;
    memcpy(&tv, &t, sizeof(struct tm)) ;
    tv.tm_year = tv.tm_year - 1900 ;

    time_t ltime=time(NULL);
    ltime = mktime(&tv);

    return DVR_GetRecDays(ltime, pRecDayTBL);

}

long LIB816x_GetRecHour(int ch, struct tm t, int* pRecHourTBL)
{

    struct tm tv ;
    memcpy(&tv, &t, sizeof(struct tm)) ;

    tv.tm_year = tv.tm_year - 1900 ;
    time_t ltime=time(NULL);
    ltime = mktime(&tv);

    return DVR_GetRecHour(ch, ltime, pRecHourTBL);
}

long LIB816x_GetLastRecTime(void)
{
    return DVR_GetLastRecTime();
}

int LIB816x_HddFormat(char *mntpath, char *devpath, int hddstatus)
{
    return DVR_FormatHDD(mntpath, devpath, hddstatus);
}

/*----------------------------------------------------------------------------
 System Info
-----------------------------------------------------------------------------*/
int LIB816x_sys_info(char *mac0, char *mac1, char *hwver)
{
    int ret=0;
//	printf("####LIB816x_sys_info() \n");

	if(mac0==NULL || mac1==NULL || hwver==NULL)
        return -1;

    ret = get_hw_info(mac0, mac1, hwver);

    return ret;
}

int LIB816x_net_info(int set, int devno, void *net_info)
{
    int ret=0;

    dvr_net_info_t *inet = (dvr_net_info_t *)net_info;

    if(set) {
        ret = set_net_info(devno, inet);
    } else {
        ret = get_net_info(devno, inet);
    }

    return ret;
}

///MULTI-HDD SKSUNG////
int LIB816x_disk_info(void *disk_info)
{
    dvr_disk_info_t *idisk = (dvr_disk_info_t *)disk_info;
//  printf("####LIB816x_disk_info() \n");

    return DVR_GetDiskInfo(idisk);
}

int LIB816x_disk_size(char *mntpath, unsigned long *total, unsigned long *used)
{
    int ret=0;

    ret = get_dvr_disk_size(mntpath, total, used);
    if(ret<0)
        return -1;

    return 0;
}

////MULTI-HDD SKSUNG////
int LIB816x_rec_disk_size(unsigned long *total, unsigned long *used)
{
//  printf("####LIB816x_rec_disk_size() \n");

    return DVR_GetRecDiskSize(total, used);
}


int LIB816x_BasketCreate(char *mntpath)
{
    return DVR_Basket_Create(mntpath);
}

int LIB816x_BasketInfo(long *bkt_count, long *bkt_size)
{
//  printf("####LIB816x_BasketInfo() \n");

    return DVR_BasketInfo(bkt_count, bkt_size) ;
}

void LIB816x_systemReboot(void)
{
    DVR_closeBasket();
    dvr_send_front(SYSTEM_REBOOT);
}

/*----------------------------------------------------------------------------
    Encoder Set/Get Property
-----------------------------------------------------------------------------*/
int LIB816x_set_encoder_property(int type, int channel_bitmask, int *pValue)
{
    int rv = 0, ch;

    switch (type)
    {
		case LIB_ENC_SET_FRAMERATE:
			for (ch = 0; ch < MAX_CH; ch++)
				if ((channel_bitmask & (0x01 << ch)) && (pValue[ch] != gInitSettings.camera[ch].camrec.iFps))
				{
					DVR_set_encoder_fps(ch, pValue[ch]);
					gInitSettings.camera[ch].camrec.iFps = pValue[ch];
				}
			break;
		case LIB_ENC_SET_BITRATE:
			for (ch = 0; ch < MAX_CH; ch++)
				if ((channel_bitmask & (0x01 << ch))&& (pValue[ch] != (gInitSettings.camera[ch].camrec.ikbps*1000)))
				{
					DVR_set_encoder_bps(ch, pValue[ch]);
					gInitSettings.camera[ch].camrec.ikbps = pValue[ch]/1000;
				}
			break;
		case LIB_ENC_SET_I_FRAME_INTERVAL:
			for (ch = 0; ch < MAX_CH; ch++)
				if ((channel_bitmask & (0x01 << ch)) && pValue[ch] != gInitSettings.camera[ch].camrec.iIFrameInterval)
				{
					DVR_set_encoder_iframeinterval(ch, pValue[ch]);
					gInitSettings.camera[ch].camrec.iIFrameInterval = pValue[ch];
				}
			break;	
		default:
			eprintf("set_property invaild type %d\n",type);
			break;
	}

	return rv;
}

int LIB816x_get_encoder_property(int type, int channel_bitmask, int *pValue)
{

    int rv = 0, ch, width, height;

    switch (type)
    {
        case LIB_ENC_GET_FRAMERATE:
            for (ch = 0; ch < MAX_CH; ch++)
                if (channel_bitmask & (0x01 << ch))
                    DVR_get_encoder_fps(ch, &pValue[ch]);
            break;
        case LIB_ENC_GET_BITRATE:
            for (ch = 0; ch < MAX_CH; ch++)
                if (channel_bitmask & (0x01 << ch))
                    DVR_get_encoder_bps(ch, &pValue[ch]);
            break;
        case LIB_ENC_GET_I_FRAME_INTERVAL:
            for (ch = 0; ch < MAX_CH; ch++)
                if (channel_bitmask & (0x01 << ch))
                    DVR_get_encoder_iframeinterval(ch, &pValue[ch]);
            break;

        case LIB_ENC_GET_RESOLUTION:
            for (ch = 0; ch < MAX_CH; ch++)
                if (channel_bitmask & (0x01 << ch)) {
                    DVR_get_encoder_resolution(ch, &width, &height);
                    if      (width == 704 && height == 576)
                        pValue[ch] = LIB_ENC_RESOLUTION_704x576;
                    else if (width == 704 && height == 480)
                        pValue[ch] = LIB_ENC_RESOLUTION_704x480;
                    else if (width == 352 && height == 288)
                        pValue[ch] = LIB_ENC_RESOLUTION_352x288;
                    else if (width == 352 && height == 240)
                        pValue[ch] = LIB_ENC_RESOLUTION_352x240;
                    else if (width == 176 && height == 144)
                        pValue[ch] = LIB_ENC_RESOLUTION_176x144;
                    else if (width == 176 && height == 112)
                        pValue[ch] = LIB_ENC_RESOLUTION_176x112;
                    else
                        eprintf("get invalid resolution ch%02d %d:%d\n",ch ,width ,height);
                }
            break;
#if 0
        case LIB_ENC_GET_REQ_KEY_STATUS:
            for (ch = 0; ch < MAX_CH; ch++)
                if (channel_bitmask & (0x01 << ch))
                    DVR_get_encoder_request_key(pHandle, ch, &pValue[ch]);
            break;
#endif
        default:
            eprintf("set_property invaild type %d\n",type);
            break;
    }

    return rv;

}

int LIB816x_getDvrMessage(DVR_MSG_T *pMsg)
{
    DVR_CONTEXT *pCtx = &gDvrCtx;

    int osal_err = OSA_SOK;

    if (pCtx->qt_pipe_id == NULL)
    {
        OSA_CreatePipe(&pCtx->qt_pipe_id, sizeof(DVR_MSG_T), sizeof(DVR_MSG_T), 1);

        (void) app_playback_set_pipe(pCtx->qt_pipe_id);
        (void) app_avibackup_set_pipe(pCtx->qt_pipe_id);
        (void) app_bskbackup_set_pipe(pCtx->qt_pipe_id) ;
        (void) DVR_setWriterMsgPipe(pCtx->qt_pipe_id);  //MULTI-HDD SKSUNG
        dprintf("\n\napp: #### creation pipe ... done\n\n");
    }

    osal_err = OSA_ReadFromPipe(pCtx->qt_pipe_id, pMsg, sizeof(DVR_MSG_T), &pMsg->actual_size, OSA_NO_SUSPEND);

    return osal_err;

}

/*-------------------------*/
/* Motion & Sensor & Alarm */
/*-------------------------*/
void LIB816x_setSensor(int iSensorId,int iSensorEnable,int iSensorType)
{
    DVR_setSensor(iSensorId,iSensorEnable,iSensorType);
}
unsigned int LIB816x_getSensorStatus()
{

    return DVR_getSensorStatus();
}

void LIB816x_setAlarm(int iAlarmId,int iAlarmEnable,int iAlarmType,int iAlarmDelay)
{
    DVR_setAlarm(iAlarmId,iAlarmEnable,iAlarmType,iAlarmDelay);
}

void LIB816x_operateAlarm(int iAlarmId,int iAlarmOnOff)
{
    dvr_send_alarm(iAlarmId,iAlarmOnOff);
}

unsigned int LIB816x_getAlarmStatus()
{
    return DVR_getAlarmStatus();
}

void LIB816x_setDIOCallback(void* fncb)
{
    DVR_set_dio_callback(fncb);
}

int LIB816x_setMotion(int Ch,int bEnable, int sensitivity, unsigned char* motionTable)
{
    int rv=0;;
    rv = DM816X_MD_SetPrm(Ch, bEnable, sensitivity, motionTable) ;
    return 0;
}

unsigned int LIB816x_getMotionDetectStatus()
{
   int MotionStatus[MAX_CH], status, i, retMotion = 0;

   status = getMotionStatus(MotionStatus) ;
   for(i = 0; i <MAX_CH; i++)
   {
       if(MotionStatus[i])
       {
           retMotion += (1 << i) ;
       }
   }
   return retMotion  ;
}

unsigned int LIB816x_getVideoLossDetectStatus()
{
	return DVR_GetVideoLossStatus();
}

int LIB816x_setVideoResolution(int Ch,int iResolution)
{
	return DVR_setCaptureResolution(Ch,iResolution);
}

void LIB816x_startCamProperty(int selectedCh, int startX, int startY, int width, int height)
{
	DVR_startCamProperty(selectedCh, startX, startY, width, height);
}

void LIB816x_endCamProperty()
{
	DVR_endCamProperty();
}

void LIB816x_selCamPropCh(int nCh)
{
	DVR_selCamPropCh(nCh);
}


/*------------------------*/
/* PTZ internal functions */
/*------------------------*/
int LIB816x_getIntPtzCount()
{
    return DVR_ptz_getIntPtzCount();
}

int LIB816x_getIntPtzInfo(int ptzIdx, char* pPtzName)
{
    return DVR_ptz_getIntPtzInfo(ptzIdx, pPtzName);
}

int LIB816x_ptzCtrl(int ptzIdx, int ptzTargetAddr,int ctrl)
{
    return DVR_ptz_setInternalCtrl(ptzIdx, ptzTargetAddr, ctrl);
}

/*--------------------*/
/* PTZ user functions */
/*--------------------*/
int LIB816x_setPtzSerialInfo(int ptzDataBit, int ptzParityBit, int ptzStopBit, int ptzBaudRate)
{
    return DVR_ptz_setSerialInfo(ptzDataBit, ptzParityBit, ptzStopBit, ptzBaudRate);
}

int LIB816x_ptzSendBypass(char* ptzBuf, int bufSize)
{
    return DVR_ptz_ptzSendBypass(ptzBuf, bufSize);
}

/*-----------------------*/
/* CR-ROM user functions */
/*-----------------------*/

int LIB816x_CDROM_MEDIA(char *device)
{
    stNSDVR_CDROMStatus l_info;
    memset(&l_info, 0, sizeof(l_info));


    // check CD-ROM status

    if ( nsDVR_CDROM_check_ready(device) < 0 )
    {
        printf("CD-ROM(%s) not ready or No media !!\n", device);
                return 1;
    }

    if ( nsDVR_CDROM_get_media_info(device, &l_info) == NSDVR_ERR_NONE )
    {
        printf("MediaType=%s\n", nsDVR_CDROM_media_type_name(l_info.media_type));
        printf("Writeable=%d\n", l_info.writable);
        printf("Erasable =%d\n", l_info.erasable);
        if(!l_info.writable)
        {
            return 1 ;
        }
        else
        {
            printf("l_info.media_type = %d\n",l_info.media_type) ;
            if(l_info.media_type > 2)  // DVD
            {
                return 2 ;
            }
            return 0 ;                 // CDROM
        }
    }
    else
    {
        printf("Error get info for \"%s\"\n", device);
    }
    return 1;
}

int LIB816x_CDROM_EJECT(char *device)
{
    stNSDVR_CDROMStatus l_info ;
    memset(&l_info, 0, sizeof(l_info));

    if ( nsDVR_CDROM_eject(device) != NSDVR_ERR_NONE )
    {
        printf("Error eject \"%s\"\n", device);
    }
    return 1;
}

int LIB816x_CDROM_ERASE(char *device)
{
    if ( nsDVR_CDROM_check_ready(device) < 0 )
    {
        return 1;
    }
    stNSDVR_CDROMStatus l_info;
    if ( nsDVR_CDROM_get_media_info(device, &l_info) < 0 )
    {
        return 1;
    }
    if ( !l_info.erasable )
    {
        return 1;
    }

    int error_code;
    handle hEraser = nsDVR_CDROM_eraser_open(device, &error_code);
    if ( !hEraser )
    {
        return 1;
    }

    while ( 1 )
    {
        int status = nsDVR_CDROM_eraser_status(hEraser);
        if ( status < 0 )
        {
            break;
        }
        if ( status == 100 )
        {
            printf("\n");
            break;
        }

        //printf("Progress=%d\r", status); fflush(stdout);
        sleep(1);
    }

    nsDVR_CDROM_eraser_close(hEraser);
    return 1;

}

int LIB816x_CDROM_MAKE_ISO(char *iso_name, char **filelist, int filecnt)
{
    int error_code ;
    int i = 0 ;

    for(i = 0 ; i < filecnt; i++)
    {
        printf("iso_name = %s filelist = '%s' filecnt = %d\n",iso_name, filelist[i], filecnt) ;
    }

    handle h = nsDVR_CDROM_mkiso_open(iso_name, filelist, filecnt, &error_code) ;
    if( !h )
    {
        printf("Error nsDVR_CDROM_mkiso_open(), error=%s\n", nsDVR_util_error_name(error_code));
        return 1 ;
    }

    while(1)
    {
        int status = nsDVR_CDROM_mkiso_status(h) ;
        if( status < 0)
        {
            printf("Error nsDVR_CDROM_mkiso_status, error=%s\n", nsDVR_util_error_name(status));
            break ;
        }
        if(status == 100)
        {
            printf("\n") ;
            break ;
        }

        //printf("Progress=%d\r",status);
        fflush(stdout) ;
        sleep(1) ;
    }

    nsDVR_CDROM_mkiso_close(h);
    return 1 ;
}

int LIB816x_CDROM_WRITE_ISO(char *device, char *iso_file)
{
	//struct stat64 l_stat ;
	struct stat l_stat ;
    long file_size_MB ;
    int status = TRUE ;

	//if(stat64(iso_file, &l_stat) < 0)
	if(stat(iso_file, &l_stat) < 0)
    {
        printf("Cannot find ISO file(%s) !!\n", iso_file );
        return FALSE ;
    }

    file_size_MB = l_stat.st_size/1024/1024 ;
    printf("backup device = %s\n",device) ;
    printf("ISO file size=%ld(MB)\n", file_size_MB);

    if ( nsDVR_CDROM_check_ready(device) < 0 )
    {
        printf("CD-ROM(%s) not ready or No media !!\n", device);
        return FALSE ;
    }
    stNSDVR_CDROMStatus l_info ;
    if(nsDVR_CDROM_get_media_info(device, &l_info) < 0 )
    {
        printf("Error get info for \"%s\"\n", device);
        return FALSE ;
    }
    if( !l_info.writable )
    {
        printf("Media not writable  = %s !!\n", device);
        return FALSE ;
    }

    long media_size = 0 ;
    switch( l_info.media_type )
    {
        case NSDVR_MEDIA_TYPE_CD_ROM:
        case NSDVR_MEDIA_TYPE_CD_RW:
        // 70MB
            media_size = 700 ;
            break ;
        case NSDVR_MEDIA_TYPE_DVD_ROM:
        case NSDVR_MEDIA_TYPE_DVD_RW:
        case NSDVR_MEDIA_TYPE_DVD_RAM:
        // 4.7GB
            media_size = 4.7*1024 ;
            break ;
        default:
            printf("Unknown Media type(%d) !!\n",l_info.media_type) ;
            return FALSE ;
    }

    printf("Media size = %ld(MB)\n",media_size) ;

    if ( file_size_MB > media_size )
    {
        printf("ISO file(%ld MB) too big, media size = %ld(MB)\n", file_size_MB, media_size);
        return FALSE ;
    }

    int error_code;
    handle h = nsDVR_CDROM_writer_open(device, iso_file, &error_code);
    if ( !h )
    {
        printf("Error open %s, error=%s\n", device, nsDVR_util_error_name(error_code));
        return FALSE;
    }

    while(1)
    {
        status = nsDVR_CDROM_writer_status(h) ;
        if(status < 0)
        {
            printf("Error write CDROM, error=%s\n", nsDVR_util_error_name(status));
            break ;
        }
        if( status == 100)
        {
            status = TRUE ;
            printf("\n") ;
            break ;
        }
        printf("Progress = %d\r", status) ;
        fflush(stdout) ;
        usleep(1000) ;
    }

    nsDVR_CDROM_writer_close(h) ;
    return status;

}


int LIB816x_backupToAVI(int media, char *path, int ch_bitmask, struct tm start_t, struct tm end_t)
{
    // channel = mask
    struct tm stv, etv ;
    time_t start_time = time(NULL) ;
    time_t end_time = time(NULL) ;

    memcpy(&stv, &start_t, sizeof(struct tm)) ;
    stv.tm_year = stv.tm_year - 1900 ;

    memcpy(&etv, &end_t, sizeof(struct tm)) ;
    etv.tm_year = etv.tm_year - 1900 ;

    start_time = mktime(&stv) ;
    end_time = mktime(&etv) ;

    dprintf("start_time = %ld\n",start_time) ;
    dprintf("end_time = %ld\n",end_time) ;
    return app_backup_start(media, path, ch_bitmask, start_time, end_time);
}

int LIB816x_backupToBASKET(int media, char *path, int ch_bitmask, struct tm start_t, struct tm end_t)
{
    struct tm stv, etv ;
    time_t start_time = time(NULL) ;
    time_t end_time = time(NULL) ;

    memcpy(&stv, &start_t, sizeof(struct tm)) ;
    stv.tm_year = stv.tm_year - 1900 ;

    memcpy(&etv, &end_t, sizeof(struct tm)) ;
    etv.tm_year = etv.tm_year - 1900 ;

    start_time = mktime(&stv) ;
    end_time = mktime(&etv) ;

    dprintf("start_time = %ld\n",start_time) ;
    return app_bskbackup_create(media, path, ch_bitmask,start_time, end_time);
}

int LIB816x_setColorAdjust(int nChannelIndex,COLORADJUST* padjust)
{
	int rv=0;
	if(gInitSettings.camera[nChannelIndex].coloradjust.iContrast != padjust->iContrast)
	{
		rv=DVR_SetContrast(nChannelIndex, padjust->iContrast);
		gInitSettings.camera[nChannelIndex].coloradjust.iContrast = padjust->iContrast;
	}
	if(rv) return rv;
	if(gInitSettings.camera[nChannelIndex].coloradjust.iSaturation != padjust->iSaturation)
	{
		rv=DVR_SetSaturation(nChannelIndex, padjust->iSaturation);
		gInitSettings.camera[nChannelIndex].coloradjust.iSaturation = padjust->iSaturation;
	}
	if(rv) return rv;
	if(gInitSettings.camera[nChannelIndex].coloradjust.iBrightness != padjust->iBrightness)
	{
		rv=DVR_SetBrightness(nChannelIndex, padjust->iBrightness);		
		gInitSettings.camera[nChannelIndex].coloradjust.iBrightness = padjust->iBrightness;		
	}
	return rv;
}

int LIB816x_getColorAdjust(int nChannelIndex,COLORADJUST* padjust)
{
    int rv=0;

    rv=DVR_GetBrightness(nChannelIndex, &padjust->iBrightness);
    if(rv) return rv;

    rv=DVR_GetContrast(nChannelIndex, &padjust->iContrast);
    if(rv) return rv;

    rv=DVR_GetSaturation(nChannelIndex, &padjust->iSaturation);

    return rv;
}

int LIB816x_setCameraEnable(int nChannelIndex, int bEnable)
{
	int rv=0;
	if(nChannelIndex < MAX_DVR_CHANNELS)
	{
		if(gInitSettings.camera[nChannelIndex].iEnable != bEnable)
		{
			if((rv = DVR_SetCameraEnable(nChannelIndex, bEnable)) == 0)
				gInitSettings.camera[nChannelIndex].iEnable = bEnable;
		}
	}
	else
	{
		if(gInitSettings.playbackcamera[nChannelIndex-MAX_DVR_CHANNELS].iEnable != bEnable)
		{
			if((rv = DVR_SetCameraEnable(nChannelIndex, bEnable)) == 0)
				gInitSettings.playbackcamera[nChannelIndex-MAX_DVR_CHANNELS].iEnable = bEnable;
		}
	}
	return rv;
}

int LIB816x_setCameraLayout(int iOutput, int nStartChannelIndex, int iLayoutMode)
{
	int rv=0;
	
	// BKKIM

	if(nStartChannelIndex>=16)
	{
		app_tplay_save_layout(iLayoutMode, nStartChannelIndex);
	}
	rv = DVR_SetCameraLayout(iOutput, nStartChannelIndex,iLayoutMode,0xff);
    return rv;
}

int LIB816x_setCameraLayout_set(int iOutput, int nStartChannelIndex, int iLayoutMode, int m_iCameraSelectId)
{
	int rv=0;
	rv = DVR_SetCameraLayout(iOutput, nStartChannelIndex,iLayoutMode,m_iCameraSelectId);
    return rv;
}

int LIB816x_setCovert(int nChannelIndex, int bEnable)
{
	return DVR_SetCovert(nChannelIndex, bEnable);
}

int LIB816x_getSourceStatus(int *nChannelCount, SOURCE_CH_STATUS_S* pChStatus)
{
	*nChannelCount = gCaptureInfo.chCount;
	memcpy(pChStatus, &gCaptureInfo.chStatus, sizeof(gCaptureInfo.chStatus));
	
	return 0;
}

int LIB816x_setVideoCodecType(int Ch,int iCodecType)
{
	int rv=0;
	if(iCodecType == CODECTYPE_H264)
	{
	}
	else if(iCodecType == CODECTYPE_MPEG4)
	{
	}
	else	//CODECTYPE_MJPEG
	{
	}
	return rv;
}

int LIB816x_setAudioCodecType(int Ch,int iCodecType)
{
    int rv=0;
    if(iCodecType == AUDIO_CODEC_G711)
    {
    }
    else    //AUDIO_CODEC_AAC
    {
    }
    return rv;
}
int LIB816x_setRecDuration(int Ch,int bPrevRecEnable, int iPrevDuration)
{
    int rv=0;
    if(bPrevRecEnable && (iPrevDuration>=PREVRECORDDURATION_SEC_MIN && iPrevDuration<=PREVRECORDDURATION_SEC_MAX))
    {
#if PREREC_ENABLE // BKKIM pre-record
		DVR_SetPreRecDuration(Ch, iPrevDuration);
#endif
	}
	else if(bPrevRecEnable == 0)
	{
#if PREREC_ENABLE // BKKIM pre-record
		DVR_SetPreRecDuration(Ch, 0);
#endif
	}

    return rv;
}

int LIB816x_setAudioInput(int Ch,int bEnable)
{
    int rv=0;
	printf("LIB816x_setAudioInput() ch[%d] enable[%d]=======================\n", Ch, bEnable);
	gInitSettings.audioin.audioinch[Ch].iAudioInOn = bEnable;
	return rv;
}
int LIB816x_setAudioInputParams(int iSampleRate, int iVolume)
{
	int rv=0;
	if((iSampleRate == AUDIO_SAMPLERATE_8KHZ || iSampleRate == AUDIO_SAMPLERATE_16KHZ)
		&&(iVolume >= AUDIO_VOLUME_MUTE && iVolume <= AUDIO_VOLUME_MAX))
	{
		rv = DVR_setAudioInputParams( iSampleRate, iVolume);
		gInitSettings.audioin.iSampleRate 	= iSampleRate;
		gInitSettings.audioin.iVolume 		= iVolume;
	}
	return rv;
}

int LIB816x_setAudioOutput(int bEnable, int iVolume, int iInputCh)
{
	AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun
	int rv=0;
	
	printf("LIB816x_setAudioOutput bEnable = %d iInputCh = %d\n",bEnable, iInputCh) ;

	if(bEnable == TRUE)
	{
		if(iVolume >= AUDIO_VOLUME_MUTE && iVolume <= AUDIO_VOLUME_MAX)
		{
			if( DVR_setAudioAIC3volume(iVolume) == 0)
				gInitSettings.audioout.iVolume = iVolume;
		}
		
		if(iInputCh < MAX_CH) // live 0 - 15ch
		{
			if(pAudstate->livechannel != iInputCh)
				pAudstate->livechannel = iInputCh ;
		}
		else  // playback 16 - 31
		{
			if(iInputCh >= MAX_CH && iInputCh < MAX_CH*2)
				pAudstate->pbchannel = iInputCh - MAX_CH ;
		}
	}

	gInitSettings.audioout.iAudioOutOn = bEnable;	

	return rv;	
}

int LIB816x_setAudioOutputDev(int devId) 
{
	int rv = 0 ;	
	rv = DVR_setAudioOutputDev(devId) ;
	return rv ;
}

void LIB816x_Vdis_start()
{
    Vdis_start();
}

void LIB816x_Vdis_stopDrv(int displayId)
{
    DVR_VdisStopDrv(displayId);
}

void LIB816x_Vdis_startDrv(int displayId)
{
    DVR_VdisStartDrv(displayId);
}

void LIB816x_startRTSP()
{
	DVR_startRTSP();
}

void LIB816x_stopRTSP()
{
	DVR_stopRTSP();
}

int LIB816x_setBitrateType(int chId, int bitrateType)
{
	int rv = 0;
	if(bitrateType != gInitSettings.camera[chId].camrec.iBitrateType)
	{
		rv = DVR_SetBitrateType(chId, bitrateType);
		gInitSettings.camera[chId].camrec.iBitrateType = bitrateType;
	}

	return rv;
}


int LIB816x_setDisplayRes(int devId, int resType)
{
	return DVR_SetDisplayRes(devId, resType);
}

int LIB816x_setSpotChannel(int chId)
{
	int ret = 0;
	if(gInitSettings.iSpotOutputCh != chId)
	{
		ret = DVR_SetSpotChannel(chId);
		gInitSettings.iSpotOutputCh = chId;
	}

	return ret;
}

int LIB816x_setDisplayMainSub(int mainDevId, int subDevId, int spotDevId, int mainResolution, int subResolution, int spotResolution)
{
	return DVR_SwitchDisplay(mainDevId, subDevId, spotDevId, mainResolution, 
	subResolution, spotResolution);
}


void LIB816x_initSettingParam(PROFILE* pParam)
{
	int i;
	memcpy(&gInitSettings,pParam,sizeof(PROFILE));
    gInitSettings.layout.iSubOutput = gInitSettings.layout.iMainOutput;

    switch(pParam->maindisplayoutput.iResolution)
    {
    	case OUTPUT_RESOLUTION_1080P:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_1080P_60;
    		break;
    	case OUTPUT_RESOLUTION_1080P50:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_1080P_50;
    		break;
    	case OUTPUT_RESOLUTION_720P:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_720P_60;
    		break;
    	case OUTPUT_RESOLUTION_SXGA:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_SXGA_60;
    		break;
    	case OUTPUT_RESOLUTION_XGA:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_XGA_60;
    		break;
    	case OUTPUT_RESOLUTION_480P:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_480P;
    		break;
    	case OUTPUT_RESOLUTION_576P:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_576P;
    		break;
    	default:
    		gInitSettings.maindisplayoutput.iResolution = DC_MODE_1080P_60;
    		break;
    }
    switch(pParam->subdisplayoutput.iResolution)
    {
    	case OUTPUT_RESOLUTION_1080P:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_1080P_60;
    		break;
    	case OUTPUT_RESOLUTION_1080P50:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_1080P_50;
    		break;
    	case OUTPUT_RESOLUTION_720P:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_720P_60;
    		break;
    	case OUTPUT_RESOLUTION_SXGA:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_SXGA_60;
    		break;
    	case OUTPUT_RESOLUTION_XGA:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_XGA_60;
    		break;
    	case OUTPUT_RESOLUTION_480P:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_480P;
    		break;
    	case OUTPUT_RESOLUTION_576P:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_576P;
    		break;
    	default:
    		gInitSettings.subdisplayoutput.iResolution = DC_MODE_1080P_60;
    		break;
    }
    switch(pParam->spotdisplayoutput.iResolution)
    {
    	case OUTPUT_RESOLUTION_NTSC:
    		gInitSettings.spotdisplayoutput.iResolution = DC_MODE_NTSC;
    		break;
    	case OUTPUT_RESOLUTION_PAL:
    		gInitSettings.spotdisplayoutput.iResolution = DC_MODE_PAL;
    		break;
    	default:
    		gInitSettings.spotdisplayoutput.iResolution = DC_MODE_NTSC;
    		break;
    }
    if(gInitSettings.layout.iMainOutput == gInitSettings.layout.iSubOutput)
    {
        gInitSettings.iSubOutputEnable=0;
    }
    else
    {
        gInitSettings.iSubOutputEnable=1;
    }
    for(i=0;i<MAX_INTERNAL_CAMERA;i++)
    {
        switch(pParam->camera[i].camrec.iFps)
      {
      	case 0:
      		gInitSettings.camera[i].camrec.iFps = FRAMERATE_04_03;
      		break;
      	case 1:
      		gInitSettings.camera[i].camrec.iFps = FRAMERATE_08_06;
      		break;
      	case 2:
      		gInitSettings.camera[i].camrec.iFps = FRAMERATE_15_13;
      		break;
      	case 3:
      	default:
      		gInitSettings.camera[i].camrec.iFps = FRAMERATE_30_25;
      		break;
      }
    }    
}


