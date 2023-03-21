/*
	Made by young(July 1, 2011)
*/

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

#include "app_manager.h"
#include "app_playback.h"
#include "udbasket.h"
#include "bsk_tplay.h"

#include "ti_vdec.h"
#include "ti_vdis.h" 
#include "app_audio.h"
#include "app_audio_priv.h"
#include "app_audio_EncDec.h"

#include "app_writer.h"
#include "ti_vdis_common_def.h"

//////////////////////////////////////////////////////////////////////////
//#define PB_DEBUG

#ifdef PB_DEBUG
#define PB_DBG0(msg, args...) {printf("[PB] - " msg, ##args);}
#define PB_DBG1(msg, args...) {printf("[PB] - %s(%d): " msg, __FUNCTION__, __LINE__, ##args);}
#define PB_DBG2(msg, args...) {printf("[PB] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args);}
#define PB_DERR(msg, args...) {printf("[PB_ERR] - %s(%d):%s: " msg, __FILE__, __LINE__, __FUNCTION__, ##args);}
#else
#define PB_DBG0(msg, args...) {((void)0);}
#define PB_DBG1(msg, args...) {((void)0);}
#define PB_DBG2(msg, args...) {((void)0);}
#define PB_DERR(msg, args...) {((void)0);}
#endif
//////////////////////////////////////////////////////////////////////////

//#define SUPPORT_DBG_THREAD

#define PB_DEFAULT_WIDTH					(720)
#define PB_DEFAULT_HEIGHT					(576)

#define PB_GET_BITBUF_SIZE(width,height)   (((width) * (height) * (2))/2)

#ifdef AUDIO_STREAM_ENABLE
static Int8	audioPlaybackBuf[AUDIO_MAX_PLAYBACK_BUF_SIZE];
#endif

static PbContext_t			gPbCtx;
AUDIO_STATE gAudio_state ;  // hwjun

//sk_aud
extern WRITER_bufInfoTbl_AudPlay g_bufInfoTblAudPlay;
/////////
extern PROFILE gInitSettings;


//							   1x   2x   3x   4x   8x   16x 32x 64x 128x
static long speed_to_usec[9]={1000, 500, 333, 250, 125, 62, 30, 15, 7};

static Int64 get_basket_ts_time_to_msec(T_TS *pTS)
{
	return ((Int64)(pTS->sec)*1000 + pTS->usec/1000);
}

static Int64 get_current_time_to_msec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return ((Int64)tv.tv_sec*1000 + tv.tv_usec/1000);
}

static long
calc_wait_time(int is_end)
{
	PbContext_t *pPb = &gPbCtx;
	long diff_msec = 0;

	if (is_end)
	{
		gettimeofday(&pPb->tv_end, NULL);
		diff_msec = ((Int64)(pPb->tv_end.tv_sec)*1000 + pPb->tv_end.tv_usec/1000) - ((Int64)(pPb->tv_start.tv_sec)*1000 + pPb->tv_start.tv_usec/1000);
	}
	else
	{
		gettimeofday(&pPb->tv_start, NULL);
	}

	return diff_msec;
}

static long
calc_dbg_elapsed_time(struct timeval *start, struct timeval *end)
{
	return ((Int64)(end->tv_sec)*1000 + end->tv_usec/1000) - ((Int64)(start->tv_sec)*1000 + start->tv_usec/1000);
}

static int
calc_time_control_low_frame(int chId, int ratio, Int64 play_now_msec)
{
	PbContext_t *pPb = &gPbCtx;

	Int64 s_ref_msec = pPb->start_ref_time_msec[chId];
	Int64 s_play_msec = pPb->start_play_time_msec[chId];

	Int64 n_ref_msec = get_current_time_to_msec();
	Int64 n_play_msec = play_now_msec;

	Int64 elapsed_ref_time;
	Int64 elapsed_play_time;

	if ((s_ref_msec < 0) || (s_play_msec < 0) || (n_ref_msec < 0) || (n_play_msec < 0))
		return 0;

	elapsed_ref_time  = n_ref_msec - s_ref_msec;
	elapsed_play_time = (n_play_msec - s_play_msec) * ratio;

	return elapsed_play_time - elapsed_ref_time;
}

static int
calc_time_control(int chId, Int64 play_now_msec)
{
	PbContext_t *pPb = &gPbCtx;

	Int64 s_ref_msec = pPb->start_ref_time_msec[chId];
	Int64 s_play_msec = pPb->start_play_time_msec[chId];

	Int64 n_ref_msec = get_current_time_to_msec();
	Int64 n_play_msec = play_now_msec;

	Int64 elapsed_ref_time;
	Int64 elapsed_play_time;

	if ((s_ref_msec < 0) || (s_play_msec < 0) || (n_ref_msec < 0) || (n_play_msec < 0))
		return 0;

	elapsed_ref_time  = n_ref_msec - s_ref_msec;
	elapsed_play_time = n_play_msec - s_play_msec;

	return elapsed_play_time - elapsed_ref_time;
}

static void
send_message_to_qt(int which, int data, void *pData)
{
	PbContext_t *pPb = &gPbCtx;
	DVR_MSG_T	msg;

	if (pPb->qt_pipe_id == NULL)
		return;

	msg.which = which;
	msg.data = data;
	msg.pData = pData;

    OSA_WriteToPipe(pPb->qt_pipe_id, &msg, sizeof(DVR_MSG_T), OSA_SUSPEND);
}


