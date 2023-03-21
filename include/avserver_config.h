#ifndef _AV_SERVER_CONFIG_H_
#define _AV_SERVER_CONFIG_H_

#include <osa.h>

#define AVSERVER_MAX_STREAMS (16)	//RTSP_EXT_CHANNEL

#define MAX_STRING_LENGTH 22

typedef struct {

  Bool   captureEnable;
  Uint32 samplingRate;
  Uint16 codecType;
  Bool   fileSaveEnable;

} AUDIO_Config;


typedef struct {

  Uint16 captureStreamId;
  Uint16 cropWidth;
  Uint16 cropHeight;
  Uint32 frameRateBase;
  Uint32 frameSkipMask;
  Uint16 codecType;
  Uint32 codecBitrate;
  Bool   encryptEnable;
  Bool   fileSaveEnable;
  Bool   motionVectorOutputEnable;
  Int16  qValue;
  Uint16 rateControlType;

} VIDEO_EncodeConfig;

typedef struct {

  int mem_layou_set;

} STREAM_Config;

typedef struct {

  Uint16 numCaptureStream;
  Uint16 numEncodeStream;

  VIDEO_EncodeConfig      encodeConfig[AVSERVER_MAX_STREAMS];
  AUDIO_Config            audioConfig;
  STREAM_Config			  streamConfig;
  
} AVSERVER_Config;

#endif  /*  _AV_SERVER_CONFIG_H_  */

