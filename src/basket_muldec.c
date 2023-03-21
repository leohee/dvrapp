/**
 @file basket_muldec.c
 @brief
*/
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#include "basket_muldec.h"
#include "udbasket.h"

//////////////////////////////////////////////////////////////////////////
//#define BKTMD_DEBUG

#ifdef BKTMD_DEBUG
#define TRACE0_BKTMD(msg, args...) printf("[BKTMD] - " msg, ##args)
#define TRACE1_BKTMD(msg, args...) printf("[BKTMD] - %s(%d):" msg, __FUNCTION__, __LINE__, ##args)
#define TRACE2_BKTMD(msg, args...) printf("[BKTMD] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE0_BKTMD(msg, args...) ((void)0)
#define TRACE1_BKTMD(msg, args...) ((void)0)
#define TRACE2_BKTMD(msg, args...) ((void)0)
#endif

#define BKTMD_ERR(msg, args...) printf("[BKTMD] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
T_BKTMD_CTRL gBKTMD_CtrlV; ///< multiple video decode control
T_BKTMD_CTRL gBKTMD_CtrlA; ///< multiple audio decode control
//////////////////////////////////////////////////////////////////////////

////MULTI-HDD SKSUNG////
BKT_Mount_Info gbkt_mount_info;
////////////////////////


void BKTMD_close(int bStop)
{
	TRACE0_BKTMD("close basket decoding. CH:%d, BID:%ld\n", gBKTMD_CtrlV.ch, gBKTMD_CtrlV.bid);

	if(bStop)
	{
		gBKTMD_CtrlV.bkt_status = BKTMD_STATUS_STOP;
	}

	SAFE_CLOSE_BKT(gBKTMD_CtrlV.hbkt);
	SAFE_CLOSE_BKT(gBKTMD_CtrlV.hidx);

	gBKTMD_CtrlV.ch  = 0;
	gBKTMD_CtrlV.bid = 0; // caution not zero-base

	// BK-1222
	gBKTMD_CtrlV.fpos_bkt =0;

	gBKTMD_CtrlV.iframe_cnt = 0;
	gBKTMD_CtrlV.iframe_cur = 0;
}

long BKTMD_Open(long sec, T_BKTMD_OPEN_PARAM* pOP)
{
	BKTMD_close(TRUE);

	TRACE1_BKTMD("Enter %ld\n", sec);

	int i, ch;
	T_BKT_TM tm1;

	BKT_FHANDLE fd; // file descript

	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdb;
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	BKT_GetLocalTime(sec, &tm1);

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);
	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	// read events
	if(!READ_PTSZ(fd, evts, sizeof(evts)))
 	{
 		BKTMD_ERR("failed read evt data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	// read record database
	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
 	{
 		BKTMD_ERR("failed read rdb data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	// done read rec information from rdb file
	CLOSE_BKT(fd);

	int bFound=0;
	rdb.bid = 0;
	for(i = tm1.hour*60+tm1.min; i < TOTAL_MINUTES_DAY ; i++)
	{
		for( ch = 0 ; ch < BKT_MAX_CHANNEL ; ch++)
		{
			if(evts[TOTAL_MINUTES_DAY*ch+i] != 0 && rdbs[TOTAL_MINUTES_DAY*ch+i].bid != 0)
			{
				if(bFound==0)
					bFound = 1;

				if(rdb.bid==0)
				{
					rdb.bid     = rdbs[TOTAL_MINUTES_DAY*ch+i].bid;
					rdb.idx_pos = rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos;
				}
				else
				{
					// rarely appears
					if(rdb.bid != rdbs[TOTAL_MINUTES_DAY*ch+i].bid)
						break;
					else
					{
						rdb.idx_pos = min(rdb.idx_pos, rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos);
					}
				}

				pOP->recChannelCount++;
				pOP->recChannels[ch]=1;

				TRACE0_BKTMD("found start rec time. [%02d:%02d], CH:%02d, BID:%ld, idx_fpos:%ld\n",
				              i/60, i%60, ch, rdbs[TOTAL_MINUTES_DAY*ch+i].bid, rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos);
			}
		}

		// found!!!!!
		if(bFound) break;
	}

	if(bFound == 0)
	{
		BKTMD_ERR("failed find start record time. path:%s, ts:%04d/%02d/%02d %02d:%02d:%02d\n", path, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	// open the basket file using above found ID
	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlV.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hbkt, path))
	{
		BKTMD_ERR("failed open basket file for video decoding -- path:%s\n", path);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	// open the index file using above found ID
	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlV.target_path, rdb.bid);
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hidx, path))
	{
		BKTMD_ERR("failed open index file for decoding -- path:%s\n", path);
		CLOSE_BKT(gBKTMD_CtrlV.hbkt);
		return BKT_ERR;
	}

	// read index header
	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTMD_CtrlV.hidx, ihd))
	{
		BKTMD_ERR("failed read index hdr\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	// seek index file pointer using above found index file point
	if(!LSEEK_SET(gBKTMD_CtrlV.hidx, rdb.idx_pos))
	{
		BKTMD_ERR("failed seek index:%ld.\n", rdb.idx_pos);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	// search being index file point.
	T_INDEX_DATA idd;
	long idx_pos=rdb.idx_pos;

	if(!READ_ST(gBKTMD_CtrlV.hidx, idd))
	{
		BKTMD_ERR("failed read index data. idx_pos:%ld\n", idx_pos);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}
	idx_pos = LTELL(gBKTMD_CtrlV.hidx) - sizeof(idd);

	gBKTMD_CtrlV.iframe_cnt = BKT_isRecordingBasket(rdb.bid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTMD_CtrlV.iframe_cur = BKT_transIndexPosToNum(idx_pos);

	if(!LSEEK_SET(gBKTMD_CtrlV.hbkt, idd.fpos))
	{
		BKTMD_ERR("failed seek basket:%ld.\n", idd.fpos);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	int first_ch=0, first_width=0, first_height=0, withAudio=0, capMode=0;
	if(BKT_ERR == BKTMD_GetRelsolutionFromVHDR(&first_ch, &first_width, &first_height, &withAudio, &capMode))
	{
		BKTMD_ERR("failed read resolution.\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}
	pOP->firstCh     = first_ch;
	pOP->frameWidth  = first_width;
	pOP->frameHeight = first_height;
	pOP->withAudio   = withAudio;
	pOP->capMode     = capMode;

	if(!LSEEK_SET(gBKTMD_CtrlV.hidx, idx_pos))
	{
		BKTMD_ERR("failed seek index:%ld.\n", idx_pos);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	gBKTMD_CtrlV.bid = rdb.bid;
	gBKTMD_CtrlV.sec = idd.ts.sec;
	gBKTMD_CtrlV.fpos_bkt =idd.fpos; // BK-1222

	pOP->openbid = rdb.bid;
	pOP->openfpos = idd.fpos;
	pOP->open_ts.sec = idd.ts.sec;
	pOP->open_ts.usec = idd.ts.usec;

	//////////////////////////////////////////////////////////////////////////
	// set open because audio thread
	gBKTMD_CtrlV.bkt_status = BKTMD_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////

	TRACE0_BKTMD("open basket decoding -- CH:%ld, w:%d, h:%d, BID:%ld, B-POS:%ld, I-POS:%ld, curiframe:%d, totIFrame:%d \n",
					idd.ch, idd.width, idd.height, rdb.bid, idd.fpos, idx_pos ,gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);

	return BKT_OK;
}

long BKTMD_OpenNext(long nextbid)
{
	BKTMD_close(FALSE);

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlV.target_path, nextbid);
	//TRACE1_BKTMD("path = %s nextbid = %d\n",path, nextbid) ;

	// open basket
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hbkt, path))
	{
		BKTMD_ERR("failed open basket file -- path:%s\n", path);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTMD_CtrlV.hbkt, bhdr))
	{
		BKTMD_ERR("failed read basket header.\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlV.target_path, nextbid);
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hidx, path))
	{
		BKTMD_ERR("failed open index file -- path:%s\n", path);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	// read index hdr
	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTMD_CtrlV.hidx, ihd))
	{
		BKTMD_ERR("failed read index hdr\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	gBKTMD_CtrlV.bid = nextbid;
	gBKTMD_CtrlV.iframe_cnt = BKT_isRecordingBasket(nextbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTMD_CtrlV.sec = BKT_GetMinTS(bhdr.ts.t1);
	if(gBKTMD_CtrlV.sec == 0)
		gBKTMD_CtrlV.sec = BKT_GetMinTS(ihd.ts.t1);

	// BK - 1223 - Start
	gBKTMD_CtrlV.iframe_cur = 0;
	gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

	//////////////////////////////////////////////////////////////////////////
	// set open status because audio thread
	gBKTMD_CtrlV.bkt_status = BKTMD_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////
	// BK - 1223 - End

	TRACE0_BKTMD("open next basket mul. BID:%ld, iframe-current:%d, iframe_total:%d, last_sec:%ld\n", nextbid, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt, gBKTMD_CtrlV.sec);

	return BKT_OK;
}

long BKTMD_OpenPrev(long prevbid)
{
	BKTMD_close(FALSE);

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlV.target_path, prevbid);

	// open basket
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hbkt, path))
	{
		BKTMD_ERR("failed open basket file -- path:%s\n", path);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTMD_CtrlV.hbkt, bhdr))
	{
		BKTMD_ERR("failed read basket vdo header.\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlV.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTMD_CtrlV.hidx, path))
	{
		BKTMD_ERR("failed open index file -- path:%s\n", path);
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTMD_CtrlV.hidx, ihd))
	{
		BKTMD_ERR("failed read index hdr\n");
		BKTMD_close(TRUE);
		return BKT_ERR;
	}

	if(ihd.count != 0)
	{
		if(!LSEEK_CUR(gBKTMD_CtrlV.hidx, sizeof(T_INDEX_DATA)*(ihd.count-1)))
		{
			BKTMD_ERR("failed seek idx. ihd.count=%ld\n", ihd.count);
			BKTMD_close(TRUE);
			return BKT_ERR;
		}

		T_INDEX_DATA idd;
		if(!READ_ST(gBKTMD_CtrlV.hidx, idd))
		{
			BKTMD_ERR("failed read index data\n");
			BKTMD_close(TRUE);
			return BKT_ERR;
		}

		if(!LSEEK_SET(gBKTMD_CtrlV.hbkt, idd.fpos))
		{
			BKTMD_ERR("failed seek basket.\n");
			BKTMD_close(TRUE);
			return BKT_ERR;
		}
	}
	else
	{
		long prev_sec = 0;
		long prev_usec = 0;
		int index_count=0, bkt_fpos=0;
		T_INDEX_DATA idd;
		while(READ_ST(gBKTMD_CtrlV.hidx, idd))
		{
			if( (idd.ts.sec <= prev_sec && idd.ts.usec <= prev_usec)
				|| idd.id != ID_INDEX_DATA
				|| idd.fpos <= bkt_fpos)
			{
				TRACE1_BKTMD("Fount last record index pointer. sec=%ld, usec=%ld, count=%d\n", idd.ts.sec, idd.ts.usec, index_count);
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
			if(!LSEEK_SET(gBKTMD_CtrlV.hbkt, bkt_fpos))
			{
				BKTMD_ERR("failed seek basket.\n");
				BKTMD_close(TRUE);
				return BKT_ERR;
			}

			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, sizeof(idd)*index_count))
			{
				BKTMD_ERR("failed seek index.\n");
				BKTMD_close(TRUE);
				return BKT_ERR;
			}
		}
		else
		{
			if(!LSEEK_END(gBKTMD_CtrlV.hbkt, 0))
			{
				BKTMD_ERR("failed seek basket.\n");
				BKTMD_close(TRUE);
				return BKT_ERR;
			}

			int offset = sizeof(T_INDEX_DATA);
			if(!LSEEK_END(gBKTMD_CtrlV.hidx, -offset))
			{
				BKTMD_ERR("failed seek idx.\n");
				BKTMD_close(TRUE);
				return BKT_ERR;
			}
		}

	}

	gBKTMD_CtrlV.bid = prevbid;
	gBKTMD_CtrlV.iframe_cnt = BKT_isRecordingBasket(prevbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTMD_CtrlV.sec = BKT_GetMaxTS(bhdr.ts.t2);
	if(gBKTMD_CtrlV.sec == 0)
		gBKTMD_CtrlV.sec = BKT_GetMaxTS(ihd.ts.t2);

	gBKTMD_CtrlV.iframe_cur = gBKTMD_CtrlV.iframe_cnt - 1;

	// BK - 1223
	gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

	TRACE0_BKTMD("open prev basket multi. BID:%ld, iframe-current:%d, iframe_total:%d\n", prevbid, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);

	return BKT_OK;
}

// search functions

long BKTMD_SearchPrevHddStartBID(char* path, long sec)
{
	long sec_seed = sec;

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[30];

	struct dirent **ents;
	int nitems, i;

	sprintf(buf, "%s/rdb/", path);
	nitems = scandir(buf, &ents, NULL, alphasort_reverse);

	if(nitems < 0) {
	    perror("scandir search next hdd start");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
		free(ents); // BKKIM 20111101
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int prev_rdb=0;
	int m, ch;
	
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
		TRACE1_BKTMD("prev_rdb=%d,  curr_rdb=%d=======\n", prev_rdb, curr_rdb);

		if(prev_rdb <= curr_rdb)
		{
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_prev_hdd_start_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_prev_hdd_start_bid;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_prev_hdd_start_bid;
			}
			CLOSE_BKT(fd);

			TRACE1_BKTMD("min = [%04d]=======\n", tm1.hour*60+tm1.min);
			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0)
					{
						TRACE0_BKTMD("found prev HDD BID:%ld, first rec channel:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, ch);
					    for(;i<nitems;i++) {
							free(ents[i]);
						}
						free(ents);

						return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
					}
				}
			}

			// in case of First Fail
			tm1.hour = 23;
			tm1.min  = 60; // because index m is zero base.

		} //end if(prev_rdb >= curr_rdb)

		free(ents[i]);

	} // end for

