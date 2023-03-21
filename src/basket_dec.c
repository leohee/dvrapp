/*
* basket_dec.c
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

#include <basket_dec.h>

#define BKTDEC_STATUS_STOP (0)
#define BKTDEC_STATUS_OPEN (1)

//////////////////////////////////////////////////////////////////////////
//#define BKTDEC_DEBUG

#ifdef BKTDEC_DEBUG
#define TRACE0_BKTDEC(msg, args...) printf("[BKTDEC] - " msg, ##args)
#define TRACE1_BKTDEC(msg, args...) printf("[BKTDEC] - %s(%d):" msg, __FUNCTION__, __LINE__, ##args)
#define TRACE2_BKTDEC(msg, args...) printf("[BKTDEC] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE0_BKTDEC(msg, args...) ((void)0)
#define TRACE1_BKTDEC(msg, args...) ((void)0)
#define TRACE2_BKTDEC(msg, args...) ((void)0)
#endif
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
#ifdef SEP_AUD_TASK
BKTDEC_CTRL gBKTDEC_CtrlAud;
#endif
//////////////////////////////////////////////////////////////////////////

int gBKTDEC_status=BKTDEC_STATUS_STOP;
BKTDEC_CTRL gBKTDEC_Ctrl;

long BKTDEC_GetBasketTime(int ch, long bid, long *o_begin_sec, long *o_end_sec)
{
	char path[255];
	T_BASKET_HDR bhdr;

#ifdef BKT_SYSIO_CALL
	int fd=0;
#else
	FILE *fd;
#endif

	sprintf(path, "%s/%08ld.bkt", gBKTDEC_Ctrl.target_path, bid);

	// open basket
	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	if(!READ_ST(fd, bhdr))
	{
		TRACE1_BKTDEC("failed read basket header.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	*o_begin_sec = bhdr.ts.t1[ch].sec;
	*o_end_sec   = bhdr.ts.t2[ch].sec;

	TRACE0_BKTDEC("Get Basket Time. CH:%d, BID:%ld, B_SEC:%ld, E_SEC:%ld, Duration:%ld\n", ch, bid, bhdr.ts.t1[ch].sec,  bhdr.ts.t2[ch].sec, bhdr.ts.t2[ch].sec - bhdr.ts.t1[ch].sec);

	return BKT_OK;
}

long BKTDEC_SearchNextBID(int ch, long curbid, long sec)
{
	if(BKT_isRecordingBasket(curbid))
	{
		TRACE1_BKTDEC("There is no a next basekt. BID:%ld is recording.\n", curbid);
		return BKT_ERR;
	}

	long sec_seed = sec;
	if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTDEC_GetBasketTime(ch, curbid, &bts, &ets) || ets == 0)
		{
			TRACE1_BKTDEC("There is no a next basekt. CH:%d, BID:%ld is recording.\n", ch, curbid);
			return BKT_ERR;
		}

		sec_seed = ets;
		//TRACE0_BKTDEC("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[255];
	sprintf(buf, "%s/rdb/", gBKTDEC_Ctrl.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort);

	if(nitems < 0) {
	    perror("scandir search next bid");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TRACE0_BKTDEC("There is no rdb directory %s.\n", buf);
		free(ents);
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int next_rdb=0;

    BKT_FHANDLE fd;

    for(i=0;i<nitems;i++)
    {
		if (ents[i]->d_name[0] == '.') // except [.] and [..]
		{
			if (!ents[i]->d_name[1] || (ents[i]->d_name[1] == '.' && !ents[i]->d_name[2]))
			{
				free(ents[i]);
				continue;
			}
		}

		next_rdb = atoi(ents[i]->d_name);

		if(next_rdb >= curr_rdb)
		{
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTDEC_Ctrl.target_path, ents[i]->d_name);

			if(-1 == access(buf, 0)) {
				free(ents[i]);
				continue;
			}

			if(!OPEN_RDONLY(fd, buf))
			{
				TRACE1_BKTDEC("failed open rdb file. %s\n", buf);
				goto lbl_err_search_next_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				TRACE1_BKTDEC("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				TRACE1_BKTDEC("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}
			CLOSE_BKT(fd);

			int m;
			for(m=tm1.hour*60+tm1.min+1;m<TOTAL_MINUTES_DAY;m++)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTDEC("found next BID:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid);

					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}


			tm1.hour = 0;
			tm1.min  = -1;

		} // if(next_rdb >= curr_rdb)

		free(ents[i]);

    } // end for

lbl_err_search_next_bid:
    for(;i<nitems;i++){
	    free(ents[i]);
	}
	free(ents);

	TRACE0_BKTDEC("Can't find next basket. CUR-BID:%ld, CH:%d, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n", curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found
}


long BKTDEC_SearchPrevBID(int ch, long curbid, long sec)
{
	long sec_seed = sec;
	if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTDEC_GetBasketTime(ch, curbid, &bts, &ets) || bts == 0)
		{
			TRACE1_BKTDEC("There is no a previous basekt. CH:%d, BID:%ld.\n", ch, curbid);
			return BKT_ERR;
		}

		sec_seed = bts;
		//TRACE0_BKTDEC("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[255];
	sprintf(buf, "%s/rdb/", gBKTDEC_Ctrl.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort_reverse);

	if(nitems < 0) {
	    perror("scandir search(reverse) previous bid");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TRACE0_BKTDEC("There is no rdb directory %s.\n", buf);
		free(ents);
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int prev_rdb=0;

    BKT_FHANDLE fd;

    for(i=0;i<nitems;i++)
    {
		if (ents[i]->d_name[0] == '.') // except [.] and [..]
		{
			if (!ents[i]->d_name[1] || (ents[i]->d_name[1] == '.' && !ents[i]->d_name[2]))
			{
				free(ents[i]);
				continue;
			}
		}

		prev_rdb = atoi(ents[i]->d_name);

		if(prev_rdb <= curr_rdb)
		{
			sprintf(buf, "%s/rdb/%s", gBKTDEC_Ctrl.target_path, ents[i]->d_name);

			if(-1 == access(buf, 0)) {
				free(ents[i]);
				continue;
			}

			if(!OPEN_RDONLY(fd, buf))
			{
				TRACE1_BKTDEC("failed open rdb file. %s\n", buf);
				goto lbl_err_search_prev_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				TRACE1_BKTDEC("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}

			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				TRACE1_BKTDEC("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}
			CLOSE_BKT(fd);

			int m;
			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTDEC("found prev BID:%ld, IDXPOS:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, rdbs[TOTAL_MINUTES_DAY*ch+m].idx_pos);

					for(;i<nitems;i++){
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}

			tm1.hour = 23;
			tm1.min  = 60;

		}// if(prev_rdb <= curr_rdb)

		free(ents[i]);

    }

lbl_err_search_prev_bid:
    for(;i<nitems;i++){
	    free(ents[i]);
	}
	free(ents);

	TRACE0_BKTDEC("Can't find prev basket. CUR-BID:%ld, CH:%d, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n", curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found
}

long BKTDEC_OpenNext(int ch, long nextbid)
{
	long last_read_sec = gBKTDEC_Ctrl.sec;

	BKTDEC_close(FALSE);

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_Ctrl.target_path, nextbid);

	// open basket
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file -- path:%s\n", path);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTDEC_Ctrl.target_path, nextbid);
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file -- path:%s\n", path);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	// read index hdr
	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_Ctrl.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	TRACE0_BKTDEC("Read index hdr. CH:%d, BID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ch, ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTDEC_Ctrl.hbkt, bhdr))
	//if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, sizeof(T_BASKET_HDR)))
	{
		TRACE1_BKTDEC("failed read basket header .\n");
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	gBKTDEC_Ctrl.ch  = ch;
	gBKTDEC_Ctrl.bid = nextbid;
	gBKTDEC_Ctrl.iframe_cnt = BKT_isRecordingBasket(nextbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTDEC_Ctrl.sec = last_read_sec;

	// BK - 1223 - Start
	gBKTDEC_Ctrl.iframe_cur = 0;
	gBKTDEC_Ctrl.fpos_bkt = sizeof(T_BASKET_HDR);

	//////////////////////////////////////////////////////////////////////////
	// make because audio thread
	gBKTDEC_status = BKTDEC_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////
	// BK - 1223 - End

	TRACE0_BKTDEC("open next basket. BID:%ld, iframe-current:%d, iframe_total:%d, last_sec:%ld\n",
		nextbid, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt, gBKTDEC_Ctrl.sec);

	return BKT_OK;
}


long BKTDEC_OpenPrev(int ch, long prevbid)
{
	BKTDEC_close(FALSE);

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_Ctrl.target_path, prevbid);

	// open basket
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file -- path:%s\n", path);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTDEC_Ctrl.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file -- path:%s\n", path);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_Ctrl.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}
	TRACE0_BKTDEC("Read index hdr. BID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	if(ihd.count != 0)
	{
		if(!LSEEK_CUR(gBKTDEC_Ctrl.hidx, sizeof(T_INDEX_DATA)*(ihd.count-1)))
		{
			TRACE1_BKTDEC("failed seek idx. ihd.count=%d\n", ihd.count);
			BKTDEC_close(TRUE);
			return BKT_ERR;
		}

		T_INDEX_DATA idd;
		if(!READ_ST(gBKTDEC_Ctrl.hidx, idd))
		{
			TRACE1_BKTDEC("failed read index data\n");
			BKTDEC_close(TRUE);
			return BKT_ERR;
		}

		if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, idd.fpos))
		{
			TRACE1_BKTDEC("failed seek basket.\n");
			BKTDEC_close(TRUE);
			return BKT_ERR;
		}

	}
	else
	{
		long prev_sec = 0;
		long prev_usec = 0;
		int index_count=0, bkt_fpos=0;
		T_INDEX_DATA idd;
		while(READ_ST(gBKTDEC_Ctrl.hidx, idd))
		{
			if( idd.id != ID_INDEX_DATA
				|| (idd.ts.sec <= prev_sec && idd.ts.usec <= prev_usec)
				|| idd.fpos <= bkt_fpos)
			{
				TRACE1_BKTDEC("Fount last record index pointer. idd.id=0x%08x, sec=%d, usec=%d, count=%d\n", idd.ts.sec, idd.ts.usec, index_count);
				break;
			}

			prev_sec  = idd.ts.sec;
			prev_usec = idd.ts.usec;
			bkt_fpos  = idd.fpos;
			index_count++;
		}

		if(index_count != 0)
		{
			if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, bkt_fpos))
			{
				TRACE1_BKTDEC("failed seek basket.\n");
				BKTDEC_close(TRUE);
				return BKT_ERR;
			}

			if(!LSEEK_CUR(gBKTDEC_Ctrl.hidx, sizeof(T_INDEX_DATA)*ihd.count))
			{
				TRACE1_BKTDEC("failed seek idx. ihd.count=%d\n", ihd.count);
				BKTDEC_close(TRUE);
				return BKT_ERR;
			}
		}
		else
		{
			if(!LSEEK_END(gBKTDEC_Ctrl.hbkt, 0))
			{
				TRACE1_BKTDEC("failed seek basket.\n");
				BKTDEC_close(TRUE);
				return BKT_ERR;
			}

			int offset = sizeof(T_INDEX_DATA);
			if(!LSEEK_END(gBKTDEC_Ctrl.hidx, -offset))
			{
				TRACE1_BKTDEC("failed seek idx.\n");
				BKTDEC_close(TRUE);
				return BKT_ERR;
			}
		}
	}

	gBKTDEC_Ctrl.ch  = ch;
	gBKTDEC_Ctrl.bid = prevbid;
	gBKTDEC_Ctrl.iframe_cnt = BKT_isRecordingBasket(prevbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTDEC_Ctrl.sec = ihd.ts.t2[ch].sec;
	gBKTDEC_Ctrl.iframe_cur = gBKTDEC_Ctrl.iframe_cnt - 1;

	// BK - 1223
	gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

	TRACE0_BKTDEC("open prev basket. BID:%ld, iframe-current:%d, iframe_total:%d\n", prevbid, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);

	return BKT_OK;
}

long BKTDEC_Open(int ch, long sec, int nDirection)
{
	BKTDEC_close(TRUE);

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

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path);
		return BKT_ERR;
	}
	TRACE0_BKTDEC("opened rdb file. %s\n", path);

	if(!LSEEK_SET(fd, size_evt))
	{
		TRACE1_BKTDEC("failed seek rdbs\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	// read rdb(bid, pos)
	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
 	{
 		TRACE1_BKTDEC("failed read rdb data\n");
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

				TRACE0_BKTDEC("found rec time forward, CH:%d, %02d:%02d, BID:%ld, idx_pos:%ld\n", ch, i/60, i%60, rdb.bid, rdb.idx_pos);
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

				TRACE0_BKTDEC("found rec time backward, CH:%d, %02d:%02d, BID:%ld, idx_pos:%ld\n", ch, i/60, i%60, rdb.bid, rdb.idx_pos);

				break;
			}
		}
	}

	if(rdb.bid ==0)
	{
		TRACE1_BKTDEC("failed open basket for decoding -- path:%s, ts:%s\n", path, ctime(&sec));
		return BKT_ERR;
	}


	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_Ctrl.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file for vdio decoding -- path:%s\n", path);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.idx", gBKTDEC_Ctrl.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTDEC_Ctrl.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file for decoding -- path:%s\n", path);
		CLOSE_BKT(gBKTDEC_Ctrl.hbkt);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_Ctrl.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_close(TRUE);

		return BKT_ERR;
	}
	gBKTDEC_Ctrl.iframe_cnt = BKT_isRecordingBasket(rdb.bid)? BKT_GetRecordingIndexCount():ihd.count;

	if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, rdb.idx_pos))
	{
		TRACE1_BKTDEC("failed seek index:%ld.\n", rdb.idx_pos);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	// search being index file point.
	T_INDEX_DATA idd;
	long idx_pos=rdb.idx_pos;
	while(1)
	{
		if(!READ_ST(gBKTDEC_Ctrl.hidx, idd))
		{
			TRACE1_BKTDEC("failed read index data. idx_pos:%ld\n", idx_pos);
			BKTDEC_close(TRUE);
			return BKT_ERR;
		}

		if( idd.ch == ch )
		{
			idx_pos = LTELL(gBKTDEC_Ctrl.hidx) - sizeof(idd);
			TRACE0_BKTDEC("found index fpos:%ld, saved fpos:%ld\n", idx_pos, rdb.idx_pos);
			break;
		}
	}

	gBKTDEC_Ctrl.iframe_cur = BKT_transIndexPosToNum(idx_pos);

	TRACE0_BKTDEC("read index hdr, curiframe:%d, totiframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);

	if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, idd.fpos))
	{
		TRACE1_BKTDEC("failed seek basket:%ld.\n", idd.fpos);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, idx_pos))
	{
		TRACE1_BKTDEC("failed seek index:%ld.\n", idx_pos);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	gBKTDEC_Ctrl.ch  = ch;
	gBKTDEC_Ctrl.bid = rdb.bid;
	gBKTDEC_Ctrl.sec = idd.ts.sec;

	// BK-1222
	gBKTDEC_Ctrl.fpos_bkt =idd.fpos;

	//////////////////////////////////////////////////////////////////////////
	// make because audio thread
	gBKTDEC_status = BKTDEC_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////

	TRACE0_BKTDEC("open basket file for video decoding -- CH:%ld, BID:%ld, B-POS:%ld, I-POS:%ld \n", idd.ch, rdb.bid, idd.fpos, idx_pos);

	return BKT_OK;
}

long BKTDEC_GetRelsolutionFromVHDR(int* pCh, int *pWidth, int *pHeight)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	long cur_fpos = LTELL(gBKTDEC_Ctrl.hbkt);

	int ret = READ_STRC(gBKTDEC_Ctrl.hbkt, shd);
#ifdef BKT_SYSIO_CALL
	if(ret < 1)
		return BKT_ERR;
#else
	if(ret != sizeof(shd))
		return BKT_ERR;
#endif

	if(shd.id == ID_VIDEO_HEADER)
	{
		if(READ_ST(gBKTDEC_Ctrl.hbkt, vhd))
		{
			if(LSEEK_SET(gBKTDEC_Ctrl.hbkt, cur_fpos))
			{
				*pCh = shd.ch;
				*pWidth  = vhd.width;
				*pHeight = vhd.height;
				TRACE0_BKTDEC("Read Resolution CH:%02d, %d X %d \n", shd.ch, vhd.width, vhd.height);
				return BKT_OK;
			}
		}
	}

	return BKT_ERR;
}

long BKTDEC_close(int bStop)
{
	TRACE0_BKTDEC("close basket decoding. CH:%d, BID:%ld\n", gBKTDEC_Ctrl.ch, gBKTDEC_Ctrl.bid);

	if(bStop)
	{
		gBKTDEC_status = BKTDEC_STATUS_STOP;
	}

	SAFE_CLOSE_BKT(gBKTDEC_Ctrl.hbkt);
	SAFE_CLOSE_BKT(gBKTDEC_Ctrl.hidx);

	gBKTDEC_Ctrl.ch  = 0;
	gBKTDEC_Ctrl.bid = 0; // caution not zero-base

	// BK-1222
	gBKTDEC_Ctrl.fpos_bkt =0;

	gBKTDEC_Ctrl.iframe_cnt = 0;
	gBKTDEC_Ctrl.iframe_cur = 0;

	return BKT_OK;
}

long BKTDEC_GetFirstRecTime(int ch)
{
    struct dirent *ent;
    DIR *dp;

	char buf[255];
	sprintf(buf, "%s/rdb", gBKTDEC_Ctrl.target_path);
    dp = opendir(buf);

    if (dp != NULL)
    {
		int min_rdb=0;

        for(;;)
        {
            ent = readdir(dp);
            if (ent == NULL)
                break;

			if (ent->d_name[0] == '.')  // except [.] and [..]
			{
				if (!ent->d_name[1] || (ent->d_name[1] == '.' && !ent->d_name[2]))
				{
					continue;
				}
			}

			if(min_rdb==0)
				min_rdb = atoi(ent->d_name);
			else
				min_rdb = min(atoi(ent->d_name), min_rdb);

        }

        closedir(dp);

		if(min_rdb != 0)
		{
#ifdef BKT_SYSIO_CALL
			int fd;
#else
			FILE *fd;
#endif
			char path[LEN_PATH];
			int c;
			sprintf(path, "%s/rdb/%08d", gBKTDEC_Ctrl.target_path, min_rdb);

			if(!OPEN_RDONLY(fd, path))
			{
				TRACE1_BKTDEC("failed open rdb %s\n", path);
				return BKT_ERR;
			}

			char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, evts, sizeof(evts)))
			{
				TRACE1_BKTDEC("failed read rec evt\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
			CLOSE_BKT(fd);

			for(c=0;c<TOTAL_MINUTES_DAY;c++)
			{
				if(evts[TOTAL_MINUTES_DAY*ch+c] != 0)
				{
					char buff[64];
					struct tm tm1;
					time_t t1;

					int yy = min_rdb/10000;
					int mm = (min_rdb%10000)/100;
					int dd = (min_rdb%10000)%100;

					int HH = c/60;
					int MM = c%60;

					sprintf(buff, "%04d-%02d-%02d %02d:%02d:00", yy, mm, dd, HH, MM);
					strptime(buff, "%Y-%m-%d %H:%M:%S", &tm1);

					t1 = mktime(&tm1);

					TRACE0_BKTDEC("get first record time:%s\n", ctime(&t1));


					return t1;
				}
			}
		}

    }

	TRACE0_BKTDEC("failed get first record time\n");
	return BKT_ERR;
}

long BKTDEC_GetLastRecTime(int ch)
{
    struct dirent *ent;
    DIR *dp;

	char buf[255];
	sprintf(buf, "%s/rdb", gBKTDEC_Ctrl.target_path);
    dp = opendir(buf);

    if (dp != NULL)
    {
		int last_rdb=0;

        for(;;)
        {
            ent = readdir(dp);
            if (ent == NULL)
                break;

			if (ent->d_name[0] == '.')
			{
				if (!ent->d_name[1] || (ent->d_name[1] == '.' && !ent->d_name[2]))
				{
					continue;
				}
			}

			if(last_rdb==0)
				last_rdb = atoi(ent->d_name);
			else
				last_rdb = max(atoi(ent->d_name), last_rdb);

        }

        closedir(dp);

		if(last_rdb != 0)
		{
			int c;
			int bFoundFirst=FALSE;
#ifdef BKT_SYSIO_CALL
			int fd;
#else
			FILE *fd;
#endif
			char path[LEN_PATH];
			sprintf(path, "%s/rdb/%08d", gBKTDEC_Ctrl.target_path, last_rdb);

			if(!OPEN_RDONLY(fd, path))
			{
				TRACE1_BKTDEC("failed open rdb %s\n", path);
				return BKT_ERR;
			}

			char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, evts, sizeof(evts)))
			{
				TRACE1_BKTDEC("failed read rec data\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
			CLOSE_BKT(fd);

			for(c=TOTAL_MINUTES_DAY-1;c>=0;c--)
			{
				if(evts[TOTAL_MINUTES_DAY*ch+c] != 0)
				{
					if(bFoundFirst == TRUE)
					{
						char buff[64];
						struct tm tm1;
						time_t t1;
						int yy = last_rdb/10000;
						int mm = (last_rdb%10000)/100;
						int dd = (last_rdb%10000)%100;

						int HH = c/60;
						int MM = c%60;

						sprintf(buff, "%04d-%02d-%02d %02d:%02d:00", yy, mm, dd, HH, MM);
						strptime(buff, "%Y-%m-%d %H:%M:%S", &tm1);

						t1 = mktime(&tm1);

						TRACE0_BKTDEC("get last -1 rec time. %04d-%02d-%02d %02d:%02d:00\n", yy, mm, dd, HH, MM);
						CLOSE_BKT(fd);

						return t1;
					}
					bFoundFirst = TRUE;

					TRACE0_BKTDEC("found real last rec time. %02d:%02d\n", c/60, c%60);
				}
			}

		}

    }

	TRACE0_BKTDEC("failed get last record time\n");
	return BKT_ERR;
}

long BKTDEC_GetRecDays(int ch, long t_o, char* pRecMonTBL)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(t_o, &tm1);

	int i, bFindOK=0;

	char path[LEN_PATH];

	for(i=0;i<31;i++)
	{
		sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, i+1);

		if(-1 != access(path, 0))
		{
			*(pRecMonTBL+i) = 1;

			if(!bFindOK)
				bFindOK=1;
		}
	}

	return bFindOK;
}

long BKTDEC_GetRecHour(int ch, long sec, char* pRecHourTBL)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	long offset = TOTAL_MINUTES_DAY*ch;
	if(!LSEEK_SET(fd, offset))
	{
		TRACE1_BKTDEC("failed seek rdb.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, pRecHourTBL, TOTAL_MINUTES_DAY))
	{
		TRACE1_BKTDEC("failed read rdb CH:%d, offset:%ld\n", ch, offset);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	TRACE0_BKTDEC("Succeeded Get record hour info, ch:%d, %04d/%02d/%02d %02d:%02d:%02d\n", ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	CLOSE_BKT(fd);

	return 0;

}

long BKTDEC_GetRecHourInfoAll(long sec, char* pRecHourAll)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	// read all channel rec time info.
	if(!READ_PTSZ(fd, pRecHourAll, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
	{
		TRACE1_BKTDEC("failed read rdb table.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	TRACE0_BKTDEC("Succeeded Get record hour all info, %04d/%02d/%02d\n", tm1.year, tm1.mon, tm1.day);
	CLOSE_BKT(fd);

	return 0;
}

int  BKTDEC_isRecData(int ch, long sec, int direction)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	int i;
	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		TRACE1_BKTDEC("failed read rec data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	if(BKT_FORWARD == direction)
	{
		for(i=tm1.hour*60+tm1.min;i<TOTAL_MINUTES_DAY;i++)
		{
			if(evts[ch*TOTAL_MINUTES_DAY+i] != 0)
			{
				return BKT_OK;
			}
		}
	}
	else if(BKT_BACKWARD == direction)
	{
		for(i=tm1.hour*60+tm1.min; i>=0 ; i--)
		{
			if(evts[ch*TOTAL_MINUTES_DAY+i] != 0)
			{
				return BKT_OK;
			}
		}
	}
	else //(BKT_NOSEARCHDIRECTION== direction)
	{
		if(evts[ch*TOTAL_MINUTES_DAY+tm1.hour*60+tm1.min] != 0)
		{
			return BKT_OK;
		}
	}

	TRACE1_BKTDEC("No record data. %04d-%02d-%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	//TRACE1_BKTDEC("failed search rec times. %04d-%02d-%02d %02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min);

	return BKT_ERR;
}

// pChannels is multiple channel flags.
int  BKTDEC_getRecFileCount(int *pChannels, long t1, long t2)
{
	char path1[LEN_PATH];
	char path2[LEN_PATH];

	// ignore seconds because rdb is min base.
	T_BKT_TM tm1;
	BKT_GetLocalTime(t1, &tm1);
	sprintf(path1, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	T_BKT_TM tm2;
	BKT_GetLocalTime(t2, &tm2);
	sprintf(path2, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm2.year, tm2.mon, tm2.day);

	if(strcmp(path1, path2) != 0)
	{
		TRACE0_BKTDEC("different from-date:%s and to-date:%s\n", path1, path2);
		return 0;
	}

	if(-1 == access(path1, 0)) // existence only
	{
		TRACE1_BKTDEC("There is no rdb file. %s\n", path1);
		return 0;
	}

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	if(!OPEN_RDONLY(fd, path1))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path1);
		return BKT_ERR;
	}

	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		TRACE1_BKTDEC("failed read evt data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		TRACE1_BKTDEC("failed read rdbs data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	long prev_bid=0;
	int basket_count=0;
	int m, c;
	int chs[BKT_MAX_CHANNEL];
	memcpy(chs, pChannels, sizeof(chs));

	for(m=tm1.hour*60+tm1.min;m<=tm2.hour*60+tm2.min;m++)
	{
		for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
		{
			printf("chs[%d]=%d\n", c, chs[c]);
			if(rdbs[TOTAL_MINUTES_DAY*c+m].bid != 0
				&& prev_bid != rdbs[TOTAL_MINUTES_DAY*c+m].bid
				&& chs[c] == 1)
			{
				basket_count++;
				prev_bid = rdbs[TOTAL_MINUTES_DAY*c+m].bid;
				break;
			}
		}
	}

	TRACE0_BKTDEC("get backup file count :%d\n", basket_count);

	return basket_count;
}


int  BKTDEC_getRecFileList(int *pChannels, long t1, long t2, int *pFileArray, char *srcPath)
{
	char path1[LEN_PATH];
	char path2[LEN_PATH];

	// ignore seconds because rdb is min base.
	T_BKT_TM tm1;
	BKT_GetLocalTime(t1, &tm1);
	sprintf(path1, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);

	T_BKT_TM tm2;
	BKT_GetLocalTime(t2, &tm2);
	sprintf(path2, "%s/rdb/%04d%02d%02d", gBKTDEC_Ctrl.target_path, tm2.year, tm2.mon, tm2.day);

	if(strcmp(path1, path2) != 0)
	{
		TRACE0_BKTDEC("different from-date:%s and to-date:%s\n", path1, path2);
		return 0;
	}

	if(-1 == access(path1, 0)) // existence only
	{
		TRACE1_BKTDEC("There is no rdb file. %s\n", path1);
		return 0;
	}

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	if(!OPEN_RDONLY(fd, path1))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path1);
		return BKT_ERR;
	}

	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		TRACE1_BKTDEC("failed read evt data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		TRACE1_BKTDEC("failed read rdbs data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	long prev_bid=0;
	int basket_count=0;
	int m,c;
	int chs[BKT_MAX_CHANNEL];
	memcpy(chs, pChannels, sizeof(chs));
	sprintf(srcPath, "%s", gBKTDEC_Ctrl.target_path);

	for(m=tm1.hour*60+tm1.min;m<=tm2.hour*60+tm2.min;m++)
	{
		for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
		{
			if(rdbs[TOTAL_MINUTES_DAY*c+m].bid != 0
				&& prev_bid != rdbs[TOTAL_MINUTES_DAY*c+m].bid
				&& chs[c] == 1)
			{
				//sprintf(buff, "%s/%08ld.bkt", gBKTDEC_Ctrl.target_path, rdbs[TOTAL_MINUTES_DAY*c+m].bid);
				//TRACE0_BKTDEC("copy backup file list [%02d:%02d], ch:%d - BID:%d, %s\n", m/60, m%60, c, rdbs[TOTAL_MINUTES_DAY*c+m].bid, buff);
				//memcpy(&pFileArray[basket_count*BKT_PATH_LEN], buff, BKT_PATH_LEN);

				*(pFileArray+basket_count) = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

				TRACE0_BKTDEC("found backup basket. BID:%d\n", *(pFileArray+basket_count));

				basket_count++;
				prev_bid = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

				break;
			}
		}
	}

	return basket_count;
}

void BKTDEC_setTargetDisk(const char *path)
{
	sprintf(gBKTDEC_Ctrl.target_path, "%s", path);

#ifdef SEP_AUD_TASK
	sprintf(gBKTDEC_CtrlAud.target_path, "%s", path);
#endif

	TRACE0_BKTDEC("set playback target disk path :%s\n", gBKTDEC_Ctrl.target_path);
}

int  BKTDEC_GetNextRecordChannel(long sec, int *recCh)
{
	T_INDEX_DATA idd;
	int  recChannel[BKT_MAX_CHANNEL]={0};
	int  recChannelCount=0;
	long fpos_index = LTELL(gBKTDEC_Ctrl.hidx);


	while(fpos_index != 0)
	{
		long ret = READ_STRC(gBKTDEC_Ctrl.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
			return recChannelCount;
		}
#else
		if(ret != sizeof(idd))
		{
			LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
			return recChannelCount;
		}
#endif//BKT_SYSIO_CALL

		if(idd.ts.sec >=sec)
		{
			if(recChannel[idd.ch] == 1)
			{
				LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
				memcpy(recCh, recChannel, sizeof(recChannel));
				TRACE0_BKTDEC("get current record channel count:%d\n", recChannelCount);
				return recChannelCount;
			}
			else
			{
				recChannel[idd.ch] = 1;
				recChannelCount++;
			}
		}
	}

	return 0;
}


int  BKTDEC_GetPrevRecordChannel(long sec, int *recCh)
{
	T_INDEX_DATA idd;
	int recChannel[BKT_MAX_CHANNEL]={0};
	int recChannelCount=0;

	long fpos_index = LTELL(gBKTDEC_Ctrl.hidx);
	long iframe_cur = gBKTDEC_Ctrl.iframe_cur;

	while(fpos_index != 0)
	{
		if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, BKT_transIndexNumToPos(iframe_cur)))
		{
			LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
			return recChannelCount;
		}

		long ret = READ_STRC(gBKTDEC_Ctrl.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
			return recChannelCount;
		}
#else
		if(ret != sizeof(idd))
		{
			LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
			return recChannelCount;
		}
#endif//BKT_SYSIO_CALL

		if(idd.ts.sec <=sec)
		{
			if(recChannel[idd.ch] == 1)
			{
				LSEEK_SET(gBKTDEC_Ctrl.hidx, fpos_index);
				memcpy(recCh, recChannel, sizeof(recChannel));
				TRACE0_BKTDEC("get current record channel count:%d\n", recChannelCount);
				return recChannelCount;
			}
			else
			{
				recChannel[idd.ch] = 1;
				recChannelCount++;
			}
		}
		iframe_cur--;
	}

	return 0;
}

// normal read with audio
long BKTDEC_ReadNextFrame(T_BKTDEC_PARAM *dp)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	int search_next_iframe=FALSE;
	int ret;

	while(gBKTDEC_status == BKTDEC_STATUS_OPEN)
	{
		ret = READ_STRC(gBKTDEC_Ctrl.hbkt, shd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#else
		if(ret != sizeof(shd))
		{
			if(0 != feof(gBKTDEC_Ctrl.hbkt))
			{
				TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cnt, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#endif

		if(shd.id != ID_VIDEO_HEADER && shd.id != ID_AUDIO_HEADER)
		{
			search_next_iframe = TRUE;
			TRACE0_BKTDEC("maybe appeared broken stream .Search next i-frame. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			break;
		}
		else if(shd.id == ID_VIDEO_HEADER)
		{
			if(shd.ts.sec - gBKTDEC_Ctrl.sec > 4 || shd.ts.sec - gBKTDEC_Ctrl.sec < 0) // maybe max interval frame. for broken stream or motion.
			{
				search_next_iframe = TRUE;
				TRACE0_BKTDEC("time interval is too long. So search next frame. shd.id:0x%08lx\n", shd.id);
				break;
			}

			if(shd.ch == dp->ch)
			{
				if(!READ_ST(gBKTDEC_Ctrl.hbkt, vhd))
				{
					TRACE1_BKTDEC("failed read video header\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTDEC_Ctrl.hbkt, dp->vbuffer, shd.framesize))
				{
					TRACE1_BKTDEC("failed read video data\n");
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

				gBKTDEC_Ctrl.sec = shd.ts.sec;

				// BK-1222
				gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

				if(shd.frametype == FT_IFRAME )
				{
					gBKTDEC_Ctrl.iframe_cur += 1;

					if( BKT_isRecordingBasket(gBKTDEC_Ctrl.bid))
					{
						gBKTDEC_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
					}
				}

				//TRACE0_BKTDEC("Read frame, frameType:%d, iframe-num:%d, size:%ld\n", shd.frametype, gBKTDEC_Ctrl.iframe_cur, shd.framesize);

                dp->bktId      = gBKTDEC_Ctrl.bid;
                dp->fpos_bkt   = gBKTDEC_Ctrl.fpos_bkt;

				return BKT_OK;
			}
			else
			{
				// skip fpos to next frame
				if(!LSEEK_CUR(gBKTDEC_Ctrl.hbkt, sizeof(vhd) + shd.framesize))
				{
					TRACE1_BKTDEC("failed seek cur basket.\n");
					return BKT_ERR;
				}

				// BK-1222
				gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

				if(shd.frametype == FT_IFRAME )
				{
					gBKTDEC_Ctrl.iframe_cur += 1;

					if( BKT_isRecordingBasket(gBKTDEC_Ctrl.bid))
					{
						gBKTDEC_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
					}

					//TRACE0_BKTDEC("read iframe normal mode. CH:%d, num:%d, total:%d\n", shd.ch, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
			}
		}
		else if( shd.id == ID_AUDIO_HEADER )
		{
#ifndef SEP_AUD_TASK
			if(shd.ch == dp->ch)
			{
				if(!READ_ST(gBKTDEC_Ctrl.hbkt, ahd))
				{
					TRACE1_BKTDEC("failed read audio header\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTDEC_Ctrl.hbkt, dp->abuffer, shd.framesize))
				{
					TRACE1_BKTDEC("failed read audio data\n");
					return BKT_ERR;
				}

				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->streamtype  = ST_AUDIO;
				dp->framesize   = shd.framesize;
				dp->samplingrate= ahd.samplingrate;
				dp->fpos = 0;

				// BK-1222
				gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

				return BKT_OK;
			}
			else
#endif
			{
				// skip
				if(!LSEEK_CUR(gBKTDEC_Ctrl.hbkt, sizeof(ahd) + shd.framesize))
				{
					TRACE1_BKTDEC("failed seek cur basket.\n");
					return BKT_ERR;
				}

				// BK-1222
				gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

			}
		}


	}// while

	if(search_next_iframe == TRUE)
	{
		return BKTDEC_ReadNextIFrame(dp);
	}
	else if(gBKTDEC_status == BKTDEC_STATUS_OPEN) //if(gBKTDEC_Ctrl.iframe_cur >= gBKTDEC_Ctrl.iframe_cnt)
	{
		//TRACE1_BKTDEC("cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);

		long bid = BKTDEC_SearchNextBID(gBKTDEC_Ctrl.ch, gBKTDEC_Ctrl.bid, gBKTDEC_Ctrl.sec);

		if( bid == BKT_ERR )
			return BKT_ERR;

		if( BKT_ERR == BKTDEC_OpenNext(gBKTDEC_Ctrl.ch, bid))
			return BKT_ERR;

		return BKTDEC_ReadNextFrame(dp);
	}

	return BKT_ERR;
}

long BKTDEC_ReadNextIFrame(T_BKTDEC_PARAM *dp)
{
	//TRACE1_BKTDEC("cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);

	if(BKT_isRecordingBasket(gBKTDEC_Ctrl.bid))
	{
		gBKTDEC_Ctrl.iframe_cnt = BKT_GetRecordingIndexCount();
		//TRACE0_BKTDEC("get total i-frame count:%d\n", gBKTDEC_Ctrl.iframe_cur);
	}

	if(gBKTDEC_Ctrl.iframe_cur + 1 >= gBKTDEC_Ctrl.iframe_cnt) // find next basket
	{
		long bid = BKTDEC_SearchNextBID(gBKTDEC_Ctrl.ch, gBKTDEC_Ctrl.bid, gBKTDEC_Ctrl.sec);

		if( bid == BKT_ERR )
			return BKT_ERR;

		if( BKT_ERR == BKTDEC_OpenNext(gBKTDEC_Ctrl.ch, bid))
			return BKT_ERR;
	}
	else
	{
		gBKTDEC_Ctrl.iframe_cur += 1;
	}

	if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, BKT_transIndexNumToPos(gBKTDEC_Ctrl.iframe_cur)))
	{
		TRACE1_BKTDEC("failed seek read next frame. iframe:%d\n", gBKTDEC_Ctrl.iframe_cur);
		return BKT_ERR;
	}

	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;

	int search_next_iframe=FALSE;
	int ret;

	while(gBKTDEC_status == BKTDEC_STATUS_OPEN)
	{
		ret = READ_STRC(gBKTDEC_Ctrl.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				TRACE1_BKTDEC("failed read index data. EOF. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTDEC("failed read index data. maybe broken. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#else
		if(ret != sizeof(idd))
		{
			if(0 != feof(gBKTDEC_Ctrl.hidx))
			{
				TRACE1_BKTDEC("failed read index data. EOF. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_next_iframe = TRUE;
				TRACE1_BKTDEC("failed read index data. maybe broken. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#endif//BKT_SYSIO_CALL

		//TRACE0_BKTDEC("read index data. fpos:%d, ch:%d, sec:%ld\n", idd.fpos, idd.ch, idd.ts.sec);

		if(idd.ch == dp->ch && idd.ts.sec - gBKTDEC_Ctrl.sec > 0)
		{
			if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, idd.fpos))
			{
				TRACE1_BKTDEC("failed seek next index data. i-fpos:%ld\n", idd.fpos);
				return BKT_ERR;
			}

			ret = READ_STRC(gBKTDEC_Ctrl.hbkt, shd);
#ifdef BKT_SYSIO_CALL
			if(ret < 1)
			{
				if(ret == 0)
				{
					TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
				else
				{
					search_next_iframe = TRUE;
					TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
				break;
			}
#else
			if(ret != sizeof(shd))
			{
				if(0 != feof(gBKTDEC_Ctrl.hbkt))
				{
					TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cnt, gBKTDEC_Ctrl.iframe_cnt);
				}
				else
				{
					search_next_iframe = TRUE;
					TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cnt, gBKTDEC_Ctrl.iframe_cnt);
				}

				break;
			}
#endif//BKT_SYSIO_CALL

			if(shd.id != ID_VIDEO_HEADER) // Must be Video Frame. because searching iframe by index.
			{
				TRACE1_BKTDEC("failed read next stream header.CH:%ld, SHID:0x%08lx\n", shd.ch, shd.id);
				return BKT_ERR;
			}

			if(!READ_ST(gBKTDEC_Ctrl.hbkt, vhd))
			{
				TRACE1_BKTDEC("failed read next video header.\n");
				return BKT_ERR;
			}

			if(!READ_PTSZ(gBKTDEC_Ctrl.hbkt, dp->vbuffer, shd.framesize))
			{
				TRACE0_BKTDEC("failed read next video i-frame data.\n");
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
			dp->audioONOFF = vhd.audioONOFF; // AYK - 0201
			dp->capMode     = vhd.capMode;

			if(strlen(vhd.camName))
				strncpy(dp->camName, vhd.camName, 16);

			gBKTDEC_Ctrl.sec = shd.ts.sec;

			// BK-1222
			gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

            dp->bktId    = gBKTDEC_Ctrl.bid;
            dp->fpos_bkt = gBKTDEC_Ctrl.fpos_bkt;

			return BKT_OK;
		}
		else
		{
			gBKTDEC_Ctrl.iframe_cur +=1;

			if(gBKTDEC_Ctrl.iframe_cur >= gBKTDEC_Ctrl.iframe_cnt) // find next basket
			{
				break;
			}
			else if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, BKT_transIndexNumToPos(gBKTDEC_Ctrl.iframe_cur)))
			{
				TRACE1_BKTDEC("failed seek read prev frame. iframe-cur:%d\n", gBKTDEC_Ctrl.iframe_cur);
				return BKT_ERR;
			}
		}

	}// end while

	if(search_next_iframe == TRUE)
	{
		return BKTDEC_ReadNextIFrame(dp);
	}

	return BKTDEC_ReadNextIFrame(dp);
}

long BKTDEC_ReadPrevIFrame(T_BKTDEC_PARAM *dp)
{
	//TRACE0_BKTDEC("seek idx prev iframe:%d, fpos:%ld\n", gBKTDEC_Ctrl.iframe_cur, BKT_transIndexNumToPos(gBKTDEC_Ctrl.iframe_cur));

	if(gBKTDEC_Ctrl.iframe_cur  - 1 < 0) // find prev basket
	{
		long bid = BKTDEC_SearchPrevBID(gBKTDEC_Ctrl.ch, gBKTDEC_Ctrl.bid, gBKTDEC_Ctrl.sec);

		if( bid == BKT_ERR )
			return BKT_ERR;

		if( BKT_ERR == BKTDEC_OpenPrev(gBKTDEC_Ctrl.ch, bid))
			return BKT_ERR;
	}
	else
	{
		gBKTDEC_Ctrl.iframe_cur -= 1;
	}

	if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, BKT_transIndexNumToPos(gBKTDEC_Ctrl.iframe_cur)))
	{
		TRACE1_BKTDEC("failed seek read prev frame. iframe:%d\n", gBKTDEC_Ctrl.iframe_cur);
		return BKT_ERR;
	}

	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;

	int search_prev_iframe=FALSE;
	int ret;

	while(gBKTDEC_status == BKTDEC_STATUS_OPEN)
	{
		ret = READ_STRC(gBKTDEC_Ctrl.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				TRACE1_BKTDEC("failed read index data. EOF. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_prev_iframe = TRUE;
				TRACE1_BKTDEC("failed read index data. maybe broken. go prev iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#else
		if(ret != sizeof(idd))
		{
			if(0 != feof(gBKTDEC_Ctrl.hidx))
			{
				TRACE1_BKTDEC("failed read index data. EOF. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			else
			{
				search_prev_iframe = TRUE;
				TRACE1_BKTDEC("failed read index data. maybe broken. go prev iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
			}
			break;
		}
#endif//BKT_SYSIO_CALL

		//TRACE0_BKTDEC("index data fpos:%d, ch:%d, sec:%ld, curiframe:%d, totiframe:%d\n", idd.fpos, idd.ch, idd.ts.sec, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);

		if(idd.ch == dp->ch && gBKTDEC_Ctrl.sec - idd.ts.sec > 0)
		{
			if(!LSEEK_SET(gBKTDEC_Ctrl.hbkt, idd.fpos))
			{
				TRACE1_BKTDEC("failed seek prev index data. i-fpos:%ld\n", idd.fpos);
				return BKT_ERR;
			}

			ret = READ_STRC(gBKTDEC_Ctrl.hbkt, shd);
#ifdef BKT_SYSIO_CALL
			if(ret < 1)
			{
				if(ret == 0)
				{
					TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
				else
				{
					search_prev_iframe = TRUE;
					TRACE1_BKTDEC("failed read stream header. go prev iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
				break;
			}
#else
			if(ret != sizeof(shd))
			{
				if(0 != feof(gBKTDEC_Ctrl.hbkt))
				{
					TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}
				else
				{
					search_prev_iframe = TRUE;
					TRACE1_BKTDEC("failed read stream header. go prev iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTDEC_Ctrl.iframe_cur, gBKTDEC_Ctrl.iframe_cnt);
				}

				break;
			}
#endif//BKT_SYSIO_CALL

			if(shd.id != ID_VIDEO_HEADER) // Must be Video Frame. because searching iframe by index.
			{
				TRACE1_BKTDEC("failed read prev stream header.CH:%ld, SHID:0x%08lx\n", shd.ch, shd.id);
				return BKT_ERR;
			}

			if(!READ_ST(gBKTDEC_Ctrl.hbkt, vhd))
			{
				TRACE1_BKTDEC("failed read prev video header.\n");
				return BKT_ERR;
			}

			if(!READ_PTSZ(gBKTDEC_Ctrl.hbkt, dp->vbuffer, shd.framesize))
			{
				TRACE0_BKTDEC("failed read prev video i-frame data.\n");
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
			dp->audioONOFF = vhd.audioONOFF; // AYK - 0201
			dp->capMode     = vhd.capMode;

			if(strlen(vhd.camName))
				strncpy(dp->camName, vhd.camName, 16);

			gBKTDEC_Ctrl.sec = shd.ts.sec;

			// BK-1222
			gBKTDEC_Ctrl.fpos_bkt = LTELL(gBKTDEC_Ctrl.hbkt);

            dp->bktId    = gBKTDEC_Ctrl.bid;
            dp->fpos_bkt = gBKTDEC_Ctrl.fpos_bkt;

			return BKT_OK;
		}
		else
		{
			gBKTDEC_Ctrl.iframe_cur -=1;

			if(gBKTDEC_Ctrl.iframe_cur < 0 ) // find prev basket.
			{
				break;
			}
			else if(!LSEEK_SET(gBKTDEC_Ctrl.hidx, BKT_transIndexNumToPos(gBKTDEC_Ctrl.iframe_cur)))
			{
				TRACE1_BKTDEC("failed seek read prev frame. iframe-cur:%d\n", gBKTDEC_Ctrl.iframe_cur);
				return BKT_ERR;
			}
		}

	}// end while

	if(search_prev_iframe == TRUE)
	{
		return BKTDEC_ReadPrevIFrame(dp);
	}

	return BKTDEC_ReadPrevIFrame(dp);
}


//////////////////////////////////////////////////////////////////////////
#ifdef SEP_AUD_TASK
long BKTDEC_closeAud()
{
	TRACE0_BKTDEC("close basket audio. BID_aud:%ld\n", gBKTDEC_CtrlAud.bid);

	SAFE_CLOSE_BKT(gBKTDEC_CtrlAud.hbkt);
	SAFE_CLOSE_BKT(gBKTDEC_CtrlAud.hidx);

	gBKTDEC_CtrlAud.ch  = 0;
	gBKTDEC_CtrlAud.bid = 0; // caution not zero-base

	// BK-1222
	gBKTDEC_CtrlAud.fpos_bkt =0;

	gBKTDEC_CtrlAud.iframe_cnt = 0;
	gBKTDEC_CtrlAud.iframe_cur = 0;


	return BKT_OK;
}

long BKTDEC_OpenAud(int ch, long sec, int nDirection)
{
	BKTDEC_closeAud();

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

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTDEC_CtrlAud.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		TRACE1_BKTDEC("failed open rdb %s\n", path);
		return BKT_ERR;
	}
	TRACE0_BKTDEC("open rdb. %s\n", path);

	if(!LSEEK_SET(fd, size_evt))
	{
 		TRACE1_BKTDEC("failed seek evts.\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	// read rdb(bid, pos)
	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
 	{
 		TRACE1_BKTDEC("failed read rdb data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}
 	CLOSE_BKT(fd);

	if(nDirection == BKT_FORWARD ) // forward
	{
		for(i=tm1.hour*60+tm1.min;i<TOTAL_MINUTES_DAY;i++)
		{
			if(rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0)
			{
				rdb.bid     = rdbs[TOTAL_MINUTES_DAY*ch+i].bid;
				rdb.idx_pos = rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos;

				TRACE0_BKTDEC("found rec time forward, CH:%d, %02d:%02d, BID:%ld, idx_pos:%ld\n", ch, i/60, i%60, rdb.bid, rdb.idx_pos);
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

				TRACE0_BKTDEC("found rec time backward, %02d:%02d, BID:%ld, idx_pos:%ld\n", i/60, i%60, rdb.bid, rdb.idx_pos);

				break;
			}
		}
	}

	if(rdb.bid ==0)
	{
		TRACE1_BKTDEC("failed open basket for decoding -- path:%s, ts:%s\n", path, ctime(&sec));
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_CtrlAud.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file for audio decoding -- path:%s\n", path);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	sprintf(path, "%s/%08ld.idx", gBKTDEC_CtrlAud.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file for decoding -- path:%s\n", path);
		CLOSE_BKT(gBKTDEC_CtrlAud.hbkt);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_CtrlAud.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_closeAud();

		return BKT_ERR;
	}

	gBKTDEC_CtrlAud.iframe_cnt = BKT_isRecordingBasket(rdb.bid)? BKT_GetRecordingIndexCount():ihd.count;

	if(!LSEEK_SET(gBKTDEC_CtrlAud.hidx, rdb.idx_pos))
	{
		TRACE1_BKTDEC("failed seek index hdr\n");
		BKTDEC_closeAud();

		return BKT_ERR;
	}

	T_INDEX_DATA idd;
	long idx_pos=rdb.idx_pos;
	while(1)
	{
		if(!READ_ST(gBKTDEC_CtrlAud.hidx, idd))
		{
			TRACE1_BKTDEC("failed read index data\n");
			BKTDEC_closeAud();
			return BKT_ERR;
		}

		if( idd.ch == ch )
		{
			idx_pos = LTELL(gBKTDEC_CtrlAud.hidx) - sizeof(idd);
			TRACE0_BKTDEC("found index fpos:%ld, saved fpos:%ld\n", idx_pos, rdb.idx_pos);
			break;
		}
	}

	gBKTDEC_CtrlAud.iframe_cur = BKT_transIndexPosToNum(idx_pos);

	TRACE0_BKTDEC("read index hdr, iframe count:%d, iframe-number:%d\n", gBKTDEC_CtrlAud.iframe_cnt, gBKTDEC_CtrlAud.iframe_cur);

	if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt, idd.fpos))
	{
		TRACE1_BKTDEC("failed seek audio data\n");
		BKTDEC_closeAud();
		return BKT_ERR;
	}

	if(!LSEEK_SET(gBKTDEC_CtrlAud.hidx, idx_pos))
	{
		TRACE1_BKTDEC("failed seek \n");
		BKTDEC_closeAud();
		return BKT_ERR;
	}

	gBKTDEC_CtrlAud.ch  = ch;
	gBKTDEC_CtrlAud.bid = rdb.bid;
	gBKTDEC_CtrlAud.sec = idd.ts.sec;

	// BK-1222
	gBKTDEC_CtrlAud.fpos_bkt =idd.fpos;


	TRACE0_BKTDEC("open basket file for audio decoding -- CH:%ld, BID:%ld, B-POS:%ld, I-POS:%ld \n", idd.ch, rdb.bid, idd.fpos, idx_pos);

	return BKT_OK;
}

long BKTDEC_OpenNextAud(int ch, long nextbid)
{
	long last_read_sec = gBKTDEC_CtrlAud.sec;

	BKTDEC_closeAud();

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_CtrlAud.target_path, nextbid);

	// open basket
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTDEC_CtrlAud.target_path, nextbid);
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file -- path:%s\n", path);
		CLOSE_BKT(gBKTDEC_CtrlAud.hbkt);
		return BKT_ERR;
	}

	// read index hdr
	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_CtrlAud.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_closeAud();
		return BKT_ERR;
	}

	TRACE0_BKTDEC("Read index hdr. CH:%d, AudBID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ch, ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTDEC_CtrlAud.hbkt, bhdr))
	//if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt, sizeof(T_BASKET_HDR)))
	{
		TRACE1_BKTDEC("failed read basket aud header.\n");
		BKTDEC_closeAud();
		return BKT_ERR;
	}

	gBKTDEC_CtrlAud.ch  = ch;
	gBKTDEC_CtrlAud.bid = nextbid;
	gBKTDEC_CtrlAud.iframe_cnt = BKT_isRecordingBasket(nextbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTDEC_CtrlAud.sec = BKT_GetMinTS(bhdr.ts.t1);

	// BK - 1223 - Start
	gBKTDEC_CtrlAud.iframe_cur = 0;
	gBKTDEC_CtrlAud.fpos_bkt = sizeof(T_BASKET_HDR);


	TRACE0_BKTDEC("open next basket. AudBID:%ld, iframe-cur:%d, iframe_tot:%d\n", nextbid, gBKTDEC_CtrlAud.iframe_cur, gBKTDEC_CtrlAud.iframe_cnt);

	return BKT_OK;
}


long BKTDEC_SearchNextBIDAud(int ch, long curbid, long sec)
{
	if(BKT_isRecordingBasket(curbid))
	{
		TRACE1_BKTDEC("There is no a next basekt aud. BID:%ld is recording.\n", curbid);
		return BKT_ERR;
	}

	long sec_seed = sec;
	if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTDEC_GetBasketTime(ch, curbid, &bts, &ets) || ets == 0)
		{
			TRACE1_BKTDEC("There is no a next basekt aud. CH:%d, BID:%ld is recording.\n", ch, curbid);
			return BKT_ERR;
		}

		sec_seed = ets;
		//TRACE0_BKTDEC("last play time is zero. getting last time stamp from basket aud. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[255];
	sprintf(buf, "%s/rdb/", gBKTDEC_CtrlAud.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort);

	if(nitems < 0) {
	    perror("scandir search next bid for audio");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TRACE0_BKTDEC("There is no rdb directory %s.\n", buf);
		free(ents);
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int next_rdb=0;

    BKT_FHANDLE fd;

    for(i=0;i<nitems;i++)
    {
		if (ents[i]->d_name[0] == '.') // except [.] and [..]
		{
			if (!ents[i]->d_name[1] || (ents[i]->d_name[1] == '.' && !ents[i]->d_name[2]))
			{
				free(ents[i]);
				continue;
			}
		}

		next_rdb = atoi(ents[i]->d_name);

		if(next_rdb >= curr_rdb)
		{
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTDEC_CtrlAud.target_path, ents[i]->d_name);

			if(-1 == access(buf, 0)) {
				free(ents[i]);
				continue;
			}

			if(!OPEN_RDONLY(fd, buf))
			{
				TRACE1_BKTDEC("failed open rdb file. %s\n", buf);
				goto lbl_err_search_next_bid_aud;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				TRACE1_BKTDEC("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid_aud;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				TRACE1_BKTDEC("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid_aud;
			}
			CLOSE_BKT(fd);

			int m;
			for(m=tm1.hour*60+tm1.min+1;m<TOTAL_MINUTES_DAY;m++)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTDEC("found next AudBID:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid);

					for(;i<nitems;i++){
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}

			tm1.hour = 0;
			tm1.min  = -1;

		}  //if(next_rdb >= curr_rdb)

		free(ents[i]);

    }

lbl_err_search_next_bid_aud:
    for(;i<nitems;i++){
	    free(ents[i]);
	}
	free(ents);
    
	TRACE0_BKTDEC("Can't find next basket. CUR-AudBID:%ld, CH:%d, CUR-AudTS:%04d-%02d-%02d %02d:%02d:%02d\n", curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found
}

// AYK - 0119 - START

long BKTDEC_SearchPrevBIDAud(int ch, long curbid, long sec)
{
    long sec_seed = sec;
    if(sec_seed == 0)
    {
        long bts=0,ets=0;
        if(BKT_ERR == BKTDEC_GetBasketTime(ch, curbid, &bts, &ets) || bts == 0)
        {
            TRACE1_BKTDEC("There is no a previous basekt. CH:%d, BID:%ld.\n", ch, curbid);
            return BKT_ERR;
        }

        sec_seed = bts;
        //TRACE0_BKTDEC("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
    }

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

    char buf[255];
    sprintf(buf, "%s/rdb/", gBKTDEC_CtrlAud.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort_reverse);

	if(nitems < 0) {
	    perror("scandir search previous(reverse) bid for audio");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TRACE0_BKTDEC("There is no rdb directory %s.\n", buf);
		free(ents);
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int prev_rdb=0;

    BKT_FHANDLE fd;

    for(i=0;i<nitems;i++)
    {
		if (ents[i]->d_name[0] == '.') // except [.] and [..]
		{
			if (!ents[i]->d_name[1] || (ents[i]->d_name[1] == '.' && !ents[i]->d_name[2]))
			{
				free(ents[i]);
				continue;
			}
		}

		prev_rdb = atoi(ents[i]->d_name);

		if(prev_rdb <= curr_rdb)
		{
			sprintf(buf, "%s/rdb/%s", gBKTDEC_CtrlAud.target_path, ents[i]->d_name);

			if(-1 == access(buf, 0)) {
				free(ents[i]);
				continue;
			}

			if(!OPEN_RDONLY(fd, buf))
			{
				TRACE1_BKTDEC("failed open rdb file. %s\n", buf);
				goto lbl_err_search_prev_bid_aud;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				TRACE1_BKTDEC("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid_aud;
			}

			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				TRACE1_BKTDEC("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid_aud;
			}
			CLOSE_BKT(fd);

			int m;
			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0 && rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTDEC("found prev BID:%ld, IDXPOS:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, rdbs[TOTAL_MINUTES_DAY*ch+m].idx_pos);

					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}

			// begin again
			tm1.hour = 23;
			tm1.min  = 60;

		} //if(prev_rdb <= curr_rdb)

		free(ents[i]);

    } // end for

lbl_err_search_prev_bid_aud:
	for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	TRACE0_BKTDEC("Can't find prev basket. CUR-BID:%ld, CH:%d, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n", curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found
}

long BKTDEC_OpenPrevAud(int ch, long prevbid)
{
	BKTDEC_closeAud(FALSE);

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTDEC_CtrlAud.target_path, prevbid);

	// open basket
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hbkt, path))
	{
		TRACE1_BKTDEC("failed open basket file -- path:%s\n", path);
		BKTDEC_closeAud(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTDEC_CtrlAud.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTDEC_CtrlAud.hidx, path))
	{
		TRACE1_BKTDEC("failed open index file -- path:%s\n", path);
		BKTDEC_close(TRUE);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTDEC_CtrlAud.hidx, ihd))
	{
		TRACE1_BKTDEC("failed read index hdr\n");
		BKTDEC_closeAud(TRUE);
		return BKT_ERR;
	}

	TRACE0_BKTDEC("Read index hdr. BID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	if(ihd.count != 0)
	{
		if(!LSEEK_CUR(gBKTDEC_CtrlAud.hidx, sizeof(T_INDEX_DATA)*(ihd.count-1)))
		{
			TRACE1_BKTDEC("failed seek idx. ihd.count=%d\n", ihd.count);
			BKTDEC_closeAud(TRUE);
			return BKT_ERR;
		}

		T_INDEX_DATA idd;
		if(!READ_ST(gBKTDEC_CtrlAud.hidx, idd))
		{
			TRACE1_BKTDEC("failed read index data\n");
			BKTDEC_closeAud(TRUE);
			return BKT_ERR;
		}

		if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt, idd.fpos))
		{
			TRACE1_BKTDEC("failed seek basket.\n");
			BKTDEC_closeAud(TRUE);
			return BKT_ERR;
		}
	}
	else
	{
		long prev_sec = 0;
		long prev_usec = 0;
		int index_count=0, bkt_fpos=0;
		T_INDEX_DATA idd;
		while(READ_ST(gBKTDEC_CtrlAud.hidx, idd))
		{
			if( (idd.ts.sec <= prev_sec && idd.ts.usec <= prev_usec)
				|| idd.id != ID_INDEX_DATA
				|| idd.fpos <= bkt_fpos)
			{
				TRACE1_BKTDEC("Fount last record index pointer. idd.id=0x%08x, sec=%d, usec=%d, count=%d\n", idd.ts.sec, idd.ts.usec, index_count);
				break;
			}

			prev_sec  = idd.ts.sec;
			prev_usec = idd.ts.usec;
			bkt_fpos = idd.fpos;
			index_count++;
		}

		if(index_count != 0)
		{
			if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt, bkt_fpos))
			{
				TRACE1_BKTDEC("failed seek basket.\n");
				BKTDEC_closeAud(TRUE);
				return BKT_ERR;
			}

			if(!LSEEK_CUR(gBKTDEC_CtrlAud.hidx, sizeof(T_INDEX_DATA)*ihd.count))
			{
				TRACE1_BKTDEC("failed seek idx. ihd.count=%d\n", ihd.count);
				BKTDEC_closeAud(TRUE);
				return BKT_ERR;
			}
		}
		else
		{
			if(!LSEEK_END(gBKTDEC_CtrlAud.hbkt, 0))
			{
				TRACE1_BKTDEC("failed seek basket.\n");
				BKTDEC_closeAud(TRUE);
				return BKT_ERR;
			}

			int offset = sizeof(T_INDEX_DATA);
			if(!LSEEK_END(gBKTDEC_CtrlAud.hidx, -offset))
			{
				TRACE1_BKTDEC("failed seek idx.\n");
				BKTDEC_closeAud(TRUE);
				return BKT_ERR;
			}
		}
	}


	gBKTDEC_CtrlAud.ch  = ch;
	gBKTDEC_CtrlAud.bid = prevbid;
	gBKTDEC_CtrlAud.iframe_cnt = BKT_isRecordingBasket(prevbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTDEC_CtrlAud.sec = ihd.ts.t2[ch].sec;
	gBKTDEC_CtrlAud.iframe_cur = gBKTDEC_CtrlAud.iframe_cnt - 1;

	// BK - 1223
	gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);

	TRACE0_BKTDEC("open prev basket. BID:%ld, iframe-current:%d, iframe_total:%d\n", prevbid, gBKTDEC_CtrlAud.iframe_cur, gBKTDEC_CtrlAud.iframe_cnt);

	return BKT_OK;
}

// AYK - 0119 - END

long BKTDEC_ReadNextFrameAud(T_BKTDEC_PARAM *dp)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	int search_next_frame=FALSE;
	int ret;
	int frameCnt = 0; // AYK - 0118

	while(gBKTDEC_status == BKTDEC_STATUS_OPEN)
	{
		ret = READ_STRC(gBKTDEC_CtrlAud.hbkt, shd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				TRACE1_BKTDEC("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTDEC_CtrlAud.iframe_cur, gBKTDEC_CtrlAud.iframe_cnt);
			}
			else
			{
				search_next_frame = TRUE;
				TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			}

			break;
		}
#else
		if(ret != sizeof(shd))
		{
			if(0 != feof(gBKTDEC_CtrlAud.hbkt))
			{
				TRACE1_BKTDEC("failed read stream header. EOF.\n");
			}
			else
			{
				search_next_frame = TRUE;
				TRACE1_BKTDEC("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			}

			break;
		}
#endif

		if( shd.id == ID_AUDIO_HEADER )
		{
			if(shd.ch == dp->ch)
			{
				if(!READ_ST(gBKTDEC_CtrlAud.hbkt, ahd))
				{
					TRACE1_BKTDEC("failed read audio header data\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTDEC_CtrlAud.hbkt, dp->abuffer, shd.framesize))
				{
					TRACE1_BKTDEC("failed read audio data\n");
					return BKT_ERR;
				}

				dp->ch		    = shd.ch;
				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->streamtype  = ST_AUDIO;
				dp->framesize   = shd.framesize;
				dp->samplingrate= ahd.samplingrate;
				dp->fpos = 0;

                gBKTDEC_CtrlAud.sec = shd.ts.sec; // AYK - 1215

				// BK-1222
				gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);


				return BKT_OK;
			}
			else // skip
			{
				if(!LSEEK_CUR(gBKTDEC_CtrlAud.hbkt, sizeof(ahd) + shd.framesize))
				{
					TRACE1_BKTDEC("failed seek basket audio\n");
					BKTDEC_closeAud();
					return BKT_ERR;
				}

				// BK-1222
				gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);

                // AYK - 0201 - START
                frameCnt ++;

                if(frameCnt >= MAX_NUM_FRAME_SEARCH)
                {
                    TRACE0_BKTDEC("WARNING:Aud frames of ch %d not found in the vicinity\n",dp->ch);

					return BKT_NOAUDFR;
				}
				// AYK - 0201 - END
			}

			/* don't remove this... BK 0330
			char read_AudBuf[8*1024];
			//if(shd.ch == vp->ch)
			{
			if(!READ_PTSZ(gBKTDEC_CtrlAud.hbkt, read_AudBuf, shd.framesize + sizeof(ahd)))
			{
			TRACE1_BKTDEC("failed read audio data\n");
			return BKT_ERR;
			}

			 memcpy(&ahd, read_AudBuf, sizeof(ahd));
			 memcpy(vp->abuffer, read_AudBuf+sizeof(ahd), shd.framesize);

			  vp->ch		    = shd.ch;
			  vp->ts.sec      = shd.ts.sec;
			  vp->ts.usec     = shd.ts.usec;
			  vp->streamtype  = ST_AUDIO;
			  vp->framesize   = shd.framesize;
			  vp->samplingrate= ahd.samplingrate;
			  vp->fpos = 0;

			   gBKTDEC_CtrlAud.sec = shd.ts.sec; // AYK - 1215

				// BK-1222
				gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);


				  return BKT_OK;
				  }
				  else // skip
				  {
				  // 				if(!LSEEK_CUR(gBKTDEC_CtrlAud.hbkt, sizeof(ahd) + shd.framesize))
				  // 				{
				  // 					TRACE1_BKTDEC("failed seek basket audio\n");
				  // 					BKTDEC_closeAud();
				  // 					return BKT_ERR;
				  // 				}
				  //
				  // 				// BK-1222
				  // 				gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);
				  }
			*/

		}
		else if(shd.id == ID_VIDEO_HEADER)
		{
			// skip fpos to next frame
			if(!LSEEK_CUR(gBKTDEC_CtrlAud.hbkt, sizeof(vhd) + shd.framesize))
			{
				TRACE1_BKTDEC("failed seek basket audio\n");
				BKTDEC_closeAud();
				return BKT_ERR;
			}

			// BK-1222
			gBKTDEC_CtrlAud.fpos_bkt = LTELL(gBKTDEC_CtrlAud.hbkt);

			if(shd.frametype == FT_IFRAME )
			{
				gBKTDEC_CtrlAud.iframe_cur += 1;

				if( BKT_isRecordingBasket(gBKTDEC_Ctrl.bid))
				{
					gBKTDEC_CtrlAud.iframe_cnt = BKT_GetRecordingIndexCount();
				}
			}

            // AYK - 0118 - START
            frameCnt ++;

            if(frameCnt >= MAX_NUM_FRAME_SEARCH)
            {
                TRACE0_BKTDEC("WARNING:Aud frames of ch %d not found in the vicinity\n",dp->ch);

				return BKT_NOAUDFR;
			}
			// AYK - 0118 - END
		}
		else
		{
			TRACE1_BKTDEC("incorrect stream header. SHID:0x%08lx, CH:%ld, framesize:%ld\n", shd.id, shd.ch, shd.framesize);

			return BKT_ERR; // BK - 100115
			//break;
		}

	}// while


	if(gBKTDEC_status == BKTDEC_STATUS_OPEN)
	{
		long bid = BKTDEC_SearchNextBIDAud(gBKTDEC_CtrlAud.ch, gBKTDEC_CtrlAud.bid, gBKTDEC_CtrlAud.sec);

		if( bid == BKT_ERR )
			return BKT_NONEXTBID;

		if( BKT_ERR == BKTDEC_OpenNextAud(gBKTDEC_CtrlAud.ch, bid))
			return BKT_ERR;

		return BKTDEC_ReadNextFrameAud(dp);
	}

	return BKT_ERR;
}

