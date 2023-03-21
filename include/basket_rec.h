/**
 @file basket_rec.h
 @defgroup BASKET_REC BSK_RECORD
 @ingroup BASKET
 @brief
 */

///////////////////////////////////////////////////////////////////////////////
#ifndef _BASKET_REC_H_
#define _BASKET_REC_H_
///////////////////////////////////////////////////////////////////////////////

#include "udbasket.h"

///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"{
#endif
///////////////////////////////////////////////////////////////////////////////

/**
  @struct BKTREC_CTRL
  @ingroup BASKET_REC
  @brief value of basket record control
  */
typedef struct
{
	int  fps[BKT_MAX_CHANNEL]; ///< frame per seconds
	int  bps[BKT_MAX_CHANNEL]; ///< byte per seconds
	long prev_recv_sec[BKT_MAX_CHANNEL]; ///< previous received seconds from WRITER

	long bid;		    ///< basket id
	long fpos_bkt;      ///< file position of bkt
	long fpos_idx;      ///< file position of idx
	long prev_day;      ///< previous day

	char path_bkt[LEN_PATH]; ///< basket file path
	char path_idx[LEN_PATH]; ///< index file path

	long prev_min[BKT_MAX_CHANNEL]; ///< previous minutes. each basket timestamp all channel

	T_BASKET_TIME bkt_secs; ///< for all channel
	T_BASKET_TIME idx_secs; ///< for all channel

	char b_buf[BKT_BBUF_SIZE]; ///< stream buffer
	int  b_pos;

	char i_buf[BKT_IBUF_SIZE]; ///< index buffer
	int  i_pos;
	int  idx_pos[BKT_MAX_CHANNEL];

	char r_buf[BKT_RBUF_SIZE]; ///< rdb buffer
	int  r_pos;
	int  r_skip;
	int  target_status ;
	char target_path[LEN_PATH];
	int  bkt_cnt ;

}BKTREC_CTRL;

typedef struct {
    int hddcnt ;
    char curdiskpath[40] ;
} BKT_Mount ;

extern BKT_Mount gbkt_mount ;

/*
 * RETURN VALUE
 *	On success, count of BASKET_FILES is returned. On error, -1 is returned.
 * PARAMETER
 *	path is the pathname of mounted storage(HDD). ex) /mnt/disk1, /mnt/disk2
 */
int  BKTREC_TotalBasketInfo(long *bktcnt, long *bktsize ) ;
int  BKTREC_BasketInfo(const char *diskpath, long *bkt_count, long *bkt_size ) ;


/**
  @brief create basket files with target path
  @ingroup BASKET_REC
  @param[in] path target path
  @return 1 if succeed, -1 if failed
  */
long BKTREC_CreateBasket(const char *path);
long BKTREC_CreateBasketFiles(const char *diskpath, long bkt_size, long bkt_count);

/*
 * RETURN VALUE
 *	On success, count of deleted files, On error, -1 is returned.
 * PARAMETER
 *	path is the pathname of mounted storage(HDD). ex) /mnt/disk1, /mnt/disk2
 */

long BKTREC_deleteBasket(const char *path);
long BKTREC_showBasket(const char *path);

/**
  @brief Check basket files and search a target basket file.
  @ingroup BASKET_REC
  @param[in] path target path
  @return 1 if succeed, -1 if failed
  */
long BKTREC_open(const char *path);
long BKTREC_SaveFull();
long BKTREC_exit( int save_flag, int flushbuffer);

/**
  @brief write video stream
  @ingroup BASKET_REC
  @param[in] pv video frame and information
  @return 1 if succeed, -1 if failed
  */
long BKTREC_WriteVideoStream(T_VIDEO_REC_PARAM* pv);

/**
  @brief write audio stream
  @ingroup BASKET_REC
  @param[in] pa audio frame and information
  @return 1 if succeed, -1 if failed
  */
long BKTREC_WriteAudioStream(T_AUDIO_REC_PARAM* pa);

/////MULTI-HDD SKSUNG/////
int BKTREC_setTargetDisk(const int curHddIdx);


#if 0
int BKTREC_getTargetDisk(const char *path, int status) ;
#else
int BKTREC_getTargetDisk() ;
#endif

int BKTREC_updateHddInfo();
//////////////////////////

int  BKTREC_IsOpenBasket();
long BKTREC_FlushWriteBuffer();





///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#endif//_BASKET_REC_H_
///////////////////////////////////////////////////////////////////////////////












