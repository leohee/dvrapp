#include <unistd.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <sys/time.h>

#include <sys/types.h> // search target storage
#include <sys/stat.h>
#include <fcntl.h>

#include "udbasket.h"
#include "basket_muldec.h"
#include <app_bskbackup.h>
#include <app_manager.h>
#include "app_util.h"

BACKUP_Ctrl gBackupCtrl;
int BackupTsk_Running = TRUE;



//////////////////////////////////////////////////////////
//#define BACKUP_DEBUG

#ifdef BACKUP_DEBUG
#define TRACE_BACKUP(msg, args...)  printf("[BACKUP] - " msg, ##args)
#define TRACEF_BACKUP(msg, args...) printf("[BACKUP] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE_BACKUP(msg, args...) ((void)0)
#define TRACEF_BACKUP(msg, args...) ((void)0)
#endif
//////////////////////////////////////////////////////////


static void send_message_bskbackup_to_qt(int which, int data, void *pData)
{
	BACKUP_Ctrl *gCtrl = &gBackupCtrl ;
    DVR_MSG_T   msg;

    if (gCtrl->msg_pipe_id == NULL)
        return;

    msg.which = which;
    msg.data = data;
    msg.pData = pData;

    OSA_WriteToPipe(gCtrl->msg_pipe_id, &msg, sizeof(DVR_MSG_T), OSA_SUSPEND);
}



#ifdef BKT_SYSIO_CALL
long BACKUP_GetIndexCount(int fd)
#else
long BACKUP_GetIndexCount(FILE *fd)
#endif
{
	if(!LSEEK_END(fd,0))
	{
		TRACEF_BACKUP("failed seek index end point\n");
		return 0;
	}

	long fpos = LTELL(fd);
	
	return  (fpos-sizeof(T_INDEX_HDR))/sizeof(T_INDEX_DATA);
}


int BACKUP_updateRDB(T_BASKET_RDB_PARAM *pm, int pathcnt)
{
	long offset;
	char evts[TOTAL_MINUTES_DAY*MAX_CH];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*MAX_CH];
	char path[LEN_PATH];

	BKT_FHANDLE fd;
	long sec_seed = pm->sec;
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec_seed, &tm1);
	
//	sprintf(path, "%s/rdb/%04d%02d%02d", gBackupCtrl.dst_path, tm1.year, tm1.mon, tm1.day);	
	sprintf(path, "%s/hdd%d/rdb/%04d%02d%02d", gBackupCtrl.dst_path, pathcnt, tm1.year, tm1.mon, tm1.day);	

	if(-1 != access(path, 0)) // existence only
	{
		if(!OPEN_RDWR(fd, path)) // open rdb file.
		{
			TRACEF_BACKUP("failed open rdb file:%s\n", path);
			return 0;
		}

		if(!READ_PTSZ(fd, evts, sizeof(evts)))
		{
			TRACEF_BACKUP("failed read evts data\n");
			CLOSE_BKT(fd);
			return 0;
		}

		if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
		{
			TRACEF_BACKUP("failed read rdb data\n");
			CLOSE_BKT(fd);
			return 0;
		}
	}
	else// new one
	{
		char rdb_path[LEN_PATH];
		
		memset(evts,0,sizeof(evts));		
		memset(rdbs,0,sizeof(rdbs));
		
		sprintf(rdb_path, "%s/hdd%d/rdb", gBackupCtrl.dst_path, pathcnt);
		if(-1 == access(rdb_path, 0))
		{
			if( 0 != mkdir(rdb_path, 0755))
			{
				TRACEF_BACKUP("failed create rdb directory. %s\n", rdb_path);
				return 0;
			}
		}

		if(!OPEN_CREATE(fd, path))
		{
			TRACE_BACKUP("failed create rdb file:%s\n", path);
			return 0;
		}
	}

	// write rdb
	offset = TOTAL_MINUTES_DAY*pm->ch+(tm1.hour*60+tm1.min);

	if(rdbs[offset].bid == 0)
	{
		evts[offset]         = pm->evt;
		rdbs[offset].bid     = pm->bid;
		rdbs[offset].idx_pos = pm->idx_pos;
		
		if(!LSEEK_SET(fd, 0))
		{
			TRACE_BACKUP("failed seek BEGIN.\n");
			CLOSE_BKT(fd);
			return 0;
		}
		
		// write evts
		if(!WRITE_PTSZ(fd, evts, sizeof(evts)))
		{
			TRACE_BACKUP("failed write events.\n");
			CLOSE_BKT(fd);
			return 0;
		}
		
		// write rdbs
		if(!WRITE_PTSZ(fd, rdbs, sizeof(rdbs)))
		{
			TRACE_BACKUP("failed write rdbs\n");
			CLOSE_BKT(fd);
			return 0;
		}
		
		TRACE_BACKUP("update rdb CH:%d, BID:%ld, TS:%04d-%02d-%02d %02d:%02d:%02d, idxpos:%ld\n", pm->ch, pm->bid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec, pm->idx_pos);
		CLOSE_BKT(fd);
		return 1;
	}

	CLOSE_BKT(fd);
	TRACE_BACKUP("skip rdb CH:%d, BID:%ld, TS:%04d-%02d-%02d %02d:%02d:%02d, idxpos:%ld\n", pm->ch, pm->bid, tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec, pm->idx_pos);

	return 1;	
}

