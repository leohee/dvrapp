#include "common_def.h"
#include "avi.h"
#include "app_manager.h"
#include "disk_control.h"
#include "mcfw/interfaces/ti_media_std.h"
#include "ti/ipc/SharedRegion.h"
#include "ti/ipc/MessageQ.h"
#include "ti/syslink/ProcMgr.h"
#include "ti/syslink/utils/List.h"
#include "mcfw/src_linux/devices/sii9022a/src/sii9022a_priv.h"
#include "mcfw/interfaces/link_api/avsync.h"
#include "mcfw/interfaces/link_api/vidbitstream.h"
#include "ti/syslink/utils/_MemoryDefs.h"
#include "ti/ipc/GateMP.h"
#include "ti/ipc/ListMP.h"
#include "ti/ipc/HeapMemMP.h"
#include "mcfw/interfaces/link_api/system.h"
#include "mcfw/interfaces/link_api/ipcLink.h"
#include "mcfw/interfaces/link_api/audioLink.h"
#include "ti/syslink/utils/IHeap.h"
#include "ti/ipc/Notify.h"

Ptr SharedRegion_getPtr(SharedRegion_SRPtr srptr)
{
	return NULL;
}

Bool SharedRegion_isCacheEnabled(UInt16 regionId)
{
	return FALSE;
}

SharedRegion_SRPtr SharedRegion_getSRPtr(Ptr addr, UInt16 regionId)
{
	return 0;
}

Ptr SharedRegion_getHeap(UInt16 regionId)
{
	return NULL;
}

SharedRegion_SRPtr SharedRegion_invalidSRPtr(Void)
{
	return 0;
}

UInt16 SharedRegion_getId(Ptr addr)
{
	return 0;
}

Void *Audio_createG711DecAlgorithm(Void *ctxMem)
{
}

Void *Audio_createAacDecAlgorithm(Void *ctxMem, Audio_DecodeCreateParams *pPrm)
{
}

Void* Audio_createAacEncAlgorithm(Void *ctxMem, Audio_EncodeCreateParams *pPrm)
{
}

Int32 Audio_decode(Void* ctxMem, Audio_DecodeProcessParams *pPrm)
{
	return 0;
}

Int32  Audio_getDecoderContextSize (Int32 codecType)
{
	return 0;
}

Int32   Audio_getEncoderContextSize (Int32 codecType)
{
	return 0;
}

Int32 Audio_deleteEncAlgorithm(Void *ctxMem)
{
	return 0;
}

Int32 Audio_deleteDecAlgorithm(Void *ctxMem)
{
	return 0;
}

Int32 Audio_encode(Void* ctxMem, Audio_EncodeProcessParams *pPrm)
{
	return 0;
}

Void *Audio_createG711EncAlgorithm(Void *ctxMem)
{
}

Int ProcMgr_open (ProcMgr_Handle * handlePtr, UInt16 procId)
{
	return 0;
}

Int ProcMgr_getSymbolAddress (ProcMgr_Handle handle,
                              UInt32         fileId,
                              String         symbolName,
                              UInt32 *       symValue)
{
	return 0;
}

ProcMgr_State ProcMgr_getState (ProcMgr_Handle handle)
{
	return ProcMgr_State_Unknown;
}

UInt32 ProcMgr_getLoadedFileId (ProcMgr_Handle handle)
{
	return 0;
}

Int ProcMgr_close (ProcMgr_Handle * handlePtr)
{
	return 0;
}

Int MessageQ_close(MessageQ_QueueId *queueId)
{
	return 0;
}

Void MessageQ_setReplyQueue(MessageQ_Handle handle, MessageQ_Msg msg)
{
}

Int MessageQ_unregisterHeap(UInt16 heapId)
{
	return 0;
}

Int MessageQ_get(MessageQ_Handle handle, MessageQ_Msg *msg, UInt timeout)
{
	return 0;
}

Int MessageQ_free(MessageQ_Msg msg)
{
	return 0;
}

Int MessageQ_open(String name, MessageQ_QueueId *queueId)
{
	return 0;
}

Int MessageQ_put(MessageQ_QueueId queueId, MessageQ_Msg msg)
{
	return 0;
}

MessageQ_Msg MessageQ_alloc(UInt16 heapId, UInt32 size)
{
	return NULL;
}

MessageQ_Handle MessageQ_create(String name, const MessageQ_Params *params)
{
	return NULL;
}

Void MessageQ_Params_init(MessageQ_Params *params)
{
}

Void MessageQ_unblock(MessageQ_Handle handle)
{
}

Int MessageQ_delete(MessageQ_Handle *handlePtr)
{
	return 0;
}

