/**
 @file basket_muldec.h
 @defgroup BASKET_MULDEC Multiple Decoding
 @ingroup BASKET
 @brief Implements of basket's  multiple-decode
 */

///////////////////////////////////////////////////////////////////////////////
#ifndef _BASKET_MULDEC_H_
#define _BASKET_MULDEC_H_
///////////////////////////////////////////////////////////////////////////////

#include <udbasket.h>
#include "global.h"

#ifdef __cplusplus
extern "C"{
#endif

#define BKTMD_STATUS_STOP (0)
#define BKTMD_STATUS_OPEN (1)

#define BACKUPDIR   "/media/sda2/backup"
#define MAXLINE	     100	
#define BACKUP_NAME  30

/**
 @struct T_BKTMD_OPEN_PARAM
 @ingroup BASKET_MULDEC
 @brief  This structure contains parameters for playback to open BASKET
 @param[out] openbid                      : basket file ID
 @param[out] openfpos                     : file pointer of opened
 @param[out] withAudio                    : whether with audio or not
 @param[out] open_ts                      : timestamp
 @param[out] firstCh                      : be found first channel by timestamp 
 @param[out] recChannelCount              : only record, count of channel
 @param[out] recChannels[BKT_MAX_CHANNEL] : 1: used for recording, 0: not used for recording
 @param[out] frameWidth                   : frame width
 @param[out] frameHeight                  : frame height
 @param[out] capMode                      : 2D1, 4CIF, 8CIF, 4D1
*/
typedef struct {
	long openbid;
	long openfpos;
	long withAudio;
	T_TS open_ts;

	int  firstCh;
	int  recChannelCount;
	int  recChannels[BKT_MAX_CHANNEL];
	int  frameWidth;
	int  frameHeight;
	int  capMode;
}T_BKTMD_OPEN_PARAM;

/**
 @struct T_BKTMD_SYNC_PARAM
 @ingroup BASKET_MULDEC
 @brief  AV Sync parameter
 @param[in] ch                           : audio play channel
 @param[in] bid                          : video basket handle
 @param[in] fpos                         : video current file pointer
 @param[in] ts                           : video current timestamp
 */
typedef struct {
	int  ch;	///< audio play channel
	long bid;   ///< video basket handle
	long fpos;  ///< video current file pointer
	T_TS ts;    ///< video current timestamp
}T_BKTMD_SYNC_PARAM;

/**
 @struct T_BKTMD_SYNC_PARAM
 @ingroup BASKET_MULDEC
 */
typedef struct
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

	int  focusChannel;

    long bktId;
    long fpos_bkt;

    short audioONOFF; // AYK - 0201
	int   capMode;  // 2D1, 4CIF, 8CIF, 4D1

	int*  decFlags; // current decode flag, 0:no, 1:decode

}T_BKTMD_PARAM;

/**
 @struct T_BKTMD_CTRL
 @ingroup BASKET_MULDEC
 @brief basket control value for multiple-playback 
 */