// AYK - 1215
long BKTDEC_SyncAudioWithVideo(T_BKTDEC_PARAM *dp)
{
#ifdef PREV_FPOS
    int ret;
    int audVidDiff;
    int searchDirn     = 0;
    int prevSearchDirn = 0;
    int gotoNextBkt    = 0;
    int noPrevFrSearch = 0;
    int frameCnt       = 0;
    long curFpos;
    long curAudFpos = 0;
    long prevAudFpos;
    int  firstAudFrame = 1;

    T_STREAM_HDR shd;
    T_VIDEO_STREAM_HDR vhd;
    T_AUDIO_STREAM_HDR ahd;
#endif

    // check if audio and video decoding from the same basket
    if(dp->bktId != gBKTDEC_CtrlAud.bid)
    {
        // open the new basket for audio
        if(BKT_ERR == BKTDEC_OpenNextAud(gBKTDEC_CtrlAud.ch,dp->bktId))
        {
            TRACE1_BKTDEC("failed sync audio with video. BID:%ld, ABID:%ld\n",dp->bktId,gBKTDEC_CtrlAud.bid);
            return BKT_ERR;
        }
    }

    // Seek audio to video posn
    if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,dp->fpos_bkt))
    {
        TRACE1_BKTDEC("failed seek basket audio. vp->fpos_bkt:%ld\n", dp->fpos_bkt);
        BKTDEC_closeAud();
        return BKT_ERR;
    }

    TRACE0_BKTDEC("seeking audio to video. video-bid:%ld, video-fpos:%ld\n",dp->bktId,dp->fpos_bkt);

