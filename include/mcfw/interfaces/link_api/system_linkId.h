/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup SYSTEM_LINK_API
    \defgroup SYSTEM_LINK_ID  System Link ID

    The Link ID for the links present in the system are defined here 

    @{
*/

/**
    \file system_linkId.h
    \brief  System Link ID's
*/

#ifndef _SYSTEM_LINK_ID_H_
#define _SYSTEM_LINK_ID_H_

#ifdef __cplusplus
extern "C" {
#endif 


/* Define's */

/* @{ */

#define SYSTEM_PROC_DSP         (0) 
/**< DSP Proc ID */

#define SYSTEM_PROC_M3VIDEO     (1)
/**< Video M3 Proc ID */

#define SYSTEM_PROC_M3VPSS      (2)
/**< VPSS M3 Proc ID */

#define SYSTEM_PROC_HOSTA8      (3)
/**< A8 Proc ID */

#define SYSTEM_PROC_MAX         (4)
/**< Max supported processors */

#define SYSTEM_PROC_INVALID     (0xFFFF)
/**< Invalid proc Id, if received indicates some corruption */

#define SYSTEM_MAKE_LINK_ID(p, x) (((p) <<28) | ((x) & 0x0FFFFFFF))
/**< Create link id which indicates the link & processor in which it resides */

#define SYSTEM_GET_LINK_ID(x)     ((x) & 0x0FFFFFFF)
/**< Get the link id - strip off proc id */

#define SYSTEM_GET_PROC_ID(x)     (((x) & ~0x0FFFFFFF)>>28)
/**< Get the proc id - strip off link id */

#define VPSS_LINK(x)            SYSTEM_MAKE_LINK_ID(SYSTEM_PROC_M3VPSS , (x))
/**< VPSS-M3 System Link - used for non-link specific proc level communication */

#define VIDEO_LINK(x)           SYSTEM_MAKE_LINK_ID(SYSTEM_PROC_M3VIDEO, (x))
/**< Video-M3 System Link - used for non-link specific proc level communication */

#define DSP_LINK(x)             SYSTEM_MAKE_LINK_ID(SYSTEM_PROC_DSP    , (x))
/**< DSP System Link - used for non-link specific proc level communication */

#define HOST_LINK(x)            SYSTEM_MAKE_LINK_ID(SYSTEM_PROC_HOSTA8 , (x))
/**< A8 Host Link - used for non-link specific proc level communication */



/**< link ID's */
#define SYSTEM_LINK_ID_M3VPSS               VPSS_LINK(SYSTEM_LINK_ID_MAX-1)
/**< See \ref  VPSS_LINK */

#define SYSTEM_LINK_ID_M3VIDEO              VIDEO_LINK(SYSTEM_LINK_ID_MAX-1)
/**< See \ref  VIDEO_LINK */

#define SYSTEM_LINK_ID_DSP                  DSP_LINK(SYSTEM_LINK_ID_MAX-1)
/**< See \ref  DSP_LINK */

#define SYSTEM_LINK_ID_HOST                 HOST_LINK(SYSTEM_LINK_ID_MAX-1)
/**< See \ref  HOST_LINK */

/** 
    \brief Common Link Ids. These identifiers are not valid directly. 
*/
typedef enum 
{
    SYSTEM_LINK_ID_IPC_OUT_M3_0 = 0,
    /**< IPC Output Link Id  in M3 core - used to xfr data across processors : 0*/

    SYSTEM_LINK_ID_IPC_OUT_M3_1,
    /**< IPC Output Link Id  in M3 core - used to xfr data across processors : 1*/

    SYSTEM_LINK_ID_IPC_IN_M3_0,
    /**< IPC Input Link Id  in M3 core - used to xfr data across processors : 2*/

    SYSTEM_LINK_ID_IPC_IN_M3_1,
    /**< IPC Input Link Id  in M3 core - used to xfr data across processors : 3*/

    SYSTEM_LINK_ID_NULL_0,
    /**< Null Link - Can be used as a tap point to verify various sub-chains. Doesnt do any processing : 4*/

    SYSTEM_LINK_ID_NULL_1,
    /**< Null Link - Can be used as a tap point to verify various sub-chains. Doesnt do any processing : 5*/

    SYSTEM_LINK_ID_NULL_2,
    /**< Null Link - Can be used as a tap point to verify various sub-chains. Doesnt do any processing : 6*/

    SYSTEM_LINK_ID_NULL_SRC_0,
    /**< Null source link - can be used as a source link providing dummy data : 7*/

    SYSTEM_LINK_ID_NULL_SRC_1,
    /**< Null source link - can be used as a source link providing dummy data : 8*/

    SYSTEM_LINK_ID_NULL_SRC_2,
    /**< Null source link - can be used as a source link providing dummy data : 9*/

    SYSTEM_LINK_ID_DUP_0,
    /**< Dup Link - Duplicate frames and provides multiple outputs : 10*/

    SYSTEM_LINK_ID_DUP_1,
    /**< Dup Link - Duplicate frames and provides multiple outputs : 11*/

    SYSTEM_LINK_ID_DUP_2,
    /**< Dup Link - Duplicate frames and provides multiple outputs : 12*/

    SYSTEM_LINK_ID_DUP_3,
    /**< Dup Link - Duplicate frames and provides multiple outputs : 13*/

    SYSTEM_LINK_ID_MERGE_0,
    /**< Merge Link - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues : 14*/

    SYSTEM_LINK_ID_MERGE_1,
    /**< Merge Link - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues : 15*/

    SYSTEM_LINK_ID_MERGE_2,
    /**< Merge Link - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues : 16*/

    SYSTEM_LINK_ID_MERGE_3,
    /**< Merge Link - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues : 17*/

    SYSTEM_LINK_ID_MERGE_4,
    /**< Merge Link - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues : 18*/

    SYSTEM_LINK_ID_IPC_FRAMES_OUT_0,
    /**< Frames Out Link - Enables processing of frames in a different processor synchronously : 19*/

    SYSTEM_LINK_ID_IPC_FRAMES_OUT_1,
    /**< Frames Out Link - Enables processing of frames in a different processor synchronously : 20*/

    SYSTEM_LINK_ID_IPC_FRAMES_OUT_2,
    /**< Frames Out Link - Enables processing of frames in a different processor synchronously : 21*/

    SYSTEM_LINK_ID_IPC_FRAMES_IN_0,
    /**< Frames In Link - Enables processing of frames in a different processor synchronously : 22*/

    SYSTEM_LINK_ID_IPC_FRAMES_IN_1,
    /**< Frames In Link - Enables processing of frames in a different processor synchronously : 23*/

    SYSTEM_LINK_ID_IPC_BITS_OUT_0,
    /**< IPC Bits Out - Used for bit buf passing between host & other processors : 24*/

    SYSTEM_LINK_ID_IPC_BITS_OUT_1,
    /**< IPC Bits Out - Used for bit buf passing between host & other processors : 25*/

    SYSTEM_LINK_ID_IPC_BITS_IN_0,
    /**< IPC Bits In - Used for bit buf passing between host & other processors  : 26*/

    SYSTEM_LINK_ID_IPC_BITS_IN_1,
    /**< IPC Bits In - Used for bit buf passing between host & other processors  : 27*/

    SYSTEM_LINK_ID_SELECT_0,
    /**< Select Link enables configurable mapping of specific channels to be sent out in multiple queues : 28*/

    SYSTEM_LINK_ID_SELECT_1,
    /**< Select Link enables configurable mapping of specific channels to be sent out in multiple queues : 29*/

    SYSTEM_LINK_ID_SELECT_2,
    /**< Select Link enables configurable mapping of specific channels to be sent out in multiple queues : 30*/

    SYSTEM_LINK_ID_SELECT_3,
    /**< Select Link enables configurable mapping of specific channels to be sent out in multiple queues : 31*/

    SYSTEM_LINK_COMMON_LINKS_MAX_ID
    /**< Common Links - Max Id */
} SYSTEM_LINK_IDS_COMMON;


/**
    \brief Link Id numbering using in VPSS. These identifiers are not valid directly. 
 */
typedef  enum
{
    SYSTEM_VPSS_LINK_ID_CAPTURE   = (SYSTEM_LINK_COMMON_LINKS_MAX_ID+1), 

    SYSTEM_VPSS_LINK_ID_NSF_0,
    SYSTEM_VPSS_LINK_ID_NSF_1,
    SYSTEM_VPSS_LINK_ID_NSF_2,
    SYSTEM_VPSS_LINK_ID_NSF_3,

    SYSTEM_VPSS_LINK_ID_DEI_HQ_0,
    SYSTEM_VPSS_LINK_ID_DEI_HQ_1,

    SYSTEM_VPSS_LINK_ID_DEI_0,
    SYSTEM_VPSS_LINK_ID_DEI_1, 

    SYSTEM_VPSS_LINK_ID_DISPLAY_0, 
    SYSTEM_VPSS_LINK_ID_DISPLAY_1, 
    SYSTEM_VPSS_LINK_ID_DISPLAY_2, 

    SYSTEM_VPSS_LINK_ID_GRPX_0, 
    SYSTEM_VPSS_LINK_ID_GRPX_1, 
    SYSTEM_VPSS_LINK_ID_GRPX_2, 

    SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_0,
    SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_1,
    SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_2,
    SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_3,

    SYSTEM_VPSS_LINK_ID_SCLR_INST_0,
    SYSTEM_VPSS_LINK_ID_SCLR_INST_1,

    SYSTEM_VPSS_LINK_ID_AVSYNC,

    SYSTEM_VPSS_LINK_ID_MP_SCLR_INST_0,
    SYSTEM_VPSS_LINK_ID_MP_SCLR_INST_1

} SYSTEM_VPSS_PROC_LINK_IDS;


/**
    \brief Links Ids supported in VPSS. These are valid Link Ids available in VPSS-M3.
             Few of the links create multiple instances with unique identifier - like _0, _1, etc
 */
typedef  enum
{
    SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0  = VPSS_LINK(SYSTEM_LINK_ID_IPC_OUT_M3_0),
     /**< IPC Output Link Id  in VPSS-M3 core - used to xfr data across processors */
    SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_1  = VPSS_LINK(SYSTEM_LINK_ID_IPC_OUT_M3_1),
    /**< IPC Output Link Id  in VPSS-M3 core - used to xfr data across processors */

    SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0   = VPSS_LINK(SYSTEM_LINK_ID_IPC_IN_M3_0),
    /**< IPC Input Link Id  in VPSS-M3 core - used to xfr data across processors */
    SYSTEM_VPSS_LINK_ID_IPC_IN_M3_1   = VPSS_LINK(SYSTEM_LINK_ID_IPC_IN_M3_1),
    /**< IPC Input Link Id  in VPSS-M3 core - used to xfr data across processors */

    SYSTEM_VPSS_LINK_ID_NULL_0          = VPSS_LINK(SYSTEM_LINK_ID_NULL_0),
    /**< Null Link in VPSS-M3 - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_VPSS_LINK_ID_NULL_1          = VPSS_LINK(SYSTEM_LINK_ID_NULL_1),
    /**< Null Link in VPSS-M3 - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_VPSS_LINK_ID_NULL_2          = VPSS_LINK(SYSTEM_LINK_ID_NULL_2),
    /**< Null Link in VPSS-M3 - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */

    SYSTEM_VPSS_LINK_ID_NULL_SRC_0      = VPSS_LINK(SYSTEM_LINK_ID_NULL_SRC_0),
    /**< Null source link in VPSS-M3 - can be used as a source link providing dummy data */
    SYSTEM_VPSS_LINK_ID_NULL_SRC_1      = VPSS_LINK(SYSTEM_LINK_ID_NULL_SRC_1),
    /**< Null source link in VPSS-M3 - can be used as a source link providing dummy data */
    SYSTEM_VPSS_LINK_ID_NULL_SRC_2      = VPSS_LINK(SYSTEM_LINK_ID_NULL_SRC_2),
    /**< Null source link in VPSS-M3 - can be used as a source link providing dummy data */

    SYSTEM_VPSS_LINK_ID_DUP_0           = VPSS_LINK(SYSTEM_LINK_ID_DUP_0),
    /**< Dup Link in VPSS-M3 - Duplicate frames and provides multiple outputs */
    SYSTEM_VPSS_LINK_ID_DUP_1           = VPSS_LINK(SYSTEM_LINK_ID_DUP_1),
    /**< Dup Link in VPSS-M3 - Duplicate frames and provides multiple outputs */
    SYSTEM_VPSS_LINK_ID_DUP_2           = VPSS_LINK(SYSTEM_LINK_ID_DUP_2),
    /**< Dup Link in VPSS-M3 - Duplicate frames and provides multiple outputs */
    SYSTEM_VPSS_LINK_ID_DUP_3           = VPSS_LINK(SYSTEM_LINK_ID_DUP_3),
    /**< Dup Link in VPSS-M3 - Duplicate frames and provides multiple outputs */

    SYSTEM_VPSS_LINK_ID_MERGE_0         = VPSS_LINK(SYSTEM_LINK_ID_MERGE_0),
    /**< Merge Link in VPSS-M3 - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VPSS_LINK_ID_MERGE_1         = VPSS_LINK(SYSTEM_LINK_ID_MERGE_1),
    /**< Merge Link in VPSS-M3 - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VPSS_LINK_ID_MERGE_2         = VPSS_LINK(SYSTEM_LINK_ID_MERGE_2),
    /**< Merge Link in VPSS-M3 - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VPSS_LINK_ID_MERGE_3         = VPSS_LINK(SYSTEM_LINK_ID_MERGE_3),
    /**< Merge Link in VPSS-M3 - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VPSS_LINK_ID_MERGE_4         = VPSS_LINK(SYSTEM_LINK_ID_MERGE_4),
    /**< Merge Link in VPSS-M3 - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */

    SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_0  = VPSS_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_0),
    /**< Frames In Link in VPSS-M3- Enables processing of frames in a different processor synchronously */
    SYSTEM_VPSS_LINK_ID_IPC_FRAMES_IN_1  = VPSS_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_1),
    /**< Frames In Link in VPSS-M3- Enables processing of frames in a different processor synchronously */

    SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0  = VPSS_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_0),
    /**< Frames Out Link in VPSS-M3- Enables processing of frames in a different processor synchronously */
    SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1  = VPSS_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_1),
    /**< Frames Out Link in VPSS-M3- Enables processing of frames in a different processor synchronously */
    SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_2  = VPSS_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_2),
    /**< Frames Out Link in VPSS-M3- Enables processing of frames in a different processor synchronously */

    SYSTEM_VPSS_LINK_ID_SELECT_0          = VPSS_LINK(SYSTEM_LINK_ID_SELECT_0),
    /**< Select Link in VPSS-M3 - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VPSS_LINK_ID_SELECT_1          = VPSS_LINK(SYSTEM_LINK_ID_SELECT_1),
    /**< Select Link in VPSS-M3 - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VPSS_LINK_ID_SELECT_2          = VPSS_LINK(SYSTEM_LINK_ID_SELECT_2),
    /**< Select Link in VPSS-M3 - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VPSS_LINK_ID_SELECT_3          = VPSS_LINK(SYSTEM_LINK_ID_SELECT_3),
    /**< Select Link in VPSS-M3 - enables configurable mapping of specific channels to be sent out in multiple queues */

    SYSTEM_LINK_ID_CAPTURE              = VPSS_LINK(SYSTEM_VPSS_LINK_ID_CAPTURE),
    /**< Capture link. Present in VPSS-M3 */

    SYSTEM_LINK_ID_NSF_0                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_NSF_0),
    /**< Noise filter link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_NSF_1                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_NSF_1),
    /**< Noise filter link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_NSF_2                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_NSF_2),
    /**< Noise filter link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_NSF_3                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_NSF_3),
    /**< Noise filter link. Present in VPSS-M3 */

    SYSTEM_LINK_ID_DEI_HQ_0             = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DEI_HQ_0),
    /**< De-Interlacer - High Quality link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_DEI_HQ_1             = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DEI_HQ_1),
    /**< De-Interlacer - High Quality link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_DEI_0                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DEI_0),
    /**< De-Interlacer link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_DEI_1                = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DEI_1),
    /**< De-Interlacer link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_DISPLAY_0            = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DISPLAY_0),
    /**< Display link - enables one of the outputs like HDTV, SDTV etc. Present in VPSS-M3 */
    SYSTEM_LINK_ID_DISPLAY_1            = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DISPLAY_1),
    /**< Display link - enables one of the outputs like HDTV, SDTV etc. Present in VPSS-M3 */
    SYSTEM_LINK_ID_DISPLAY_2            = VPSS_LINK(SYSTEM_VPSS_LINK_ID_DISPLAY_2),
    /**< Display link - enables one of the outputs like HDTV, SDTV etc. Present in VPSS-M3 */
   
    SYSTEM_LINK_ID_GRPX_0               = VPSS_LINK(SYSTEM_VPSS_LINK_ID_GRPX_0),
    /**< Graphics Link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_GRPX_1               = VPSS_LINK(SYSTEM_VPSS_LINK_ID_GRPX_1),
    /**< Graphics Link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_GRPX_2               = VPSS_LINK(SYSTEM_VPSS_LINK_ID_GRPX_2),
    /**< Graphics Link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_SW_MS_MULTI_INST_0   = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_0),
    /**< Software Mosaic Link - supports multiple driver instances in one link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_SW_MS_MULTI_INST_1   = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_1),
    /**< Software Mosaic Link - supports multiple driver instances in one link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_SW_MS_MULTI_INST_2   = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_2),
    /**< Software Mosaic Link - supports multiple driver instances in one link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_SW_MS_MULTI_INST_3   = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SW_MS_MULTI_INST_3),
    /**< Software Mosaic Link - supports multiple driver instances in one link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_SCLR_INST_0          = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SCLR_INST_0),
    /**< Scalar Link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_SCLR_INST_1          = VPSS_LINK(SYSTEM_VPSS_LINK_ID_SCLR_INST_1),
    /**< Scalar Link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_AVSYNC               = VPSS_LINK(SYSTEM_VPSS_LINK_ID_AVSYNC),
    /**< AVSync Link. Present in VPSS-M3 */
    
    SYSTEM_LINK_ID_MP_SCLR_INST_0       = VPSS_LINK(SYSTEM_VPSS_LINK_ID_MP_SCLR_INST_0),
    /**< Mega Pixel Scalar Link. Present in VPSS-M3 */
    SYSTEM_LINK_ID_MP_SCLR_INST_1       = VPSS_LINK(SYSTEM_VPSS_LINK_ID_MP_SCLR_INST_1)
   /**< Mega Pixel Scalar Link. Present in VPSS-M3 */
} SYSTEM_VPSS_LINK_IDS;