static Void*
task_player_main(Void * prm)
{
    VCODEC_BITSBUF_LIST_S emptyBufList;
    VDEC_BUF_REQUEST_S reqInfo;
	UInt32 bitBufSize;

	int i,j, rv=0, bufCnt=0;;
	int ch, type, size, low_speed_ratio;

	long sec, usec;
	long sleep_msec=0;
	long sleep_done=0;

	int a_fix_ch_enable[MAX_CH];

	PbContext_t *pPb = &gPbCtx;
	stDbgInfo	*pDbg = &pPb->dbg;
	T_BKTMD_PARAM f_info;

	AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun
	UInt64 captime;

#if 1 // BKKIM, test
	char *ebuffer=NULL;

	ebuffer = malloc(BE_ENC_BUFF_SIZE);
	if (ebuffer == NULL) 
	{
		printf("app_pb: pebuffer is Null !!!\n");
		free(ebuffer);

		return NULL;
	}

	f_info.vbuffer = (unsigned char*)ebuffer;
#endif

	unsigned long eventsFromOS;

    bitBufSize = PB_GET_BITBUF_SIZE(PB_DEFAULT_WIDTH, PB_DEFAULT_HEIGHT);	

	for (i = 0; i < MAX_CH; i++){
		a_fix_ch_enable[i] = 1;
	}

	for(;;)
	{
		if (PB_START == pPb->state)
		{
			emptyBufList.numBufs = 0;
			reqInfo.numBufs = VCODEC_BITSBUF_MAX;
			reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;

			for (j = 0; j < VCODEC_BITSBUF_MAX; j++)				
			{
				reqInfo.u[j].minBufSize = bitBufSize;
			}

			while(Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0) == OSA_EFAIL)
			{
				if (calc_wait_time(TRUE) > DMUX_CHK_FREE_BUFF_TIMEOUT || emptyBufList.numBufs > 0)
				break;
			}

			bufCnt = 0;
			for (i = 0; i < emptyBufList.numBufs; i++)
			{
				// for fast response to change state machine.
				if (PB_CHANGE == pPb->state)
					break;

				f_info.focusChannel = pAudstate->pbchannel;
				rv = BKTMD_ReadNextFrame(&f_info, a_fix_ch_enable);
				if (rv != BKT_OK)
				{
					printf("app_pb: Warning, player read frame error. thent player will be stopped rv(%d)\n",rv);
					send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
					pPb->state = PB_INVALID;
					break;
				}
				
				if(f_info.streamtype == ST_VIDEO)
				{
					ch 	 = f_info.ch;
					type = f_info.frametype;
					size = f_info.framesize;
					sec	 = f_info.ts.sec;
					usec = f_info.ts.usec;

					if (FALSE == pPb->init_pb_time[ch]) {
						pPb->start_play_time[ch] = (Int64)sec;
						pPb->start_play_time_msec[ch] = (Int64)sec*1000 + usec/1000;
						pPb->start_ref_time_msec[ch] = get_current_time_to_msec();
						pPb->init_pb_time[ch] = TRUE;
					}

					if (0 == pPb->a_prv_time_sec[ch]) {
						pPb->a_prv_time_sec[ch] = sec;
						pPb->a_prv_time_usec[ch] = usec;
					}

					// Check time interval with previous frame time.
					if ((sec - pPb->a_prv_time_sec[ch]) > AVAILABLE_FRAME_SEC_INTERVAL || (sec - pPb->a_prv_time_sec[ch]) < 0) {
						dprintf("app_pb: ch %2d time interval is over two seconds. then reference time will be reset with current time !!!\n",ch);
						pPb->start_play_time[ch] = sec;
						pPb->start_play_time_msec[ch] = (Int64)sec*1000 + usec/1000;
						pPb->start_ref_time_msec[ch] = get_current_time_to_msec();
					}

					pPb->a_prv_time_sec[ch] = sec;
					pPb->a_prv_time_usec[ch] = usec;

					gettimeofday(&pDbg->start, NULL);

					if (TRUE == pPb->a_ch_enable[ch]) {
						if (FALSE == pPb->a_once_key[ch]) {
							if (F_INTRA == f_info.frametype)
								pPb->a_once_key[ch] = TRUE;
						}

						if (TRUE == pPb->a_once_key[ch]) 
						{
							calc_wait_time(FALSE);
							
							if (emptyBufList.numBufs > 0)
							{
								i = bufCnt;
					            emptyBufList.bitsBuf[i].frameType   	= f_info.frametype;
					            emptyBufList.bitsBuf[i].chnId       	= f_info.ch;

					            emptyBufList.bitsBuf[i].codecType   	= VCODEC_TYPE_H264;
					            emptyBufList.bitsBuf[i].filledBufSize	= f_info.framesize;
					            emptyBufList.bitsBuf[i].timestamp 	 	= (Int64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
							    captime = (UInt64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
							    emptyBufList.bitsBuf[i].lowerTimeStamp  = (UInt32)(captime & 0xFFFFFFFF);
							    emptyBufList.bitsBuf[i].upperTimeStamp  = (UInt32)(captime >> 32);


								memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
								OSA_assert(f_info.framesize <= bitBufSize);

								pDbg->a_bps[ch] += size;
								pDbg->a_fps[ch]++;
								bufCnt++;
								pPb->a_ch_doDec[ch] = 1;
							}
						}
					}

#if 1 // BKKIM
					sleep_msec = calc_time_control(ch, get_basket_ts_time_to_msec(&f_info.ts));

					if (sleep_msec > 0) {
						OSA_waitMsecs(sleep_msec);
						sleep_done++;
					}
#endif
					
					gettimeofday(&pDbg->end, NULL);
					pDbg->a_et[ch] = calc_dbg_elapsed_time(&pDbg->start, &pDbg->end);

					pPb->now_play_time = (Int64)sec;
				}
				else if(f_info.streamtype == ST_AUDIO)
				{
					if(bufCnt == 0)
						i--;
					else
						i = bufCnt-1;
					
					ch 	 = f_info.ch;
					sec	 = f_info.ts.sec;
					usec = f_info.ts.usec;
					
#ifdef AUDIO_STREAM_ENABLE // hwjun					
					if (pAudstate->state == LIB_PLAYBACK_MODE)
					{
						if(f_info.ch == pAudstate->pbchannel)
						{
							VCODEC_BITSBUF_S *pEmptyBufAudPlay;
							Int32 err;
							WRITER_bufInfoTbl_AudPlay *pAudPlay = &g_bufInfoTblAudPlay; 						
							AUDIO_audioDecode(1, f_info.vbuffer, audioPlaybackBuf, f_info.framesize);
							
							///////sk_aud///////
							err = OSA_queGet(&pAudPlay->bufQFreeBufsAudPlay, (Int32*)(&pEmptyBufAudPlay), OSA_TIMEOUT_NONE);
							if (err == 0) 
							{
								pEmptyBufAudPlay->chnId = ch;
								pEmptyBufAudPlay->filledBufSize = f_info.framesize*2;
								memcpy(pEmptyBufAudPlay->bufVirtAddr, (Void*)audioPlaybackBuf, f_info.framesize*2);
							
								OSA_quePut(&pAudPlay->bufQFullBufsAudPlay, (Int32)pEmptyBufAudPlay, OSA_TIMEOUT_NONE);
							} 
							/////////////////////
						}
					}
#endif						
				}
			}
			
			if(emptyBufList.numBufs > 0)
			{
				Vdec_putBitstreamBuffer(&emptyBufList);
			}
			
		}
		else if (PB_TPLAY == pPb->state)
		{
			app_tplay_read_frame(pPb, ebuffer);
		}
		else if( PB_TPLAY_STEP == pPb->state)
		{
			#if 0 // BKKIM, same to do or not
			for(i=0;i<MAX_CH;i++){
				if(i==pPb->tplay_ch)
					Vdec_enableChn(i);
				else
					Vdec_disableChn(i);
			}
			#endif

			app_tplay_step(pPb, ebuffer, FALSE);

			if (PB_CHANGE == pPb->state)
				break;
			else
				pPb->state = PB_PAUSE;

			#if 0 // BKKIM
			for(i=0;i<MAX_CH;i++){
				Vdec_enableChn(i);
			}
			#endif

			OSA_waitMsecs(16);
		}
		else if( PB_TPLAY_STEP_BACK == pPb->state)
		{
			#if 0 // BKKIM, same to do or not
			for(i=0;i<MAX_CH;i++){
				if(i==pPb->tplay_ch)
					Vdec_enableChn(i);
				else
					Vdec_disableChn(i);
			}
			#endif

			app_tplay_step(pPb, ebuffer, TRUE);

			if (PB_CHANGE == pPb->state)
				break;
			else
				pPb->state = PB_PAUSE;

			#if 0 // BKKIM, same to do or not
			for(i=0;i<MAX_CH;i++){
				Vdec_enableChn(i);
			}
			#endif

			OSA_waitMsecs(16);
		}

		else if (PB_SPEED == pPb->state)
		{
			if (PB_SPEED_0_25X <= pPb->speed)
			{
				emptyBufList.numBufs = 0;
				reqInfo.numBufs = MAX_CH;
				reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
				for (j = 0; j < MAX_CH; j++)
				{
					reqInfo.u[j].minBufSize = bitBufSize;
				}

				while(Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0) == OSA_EFAIL)
				{
					if (calc_wait_time(TRUE) > DMUX_CHK_FREE_BUFF_TIMEOUT || emptyBufList.numBufs > 0)
					break;
				}			

				bufCnt = 0;
				for (i = 0; i < /*MAX_CH*/emptyBufList.numBufs; i++)
				{
					// for fast response to change state machine.
					if (PB_CHANGE == pPb->state)
						break;
					
					if(PLAY_FORWARD == pPb->direction){
						f_info.focusChannel = pAudstate->pbchannel;
						rv = BKTMD_ReadNextFrame(&f_info, a_fix_ch_enable);
					}
					else
						rv = BKTMD_ReadPrevIFrame(&f_info, a_fix_ch_enable);
					
					if (rv != BKT_OK)
					{
						printf("app_pb: Warning, player read frame error. thent player will be stopped rv(%d)\n",rv);
						send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
						pPb->state = PB_INVALID;
						break;
					}
					
					if(f_info.streamtype == ST_VIDEO)
					{
						ch 	 = f_info.ch;
						type = f_info.frametype;
						size = f_info.framesize;
						sec	 = f_info.ts.sec;
						usec = f_info.ts.usec;

						if (FALSE == pPb->init_pb_time[ch]) {
							pPb->start_play_time[ch] = sec;
							pPb->start_play_time_msec[ch] = (Int64)sec*1000 + usec/1000;
							pPb->start_ref_time_msec[ch] = get_current_time_to_msec();
							pPb->init_pb_time[ch] = TRUE;
						}

						if (0 == pPb->a_prv_time_sec[ch]) {
							pPb->a_prv_time_sec[ch] = sec;
							pPb->a_prv_time_usec[ch] = usec;
						}
						// Check time interval with previous frame time.
						if ((sec - pPb->a_prv_time_sec[ch]) > AVAILABLE_FRAME_SEC_INTERVAL || (sec - pPb->a_prv_time_sec[ch]) < 0) {
							dprintf("app_pb: ch %2d time interval is over two seconds. then reference time will be reset with current time !!!\n",ch);
							pPb->start_play_time[ch] = sec;
							pPb->start_play_time_msec[ch] = (Int64)sec*1000 + usec/1000;
							pPb->start_ref_time_msec[ch] = get_current_time_to_msec();
						}

						pPb->a_prv_time_sec[ch] = sec;
						pPb->a_prv_time_usec[ch] = usec;

						if (TRUE == pPb->a_ch_enable[ch]) {
							if (FALSE == pPb->a_once_key[ch]) {
								if (F_INTRA == f_info.frametype)
									pPb->a_once_key[ch] = TRUE;
							}

							if (TRUE == pPb->a_once_key[ch]) {
								calc_wait_time(FALSE);

								if (emptyBufList.numBufs > 0) {
									i = bufCnt;
									emptyBufList.bitsBuf[i].frameType		= f_info.frametype;
									emptyBufList.bitsBuf[i].chnId			= f_info.ch;
									
									emptyBufList.bitsBuf[i].codecType		= VCODEC_TYPE_H264;
									emptyBufList.bitsBuf[i].filledBufSize	= f_info.framesize;
									emptyBufList.bitsBuf[i].timestamp		= f_info.ts.sec*1000;
   									captime = (UInt64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
		    						emptyBufList.bitsBuf[i].lowerTimeStamp  = (UInt32)(captime & 0xFFFFFFFF);
				    				emptyBufList.bitsBuf[i].upperTimeStamp  = (UInt32)(captime >> 32);
									
									memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);

									pDbg->a_bps[ch] += size;
									pDbg->a_fps[ch]++;
									bufCnt++;
									pPb->a_ch_doDec[ch] = 1;
								}
							}
						}

						if   	(PB_SPEED_0_25X == pPb->speed)
							low_speed_ratio = 4;
						else if (PB_SPEED_0_5X == pPb->speed)
							low_speed_ratio = 2;
						else
							low_speed_ratio = 2;

						sleep_msec = calc_time_control_low_frame(ch, low_speed_ratio, get_basket_ts_time_to_msec(&f_info.ts));
						if (sleep_msec > 0) {
							OSA_waitMsecs(sleep_msec);
							sleep_done++;
						}
						pPb->now_play_time = sec;
					}
				}
				
				if(emptyBufList.numBufs > 0)
				{
					Vdec_putBitstreamBuffer(&emptyBufList);
					OSA_waitMsecs(16);
				}
			}
			else
			{
				emptyBufList.numBufs = 0;
				reqInfo.numBufs = MAX_CH;
				reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
				for (j = 0; j < MAX_CH; j++)
				{
					reqInfo.u[j].minBufSize = bitBufSize;
				}

				while(Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0) == OSA_EFAIL)
				{
					if (calc_wait_time(TRUE) > DMUX_CHK_FREE_BUFF_TIMEOUT || emptyBufList.numBufs > 0)
					break;
				}			
				
				for (i = 0; i < /*MAX_CH*/emptyBufList.numBufs; i++)
				{
					if (PB_CHANGE == pPb->state)
						break;

					if(PLAY_FORWARD == pPb->direction)
						rv = BKTMD_ReadNextIFrame(&f_info, a_fix_ch_enable);
					else
						rv = BKTMD_ReadPrevIFrame(&f_info, a_fix_ch_enable);
					
					if (rv != BKT_OK)
					{
						printf("app_pb: Warning, player read frame error. thent player will be stopped rv(%d)\n",rv);
						send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
						pPb->state = PB_INVALID;
						break;
					}
					else
					{
						ch 	 = f_info.ch;
						type = f_info.frametype;
						size = f_info.framesize;
						sec	 = f_info.ts.sec;
						usec = f_info.ts.usec;

						gettimeofday(&pDbg->start, NULL);

						if (TRUE == pPb->a_ch_enable[ch])
						{
							calc_wait_time(FALSE);
							
							if (emptyBufList.numBufs > 0) {
								
								emptyBufList.bitsBuf[i].frameType		= f_info.frametype;
								emptyBufList.bitsBuf[i].chnId			= f_info.ch;
								
								emptyBufList.bitsBuf[i].codecType		= VCODEC_TYPE_H264;
								emptyBufList.bitsBuf[i].filledBufSize	= f_info.framesize;
								emptyBufList.bitsBuf[i].timestamp		= (Int64)f_info.ts.sec*1000;
 								captime = (UInt64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
	  							emptyBufList.bitsBuf[i].lowerTimeStamp  = (UInt32)(captime & 0xFFFFFFFF);
		  						emptyBufList.bitsBuf[i].upperTimeStamp  = (UInt32)(captime >> 32);

								
								memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
							
								pDbg->a_bps[ch] += size;
								pDbg->a_fps[ch]++;
								pPb->a_ch_doDec[ch] = 1;
							}
						}

						gettimeofday(&pDbg->end, NULL);
						pDbg->a_et[ch] = calc_dbg_elapsed_time(&pDbg->start, &pDbg->end);

						pPb->now_play_time = sec;
					}
				}

				if(emptyBufList.numBufs > 0)
				{
					Vdec_putBitstreamBuffer(&emptyBufList);
					OSA_waitMsecs(16);
				}				

				if (PB_SPEED == pPb->state)
				{
					if 		(PB_SPEED_2X  == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_2X];
					else if (PB_SPEED_3X  == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_3X];
					else if (PB_SPEED_4X  == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_4X];
					else if (PB_SPEED_8X  == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_8X];
					else if (PB_SPEED_16X == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_16X];
					else if (PB_SPEED_32X == pPb->speed)
						sleep_msec = speed_to_usec[PB_SPEED_32X];
					else
						sleep_msec = speed_to_usec[PB_SPEED_1X];

					OSA_waitMsecs(sleep_msec);
					sleep_done++;
				}
			}
		}

		else if (PB_STOP == pPb->state)
		{
			//printf("STOP\n");
		}

		else if (PB_PAUSE == pPb->state)
		{

		}

		else if (PB_STEP == pPb->state)
		{
			emptyBufList.numBufs = 0;
			reqInfo.numBufs = MAX_CH;
			reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
			for (j = 0; j < MAX_CH; j++)
			{
				reqInfo.u[j].minBufSize = bitBufSize;
			}

			while(Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0) == OSA_EFAIL)
			{
				if (calc_wait_time(TRUE) > DMUX_CHK_FREE_BUFF_TIMEOUT || emptyBufList.numBufs > 0)
				break;
			}			
			
			for (i = 0; i < /*MAX_CH*/emptyBufList.numBufs; i++)
			{
				// for fast response to change state machine.
				if (PB_CHANGE == pPb->state)
					break;

				f_info.focusChannel = pAudstate->pbchannel;
				rv = BKTMD_ReadNextFrame(&f_info, a_fix_ch_enable);
				if (rv != BKT_OK)
				{
					printf("app_pb: Warning, player read frame error. then player will be stopped rv(%d)\n",rv);
					send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
					pPb->state = PB_INVALID;
					break;
				}
				else
				{
					ch 	 = f_info.ch;
					type = f_info.frametype;
					size = f_info.framesize;
					sec	 = f_info.ts.sec;
					usec = f_info.ts.usec;

					if (TRUE == pPb->a_ch_enable[ch]) 
					{
						if (FALSE == pPb->a_once_key[ch]) 
						{
							if (F_INTRA == f_info.frametype)
								pPb->a_once_key[ch] = TRUE;
						}

						if (TRUE == pPb->a_once_key[ch])
						{
							calc_wait_time(FALSE);
							
							if (emptyBufList.numBufs > 0) 
							{
								emptyBufList.bitsBuf[i].frameType		= f_info.frametype;
								emptyBufList.bitsBuf[i].chnId			= f_info.ch;
								
								emptyBufList.bitsBuf[i].codecType		= VCODEC_TYPE_H264;
								emptyBufList.bitsBuf[i].filledBufSize	= f_info.framesize;
								emptyBufList.bitsBuf[i].timestamp		= (Int64)f_info.ts.sec*1000;
 								captime = (UInt64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
	  							emptyBufList.bitsBuf[i].lowerTimeStamp  = (UInt32)(captime & 0xFFFFFFFF);
		  						emptyBufList.bitsBuf[i].upperTimeStamp  = (UInt32)(captime >> 32);

								
								memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
							
								pDbg->a_bps[ch] += size;
								pDbg->a_fps[ch]++;
								pPb->a_ch_doDec[ch] = 1;
							}
						}
					}
				}
			}

			if(emptyBufList.numBufs > 0)
			{
				Vdec_putBitstreamBuffer(&emptyBufList);
				OSA_waitMsecs(16);
			}			

			if (PB_CHANGE == pPb->state)
				break;
			else
				pPb->state = PB_PAUSE;
		}
		
		else if (PB_STEP_BACK == pPb->state)
		{
			emptyBufList.numBufs = 0;
			reqInfo.numBufs = MAX_CH;
			reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
			for (j = 0; j < MAX_CH; j++)
			{
				reqInfo.u[j].minBufSize = bitBufSize;
			}

			while(Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0) == OSA_EFAIL)
			{
				if (calc_wait_time(TRUE) > DMUX_CHK_FREE_BUFF_TIMEOUT || emptyBufList.numBufs > 0)
				break;
			}			
			
			for (i = 0; i < emptyBufList.numBufs; i++)
			{
				// for fast response to change state machine.
				if (PB_CHANGE == pPb->state)
					break;

				rv = BKTMD_ReadPrevIFrame(&f_info, a_fix_ch_enable);
				if (rv != BKT_OK)
				{
					printf("app_pb: Warning, player read frame error. then player will be stopped rv(%d)\n",rv);
					send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
					pPb->state = PB_INVALID;
					break;
				}
				else
				{
					ch 	 = f_info.ch;
					type = f_info.frametype;
					size = f_info.framesize;
					sec	 = f_info.ts.sec;
					usec = f_info.ts.usec;

					if (TRUE == pPb->a_ch_enable[ch]) 
					{
						calc_wait_time(FALSE);
						
						if (emptyBufList.numBufs > 0) 
						{
							emptyBufList.bitsBuf[i].frameType		= f_info.frametype;
							emptyBufList.bitsBuf[i].chnId			= f_info.ch;
							
							emptyBufList.bitsBuf[i].codecType		= VCODEC_TYPE_H264;
							emptyBufList.bitsBuf[i].filledBufSize	= f_info.framesize;
							emptyBufList.bitsBuf[i].timestamp		= (Int64)f_info.ts.sec*1000;
							captime = (UInt64)(f_info.ts.sec)*1000 + f_info.ts.usec/1000;
							emptyBufList.bitsBuf[i].lowerTimeStamp  = (UInt32)(captime & 0xFFFFFFFF);
							emptyBufList.bitsBuf[i].upperTimeStamp  = (UInt32)(captime >> 32);
							
							memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
						
							pDbg->a_bps[ch] += size;
							pDbg->a_fps[ch]++;
							pPb->a_ch_doDec[ch] = 1;
						}

					}
				}
			}

			if(emptyBufList.numBufs > 0)
			{
				Vdec_putBitstreamBuffer(&emptyBufList);
				OSA_waitMsecs(16);
			}			

			if (PB_CHANGE == pPb->state)
				break;
			else
				pPb->state = PB_PAUSE;
		}

		else if (PB_CHANGE == pPb->state)
		{
			OSA_EventSet(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR);
			OSA_EventRetrieve(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR_CONSUME, &eventsFromOS, OSA_SUSPEND);

			printf("app_pb: player state ... (%d) !!!!!!!!!!!\n",pPb->next_state);

			if (pPb->next_state < PB_START || pPb->next_state >= PB_STATE_MAX)
			{
				printf("\n\n\n");
				printf("app_pb: Warning invalid player state ... then playerwill be stopped. state(%d) !!!!!!!!!!!\n",pPb->next_state);
				printf("\n\n\n");
				send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
				pPb->state = PB_STOP;
			}
			else
			{
				pPb->state = pPb->next_state;
			}
		}

		if (0 == sleep_done)
			OSA_waitMsecs(PB_TASK_SLEEP_MSEC);
		sleep_done = 0;
	}
	return NULL;
}

