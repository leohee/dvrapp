#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

#include <time.h>

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                              DEFINE                              */
/*==================================================================*/
#define AUDIO_STREAM_ENABLE
#define RTSP_STREAM_ENABLE

#define LIB_MAIN_VERSION        "VER_02.80.00.11"

#define MAX_DVR_CHANNELS        16

/*==============================*/
/*           FOR DISPLAY        */
/*==============================*/
#define LIB_MAIN_SWAPOFF                0
#define LIB_MAIN_SWAPON                 1

#define LIB_WINDOW_FULLHD               0
#define LIB_WINDOW_SXGA                 1

#define LIB_NORMAL_SCREEN_MODE          0
#define LIB_FULL_SCREEN_MODE            1

#define LIB_LIVE_MODE                   0
#define LIB_PLAYBACK_MODE               1


// Encoder Resolution
#define LIB_ENC_RESOLUTION_704x576      0
#define LIB_ENC_RESOLUTION_704x480      1
#define LIB_ENC_RESOLUTION_352x288      2
#define LIB_ENC_RESOLUTION_352x240      3
#define LIB_ENC_RESOLUTION_176x144      4
#define LIB_ENC_RESOLUTION_176x112      5

// MOSAIC LAYOUT
#define LIB_LAYOUTMODE_1                1
#define LIB_LAYOUTMODE_4                2
#define LIB_LAYOUTMODE_9                3
#define LIB_LAYOUTMODE_8                4
#define LIB_LAYOUTMODE_8_1              5
#define LIB_LAYOUTMODE_12               6
#define LIB_LAYOUTMODE_12_1             7
#define LIB_LAYOUTMODE_14               8
#define LIB_LAYOUTMODE_16               9
#define LIB_LAYOUTMODE_20               10
#define LIB_LAYOUTMODE_CAMFLOW          11

// Encoder Set Property
#define LIB_ENC_SET_FRAMERATE           12
#define LIB_ENC_SET_BITRATE             13
#define LIB_ENC_SET_RESOLUTION          14
#define LIB_ENC_SET_REQ_KEY_FRAME       15
#define LIB_ENC_SET_I_FRAME_INTERVAL    16

// Encoder Get Property
#define LIB_ENC_GET_FRAMERATE           17
#define LIB_ENC_GET_BITRATE             18
#define LIB_ENC_GET_RESOLUTION          19
#define LIB_ENC_GET_REQ_KEY_STATUS      20
#define LIB_ENC_GET_I_FRAME_INTERVAL    21


/*==============================*/
/*       FOR RECORDING          */
/*==============================*/
#define LIB_EVENT_REC_CONT      1
#define LIB_EVENT_REC_MOT       2
#define LIB_EVENT_REC_SENSOR    3
#define LIB_EVENT_REC_VLOSS     4
#define LIB_EVENT_REC_EMG       5

#define BACKUP_BINARY_MKISO "open_bin/mkisofs"
#define BACKUP_BINARY_CDRECORD "open_bin/cdrecord"

// Playback Command Table
#define LIB_PB_START                    22
#define LIB_PB_STOP                     23
#define LIB_PB_JUMP                     24

// Playback Sub Command Table
#define LIB_PB_SUB_CMD_CH_ENABLE        30
#define LIB_PB_SUB_CMD_CH_SET_SPEED     31

// Playback speed steps
#define LIB_PB_SPEED_1X                 1
#define LIB_PB_SPEED_2X                 2
#define LIB_PB_SPEED_3X                 3
#define LIB_PB_SPEED_4X                 4
#define LIB_PB_SPEED_8X                 5
#define LIB_PB_SPEED_16X                6
#define LIB_PB_SPEED_32X                7
#define LIB_PB_SPEED_0_25X              8
#define LIB_PB_SPEED_0_5X               9

// Playback Error Table
#define LIB_PB_NO_ERR                   (0)
#define LIB_PB_START_FAILED             (-1)
#define LIB_PB_JUMP_FAILED              (-2)
#define LIB_PB_ALLOC_FAILED             (-3)
#define LIB_PB_BASKET_FAILED            (-4)
#define LIB_PB_OSAL_FAILED              (-5)
#define LIB_PB_READ_FRAME_FAILED        (-6)
#define LIB_PB_NOT_ALLOWED              (-7)

