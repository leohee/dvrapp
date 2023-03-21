/**
 @file udbasket.h
 @defgroup BASKET The Basket Storage System
 @brief
 */


///////////////////////////////////////////////////////////////////////////////
#ifndef _UDBASKET_H_
#define _UDBASKET_H_
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "app_manager.h"

//////////////////////////////////////////////////////////////////////////
#define SEP_AUD_TASK
#define PREV_FPOS          // AYK - 0201
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
/// determine use file descriptor(int) or FILE*
#define BKT_SYSIO_CALL
//////////////////////////////////////////////////////////////////////////

/////MULTI-HDD SKSUNG/////
#define MAX_HDD_COUNT	MAX_PARTITION
//////////////////////////


/// basket max channel. same as supported dvr max channel
#define BKT_MAX_CHANNEL 	MAX_DVR_CHANNELS

#define BKT_OK 1        ///< OK
#define BKT_FULL 2      ///< basket status is full.  Using Only if g_bkt_rec_status is BKT_STATUS_FULL
#define BKT_NOAUDFR   3   // AYK - 0201
#define BKT_NONEXTBID 4   // AYK - 0201

////MULTI-HDD SKSUNG////
#define BKT_OVERHDD		5
#define BKT_CREATE_FAIL 6
#define BKT_OPEN_FAIL 	7
#define BKT_READ_FAIL 	8
#define BKT_WRITE_FAIL 	9
////////////////////////

#define BKT_ERR -1      ///< Failed or error

#define S_OK   1
#define S_FAIL 0

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define MAX_NUM_FRAME_SEARCH 64 // AYK - 0201
///////////////////////////////////////////////////////////////////////////////
#define SIZE_KB 1024 ///< kilo bytes
#define SIZE_MB (SIZE_KB*SIZE_KB)
#define SIZE_GB (SIZE_MB*SIZE_MB)

#define LEN_PATH   64    ///< length of path
#define LEN_TITLE  32    ///< length of title
#define BKT_PATH_LEN 64  ///< length of basket path
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// seconds
#define SEC_DAY  86400 ///< total seconds of a day, 3600*24
#define SEC_HOUR 3600  ///< total secodns of a hour, 60*60
#define SEC_MIN  60    ///< total seconds of a minute
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// default basket size. unit size of file seperate
//#define BASKET_SIZE (10*SIZE_MB)
//#define BASKET_SIZE 104857600 // (100*SIZE_MB)
//#define BASKET_SIZE 209715200 ///< (200*SIZE_MB)
//#define BASKET_SIZE 314572800 // (300*SIZE_MB)
#define BASKET_SIZE 629145600 // (600*SIZE_MB)
///////////////////////////////////////////////////////////////////////////////

#define MAX_TIME   0x7FFFFFFF ///< 4 bytes
#define MAX_TIME64 0x7FFFFFFFFFFFFFFF ///< 8 bytes
///////////////////////////////////////////////////////////////////////////////
///< basket structure version
#define VER_UD_BASKET_FORMAT 1

#define BKT_PLAYDIR_BACKWARD 0  ///< currently playback reverse
#define BKT_PLAYDIR_FORWARD  1  ///< currently playback forward

#define BKT_FORWARD 1           ///< read basket for playback forward
#define BKT_BACKWARD 0          ///< read basket for playback reverse
#define BKT_TIMEPOINT 2         ///< read basket playback time indexing
#define BKT_NOSEARCHDIRECTION 3 ///< not searching...read record flag for only particular time point

#define BKT_PLAYMODE_NORMAL 0   ///< normal playback mode
#define BKT_PLAYMODE_IFRAME 1   ///< iframe only playback mode

#define PLAY_STATUS_STOP  0 ///< playback status is stopped
#define PLAY_STATUS_PLAY  1 ///< playback status is playing
#define PLAY_STATUS_PAUSE 2 ///< playback status is paused
#define PLAY_STATUS_SKIP  3 ///< playback status is skip frame mode

// basket status ...
// empty means that remains EMPTY baskets.
// full  means that there is no empty baskets.
#define BKT_SPACE_REMAIN (0) ///< basket not full
#define BKT_SPACE_FULL (1)   ///< basket full

/// frame type
enum _enumFrameType{FT_IFRAME=0, FT_PFRAME, FT_AUDIO};