Int MessageQ_registerHeap(Ptr heap, UInt16 heapId)
{
	return 0;
}





Int32 AvsyncLink_init()
{
	return 0;
}

Int32 Avsync_doPlay(Avsync_PlayParams *playParams)
{
	return 0;
}

Int32 Avsync_stepFwd(Avsync_StepFwdParams *stepFwdParams)
{
	return 0;
}

Int32 Avsync_setVideoBackEndDelay(Avsync_VideoBackendDelayParams *delayParams)
{
	return 0;
}

Int32 Avsync_doScan(Avsync_ScanParams *scanParams)
{
	return 0;
}

Int32 Avsync_seekPlayback(Avsync_SeekParams *seekParams)
{
	return 0;
}

Int32 Avsync_doPause(Avsync_PauseParams *pauseParams)
{
	return 0;
}

Int32 Avsync_configSyncConfigInfo(AvsyncLink_LinkSynchConfigParams *cfg)
{
	return 0;
}

Int32 Avsync_setPlaybackSpeed(Avsync_TimeScaleParams *timeScaleParams)
{
	return 0;
}

Int32 AvsyncLink_deInit()
{
	return 0;
}

Int32 Avsync_resetPlayerTimer(Avsync_ResetPlayerTimerParams *resetParams)
{
	return 0;
}

Int32 Avsync_printStats()
{
	return 0;
}

Int32 Avsync_setFirstVidPTS(Avsync_FirstVidPTSParams *vidPTSParams)
{
	return 0;
}

Int32 Avsync_doUnPause(Avsync_UnPauseParams *unPauseParams)
{
	return 0;
}

Int32 Avsync_setFirstAudPTS(Avsync_FirstAudPTSParams *audPTSParams)
{
	return 0;
}

Int32 Avsync_setWallTimeBase(UInt64 wallTimeBase)
{
	return 0;
}

Ptr List_get (List_Handle handle)
{
	return NULL;
}

Void List_construct (List_Object * obj, List_Params * params)
{
}

Void List_destruct (List_Object * obj)
{
}

Void List_put (List_Handle handle, List_Elem * elem)
{
}

Void List_Params_init (List_Params * params)
{
}

Int32 Device_sii9022aDelete(Device_Sii9022aHandle handle, Ptr deleteArgs)
{
	return 0;
}

Device_Sii9022aHandle Device_sii9022aCreate (UInt32 drvId,
                                               UInt32 instanceId,
                                               Ptr createArgs,
                                               Ptr createStatusArgs)
{
	return NULL;
}

Int32 Device_sii9022aInit(void)
{
	return 0;
}

UInt32 Device_getVidDecI2cAddr(UInt32 vidDecId, UInt32 vipInstId)
{
	return 0;
}

Int32 Device_sii9022aControl (Device_Sii9022aHandle handle,
                              UInt32 cmd,
                              Ptr cmdArgs,
                              Ptr cmdStatusArgs)
{
	return 0;
}

Int Notify_sendEvent(UInt16 procId, UInt16 lineId, UInt32 eventId,
        UInt32 payload, Bool waitClear)
{
	return 0;
}

Bool Notify_intLineRegistered(UInt16 procId, UInt16 lineId)
{
	return FALSE;
}

Int Notify_unregisterEvent(UInt16 procId, UInt16 lineId, UInt32 eventId,
        Notify_FnNotifyCbck fnNotifyCbck, UArg cbckArg)
{
	return 0;
}

Bool Notify_eventAvailable(UInt16 procId, UInt16 lineId, UInt32 eventId)
{
	return FALSE;
}

Int Notify_registerEvent(UInt16 procId,
                         UInt16 lineId,
                         UInt32 eventId,
                         Notify_FnNotifyCbck fnNotifyCbck,
                         UArg cbckArg)
{
	return 0;
}

Ptr Memory_translate (Ptr srcAddr, Memory_XltFlags flags)
{
	return NULL;
}

Ptr Memory_alloc (IHeap_Handle heap, SizeT size, SizeT align, Ptr eb)
{
	return NULL;
}

Int MemoryOS_map (Memory_MapInfo * mapInfo)
{
	return 0;
}

Int MemoryOS_unmap (Memory_UnmapInfo * unmapInfo)
{
	return 0;
}

Void Memory_free (IHeap_Handle heap, Ptr block, SizeT size)
{
}


GateMP_Handle GateMP_create(const GateMP_Params *params)
{
	return NULL;
}

Int GateMP_delete(GateMP_Handle *handlePtr)
{
	return 0;
}

Void GateMP_Params_init(GateMP_Params *params)
{
}