lbl_err_prev_hdd_start_bid:
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);
	BKTMD_ERR("Can't find prev HDD basket. CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found

}
//////////MULTI-HDD SKSUNG//////////
long BKTMD_SearchNextHddStartBID(char* nextPath, long sec)
{
	long sec_seed = sec;

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[30];

	struct dirent **ents;
	int nitems, i;

	sprintf(buf, "%s/rdb/", nextPath);
	nitems = scandir(buf, &ents, NULL, alphasort);

	if(nitems < 0) {
	    perror("scandir search next hdd start");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
		free(ents); // BKKIM 20111101
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int curr_rdb=atoi(buf);
	int next_rdb=0;
	int m, ch;
	
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
		TRACE1_BKTMD("next_rdb=%d,  curr_rdb=%d=======\n", next_rdb, curr_rdb);

		if(next_rdb >= curr_rdb)
		{
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", nextPath, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_next_hdd_start_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_next_hdd_start_bid;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_next_hdd_start_bid;
			}
			CLOSE_BKT(fd);

			TRACE1_BKTMD("min = [%04d]=======\n", tm1.hour*60+tm1.min);
			for(m=tm1.hour*60+tm1.min;m<TOTAL_MINUTES_DAY;m++)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0)
					{
						TRACE0_BKTMD("found next HDD BID:%ld, first rec channel:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, ch);
					    for(;i<nitems;i++) {
							free(ents[i]);
						}
						free(ents);

						return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
					}
				}
			}

			// in case of First Fail
			tm1.hour = 0;
			tm1.min  = -1;

		} //end if(next_rdb >= curr_rdb)

		free(ents[i]);

	} // end for

lbl_err_next_hdd_start_bid:
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);
	BKTMD_ERR("Can't find next HDD basket. CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	return BKT_ERR;// not found

}
//////////END MULTI-HDD SKSUNG//////


long BKTMD_SearchNextBID(long curbid, long sec)
{
//	printf("BKT_DEC: curid = %ld in BKTMD_SearchNextBID func\n",curbid) ;
	if(BKT_isRecordingBasket(curbid))
	{
		BKTMD_ERR("There is no a next basekt. BID:%ld is recording.\n", curbid);
		return BKT_ERR;
	}

	long sec_seed = sec;
	//if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTMD_GetBasketTime(curbid, &bts, &ets))
		{
			BKTMD_ERR("There is no a next basekt. BID:%ld is recording.\n", curbid);
			return BKT_ERR;
		}

		if(sec < ets)
			sec_seed = ets;
		//TRACE0_BKTMD("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[30];
	sprintf(buf, "%s/rdb/", gBKTMD_CtrlV.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort);
    
	if(nitems < 0) {
	    perror("scandir - search next bid");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
		free(ents); // BKKIM 20111031
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
			int m, ch;
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTMD_CtrlV.target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_next_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min+1;m<TOTAL_MINUTES_DAY;m++)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0
					&& rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
					{
						TRACE0_BKTMD("found next BID:%ld, first rec channel:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, ch);

						for(;i<nitems;i++) {
							free(ents[i]);
						}
						free(ents);

						return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
					}
				}
			}

			// in case of First Fail
			tm1.hour = 0;
			tm1.min  = -1;

		} //end if(next_rdb >= curr_rdb)

		free(ents[i]);

    } // end for

lbl_err_search_next_bid:
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	BKTMD_ERR("Can't find next basket. CUR-BID:%ld, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n",
                  curbid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found
}

long BKTMD_SearchPrevBID(long curbid, long sec)
{
	long sec_seed = sec;

	//if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTMD_GetBasketTime(curbid, &bts, &ets))
		{
			BKTMD_ERR("There is no a previous basekt. BID:%ld.\n", curbid);
			return BKT_ERR;
		}

		if(bts < sec)
			sec_seed = bts;
		//TRACE0_BKTMD("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[30];
	sprintf(buf, "%s/rdb/", gBKTMD_CtrlV.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort_reverse);

	if(nitems < 0) {
	    perror("scandir reverse");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
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
			int m, ch;
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTMD_CtrlV.target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_prev_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0 
					&& rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
					{
						TRACE0_BKTMD("found prev BID:%ld, last rec channel:%d\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, ch);

						for(;i<nitems;i++) {
							free(ents[i]);
						}
						free(ents);

						return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
					}
				}
			}

			// in case of First fail...by reverse
			tm1.hour = 23;
			tm1.min  = 60;

		} // if(prev_rdb <= curr_rdb)

		free(ents[i]);

    } // end for

lbl_err_search_prev_bid:
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	BKTMD_ERR("Can't find prev basket. CUR-BID:%ld, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n",
	              curbid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found
}

int  BKTMD_hasRecData(int ch, long sec, int direction)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	BKT_FHANDLE fd;

	int i;
	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTMD_ERR("failed read rec data\n");
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

	BKTMD_ERR("There is no record data. %04d-%02d-%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	//TRACE1_BKTMD("failed search rec times. %04d-%02d-%02d %02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min);

	return BKT_ERR;
}

int  BKTMD_hasRecDataAll(long sec, int direction)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	int i, ch;
	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTMD_ERR("failed read rec data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	if(BKT_FORWARD == direction)
	{
		for(i=tm1.hour*60+tm1.min;i<TOTAL_MINUTES_DAY;i++)
		{
			for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
			{
				if(evts[ch*TOTAL_MINUTES_DAY+i] != 0)
				{
					return BKT_OK;
				}
			}
		}
	}
	else if(BKT_BACKWARD == direction)
	{
		for(i=tm1.hour*60+tm1.min; i>=0 ; i--)
		{
			for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
			{
				if(evts[ch*TOTAL_MINUTES_DAY+i] != 0)
				{
					return BKT_OK;
				}
			}
		}
	}
	else //(BKT_NOSEARCHDIRECTION== direction)
	{
		for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
		{
			if(evts[ch*TOTAL_MINUTES_DAY+tm1.hour*60+tm1.min] != 0)
			{
				return BKT_OK;
			}
		}
	}

	BKTMD_ERR("There is no record data. %04d-%02d-%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	//TRACE1_BKTMD("failed search rec times. %04d-%02d-%02d %02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min);

	return BKT_ERR;
}

long BKTMD_GetFirstRecTime()
{
	struct dirent *ent;
	DIR *dp;

	char buf[30];
	sprintf(buf, "%s/rdb", gBKTMD_CtrlV.target_path);
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
			int m, ch;
			sprintf(path, "%s/rdb/%08d", gBKTMD_CtrlV.target_path, min_rdb);

			if(!OPEN_RDONLY(fd, path))
			{
				BKTMD_ERR("failed open rdb %s\n", path);
				return BKT_ERR;
			}

			char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, evts, sizeof(evts)))
			{
				BKTMD_ERR("failed read rec evt\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
			CLOSE_BKT(fd);

			for(m=0;m<TOTAL_MINUTES_DAY;m++)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(evts[TOTAL_MINUTES_DAY*ch+m] != 0)
					{
						char buff[64];
						struct tm tm1;
						time_t t1;

						int yy = min_rdb/10000;
						int mm = (min_rdb%10000)/100;
						int dd = (min_rdb%10000)%100;

						int HH = m/60;
						int MM = m%60;

						sprintf(buff, "%04d-%02d-%02d %02d:%02d:00", yy, mm, dd, HH, MM);
						strptime(buff, "%Y-%m-%d %H:%M:%S", &tm1);

						t1 = mktime(&tm1);

						TRACE0_BKTMD("Get first record data. CH:%d, %04d-%02d-%02d %02d:%02d:00\n", ch, yy, mm, dd, HH, MM);

						return t1;
					}
				}
			}
		}
	}

	BKTMD_ERR("failed get first record time\n");
	return BKT_ERR;
}