// Writer Error Type
#define LIB_WR_NO_ERR                   (0)
#define LIB_WR_OPEN_FAIL                (-1)
#define LIB_WR_REC_FAIL                 (-2)
#define LIB_WR_HDD_FULL                 (-3)
#define LIB_WR_NOT_MOUNTED              (-4)

// Backup Error
#define LIB_BA_NO_ERR                   (0)
#define LIB_BA_START_FAILED             (-1)
#define LIB_BA_ALLOC_FAILED             (-2)
#define LIB_BA_BASKET_FAILED            (-3)
#define LIB_BA_OSAL_FAILED              (-4)
#define LIB_BA_READ_FRAME_FAILED        (-5)
#define LIB_BA_CREATE_AVI_FAILED        (-6)
#define LIB_BA_MAKE_ISO_FAILED          (-7)
#define LIB_BA_WRITE_ISO_FAILED         (-8)
#define LIB_BA_NOT_ALLOWED              (-9)
#define LIB_BA_NOT_SUFFICIENT           (-10)
#define LIB_BA_INVALID_MEDIA            (-11)

#define TYPE_CDROM                      0
#define TYPE_USB                        1

// CB Request Owner Table
#define LIB_PLAYER                      (1)
#define LIB_RECORDER                    (2)
#define LIB_MISC                        (3)
#define LIB_BACKUP                      (4)

// PTZ Ctrl
#define PAN_RIGHT                                   0
#define PAN_LEFT                                    1
#define TILT_UP                                     2
#define TILT_DOWN                                   3
#define ZOOM_IN                                     4
#define ZOOM_OUT                                    5
#define FOCUS_OUT                                   6
#define FOCUS_IN                                    7
#define PTZ_STOP                                    8

// DVR HDD INFO
#define MAX_DVR_DISK                    7 //# sata 4 + sd 1 + usb 2
#define MAX_PARTITION                   4
#define MAX_DEVLIST                     MAX_DVR_DISK*MAX_PARTITION
#define SIZE_DEV_NAME                   64
#define SIZE_DIR_NAME                   256
#define SIZE_TYP_NAME                   128

#define RFS_OTHER                       0
#define RFS_NFS                         1

//#
#define CAM_NTSC            1
#define CAM_PAL             0

typedef enum {
    DISP_PATH_HDMI0 = 0,
    DISP_PATH_HDMI1,          //# DISP_PATH_DVO2
    DISP_PATH_VGA,            //# DISP_PATH_HDCOMP
    DISP_PATH_SD,
} DisplayPath_e;

typedef enum {
    DC_MODE_1080P_30 = 0,   //# (1920x1080)
    DC_MODE_1080I_60,
    DC_MODE_1080P_60,
    DC_MODE_720P_60,        //# (1280x720)
    DC_MODE_NTSC,           //# (720x480)
    DC_MODE_PAL,            //# (720x576)
    DC_MODE_XGA_60,         //# (1024x768)
    DC_MODE_SXGA_60,        //# (1280x1024)
    DC_MODE_1080P_50,
    DC_MODE_480P,
    DC_MODE_576P,
    DC_MAX_MODE
} DisplayDcMode_e;


//COLORADJUST
#define BRIGHTNESS_VALUE_MIN                        0
#define BRIGHTNESS_VALUE_DEFAULT                    128
#define BRIGHTNESS_VALUE_MAX                        255

#define CONTRAST_VALUE_MIN                          0
#define CONTRAST_VALUE_DEFAULT                      128
#define CONTRAST_VALUE_MAX                          255

#define SATURATION_VALUE_MIN                        0
#define SATURATION_VALUE_DEFAULT                    128
#define SATURATION_VALUE_MAX                        255

#define HUE_VALUE_MIN                               -180
#define HUE_VALUE_DEFAULT                           0
#define HUE_VALUE_MAX                               180

#define SHARPNESS_VALUE_MIN                         0
#define SHARPNESS_VALUE_DEFAULT                     4
#define SHARPNESS_VALUE_MAX                         9

#define BITRATE_TYPE_CBR                            0
#define BITRATE_TYPE_VBR                            1

#define IFRAME_INTERVAL_30                          0
#define IFRAME_INTERVAL_15                          1
#define IFRAME_INTERVAL_10                          2
#define IFRAME_INTERVAL_5                           3
#define IFRAME_INTERVAL_1                           4