///////////////////////////////////////////////////////////////////////////////
/// basket priority
enum _enumBasketPriority{BP_NONE=0, BP_LOW, BP_NORMAL, BP_HIGH, MAX_BP};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// stream type
enum _enumStreamType{ST_NONE=0, ST_VIDEO, ST_AUDIO, MAX_ST};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// save flag
enum _enumSaveFlag{SF_EMPTY=0, SF_USING, SF_STOP, SF_FULL, SF_ERR, SF_BACKUP, MAX_SF};

/// event flag
enum _enumEventFlag{EVT_NONE=0, ///< none
                    EVT_CONT=1, ///< continuous recording
					EVT_MOT,    ///< motion
					EVT_SENSOR, ///< sensor
					EVT_VLOSS}; ///< video loss

/// basket recycle Mode
enum _enumBasketRecycleMode{
	BKT_RECYCLE_ON=0,  ///< recycle mode
    BKT_RECYCLE_ONCE ///< not recycle. if all basket is fulled, will stop record
};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// file name
#define DEFAULT_DISK_MPOINT1 "/media/sda2"
#define DEFAULT_DISK_MPOINT2 "/media/sdb2"
//#define DEFAULT_DISK_MPOINT "/mnt/mmcblkp1"

#define NAME_TOPMGR "topmgr.udf"	//MULTI-HDD SKSUNG
#define NAME_HDDMGR "hddmgr.udf"
#define NAME_BKTMGR "bktmgr.udf"
#define HDDMGR_PATH "bin/dvrapp_cfg"
#define MAX_MGR_PATH	64

#define NAME_BKTREC_INFO "bkt_rec_info.udf"  ///< basket record info...for currently using file

#define BKTMGR_INFO_ENABLE	0

#define PREREC_ENABLE 1 // BKKIM, pre-record enable


///////////////////////////////////////////////////////////////////////////////
// identifier
#define ID_TOPMGR_HEADER   0xA0000001   ///< header structure of topmgr.udf file
#define ID_TOPMGR_BODY	   0xA0000002   ///< body structure of tomgr.udf file

#define ID_BKTMGR_HEADER   0xB0000001   ///< header structure of bktmgr.udf
#define ID_BKTMGR_BODY	   0xB0000002   ///< body structure of bktmgr.udf

#define ID_BASKET_HDR      0xC0000001
#define ID_BASKET_BODY     0xC0000002

#define ID_VIDEO_HEADER    0xD0000001
#define ID_VIDEO_STREAM	   0xD0000002

#define ID_AUDIO_HEADER    0xE0000001
#define ID_AUDIO_STREAM	   0xE0000002

#define ID_INDEX_HEADER	   0xF0000001
#define ID_INDEX_DATA	   0xF0000002
#define ID_INDEX_FOOTER    0xF0000004

#define ID_BASKET_END      0xFFFFFFFF
///////////////////////////////////////////////////////////////////////////////

/// 1~31, 0 is not used
#define MAX_RECDATE 32

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif//max

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif//min

#define HI32(h) ((short)((long)(h) >> 16))
#define LO32(l) ((short)((long)(l) & 0xffff))
#define MK32(h, l) ((long)(((short)((long)(l) & 0xffff)) | ((long)((short)((long)(h) & 0xffff))) << 16))

#define HI64(l) ((unsigned long)((int64_t)(l) >> 32))
#define LO64(l) ((unsigned long)((int64_t)(l) & 0xffffffff))
#define MK64(h, l) ((int64_t)(((long)((int64_t)(l) & 0xffffffff)) | ((int64_t)((long)((int64_t)(h) & 0xffffffff))) << 32))


#ifndef SAFE_FREE
#define SAFE_FREE(p) if(NULL != p){free(p);p=NULL;}
#endif

#ifdef BKT_SYSIO_CALL

/// basket file descriptor
#define BKT_FHANDLE int
#define BKT_ISVALIDHANDLE(fd) (fd > 0)


#ifndef O_DIRECT
#define O_DIRECT	0400000
#endif

#define OPEN_CREATE(fd, path) (-1 != (fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0644)))
#define OPEN_EMPTY(fd, path)  (-1 != (fd = open(path, O_RDWR|O_TRUNC, 0644)))
#define OPEN_RDWR(fd, path)   (-1 != (fd = open(path, O_RDWR, 0644)))
#define OPEN_RDONLY(fd, path) (-1 != (fd = open(path, O_RDONLY, 0644)))
//#define OPEN_RDWR(fd, path)   (-1 != (fd = open(path, O_RDWR|O_NODELAY, 0644)))
//#define OPEN_RDONLY(fd, path) (-1 != (fd = open(path, O_RDONLY|O_NODELAY, 0644)))