long BKTMD_GetLastRecTime()
{
    struct dirent *ent;
    DIR *dp;

	char buf[30];
	sprintf(buf, "%s/rdb", gBKTMD_CtrlV.target_path);
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
			int m, ch;
			int bFoundFirst=FALSE;
#ifdef BKT_SYSIO_CALL
			int fd;
#else
			FILE *fd;
#endif
			char path[LEN_PATH];
			sprintf(path, "%s/rdb/%08d", gBKTMD_CtrlV.target_path, last_rdb);

			if(!OPEN_RDONLY(fd, path))
			{
				BKTMD_ERR("failed open rdb %s\n", path);
				return BKT_ERR;
			}

			char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

			if(!READ_PTSZ(fd, evts, sizeof(evts)))
			{
				BKTMD_ERR("failed read rec data\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
			CLOSE_BKT(fd);

			for(m=TOTAL_MINUTES_DAY-1;m>=0;m--)
			{
				for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
				{
					if(evts[TOTAL_MINUTES_DAY*ch+m] != 0)
					{
						if(bFoundFirst == TRUE)
						{
							char buff[64];
							struct tm tm1;
							time_t t1;
							int yy = last_rdb/10000;
							int mm = (last_rdb%10000)/100;
							int dd = (last_rdb%10000)%100;

							int HH = m/60;
							int MM = m%60;

							sprintf(buff, "%04d-%02d-%02d %02d:%02d:00", yy, mm, dd, HH, MM);
							strptime(buff, "%Y-%m-%d %H:%M:%S", &tm1);

							t1 = mktime(&tm1);

							TRACE0_BKTMD("get last -1 rec data. CH:%d, %04d-%02d-%02d %02d:%02d:00\n", ch, yy, mm, dd, HH, MM);

							return t1;
						}
						bFoundFirst = TRUE;

						TRACE0_BKTMD("found real last rec time. %02d:%02d\n", m/60, m%60);
					}
				}
			}
		}
    }

	BKTMD_ERR("failed get last record time\n");
	return BKT_ERR;
}

long BKTMD_ReadNextFrame(T_BKTMD_PARAM *dp, int decflags[])
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	while(gBKTMD_CtrlV.bkt_status == BKTMD_STATUS_OPEN)
	{
		// read stream header
		if(!READ_HDR(gBKTMD_CtrlV.hbkt, shd)) {
			perror("read stream header");
			TRACE1_BKTMD("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n", shd.id, shd.framesize, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
			break;
		}

		if(shd.id == ID_VIDEO_HEADER)
		{
			if(decflags[shd.ch] == 1) // check enable video channel
			{
				// maybe max interval frame. for broken stream or motion.
				if(shd.ts.sec - gBKTMD_CtrlV.sec > 4 || shd.ts.sec - gBKTMD_CtrlV.sec < 0) {
					TRACE0_BKTMD("time interval is too long. So search next frame. CH:%ld, shd.id:0x%08lx, frametype:%ld\n", shd.ch, shd.id, shd.frametype);
					break;
				}

				// read video header
				if(!READ_ST(gBKTMD_CtrlV.hbkt, vhd)) {
					perror("read video header");
					BKTMD_ERR("failed read video header\n");
					break;
				}
				// read video data
				if(!READ_PTSZ(gBKTMD_CtrlV.hbkt, dp->vbuffer, shd.framesize)) {
					perror("read video data");
					BKTMD_ERR("failed read video data\n");
					break;
				}

				dp->streamtype  = ST_VIDEO;
				dp->ch          = shd.ch;
				dp->framesize   = shd.framesize;
				dp->frametype   = shd.frametype;
				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->framerate   = vhd.framerate;
				dp->frameWidth  = vhd.width;
				dp->frameHeight = vhd.height;
				dp->event       = vhd.event;
				dp->audioONOFF  = vhd.audioONOFF; // BK - 100119
				dp->capMode     = vhd.capMode;

				if(strlen(vhd.camName))
					strncpy(dp->camName, vhd.camName, 16);

				gBKTMD_CtrlV.sec = shd.ts.sec;

				gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

				if(shd.frametype == FT_IFRAME )
				{
					gBKTMD_CtrlV.iframe_cur += 1;

					if( BKT_isRecordingBasket(gBKTMD_CtrlV.bid))
						gBKTMD_CtrlV.iframe_cnt = BKT_GetRecordingIndexCount();
				}

				dp->bktId      = gBKTMD_CtrlV.bid;
				dp->fpos_bkt   = gBKTMD_CtrlV.fpos_bkt;

				//TRACE0_BKTMD("Read frame, frameType:%d, iframe-num:%d, size:%ld\n", shd.frametype, gBKTMD_CtrlV.iframe_cur, shd.framesize);
				return BKT_OK;
			}
			else // if(dec_flag...
			{
				// skip
				if(!LSEEK_CUR(gBKTMD_CtrlV.hbkt, sizeof(vhd) + shd.framesize))
				{
					perror("lseek");
					BKTMD_ERR("failed seek cur basket.\n");
					break;
				}

				gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);
			}
		}
		else if( shd.id == ID_AUDIO_HEADER ) {
			// check enable audio channel
			if(dp->focusChannel != shd.ch) {
				// skip
				if(!LSEEK_CUR(gBKTMD_CtrlV.hbkt, sizeof(ahd) + shd.framesize)) {
					perror("lseek");
					TRACE1_BKTMD("failed seek basket.\n");
					break; // broken stream...
				}

				gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt); // save current file pointer
			}
			else
			{
				if(!READ_ST(gBKTMD_CtrlV.hbkt, ahd)) { // read audio header
					perror("read aud header");
					BKTMD_ERR("failed read audio header data\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTMD_CtrlV.hbkt, dp->vbuffer, shd.framesize)) { // read audio data
					perror("read aud data");
					BKTMD_ERR("failed read audio data\n");
					return BKT_ERR;
				}

				dp->ch		    = shd.ch;
				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->streamtype  = ST_AUDIO;
				dp->framesize   = shd.framesize;
				dp->samplingrate= ahd.samplingrate;

				gBKTMD_CtrlV.sec = shd.ts.sec; // AYK - 1215

				gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt); // save current file pointer			
				
				return BKT_OK;				
			}
		}
		else {// if(shd.id != ID_VIDEO_HEADER && shd.id != ID_AUDIO_HEADER)
			TRACE0_BKTMD("maybe appeared broken stream .Search next i-frame. shd.id:0x%08lx, framesize:%ld\n",
			shd.id, shd.framesize);
			break;
		}

	}// end while

	if(shd.id == ID_BASKET_END) // Normal sequence closed basket
	{

		return BKTMD_ReadNextFirstFrame(dp, decflags); // reading any FirstFrame on NextBasket
	}
	else
	{
		return BKTMD_ReadNextIFrame(dp, decflags); // Reading First Iframe on NextBasket
	}
}


