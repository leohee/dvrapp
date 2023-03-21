/********************************
** common_def.h
**
** by CSNAM, 2010/11/24
**
** Copyright nSolutions Co.
**
*********************************/

#ifndef __NSDVR_COMMON_DEF__
#define __NSDVR_COMMON_DEF__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/////////////////////////////////
// Data types
/////////////////////////////////
typedef void*	handle;

/*
#ifndef ulong
typedef	unsigned long ulong;
#endif
*/
#include <sys/types.h>
//typedef unsigned char	BOOL;
//#define TRUE	1
//#define FALSE	0

#define NSDVR_MAX_PATH_LENGTH		256
#define NSDVR_MAX_CHANNELS		16
#define NSDVR_MAX_GOP		30
#define NSDVR_MAX_FRAME_SIZE		(256*1024)

typedef enum {
	NSDVR_OPEN_WRITE,
	NSDVR_OPEN_READ,
	NSDVR_OPEN_MANAGE,
} enumNSDVR_OPEN_MODE;

/////////////////////////////////
// Error code
/////////////////////////////////
enum {
	NSDVR_ERR_NONE=0,
	NSDVR_OK = NSDVR_ERR_NONE,
	
	NSDVR_ERR_SYSTEM					= -1,
	NSDVR_ERR_INVALID_HANDLE	= -100,
	NSDVR_ERR_INVALID_MODE,
	NSDVR_ERR_INVALID_CHANNEL,
	
	NSDVR_ERR_BASKETDB,
	NSDVR_ERR_BASKET,
	NSDVR_ERR_FULL,
	NSDVR_ERR_NO_MORE_DATA,
	NSDVR_ERR_TOO_BIG,
	NSDVR_ERR_ALREADY_INUSE,
	
	NSDVR_ERR_CREATE_FILE,
	NSDVR_ERR_OPEN_FILE,
	NSDVR_ERR_FILE_NAME,
	
	NSDVR_ERR_RANGE,
	NSDVR_ERR_INVALID_DATA,
	NSDVR_ERR_INVALID_PARAM,
	NSDVR_ERR_NOT_SUPPORT,
	
	NSDVR_ERR_NOT_EXIST,
	NSDVR_ERR_NOT_INITIALIZED,
	NSDVR_ERR_NOT_READY,
};


/////////////////////////////////
// Frame
/////////////////////////////////

typedef enum {
	NSDVR_VIDEO_TYPE_UNKNOWN=0,
	NSDVR_VIDEO_TYPE_NTSC,
	NSDVR_VIDEO_TYPE_PAL,
} enumNSDVR_VIDEO_TYPE;

typedef enum {
	NSDVR_DATA_TYPE_VIDEO,
	NSDVR_DATA_TYPE_AUDIO,
	NSDVR_DATA_TYPE_TEXT,
} enumNSDVR_DATA_TYPE;

typedef enum {
	NSDVR_PICTURE_TYPE_I,
	NSDVR_PICTURE_TYPE_P,
	NSDVR_PICTURE_TYPE_B,
} enumNSDVR_PICTURE_TYPE;

typedef enum {
	NSDVR_ENCODING_UNKNOWN=0,
	
	// Video
	NSDVR_ENCODING_MPEG4=1,
	NSDVR_ENCODING_JPEG,
	NSDVR_ENCODING_H264,
	
	// Audio
	NSDVR_ENCODING_PCM=32,
	NSDVR_ENCODING_ULAW,
	NSDVR_ENCODING_ALAW,
	NSDVR_ENCODING_ADPCM,
	NSDVR_ENCODING_G721,
	NSDVR_ENCODING_G723,
	NSDVR_ENCODING_MP3,
} enumNSDVR_ENCODING_TYPE;

typedef enum {
	NSDVR_CODEC_VENDOR_UNKNOWN=0,
	NSDVR_CODEC_VENDOR_PENTAMICRO,
	NSDVR_CODEC_VENDOR_SOFTLOGIC,
	NSDVR_CODEC_VENDOR_ALOGICS,
	NSDVR_CODEC_VENDOR_TI,
	NSDVR_CODEC_VENDOR_HISILICON,
} enumNSDVR_CODEC_VENDORE;

typedef enum {
	NSDVR_PLAY_DIRECTION_FORWARD,
	NSDVR_PLAY_DIRECTION_BACKWARD,
} enumNSDVR_PLAY_DIRECTION;

