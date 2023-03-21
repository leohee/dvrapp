#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "app_avibackup.h"
#include "app_bskbackup.h"
#include "app_manager.h"
#include "global.h"

#include "disk_control.h"
#include "common_def.h"
#include "avi.h"
#include "app_util.h"

static BaContext_t gBaCtx ;
static	stNSDVR_FrameInfo frameinfo;

//////////////////////////////////////////////////////////
//#define BACKUP_DEBUG

#ifdef BACKUP_DEBUG
#define TRACE_BACKUP(msg, args...)  printf("[BACKUP] - " msg, ##args)
#define TRACEF_BACKUP(msg, args...) printf("[BACKUP] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TRACE_BACKUP(msg, args...) ((void)0)
#define TRACEF_BACKUP(msg, args...) ((void)0)
#endif
////////////////////////////////////////////////////////////


static void send_message_avibackup_to_qt(int which, int data, void *pData)
{
    BaContext_t *pBa = &gBaCtx;
    DVR_MSG_T   msg;

    if (pBa->qt_pipe_id == NULL)
    {
        return;
    }
    msg.which = which;
    msg.data = data;
    msg.pData = pData;

    OSA_WriteToPipe(pBa->qt_pipe_id, &msg, sizeof(DVR_MSG_T), OSA_SUSPEND);
}