int BACKUP_tskMain(struct OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg, Uint32 curState)
{
	int ackMsg=FALSE;
	int status = 0, ret;
	int loop = 0 ;
	long end_sec = 0, t1, t2 ;

	char backup_rbuffer[100*1024]; // read buffer
	char backup_wbuffer[100*1024]; // write buffer

	// open bktmgr
	T_BKTMGR_HDR  mhdr;
	T_BKTMGR_BODY mbd;
	char path[LEN_PATH];

#ifdef BKT_SYSIO_CALL
	int fp_mgr=0;
	int fp_wbkt=0, fp_widx=0;
	int fp_rbkt=0, fp_ridx=0;
#else
	FILE *fp_mgr=NULL;
	FILE *fp_wbkt=NULL, *fp_widx=NULL;
	FILE *fp_rbkt=NULL ,*fp_ridx=NULL;
#endif
	

	FILE *fd ;

	char **bskbuf ;
    char tempbuf[100][30] ;
    char namebuf[30] ;
	char Tempbackup[30] ;

	int basket_number=0 , j = 0, i = 0;

	//////////////////////////////////////
	time_t cur_sec;
	int fcount=0;
	int bFirstIFrame=0;
	T_BASKET_HDR wbhd,rbhd;
	T_STREAM_HDR shd;
	T_VIDEO_STREAM_HDR vhd;
	T_AUDIO_STREAM_HDR ahd;
	
	T_BASKET_TIME bkt_duration;
	T_BASKET_TIME idx_duration;
	long prev_min[MAX_CH];

	memset(&bkt_duration, 0, sizeof(bkt_duration));
	memset(&idx_duration, 0, sizeof(idx_duration));
	memset(prev_min, -1, sizeof(prev_min));


	bskbuf = (char**)malloc(sizeof(char*)*100) ;
    if(bskbuf == NULL)
        TRACEF_BACKUP("malloc fail...\n") ;

    for(i = 0; i < 100; i++)
    {
        bskbuf[i] = (char*)malloc(sizeof(char)*60) ;
    }

	//////////////////////////////////////////////////////////////////////////

	Uint16 cmd = OSA_msgGetCmd(pMsg);
	BACKUP_CreatePrm *pCreatePrm = (BACKUP_CreatePrm*)OSA_msgGetPrm(pMsg);

	if( cmd != BACKUP_CMD_CREATE || pCreatePrm==NULL)
	{
		OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
		return OSA_SOK;
	}

	TRACE_BACKUP("Sending ACK BACKUP_CMD_CREATE (%d) ======> \n", status);

	OSA_tskAckOrFreeMsg(pMsg, status);



add_bkt :

	if(loop == 1)
	{
		fcount=0;
		bFirstIFrame=0;

		memset(&bkt_duration, 0, sizeof(bkt_duration));
    	memset(&idx_duration, 0, sizeof(idx_duration));
    	memset(prev_min, -1, sizeof(prev_min)) ;

		gBackupCtrl.pathcount = 1 ;	
	}
	sprintf(path, "%s/hdd%d", gBackupCtrl.dst_path, loop) ;
	mkdir(path, O_RDWR) ;	
	
	sprintf(path, "%s/hdd%d/bktmgr.udf", gBackupCtrl.dst_path, loop);
	

	if(-1 != access(path, 0)) // existence only
	{
		if(!OPEN_RDWR(fp_mgr, path)) // open manager file
		{
			TRACE_BACKUP("failed open dst basket_manager. %s\n", path);
			gBackupCtrl.status  = BACKUP_STATUS_ERROR;
			gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
			
			goto backup_tsk_error;
		}
		
		ret = READ_STRC(fp_mgr, mhdr);
		if(ret == 0) // EOF
		{
			TRACE_BACKUP("basket manager file is EOF.\n");

			if(!LSEEK_SET(fp_mgr, 0))
			{
				TRACEF_BACKUP("faile seek begin\n");
				CLOSE_BKT(fp_mgr);
				goto backup_tsk_error;
			}

			mhdr.id = ID_BKTMGR_HEADER;
			mhdr.mgrid = 0; // must have new ID for using
		    time(&cur_sec);
			mhdr.date=cur_sec;
			mhdr.latest_update = mhdr.date;
			mhdr.bkt_count = 0;
			mhdr.reserved = 0;
			
			if(!WRITE_ST(fp_mgr, mhdr))
			{
				TRACEF_BACKUP("faile write bktmgr hdr\n");
				CLOSE_BKT(fp_mgr);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
				
				goto backup_tsk_error;	
			}
		}
		else if ( ret < 0)
		{
			TRACE_BACKUP("failed read basket manager header\n");
			CLOSE_BKT(fp_mgr);

			gBackupCtrl.status = BACKUP_STATUS_ERROR;
			gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
			
			goto backup_tsk_error;
		}
		
		basket_number = mhdr.bkt_count + 1;
		
		TRACE_BACKUP("update basket count :%ld\n", mhdr.bkt_count);
	}
	else // create new bktmgr
	{
		if(!OPEN_CREATE(fp_mgr, path))
		{
			TRACE_BACKUP("failed create dst basket manager. %s\n", path);
			gBackupCtrl.status = BACKUP_STATUS_ERROR;
			gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
			
			goto backup_tsk_error;
		}

	    mhdr.id = ID_BKTMGR_HEADER;
	    mhdr.mgrid = 0; // must have new ID for using
		time(&mhdr.date);
		//time(&cur_sec);
		//mhdr.date = cur_sec;
	    mhdr.latest_update = cur_sec;
	    mhdr.bkt_count = 0;
	    mhdr.reserved = 0;
	    
	    if(!WRITE_ST(fp_mgr, mhdr))
	    {
			TRACEF_BACKUP("faile write bktmgr hdr\n");
			CLOSE_BKT(fp_mgr);
			gBackupCtrl.status = BACKUP_STATUS_ERROR;
			gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
			
			goto backup_tsk_error;	
	    }
	    
	    basket_number = mhdr.bkt_count + 1;
	    
	    TRACE_BACKUP("create basket count :%ld\n", mhdr.bkt_count);
	}

	memcpy(gBackupCtrl.ch, pCreatePrm->ch, sizeof(gBackupCtrl.ch));

	
	if(loop == 0)
	{
		gBackupCtrl.t1 = pCreatePrm->t1;
		gBackupCtrl.t2 = pCreatePrm->t2;
	}
	

	gBackupCtrl.status = BACKUP_STATUS_RUNNING;
	BackupTsk_Running = TRUE;

	while(BackupTsk_Running)
	{
		// main loop
		if(fp_rbkt)
		{
			if(READ_ST(fp_rbkt, shd))
			{
				// ready exit.
				if(gBackupCtrl.fpos_cur >= gBackupCtrl.fpos_end)
				{
					TRACE_BACKUP("read done source basket.fpos_cur:%ld, fpos_end:%ld\n", gBackupCtrl.fpos_cur, gBackupCtrl.fpos_end);

					// write basket manager header
					mhdr.bkt_count += 1;
				    time(&mhdr.latest_update);
				    
				    if(!LSEEK_SET(fp_mgr, 0))
					{
						TRACEF_BACKUP("failed seek basket mgr hdr\n");
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						CLOSE_BKT(fp_mgr);
						break;
					}

				    if(!WRITE_ST(fp_mgr, mhdr))
				    {
						TRACEF_BACKUP("failed write basket mgr hdr\n");
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						CLOSE_BKT(fp_mgr);
						break;
				    }
	
					//wirte basket mangaer body.
					memset(&mbd, 0, sizeof(mbd));
					
					mbd.id		  = ID_BKTMGR_BODY;
					mbd.bid       = wbhd.bid;
					mbd.save_flag = SF_BACKUP;
					time(&mbd.latest_update);
					memcpy(&mbd.ts, &bkt_duration, sizeof(bkt_duration));
					
//					sprintf(mbd.path, "%s", gBackupCtrl.dst_path);
					sprintf(mbd.path, "%s/hdd%d", gBackupCtrl.dst_path, loop);
					
					if(!LSEEK_END(fp_mgr, 0))
					{
						TRACEF_BACKUP("failed seek basket mgr.\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						break;
					}
					if(!WRITE_ST(fp_mgr, mbd)) 
					{
						TRACEF_BACKUP("failed write basket mgr body\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						break;
					}
					TRACE_BACKUP("close dst mgr file.\n");
					CLOSE_BKT(fp_mgr);
	
					// close index file
					T_INDEX_HDR idx_hdr;
					idx_hdr.id    = ID_INDEX_HEADER;
					idx_hdr.count = BACKUP_GetIndexCount(fp_widx);
					idx_hdr.bid   = wbhd.bid;
					memcpy(&idx_hdr.ts, &idx_duration, sizeof(idx_duration));
					
					if(!LSEEK_SET(fp_widx, 0))
					{
						TRACEF_BACKUP("failed seek index header.\n");
						CLOSE_BKT(fp_widx);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
						break;
					}

					if(!WRITE_ST(fp_widx, idx_hdr))
					{
						TRACE_BACKUP("failed write index header\n");
						CLOSE_BKT(fp_widx);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
						break;
					}
					TRACE_BACKUP("close dst index file. index count :%ld. 1] \n", idx_hdr.count);
					CLOSE_BKT(fp_widx);
	
					// close dst basket.
					TRACE_BACKUP("close dst basket. BID:%08ld\n", wbhd.bid);
					CLOSE_BKT(fp_wbkt);
			
					// close src basket
					TRACE_BACKUP("close src basket. count:%d\n", fcount);
					CLOSE_BKT(fp_rbkt);
#ifdef BKT_SYSIO_CALL
					fp_rbkt=0;
#else
					fp_rbkt=NULL;
#endif
					gBackupCtrl.status = BACKUP_STATUS_DONE;
								
					break;
				}
				else // making backup...
				{
					if(gBackupCtrl.ch[shd.ch] == 1 && (shd.ts.sec >= gBackupCtrl.t1))
					{
						if(shd.id == ID_VIDEO_HEADER)
						{
							if(!READ_ST(fp_rbkt, vhd))
							{
								TRACEF_BACKUP("failed read video header\n");
								CLOSE_BKT(fp_rbkt);

								gBackupCtrl.status  = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
								break;
							}
							
							if(bFirstIFrame == 0 && shd.frametype == FT_IFRAME)
							{
								bFirstIFrame = 1;
								TRACE_BACKUP("found first I-Frames. CH:%ld\n", shd.ch);
							}
							
							if(bFirstIFrame)
							{
								TRACEF_BACKUP("write video stream\n");
								long cur_sec = shd.ts.sec;
								struct tm tm1;
								localtime_r(&cur_sec, &tm1);
								int cur_min = tm1.tm_min;

								// read frame data
								if(!READ_PTSZ(fp_rbkt, backup_rbuffer, shd.framesize))
								{
									TRACE_BACKUP("failed read normal video frame.\n");
									CLOSE_BKT(fp_rbkt);

									gBackupCtrl.status = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
									break;
								}
								
								T_INDEX_DATA idd;
								idd.fpos = LTELL(fp_wbkt);
								
								//write video stream...
								memcpy(backup_wbuffer, &shd, SIZEOF_STREAM_HDR);
								memcpy(backup_wbuffer+SIZEOF_STREAM_HDR, &vhd, SIZEOF_VSTREAM_HDR);
								memcpy(backup_wbuffer+SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR, backup_rbuffer, shd.framesize);
							
								if(!WRITE_PTSZ(fp_wbkt, backup_wbuffer, SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR+shd.framesize))
								{
									TRACE_BACKUP("failed write video stream ch:%ld\n", shd.ch);
									CLOSE_BKT(fp_wbkt);
									gBackupCtrl.status = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_DSTBASKET;
									break;
								}
									TRACE_BACKUP("success write video stream ch:%ld\n", shd.ch);
								
								T_BASKET_RDB_PARAM rdb;
								rdb.idx_pos = LTELL(fp_widx);
								
								//////////////////////////////////////////////////////////////////////////
								// write index info if frame is intra-frame 
								if(FT_IFRAME == shd.frametype)
							    {
									idd.id     = ID_INDEX_DATA;
									idd.ch     = shd.ch;
									idd.event  = vhd.event;
									idd.s_type = ST_VIDEO;
									idd.ts.sec = shd.ts.sec;
									idd.ts.usec= shd.ts.usec;
									idd.width  = vhd.width;
									idd.height = vhd.height;
									idd.capMode = vhd.capMode;
									
									//write index
									if(!WRITE_ST(fp_widx, idd))
									{
										TRACEF_BACKUP("failed write index\n");
										CLOSE_BKT(fp_widx);

										gBackupCtrl.status = BACKUP_STATUS_ERROR;
										gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
										break;
									}

									if(bkt_duration.t1[shd.ch].sec == 0)
									{
										// basket start time stamp
										bkt_duration.t1[shd.ch].sec  = shd.ts.sec;
										bkt_duration.t1[shd.ch].usec = shd.ts.usec;

										// index start time stamp
										idx_duration.t1[shd.ch].sec  = shd.ts.sec;
										idx_duration.t1[shd.ch].usec = shd.ts.usec;
									} 
									// update end timestamp every iFrame..
									idx_duration.t2[shd.ch].sec  = shd.ts.sec;
									idx_duration.t2[shd.ch].usec = shd.ts.usec;
								}
								///////////////////////////////////////////////////////////////////////////						
								

								//////////////////////////////////////////////////////////////////////////
								// update rdb
								if(idx_duration.t1[shd.ch].sec != 0 )
								{
									if(prev_min[shd.ch] != cur_min && idx_duration.t2[shd.ch].sec >= shd.ts.sec)
									{
										rdb.ch  = shd.ch;
										rdb.sec = shd.ts.sec;
										rdb.evt = (char)vhd.event;
										rdb.bid = basket_number-1;

//										if(!BACKUP_updateRDB(&rdb))
										if(!BACKUP_updateRDB(&rdb, loop))
										{
											TRACEF_BACKUP("failed update rdb\n");
											gBackupCtrl.status = BACKUP_STATUS_ERROR;
											gBackupCtrl.err_msg = BACKUP_ERR_DSTRDB;
											break;
										}
										TRACEF_BACKUP("success update rdb\n");
										prev_min[shd.ch] = cur_min;
									}
								}

								bkt_duration.t2[shd.ch].sec  = shd.ts.sec;
								bkt_duration.t2[shd.ch].usec = shd.ts.usec;

								//////////////////////////////////////////////////////////////////////////
							}
							else
							{
								if(!LSEEK_CUR(fp_rbkt, shd.framesize))
								{
									TRACEF_BACKUP("failed seek video header\n");
									CLOSE_BKT(fp_rbkt);
									gBackupCtrl.status = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
									break;
								}
							}							
						}
						else if( shd.id == ID_AUDIO_HEADER )
						{
							if(!READ_ST(fp_rbkt, ahd))
							{
								TRACEF_BACKUP("failed read audio header\n");
								CLOSE_BKT(fp_rbkt);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
								break;
							}

							if(bFirstIFrame)
							{
								//TRACEF_BACKUP("write audio stream\n");
								// read frame data
								if(!READ_PTSZ(fp_rbkt, backup_rbuffer, shd.framesize))
								{
									TRACEF_BACKUP("failed read audio stream\n");
									CLOSE_BKT(fp_rbkt);
									gBackupCtrl.status  = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
									break;
								}
								
								//write audio stream...
								memcpy(backup_wbuffer, &shd, SIZEOF_STREAM_HDR);
								memcpy(backup_wbuffer+SIZEOF_STREAM_HDR, &ahd, SIZEOF_ASTREAM_HDR);
								memcpy(backup_wbuffer+SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR, backup_rbuffer, shd.framesize);
								
								if(!WRITE_PTSZ(fp_wbkt, backup_wbuffer, SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR+shd.framesize))
								{
									TRACE_BACKUP("failed write audio stream ch:%ld, size:%ld\n", shd.ch, shd.framesize);
									CLOSE_BKT(fp_wbkt);
									gBackupCtrl.status = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_DSTBASKET;
									break;
								}
							}
							else
							{
								if(!LSEEK_CUR(fp_rbkt, shd.framesize))
								{
									TRACEF_BACKUP("failed read audio stream\n");
									CLOSE_BKT(fp_rbkt);
									gBackupCtrl.status  = BACKUP_STATUS_ERROR;
									gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
									break;
								}
							}							
						}
						else // maybe file end point.
						{
							end_sec = shd.ts.sec ;							

							TRACE_BACKUP("update basket manager. %08ld\n", wbhd.bid);
							
							// write basket manager header
							mhdr.bkt_count += 1;
							time(&cur_sec);
						    mhdr.latest_update=cur_sec;
						    if(!LSEEK_SET(fp_mgr, 0))
							{
								TRACEF_BACKUP("failed seek basket mgr hdr\n");
								CLOSE_BKT(fp_mgr);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								break;
							}

						    if(!WRITE_ST(fp_mgr, mhdr))
						    {
								TRACEF_BACKUP("failed write basket mgr hdr\n");
								CLOSE_BKT(fp_mgr);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								
								break;
						    }
			
							//wirte basket mangaer body.
							memset(&mbd, 0, sizeof(mbd));
							
							mbd.id		  = ID_BKTMGR_BODY;
							mbd.bid = wbhd.bid;
							mbd.save_flag = SF_BACKUP;
							time(&cur_sec);
							mbd.latest_update=cur_sec;
							
//							sprintf(mbd.path, "%s", gBackupCtrl.dst_path);
							sprintf(mbd.path, "%s/hdd%d", gBackupCtrl.dst_path, loop);
							
							if(!LSEEK_END(fp_mgr, 0))
							{
								TRACEF_BACKUP("failed seek basket mgr body\n");
								CLOSE_BKT(fp_mgr);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								break;
							}

							if(!WRITE_ST(fp_mgr, mbd)) 
							{
								TRACEF_BACKUP("failed write basket mgr body\n");
								CLOSE_BKT(fp_mgr);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								break;
							}
			
							// close index file
							T_INDEX_HDR idx_hdr;
							idx_hdr.id = ID_INDEX_HEADER;
							idx_hdr.count    = BACKUP_GetIndexCount(fp_widx);
							idx_hdr.bid = wbhd.bid;
							
							if(!LSEEK_SET(fp_widx, 0))
							{
								TRACE_BACKUP("failed write index header\n");
								CLOSE_BKT(fp_widx);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								break;

							}

							if(!WRITE_ST(fp_widx, idx_hdr))
							{
								TRACE_BACKUP("failed write index header\n");
								CLOSE_BKT(fp_widx);
								gBackupCtrl.status = BACKUP_STATUS_ERROR;
								gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
								break;
							}							
							CLOSE_BKT(fp_widx);
							TRACE_BACKUP("close dst index file. index count :%ld 2]\n", idx_hdr.count);
			
							// close dst basket.
							TRACE_BACKUP("close dst basket. BID:%08ld\n", wbhd.bid);
							CLOSE_BKT(fp_wbkt);
					
							// close src basket
							TRACE_BACKUP("close src basket. BID:%08d\n", fcount);
							CLOSE_BKT(fp_rbkt);
#ifdef BKT_SYSIO_CALL
							fp_rbkt=0;
#else
							fp_rbkt=NULL;
#endif
							
							bFirstIFrame = 0;
							memset(&bkt_duration, 0, sizeof(bkt_duration));
							memset(&idx_duration, 0, sizeof(idx_duration));
							memset(prev_min, -1, sizeof(prev_min));

						}// file end point
					}
					else // skip fpos to next frame
					{
						//TRACE_BACKUP("tskMain skip frame gch:%d, ch:%d, SHID:0x%08x, gt1:%ld, gt2:%ld, shd.ts.sec:%ld\n", gBackupCtrl.ch, shd.ch, shd.id, gBackupCtrl.t1, gBackupCtrl.t2, shd.ts.sec);
						if(!LSEEK_CUR(fp_rbkt, shd.framesize+((shd.id == ID_VIDEO_HEADER)?sizeof(vhd):sizeof(ahd))))
						{
							TRACE_BACKUP("failed seek.\n");
							CLOSE_BKT(fp_rbkt);
							gBackupCtrl.status = BACKUP_STATUS_ERROR;
							gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
							break;
						}
					}
				}
				
				gBackupCtrl.fpos_cur = LTELL(fp_rbkt);
				
				//TRACE_BACKUP("fpos_cur:%ld, channels[%ld]:%d\n", gBackupCtrl.fpos_cur, gBackupCtrl.ch[shd.ch], shd.ch);
			}
			else // maybe file end point.
			{
				// write basket manager header
				mhdr.bkt_count += 1;
			    time(&mhdr.latest_update);
			    if(!LSEEK_SET(fp_mgr, 0))
				{
					TRACEF_BACKUP("failed seek basket mgr hdr\n");
					CLOSE_BKT(fp_mgr);
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					break;
				}

			    if(!WRITE_ST(fp_mgr, mhdr))
			    {
					TRACEF_BACKUP("failed write basket mgr hdr\n");
					CLOSE_BKT(fp_mgr);
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					break;
			    }

				//wirte basket mangaer body.
				memset(&mbd, 0, sizeof(mbd));
				
				mbd.id		  = ID_BKTMGR_BODY;
				mbd.bid       = wbhd.bid;
				mbd.save_flag = SF_BACKUP;
				time(&mbd.latest_update);
				memcpy(&mbd.ts, &bkt_duration, sizeof(bkt_duration));
				
				sprintf(mbd.path, "%s/hdd%d", gBackupCtrl.dst_path, loop);
				
				if(!LSEEK_END(fp_mgr, 0))
				{
					TRACEF_BACKUP("failed seek basket mgr\n");
					CLOSE_BKT(fp_mgr);
					
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					
					break;
				}

				if(!WRITE_ST(fp_mgr, mbd)) 
				{
					TRACEF_BACKUP("failed write basket mgr body\n");
					CLOSE_BKT(fp_mgr);
					
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					
					break;
				}

				// close index file
				T_INDEX_HDR idx_hdr;
				idx_hdr.id = ID_INDEX_HEADER;
				idx_hdr.count    = BACKUP_GetIndexCount(fp_widx);
				idx_hdr.bid = wbhd.bid;
				memcpy(&idx_hdr.ts, &idx_duration, sizeof(idx_duration));
				
				if(!LSEEK_SET(fp_widx, 0))
				{
					TRACE_BACKUP("failed seek index header\n");
					CLOSE_BKT(fp_widx);
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					break;
				}
				if(!WRITE_ST(fp_widx, idx_hdr))
				{
					TRACE_BACKUP("failed write index header\n");
					CLOSE_BKT(fp_widx);
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
					break;
				}
				TRACE_BACKUP("close dst index file. index count :%ld 3]\n", idx_hdr.count);
				CLOSE_BKT(fp_widx);

				// close dst basket.
				TRACE_BACKUP("close dst basket. BID:%08ld\n", wbhd.bid);
				CLOSE_BKT(fp_wbkt);
		
				// close src basket
				TRACE_BACKUP("close src basket. BID:%08d\n", fcount);
				CLOSE_BKT(fp_rbkt);

#ifdef BKT_SYSIO_CALL
				fp_rbkt=0;
#else
				fp_rbkt=NULL;
#endif
				
				bFirstIFrame = 0;
				memset(&bkt_duration, 0, sizeof(bkt_duration));
				memset(&idx_duration, 0, sizeof(idx_duration));
				memset(prev_min, -1, sizeof(prev_min));

			}
		}
		else // open
		{
			if(fcount>=gBackupCtrl.filecount)
			{
				TRACE_BACKUP("backup is done. fcount is %d. searched count is %d\n", fcount, gBackupCtrl.filecount);
				gBackupCtrl.status = BACKUP_STATUS_DONE;
				break;
			}
			
			// open src index
			sprintf(path, "%s/%08d.idx", gBackupCtrl.src_path, *(gBackupCtrl.filelist+fcount));
			if(!OPEN_RDONLY(fp_ridx, path))
			{
				TRACEF_BACKUP("failed open src index %s\n", path);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCINDEX;
				break;
			}
			
			if(!LSEEK_SET(fp_ridx, sizeof(T_INDEX_HDR)))
			{
				TRACEF_BACKUP("failed seek src index.\n");
				CLOSE_BKT(fp_ridx);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCINDEX;
				break;
			}
			
			gBackupCtrl.fpos_end=0;
			gBackupCtrl.fpos_cur=0;
			memset(&bkt_duration, 0, sizeof(bkt_duration));
			memset(&idx_duration, 0, sizeof(idx_duration));
			memset(prev_min, -1, sizeof(prev_min));

			T_INDEX_DATA src_idd;
			while(READ_ST(fp_ridx, src_idd))
			{
				if(gBackupCtrl.ch[src_idd.ch]==1)
				{
					if(gBackupCtrl.fpos_cur != 0)
					{
						if(src_idd.ts.sec >= gBackupCtrl.t2)
						{
							gBackupCtrl.fpos_end=src_idd.fpos;
							break;
						}
					}
					else
					{
						if(src_idd.ts.sec >= gBackupCtrl.t1)
						{
							gBackupCtrl.fpos_cur = src_idd.fpos;
						}
					}
				}
			}
			CLOSE_BKT(fp_ridx);
			
			if(gBackupCtrl.fpos_cur == 0 && gBackupCtrl.fpos_end == 0)
			{
				TRACE_BACKUP("failed find src backet file point. t1:%ld, t2:%ld\n", gBackupCtrl.t1, gBackupCtrl.t2);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCINDEX;
				break;
			}
			
			TRACE_BACKUP("found src backet file point. cur:%ld, end:%ld, t1:%ld, t2:%ld\n", gBackupCtrl.fpos_cur, gBackupCtrl.fpos_end, gBackupCtrl.t1, gBackupCtrl.t2);
			
			// open src basket
			sprintf(path, "%s/%08d.bkt", gBackupCtrl.src_path, *(gBackupCtrl.filelist+fcount));
			if(!OPEN_RDONLY(fp_rbkt, path))
			{
				TRACEF_BACKUP("failed open src backet %s\n", path);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
				break;
			}
		
			// not found end file point
			if(gBackupCtrl.fpos_cur != 0 && gBackupCtrl.fpos_end==0)
			{
				if(!LSEEK_END(fp_rbkt,0))
				{
					TRACEF_BACKUP("failed seek src backet.\n");
					CLOSE_BKT(fp_rbkt);
					gBackupCtrl.status = BACKUP_STATUS_ERROR;
					gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
					break;
				}
				gBackupCtrl.fpos_end = LTELL(fp_rbkt);
			}
			
			if(!LSEEK_SET(fp_rbkt,0))
			{
				TRACEF_BACKUP("failed seek src backet.\n");
				CLOSE_BKT(fp_rbkt);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
				break;
			}
			if(!READ_ST(fp_rbkt, rbhd))
			{
				TRACEF_BACKUP("failed read src backet.\n");
				CLOSE_BKT(fp_rbkt);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
				break;
			}
			
			if(!LSEEK_SET(fp_rbkt, gBackupCtrl.fpos_cur))
			{
				TRACEF_BACKUP("failed seek src backet.\n");
				CLOSE_BKT(fp_rbkt);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_SRCBASKET;
				break;
			}

			TRACE_BACKUP("opened src basket. %s\n", path);
			
			// open dst basket
//			sprintf(path, "%s/%08d.bkt", gBackupCtrl.dst_path, basket_number);
			sprintf(path, "%s/hdd%d/%08d.bkt", gBackupCtrl.dst_path, loop, basket_number);
			if(!OPEN_CREATE(fp_wbkt, path))
			{
				TRACEF_BACKUP("failed create dst backet %s.\n", path);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_DSTBASKET;
				break;
			}
			
			memset(&wbhd, 0, sizeof(wbhd));
			wbhd.id = ID_BASKET_HDR;
			wbhd.bid = basket_number;
			wbhd.latest_update = rbhd.latest_update;
			wbhd.save_flag = SF_BACKUP;
			
			if(!WRITE_ST(fp_wbkt, wbhd))
			{
				TRACEF_BACKUP("failed write dst basket header\n");
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_DSTBASKET;
				break;
			}
			TRACE_BACKUP("opened dst basket. %s\n", path);
			
			// open dst index
//			sprintf(path, "%s/%08d.idx", gBackupCtrl.dst_path, basket_number);
			sprintf(path, "%s/hdd%d/%08d.idx", gBackupCtrl.dst_path, loop, basket_number);
			if(!OPEN_CREATE(fp_widx, path))
			{
				TRACEF_BACKUP("failed create dst index %s.\n", path);
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
				break;
			}
			T_INDEX_HDR dst_ihdr;
			dst_ihdr.id = ID_INDEX_HEADER;
			dst_ihdr.bid = basket_number;
			memset(&dst_ihdr.ts, 0, sizeof(dst_ihdr.ts));
			dst_ihdr.count=0;
			
			if(!WRITE_ST(fp_widx, dst_ihdr))
			{
				TRACEF_BACKUP("failed write dst index header\n");
				gBackupCtrl.status = BACKUP_STATUS_ERROR;
				gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
				break;
			}
						
			TRACE_BACKUP("opened dst index. %s\n", path);

			basket_number++;
			fcount++;
			
			gBackupCtrl.file_curr_number = fcount;
		}
		
		usleep(1000);
		
		status = OSA_tskCheckMsg(pTsk, &pMsg);
		if(status != OSA_SOK) 
		{
			continue;
		}		
		
		cmd = OSA_msgGetCmd(pMsg);
		
		switch(cmd) 
		{
			case BACKUP_CMD_DELETE:

				TRACE_BACKUP("RECV BACKUP_CMD_DELETE ===>\n");
				{
					gBackupCtrl.status = BACKUP_STATUS_CANCEL;
					// write basket manager header
					mhdr.bkt_count += 1;
					time(&cur_sec);
					mhdr.latest_update=cur_sec;
					
					if(!LSEEK_SET(fp_mgr, 0))
					{
						TRACEF_BACKUP("failed seek basket mgr hdr\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						
						break;
					}
					if(!WRITE_ST(fp_mgr, mhdr))
					{
						TRACEF_BACKUP("failed write basket mgr hdr\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						
						break;
					}
					
					//wirte basket mangaer body.
					memset(&mbd, 0, sizeof(mbd));
					
					mbd.id		  = ID_BKTMGR_BODY;
					mbd.bid = wbhd.bid;
					mbd.save_flag = SF_BACKUP;
					time(&cur_sec);
					mbd.latest_update=cur_sec;
					memcpy(&mbd.ts, &bkt_duration, sizeof(bkt_duration));
					
					sprintf(mbd.path, "%s", gBackupCtrl.dst_path);
					
					if(!LSEEK_END(fp_mgr, 0))
					{
						TRACEF_BACKUP("failed seek basket mgr body\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						break;
					}
					if(!WRITE_ST(fp_mgr, mbd)) 
					{
						TRACEF_BACKUP("failed write basket mgr body\n");
						CLOSE_BKT(fp_mgr);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTBKTMGR;
						break;
					}
					TRACE_BACKUP("close dst mgr file.\n");
					CLOSE_BKT(fp_mgr);
					
					// close index file
					T_INDEX_HDR idx_hdr;
					idx_hdr.id    = ID_INDEX_HEADER;
					idx_hdr.count = BACKUP_GetIndexCount(fp_widx);
					idx_hdr.bid   = wbhd.bid;
					memcpy(&idx_hdr.ts, &idx_duration, sizeof(idx_duration));
					
					if(!LSEEK_SET(fp_widx, 0))
					{
						TRACEF_BACKUP("failed seek index header.\n");
						CLOSE_BKT(fp_widx);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
						break;
					}

					if(!WRITE_ST(fp_widx, idx_hdr))
					{
						TRACE_BACKUP("failed write index header\n");
						CLOSE_BKT(fp_widx);
						gBackupCtrl.status = BACKUP_STATUS_ERROR;
						gBackupCtrl.err_msg = BACKUP_ERR_DSTINDEX;
						break;
					}					
					TRACE_BACKUP("close dst index file. index count :%ld 4]\n", idx_hdr.count);
					CLOSE_BKT(fp_widx);
					
					// close dst basket.
					TRACE_BACKUP("close dst basket. BID:%08ld\n", wbhd.bid);
					CLOSE_BKT(fp_wbkt);
					
					// close src basket
					TRACE_BACKUP("close src basket. BID:%08d\n", fcount);
					CLOSE_BKT(fp_rbkt);
#ifdef BKT_SYSIO_CALL
					fp_rbkt=0;
#else
					fp_rbkt=NULL;
#endif
					BackupTsk_Running = FALSE;
					ackMsg=TRUE;
					
				}
				break;
			
			default:
				OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);
				break;
		}

	}// end while

	if(gBackupCtrl.pathcount == 2)
	{
		sprintf(gBackupCtrl.src_path, "%s",gBackupCtrl.src_endpath) ;

		BKTMD_getTargetDisk(gBackupCtrl.t2) ;

		BKTMD_GetBasketTime(1, &t1, &end_sec) ;
		t2 = gBackupCtrl.t2 ;

		int ncount = BKTMD_getRecFileCount(gBackupCtrl.ch, end_sec, t2);
    	if(1 > ncount)
    	{
        	TRACE_BACKUP("There is no backup file.\n");

        	return OSA_EFAIL;
    	}

		gBackupCtrl.filecount = ncount;
    	gBackupCtrl.filelist = (int*)malloc((ncount+1)*sizeof(int)); // for surplus

    	if(!gBackupCtrl.filelist)
    	{
        	TRACE_BACKUP("failed, malloc for file list.\n");
        	return OSA_EFAIL;
    	}

    	if( ncount != BKTMD_getRecFileList(gBackupCtrl.ch, end_sec, t2, gBackupCtrl.filelist, gBackupCtrl.src_path, &gBackupCtrl.pathcount))
    	{
        	TRACE_BACKUP("Sorry, There are differnt backup file count. Please, try again.\n");
        	if(gBackupCtrl.filelist)
            free(gBackupCtrl.filelist);

        	return OSA_EFAIL;
    	}
		loop = 1 ;

		goto add_bkt ;
	}


	if(gBackupCtrl.dstmedia == TYPE_CDROM)
    {
        sprintf(bskbuf[0],"%s",gBackupCtrl.dst_path) ;
		printf("gBackupCtrl.dst_path = %s\n",gBackupCtrl.dst_path) ;		

		char cwd[MAX_MGR_PATH];

        getcwd(cwd, MAX_MGR_PATH);

		sprintf(Tempbackup, "cp -rf %s/%s %s",cwd, BACKUP_BINARY_MKISO, gBackupCtrl.dst_path) ;
		fd = popen(Tempbackup,"w") ;
    	pclose(fd) ;
        chdir(bskbuf[0]) ;

        sprintf(namebuf,"%s", "bskbackup.iso") ;
        ret = LIB816x_CDROM_MAKE_ISO(namebuf, bskbuf, 1) ;

		chdir(cwd) ;
        if(ret == TRUE)
        {
            TRACE_BACKUP("Make iso Success \n") ;

			sprintf(Tempbackup, "cp -rf %s/%s %s",cwd, BACKUP_BINARY_CDRECORD, gBackupCtrl.dst_path) ;
			fd = popen(Tempbackup,"w") ;
    		pclose(fd) ;

			chdir(bskbuf[0]) ;
            ret = LIB816x_CDROM_WRITE_ISO(gBackupCtrl.media_path, "bskbackup.iso") ;

            if(ret == TRUE)
            {
                send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_NO_ERR, NULL);
                TRACE_BACKUP("CDROM_WRITE SUCCESS..\n") ;
            }
            else
            {
                send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_WRITE_ISO_FAILED, NULL);
                TRACEF_BACKUP("CDROM_WRITE FAIL..\n") ;
            }
				chdir(cwd) ;
				sprintf(Tempbackup, "rm -rf %s/backup/cdrecord ",gBackupCtrl.dst_path) ;
				fd = popen(Tempbackup,"w") ;
    			pclose(fd) ;
			
        }
        else
        {
            TRACEF_BACKUP("Make iso Failed \n") ;
            send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_MAKE_ISO_FAILED, NULL);
        }
			sprintf(Tempbackup, "rm -rf %s/backup/mkisofs",gBackupCtrl.dst_path) ;
			fd = popen(Tempbackup,"w") ;
    		pclose(fd) ;
	}
	else
	{
		send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_NO_ERR, NULL);
		TRACE_BACKUP("USB_WRITE SUCCESS..\n") ;
	}
	
backup_tsk_error:

	if(ackMsg)
		OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);

	if(gBackupCtrl.filelist)
		free(gBackupCtrl.filelist);

    for(i = 0; i < 100; i++)
    {
        free(bskbuf[i]) ; ;
    }
	free(bskbuf) ;
	
	status = OSA_tskDelete( &gBackupCtrl.tskHndl );
    OSA_assertSuccess(status);

    status = OSA_mbxDelete( &gBackupCtrl.mbxHndl);
    OSA_assertSuccess(status);

	BackupTsk_Running = FALSE;
	
	TRACE_BACKUP("end tskMain\n");
	
	return OSA_SOK;
}