#define FRAMERATE_30_25                             0   //NTSC 30, PAL 25
#define FRAMERATE_15_13                             1   //NTSC 15, PAL 13
#define FRAMERATE_08_06                             2   //NTSC 8,  PAL 6
#define FRAMERATE_04_03                             3   //NTSC 4,  PAL 3

//DIO
#define MAX_SENSOR                                  16
#define SENSOR_DETECT_OFF                           0
#define SENSOR_DETECT_ON                            1

#define SENSOR_TYPE_NO                              0
#define SENSOR_TYPE_NC                              1

#define MAX_ALARM                                   4
#define ALARM_DETECT_OFF                            0
#define ALARM_DETECT_ON                             1

#define ALARM_TYPE_NO                               0
#define ALARM_TYPE_NC                               1

#define ALARM_DELAYSEC_MIN                          1
#define ALARM_DELAYSEC_MAX                          30
#define ALARM_DELAYSEC_DEFAULT                      3

//MOTION DETECT
#define MOTION_W_CELL                               22
#define MOTION_H_CELL                               15
#define MOTION_PAL_INC                              3

#define MAX_MOTION_CELL                             MOTION_W_CELL*(MOTION_H_CELL+MOTION_PAL_INC)

#define MOTIONAREA_MODE_CLEAR                       0
#define MOTIONAREA_MODE_SET                         1

#define CODECTYPE_H264                              0
#define CODECTYPE_MPEG4                             1
#define CODECTYPE_MJPEG                             2

#define RESOLUTION_D1                               0
#define RESOLUTION_CIF                              1
#define RESOLUTION_HALFD1                           2
#define RESOLUTION_QCIF                             3
#define RESOLUTION_720P                             4

//AUDIO
#define MAX_AUDIOIN                                 16

#define AUDIO_CODEC_G711                            0
#define AUDIO_CODEC_AAC                             1

#define AUDIO_SAMPLERATE_8KHZ                       0
#define AUDIO_SAMPLERATE_16KHZ                      1

#define AUDIO_VOLUME_MUTE                           0
#define AUDIO_VOLUME_MAX                            10
#define AUDIO_VOLUME_DEFAULT                        5

#define AUDIO_MONO                                  0
#define AUDIO_STEREO                                1

//CAMREA POSITION
#define CAMERA_POSITION_X_MIN                       0
#define CAMERA_POSITION_X_MAX                       16
#define CAMERA_POSITION_Y_MIN                       0
#define CAMERA_POSITION_Y_MAX                       16
#define CAMERA_POSITION_X_DEFAULT                   0
#define CAMERA_POSITION_Y_DEFAULT                   0
#define CAMERA_POSITION_X_STEP                      2
#define CAMERA_POSITION_Y_STEP                      2

#define PREVRECORDDURATION_SEC_MIN                  1
#define PREVRECORDDURATION_SEC_MAX                  10
#define PREVRECORDDURATION_SEC_DEFAULT              10

#define POSTRECORDDURATION_MINUTE_MIN               1
#define POSTRECORDDURATION_MINUTE_MAX               60
#define POSTRECORDDURATION_MINUTE_DEFALUT           3

#define MAX_DAY                                     7
#define MAX_HOUR                                    24
#define MAX_TITLE_CHAR                              16
#define MAX_CAMERA                                  24//16//24
#define MAX_INTERNAL_CAMERA                         16
#define MAX_IPNC_CAMERA                             MAX_CAMERA-MAX_INTERNAL_CAMERA
#define MAX_PASSWORD_CHAR                           8
#define MAX_CHAR_4              4
#define MAX_CHAR_8              8
#define MAX_CHAR_10             10
#define MAX_CHAR_16             16
#define MAX_CHAR_20             20
#define MAX_CHAR_24             24
#define MAX_CHAR_32             32
#define MAX_CHAR_48             48
#define MAX_CHAR_256            256
//DISPLAYOUTPUT
#define OUTPUT_RESOLUTION_1080P                     0
#define OUTPUT_RESOLUTION_720P                      1
#define OUTPUT_RESOLUTION_SXGA                      2
#define OUTPUT_RESOLUTION_XGA                       3
#define OUTPUT_RESOLUTION_1080P50                   4
#define OUTPUT_RESOLUTION_480P                      5
#define OUTPUT_RESOLUTION_576P                      6

