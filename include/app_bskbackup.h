#ifndef _BSKBACKUP_H_
#define _BSKBACKUP_H_

#include <osa_tsk.h>
#include <osa_mutex.h>
//#include <osa_cmem.h>
#include <osa_pipe.h>
#include <string.h>

#include "global.h"

typedef struct {
  int ch[MAX_CH];
  long t1;
  long t2;
} BACKUP_CreatePrm;


#define BACKUP_MIN_IN_BUF_PER_CH  (1)

#define BACKUP_THR_PRI       (10)
#define BACKUP_STACK_SIZE    (2000*KB)

#define BACKUP_CMD_CREATE    (0x0800)
#define BACKUP_CMD_RUN       (0x0801)
#define BACKUP_CMD_DELETE    (0x0802)

#define BACKUP_FLAG_DONE     (0x0001)
#define BACKUP_FLAG_ERROR    (0x0002)

#ifndef LEN_PATH
#define LEN_PATH 64
#endif

#define BACKUP_STATUS_STOP      (0)
#define BACKUP_STATUS_DONE      (1)
#define BACKUP_STATUS_RUNNING   (2)
#define BACKUP_STATUS_CANCEL    (3)
#define BACKUP_STATUS_ERROR     (4)

#define BACKUP_ERR_NONE          (0)
#define BACKUP_ERR_DSTBKTMGR     (1)
#define BACKUP_ERR_NONEDISK      (2)
#define BACKUP_ERR_NOTENOUGHDISK (3)

#define BACKUP_ERR_SRCBASKET    (10)
#define BACKUP_ERR_SRCINDEX     (11)

#define BACKUP_ERR_DSTBASKET    (20)
#define BACKUP_ERR_DSTINDEX     (21)
#define BACKUP_ERR_DSTRDB       (22)

#define MAX_PATH_LENGTH			256

typedef struct {

    OSA_TskHndl tskHndl;
    OSA_MbxHndl mbxHndl;

    BACKUP_CreatePrm createPrm;

    int status;
    int err_msg;

    int  ch[MAX_CH];
    long t1;
    long t2;

    long  fpos_cur;      // for progress status
    long  fpos_end;          // for progress status
    int  file_curr_number; // for progress status
    int  file_total_count; // for progress status

	int  dstmedia ;
    int  filecount;
	int  pathcount ;
    int *filelist;
    char dst_path[LEN_PATH]; // backup path 
	char media_path[LEN_PATH] ;  // media path to backup for cdrom
    char src_path[LEN_PATH]; // recorded data path
	char src_endpath[LEN_PATH] ;

    //OSA_MutexHndl mutexLock;
//    OSA_PrfHndl prfFileWr;
	void*   msg_pipe_id;    

} BACKUP_Ctrl;

extern BACKUP_Ctrl gBackupCtrl;
    
int app_bskbackup_create(int media, char *path, int ch_bitmask, long t1, long t2) ;
int app_bskbackup_delete(void);
int BACKUP_sizeTargetStorage(char *targetpath, int media) ;


int LIBDVR365_getPlaybackBackupStorage(char *buf_out);
int BACKUP_getProgressStatus(long *curfpos, long *endfpos, int *curfnum, int *totalfnum);
int BACKUP_getPlaybackStoragePath(char *buf_out) ;
int  BKTDEC_getRecFileCount(int *pChannels, long t1, long t2) ;
int  BKTDEC_getRecFileList(int *pChannels, long t1, long t2, int *pFileArray, char *srcPath) ;
void app_bskbackup_set_pipe(void *pipe_id) ;


#endif