long BKTMD_ReadNextFirstFrame(T_BKTMD_PARAM *dp, int decflags[])
{
	if(BKT_isRecordingBasket(gBKTMD_CtrlV.bid))
	{
		gBKTMD_CtrlV.iframe_cnt = BKT_GetRecordingIndexCount();
		TRACE1_BKTMD("get total i-frame count:%d\n", gBKTMD_CtrlV.iframe_cur);
	}

	if(gBKTMD_CtrlV.iframe_cur + 1 >= gBKTMD_CtrlV.iframe_cnt) // find next basket
	{
		long nextbid = BKTMD_SearchNextBID(gBKTMD_CtrlV.bid, gBKTMD_CtrlV.sec);
		printf("nextbid = %ld in BKTMD_ReadNextFirstFrame()\n", nextbid) ;

		////MULTI-HDD SKSUNG/////
		if( gbkt_mount_info.hddCnt == 1 && nextbid == BKT_ERR )
		{
			return BKT_ERR ;
		}

		if(gbkt_mount_info.hddCnt > 1)
		{
			if(gBKTMD_CtrlV.bktcnt == gBKTMD_CtrlV.bid)
			{
	    		int oldHddidx;
				
				nextbid 	= 0;
				oldHddidx 	= gBKTMD_CtrlV.curhdd;
				
				do{
		        	nextbid = BKTMD_setTargetDisk(gBKTMD_CtrlV.nexthdd, 0);
					if(nextbid != BKT_ERR)
						break;
				}while(gBKTMD_CtrlV.nexthdd == oldHddidx);
				
	            if(nextbid == BKT_ERR)
					return nextbid;

//				printf("##################BKTMD_ReadNextFirstFrame BKTMD_OpenNext(nextbid %02d)##################\n", nextbid);
//				return BKT_ERR;
			}
		}

		if( BKT_ERR == BKTMD_OpenNext(nextbid))
		{
			BKTMD_ERR("Error nextbid = %ld\n",nextbid) ;
            return BKT_ERR;
		}
	}
	else
	{
		gBKTMD_CtrlV.iframe_cur += 1;
	}


	if(!LSEEK_SET(gBKTMD_CtrlV.hidx, BKT_transIndexNumToPos(gBKTMD_CtrlV.iframe_cur)))
	{
		BKTMD_ERR("failed seek read next frame. iframe:%d\n", gBKTMD_CtrlV.iframe_cur);
		return BKT_ERR;
	}
//		printf("success seek read next iframe cur:%d\n", gBKTMD_CtrlV.iframe_cur);

	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;

	if(gBKTMD_CtrlV.bkt_status == BKTMD_STATUS_OPEN)
	{
		if(READ_ST(gBKTMD_CtrlV.hidx, idd))
		{
			if(READ_HDR(gBKTMD_CtrlV.hbkt, shd))
			{
				if(shd.id == ID_VIDEO_HEADER)
				{
					if(decflags[shd.ch]==1)
					{
						if(READ_HDR(gBKTMD_CtrlV.hbkt, vhd))
						{
							if(READ_PTSZ(gBKTMD_CtrlV.hbkt, dp->vbuffer, shd.framesize ))
							{
								dp->ch          = shd.ch;
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
								dp->audioONOFF  = vhd.audioONOFF; // BK - 100119
								dp->capMode     = vhd.capMode;

								if(strlen(vhd.camName))
									strncpy(dp->camName, vhd.camName, 16);

								gBKTMD_CtrlV.sec = shd.ts.sec;
								gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

//							printf("ReadNext First Frame bktID = %d, gBKTMD_CtrlV.fpos_bkt=%d\n",gBKTMD_CtrlV.bid,gBKTMD_CtrlV.fpos_bkt) ;
								dp->bktId    = gBKTMD_CtrlV.bid;
								dp->fpos_bkt = gBKTMD_CtrlV.fpos_bkt;

								return BKT_OK;
							}
							else
							{
								TRACE1_BKTMD("failed read video raw data.\n");
							}
						}
						else
						{
							TRACE1_BKTMD("failed read video stream header.\n");
						}
					}
					else
					{
						TRACE1_BKTMD("decflag[%ld] is off.\n", shd.ch);
					}
				}
				else
				{
					TRACE1_BKTMD("Invalid index file pointer.maybe broken iframe-index file.\n");
				}
			}
			else
			{
				TRACE1_BKTMD("failed read stream header.\n");
			}
		}
		else
		{
			TRACE1_BKTMD("failed read index data. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
//			printf("failed read index data. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
		}

	}// end if

	return BKTMD_ReadNextIFrame(dp, decflags);
}


long BKTMD_ReadNextIFrame(T_BKTMD_PARAM *dp, int decflags[])
{
	TRACE1_BKTMD("BKTMD_ReadNextIFrame\n") ;
	if(BKT_isRecordingBasket(gBKTMD_CtrlV.bid))
	{
		gBKTMD_CtrlV.iframe_cnt = BKT_GetRecordingIndexCount();
		//TRACE0_BKTMD("get total i-frame count:%d\n", gBKTMD_Ctrl.iframe_cur);
	}

	if(gBKTMD_CtrlV.iframe_cur + 1 >= gBKTMD_CtrlV.iframe_cnt) // find next basket
	{
//		printf("gBKTMD_CtrlV.iframe_cur + 1 = %d >= gBKTMD_CtrlV.iframe_cnt = %d\n",gBKTMD_CtrlV.iframe_cur + 1, gBKTMD_CtrlV.iframe_cnt) ;
		long nextbid = BKTMD_SearchNextBID(gBKTMD_CtrlV.bid, gBKTMD_CtrlV.sec);

		////MULTI-HDD SKSUNG/////
		if(gbkt_mount_info.hddCnt == 1 && nextbid == BKT_ERR )
		{
   	        return BKT_ERR ;
		}

		if(gbkt_mount_info.hddCnt > 1)
		{
	    	if(gBKTMD_CtrlV.bktcnt == gBKTMD_CtrlV.bid)
	    	{
	    		int oldHddidx;
				
				nextbid 	= 0;
				oldHddidx 	= gBKTMD_CtrlV.curhdd;
				
				do{
		        	nextbid = BKTMD_setTargetDisk(gBKTMD_CtrlV.nexthdd, 0);
					if(nextbid != BKT_ERR)
						break;
				}while(gBKTMD_CtrlV.nexthdd == oldHddidx);
				
	            if(nextbid == BKT_ERR)
					return nextbid;

//				printf("##################BKTMD_ReadNextIFrame BKTMD_OpenNext(nextbid %02d)##################\n", nextbid); 		
//				return BKT_ERR;				
			}
		}

        if( BKT_ERR == BKTMD_OpenNext(nextbid))
            return BKT_ERR;

		/////////////////////////
	}
	else
	{
		gBKTMD_CtrlV.iframe_cur += 1;
	}

	if(!LSEEK_SET(gBKTMD_CtrlV.hidx, BKT_transIndexNumToPos(gBKTMD_CtrlV.iframe_cur)))
	{
		BKTMD_ERR("failed seek read next frame. iframe:%d\n", gBKTMD_CtrlV.iframe_cur);
		return BKT_ERR;
	}
//		printf("success seek read next iframe cur:%d\n", gBKTMD_CtrlV.iframe_cur);

	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;

	if(gBKTMD_CtrlV.bkt_status == BKTMD_STATUS_OPEN)
	{
		if(READ_ST(gBKTMD_CtrlV.hidx, idd))
		{
			if(idd.fpos>=gBKTMD_CtrlV.fpos_bkt)
			{
//				printf("idd.fpos = %d >= gBKTMD_CtrlV.fpos_bkt = %d\n",idd.fpos, gBKTMD_CtrlV.fpos_bkt) ;
				if(LSEEK_SET(gBKTMD_CtrlV.hbkt, idd.fpos))
				{
					if(READ_HDR(gBKTMD_CtrlV.hbkt, shd))
					{
						if(shd.id == ID_VIDEO_HEADER)
						{
							if(decflags[shd.ch]==1)
							{
								if(READ_HDR(gBKTMD_CtrlV.hbkt, vhd))
								{
									if(READ_PTSZ(gBKTMD_CtrlV.hbkt, dp->vbuffer, shd.framesize ))
									{
										dp->ch          = shd.ch;
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
										dp->audioONOFF  = vhd.audioONOFF; // BK - 100119
										dp->capMode     = vhd.capMode;

										if(strlen(vhd.camName))
											strncpy(dp->camName, vhd.camName, 16);

										gBKTMD_CtrlV.sec = shd.ts.sec;
										gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

//							printf("ReadNext I Frame bktID = %d, gBKTMD_CtrlV.fpos_bkt=%d\n",gBKTMD_CtrlV.bid,gBKTMD_CtrlV.fpos_bkt) ;
										dp->bktId    = gBKTMD_CtrlV.bid;
										dp->fpos_bkt = gBKTMD_CtrlV.fpos_bkt;

										return BKT_OK;
									}
									else
									{
										TRACE1_BKTMD("failed read video raw data.\n");
									}
								}
								else
								{
									TRACE1_BKTMD("failed read video stream header.\n");
								}
							}
							else
							{
								TRACE0_BKTMD("decflag[%ld] is off.\n", shd.ch);
							}
						}
						else
						{
							TRACE1_BKTMD("Invalid index file pointer.maybe broken iframe-index file.\n");
						}
					}
					else
					{
						TRACE1_BKTMD("failed read stream header.\n");
					}
				}
				else
				{
					TRACE1_BKTMD("failed seek next index data. i-fpos:%ld\n", idd.fpos);
				}
			}
			else
			{
				TRACE1_BKTMD("invalid file pointer. idd.fpos=%ld, gBKTMD_CtrlV.fpos_bkt=%ld.\n", idd.fpos, gBKTMD_CtrlV.fpos_bkt);
			}
		}
		else
		{
			TRACE1_BKTMD("failed read index data. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
//			printf("failed read index data. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
		}

	}// end if

	return BKTMD_ReadNextIFrame(dp, decflags);

}

long BKTMD_ReadPrevIFrame(T_BKTMD_PARAM *dp, int decflags[])
{
	T_INDEX_DATA idd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	long prevbid=-1;

	while(gBKTMD_CtrlV.bkt_status == BKTMD_STATUS_OPEN)
	{
		// find prev basket
		if(gBKTMD_CtrlV.iframe_cur  - 1 < 0){
			prevbid = BKTMD_SearchPrevBID(gBKTMD_CtrlV.bid, gBKTMD_CtrlV.sec);

			if( prevbid == BKT_ERR )
				return BKT_ERR;

			// open previous basket with prevbid
			if( BKT_ERR == BKTMD_OpenPrev(prevbid))
				return BKT_ERR;
		}
		else {
			gBKTMD_CtrlV.iframe_cur -= 1;
		}

		if(!LSEEK_SET(gBKTMD_CtrlV.hidx, BKT_transIndexNumToPos(gBKTMD_CtrlV.iframe_cur)))
		{
			BKTMD_ERR("failed seek read prev frame. iframe:%d\n", gBKTMD_CtrlV.iframe_cur);
			return BKT_ERR;
		}

		// read index data with current iframe number
		if(!READ_ST(gBKTMD_CtrlV.hidx, idd)) {
			TRACE1_BKTMD("failed read index data. go prev iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, gBKTMD_CtrlV.iframe_cur, gBKTMD_CtrlV.iframe_cnt);
			continue;
		}
#if 0
		if(idd.fpos >= gBKTMD_CtrlV.fpos_bkt){
			TRACE1_BKTMD("invalid file pointer. idd.fpos=%ld, gBKTMD_CtrlV.fpos_bkt=%ld.\n", idd.fpos, gBKTMD_CtrlV.fpos_bkt);
		}
#endif

		if(!LSEEK_SET(gBKTMD_CtrlV.hbkt, idd.fpos)) {
			TRACE1_BKTMD("failed seek prev index data. i-fpos:%ld\n", idd.fpos);
			continue;
		}

		if(!READ_HDR(gBKTMD_CtrlV.hbkt, shd)) {
			TRACE1_BKTMD("failed read stream header.\n");
			continue;
		}

		if(shd.id != ID_VIDEO_HEADER) {
			TRACE1_BKTMD("Invalid index file pointer.maybe broken iframe-index file.\n");
			continue;
		}

		if(decflags[shd.ch]!=1) {
			TRACE0_BKTMD("decflag[%ld] is off.\n", shd.ch);
			continue;
		}

		if(!READ_HDR(gBKTMD_CtrlV.hbkt, vhd)) {
			TRACE1_BKTMD("failed read video stream header.\n");
			continue;
		}

		if(!READ_PTSZ(gBKTMD_CtrlV.hbkt, dp->vbuffer, shd.framesize)) {
			TRACE1_BKTMD("failed read video raw data.\n");
			continue;
		}

		dp->ch          = shd.ch;
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
		dp->audioONOFF  = vhd.audioONOFF; // BK - 100119
		dp->capMode     = vhd.capMode;

		if(strlen(vhd.camName))
			strncpy(dp->camName, vhd.camName, 16);

		gBKTMD_CtrlV.sec = shd.ts.sec;
		gBKTMD_CtrlV.fpos_bkt = LTELL(gBKTMD_CtrlV.hbkt);

		dp->bktId    = gBKTMD_CtrlV.bid;
		dp->fpos_bkt = gBKTMD_CtrlV.fpos_bkt;

		return BKT_OK;

	}// end while


	return BKT_ERR;
}


// AYK - 0105
long BKTMD_SeekBidFpos(long bid,long fpos)
{
    // check if the given bid is same as current bid
    if(bid != 0 && bid != gBKTMD_CtrlV.bid)
    {
        // open the basket with id = bid
        if(BKT_ERR == BKTMD_OpenNext(bid))
        {
			BKTMD_ERR("failed open basket skip fpos. BID:%ld, fpos:%ld\n", bid, fpos);
            return BKT_ERR;
        }
    }

	// Seek to the fpos
    if(fpos != 0 && LSEEK_SET(gBKTMD_CtrlV.hbkt, fpos))
	{
		return BKT_OK;
	}

	BKTMD_ERR("failed seek basket skip fpos. BID:%ld, fpos:%ld\n", bid, fpos);
	return BKT_ERR;
}

long BKTMD_GetBasketTime(long bid, long *o_begin_sec, long *o_end_sec)
{
	char path[30];
	T_BASKET_HDR bhdr;

    BKT_FHANDLE fd;

	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlV.target_path, bid);
	TRACE0_BKTMD("BKTMD_GetBasketTime path = %s\n", path);

	// open basket
	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	if(!READ_ST(fd, bhdr))
	{
		BKTMD_ERR("failed read basket header.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	*o_begin_sec = BKT_GetMinTS(bhdr.ts.t1);
	*o_end_sec   = BKT_GetMaxTS(bhdr.ts.t2);

	TRACE0_BKTMD("Get Basket Time. BID:%ld, B_SEC:%ld, E_SEC:%ld, Duration:%ld\n", bid, *o_begin_sec,  *o_end_sec, *o_end_sec - *o_begin_sec);

	return BKT_OK;
}


long BKTMD_setTargetDisk(int hddid, int bid)
{
#ifdef BKT_SYSIO_CALL
    int fd_bkt ;
#else
    FILE *fd_bkt ;
#endif
	
	T_BKTMGR_HDR  hd;

	char diskpath[30] ;
	long firstBid = 0;

	//////MULTI-HDD SKSUNG/////
	gBKTMD_CtrlV.curhdd 	= hddid;
	gBKTMD_CtrlV.nexthdd 	= gbkt_mount_info.hddInfo[hddid].nextHddIdx;
	

	firstBid = BKTMD_SearchNextHddStartBID(gbkt_mount_info.hddInfo[hddid].mountDiskPath, gBKTMD_CtrlV.sec);
	if(firstBid == BKT_ERR)
		return firstBid;	//not founed playback bid.
	
	sprintf(diskpath, "%s/%s", gbkt_mount_info.hddInfo[hddid].mountDiskPath, NAME_BKTMGR) ;

    if(!OPEN_RDWR(fd_bkt, diskpath)) // open bktmgr.udf file
    {
        BKTMD_ERR("failed open basket manager file in decoding. %s\n", diskpath);
        return BKT_ERR;
    }

    if(!READ_ST(fd_bkt, hd))
    {
        BKTMD_ERR("failed read basket manager header in decoding.\n");
        CLOSE_BKT(fd_bkt);
        return BKT_ERR;
    }
	gBKTMD_CtrlV.bktcnt = hd.bkt_count ;		

	sprintf(gBKTMD_CtrlV.target_path, "%s", gbkt_mount_info.hddInfo[hddid].mountDiskPath );
	sprintf(gBKTMD_CtrlA.target_path, "%s", gbkt_mount_info.hddInfo[hddid].mountDiskPath );

	CLOSE_BKT(fd_bkt);

	return firstBid;	//found firt  playback bid
	///////////////////////////

//	gBKTMD_CtrlV.bid = bid ;
}

//////MULTI-HDD SKSUNG///////
int BKTMD_getTargetDisk(long sec)
{

#ifdef BKT_SYSIO_CALL
    int fd, fd_bkt;
#else
    FILE *fd, *fd_bkt;
#endif

    int i = 0 , j = 0, k = 0, searchmin = 0, hddIdx = -1;
    long offset = 0;
    char inRecHourTBL[TOTAL_MINUTES_DAY] ;

	T_BKT_TM tm1;
	T_BKTMGR_HDR  hd;

    BKT_GetLocalTime(sec, &tm1);
	char diskpath[30];	
	char fullpath[30] ;	

	searchmin = tm1.hour*60 + tm1.min ;

	for(j = 0; j < MAX_HDD_COUNT; j++)
    {
    	if(gbkt_mount_info.hddInfo[j].isMounted)
   		{
            sprintf(diskpath, "%s", gbkt_mount_info.hddInfo[j].mountDiskPath) ;
			
	        sprintf(fullpath, "%s/rdb/%04d%02d%02d", diskpath, tm1.year, tm1.mon, tm1.day);

	        if(!OPEN_RDONLY(fd, fullpath))
	        {
	            continue;
	        }

			for(k = 0; k < MAX_DVR_CHANNELS; k++)
			{
				offset = TOTAL_MINUTES_DAY*k;
				
				if(!LSEEK_SET(fd, offset))
				{
					CLOSE_BKT(fd);
					continue;
				}
			
				if(!READ_PTSZ(fd, inRecHourTBL, TOTAL_MINUTES_DAY))
				{
					CLOSE_BKT(fd);
					continue;
				}
			
				for(i = 0; i < TOTAL_MINUTES_DAY; i++)
				{
					if(searchmin == i) 
					{
						if(0 != (int)inRecHourTBL[i])
						{
							hddIdx = j ;
							CLOSE_BKT(fd);
							break ;
						}
					}
				}
			}

			memset(diskpath, 0, 30) ;

			if(hddIdx >= 0) 
			{
				sprintf(diskpath, "%s/%s", gbkt_mount_info.hddInfo[hddIdx].mountDiskPath, NAME_BKTMGR) ;

				if(!OPEN_RDWR(fd_bkt, diskpath)) // open bktmgr.udf file
		        {
		            BKTMD_ERR("failed open basket manager file in decoding. %s\n", diskpath);
		            return BKT_ERR;
		        }

		        if(!READ_ST(fd_bkt, hd))
		        {
		            BKTMD_ERR("failed read basket manager header in decoding.\n");
		            CLOSE_BKT(fd_bkt);
		            return BKT_ERR;
		        }
			
				gBKTMD_CtrlV.nexthdd 	= gbkt_mount_info.hddInfo[hddIdx].nextHddIdx;
				gBKTMD_CtrlV.curhdd 	= hddIdx;
				gBKTMD_CtrlV.bktcnt 	= hd.bkt_count ;

				sprintf(gBKTMD_CtrlV.target_path, "%s", gbkt_mount_info.hddInfo[hddIdx].mountDiskPath);
				sprintf(gBKTMD_CtrlA.target_path, "%s", gbkt_mount_info.hddInfo[hddIdx].mountDiskPath) ;

				CLOSE_BKT(fd_bkt) ;

				TRACE0_BKTMD("set multi playback target disk path :%s\n", gBKTMD_CtrlV.target_path);

				return TRUE ;
			}
   		}
    }

	BKTMD_ERR("Failed set multi playback target disk path !!! %s\n",gBKTMD_CtrlV.target_path);

	return FALSE;
}
////////////////////////////

int BKTMD_exgetTargetDisk(char *path) 
{
	sprintf(path, "%s",gBKTMD_CtrlV.target_path) ;
	return TRUE ;
}


long BKTMD_GetRelsolutionFromVHDR(int* pCh, int *pWidth, int *pHeight, int *withAudio, int* capMode)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	long cur_fpos = LTELL(gBKTMD_CtrlV.hbkt);

	int ret = READ_STRC(gBKTMD_CtrlV.hbkt, shd);
#ifdef BKT_SYSIO_CALL
	if(ret < 1)
	{
		BKTMD_ERR("failed read stream header -- ret=%d\n", ret);
		return BKT_ERR;
	}
#else
	if(ret != sizeof(shd))
	{
		BKTMD_ERR("failed read stream header -- ret=%d\n", ret);
		return BKT_ERR;
	}		
#endif

	if(shd.id == ID_VIDEO_HEADER)
	{
		if(READ_ST(gBKTMD_CtrlV.hbkt, vhd))
		{
			if(LSEEK_SET(gBKTMD_CtrlV.hbkt, cur_fpos))
			{
				*pCh = shd.ch;
				*pWidth  = vhd.width;
				*pHeight = vhd.height;
				*withAudio = vhd.audioONOFF;
				*capMode   = vhd.capMode;
				TRACE0_BKTMD("Read Resolution first CH:%02d, %d X %d, withAudio:%d, capMode:%d \n", shd.ch, vhd.width, vhd.height, vhd.audioONOFF, vhd.capMode);
				return BKT_OK;
			}
		}
	}

	BKTMD_ERR("failed read stream id -- id=0x%08x\n", shd.id);
	return BKT_ERR;
}

int  BKTMD_GetNextRecordChannel(long sec, int *recCh)
{
	T_INDEX_DATA idd;
	int  recChannel[BKT_MAX_CHANNEL]={0};
	int  recChannelCount=0;
	long fpos_index = LTELL(gBKTMD_CtrlV.hidx);


	while(fpos_index != 0)
	{
		long ret = READ_STRC(gBKTMD_CtrlV.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
			    perror("lseek");
			}
			return recChannelCount;
		}
#else
		if(ret != sizeof(idd))
		{
			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
			    perror("fseek");
			}
			return recChannelCount;
		}
#endif//BKT_SYSIO_CALL

		if(idd.ts.sec >=sec)
		{
			if(recChannel[idd.ch] == 1)
			{
				if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
				    perror("seek");
				}
				memcpy(recCh, recChannel, sizeof(recChannel));
				TRACE0_BKTMD("get current record channel count:%d\n", recChannelCount);
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
int  BKTMD_GetPrevRecordChannel(long sec, int *recCh)
{
	T_INDEX_DATA idd;
	int recChannel[BKT_MAX_CHANNEL]={0};
	int recChannelCount=0;

	long fpos_index = LTELL(gBKTMD_CtrlV.hidx);
	long iframe_cur = gBKTMD_CtrlV.iframe_cur;

	while(fpos_index != 0)
	{
		if(!LSEEK_SET(gBKTMD_CtrlV.hidx, BKT_transIndexNumToPos(iframe_cur)))
		{
			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
			    perror("seek");
			}
			return recChannelCount;
		}

		long ret = READ_STRC(gBKTMD_CtrlV.hidx, idd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
			    perror("seek");
			}
			return recChannelCount;
		}
#else
		if(ret != sizeof(idd))
		{
			if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
			    perror("seek");
			}
			return recChannelCount;
		}
#endif//BKT_SYSIO_CALL

		if(idd.ts.sec <=sec)
		{
			if(recChannel[idd.ch] == 1)
			{
				if(!LSEEK_SET(gBKTMD_CtrlV.hidx, fpos_index)){
					perror("seek");
				}
				memcpy(recCh, recChannel, sizeof(recChannel));
				TRACE0_BKTMD("get current record channel count:%d\n", recChannelCount);
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

long BKTMD_SyncVideoIntoAudio(T_BKTMD_SYNC_PARAM* pSyncParam)
{
    // check if audio and video decoding from the same basket
    if(pSyncParam->bid != gBKTMD_CtrlV.bid)
    {
        // open the new basket for audio
        if(BKT_ERR == BKTMD_OpenNext(pSyncParam->bid))
        {
            BKTMD_ERR("failed sync audio with video. BID:%ld, ABID:%ld\n",pSyncParam->bid, gBKTMD_CtrlV.bid);
            return BKT_ERR;
        }
    }

    // Seek audio to video posn
    if(!LSEEK_SET(gBKTMD_CtrlV.hbkt, pSyncParam->fpos))
    {
        BKTMD_ERR("failed seek basket video. vp->fpos_bkt:%ld\n", pSyncParam->fpos);
        BKTMD_close(TRUE);
        return BKT_ERR;
    }

    TRACE0_BKTMD("seeking video to audio. audio-bid:%ld, audio-fpos:%ld\n",pSyncParam->bid,pSyncParam->fpos);

    return BKT_OK;
}

long BKTMD_SyncAudioIntoVideo(T_BKTMD_SYNC_PARAM* pSyncParam)
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
    if(pSyncParam->bid != gBKTMD_CtrlA.bid)
    {
        // open the new basket for audio
        if(BKT_ERR == BKTMD_OpenNextAud(gBKTMD_CtrlA.ch, pSyncParam->bid))
        {
            BKTMD_ERR("failed sync audio with video. BID:%ld, ABID:%ld\n",pSyncParam->bid,gBKTMD_CtrlA.bid);
            return BKT_ERR;
        }
    }

    // Seek audio to video posn
    if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,pSyncParam->fpos))
    {
        BKTMD_ERR("failed seek basket audio. vp->fpos_bkt:%ld\n", pSyncParam->fpos);
        BKTMD_closeAud();
        return BKT_ERR;
    }

    TRACE0_BKTMD("seeking audio to video. video-bid:%ld, video-fpos:%ld\n", pSyncParam->bid,pSyncParam->fpos);

#ifdef PREV_FPOS // AYK - 0113

nextFrame:

    // seek to the next audio frame
    ret = READ_STRC(gBKTMD_CtrlA.hbkt,shd);

#ifdef BKT_SYSIO_CALL
    if(ret == 0)
    {
        gotoNextBkt = 1;
        BKTMD_ERR("failed read stream header. EOF\n");
    }
#else
    if(0 != feof(gBKTMD_CtrlA.hbkt))
    {
        gotoNextBkt = 1;
        BKTMD_ERR("failed read stream header. EOF.\n");
    }
#endif

    if(gotoNextBkt == 1)
    {
        // Open next basket
        long bid = BKTMD_SearchNextBIDAud(gBKTMD_CtrlA.ch,gBKTMD_CtrlA.bid,gBKTMD_CtrlA.sec);

        if( bid == BKT_ERR )
            return BKT_ERR;

        if( BKT_ERR == BKTMD_OpenNextAud(gBKTMD_CtrlA.ch, bid))
            return BKT_ERR;

        gotoNextBkt = 0;

        goto nextFrame;
    }

    // AYK - 0119
    curFpos  = LTELL(gBKTMD_CtrlA.hbkt);

    if((shd.id != ID_AUDIO_HEADER) && (shd.id != ID_VIDEO_HEADER))
    {
        BKTMD_ERR("Invalid stream header\n");
        BKTMD_closeAud();
        return BKT_ERR;
    }

    if(shd.id == ID_AUDIO_HEADER)
    {
        if(shd.ch == pSyncParam->ch)
        {
            // AYK - 0118
            frameCnt = 0;
            gBKTMD_CtrlA.sec = shd.ts.sec;
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
			TRACE0_BKTMD("WARNING:Aud frames of ch %d not found in the vicinity. sync\n", pSyncParam->ch);
			if(firstAudFrame == 0)
			{
				if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,curAudFpos - sizeof(shd)))
				{
					BKTMD_ERR("failed seek basket audio\n");
					BKTMD_closeAud();
					return BKT_ERR;
				}
			}
			else
			{
				if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt,-sizeof(shd))){
				    perror("lseek cur");
				}
			}

			return BKT_NOAUDFR;
		}

		if(searchDirn == 0) // Forward
		{
			// audio frame of other channel so goto next frame
			if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt,sizeof(ahd) + shd.framesize))
			{
				BKTMD_ERR("failed seek basket audio\n");
				BKTMD_closeAud();
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
					if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,shd.prevFpos))
					{
						BKTMD_ERR("failed seek basket audio\n");
						BKTMD_closeAud();
						return BKT_ERR;
					}
				}
				else
				{
					TRACE0_BKTMD("Beginning of basket reached,opening previous basket\n");

					long bid = BKTMD_SearchPrevBIDAud(gBKTMD_CtrlA.ch,gBKTMD_CtrlA.bid,gBKTMD_CtrlA.sec);

					if( bid == BKT_ERR )
						return BKT_ERR;

					if(BKT_ERR == BKTMD_OpenPrevAud(gBKTMD_CtrlA.ch,bid))
						return BKT_ERR;

					if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,shd.prevFpos))
					{
						BKTMD_ERR("failed seek basket audio\n");
						BKTMD_closeAud();
						return BKT_ERR;
					}
				}

				goto nextFrame;
			}
			else if(shd.prevFpos == 0)
			{
				// goto previous basket
				TRACE0_BKTMD("Beginning of basket reached\n");
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
            TRACE0_BKTMD("WARNING:Aud frames of ch %d not found in the vicinity\n",pSyncParam->ch);
            if(firstAudFrame == 0)
            {
                if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,curAudFpos - sizeof(shd)))
                {
                    BKTMD_ERR("failed seek basket audio\n");
                    BKTMD_closeAud();
                    return BKT_ERR;
                }
            }
            else
            {
                if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt,-sizeof(shd))){
				    perror("lseek cur");
				}
            }

            return BKT_NOAUDFR;
        }

        if(searchDirn == 0) // Forward
        {
            // video frame so goto next frame
            if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt,sizeof(vhd) + shd.framesize))
            {
                BKTMD_ERR("failed seek basket audio\n");
                BKTMD_closeAud();
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
                    if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,shd.prevFpos))
                    {
                        BKTMD_ERR("failed seek basket audio\n");
                        BKTMD_closeAud();
                        return BKT_ERR;
                    }
                }
                else
                {
                    TRACE0_BKTMD("Beginning of basket reached,opening previous basket\n");

                    long bid = BKTMD_SearchPrevBIDAud(gBKTMD_CtrlA.ch,gBKTMD_CtrlA.bid,gBKTMD_CtrlA.sec);

                    if( bid == BKT_ERR )
                        return BKT_ERR;

                    if(BKT_ERR == BKTMD_OpenPrevAud(gBKTMD_CtrlA.ch,bid))
                        return BKT_ERR;

                    if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,shd.prevFpos))
                    {
                        BKTMD_ERR("failed seek basket audio\n");
                        BKTMD_closeAud();
                        return BKT_ERR;
                    }
                }

                goto nextFrame;
            }
            else if(shd.prevFpos == 0)
			{
				// goto previous basket
				TRACE0_BKTMD("Beginning of basket reached\n");
				noPrevFrSearch = 1;
			}
        }
    }

    TRACE0_BKTMD("cur aud frame time stamp:%ld sec,%ld msec\n",shd.ts.sec,shd.ts.usec/1000);

    // We are at an audio frame
    audVidDiff = (pSyncParam->ts.sec - shd.ts.sec) * 1000 + (pSyncParam->ts.usec - shd.ts.usec)/1000;

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
                TRACE0_BKTMD("cannot find aud frame close to vid frame(%ld sec,%ld msec)\n",pSyncParam->ts.sec,pSyncParam->ts.usec/1000);
                if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,prevAudFpos))
                {
                    BKTMD_ERR("failed seek basket audio\n");
                    BKTMD_closeAud();
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
				TRACE0_BKTMD("cannot find aud frame close to vid frame(%ld sec,%ld msec)\n",pSyncParam->ts.sec, pSyncParam->ts.usec/1000);
				if(!LSEEK_SET(gBKTMD_CtrlA.hbkt,prevAudFpos))
				{
					BKTMD_ERR("failed seek basket audio\n");
					BKTMD_closeAud();
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

    if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt,-sizeof(shd))){
	    perror("lseek cur");
	}

    TRACE0_BKTMD("Found the aud frame with time stamp close to vid time stamp. search frame count:%d\n", frameCnt);
    TRACE0_BKTMD("Vid time stamp = %ld sec, %ld msec\n",pSyncParam->ts.sec,pSyncParam->ts.usec/1000);
    TRACE0_BKTMD("Aud time stamp = %ld sec, %ld msec\n",shd.ts.sec,shd.ts.usec/1000);