#define OUTPUT_RESOLUTION_NTSC                      0
#define OUTPUT_RESOLUTION_PAL                       1
/*==================================================================*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                           STRUCTURE                              */
/*==================================================================*/
/**
 *******************************************************************************
 *  @struct ST_DISPLAY_FRMSIZE
 *  @brief  This structure contains display frame size on UI
 *
 *  @param  nStartx     : start point of horizontal
 *  @param  nStartY     : start point of vertical
 *  @param  nFrmWidth   : frame width
 *  @param  nFrmHeight  : frame height
 *******************************************************************************
*/
typedef struct ST_DISPLAY_FRMSIZE
{
    int nStartX;
    int nStartY;
    int nFrmWidth;
    int nFrmHeight;
}ST_DISPLAY_FRMSIZE;

typedef struct {
    int which;
    int data;
    void *pData;

    int actual_size;
} DVR_MSG_T;

/**
 *******************************************************************************
 *  @struct ST_DISPLAY_PROPERTY
 *  @brief  This structure contains display property
 *
 *  @param  nSwapMode           : state of swap between On chip and Off chip
 *  @param  nVideoOutputMode    : Off chip output status of the full HD or VGA
 *  @param  nScreenMode         : status of Full screen or UI screen
 *  @param  nDefaultLayoutMode  : start layout mode
 *  @param  displayInfo         : display size informaiton
 *******************************************************************************
*/
typedef struct ST_DISPLAY_PROPERTY
{
    int                 nSwapMode;
    int                 nVideoOutputMode;
    int                 nScreenMode;
    int                 nDefaultLayoutMode;
    ST_DISPLAY_FRMSIZE  displayInfo;
} ST_DISPLAY_PROPERTY;


typedef enum {
    DISK_IDLE = 0,
    DISK_CDROM,
    DONE_FDISK,
    DONE_FORMAT,
    DONE_EXT3,
    MAX_STATE
} disk_state_e;

/**
 *******************************************************************************
 *  @struct dvr_dev_info_t
 *  @brief  This structure contains mounted device information
 *
 *  @param  dev         : device (path)name
 *  @param  dir         : mounted directory
 *  @param  type        : filesystem type
 *  @param  sz_total    : total size (MB)
 *  @param  sz_avail    : free size (MB)
 *  @param  sz_used     : used size (MB)
 *  @param  state       : refer to disk_state_e
 *  @param  rec_used    : 1: used for recording, 0: not used for recording
 *******************************************************************************
*/
typedef struct {
    char dev[SIZE_DEV_NAME];
    char dir[SIZE_DIR_NAME];
    char type[SIZE_TYP_NAME];
    long sz_total;
    long sz_avail;
    long sz_used;
    int  state;
} dvr_dev_info_t;

typedef struct {
    int cnt;
    dvr_dev_info_t dev[MAX_DEVLIST];
    int                 rfs_type;
    int  rec_used[MAX_DEVLIST];
} dvr_disk_info_t;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DIO
typedef struct
{
    int iSensorOn;
    int iSensorType;
}SENSOR,*PSENSOR;       //SENSOR INFO

typedef struct
{
    int iAlarmOn;
    int iAlarmType;
    int iDelayTime;
}ALARM,*PALARM;         //ALARM INFO

typedef struct
{
    int iBrightness;
    int iContrast;
    int iSaturation;
}COLORADJUST,*PCOLORADJUST; //COLORADJUST INFO

typedef struct
{
    int iMotionOn;
    int iSensitivity;
    unsigned char motiontable[MAX_MOTION_CELL];
}MOTION,*PMOTION;   //MOTION DETECTION  INFO

typedef struct
{
    int iEnable;
    int iPlaybackCameraEnable;
    char strcameraname[16];
    COLORADJUST coloradjust;
    MOTION motion;
    //RECORDDURATION
    int iPrevRecordOn;
    int iPrevRecordDuration;
    //Record
    int iCodecType;
    int iResolution;
    int iIFrameInterval;
    int iBitrateType;
    int iFps;
    int ikbps;
    int iAudioCodecType;
    int iAudioSamplingType;
}CAMERAINFO;

//AUDIO
typedef struct
{
    int iAudioInOn;
}AUDIOINCH,*PAUDIOINCH;         //AUDIOCH IN INFO

typedef struct
{
    AUDIOINCH audioinch[MAX_AUDIOIN];
    int iSampleRate;
    int iVolume;
}AUDIOIN,*PAUDIOIN;  //AUDIO IN INFO