int BACKUP_sendCmd(Uint16 cmd, void *prm, Uint16 flags )
{
  return OSA_mbxSendMsg(&gBackupCtrl.tskHndl.mbxHndl, &gBackupCtrl.mbxHndl, cmd, prm, flags);
}

int BACKUP_sizeTargetStorage(char *targetpath, int media)
{
	FILE *fd ;
	int ret = 0 ;
	char buf_in[255];
	char mount_point[LEN_PATH];
	char namebuf[30] ;
	
    struct statfs _statfs;
    
    unsigned long disk_free_size=0;

   	sprintf(mount_point, "%s", targetpath);
    		
	if(statfs(mount_point, &_statfs) == 0)
	{
		disk_free_size = (_statfs.f_bfree*_statfs.f_bsize)/1024;

// temp code
// need to get cdrom_freesize
		if(media == TYPE_CDROM)
		{
			ret = LIB816x_CDROM_MEDIA(targetpath) ;
        	if(ret == 0) // cdrom
            	disk_free_size = CDROM_SIZE ; // KB
        	else if(ret == 2)
            	disk_free_size = DVD_SIZE ;
        	else if(ret == 1)
            	disk_free_size = 0 ; // INVALID MEDIA

		}
		if(disk_free_size)
		{
			if(media == TYPE_CDROM)
			{
				sprintf(namebuf,"rm -rf %s",gBackupCtrl.dst_path); // remove previous backup directory
    			fd = popen(namebuf,"w") ;
    			pclose(fd) ;
				
				mkdir(gBackupCtrl.dst_path, O_RDWR) ;
				sprintf(gBackupCtrl.media_path, "%s", targetpath);  // cdrom or usb

			}
			else	
			{
				sprintf(gBackupCtrl.dst_path, "%s", targetpath);
			}
//			TRACE_BACKUP("found backup storage path:%s, free_size:%ld MB\n", mount_point, disk_free_size);
			TRACE_BACKUP("found backup storage path:%s, free_size:%ld MB\n", gBackupCtrl.dst_path, disk_free_size);


			return disk_free_size;
		}
	}
  
	TRACE_BACKUP("failed search target media\n");
    
    return 0;
	
}