/** Link Ids validation */
#define SYSTEM_LINK_ID_DISPLAY_FIRST        (SYSTEM_LINK_ID_DISPLAY_0)
#define SYSTEM_LINK_ID_DISPLAY_LAST         (SYSTEM_LINK_ID_DISPLAY_2)
#define SYSTEM_LINK_ID_DISPLAY_COUNT        ((SYSTEM_LINK_ID_DISPLAY_LAST - \
                                              SYSTEM_LINK_ID_DISPLAY_FIRST) + 1)
#define SYSTEM_LINK_ID_SW_MS_START          (SYSTEM_LINK_ID_SW_MS_MULTI_INST_0)
#define SYSTEM_LINK_ID_SW_MS_END            (SYSTEM_LINK_ID_SW_MS_MULTI_INST_3)
#define SYSTEM_LINK_ID_SW_MS_COUNT          (SYSTEM_LINK_ID_SW_MS_END -        \

#define SYSTEM_LINK_ID_SCLR_START           (SYSTEM_LINK_ID_SCLR_INST_0)
#define SYSTEM_LINK_ID_SCLR_END             (SYSTEM_LINK_ID_SCLR_INST_1)
#define SYSTEM_LINK_ID_SCLR_COUNT           (SYSTEM_LINK_ID_SCLR_END -         \
                                             SYSTEM_LINK_ID_SCLR_START) + 1