//#define OPEN_RDWR(fd, path)   (-1 != (fd = open(path, O_RDWR|O_SYNC, 0644)))

//#define OPEN_RDWR(fd, path)   (-1 != (fd = open(path, O_RDWR|O_DIRECT, 0644)))
//#define OPEN_RDONLY(fd, path) (-1 != (fd = open(path, O_RDONLY|O_DIRECT)))



#define SAFE_CLOSE_BKT(fd) if(fd){ close(fd);fd=0;};
#define CLOSE_BKT(fd) close(fd) //(-1 != close(fd))

#define READ_ST(fd, st) ( 0 < read(fd, &st, sizeof(st)))
#define READ_HDR(fd, st) ( 0 < read(fd, &st, sizeof(st)))
#define READ_STRC(fd, st) read(fd, &st, sizeof(st))
#define READ_PTSZ(fd, pt, sz)   ( 0 < read(fd, pt, sz))

#define WRITE_ST(fd, st) ( 0 < write(fd, &st, sizeof(st)))
#define WRITE_PTSZ(fd, pt, sz) ( 0 < write(fd, pt, sz))

#define LSEEK_END(fd, pos) ( -1 != lseek(fd, (off_t)pos, SEEK_END))
#define LSEEK_CUR(fd, pos) ( -1 != lseek(fd, (off_t)pos, SEEK_CUR))
#define LSEEK_SET(fd, pos) ( -1 != lseek(fd, (off_t)pos, SEEK_SET))

#define LTELL(fd) lseek(fd, (off_t)0, SEEK_CUR)

#else

/// basket file pointer
#define BKT_FHANDLE FILE*
#define BKT_ISVALIDHANDLE(fd) (fd != NULL)

#define OPEN_CREATE(fd, path) (NULL != (fd = fopen(path, "wb+")))
#define OPEN_EMPTY(fd, path) (NULL != (fd = fopen(path, "wb+")))
#define OPEN_RDONLY(fd, path) (NULL != (fd = fopen(path, "rb")))
#define OPEN_RDWR(fd, path) (NULL != (fd = fopen(path, "rb+")))

#define SAFE_CLOSE_BKT(fd) if(fd){ fclose(fd);fd=NULL;};
#define CLOSE_BKT(fd) fclose(fd)

#define READ_ST(fd, st)  ( sizeof(st) == fread(&st, 1, sizeof(st), fd))
#define READ_HDR(fd, st) ( sizeof(st) == fread(&st, 1, sizeof(st), fd))
#define READ_STRC(fd, st) fread(&st, 1, sizeof(st), fd)
#define READ_PTSZ(fd, pt, size)   ( size == fread(pt, 1, size, fd))

#define WRITE_ST(fd, st) ( sizeof(st) == fwrite(&st, 1, sizeof(st), fd))
#define WRITE_PTSZ(fd, pt, sz)   ( sz == fwrite(pt, 1, sz, fd))

#define LSEEK_END(fd, pos) (-1 != fseek(fd, pos, SEEK_END))
#define LSEEK_CUR(fd, pos) (-1 != fseek(fd, pos, SEEK_CUR))
#define LSEEK_SET(fd, pos) (-1 != fseek(fd, pos, SEEK_SET))

#define LTELL(fd) ftell(fd)

#endif//BKT_SYSIO_CALL
//////////////////////////////////////////////////////////////////////////


#define BKT_STATUS_CLOSED (0)
#define BKT_STATUS_OPENED (1)
#define BKT_STATUS_IDLE (2)
#define BKT_STATUS_FULL (3)

////////////////////////////////////////////////////////////////////////////////


#define SIZEOF_CHAR 1
#define TOTAL_MINUTES_DAY (1440)//24*60
////////////////////////////////////////////////////////////////////////////////

//#pragma pack(push, 1)

/**
  @struct T_TIMESTAMP
  @ingroup BASKET
  @brief  basket timestamp
  */
typedef struct
{
    long sec;	///< seconds
	long usec;  ///< microseconds (1/1000000 seconds)
}T_TIMESTAMP, T_TS;
/// size of stucture timestamp
#define SIZEOF_TIMESTAMP 8

