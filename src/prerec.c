/**
  @file prerec.c
  @brief pre-event list functions
  @author BKKIM
  @date 2011.12.02
  */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "global.h"

#include "prerec.h"
#include "app_writer.h"
#include "global.h"
///////////////////////////////////////////////////////////////////////////////
//#define PREREC_DEBUG

#ifdef PREREC_DEBUG
#define DBG0_PREREC(msg, args...) printf("[PREREC] - " msg, ##args)
#define DBG1_PREREC(msg, args...) printf("[PREREC] - %s:" msg, __FUNCTION__, ##args)
#define DBG2_PREREC(msg, args...) printf("[PREREC] - %s(%d):" msg, __FUNCTION__, __LINE__, ##args)
#define DBG3_PREREC(msg, args...) printf("[PREREC] - %s(%d):\t%s:" msg, __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define DBG0_PREREC(msg, args...) ((void)0)
#define DBG1_PREREC(msg, args...) ((void)0)
#define DBG2_PREREC(msg, args...) ((void)0)
#define DBG3_PREREC(msg, args...) ((void)0)
#endif
///////////////////////////////////////////////////////////////////////////////

T_PRELIST  gPREREC_ctrl; ///< pre-event list.

T_PRELIST* PRELIST_create(int max_msec)
{
    T_PRELIST* plist = malloc(sizeof(T_PRELIST));

	if(NULL == plist) {
	    perror("malloc");
        DBG1_PREREC("can't malloc T_PRELIST\n");
		return NULL;
	}
	memset(plist, 0, sizeof(T_PRELIST));

	pthread_mutex_init(&plist->mutex, NULL);

	DBG1_PREREC("create T_PRELIST\n");

    return plist;
}

void PRELIST_init_instance(int max_msec)
{
	T_PRELIST* plist = &gPREREC_ctrl;

	if(plist->diff_msec != 0) {
	    PRELIST_deinit_instance(plist);
	}
	memset(plist, 0, sizeof(T_PRELIST));

	if(max_msec == 0) // disabled
		return;

	plist->max_premsec = max_msec;

	pthread_mutex_init(&plist->mutex, NULL);

	DBG0_PREREC("%s, init T_PRELIST\n", __FUNCTION__);
}

void PRELIST_deinit_instance(T_PRELIST* plist)
{
    PRELIST_delete_all_items(plist);
	pthread_mutex_destroy(&plist->mutex);

	DBG1_PREREC("deinit T_PRELIST\n");
}

void PRELIST_remove(T_PRELIST *plist)
{
    if(plist != NULL) {

		PRELIST_lock(plist);

	    T_PRELIST_ENT* pNext = NULL;
	    T_PRELIST_ENT* pCurr = plist->head;

		while(NULL != pCurr){
		    pNext = pCurr->next;

			PRELIST_free_ent(pCurr);

			pCurr = pNext;
		}

		PRELIST_unlock(plist);
		pthread_mutex_destroy(&plist->mutex);
		SAFE_FREE(plist);
	}
}

void PRELIST_free_ent(T_PRELIST_ENT* ent)
{
	if(NULL != ent){
		if( VIDEO == ent->type){
			SAFE_FREE(ent->vp.buff);
		}
		else {
			SAFE_FREE(ent->ap.buff);
		}
		SAFE_FREE(ent);
	}
}

void PRELIST_delete_all_items(T_PRELIST *plist)
{
    if(plist != NULL) {

		PRELIST_lock(plist);

	    T_PRELIST_ENT* pNext = NULL;
	    T_PRELIST_ENT* pCurr = plist->head;

		while(NULL != pCurr){
		    pNext = pCurr->next;

			PRELIST_free_ent(pCurr);

			pCurr = pNext;
		}
		plist->head = NULL;
		plist->tail = NULL;
		plist->pre_acnt = 0;
		plist->pre_vcnt = 0;
		plist->pre_tcnt = 0;
		plist->diff_msec = 0;

		DBG1_PREREC("count=%d, head=%p, tail=%p\n",
	              plist->pre_tcnt, plist->head, plist->tail);

		PRELIST_unlock(plist);
	}
}

