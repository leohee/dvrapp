/**
 @file bsk_tplay.c
 @brief trick play
*/

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#include "osa.h"
#include "bsk_tplay.h"
#include "ti_vdec.h"

//////////////////////////////////////////////////////////////////////////
//#define TPLAY_DEBUG

#ifdef TPLAY_DEBUG
static char *tty_clr[] = {
      "\033[0;40;30m",       /* 0   black on black */
      "\033[0;40;31m",       /* 1   red */
      "\033[0;40;32m",       /* 2   green */
      "\033[0;40;33m",       /* 3   brown */
      "\033[0;40;34m",       /* 4   blue */
      "\033[0;40;35m",       /* 5   magenta */
      "\033[0;40;36m",       /* 6   cyan */
      "\033[0;40;37m",       /* 7   light gray */
      "\033[1;40;30m",       /* 0   gray */
      "\033[1;40;31m",       /* 1   brightred */
      "\033[1;40;32m",       /* 2   brightgreen */
      "\033[1;40;33m",       /* 3   yellow */
      "\033[1;40;34m",       /* 4   brightblue */
      "\033[1;40;35m",       /* 5   brighmagenta */
      "\033[1;40;36m",       /* 6   brightcyan */
      "\033[1;40;37m",       /* 7   white */
};
#define TPLAY_DBG0(msg, args...) {printf("[TPLAY] - " msg, ##args);}
#define TPLAY_DBG1(msg, args...) {printf("[TPLAY] - %s%s(%d):%s " msg, tty_clr[2], __FUNCTION__, __LINE__, tty_clr[7], ##args);}
#define TPLAY_DBG2(msg, args...) {printf("%s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args);}
#define TPLAY_DERR(msg, args...)  {printf("%s[ERR] - %s(%d):%s:%s " msg, tty_clr[1], __FILE__, __LINE__, __FUNCTION__, tty_clr[7], ##args);}
#else
#define TPLAY_DBG0(msg, args...) {((void)0);}
#define TPLAY_DBG1(msg, args...) {((void)0);}
#define TPLAY_DBG2(msg, args...) {((void)0);}
#define TPLAY_DERR(msg, args...) {((void)0);}
#endif
//////////////////////////////////////////////////////////////////////////

T_BKTMD_CTRL gTPLAY_Ctrl; ///< video decode control for trick play

void tplay_close(int bStop)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	TPLAY_DBG0("close basket. CH:%2d, BID:%ld\n", pCtrl->ch, pCtrl->bid);

	if(bStop) {
		pCtrl->bkt_status = BKTMD_STATUS_STOP;
		pCtrl->ch = 0;
		pCtrl->bid = 0; // caution not zero-base
		pCtrl->fpos_bkt =0;
		pCtrl->iframe_cnt = 0;
		pCtrl->iframe_cur = 0;
	}

	// file handle
	SAFE_CLOSE_BKT(pCtrl->hbkt);
	SAFE_CLOSE_BKT(pCtrl->hidx);

}