#endif

    return BKT_OK;
}

long BKTMD_closeAud()
{
	TRACE0_BKTMD("close basket audio. BID_aud:%ld\n", gBKTMD_CtrlA.bid);

	SAFE_CLOSE_BKT(gBKTMD_CtrlA.hbkt);
	SAFE_CLOSE_BKT(gBKTMD_CtrlA.hidx);

	gBKTMD_CtrlA.bkt_status = BKTMD_STATUS_STOP;

	gBKTMD_CtrlA.ch  = 0;
	gBKTMD_CtrlA.bid = 0; // caution not zero-base

	// BK-1222
	gBKTMD_CtrlA.fpos_bkt =0;

	gBKTMD_CtrlA.iframe_cnt = 0;
	gBKTMD_CtrlA.iframe_cur = 0;


	return BKT_OK;
}

long BKTMD_ReadNextFrameAud(T_BKTMD_PARAM *dp)
{
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	int ret;
	int SearchFrameCnt = 0; // AYK - 0118

	while(gBKTMD_CtrlA.bkt_status == BKTMD_STATUS_OPEN)
	{
		ret = READ_STRC(gBKTMD_CtrlA.hbkt, shd);
#ifdef BKT_SYSIO_CALL
		if(ret < 1)
		{
			if(ret == 0)
			{
				BKTMD_ERR("failed read stream header. EOF. cur-iframe:%d, tot-iframe:%d\n", gBKTMD_CtrlA.iframe_cur, gBKTMD_CtrlA.iframe_cnt);
			}
			else
			{
				BKTMD_ERR("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			}

			break;
		}
#else
		if(ret != sizeof(shd))
		{
			if(0 != feof(gBKTMD_CtrlA.hbkt))
			{
				BKTMD_ERR("failed read stream header. EOF.\n");
			}
			else
			{
				BKTMD_ERR("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld\n", shd.id, shd.framesize);
			}

			break;
		}
#endif

		if( shd.id == ID_AUDIO_HEADER )
		{
			if(shd.ch == dp->focusChannel)
			{
				if(!READ_ST(gBKTMD_CtrlA.hbkt, ahd))
				{
					BKTMD_ERR("failed read audio header data\n");
					return BKT_ERR;
				}

				if(!READ_PTSZ(gBKTMD_CtrlA.hbkt, dp->abuffer, shd.framesize))
				{
					BKTMD_ERR("failed read audio data\n");
					return BKT_ERR;
				}

				dp->ch		    = shd.ch;
				dp->ts.sec      = shd.ts.sec;
				dp->ts.usec     = shd.ts.usec;
				dp->streamtype  = ST_AUDIO;
				dp->framesize   = shd.framesize;
				dp->samplingrate= ahd.samplingrate;
				dp->fpos = 0;

                gBKTMD_CtrlA.sec = shd.ts.sec; // AYK - 1215

				// BK-1222
//				gBKTMD_CtrlA.fpos_bkt = LTELL(gBKTMD_CtrlA.hbkt);

				return BKT_OK;
			}
			else // skip
			{
				if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt, sizeof(ahd) + shd.framesize))
				{
					BKTMD_ERR("failed seek basket audio\n");
					BKTMD_closeAud();
					return BKT_ERR;
				}

				// BK-1222
//				gBKTMD_CtrlA.fpos_bkt = LTELL(gBKTMD_CtrlA.hbkt);

                // AYK - 0201 - START
                SearchFrameCnt ++;

                if(SearchFrameCnt >= MAX_NUM_FRAME_SEARCH)
                {
                    BKTMD_ERR("WARNING:Aud frames of ch %ld not found in the vicinity. Aud\n",dp->ch);

			        return BKT_NOAUDFR;
			    }
			    // AYK - 0201 - END
			}
		}
		else if(shd.id == ID_VIDEO_HEADER)
		{
			// skip fpos to next frame
			if(!LSEEK_CUR(gBKTMD_CtrlA.hbkt, sizeof(vhd) + shd.framesize))
			{
				BKTMD_ERR("failed seek basket audio\n");
				BKTMD_closeAud();
				return BKT_ERR;
			}

// 			// BK-1222
// 			gBKTMD_CtrlA.fpos_bkt = LTELL(gBKTMD_CtrlA.hbkt);
//
// 			if(shd.frametype == FT_IFRAME )
// 			{
// 				gBKTMD_CtrlA.iframe_cur += 1;
//
// 				if( BKTREC_isRecordingBasket(gBKTMD_CtrlV.bid))
// 				{
// 					gBKTMD_CtrlA.iframe_cnt = BKTREC_GetIndexCount();
// 				}
// 			}

            // AYK - 0118 - START
            SearchFrameCnt ++;

            if(SearchFrameCnt >= MAX_NUM_FRAME_SEARCH)
            {
                BKTMD_ERR("WARNING:Aud frames of ch %ld not found in the vicinity. Vid\n",dp->ch);

			    return BKT_NOAUDFR;
			}
			// AYK - 0118 - END
		}
		else
		{
			BKTMD_ERR("incorrect stream header. SHID:0x%08lx, CH:%ld, framesize:%ld\n", shd.id, shd.ch, shd.framesize);

			break;
			//BKTMD_closeAud();
			//return BKT_ERR; // BK - 100115
		}

	}// while


	if(gBKTMD_CtrlA.bkt_status == BKTMD_STATUS_OPEN)
	{
		long bid = BKTMD_SearchNextBIDAud(gBKTMD_CtrlA.ch, gBKTMD_CtrlA.bid, gBKTMD_CtrlA.sec);

		if( bid == BKT_ERR )
			return BKT_NONEXTBID;

		if( BKT_ERR == BKTMD_OpenNextAud(gBKTMD_CtrlA.ch, bid))
			return BKT_ERR;

		return BKTMD_ReadNextFrameAud(dp);
	}

	return BKT_ERR;
}