static Void* task_backup_main(Void *prm)
{
	int ret = 0, err_code;
	int a_fix_ch_enable[MAX_CH], i, ch ;

	int video_width[MAX_CH] = {0,};
	int video_height[MAX_CH] = {0,};
	int file_index = 0, j;

	FILE *fd ;

	char *buf = NULL ;
	char **avibuf ;
	char tempbuf[MAXLINE][BACKUP_NAME] ;
	char namebuf[BACKUP_NAME] ;
	char tbuff[BACKUP_NAME] ;	

	char cwd[MAX_MGR_PATH] ;
	getcwd(cwd, MAX_MGR_PATH) ;

	stNSDVR_AVIInfo avi_info;
	T_BKT_TM C_tm, E_tm;

	BaContext_t *pBa = &gBaCtx ;
	T_BKTMD_PARAM basketParam;

	unsigned long eventsFromOS;
    int osalErr = OSA_SOK;

	handle hAVI[MAX_CH] = {NULL, } ;
	stNSDVR_FrameInfo frame = frameinfo;
	
	basketParam.vbuffer = frame.buf ;

	avibuf = (char**)malloc(sizeof(char*)*MAXLINE) ;
	if(avibuf == NULL)
	{
		TRACEF_BACKUP("malloc for avi_backup fail...\n") ;
		return 0 ;
	}

	for(i = 0; i < MAXLINE; i++)
	{
		avibuf[i] = (char*)malloc(sizeof(char)*60) ;
	}

	for(i = 0; i < MAX_CH; i++)
		a_fix_ch_enable[i] = TRUE ;

	getcwd(cwd, MAX_MGR_PATH) ;
	for(;;)
	{
		if(BA_START == pBa->state)
		{
			BKT_GetLocalTime(pBa->end_backup_time, &C_tm) ;
			for(i = 0; i < MAX_CH; i++)
			{
				ret	= BKTMD_ReadNextFrame(&basketParam, a_fix_ch_enable) ;
				if(ret != BKT_OK)
				{
					TRACEF_BACKUP("app_backup : Warning, backup read frame error.. backup will be stop ret(%d)\n",ret) ;
					send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_READ_FRAME_FAILED, NULL);
					pBa->state = BA_INVALID ;
					break ;
				}
				else
				{
					if(basketParam.streamtype == ST_AUDIO)
					{
						continue ;
					}
					ch	= basketParam.ch ;

					frame.channel 		= basketParam.ch ;
					frame.size		 	= basketParam.framesize ;
					frame.data_type	 	= DATA_TYPE_VIDEO ;
					frame.picture_type	= basketParam.frametype ;
					frame.time_sec		= basketParam.ts.sec ;
					frame.time_usec		= basketParam.ts.usec ;
					frame.video_width	= basketParam.frameWidth ;
					frame.video_height  = basketParam.frameHeight ;
					frame.encoding_type = ENCODING_H264 ;

					if(basketParam.frametype == FT_IFRAME && pBa->a_ch_enable[ch])
						pBa->a_once_key[ch] = TRUE ;

					if(pBa->a_ch_enable[ch] != TRUE)
						continue ;

					BKT_GetLocalTime(basketParam.ts.sec, &C_tm) ;

					if((basketParam.frameWidth != video_width[ch]) || (basketParam.frameHeight != video_height[ch]) || (frame.time_sec >= pBa->end_backup_time)) 		
					{
						video_width[ch]  = basketParam.frameWidth ;
						video_height[ch] = basketParam.frameHeight ;
						
						if(frame.time_sec >= pBa->end_backup_time)
						{
							for(ch = 0 ; ch <MAX_CH ;ch++)
							{
								if(hAVI[ch])
								{
									nsDVR_AVI_close(hAVI[ch]) ;
					TRACEF_BACKUP("app_backup : Success, avi backup Close\n") ;
									hAVI[ch] = NULL ;
									pBa->state = BA_INVALID ;
									pBa->a_once_key[ch] ;
								}
							}

							sprintf(tbuff,"%s",pBa->Temppath) ;
							chdir(tbuff) ;
							fd = popen("ls","r") ;
							for(j = 0; j < MAXLINE; j++)
							{
								if(fgets(tempbuf[j], sizeof(tempbuf[j]) -1, fd) != NULL) 
								{
									if(tempbuf[j][strlen(tempbuf[j]) - 1] == '\n')
										tempbuf[j][strlen(tempbuf[j]) - 1] = '\0' ;

									sprintf(avibuf[j],"%s/%s",pBa->Temppath, tempbuf[j]) ;
								}
								else
								{
									break ;
								}
							}
							pclose(fd) ;

							if(pBa->media == TYPE_CDROM)
							{
									sprintf(tbuff, "cp -rf %s/%s %s",cwd, BACKUP_BINARY_MKISO, pBa->Temppath) ;
									fd = popen(tbuff,"r") ;
        							pclose(fd) ;
									
									sprintf(namebuf,"%s","avibackup.iso") ;
									ret = LIB816x_CDROM_MAKE_ISO(namebuf, avibuf, j) ;
							
									if(ret == TRUE)
									{
//										fd = popen("./mkisofs -o avibackup.iso backup","w") ;

										sprintf(tbuff, "cp -rf %s/%s %s",cwd, BACKUP_BINARY_CDRECORD, pBa->Temppath) ;
										fd = popen(tbuff,"w") ;
        								pclose(fd) ;

										TRACE_BACKUP("Make iso Success \n") ;
										ret = LIB816x_CDROM_WRITE_ISO(pBa->path, "avibackup.iso") ;
							
										if(ret == TRUE)
										{
											send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_NO_ERR, NULL);
										
										}
										else
										{	
											send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_WRITE_ISO_FAILED, NULL);
											TRACEF_BACKUP("CDROM_WRITE FAIL..\n") ;
										}
											chdir(cwd) ;
											sprintf(tbuff, "rm -rf %s/cdrecord",pBa->Temppath) ;
											fd = popen(tbuff,"w") ;
        									pclose(fd) ;
									}
									else
									{
										TRACEF_BACKUP("Make iso Failed \n") ;
										send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_MAKE_ISO_FAILED, NULL);
									}
										sprintf(tbuff, "rm -rf %s/mkisofs",pBa->Temppath) ;
										fd = popen(tbuff,"w") ;
        								pclose(fd) ;
									
								pBa->state = BA_INVALID ;
								break ;
				
							}
							else  // BACKUP TO USB
							{
								TRACEF_BACKUP("BACKUP TO USB... ..\n") ;

								sprintf(namebuf, "cp -rf * %s",pBa->path) ;
								fd = popen(namebuf, "w") ;
								pclose(fd) ;	
								TRACE_BACKUP("Backup to USB seccess....\n") ;
								chdir(cwd) ;
								send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_NO_ERR, NULL);

								pBa->state = BA_INVALID ;
								break ;
			
							}
						}
					}

					if(!hAVI[ch] && pBa->a_ch_enable[ch])
                    {
                        sprintf(avi_info.file_name, "%s/%d_%d_%d_%d_%d_ch%d.avi",pBa->Temppath, C_tm.year, C_tm.mon, C_tm.day, C_tm.hour, C_tm.min, ch) ;

                        avi_info.audio_flag = 0 ;
                        avi_info.smi_flag = 1 ;
                        avi_info.video_encoding = ENCODING_H264 ;
                        avi_info.video_fps = (video_height==240)||(video_height==480) ? 30 : 25;
                        avi_info.video_width = video_width;
                        avi_info.video_height = video_height;


                        hAVI[ch] = nsDVR_AVI_create(&avi_info, &err_code);
                        if ( !hAVI[ch] )
                        {
                            TRACEF_BACKUP("nsDVR_AVI_create()  error %s\n", nsDVR_util_error_name(err_code));
							send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_CREATE_AVI_FAILED, NULL);	
							break ;
                        }
						TRACE_BACKUP("AVI Create end backup time  day = %d, hour = %d, min = %d sec = %d\n",C_tm.day, C_tm.hour, C_tm.min, C_tm.sec) ; 	
                    }
					
					if(hAVI[ch] && pBa->a_ch_enable[ch])
					{
						if(pBa->a_once_key[ch] == TRUE)
						{

							ret = nsDVR_AVI_add_frame(hAVI[ch], &frame);

                			if ( ret != NSDVR_ERR_NONE ) 
							{
                        		TRACEF_BACKUP("nsDVR_AVI_add_frame()  error !!! %s\n", nsDVR_util_error_name(ret));
								if( hAVI[ch] )
								{
            						nsDVR_AVI_close(hAVI[ch]) ;
								}
                			}
						}
					}
				}
			}
		}
		else 
		{
			OSA_waitMsecs(BA_TASK_SLEEP_MSEC);
		}
	}	
	return NULL;
}