typedef struct
{
    int iAudioOutOn;
    int iVolume;
    int iSelectAudioIn;
    int iStereo;
    int iDevId;
}AUDIOOUT,*PAUDIOOUT;       //AUDIO OUT INFO

typedef struct
{
    int  iMainOutput;
    int  iSubOutput;
    int  iLayoutMode;
    int  iSecondLayoutMode;
}LAYOUT,*PLAYOUT;   //LAYOUT INFO

//DISPLAY
typedef struct
{
    int iSequenceModeOn;
    int iInterval;
}SEQUENCE,*PSEQUENCE;  //SEQUENCE INFO

typedef struct
{
    ///border line
    int  iBorderLineOn;
    ///date/time
    int  iDateFormat;
}OSD,*POSD;         //OSD INFO

typedef struct
{
    int  iResolution;
}DISPLAYOUTPUT,*PDISPLAYOUTPUT;  //DISPLAY INFO
typedef struct
{
    int iPtzOn;
    int iPtzdriverIndex;
    int iPtzAddress;
    int iBaudRate;
    int iDataBit;
    int iStopBit;
    int iParityBit;
}PTZINFO,*PPTZINFO;     // P/T/Z INFO

//RECORD
typedef struct
{
    int ScheduleTable[MAX_DAY][MAX_HOUR];
 }RECSCHEDULE,*PRECSCHEDULE; //RECSCHEDULE TABLE

typedef struct
{
    int iRecordingOn;
    int iCodecType;
    int iResolution;
    int InputVideoSignalType;
    int iFps;
    int ikbps;
    int iIFrameInterval;
    int iBitrateType;
    int iAudioOn;
    int iAudioCodecType;
    int iAudioSamplingType;
 }CAMREC,*PCAMREC;  // RECORDING INFO
 
typedef struct
{
    int iPrevRecordOn;
    int iPrevRecordDuration;
    int iPostRecordOn;
    int iPostRecordDuration;
}RECORDDURATION,*PRECORDDURATION;       //RECORDING DURATION TIME INFO

typedef struct
{
    int iEnable;
    int iCameraType;
    int iCovertOn;
    char strcameraname[MAX_TITLE_CHAR];
    PTZINFO ptzinfo;
    MOTION motion;
    COLORADJUST coloradjust;
    CAMREC  camrec;
    RECSCHEDULE recschedule;
    RECORDDURATION recordduration;
}CAMERA,*PCAMERA;   //CAMERA INFO

typedef struct
{
        ///playback mode?êÏÑú camera enable ?¨Î?
    int iEnable;
}PLAYBACKCAMERA,*PPLAYBACKCAMERA;   //PLAYBACKCAMERA INFO

//EVENT
typedef struct
{
    int iMotionRecordingLink[MAX_INTERNAL_CAMERA][MAX_INTERNAL_CAMERA];
    int iMotionAlarmLink[MAX_INTERNAL_CAMERA][MAX_ALARM];

    int iSensorRecordingLink[MAX_SENSOR][MAX_INTERNAL_CAMERA];
    int iSensorAlarmLink[MAX_SENSOR][MAX_ALARM];

    int iVideoLossRecordingLink[MAX_INTERNAL_CAMERA][MAX_INTERNAL_CAMERA];
    int iVideoLossAlarmLink[MAX_INTERNAL_CAMERA][MAX_ALARM];
}EVENTLINK,*PEVENTLINK;     //EVENTLINK INFO



//STORAGE
typedef struct
{
    int iHddRecylceModeOn;
}STORAGE,*PSTORAGE;

// IPNC
typedef struct
{
    char IpAddress[MAX_CHAR_16];
    int port;
    char  id[MAX_PASSWORD_CHAR];
    char  password[MAX_PASSWORD_CHAR];
}IPNCINFO,*PIPNCINFO;

typedef struct
{
   int iRecordEmergencyOn;
}DVRALL,*PDVRALL;

//NETWORK
typedef struct
{
    char enable;                // 0:link down, 1: link up
    char type;                  // 0:static, 1: dhcp
    char ipaddr[MAX_CHAR_16];
    char netmask[MAX_CHAR_16];
    char gateway[MAX_CHAR_16];
} NETWORKINFO,*PNETWORKINFO;     //NETWORK INFO


//DVR PROFILE