long tplay_open(int ch, long sec, T_BKTMD_OPEN_PARAM* pOP)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	int i, bFound=0;
	T_BKT_TM tm1;
	BKT_FHANDLE fd; // file descript

	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdb;
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_INDEX_HDR  ihd;
	T_INDEX_DATA idd;
	long idx_pos;

	TPLAY_DBG1("Enter new-ch:%d, old-ch:%d, sec:%ld\n", ch, pCtrl->ch, sec);
	tplay_close(TRUE);

	// importance
	pCtrl->ch = ch;

	if(FALSE == tplay_get_target_disk(ch, sec)){
	    return BKT_ERR;
	}

	BKT_GetLocalTime(sec, &tm1);
	sprintf(path, "%s/rdb/%04d%02d%02d", pCtrl->target_path, tm1.year, tm1.mon, tm1.day);
	if(!OPEN_RDONLY(fd, path))
	{
		TPLAY_DERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	// read events
	if(!READ_PTSZ(fd, evts, sizeof(evts)))
 	{
 		TPLAY_DERR("failed read evt data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	// read record database
	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
 	{
 		TPLAY_DERR("failed read rdb data\n");
 		CLOSE_BKT(fd);
 		return BKT_ERR;
	}

	// done read rec information from rdb file
	CLOSE_BKT(fd);

	rdb.bid = 0;
	for(i = tm1.hour*60+tm1.min; i < TOTAL_MINUTES_DAY ; i++)
	{
		if (evts[TOTAL_MINUTES_DAY * ch + i] != 0 && rdbs[TOTAL_MINUTES_DAY*ch + i].bid != 0) {

			rdb.bid     = rdbs[TOTAL_MINUTES_DAY * ch + i].bid;
			rdb.idx_pos = rdbs[TOTAL_MINUTES_DAY * ch + i].idx_pos;

			pOP->recChannelCount++;
			pOP->recChannels[ch] = 1;

			TPLAY_DBG0("found start rec time. [%02d:%02d], CH:%02d, BID:%ld, idx_fpos:%ld\n",
					i/60, i%60, ch, rdbs[TOTAL_MINUTES_DAY*ch+i].bid, rdbs[TOTAL_MINUTES_DAY*ch+i].idx_pos);

			bFound = 1;
			break;
		}
	}

	if(bFound == 0)
	{
		TPLAY_DERR("failed find start record time. ch:%d, path:%s, ts:%04d/%02d/%02d %02d:%02d:%02d\n", ch, path, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	// open the basket file using above found ID
	sprintf(path, "%s/%08ld.bkt", pCtrl->target_path, rdb.bid);
	if(!OPEN_RDONLY(pCtrl->hbkt, path))
	{
		TPLAY_DERR("failed open basket file for video decoding -- path:%s\n", path);
		return BKT_ERR;
	}

	//////////////////////////////////////////////////////////////////////////
	// open the index file using above found ID
	sprintf(path, "%s/%08ld.idx", pCtrl->target_path, rdb.bid);
	if(!OPEN_RDONLY(pCtrl->hidx, path))
	{
		TPLAY_DERR("failed open index file for decoding -- ch:%d, path:%s\n", ch, path);
		CLOSE_BKT(pCtrl->hbkt);
		return BKT_ERR;
	}

	// read index header
	if(!READ_ST(pCtrl->hidx, ihd))
	{
		TPLAY_DERR("failed read index hdr\n");
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// seek index file pointer using above found index file point
	if(!LSEEK_SET(pCtrl->hidx, rdb.idx_pos))
	{
		TPLAY_DERR("failed seek ch:%d, index:%ld.\n", ch, rdb.idx_pos);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// search being index file point.
	idx_pos=rdb.idx_pos;

	// read index data for find the basket file point
	if(!READ_ST(pCtrl->hidx, idd))
	{
		TPLAY_DERR("failed read index data. idx_pos:%ld\n", idx_pos);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// save index file point to return after read index data.
	idx_pos = LTELL(pCtrl->hidx) - sizeof(idd);

	// calculate iframe count
	pCtrl->iframe_cnt = BKT_isRecording(rdb.bid, pCtrl->target_path)? BKT_GetRecordingIndexCount():ihd.count;
	pCtrl->iframe_cur = BKT_transIndexPosToNum(idx_pos);

	// seek basket file point with was read index data's fpos
	if(!LSEEK_SET(pCtrl->hbkt, idd.fpos))
	{
		TPLAY_DERR("failed seek basket:%ld.\n", idd.fpos);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// seek orginal file point
	if(!LSEEK_SET(pCtrl->hidx, idx_pos))
	{
		TPLAY_DERR("failed seek index:%ld.\n", idx_pos);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	pCtrl->ch  = ch;
	pCtrl->bid = rdb.bid;
	pCtrl->sec = idd.ts.sec;
	pCtrl->fpos_bkt =idd.fpos; // BK-1222

	pOP->openbid = rdb.bid;
	pOP->openfpos = idd.fpos;
	pOP->open_ts.sec = idd.ts.sec;
	pOP->open_ts.usec = idd.ts.usec;

	//////////////////////////////////////////////////////////////////////////
	// set open because audio thread
	pCtrl->bkt_status = BKTMD_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////

	TPLAY_DBG0("open basket decoding -- CH:%ld, w:%d, h:%d, BID:%ld, B-POS:%ld, I-POS:%ld, curiframe:%d, totIFrame:%d \n",
					idd.ch, idd.width, idd.height, rdb.bid, idd.fpos, idx_pos ,pCtrl->iframe_cur, pCtrl->iframe_cnt);

	return BKT_OK;
}

long tplay_open_prev(int ch, long prevbid)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;
    tplay_close(FALSE);

	T_BASKET_HDR bhdr;
	T_INDEX_HDR ihd;
	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", pCtrl->target_path, prevbid);
	//TPLAY_DERR("path = %s prevbid = %d\n",path, prevbid) ;

	// open basket
	if(!OPEN_RDONLY(pCtrl->hbkt, path))
	{
		TPLAY_DERR("failed open basket file -- path:%s\n", path);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	if(!READ_ST(pCtrl->hbkt, bhdr))
	{
		TPLAY_DERR("failed read basket header.\n");
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", pCtrl->target_path, prevbid);
	if(!OPEN_RDONLY(pCtrl->hidx, path))
	{
		TPLAY_DERR("failed open index file -- path:%s\n", path);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// read index hdr
	if(!READ_ST(pCtrl->hidx, ihd))
	{
		TPLAY_DERR("failed read index hdr\n");
		tplay_close(TRUE);
		return BKT_ERR;
	}

	pCtrl->ch  = ch;
	pCtrl->bid = prevbid;
	pCtrl->iframe_cnt = BKT_isRecording(prevbid, pCtrl->target_path)? BKT_GetRecordingIndexCount():ihd.count;
	pCtrl->sec = BKT_GetMinTS(bhdr.ts.t1);
	if(pCtrl->sec == 0)
		pCtrl->sec = BKT_GetMinTS(ihd.ts.t1);

	pCtrl->iframe_cur = pCtrl->iframe_cnt-1;

	if(!LSEEK_END(pCtrl->hbkt, 0)){
	    perror("lseek end");
	}
	pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

	//////////////////////////////////////////////////////////////////////////
	// set open status because audio thread
	pCtrl->bkt_status = BKTMD_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////
	// BK - 1223 - End

	TPLAY_DBG0("open prev basket mul. BID:%ld, iframe-current:%d, iframe_total:%d, last_sec:%ld\n", prevbid, pCtrl->iframe_cur, pCtrl->iframe_cnt, pCtrl->sec);

	return BKT_OK;

}

long tplay_open_next(int ch, long nextbid)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

    tplay_close(FALSE); // only handle

	T_BASKET_HDR bhdr;
	T_INDEX_HDR ihd;
	char path[LEN_PATH];
	sprintf(path, "%s/%08ld.bkt", pCtrl->target_path, nextbid);
	//TPLAY_DERR("path = %s nextbid = %d\n",path, nextbid) ;

	// open basket
	if(!OPEN_RDONLY(pCtrl->hbkt, path))
	{
		TPLAY_DERR("failed open basket file -- path:%s\n", path);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	if(!READ_ST(pCtrl->hbkt, bhdr))
	{
		TPLAY_DERR("failed read basket header.\n");
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// open index
	sprintf(path, "%s/%08ld.idx", pCtrl->target_path, nextbid);
	if(!OPEN_RDONLY(pCtrl->hidx, path))
	{
		TPLAY_DERR("failed open index file -- path:%s\n", path);
		tplay_close(TRUE);
		return BKT_ERR;
	}

	// read index hdr
	if(!READ_ST(pCtrl->hidx, ihd))
	{
		TPLAY_DERR("failed read index hdr\n");
		tplay_close(TRUE);
		return BKT_ERR;
	}

	pCtrl->ch  = ch;
	pCtrl->bid = nextbid;
	pCtrl->iframe_cnt = BKT_isRecording(nextbid, pCtrl->target_path)? BKT_GetRecordingIndexCount():ihd.count;
	pCtrl->sec = BKT_GetMinTS(bhdr.ts.t1);
	if(pCtrl->sec == 0)
		pCtrl->sec = BKT_GetMinTS(ihd.ts.t1);

	pCtrl->iframe_cur = 0;
	pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

	//////////////////////////////////////////////////////////////////////////
	// set open status because audio thread
	pCtrl->bkt_status = BKTMD_STATUS_OPEN;
	//////////////////////////////////////////////////////////////////////////
	// BK - 1223 - End

	TPLAY_DBG0("open next basket mul. BID:%ld, iframe-current:%d, iframe_total:%d, last_sec:%ld\n", nextbid, pCtrl->iframe_cur, pCtrl->iframe_cnt, pCtrl->sec);

	return BKT_OK;

}
 
long tplay_read_next_frame(int ch, T_BKTMD_PARAM *dp)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	while(pCtrl->bkt_status == BKTMD_STATUS_OPEN)
	{
	    usleep(1);

		// read stream header
		if(!READ_HDR(pCtrl->hbkt, shd))
		{
TPLAY_DERR("failed read stream header. go next iframe. ch:%d, bid:%d, cur-iframe:%d, tot-iframe:%d, fpos=%ld\n",
			ch, pCtrl->bid, pCtrl->iframe_cur, pCtrl->iframe_cnt, pCtrl->fpos_bkt);
			break;
		}

		// skip
		if(shd.ch != ch) {

			int hd_size =  (shd.id == ID_VIDEO_HEADER)? SIZEOF_VSTREAM_HDR:SIZEOF_ASTREAM_HDR;

			if(!LSEEK_CUR(pCtrl->hbkt, hd_size + shd.framesize)) {
				perror("tplay read next frame");
				TPLAY_DERR("failed seek cur basket. ch=%d\n",ch);
				break; // search next basket
			}

			if(shd.frametype == FT_IFRAME )
			{
				pCtrl->iframe_cur += 1;

				if( BKT_isRecording(pCtrl->bid, pCtrl->target_path))
					pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
			}

			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);
			continue;
		}

		if(shd.id == ID_VIDEO_HEADER)
		{
			// maybe max interval frame. for broken stream or motion.
			if(shd.ts.sec - pCtrl->sec > 4 || shd.ts.sec - pCtrl->sec < 0)
			{
TPLAY_DBG0("time interval is too long. Search next iframe. CH:%ld, shd.id:0x%08lx, frametype:%ld, diff=%ld\n", 
            shd.ch, shd.id, shd.frametype, shd.ts.sec - pCtrl->sec);
				break;
			}

			// read video stream header
			if(!READ_ST(pCtrl->hbkt, vhd))
			{
				TPLAY_DERR("failed read video header\n");
				break;
			}

			if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize))
			{
				TPLAY_DERR("failed read video data\n");
				break;
			}

			dp->ch          = shd.ch;
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
			dp->audioONOFF  = vhd.audioONOFF; // BK - 100119
			dp->capMode     = vhd.capMode;

			if(strlen(vhd.camName))
				strncpy(dp->camName, vhd.camName, 16);

			pCtrl->sec = shd.ts.sec;

			// BK-1222
			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

			if(shd.frametype == FT_IFRAME )
			{
				pCtrl->iframe_cur += 1;

				if( BKT_isRecording(pCtrl->bid, pCtrl->target_path))
					pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
			}
			dp->bktId      = pCtrl->bid;
			dp->fpos_bkt   = pCtrl->fpos_bkt;

			//TPLAY_DBG0("Read frame, frameType:%d, iframe-num:%d, size:%ld\n", shd.frametype, pCtrl->iframe_cur, shd.framesize);
			return BKT_OK;

		}
		else if( shd.id == ID_AUDIO_HEADER )
		{
#if 0
			// skip
			if(!LSEEK_CUR(pCtrl->hbkt, sizeof(ahd) + shd.framesize))
			{
				TPLAY_DERR("failed seek basket.\n");
				break;
			}

			// BK-1222
			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);
#else

			if(!READ_ST(pCtrl->hbkt, ahd))
			{
				TPLAY_DERR("failed read audio header data\n");
				return BKT_ERR;
			}

			if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize))
			{
				TPLAY_DERR("failed read audio data\n");
				return BKT_ERR;
			}

			dp->ch		    = shd.ch;
			dp->ts.sec      = shd.ts.sec;
			dp->ts.usec     = shd.ts.usec;
			dp->streamtype  = ST_AUDIO;
			dp->framesize   = shd.framesize;
			dp->samplingrate= ahd.samplingrate;
			dp->fpos = 0;

			pCtrl->sec = shd.ts.sec; // AYK - 1215

			// BK-1222
			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);
#endif
			return BKT_OK;
		}
		else // if(shd.id != ID_VIDEO_HEADER && shd.id != ID_AUDIO_HEADER)
		{
			TPLAY_DBG0("maybe appeared broken stream .Search next frame. ch:%d, shd.id:0x%08lx, bid=%ld, fpos=%ld\n",
			            ch, shd.id, pCtrl->bid, pCtrl->fpos_bkt);
			break;
		}

	} // while(pCtrl->bkt_status == BKTMD_STATUS_OPEN)

	if(shd.id == ID_BASKET_END) // Normal sequence closed basket
	{
		return tplay_read_1st_frame_of_next_basket(pCtrl, dp); // reading any FirstFrame on NextBasket
	}

	return tplay_read_next_iframe(pCtrl->ch, dp); // Reading First Iframe on NextBasket
}

long tplay_read_prev_iframe(int ch, T_BKTMD_PARAM *dp)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	long prevbid=-1;
	int oldHddidx;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_INDEX_DATA idd;

	while (pCtrl->bkt_status == BKTMD_STATUS_OPEN)
	{
		if(BKT_isRecording(pCtrl->bid, pCtrl->target_path)) {
			pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
		}

		if(pCtrl->iframe_cur - 1 > 0) {
			pCtrl->iframe_cur -= 1;
		}
		else { // find prev basket

			// by current basket
			prevbid = tplay_get_prev_bid(pCtrl);

			////MULTI-HDD SKSUNG/////
			if( gbkt_mount_info.hddCnt == 1 && prevbid == BKT_ERR ) {
				return BKT_ERR ;
			}

			if(gbkt_mount_info.hddCnt > 1)
			{
				if(1 == pCtrl->bid)
				{
					prevbid 	= 0;
					oldHddidx 	= pCtrl->curhdd;

					do{
						prevbid = tplay_get_prev_bid_hdd(ch, pCtrl->nexthdd);
						if(prevbid != BKT_ERR)
							break;
					}while(pCtrl->nexthdd == oldHddidx);

					if(prevbid == BKT_ERR){ return prevbid; }
				}
			}

			if( BKT_ERR == tplay_open_prev(ch, prevbid))
			{
				TPLAY_DERR("Error tplay open with prevBID = %ld\n",prevbid) ;
				return BKT_ERR; // There is no more basket....
			}
		}

		// set index file pointer
		if(!LSEEK_SET(pCtrl->hidx, BKT_transIndexNumToPos(pCtrl->iframe_cur))) {
			TPLAY_DERR("failed seek read prev frame. iframe:%d\n", pCtrl->iframe_cur);
			return BKT_ERR;
		}
		// and read index data
		if(!READ_ST(pCtrl->hidx, idd)) {
			TPLAY_DERR("failed read index data. go prev iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, pCtrl->iframe_cur, pCtrl->iframe_cnt);
			continue;
		}

		// check givne channel
		if(idd.ch != ch ) {
			TPLAY_DBG1("different ch --  idd.ch=%2d, pCtrl.ch=%2ld. BID:%ld, cur-iframe:%d, tot-iframe:%d\n", idd.ch, ch, pCtrl->bid, pCtrl->iframe_cur, pCtrl->iframe_cnt);
			continue;
		}

#if 0   // do not need because set basket file point with index info
		if(idd.fpos < pCtrl->fpos_bkt) {
			TPLAY_DERR("invalid file pointer. idd.fpos=%ld, pCtrl->fpos_bkt=%ld.\n",
					idd.fpos, pCtrl->fpos_bkt);
			continue;
		}
#endif

		// set basket file pointer with index data
		if(!LSEEK_SET(pCtrl->hbkt, idd.fpos)) {
			TPLAY_DERR("failed seek prev index data. i-fpos:%ld\n", idd.fpos);
			continue;
		}

		// read stream header info of basket file
		if(!READ_HDR(pCtrl->hbkt, shd)) {
			TPLAY_DERR("failed read stream header.\n");
			continue;
		}

		// check given channel
		if(shd.ch != ch ) {
			TPLAY_DBG1("different channel ch=%d, shd.ch=%ld.\n", ch, shd.ch);
			continue;
		}

		// we need only video info because of reading iframe
		if(shd.id != ID_VIDEO_HEADER) {
			TPLAY_DERR("Invalid index data's basket file pointer.maybe broken iframe-index file.\n");
			continue;
		}

#if 0   // do not need this, because read i-frame only
		if(shd.frametype != FT_IFRAME) {
			TPLAY_DERR("Invalid frame type. should be iframe.index data's basket file pointer. maybe broken iframe-index file.\n");
			continue;
		}
#endif

		if(!READ_HDR(pCtrl->hbkt, vhd)) {
			TPLAY_DERR("failed read video stream header.\n");
			continue;
		}

		if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize )) {
			TPLAY_DERR("failed read video raw data.\n");
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

		pCtrl->sec = shd.ts.sec;
		pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

		printf("Read Prev I-Frame, ch=%d,  bktID = %ld, pCtrl->fpos_bkt=%ld\n",ch, pCtrl->bid,pCtrl->fpos_bkt) ;
		dp->bktId    = pCtrl->bid;
		dp->fpos_bkt = pCtrl->fpos_bkt;

		return BKT_OK;

	} // end if(pCtrl->bkt_status == BKTMD_STATUS_OPEN)

	TPLAY_DERR("Not open basket. CH:%d\n", ch);
	return BKT_ERR;
}

long tplay_read_next_iframe(int ch, T_BKTMD_PARAM *dp)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	long nextbid=-1;
	int oldHddidx;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_INDEX_DATA idd;

	while (pCtrl->bkt_status == BKTMD_STATUS_OPEN)
	{
		if(BKT_isRecording(pCtrl->bid, pCtrl->target_path)) {
			pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
		}

		if(pCtrl->iframe_cur + 1 < pCtrl->iframe_cnt) {
			pCtrl->iframe_cur += 1;
		}
		else { // find next basket

			// by current basket
			nextbid = tplay_get_next_bid(pCtrl);

			////MULTI-HDD SKSUNG/////
			if( gbkt_mount_info.hddCnt == 1 && nextbid == BKT_ERR ) {
				return BKT_ERR ;
			}

			if(gbkt_mount_info.hddCnt > 1)
			{
				if(pCtrl->bktcnt == pCtrl->bid)
				{
					nextbid 	= 0;
					oldHddidx 	= pCtrl->curhdd;

					do{
						nextbid = tplay_get_next_bid_hdd(ch, pCtrl->nexthdd);
						if(nextbid != BKT_ERR)
							break;
					}while(pCtrl->nexthdd == oldHddidx);

					if(nextbid == BKT_ERR){ return nextbid; }
				}
			}

			if( BKT_ERR == tplay_open_next(ch, nextbid))
			{
				TPLAY_DERR("Error nextbid = %ld\n",nextbid) ;
				return BKT_ERR; // There is no more basket....
			}
		}

		if(!LSEEK_SET(pCtrl->hidx, BKT_transIndexNumToPos(pCtrl->iframe_cur))) {
			TPLAY_DERR("failed seek read next frame. iframe:%d\n", pCtrl->iframe_cur);
			return BKT_ERR;
		}

		// read index info
		if(!READ_ST(pCtrl->hidx, idd)) {
			TPLAY_DERR("failed read index data. go next iframe. idd.id:0x%08lx, cur-iframe:%d, tot-iframe:%d\n", idd.id, pCtrl->iframe_cur, pCtrl->iframe_cnt);
			continue;
		}

		if(idd.ch != ch ) {
			TPLAY_DBG1("different ch --  idd.ch=%d, pCtrl.ch=%ld. BID:%ld, cur-iframe:%d, tot-iframe:%d\n", idd.ch, ch, pCtrl->bid, pCtrl->iframe_cur, pCtrl->iframe_cnt);
			continue;
		}

#if 0
		if(idd.fpos < pCtrl->fpos_bkt) {
			TPLAY_DERR("invalid file pointer. idd.fpos=%ld, pCtrl->fpos_bkt=%ld.\n",
					idd.fpos, pCtrl->fpos_bkt);
			continue;
		}
#endif

		// read basket file
		if(!LSEEK_SET(pCtrl->hbkt, idd.fpos)) {
			TPLAY_DERR("failed seek next index data. i-fpos:%ld\n", idd.fpos);
			continue;
		}

		if(!READ_HDR(pCtrl->hbkt, shd)) {
			TPLAY_DERR("failed read stream header.\n");
			continue;
		}

		if(shd.id != ID_VIDEO_HEADER) {
			TPLAY_DERR("Invalid index data's basket file pointer.maybe broken iframe-index file.\n");
			continue;
		}

		if(shd.ch != ch ) {
			TPLAY_DBG1("different channel ch=%d, shd.ch=%ld.\n", ch, shd.ch);
			continue;
		}

#if 0 // do not need this, because read i-frame only
		if(shd.frametype != FT_IFRAME) {
			TPLAY_DERR("Invalid frame type. should be iframe.index data's basket file pointer. maybe broken iframe-index file.\n");
			continue;
		}
#endif

		if(!READ_HDR(pCtrl->hbkt, vhd)) {
			TPLAY_DERR("failed read video stream header.\n");
			continue;
		}

		if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize )) {
			TPLAY_DERR("failed read video raw data.\n");
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

		pCtrl->sec = shd.ts.sec;
		pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

		TPLAY_DBG0("Read Next I-Frame, ch=%d,  bktID = %d, pCtrl->fpos_bkt=%d\n", ch, pCtrl->bid,pCtrl->fpos_bkt) ;
		dp->bktId    = pCtrl->bid;
		dp->fpos_bkt = pCtrl->fpos_bkt;

		return BKT_OK;

	} // end if(pCtrl->bkt_status == BKTMD_STATUS_OPEN)

	TPLAY_DERR("Not open basket. CH:%d\n", ch);
	return BKT_ERR;
}

long tplay_read_1st_frame_of_next_basket(T_BKTMD_CTRL* pCtrl, T_BKTMD_PARAM *dp)
{
	long nextbid = -1;
	int oldHddidx;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;

	if(BKT_isRecording(pCtrl->bid, pCtrl->target_path)) {
		pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
	}
	
	if(pCtrl->iframe_cur + 1 < pCtrl->iframe_cnt) {
		pCtrl->iframe_cur += 1;
	}
	else { // find next basket
		nextbid = tplay_get_next_bid(pCtrl);
		TPLAY_DBG0("ch:%d, nextbid = %ld\n", pCtrl->ch, nextbid);

		////MULTI-HDD SKSUNG/////
		if( gbkt_mount_info.hddCnt == 1 && nextbid == BKT_ERR ) {
			return BKT_ERR ;
		}

		if(gbkt_mount_info.hddCnt > 1)
		{
			if(pCtrl->bktcnt == pCtrl->bid)
			{
				nextbid 	= 0;
				oldHddidx 	= pCtrl->curhdd;
				
				do{
		        	nextbid = tplay_get_next_bid_hdd(pCtrl->ch, pCtrl->nexthdd);
					if(nextbid != BKT_ERR)
						break;
				}while(pCtrl->nexthdd == oldHddidx);
				
	            if(nextbid == BKT_ERR) return nextbid;
			}
		}

		if( BKT_ERR == tplay_open_next(pCtrl->ch, nextbid))
		{
			TPLAY_DERR("Error nextbid = %ld\n",nextbid);
            return BKT_ERR;
		}
	}

	if(!LSEEK_SET(pCtrl->hidx, BKT_transIndexNumToPos(pCtrl->iframe_cur))) {
		TPLAY_DERR("failed seek read next frame. iframe:%d\n", pCtrl->iframe_cur);
		return BKT_ERR;
	}

	while(pCtrl->bkt_status == BKTMD_STATUS_OPEN)
	{
	    usleep(1);

		// read stream header
		if(!READ_HDR(pCtrl->hbkt, shd))
		{
			TPLAY_DERR("failed read stream header. go next iframe. shd.id:0x%08lx, framesize:%ld, cur-iframe:%d, tot-iframe:%d\n",
			              shd.id, shd.framesize, pCtrl->iframe_cur, pCtrl->iframe_cnt);
			break;
		}

		if(shd.id == ID_VIDEO_HEADER) {

			if(shd.ch != pCtrl->ch ) {  // skip

				if(!LSEEK_CUR(pCtrl->hbkt, sizeof(vhd) + shd.framesize)) {
					TPLAY_DERR("failed seek cur basket.\n");
					break; // search next basket
				}

				if(shd.frametype == FT_IFRAME){

					pCtrl->iframe_cur += 1; // increase total iframe count

					if(pCtrl->iframe_cur > pCtrl->iframe_cnt) {
						TPLAY_DBG0("over iframe. ch=%d, cur-iframe:%d, tot-iframe:%d, fpos=%ld\n",
								pCtrl->ch, pCtrl->iframe_cur, pCtrl->iframe_cnt, pCtrl->fpos_bkt);
						break;
					}

					// if It using current recording file, recalculate index count
					if( BKT_isRecording(pCtrl->bid, pCtrl->target_path))
						pCtrl->iframe_cnt = BKT_GetRecordingIndexCount();
				}

				pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);
				continue;

			} // end skip

			// read video header
			if(!READ_HDR(pCtrl->hbkt, vhd)) {
				TPLAY_DERR("failed read video stream header.\n");
				break;
			}

			// read video frame
			if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize )) {
				TPLAY_DERR("failed read video raw data.\n");
				break;
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

			pCtrl->sec = shd.ts.sec;
			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

			//printf("Read Next First Frame bktID = %d, pCtrl->fpos_bkt=%d\n",pCtrl->bid,pCtrl->fpos_bkt) ;
			// save new basket id
			dp->bktId    = pCtrl->bid;
			dp->fpos_bkt = pCtrl->fpos_bkt;

			return BKT_OK;

		}
		else if (shd.id == ID_AUDIO_HEADER){

			if(shd.ch != pCtrl->ch ) {  // skip

				if(!LSEEK_CUR(pCtrl->hbkt, sizeof(ahd) + shd.framesize)) {
					TPLAY_DERR("failed seek cur basket.\n");
					break; // search next basket
				}

				pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);
				continue;
			}

			if(!READ_HDR(pCtrl->hbkt, ahd))
			{
				TPLAY_DERR("failed read audio header data\n");
				break;
			}

			// read audio frame
			if(!READ_PTSZ(pCtrl->hbkt, dp->vbuffer, shd.framesize))
			{
				TPLAY_DERR("failed read audio data\n");
				break;
			}

			dp->ch		    = shd.ch;
			dp->ts.sec      = shd.ts.sec;
			dp->ts.usec     = shd.ts.usec;
			dp->streamtype  = ST_AUDIO;
			dp->framesize   = shd.framesize;
			dp->samplingrate= ahd.samplingrate;
			dp->fpos = 0;

			pCtrl->sec = shd.ts.sec; // AYK - 1215
			pCtrl->fpos_bkt = LTELL(pCtrl->hbkt);

			// save new basket id
			dp->bktId    = pCtrl->bid;
			dp->fpos_bkt = pCtrl->fpos_bkt;

			return BKT_OK;

		} //end else if (shd.id == ID_AUDIO_HEADER)
		else {
			TPLAY_DERR("Invalid shd.id=0x%08X\n\n", shd.id);
		    break;
		}

	}// end while

	return tplay_read_next_iframe(pCtrl->ch, dp);
}