#ifdef PREV_FPOS // AYK - 0113

nextFrame:

    // seek to the next audio frame
    ret = READ_STRC(gBKTDEC_CtrlAud.hbkt,shd);

#ifdef BKT_SYSIO_CALL
    if(ret == 0)
    {
        gotoNextBkt = 1;
        TRACE1_BKTDEC("failed read stream header. EOF\n");
    }
#else
    if(0 != feof(gBKTDEC_CtrlAud.hbkt))
    {
        gotoNextBkt = 1;
        TRACE1_BKTDEC("failed read stream header. EOF.\n");
    }
#endif

    if(gotoNextBkt == 1)
    {
        // Open next basket
        long bid = BKTDEC_SearchNextBIDAud(gBKTDEC_CtrlAud.ch,gBKTDEC_CtrlAud.bid,gBKTDEC_CtrlAud.sec);

        if( bid == BKT_ERR )
            return BKT_ERR;

        if( BKT_ERR == BKTDEC_OpenNextAud(gBKTDEC_CtrlAud.ch, bid))
            return BKT_ERR;

        gotoNextBkt = 0;

        goto nextFrame;
    }

    // AYK - 0119
    curFpos  = LTELL(gBKTDEC_CtrlAud.hbkt);

    if((shd.id != ID_AUDIO_HEADER) && (shd.id != ID_VIDEO_HEADER))
    {
        TRACE1_BKTDEC("Invalid stream header\n");
        BKTDEC_closeAud();
        return BKT_ERR;
    }

    if(shd.id == ID_AUDIO_HEADER)
    {
        if(shd.ch == dp->ch)
        {
            // AYK - 0118
            frameCnt = 0;
            gBKTDEC_CtrlAud.sec = shd.ts.sec;
            prevAudFpos = curAudFpos;
            curAudFpos  = curFpos;
        }
        else
        {
nextAudFrame:
            // AYK - 0118
            frameCnt ++;

            if(frameCnt >= MAX_NUM_FRAME_SEARCH)
            {
                TRACE0_BKTDEC("WARNING:Aud frames of ch %d not found in the vicinity\n",dp->ch);
                if(firstAudFrame == 0)
                {
                    if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,curAudFpos - sizeof(shd)))
                    {
                        TRACE1_BKTDEC("failed seek basket audio\n");
                        BKTDEC_closeAud();
	                    return BKT_ERR;
                    }
                }
                else
                {
                    LSEEK_CUR(gBKTDEC_CtrlAud.hbkt,-sizeof(shd));
                }

                return BKT_NOAUDFR;
            }

            if(searchDirn == 0) // Forward
            {
                // audio frame of other channel so goto next frame
                if(!LSEEK_CUR(gBKTDEC_CtrlAud.hbkt,sizeof(ahd) + shd.framesize))
                {
                    TRACE1_BKTDEC("failed seek basket audio\n");
                    BKTDEC_closeAud();
                    return BKT_ERR;
                }

                goto nextFrame;
            }
            else // backward
            {
                if(shd.prevFpos > 0)
                {
                    if(curFpos > shd.prevFpos)
                    {
                        if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,shd.prevFpos))
                        {
                            TRACE1_BKTDEC("failed seek basket audio\n");
                            BKTDEC_closeAud();
                            return BKT_ERR;
                        }
                    }
                    else
                    {
                        TRACE0_BKTDEC("Beginning of basket reached,opening previous basket\n");

                        long bid = BKTDEC_SearchPrevBIDAud(gBKTDEC_CtrlAud.ch,gBKTDEC_CtrlAud.bid,gBKTDEC_CtrlAud.sec);

                        if( bid == BKT_ERR )
                            return BKT_ERR;

                        if(BKT_ERR == BKTDEC_OpenPrevAud(gBKTDEC_CtrlAud.ch,bid))
                            return BKT_ERR;

                        if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,shd.prevFpos))
                        {
                            TRACE1_BKTDEC("failed seek basket audio\n");
                            BKTDEC_closeAud();
                            return BKT_ERR;
                        }
                    }

                    goto nextFrame;
                }
                else if(shd.prevFpos == 0)
                     {
                         // goto previous basket
                         TRACE0_BKTDEC("Beginning of basket reached\n");
                         noPrevFrSearch = 1;
                     }
            }
        }
    }
    else  // video frame
    {
        // AYK - 0118
        frameCnt ++;

        if(frameCnt >= MAX_NUM_FRAME_SEARCH)
        {
            TRACE0_BKTDEC("WARNING:Aud frames of ch %d not found in the vicinity\n",dp->ch);
            if(firstAudFrame == 0)
            {
                if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,curAudFpos - sizeof(shd)))
                {
                    TRACE1_BKTDEC("failed seek basket audio\n");
                    BKTDEC_closeAud();
                    return BKT_ERR;
                }
            }
            else
            {
                LSEEK_CUR(gBKTDEC_CtrlAud.hbkt,-sizeof(shd));
            }

            return BKT_NOAUDFR;
        }

        if(searchDirn == 0) // Forward
        {
            // video frame so goto next frame
            if(!LSEEK_CUR(gBKTDEC_CtrlAud.hbkt,sizeof(vhd) + shd.framesize))
            {
                TRACE1_BKTDEC("failed seek basket audio\n");
                BKTDEC_closeAud();
                return BKT_ERR;
            }

            goto nextFrame;
        }
        else  // backward
        {
            if(shd.prevFpos > 0)
            {
                if(curFpos > shd.prevFpos)
                {
                    if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,shd.prevFpos))
                    {
                        TRACE1_BKTDEC("failed seek basket audio\n");
                        BKTDEC_closeAud();
                        return BKT_ERR;
                    }
                }
                else
                {
                    TRACE0_BKTDEC("Beginning of basket reached,opening previous basket\n");

                    long bid = BKTDEC_SearchPrevBIDAud(gBKTDEC_CtrlAud.ch,gBKTDEC_CtrlAud.bid,gBKTDEC_CtrlAud.sec);

                    if( bid == BKT_ERR )
                        return BKT_ERR;

                    if(BKT_ERR == BKTDEC_OpenPrevAud(gBKTDEC_CtrlAud.ch,bid))
                        return BKT_ERR;

                    if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,shd.prevFpos))
                    {
                        TRACE1_BKTDEC("failed seek basket audio\n");
                        BKTDEC_closeAud();
                        return BKT_ERR;
                    }
                }

                goto nextFrame;
            }
            else if(shd.prevFpos == 0)
                 {
                     // goto previous basket
                     TRACE0_BKTDEC("Beginning of basket reached\n");
                     noPrevFrSearch = 1;
                 }
        }
    }

    TRACE0_BKTDEC("cur aud frame time stamp:%d sec,%d msec\n",shd.ts.sec,shd.ts.usec/1000);

    // We are at an audio frame
    audVidDiff = (dp->ts.sec - shd.ts.sec) * 1000 + (dp->ts.usec - shd.ts.usec)/1000;

    if(audVidDiff < -128)
    {
        searchDirn = 1; // backward

        if(firstAudFrame == 1)
        {
            firstAudFrame  = 0;
            prevSearchDirn = searchDirn;

            if(noPrevFrSearch == 0)
            {
                goto nextAudFrame;
            }
        }
        else
        {
            if(searchDirn != prevSearchDirn)
            {
                TRACE0_BKTDEC("cannot find aud frame close to vid frame(%d sec,%d msec)\n",dp->ts.sec,dp->ts.usec/1000);
                if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,prevAudFpos))
                {
                    TRACE1_BKTDEC("failed seek basket audio\n");
                    BKTDEC_closeAud();
                    return BKT_ERR;
                }
            }
            else
            {
                prevSearchDirn = searchDirn;

                if(noPrevFrSearch == 0)
                {
                    goto nextAudFrame;
                }
            }
        }
    }
    else if(audVidDiff > 128)
         {
             searchDirn = 0; // forward

             if(firstAudFrame == 1)
             {
                 firstAudFrame = 0;
                 prevSearchDirn = searchDirn;
                 goto nextAudFrame;
             }
             else
             {
                 if(searchDirn != prevSearchDirn)
                 {
                     TRACE0_BKTDEC("cannot find aud frame close to vid frame(%d sec,%d msec)\n",dp->ts.sec,dp->ts.usec/1000);
                     if(!LSEEK_SET(gBKTDEC_CtrlAud.hbkt,prevAudFpos))
	                 {
		                 TRACE1_BKTDEC("failed seek basket audio\n");
		                 BKTDEC_closeAud();
		                 return BKT_ERR;
	                 }
                 }
                 else
                 {
                     prevSearchDirn = searchDirn;
                     goto nextAudFrame;
                 }
             }
         }

    LSEEK_CUR(gBKTDEC_CtrlAud.hbkt,-sizeof(shd));

    TRACE0_BKTDEC("Found the aud frame with time stamp close to vid time stamp\n");
    TRACE0_BKTDEC("Vid time stamp = %d sec,%d msec\n",dp->ts.sec,dp->ts.usec/1000);
    TRACE0_BKTDEC("Aud time stamp = %d sec,%d msec\n",shd.ts.sec,shd.ts.usec/1000);

#endif

    return BKT_OK;
}


#endif //SEP_AUD_TASK

// AYK - 0105
long BKTDEC_SeekBidFpos(long bid,long fpos)
{
    // check if the given bid is same as current bid
    if(bid != gBKTDEC_Ctrl.bid)
    {
        // open the basket with id = bid
        if(BKT_ERR == BKTDEC_OpenNext(0, bid))
        {
            return BKT_ERR;
        }
    }

   // Seek to the fpos
    LSEEK_SET(gBKTDEC_Ctrl.hbkt,fpos);

    return BKT_OK;
}

long BKTDEC_GetCurPlayTime()
{
	return gBKTDEC_Ctrl.sec ;
}

//////////////////////////////////////////////////////////////////////////
// EOF basket_dec.c
//////////////////////////////////////////////////////////////////////////