#define SYSTEM_LINK_ID_MP_SCLR_START        (SYSTEM_LINK_ID_MP_SCLR_INST_0)
#define SYSTEM_LINK_ID_MP_SCLR_END          (SYSTEM_LINK_ID_MP_SCLR_INST_1)
#define SYSTEM_LINK_ID_MP_SCLR_COUNT        (SYSTEM_LINK_ID_SCLR_END -         \
                                             SYSTEM_LINK_ID_SCLR_START) + 1

/** Video Related */
/**
    \brief Link Id numbering using in Video. These identifiers are not valid directly. 
 */
typedef enum
{
   SYSTEM_VIDEO_LINK_ID_VENC_0 = SYSTEM_LINK_COMMON_LINKS_MAX_ID+1,

   SYSTEM_VIDEO_LINK_ID_VDEC_0,

   SYSTEM_VIDEO_LINK_ID_VIDEO_ALG_0,
   SYSTEM_VIDEO_LINK_ID_VIDEO_ALG_1,

} SYSTEM_VIDEO_PROC_LINK_IDS;


/**
    \brief Links Ids supported in Video. These are valid Link Ids available in Video-M3.
             Few of the links create multiple instances with unique identifier - like _0, _1, etc
 */
typedef enum
{
    SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0    = VIDEO_LINK(SYSTEM_LINK_ID_IPC_OUT_M3_0),
    /**< IPC Output Link Id  in Video -M3 core - used to xfr data across M3 processors */
    SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_1    = VIDEO_LINK(SYSTEM_LINK_ID_IPC_OUT_M3_1),
    /**< IPC Output Link Id  in Video -M3 core - used to xfr data across M3 processors */

    SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0     = VIDEO_LINK(SYSTEM_LINK_ID_IPC_IN_M3_0),
    /**< IPC Input Link Id  in Video -M3 core - used to xfr data across M3 processors */
    SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_1     = VIDEO_LINK(SYSTEM_LINK_ID_IPC_IN_M3_1),
    /**< IPC Input Link Id  in Video -M3 core - used to xfr data across M3 processors */

    SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_BITS_IN_0),
    /**< IPC Bits In in Video-M3 core - Used for bit buf passing between A8 Host & Video-M3 */
    SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_1   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_BITS_IN_1),
    /**< IPC Bits In in Video-M3 core - Used for bit buf passing between A8 Host & Video-M3   */

    SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_0),
    /**< IPC Bits Out in Video-M3 core - Used for bit buf passing between host & Video-M3 */
    SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_1   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_1),
    /**< IPC Bits Out in Video-M3 core - Used for bit buf passing between host & Video-M3 */

    SYSTEM_VIDEO_LINK_ID_NULL_0          = VIDEO_LINK(SYSTEM_LINK_ID_NULL_0),
    /**< Null Link in Video-M3 core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_VIDEO_LINK_ID_NULL_1          = VIDEO_LINK(SYSTEM_LINK_ID_NULL_1),
    /**< Null Link in Video-M3 core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_VIDEO_LINK_ID_NULL_2          = VIDEO_LINK(SYSTEM_LINK_ID_NULL_2),
    /**< Null Link in Video-M3 core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    
    SYSTEM_VIDEO_LINK_ID_NULL_SRC_0      = VIDEO_LINK(SYSTEM_LINK_ID_NULL_SRC_0),
    /**< Null source link in Video-M3 core - can be used as a source link providing dummy data */
    SYSTEM_VIDEO_LINK_ID_NULL_SRC_1      = VIDEO_LINK(SYSTEM_LINK_ID_NULL_SRC_1),
    /**< Null source link in Video-M3 core - can be used as a source link providing dummy data */
    SYSTEM_VIDEO_LINK_ID_NULL_SRC_2      = VIDEO_LINK(SYSTEM_LINK_ID_NULL_SRC_2),
    /**< Null source link in Video-M3 core - can be used as a source link providing dummy data */

    SYSTEM_VIDEO_LINK_ID_DUP_0           = VIDEO_LINK(SYSTEM_LINK_ID_DUP_0),
    /**< Dup Link in Video-M3 core - Duplicate frames and provides multiple outputs */
    SYSTEM_VIDEO_LINK_ID_DUP_1           = VIDEO_LINK(SYSTEM_LINK_ID_DUP_1),
    /**< Dup Link in Video-M3 core - Duplicate frames and provides multiple outputs */
    SYSTEM_VIDEO_LINK_ID_DUP_2           = VIDEO_LINK(SYSTEM_LINK_ID_DUP_2),
    /**< Dup Link in Video-M3 core - Duplicate frames and provides multiple outputs */
    SYSTEM_VIDEO_LINK_ID_DUP_3           = VIDEO_LINK(SYSTEM_LINK_ID_DUP_3),
    /**< Dup Link in Video-M3 core - Duplicate frames and provides multiple outputs */

    SYSTEM_VIDEO_LINK_ID_MERGE_0         = VIDEO_LINK(SYSTEM_LINK_ID_MERGE_0),
    /**< Merge Link in Video-M3 core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VIDEO_LINK_ID_MERGE_1         = VIDEO_LINK(SYSTEM_LINK_ID_MERGE_1),
    /**< Merge Link in Video-M3 core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VIDEO_LINK_ID_MERGE_2         = VIDEO_LINK(SYSTEM_LINK_ID_MERGE_2),
    /**< Merge Link in Video-M3 core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VIDEO_LINK_ID_MERGE_3         = VIDEO_LINK(SYSTEM_LINK_ID_MERGE_3),
    /**< Merge Link in Video-M3 core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_VIDEO_LINK_ID_MERGE_4         = VIDEO_LINK(SYSTEM_LINK_ID_MERGE_4),
    /**< Merge Link in Video-M3 core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    
    SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_IN_0   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_0),
    /**< Frames In Link in Video-M3 core - Enables processing of frames in a different processor synchronously */
    SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_IN_1   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_1),
    /**< Frames In Link in Video-M3 core - Enables processing of frames in a different processor synchronously */

    SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_OUT_0   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_0),
    /**< Frames Out Link in Video-M3 core - Enables processing of frames in a different processor synchronously */
    SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_OUT_1   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_1),
    /**< Frames Out Link in Video-M3 core - Enables processing of frames in a different processor synchronously */
    SYSTEM_VIDEO_LINK_ID_IPC_FRAMES_OUT_2   = VIDEO_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_2),
    /**< Frames Out Link in Video-M3 core - Enables processing of frames in a different processor synchronously */
        
    SYSTEM_VIDEO_LINK_ID_SELECT_0           = VIDEO_LINK(SYSTEM_LINK_ID_SELECT_0),
    /**< Select Link  in Video-M3 core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VIDEO_LINK_ID_SELECT_1           = VIDEO_LINK(SYSTEM_LINK_ID_SELECT_1),
    /**< Select Link  in Video-M3 core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VIDEO_LINK_ID_SELECT_2           = VIDEO_LINK(SYSTEM_LINK_ID_SELECT_2),
    /**< Select Link  in Video-M3 core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_VIDEO_LINK_ID_SELECT_3           = VIDEO_LINK(SYSTEM_LINK_ID_SELECT_3),
    /**< Select Link  in Video-M3 core - enables configurable mapping of specific channels to be sent out in multiple queues */
    
    SYSTEM_LINK_ID_VENC_0                = VIDEO_LINK(SYSTEM_VIDEO_LINK_ID_VENC_0),
    /**< Video Encoder Link */

    SYSTEM_LINK_ID_VDEC_0                = VIDEO_LINK(SYSTEM_VIDEO_LINK_ID_VDEC_0),
    /**< Video Decoder Link */

    SYSTEM_LINK_ID_VIDEO_ALG_0           = VIDEO_LINK(SYSTEM_VIDEO_LINK_ID_VIDEO_ALG_0),
    /**< Algorithm Link  in Video core - enables combination of some video or image processing algorithms to be applied on video frames */
    SYSTEM_LINK_ID_VIDEO_ALG_1           = VIDEO_LINK(SYSTEM_VIDEO_LINK_ID_VIDEO_ALG_1),
    /**< Algorithm Link  in Video core - enables combination of some video or image processing algorithms to be applied on video frames */
} SYSTEM_VIDEO_LINK_IDS;