static Void*
task_debugger_main(Void * prm)
{
	int i;
	PbContext_t *pPb = &gPbCtx;
	stDbgInfo	*pDbg = &pPb->dbg;
//	unsigned long eventsFromOS;

	OSA_semWait(&pPb->sem_id, OSA_TIMEOUT_FOREVER);
	printf("app_pb: #### start debugger task\n");

	for(;;)
	{
		if (pPb->dbg_enable)
		{
			printf("(PB)- ");
			for (i = 0; i < MAX_CH; i++)
				printf("%4u ",pDbg->a_bps[i] / (PB_DBG_TASK_SLEEP_MSEC/1000) * 8 / 1024);
			printf(", ");
			for (i = 0; i < MAX_CH; i++)
				printf("%2u ",pDbg->a_fps[i] / (PB_DBG_TASK_SLEEP_MSEC/1000));
			printf("\n");

			printf("(ET)- ");
			for (i = 0; i < MAX_CH; i++)
				printf("%4u ",pDbg->a_et[i]);
			printf("\n");

			memset(&pPb->dbg, 0, sizeof(stDbgInfo));
		}

		OSA_waitMsecs(PB_DBG_TASK_SLEEP_MSEC);
	}
	return NULL;
}

static Void*
task_display_main(Void* prm)
{
	int i, doChange, numWin;
	int chnMap[VDIS_MOSAIC_WIN_MAX];
	VDIS_MOSAIC_S vdMosaicParam;	
	PbContext_t *pPb = &gPbCtx;

	while(pPb->display_tsk_enable)
	{
		doChange = 0;
		
		OSA_waitMsecs(1000);	
		
		if(pPb->display_pb_run == TRUE)
		{
			Vdis_getMosaicParams(gInitSettings.layout.iSubOutput, &vdMosaicParam);
			DVR_getPbMosaicInfo(&numWin, chnMap);
			
			for(i = 0; i < numWin; i++)
			{
//printf("doDec[%d]:%d mosaciPrm[%d]:%d======\n", chnMap[i]-MAX_CH, pPb->a_ch_doDec[chnMap[i]-MAX_CH], i, vdMosaicParam.chnMap[i]);
				if((pPb->a_ch_doDec[chnMap[i]-MAX_CH] != 0)&&(vdMosaicParam.chnMap[i] == VDIS_CHN_INVALID))
				{
					doChange = 1;
					vdMosaicParam.chnMap[i] = chnMap[i];
				}
				if((pPb->a_ch_doDec[chnMap[i]-MAX_CH] == 0)&&(vdMosaicParam.chnMap[i] != VDIS_CHN_INVALID))
				{
					doChange = 1;
					vdMosaicParam.chnMap[i] = VDIS_CHN_INVALID;
				}
			}
	
			if(doChange)
				Vdis_setMosaicParams(gInitSettings.layout.iSubOutput, &vdMosaicParam);
		
			if(pPb->state != PB_PAUSE && pPb->state != PB_STEP && pPb->state != PB_STEP_BACK)
			{
				for(i = 0; i < MAX_CH; i++)
					pPb->a_ch_doDec[i] = 0;
			}
		}
		else
		{
			OSA_waitMsecs(1);
		}
	}

	OSA_EventSet(pPb->event_display, PB_EVT_SETUP_DONE, OSA_EVENT_OR);
	
	return NULL;
}



