/**
 @file basket_dec.h
 @defgroup BASKET_DEC Single Decoding
 @ingroup BASKET
 */

///////////////////////////////////////////////////////////////////////////////
#ifndef _BASKET_DEC_H_
#define _BASKET_DEC_H_
///////////////////////////////////////////////////////////////////////////////

#include <udbasket.h>

///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"{
#endif
///////////////////////////////////////////////////////////////////////////////


enum _enumBasketDecOpenFlag{BKTDEC_PF_PREV, BKTDEC_PF_NORMAL, BKTDEC_PF_NEXT};

typedef struct
{
	int  ch;
	long bid;

	long sec;

#ifdef BKT_SYSIO_CALL
	int hbkt;
	int hidx;
#else
	FILE* hbkt;
	FILE* hidx;
#endif

	// BK-1222
	long fpos_bkt;

	int iframe_cnt; // total count
	int iframe_cur; // zero base

	char target_path[LEN_PATH];

}BKTDEC_CTRL;


typedef struct {
	int recChannelCount;
	int recChannels[BKT_MAX_CHANNEL];
	int frameWidth;
	int frameHeight;
}T_BKTDEC_OPEN_PARAM;

long BKTDEC_close(int bStop);
long BKTDEC_GetFirstRecTime(int channel);
long BKTDEC_GetLastRecTime(int channel);

long BKTDEC_GetBasketTime(int ch, long bid, long *o_begin_sec, long *o_end_sec);
int  BKTDEC_getRecFileCount(int *pChannels, long t1, long t2);
int  BKTDEC_getRecFileList(int *pChannels, long t1, long t2, int *BackupFileList, char *srcPath);

int  BKTDEC_isRecData(int ch, long ts, int direction);
long BKTDEC_GetRecHour(int ch, long t, char* pRecDayTBL);
long BKTDEC_GetRecDays(int ch, long t, char* pRecMonTBL);
long BKTDEC_ReadNextFrame(T_BKTDEC_PARAM *dp);
long BKTDEC_ReadNextIFrame(T_BKTDEC_PARAM *dp);
long BKTDEC_ReadPrevIFrame(T_BKTDEC_PARAM *dp);

long BKTDEC_Open(int ch, long sec, int direction);
long BKTDEC_OpenNext(int ch, long curbid);
long BKTDEC_OpenPrev(int ch, long curbid);
long BKTDEC_SearchNextBID(int ch, long bid, long t);
long BKTDEC_SearchPrevBID(int ch, long bid, long t);
void BKTDEC_setTargetDisk(const char *path);

long BKTDEC_GetRelsolutionFromVHDR(int* pCh, int *pWidth, int *pHeight);

#ifdef SEP_AUD_TASK
long BKTDEC_OpenAud(int ch, long sec, int direction);
long BKTDEC_OpenNextAud(int ch, long bid);
long BKTDEC_closeAud();
long BKTDEC_ReadNextFrameAud(T_BKTDEC_PARAM *dp);
long BKTDEC_SyncAudioWithVideo(T_BKTDEC_PARAM *dp);
#endif

long BKTDEC_SeekBidFpos(long bid,long fpos);  // AYK - 0201

int  BKTDEC_GetNextRecordChannel(long sec, int *recCh);
int  BKTDEC_GetPrevRecordChannel(long sec, int *recCh);

long BKTDEC_GetCurPlayTime() ;

inline long BKTDEC_GetIndexCount();


///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
#endif//_BASKET_DEC_H_
///////////////////////////////////////////////////////////////////////////////