ListMP_Handle ListMP_create(const ListMP_Params *params)
{
	return NULL;
}

Bool ListMP_empty(ListMP_Handle handle)
{
	return FALSE;
}

Int ListMP_putTail(ListMP_Handle handle, ListMP_Elem *elem)
{
	return 0;
}

Ptr ListMP_getHead(ListMP_Handle handle)
{
	return 0;
}

Void ListMP_Params_init(ListMP_Params *params)
{
}

Int ListMP_delete(ListMP_Handle *handlePtr)
{
	return 0;
}

Int ListMP_open(String name, ListMP_Handle *handlePtr)
{
	return 0;
}

Int ListMP_close(ListMP_Handle *handlePtr)
{
	return 0;
}

Void List_elemClear (List_Elem * elem)
{
}

Bool List_empty (List_Handle handle)
{
	return FALSE;
}


Int HeapMemMP_close(HeapMemMP_Handle *handlePtr)
{
	return 0;
}

Int HeapMemMP_open(String name, HeapMemMP_Handle *handlePtr)
{
	return 0;
}


Int32 IpcBitsInLink_deInit()
{
	return 0;
}

Int32 IpcBitsInLink_putEmptyVideoBitStreamBufs(UInt32 linkId, Bitstream_BufList *bufList)
{
	return 0;
}

Int32 IpcBitsOutLink_putFullVideoBitStreamBufs(UInt32 linkId, Bitstream_BufList *bufList)
{
	return 0;
}

Int32 IpcBitsOutLink_getEmptyVideoBitStreamBufs(UInt32 linkId,
	Bitstream_BufList *bufList, IpcBitsOutLinkHLOS_BitstreamBufReqInfo *reqInfo)
{
	return 0;
}

Int32 IpcBitsInLink_getFullVideoBitStreamBufs(UInt32 linkId, Bitstream_BufList *bufList)
{
	return 0;
}

Int32 IpcBitsOutLink_getInQueInfo(UInt32 linkId, System_LinkQueInfo *inQueInfo)
{
	return 0;
}

Int32 IpcBitsOutLink_init()
{
	return 0;
}

Int32 IpcBitsInLink_init()
{
	return 0;
}

Int32 IpcBitsOutLink_deInit()
{
	return 0;
}

UInt16 MultiProc_self(Void)
{
	return 0;
}

String MultiProc_getName(UInt16 id)
{
	return NULL;
}

UInt16 MultiProc_getId(String name)
{
	return 0;
}


Void SysLink_destroy (Void)
{
}

Void SysLink_setup (Void)
{
}


void nsDVR_AVI_close(handle hAVI)
{
}


handle nsDVR_AVI_create(stNSDVR_AVIInfo* pInfo, int *error_code)
{
	return NULL;
}

char* nsDVR_util_error_name(int code)
{
	return NULL;
}

int nsDVR_AVI_add_frame(handle hAVI, stNSDVR_FrameInfo* pFrame)
{
	return 0;
}

int nsDVR_CDROM_check_ready(char* devicePath)
{
	return 0;
}

int nsDVR_CDROM_get_media_info(char* devicePath, stNSDVR_CDROMStatus* pInfo)
{
	return 0;
}

char* nsDVR_CDROM_media_type_name(int type)
{
	return NULL;
}

int nsDVR_CDROM_eject(char* devicePath)
{
	return 0;
}

handle nsDVR_CDROM_eraser_open(char* devicePath, int *error_code)
{
	return NULL;
}

int nsDVR_CDROM_eraser_status(handle hEraser)
{
	return 0;
}

void nsDVR_CDROM_eraser_close(handle hEraser)
{
}

handle nsDVR_CDROM_mkiso_open(char* isoFilePath, char** fileList, int nFiles, int *error_code)
{
	return NULL;
}

int nsDVR_CDROM_mkiso_status(handle hMKISO)
{
	return 0;
}

void nsDVR_CDROM_mkiso_close(handle hMKISO)
{
}

handle nsDVR_CDROM_writer_open(char* devicePath, char* isoFilePath, int *error_code)
{
	return NULL;
}

int nsDVR_CDROM_writer_status(handle hWriter)
{
	return 0;
}

void nsDVR_CDROM_writer_close(handle hWriter)
{
}

int get_dvr_disk_size(const char *mount_name, unsigned long *total, unsigned long *used)
{
	return 0;
}

int get_dvr_disk_info(dvr_disk_info_t *ddi)
{
	return 0;
}

int dvr_fdisk(char *dev)
{
	return 0;
}

Void Cache_wb(Ptr blockPtr, UInt32 byteCnt, Bits16 type, Bool wait)
{
}