long BKTMD_SearchNextBIDAud(int ch, long curbid, long sec)
{
	if(BKT_isRecordingBasket(curbid))
	{
		BKTMD_ERR("There is no a next basket aud. BID:%ld is recording.\n", curbid);
		return BKT_ERR;
	}

	long sec_seed = sec;

	//if(sec_seed == 0)
	{
		long bts=0,ets=0;
		if(BKT_ERR == BKTMD_GetBasketTime(curbid, &bts, &ets))
		{
			BKTMD_ERR("There is no a next basekt aud. CH:%d, BID:%ld is recording.\n", ch, curbid);
			return BKT_ERR;
		}

		if( sec < ets)
			sec_seed = ets;
		//TRACE0_BKTMD("last play time is zero. getting last time stamp from basket aud. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
	}

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

	char buf[30];
	sprintf(buf, "%s/rdb/", gBKTMD_CtrlA.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort);

	if(nitems < 0) {
	    perror("scandir - search next bid audio");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
		free(ents); // BKKIM 20111031
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
			int m;
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTMD_CtrlA.target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_next_bid_aud;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid_aud;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid_aud;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min+1;m<TOTAL_MINUTES_DAY;m++)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0
				&& rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTMD("found next AudBID:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid);

					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}

			// in case of first fail
			tm1.hour = 0;
			tm1.min  = -1;

		} // end if(next_rdb >= curr_rdb)

		free(ents[i]);

    } // end for