void PRELIST_lock(T_PRELIST *plist)
{
	if(plist!=NULL){
	    pthread_mutex_lock(&plist->mutex);
	}
}

void PRELIST_unlock(T_PRELIST *plist)
{
	if(plist!=NULL){
	    pthread_mutex_unlock(&plist->mutex);
	}
}

void PRELIST_delete_head(T_PRELIST* plist)
{
    if(NULL != plist){
	    //PRELIST_lock(plist);
		{
		    T_PRELIST_ENT* temp = plist->head;

			plist->head = temp->next;

			if(temp->type == VIDEO)
			    plist->pre_vcnt--;
			else
			    plist->pre_acnt--;

			PRELIST_free_ent(temp);

			plist->pre_tcnt--;
			plist->diff_msec = plist->tail->msec - plist->head->msec;
		}
	    //PRELIST_unlock(plist);
	}
}

int PRELIST_check_time(T_PRELIST* plist)
{
    if(NULL != plist){
	    //PRELIST_lock(plist);
		{
			int  fileSaveState;
			int  status=OSA_SOK;
			WRITER_ChInfo *pChInfo;
			
			if(plist->tail->msec == plist->head->msec || plist->tail->msec < plist->head->msec)
				dprintf("PRELIST_check_time() tail_msec[%d] head_msec[%d] max_premsec[%d]===\n", plist->tail->msec, plist->head->msec, plist->max_premsec);

			// delete header
			while(NULL != plist->head && NULL != plist->tail
			  && (plist->tail->msec - plist->head->msec) > plist->max_premsec) 
			{

				pChInfo = &gWRITER_ctrl.chInfo[plist->head->ch];
				fileSaveState = pChInfo->fileSaveState;

				if(pChInfo->enableChSave)
				{
					if(fileSaveState==WRITER_FILE_SAVE_STATE_IDLE)
						fileSaveState=WRITER_FILE_SAVE_STATE_OPEN;
				}
				else
				{
					if(fileSaveState==WRITER_FILE_SAVE_STATE_OPEN)
						fileSaveState=WRITER_FILE_SAVE_STATE_IDLE;
					else
					{
						if(fileSaveState==WRITER_FILE_SAVE_STATE_WRITE)
							fileSaveState=WRITER_FILE_SAVE_STATE_CLOSE;
					}
				}

				if(fileSaveState==WRITER_FILE_SAVE_STATE_OPEN)
				{
					if((plist->head->type == VIDEO) && plist->head->vp.frametype == FT_IFRAME)
						fileSaveState = WRITER_FILE_SAVE_STATE_WRITE;
				}

				if(fileSaveState==WRITER_FILE_SAVE_STATE_WRITE){
					if(plist->head->type == VIDEO)
					{
						if(pChInfo->firstKeyframe == FALSE)
							pChInfo->firstKeyframe = TRUE;

						if(pChInfo->firstKeyframe == TRUE)
						{
							status = BKTREC_WriteVideoStream(&(plist->head->vp));
							if(status == BKT_ERR){
								pChInfo->enableChSave = FALSE;
								fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
								WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_REC_FAIL, NULL);
								eprintf("failed write video Ch %d \n", plist->head->ch);
							}
							else if(status == BKT_FULL){
								pChInfo->enableChSave = FALSE;
								fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
								WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_HDD_FULL, NULL);
								eprintf("failed write video Ch %d \n", plist->head->ch);
							}
						}
					}
					else {
						status = BKTREC_WriteAudioStream(&(plist->head->ap));
						if(status == BKT_ERR){
							pChInfo->enableChSave = FALSE;
							fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
							WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_REC_FAIL, NULL);
							eprintf("failed write video Ch %d \n", plist->head->ch);
						}
						else if(status == BKT_FULL){
							pChInfo->enableChSave = FALSE;
							fileSaveState = WRITER_FILE_SAVE_STATE_CLOSE;
							WRITER_sendMsgPipe(LIB_RECORDER, LIB_WR_HDD_FULL, NULL);
							eprintf("failed write video Ch %d \n", plist->head->ch);
						}
					}
				}

				if(fileSaveState==WRITER_FILE_SAVE_STATE_CLOSE) {

					fileSaveState=WRITER_FILE_SAVE_STATE_IDLE;
					pChInfo->firstKeyframe = FALSE;
				}

				pChInfo->fileSaveState = fileSaveState;

				PRELIST_delete_head(plist);
			}
		}
	    //PRELIST_unlock(plist);
	}

    return plist->pre_tcnt;
}

