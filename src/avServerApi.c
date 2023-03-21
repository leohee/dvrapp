
#include <avserver.h>

AVSERVER_Config gAVSERVER_config;
VIDEO_Ctrl      gVIDEO_ctrl;
AUDIO_Ctrl      gAUDIO_ctrl;
AVSERVER_Ctrl   gAVSERVER_ctrl;


int AVSERVER_init()
{
  int status;

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: AVSERVER_init().\n");
  #endif


  memset(&gAVSERVER_config, 0, sizeof(gAVSERVER_config));
  memset(&gAVSERVER_ctrl, 0, sizeof(gAVSERVER_ctrl));
  memset(&gAUDIO_ctrl, 0, sizeof(gAUDIO_ctrl));
  memset(&gVIDEO_ctrl, 0, sizeof(gVIDEO_ctrl));

  status = OSA_mutexCreate(&gAVSERVER_ctrl.lockMutex);
  status |= AVSERVER_mainCreate();

  if(status!=OSA_SOK) {
    OSA_ERROR("AVSERVER_init()\n");
    return status;
  }

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: AVSERVER_init()...DONE\n");
  #endif

  return status;
}

int AVSERVER_exit()
{
  int status = OSA_SOK;

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Deleting TSKs.\n");
  #endif

  status = OSA_mutexDelete(&gAVSERVER_ctrl.lockMutex);	

  status |= AVSERVER_mainDelete();

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Deleting TSKs...DONE\n");
  #endif

  return status;
}

int AVSERVER_start(AVSERVER_Config *config)
{
  int status;

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Sending START.\n");
  #endif

  memcpy(&gAVSERVER_config, config, sizeof(gAVSERVER_config));

  status = OSA_mbxSendMsg(&gAVSERVER_ctrl.mainTsk.mbxHndl, &gAVSERVER_ctrl.uiMbx, AVSERVER_MAIN_CMD_START, NULL, OSA_MBX_WAIT_ACK);

  if(status!=OSA_SOK) {
    OSA_ERROR("AVSERVER_start()\n");
  }

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Sending START...DONE\n");
  #endif

  return status;
}

int AVSERVER_stop()
{
  int status;

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Sending STOP.\n");
  #endif

  status = OSA_mbxSendMsg(&gAVSERVER_ctrl.mainTsk.mbxHndl, &gAVSERVER_ctrl.uiMbx, AVSERVER_MAIN_CMD_STOP, NULL, OSA_MBX_WAIT_ACK);

  if(status!=OSA_SOK) {
    OSA_ERROR("AVSERVER_stop()\n");
  }

  #ifdef AVSERVER_DEBUG_API
  OSA_printf(" AVSERVER API: Sending STOP...DONE\n");
  #endif

  return status;
}

int AVSERVER_rtspServerStart()
{
  OSA_printf(" AVSERVER UI: Starting Streaming Server...\n");
  system(WIS_STREAMER);		//# phoong - need to check for system_user
  OSA_waitMsecs(2000);
  OSA_printf(" AVSERVER UI: Starting Streaming Server...DONE\n");

  return 0;
}

int AVSERVER_rtspServerStop()
{
  OSA_printf(" AVSERVER UI: Stopping Streaming Server...\n");
  system_user("killall wis-streamer");
  OSA_waitMsecs(3000);
  OSA_printf(" AVSERVER UI: Stopping Streaming Server...DONE\n");

  return 0;
}

