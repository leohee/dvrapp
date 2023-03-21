/**
 @file basket_stm.h
 @defgroup BASKET_STM Streaming
 @ingroup BASKET
 @brief
 */


///////////////////////////////////////////////////////////////////////////////
#ifndef _BASKET_STM_H_
#define _BASKET_STM_H_
///////////////////////////////////////////////////////////////////////////////


#include <udbasket.h>


///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"{
#endif
///////////////////////////////////////////////////////////////////////////////


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

	int iframe_cnt; // total count
	int iframe_cur; // zero base

	char target_path[LEN_PATH];

}BKT_STM_CTRL;

//////////////////////////////////////////////////////////////////////////
// streaming interfaces
long BKTSTM_Open(int ch, long t, int direction);
long BKTSTM_close();
long BKTSTM_OpenNext(int ch, long bid);
long BKTSTM_OpenPrev(int ch, long bid);
void BKTSTM_setTargetDisk(const char *path);
int  BKTSTM_getRecordDays(int year, int mon, char* pRecMonTBL); // 31 day
int  BKTSTM_getRecordChData(int ch, int year, int mon, int day, char* pRecDayTBL); // 1440 minutes (24*60)
int  BKTSTM_getRecordChDataAll(int year, int mon, int day, char* pRecDayTBL); // 1440*BKT_MAX_CHANNEL minutes (24*60*BKT_MAX_CHANNEL)
long BKTSTM_SearchPrevBID(int ch, long bid, long sec);
long BKTSTM_SearchNextBID(int ch, long bid, long sec);

//////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
#endif//_BASKET_STM_H_
///////////////////////////////////////////////////////////////////////////////