int PRELIST_add_tail_vdo(T_VIDEO_REC_PARAM* vp)
{
	T_PRELIST *plist = &gPREREC_ctrl;

#if 0// BKKIM - print info.
	{
		static unsigned long prev_msec = 0;
		static unsigned long frame_count= 0;
		unsigned long curr_msec;

		frame_count++;
		curr_msec = vp->ts.sec * 1000 + vp->ts.usec / 1000;

		if (curr_msec - prev_msec > 999) {

			T_PRELIST* plist = &gPREREC_ctrl;
			DBG0_PREREC("tcnt=%4d, vcnt=%4d, acnt=%4d, diff_msec=%d, max_premsec=%d\n",
					plist->pre_tcnt, plist->pre_vcnt, plist->pre_acnt,
					plist->diff_msec, plist->max_premsec);

			prev_msec = curr_msec;
			frame_count = 0;
		}

	}
#endif


	//if(NULL != plist && plist->max_premsec > 0)
	//if(plist->max_premsec > 0)
	{
		PRELIST_lock(plist);

		// dynamic allocation and free
		{
			//T_PRELIST_ENT* pHead = plist->head; // unused
			T_PRELIST_ENT* pTail = plist->tail;
			T_PRELIST_ENT* pNew = NULL;

			long new_msec = vp->ts.sec*1000 + vp->ts.usec/1000;

			pNew  = malloc(sizeof(T_PRELIST_ENT));

			if(NULL == pNew ) {
				perror("Can't allocate memory");
				DBG1_PREREC("Can't allocate video pre-event list entry memory\n");
				PRELIST_unlock(plist);
				return 0;
			}

			pNew->vp.buff = malloc(vp->framesize);
			if(NULL == pNew->vp.buff) {
				perror("Can't allocate memory");
				DBG1_PREREC("Can't allocate video buffer memory\n");
				SAFE_FREE(pNew);
				PRELIST_unlock(plist);
				return 0;
			}

			pNew->ch                = vp->ch;
			pNew->type              = VIDEO;
			pNew->msec              = new_msec;
			pNew->next              = NULL;

			pNew->vp.ch   			= vp->ch;
			pNew->vp.framesize      = vp->framesize;
			pNew->vp.ts.sec       	= vp->ts.sec;
			pNew->vp.ts.usec      	= vp->ts.usec;
			pNew->vp.event        	= vp->event;
			pNew->vp.frametype    	= vp->frametype;
			pNew->vp.framerate    	= vp->framerate;
			pNew->vp.width        	= vp->width;
			pNew->vp.height       	= vp->height;
			pNew->vp.audioONOFF   	= vp->audioONOFF;
			strcpy(pNew->vp.camName, vp->camName);
			memcpy(pNew->vp.buff, vp->buff, vp->framesize);

			if(NULL == pTail) {

				plist->head = pNew;
				plist->tail = pNew;

				DBG0_PREREC("Pre-event list has no entry. and this entry is first.\n");
			}
			else 
			{
				if(NULL == pTail->next) {

					pTail->next = pNew;
				}
				else 
				{
					DBG1_PREREC("Invalid tail pointer. It will searching tail\n");

					T_PRELIST_ENT* pPrev = pTail;
					T_PRELIST_ENT* pCurr = pTail->next;
					while(NULL != pCurr->next){
						pPrev = pCurr;
						pCurr = pCurr->next;
					}

					if(NULL == pCurr->next) {
						pCurr->next = pNew;
					}

				}

				// update list info
				plist->tail = pNew;
			}

			plist->pre_vcnt++;
			plist->pre_tcnt++;
		}

		// update list info
		plist->diff_msec = plist->tail->msec - plist->head->msec;

		PRELIST_check_time(plist);

		PRELIST_unlock(plist);
	}

#if 0
	DBG0_PREREC("ch[%02d] tcnt=%4d, vcnt=%4d, acnt=%4d, diff_msec=%d, max_premsec=%d\n",
			vp->ch, plist->pre_tcnt, plist->pre_vcnt, plist->pre_acnt, plist->diff_msec, plist->max_premsec);
#endif

	return plist->pre_tcnt;
}

