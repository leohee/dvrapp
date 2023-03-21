/**
  @file prerec.h
  @defgroup PRE-RECORD pre-recording module
  @ingroup BASKET_REC
  @brief
  @author BKKIM
  @date 2011.12.02
  */

#ifndef _PREREC_H_
#define _PREREC_H_

#include "udbasket.h"

/**
 @struct T_PRELIST_ENT
 @brief pre event list's entry
 @ingroup PRE-RECORD
 */
typedef struct T_PRELIST_ENT{
   int ch;
   int type;
   long msec;
   T_VIDEO_REC_PARAM vp;  ///< param of video
   T_AUDIO_REC_PARAM ap;  ///< param of audio

   struct T_PRELIST_ENT *next;
}T_PRELIST_ENT; 

/**
 @struct T_PRELIST
 @brief pre-event list
 @ingroup PRE-RECORD
 */
typedef struct {
    T_PRELIST_ENT *head;  ///< header of pre-event list
    T_PRELIST_ENT *tail;  ///< tail of pre-event list

	int pre_vcnt; ///< video count
	int pre_acnt; ///< audio count
	int pre_tcnt; ///< total count

	int diff_msec; ///< diff milli-seconds from start to end on list
	int max_premsec; ///< N/A, for each channel, max pre-event milli-seconds, if this is zero, disable pre-event buffering

	pthread_mutex_t mutex; ///< lock and unlock
}T_PRELIST;

/**
 @brief create pre-event list
 @param[in] max_msec max pre-event time as milliseconds
 @return NULL if failed, else if succeed
 @ingroup PRE-RECORD
 */
T_PRELIST* PRELIST_create(int max_msec);

/**
 @ingroup PRE-RECORD
 @brief init pre-event list structure for only not point declared
        initialized zero and init mutex
 @param[in] max_msec max pre-event time as milliseconds
 */
void PRELIST_init_instance(int max_msec);

/**
 @ingroup PRE-RECORD
 @brief deinit pre-event list structure for only not point declared
        delete all items and mutex
 @param[in] plist target list declared not point
 */
void PRELIST_deinit_instance(T_PRELIST* plist);

/**
 @ingroup PRE-RECORD
 @brief remove list and all items
 @param[in] plist pre-event list pointer
 */
void PRELIST_remove(T_PRELIST *plist);

/**
 @ingroup PRE-RECORD
 @brief delete all items, not delete list
 @param[in] plist pre-event list pointer
 */
void PRELIST_delete_all_items(T_PRELIST *plist);

/**
 @ingroup PRE-RECORD
 @brief delete head ent and free memory
 @param[in] plist pre-event list pointer
 */
void PRELIST_delete_head(T_PRELIST* plist);

/**
 @ingroup PRE-RECORD
 @brief free entry
 @param[in] ent entry pointer
 */
void PRELIST_free_ent(T_PRELIST_ENT* ent);

/**
 @ingroup PRE-RECORD
 @brief lock list
 @param[in] plist list of pre-event
 */
void PRELIST_lock(T_PRELIST *plist);

/**
 @ingroup PRE-RECORD
 @brief check buffered frames time...
        anyway frames has time ordered, calculate time between head and tail
		caution to do not use mutex...
 @param[in] plist pre-event list
 @return count of list entries
 */
int PRELIST_check_time(T_PRELIST* plist);

/**
 @ingroup PRE-RECORD
 @brief unlock list
 @param[in] plist list of pre-event
 */
void PRELIST_unlock(T_PRELIST *plist);

/**
 @ingroup PRE-RECORD
 @brief add video frame to pre-list
 @param[in] vp video record parameter
 */
int PRELIST_add_tail_vdo(T_VIDEO_REC_PARAM* vp);

/**
 @ingroup PRE-RECORD
 @brief add audioframe to pre-list
 @param[in] ap audio record parameter
 */
int PRELIST_add_tail_ado(T_AUDIO_REC_PARAM* ap);

/**
 @ingroup PRE-RECORD
 @brief write all enties of list
 @return -1 if failed, 0 if succeed
 */
int PRELIST_flush();

/**
 @brief write frame given entry to list
 @param[in] pEnt list of pre-event
 @return -1 if failed, 0 if succeed
 */
int PRELIST_write_frame(T_PRELIST_ENT* pEnt);
int PRELIST_get_presec(T_PRELIST *pList);
int PRELIST_get_vcnt(T_PRELIST *pList);
int PRELIST_get_acnt(T_PRELIST *pList);
int PRELIST_get_tcnt(T_PRELIST *pList);

void PRELIST_set_max_duration(int msec);
void PRELIST_check_enable(int presec);

extern T_PRELIST gPREREC_ctrl; ///< pre-event list. BKKIM

#endif//_PREREC_H_