/**
  @struct T_BASKET_TIME
  @ingroup BASKET
  @brief  bundle all channel for basket timestamp
  */
typedef struct
{
    T_TIMESTAMP t1[BKT_MAX_CHANNEL]; ///< start time stamps
    T_TIMESTAMP t2[BKT_MAX_CHANNEL]; ///< end time stamps
}T_BASKET_TIME;

/**
  @brief basket tm value. same as struct tm
  @struct T_BKT_TM
  @ingroup BASKET
  */
typedef struct
{
	int year; ///< 1970~
	int mon;  ///< 1~12
	int day;  ///< 1~31
	int hour; ///< 0~23
	int min;  ///< 0~59
	int sec;  ///< 0~59
}T_BKT_TM;
void BKT_GetLocalTime(long sec, T_BKT_TM* ptm);

//////////////////////////////////////////////////////////////////////////
//#define BKT_BBUF_SIZE 0x800000 //8192*1024
//#define BKT_BBUF_SIZE 0x400000 //4096*1024
#define BKT_BBUF_SIZE (12 * 1024 * 1024) ///< basket stream buffer size
//#define BKT_BBUF_SIZE 0x100000 //1024*1024
//#define BKT_BBUF_SIZE 524288 //512*1024
//#define BKT_BBUF_SIZE 393216 //384*1024
//#define BKT_BBUF_SIZE 262144 //256*1024
//#define BKT_BBUF_SIZE 131072 //128*1024
#define BKT_IBUF_SIZE (1 * 1024) ///< basket index buffer size
#define BKT_RBUF_SIZE 1024 ///< basket rdb buffer size
//////////////////////////////////////////////////////////////////////////

/**
 @struct T_BASKET_RDB_PARAM
 @ingroup BASKET_REC
 @brief basket RDB parameter
 */
typedef struct
{
	int  ch;      ///< channel
	long sec;     ///< seconds
	int  evt;     ///< event type, save flag(continuous, motion, alarm )
	long bid;     ///< basket id
	long idx_pos; ///< index file point
}T_BASKET_RDB_PARAM;
#define SIZEOF_RDB_PRM 20

/**
 @struct T_BASKET_RDB
 @ingroup BASKET_REC
 @brief RDB
 */
typedef struct
{
	long bid;     ///< basket id
	long idx_pos; ///< index file point
}T_BASKET_RDB;
#define SIZEOF_RDB 8

/**
 @struct T_BKTREC_INFO
 @ingroup BASKET_REC
 @brief  basket recording information
 */
typedef struct
{
	int  save_flag;      ///< save flag. @see _enumSaveFlag
	long bid;            ///< basket id
	char path[LEN_PATH]; ///< current record basket's path
}T_BKTREC_INFO;


/**
 @struct BKT_HDD_Info
 @ingroup BASKET
 @brief basket MULTI-HDD SKSUNG
 */
typedef struct {
	int  isMounted;
	int  prevHddIdx;
	int  nextHddIdx;
	long formatDate;
	long initUsedSize;
	char mountDiskPath[LEN_PATH];
} BKT_HDD_Info;

/**
 @struct BKT_Mount_Info
 @ingroup BASKET
 @brief basket HDD mount information SKSUNG
 */
typedef struct {
	int hddCnt;
	int curHddIdx;
	BKT_HDD_Info hddInfo[MAX_HDD_COUNT];
} BKT_Mount_Info;

extern BKT_Mount_Info gbkt_mount_info;
////////////////////////

/**
 @struct T_TOPMGR
 @ingroup BASKET
 @brief management information for multi hdd
 */
typedef struct
{
	long id ;			///< top manager id
	long hddIdx ;		///< hdd index (0 base)
	long nextHddIdx ;	///< next recording hdd index
	long formatDate ;	///< format date (seconds)
	long initUsedSize ;	///< initial used size after hdd format.
	long reserved2 ;
	char dvrMAC[LEN_PATH] ;	///< DVR's MAC address on mounted.
} T_TOPMGR ;
/////////////////////////

typedef struct _tHddMgr
{
	long curid ;
	long nextid ;
	int  hddcnt ;
	int  hddfull[2] ;
	char path[2][LEN_PATH] ;
} T_HDDMGR ;

/**
 @struct T_BKTMGR_HDR
 @ingroup BASKET
 @brief  basket manager header
 */