typedef struct
{
    ///layout(main/sub output device, layout mode)
    LAYOUT layout;
    ///main display sequence mode time interval
    SEQUENCE sequence;
    ///sub display sequence mode time interval
    SEQUENCE secondsequence;
    ///layout borderline, date format
    OSD mainosd;
    ///main output resolution
    DISPLAYOUTPUT maindisplayoutput;
    ///sub output resolution
    DISPLAYOUTPUT subdisplayoutput;
    ///spot output resolution
    DISPLAYOUTPUT spotdisplayoutput;
    ///spot channel number
    int iSpotOutputCh;
    ///sub display output(one monitor)
    int iSubOutputEnable;
    ///camera(enable,motion,color)
    CAMERA camera[MAX_CAMERA];
    ///playback mode camera enable
    PLAYBACKCAMERA playbackcamera[MAX_CAMERA];
    ///audio input
    AUDIOIN audioin;
    ///audio output
    AUDIOOUT audioout;
    ///sensor
    SENSOR sensor[MAX_SENSOR];
    ///alram
    ALARM alarm[MAX_ALARM];
    ///eventÄ Record, alram
    EVENTLINK eventlink;
    ///Storage device
    STORAGE storage;
    ///IPNC
    IPNCINFO ipnc[MAX_IPNC_CAMERA];
    ///Emergency Recording
;    DVRALL dvrall;
    ///eth0 network
    NETWORKINFO net_info1;
    ///eth1 network
    NETWORKINFO net_info2;
    ///live mode(audio output channel)
    int  iSelectChannel;
    ///playback mode(audio output channel)
    int  iSecondSelectChannel;
}PROFILE,*PPROFILE;

typedef struct
{
  int chId;
  /**< channel ID */

  int frameWidth;
  /**< Frame or field width in pixels
   *
   * This is detected video signal frame or field width.
   *
   * Further change in width or height due to additional
   * cropping, scaling like CIF, HALF-D1 is not accoutned for in this field
  */

  int frameHeight;
  /**< Frame or field height in lines
   *
   * This is detected video signal frame or field height.
   *
   * Further change in width or height due to additional
   * cropping, scaling like CIF, HALF-D1 is not accoutned for in this field
  */

  //int isInterlaced;
  /**< TRUE: Source is Interlaced, FALSE: Source is Progressive */

} SOURCE_CH_STATUS_S;

/*==================================================================*/
/*                         STRUCTURE END                            */
/*==================================================================*/



/*==================================================================*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                     INTERFACE PROTOTYPE                          */
/*==================================================================*/
void LIB816x_initSettingParam(PROFILE* pParam);
//camera
int LIB816x_setColorAdjust(int nChannelIndex,COLORADJUST* padjust);
int LIB816x_getColorAdjust(int nChannelIndex,COLORADJUST* padjust);
int LIB816x_setMotion(int Ch,int bEnable, int sensitivity, unsigned char* motionTable);
unsigned int LIB816x_getMotionDetectStatus() ;
unsigned int LIB816x_getVideoLossDetectStatus() ;
int LIB816x_setVideoResolution(int Ch,int iResolution);


void LIB816x_startCamProperty(int selectedCh, int startX, int startY, int width, int height);
void LIB816x_endCamProperty();
void LIB816x_selCamPropCh(int nCh);

//record
int LIB816x_setVideoCodecType(int Ch,int iCodecType);
int LIB816x_setAudioCodecType(int Ch,int iCodecType);
int LIB816x_setRecDuration(int Ch,int bPrevRecEnable, int iPrevDuration);

//audio
int LIB816x_setAudioInput(int Ch,int bEnable);
int LIB816x_setAudioInputParams(int iSampleRate, int iVolume);
int LIB816x_setAudioOutput(int bEnable, int iVolume, int iInputCh);
int LIB816x_setAudioOutputDev(int devId) ;

/*-------------------*/
/* Initialize system */
/*-------------------*/
void LIB816x_initDisplayInfo(ST_DISPLAY_PROPERTY *pDspProperty);



/*-------------------*/
/*   Start system    */
/*-------------------*/

/*
*   @brief  Start the DVR system that VFCC, VFPC, VFDC, VENC, VDEC, CMUX and DMUX.
*
*   @param  void.
*   @return always 1.
*/
int LIB816x_startSystem(int disp_main, int disp_sub, int bNtsc, int vmode);

/*-------------------*/
/*   Stop system    */
/*-------------------*/

/*
*   @brief  Stop everything.
*
*   @param  void.
*   @return always 1.
*/
int LIB816x_stopSystem(void);
int LIB816x_getDvrMessage(DVR_MSG_T *pMsg);