#define SYSTEM_LINK_ID_VENC_START           (SYSTEM_LINK_ID_VENC_0)
#define SYSTEM_LINK_ID_VENC_END             (SYSTEM_LINK_ID_VENC_0)
#define SYSTEM_LINK_ID_VENC_COUNT           (((SYSTEM_LINK_ID_VENC_END) -         \
                                             (SYSTEM_LINK_ID_VENC_START)) + 1)

#define SYSTEM_LINK_ID_VDEC_START           (SYSTEM_LINK_ID_VDEC_0)
#define SYSTEM_LINK_ID_VDEC_END             (SYSTEM_LINK_ID_VDEC_0)
#define SYSTEM_LINK_ID_VDEC_COUNT           (((SYSTEM_LINK_ID_VDEC_END) -         \
                                             (SYSTEM_LINK_ID_VDEC_START)) + 1)

/** DSP Links */

/**
    \brief Link Id numbering using in DSP. These identifiers are not valid directly. 
 */
typedef enum
{
    SYSTEM_DSP_LINK_ID_ALG_0 = SYSTEM_LINK_COMMON_LINKS_MAX_ID+1,
    /**< Algorithm Link  */
    SYSTEM_DSP_LINK_ID_ALG_1
    /**< Algorithm Link  */

} SYSTEM_DSP_PROC_LINK_IDS;

