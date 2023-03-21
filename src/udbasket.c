/**
 @file udbasket.c
*/

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <udbasket.h>
#include <basket_dec.h>
#include <basket_rec.h>
#include <basket_muldec.h>	//MULTI-HDD SKSUNG
#include <basket_stm.h>
///////////////////////////////////////
//#define BKT_DEBUG

#ifdef BKT_DEBUG
#define DBG_BKT(msg, args...)  printf("[BKT] - " msg, ##args)
#define DBG1_BKT(msg, args...) printf("[BKT] - %s:" msg, __FUNCTION__, ##args)
#define DBG2_BKT(msg, args...) printf("[BKT] - %s(%d):" msg, __FUNCTION__, __LINE__, ##args)
#define DBG3_BKT(msg, args...) printf("[BKT] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define DBG_BKT(msg, args...)  ((void)0)
#define DBG1_BKT(msg, args...) ((void)0)
#define DBG2_BKT(msg, args...) ((void)0)
#define DBG3_BKT(msg, args...) ((void)0)
#endif
///////////////////////////////////////

extern int  g_bkt_recycle;  // 0:recycle, 1:once
extern BKTREC_CTRL gBKTREC_Ctrl;
extern T_BKTMD_CTRL gBKTMD_CtrlV;	//MULTI-HDD SKSUNG


int  alphasort_reverse(const void *a, const void *b)
{
    return alphasort(b, a);
}

long BKT_GetMaxTS(T_TIMESTAMP *arr_t)
{
	long max_t=0;
	int c=0;

	for(c=0;c<BKT_MAX_CHANNEL;c++)
	{
		max_t = max( (arr_t[c].sec), max_t);
	}

	return max_t;
}

long BKT_GetMinTS(T_TIMESTAMP *arr_t)
{
	long min_t=0x7fffffff;
	int c=0;
	time_t t=0;

	for(c=0;c<BKT_MAX_CHANNEL;c++)
	{
		t = arr_t[c].sec;

		if(t != 0)
		{
			min_t = min( (arr_t[c].sec), min_t);
			//DBG1_BKT("CH:%02d -- %s\n", c+1, ctime(&min_t));
		}
	}

	return min_t;
}

long BKT_transIndexPosToNum(long fpos_index)
{
	return  (fpos_index-sizeof(T_INDEX_HDR))/sizeof(T_INDEX_DATA);
}

long BKT_transIndexNumToPos(int index_number)
{
	return (index_number*sizeof(T_INDEX_DATA)+sizeof(T_INDEX_HDR));
}

int BKT_SetRecycleMode(int recycle)
{
	g_bkt_recycle = recycle;

	DBG_BKT("set recycle mode : %s\n", g_bkt_recycle==BKT_RECYCLE_ON?"ON":"OFF");

	////MULTI-HDD SKSUNG////
#if 0
	if(g_bkt_recycle == BKT_RECYCLE_ON)
	{
		char mpath[30];

		if(strlen(gBKTREC_Ctrl.target_path) == 0)
			sprintf(gBKTREC_Ctrl.target_path, "%s", DEFAULT_DISK_MPOINT1);

		sprintf(mpath, "%s", gBKTREC_Ctrl.target_path);

		return (BKT_ERR != BKTREC_open(mpath));
	}
#endif
	/////////////////////////

	return TRUE;
}

long BKT_closeAll()
{
	BKTDEC_close(TRUE);
	BKTMD_close(TRUE);
	BKTMD_closeAud();
	BKTSTM_close();

#ifdef SEP_AUD_TASK
	BKTDEC_closeAud();
#endif

	BKTREC_exit(SF_STOP, BKT_REC_CLOSE_BUF_FLUSH);

	return 0;

}

void BKT_GetLocalTime(long sec, T_BKT_TM* ptm)
{
	struct tm t1;
	localtime_r(&sec, &t1);

	ptm->year = t1.tm_year+1900;
	ptm->mon  = t1.tm_mon+1;
	ptm->day  = t1.tm_mday;
	ptm->hour = t1.tm_hour;
	ptm->min  = t1.tm_min;
	ptm->sec  = t1.tm_sec;
}


int BKT_isRecordingBasket(long bid)
{
	////MULTI-HDD SKSUNG////
	int status = 0;
	if ((bid == gBKTREC_Ctrl.bid) && strcmp(gBKTREC_Ctrl.target_path, gBKTMD_CtrlV.target_path)==0)
		status = 1;

	return status;
}

int BKT_isRecording(long bid, char* target_path)
{
	////MULTI-HDD SKSUNG////
	int status = 0;
	if ((bid == gBKTREC_Ctrl.bid) && strcmp(gBKTREC_Ctrl.target_path, target_path)==0)
		status = 1;

	return status;
}


long BKT_GetRecordingIndexCount()
{
	//long fpos = LTELL(g_handle_rec_idx);
	long fpos = gBKTREC_Ctrl.fpos_idx;

	return  (fpos-sizeof(T_INDEX_HDR))/sizeof(T_INDEX_DATA);
}



////////////////////////////
// EOF basket.c

