
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <osa_tsk.h>
#include <osa_buf.h>
#include <osa_mutex.h>

#define VIDEO_NUM_BUF           (3)

#define VIDEO_TSK_NONE          (0x0000)
#define VIDEO_TSK_CAPTURE       (0x0001)
#define VIDEO_TSK_LDC           (0x0002)
#define VIDEO_TSK_VNF           (0x0004)
#define VIDEO_TSK_RESIZE        (0x0008)
#define VIDEO_TSK_ENCODE        (0x0010)
#define VIDEO_TSK_ENCRYPT       (0x0020)
#define VIDEO_TSK_STREAM        (0x0040)

#define VIDEO_BUF_HEADER_SIZE   (128)

#define VIDEO_ENC_FRAME_TYPE_NON_KEY_FRAME    (0)
#define VIDEO_ENC_FRAME_TYPE_KEY_FRAME        (1)

#define VIDEO_VS_FLAG_NEW_DATA     (0x0FFF)
#define VIDEO_VS_FLAG_EXIT         (0x8000)
#define VIDEO_VS_NUM_STATUS        (100)

#define VIDEO_STREAM_FILE_SAVE_STATE_IDLE   (0)
#define VIDEO_STREAM_FILE_SAVE_STATE_OPEN   (1)
#define VIDEO_STREAM_FILE_SAVE_STATE_WRITE  (2)
#define VIDEO_STREAM_FILE_SAVE_STATE_CLOSE  (3)


typedef struct {

  Uint32 timestamp;
  Uint32 frameNum;
  Uint32 startX;
  Uint32 startY;
  Uint32 width;
  Uint32 height;
  Uint32 offsetH;
  Uint32 offsetV;
  Uint32 encFrameSize;
  Uint32 encFrameType;

} VIDEO_BufHeader;

typedef struct {

  Uint32 frameNum;
  Uint16 startX;
  Uint16 startY;

} VIDEO_VsStatus;


typedef struct {

  int encodeNextTsk;

  OSA_BufHndl bufStreamIn;

  OSA_BufCreate bufStreamInCreatePrm;

  int curFrameCount;

  int 	newBitrate;
  int 	newFps;
  int	newMotionUpdate;
  int	newMotionStatus;

} VIDEO_EncodeStream;


typedef struct {

  OSA_TskHndl encodeTsk;
  OSA_TskHndl streamTsk;

  int rawCaptureCount;

  VIDEO_EncodeStream     encodeStream[AVSERVER_MAX_STREAMS];

} VIDEO_Ctrl;


int VIDEO_streamShmCopy(int streamId, OSA_BufInfo *pBufInfo);
int VIDEO_streamGetJPGSerialNum(void);

int VIDEO_streamSysInit();
int VIDEO_streamSysExit();


#endif  /*  _VIDEO_H_ */

