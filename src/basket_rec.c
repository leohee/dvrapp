/**
 @file basket_rec.c
 @brief
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

#include <basket_rec.h>

#include "app_util.h"
#include "global.h"

///////////////////////////////////////
//#define BKTREC_DEBUG

#ifdef BKTREC_DEBUG
#define TRACE0_BKTREC(msg, args...) printf("[BKTREC] - " msg, ##args)
#define TRACE1_BKTREC(msg, args...) printf("[BKTREC] - %s:" msg, __FUNCTION__, ##args)
#define TRACE2_BKTREC(msg, args...) printf("[BKTREC] - %s(%d):" msg, __FUNCTION__, __LINE__, ##args)
#define TRACE3_BKTREC(msg, args...) printf("[BKTREC] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE0_BKTREC(msg, args...) ((void)0)
#define TRACE1_BKTREC(msg, args...) ((void)0)
#define TRACE2_BKTREC(msg, args...) ((void)0)
#define TRACE3_BKTREC(msg, args...) ((void)0)
#endif

#define BKTREC_ERR(msg, args...) printf("[BKTREC] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)

#ifndef VIDEO
#define VIDEO 0
#endif
#ifndef AUDIO
#define AUDIO 1
#endif

#define BKT_OPEN_MODE_RDWR

////MULTI-HDD SKSUNG////
BKT_Mount_Info gbkt_mount_info;

int  g_bkt_rec_space  =BKT_SPACE_REMAIN;  ///< 0: remain empty basket, 1:there is no empty basket.
int  g_bkt_rec_status =BKT_STATUS_CLOSED; ///< 0:closed, 1:opened, 2:idle
int  g_bkt_recycle=BKT_RECYCLE_ON;        ///< 0:recycle, 1:once

BKTREC_CTRL gBKTREC_Ctrl;
BKT_FHANDLE g_handle_rec_bkt;
BKT_FHANDLE g_handle_rec_idx;

#ifdef BKTREC_DEBUG
const char* g_szSaveFlags[]=
{
    "EMPTY", "USING", "STOP", "FULL", "ERR"
};
#endif
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
int BKTREC_TotalBasketInfo(long *bktcnt, long *bktsize )
{
    long bkt_size = BASKET_SIZE/SIZE_MB;
	T_BKTMGR_HDR  hd;

#ifdef BKT_SYSIO_CALL
    int fd;
#else
    FILE *fd;
#endif

    int i=0;
	char mgr_path[30] ;

	*bktcnt = 0;
	*bktsize = bkt_size ;

	if(gbkt_mount_info.hddCnt > 0)
    {
		for(i = 0; i < MAX_HDD_COUNT; i++)
		{
			if(gbkt_mount_info.hddInfo[i].isMounted)
			{
				sprintf(mgr_path, "%s/%s", gbkt_mount_info.hddInfo[i].mountDiskPath, NAME_BKTMGR);

	    		if(-1 == access(mgr_path, 0))
	    		{
	        		BKTREC_ERR("can't finid basket manager file. %s. \n", mgr_path);

	        		return -1;
	    		}

	 		   	if(!OPEN_RDWR(fd, mgr_path)) // open bktmgr.udf file
	    		{
	        		BKTREC_ERR("failed open basket manager file. %s\n", mgr_path);
		     		return BKT_ERR;
	    		}

	    		if(!READ_ST(fd, hd))
	    		{
	       	 		BKTREC_ERR("failed read basket manager header.\n");
	       	 		CLOSE_BKT(fd);
	        		return BKT_ERR;
	    		}

				*bktcnt += hd.bkt_count ;
				*bktsize = bkt_size ;

				if(gbkt_mount_info.curHddIdx == i)
				{
					gBKTREC_Ctrl.bkt_cnt = hd.bkt_count ;
				}

				CLOSE_BKT(fd) ;
			}
		}
	}

	#if 0
	printf("\n");
	printf("=================================================\n");
	printf("              GET BASKET INFORMATION             \n");
	printf("=================================================\n");
	printf("     TOT-BKT   BKT-SIZE   CURIDX   BKT-CNT   \n");
	printf("=================================================\n");

    printf("       %05d      %03d       %02d        %05d    \n", *bktcnt, *bktsize, gbkt_mount_info.curHddIdx, gBKTREC_Ctrl.bkt_cnt);
	printf("=================================================\n");
	printf("\n");
	#endif

	return TRUE ;
}

static
int BKTREC_get_count(const char *diskpath)
{
	BKT_FHANDLE fd;
	T_BKTMGR_HDR  hdr;
	char mgr_path[30] ;

	sprintf(mgr_path, "%s/%s", diskpath, NAME_BKTMGR);

	if(-1 == access(mgr_path, 0))
	{
		BKTREC_ERR("can't finid basket manager file. %s. \n", mgr_path);
		return 0;
	}

	if(!OPEN_RDONLY(fd, mgr_path)) // open bktmgr.udf file
	{
		BKTREC_ERR("failed open basket manager file. %s\n", mgr_path);
 		return 0;
	}

	if(!READ_ST(fd, hdr))
	{
	 	BKTREC_ERR("failed read basket manager header.\n");
	 	CLOSE_BKT(fd);
		return 0;
	}
	CLOSE_BKT(fd) ;

	return hdr.bkt_count;

}

int BKTREC_BasketInfo(const char *diskpath, long *bktcnt, long *bktsize )
{
    long bkt_size = BASKET_SIZE/SIZE_MB;
	T_BKTMGR_HDR  hd;

#ifdef BKT_SYSIO_CALL
    int fd;
#else
    FILE *fd;
#endif

	char mgr_path[30] ;

	*bktcnt = 0;
	*bktsize = bkt_size ;

	sprintf(mgr_path, "%s/%s", diskpath, NAME_BKTMGR);

	if(-1 == access(mgr_path, 0))
	{
		BKTREC_ERR("can't finid basket manager file. %s. \n", mgr_path);

		return -1;
	}

	if(!OPEN_RDWR(fd, mgr_path)) // open bktmgr.udf file
	{
		BKTREC_ERR("failed open basket manager file. %s\n", mgr_path);
 		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
	 	BKTREC_ERR("failed read basket manager header.\n");
	 	CLOSE_BKT(fd);
		return BKT_ERR;
	}

	*bktcnt  = hd.bkt_count ;
	*bktsize = bkt_size ;

	CLOSE_BKT(fd) ;

	#if 0
	printf("\n");
	printf("=================================================\n");
	printf("              GET BASKET INFORMATION             \n");
	printf("=================================================\n");
	printf("     TOT-BKT   BKT-SIZE   CURIDX   BKT-CNT   \n");
	printf("=================================================\n");

    printf("       %05d      %03d       %02d        %05d    \n", *bktcnt, *bktsize, gbkt_mount_info.curHddIdx, gBKTREC_Ctrl.bkt_cnt);
	printf("=================================================\n");
	printf("\n");
	#endif

	return TRUE ;
}


long BKTREC_CreateBasket(const char *diskpath)
{
	long msize=0, adjust_disk_size=0;
	int64_t size64;
	struct statfs s;
	long bkt_size = BASKET_SIZE/SIZE_MB;
	long bkt_count=0;


	// get disk free size
	if(statfs(diskpath, &s) == 0)
	{
		size64 = s.f_bfree;
		size64 *= s.f_bsize;
		msize  = size64/(SIZE_MB);
	}

	adjust_disk_size = msize - (msize/100); //-1.0%

	if(adjust_disk_size < 0)
	{
		TRACE0_BKTREC("Check disk space.\n");
		return BKT_ERR;
	}

	bkt_count = (adjust_disk_size/bkt_size) - 1;
	bkt_count = bkt_count - (bkt_count/20); // - 5%;

	if(bkt_count < 2)// BKT_MAX_CHANNEL
	{
		BKTREC_ERR("failed create basket files. basket count=%d ( < 2).\n", bkt_count);
		return BKT_ERR;
	}

	return BKTREC_CreateBasketFiles(diskpath, bkt_size, bkt_count);
}

long BKTREC_CreateBasketFiles(const char *diskpath, long bkt_size, long bkt_count)
{
    TRACE0_BKTREC("making basket files.....%s\n", diskpath);

////////MULTI-HDD SKSUNG/////////
#ifdef BKT_SYSIO_CALL
	int fd_tm, fd_mgr, fd_bkt, fd_idx;
#else
	FILE *fd_tm, *fd_mgr, *fd_bkt, *fd_idx;
#endif

	T_TOPMGR tm;
	T_BKTMGR_HDR hd;
	T_BKTMGR_BODY bd;

	time_t cur_sec;
	int  i;

	char path_top[30];
	char path_mgr[30];
	char path_bkt[LEN_PATH];
	char path_idx[LEN_PATH];

  if(0 == strlen(diskpath))
  {
		BKTREC_ERR("target disk path's length is zero.\n");
		return BKT_ERR;
	}

	if(bkt_count < 2)// BKT_MAX_CHANNEL
	{
		BKTREC_ERR("failed create basket files. basket count=%d ( < 2).\n", bkt_count);
		return BKT_ERR;
	}

/*  top manager create & open   */

    sprintf(path_top, "%s/%s", diskpath, NAME_TOPMGR);

	if(!OPEN_CREATE(fd_tm, path_top)) // create top manager file. (topmgr.udf)
    {
        BKTREC_ERR("Failed create top manager. %s\n", path_top);
       	return BKT_ERR;
    }

	tm.hddIdx = -1;
	hd.mgrid = -1;

	for(i = 0; i < MAX_HDD_COUNT; i++)
	{
		if(gbkt_mount_info.hddInfo[i].isMounted == 0)
		{
			tm.id = ID_TOPMGR_HEADER;
			tm.hddIdx = i;
			time(&tm.formatDate);
			sprintf(tm.dvrMAC, "%s", gMac0);

			hd.mgrid = i;
			break;
		}
	}
#if 0
	if(tm.hddIdx >= 0)
	{
		if(!WRITE_ST(fd_tm, tm))
		{
			printf("failed write top mgr file\n");
			CLOSE_BKT(fd_tm);
			return BKT_ERR;
		}
	}
#endif

	if(tm.hddIdx == -1)
	{
		BKTREC_ERR("Create Basket file system -- OVER_THE_HDD_COUNT !!\n");
		CLOSE_BKT(fd_tm);
		return BKT_OVERHDD;
	}

#if 0
	CLOSE_BKT(fd_tm);
#endif
//////////////////////////////////////


	TRACE0_BKTREC("create basket files -- size=%d(MB), count=%d\n", bkt_size, bkt_count);

	sprintf(path_mgr, "%s/%s", diskpath, NAME_BKTMGR);

	if(!OPEN_CREATE(fd_mgr, path_mgr)) // create basket manager file. (bktmgr.udf)
	{
		BKTREC_ERR("Failed create basket manager. %s\n", path_mgr);
		return BKT_ERR;
	}

//////////////////
	hd.id = ID_BKTMGR_HEADER;

	time(&hd.date);
	hd.latest_update = hd.date;
	hd.reserved = 0;
	hd.bkt_count = bkt_count;

////////////////////////

	memset(&bd, 0, sizeof(bd));

	T_BASKET_HDR bkthd;
	memset(&bkthd, 0, sizeof(bkthd));

	// write bktmgr header info
	if(!WRITE_ST(fd_mgr, hd))
	{
		BKTREC_ERR("failed write basket mgr header\n");
		CLOSE_BKT(fd_mgr);
		return BKT_ERR;
	}

	for(i=1;i<=bkt_count;i++)
	{
		// create basket file
		sprintf(path_bkt,"%s/%08d.bkt", diskpath, i);

		if(!OPEN_CREATE(fd_bkt, path_bkt)) // create basket file.
		{
			BKTREC_ERR("failed making basket bkt file. %s\n", path_bkt);
			CLOSE_BKT(fd_mgr);
			return BKT_ERR;
		}

		memset(&bkthd, 0, sizeof(bkthd));
		bkthd.id = ID_BASKET_HDR;
		bkthd.bid = i;
		time(&cur_sec);
		bkthd.latest_update=cur_sec;

		if(!WRITE_ST(fd_bkt, bkthd))
		{
			BKTREC_ERR("failed write basket header\n");
			CLOSE_BKT(fd_mgr);
			CLOSE_BKT(fd_bkt);
			return BKT_ERR;
		}

		//ftruncate(fd_bkt, BASKET_SIZE);
		CLOSE_BKT(fd_bkt);
		truncate(path_bkt, BASKET_SIZE);

		// create idx file
		sprintf(path_idx,"%s/%08d.idx", diskpath, i);
		if(!OPEN_CREATE(fd_idx, path_idx)) // create index file
		{
			BKTREC_ERR("failed create index.%s\n", path_idx);
			CLOSE_BKT(fd_mgr);
			CLOSE_BKT(fd_bkt);
			return BKT_ERR;
		}
		CLOSE_BKT(fd_idx);

		//wirte basket file info to bktmgr
		bd.bid = i;
		sprintf(bd.path, "%s", diskpath);

		if(!WRITE_ST(fd_mgr, bd))
		{
			BKTREC_ERR("failed write basket mgr body\n");
			CLOSE_BKT(fd_mgr);
			CLOSE_BKT(fd_bkt);

			return BKT_ERR;
		}
	}

	CLOSE_BKT(fd_mgr);

	//// record initial used size after format and basket create ////
	unsigned long mntTot, mntUsed;
	get_dvr_disk_size(diskpath, &mntTot, &mntUsed);
	tm.initUsedSize = mntUsed+4;

	if(tm.hddIdx >= 0)
	{
		if(!WRITE_ST(fd_tm, tm))
		{
			BKTREC_ERR("failed write top mgr file\n");
			CLOSE_BKT(fd_tm);
			return BKT_ERR;
		}
	}

	CLOSE_BKT(fd_tm);

	g_bkt_rec_space  =BKT_SPACE_REMAIN;

  return bkt_count;
}