int app_playback_init(void)
{
	int rv=0;

	PbContext_t *pPb = &gPbCtx;

	memset(pPb, 0, sizeof(PbContext_t));

	pPb->dbg_enable = FALSE;

	OSA_EventCreate(&pPb->event_id);
	OSA_semCreate(&pPb->sem_id, PB_MAX_PENDING_RECV_SEM_COUNT,0);	
	OSA_mutexCreate(&pPb->mutex_id);

    OSA_thrCreate(&pPb->task_id, task_player_main, PB_TASK_PRIORYTY,
                  PB_TASK_STACK_SIZE, &pPb);

	OSA_thrCreate(&pPb->dbg_task_id, task_debugger_main, PB_DBG_TASK_PRIORYTY,
				  PB_DBG_TASK_STACK_SIZE, &pPb);

	////////display task////////
	pPb->display_tsk_enable = TRUE;
	OSA_EventCreate(&pPb->event_display);		
	OSA_thrCreate(&pPb->display_id, task_display_main, PB_TASK_PRIORYTY, PB_TASK_STACK_SIZE, &pPb);
	////////////////////////////
	

#ifdef SUPPORT_DBG_THREAD
	pPb->dbg_enable = TRUE;
	app_playback_dgb_start();
#endif

	printf("app_pb: app_playback_init ... success\n");

	return rv;

}