long tplay_get_target_disk(int ch, long sec)
{
	BKT_FHANDLE fd;
	BKT_FHANDLE fd_bkt;

	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

    int i = 0 , j = 0, searchmin = 0, hddIdx = -1;
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

	        if(!OPEN_RDONLY(fd, fullpath)) {
	            continue;
			}

			offset = TOTAL_MINUTES_DAY*ch;

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

			// forward search..... until end of day....as minute
			for(i = searchmin; i < TOTAL_MINUTES_DAY; i++)
			{
				if(0 != (int)inRecHourTBL[i])
				{
					hddIdx = j ;
					CLOSE_BKT(fd);
					break ;
				}
			}

			memset(diskpath, 0, 30) ;

			if(hddIdx >= 0) 
			{
				sprintf(diskpath, "%s/%s", gbkt_mount_info.hddInfo[hddIdx].mountDiskPath, NAME_BKTMGR) ;

				if(!OPEN_RDWR(fd_bkt, diskpath)) // open bktmgr.udf file
		        {
		            TPLAY_DERR("failed open basket manager file in decoding. %s\n", diskpath);
		            return BKT_ERR;
		        }

		        if(!READ_ST(fd_bkt, hd))
		        {
		            TPLAY_DERR("failed read basket manager header in decoding.\n");
		            CLOSE_BKT(fd_bkt);
		            return BKT_ERR;
		        }

				CLOSE_BKT(fd_bkt) ;
			
				pCtrl->nexthdd 	= gbkt_mount_info.hddInfo[hddIdx].nextHddIdx;
				pCtrl->curhdd 	= hddIdx;
				pCtrl->bktcnt 	= hd.bkt_count ;

				sprintf(pCtrl->target_path,       "%s", gbkt_mount_info.hddInfo[hddIdx].mountDiskPath);

				TPLAY_DBG0("set multi playback target disk. ch:%d, path:%s\n", ch, pCtrl->target_path);

				return TRUE ;


			} //if(hddIdx >= 0) 

   		} //if(gbkt_mount_info.hddInfo[j].isMounted)

    } //for(j = 0; j < MAX_HDD_COUNT; j++)

	TPLAY_DERR("Failed set multi playback target disk. ch:%d, path:%s\n", ch, pCtrl->target_path);

	return FALSE;
}