/*-------------------*/
/*  System settings  */
/*-------------------*/

/*
*   @brief  change mosaic layout mode
*
*   @param  int mode    [I] LIB_LAYOUTMODE_1 to LIB_LAYOUTMODE_16.
*   @return If tath does not support mode, return 0.
*/
int LIB816x_setDisplayLayout(int mode, int nChannelIndex);


int LIB816x_setPlaybackDisplayLayout(int mode, int nChannelIndex);

int LIB816x_setCameraEnable(int nChannelIndex, int bEnable);
int LIB816x_setCameraLayout(int iOutput, int nStartChannelIndex, int iLayoutMode);
int LIB816x_setCameraLayout_set(int iOutput, int nStartChannelIndex, int iLayoutMode, int m_iCameraSelectId);
int LIB816x_setCovert(int nChannelIndex, int bEnable);
int LIB816x_getSourceStatus(int *nChannelCount, SOURCE_CH_STATUS_S* pChStatus);

int LIB816x_setBitrateType(int chId, int bitrateType);
int LIB816x_setDisplayRes(int devId, int resType);  //devId: enum DisplayPath_e, resType: enum DisplayDcMode_e
int LIB816x_setSpotChannel(int chId);
int LIB816x_setDisplayMainSub(int mainDevId, int subDevId, int spotDevId, int mainResolution, int subResolution, int spotResolution);


/*----------------------------*/
/* Record & Playback & backup */
/*----------------------------*/
int LIB816x_setRecChannel(int chId, int enableRec, int eventMode, int addAudio, char* camName);
int LIB816x_changeCamName(int chId, char* camName);

int LIB816x_setRecordingType(int iRecycle) ; // iRecycle =0:Recycle, iRecycle=1:Once
int LIB816x_getCurPlaybackTime(struct tm *tp) ;

int LIB816x_initPlayback_x(void);
int LIB816x_startPlayback_x(int ch_bitmask, struct tm *ptm);
int LIB816x_stopPlayback_x(void);
int LIB816x_jumpPlayback_x(int ch_bitmask, struct tm *ptm);
int LIB816x_pausePlayback_x(void);
int LIB816x_restartPlayback_x(int ch_bitmask);
int LIB816x_fastBackward_x(int ch_bitmask);
int LIB816x_fastforward_x(int ch_bitmask);
int LIB816x_stepPlayback_x(int ch_bitmask, int bReverse);
int LIB816x_setPlaybackProperty_x(int cmd, int ch_bitmask, int value, void *pData);

int LIB816x_backupToAVI(int media, char *path, int ch_bitmask, struct tm start_t, struct tm end_t) ;
int LIB816x_backupToBASKET(int media, char *path, int ch_bitmask, struct tm start_t, struct tm end_t) ;


/*---------------------*/
/*   CDROM,DVD         */
/*---------------------*/

int LIB816x_CDROM_MEDIA(char *device) ;
int LIB816x_CDROM_EJECT(char *device) ;
int LIB816x_CDROM_ERASE(char *device) ;
int LIB816x_CDROM_MAKE_ISO(char *iso_name, char **filelist, int filecnt) ;
int LIB816x_CDROM_WRITE_ISO(char *device, char *iso_file) ;

/*------------------*/
/* Basket functions */
/*------------------*/
int LIB816x_BasketCreate(char *path);
int LIB816x_HddFormat(char *mntpath, char *devpath, int hddstatus);

int LIB816x_BasketInfo(long *bkt_count, long *bkt_size) ;

long LIB816x_GetRecDays(struct tm t, int* pRecDayTBL);
long LIB816x_GetRecHour(int ch, struct tm t, int* pRecHourTBL);
long LIB816x_GetLastRecTime(void);



/*---------------------------*/
/* Encoder/Decoder functions */
/*---------------------------*/
int LIB816x_set_encoder_property(int type, int channel_bitmask, int *pValue);
int LIB816x_get_encoder_property(int type, int channel_bitmask, int *pValue);


/*-------------------------*/
/* Motion & Sensor & Alarm */
/*-------------------------*/

/*
    @description    set sensor information
    @param  iSensorId       [I] sensor number 0~7
    @param  iSensorEnable   [I] 0:sensor off, 1:sensor on
    @param  iSensorType     [I] 0:NO type, 1:NC type
*/
void LIB816x_setSensor(int iSensorId,int iSensorEnable,int iSensorType);

