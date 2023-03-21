/**
 @file bsk_tplay.h
 @defgroup BASKET_TPLAY Trick play
 @ingroup BASKET
 @brief Implements of basket's  decode
 */

///////////////////////////////////////////////////////////////////////////////
#ifndef _BSK_TPLAY_H_
#define _BSK_TPLAY_H_
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#include <udbasket.h>
#include "global.h"
#include "basket_muldec.h"

#include "ti_vdis.h"

///////////////////////////////////////////////////////////////////////////////

/* =============================================================================
 * Structure
 * =============================================================================
 */

#define MCFW_IPC_BITS_GENERATE_INI_AUTOMATIC                        0
#define MCFW_IPC_BITS_INI_FILE_NO                                   "file"
#define MCFW_IPC_BITS_INI_FILE_PATH                                 "path"
#define MCFW_IPC_BITS_INI_FILE_WIDTH                                "width"
#define MCFW_IPC_BITS_INI_FILE_HEIGHT                               "height"
#define MCFW_IPC_BITS_INI_FILE_ENABLE                               "enable"
#define MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI                           100
#define MCFW_IPC_BITS_MAX_RES_NUM                                   10

#define MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH                 (256)
#define MCFW_IPCBITS_SENDFXN_TSK_PRI                                (2)
#define MCFW_IPCBITS_SENDFXN_TSK_STACK_SIZE                         (0) /* 0 means system default will be used */
#define MCFW_IPCBITS_SENDFXN_PERIOD_MS                              (16)
#define MCFW_IPCBITS_INFO_PRINT_INTERVAL                            (1000)
#define MCFW_IPCBITS_GET_BITBUF_SIZE(width,height)                  ((width) * (height)/2)

typedef struct {

    FILE    *fpRdHdr[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];

    int      fdRdData[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];

    UInt32 lastId[MCFW_IPC_BITS_MAX_RES_NUM];

    //OSA_ThrHndl   thrHandle;
    //OSA_SemHndl   thrStartSem;
    volatile Bool thrExit;

} VdecVdis_IpcBitsCtrl;


typedef struct {

    char    path[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];
    UInt32  width;
    UInt32  height;
    Bool    enable;
    UInt32  validResId;

} VdecVdis_FileInfo;


typedef struct {

    UInt32 width;
    UInt32 height;

} VdecVdis_res;

typedef struct {
    UInt32 channel;
    UInt16 speed;

} VdecVdis_TrickPlayMode;

typedef struct {

    UInt32              fileNum;
    UInt32              numRes; // Numbers of resolution in vaild file
    VdecVdis_res        res[MCFW_IPC_BITS_MAX_RES_NUM];
    UInt32              numChnlInRes[MCFW_IPC_BITS_MAX_RES_NUM]; // file numbers in each valid resolution
    UInt32              resToChnl[MCFW_IPC_BITS_MAX_RES_NUM][MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI]; // chnl IDs array for each res
    //VdecVdis_FileInfo   fileInfo[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];
    VdecVdis_TrickPlayMode   gDemo_TrickPlayMode;

}VdecVdis_Config;


extern VdecVdis_Config  gVdecVdis_config;


Int32 VdecVdis_bitsRdInit();

Void VdecVdis_bitsRdGetEmptyBitBufs(VCODEC_BITSBUF_LIST_S *emptyBufList, UInt32 resId);
Void VdecVdis_bitsRdReadData(VCODEC_BITSBUF_LIST_S  *emptyBufList,UInt32 resId);
Void VdecVdis_bitsRdSendFullBitBufs( VCODEC_BITSBUF_LIST_S *fullBufList);
UInt32 VdecVdis_getChnlIdFromBufSize(UInt32 resId);

void  VdecVdis_setTplayConfig(VDIS_CHN vdispChnId, VDIS_AVSYNC speed); // BKKIM


Int32 VdecVdis_bitsRdInit();
Int32 VdecVdis_bitsRdExit();

Void  VdecVdis_bitsRdStop(void);
Void  VdecVdis_bitsRdStart(void);


///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"{
#endif
///////////////////////////////////////////////////////////////////////////////

typedef T_BKTMD_CTRL T_TPLAY_CTRL;

void tplay_close(int bStop);

long tplay_open(int ch, long sec, T_BKTMD_OPEN_PARAM* pOP);
long tplay_open_next(int ch, long nextbid);
long tplay_open_prev(int ch, long prevbid);
long tplay_get_next_bid(T_TPLAY_CTRL* pCtrl);
long tplay_get_target_disk(int ch, long sec);
void tplay_set_target_disk(int ch, char* path);
long tplay_read_next_frame(int ch, T_BKTMD_PARAM *prm);
long tplay_read_next_iframe(int ch, T_BKTMD_PARAM *dp);
long tplay_read_1st_frame_of_next_basket(T_BKTMD_CTRL* pCtrl, T_BKTMD_PARAM *dp);

long tplay_get_prev_bid(T_TPLAY_CTRL* pCtrl);
long tplay_get_prev_bid_hdd(int ch, long curhdd);
long tplay_read_prev_iframe(int ch, T_BKTMD_PARAM *dp);

long tplay_get_next_bid_hdd(int ch, long curhdd);
long tplay_get_basket_time(T_BKTMD_CTRL* pCtrl, long *o_begin_sec, long *o_end_sec);
long tplay_get_cur_play_sec(int ch);
int  tplay_get_bsk_info(long *bid, long *fpos, int *curhdd, int *nexthdd);

int  tplay_is_open();

///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#endif//_BSK_TPLAY_H_
///////////////////////////////////////////////////////////////////////////////