int PRELIST_add_tail_ado(T_AUDIO_REC_PARAM* ap)
{
	T_PRELIST *plist = &gPREREC_ctrl;

    //if(NULL != plist && plist->max_premsec > 0)
    //if(plist->max_premsec > 0)
	{
	    PRELIST_lock(plist);
		{
		    T_PRELIST_ENT* pTail = plist->tail;
			T_PRELIST_ENT* pNew  = malloc(sizeof(T_PRELIST_ENT));

			if(NULL == pNew ) {
				perror("Can't allocate memory");
				DBG1_PREREC("Can't allocate audio pre-event list entry memory\n");
				return 0;
			}

			pNew->ap.buff = malloc(ap->framesize);
			if(NULL == pNew->ap.buff) {
				perror("Can't allocate memory");
				DBG1_PREREC("Can't allocate audio buffer memory\n");
				return 0;
			}

			pNew->ch                = ap->ch;
			pNew->type              = AUDIO;
			pNew->msec              = ap->ts.sec*1000 + ap->ts.usec/1000;
			pNew->next              = NULL;

			pNew->ap.ch   			= ap->ch;
			pNew->ap.framesize 	    = ap->framesize;
			pNew->ap.samplingrate 	= ap->samplingrate;
			pNew->ap.achannel     	= ap->achannel;
			pNew->ap.ts.sec       	= ap->ts.sec;
			pNew->ap.ts.usec      	= ap->ts.usec;
			pNew->ap.bitspersample = ap->bitspersample;
			memcpy(pNew->ap.buff, ap->buff, ap->framesize);

			if(NULL == pTail) {
			    plist->head = pNew;
			    plist->tail = pNew;
				DBG0_PREREC("Pre-event list has no entry. and this entry is first.\n");
			}
			else {
			    if(NULL == pTail->next){
				    pTail->next = pNew;
				}
				else {
				    DBG1_PREREC("Invalid tail pointer. It will searching tail\n");

					T_PRELIST_ENT* pPrev = pTail;
					T_PRELIST_ENT* pCurr = pTail->next;
					while(NULL != pCurr->next){
					    pPrev = pCurr;
						pCurr = pCurr->next;
					}

					if(NULL == pCurr->next) {
					    pCurr->next = pNew;
					}
				}

				// update list info
				plist->tail = pNew;
			}
		}
		// update list info
		plist->pre_acnt++;
		plist->pre_tcnt++;
		plist->diff_msec = plist->tail->msec - plist->head->msec;

		PRELIST_check_time(plist);

		PRELIST_unlock(plist);
	}

	DBG0_PREREC("pre list tcnt=%d, acnt=%d\n", plist->pre_tcnt, plist->pre_acnt);

    return plist->pre_tcnt;
}

int PRELIST_write_frame(T_PRELIST_ENT* pEnt)
{
	if( pEnt == NULL) {
		DBG1_PREREC("invalid handle entry of list \n");
	    return BKT_ERR;
	}

	if(pEnt->type == VIDEO) {
		// if frames are bufferd, to save no reason.
		//if(0 != gWRITER_ctrl.chInfo[pEnt->vp.ch].presec)
			BKTREC_WriteVideoStream(&pEnt->vp);
	}
	else {
		//if(0 != gWRITER_ctrl.chInfo[pEnt->ap.ch].presec)
			BKTREC_WriteAudioStream(&pEnt->ap);
	}
	return 0;
}