typedef enum {
	NSDVR_PLAY_MODE_STOP,
	NSDVR_PLAY_MODE_NORMAL,
	NSDVR_PLAY_MODE_PAUSE,
	NSDVR_PLAY_MODE_STEP,
} enumNSDVR_PLAY_MODE;

typedef struct {
	long basket;
	long pos;
} stNSDVR_FrameID;

typedef struct stream_info {
	stNSDVR_FrameID frame_id;
	ulong frame_pos;
	//
	char *buf;
	int size;
	//
	int channel;
	int data_type;
	int time_sec;
	int time_usec;
	int encoding_type;
	int codec_vendor;
	//
	int video_width;
	int video_height;
	int picture_type;
} stNSDVR_FrameInfo;


/////////////////////////////////
// Event code
/////////////////////////////////
typedef enum {
	// RECORDING EVENT
	// shoud be less than 32
	NSDVR_EVENT_MOTION,
	NSDVR_EVENT_SENSOR,
	NSDVR_EVENT_VLOSS,
	NSDVR_EVENT_BLIND,

	NSDVR_EVENT_PANIC_REC,
	NSDVR_EVENT_SCHEDULE_REC,
	NSDVR_EVENT_EVENT_REC,
	NSDVR_EVENT_MOTION_REC,
	NSDVR_EVENT_SENSOR_REC,
	NSDVR_EVENT_VLOSS_REC,
	NSDVR_EVENT_BLIND_REC,

	// SYSTEM EVENT
	// shoud be less than 32
	NSDVR_EVENT_POWER_ON,
	NSDVR_EVENT_POWER_OFF,
	
	NSDVR_EVENT_LOGIN,
	NSDVR_EVENT_LOGOUT,
	
	NSDVR_EVENT_CONFIG_CHANGED,
	
	NSDVR_EVENT_HDD_FORMAT,
	NSDVR_EVENT_HDD_ERROR,
	NSDVR_EVENT_HDD_FULL,
	NSDVR_EVENT_HDD_OVERWRITE,
} enumBSDVR_EVENT_TYPE;

#define NSDVR_MAX_EVENT_DESC_LEN	128

typedef stNSDVR_FrameID	stEventID;

typedef struct {
	stEventID id;
	int type;
	int on_off;
	time_t time;
	int device_no;
	ulong rec_mask;
	char desc[NSDVR_MAX_EVENT_DESC_LEN];
} stNSDVR_EventInfo;

typedef stNSDVR_FrameID	stTextID;
typedef struct {
	stTextID id;
	time_t time;
	int device_no;
	ulong rec_mask;
	char *text;
	int size;
} stNSDVR_TextInfo;

/***************************************************************************** 
 ** debug functions
 *****************************************************************************/
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

extern int _nsdvr_debug_level;

#define NSDVR_DEBUG_NONE		0
#define NSDVR_DEBUG_LOW			1
#define NSDVR_DEBUG_HIGH		2

/*
#define DEBUG_MSG(level, format, args...)  { \
	struct tm _t; \
	struct 	timeval _cur_time;\
	if ( _ndsvr_debug_level >= level) {\
		gettimeofday(&_cur_time, 0);\
		localtime_r(&_cur_time.tv_sec, &_t);\
		fprintf(stdout,"[%02d/%02d/%02d %02d:%02d:%02d.%03ld] : ",\
			_t.tm_year+1900, _t.tm_mon+1, _t.tm_mday,\
			_t.tm_hour, _t.tm_min, _t.tm_sec, _cur_time.tv_usec/1000);\
		fprintf(stdout, format, ## args);fflush(stdout); \
	}\
}
*/
#define DEBUG_LOW(format, args...)  { \
	if ( _nsdvr_debug_level >= NSDVR_DEBUG_LOW) {\
		fprintf(stdout,"[%s:%d] : ", __FUNCTION__, __LINE__);\
		fprintf(stdout, (char*)format, ## args);\
		fflush(stdout); \
	}\
}

#define DEBUG_HIGH(format, args...)  { \
	if ( _nsdvr_debug_level >= NSDVR_DEBUG_HIGH) {\
		fprintf(stdout,"[%s:%d] : ", __FUNCTION__, __LINE__);\
		fprintf(stdout, (char*)format, ## args);\
		fflush(stdout); \
	}\
}

#define m_MSG		DEBUG_LOW
#define m_DEBUG	DEBUG_LOW
#define m_ERROR	DEBUG_LOW

#ifndef DBG
#define DBG {printf("%s:%d\n", __FUNCTION__, __LINE__); fflush(stdout);}
#endif

#endif // __NSDVR_COMMON_DEF__