/**
    \brief Links Ids supported in DSP. These are valid Link Ids available in DSP.
             Few of the links create multiple instances with unique identifier - like _0, _1, etc
 */
typedef enum
{
    SYSTEM_DSP_LINK_ID_NULL_0          = DSP_LINK(SYSTEM_LINK_ID_NULL_0),
    /**< Null Link in DSP core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_DSP_LINK_ID_NULL_1          = DSP_LINK(SYSTEM_LINK_ID_NULL_1),
    /**< Null Link in DSP core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    SYSTEM_DSP_LINK_ID_NULL_2          = DSP_LINK(SYSTEM_LINK_ID_NULL_2),
    /**< Null Link in DSP core - Can be used as a tap point to verify various sub-chains. Doesnt do any processing */
    
    SYSTEM_DSP_LINK_ID_NULL_SRC_0      = DSP_LINK(SYSTEM_LINK_ID_NULL_SRC_0),
    /**< Null source link in DSP core - can be used as a source link providing dummy data */
    SYSTEM_DSP_LINK_ID_NULL_SRC_1      = DSP_LINK(SYSTEM_LINK_ID_NULL_SRC_1),
        /**< Null source link in DSP core - can be used as a source link providing dummy data */
    SYSTEM_DSP_LINK_ID_NULL_SRC_2      = DSP_LINK(SYSTEM_LINK_ID_NULL_SRC_2),
    /**< Null source link in DSP core - can be used as a source link providing dummy data */
    
    SYSTEM_DSP_LINK_ID_DUP_0           = DSP_LINK(SYSTEM_LINK_ID_DUP_0),
    /**< Dup Link in DSP core - Duplicate frames and provides multiple outputs */
    SYSTEM_DSP_LINK_ID_DUP_1           = DSP_LINK(SYSTEM_LINK_ID_DUP_1),
    /**< Dup Link in DSP core - Duplicate frames and provides multiple outputs */
    SYSTEM_DSP_LINK_ID_DUP_2           = DSP_LINK(SYSTEM_LINK_ID_DUP_2),
    /**< Dup Link in DSP core - Duplicate frames and provides multiple outputs */
    SYSTEM_DSP_LINK_ID_DUP_3           = DSP_LINK(SYSTEM_LINK_ID_DUP_3),
    /**< Dup Link in DSP core - Duplicate frames and provides multiple outputs */
    
    SYSTEM_DSP_LINK_ID_MERGE_0         = DSP_LINK(SYSTEM_LINK_ID_MERGE_0),
    /**< Merge Link in DSP  core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_DSP_LINK_ID_MERGE_1         = DSP_LINK(SYSTEM_LINK_ID_MERGE_1),
    /**< Merge Link in DSP  core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_DSP_LINK_ID_MERGE_2         = DSP_LINK(SYSTEM_LINK_ID_MERGE_2),
    /**< Merge Link in DSP  core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_DSP_LINK_ID_MERGE_3         = DSP_LINK(SYSTEM_LINK_ID_MERGE_3),
    /**< Merge Link in DSP  core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */
    SYSTEM_DSP_LINK_ID_MERGE_4         = DSP_LINK(SYSTEM_LINK_ID_MERGE_4),
    /**< Merge Link in DSP  core - Merge different input queue frames & provide them as single output source. Channel numbering is sequential wrt to input queues */

    SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0  = DSP_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_0),
    /**< Frames In Link in DSP  core - Enables processing of frames in a different processor synchronously */
    SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1  = DSP_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_1),
    /**< Frames In Link in DSP  core - Enables processing of frames in a different processor synchronously */
    
    SYSTEM_DSP_LINK_ID_IPC_FRAMES_OUT_0  = DSP_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_0),
    /**< Frames Out Link in DSP  core - Enables processing of frames in a different processor synchronously */
    SYSTEM_DSP_LINK_ID_IPC_FRAMES_OUT_1  = DSP_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_1),
    /**< Frames Out Link in DSP  core - Enables processing of frames in a different processor synchronously */
    SYSTEM_DSP_LINK_ID_IPC_FRAMES_OUT_2  = DSP_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_2),
    /**< Frames Out Link in DSP  core - Enables processing of frames in a different processor synchronously */
    
    SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_0  = DSP_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_0),
    /**< IPC Bits Out in DSP  core - Used for bit buf passing between host & other processors */
    SYSTEM_DSP_LINK_ID_IPC_BITS_OUT_1  = DSP_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_1),
    /**< IPC Bits Out in DSP  core - Used for bit buf passing between host & other processors */
    
    SYSTEM_DSP_LINK_ID_SELECT_0           = DSP_LINK(SYSTEM_LINK_ID_SELECT_0),
    /**< Select Link in DSP core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_DSP_LINK_ID_SELECT_1           = DSP_LINK(SYSTEM_LINK_ID_SELECT_1),
    /**< Select Link in DSP core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_DSP_LINK_ID_SELECT_2           = DSP_LINK(SYSTEM_LINK_ID_SELECT_2),
    /**< Select Link in DSP core - enables configurable mapping of specific channels to be sent out in multiple queues */
    SYSTEM_DSP_LINK_ID_SELECT_3           = DSP_LINK(SYSTEM_LINK_ID_SELECT_3),
    /**< Select Link in DSP core - enables configurable mapping of specific channels to be sent out in multiple queues */

    SYSTEM_LINK_ID_ALG_0                   = DSP_LINK(SYSTEM_DSP_LINK_ID_ALG_0),
    /**< Algorithm Link  in DSP core - enables combination of some video or image processing algorithms to be applied on video frames */
    SYSTEM_LINK_ID_ALG_1                   = DSP_LINK(SYSTEM_DSP_LINK_ID_ALG_1)
    /**< Algorithm Link  in DSP core - enables combination of some video or image processing algorithms to be applied on video frames */
} SYSTEM_DSP_LINK_IDS;