typedef struct
{
    long id;            ///< header id
    long mgrid;	        ///< manager file numbering
    long date;		    ///< seconds since the Epoch
    long latest_update;	///<  date of latest update
    long bkt_count;     ///< basket count
    long reserved;
}T_BKTMGR_HDR;

/**
 @struct T_BKTMGR_BODY
 @ingroup BASKET
 @brief  basket manager body. same as Basket header info..@see T_BASKET_HDR
 */
typedef struct
{
    long id;              ///< header id
    long bid;             ///< basket id
    char path[LEN_PATH];  ///< basket path
    long latest_update;   ///< latest update
    long lPriority;       ///< priority, @ref _enumBasketPriority
    long s_type;
	T_BASKET_TIME ts;
    long save_flag;
    long lRecycleCount;
    long reserved;
}T_BKTMGR_BODY;

/**
 @struct T_BASKET_HDR
 @ingroup BASKET
 @brief  basket header.
 */
typedef struct
{
    long id;            ///< basket header id
    long bid;           ///< basket number
    long latest_update; ///< update seconds
    long lPriority;     ///< priority, @ref _enumBasketPriority
    long s_type;        ///< video or audio
	T_BASKET_TIME ts;   ///< time stamp
    long save_flag;     ///< basket save flag, stop, full, using...
    long totalframecount; ///< total frame count
    long reserved;  
}T_BASKET_HDR;


/**
 @struct T_VIDEO_REC_PARAM
 @ingroup BASKET_REC
 @brief  video record param for write stream
 */
typedef struct
{
    long  ch;         ///< channel
    long  framesize;  ///< frame size
    T_TS  ts;         ///< timestamp

    long  event;      ///< event type
    short frametype;  ///< frame type
    short framerate;  ///< frame rate
    short width;      ///< frame width
    short height;     ///< frame height
    char* buff;       ///< frame buffer
    char camName[16]; ///< channel title

    short  audioONOFF; ///< AYK - 0201, with audio or not
	int   capMode;     ///< N/A, BK - 0428 : 2D1, 4CIF, 8CIF, 4D1

}T_VIDEO_REC_PARAM;

/**
 @struct T_AUDIO_REC_PARAM
 @ingroup BASKET_REC
 @brief  audio record param for write stream
 */
typedef struct
{
    long ch;            ///< channel
    long framesize;     ///< frame size
    T_TS ts;            ///< timestamp

    long samplingrate;  ///< sampling rate
    long bitspersample; ///< bitrate
    long achannel;      /// stero or mono?

    char* buff;         ///< frame buffer
}T_AUDIO_REC_PARAM;

///////////////////////////////////////////////////////////////////////////////
/**
 @struct T_STREAM_HDR
 @ingroup BASKET
 @brief stream common header
 */
typedef struct
{
    long id;         ///< video or audio
    long ch;         ///< channel
    long framesize;  ///< frame size
    T_TS ts;         ///< timestamp

    long frametype;  /// video frame type.i or p frame...

#ifdef PREV_FPOS    // AYK - 0201
    long prevFpos;   ///< previous file point
#endif

	long reserved1;
	long reserved2;
	long reserved3;
	long reserved4;

}T_STREAM_HDR;

#ifdef PREV_FPOS    // AYK - 0201
    #define SIZEOF_STREAM_HDR (44)
#else
    #define SIZEOF_STREAM_HDR (40)
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 @struct T_VIDEO_STREAM_HDR
 @ingroup BASKET
 @brief video stream header
 */
typedef struct
{
    short audioONOFF;	///< 0:No, 1:Yes
    short framerate;    ///< frame rate
    long  event;		///< Continuous, Motion, Sensor, ...
    short width;        ///< frame width
    short height;       ///< frame height
    char  camName[16];  ///< camera title

	int  capMode;       ///< N/A, 2D1, 4CIF, 8CIF, 4D1

	long reserved1;
	long reserved2;

}T_VIDEO_STREAM_HDR;
#define SIZEOF_VSTREAM_HDR (40)

///////////////////////////////////////////////////////////////////////////////
/**
 @struct T_AUDIO_STREAM_HDR
 @ingroup BASKET
 @brief audio stream header
 */
typedef struct
{
    long samplingrate;  ///< sampling rate
    long bitspersample; ///< bits per sample
    long achannel;      ///< stero or mono?
    long codec;         ///< codec type

    long  reserved1;
	long  reserved2;
}T_AUDIO_STREAM_HDR;
#define SIZEOF_ASTREAM_HDR (24)


