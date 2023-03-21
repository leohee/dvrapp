
#ifndef _REMOTE_PRIV_H_
#define _REMOTE_PRIV_H_

#include <mcvip.h>
#include <remoteplay.h>
#include <osa_tsk.h>
#include <osa_mutex.h>
#include <osa_cmem.h>
#include <osa_prf.h>
#define REMOTEPLAY_MIN_IN_BUF_PER_CH  (1)

#define REMOTEPLAY_THR_PRI       (MCVIP_CAPTURE_THR_PRI_HIGH-8)
#define REMOTEPLAY_STACK_SIZE    (10*KB)

#define REMOTEPLAY_CMD_CREATE    (0x0700)
#define REMOTEPLAY_CMD_RUN       (0x0701)
#define REMOTEPLAY_CMD_DELETE    (0x0702)


typedef struct {

  OSA_TskHndl tskHndl;
  OSA_MbxHndl mbxHndl;

  int channel ;
  int sockchannel ;
  int pause ;
  int stop ;
  int runflag ;
  unsigned long rectime ;
  OSA_MutexHndl mutexLock;

  OSA_PrfHndl prfFileWr;

} REMOTEPLAY_Ctrl;

extern REMOTEPLAY_Ctrl gREMOTEPLAY_ctrl;


#endif