lbl_err_search_next_bid_aud:
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	BKTMD_ERR("Can't find next basket. CUR-AudBID:%ld, CH:%d, CUR-AudTS:%04d-%02d-%02d %02d:%02d:%02d\n",
	              curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found
}

long BKTMD_SearchPrevBIDAud(int ch, long curbid, long sec)
{
    long sec_seed = sec;

	//if(sec_seed == 0)
    {
        long bts=0,ets=0;
        if(BKT_ERR == BKTMD_GetBasketTime(curbid, &bts, &ets))
        {
            BKTMD_ERR("There is no a previous basekt. CH:%d, BID:%ld.\n", ch, curbid);
            return BKT_ERR;
        }

		if(bts < sec)
			sec_seed = bts;
        //TRACE0_BKTMD("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);
    }

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);

    char buf[30];
    sprintf(buf, "%s/rdb/", gBKTMD_CtrlA.target_path);

    struct dirent **ents;
    int nitems, i;

    nitems = scandir(buf, &ents, NULL, alphasort_reverse);

	if(nitems < 0) {
	    perror("scandir reverse aud");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		BKTMD_ERR("There is no rdb directory %s.\n", buf);
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
			int m;
			T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
			sprintf(buf, "%s/rdb/%s", gBKTMD_CtrlA.target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				BKTMD_ERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_prev_bid_aud;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL))
			{
				BKTMD_ERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid_aud;
			}

			if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTMD_ERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid_aud;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				if(rdbs[TOTAL_MINUTES_DAY*ch+m].bid != 0
				&& rdbs[TOTAL_MINUTES_DAY*ch+m].bid !=curbid)
				{
					TRACE0_BKTMD("found prev BID:%ld, IDXPOS:%ld\n", rdbs[TOTAL_MINUTES_DAY*ch+m].bid, rdbs[TOTAL_MINUTES_DAY*ch+m].idx_pos);

					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					return rdbs[TOTAL_MINUTES_DAY*ch+m].bid; // found!!
				}
			}

			// ù??° ???? ??; ???쿡...
			tm1.hour = 23;
			tm1.min  = 60;

		} //end  if(prev_rdb <= curr_rdb)

		free(ents[i]);

    } // end for(i=0;i<nitems;i++)

lbl_err_search_prev_bid_aud:
    for(;i<nitems;i++){
	    free(ents[i]);
	}
	free(ents);

	BKTMD_ERR("Can't find prev basket. CUR-BID:%ld, CH:%d, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n",
	              curbid, ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found
}


long BKTMD_OpenNextAud(int ch, long nextbid)
{
	BKTMD_closeAud();

	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlA.target_path, nextbid);

	// open basket
	if(!OPEN_RDONLY(gBKTMD_CtrlA.hbkt, path))
	{
		BKTMD_ERR("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTMD_CtrlA.hbkt, bhdr))
	{
		BKTMD_ERR("failed read basket aud header.\n");
		BKTMD_closeAud();
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlA.target_path, nextbid);
	if(!OPEN_RDONLY(gBKTMD_CtrlA.hidx, path))
	{
		BKTMD_ERR("failed open index file -- path:%s\n", path);
		BKTMD_closeAud();
		return BKT_ERR;
	}

	// read index hdr
	T_INDEX_HDR ihd;
	if(!READ_ST(gBKTMD_CtrlA.hidx, ihd))
	{
		BKTMD_ERR("failed read index hdr\n");
		BKTMD_closeAud();
		return BKT_ERR;
	}
	SAFE_CLOSE_BKT(gBKTMD_CtrlA.hidx);

	TRACE0_BKTMD("Read index hdr. CH:%d, AudBID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ch, ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	gBKTMD_CtrlA.ch  = ch;
	gBKTMD_CtrlA.bid = nextbid;
	gBKTMD_CtrlA.iframe_cnt = BKT_isRecordingBasket(nextbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTMD_CtrlA.sec = BKT_GetMinTS(bhdr.ts.t1);
	if(gBKTMD_CtrlA.sec == 0)
		gBKTMD_CtrlA.sec = BKT_GetMinTS(ihd.ts.t1);

	// BK - 1223 - Start
	gBKTMD_CtrlA.iframe_cur = 0;
	gBKTMD_CtrlA.fpos_bkt = sizeof(T_BASKET_HDR);
	gBKTMD_CtrlA.bkt_status = BKTMD_STATUS_OPEN;

	TRACE0_BKTMD("open next basket. AudBID:%ld, iframe-cur:%d, iframe_tot:%d\n", nextbid, gBKTMD_CtrlA.iframe_cur, gBKTMD_CtrlA.iframe_cnt);

	return BKT_OK;
}

long BKTMD_OpenPrevAud(int ch, long prevbid)
{
	BKTMD_closeAud();

	char path[LEN_PATH];

	// open basket
	sprintf(path, "%s/%08ld.bkt", gBKTMD_CtrlA.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTMD_CtrlA.hbkt, path))
	{
		BKTMD_ERR("failed open basket file -- path:%s\n", path);
		BKTMD_closeAud(TRUE);
		return BKT_ERR;
	}

	T_BASKET_HDR bhdr;
	if(!READ_ST(gBKTMD_CtrlA.hbkt, bhdr))
	{
		BKTMD_ERR("failed read basket aud header.\n");
		BKTMD_closeAud();
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlA.target_path, prevbid);
	if(!OPEN_RDONLY(gBKTMD_CtrlA.hidx, path))
	{
		BKTMD_ERR("failed open index file -- path:%s\n", path);
		BKTMD_closeAud(TRUE);
		return BKT_ERR;
	}

	T_INDEX_HDR ihd;
	if(!READ_HDR(gBKTMD_CtrlA.hidx, ihd))
	{
		BKTMD_ERR("failed read index hdr\n");
		BKTMD_closeAud(TRUE);
		return BKT_ERR;
	}
	SAFE_CLOSE_BKT(gBKTMD_CtrlA.hidx);

	TRACE0_BKTMD("Read index hdr. BID:%ld, CNT:%ld, t1:%08ld, t2:%08ld\n", ihd.bid, ihd.count, ihd.ts.t1[ch].sec, ihd.ts.t2[ch].sec);

	if(ihd.count != 0)
	{
		if(!LSEEK_CUR(gBKTMD_CtrlA.hidx, sizeof(T_INDEX_DATA)*(ihd.count-1)))
		{
			BKTMD_ERR("failed seek idx. ihd.count=%ld\n", ihd.count);
			BKTMD_closeAud(TRUE);
			return BKT_ERR;
		}

		T_INDEX_DATA idd;
		if(!READ_ST(gBKTMD_CtrlA.hidx, idd))
		{
			BKTMD_ERR("failed read index data\n");
			BKTMD_closeAud(TRUE);
			return BKT_ERR;
		}

		if(!LSEEK_SET(gBKTMD_CtrlA.hbkt, idd.fpos))
		{
			BKTMD_ERR("failed seek basket.\n");
			BKTMD_closeAud(TRUE);
			return BKT_ERR;
		}
	}
	else
	{
		long prev_sec = 0;
		long prev_usec = 0;
		int index_count=0, bkt_fpos=0;
		T_INDEX_DATA idd;
		while(READ_ST(gBKTMD_CtrlA.hidx, idd))
		{
			if( (idd.ts.sec <= prev_sec && idd.ts.usec <= prev_usec)
				|| idd.id != ID_INDEX_DATA
				|| idd.fpos <= bkt_fpos)
			{
				TRACE1_BKTMD("Fount last record index pointer. sec=%ld, usec=%ld, count=%d\n", idd.ts.sec, idd.ts.usec, index_count);
				break;
			}

			prev_sec  = idd.ts.sec;
			prev_usec = idd.ts.usec;
			bkt_fpos = idd.fpos;
			index_count++;
		}

		if(index_count != 0)
		{
			if(!LSEEK_SET(gBKTMD_CtrlA.hbkt, bkt_fpos))
			{
				BKTMD_ERR("failed seek basket.\n");
				BKTMD_closeAud(TRUE);
				return BKT_ERR;
			}

			if(!LSEEK_SET(gBKTMD_CtrlA.hidx, sizeof(idd)*index_count))
			{
				BKTMD_ERR("failed seek index.\n");
				BKTMD_closeAud(TRUE);
				return BKT_ERR;
			}
		}
		else
		{
			if(!LSEEK_END(gBKTMD_CtrlA.hbkt, 0))
			{
				BKTMD_ERR("failed seek basket.\n");
				BKTMD_closeAud(TRUE);
				return BKT_ERR;
			}

			int offset = sizeof(T_INDEX_DATA);
			if(!LSEEK_END(gBKTMD_CtrlA.hidx, -offset))
			{
				BKTMD_ERR("failed seek idx.\n");
				BKTMD_closeAud(TRUE);
				return BKT_ERR;
			}
		}

	}

	gBKTMD_CtrlA.ch  = ch;
	gBKTMD_CtrlA.bid = prevbid;
	gBKTMD_CtrlA.iframe_cnt = BKT_isRecordingBasket(prevbid)? BKT_GetRecordingIndexCount():ihd.count;
	gBKTMD_CtrlA.sec = BKT_GetMaxTS(bhdr.ts.t2);
	if(gBKTMD_CtrlA.sec == 0)
		BKT_GetMaxTS(ihd.ts.t2);

	gBKTMD_CtrlA.iframe_cur = gBKTMD_CtrlA.iframe_cnt - 1;

	// BK - 1223
	gBKTMD_CtrlA.fpos_bkt = LTELL(gBKTMD_CtrlA.hbkt);
	gBKTMD_CtrlA.bkt_status = BKTMD_STATUS_OPEN;

	TRACE0_BKTMD("open prev basket. BID:%ld, iframe-current:%d, iframe_total:%d\n", prevbid, gBKTMD_CtrlA.iframe_cur, gBKTMD_CtrlA.iframe_cnt);

	return BKT_OK;
}

long BKTMD_GetRecDays(int ch, long t_o, char* pRecMonTBL)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(t_o, &tm1);

	int i, bFindOK=0;

	char path[LEN_PATH];

	for(i=0;i<31;i++)
	{
		sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, i+1);

		if(-1 != access(path, 0))
		{
			*(pRecMonTBL+i) = 1;

			if(!bFindOK)
				bFindOK=1;
		}
	}

	return bFindOK;
}

