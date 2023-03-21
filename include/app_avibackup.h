#ifndef _APP_BACKUP_H_
#define _APP_BACKUP_H_

#include <pthread.h>
#include <osa_event.h>
#include <osa_tsk.h>
#include <osa_mutex.h>
#include <osa_sem.h>
#include <osa_pipe.h>

#include "global.h"
#include "basket_muldec.h"

#define BACKUP_MAX_FRAME_SIZE 				(256 * 1024) 
#define BA_ENC_BUFF_SIZE  		            (512 * 1024)
//#define BA_TASK_STACK_SIZE					(50 * 1024)
#define BA_TASK_STACK_SIZE					(1000 * 1024)

#define BA_TASK_PRIORYTY          			(3)

#define BA_EVT_CHANGE_READY     		    (0X10)
#define BA_EVT_SETUP_DONE       		    (0X20)
#define BA_EVT_TEST               			(0X40)

#define BA_ERR_INVALID_STATE        (-1)
#define BA_ERR_INVALID_PARAM        (-2)
#define BA_ERR_READ_ERROR           (-3)

#define AVI_MAX_PATH_LENGTH			256
#define BA_MAX_PENDING_RECV_SEM_COUNT (10)
#define BA_TASK_SLEEP_MSEC	    1


typedef enum 
{
    DATA_TYPE_VIDEO,
    DATA_TYPE_AUDIO,
    DATA_TYPE_TEXT,
} enumDVR_DATA_TYPE;

typedef enum 
{
    ENCODING_UNKNOWN=0,

    /* Video */
    ENCODING_MPEG4=1,
    ENCODING_JPEG,
    ENCODING_H264,

    /* Audio */
    ENCODING_PCM=32,
    ENCODING_ULAW,
    ENCODING_ALAW,
    ENCODING_ADPCM,
    ENCODING_G721,
    ENCODING_G723,
    ENCODING_MP3,
} enumDVR_ENCODING_TYPE;


typedef struct 
{
    int init;

    int error;
    int error_type;
	int bkt_opened;

    OSA_ThrHndl   task_id;
	OSA_PTR       event_id;
    OSA_MutexHndl mutex_id;
    OSA_SemHndl   sem_id;
	OSA_PTR       pipe_id ;
	void *qt_pipe_id ;


	int state ;
	int a_ch_enable[MAX_CH] ;
	int a_once_key[MAX_CH] ;

	int media ;
	char path[AVI_MAX_PATH_LENGTH] ;
	char Temppath[AVI_MAX_PATH_LENGTH] ;

    long start_backup_time;               // ---------> time_t
    long end_backup_time;

} BaContext_t ;

typedef enum
{
    BA_INVALID = 0,
    BA_START,
    BA_STOP,
} BA_CMD_E ;


typedef struct {
    BA_CMD_E eCmd;
    long param;
    void *pData;
} BaMsg_t;


int app_backup_init(void);
int app_backup_deinit(void) ;
int app_backup_start(int mode, char *path, int ch_bitmask, time_t start_time, time_t end_time) ;
void app_avibackup_set_pipe(void *pipe_id) ;


#endif