///////////////////////////////////////////////////////////////////////////////
/**
 @struct T_INDEX_HDR
 @ingroup BASKET
 @brief index header
 */
typedef struct
{
	long   id;        ///< index header id
    long   bid;       ///< Maybe this is same as basket id...
    long   count;     ///< count index data
	T_BASKET_TIME ts; ///< timestamp
    long   reserved;
}T_INDEX_HDR;
#define SIZEOF_INDEX_HDR 144

/**
 @struct T_INDEX_DATA
 @ingroup BASKET
 @brief index data
 */
typedef struct
{
	long  id;		///< index data struct id
	T_TS  ts;		///< time stamp..sec and usec
	long  fpos;		///< file position
	long  ch;       ///< channel
	long  s_type;	///< stream type audio, video, etc..
	long  event;    ///< event type
	short width;	///< video frame width
	short height;	///< video frame height

	int   capMode;  ///< N/A, capture mode...

    long  reserved1;
	long  reserved2;

}T_INDEX_DATA;
#define SIZEOF_INDEX_DATA 44

typedef struct _tBasketDecodeParameter
{
    long fpos;
    long ch;

    T_TS ts;

    long  framesize;
    short framerate;
    short frameWidth;
    short frameHeight;
    int   frametype;

    long  streamtype;
    long  event;

    long  samplingrate;
    long  achannel; // mono or stero
    long  bitpersample;
    char  camName[16];

    unsigned char *vbuffer; // video read buffer pointer
    unsigned char *abuffer; // audio read buffer pointer

    long bktId;
    long fpos_bkt;

    short audioONOFF; // AYK - 0201
	int   capMode;  // 2D1, 4CIF, 8CIF, 4D1

	int*  decFlags; // current decode flag, 0:no, 1:decode

}T_BKTDEC_PARAM,T_BKTSTM_PARAM;
//#pragma pack(pop)

/**
 @brief search reverse basket for decoding....
 @see manpage...(man scandir, alphsort)
 @param[in] a compare parameter 1
 @param[in] b compare parameter 2
 @return less than zero if the first argument is considered to be respectively greater than
 @return zero if the first argument equal to the second.
 @return less than zero if the first argment is greater than second.
 */
int  alphasort_reverse(const void *a, const void *b);

/**
 @brief search minimum timestamp of array all channel's timestamp
 @param[in] arr_t array of all channel's timestamp
 @return timestamp as seconds
 */
long BKT_GetMinTS(T_TIMESTAMP *arr_t);

/**
 @brief search maximum timestamp of array all channel's timestamp
 @param[in] arr_t array of all channel's timestamp
 @return timestamp as seconds
 */
long BKT_GetMaxTS(T_TIMESTAMP *arr_t);

////////////////////////////////////////////////////////////////////////////////
// search ....outer interfaces


/*
 * RETURN VALUE
 *
 * PARAMETER
 *
 */

// 0:on, 1:once

#define BKT_REC_CLOSE_SLIENT 0     ///< N/A
#define BKT_REC_CLOSE_BUF_FLUSH 1  ///< flush basket stream buffer before file close.

/**
 @brief close all basket handles. rec. streaming. dec....
 @return 0 fixed value
 */
long BKT_closeAll();

/**
 @brief set basket recycle mode
 @param[in] recycle recycle mode. @ref _enumBasketRecycleMode
 @return 1 fixed
 */
int  BKT_SetRecycleMode(int recycle);


// BK - End

/**
 @brief Get Index Count of recording idx file.
 @return count of index
 */
inline long BKT_GetRecordingIndexCount();

/**
 @brief trans index file pointer to count
 @param[in] fpos file point
 @return count index data of index file
 */
inline long BKT_transIndexPosToNum(long fpos);

/**
 @brief trans count of index to index file pointer
 @param[in] index_number index number
 @return file point
 */
inline long BKT_transIndexNumToPos(int index_number);

//////////////////////////////////////////////////////////////////////////
// recording interfaces
/**
 @brief the basket file is recording?
 @param[in] bid basket id
 @return 0 if not recording
 @return 1 if recording
 */
int BKT_isRecordingBasket(long bid);
int BKT_isRecording(long bid, char* path);
//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif


///////////////////////////////////////////////////////////////////////////////
#endif//_BASKET_H_
///////////////////////////////////////////////////////////////////////////////