//long BKTMD_GetRecHour(int ch, long sec, char* pRecHourTBL) // 2010/12/14 CSNAM
long BKTMD_GetRecMin(int ch, long sec, char* pRecHourTBL)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	long offset = TOTAL_MINUTES_DAY*ch;
	if(!LSEEK_SET(fd, offset))
	{
		BKTMD_ERR("failed seek rdb.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, pRecHourTBL, TOTAL_MINUTES_DAY))
	{
		BKTMD_ERR("failed read rdb CH:%d, offset:%ld\n", ch, offset);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	TRACE0_BKTMD("Succeeded Get record hour info, ch:%d, %04d/%02d/%02d %02d:%02d:%02d\n", ch, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	CLOSE_BKT(fd);

	return 0;

}

int BKTMD_getCapMode(long sec)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

	BKT_FHANDLE fd;

	int ch;
	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

	if(!OPEN_RDONLY(fd, path))
	{
		BKTMD_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTMD_ERR("failed read rec evt data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		BKTMD_ERR("failed read rec rdb data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	long bid=0;
	long ipos=sizeof(T_INDEX_HDR);

	for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
	{
		if(evts[ch*TOTAL_MINUTES_DAY+tm1.hour*60+tm1.min] != 0)
		{
			bid  = rdbs[ch*TOTAL_MINUTES_DAY+tm1.hour*60+tm1.min].bid;
			ipos = rdbs[ch*TOTAL_MINUTES_DAY+tm1.hour*60+tm1.min].idx_pos;

			TRACE0_BKTMD("found index info. [%02d:%02d], CH:%02d, BID:%ld, ipos:%ld\n", tm1.hour, tm1.min, ch, bid, ipos);
			break;
		}
	}

	sprintf(path, "%s/%08ld.idx", gBKTMD_CtrlV.target_path, bid);

	BKT_FHANDLE fd_idx;
	T_INDEX_DATA idd;

	if(!OPEN_RDONLY(fd_idx, path))
	{
		BKTMD_ERR("failed open idx file %s\n", path);
		return BKT_ERR;
	}
	if(!LSEEK_SET(fd_idx, ipos))
	{
		BKTMD_ERR("failed open idx file %s\n", path);
		CLOSE_BKT(fd_idx);
		return BKT_ERR;
	}

	if(!READ_ST(fd_idx, idd) || idd.id != ID_INDEX_DATA)
	{
		BKTMD_ERR("failed read idx data.\n");
		CLOSE_BKT(fd_idx);
		return BKT_ERR;
	}
	CLOSE_BKT(fd_idx);

	TRACE0_BKTMD("idd.id=0x%08x, idd.capMode=%d, idd.ch=%ld, idd.event=%ld, idd.width=%d, idd.height=%d\n", idd.id, idd.capMode, idd.ch, idd.event, idd.width, idd.height);

	return idd.capMode;
}

long BKTMD_GetCurPlayTime()
{
    return gBKTMD_CtrlV.sec ;
}

int BKTMD_getRecFileCount(int *pChannels, long t1, long t2 )
{
	int ret = 0, i = 0, pathcnt = 0 ;
    int basket_count=0;

    char path1[LEN_PATH];
    char path2[LEN_PATH];

	char pathbuf[LEN_PATH] ;

	ret = BKTMD_getTargetDisk(t1) ;
    T_BKT_TM tm1;
    BKT_GetLocalTime(t1, &tm1);

	if(ret != TRUE)
		return FALSE ;

    sprintf(path1, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

    T_BKT_TM tm2;
    BKT_GetLocalTime(t2, &tm2);

	ret = BKTMD_getTargetDisk(t2) ;
	if(ret != TRUE)
		return FALSE ;
	
    sprintf(path2, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm2.year, tm2.mon, tm2.day);

    if(strcmp(path1, path2) != 0)
    {
        TRACE0_BKTMD("different from-date:%s and to-date:%s\n", path1, path2);
		pathcnt = 2 ;
	}
	else
	{
		pathcnt = 1 ;
	}
	sprintf(pathbuf, "%s", path1) ;

    if(-1 == access(pathbuf, 0)) // existence only
    {
       	BKTMD_ERR("There is no rdb file. %s\n", pathbuf[i]);
       	return 0;
    }

#ifdef BKT_SYSIO_CALL
    int fd;
#else
    FILE *fd;
#endif

    if(!OPEN_RDONLY(fd, pathbuf))
    {
       	BKTMD_ERR("failed open rdb %s\n", pathbuf);
       	return BKT_ERR;
   	}

    char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
    T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
    {
       	BKTMD_ERR("failed read evt data\n");
       	CLOSE_BKT(fd);
       	return BKT_ERR;
   	}

    if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
   	{
       	BKTMD_ERR("failed read rdbs data\n");
       	CLOSE_BKT(fd);
       	return BKT_ERR;
   	}
   	CLOSE_BKT(fd);

   	long prev_bid=0;
   	int m, c;
   	int chs[BKT_MAX_CHANNEL];
   	memcpy(chs, pChannels, sizeof(chs));

	if(pathcnt == 1)
	{
   		for(m=tm1.hour*60+tm1.min ; m<=tm2.hour*60+tm2.min ; m++)
   		{
       		for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
       		{
//           		printf("bkt count chs[%d]=%d\n", c, chs[c]);
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
	}
	else
	{
		m = tm1.hour*60+tm1.min ;
		while(1)
		{
			for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
            {
//                printf("chs[%d]=%d\n", c, chs[c]);
                if(rdbs[TOTAL_MINUTES_DAY*c+m].bid != 0
                    && prev_bid != rdbs[TOTAL_MINUTES_DAY*c+m].bid
                    && chs[c] == 1)
                {
                    basket_count++;
                    prev_bid = rdbs[TOTAL_MINUTES_DAY*c+m].bid;
                    break;
                }
				else if(rdbs[TOTAL_MINUTES_DAY*c+m].bid == 0)
				{
    				TRACE0_BKTMD("get backup file count on HDD end :%d\n", basket_count);
					return basket_count ;
				}
            }
			m++ ;
		}
	}

    TRACE0_BKTMD("get backup file count :%d\n", basket_count);

    return basket_count;
}

int BKTMD_getRecFileList(int *pChannels, long t1, long t2, int *pFileArray, char *srcPath, int *pathcount )
{
	int ret = 0, i = 0 ;
    int basket_count= 0;
	int pathcnt = 0 ;

    char path1[LEN_PATH];
    char path2[LEN_PATH];

	char pathbuf[LEN_PATH] ;

    T_BKT_TM tm1;
    BKT_GetLocalTime(t1, &tm1);

	ret = BKTMD_getTargetDisk(t1) ;
	if(ret != TRUE)
        return FALSE ;

   	sprintf(srcPath, "%s", gBKTMD_CtrlV.target_path);
    sprintf(path1, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm1.year, tm1.mon, tm1.day);

    T_BKT_TM tm2;
    BKT_GetLocalTime(t2, &tm2);

	ret = BKTMD_getTargetDisk(t2) ;
	if(ret != TRUE)
        return FALSE ;

    sprintf(path2, "%s/rdb/%04d%02d%02d", gBKTMD_CtrlV.target_path, tm2.year, tm2.mon, tm2.day);

    if(strcmp(path1, path2) != 0)
    {
        TRACE0_BKTMD("different from-date:%s and to-date:%s\n", path1, path2);

		pathcnt = 2 ;

    }
	else
	{
		pathcnt = 1 ;
	}
	*pathcount = pathcnt ;
        sprintf(pathbuf, "%s", path1) ;	

    if(-1 == access(pathbuf, 0)) // existence only
    {
       	BKTMD_ERR("There is no rdb file. %s\n", path1);
       	return 0;
   	}

#ifdef BKT_SYSIO_CALL
    int fd;
#else
    FILE *fd;
#endif

    if(!OPEN_RDONLY(fd, pathbuf))
   	{
      	BKTMD_ERR("failed open rdb %s\n", pathbuf);
        return BKT_ERR;
    }

    char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
    T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

    if(!READ_PTSZ(fd, evts, sizeof(evts)))
    {
       	BKTMD_ERR("failed read evt data\n");
       	CLOSE_BKT(fd);
       	return BKT_ERR;
    }

   	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
   	{
       	BKTMD_ERR("failed read rdbs data\n");
       	CLOSE_BKT(fd);
       	return BKT_ERR;
   	}

	CLOSE_BKT(fd);

   	long prev_bid=0;
   	int m,c;
   	int chs[BKT_MAX_CHANNEL];
   	memcpy(chs, pChannels, sizeof(chs));

	if(pathcnt == 1)
	{
    	for(m=tm1.hour*60+tm1.min;m<=tm2.hour*60+tm2.min;m++)
    	{
        	for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
        	{
            	if(rdbs[TOTAL_MINUTES_DAY*c+m].bid != 0
                	&& prev_bid != rdbs[TOTAL_MINUTES_DAY*c+m].bid
                	&& chs[c] == 1)
            	{

                	*(pFileArray+basket_count) = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

                	TRACE0_BKTMD("found backup basket. BID:%d\n", *(pFileArray+basket_count));

                	basket_count++;
                	prev_bid = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

                	break;
            	}
        	}
		}
    }
	else
	{
		m = tm1.hour*60+tm1.min ;
        while(1)
        {
            for(c = 0 ; c < BKT_MAX_CHANNEL ; c ++)
            {
//                printf("FileList chs[%d]=%d\n", c, chs[c]);
                if(rdbs[TOTAL_MINUTES_DAY*c+m].bid != 0
                    && prev_bid != rdbs[TOTAL_MINUTES_DAY*c+m].bid
                    && chs[c] == 1)
                {
                	*(pFileArray+basket_count) = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

                	TRACE0_BKTMD("found backup basket. BID:%d\n", *(pFileArray+basket_count));

                	basket_count++;
                	prev_bid = rdbs[TOTAL_MINUTES_DAY*c+m].bid;

                	break;
                }
                else if(rdbs[TOTAL_MINUTES_DAY*c+m].bid == 0)
                {
                    TRACE0_BKTMD("get backup file count on HDD end :%d\n", basket_count);
                    return basket_count ;
                }
            }
            m++ ;
        }
	}

    return basket_count ;
}

//////////////////////////////////////////////////////////////////////////
// EOF basket_muldec.c
//////////////////////////////////////////////////////////////////////////