int app_playback_deinit(void)
{
	int rv=0;
	unsigned long eventsFromOS;	
	PbContext_t *pPb = &gPbCtx;

	printf("app_pb: app_playback_deinit\n");
	OSA_EventDelete(pPb->event_id);
	OSA_semDelete(&pPb->sem_id);
	OSA_mutexDelete(&pPb->mutex_id);

	OSA_thrDelete(&pPb->task_id);
	OSA_thrDelete(&pPb->dbg_task_id);

	//////display task//////
	pPb->display_tsk_enable = FALSE;
	OSA_EventRetrieve(pPb->event_display, PB_EVT_SETUP_DONE, OSA_EVENT_OR_CONSUME, &eventsFromOS, OSA_SUSPEND);
	OSA_EventDelete(pPb->event_display);	
	OSA_thrDelete(&pPb->display_id);
	////////////////////////

	BKTMD_close(TRUE);
	tplay_close(TRUE);

	return rv;
}

int app_playback_start(int ch_bitmask, time_t start_time)
{
	PbContext_t *pPb = &gPbCtx;
	AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun

	T_BKTMD_OPEN_PARAM open_info;

	printf("app_pb: %s() ... start_time:%ld\n", __FUNCTION__, start_time);

	unsigned long eventsFromOS;
	int rv=LIB_PB_NO_ERR, ch;

	if (PB_START == pPb->state
	 || PB_TPLAY == pPb->state)  // BKKIM
		return LIB_PB_NO_ERR;


	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	&eventsFromOS, OSA_SUSPEND);
		printf("app_pb: %s() ... state PB_CHANGE\n", __FUNCTION__);
	}

	if (TRUE == pPb->bkt_opened)
		BKTMD_close(TRUE);

	rv = BKTMD_getTargetDisk(start_time);
	if (TRUE == rv) {
		rv = BKTMD_Open(start_time, &open_info);
		if (BKT_ERR == rv) {
			eprintf("app_pb: basket open error ... %d\n",rv);
			OSA_mutexUnlock(&pPb->mutex_id);
			return LIB_PB_BASKET_FAILED;
		} else {
			rv = LIB_PB_NO_ERR;
		}
	} else {
		rv = LIB_PB_BASKET_FAILED;
	}

	if(rv == LIB_PB_NO_ERR)
	{
//		pPb->init_pb_time = FALSE;
		pPb->bkt_opened = TRUE;
		
		pAudstate->state = LIB_PLAYBACK_MODE ;  // hwjun

		for (ch = 0; ch < MAX_CH; ch++)
			if (ch_bitmask & (0x01 << ch))
				pPb->a_ch_enable[ch] = TRUE;
			else
				pPb->a_ch_enable[ch] = FALSE;
		
		for (ch = 0; ch < MAX_CH; ch++) {
			pPb->init_pb_time[ch] = FALSE;			
			pPb->a_once_key[ch] = FALSE;
			pPb->a_prv_time_sec[ch] = 0;
			pPb->a_prv_time_usec[ch] = 0;
			pPb->a_ch_doDec[ch] = 0;
		}

		pPb->display_pb_run = TRUE;		
		
#if 1	// BKKIM, open trick play handle
		if( LIB_PB_NO_ERR != app_tplay_ready(pPb, start_time)) {
			tplay_close(TRUE);
			//OSA_mutexUnlock(&pPb->mutex_id);
			//return LIB_PB_BASKET_FAILED;
		}
#endif
		if(pPb->tplay_enable)
			pPb->next_state = PB_TPLAY;
		else
			pPb->next_state = PB_START;

		pPb->direction = PLAY_FORWARD;
		OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);
	}

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: %s()... done rv(%d)\n", __FUNCTION__, rv);
	return rv;
}

int app_playback_stop(void)
{
	PbContext_t *pPb = &gPbCtx;
	AUDIO_STATE *pAudstate = &gAudio_state ; // hwjun


#ifdef SUPPORT_DBG_THREAD	
	stDbgInfo	*pDbg = &pPb->dbg;
#endif

	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR, ch;

	pPb->display_pb_run = FALSE;

	OSA_mutexLock(&pPb->mutex_id);

	// Request state to PB_CHANGE
	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;

		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
								&eventsFromOS, OSA_SUSPEND);
	}

	BKTMD_close(TRUE);

	tplay_close(TRUE);

	memset(pPb->start_play_time, 0, MAX_CH);
	memset(pPb->start_play_time_msec, 0, MAX_CH);
	memset(pPb->start_ref_time_msec, 0, MAX_CH);
	pPb->speed = PB_SPEED_1X;
	pPb->bkt_opened = FALSE;

	for (ch = 0; ch < MAX_CH; ch++) {
		pPb->a_ch_enable[ch] = FALSE;
		pPb->a_once_key[ch] = FALSE;
		pPb->a_ch_doDec[ch] = FALSE;
	}

#ifdef SUPPORT_DBG_THREAD
	memset(&pPb->dbg, 0, sizeof(stDbgInfo));
#endif

	pPb->next_state = PB_STOP;
	pAudstate->state = LIB_LIVE_MODE ; // hwjun

	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_stop ... done rv(%d)\n",rv);
	return rv;
}