long tplay_get_next_bid_hdd(int ch, long curhdd)
{
    BKT_FHANDLE fd;
	T_BKTMGR_HDR  hd;
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	char diskpath[30] ;
	long firstBid = 0;

	//////MULTI-HDD SKSUNG/////
	pCtrl->curhdd  = curhdd;
	pCtrl->nexthdd = gbkt_mount_info.hddInfo[curhdd].nextHddIdx;
	
	firstBid = BKTMD_SearchNextHddStartBID(gbkt_mount_info.hddInfo[curhdd].mountDiskPath, pCtrl->sec);
	if(firstBid == BKT_ERR)
		return firstBid;	//not founed playback bid.
	
	sprintf(diskpath, "%s/%s", gbkt_mount_info.hddInfo[curhdd].mountDiskPath, NAME_BKTMGR) ;

    if(!OPEN_RDONLY(fd, diskpath)) // open bktmgr.udf file
    {
        TPLAY_DERR("failed open basket manager file in decoding. %s\n", diskpath);
        return BKT_ERR;
    }

    if(!READ_ST(fd, hd))
    {
        TPLAY_DERR("failed read basket manager header in decoding.\n");
        CLOSE_BKT(fd);
        return BKT_ERR;
    }
	CLOSE_BKT(fd);

	pCtrl->bktcnt = hd.bkt_count;

	sprintf(pCtrl->target_path, "%s", gbkt_mount_info.hddInfo[curhdd].mountDiskPath );

	return firstBid;	//found first  playback bid
}