int app_backup_init(void) 
{
	int ret = LIB_BA_NO_ERR ;
	int osalErr = OSA_SOK;
	char *buf ;
	stNSDVR_FrameInfo *frame = &frameinfo;	

	BaContext_t *pBa = &gBaCtx ; 
	memset(pBa, 0, sizeof(BaContext_t)) ;
	
	buf = (char*)malloc(BACKUP_MAX_FRAME_SIZE) ;
    if(buf == NULL)
    {
        TRACEF_BACKUP("Error malloc buffer for backup\n") ;
        goto error_return ;
    }
    frame->buf = buf ;

	OSA_EventCreate(&pBa->event_id);
	OSA_semCreate(&pBa->sem_id, BA_MAX_PENDING_RECV_SEM_COUNT, 0);
	OSA_mutexCreate(&pBa->mutex_id);
//	TIMM_OSAL_CreatePipe(&pBa->pipe_id, (10 * sizeof(BaMsg_t)), sizeof(BaMsg_t), 1);

	OSA_thrCreate(&pBa->task_id, task_backup_main, BA_TASK_PRIORYTY,
                  BA_TASK_STACK_SIZE, &pBa);

	TRACE_BACKUP("app_backup: app_backup_init success\n") ;
	return ret ;

error_return :
	free(buf) ;
	return ret ;

}


int app_backup_deinit(void) 
{
	int ret = 0;

	stNSDVR_FrameInfo frame = frameinfo;	
	BaContext_t *pBa = &gBaCtx ;

	TRACE_BACKUP("app_backup: app_backup_deinit\n") ;
	
	OSA_EventDelete(pBa->event_id);
	OSA_semDelete(&pBa->sem_id);
	OSA_mutexDelete(&pBa->mutex_id);
//	TIMM_OSAL_DeletePipe(pBa->pipe_id);
	OSA_thrDelete(&pBa->task_id);

	if(frame.buf)
		free(frame.buf) ;

	BKTMD_close(TRUE) ;

	return ret ;

}