int app_playback_jump(int ch_bitmask, time_t request_time)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;
	T_BKTMD_OPEN_PARAM open_info;

	int rv=LIB_PB_NO_ERR, ch;

	printf("app_pb: %s() ... request_sec:%ld\n", __FUNCTION__, request_time);

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
								&eventsFromOS, OSA_SUSPEND);
	}

	BKTMD_close(TRUE);

	tplay_close(TRUE);

	////MULTI-HDD SKSUNG////
	BKTMD_getTargetDisk(request_time);

	if (BKT_ERR == BKTMD_Open(request_time, &open_info)) {
		eprintf("app_pb: app_playback_jump ... basket open error\n");
		OSA_mutexUnlock(&pPb->mutex_id);
		return LIB_PB_BASKET_FAILED;
	}

	for (ch = 0; ch < MAX_CH; ch++)
		if (ch_bitmask & (0x01 << ch))
			pPb->a_ch_enable[ch] = TRUE;
		else
			pPb->a_ch_enable[ch] = FALSE;

	for (ch = 0; ch < MAX_CH; ch++) {
		pPb->init_pb_time[ch] = FALSE;		
		pPb->a_once_key[ch] = FALSE;
		pPb->a_prv_time_sec[ch] = 0;
		pPb->a_prv_time_usec[ch] = 0;
	}

//	pPb->init_pb_time = FALSE;
	pPb->bkt_opened = TRUE;

#if 1	// BKKIM, open trick play handle
	if( LIB_PB_NO_ERR != app_tplay_ready(pPb, request_time)) {
		tplay_close(TRUE);

		//OSA_mutexUnlock(&pPb->mutex_id);
		//return LIB_PB_BASKET_FAILED;
	}
#endif

	if(pPb->tplay_enable)
		pPb->next_state = PB_TPLAY;
	else
		pPb->next_state = PB_START;

	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_jump ... done rv(%d)\n",rv);
	return rv;
}

int app_playback_pause(void)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR;

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->back_state = pPb->state;
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
								&eventsFromOS, OSA_SUSPEND);
	}

	pPb->next_state = PB_PAUSE;
	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_pause ... done rv(%d)\n",rv);
	return rv;
}

int app_playback_restart(int ch_bitmask)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR, ch;

	if ((PB_PAUSE != pPb->state) && (PB_STEP != pPb->state)) {
		printf("app_pb: app_playback_restart ... not allowed state %d\n",pPb->state);
		return LIB_PB_NOT_ALLOWED;
	}

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
								&eventsFromOS, OSA_SUSPEND);
	}

	for (ch = 0; ch < MAX_CH; ch++)
		if (ch_bitmask & (0x01 << ch))
			pPb->a_ch_enable[ch] = TRUE;
		else
			pPb->a_ch_enable[ch] = FALSE;

	for (ch = 0; ch < MAX_CH; ch++) {
		pPb->init_pb_time[ch] = FALSE;		
		pPb->a_prv_time_sec[ch] = 0;
		pPb->a_prv_time_usec[ch] = 0;
	}
//	pPb->init_pb_time = FALSE;

	pPb->next_state = pPb->back_state;
	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_restart ... done rv(%d)\n",rv);
	return rv;
}

int app_playback_fastbackward(int ch_bitmask)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR, ch;

	if(pPb->bkt_opened == FALSE)
		return LIB_PB_NOT_ALLOWED;
	
	if (PB_PAUSE != pPb->state) {
		app_playback_pause();
	}

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME, \
								&eventsFromOS, OSA_SUSPEND);
	}

	for (ch = 0; ch < MAX_CH; ch++)
	{
		if (ch_bitmask & (0x01 << ch))
			pPb->a_ch_enable[ch] = TRUE;
		else
			pPb->a_ch_enable[ch] = FALSE;	

		pPb->a_prv_time_sec[ch] = 0;
		pPb->a_prv_time_usec[ch] = 0;
	}


	pPb->next_state = PB_SPEED;
	pPb->speed = PB_SPEED_2X;
	pPb->direction = PLAY_BACKWARD;
	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_fastbackward ... done rv(%d)\n",rv);
	
	return rv;
}

int app_playback_fastforward(int ch_bitmask)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR, ch;

	if(pPb->bkt_opened == FALSE)
		return LIB_PB_NOT_ALLOWED;
	
	if (PB_PAUSE != pPb->state) {
		app_playback_pause();
	}

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME, \
								&eventsFromOS, OSA_SUSPEND);
	}

	for (ch = 0; ch < MAX_CH; ch++)
	{
		if (ch_bitmask & (0x01 << ch))
			pPb->a_ch_enable[ch] = TRUE;
		else
			pPb->a_ch_enable[ch] = FALSE;	

		pPb->a_prv_time_sec[ch] = 0;
		pPb->a_prv_time_usec[ch] = 0;
	}


	pPb->next_state = PB_SPEED;
	pPb->speed = PB_SPEED_2X;
	pPb->direction = PLAY_FORWARD;
	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_fastforward ... done rv(%d)\n",rv);
	
	return rv;
}

int app_playback_step(int ch_bitmask, int bReverse)
{
	PbContext_t *pPb = &gPbCtx;
	unsigned long eventsFromOS;

	int rv=LIB_PB_NO_ERR, ch;

	if(pPb->bkt_opened == FALSE
	&& FALSE == pPb->tplay_enable) // BKKIM
		return LIB_PB_NOT_ALLOWED;

	if (PB_PAUSE != pPb->state) {
		app_playback_pause();
//		printf("app_pb: app_playback_step ... not allowed state %d\n",pPb->state);
//		return LIB_PB_NOT_ALLOWED;
	}

	OSA_mutexLock(&pPb->mutex_id);

	if (PB_CHANGE != pPb->state) {
		pPb->state = PB_CHANGE;
		OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
								&eventsFromOS, OSA_SUSPEND);
	}

	for (ch = 0; ch < MAX_CH; ch++)
		if (ch_bitmask & (0x01 << ch))
			pPb->a_ch_enable[ch] = TRUE;
		else
			pPb->a_ch_enable[ch] = FALSE;

	if(pPb->tplay_enable) {
		if(bReverse == FALSE){
			pPb->next_state = PB_TPLAY_STEP;
			PB_DBG0("app_pb: app tplay step forward\n");
		}
		else {
			pPb->next_state = PB_TPLAY_STEP_BACK;
			PB_DBG0("app_pb: app tplay step backward\n");
		}

	}
	else {
		if(bReverse == FALSE)
			pPb->next_state = PB_STEP;
		else
			pPb->next_state = PB_STEP_BACK;
	}
	
	OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);

	OSA_mutexUnlock(&pPb->mutex_id);
	printf("app_pb: app_playback_step ... done rv(%d)\n",rv);
	return rv;
}

int app_playback_ctrl(int cmd, int ch_bitmask, int value, void *pData)
{
	int rv=LIB_PB_NO_ERR, ch;
	unsigned long eventsFromOS;
	PbContext_t *pPb = &gPbCtx;

	printf("app_pb: app_playback_ctrl ... cmd(%d) value(%d)\n",cmd ,value);
	if (PB_STOP == pPb->state)
	{
		eprintf("app_pb: app_playback_ctrl ... invalid state %d\n",pPb->state);
		return LIB_PB_NO_ERR;
	}

	OSA_mutexLock(&pPb->mutex_id);

	if (LIB_PB_SUB_CMD_CH_ENABLE == cmd)
	{
		pPb->back_state = pPb->state;
		if (PB_CHANGE != pPb->state) {
			pPb->state = PB_CHANGE;
			OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
									&eventsFromOS, OSA_SUSPEND);
		}

		for (ch = 0; ch < MAX_CH; ch++) {
			if (ch_bitmask & (0x01 << ch))
				pPb->a_ch_enable[ch] = TRUE;
			else
				pPb->a_ch_enable[ch] = FALSE;
		}

		pPb->next_state = pPb->back_state;
		OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);
	}

	else if (LIB_PB_SUB_CMD_CH_SET_SPEED == cmd)
	{
		// BKKIM
		if (PB_TPLAY == pPb->state) {
			pPb->speed = value;
			OSA_mutexUnlock(&pPb->mutex_id);
			return LIB_PB_NO_ERR;
		}

		if(pPb->bkt_opened == FALSE)
			return LIB_PB_NOT_ALLOWED;
	
		pPb->back_state = pPb->state;
		if (PB_CHANGE != pPb->state) {
			pPb->state = PB_CHANGE;
			OSA_EventRetrieve(pPb->event_id, PB_EVT_CHANGE_READY, OSA_EVENT_OR_CONSUME,	\
									&eventsFromOS, OSA_SUSPEND);
		}

		if (value < PB_SPEED_1X && value > PB_SPEED_0_5X) {
			printf("app_pb: app_playback_ctrl ... invalid speed step %d. then speed will be set to PB_SPEED_1X.\n",value);
			pPb->speed = PB_SPEED_1X;
		} else {
			pPb->speed = value;
		}

		for (ch = 0; ch < MAX_CH; ch++) {
			if (PB_SPEED_1X == value || PB_SPEED_0_25X <= value)
				pPb->init_pb_time[ch] = FALSE;
			
			pPb->a_prv_time_sec[ch] = 0;
			pPb->a_prv_time_usec[ch] = 0;
		}

//		if (PB_SPEED_0_25X <= value)
//			pPb->init_pb_time = FALSE;

		if (PB_SPEED_1X == value) {
//			pPb->init_pb_time = FALSE;
			if(pPb->direction == PLAY_FORWARD)
				pPb->next_state = PB_START;
			else
				pPb->next_state = PB_SPEED;
		} else {
			pPb->next_state = PB_SPEED;
		}

		OSA_EventSet(pPb->event_id, PB_EVT_SETUP_DONE, OSA_EVENT_OR);
	}

	else
	{
		printf("app_pb: app_playback_ctrl ... invalid command set %d\n",cmd);
	}

	OSA_mutexUnlock(&pPb->mutex_id);

	return rv;
}