long tplay_get_prev_bid(T_TPLAY_CTRL* pCtrl)
{
	long sec_seed = pCtrl->sec;
	long bts=0,ets=0;
	int curr_rdb;
	int prev_rdb=0;
	T_BKT_TM tm1;
    BKT_FHANDLE fd;
	char buf[30];
    int nitems, i, m;
    struct dirent **ents;

	T_BASKET_RDB rdb[TOTAL_MINUTES_DAY];

	if(sec_seed <= 0) {
		if(BKT_ERR == tplay_get_basket_time(pCtrl, &bts, &ets))
		{
			TPLAY_DERR("There is no a prev basekt. BID:%ld is recording.\n", pCtrl->bid);
			return BKT_ERR;
		}
		sec_seed = ets;
	}

	BKT_GetLocalTime(sec_seed, &tm1);

	sprintf(buf, "%s/rdb/", pCtrl->target_path);

    nitems = scandir(buf, &ents, NULL, alphasort_reverse);
    
	if(nitems < 0) {
	    perror("scandir reverse");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TPLAY_DBG0("There is no rdb directory %s. ch=%d\n", pCtrl->ch, buf);
		free(ents); // BKKIM 20111031
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	curr_rdb=atoi(buf);
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
			sprintf(buf, "%s/rdb/%s", pCtrl->target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				TPLAY_DERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_prev_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL+sizeof(T_BASKET_RDB)*TOTAL_MINUTES_DAY*pCtrl->ch))
			{
				TPLAY_DERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}

			if(!READ_PTSZ(fd, rdb, sizeof(rdb)))
			{
				TPLAY_DERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_prev_bid;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min-1;m>=0;m--)
			{
				if(rdb[m].bid != 0 && rdb[m].bid != pCtrl->bid)
				{
				    // free scandir's entry
					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					TPLAY_DBG1("found prev BID:%ld, channel:%d\n", rdb[m].bid, pCtrl->ch);

					return rdb[m].bid;
					//return nbid; // found!!
				}
			}

			// in case of First Fail
			tm1.hour = 23;
			tm1.min  = 60;
		}

		// free scandir's entry
		free(ents[i]);

    } // end for

