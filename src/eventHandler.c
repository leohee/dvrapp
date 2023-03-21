#include "ti_media_std.h"
#include "ti_vcap_common_def.h"
#include "ti_vdec_common_def.h"
#include <mcfw/src_linux/mcfw_api/ti_vcap_priv.h>
#include "crash_dump_analyzer_format.h"

int Demo_captureGetVideoSourceStatus()
{
    UInt32 chId;

    VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
    VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;

    Vcap_getVideoSourceStatus( &videoSourceStatus );

    printf(" \n");

    for(chId=0; chId<videoSourceStatus.numChannels; chId++)
    {
        pChStatus = &videoSourceStatus.chStatus[chId];

        if(pChStatus->isVideoDetect)
        {
            printf (" DEMO: %2d: Detected video at CH [%d,%d] (%dx%d@%dHz, %d)!!!\n",
                     chId,
                     pChStatus->vipInstId, pChStatus->chId, pChStatus->frameWidth,
                     pChStatus->frameHeight,
                     1000000 / pChStatus->frameInterval,
                     pChStatus->isInterlaced);
        }
        else
        {
            printf (" DEMO: %2d: No video detected at CH [%d,%d] !!!\n",
                 chId, pChStatus->vipInstId, pChStatus->chId);

        }
    }

    printf(" \n");

    return 0;
}

int Demo_captureGetCaptureChNum (int scdChNum)
{
    int newChNum = scdChNum;
    VSYS_PARAMS_S contextInfo;

    Vsys_getContext(&contextInfo);
/*
    if ((contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_VCAP_VENC)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH)
            || (contextInfo.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH)
        )
*/
    if(scdChNum >= Vcap_getNumChannels())
    {
        newChNum -=   Vcap_getNumChannels();
    }
    /* Please add for other usecase */
    return newChNum;
}

int Demo_captureGetTamperStatus(Ptr pPrm)
{
    AlgLink_ScdChStatus *pScdStat = (AlgLink_ScdChStatus *)pPrm;

    if(pScdStat->frmResult == ALG_LINK_SCD_DETECTOR_CHANGE)
    {
        printf(" [TAMPER DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdStat->chId, Demo_captureGetCaptureChNum(pScdStat->chId));
    }

    return 0;
}


int Demo_captureGetMotionStatus(Ptr pPrm)
{
    AlgLink_ScdResult *pScdResult = (AlgLink_ScdResult *)pPrm;

    printf(" [MOTION DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdResult->chId, Demo_captureGetCaptureChNum(pScdResult->chId));

    return 0;
}

/** 
********************************************************************************
 *  @fn     TestApp_errorReport
 *  @brief  Printing all the errors that are set by codec
 *
 *  @param[in] errMsg : Error Message
 *          
 *  @return None
********************************************************************************
*/

int Demo_decodeErrorStatus(Ptr pPrm)
{

   int errBits;
   int firstTime = 1;
   VDEC_CH_ERROR_MSG * decodeErrMsg = (VDEC_CH_ERROR_MSG *) pPrm;
   int chId;
   int decodeErr;

   chId      = decodeErrMsg->chId;
   decodeErr = decodeErrMsg->errorMsg;

   for(errBits = 0; errBits < 32; errBits++)
   {
     if(decodeErr & (1 << errBits))
     {
       {
        if(firstTime)
        {
//          printf("Error Name: \t BitPositon in ErrorMessage\n");
            firstTime = 0;
        }
        printf("[DECODER ERROR] %d: DECODE CH <%d> ERROR: %s \n", OSA_getCurTimeInMsec(), chId, gDecoderErrorStrings[errBits].errorName);           
       }
     }
   }

    return 0;
}

static Void  Demo_printSlaveCoreExceptionContext(VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo)
{
    FILE *fpCcsCrashDump;
    char crashDumpFileName[100];

    printf("\n\n%d:!!!SLAVE CORE DOWN!!!.EXCEPTION INFO DUMP\n",OSA_getCurTimeInMsec());
    printf("\n !!HW EXCEPTION ACTIVE (0/1): [%d]\n",excInfo->exceptionActive);
    printf("\n !!EXCEPTION CORE NAME      : [%s]\n",excInfo->excCoreName);
    printf("\n !!EXCEPTION TASK NAME      : [%s]\n",excInfo->excTaskName);
    printf("\n !!EXCEPTION LOCATION       : [%s]\n",excInfo->excSiteInfo);
    printf("\n !!EXCEPTION INFO           : [%s]\n",excInfo->excInfo);
    snprintf(crashDumpFileName, sizeof(crashDumpFileName),"CCS_CRASH_DUMP_%s.txt",excInfo->excCoreName);
    fpCcsCrashDump = fopen(crashDumpFileName,"w");
    if (fpCcsCrashDump)
    {
        Demo_CCSCrashDumpFormatSave(excInfo,fpCcsCrashDump);
        fclose(fpCcsCrashDump);
        printf("\n !!EXCEPTION CCS CRASH DUMP FORMAT FILE STORED @ ./%s\n",crashDumpFileName);
    }
}

Int32 Demo_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData)
{
    if(eventId==VSYS_EVENT_VIDEO_DETECT)
    {
        printf(" \n");
        printf(" DEMO: Received event VSYS_EVENT_VIDEO_DETECT [0x%04x]\n", eventId);

        Demo_captureGetVideoSourceStatus();
    }

    if(eventId==VSYS_EVENT_TAMPER_DETECT)
    {
        Demo_captureGetTamperStatus(pPrm);
    }

    if(eventId==VSYS_EVENT_MOTION_DETECT)
    {
        Demo_captureGetMotionStatus(pPrm);
    }

    if(eventId== VSYS_EVENT_DECODER_ERROR)
    {
        Demo_decodeErrorStatus(pPrm);
    }

    if (eventId == VSYS_EVENT_SLAVE_CORE_EXCEPTION)
    {
        Demo_printSlaveCoreExceptionContext(pPrm);
    }

    return 0;
}
