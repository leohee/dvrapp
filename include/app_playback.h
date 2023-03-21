#ifndef _APP_PLAYBACK_H_
#define _APP_PLAYBACK_H_

#include <sys/time.h>
#include <pthread.h>
#include <osa_event.h>
#include <osa_tsk.h>
#include <osa_mutex.h>
#include <osa_sem.h>
#include <osa_pipe.h>

#include "global.h"
#include "basket_muldec.h"


#define BE_ENC_BUFF_SIZE			(512 * 1024)
#define PB_TASK_STACK_SIZE			(300 * 1024)
#define PB_DBG_TASK_STACK_SIZE		(10 * 1024)

#define PB_TASK_PRIORYTY			(1)
#define PB_TASK_SLEEP_MSEC			(5)

#define PB_DBG_TASK_PRIORYTY		(PB_TASK_PRIORYTY + 1)
#define PB_DBG_TASK_SLEEP_MSEC		(3000)

#define PB_EVT_CHANGE_READY			(0X01)
#define PB_EVT_SETUP_DONE			(0X02)
#define PB_EVT_TEST					(0X04)

//#define PB_EVT_ALL					

#define PB_ERR_INVALID_STATE		(-1)
#define PB_ERR_INVALID_PARAM		(-2)
#define PB_ERR_READ_ERROR			(-3)

#define PB_SPEED_1X					(1)
#define PB_SPEED_2X					(2)
#define PB_SPEED_3X					(3)
#define PB_SPEED_4X					(4)
#define PB_SPEED_8X					(5)
#define PB_SPEED_16X				(6)
#define PB_SPEED_32X				(7)
#define PB_SPEED_0_25X				(8)
#define PB_SPEED_0_5X				(9)

#define PB_SPEED_2X_MSEC			(15)	// 2x
#define PB_SPEED_3X_MSEC			(7)		// 3x
#define PB_SPEED_4X_MSEC			(1)		// 4x (???)

#define PB_MAX_PENDING_RECV_SEM_COUNT (10)


#define F_INTRA						(0)
#define F_INTER						(1)

#define PLAY_FORWARD				(0)
#define PLAY_BACKWARD				(1)

#define DMUX_CHK_FREE_BUFF_TIMEOUT	(1000)
#define AVAILABLE_FRAME_SEC_INTERVAL (2)

typedef enum {
	PB_INVALID = 0,
	PB_START,
	PB_STOP,
	PB_PAUSE,	
	PB_STEP,
	PB_SPEED,	
	PB_CHANGE,
	PB_START_BACK,
	PB_STEP_BACK,
	PB_FAST_BACKWARD,
	PB_TPLAY,      // BKKIM  20111111
	PB_TPLAY_STEP, // BKKIM  20111125
	PB_TPLAY_STEP_BACK, // BKKIM  20111125
	PB_STATE_MAX
} PB_STATE_E;

typedef enum {
	PB_CMD_START = 1,
	PB_CMD_STOP,
	PB_CMD_PAUSE,	
	PB_CMD_JUMP,
	PB_CMD_CHANGE
} PB_CMD_E;

typedef struct {
	struct timeval start;
	struct timeval end;
	int a_fps[MAX_CH];
	int a_bps[MAX_CH];
	int a_et[MAX_CH];
} stDbgInfo;

typedef struct {
    PB_CMD_E eCmd;
    long param;
    void *pData;
} PbMsg_t;

typedef struct {
	int init;
	
	int error;
	int error_type;

	OSA_ThrHndl		display_id;
	OSA_ThrHndl 	task_id;
	OSA_ThrHndl 	dbg_task_id;	
	OSA_PTR			event_display;
	OSA_PTR 		event_id;
	OSA_MutexHndl	mutex_id;
	OSA_SemHndl 	sem_id;
	OSA_PTR pipe_id;	

	void * qt_pipe_id;	
	
	int state;
	int next_state;
	int back_state;
	int direction;						// forward or backward

	int a_ch_doDec[MAX_CH];				// enable display decoded video data
	int a_ch_enable[MAX_CH];			// Send flag to DMUX for each playback control
	int speed;

#if 1 // BKKIM tplay
	int tplay_enable;
	int tplay_ch;
	int tplay_prev_speed;
	int tplay_prev_ch;
	int layout_mode;
#endif

	int a_once_key[MAX_CH];				// on every start/jump operation, Feed I frame. 
	int bkt_opened;

	int a_prv_time_sec[MAX_CH];
	int a_prv_time_usec[MAX_CH];	
	int a_diff_time[MAX_CH];
	
	int init_pb_time[MAX_CH];
	
	Int64 start_play_time[MAX_CH];				// ---------> time_t
	Int64 now_play_time;
	
	Int64 start_play_time_msec[MAX_CH];			// ---------> time_t * 1000	
	Int64 start_ref_time_msec[MAX_CH];			// when start playback, timeval to msec(tv_sec*1000 + tv_usec/1000)

	int dbg_enable;
	int display_tsk_enable;
	int display_pb_run;
	stDbgInfo dbg;

	// for time measurement ... debugging
	struct timeval tv_start;
	struct timeval tv_end;	
} PbContext_t;

int app_playback_init(void);
int app_playback_deinit(void);
int app_playback_start(int ch_bitmask, time_t start_time);
int app_playback_stop(void);
int app_playback_jump(int ch_bitmask, time_t request_time);
int app_playback_pause(void);
int app_playback_restart(int ch_bitmask);
int app_playback_step(int ch_bitmask, int bReverse);
int app_playback_ctrl(int cmd, int ch_bitmask, int value, void *pData);
int app_playback_fastbackward(int ch_bitmask);
int app_playback_fastforward(int ch_bitmask);

void app_playback_set_pipe(void *pipe_id);
void app_playback_dgb_start(void);

// BKKIM - TPLAY
int  app_playback_get_cur_sec(struct tm *tp);
int  app_tplay_ready(PbContext_t *pPb, time_t sec);
int  app_tplay_start(time_t start_time, int ch_bitmask);
int  app_tplay_stop();
void app_tplay_save_layout(int layout, int ch);
void app_tplay_set_config(UInt32 ch, UInt32 speed);
void app_tplay_check_config();
int  app_tplay_read_frame(PbContext_t *pPb, char* buffer);
int  app_tplay_step(PbContext_t *pPb, char* buffer, BOOL bReverse);

#endif