int app_bskbackup_create(int media, char *path, int ch_bitmask, long t1, long t2)
{
	int i = 0, ret = 0, media_size = 0, sleep_msec = 0 ;

	// BKKIM, This is not used.
	//T_BKTMD_CTRL gBKTMD_CtrlV ;

//	int channels[MAX_CH];
//	memcpy(channels, pChannels, sizeof(channels));
/*
#ifdef BACKUP_DEBUG
	T_BKT_TM tm1;
	BKT_GetLocalTime(t1, &tm1);
	
	T_BKT_TM tm2;
	BKT_GetLocalTime(t2, &tm2);
	
	TRACE_BACKUP("====================================================>\n");
	TRACE_BACKUP("BACKUP_createEx() ==== \n");
	TRACE_BACKUP("\tparam : Chs - %d, %d, %d, %d, %d, %d, %d, %d\n", channels[0],channels[1], channels[2], channels[3], channels[4], channels[5], channels[6], channels[7] );
	TRACE_BACKUP("\tparam : TS1 - %04d/%02d/%02d %02d:%02d:%02d\n", tm1.year, tm1.mon, tm1.day, tm1.hour, tm1.min, tm1.sec);
	TRACE_BACKUP("\tparam : TS2 - %04d/%02d/%02d %02d:%02d:%02d\n", tm2.year, tm2.mon, tm2.day, tm2.hour, tm2.min, tm2.sec);
	TRACE_BACKUP("====================================================>\n\n");
#endif
*/
	if(BackupTsk_Running==FALSE)
	{
		app_bskbackup_delete();
		OSA_waitMsecs(sleep_msec);
	}

	//memset(&gBackupCtrl, 0, sizeof(gBackupCtrl));
	
	gBackupCtrl.status=0;
	gBackupCtrl.err_msg=0;
	
	memset(gBackupCtrl.ch, 0, sizeof(gBackupCtrl.ch));
	gBackupCtrl.t1=0;
	gBackupCtrl.t2=0;
	
	gBackupCtrl.fpos_cur=0;      // for progress status
	gBackupCtrl.fpos_end=0; 		 // for progress status
	gBackupCtrl.file_curr_number=0; // for progress status
	gBackupCtrl.file_total_count=0; // for progress status
	
	gBackupCtrl.filecount=0;
	gBackupCtrl.dstmedia = media ;

	for(i = 0; i < MAX_CH; i++)
    {
        if(ch_bitmask & (0x01 << i))
            gBackupCtrl.ch[i] = TRUE ;
        else
            gBackupCtrl.ch[i] = FALSE ;
    }
	
	int ncount = BKTMD_getRecFileCount(gBackupCtrl.ch, t1, t2);
	if(1 > ncount)
	{
		TRACE_BACKUP("There is no backup file.\n");
		
		return OSA_EFAIL;
	}

	gBackupCtrl.filecount = ncount;
	gBackupCtrl.filelist = (int*)malloc((ncount+1)*sizeof(int)); // for surplus

	if(!gBackupCtrl.filelist)
	{
		TRACE_BACKUP("failed, malloc for file list.\n");
		return OSA_EFAIL;
	}
	
//	sprintf(gBackupCtrl.dst_path, "%s",path) ;	

    ret = BKTMD_getTargetDisk(t2) ;
    if(ret != TRUE)
        return OSA_EFAIL;

	ret = BKTMD_exgetTargetDisk(gBackupCtrl.src_endpath) ;
//    sprintf(gBackupCtrl.src_endpath, "%s", gBKTMD_CtrlV.target_path);

	sprintf(gBackupCtrl.dst_path, "%s/backup", gBackupCtrl.src_endpath);
	// search target storage.
	media_size = BACKUP_sizeTargetStorage(path, media) ;
	if(media_size == 0)
	{
		send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_INVALID_MEDIA, NULL);
		TRACE_BACKUP("There is no backup storage.\n");
		
		return OSA_EFAIL;
	}

	if(media_size < ncount*(BASKET_SIZE/1024))
	{
		send_message_bskbackup_to_qt(LIB_BACKUP, LIB_BA_NOT_SUFFICIENT, NULL);
		return OSA_EFAIL ;
	
	}

	if( ncount != BKTMD_getRecFileList(gBackupCtrl.ch, t1, t2, gBackupCtrl.filelist, gBackupCtrl.src_path, &gBackupCtrl.pathcount))
	{
		TRACE_BACKUP("Sorry, There are differnt backup file count. Please, try again.\n");
		if(gBackupCtrl.filelist)
			free(gBackupCtrl.filelist);
				
		return OSA_EFAIL;
	}

	for(i = 0; i < MAX_CH; i++)
	{
		if(ch_bitmask & (0x01 << i))
			gBackupCtrl.createPrm.ch[i] = TRUE ;
		else
			gBackupCtrl.createPrm.ch[i] = FALSE ;
	}

	gBackupCtrl.createPrm.t1 = t1;
	gBackupCtrl.createPrm.t2 = t2;

	int status = OSA_tskCreate( &gBackupCtrl.tskHndl, BACKUP_tskMain, BACKUP_THR_PRI, BACKUP_STACK_SIZE, 0,&gBackupCtrl);
	
	OSA_assertSuccess(status);
	status = OSA_mbxCreate( &gBackupCtrl.mbxHndl);
	OSA_assertSuccess(status);
	
	TRACE_BACKUP("send BACKUP_CMD_CREATE ===>\n");
	status = BACKUP_sendCmd(BACKUP_CMD_CREATE, &gBackupCtrl.createPrm, OSA_MBX_WAIT_ACK );
	TRACE_BACKUP("recv BACKUP_CMD_CREATE ===>\n");

	return status;
}