lbl_err_search_prev_bid:

	// free scandir' entry
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	TPLAY_DBG0("Can't find prev basket. CUR-BID:%ld, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n",
                                pCtrl->bid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found

}

long tplay_get_prev_bid_hdd(int ch, long curhdd)
{
    BKT_FHANDLE fd;
	T_BKTMGR_HDR  hd;
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	char diskpath[30] ;
	long firstBid = 0;

	//////MULTI-HDD SKSUNG/////
	pCtrl->curhdd  = curhdd;
	pCtrl->nexthdd = gbkt_mount_info.hddInfo[curhdd].nextHddIdx;
	
	firstBid = BKTMD_SearchPrevHddStartBID(gbkt_mount_info.hddInfo[curhdd].mountDiskPath, pCtrl->sec);
	if(firstBid == BKT_ERR)
		return firstBid;	//not founed playback bid.
	
	// open basket manager file(bktmgr.udf)
	sprintf(diskpath, "%s/%s", gbkt_mount_info.hddInfo[curhdd].mountDiskPath, NAME_BKTMGR) ;
    if(!OPEN_RDONLY(fd, diskpath))
    {
        TPLAY_DERR("failed open basket manager file in decoding. %s\n", diskpath);
        return BKT_ERR;
    }

	// read bktmgr.udf header
    if(!READ_ST(fd, hd))
    {
        TPLAY_DERR("failed read basket manager header in decoding.\n");
        CLOSE_BKT(fd);
        return BKT_ERR;
    }
	CLOSE_BKT(fd);

	// set new value 
	pCtrl->bktcnt = hd.bkt_count;
	sprintf(pCtrl->target_path, "%s", gbkt_mount_info.hddInfo[curhdd].mountDiskPath );

	return firstBid;	//found first  playback bid
}