void app_playback_set_pipe(void *pipe_id)
{
	PbContext_t *pPb = &gPbCtx;

	if (pipe_id == NULL)
		return;

	pPb->qt_pipe_id = pipe_id;
}

void app_playback_dgb_start(void)
{
	PbContext_t *pPb = &gPbCtx;
	OSA_semSignal(&pPb->sem_id);
}

void app_tplay_enable(int ch, int enable)
{
   if(enable) {
	   //Vdis_setAvsyncChnEnable(ch);
	   int c;
	   for(c=0;c<MAX_CH;c++)
	   {
	   }
   }
   else {
	   int c;
	   for(c=0;c<MAX_CH;c++)
	   {
		   //Vdis_setAvsyncChnDisable(c);
		   //Vdec_enableChn(c);
	   }
   }

   //Vdis_setAvsyncCompEnable(0);
}

void app_tplay_save_layout(int mode, int start_idx)
{
	PbContext_t *pPb = &gPbCtx;

	int ch = start_idx%MAX_CH;
	int idx;

	printf("app_pb: %s,  mode=%d, ch=%d\n", __FUNCTION__, mode, ch);

	OSA_mutexLock(&pPb->mutex_id);

	// on tplay mode
	if( mode == LIB_LAYOUTMODE_1) {

		// set init to read i-frame
		for (idx = 0; idx < MAX_CH; idx++) {
			pPb->a_once_key[idx] = FALSE;
		}

		// change channel when tplay mode
		if(pPb->tplay_enable)
		{
			if(ch>=0 && ch < MAX_CH){
				pPb->tplay_ch = ch;
			}
			else {
			    pPb->tplay_enable = FALSE;
				pPb->tplay_ch = 0;
				pPb->tplay_prev_ch = 0;
				pPb->tplay_prev_speed = 0;
			}
		}
		else {

			if(ch>=0 && ch < MAX_CH){
				pPb->tplay_enable = TRUE;
				pPb->tplay_ch = ch;
			}

		}

		// previous layout
		if(pPb->layout_mode != LIB_LAYOUTMODE_1){

			//Vdis_setAvsyncCompEnable(TRUE);

			if(pPb->state == PB_SPEED || pPb->state == PB_START)
				pPb->state = PB_TPLAY;
			else if(pPb->state == PB_PAUSE){
				pPb->next_state = PB_TPLAY;
			}
		}

	}
	else {
		if(pPb->tplay_enable){

			//Vdis_setAvsyncCompEnable(FALSE);

			// set init to read i-frame
			for (idx = 0; idx < MAX_CH; idx++) {
				pPb->a_once_key[idx] = FALSE;
				pPb->a_ch_doDec[idx] = 0;
			}

			if(pPb->state == PB_TPLAY){
				if(pPb->speed != PB_SPEED_1X)
					pPb->state = PB_SPEED;
				else
					pPb->state = PB_START;
			}
			else if(pPb->state == PB_PAUSE){
				pPb->next_state = PB_START;
			}

		}

		pPb->tplay_enable = FALSE;
		pPb->tplay_ch = 0;
		pPb->tplay_prev_ch = 0;
		pPb->tplay_prev_speed = 0;

	}

	pPb->layout_mode = mode;

	OSA_mutexUnlock(&pPb->mutex_id);
}

int app_playback_get_cur_sec(struct tm *tp)
{
	PbContext_t *pPb = &gPbCtx;

    T_BKT_TM tm1;
    long sec;

	if(pPb->tplay_enable){
	    sec = tplay_get_cur_play_sec(pPb->tplay_ch); 
	}
	else {
		sec = BKTMD_GetCurPlayTime();
	}

    BKT_GetLocalTime(sec, &tm1);

    tp->tm_year = tm1.year;
    tp->tm_mon  = tm1.mon - 1;
    tp->tm_mday = tm1.day;
    tp->tm_hour = tm1.hour;
    tp->tm_min  = tm1.min;
    tp->tm_sec  = tm1.sec;

	return 1;
}

void app_tplay_set_config(UInt32 ch, UInt32 speed)
{
	PB_DBG0("app_tplay : set tplay config ch:%d, spd:%d\n", ch, speed);
	// Decode channels start from 16 in display
	Vdis_setPlaybackSpeed(VDIS_DEV_DVO2, ch+16, speed, VDIS_DISPLAY_SEQID_DEFAULT);

	VdecVdis_setTplayConfig(ch, speed);
}

void app_tplay_check_config()
{
	PbContext_t *pPb = &gPbCtx;

	int tplay_speed=VDIS_AVSYNC_1X;

	if(pPb->tplay_prev_speed != pPb->speed) {

		switch(pPb->speed) {
			case PB_SPEED_0_25X:
				tplay_speed=VDIS_AVSYNC_QUARTERX;
				break;
			case PB_SPEED_0_5X:
				tplay_speed=VDIS_AVSYNC_HALFX;
				break;
			case PB_SPEED_2X:
				tplay_speed=VDIS_AVSYNC_2X;
				break;
			case PB_SPEED_4X:
				tplay_speed=VDIS_AVSYNC_4X;
				break;
			case PB_SPEED_8X:
				tplay_speed=VDIS_AVSYNC_8X;
				break;
			case PB_SPEED_16X:
				tplay_speed=VDIS_AVSYNC_16X;
				break;
			case PB_SPEED_32X:
				tplay_speed=VDIS_AVSYNC_32X;
				break;
			case PB_SPEED_1X:
			default:
				tplay_speed=VDIS_AVSYNC_1X;
				break;
		}

		app_tplay_set_config((UInt32)pPb->tplay_ch, (UInt32)tplay_speed);

		pPb->tplay_prev_ch     = pPb->tplay_ch;
		pPb->tplay_prev_speed  = pPb->speed;
	}

	if( pPb->tplay_prev_ch != pPb->tplay_ch) {

		PB_DBG0("app_tplay : check config, prev_ch=%d, ch=%d\n", pPb->tplay_prev_ch, pPb->tplay_ch);

		app_tplay_set_config((UInt32)pPb->tplay_ch, (UInt32)pPb->tplay_prev_speed);

		pPb->tplay_prev_ch     = pPb->tplay_ch;
	}
}