int PRELIST_flush()
{
	T_PRELIST *plist = &gPREREC_ctrl;

    //if(NULL != plist && 0 != plist->pre_tcnt)
    if(0 != plist->pre_tcnt)
	{
		DBG0_PREREC("flush pre list -- tcnt:%d\n", plist->pre_tcnt);

	    PRELIST_lock(plist);
		{
			int bFirstIFrame[BKT_MAX_CHANNEL]={0};
		    T_PRELIST_ENT* pCurr = plist->head;
		    T_PRELIST_ENT* pNext = NULL;

			while(NULL != pCurr){

				if(pCurr->type == VIDEO && pCurr->vp.frametype == FT_IFRAME){
				    bFirstIFrame[pCurr->vp.ch] = 1;
				}

				if(bFirstIFrame[pCurr->vp.ch]) {
					PRELIST_write_frame(pCurr);
				}

				pNext = pCurr->next;

				PRELIST_free_ent(pCurr);

				plist->pre_tcnt--;

				pCurr = pNext;
			}

			plist->head = NULL;
			plist->tail = NULL;
			plist->pre_tcnt = 0;
			plist->pre_vcnt = 0;
			plist->pre_acnt = 0;
			plist->diff_msec = 0;
		}
	    PRELIST_unlock(plist);

		return BKT_OK;
	}

	DBG1_PREREC("pre list is null\n");
	return BKT_ERR;
}

int PRELIST_get_presec(T_PRELIST *plist)
{
    if(NULL != plist){
		return plist->diff_msec;
	}
	return 0;
}

int PRELIST_get_vcnt(T_PRELIST *plist)
{
    if(NULL != plist){
		return plist->pre_vcnt;
	}
	return 0;
}

int PRELIST_get_acnt(T_PRELIST *plist)
{
    if(NULL != plist){
		return plist->pre_acnt;
	}
	return 0;
}

int PRELIST_get_tcnt(T_PRELIST *plist)
{
    if(NULL != plist){
		return plist->pre_tcnt;
	}
	return 0;
}

void PRELIST_set_max_duration(int msec)
{
	T_PRELIST *plist = &gPREREC_ctrl;

	plist->max_premsec = msec;
	DBG0_PREREC("set max duration=%d ms\n", msec);
}

int PRELIST_get_header(int *type, T_VIDEO_REC_PARAM *vp, T_AUDIO_REC_PARAM *ap)
{
	T_PRELIST *plist = &gPREREC_ctrl;
	T_PRELIST_ENT* pCurr = plist->head;

	PRELIST_lock(plist);
	if(NULL != pCurr){

		*type = pCurr->type;
		if(pCurr->type == VIDEO){
			memcpy(vp, &(pCurr->vp), sizeof(T_VIDEO_REC_PARAM));
		}
		else {
			memcpy(ap, &(pCurr->ap), sizeof(T_AUDIO_REC_PARAM));
		}

		PRELIST_delete_head(plist);
		PRELIST_unlock(plist);
	    return 1;
	}

	PRELIST_unlock(plist);
	return 0;
}

void PRELIST_check_enable(int presec)
{
	// check all channel disable
	if( 0 == presec) {
		int c,noprecnt=0;
		for(c=0;c<BKT_MAX_CHANNEL;c++){
			if(gWRITER_ctrl.chInfo[c].presec>0){
				gWRITER_ctrl.bPreEnabled = TRUE;
				break;
			}
			noprecnt++;
		}
		if(noprecnt == BKT_MAX_CHANNEL){
			gWRITER_ctrl.bPreEnabled = FALSE; 

			PRELIST_flush();

			printf("Pre disabled - presec:%d\n", presec);
		}
	}
	else {
		gWRITER_ctrl.bPreEnabled = TRUE;
	}

}