long tplay_get_next_bid(T_TPLAY_CTRL* pCtrl)
{
	long sec_seed = pCtrl->sec;
	long bts=0,ets=0;
	int curr_rdb;
	int next_rdb=0;
	T_BKT_TM tm1;
    BKT_FHANDLE fd;
	char buf[30];
    int nitems, i, m;
    struct dirent **ents;

	T_BASKET_RDB rdb[TOTAL_MINUTES_DAY];

	if(BKT_isRecording(pCtrl->bid, pCtrl->target_path))
	{
		TPLAY_DERR("There is no a next basekt. BID:%ld is recording.\n", pCtrl->bid);
		return BKT_ERR;
	}

	if(BKT_ERR == tplay_get_basket_time(pCtrl, &bts, &ets))
	{
		TPLAY_DERR("There is no a next basekt. BID:%ld is recording.\n", pCtrl->bid);
		return BKT_ERR;
	}

	if(pCtrl->sec < ets)
		sec_seed = ets;
	//TPLAY_DBG0("last play time is zero. getting last time stamp from basket. CH:%02d, BID:%ld, BEGIN_SEC:%ld, END_SEC:%ld, DURATION\n", ch, curbid, bts, ets, ets-bts);

	BKT_GetLocalTime(sec_seed, &tm1);

	sprintf(buf, "%s/rdb/", pCtrl->target_path);

    nitems = scandir(buf, &ents, NULL, alphasort);
    
	if(nitems < 0) {
	    perror("scandir");
		return BKT_ERR;
	}

	if(nitems == 0)
	{
		TPLAY_DBG0("There is no rdb directory %s. ch=%d\n", pCtrl->ch, buf);
		free(ents); // BKKIM 20111031
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	curr_rdb=atoi(buf);
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
			sprintf(buf, "%s/rdb/%s", pCtrl->target_path, ents[i]->d_name);

			if(!OPEN_RDONLY(fd, buf))
			{
				TPLAY_DERR("failed open rdb file. %s\n", buf);
				goto lbl_err_search_next_bid;
			}

			if(!LSEEK_SET(fd, TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL+sizeof(T_BASKET_RDB)*TOTAL_MINUTES_DAY*pCtrl->ch))
			{
				TPLAY_DERR("failed seek\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}

			if(!READ_PTSZ(fd, rdb, sizeof(rdb)))
			{
				TPLAY_DERR("failed read rdbs data\n");
				CLOSE_BKT(fd);
				goto lbl_err_search_next_bid;
			}
			CLOSE_BKT(fd);

			for(m=tm1.hour*60+tm1.min+1;m<TOTAL_MINUTES_DAY;m++)
			{
				if(rdb[m].bid != 0 && rdb[m].bid != pCtrl->bid)
				{
				    // free scandir's entry
					for(;i<nitems;i++) {
						free(ents[i]);
					}
					free(ents);

					TPLAY_DBG1("found next BID:%ld, channel:%d\n", rdb[m].bid, pCtrl->ch);

					return rdb[m].bid;
					//return nbid; // found!!
				}
			}

			// in case of First Fail
			tm1.hour = 0;
			tm1.min  = -1;
		}

		// free scandir's entry
		free(ents[i]);

    } // end for

lbl_err_search_next_bid:

	// free scandir' entry
    for(;i<nitems;i++) {
		free(ents[i]);
	}
	free(ents);

	TPLAY_DBG0("Can't find next basket. CUR-BID:%ld, CUR-TS:%04d-%02d-%02d %02d:%02d:%02d\n",
                                pCtrl->bid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);

	return BKT_ERR;// not found

}

long tplay_get_basket_time(T_BKTMD_CTRL* pCtrl, long *o_begin_sec, long *o_end_sec)
{
	char path[30];
	T_BASKET_HDR bhdr;

    BKT_FHANDLE fd;

	sprintf(path, "%s/%08ld.bkt", pCtrl->target_path, pCtrl->bid);
	TPLAY_DBG1("path = %s\n", path);

	// open basket
	if(!OPEN_RDONLY(fd, path))
	{
		TPLAY_DBG1("failed open basket file -- path:%s\n", path);
		return BKT_ERR;
	}

	if(!READ_ST(fd, bhdr))
	{
		TPLAY_DBG1("failed read basket header.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	*o_begin_sec = BKT_GetMinTS(bhdr.ts.t1);
	*o_end_sec   = BKT_GetMaxTS(bhdr.ts.t2);

	TPLAY_DBG1("BID:%ld, B_SEC:%ld, E_SEC:%ld, Duration:%ld\n", pCtrl->bid, *o_begin_sec,  *o_end_sec, *o_end_sec - *o_begin_sec);

	return BKT_OK;
}

int  tplay_is_open(){
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	return (pCtrl->bkt_status == BKTMD_STATUS_OPEN);
}

long tplay_get_cur_play_sec(int ch)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

    return pCtrl->sec;
}

int tplay_get_bsk_info(long *bid, long *fpos, int *curhdd, int *nexthdd)
{
	T_BKTMD_CTRL* pCtrl = &gTPLAY_Ctrl;

	if(pCtrl->bkt_status == BKTMD_STATUS_OPEN){
	    *bid = pCtrl->bid;
	    *fpos = pCtrl->fpos_bkt;
	    *curhdd  = pCtrl->curhdd;
	    *nexthdd = pCtrl->nexthdd;

		return BKT_OK;
	}

	return BKT_ERR;

}


VdecVdis_Config      gVdecVdis_config;
VdecVdis_IpcBitsCtrl gVdecVdis_obj;

Int32 VdecVdis_bitsRdInit()
{
    memset(&gVdecVdis_obj   , 0, sizeof(gVdecVdis_obj));
    memset(&gVdecVdis_config, 0, sizeof(gVdecVdis_config));

    int resId, width, height, i;
   // int found = FALSE;

    width  = 720;
    height = 576;

#if 1
    gVdecVdis_config.numRes = 1;
    gVdecVdis_config.fileNum = 16;

    for(resId=0;resId<gVdecVdis_config.numRes;resId++){
    	gVdecVdis_config.res[resId].width  = width;
    	gVdecVdis_config.res[resId].height = height;

        gVdecVdis_config.numChnlInRes[resId] = gVdecVdis_config.fileNum;

        for (i = 0; i < gVdecVdis_config.fileNum; i++) {
			gVdecVdis_config.resToChnl[resId][i] = i;
        }
    }

    gVdecVdis_config.gDemo_TrickPlayMode.speed = 0;
    gVdecVdis_config.gDemo_TrickPlayMode.channel = 0;

    for (i = 0; i < MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI; i++)
    {
        if(gVdecVdis_obj.fpRdHdr[i]!=NULL)
        {
            fclose(gVdecVdis_obj.fpRdHdr[i]);
            gVdecVdis_obj.fpRdHdr[i] = NULL;
        }
        if(gVdecVdis_obj.fdRdData[i] > 0)
        {
            close(gVdecVdis_obj.fdRdData[i]);
            gVdecVdis_obj.fdRdData[i] = -1;
        }
    }

#else
    gVdecVdis_config.numRes = 0;
    for (ChId = 0; ChId < MCFW_IPC_BITS_MAX_RES_NUM; ChId++) {


		gVdecVdis_config.res[ChId].width = width;
		gVdecVdis_config.res[ChId].height = height;
		gVdecVdis_config.numRes++;
		gVdecVdis_config.resToChnl[ChId][gVdecVdis_config.numChnlInRes[ChId]] = ChId;
		gVdecVdis_config.numChnlInRes[ChId]++;

		found = FALSE;

		for (resId = 0; resId < gVdecVdis_config.numRes; resId++) {
			if (( width  == gVdecVdis_config.res[resId].width)
			&& (  height == gVdecVdis_config.res[resId].height)) {
				found = TRUE;
				break;
			}
		}

		if (!found) {
			gVdecVdis_config.res[resId].width = width;
			gVdecVdis_config.res[resId].height = height;

			gVdecVdis_config.numRes++;
		}

		gVdecVdis_config.numChnlInRes[resId]++;
		gVdecVdis_config.resToChnl[resId][gVdecVdis_config.numChnlInRes[resId]-1] = ChId;
	}
#endif

    //VdecVdis_bitsRdInitThrObj();

    return OSA_SOK;
}

Void VdecVdis_bitsRdGetEmptyBitBufs(VCODEC_BITSBUF_LIST_S *emptyBufList, UInt32 resId)
{
    VDEC_BUF_REQUEST_S reqInfo;
    UInt32 bitBufSize;
    UInt32 i;

    emptyBufList->numBufs = 0;

    // require 2 buffers for each channel
    reqInfo.numBufs = gVdecVdis_config.numChnlInRes[resId] ;
	reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
    bitBufSize = MCFW_IPCBITS_GET_BITBUF_SIZE(
                    gVdecVdis_config.res[resId].width,
                    gVdecVdis_config.res[resId].height
                    );

    for (i = 0; i < reqInfo.numBufs ; i++)
    {
        reqInfo.u[i].minBufSize = bitBufSize;
    }

    Vdec_requestBitstreamBuffer(&reqInfo, emptyBufList, 0);
}

UInt32 VdecVdis_getChnlIdFromBufSize(UInt32 resId)
{
    UInt32 channel;
    UInt32 lastId;
    static UInt16 tempSkipCount = 1;
    static UInt16 tempSkipFlag = 0;
    lastId = gVdecVdis_obj.lastId[resId];

    // get channel ID
    channel = gVdecVdis_config.resToChnl[resId][lastId];
    if (channel == gVdecVdis_config.gDemo_TrickPlayMode.channel){
        if (tempSkipCount++ < gVdecVdis_config.gDemo_TrickPlayMode.speed) {
            tempSkipFlag = 1;
        }
        else {
            tempSkipFlag = 0;
            tempSkipCount = 1;
        }
    }
    else {
        tempSkipFlag = 0;
    }

    if (tempSkipFlag == 0) {
        lastId ++;
    }

    if (lastId >= gVdecVdis_config.numChnlInRes[resId])
    {
        lastId = 0;
    }

    gVdecVdis_obj.lastId[resId] = lastId;

    OSA_assert(channel < gVdecVdis_config.fileNum);

    return channel;

}

static void VdecVdis_bitsRdFillEmptyBuf(VCODEC_BITSBUF_S *pEmptyBuf, UInt32 resId)
{
    //int statHdr, statData;
    int curCh;

    curCh = VdecVdis_getChnlIdFromBufSize(resId);

    // TODO - check handle.....
    // ...

    pEmptyBuf->chnId    = curCh;

    // TODO - read data size
    // statHdr = fscanf(gVdecVdis_obj.fpRdHdr[curCh],"%d",&(pEmptyBuf->filledBufSize));

    OSA_assert(pEmptyBuf->filledBufSize <= pEmptyBuf->bufSize);

    // TODO - read raw data
    // statData = read(gVdecVdis_obj.fdRdData[curCh], pEmptyBuf->bufVirtAddr, pEmptyBuf->filledBufSize);
}

Void VdecVdis_bitsRdReadData(VCODEC_BITSBUF_LIST_S  *emptyBufList,UInt32 resId)
{
    VCODEC_BITSBUF_S *pEmptyBuf;
    Int i;

    for (i = 0; i < emptyBufList->numBufs; i++)
    {
        pEmptyBuf = &emptyBufList->bitsBuf[i];
        VdecVdis_bitsRdFillEmptyBuf(pEmptyBuf, resId);
    }
}

Void VdecVdis_bitsRdSendFullBitBufs( VCODEC_BITSBUF_LIST_S *fullBufList)
{
    if (fullBufList->numBufs)
    {
        Vdec_putBitstreamBuffer(fullBufList);
    }
}

void  VdecVdis_setTplayConfig(VDIS_CHN vdispChnId, VDIS_AVSYNC speed)
{
    gVdecVdis_config.gDemo_TrickPlayMode.channel= vdispChnId;

#if 1
    if (speed == VDIS_AVSYNC_2X || speed == VDIS_AVSYNC_4X) {
        gVdecVdis_config.gDemo_TrickPlayMode.speed = speed;
    }
    else {
        gVdecVdis_config.gDemo_TrickPlayMode.speed = 1;
    }
#else
	gVdecVdis_config.gDemo_TrickPlayMode.speed = speed;
#endif
}



// EOF