int app_tplay_ready(PbContext_t *pPb, time_t sec)
{
	T_BKTMD_OPEN_PARAM open_info_tplay;

	if(tplay_is_open())
		tplay_close(TRUE);

	if ( FALSE   == tplay_get_target_disk(pPb->tplay_ch, sec)
	  || BKT_ERR == tplay_open(pPb->tplay_ch, sec, &open_info_tplay))
	{
		eprintf("app_tpb: %s()... ch:%d, basket open error\n", __FUNCTION__, pPb->tplay_ch);
		tplay_close(TRUE);
		return LIB_PB_BASKET_FAILED;
	}

	return LIB_PB_NO_ERR;
}

char tplay_tmpbuf[BE_ENC_BUFF_SIZE];
int  tplay_tmpbuf_size;

int app_tplay_read_frame(PbContext_t *pPb, char *buffer)
{
    VCODEC_BITSBUF_LIST_S emptyBufList;
	T_BKTMD_PARAM f_info;
	int curCh=0, TPLAY_RESID=0;
	int i;

	OSA_mutexLock(&pPb->mutex_id);

	VdecVdis_bitsRdGetEmptyBitBufs(&emptyBufList, TPLAY_RESID);

	f_info.vbuffer = buffer;
	for (i = 0; i < emptyBufList.numBufs; i++) 
	{
		// for fast response to change state machine.
		if (PB_CHANGE == pPb->state) break;

		curCh = VdecVdis_getChnlIdFromBufSize(TPLAY_RESID);

		if(curCh == pPb->tplay_ch) {
#if 1
			if( pPb->speed == PB_SPEED_8X
			  ||pPb->speed == PB_SPEED_16X
			  ||pPb->speed == PB_SPEED_32X) {
				if( BKT_OK != tplay_read_next_iframe(pPb->tplay_ch, &f_info)){
					printf("app_tpb: Warning, player read frame error.\n");
					send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
					pPb->state = PB_INVALID;
					break;
				}
			}
			else if ( pPb->speed == PB_SPEED_1X
					||pPb->speed == PB_SPEED_2X
					||pPb->speed == PB_SPEED_4X
			        ||pPb->speed == PB_SPEED_0_25X
			        ||pPb->speed == PB_SPEED_0_5X ){
				if( BKT_OK != tplay_read_next_frame(pPb->tplay_ch, &f_info)){
					printf("app_tpb: Warning, player read frame error.\n");
					send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
					pPb->state = PB_INVALID;
					break;
				}
			}
#else

			if( BKT_OK != tplay_read_next_frame(pPb->tplay_ch, &f_info)){
				printf("app_tpb: Warning, player read frame error.\n");
				send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
				pPb->state = PB_INVALID;
				//OSA_waitMsecs(16);
				//return BKT_ERR;
				break;
			}
#endif
		}

		if (f_info.streamtype == ST_VIDEO)
		{
			if (TRUE == pPb->a_ch_enable[curCh]) {
				if(curCh == pPb->tplay_ch) {

					if (FALSE == pPb->a_once_key[curCh]) {
						if (F_INTRA == f_info.frametype) {
							pPb->a_once_key[curCh] = TRUE;
							memcpy(tplay_tmpbuf, f_info.vbuffer, f_info.framesize);
							tplay_tmpbuf_size = f_info.framesize;
							pPb->a_ch_doDec[curCh] = 1;
						}
					}

					if ( TRUE == pPb->a_once_key[curCh]) {
						emptyBufList.bitsBuf[i].chnId     = curCh;
						emptyBufList.bitsBuf[i].filledBufSize = f_info.framesize;
						memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
						pPb->a_ch_doDec[curCh] = 1;
					}
				}
				else {
					if ( TRUE == pPb->a_once_key[pPb->tplay_ch]) {
						emptyBufList.bitsBuf[i].chnId     = curCh;
						emptyBufList.bitsBuf[i].filledBufSize = tplay_tmpbuf_size;
						memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, tplay_tmpbuf, tplay_tmpbuf_size);
						pPb->a_ch_doDec[curCh] = 1;
					}
				    
				}
			}

		} // if (f_info.streamtype == ST_VIDEO)

		else if(f_info.streamtype == ST_AUDIO){
		}

	} // for (i = 0; i < emptyBufList.numBufs; i++) 

	VdecVdis_bitsRdSendFullBitBufs(&emptyBufList);

	app_tplay_check_config();

	OSA_mutexUnlock(&pPb->mutex_id);

	if ( pPb->speed == PB_SPEED_0_25X)
		OSA_waitMsecs(60);
	else if ( pPb->speed == PB_SPEED_0_5X)
		OSA_waitMsecs(30);
	else 
		OSA_waitMsecs(15);

	return BKT_OK;
}

int app_tplay_step(PbContext_t *pPb, char *buffer, BOOL bReverse)
{
    VCODEC_BITSBUF_LIST_S emptyBufList;
	T_BKTMD_PARAM f_info;
	int curCh=0, TPLAY_RESID=0;
	int i, rv;

	f_info.vbuffer = buffer;
	if(bReverse){

		// iframe only
		rv = tplay_read_prev_iframe(pPb->tplay_ch, &f_info);

		//rv = tplay_read_prev_frame(pPb->tplay_ch, &f_info);
	}
	else {
		// i-frame only
		//rv = tplay_read_next_iframe(pPb->tplay_ch, &f_info);

		rv = tplay_read_next_frame(pPb->tplay_ch, &f_info);
	}

	if (rv != BKT_OK) {
		printf("app_tpb: Warning, player read step error.play stop. ch:%d\n", pPb->tplay_ch);
		send_message_to_qt(LIB_PLAYER, LIB_PB_READ_FRAME_FAILED, NULL);
		pPb->state = PB_INVALID;
		OSA_waitMsecs(16);
		return BKT_ERR;
	}

	OSA_mutexLock(&pPb->mutex_id);
	VdecVdis_bitsRdGetEmptyBitBufs(&emptyBufList, TPLAY_RESID);

	for (i = 0; i < emptyBufList.numBufs; i++)
	{
		// for fast response to change state machine.
		if (PB_CHANGE == pPb->state) break;

		curCh = VdecVdis_getChnlIdFromBufSize(TPLAY_RESID);

		if (f_info.streamtype == ST_VIDEO) 
		{
			if (TRUE == pPb->a_ch_enable[curCh]) {
				if(curCh == pPb->tplay_ch) {

					if (FALSE == pPb->a_once_key[curCh]) {
						if (F_INTRA == f_info.frametype) {
							pPb->a_once_key[curCh] = TRUE;
							memcpy(tplay_tmpbuf, f_info.vbuffer, f_info.framesize);
							tplay_tmpbuf_size = f_info.framesize;
							pPb->a_ch_doDec[curCh] = 1;
						}
					}

					if ( TRUE == pPb->a_once_key[curCh]) {
						emptyBufList.bitsBuf[i].chnId     = curCh;
						emptyBufList.bitsBuf[i].filledBufSize = f_info.framesize;
						memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, f_info.vbuffer, f_info.framesize);
						pPb->a_ch_doDec[curCh] = 1;						
					}
				}
				else {
					if ( TRUE == pPb->a_once_key[pPb->tplay_ch]) {
						emptyBufList.bitsBuf[i].chnId     = curCh;
						emptyBufList.bitsBuf[i].filledBufSize = tplay_tmpbuf_size;
						memcpy(emptyBufList.bitsBuf[i].bufVirtAddr, tplay_tmpbuf, tplay_tmpbuf_size);
						pPb->a_ch_doDec[curCh] = 1;						
					}
				    
				}
			}

		} // if (f_info.streamtype == ST_VIDEO)

	} // for (i = 0; i < emptyBufList.numBufs; i++) 

	VdecVdis_bitsRdSendFullBitBufs(&emptyBufList);

	OSA_mutexUnlock(&pPb->mutex_id);

	return BKT_OK;
}


// EOF
