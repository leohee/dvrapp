/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VSYS_API McFW System (VSYS) API

    Defines some common system level data structures

    @{
*/

/**
    \file ti_vsys_common_def.h
    \brief McFW System (VSYS) API
*/


#ifndef __TI_VSYS_COMMON_DEF_H__
#define __TI_VSYS_COMMON_DEF_H__

#ifdef  __cplusplus
extern "C" {
#endif

/* Define's */

/* @{ */

/**
   \brief 
    This event is received by McFW when there is a change in the video source.
    A change means either a video source is
    - connected
    - or disconnected
    - or changed from one standard to another standard

    On receiving this event user call Vcap APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_VIDEO_DETECT     (0x0000)


/**
    \brief 
    This event is received by McFW when camera tamper is detected by SCD link

    On receiving this event user call Scd APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_TAMPER_DETECT     (0x0001)

/**
    \brief 
    This event is received by McFW when motion is detected by SCD link

    On receiving this event user call Scd APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_MOTION_DETECT     (0x0002)

/**
    \brief 
    This event is received by McFW when decoder sees an error in decoding

    On receiving this event user can take appropriate action in the decoder
    channel on which this event has happended. The message packet received with
    this event is decoder error message and can be extracted by VDEC_CH_ERROR_MSG 
    structre defined in <mcfw/interfaces/common_def/ti_vdec_common_def.h>.
*/
#define VSYS_EVENT_DECODER_ERROR     (0x0003)

/**
    \brief 
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    \brief 
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    \brief 
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    \brief 
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
   \brief 
          Max number of encoder or decoder video channles
          that can be assign to any single IVA-HD
*/
#define VSYS_MAX_IVACH                      (48)

/**
   \brief Max number of IVA-HDs supported on the SoC
*/
#define VSYS_MAX_NUMHDVICP                  (3)

/**
   \brief Size of exception stack dump
*/
#define VSYS_EXCEPTION_TSK_STACK_SIZE       (32768)

/**
   \brief Max length of exception task name
*/
#define VSYS_EXCEPTION_TSK_NAME_LENGTH      (32)

/**
   \brief Max length of exception core name
*/
#define VSYS_EXCEPTION_CORE_NAME_LENGTH     (32)

/**
   \brief Max length of site info (Filename and line)
*/
#define VSYS_EXCEPTION_SITE_NAME_LENGTH     (1024)

/**
   \brief Max length of error info
*/
#define VSYS_EXCEPTION_INFO_LENGTH          (1024)

/**
   \brief Name of global Symbol storing slave core exception info
*/
#define VSYS_EXCEPTION_CONTEXT_SYMBOL_NAME    gExceptionInfo

/* @} */


/* Enum's */

/* @{ */

/**
    \brief Possible Video Standards - NTSC, PAL, CIF, etc
*/
typedef enum
{
    VSYS_STD_NTSC = 0u,
    /**< 720x480 30FPS interlaced NTSC standard. */
    VSYS_STD_PAL,
    /**< 720x576 30FPS interlaced PAL standard. */

    VSYS_STD_480I,
    /**< 720x480 30FPS interlaced SD standard. */
    VSYS_STD_576I,
    /**< 720x576 30FPS interlaced SD standard. */

    VSYS_STD_CIF,
    /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
    VSYS_STD_HALF_D1,
    /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
    VSYS_STD_D1,
    /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */

    VSYS_STD_480P,
    /**< 720x480 60FPS progressive ED standard. */
    VSYS_STD_576P,
    /**< 720x576 60FPS progressive ED standard. */

    VSYS_STD_720P_60,
    /**< 1280x720 60FPS progressive HD standard. */
    VSYS_STD_720P_50,
    /**< 1280x720 50FPS progressive HD standard. */

    VSYS_STD_1080I_60,
    /**< 1920x1080 30FPS interlaced HD standard. */
    VSYS_STD_1080I_50,
    /**< 1920x1080 50FPS interlaced HD standard. */

    VSYS_STD_1080P_60,
    /**< 1920x1080 60FPS progressive HD standard. */
    VSYS_STD_1080P_50,
    /**< 1920x1080 50FPS progressive HD standard. */

    VSYS_STD_1080P_24,
    /**< 1920x1080 24FPS progressive HD standard. */
    VSYS_STD_1080P_30,
    /**< 1920x1080 30FPS progressive HD standard. */

    VSYS_STD_VGA_60,
    /**< 640x480 60FPS VESA standard. */
    VSYS_STD_VGA_72,
    /**< 640x480 72FPS VESA standard. */
    VSYS_STD_VGA_75,
    /**< 640x480 75FPS VESA standard. */
    VSYS_STD_VGA_85,
    /**< 640x480 85FPS VESA standard. */

    VSYS_STD_SVGA_60,
    /**< 800x600 60FPS VESA standard. */
    VSYS_STD_SVGA_72,
    /**< 800x600 72FPS VESA standard. */
    VSYS_STD_SVGA_75,
    /**< 800x600 75FPS VESA standard. */
    VSYS_STD_SVGA_85,
    /**< 800x600 85FPS VESA standard. */

    VSYS_STD_XGA_60,
    /**< 1024x768 60FPS VESA standard. */
    VSYS_STD_XGA_70,
    /**< 1024x768 72FPS VESA standard. */
    VSYS_STD_XGA_75,
    /**< 1024x768 75FPS VESA standard. */
    VSYS_STD_XGA_85,
    /**< 1024x768 85FPS VESA standard. */

    VSYS_STD_WXGA_60,
    /**< 1280x768 60FPS VESA standard. */
    VSYS_STD_WXGA_75,
    /**< 1280x768 75FPS VESA standard. */
    VSYS_STD_WXGA_85,
    /**< 1280x768 85FPS VESA standard. */

    VSYS_STD_SXGA_60,
    /**< 1280x1024 60FPS VESA standard. */
    VSYS_STD_SXGA_75,
    /**< 1280x1024 75FPS VESA standard. */
    VSYS_STD_SXGA_85,
    /**< 1280x1024 85FPS VESA standard. */

    VSYS_STD_SXGAP_60,
    /**< 1400x1050 60FPS VESA standard. */
    VSYS_STD_SXGAP_75,
    /**< 1400x1050 75FPS VESA standard. */

    VSYS_STD_UXGA_60,
    /**< 1600x1200 60FPS VESA standard. */

    VSYS_STD_MUX_2CH_D1,
    /**< Interlaced, 2Ch D1, NTSC or PAL. */
    VSYS_STD_MUX_2CH_HALF_D1,
    /**< Interlaced, 2ch half D1, NTSC or PAL. */
    VSYS_STD_MUX_2CH_CIF,
    /**< Interlaced, 2ch CIF, NTSC or PAL. */
    VSYS_STD_MUX_4CH_D1,
    /**< Interlaced, 4Ch D1, NTSC or PAL. */
    VSYS_STD_MUX_4CH_CIF,
    /**< Interlaced, 4Ch CIF, NTSC or PAL. */
    VSYS_STD_MUX_4CH_HALF_D1,
    /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
    VSYS_STD_MUX_8CH_CIF,
    /**< Interlaced, 8Ch CIF, NTSC or PAL. */
    VSYS_STD_MUX_8CH_HALF_D1,
    /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */

    VSYS_STD_AUTO_DETECT,
    /**< Auto-detect standard. Used in capture mode. */
    VSYS_STD_CUSTOM,
    /**< Custom standard used when connecting to external LCD etc...
         The video timing is provided by the application.
         Used in display mode. */

    VSYS_STD_MAX
    /**< Should be the last value of this enumeration.
         Will be used by driver for validating the input parameters. */
} VSYS_VIDEO_STANDARD_E;


/**
    \brief Processors in the hardware platform - Not Used
*/
typedef enum VSYS_CORE_E
{
    VSYS_CORE_SLAVE_C674,
    /**< C674 Core - DSP */

    VSYS_CORE_SLAVE_VIDEO_M3,
    /**< M3- Video Core */

    VSYS_CORE_SLAVE_VPSS_M3,
    /**< M3- VPSS Core */

    VSYS_CORE_MASTER_CORTEXA8,
    /**< Cortex A8 - Master Core */

    VSYS_CORE_MAX
    /**< Max cores in the platform */
} VSYS_CORE_E;

/* @} */


/* Data structure's */
/* @{ */

/**
    \brief Window Data structure

    Used to specific resolution is VCAP, VDIS and other sub-systems
*/
typedef struct WINDOW_S
{
    UInt32 start_X;
    /**< Horizontal offset in pixels, from which picture needs to be positioned. */

    UInt32 start_Y;
    /**< Vertical offset from which picture needs to be positioned. */

    UInt32 width;
    /**< Width of frame in pixels. */

    UInt32 height;
    /**< Height of frame in lines. */

} WINDOW_S;


/**
    This event is received by McFW when there is a change in the video source.
    A change means either a video source is
    - connected
    - or disconnected
    - or changed from one standard to another standard

    On receiving this event user call Vcap APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_VIDEO_DETECT     (0x0000)


/**
    This event is received by McFW when camera tamper is detected by SCD link

    On receiving this event user call Scd APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_TAMPER_DETECT     (0x0001)

/**
    This event is received by McFW when motion is detected by SCD link

    On receiving this event user call Scd APIs to get details about the
    channel on which this event has happended
*/
#define VSYS_EVENT_MOTION_DETECT     (0x0002)

/**
    This event is received by McFW when decoder sees an error in decoding

    On receiving this event user can take appropriate action in the decoder
    channel on which this event has happended. The message packet received with
    this event is decoder error message and can be extracted by VDEC_CH_ERROR_MSG 
    structre defined in <mcfw/interfaces/common_def/ti_vdec_common_def.h>.
*/
#define VSYS_EVENT_DECODER_ERROR     (0x0003)

/**
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
    This event is received by McFW when one of the slave cores (VIDEO_M3/VPSS_M3/DSP)
    has encountered an unrecoverable exception

    This error indicates catastrophic failure of one of the slave cores.
*/
#define VSYS_EVENT_SLAVE_CORE_EXCEPTION  (0x0004)

/**
   \brief Max number of encoder or decoder video channles
          that can be assign to any single IVA-HD
*/
#define VSYS_MAX_IVACH                      (48)

/**
   \brief Max number of IVA-HDs supported on the SoC
*/
#define VSYS_MAX_NUMHDVICP                  (3)

/**
   \brief Size of exception stack dump
*/
#define VSYS_EXCEPTION_TSK_STACK_SIZE       (32768)

/**
   \brief Max length of exception task name
*/
#define VSYS_EXCEPTION_TSK_NAME_LENGTH      (32)

/**
   \brief Max length of exception core name
*/
#define VSYS_EXCEPTION_CORE_NAME_LENGTH     (32)

/**
   \brief Max length of site info (Filename and line)
*/
#define VSYS_EXCEPTION_SITE_NAME_LENGTH     (1024)

/**
   \brief Max length of error info
*/
#define VSYS_EXCEPTION_INFO_LENGTH          (1024)

/**
   \brief Name of global Symbol storing slave core exception info
*/
#define VSYS_EXCEPTION_CONTEXT_SYMBOL_NAME    gExceptionInfo

/**
    \brief User specified callback that gets called when a event occurs on the slave processor

    \param eventId  [OUT] This tells the user which event occured
                    based on this user can call other McFW APIs to get more details about the event

    \param appData  [OUT] User specified app data pointer during Vsys_registerEventHandler()
                    is returned here to the user.

    \return ERROR_NONE on sucess
*/
typedef Int32 (*VSYS_EVENT_HANDLER_CALLBACK)(UInt32 eventId, Ptr pPrm, Ptr appData);

/**
    \brief Slave Core Status
*/
typedef struct {

    VSYS_CORE_E  coreId;
    /**< coreId enum value - C674 or VPSS-M3, ... */

    Bool         isAlive;
    /**< Flag indicating core is alive */
} VSYS_CORE_STATUS_S;


/**
    \brief Slave Core Status Table
*/
typedef struct {

    UInt32 numSlaveCores;
    /**< Number of slave cores queired*/

    VSYS_CORE_STATUS_S coreStatus[VSYS_CORE_MAX];
    /**< Flag indicating core is alive */
} VSYS_CORE_STATUS_TBL_S;

/**
   \brief Data structure to assign the video channles to any single IVA-HD
*/
typedef struct VSYS_IVA2CHMAP_S {
    UInt32 EncNumCh;
    /**< Number of Encoder channels */
    UInt32 EncChList[VSYS_MAX_IVACH];
    /**< Encoder channel list */
    UInt32 DecNumCh;
    /**< Number of Decoder channels */
    UInt32 DecChList[VSYS_MAX_IVACH];
    /**< Decoder channel list */
} VSYS_IVA2CHMAP_S;

/**
   \brief Data structure to assign the video channles to all 3 IVA-HDs
*/
typedef struct VSYS_IVA2CHMAP_TBL_S {
    UInt32 isPopulated;
    /**< Flag to verify if the table is populated */
    VSYS_IVA2CHMAP_S ivaMap[VSYS_MAX_NUMHDVICP];
    /**< Structure to assign the video channles to all 3 IVA-HDs */
} VSYS_IVA2CHMAP_TBL_S;

/**
   \brief Data structure storing the exception context of M3
*/
typedef struct VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S {
    UInt8 threadType;   /**< Thread Type */
    Ptr threadHandle;   /**< Thread Handle */
    Ptr threadStack;    /**< Stack pointer */
    SizeT threadStackSize; /**< Stack Size */
    Ptr r0; /**< R0 Register value when exception occurred */
    Ptr r1; /**< R1 Register value when exception occurred */
    Ptr r2; /**< R2 Register value when exception occurred */
    Ptr r3; /**< R3 Register value when exception occurred */
    Ptr r4; /**< R4 Register value when exception occurred */
    Ptr r5; /**< R5 Register value when exception occurred */
    Ptr r6; /**< R6 Register value when exception occurred */
    Ptr r7; /**< R7 Register value when exception occurred */
    Ptr r8; /**< R8 Register value when exception occurred */
    Ptr r9; /**< R9 Register value when exception occurred */
    Ptr r10; /**< R10 Register value when exception occurred */
    Ptr r11; /**< R11 Register value when exception occurred */
    Ptr r12; /**< R12 Register value when exception occurred */
    Ptr sp; /**< Stack pointer Register value when exception occurred */
    Ptr lr; /**< LR  Register value when exception occurred */
    Ptr pc; /**< Program Counter value when exception occurred */
    Ptr psr; /**< psr register value when exception occurred */
    Ptr ICSR; /**< ICSR register value when exception occurred */
    Ptr MMFSR; /**< MMFSR register value when exception occurred */
    Ptr BFSR; /**< BFSR register value when exception occurred */
    Ptr UFSR; /**< UFSR register value when exception occurred */
    Ptr HFSR; /**< HFSR register value when exception occurred */
    Ptr DFSR; /**< DFSR register value when exception occurred */
    Ptr MMAR; /**< MMAR register value when exception occurred */
    Ptr BFAR; /**< BFAR register value when exception occurred */
    Ptr AFSR; /**< AFSR register value when exception occurred */
} VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S;

/**
   \brief Data structure storing the exception context 
*/
typedef struct VSYS_SLAVE_CORE_EXCEPTION_INFO_S {
    UInt32 exceptionActive; /**<  exception active flag */
    char  excTaskName[VSYS_EXCEPTION_TSK_NAME_LENGTH]; /**<  exception task name */
    char  excCoreName[VSYS_EXCEPTION_CORE_NAME_LENGTH]; /**<  exception core name */
    char  excSiteInfo[VSYS_EXCEPTION_SITE_NAME_LENGTH]; /**<  exception site information */
    char  excInfo[VSYS_EXCEPTION_INFO_LENGTH]; /**<  exception information */
    VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S  excContextM3; /**<  M3 core exception context */
    UInt8 excStack[VSYS_EXCEPTION_TSK_STACK_SIZE]; /**<  exception task stack */
} VSYS_SLAVE_CORE_EXCEPTION_INFO_S;
/*@}*/

#ifdef  __cplusplus
}
#endif
#endif /* __TI_VSYS_COMMON_DEF_H__ */

/*@}*/