/*
    @description    get sensor bitmask
    @return sensor decting status as bitmask value
*/
unsigned int LIB816x_getSensorStatus();

/*
    @description    set alarm information
    @param  iAlarmId        [I] alarm number 0~3
    @param  iAlarmEnable    [I] 0:alarm off, 1:alarm on
    @param  iAlarmType      [I] 0:NO type, 1:NC type
    @param  iAlarmDelay     [I] alarm out interval 1~30(sec.)
*/
void LIB816x_setAlarm(int iAlarmId,int iAlarmEnable,int iAlarmType,int iAlarmDelay);

/*
    @description    alarm on or off immediately
    @param  iAlarmId        [I] alarm number 0~3
    @param  iAlarmOnOff     [I] 0:alarm off, 1:alarm on
*/
void LIB816x_operateAlarm(int iAlarmId,int iAlarmOnOff);

/*
    @description    get alarm bitmask
    @return alarm decting status as bitmask value
*/
unsigned int LIB816x_getAlarmStatus();

void LIB816x_setDIOCallback(void* fncb);



/*------------------------*/
/* PTZ internal functions */
/*------------------------*/
/*
    @description    get the embedded ptz control list
    @param  ptzIdx      [I] selected channel, zero base.
    @return ptz control list count.
*/
int LIB816x_getIntPtzCount();

/*
    @description    get the embedded ptz control information.
    @param  ptzIdx          [I] ptz controller index from LIB816x_getIntPtzCount() funcion.  zero base.
    @param  *pPtzName       [0]  ptz controller name (max 32bytes)
    @return 1 on sucess else failure
*/
int LIB816x_getIntPtzInfo(int ptzIdx, char* pPtzName);

/*
    @description    control functions of the ptz controller
    @param  ptzIdx          [I] selected ptzIdx, zero base.
    @param  ptzTargetAddr   [I] ptz controller target address
    @return size of send bytes on success else 0.
*/
int LIB816x_ptzCtrl(int ptzIdx, int ptzTargetAddr,int ctrl);


/*--------------------*/
/* PTZ user functions */
/*--------------------*/
/*
    @description    set functions for serial config.
    //////  You must call before use LIB816x_ptzStop(),LIB816x_ptzLeft()..., LIB816x_ptzSendBypass(). //////
    @param  ptzDataBit      [I] data bit of ptz controller. (5, 6, 7, 8).
    @param  ptzParityBit    [I] parity bit of ptz controller. (0: NONE, 1: EVEN, 2: ODD)
    @param  ptzStopBit      [I] stopbit of ptzIdx controller.   (1, 2)
    @param  ptzBaudRate     [I] baud rate of ptzIdx controller. (1200 ~ 115200)
    @return always 1.
*/
int LIB816x_setPtzSerialInfo(int ptzDataBit, int ptzParityBit, int ptzStopBit, int ptzBaudRate);

/*
    @description    control functions of the ptz controller with make by user
    @param  ptzBuf      [I] protocol data of the ptz controller.
    @param  bufSize     [I] size of ptzBuf
    @return size of send bytes on success else failure
*/
int LIB816x_ptzSendBypass(char* ptzBuf, int bufSize);


/*-------------------*/
/*  System Info      */
/*-------------------*/
int LIB816x_sys_info(char *mac0, char *mac1, char *hwver);
int LIB816x_net_info(int set, int devno, void *net_info);
int LIB816x_disk_info(void *mount_info);
int LIB816x_disk_size(char *mntpath, unsigned long *total, unsigned long *used);
///MULTI-HDD SKSUNG///
int LIB816x_rec_disk_size(unsigned long *total, unsigned long *used);

void LIB816x_systemReboot(void);

/*-------------------*/
/*  RTSP Stream      */
/*-------------------*/
void LIB816x_startRTSP();
void LIB816x_stopRTSP();

/*-------------------*/
/* Display Start     */
/*-------------------*/
void LIB816x_Vdis_start();

/*-------------------*/
/* Display Driver Start(Graphics & Video) */
/*-------------------*/

void LIB816x_Vdis_startDrv(int displayId);

/*-------------------*/
/* Display Driver Stop(Graphics & Video) */
/*-------------------*/

void LIB816x_Vdis_stopDrv(int displayId);




#ifdef __cplusplus
}
#endif

#endif