#define SYSTEM_LINK_ID_ALG_START           (SYSTEM_LINK_ID_ALG_0)
#define SYSTEM_LINK_ID_ALG_END             (SYSTEM_LINK_ID_ALG_1)
#define SYSTEM_LINK_ID_ALG_COUNT           (SYSTEM_LINK_ID_ALG_END -         \
                                             SYSTEM_LINK_ID_ALG_START) + 1

/** Host Links */
/**
    \brief Links Ids supported in DSP. These are valid Link Ids available in DSP.
             Few of the links create multiple instances with unique identifier - like _0, _1, etc
 */
typedef enum
{
    SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0   = HOST_LINK(SYSTEM_LINK_ID_IPC_BITS_IN_0),
    /**< IPC Bits In in Host processor - Used for bit buf passing between host & other processors  */
    SYSTEM_HOST_LINK_ID_IPC_BITS_IN_1     = HOST_LINK(SYSTEM_LINK_ID_IPC_BITS_IN_1),
    /**< IPC Bits In in Host processor - Used for bit buf passing between host & other processors  */
    SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0    = HOST_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_0),
    /**< IPC Bits Out in Host processor - Used for bit buf passing between host & other processors  */
    SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_1    = HOST_LINK(SYSTEM_LINK_ID_IPC_BITS_OUT_1),
    /**< IPC Bits Out in Host processor - Used for bit buf passing between host & other processors  */
    SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0   = HOST_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_0),
    /**< Frames In Link of Host Processor - Enables processing of frames in a different processor synchronously */
    SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_1   = HOST_LINK(SYSTEM_LINK_ID_IPC_FRAMES_IN_1),
    /**< Frames In Link of Host Processor - Enables processing of frames in a different processor synchronously */
    SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_0  = HOST_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_0),
    /**< Frames Out Link of Host Processor - Enables processing of frames in a different processor synchronously */
    SYSTEM_HOST_LINK_ID_IPC_FRAMES_OUT_1  = HOST_LINK(SYSTEM_LINK_ID_IPC_FRAMES_OUT_1)
    /**< Frames Out Link of Host Processor - Enables processing of frames in a different processor synchronously */
} SYSTEM_HOST_LINK_IDS;

#define SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_BASE          HOST_LINK(SYSTEM_LINK_COMMON_LINKS_MAX_ID + 1)
#define SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_DSP      (SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_BASE + SYSTEM_PROC_DSP)
#define SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VIDEOM3  (SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_BASE + SYSTEM_PROC_M3VIDEO)
#define SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_FROM_VPSSM3   (SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_BASE + SYSTEM_PROC_M3VPSS)

#define SYSTEM_LINK_ID_MAX                  (64)

#define SYSTEM_LINK_ID_INVALID              (0xFFFFFFFF)

/*@}*/

#ifdef  __cplusplus
}
#endif

#endif

/*@}*/

/**
    \ingroup SYSTEM_LINK_API
    \defgroup SYSTEM_LINK_ID  System Link ID
*/


