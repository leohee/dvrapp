/*
 *  * DM816X Motion Detection Algotithm
 *   */

#ifndef _DM81XX_MD_ALG_H_
#define _DM81XX_MD_ALG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <osa_buf.h>
#include <osa_tsk.h>
#include <osa_mutex.h>
#include <osa_prf.h>
#include "global.h"
#include "app_manager.h"


#define VIDEOMOTION_THR_PRI       (82)
#define VIDEOMOTION_STACK_SIZE    (10*KB)

#define VIDEOMOTION_BUF_MAX_FRAMES_IN_BUF  (16)
#define VIDEOMOTION_MIN_IN_BUF_PER_CH  (1)
#define VIDEOMOTION_CMD_CREATE    (0x0A00)
#define VIDEOMOTION_CMD_RUN       (0x0A01)
#define VIDEOMOTION_CMD_DELETE    (0x0A02)







#define ALG_MOTION_DETECT_HORZ_REGIONS  			MOTION_W_CELL
#define ALG_MOTION_DETECT_VERT_REGIONS  			(MOTION_H_CELL + MOTION_PAL_INC)

/*Following values are Sensitivity levels that can come from UI */

#define ALG_MOTION_DETECT_SENSITIVITY_LOW           (0)
#define ALG_MOTION_DETECT_SENSITIVITY_MEDIUM        (1)
#define ALG_MOTION_DETECT_SENSITIVITY_HIGH          (2)

#define ALG_MOTION_DETECT_MAX_DETECT_CNT            (5)

enum
{
    ALG_MOTION_S_DETECT = 100,
    ALG_MOTION_S_NO_DETECT ,

    ALG_MOTION_S_FAIL   = -1,
    ALG_MOTION_S_OK     = 0
};

/* MV and SAD structures */


typedef struct
{
	/*Starting position of data from the buffer base address*/
	unsigned int StartPos;
	/* No. of bytes to jump from the current position to get the next data of this element group */
	unsigned short Jump;
	/* Number of data elements in this group */
	unsigned short Count;

}ElementInfo;


typedef struct
{
	unsigned int NumElements;
	ElementInfo elementInfoField0SAD;
	ElementInfo elementInfoField1SAD;
	ElementInfo elementInfoField0MVL0;
	ElementInfo elementInfoField0MVL1;
	ElementInfo elementInfoField1MVL0;
	ElementInfo elementInfoField1MVL1;

}AnalyticHeaderInfo;

typedef struct
{
    unsigned int mbMvInfo;
    unsigned int mvJump;

    int ImageWidth;
    int ImageHeight;
    int windowWidth;
    int windowHeight;
    int isKeyFrame;
    int codecType;
    int isDateTimeDraw;
    unsigned char windowEnableMap[MAX_CH][MAX_MOTION_CELL];

}MD_RunPrm_t;

/*The following structure will store the information for motion detected for a particular selected window */

typedef struct
{
    char windowMotionDetected[ALG_MOTION_DETECT_HORZ_REGIONS][ALG_MOTION_DETECT_VERT_REGIONS];

}MD_RunStatus_t;


typedef struct
{
	int chId ;
    int frame_width;
    int frame_height;
    int frameCnt;
    int startCnt;
    int SAD_THRESHOLD;
    int threshold;
    int win_width;
    int win_height;
    int MvDataOffset;
    int warning_count;
    int detectCnt;

    int motionenable[MAX_CH];
    int motioncenable;
    int motioncvalue;
    int motionlevel[MAX_CH];
    int motionsens;
    int blockNum;

    MD_RunPrm_t         runPrm;
    MD_RunStatus_t      runStatus;

}MD_Hndl_t;

typedef struct {

  int numCh;
  int frameWidth;
  int frameHeight;
  int isKeyFrame;
  int codecType;

} VIDEOMOTION_CreatePrm;


typedef struct {

  OSA_TskHndl tskHndl;
  OSA_MbxHndl mbxHndl;

  OSA_MutexHndl mutexLock;

  VIDEOMOTION_CreatePrm createPrm;

  Uint16 bufNum;
  Uint16 motionEnable ;
  Uint16 motionCEnable ;
  Uint16 motionCValue ;
  Uint16 motionLevel ;
  Uint16 motionBlock ;
  int motionstatus[MAX_DVR_CHANNELS] ;

  Uint8 *bufVirtAddr;
  Uint8 *bufPhysAddr;
  Uint32 bufSize;

  OSA_BufCreate bufCreate;
  OSA_BufHndl   bufHndl;

  void *algMotionHndl ;

  OSA_PrfHndl prfFileWr;

} VIDEOMOTION_Ctrl;

extern VIDEOMOTION_Ctrl gVIDEOMOTION_ctrl;


int DM816X_MD_Create();
int DM816X_MD_SetPrm(int streamId, int pPrm, int sensitivity, unsigned char* motionTable) ;
int DM816X_MD_Apply(unsigned int mvBuf,unsigned int frameWidth,unsigned int frameHeight,unsigned int isKeyFrame);
int DM816X_MD_Delete();
int VIDEO_motionCreate(VIDEOMOTION_CreatePrm *prm);
int VIDEO_motionDelete();
unsigned int getMotionStatus( int *MotionStatus) ;


OSA_BufInfo *VIDEOMOTION_getEmptyBuf(int *bufId, int timeout);
int VIDEOMOTION_putFullBuf(int chId, OSA_BufInfo *buf, int bufId);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DM81XX_MD_ALG_H_ */
