/*
* basket_stm.c
*/

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <basket_stm.h>

//////////////////////////////////////////////////////////////////////////
//#define BKTSTM_DEBUG

#ifdef BKTSTM_DEBUG
#define TRACE0_BKTSTM(msg, args...) printf("[BKTSTM] - " msg, ##args)
#define TRACE1_BKTSTM(msg, args...) printf("[BKTSTM] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE0_BKTSTM(msg, args...) ((void)0)
#define TRACE1_BKTSTM(msg, args...) ((void)0)
#endif
//////////////////////////////////////////////////////////////////////////

#define BKTSTM_STATUS_OPEN 1
#define BKTSTM_STATUS_STOP 0

int gBKTSTM_status=BKTSTM_STATUS_STOP;

BKT_STM_CTRL gBKTSTM_Ctrl;

long BKTSTM_Open(int ch, long sec, int nDirection)
{
	BKTSTM_close();

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	char path[LEN_PATH];
	T_BASKET_RDB rdb;
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	rdb.bid = 0;
	rdb.idx_pos = 0;

	int size_evt = TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL;
	int i;

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTSTM("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	TRACE0_BKTSTM("opened rdb file for remote playback. %s\n", path);

	if(!LSEEK_SET(fd, size_evt))
	{
		TRACE1_BKTSTM("failed seek rdbs.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	// read rdb(bid, pos)
	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
 	{
 		TRACE1_BKTSTM("failed read rdb data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	CLOSE_BKT(fd); // done read rec info.

	if(nDirection == BKT_FORWARD ) // forward
	{
		for(i=tm1.hour*60+tm1.min;i<TOTAL_MINUTES_DAY;i++)
		{
			if(rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0)
			{
				rdb.bid     = rdbs[TOTAL_MINUTES_DAY*ch+i].bid;
				rdb.idx_pos = rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos;

				TRACE0_BKTSTM("found rec time forward, CH:%d, %02d:%02d, BID:%ld, idx_pos:%ld\n", ch, i/60, i%60, rdb.bid, rdb.idx_pos);
				break;
			}
		}
	}
	else // backward
	{
		for(i=tm1.hour*60+tm1.min;i>=0 ;i--)
		{
			if(rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0)
			{
				rdb.bid     = rdbs[TOTAL_MINUTES_DAY*ch+i].bid;
				rdb.idx_pos = rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos;

				TRACE0_BKTSTM("found rec time backward, %02d:%02d, BID:%ld, idx_pos:%ld\n", i/60, i%60, rdb.bid, rdb.idx_pos);

				break;
			}
		}
	}


	if(rdb.bid ==0)
	{
		TRACE1_BKTSTM("failed open basket for streaming -- path:%s, ts:%s\n", path, ctime(&sec));
		return BKT_ERR;
	}


	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.bkt", gBKTSTM_Ctrl.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hbkt, path))
	{
		TRACE1_BKTSTM("failed open basket file for decoding -- path:%s\n", path);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.idx", gBKTSTM_Ctrl.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hidx, path))
	{
		TRACE1_BKTSTM("failed open index file for decoding -- path:%s\n", path);
		CLOSE_BKT(gBKTSTM_Ctrl.hbkt);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTSTM_Ctrl.hidx, ihd))
	{
		TRACE1_BKTSTM("failed read index hdr\n");
		BKTSTM_close();

		return BKT_ERR;
	}
	gBKTSTM_Ctrl.iframe_cnt = BKT_isRecordingBasket(rdb.bid)? BKT_GetRecordingIndexCount():ihd.count;

	if(!LSEEK_SET(gBKTSTM_Ctrl.hidx, rdb.idx_pos))
	{
		TRACE1_BKTSTM("failed seek index.\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	// search being index file point.
	T_INDEX_DATA idd;
	long idx_pos=rdb.idx_pos;
	while(1)
	{
		if(!READ_ST(gBKTSTM_Ctrl.hidx, idd))
		{
			TRACE1_BKTSTM("failed read index data. idx_pos:%ld\n", idx_pos);
			BKTSTM_close();
			return BKT_ERR;
		}

		if( idd.ch == ch )
		{
			idx_pos = LTELL(gBKTSTM_Ctrl.hidx) - sizeof(idd);
			TRACE0_BKTSTM("found index fpos:%ld, saved fpos:%ld\n", idx_pos, rdb.idx_pos);
			break;
		}
	}

	gBKTSTM_Ctrl.iframe_cur = BKT_transIndexPosToNum(idx_pos);

	TRACE0_BKTSTM("read index hdr, curiframe:%d, totiframe:%d\n", gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);

	if(!LSEEK_SET(gBKTSTM_Ctrl.hbkt, idd.fpos))
	{
		TRACE1_BKTSTM("failed seek basket:%ld.\n", idd.fpos);
		BKTSTM_close();
		return BKT_ERR;
	}

	if(!LSEEK_SET(gBKTSTM_Ctrl.hidx, idx_pos))
	{
		TRACE1_BKTSTM("failed seek index:%ld.\n", idx_pos);
		BKTSTM_close();
		return BKT_ERR;
	}

	gBKTSTM_Ctrl.ch = ch;
	gBKTSTM_Ctrl.bid = rdb.bid;
	gBKTSTM_Ctrl.sec = idd.ts.sec;

	gBKTSTM_status = BKTSTM_STATUS_OPEN;

	TRACE0_BKTSTM("open basket file for streaming -- CH:%ld, BID:%ld, B-POS:%ld, I-POS:%ld \n", idd.ch, rdb.bid, idd.fpos, idx_pos);

	return BKT_OK;
}

int BKTSTM_getRecordDays(int year, int mon, char* pRecMonTBL)
{
	int i, bFindOK=0;

	char path[LEN_PATH];

	for(i=0;i<31;i++)
	{
		sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, year, mon, i+1);

		if(-1 != access(path, 0))
		{
			*(pRecMonTBL+i) = 1;

			if(!bFindOK)
				bFindOK=1;
		}
	}

	return bFindOK;
}

int BKTSTM_getRecordChData(int ch, int year, int mon, int day, char* pRecDayTBL)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, year, mon, day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTSTM("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	long offset = TOTAL_MINUTES_DAY*ch;

	if(!LSEEK_SET(fd, offset))
	{
		TRACE1_BKTSTM("failed seek rdb.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, pRecDayTBL, TOTAL_MINUTES_DAY))
	{
		TRACE1_BKTSTM("failed read rdb CH:%d, offset:%ld\n", ch, offset);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	TRACE0_BKTSTM("Succeeded Get record hour info, ch:%d, year:%d, mon:%d, day:%d\n", ch, year, mon, day);
	CLOSE_BKT(fd);

	return 0;

}

int BKTSTM_getRecordChDataAll(int year, int mon, int day, char* pRecDayTBL)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, year, mon, day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTSTM("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, pRecDayTBL, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
	{
		TRACE1_BKTSTM("failed read rdb ch all data.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	TRACE0_BKTSTM("Succeeded Get record hour info all, year:%d, mon:%d, day:%d\n", year, mon, day);
	CLOSE_BKT(fd);

	return 0;
}

void BKTSTM_setTargetDisk(const char *path)
{
	sprintf(gBKTSTM_Ctrl.target_path, "%s", path);
	TRACE0_BKTSTM("set stream target disk. path :%s\n", gBKTSTM_Ctrl.target_path);
}

long BKTSTM_close()
{
	SAFE_CLOSE_BKT(gBKTSTM_Ctrl.hbkt);
	SAFE_CLOSE_BKT(gBKTSTM_Ctrl.hidx);

	gBKTSTM_Ctrl.ch  = 0;
	gBKTSTM_Ctrl.bid = 0; // caution not zero-base

	gBKTSTM_Ctrl.sec = 0;

	gBKTSTM_Ctrl.iframe_cnt = 0;
	gBKTSTM_Ctrl.iframe_cur = 0;

	gBKTSTM_status = BKTSTM_STATUS_STOP;

	TRACE0_BKTSTM("close basket streaming\n");

	return BKT_OK;
}


long BKTSTM_OpenNext(int ch, long nextbid)
{
	long last_read_sec = gBKTSTM_Ctrl.sec;

	BKTSTM_close();

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTSTM_Ctrl.target_path, nextbid);

	// open basket
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hbkt, path))
	{
		TRACE1_BKTSTM("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTSTM_Ctrl.target_path, nextbid);
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hidx, path))
	{
		TRACE1_BKTSTM("failed open index file -- path:%s\n", path);
		CLOSE_BKT(gBKTSTM_Ctrl.hbkt);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTSTM_Ctrl.hidx, ihd))
	{
		TRACE1_BKTSTM("failed read index hdr\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	gBKTSTM_Ctrl.ch  = ch;
	gBKTSTM_Ctrl.bid = nextbid;
	gBKTSTM_Ctrl.iframe_cnt = BKT_isRecordingBasket(nextbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTSTM_Ctrl.sec = last_read_sec;

	if(!LSEEK_SET(gBKTSTM_Ctrl.hbkt, sizeof(T_BASKET_HDR)))
	{
		TRACE1_BKTSTM("failed seek basket.\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	TRACE0_BKTSTM("open next basket. CH:%d, BID:%ld, iframe-cur:%d, iframe_total:%d\n", ch, nextbid, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);

	return BKT_OK;
}

long BKTSTM_OpenPrev(int ch, long prevbid)
{
	long last_read_sec = gBKTSTM_Ctrl.sec;

	BKTSTM_close();

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTSTM_Ctrl.target_path, prevbid);

	// open basket
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hbkt, path))
	{
		TRACE1_BKTSTM("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTSTM_Ctrl.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTSTM_Ctrl.hidx, path))
	{
		TRACE1_BKTSTM("failed open index file -- path:%s\n", path);
		CLOSE_BKT(gBKTSTM_Ctrl.hbkt);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTSTM_Ctrl.hidx, ihd))
	{
		TRACE1_BKTSTM("failed read index hdr\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	if(ihd.count != 0)
	{
		if(!LSEEK_CUR(gBKTSTM_Ctrl.hidx, sizeof(T_INDEX_DATA)*(ihd.count-1)))
		{
			TRACE1_BKTSTM("failed seek idx. ihd.count=%d\n", ihd.count);
			BKTSTM_close(TRUE);
			return BKT_ERR;
		}

		T_INDEX_DATA idd;
		if(!READ_ST(gBKTSTM_Ctrl.hidx, idd))
		{
			TRACE1_BKTSTM("failed read index data\n");
			BKTSTM_close(TRUE);
			return BKT_ERR;
		}

		if(!LSEEK_SET(gBKTSTM_Ctrl.hbkt, idd.fpos))
		{
			TRACE1_BKTSTM("failed seek basket.\n");
			BKTSTM_close(TRUE);
			return BKT_ERR;
		}
	}
	else
	{
		long prev_sec = 0;
		long prev_usec = 0;
		int index_count=0, bkt_fpos=0;
		T_INDEX_DATA idd;
		while(READ_ST(gBKTSTM_Ctrl.hidx, idd))
		{
			if( (idd.ts.sec <= prev_sec && idd.ts.usec <= prev_usec)
				|| idd.id != ID_INDEX_DATA
				|| idd.fpos <= bkt_fpos)
			{
				TRACE1_BKTSTM("Fount last record index pointer. idd.id=0x%08x, sec=%d, usec=%d, count=%d\n", idd.ts.sec, idd.ts.usec, index_count);
				break;
			}

			prev_sec  = idd.ts.sec;
			prev_usec = idd.ts.usec;
			bkt_fpos = idd.fpos;
			index_count++;
		}

		if(index_count != 0)
		{
			//////////////////////////////////////////////////////////////////////////
			if(!LSEEK_SET(gBKTSTM_Ctrl.hbkt, bkt_fpos))
			{
				TRACE1_BKTSTM("failed seek basket.\n");
				BKTSTM_close(TRUE);
				return BKT_ERR;
			}

			if(!LSEEK_SET(gBKTSTM_Ctrl.hidx, sizeof(idd)*index_count))
			{
				TRACE1_BKTSTM("failed seek index.\n");
				BKTSTM_close(TRUE);
				return BKT_ERR;
			}
		}
		else
		{
			if(!LSEEK_END(gBKTSTM_Ctrl.hbkt, 0))
			{
				TRACE1_BKTSTM("failed seek basket.\n");
				BKTSTM_close(TRUE);
				return BKT_ERR;
			}

			int offset = sizeof(T_INDEX_DATA);
			if(!LSEEK_END(gBKTSTM_Ctrl.hidx, -offset))
			{
				TRACE1_BKTSTM("failed seek idx.\n");
				BKTSTM_close(TRUE);
				return BKT_ERR;
			}
		}
	}

	gBKTSTM_Ctrl.ch  = ch;
	gBKTSTM_Ctrl.bid = prevbid;
	gBKTSTM_Ctrl.iframe_cnt = BKT_isRecordingBasket(prevbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTSTM_Ctrl.sec = last_read_sec;
	gBKTSTM_Ctrl.iframe_cur = gBKTSTM_Ctrl.iframe_cnt - 1;

	if(!LSEEK_END(gBKTSTM_Ctrl.hbkt, 0))
	{
		TRACE1_BKTSTM("failed seek basket.\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	int offset = sizeof(T_INDEX_DATA);
	if(!LSEEK_END(gBKTSTM_Ctrl.hidx, -offset))
	{
		TRACE1_BKTSTM("failed seek idx.\n");
		BKTSTM_close();
		return BKT_ERR;
	}

	TRACE0_BKTSTM("open prev basket. CH:%d, BID:%ld, iframe-current:%d, iframe_total:%d\n", ch, prevbid, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);

	return BKT_OK;
}

long BKTSTM_ReadNextIFrame(T_BKTSTM_PARAM *dp)
{
	if(BKT_isRecordingBasket(gBKTSTM_Ctrl.bid))
	{
		gBKTSTM_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
		//TRACE0_BKTSTM("get total i-frame count:%d\n", gBKTSTM_Ctrl.gBKTSTM_Ctrl);
	}

	if(gBKTSTM_Ctrl.iframe_cur + 1 >= gBKTSTM_Ctrl.iframe_cnt) // find next basket
	{
		long bid = BKTSTM_SearchNextBID(gBKTSTM_Ctrl.ch, gBKTSTM_Ctrl.bid, gBKTSTM_Ctrl.sec);

		if( bid == BKT_ERR )
			return BKT_ERR;

		if( BKT_ERR == BKTSTM_OpenNext(gBKTSTM_Ctrl.ch, bid))
			return BKT_ERR;
	}
	else
	{
		gBKTSTM_Ctrl.iframe_cur += 1;
	}

	//TRACE0_BKTSTM("seek idx next iframe:%d, fpos:%d\n", gBKTSTM_Ctrl.iframe_cur, BKT_transIndexNumToPos(gBKTSTM_Ctrl.iframe_cur));

	if(!LSEEK_SET(gBKTSTM_Ctrl.hidx, BKT_transIndexNumToPos(gBKTSTM_Ctrl.iframe_cur)))
	{
		TRACE1_BKTSTM("failed seek read next frame. iframe:%d\n", gBKTSTM_Ctrl.iframe_cur);
		return BKT_ERR;
	}

	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;

	int search_next_iframe=FALSE;
	int ret;

	while(gBKTSTM_status == BKTSTM_STATUS_OPEN)
	{
		if(!READ_ST(gBKTSTM_Ctrl.hidx, idd))
		{
			TRACE1_BKTSTM("failed read idx data\n");
			break;
		}

		if(idd.ch == dp->ch && idd.ts.sec - gBKTSTM_Ctrl.sec > 0)
		{
			if(!LSEEK_SET(gBKTSTM_Ctrl.hbkt, idd.fpos))
			{
				TRACE1_BKTSTM("failed seek next index data. i-fpos:%ld\n", idd.fpos);
				return BKT_ERR;
			}

			ret = READ_STRC(gBKTSTM_Ctrl.hbkt, shd);
#ifdef BKT_SYSIO_CALL
			if(ret < 1)
			{
				if(ret == 0)
				{
					TRACE1_BKTSTM("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
				}
				else
				{
					search_next_iframe = TRUE;
					TRACE1_BKTSTM("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
				}
				break;
			}
#else
			if(ret != sizeof(shd))
			{
				if(0 != feof(gBKTSTM_Ctrl.hbkt))
				{
					TRACE1_BKTSTM("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTSTM_Ctrl.iframe_cnt, gBKTSTM_Ctrl.iframe_cnt);
				}
				else
				{
					search_next_iframe = TRUE;
					TRACE1_BKTSTM("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTSTM_Ctrl.iframe_cnt, gBKTSTM_Ctrl.iframe_cnt);
				}

				break;
			}
#endif

			if(shd.id != ID_VIDEO_HEADER) // Must be Video Frame. because searching iframe by index.
			{
				TRACE1_BKTSTM("failed read next stream header.CH:%ld, SHID:0x%08lx\n", shd.ch, shd.id);
				search_next_iframe = TRUE;
				break;
				//return BKT_ERR;
			}

			if(!READ_ST(gBKTSTM_Ctrl.hbkt, vhd))
			{
				TRACE1_BKTSTM("failed read next video header.\n");
				return BKT_ERR;
			}

			if(!READ_PTSZ(gBKTSTM_Ctrl.hbkt, dp->vbuffer, shd.framesize))
			{
				TRACE1_BKTSTM("failed read next video i-frame data.\n");
				return BKT_ERR;
			}

			dp->framesize   = shd.framesize;
			dp->frametype   = shd.frametype;
			dp->ts.sec      = shd.ts.sec;
			dp->ts.usec     = shd.ts.usec;
			dp->framerate   = vhd.framerate;
			dp->frameWidth  = vhd.width;
			dp->frameHeight = vhd.height;
			dp->fpos        = 1;
			dp->streamtype  = ST_VIDEO;
			dp->event       = vhd.event;
//			vp->withAudio   = vhd.withAudio; // BK 100119
            dp->audioONOFF  = vhd.audioONOFF; // AYK - 0201
			dp->capMode     = vhd.capMode;

			if(strlen(vhd.camName))
				strncpy(dp->camName, vhd.camName, 16);


			gBKTSTM_Ctrl.sec = shd.ts.sec;

			//TRACE0_BKTSTM("ReadNextIFrame, read frame size:%ld, camName:%s\n", shd.framesize, vp->camName);

			return BKT_OK;
		}
		else
		{
			gBKTSTM_Ctrl.iframe_cur +=1;

			if(gBKTSTM_Ctrl.iframe_cur >= gBKTSTM_Ctrl.iframe_cnt) // find next basket
			{
				long bid = BKTSTM_SearchNextBID(gBKTSTM_Ctrl.ch, gBKTSTM_Ctrl.bid, gBKTSTM_Ctrl.sec);

				if( bid == BKT_ERR )
					return BKT_ERR;

				if( BKT_ERR == BKTSTM_OpenNext(gBKTSTM_Ctrl.ch, bid))
					return BKT_ERR;
			}
			else if(!LSEEK_SET(gBKTSTM_Ctrl.hidx, BKT_transIndexNumToPos(gBKTSTM_Ctrl.iframe_cur)))
			{
				TRACE1_BKTSTM("failed seek read prev frame. iframe-cur:%d\n", gBKTSTM_Ctrl.iframe_cur);
				return BKT_ERR;
			}
		}

	}// end while

	if(search_next_iframe == TRUE)
	{
		return BKTSTM_ReadNextIFrame(dp);
	}

	//TRACE0_BKTSTM("cur-iframe:%d, tot-iframe:%d\n", gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
	long bid = BKTSTM_SearchNextBID(gBKTSTM_Ctrl.ch, gBKTSTM_Ctrl.bid, gBKTSTM_Ctrl.sec);

	if( bid == BKT_ERR )
		return BKT_ERR;

	if( BKT_ERR == BKTSTM_OpenNext(gBKTSTM_Ctrl.ch, bid))
		return BKT_ERR;

	return BKTSTM_ReadNextIFrame(dp);
}

// normal read with audio
long BKTSTM_ReadNextFrame(T_BKTSTM_PARAM *dp)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	int search_next_iframe=0;
	int ret;

	// read shd
	while(gBKTSTM_status == BKTSTM_STATUS_OPEN)
	{
		//TRACE0_BKTSTM("read stream header\n");

		ret = READ_STRC(gBKTSTM_Ctrl.hbkt, shd);

		//TRACE0_BKTSTM("read stream header done. ID:0x%08lx, CH:%ld, SIZE:%ld, TYPE:%ld, SEC:%ld\n", shd.id, shd.ch, shd.framesize, shd.frametype, shd.ts.sec);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				TRACE1_BKTSTM("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTSTM("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
			}
			break;
		}
#else
		if(ret != sizeof(shd))
		{
			if(0 != feof(gBKTSTM_Ctrl.hbkt))
			{
				TRACE1_BKTSTM("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTSTM_Ctrl.iframe_cnt, gBKTSTM_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTSTM("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
			}
			break;
		}
#endif
		if(shd.id != ID_VIDEO_HEADER && shd.id != ID_AUDIO_HEADER)
		{
			search_next_iframe = TRUE;
			TRACE0_BKTSTM("maybe appeared broken stream .Search next i-frame. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			break;
		}
		else if(shd.id == ID_VIDEO_HEADER)
		{
			if(shd.ts.sec - gBKTSTM_Ctrl.sec > 4 || shd.ts.sec - gBKTSTM_Ctrl.sec < 0) // maybe max interval frame. for broken stream or motion.
			{
				search_next_iframe = TRUE;
				TRACE0_BKTSTM("time interval is too long. So search next frame. shd.id:0x%08lx\n", shd.id);
				break;
			}

			if(shd.ch == dp->ch)
			{
				if(!READ_ST(gBKTSTM_Ctrl.hbkt, vhd))
				{
					TRACE1_BKTSTM("failed read video header\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTSTM_Ctrl.hbkt, dp->vbuffer, shd.framesize))
				{
					TRACE0_BKTSTM("failed read normal video frame.\n");
					return BKT_ERR;
				}

				dp->framesize   = shd.framesize;
				dp->frametype   = shd.frametype;
				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->framerate   = vhd.framerate;
				dp->frameWidth  = vhd.width;
				dp->frameHeight = vhd.height;
				dp->fpos = (shd.frametype == FT_IFRAME)?1:0;
				dp->streamtype  = ST_VIDEO;
				dp->event       = vhd.event;
                dp->audioONOFF  = vhd.audioONOFF; // AYK - 0201
				dp->capMode     = vhd.capMode;

				if(strlen(vhd.camName))
					strncpy(dp->camName, vhd.camName, 16);

				gBKTSTM_Ctrl.sec = shd.ts.sec;

				//TRACE0_BKTSTM("ReadNextFrame Normal-- read frame size:%ld, camName:%s\n", shd.framesize, vp->camName);

				if(shd.frametype == FT_IFRAME )
				{
					gBKTSTM_Ctrl.iframe_cur += 1;

					if( BKT_isRecordingBasket(gBKTSTM_Ctrl.bid))
					{
						gBKTSTM_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
					}

					if(gBKTSTM_Ctrl.iframe_cur >= gBKTSTM_Ctrl.iframe_cnt)
					{
						TRACE0_BKTSTM("End of basket file, count:%d\n", gBKTSTM_Ctrl.iframe_cnt);
						break;
					}

					//TRACE0_BKTSTM("read iframe normal mode. CH:%d, num:%d, total:%d\n", shd.ch, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
				}

				return BKT_OK;
			}
			else
			{
				// skip fpos to next frame
				if(!LSEEK_CUR(gBKTSTM_Ctrl.hbkt, sizeof(vhd) + shd.framesize))
				{
					TRACE1_BKTSTM("failed seek cur basket.\n");
					return BKT_ERR;
				}


				if(shd.frametype == FT_IFRAME)
				{
					gBKTSTM_Ctrl.iframe_cur += 1;

					if(BKT_isRecordingBasket(gBKTSTM_Ctrl.bid))
					{
						gBKTSTM_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
					}

					if(gBKTSTM_Ctrl.iframe_cur >= gBKTSTM_Ctrl.iframe_cnt)
					{
						TRACE0_BKTSTM("End of basket file, cur:%d, count:%d\n", gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
						break;
					}

					//TRACE0_BKTSTM("read iframe normal mode. CH:%d, num:%d, total:%d\n", shd.ch, gBKTSTM_Ctrl.iframe_cur, gBKTSTM_Ctrl.iframe_cnt);
				}
			}
		}
		else if( shd.id == ID_AUDIO_HEADER )
		{
			if(shd.ch == dp->ch)
			{
				// read audio stream header
				if(!READ_ST(gBKTSTM_Ctrl.hbkt, ahd))
				{
					TRACE1_BKTSTM("failed read audio stream header\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTSTM_Ctrl.hbkt, dp->abuffer, shd.framesize))
				{
					TRACE1_BKTSTM("failed read audio stream\n");
					return BKT_ERR;
				}

				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->streamtype  = ST_AUDIO;
				dp->framesize   = shd.framesize;
				dp->samplingrate= ahd.samplingrate;
				dp->fpos = 0;

				return BKT_OK;
			}
			else
			{
				// skip
				if(!LSEEK_CUR(gBKTSTM_Ctrl.hbkt, sizeof(ahd) + shd.framesize))
				{
					TRACE1_BKTSTM("failed seek cur basket.\n");
					return BKT_ERR;
				}

			}
		}

	}// while

	if(search_next_iframe == TRUE)
	{
		return BKTSTM_ReadNextIFrame(dp);
	}
	else //if(gBKTSTM_Ctrl.iframe_cur >= gBKTSTM_Ctrl.iframe_cnt)
	{
		long bid = BKTSTM_SearchNextBID(gBKTSTM_Ctrl.ch, gBKTSTM_Ctrl.bid, gBKTSTM_Ctrl.sec);

		if( bid == BKT_ERR )
			return BKT_ERR;

		if( BKT_ERR == BKTSTM_OpenNext(gBKTSTM_Ctrl.ch, bid))
			return BKT_ERR;

		return BKTSTM_ReadNextIFrame(dp);
	}

	return BKT_ERR;
}

long BKTSTM_SearchPrevBID(int ch, long curbid, long sec)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTSTM("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
	{
		TRACE1_BKTSTM("failed seek rdb\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		TRACE1_BKTSTM("failed read rdbs data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	int i;
	for(i=tm1.hour*60+tm1.min-1;i>=0;i--)
	{
		if(rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+i].bid !=curbid)
		{
			TRACE0_BKTSTM("found prev BID:%ld, CH:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+i].bid, ch);
			CLOSE_BKT(fd);

			return rdbs[TOTAL_MINUTES_DAY*ch+i].bid; // found!!
		}
	}

	CLOSE_BKT(fd);

	return BKT_ERR;// not found
}

long BKTSTM_SearchNextBID(int ch, long curbid, long sec)
{
	if(BKT_isRecordingBasket(curbid))
	{
		TRACE0_BKTSTM("There is no a next basekt. BID:%ld is recording.\n", curbid);
		return BKT_ERR;
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTSTM_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTSTM("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
	{
		TRACE1_BKTSTM("failed seek rdb\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		TRACE1_BKTSTM("failed read rdbs data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	int i;
	for(i=tm1.hour*60+tm1.min+1;i<TOTAL_MINUTES_DAY;i++)
	{
		if(rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+i].bid !=curbid)
		{
			TRACE0_BKTSTM("found next bid %ld, CH:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+i].bid, ch);
			CLOSE_BKT(fd);

			return rdbs[TOTAL_MINUTES_DAY*ch+i].bid; // found!!
		}
	}

	CLOSE_BKT(fd);

	return BKT_ERR;// not found
}

////////////////////////////
// EOF basket_stm.c