int app_backup_start(int media, char *path, int ch_bitmask, time_t start_time, time_t end_time)
{

	BaContext_t *pBa = &gBaCtx;
	T_BKTMD_OPEN_PARAM basketParam;
	
	// BKKIM, These are not used.
	//unsigned long eventsFromOS;
	//T_BKTMD_CTRL gBKTMD_CtrlV;
	
	char Tempbuf[BACKUP_NAME] ;
	char chbuf[MAX_CH] ;
	FILE *fd ;

	TRACE_BACKUP("app_backup_start path = %s\n",path) ;

	int ret = LIB_BA_NO_ERR, ch, chcnt = 0, ncount = 0, backupdata_size = 0, media_size = 0 ;

	for(ch = 0; ch < MAX_CH; ch++)
    {
        if(ch_bitmask & (0x01 << ch))
		{
            pBa->a_ch_enable[ch] = TRUE ;
			chbuf[ch] = TRUE ;
			chcnt+= 1 ; 
		}
        else
            pBa->a_ch_enable[ch] = FALSE ;
    }
    for(ch = 0; ch < MAX_CH; ch++)
    {
        pBa->a_once_key[ch] = FALSE ;
    }

	if(chcnt != 0)
	{
		ncount = BKTMD_getRecFileCount(pBa->a_ch_enable, start_time, end_time);
    	if(1 > ncount)
    	{
        	TRACE_BACKUP("There is no backup file.\n");

        	return OSA_EFAIL;
    	}
    }

	media_size = BACKUP_sizeTargetStorage(path, media) ;
    if(media_size == 0)
    {
        send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_INVALID_MEDIA, NULL);
        TRACE_BACKUP("There is no backup storage.\n");

        return OSA_EFAIL;
    }

	backupdata_size = (((ncount*(BASKET_SIZE/1024))*chcnt)/MAX_CH) ;

	printf("avi backup media_size = %d backup data_size = %d\n",media_size, backupdata_size) ; 
    if(media_size < backupdata_size)
    {
		printf("avi backup media_size = %d backup data_size = %d\n",media_size, backupdata_size) ; 
        send_message_avibackup_to_qt(LIB_BACKUP, LIB_BA_NOT_SUFFICIENT, NULL);
        return OSA_EFAIL ;

    }

	if(TRUE == pBa->bkt_opened)
		BKTMD_close(TRUE) ;
	
	ret = BKTMD_getTargetDisk(start_time) ;

	ret = BKTMD_exgetTargetDisk(Tempbuf) ;

 	sprintf(pBa->Temppath,"%s/backup", Tempbuf) ;
	memset(Tempbuf, 0, BACKUP_NAME) ;

	sprintf(Tempbuf,"rm -rf %s",pBa->Temppath); // remove previous backup directory
	fd = popen(Tempbuf,"w") ;   
	pclose(fd) ;

	mkdir(pBa->Temppath, O_RDWR) ;  // create backup directory 

	if(TRUE == ret)
	{
		ret = BKTMD_Open(start_time, &basketParam) ;
		if(BKT_ERR == ret)
		{
			printf("app_backup: BKTMD_Open Error ..%d\n",ret) ;
			return LIB_BA_BASKET_FAILED ;

		}
		else
		{
			pBa->end_backup_time = end_time ;
			pBa->bkt_opened = TRUE ;
			ret = LIB_BA_NO_ERR ;
	
		}
	}
	else
	{
		ret = LIB_BA_BASKET_FAILED ;
	}

	OSA_mutexLock(&pBa->mutex_id);

	pBa->media = media ;
	sprintf(pBa->path,"%s", path) ;
	pBa->state = BA_START ;

	OSA_mutexUnlock(&pBa->mutex_id);

	TRACE_BACKUP("app_backup: app_backup_start .... done ret(%d)\n",ret) ;
	return ret ;

}

void app_avibackup_set_pipe(void *pipe_id)
{
	BaContext_t *pBa = &gBaCtx;

    if (pipe_id == NULL)
        return;

    pBa->qt_pipe_id = pipe_id;

}