typedef struct
{
	int  ch;          ///< particular channel
	long bid;         ///< particular basket's id

	long sec;         ///< seconds
	int  bkt_status;  ///< basket status, can set BKTMD_STATUS_OPEN or BKTMD_STATUS_STOP

	BKT_FHANDLE hbkt; ///< handle of basket file
	BKT_FHANDLE hidx; ///< handle of index file

	// BK-1222
	long fpos_bkt;    ///< basket file pointer

	int  iframe_cnt;  ///< total count
	int  iframe_cur;  ///< current iframe, zero base
	int  bktcnt ;     ///< count of basket
	int  curhdd ;     ///< current hard disk
	int  nexthdd ;    ///< next hard disk

	char target_path[LEN_PATH]; ///< target path

}T_BKTMD_CTRL;


	/**
	  @brief close basket file
	  @param[in] bstop if TRUE, set bkt_status as BKTMD_STATUS_STOP
	  */
	void BKTMD_close(int bstop);

	// BK - play Start ///////////////////////////////////////////////////////////

	/**
	 @ingroup BASKET_MULDEC
	 @brief open basket as given seconds(play start time) for multiple playback
	 @param[in] sec start seconds
	 @param[out] pParam information of read basket for given sec
	 @return BKT_OK if succeed, BKT_ERR if failed
	 @see BKTMD_OpenNext, BKTMD_OpenPrev
	 */
	long BKTMD_Open(long sec, T_BKTMD_OPEN_PARAM* pParam);

	/**
	 @brief just only open basket as given basket ID, and set file pointer as begin
	 @param[in] bid basket ID
	 @return BKT_OK if succeed, BKT_ERR if failed
	 @see BKTMD_OpenPrev
	 */
	long BKTMD_OpenNext(long bid);

	/**
	 @brief just only open basket as given basket ID, and set file pointer as end
	 @param[in] bid basket ID
	 @return BKT_OK if succeed, BKT_ERR if failed
	 @see BKTMD_OpenNext
	 */
	long BKTMD_OpenPrev(long bid);

	// search functions ////////////////////////////////////////////////////////
	/**
	 @brief Searching basket ID, after current seconds
	 @param[in] curbid current basket ID
	 @param[in] cursec current seconds
	 @return basket id if succeed, BKT_ERR if failed
	 @see BKTMD_SearchPrevBID
	 */
	long BKTMD_SearchNextBID(long curbid, long cursec);

	/**
	 @brief Searching basket ID, before current seconds
	 @param[in] curbid current basket ID
	 @param[in] cursec current seconds
	 @return basket ID if succeed, BKT_ERR if failed
	 @see BKTMD_SearchNextBID
	 */
	long BKTMD_SearchPrevBID(long curbid, long cursec);

	long BKTMD_SearchNextHddStartBID(char* nextPath, long sec);	//MULTI-HDD SKSUNG
	long BKTMD_SearchPrevHddStartBID(char* nextPath, long sec);	// BKKIM

	// 2010/12/14 CSNAM
	long BKTMD_GetRecMin(int ch, long t, char* pRecDayTBL);
	#define BKTMD_GetRecHour BKTMD_GetRecMin
	long BKTMD_GetRecDays(int ch, long t, char* pRecMonTBL);
	int  BKTMD_hasRecData(int ch, long sec, int direction);
	int  BKTMD_hasRecDataAll(long sec, int direction);
	long BKTMD_GetFirstRecTime();
	long BKTMD_GetLastRecTime();
	long BKTMD_GetBasketTime(long bid, long *o_begin_sec, long *o_end_sec);

	// read functions
	/**
	 @brief read frame from basket
	 @param[in] dp decode parameter
	 @param[in] decflags enabled channels
	 @return 1 if succeed, -1 if failed
	 @ingroup BASKET_MULDEC
	*/
	long BKTMD_ReadNextFrame(T_BKTMD_PARAM *dp, int decflags[]);
	long BKTMD_ReadNextFirstFrame(T_BKTMD_PARAM *dp, int decflags[]);

	/**
	 @brief read i-frame from basket
	 @param[in] dp decode parameter
	 @param[in] decflags enabled channels
	 @return 1 if succeed, -1 if failed
	 @ingroup BASKET_MULDEC
	*/
	long BKTMD_ReadNextIFrame(T_BKTMD_PARAM *dp, int decflags[]);
	long BKTMD_ReadPrevIFrame(T_BKTMD_PARAM *dp, int decflags[]);

	long BKTMD_SeekBidFpos(long bid,long fpos);  // AYK - 0201
	long BKTMD_setTargetDisk(int hddid, int bid);	//HDD-MULTI SKSUNG
	int  BKTMD_getTargetDisk(long sec);
	int  BKTMD_exgetTargetDisk(char *path) ;
	long BKTMD_GetRelsolutionFromVHDR(int* pCh, int *pWidth, int *pHeight, int *withAudio, int* capMode);

	int  BKTMD_GetNextRecordChannel(long sec, int *recCh);
	int  BKTMD_GetPrevRecordChannel(long sec, int *recCh);

	long BKTMD_SyncAudioIntoVideo(T_BKTMD_SYNC_PARAM* pSyncParam);
	long BKTMD_SyncVideoIntoAudio(T_BKTMD_SYNC_PARAM* pSyncParam);
	long BKTMD_closeAud();
	long BKTMD_ReadNextFrameAud(T_BKTMD_PARAM *dp);
	long BKTMD_SearchNextBIDAud(int ch, long curbid, long sec);
	long BKTMD_SearchPrevBIDAud(int ch, long curbid, long sec);
	long BKTMD_OpenNextAud(int ch, long bid);
	long BKTMD_OpenPrevAud(int ch, long prevbid);

	int  BKTMD_getCapMode(long sec);

	long BKTMD_GetCurPlayTime() ;

	int BKTMD_getRecFileCount(int *pChannels, long t1, long t2) ;
	int BKTMD_getRecFileList(int *pChannels, long t1, long t2, int *pFileArray, char *srcPath, int *pathcount) ;


#ifdef __cplusplus
}
#endif


///////////////////////////////////////////////////////////////////////////////
#endif//_BASKET_MULDEC_H_
///////////////////////////////////////////////////////////////////////////////