int app_bskbackup_delete()
{
	int status;

	TRACE_BACKUP("send BACKUP_CMD_DELETE ===>\n");
	status = BACKUP_sendCmd(BACKUP_CMD_DELETE, NULL, OSA_MBX_WAIT_ACK );
	TRACE_BACKUP("recv BACKUP_CMD_DELETE ===>\n");
	
	status = OSA_tskDelete( &gBackupCtrl.tskHndl );
	OSA_assertSuccess(status);

	status = OSA_mbxDelete( &gBackupCtrl.mbxHndl);
	OSA_assertSuccess(status);

	BackupTsk_Running=FALSE;

	return status;
}

int BACKUP_getPlaybackStoragePath(char *buf_out)
{
	char fbktmgr[255];
	char buf_in[255];
	char mount_path[LEN_PATH];

	FILE *fp_mounts = fopen("/proc/mounts", "r");
	char *found_mnt;
	
    while(fgets(buf_in, 255, fp_mounts) != NULL)
    {
    	found_mnt = strstr(buf_in, "/mnt/sd");
    	if (found_mnt)
    	{
    		sprintf(mount_path, "%s", strtok(found_mnt, " "));
    		sprintf(fbktmgr, "%s/bktmgr.udf", mount_path);
    		
    		if(-1 != access(fbktmgr, 0)) // existence only
    		{
    			sprintf(buf_out, "%s", mount_path);

				fclose(fp_mounts);

    			return 1;
    		}

    	}
    }
    
    return 0;
}


int BACKUP_getProgressStatus(long *curfpos, long *endfpos, int *curfnum, int *totalfnum)
{
	*curfpos = gBackupCtrl.fpos_cur;
	*endfpos = gBackupCtrl.fpos_end;
	*curfnum = gBackupCtrl.file_curr_number;
	*totalfnum = gBackupCtrl.filecount;
	
	return gBackupCtrl.status;
}

void app_bskbackup_set_pipe(void *pipe_id)
{
	BACKUP_Ctrl *gCtrl = &gBackupCtrl ;

    if (pipe_id == NULL)
        return;

    gCtrl->msg_pipe_id = pipe_id;

}




// EOF