long BKTREC_deleteBasket(const char *path_in)
{
	BKT_closeAll();

    T_BKTMGR_HDR  hd;
    T_BKTMGR_BODY bd;
	T_BKTMGR_BODY bd_new;

#ifdef BKT_SYSIO_CALL
	int fd, fd_bkt, fd_idx;
#else
	FILE *fd, *fd_bkt, *fd_idx;
#endif

	int i=0;
	char path_bkt[LEN_PATH];
	char path_idx[LEN_PATH];
	char buf[30];
	time_t cur_sec;

	// remove current record basket info.
	sprintf(buf, "%s/%s", path_in, NAME_BKTREC_INFO);
	if(-1 != access(buf, 0))
	{
		TRACE0_BKTREC("remove basket record info file. %s\n", buf);
		remove(buf);
	}

	//////////////////////////////////////////////////////////////////////////
	// remove rdbs
	TRACE0_BKTREC("remove rdb files\n");
	struct dirent *ent;
	DIR *dp;

	sprintf(buf, "%s/rdb", path_in);
	dp = opendir(buf);
	if (dp != NULL)
	{
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

			sprintf(buf,"%s/rdb/%s", path_in, ent->d_name);
			TRACE0_BKTREC("remove rdb data. %s\n", buf);
			remove(buf);
		}

		closedir(dp);
	}

	sprintf(buf,"%s/rdb", path_in);
	TRACE0_BKTREC("remove rdb directory. %s\n", buf);
	rmdir(buf);
	//////////////////////////////////////////////////////////////////////////////////////

	sprintf(buf, "%s/%s", path_in, NAME_BKTMGR);
	if(-1 == access(buf, 0))
	{
		BKTREC_ERR("can't finid basket manager file. %s. Create new basket files.\n", buf);

		return BKTREC_CreateBasket(path_in);
	}

	if(!OPEN_RDWR(fd, buf)) // open bktmgr.udf file
	{
		BKTREC_ERR("failed open basket manager file. %s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("failed read basket manager header.\n");
		CLOSE_BKT(fd);

		return BKT_ERR;
	}

	TRACE0_BKTREC("ready delete all basket files -- count:%ld\n", hd.bkt_count);

	for(i=0;i<hd.bkt_count;i++)
	{
		if(!READ_ST(fd, bd))
		{
			BKTREC_ERR("failed read basket manager body.\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		sprintf(path_bkt, "%s/%08ld.bkt", bd.path, bd.bid);
		if(!OPEN_EMPTY(fd_bkt, path_bkt)) // open basket file as empty for delete.
		{
			BKTREC_ERR("failed delete bkt data. %s\n", path_idx);
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		CLOSE_BKT(fd_bkt);
		TRACE0_BKTREC("delete bkt %s\n", path_bkt);


		sprintf(path_idx, "%s/%08ld.idx", bd.path, bd.bid);
		if(!OPEN_EMPTY(fd_idx, path_idx)) // open index file as empty for delete.
		{
			BKTREC_ERR("failed delete idx data. %s\n", path_idx);
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		CLOSE_BKT(fd_idx);
		TRACE0_BKTREC("delete idx %s\n", path_idx);


		//////////////////////////////////////////////////////////////////////////
		// update bakset manager file.
		memset(&bd_new, 0, sizeof(bd_new));
		bd_new.id = ID_BASKET_HDR;
		bd_new.bid = bd.bid;

		time(&cur_sec);
		bd_new.latest_update=cur_sec;
		sprintf(bd_new.path, "%s", bd.path);

		if(!LSEEK_SET(fd, sizeof(bd_new)*i+sizeof(hd)))
		{
			BKTREC_ERR("failed seek bakset manager.\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		if(!WRITE_ST(fd, bd_new))
		{
			BKTREC_ERR("failed write basket mgr body\n");
			CLOSE_BKT(fd);

			return BKT_ERR;
		}
		// end update basket manager file
		//////////////////////////////////////////////////////////////////////////
	}

	TRACE0_BKTREC("Succeed init data basket manager file. %s\n", buf);
	CLOSE_BKT(fd);

	sleep(5); // BK 100107

	g_bkt_rec_space  =BKT_SPACE_REMAIN;

	return 1;
}

long BKTREC_showBasket(const char *path_in)
{
	T_BKTMGR_HDR  hd;
	T_BKTMGR_BODY bd;

    BKT_FHANDLE fd;

	int i=0;
	char buf[30];


	TRACE0_BKTREC("finid basket files for %s. \n", path_in);

	sprintf(buf, "%s/%s", path_in, NAME_BKTMGR);
	if(-1 == access(buf, 0))
	{
		BKTREC_ERR("can't finid basket manager file. %s. \n", buf);

		return -1;
	}

	if(!OPEN_RDWR(fd, buf)) // open bktmgr.udf file
	{
		BKTREC_ERR("failed open basket manager file. %s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("failed read basket manager header.\n");
		CLOSE_BKT(fd);

		return BKT_ERR;
	}

	TRACE0_BKTREC("basket files -- count:%ld\n", hd.bkt_count);

	for(i=0;i<hd.bkt_count;i++)
	{
		if(!READ_ST(fd, bd))
		{
			BKTREC_ERR("failed read basket manager body.\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}
		/*
		typedef struct _tBktMgrBody
		{
		    long id;
		    long bid;
		    char path[LEN_PATH];
		    long latest_update;
		    long lPriority;
		    long s_type;
				T_BASKET_TIME ts;
		    long save_flag;
		    long lRecycleCount;
		    long reserved;
		}T_BKTMGR_BODY;
		*/

		printf(" [%d] file=%08ld.bkt", i+1, bd.bid);

		//printf("\n\t ID=%ld(0x%04X)", bd.id, bd.id);
		//printf("\n\t BID=%ld(0x%04X)", bd.bid, bd.bid);
		//printf("\n\t path=%s", bd.path);
		T_BKT_TM t;
		BKT_GetLocalTime(bd.latest_update, &t);
		printf("\n\t last=%04d/%02d/%02d %02d:%02d:%02d", t.year, t.mon, t.day, t.hour, t.min, t.sec);
		printf("\n\t priority=%ld(0x%04lX)", bd.lPriority, bd.lPriority);
		printf("\n\t type=%ld(0x%04lX)", bd.s_type, bd.s_type);
		printf("\n\t flag=%ld(0x%04lX)", bd.save_flag, bd.save_flag);
		printf("\n\t recyle=%ld(0x%04lX)", bd.lRecycleCount, bd.lRecycleCount);

		//printf(", size=%ld(MB)", bd_new.latest_update);
		//printf(", last access=%04d/%02d/%02d %02d:%02d:%02d", bd_new.latest_update);

		printf("\n");

	}

	TRACE0_BKTREC("Succeed init data basket manager file. %s\n", buf);
	CLOSE_BKT(fd);

	return 1;
}

long BKTREC_searchUsingBasket(char* pname)
{
    BKT_FHANDLE fd;
	T_BKTREC_INFO bi;
	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTREC_INFO);

	if(-1 == access(buf, 0))
	{
		BKTREC_ERR("There is no %s file.\n", buf);
		return BKT_ERR;
	}

	if(!OPEN_RDONLY(fd, buf))
	{
		BKTREC_ERR("failed open basket rec info file.path:%s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, bi))
	{
		BKTREC_ERR("failed read basket rec info data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	if(bi.save_flag == SF_USING)
	{
		////MULTI-HDD SKSUNG////
		sprintf(pname, "%s/%08ld", /*bi.path*/gBKTREC_Ctrl.target_path, bi.bid);
		////////////////////////
		TRACE0_BKTREC("found using basket:%s\n", pname);

		return bi.bid;
	}

	//TRACE0_BKTREC("There is no using basket.\n");

    return BKT_ERR;
}

long BKTREC_searchStoppedBasket(char* pname)
{
    BKT_FHANDLE fd;
	T_BKTREC_INFO bi;
	char buf[30];

	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTREC_INFO);

	if(-1 == access(buf, 0))
	{
		BKTREC_ERR("There is no %s file.\n", buf);
		return BKT_ERR;
	}

	if(!OPEN_RDONLY(fd, buf))
	{
		BKTREC_ERR("failed open basket rec info file. path:%s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, bi))
	{
		BKTREC_ERR("failed read basket rec info data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}
	CLOSE_BKT(fd);

	if(bi.save_flag == SF_STOP)
	{
		////MULTI-HDD SKSUNG////
		sprintf(pname, "%s/%08ld", /*bi.path*/gBKTREC_Ctrl.target_path, bi.bid);
		////////////////////////
		TRACE0_BKTREC("found stop basket:%s\n", pname);
		return bi.bid;
	}

	//TRACE0_BKTREC("There is no stopped basket.\n");

    return BKT_ERR;
}

long BKTREC_searchEmptyBasket(char* pname)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	T_BKTMGR_HDR  hd;
	T_BKTMGR_BODY bd;

	int i;
	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTMGR);

	// open basket manager
	if(!OPEN_RDONLY(fd, buf))
	{
		BKTREC_ERR("failed open basket manager. path:%s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("failed read basket manager header\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	for(i=0;i< hd.bkt_count;i++)
	{
		if(!READ_ST(fd, bd))
		{
			BKTREC_ERR("failed read basket manager body\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		if(bd.save_flag == SF_EMPTY)
		{
			////MULTI-HDD SKSUNG////
			sprintf(pname, "%s/%08ld", /*bd.path*/gBKTREC_Ctrl.target_path, bd.bid);
			////////////////////////
			TRACE0_BKTREC("found empty basket id:%08ld, path:%s\n", bd.bid, pname);
			CLOSE_BKT(fd);

			return bd.bid;
		}
	}

	CLOSE_BKT(fd);

	TRACE0_BKTREC("There is no empty basket file.\n");

    return BKT_ERR;
}

int BKTREC_removeRDB(long rmbid, long sec)
{
	T_BKT_TM tm1;
	BKT_GetLocalTime(sec, &tm1);

#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	int i, c;
	char path[LEN_PATH];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	//////////////////////////////////////////////////////////////////////////
    struct dirent *ent;
    DIR *dp;

	char buf[30];
	sprintf(buf, "%s/rdb/", gBKTREC_Ctrl.target_path);
    dp = opendir(buf);
	if(dp == NULL)
	{
		BKTREC_ERR("There is no rdb directory %s.\n", buf);
		return BKT_ERR;
	}

	// base.
	sprintf(buf, "%04d%02d%02d", tm1.year, tm1.mon, tm1.day);
	int cur_rdb=atoi(buf);
	int del_rdb=0;

    for(;;)
    {
        ent = readdir(dp);
        if (ent == NULL)
            break;

		if (ent->d_name[0] == '.') // except [.] and [..]
		{
			if (!ent->d_name[1] || (ent->d_name[1] == '.' && !ent->d_name[2]))
			{
				continue;
			}
		}

		del_rdb = atoi(ent->d_name);

		if(del_rdb < cur_rdb)
		{
			sprintf(path, "%s/rdb/%s", gBKTREC_Ctrl.target_path, ent->d_name);
	 		remove(path);
	 		TRACE0_BKTREC("remove old rdb file.%s\n", path);
		}
    }

    closedir(dp);
	//////////////////////////////////////////////////////////////////////////


	// delete rdb info, if less than param time.
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTREC_Ctrl.target_path, tm1.year, tm1.mon, tm1.day);
	if(!OPEN_RDWR(fd, path)) // open rdb file.
	{
		BKTREC_ERR("failed open rdb %s\n", path);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTREC_ERR("failed read event data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		BKTREC_ERR("failed read rdb data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	// need speed up
#if 0 // method 1
	int bmin[BKT_MAX_CHANNEL]={-1,};
	int emin[BKT_MAX_CHANNEL]={-1,};

	for(c=0;c<BKT_MAX_CHANNEL;c++)
	{
		// find begin minute
		for(i=0;i<TOTAL_MINUTES_DAY;i+=2)
		{
			if(rdbs[c*TOTAL_MINUTES_DAY+i].bid == bid)
			{
				bmin[c] = i;
				TRACE0_BKTREC("found delete b-sec [%02d:%02d]. CH:%d, bid:%ld\n", i/60, i%60, c,  bid);
				break;
			}

			if(rdbs[c*TOTAL_MINUTES_DAY+i+1].bid == bid)
			{
				bmin[c] = i+1;
				TRACE0_BKTREC("found delete b-sec [%02d:%02d]. CH:%d, bid:%ld\n", (i+1)/60, (i+1)%60, c,  bid);
				break;
			}
			//OSA_waitMsecs(1);
		}
	}

	for(c=0;c<BKT_MAX_CHANNEL;c++)
	{
		// find begin minute
		if(bmin[c] != -1)
		{
			for(i=TOTAL_MINUTES_DAY-1;i>=bmin[c];i-=2)
			{
				if(rdbs[c*TOTAL_MINUTES_DAY+i].bid == bid)
				{
					emin[c] = i;
					TRACE0_BKTREC("found delete e-sec [%02d:%02d]. CH:%d, bid:%ld\n", i/60, i%60, c,  bid);

					memset(evts+c*TOTAL_MINUTES_DAY+bmin[c], 0, emin[c]-bmin[c]+1);
					memset(rdbs+c*TOTAL_MINUTES_DAY+bmin[c], 0, (emin[c]-bmin[c]+1)*sizeof(T_BASKET_RDB));
					break;
				}

				if(rdbs[c*TOTAL_MINUTES_DAY+i-1].bid == bid)
				{
					emin[c] = i-1;
					TRACE0_BKTREC("found delete e-sec [%02d:%02d]. CH:%d, bid:%ld\n", (i-1)/60, (i-1)%60, c,  bid);

					memset(evts+c*TOTAL_MINUTES_DAY+bmin[c], 0, emin[c]-bmin[c]+1);
					memset(rdbs+c*TOTAL_MINUTES_DAY+bmin[c], 0, (emin[c]-bmin[c]+1)*sizeof(T_BASKET_RDB));
					break;
				}
			}
			//OSA_waitMsecs(1);//091209
		}
	}
#else // method 2

	for(c=0;c<BKT_MAX_CHANNEL;c++)
	{
		for(i=0;i<TOTAL_MINUTES_DAY;i+=2)
		{
			if(rdbs[c*TOTAL_MINUTES_DAY+i].bid == rmbid)
			{
				evts[c*TOTAL_MINUTES_DAY+i] = EVT_NONE;
				rdbs[c*TOTAL_MINUTES_DAY+i].bid = 0;
				rdbs[c*TOTAL_MINUTES_DAY+i].idx_pos = 0;

				//TRACE0_BKTREC("remove rdb. BID:%ld, CH:%d, [%02d:%02d]\n", rmbid, c, i/60, i%60);
			}

			if(rdbs[c*TOTAL_MINUTES_DAY+(i+1)].bid == rmbid)
			{
				evts[c*TOTAL_MINUTES_DAY+(i+1)] = EVT_NONE;
				rdbs[c*TOTAL_MINUTES_DAY+(i+1)].bid = 0;
				rdbs[c*TOTAL_MINUTES_DAY+(i+1)].idx_pos = 0;

				//TRACE0_BKTREC("remove rdb. BID:%ld, CH:%d, [%02d:%02d]\n", rmbid, c, (i+1)/60, (i+1)%60);
			}
		}
	}
#endif

	if(!LSEEK_SET(fd, 0))
	{
		BKTREC_ERR("failed seek rdb file.\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!WRITE_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTREC_ERR("failed write event data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!WRITE_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		BKTREC_ERR("failed write rdb data\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	return BKT_OK;
}

long BKTREC_searchOldestBasket(char* pname)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

	T_BKTMGR_HDR  hd;
	T_BKTMGR_BODY bd;

	int i;
	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTMGR);



	// open basket manager
	if(!OPEN_RDONLY(fd, buf))
	{
		BKTREC_ERR("failed open basket manager. path:%s\n", buf);
		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("failed read basket manager header\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	// read first basket info.
	if(!READ_ST(fd, bd))
	{
		BKTREC_ERR("failed read basket manager first body\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	long min_ts=BKT_GetMinTS(bd.ts.t2);
	long tmp_ts=min_ts;
	long bid = bd.bid;

	sprintf(pname, "%s/%08ld", /*bd.path*/gBKTREC_Ctrl.target_path, bd.bid);

	TRACE0_BKTREC("begin search oldest basket : %s\n", ctime(&min_ts));

	// read from second info..beacause already first body data above and we need seed TS to compare
	for(i=1; i<hd.bkt_count;i++)
	{
		if(!READ_ST(fd, bd))
		{
			perror("read basket info");
			BKTREC_ERR("failed read basket manager header. bkt count:%ld, read bid:%d\n", hd.bkt_count, i);
			break;
		}

		if(bd.save_flag== SF_FULL)
		{
			tmp_ts = min(min_ts, BKT_GetMinTS(bd.ts.t2));

			if(tmp_ts != min_ts)
			{
				min_ts = tmp_ts;
				sprintf(pname, "%s/%08ld", /*bd.path*/gBKTREC_Ctrl.target_path, bd.bid);
				bid = bd.bid;

				TRACE0_BKTREC("found oldest basket id:%ld, ts:%s, path:%s\n", bd.bid, ctime(&min_ts), pname);
			}
		}

		//OSA_waitMsecs(1);
	}

	CLOSE_BKT(fd);

	BKTREC_removeRDB(bid, min_ts);

	return bid;
}

// search basket file for recording
long
BKTREC_searchBasket(char *o_path, // output basket file path
					long *o_bid,  // output basket ID
					BOOL in_all)  // search option only empty or all
{
	long next_basket=-1;
	char path[30];
	sprintf(path, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTMGR);
	if(-1 == access(path, 0))
	{
		BKTREC_ERR("There is no basket manager file.Please verify target directory. path:%s\n", path);
		return BKT_ERR;
	}

	if(in_all)
	{
		TRACE0_BKTREC("searching using files----\n");
		next_basket = BKTREC_searchUsingBasket(path);

		if( BKT_ERR != next_basket)
		{
			*o_bid = next_basket;
			strcpy(o_path, path);

			TRACE0_BKTREC("found using basket file -- ID:%08ld, path:%s\n", *o_bid, path);

			return SF_USING;
		}

		TRACE0_BKTREC("searching stopped files----\n");
		next_basket = BKTREC_searchStoppedBasket(path);
		if( BKT_ERR != next_basket)
		{
			*o_bid = next_basket;
			strcpy(o_path, path);

			TRACE0_BKTREC("found stop basket file -- ID:%08ld, path:%s\n", *o_bid, path);

			return SF_STOP;
		}
	}// if(in_all)

	TRACE0_BKTREC("searching empty file.\n");
	if(g_bkt_rec_space == BKT_SPACE_REMAIN)
	{
		next_basket = BKTREC_searchEmptyBasket(path);
		if( next_basket != BKT_ERR)
		{
			*o_bid = next_basket;
			strcpy(o_path, path);

			//TRACE0_BKTREC("found empty basket file -- ID:%08ld, path:%s\n", *o_bid, path);

			return SF_EMPTY;
		}
	}

	TRACE0_BKTREC("searching oldest file.\n");

	g_bkt_rec_space = BKT_SPACE_FULL;
	next_basket = BKTREC_searchOldestBasket(path);
	if( next_basket != BKT_ERR)
	{
		*o_bid = next_basket;
		strcpy(o_path, path);

		//TRACE0_BKTREC("found oldest basket file -- ID:%08ld, path:%s\n", *o_bid, path);

		return SF_FULL;
	}

    return BKT_ERR;
}

long BKTREC_setBasketRecinfo(long bid, int flag, char *path)
{
    BKT_FHANDLE fd;
	T_BKTREC_INFO bi;

	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTREC_INFO);

	if(-1 != access(buf, 0)) // existence only
	{
		// BK - 0607
#ifdef BKT_OPEN_MODE_RDWR
		if(!OPEN_RDWR(fd, buf)) // open bkt_rec_info.udf file.
#else
		if(!OPEN_EMPTY(fd, buf)) // open bkt_rec_info.udf file.
#endif
		{
			BKTREC_ERR("failed open bkt rec info file. path:%s\n", buf);
			return BKT_ERR;
		}
	}
	else
	{
		if(!OPEN_CREATE(fd, buf)) // create basket record info file.
		{
			BKTREC_ERR("failed create bkt rec info file. path:%s\n", buf);
			return BKT_ERR;
		}
	}


	bi.bid = bid;
	strcpy(bi.path, path);
	bi.save_flag = flag;

	if(!WRITE_ST(fd, bi))
	{
		BKTREC_ERR("failed write basket rec info\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	TRACE0_BKTREC("Opened basket, set current rec info BID:%ld, SF:%s, path:%s\n", bid, g_szSaveFlags[flag], path);

	return BKT_OK;
}


long BKTREC_setBasketStatus(long bktid, int save_flag)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

    T_BKTMGR_HDR hd;
    T_BKTMGR_BODY bd;

	int i;
	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTMGR);

	// BK - 0607
    if(!OPEN_RDWR(fd, buf)) // open bktmgr.udf file.
	{
    	BKTREC_ERR("failed open basket manager file. path:%s\n", buf);
    	return BKT_ERR;
    }

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("failed read basket manager header\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	for(i=0;i<hd.bkt_count;i++)
	{
		if(!READ_ST(fd, bd)) {
			BKTREC_ERR("failed read basket manager body\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		if(bd.bid == bktid)
		{
			bd.save_flag = save_flag;
			time(&bd.latest_update);

			if(!LSEEK_SET(fd, sizeof(hd)+i*sizeof(bd)))
			{
				BKTREC_ERR("failed seek bakset manager.\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}


			if(!WRITE_ST(fd, bd)) {
				BKTREC_ERR("failed write basket manager body\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}

			CLOSE_BKT(fd);

			////MULTI-HDD SKSUNG////
			BKTREC_setBasketRecinfo(bktid, save_flag, /*bd.path*/gBKTREC_Ctrl.target_path);
			////////////////////////

			TRACE0_BKTREC("Done set basket status. BID:%08ld, save_flag:%d\n", bktid, save_flag);

			return S_OK;
		}

	}//end for

  	CLOSE_BKT(fd);

	BKTREC_ERR("failed bid:%ld\n", bktid);

    return BKT_ERR;
}


int  BKTREC_OpenFullBasket(int stream_type)
{
	char path[30];

	//TRACE0_BKTREC("g_bkt_recycle:%d, g_bkt_rec_space:%d\n", g_bkt_recycle, g_bkt_rec_space);
	//TRACE0_BKTREC("gbkt_mount_info.curHddIdx:%d, gbkt_mount_info.hddInfo[curHddIdx].nextHddIdx:%d\n", gbkt_mount_info.curHddIdx, gbkt_mount_info.hddInfo[gbkt_mount_info.curHddIdx].nextHddIdx);
	//TRACE0_BKTREC("gBKTREC_Ctrl.bid:%d, gBKTREC_Ctrl.bkt_cnt:%d\n",  gBKTREC_Ctrl.bid, gBKTREC_Ctrl.bkt_cnt);

	////MULTI-HDD SKSUNG////
	if((gBKTREC_Ctrl.bid == gBKTREC_Ctrl.bkt_cnt ) && gbkt_mount_info.hddCnt > 1)  // jump to next Hdd
	{
		int curHddIdx, nextHddIdx;
		
		curHddIdx					= gbkt_mount_info.curHddIdx;
		nextHddIdx					= gbkt_mount_info.hddInfo[curHddIdx].nextHddIdx;
		gbkt_mount_info.curHddIdx 	= nextHddIdx;

		BKTREC_setTargetDisk(gbkt_mount_info.curHddIdx) ;

		memset(gBKTREC_Ctrl.prev_min, -1, sizeof(gBKTREC_Ctrl.prev_min));
	}
	/////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// init variables
	memset(&gBKTREC_Ctrl.bkt_secs, 0, sizeof(gBKTREC_Ctrl.bkt_secs));
	memset(&gBKTREC_Ctrl.idx_secs, 0, sizeof(gBKTREC_Ctrl.idx_secs));

	gBKTREC_Ctrl.fpos_bkt = 0;
	gBKTREC_Ctrl.fpos_idx = 0;
	gBKTREC_Ctrl.bid = 0;
	memset(gBKTREC_Ctrl.idx_pos, 0, sizeof(gBKTREC_Ctrl.idx_pos));

	//////////////////////////////////////////////////////////////////////////

	// get basket file name and id
	long newbid=0, saveflag;

	////MULTI_HDD SKSUNG////
	if(gbkt_mount_info.hddCnt > 1)
		saveflag = BKTREC_searchBasket(path, &newbid, TRUE);
	else
		saveflag = BKTREC_searchBasket(path, &newbid, FALSE);
	////////////////////////

	if( BKT_ERR != saveflag)
	{
		if(saveflag == SF_FULL && g_bkt_recycle == BKT_RECYCLE_ONCE)
		{
			g_bkt_rec_status = BKT_STATUS_FULL;
			TRACE0_BKTREC("################# basket is full ###############\n");
			return BKT_FULL;
	    }	
		
		sprintf(gBKTREC_Ctrl.path_bkt,"%s.bkt", path);
		sprintf(gBKTREC_Ctrl.path_idx, "%s.idx", path);

		T_BASKET_HDR bhd;

		//TRACE0_BKTREC("Opening basket :%s, idx:%s\n", gBKTREC_Ctrl.path_bkt, gBKTREC_Ctrl.path_idx);

		// BK - 0607
#ifdef BKT_OPEN_MODE_RDWR
		if(!OPEN_RDWR(g_handle_rec_bkt, gBKTREC_Ctrl.path_bkt)) // open basket file.
#else
		if(!OPEN_EMPTY(g_handle_rec_bkt, gBKTREC_Ctrl.path_bkt)) // open basket file.
#endif
		{
			BKTREC_ERR("Failed open basket file --- %s\n", gBKTREC_Ctrl.path_bkt);
			return BKT_ERR;
		}

		//read basket header
		memset(&bhd, 0, sizeof(bhd));

		bhd.id = ID_BASKET_HDR;
		bhd.bid = newbid;
		time(&bhd.latest_update);
		bhd.s_type = stream_type;
		bhd.save_flag   = SF_USING;

		if(!WRITE_ST(g_handle_rec_bkt, bhd))
		{
			BKTREC_ERR("failed write basket header. new mode\n");
			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}

		//////////////////////////////////////////////////////////////////////////
		T_INDEX_HDR  ihd;

		// BK - 0607
#ifdef BKT_OPEN_MODE_RDWR
		if(!OPEN_RDWR(g_handle_rec_idx, gBKTREC_Ctrl.path_idx)) // open index file.
#else
		if(!OPEN_EMPTY(g_handle_rec_idx, gBKTREC_Ctrl.path_idx)) // open index file.
#endif
		{
			BKTREC_ERR("cannot open idx file %s\n", gBKTREC_Ctrl.path_idx);

			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}

		ihd.id = ID_INDEX_HEADER;
		ihd.bid = gBKTREC_Ctrl.bid;
		memset(&ihd.ts, 0, sizeof(bhd.ts));
		ihd.count=0;

		if(!WRITE_ST(g_handle_rec_idx, ihd))
		{
			BKTREC_ERR("failed index header\n");
			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		gBKTREC_Ctrl.fpos_bkt = LTELL(g_handle_rec_bkt);
		gBKTREC_Ctrl.fpos_idx = LTELL(g_handle_rec_idx);
		gBKTREC_Ctrl.bid      = newbid;

		g_bkt_rec_status = BKT_STATUS_OPENED;
		//////////////////////////////////////////////////////////////////////////
		BKTREC_setBasketStatus(gBKTREC_Ctrl.bid, SF_USING);
		//////////////////////////////////////////////////////////////////////////

		TRACE0_BKTREC("Succeeded open basket for rec, BKT:%s, IDX:%s\n", gBKTREC_Ctrl.path_bkt, gBKTREC_Ctrl.path_idx);

		return BKT_OK;
    }

	BKTREC_ERR("cannot find next basket file.\n");

	return BKT_ERR;
}

long BKTREC_open(const char *target_mpoint)
{
	if(g_bkt_rec_status == BKT_STATUS_OPENED)
	{
		//TRACE0_BKTREC("Already opened basket file. BID:%ld\n", gBKTREC_Ctrl.bid);
		return BKT_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// init variables
	memset(&gBKTREC_Ctrl, 0, sizeof(gBKTREC_Ctrl));
	memset(gBKTREC_Ctrl.prev_min, -1, sizeof(gBKTREC_Ctrl.prev_min));

	if(0==strlen(target_mpoint))
	{
		BKTREC_ERR("Invalid target mount path[%s]. \n",target_mpoint);
		return BKT_ERR;
	}

	////MULTI-HDD SKSUNG////
	sprintf(gBKTREC_Ctrl.target_path, "%s", target_mpoint);
	BKTREC_setTargetDisk(gbkt_mount_info.curHddIdx);
	/////////////////////

	char path[30];
	//////////////////////////////////////////////////////////////////////////
	// get basket file name and id
    long saveflag = BKTREC_searchBasket(path, &gBKTREC_Ctrl.bid, TRUE);

    if( BKT_ERR == saveflag)
    {
    	BKTREC_ERR("cannot find next basket file.\n");
    	return BKT_ERR;
    }

	sprintf(gBKTREC_Ctrl.path_bkt, "%s.bkt", path);
	sprintf(gBKTREC_Ctrl.path_idx, "%s.idx", path);

	// stopped bkt..
	if(saveflag == SF_USING || saveflag == SF_STOP)
	{
		// open basket
		if(!OPEN_RDWR(g_handle_rec_bkt, gBKTREC_Ctrl.path_bkt))
		{
			BKTREC_ERR("failed open basket -- %s\n", gBKTREC_Ctrl.path_bkt);
			return BKT_ERR;
		}

		T_BASKET_HDR bhd;
		memset(&bhd, 0, sizeof(bhd));
		bhd.id = ID_BASKET_HDR;
		bhd.bid = gBKTREC_Ctrl.bid;
		time(&bhd.latest_update);
		bhd.save_flag   = SF_USING;
		bhd.reserved =0xBBBBBBBB;

		struct tm tm1;
		localtime_r(&bhd.latest_update, &tm1);
		int tm_day = tm1.tm_mday;

		//write basket header
		if(!WRITE_ST(g_handle_rec_bkt, bhd))
		{
			BKTREC_ERR("failed write basket header--- %s\n", gBKTREC_Ctrl.path_bkt);
			CLOSE_BKT(g_handle_rec_bkt);

			return BKT_ERR;
		}

		if(!OPEN_RDWR(g_handle_rec_idx, gBKTREC_Ctrl.path_idx))
		{
			BKTREC_ERR("cannot open idx file %s\n", gBKTREC_Ctrl.path_idx);
			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}

		T_INDEX_HDR ihdr;
		if(!READ_ST(g_handle_rec_idx, ihdr))
		{
			BKTREC_ERR("failed read index header\n");
			CLOSE_BKT(g_handle_rec_bkt);
			CLOSE_BKT(g_handle_rec_idx);
			return BKT_ERR;
		}
		TRACE0_BKTREC("Read index header. 0x%08ld, count=%d\n", ihdr.bid, ihdr.count);

		if(ihdr.count <= 0) // normal saved index..
		{
			long prev_sec = 0;
			long prev_usec = 0;
			int idx_count=0, bkt_fpos=0, idx_fpos=0;
			T_INDEX_DATA idd;
			memset(&idd, 0, sizeof(idd));

			while(READ_ST(g_handle_rec_idx, idd))
			{
//				if( (idd.ts.sec <= prev_sec && idd.ts.usec < prev_usec)
//addon fix
				if(((idd.ts.sec < prev_sec) || ((idd.ts.sec == prev_sec) && (idd.ts.usec < prev_usec)))	|| idd.id != ID_INDEX_DATA
					|| idd.fpos < bkt_fpos)
				{
					TRACE1_BKTREC("Fount last record index pointer. idd.id=0x%08x, sec=%d, usec=%d, fpos:%ld, count=%d\n",
						idd.id, prev_sec, prev_usec, bkt_fpos, idx_count);
					break;
				}

				prev_sec  = idd.ts.sec;
				prev_usec = idd.ts.usec;
				bkt_fpos = idd.fpos;
				idx_count++;
			}

			if(idx_count != 0)
			{
				idx_fpos = sizeof(idd)*idx_count+sizeof(T_INDEX_HDR);

				if(!LSEEK_SET(g_handle_rec_bkt, bkt_fpos))
				{
					BKTREC_ERR("failed seek basket.\n");
					CLOSE_BKT(g_handle_rec_bkt);
					CLOSE_BKT(g_handle_rec_idx);
					return BKT_ERR;
				}

				if(!LSEEK_SET(g_handle_rec_idx, idx_fpos))
				{
					BKTREC_ERR("failed seek index.\n");
					CLOSE_BKT(g_handle_rec_bkt);
					CLOSE_BKT(g_handle_rec_idx);
					return BKT_ERR;
				}

				long prev_sec = 0;
				long prev_usec = 0;
				T_STREAM_HDR shd;
				T_VIDEO_STREAM_HDR vhdr;
				T_AUDIO_STREAM_HDR ahdr;

				while(READ_HDR(g_handle_rec_bkt, shd))
				{
					if(shd.ts.sec <= prev_sec && shd.ts.usec < prev_usec)
						break;

					if(shd.id == ID_VIDEO_HEADER)
					{
						if(!LSEEK_CUR(g_handle_rec_bkt, sizeof(vhdr)+shd.framesize))
							break;
					}
					else if(shd.id == ID_AUDIO_HEADER)
					{
						if(!LSEEK_CUR(g_handle_rec_bkt, sizeof(ahdr)+shd.framesize))
							break;
					}
					else
						break;

					prev_sec = shd.ts.sec;
					prev_usec = shd.ts.usec;
					bkt_fpos = LTELL(g_handle_rec_bkt);
				}

				if(!LSEEK_SET(g_handle_rec_bkt, bkt_fpos))
				{
					BKTREC_ERR("failed seek basket.\n");
					CLOSE_BKT(g_handle_rec_bkt);
					CLOSE_BKT(g_handle_rec_idx);
					return BKT_ERR;
				}

			}
			else
			{
				bkt_fpos = sizeof(T_BASKET_HDR);
				idx_fpos = sizeof(T_INDEX_HDR);

				if(!LSEEK_SET(g_handle_rec_bkt, bkt_fpos))
				{
					BKTREC_ERR("failed seek basket.\n");
					CLOSE_BKT(g_handle_rec_bkt);
					CLOSE_BKT(g_handle_rec_idx);
					return BKT_ERR;
				}

				if(!LSEEK_SET(g_handle_rec_idx, idx_fpos))
				{
					BKTREC_ERR("failed seek index.\n");
					CLOSE_BKT(g_handle_rec_bkt);
					CLOSE_BKT(g_handle_rec_idx);
					return BKT_ERR;
				}
			}


		}
		else
		{
			T_STREAM_HDR shd;
			T_VIDEO_STREAM_HDR vhdr;
			T_AUDIO_STREAM_HDR ahdr;

			T_INDEX_DATA idd;

			if(!LSEEK_CUR(g_handle_rec_idx, sizeof(idd)*(ihdr.count-1)))
			{
				BKTREC_ERR("failed seek index.\n");
				CLOSE_BKT(g_handle_rec_bkt);
				CLOSE_BKT(g_handle_rec_idx);
				return BKT_ERR;
			}

			if(!READ_ST(g_handle_rec_idx, idd))
			{
				BKTREC_ERR("failed read index data.\n");
				CLOSE_BKT(g_handle_rec_bkt);
				CLOSE_BKT(g_handle_rec_idx);
				return BKT_ERR;
			}

			if(!LSEEK_SET(g_handle_rec_bkt, idd.fpos))
			{
				BKTREC_ERR("failed seek basket.\n");
				CLOSE_BKT(g_handle_rec_bkt);
				CLOSE_BKT(g_handle_rec_idx);
				return BKT_ERR;
			}

			long prev_sec = 0;
			long prev_usec = 0;
			long bkt_fpos = LTELL(g_handle_rec_bkt);

			while(READ_HDR(g_handle_rec_bkt, shd))
			{
				if(shd.ts.sec <= prev_sec && shd.ts.usec < prev_usec)
					break;

				if(shd.id == ID_VIDEO_HEADER)
				{
					if(!LSEEK_CUR(g_handle_rec_bkt, sizeof(vhdr)+shd.framesize))
						break;
				}
				else if(shd.id == ID_AUDIO_HEADER)
				{
					if(!LSEEK_CUR(g_handle_rec_bkt, sizeof(ahdr)+shd.framesize))
						break;
				}
				else
					break;

				prev_sec = shd.ts.sec;
				prev_usec = shd.ts.usec;
				bkt_fpos = LTELL(g_handle_rec_bkt);
			}

			if(!LSEEK_SET(g_handle_rec_bkt, bkt_fpos))
			{
				BKTREC_ERR("failed seek basket.\n");
				CLOSE_BKT(g_handle_rec_bkt);
				CLOSE_BKT(g_handle_rec_idx);
				return BKT_ERR;
			}

		}

		// end file pointer
		gBKTREC_Ctrl.fpos_bkt = LTELL(g_handle_rec_bkt);
		gBKTREC_Ctrl.fpos_idx = LTELL(g_handle_rec_idx);
		gBKTREC_Ctrl.prev_day = tm_day;

		g_bkt_rec_status = BKT_STATUS_OPENED;

		TRACE0_BKTREC("open last written file position bktfpos:%ld, idxfpos:%ld\n", gBKTREC_Ctrl.fpos_bkt, gBKTREC_Ctrl.fpos_idx);
	}
	else // new mode( empty, oldest bkt)
	{
		if(saveflag == SF_FULL && g_bkt_recycle == BKT_RECYCLE_ONCE)
		{
			BKTREC_ERR("basket is full and recycle mode is once\n");
			return BKT_FULL;
		}

		T_BASKET_HDR bhd;

		// open basket
#ifdef BKT_OPEN_MODE_RDWR
		if(!OPEN_RDWR(g_handle_rec_bkt, gBKTREC_Ctrl.path_bkt))
#else
		if(!OPEN_EMPTY(g_handle_rec_bkt, gBKTREC_Ctrl.path_bkt))
#endif
		{
			BKTREC_ERR("failed open basket -- %s\n", gBKTREC_Ctrl.path_bkt);
			return BKT_ERR;
		}

		memset(&bhd, 0, sizeof(bhd));
		bhd.id = ID_BASKET_HDR;
		bhd.bid = gBKTREC_Ctrl.bid;
		time(&bhd.latest_update);
		bhd.save_flag   = SF_USING;
		bhd.reserved =0xBBBBBBBB;

		struct tm tm1;
		localtime_r(&bhd.latest_update, &tm1);
		int tm_day = tm1.tm_mday;

		if(!WRITE_ST(g_handle_rec_bkt, bhd)){
			BKTREC_ERR("failed write basket header\n");
			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}

		 // BK - 0607
#ifdef BKT_OPEN_MODE_RDWR
		if(!OPEN_RDWR(g_handle_rec_idx, gBKTREC_Ctrl.path_idx))	// open basket idx
#else
		if(!OPEN_EMPTY(g_handle_rec_idx, gBKTREC_Ctrl.path_idx))// open basket idx
#endif
		{
			BKTREC_ERR("cannot open idx file %s\n", gBKTREC_Ctrl.path_idx);

			CLOSE_BKT(g_handle_rec_bkt);
			return BKT_ERR;
		}

		T_INDEX_HDR ihd;
		ihd.id = ID_INDEX_HEADER;
		ihd.bid = gBKTREC_Ctrl.bid;
		memset(&ihd.ts, 0, sizeof(ihd.ts));
		ihd.count=0;
		ihd.reserved=0xDDDDDDDD;

		if(!WRITE_ST(g_handle_rec_idx, ihd))
		{
			BKTREC_ERR("failed write index header\n");

			CLOSE_BKT(g_handle_rec_bkt);
			CLOSE_BKT(g_handle_rec_idx);

			return BKT_ERR;
		}

		//////////////////////////////////////////////////////////////////////////
		gBKTREC_Ctrl.fpos_bkt = LTELL(g_handle_rec_bkt);
		gBKTREC_Ctrl.fpos_idx = LTELL(g_handle_rec_idx);
		gBKTREC_Ctrl.prev_day = tm_day;

		g_bkt_rec_status = BKT_STATUS_OPENED;
	}

	//////////////////////////////////////////////////////////////////////////
	BKTREC_setBasketStatus(gBKTREC_Ctrl.bid, SF_USING);
	//////////////////////////////////////////////////////////////////////////

	TRACE0_BKTREC("Succeeded open basket for rec, BKT:%s, IDX:%s\n", gBKTREC_Ctrl.path_bkt, gBKTREC_Ctrl.path_idx);

	return BKT_OK;
}

long BKTREC_updateBasketManager(long bid, int save_flag)
{
#ifdef BKT_SYSIO_CALL
	int fd;
#else
	FILE *fd;
#endif

    T_BKTMGR_HDR  hd;
    T_BKTMGR_BODY mbd;

	int i;
	char buf[30];
	sprintf(buf, "%s/%s", gBKTREC_Ctrl.target_path, NAME_BKTMGR);

	// open basket manager
    if(!OPEN_RDWR(fd, buf))
	{
		BKTREC_ERR("Failed read basket manager header, count:%ld\n", hd.bkt_count);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	if(!READ_ST(fd, hd))
	{
		BKTREC_ERR("Failed read basket manager header, count:%ld\n", hd.bkt_count);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	for(i=0;i<hd.bkt_count;i++) {

		if(!READ_ST(fd, mbd)) {
			BKTREC_ERR("Failed read basket manager body\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		if(mbd.bid == bid) {

			memcpy(&mbd.ts, &gBKTREC_Ctrl.bkt_secs, sizeof(gBKTREC_Ctrl.bkt_secs));

			mbd.save_flag = save_flag;
			time(&mbd.latest_update);

			if(!LSEEK_SET(fd, sizeof(hd)+i*sizeof(mbd)))
			{
				BKTREC_ERR("failed seek bakset manager.\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}

			if(!WRITE_ST(fd, mbd))
			{
				BKTREC_ERR("Failed read basket manager body\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
			CLOSE_BKT(fd);

			////MULTI-HDD SKSUNG////
			BKTREC_setBasketRecinfo(bid, save_flag, /*mbd.path*/gBKTREC_Ctrl.target_path);
			////////////////////////

#ifdef BKTREC_DEBUG
			time_t te = mbd.ts.t2[0].sec;
			TRACE0_BKTREC("Updated Basket Manager -- BID:%ld, SF:%s, ETS:%s\n", bid, g_szSaveFlags[save_flag], ctime(&te));
#endif

			return S_OK;
		}


	}//end for

	CLOSE_BKT(fd);

    BKTREC_ERR("failed Update Basket Manager bid:%ld\n", bid);

    return BKT_ERR;
}

// close basket file
long BKTREC_exit(int save_flag, int flushbuffer)
{
    if(g_bkt_rec_status != BKT_STATUS_CLOSED)
	{
		if(flushbuffer == BKT_REC_CLOSE_BUF_FLUSH)
		{
			if(BKT_OK != BKTREC_FlushWriteBuffer())
				return BKT_ERR;
		}

	    T_BASKET_HDR bhdr;

		//////////////////////////////////////////////////////////////////////////
		// close index file
		T_INDEX_HDR idx_hdr;
		idx_hdr.id = ID_INDEX_HEADER;
		idx_hdr.count = BKT_GetRecordingIndexCount();
		idx_hdr.bid = gBKTREC_Ctrl.bid;

		TRACE0_BKTREC("save index last written ipos:%ld\n", gBKTREC_Ctrl.fpos_idx);
//addon fix
		memcpy(&idx_hdr.ts, &gBKTREC_Ctrl.idx_secs, sizeof(gBKTREC_Ctrl.idx_secs));

		if(!LSEEK_SET(g_handle_rec_idx, 0)) {
			perror("lseek");
			BKTREC_ERR("failed seek index.\n");
		}

		if(!WRITE_ST(g_handle_rec_idx, idx_hdr))
		{
			perror("write");
			BKTREC_ERR("failed write index hdr\n");
		}
		CLOSE_BKT(g_handle_rec_idx);

		TRACE0_BKTREC("closed index file. BID:%08ld, size:%ld KB, count:%ld\n", gBKTREC_Ctrl.bid, gBKTREC_Ctrl.fpos_idx/SIZE_KB, idx_hdr.count);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// close basket
	    bhdr.id    = ID_BASKET_HDR;
	    bhdr.bid   = gBKTREC_Ctrl.bid;
	    time(&bhdr.latest_update);
		bhdr.save_flag   = save_flag;			// save index file offset

	    // save timestamp
	    memcpy(&bhdr.ts, &gBKTREC_Ctrl.bkt_secs, sizeof(gBKTREC_Ctrl.bkt_secs));

		TRACE0_BKTREC("save basket last written fpos:%ld\n", gBKTREC_Ctrl.fpos_bkt);

	    // write basket
		if(!LSEEK_SET(g_handle_rec_bkt, 0))
		{
		    perror("lseek");
			BKTREC_ERR("failed seek bakset.\n");
		}

	    if(!WRITE_ST(g_handle_rec_bkt, bhdr))
	    {
	    	BKTREC_ERR("failed write basket hdr\n");
	    }
		CLOSE_BKT(g_handle_rec_bkt);
		TRACE0_BKTREC("closed basket file. BID:%08ld, size:%ld KB\n", gBKTREC_Ctrl.bid, gBKTREC_Ctrl.fpos_bkt/SIZE_KB);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// update basket manager
	    if(!BKTREC_updateBasketManager(gBKTREC_Ctrl.bid, save_flag))
		{

		}
		//////////////////////////////////////////////////////////////////////////

		////MULTI-HDD SKSUNG////
		// update hdd manager file
		BKTREC_setTargetDisk(gbkt_mount_info.curHddIdx);
		////////////////////////

		gBKTREC_Ctrl.fpos_bkt = 0;
		gBKTREC_Ctrl.fpos_idx = 0;
		gBKTREC_Ctrl.bid     = 0;
		g_bkt_rec_status = BKT_STATUS_CLOSED;
		gBKTREC_Ctrl.b_pos = 0;
		gBKTREC_Ctrl.i_pos = 0;
		gBKTREC_Ctrl.r_pos = 0;
		memset(gBKTREC_Ctrl.idx_pos, 0, sizeof(gBKTREC_Ctrl.idx_pos));

		memset(&gBKTREC_Ctrl.bkt_secs, 0, sizeof(gBKTREC_Ctrl.bkt_secs));
		memset(&gBKTREC_Ctrl.idx_secs, 0, sizeof(gBKTREC_Ctrl.idx_secs));
		memset(gBKTREC_Ctrl.prev_min, -1, sizeof(gBKTREC_Ctrl.prev_min));

		return S_OK;
	}
	/////////////////////////////////////////////////////////////////////

	BKTREC_ERR("Failed close basket file and index file. Already basket file is closed.\n");

    return BKT_ERR;
}

// close basket file
long BKTREC_SaveFull()
{
    if(g_bkt_rec_status != BKT_STATUS_CLOSED)
	{
	    T_BASKET_HDR bhdr;

		//////////////////////////////////////////////////////////////////////////
		// closing index file
		T_INDEX_HDR idx_hdr;
		idx_hdr.id    = ID_INDEX_HEADER;
		idx_hdr.count = BKT_GetRecordingIndexCount();
		idx_hdr.bid   = gBKTREC_Ctrl.bid;
		idx_hdr.reserved = 0x7FFFFFFF;
	    memcpy(&idx_hdr.ts, &gBKTREC_Ctrl.idx_secs, sizeof(gBKTREC_Ctrl.idx_secs));

		TRACE0_BKTREC("BID:%ld,  index count :%ld \n", idx_hdr.bid, idx_hdr.count);

		if(!LSEEK_SET(g_handle_rec_idx, 0))
		{
			BKTREC_ERR("failed seek index.\n");
		}

		if(!WRITE_ST(g_handle_rec_idx, idx_hdr))
		{
			BKTREC_ERR("failed write index hdr\n");
		}
		TRACE0_BKTREC("closed index file, count:%ld\n", idx_hdr.count);
		CLOSE_BKT(g_handle_rec_idx);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// closing basket
	    bhdr.id          = ID_BASKET_HDR;
	    bhdr.bid   = gBKTREC_Ctrl.bid;
	    time(&bhdr.latest_update);
		bhdr.save_flag   = SF_FULL;			// save index file offset

	    // save timestamp
	    memcpy(&bhdr.ts, &gBKTREC_Ctrl.bkt_secs, sizeof(gBKTREC_Ctrl.bkt_secs));

	    // write basket
		if(!LSEEK_SET(g_handle_rec_bkt, 0))
		{
			BKTREC_ERR("failed seek basket.\n");
		}

	    if(!WRITE_ST(g_handle_rec_bkt, bhdr))
	    {
	    	BKTREC_ERR("failed write basket hdr\n");
	    }
	    TRACE0_BKTREC("closed basket id:%08ld, size:%ld KB, idx size:%ld KB, bktpos:%ld, idxpos:%ld\n", gBKTREC_Ctrl.bid, gBKTREC_Ctrl.fpos_bkt/SIZE_KB, gBKTREC_Ctrl.fpos_idx/SIZE_KB, gBKTREC_Ctrl.fpos_bkt, gBKTREC_Ctrl.fpos_idx);
		CLOSE_BKT(g_handle_rec_bkt);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// update basket manager
	    if(!BKTREC_updateBasketManager(gBKTREC_Ctrl.bid, SF_FULL))
		{

		}
		//////////////////////////////////////////////////////////////////////////

		gBKTREC_Ctrl.fpos_bkt = 0;
		gBKTREC_Ctrl.fpos_idx = 0;


//		gBKTREC_Ctrl.bid      = 0;

		//g_bkt_status = BKT_STATUS_CLOSED;

		memset(&gBKTREC_Ctrl.bkt_secs, 0, sizeof(gBKTREC_Ctrl.bkt_secs));
		memset(&gBKTREC_Ctrl.idx_secs, 0, sizeof(gBKTREC_Ctrl.idx_secs));

		return S_OK;
	}
	/////////////////////////////////////////////////////////////////////

	BKTREC_ERR("Failed close basket file and index file. Already basket file is closed.\n");

    return BKT_ERR;
}

long BKTREC_checkBasketSize(long fpbasket, long fpindex, long framesize)
{
	long basketsize = fpbasket + framesize + fpindex;

    return (basketsize < BASKET_SIZE);
}

int BKTREC_updateRDB(T_BASKET_RDB_PARAM *pr)
{
	int ch   = pr->ch;
	long sec = pr->sec;

	struct tm tm1;
	localtime_r(&sec, &tm1);
	int tm_year = tm1.tm_year+1900;
	int tm_mon  = tm1.tm_mon+1;
	int tm_day  = tm1.tm_mday;
	int tm_hour = tm1.tm_hour;
	int tm_min  = tm1.tm_min;
	int tm_sec  = tm1.tm_sec;

    BKT_FHANDLE fd;

	char path[LEN_PATH];
	sprintf(path, "%s/rdb/%04d%02d%02d", gBKTREC_Ctrl.target_path, tm_year, tm_mon, tm_day);

	//////////////////////////////////////////////////////////////////////////
	// update rdb
	if(-1 != access(path, 0)) // existence only
	{
		if(!OPEN_RDWR(fd, path))
		{
			BKTREC_ERR("failed open rdb file:%s\n", path);
			return BKT_ERR;
		}

		// write event;
		long offset = TOTAL_MINUTES_DAY*ch+(tm_hour*60+tm_min);
		if(!LSEEK_SET(fd, offset))
		{
			BKTREC_ERR("failed seek evt.\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		char evt = pr->evt;
		if(!WRITE_PTSZ(fd, &evt, 1))
		{
			BKTREC_ERR("failed write rdb event:%d\n", evt);
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		// write rdb(bid, pos)
		offset = TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL + sizeof(T_BASKET_RDB)*(TOTAL_MINUTES_DAY*ch +(tm_hour*60+tm_min));
		if(!LSEEK_SET(fd, offset))
		{
			BKTREC_ERR("failed seek rdbs.\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		T_BASKET_RDB rdb;
		rdb.bid = pr->bid;
		rdb.idx_pos = pr->idx_pos;

		if(!WRITE_ST(fd, rdb))
		{
			BKTREC_ERR("failed write rdb bid\n");
			CLOSE_BKT(fd);
			return BKT_ERR;
		}

		CLOSE_BKT(fd);

		//TRACE0_BKTREC("update rdb CH:%d, BID:%ld, TS:%04d-%02d-%02d %02d:%02d:%02d, ipos:%ld\n", ch, pr->bid, tm_year, tm_mon, tm_day, tm_hour, tm_min, tm_sec, pr->idx_pos);

		return BKT_OK;
	}


	//////////////////////////////////////////////////////////////////////////
	// create rdb
	long offset;
	char buf[30];
	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	memset(evts,0,sizeof(evts));
	memset(rdbs,0,sizeof(rdbs));

	sprintf(buf,"%s/rdb", gBKTREC_Ctrl.target_path);
	if(-1 == access(buf, 0))
	{
		if( 0 != mkdir(buf, 0755))
			return BKT_ERR;
	}

	if(!OPEN_CREATE(fd, path)) // create basket rdb file.(ex, /dvr/data/sdda/rdb/20100610)
	{
		BKTREC_ERR("failed create rdb file:%s\n", path);
		return BKT_ERR;
	}

	offset = TOTAL_MINUTES_DAY*ch+(tm_hour*60+tm_min);
	evts[offset] = pr->evt;
	if(!WRITE_PTSZ(fd, evts, sizeof(evts)))
	{
		BKTREC_ERR("failed write events:%d\n", pr->evt);
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	// write rdb
	offset = sizeof(T_BASKET_RDB)*(TOTAL_MINUTES_DAY*ch+(tm_hour*60+tm_min));
	rdbs[offset].bid     = pr->bid;
	rdbs[offset].idx_pos = pr->idx_pos;
	if(!WRITE_PTSZ(fd, rdbs, sizeof(rdbs)))
	{
		BKTREC_ERR("failed write rdbs\n");
		CLOSE_BKT(fd);
		return BKT_ERR;
	}

	CLOSE_BKT(fd);

	TRACE0_BKTREC("create rdb CH:%d, BID:%ld, TS:%04d-%02d-%02d %02d:%02d:%02d, ipos:%ld\n",
	ch, pr->bid, tm_year, tm_mon, tm_day, tm_hour, tm_min, tm_sec, pr->idx_pos);

	return BKT_OK;
}

int BKTREC_Flush_RDBDATA()
{
#ifdef BKT_SYSIO_CALL
	int fd=0;
#else
	FILE *fd=NULL;
#endif

	int i;
	T_BASKET_RDB_PARAM rdb_prm;
	int rdb_cnt = gBKTREC_Ctrl.r_pos/sizeof(rdb_prm);
	int prev_day = 0;
	int open_rdb=0;
	int beUpdate=0;

	char path[LEN_PATH];
	char buf[LEN_PATH];

	char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];
	T_BASKET_RDB rdbs[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

	memset(evts,0,sizeof(evts));
	memset(rdbs,0,sizeof(rdbs));


	for( i=0; i < rdb_cnt ; i++)
	{
		memcpy(&rdb_prm, (gBKTREC_Ctrl.r_buf+i*sizeof(rdb_prm)), sizeof(rdb_prm));

		struct tm tm1;
		localtime_r(&rdb_prm.sec, &tm1);
		int tm_year = tm1.tm_year+1900;
		int tm_mon  = tm1.tm_mon+1;
		int tm_day  = tm1.tm_mday;
		int tm_hour = tm1.tm_hour;
		int tm_min  = tm1.tm_min;

		if(open_rdb==0)
		{
			sprintf(path, "%s/rdb/%04d%02d%02d", gBKTREC_Ctrl.target_path, tm_year, tm_mon, tm_day);

			// update rdb
			if(-1 != access(path, 0)) // existence only
			{
				if(!OPEN_RDWR(fd, path))
				{
					BKTREC_ERR("failed open rdb file:%s\n", path);
					return BKT_ERR;
				}

				if(!READ_PTSZ(fd, evts, sizeof(evts)))
				{
					BKTREC_ERR("failed read evts data\n");
					CLOSE_BKT(fd);
					return BKT_ERR;
				}

				if(!READ_PTSZ(fd, rdbs, sizeof(rdbs)))
				{
					BKTREC_ERR("failed read rdb data\n");
					CLOSE_BKT(fd);
					return BKT_ERR;
				}
			}
			else // create
			{
				sprintf(buf,"%s/rdb", gBKTREC_Ctrl.target_path);
				if(-1 == access(buf, 0))
				{
					if( 0 != mkdir(buf, 0755))
					{
						BKTREC_ERR("failed create rdb directory:%s\n", buf);
						return BKT_ERR;
					}
				}
						TRACE1_BKTREC("success create rdb directory:%s\n", buf);

				if(!OPEN_CREATE(fd, path)) // create basket rdb file.(ex, /dvr/data/sdda/rdb/20100610)
				{
					BKTREC_ERR("failed create rdb file:%s\n", path);
					return BKT_ERR;
				}
						TRACE1_BKTREC("Success create rdb directory:%s\n", buf);
			}

			//////////////////////////////////////////////////////////////////////////
			prev_day = tm_day;
			open_rdb = 1;
			//////////////////////////////////////////////////////////////////////////
		}

		if(prev_day != tm_day)
		{
			int remain_cnt = (rdb_cnt-i);
			char remain_buf[BKT_RBUF_SIZE];
			memcpy(remain_buf, gBKTREC_Ctrl.r_buf+i*sizeof(rdb_prm), sizeof(rdb_prm)*remain_cnt);
			memcpy(gBKTREC_Ctrl.r_buf, remain_buf, sizeof(rdb_prm)*remain_cnt);

			//gBKTREC_Ctrl.r_pos = sizeof(rdb_prm)*remain_cnt;

			TRACE1_BKTREC("appeared different days. CH:%d, BID:%ld, prev:%d, curr:%d\n", rdb_prm.ch, rdb_prm.bid, prev_day, tm_day);
			break;

			//CLOSE_BKT(fd);
			//return BKT_ERR;
		}

		long offset = TOTAL_MINUTES_DAY*rdb_prm.ch+(tm_hour*60+tm_min);

		if(rdbs[offset].bid == 0)
		{
			evts[offset]         = rdb_prm.evt;
			rdbs[offset].bid     = rdb_prm.bid;
			rdbs[offset].idx_pos = rdb_prm.idx_pos;

//			TRACE0_BKTREC("update rdb[%s] CH:%d, BID:%ld, TS:%04d-%02d-%02d %02d:%02d:%02d, ipos:%ld, RBUF-POS:%d evt:%d offset:%d\n",
//				path, rdb_prm.ch, rdb_prm.bid, tm_year, tm_mon, tm_day, tm_hour, tm_min, tm1.tm_sec, rdb_prm.idx_pos, gBKTREC_Ctrl.r_pos,rdb_prm.evt, offset);

			beUpdate++;
		}

		//////////////////////////////////////////////////////////////////////////
		gBKTREC_Ctrl.r_pos -= sizeof(rdb_prm);
		//////////////////////////////////////////////////////////////////////////

	}

	if(open_rdb==1)
	{
		if(beUpdate != 0)
		{
			if(!LSEEK_SET(fd, 0))
			{
				BKTREC_ERR("failed seek BEGIN.\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}

			// write evts
			if(!WRITE_PTSZ(fd, evts, sizeof(evts)))
			{
				BKTREC_ERR("failed write events.\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}

			// write rdbs
			if(!WRITE_PTSZ(fd, rdbs, sizeof(rdbs)))
			{
				BKTREC_ERR("failed write rdbs\n");
				CLOSE_BKT(fd);
				return BKT_ERR;
			}
		}

		TRACE0_BKTREC("update rdb.CNT:%2d, UPDATED-CNT:%2d, RBUF-POS:%d %s===============\n", rdb_cnt, beUpdate, gBKTREC_Ctrl.r_pos,path);

		CLOSE_BKT(fd);

		return BKT_OK;
	}

	BKTREC_ERR("failed update rdb. maybe failed create rdb file.\n");

	return BKT_ERR;
}

long BKTREC_FlushWriteBuffer()
{
	if(gBKTREC_Ctrl.b_pos != 0)
	{
		if(!WRITE_PTSZ(g_handle_rec_bkt, gBKTREC_Ctrl.b_buf, gBKTREC_Ctrl.b_pos ))
		{
			BKTREC_ERR("failed write stream data. BID:%ld, b_pos:%d, fpos:%ld\n", gBKTREC_Ctrl.bid, gBKTREC_Ctrl.b_pos, gBKTREC_Ctrl.fpos_bkt);

			//BKTREC_exit(SF_FULL, BKT_REC_CLOSE_SLIENT);

			return BKT_ERR;
		}
//		TRACE0_BKTREC("flush write buffer bkt. fpos:%ld\n", gBKTREC_Ctrl.fpos_bkt);

		gBKTREC_Ctrl.fpos_bkt = LTELL(g_handle_rec_bkt);
		gBKTREC_Ctrl.b_pos = 0;

	}

	// write index data...
	if(gBKTREC_Ctrl.i_pos != 0)
	{
		if(!WRITE_PTSZ(g_handle_rec_idx, gBKTREC_Ctrl.i_buf, gBKTREC_Ctrl.i_pos))
		{
			BKTREC_ERR("failed write index data. i_pos:%d, fpos:%ld\n", gBKTREC_Ctrl.i_pos, gBKTREC_Ctrl.fpos_idx);

			//BKTREC_exit(SF_FULL, BKT_REC_CLOSE_SLIENT);

			return BKT_ERR;
		}
// 		TRACE0_BKTREC("flush write buffer idx. fpos:%ld\n", gBKTREC_Ctrl.fpos_idx);

		gBKTREC_Ctrl.fpos_idx = LTELL(g_handle_rec_idx);
		gBKTREC_Ctrl.i_pos = 0;
		//memset(gBKTREC_Ctrl.idx_pos, gBKTREC_Ctrl.fpos_idx, sizeof(gBKTREC_Ctrl.idx_pos));
	}

	// write rdb
	if(gBKTREC_Ctrl.r_pos != 0 && (gBKTREC_Ctrl.r_skip == TRUE))
	{
		if(BKT_ERR == BKTREC_Flush_RDBDATA())
		{
			BKTREC_ERR("failed update rdb. gBKTREC_Ctrl.r_pos:%d\n", gBKTREC_Ctrl.r_pos);
			gBKTREC_Ctrl.r_pos=0;

			return BKT_ERR;
		}
	}
//	TRACE0_BKTREC("Done of flush write buffer. BID:%ld, fpos:%ld\n", gBKTREC_Ctrl.bid, gBKTREC_Ctrl.fpos_bkt);

	return BKT_OK;
}

long gPrevFpos = 0; // AYK - 0201, previous file point.

long BKTREC_WriteVideoStream(T_VIDEO_REC_PARAM* pv)
{
	if(g_bkt_rec_status == BKT_STATUS_CLOSED) {
		printf("basket is closed\n");
		return BKT_ERR;
	}

    T_STREAM_HDR sh;
    T_VIDEO_STREAM_HDR vh;
	T_BASKET_RDB_PARAM rdb_prm;

    memset(&vh, 0, sizeof(vh));
	sh.ts.sec = pv->ts.sec;
	sh.ts.usec = pv->ts.usec;

	long cur_sec = sh.ts.sec;
	struct tm tm1;
	localtime_r(&cur_sec, &tm1);
	int cur_day  = tm1.tm_mday;
	int cur_min  = tm1.tm_min;

//	printf("ch:%d, size:%d, evt:%ld, type:%d, width:%d, height:%d\n", pv->ch, pv->framesize, pv->event, pv->frametype, pv->width, pv->height);
	//TRACE0_BKTREC("ch:%d, size:%d, evt:%ld, type:%d, width:%d, height:%d, pData:%p\n", pv->ch, pv->framesize, pv->frametype, pv->event, pv->frameWidth, pv->frameHeight, pv->pFrameData);


	if ( (S_OK != BKTREC_checkBasketSize(gBKTREC_Ctrl.fpos_bkt,
									  gBKTREC_Ctrl.fpos_idx,
									  gBKTREC_Ctrl.b_pos + pv->framesize + SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR + gBKTREC_Ctrl.i_pos + SIZEOF_INDEX_DATA))
		|| gBKTREC_Ctrl.prev_day != cur_day)
	{
		sh.id = ID_BASKET_END;
    	sh.ch = pv->ch;
    	sh.framesize = pv->framesize;
    	sh.frametype = pv->frametype;


		gBKTREC_Ctrl.r_skip = FALSE ;

		memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos, &sh, SIZEOF_STREAM_HDR);
		gBKTREC_Ctrl.b_pos += SIZEOF_STREAM_HDR ;

		TRACE0_BKTREC("Check Basket Size,  It will search next basket for record. CH:%ld, prev_day:%ld, cur_day:%d\n", pv->ch, gBKTREC_Ctrl.prev_day, cur_day);

		if(BKT_OK != BKTREC_FlushWriteBuffer())
			return BKT_ERR;

		if(BKT_ERR == BKTREC_SaveFull())
			return BKT_ERR;

		int open_flag = BKTREC_OpenFullBasket(ST_VIDEO);
		if(open_flag != BKT_OK)
		{
//			printf("BKT_REC: Changing Basket ... Failure CH:%02d p_day:%ld c_day:%d !!!!!!!!!!!\n", pv->ch, gBKTREC_Ctrl.prev_day, cur_day);
			return open_flag;
	}
		else
		{
//			printf("BKT_REC: Changing Basket ... Success\n");
		}
	}
	else if(  (gBKTREC_Ctrl.b_pos + pv->framesize + SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR >= BKT_BBUF_SIZE)
			||(gBKTREC_Ctrl.i_pos + SIZEOF_INDEX_DATA >= BKT_IBUF_SIZE) )
	{
		gBKTREC_Ctrl.r_skip = TRUE ;
		if(BKT_OK != BKTREC_FlushWriteBuffer())
			return BKT_ERR;
	}


	//////////////////////////////////////////////////////////////////////////
    sh.id = ID_VIDEO_HEADER;
    sh.ch = pv->ch;
    sh.framesize = pv->framesize;
	sh.frametype = pv->frametype; // I, P, B, ...

    vh.event      = pv->event;		// C, M, S, ...
    vh.audioONOFF = pv->audioONOFF; // AYK - 0201
    vh.framerate  = pv->framerate;
    vh.width      = pv->width;
    vh.height     = pv->height;
	vh.capMode    = pv->capMode;
    if(strlen(pv->camName))
    	strncpy(vh.camName, pv->camName, 16);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// write index info if frame is intra-frame
	if(FT_IFRAME == pv->frametype)
    {
		T_INDEX_DATA idd;

		idd.id      = ID_INDEX_DATA;
		idd.ch      = pv->ch;
		idd.event   = pv->event;
		idd.fpos    = gBKTREC_Ctrl.fpos_bkt+gBKTREC_Ctrl.b_pos; // frame position
		idd.s_type  = ST_VIDEO;
		idd.ts.sec  = sh.ts.sec;
		idd.ts.usec = sh.ts.usec;
		idd.width   = pv->width;
		idd.height  = pv->height;
		idd.capMode = pv->capMode;

		memcpy(gBKTREC_Ctrl.i_buf+gBKTREC_Ctrl.i_pos, &idd, SIZEOF_INDEX_DATA);
		gBKTREC_Ctrl.idx_pos[pv->ch] = gBKTREC_Ctrl.fpos_idx + gBKTREC_Ctrl.i_pos;
		gBKTREC_Ctrl.i_pos += SIZEOF_INDEX_DATA;

		// for update basket manager saving timestamp
		if(gBKTREC_Ctrl.bkt_secs.t1[sh.ch].sec == 0)
		{
			gBKTREC_Ctrl.bkt_secs.t1[sh.ch].sec  = sh.ts.sec;
			gBKTREC_Ctrl.bkt_secs.t1[sh.ch].usec = sh.ts.usec;

			gBKTREC_Ctrl.idx_secs.t1[sh.ch].sec  = sh.ts.sec;
			gBKTREC_Ctrl.idx_secs.t1[sh.ch].usec = sh.ts.usec;

//#ifdef BKTREC_DEBUG
// 			time_t t1 = gBKTREC_Ctrl.bkt_secs.t1[sh.ch].sec;
//			struct tm tm1;
// 			localtime_r(&t1, &tm1);
// 			int tm_year = tm1.tm_year+1900;
// 			int tm_mon  = tm1.tm_mon+1;
// 			int tm_day  = tm1.tm_mday;
// 			int tm_hour = tm1.tm_hour;
// 			int tm_min  = tm1.tm_min;
// 			int tm_sec  = tm1.tm_sec;
// 			TRACE0_BKTREC("set video rec start. CH:[%02ld],  BTS: %04d%02d%02d-%02d:%02d:%02d\n", sh.ch, tm_year, tm_mon, tm_day, tm_hour, tm_min, tm_sec);
//#endif
		}
		gBKTREC_Ctrl.idx_secs.t2[sh.ch].sec  = sh.ts.sec;
		gBKTREC_Ctrl.idx_secs.t2[sh.ch].usec = sh.ts.usec;
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
#ifdef PREV_FPOS // AYK - 0201
        sh.prevFpos = gPrevFpos;
        gPrevFpos = gBKTREC_Ctrl.fpos_bkt + gBKTREC_Ctrl.b_pos;
#endif
    // memcpy video stream header
	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos, &sh, SIZEOF_STREAM_HDR);
	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos+SIZEOF_STREAM_HDR, &vh, SIZEOF_VSTREAM_HDR);

	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos+SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR, pv->buff, pv->framesize);
	gBKTREC_Ctrl.b_pos += SIZEOF_STREAM_HDR+SIZEOF_VSTREAM_HDR+pv->framesize;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// memcpy rdb
	if(gBKTREC_Ctrl.idx_secs.t1[sh.ch].sec != 0 )
	{
		if(gBKTREC_Ctrl.prev_min[sh.ch] != cur_min && gBKTREC_Ctrl.idx_secs.t2[sh.ch].sec >= sh.ts.sec)
		{
			rdb_prm.ch  = sh.ch;
			rdb_prm.sec = sh.ts.sec;
			rdb_prm.evt = vh.event;
			rdb_prm.bid = gBKTREC_Ctrl.bid;
			rdb_prm.idx_pos = gBKTREC_Ctrl.idx_pos[pv->ch]; // frame position

			memcpy(gBKTREC_Ctrl.r_buf+gBKTREC_Ctrl.r_pos, &rdb_prm, sizeof(rdb_prm));
			gBKTREC_Ctrl.r_pos += sizeof(rdb_prm);
			gBKTREC_Ctrl.prev_min[sh.ch] = cur_min;
		}
	}
	//////////////////////////////////////////////////////////////////////////

	// last receive time
	gBKTREC_Ctrl.bkt_secs.t2[sh.ch].sec  = sh.ts.sec;
	gBKTREC_Ctrl.bkt_secs.t2[sh.ch].usec = sh.ts.usec;
	gBKTREC_Ctrl.prev_day = cur_day;

    return BKT_OK;
}

long BKTREC_WriteAudioStream(T_AUDIO_REC_PARAM* pa)
{
	if(g_bkt_rec_status == BKT_STATUS_CLOSED)
		return BKT_ERR;

    T_STREAM_HDR sh;
    T_AUDIO_STREAM_HDR ah;
	sh.ts.sec  = pa->ts.sec;
	sh.ts.usec = pa->ts.usec;

	//TRACE0_BKTREC("write audio ch:%d, size:%d, pData:%p\n", pa->ch, pa->framesize, pa->buff);
	//TRACE0_BKTREC("ch:%d, size:%d, evt:%ld, type:%d, width:%d, height:%d, pData:%p\n", pv->ch, pv->framesize, pv->frametype, pv->event, pv->frameWidth, pv->frameHeight, pv->pFrameData);
	if(S_OK != BKTREC_checkBasketSize(gBKTREC_Ctrl.fpos_bkt,
									  gBKTREC_Ctrl.fpos_idx,
									  gBKTREC_Ctrl.b_pos + pa->framesize + SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR + gBKTREC_Ctrl.i_pos))
	{
		TRACE0_BKTREC("Check Basket Size,  It will search next basket for record\n");


		if(BKT_OK != BKTREC_FlushWriteBuffer())
		{
			BKTREC_ERR("Check Basket Size,  BKTREC_FlushWriteBuffer Error\n");
			return BKT_ERR;
		}
		if(BKT_ERR == BKTREC_SaveFull())
		{
			BKTREC_ERR("Check Basket Size,  BKTREC_SaveFull Error\n");
			return BKT_ERR;
		}

		int open_flag = BKTREC_OpenFullBasket(ST_VIDEO);
		if(open_flag != BKT_OK)
			return open_flag;
	}
	else if(gBKTREC_Ctrl.b_pos + pa->framesize + SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR >= BKT_BBUF_SIZE)
	{
		if(BKT_OK != BKTREC_FlushWriteBuffer())
		{
			BKTREC_ERR("SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR >= BKT_BBUF_SIZE Flush Fail\n");
			return BKT_ERR;
		}
	}

    // save video header

    sh.id = ID_AUDIO_HEADER;
    sh.ch = pa->ch; //
    sh.framesize = pa->framesize;

    ah.samplingrate  = pa->samplingrate;
    //ah.bitspersample = pa->bitspersample;
    //ah.achannel = pa->achannel;
    //ah.codec = 0;

#ifdef PREV_FPOS // AYK - 0201
        sh.prevFpos = gPrevFpos;
        gPrevFpos = gBKTREC_Ctrl.fpos_bkt + gBKTREC_Ctrl.b_pos;
#endif
	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos, &sh, SIZEOF_STREAM_HDR);
	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos+SIZEOF_STREAM_HDR, &ah, SIZEOF_ASTREAM_HDR);
	memcpy(gBKTREC_Ctrl.b_buf+gBKTREC_Ctrl.b_pos+SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR, pa->buff, pa->framesize);
	gBKTREC_Ctrl.b_pos += SIZEOF_STREAM_HDR+SIZEOF_ASTREAM_HDR+pa->framesize;

	return BKT_OK;
}

//////////MULTI-HDD SKSUNG//////////
int BKTREC_setTargetDisk(const int curHddIdx)
{
    BKT_FHANDLE fd_hdmgr;

    T_HDDMGR hdd_mgr ;

	char cwd[MAX_MGR_PATH];
    char buf[MAX_MGR_PATH];

	getcwd(cwd, MAX_MGR_PATH);
    sprintf(buf, "%s/%s/%s", cwd, HDDMGR_PATH, NAME_HDDMGR);

	memset(&hdd_mgr, 0, sizeof(T_HDDMGR));

    if(-1 == access(buf, 0))
    {
		// create hdd mgr
        BKTREC_ERR("can't find hdd manager file. %s. \n", buf);
        printf("In setTargetDisk can't find hdd manager file. %s. \n", buf);

        if(!OPEN_CREATE(fd_hdmgr, buf)) // create basket manager file. (bktmgr.udf)
        {
            BKTREC_ERR("Failed create hdd manager.\n");
            return BKT_ERR;
        }

		// BKKIM
		hdd_mgr.hddcnt = gbkt_mount_info.hddCnt;
		hdd_mgr.curid  = curHddIdx ;
		//hdd_mgr.nextid = gbkt_mount_info.hddInfo[curHddIdx].nextHddIdx; 

		if(!WRITE_ST(fd_hdmgr, hdd_mgr))
		{
			BKTREC_ERR("failed write hdd mgr\n");
			CLOSE_BKT(fd_hdmgr);

			return BKT_ERR;
		}

		CLOSE_BKT(fd_hdmgr);
    }
	else
	{
		// open hddmgr.udf file
		if(!OPEN_RDWR(fd_hdmgr, buf))
		{
			BKTREC_ERR("failed open hdd manager file. %s\n", buf);
			return BKT_ERR;
		}

		if(!READ_ST(fd_hdmgr, hdd_mgr))
		{
			BKTREC_ERR("failed read hdd manager header.\n");
			CLOSE_BKT(fd_hdmgr);
			return BKT_ERR ;
		}

		hdd_mgr.curid = curHddIdx ;

		TRACE0_BKTREC("hdd_mgr.curid :%d, \n", curHddIdx);

		if(!LSEEK_SET(fd_hdmgr, 0)){
			perror("lseek");
		}

		if(!WRITE_ST(fd_hdmgr, hdd_mgr))
		{
			BKTREC_ERR("failed write hdd mgr\n");
			CLOSE_BKT(fd_hdmgr);
			return BKT_ERR;
		}
		CLOSE_BKT(fd_hdmgr);
	}

	sprintf(gBKTREC_Ctrl.target_path, "%s", gbkt_mount_info.hddInfo[curHddIdx].mountDiskPath);
	gBKTREC_Ctrl.target_status = TRUE;
	gBKTREC_Ctrl.bkt_cnt = BKTREC_get_count(gBKTREC_Ctrl.target_path) ;

	TRACE0_BKTREC("gBKTREC_Ctrl.target_path:%s, gBKTREC_Ctrl.bkt_cnt:%d\n",
	               gBKTREC_Ctrl.target_path, gBKTREC_Ctrl.bkt_cnt);

	return BKT_OK;
}

//////////MULTI-HDD SKSUNG/////////
#if 0
int BKTREC_getTargetDisk(const char *path, int status)
{

#ifdef BKT_SYSIO_CALL
    int fd_hdmgr ;
#else
    FILE *fd_hdmgr ;
#endif

    T_HDDMGR hdd_mgr ;
    dvr_disk_info_t mp;

    char hddpath[LEN_PATH];
    char buf[30];
	int i = 0;

    sprintf(buf, "%s/%s", HDDMGR_PATH, NAME_HDDMGR);

	printf("getTargetDisk buf = %s\n",buf) ;

    if(!OPEN_RDWR(fd_hdmgr, buf)) // open hddmgr.udf file
    {
        TRACE1_BKTREC("failed open hdd manager file. %s\n", buf);
        printf("get TargetDisk failed open hdd manager file. %s\n", buf);
        return BKT_ERR;
    }

    if(!READ_ST(fd_hdmgr, hdd_mgr))
    {
        TRACE1_BKTREC("failed read hdd manager header.\n");
        printf("getTargetDisk failed read hdd manager header.\n");
		return BKT_ERR ;
    }
	else
    {
		if(hdd_mgr.hddcnt > 0)
        {
            status = TRUE  ;
            printf("BKTREC_getTargetDisk\n") ;
            sprintf(path, "%s",hdd_mgr.path[hdd_mgr.curid]) ;
        }
    }

	CLOSE_BKT(fd_hdmgr);

    TRACE0_BKTREC("get record target disk path :%s\n", path);
	return TRUE ;

}
#else

int BKTREC_getTargetDisk()
{

#ifdef BKT_SYSIO_CALL
    int fd_hdmgr ;
#else
    FILE *fd_hdmgr ;
#endif

	T_HDDMGR hdd_mgr;

    char cwd[MAX_MGR_PATH];
    char buf[MAX_MGR_PATH];
    int i = 0;

	getcwd(cwd, MAX_MGR_PATH);

	BKTREC_updateHddInfo();

	/*-------------------------*/
	/* find created basket hdd */
	/* and list up hdd path    */
	/*-------------------------*/
	if(gbkt_mount_info.hddCnt > 0)
	{
    	sprintf(buf, "%s/%s/%s", cwd, HDDMGR_PATH, NAME_HDDMGR);

	    if(!OPEN_RDWR(fd_hdmgr, buf)) // open hddmgr.udf file
    	{
    		if(!OPEN_CREATE(fd_hdmgr, buf))
   			{
				BKTREC_ERR("failed open hdd manager file. %s\n", buf);
				return BKT_ERR;
   			}

			hdd_mgr.curid = 0;
			gbkt_mount_info.curHddIdx = hdd_mgr.curid;
			if(!WRITE_ST(fd_hdmgr, hdd_mgr))
			{
				BKTREC_ERR("failed write hdd manager file. %s\n", buf);
				CLOSE_BKT(fd_hdmgr);
				return BKT_ERR;
			}

//	        printf("[MULTI-HDD] Create and wrtie hdd_idx[%02d] manager file. %s\n",hdd_mgr.curid, buf);
	    }
		else
		{
			if(!READ_ST(fd_hdmgr, hdd_mgr))
			{
				BKTREC_ERR("failed read hdd manager file. %s\n", buf);
				CLOSE_BKT(fd_hdmgr);
				return BKT_ERR ;
			}
			else
			{
				if(hdd_mgr.curid >= 0)
				{
//					printf("[MULTI-HDD] Read HDDMGR curHddIdx[%02d] \n", hdd_mgr.curid);

					///curid is mounted//
					if(gbkt_mount_info.hddInfo[hdd_mgr.curid].isMounted)
					{
//						printf("[MULTI-HDD] Find mounted HddIdx[%02d] \n", hdd_mgr.curid);
						gbkt_mount_info.curHddIdx = hdd_mgr.curid;
					}
					else ///curid is not mounted///
					{
//						printf("[MULTI-HDD] Not Founded mounted HddIdx[%02d] find next idx... \n", hdd_mgr.curid);
						int idxFind = 0, isFirst = 0, idxFirst = 0;
						///find the curid next
						for(i = 0; i < MAX_HDD_COUNT; i++)
						{
							if(gbkt_mount_info.hddInfo[i].isMounted)
							{
								if(!isFirst							)
								{
									isFirst = 1;
									idxFirst = i;
								}

								if( i > hdd_mgr.curid)
								{
									gbkt_mount_info.curHddIdx = i;
									hdd_mgr.curid = i;
									idxFind = 1;
									break;
								}
							}
						}

						////hdd mount count is 1 or curid is last idx.
						if(idxFind == 0)
						{
							gbkt_mount_info.curHddIdx = idxFirst;
							hdd_mgr.curid = idxFirst;
						}

						if(!LSEEK_SET(fd_hdmgr, 0)){
							perror("lseek");
						}

						if(!WRITE_ST(fd_hdmgr, hdd_mgr)){
						    perror("write");
						}
					}
				}
			}
		}

		CLOSE_BKT(fd_hdmgr);
	}

	#if BKTMGR_INFO_ENABLE
	printf("\n");
	printf("=================================================\n");
	printf("           GET TARGET DISK INFORMATION           \n");
	printf("=================================================\n");
	printf("  IDX        Path  \n");
	printf("=================================================\n");

    printf("   %02d     %s\n", gbkt_mount_info.curHddIdx,gbkt_mount_info.hddInfo[gbkt_mount_info.curHddIdx].mountDiskPath);
	printf("=================================================\n");
	printf("\n");
	#endif

	return TRUE ;

}
#endif

int BKTREC_updateHddInfo()
{
#ifdef BKT_SYSIO_CALL
		int fd_tm;
#else
		FILE *fd_tm;
#endif

	T_TOPMGR tm ;
	dvr_disk_info_t mp;

	char buf[30];
	int i = 0;

	memset(&gbkt_mount_info, 0, sizeof(gbkt_mount_info));
	memset(&mp, 0, sizeof(mp));
	/*---------------------*/
	/*get mounted dev list */
	/*---------------------*/
	if(get_dvr_disk_info(&mp) == BKT_ERR)
	{
		BKTREC_ERR("failed get disk info.\n");
		return BKT_ERR;
	}

	/*-------------------------*/
	/* find created basket hdd */
	/* and list up hdd path    */
	/*-------------------------*/
	for(i = 0; i < mp.cnt; i++)
	{
		//done format the EXT3
		if(mp.dev[i].state == DONE_EXT3)
		{
			// open topmgr.udf file
			sprintf(buf, "%s/%s", mp.dev[i].dir, NAME_TOPMGR);
			if(OPEN_RDWR(fd_tm, buf))
			{
				if(READ_ST(fd_tm, tm))
				{
					if(tm.hddIdx < MAX_HDD_COUNT && strcmp(tm.dvrMAC, gMac0)==0)
					{
						if(gbkt_mount_info.hddInfo[tm.hddIdx].isMounted == 0)
						{
							gbkt_mount_info.hddCnt++;
							gbkt_mount_info.hddInfo[tm.hddIdx].isMounted 	= 1;
							gbkt_mount_info.hddInfo[tm.hddIdx].formatDate 	= tm.formatDate;
							gbkt_mount_info.hddInfo[tm.hddIdx].initUsedSize = tm.initUsedSize;
							strncpy(gbkt_mount_info.hddInfo[tm.hddIdx].mountDiskPath, mp.dev[i].dir, LEN_PATH);
						}
						//same idx and same dvr mac address
						else if(gbkt_mount_info.hddInfo[tm.hddIdx].isMounted)
						{
							///compare format date////
							if(gbkt_mount_info.hddInfo[tm.hddIdx].formatDate < tm.formatDate)
							{
								gbkt_mount_info.hddInfo[tm.hddIdx].formatDate	= tm.formatDate;
								gbkt_mount_info.hddInfo[tm.hddIdx].initUsedSize = tm.initUsedSize;
								strncpy(gbkt_mount_info.hddInfo[tm.hddIdx].mountDiskPath, mp.dev[i].dir, LEN_PATH);
							}
						}
					}
					CLOSE_BKT(fd_tm);
				}
			}
		}
	}

	int prevIdx = 0, firstIdx = 0, findFirst = 0;
	for(i = 0; i < MAX_HDD_COUNT; i++)
	{
		if(gbkt_mount_info.hddInfo[i].isMounted)
		{
			if(!findFirst)
			{
				findFirst = 1;
				firstIdx = i;
			}
			gbkt_mount_info.hddInfo[i].prevHddIdx = prevIdx;
			gbkt_mount_info.hddInfo[prevIdx].nextHddIdx = i;
			prevIdx = i;
		}
	}

	//first hdd's prevIdx is last hddIdx
	gbkt_mount_info.hddInfo[firstIdx].prevHddIdx = prevIdx;
	//last hdd's nextIdx is first hddIdx
	gbkt_mount_info.hddInfo[prevIdx].nextHddIdx = firstIdx;

	#if BKTMGR_INFO_ENABLE
	printf("\n");
	printf("=================================================\n");
	printf("         UPDATE MULTI-HDD INFORMATION            \n");
	printf("=================================================\n");
	printf(" IDX      Date            Path       Prev   Next \n");
	printf("=================================================\n");

	for(i = 0; i < MAX_HDD_COUNT; i++)
	{
		if(gbkt_mount_info.hddInfo[i].isMounted)
		{
			printf("  %02d   %08d     %s     %02d     %02d\n",
				i, gbkt_mount_info.hddInfo[i].formatDate, gbkt_mount_info.hddInfo[i].mountDiskPath,
				gbkt_mount_info.hddInfo[i].prevHddIdx, gbkt_mount_info.hddInfo[i].nextHddIdx);
		}
	}

	printf("=================================================\n");
	printf("\n");
	#endif
}

////////////////////////////////

int  BKTREC_IsOpenBasket()
{
	return (g_bkt_rec_status == BKT_STATUS_OPENED);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////







////////////////////////////
// EOF basket_rec.c

