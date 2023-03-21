#include <avserver.h>
#include <stream.h>
//#include <encode.h>

//GlobalData gbl = { 0, 0, 0, 0, 0, NOSTD };
static int IsDrawDateTime  = 0;

int VIDEO_streamSysInit()
{
  int status, cnt;
  STREAM_SET streamSet;

 // pthread_mutex_init(&gbl.mutex, NULL);

  memset(&streamSet, 0, sizeof(streamSet));

	for(cnt = 0; cnt < gAVSERVER_config.numEncodeStream; cnt++)
	{
  		streamSet.ImageWidth[cnt]  	= gAVSERVER_config.encodeConfig[cnt].cropWidth;
  		streamSet.ImageHeight[cnt] 	= gAVSERVER_config.encodeConfig[cnt].cropHeight;
  		streamSet.Mpeg4Quality[cnt]	= gAVSERVER_config.encodeConfig[cnt].codecBitrate;
//		streamSet.JpgQuality   		= gAVSERVER_config.encodeConfig[1].qValue;
	}

#if 0
  switch(gAVSERVER_config.encodeConfig[0].codecType) {
    case ALG_VID_CODEC_H264:
    case ALG_VID_CODEC_MPEG4:
      break;
    case ALG_VID_CODEC_MJPEG:
      streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[0].cropWidth;
      streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[0].cropHeight;
      break;

  }

  if(gAVSERVER_config.numEncodeStream>1) {
    switch(gAVSERVER_config.encodeConfig[1].codecType) {
      case ALG_VID_CODEC_H264:
      case ALG_VID_CODEC_MPEG4:
        streamSet.ImageWidth_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[1].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[1].cropHeight;
        break;
      case ALG_VID_CODEC_MJPEG:
        streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[1].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[1].cropHeight;
        break;

    }
  }

  if(gAVSERVER_config.numEncodeStream>2) {
    switch(gAVSERVER_config.encodeConfig[2].codecType) {
      case ALG_VID_CODEC_H264:
      case ALG_VID_CODEC_MPEG4:
        streamSet.ImageWidth_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[2].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[2].cropHeight;
        break;
      case ALG_VID_CODEC_MJPEG:
        streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[2].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[2].cropHeight;
        break;

    }
  }

  for( cnt = 0; cnt < STREAM_EXT_NUM ; cnt++ )
	{
    OSA_printf(" STREAM: Ext %d: %dx%d\n",
      cnt,
      streamSet.ImageWidth_Ext[cnt],
      streamSet.ImageHeight_Ext[cnt]
      );
  }
#endif
	streamSet.Mem_layout	= gAVSERVER_config.streamConfig.mem_layou_set;

	status = stream_init( stream_get_handle(), &streamSet);

	if(status!=OSA_SOK)
		OSA_ERROR("stream_init()\n");

	return status;
}

int VIDEO_streamSysExit()
{
  stream_end( stream_get_handle() );

//  pthread_mutex_destroy(&gbl.mutex);

  return OSA_SOK;
}

int VIDEO_streamShmCopy(int streamId, OSA_BufInfo *pBufInfo)
{
  int  status=OSA_SOK;
  int frameType, streamType = -1;
  Uint32 timestamp;

  switch(gAVSERVER_config.encodeConfig[streamId].codecType)
  {
    case ALG_VID_CODEC_H264:
    	streamType = streamId;
		break;
#if 0    	
      if(streamId==0)
        streamType = STREAM_H264_1;
      else
        streamType = STREAM_H264_2;

      break;
    case ALG_VID_CODEC_MPEG4:
      if(streamId==0)
        streamType = STREAM_MP4;
      else
        streamType = STREAM_MP4_EXT;
      break;
    case ALG_VID_CODEC_MJPEG:
      streamType = STREAM_MJPG;
      break;
#endif              
  }

  switch(pBufInfo->isKeyFrame) {
		case VIDEO_ENC_FRAME_TYPE_KEY_FRAME:
			frameType = 1;
			break;
		case VIDEO_ENC_FRAME_TYPE_NON_KEY_FRAME:
			frameType = 2;
			break;
		default:
			frameType = -1;
			break;
  }

  timestamp = pBufInfo->timestamp;

  #if 1
  status = stream_write(
        pBufInfo->virtAddr,
        pBufInfo->size,
        frameType,
        streamType,
        timestamp,
        stream_get_handle()
      );

  if(status!=OSA_SOK) {
    OSA_ERROR("stream_write(%d, %d, %d, %d)\n",
        pBufInfo->size,
        frameType,
        streamType,
        timestamp
      );
  }
  #endif

  return status;
}


