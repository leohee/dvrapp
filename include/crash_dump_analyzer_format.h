/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_crash_dump_analyzer_format.h
    \brief Interface to slave core exception info format functions.
*/


#ifndef DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_
#define DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <osa.h>
#include <mcfw/interfaces/ti_media_error_def.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

/**
*******************************************************************************
 *  @struct sEnumToStringMapping
 *  @brief  Error Name Mapping to give error message.
 *          This structure contains error reporting strings which are mapped to
 *          Codec errors
 *
 *  @param  errorName : Pointer to the error string
 * 
*******************************************************************************
*/
typedef struct _sEnumToStringMapping
{
  char *errorName;
}sEnumToStringMapping;

/*----------------------------------------------------------------------------*/
/* Error strings which are mapped to codec errors                             */
/* Please refer User guide for more details on error strings                  */
/*----------------------------------------------------------------------------*/
static sEnumToStringMapping gDecoderErrorStrings[32] = 
{
  {(char *)"IH264VDEC_ERR_NOSLICE : 0, \0"},
  {(char *)"IH264VDEC_ERR_SPS : 1,"},
  {(char *)"IH264VDEC_ERR_PPS : 2,\0"},
  {(char *)"IH264VDEC_ERR_SLICEHDR : 3,\0"},
  {(char *)"IH264VDEC_ERR_MBDATA : 4,\0"},
  {(char *)"IH264VDEC_ERR_UNAVAILABLESPS : 5,\0"},
  {(char *)"IH264VDEC_ERR_UNAVAILABLEPPS  : 6,\0"},
  {(char *)"IH264VDEC_ERR_INVALIDPARAM_IGNORE : 7\0"},
  {(char *)"XDM_PARAMSCHANGE : 8,\0"},
  {(char *)"XDM_APPLIEDCONCEALMENT : 9,\0"},
  {(char *)"XDM_INSUFFICIENTDATA : 10,\0"},
  {(char *)"XDM_CORRUPTEDDATA : 11,\0"},
  {(char *)"XDM_CORRUPTEDHEADER : 12,\0"},
  {(char *)"XDM_UNSUPPORTEDINPUT : 13,\0"},
  {(char *)"XDM_UNSUPPORTEDPARAM : 14,\0"},
  {(char *)"XDM_FATALERROR : 15\0"},
  {(char *)"IH264VDEC_ERR_UNSUPPFEATURE : 16,\0"},
  {(char *)"IH264VDEC_ERR_METADATA_BUFOVERFLOW : 17,\0"},
  {(char *)"IH264VDEC_ERR_STREAM_END : 18,\0"},
  {(char *)"IH264VDEC_ERR_NO_FREEBUF : 19,\0"},
  {(char *)"IH264VDEC_ERR_PICSIZECHANGE : 20,\0"},
  {(char *)"IH264VDEC_ERR_UNSUPPRESOLUTION : 21,\0"},
  {(char *)"IH264VDEC_ERR_NUMREF_FRAMES : 22,\0"},
  {(char *)"IH264VDEC_ERR_INVALID_MBOX_MESSAGE : 23,\0"},
  {(char *)"IH264VDEC_ERR_DATA_SYNC : 24,\0"},
  {(char *)"IH264VDEC_ERR_MISSINGSLICE : 25,\0"},
  {(char *)"IH264VDEC_ERR_INPUT_DATASYNC_PARAMS : 26,\0"},
  {(char *)"IH264VDEC_ERR_HDVICP2_IMPROPER_STATE : 27,\0"},
  {(char *)"IH264VDEC_ERR_TEMPORAL_DIRECT_MODE : 28,\0"},
  {(char *)"IH264VDEC_ERR_DISPLAYWIDTH : 29,\0"},
  {(char *)"IH264VDEC_ERR_NOHEADER : 30,\0"},
  {(char *)"IH264VDEC_ERR_GAPSINFRAMENUM : 31, \0"}
};


#define DEMO_CCS_CRASH_DUMP_M3_HEADER_STR                        "521177 14"
#define DEMO_CCS_CRASH_DUMP_M3_HEADER_PRINT_FMT(fp)              do { rewind(fp); fprintf(fp,"%s\n",DEMO_CCS_CRASH_DUMP_M3_HEADER_STR); } while(0)
#define DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT(fp,regName,regVal)     fprintf(fp,"R %s 0x0000000B 0x%08x\n",regName,(UInt32)regVal)
#define DEMO_CCS_CRASH_DUMP_MEM_HDR_PRINT_FMT(fp,baseAddr,size)  fprintf(fp,"M 0 0x%08x 0x%08x\n",(UInt32)baseAddr,size)
#define DEMO_CCS_CRASH_DUMP_MEM_PRINT_FMT(fp,val)                fprintf(fp,"0x%02x\n",val)

Int32 Demo_CCSCrashDumpFormatSave(VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo,FILE *fp);

#endif /* DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_ */
