/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 ******************************************************************************/

/**
 *******************************************************************************
 *  @file  lib_dvr_app.c
 *  @brief This file contains all Functions related to DVR operation
 *
 *
 *
 *  @rev 1.0
 *******************************************************************************
 */
#define USE_SLOG_PRINT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <sys/mount.h>  //MULTI-HDD SKSUNG

#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fb.h>
//#include <linux/ti81xxfb.h>

#include "ti_media_std.h"

#include "lib_dvr_app.h"
#include "device.h"
#include "app_util.h"
#include "app_writer.h"
#include "app_front_if.h"
#include "app_dio_if.h"
#include "app_ptz_if.h"
#include "app_audio.h"
#include "app_audio_priv.h"
#include "basket_muldec.h"
#include "bsk_tplay.h"

#include "avserver.h"
#include "avserver_api.h"
//#include "Module.h"

#include "ti_vsys.h"
#include "ti_vcap.h"
#include "ti_venc.h"
#include "ti_vdec.h"
#include "ti_vdis.h"
#include "ti_vdis_timings.h"
#include "sockio.h"
#include "remoteplay_priv.h"
//#include "ti_vcap_priv.h"
#define MULTICH_LAYOUT_MODE_1CH         1
#define MULTICH_LAYOUT_MODE_2x2_4CH     2
#define MULTICH_LAYOUT_MODE_3x3_9CH     3
#define MULTICH_LAYOUT_MODE_1x1_8CH     4
#define MULTICH_LAYOUT_MODE_2x2_8CH     5
#define MULTICH_LAYOUT_MODE_4x3_12CH    6
#define MULTICH_LAYOUT_MODE_12CH        7
#define MULTICH_LAYOUT_MODE_14CH        8
#define MULTICH_LAYOUT_MODE_4x4_16CH    9

#define FBDEV_NAME_0            "/dev/fb0"
#define FBDEV_NAME_1            "/dev/fb1"
#define FBDEV_NAME_2            "/dev/fb2"
#define RGB_KEY		0x00FF00FF
#define GRPX_SC_MARGIN_OFFSET   (3)

#undef Mod__MID
#define Mod__MID ((Module_findByName("DVR"))->id)


#define LIVE_START_X    200
#define LIVE_START_Y    40


/* Setting secondary out <CIF> for 15 frames - this is the validated frame rate;
any higher number will impact performance */
#define     CIF_FPS_DEI_NTSC        (30)
#define     CIF_FPS_DEI_PAL         (25)
#define     CIF_FPS_ENC_NTSC 		(30)
#define     CIF_FPS_ENC_PAL         (25)
#define     CIF_BITRATE             (500)


typedef struct ST_MOSAIC_INFO
{
	Int layoutId;
	Int numWin;
	Int isCamPropMode;
	Int selectedCh;
	Int startX;
	Int startY;
	Int width;
	Int height;
	UInt32 chMap[VDIS_MOSAIC_WIN_MAX];
}ST_MOSAIC_INFO;

ST_MOSAIC_INFO gMosaicInfo_Live;
ST_MOSAIC_INFO gMosaicInfo_Play;


AUDIO_STATE gAudio_state ;

ST_CAPTURE_INFO gCaptureInfo;
extern PROFILE gInitSettings;

//static UInt32 gReversDsp;
static UInt32 gLiveDspId;
static UInt32 gPlayDspId;
Char gBuff[100];
int fd0=0, fd1=0, fd2=0;
//BKT_Mount_Info gbtk_mount_info;
AlgLink_OsdChWinParams g_osdChParam[ALG_LINK_OSD_MAX_CH];

static UInt64 get_current_time_to_msec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((UInt64)tv.tv_sec*1000 + tv.tv_usec/1000);
}

static Void Dvr_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height)
{
    switch (outRes)
    {
        case DC_MODE_720P_60:
            *width = 1280;
            *height = 720;
            break;

        case DC_MODE_NTSC:
            *width = 720;
            *height = 480;
            break;

        case DC_MODE_PAL:
            *width = 720;
            *height = 576;
            break;

        case DC_MODE_XGA_60:
           *width = 1024;
           *height = 768;
           break;

        case DC_MODE_SXGA_60:
           *width = 1280;
           *height = 1024;
           break;

		case DC_MODE_480P:
		   *width = 720;
		   *height = 480;
		   break ;

		case DC_MODE_576P:
		   *width = 720;
		   *height = 576;
		   break ;

        default:
        case DC_MODE_1080I_60:
        case DC_MODE_1080P_60:
        case DC_MODE_1080P_30:
		case DC_MODE_1080P_50:
            *width = 1920;
            *height = 1080;
            break;
    }

}


static Void DVR_swMsGenerateLayouts(VDIS_DEV devId, UInt32 startChId, UInt32 maxChns, UInt32 LayoutId,
		VDIS_MOSAIC_S * vdMosaicParam, Bool forceLowCostScaling, UInt32 resolution, UInt32 doSwitch)
{
    UInt32 outWidth, outHeight, uiSx = 0, uiSy = 0, tempWidth;
    UInt32 row, col, rowMax, colMax, widthAlign, heightAlign;
    UInt32 i, winId = 0;

    rowMax = 0;
    colMax = 0;

    widthAlign = 8;
    heightAlign = 1;

    if(devId>=VDIS_DEV_MAX)
        devId = gLiveDspId;


	if(devId == gLiveDspId || devId == gPlayDspId || doSwitch == TRUE)
	{
		switch (resolution)
	    {

	        case DC_MODE_720P_60:

	            uiSx = LIVE_START_X*1280/1920 + 3;//1280;
	            uiSy = LIVE_START_Y*720/1080 ;//720;
	            break;
			case DC_MODE_XGA_60:
			   uiSx = LIVE_START_X*1024/1920 + 2;//1024;
			   uiSy = LIVE_START_Y*768/1080;//768;
			   break;
			case DC_MODE_SXGA_60:
			   uiSx = LIVE_START_X*1280/1920 + 3;//1280;
			   uiSy = LIVE_START_Y*1024/1080;//1024;
			   break;

			case DC_MODE_480P:
			   uiSx = LIVE_START_X*720/1920 -3 ;//720
			   uiSy = LIVE_START_Y*480/1080 ;//480
			   break ;

            case DC_MODE_576P:
               uiSx = LIVE_START_X*720/1920 -3;//720
               uiSy = LIVE_START_Y*576/1080 ;//576
               break ;

            case DC_MODE_1080I_60:
            case DC_MODE_1080P_60:
            case DC_MODE_1080P_30:
            case DC_MODE_1080P_50:
            default:
                uiSx = LIVE_START_X;
                uiSy = LIVE_START_Y;
                break;
        }
    }


    /*if(devId == gInitSettings.layout.iMainOutput)
    {
        gInitSettings.layout.iLayoutMode = LayoutId;
    }
    else if(devId == gInitSettings.layout.iSubOutput)
    {
        gInitSettings.layout.iSecondLayoutMode = LayoutId;
    }
*/
	if(startChId < VDIS_CHN_MAX/2)
	{
		gInitSettings.layout.iLayoutMode = LayoutId;
	}
	else
	{
		gInitSettings.layout.iSecondLayoutMode = LayoutId;
	}
	
	Dvr_swMsGetOutSize(resolution, &outWidth, &outHeight);

    uiSx        = VsysUtils_floor(uiSx, widthAlign) ;
 	uiSy        = VsysUtils_floor(uiSy, heightAlign);

    outWidth    = VsysUtils_floor(outWidth - uiSx, widthAlign);
    outHeight   = VsysUtils_floor(outHeight - uiSy, heightAlign);

    vdMosaicParam->outputFPS = 30;

    if(gCaptureInfo.chStatus[0].frameHeight == 288)
        vdMosaicParam->outputFPS = 25;

    vdMosaicParam->onlyCh2WinMapChanged = FALSE;


    vdMosaicParam->displayWindow.height     = VsysUtils_floor(outHeight, heightAlign);
    vdMosaicParam->displayWindow.width      = VsysUtils_floor(outWidth, widthAlign);
    vdMosaicParam->displayWindow.start_X    = VsysUtils_floor(uiSx, widthAlign);
    vdMosaicParam->displayWindow.start_Y    = VsysUtils_floor(uiSy, heightAlign);

    for (i = 0; i < VDIS_MOSAIC_WIN_MAX; i++)
    {
        vdMosaicParam->chnMap[i] = VDIS_CHN_INVALID;
    }

	
    if(LayoutId ==  MULTICH_LAYOUT_MODE_1CH)
    {
        vdMosaicParam->numberOfWindows          = 1;
        winId = 0;

        vdMosaicParam->winList[winId].width         = outWidth;
        vdMosaicParam->winList[winId].height        = outHeight;
        vdMosaicParam->winList[winId].start_X       = uiSx;
        vdMosaicParam->winList[winId].start_Y       = uiSy;
        vdMosaicParam->chnMap[winId]                = winId;

        if (forceLowCostScaling == TRUE)
            vdMosaicParam->useLowCostScaling[winId] = TRUE;
        else
            vdMosaicParam->useLowCostScaling[winId] = FALSE;
    }


    if( LayoutId ==  MULTICH_LAYOUT_MODE_1x1_8CH)
    {
        vdMosaicParam->numberOfWindows          = 8;

        winId = 0;
		tempWidth = VsysUtils_floor((outWidth/4), widthAlign);

        vdMosaicParam->winList[winId].width         = VsysUtils_floor((tempWidth*3), widthAlign);
        vdMosaicParam->winList[winId].height        = VsysUtils_floor((outHeight*3)/4, heightAlign);
        vdMosaicParam->winList[winId].start_X       = uiSx;
        vdMosaicParam->winList[winId].start_Y       = uiSy;
        vdMosaicParam->chnMap[winId]                = winId;

       if (forceLowCostScaling == TRUE)
            vdMosaicParam->useLowCostScaling[winId] = TRUE;
        else
            vdMosaicParam->useLowCostScaling[winId] = FALSE;



        for(row = 0; row < 3; row++)
        {
            winId = 1+row;

            vdMosaicParam->winList[winId].width         = VsysUtils_floor(outWidth/4, widthAlign);
            vdMosaicParam->winList[winId].height        = VsysUtils_floor(outHeight/4, heightAlign);
            vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[0].width+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);
            vdMosaicParam->chnMap[winId]                = winId;
            vdMosaicParam->useLowCostScaling[winId]     = TRUE;
        }

        for(col = 0; col < 4; col++)
        {
            winId = 4+col;

            vdMosaicParam->winList[winId].width         = VsysUtils_floor(outWidth/4, widthAlign);
            vdMosaicParam->winList[winId].height        = VsysUtils_floor(outHeight/4, heightAlign);
            vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[0].height+uiSy, heightAlign);
            vdMosaicParam->chnMap[winId]                = winId;
            vdMosaicParam->useLowCostScaling[winId]     = TRUE;
        }
    }


    if( LayoutId ==  MULTICH_LAYOUT_MODE_2x2_8CH)
    {
        vdMosaicParam->numberOfWindows = 8;

        /*Larger Windows*/
        for(row = 0; row < 2; row++)
        {
            for(col = 0; col < 2; col++)
            {
                winId = row*2+col;

                vdMosaicParam->winList[winId].width     = VsysUtils_floor((outWidth*2)/5, widthAlign);
                vdMosaicParam->winList[winId].height    = VsysUtils_floor(outHeight/2, heightAlign);;
                vdMosaicParam->winList[winId].start_X   = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx, widthAlign);
                vdMosaicParam->winList[winId].start_Y   = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);
                vdMosaicParam->chnMap[winId]            = winId;

                if (forceLowCostScaling == TRUE)
                    vdMosaicParam->useLowCostScaling[winId] = TRUE;
                else
                    vdMosaicParam->useLowCostScaling[winId] = FALSE;
            }
        }

        /*smaller Windows*/
        for(row = 0; row < 4; row++)
        {
            winId = 4 + row;

            vdMosaicParam->winList[winId].width     = VsysUtils_floor(vdMosaicParam->winList[0].width/2, widthAlign);
            vdMosaicParam->winList[winId].height    = VsysUtils_floor(vdMosaicParam->winList[0].height/2, heightAlign);
            vdMosaicParam->winList[winId].start_X   = VsysUtils_floor(vdMosaicParam->winList[0].width*2+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y   = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);
            vdMosaicParam->chnMap[winId]            = winId;
            vdMosaicParam->useLowCostScaling[winId] = TRUE;
        }
    }


    if ( LayoutId == MULTICH_LAYOUT_MODE_12CH)
    {
        vdMosaicParam->numberOfWindows          = 12;

        /*Larger Windows*/
        winId = 0;

        tempWidth = VsysUtils_floor((outWidth/5), widthAlign);
        vdMosaicParam->winList[winId].width         = VsysUtils_floor((tempWidth*3), widthAlign);

        vdMosaicParam->useLowCostScaling[winId]     = 0;
        //vdMosaicParam->winList[winId].width         = VsysUtils_floor((outWidth*3)/5, widthAlign);
        vdMosaicParam->winList[winId].height        = VsysUtils_floor((outHeight*3)/4, heightAlign);
        vdMosaicParam->winList[winId].start_X       = uiSx;
        vdMosaicParam->winList[winId].start_Y       = uiSy;
        vdMosaicParam->chnMap[winId]                = winId;

        if (forceLowCostScaling == TRUE)
            vdMosaicParam->useLowCostScaling[winId] = TRUE;
        else
            vdMosaicParam->useLowCostScaling[winId] = FALSE;


        /*smaller Windows*/
        for(row = 0; row < 3; row++)
        {
            for(col = 0; col < 2; col++)
            {
                winId = 1+(row*2+col);
                vdMosaicParam->winList[winId].width         = VsysUtils_floor(vdMosaicParam->winList[0].width/3, widthAlign);
                vdMosaicParam->winList[winId].height        = VsysUtils_floor(vdMosaicParam->winList[0].height/3, heightAlign);
                vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[0].width+(vdMosaicParam->winList[winId].width*col)+uiSx, widthAlign);
                vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);
                vdMosaicParam->chnMap[winId]                = winId;
                vdMosaicParam->useLowCostScaling[winId]     = TRUE;
            }
        }

        for(col = 0; col < 5; col++)
        {
            winId = 7 + col;
            vdMosaicParam->winList[winId].width         = VsysUtils_floor(vdMosaicParam->winList[0].width/3, widthAlign);
            vdMosaicParam->winList[winId].height        = VsysUtils_floor(vdMosaicParam->winList[0].height/3, heightAlign);
            vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[0].height+uiSy, heightAlign);
            vdMosaicParam->chnMap[winId]                = winId;
            vdMosaicParam->useLowCostScaling[winId]     = TRUE;
        }
    }


    if ( LayoutId == MULTICH_LAYOUT_MODE_14CH)
    {
        vdMosaicParam->numberOfWindows          = 14;

        /*Larger Windows*/
        for(col = 0; col < 2; col++)
        {
            winId = col;
            tempWidth = VsysUtils_floor((outWidth/5), widthAlign);
            vdMosaicParam->winList[winId].width         = VsysUtils_floor((tempWidth*2), widthAlign);
            vdMosaicParam->winList[winId].height        = VsysUtils_floor((outHeight*2)/4, heightAlign);
            vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y       = uiSy;
            vdMosaicParam->chnMap[winId]                = winId;

            if (forceLowCostScaling == TRUE)
                vdMosaicParam->useLowCostScaling[winId] = TRUE;
            else
                vdMosaicParam->useLowCostScaling[winId] = FALSE;
        }

        /*Small windows*/
        for(row = 0; row < 2; row++)
        {
            for(col = 0; col < 4; col++)
            {
				

                winId = 2+(row*4+col);
                vdMosaicParam->winList[winId].width         = VsysUtils_floor(vdMosaicParam->winList[0].width/2, widthAlign);
                vdMosaicParam->winList[winId].height        = VsysUtils_floor(vdMosaicParam->winList[0].height/2, heightAlign);
                vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx, widthAlign);
                vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[0].height+(vdMosaicParam->winList[winId].height*row)+uiSy, heightAlign);
                vdMosaicParam->chnMap[winId]                = winId;
                vdMosaicParam->useLowCostScaling[winId]     = TRUE;

            }
        }

        for(row = 0; row < 4; row++)
        {
            winId = 10 + row;
            vdMosaicParam->winList[winId].width         = VsysUtils_floor(vdMosaicParam->winList[0].width/2, widthAlign);
            vdMosaicParam->winList[winId].height        = VsysUtils_floor(vdMosaicParam->winList[0].height/2, heightAlign);
            vdMosaicParam->winList[winId].start_X       = VsysUtils_floor(vdMosaicParam->winList[0].width*2+uiSx, widthAlign);
            vdMosaicParam->winList[winId].start_Y       = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);
            vdMosaicParam->chnMap[winId]                = winId;
            vdMosaicParam->useLowCostScaling[winId]     = TRUE;
        }
    }


    switch(LayoutId)
    {
        case MULTICH_LAYOUT_MODE_2x2_4CH:
            rowMax = 2;
            colMax = 2;
            break;
        case MULTICH_LAYOUT_MODE_3x3_9CH:
            rowMax = 3;
            colMax = 3;
            break;
        case MULTICH_LAYOUT_MODE_4x3_12CH:
            rowMax = 3;
            colMax = 4;
            break;
        case MULTICH_LAYOUT_MODE_4x4_16CH:
            rowMax = 4;
            colMax = 4;
            break;
    }

    if(rowMax != 0)
    {
        vdMosaicParam->numberOfWindows = rowMax*colMax;
        for(row = 0; row < rowMax; row++)
        {
            for(col = 0; col < colMax; col++)
            {
                winId = row*colMax+col;

                vdMosaicParam->winList[winId].width      = VsysUtils_floor(outWidth/colMax, widthAlign);
                vdMosaicParam->winList[winId].height     = VsysUtils_floor(outHeight/rowMax, heightAlign);
                vdMosaicParam->winList[winId].start_X    = VsysUtils_floor(vdMosaicParam->winList[winId].width*col+uiSx + 1, widthAlign) ;
                vdMosaicParam->winList[winId].start_Y    = VsysUtils_floor(vdMosaicParam->winList[winId].height*row+uiSy, heightAlign);

                if(winId < maxChns)
                    vdMosaicParam->chnMap[winId] = winId;
                else
                    vdMosaicParam->chnMap[winId] = VDIS_CHN_INVALID;

                if (forceLowCostScaling == TRUE)
                    vdMosaicParam->useLowCostScaling[winId] = TRUE;
                else
                    vdMosaicParam->useLowCostScaling[winId] = FALSE;

                if(LayoutId != MULTICH_LAYOUT_MODE_2x2_4CH)
                {
                    vdMosaicParam->useLowCostScaling[winId] = TRUE;
                }
            }
        }
    }

    for (winId=0; winId<vdMosaicParam->numberOfWindows; winId++)
    {
        if(startChId>maxChns)
        {
            vdMosaicParam->chnMap[winId] = VDIS_CHN_INVALID;
        }
        else
            {
                if (winId < maxChns)
                    vdMosaicParam->chnMap[winId] = (vdMosaicParam->chnMap[winId] + startChId)%maxChns;
                else
                    vdMosaicParam->chnMap[winId] = VDIS_CHN_INVALID;
            }
    }   
}


/*
********************************************************************************
 *  @fn     DVR_Init
 *  @brief  Calls various initialization routines for diffrent modules.
 *
 *          Following modules are initialized.
 *          OSAL, OMX core
 *
 *  @return void
********************************************************************************
*/
void DVR_Init()
{
    DVR_start_front_if();
    DVR_start_dio_if();
    DVR_start_ptz_if();

    printf("app: dvr_init() ... done\n");
}

/*
********************************************************************************
 *  @fn     DVR_Deinit
 *  @brief  Deinitialize
 *
 *  @return void
********************************************************************************
*/
void DVR_Deinit()
{
    dprintf("app: dvr_deinit() ... done\n");
}


/*
********************************************************************************
 *  @fn     DVR_InitDisplayProperty
 *  @brief  Initialize of display information
 *
 *  @return void
********************************************************************************
*/
void DVR_InitDisplayProperty(ST_DISPLAY_PROPERTY *pDspProperty)
{

}
void Venc_ParamSet(VENC_PARAMS_S *pVencParams)
{
	int i,fps;
/* Override the context here as needed */
    for(i=0;i<MAX_DVR_CHANNELS;i++)
    {
		switch(gInitSettings.camera[i].camrec.iFps)
		{
			case FRAMERATE_30_25:
				fps = 30*1000;
				break;
			case FRAMERATE_15_13:
				fps = 15*1000;
				break;
			case FRAMERATE_08_06:
				fps = 8*1000;
				break;
			case FRAMERATE_04_03:
				fps = 4*1000;
				break;
			default:
				fps = 30*1000;
				break;
		}
        pVencParams->encChannelParams[i].dynamicParam.frameRate = fps;
		pVencParams->encChannelParams[i].enableAnalyticinfo = 1 ;

        int bps = gInitSettings.camera[i].camrec.ikbps*1000;
        if(bps >= 64000 && bps <= 4000000)
        {
            pVencParams->encChannelParams[i].dynamicParam.targetBitRate = bps;
        }

        switch(gInitSettings.camera[i].camrec.iIFrameInterval)
        {
            case IFRAME_INTERVAL_30:
                pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 30;
                break;
            case IFRAME_INTERVAL_15:
                pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 15;
                break;
            case IFRAME_INTERVAL_10:
                pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 10;
                break;
            case IFRAME_INTERVAL_5:
                pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 5;
                break;
            case IFRAME_INTERVAL_1:
                pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 1;
                break;
			default:
				pVencParams->encChannelParams[i].dynamicParam.intraFrameInterval = 30;
                break;

        }
		
        if(gInitSettings.camera[i].camrec.iBitrateType == BITRATE_TYPE_CBR)
        {
            pVencParams->encChannelParams[i].rcType = VENC_RATE_CTRL_CBR;
        }
        else
        {
            pVencParams->encChannelParams[i].rcType = VENC_RATE_CTRL_VBR;
        }
    }
}

void Vdis_ParamSet(VDIS_PARAMS_S *pVdisParams)
{
	int i,layoutId;
	Bool forceLowCostScale = FALSE;

	gLiveDspId = gInitSettings.layout.iMainOutput;
	gPlayDspId = gLiveDspId+1;//gInitSettings.layout.iSubOutput;

    /* Override the context here as needed */

	pVdisParams->deviceParams[gLiveDspId].resolution   = VSYS_STD_1080P_60;
	pVdisParams->deviceParams[gPlayDspId].resolution   = VSYS_STD_1080P_60;

	/* Since HDCOMP and HDMI are tied together they must have same resolution */
      pVdisParams->deviceParams[VDIS_DEV_HDMI].resolution   = pVdisParams->deviceParams[gLiveDspId].resolution;
	pVdisParams->deviceParams[VDIS_DEV_DVO2].resolution   = pVdisParams->deviceParams[gPlayDspId].resolution;
	//Vdis_tiedVencInit(VDIS_DEV_DVO2,VDIS_DEV_HDCOMP, pVdisParams);
 	Vdis_tiedVencInit(VDIS_DEV_HDMI, VDIS_DEV_HDCOMP, pVdisParams);//to test in vga
	pVdisParams->deviceParams[VDIS_DEV_SD].resolution     = VSYS_STD_PAL;//NTSC;

    switch(gInitSettings.layout.iLayoutMode)
    {
        case LIB_LAYOUTMODE_1:
			layoutId = MULTICH_LAYOUT_MODE_1CH;
			break;
		case LIB_LAYOUTMODE_4:
			layoutId = MULTICH_LAYOUT_MODE_2x2_4CH;
			break;
		case LIB_LAYOUTMODE_9:
			layoutId = MULTICH_LAYOUT_MODE_3x3_9CH;
			break;
		case LIB_LAYOUTMODE_8:
			layoutId = MULTICH_LAYOUT_MODE_1x1_8CH;
			break;
		case LIB_LAYOUTMODE_8_1:
			layoutId = MULTICH_LAYOUT_MODE_2x2_8CH;
			break;
		case LIB_LAYOUTMODE_12:
			layoutId = MULTICH_LAYOUT_MODE_4x3_12CH;
			break;
		case LIB_LAYOUTMODE_12_1:
			layoutId = MULTICH_LAYOUT_MODE_12CH;
			break;
		case LIB_LAYOUTMODE_14:
			layoutId = MULTICH_LAYOUT_MODE_14CH;
			break;
		case LIB_LAYOUTMODE_16:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		case LIB_LAYOUTMODE_20:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		case LIB_LAYOUTMODE_CAMFLOW:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		default:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
            break;
    }

    DVR_swMsGenerateLayouts(gLiveDspId, 0, MAX_CH*2, layoutId, &(pVdisParams->mosaicParams[gLiveDspId]),
							forceLowCostScale, pVdisParams->deviceParams[gLiveDspId].resolution, 0);

    pVdisParams->mosaicParams[gLiveDspId].userSetDefaultSWMLayout = TRUE;
    for (i=0; i<pVdisParams->mosaicParams[gLiveDspId].numberOfWindows; i++)
    {
	    gMosaicInfo_Live.chMap[i] = pVdisParams->mosaicParams[gLiveDspId].chnMap[i];
    }

	gMosaicInfo_Live.layoutId 		= layoutId;
	gMosaicInfo_Live.numWin 		= pVdisParams->mosaicParams[gLiveDspId].numberOfWindows;
	gMosaicInfo_Live.isCamPropMode 	= 0;

    switch(gInitSettings.layout.iSecondLayoutMode)
    {
        case LIB_LAYOUTMODE_1:
			layoutId = MULTICH_LAYOUT_MODE_1CH;
			break;
		case LIB_LAYOUTMODE_4:
			layoutId = MULTICH_LAYOUT_MODE_2x2_4CH;
			break;
		case LIB_LAYOUTMODE_9:
			layoutId = MULTICH_LAYOUT_MODE_3x3_9CH;
			break;
		case LIB_LAYOUTMODE_8:
			layoutId = MULTICH_LAYOUT_MODE_1x1_8CH;
			break;
		case LIB_LAYOUTMODE_8_1:
			layoutId = MULTICH_LAYOUT_MODE_2x2_8CH;
			break;
		case LIB_LAYOUTMODE_12:
			layoutId = MULTICH_LAYOUT_MODE_4x3_12CH;
			break;
		case LIB_LAYOUTMODE_12_1:
			layoutId = MULTICH_LAYOUT_MODE_12CH;
			break;
		case LIB_LAYOUTMODE_14:
			layoutId = MULTICH_LAYOUT_MODE_14CH;
			break;
		case LIB_LAYOUTMODE_16:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		case LIB_LAYOUTMODE_20:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		case LIB_LAYOUTMODE_CAMFLOW:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
			break;
		default:
			layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;

			break;
	}

	//if(gInitSettings.layout.iMainOutput != gInitSettings.layout.iSubOutput)
    {
        DVR_swMsGenerateLayouts(gPlayDspId, MAX_CH, MAX_CH*2, layoutId, &(pVdisParams->mosaicParams[gPlayDspId]),

								forceLowCostScale,pVdisParams->deviceParams[gPlayDspId].resolution, 0);
        pVdisParams->mosaicParams[gPlayDspId].userSetDefaultSWMLayout = TRUE;

	    for (i=0; i<pVdisParams->mosaicParams[gPlayDspId].numberOfWindows; i++)
	    {
		    gMosaicInfo_Play.chMap[i] = pVdisParams->mosaicParams[gPlayDspId].chnMap[i];
			pVdisParams->mosaicParams[gPlayDspId].chnMap[i] = VDIS_CHN_INVALID;
	    }

		gMosaicInfo_Play.layoutId		= layoutId;
		gMosaicInfo_Play.numWin 		= pVdisParams->mosaicParams[gPlayDspId].numberOfWindows;
		gMosaicInfo_Play.isCamPropMode 	= 0;
	}
	/*else
	{
		DVR_swMsGenerateLayouts(gPlayDspId, MAX_CH, MAX_CH*2, layoutId, &(pVdisParams->mosaicParams[VDIS_DEV_HDCOMP]),

								forceLowCostScale,pVdisParams->deviceParams[VDIS_DEV_HDCOMP].resolution, 0);
		pVdisParams->mosaicParams[VDIS_DEV_HDCOMP].userSetDefaultSWMLayout = TRUE;

	    for (i=0; i<pVdisParams->mosaicParams[VDIS_DEV_HDCOMP].numberOfWindows; i++)
	    {
		    gMosaicInfo_Play.chMap[i] = pVdisParams->mosaicParams[VDIS_DEV_HDCOMP].chnMap[i];
			pVdisParams->mosaicParams[VDIS_DEV_HDCOMP].chnMap[i] = VDIS_CHN_INVALID;
	    }

		gMosaicInfo_Play.layoutId		= gMosaicInfo_Live.layoutId;
		gMosaicInfo_Play.numWin 		= gMosaicInfo_Live.numWin;
		gMosaicInfo_Play.isCamPropMode	= 0;
	}	*/
    pVdisParams->enableLayoutGridDraw = FALSE;
}

void SecOut_Set(int vencPrimaryChn,VDIS_PARAMS_S *pVdisParams)
{
 //if(enable_sec)
    //{
        Int32 chId;
        VENC_CHN_DYNAMIC_PARAM_S params = { 0 };


        for (chId=0; chId < vencPrimaryChn; chId++)
        {
             /* now use VIP-SC secondary output, so input to VIP-SC and VIP-SC secondary channel are both
                        half of the real input framerate */

            /* At capture level,YUV420 D1 stream id is 1. Set for D1 channels 0 ~ MAX_CH */
            Vcap_setFrameRate(chId, 1, 30, 30);

            /* At capture level,YUV422 D1 stream id is 0. Set for D1 channels 0 ~ MAX_CH */
            Vcap_setFrameRate(chId, 0, 60, 0);


            {

                Int32 i;
                for (i=0; i<pVdisParams->mosaicParams[gLiveDspId].numberOfWindows; i++)
                {
                    if((pVdisParams->mosaicParams[gLiveDspId].chnMap[i] == chId) && (pVdisParams->mosaicParams[gLiveDspId].useLowCostScaling[i] == FALSE))
                        Vcap_setFrameRate(chId,0,60,30);
                }

                for (i=0; i<pVdisParams->mosaicParams[gPlayDspId].numberOfWindows; i++)
                {
                    if((pVdisParams->mosaicParams[gPlayDspId].chnMap[i] == chId) && (pVdisParams->mosaicParams[gPlayDspId].useLowCostScaling[i] == FALSE))
                        Vcap_setFrameRate(chId,0,60,30);
                }

            }


            /* At capture level, CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
            Vcap_setFrameRate(chId, 2, 30, 30);

        }

        for(chId=0;chId<vencPrimaryChn;chId++)
        {
            memset(&params, 0, sizeof(params));
            Venc_setInputFrameRate(chId, CIF_FPS_DEI_PAL);

            params.frameRate = CIF_FPS_DEI_PAL;

            Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

           // if(vsysParams.enableSecondaryOut)
            //{
                memset(&params, 0, sizeof(params));

                Venc_setInputFrameRate(chId+vencPrimaryChn, CIF_FPS_DEI_PAL);

                params.frameRate = CIF_FPS_ENC_PAL;
                params.targetBitRate = CIF_BITRATE * 1000;

                /* At encode level, CIF channel id is 16 ~ 31. Stream id is immaterial */
                Venc_setDynamicParam(chId+vencPrimaryChn, 0, &params, VENC_FRAMERATE);
            //}
        }

    //}
}

void Osd_Setparam(void)
{
	int i,j;
		VCAP_CHN_DYNAMIC_PARAM_S params;

		for(i = 0; i<MAX_DVR_CHANNELS; i++)
		{
			memset(&params, 0, sizeof(params));


	        for(j = 0; j < ALG_LINK_OSD_MAX_WINDOWS; j++)
	        {
	            g_osdChParam[i].winPrm[j].enableWin = 0;
	            /* max value check below need to be fixed below based upon capture max width and height*/
	        }

            params.osdChWinPrm = &g_osdChParam[i];

	        Vcap_setDynamicParamChn(i, &params, VCAP_OSDWINPRM);

		//DVR_setCaptureResolution(i, gInitSettings.camera[i].camrec.iResolution);
		}
}
void Get_SourceStatus(Bool enable_sec,VENC_PARAMS_S *pVencParams)
{
        UInt32 chId;

        VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
        VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;
        VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

        Vcap_getVideoSourceStatus( &videoSourceStatus );
		
	gCaptureInfo.chCount = videoSourceStatus.numChannels;
    	for(chId=0; chId<videoSourceStatus.numChannels; chId++)
    	{
        	pChStatus = &videoSourceStatus.chStatus[chId];

        	if(pChStatus->isVideoDetect)
        	{
        		gCaptureInfo.chStatus[chId].frameWidth = pChStatus->frameWidth;
        		//if(pChStatus->isInterlaced)
        		//	gCaptureInfo.chStatus[chId].frameHeight = (pChStatus->frameHeight) << 1;
        		//else
        			gCaptureInfo.chStatus[chId].frameHeight = pChStatus->frameHeight;
				
			if(pChStatus->frameHeight == 288)
			{
                    	memset(&params, 0, sizeof(params));

                    	Venc_setInputFrameRate(chId, CIF_FPS_DEI_PAL);

					switch(gInitSettings.camera[chId].camrec.iFps)
					{
						case FRAMERATE_30_25:
							params.frameRate = 25;
							break;
						case FRAMERATE_15_13:
							params.frameRate = 13;
							break;
						case FRAMERATE_08_06:
							params.frameRate = 6;
							break;
						case FRAMERATE_04_03:
							params.frameRate = 3;
							break;
						default:
							params.frameRate = 25;
							break;
					}
			
				//params.targetBitRate = pVencParams->encChannelParams[chId].dynamicParam.targetBitRate;
				//printf("DVR_StartSystem:bitrate=%d,fps=%d\n",params.targetBitRate,params.frameRate);
                    	Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                    	if(enable_sec)
                    	{
                       	 	memset(&params, 0, sizeof(params));

                        		Venc_setInputFrameRate(chId+pVencParams->numPrimaryChn, CIF_FPS_DEI_PAL);

                        		params.frameRate = CIF_FPS_ENC_PAL;
                        		params.targetBitRate = CIF_BITRATE * 1000;
			   		 //printf("DVR_StartSystem_cif:bitrate=%d,fps=%d\n",params.targetBitRate,params.frameRate);
                        		Venc_setDynamicParam(chId+pVencParams->numPrimaryChn, 0, &params, VENC_FRAMERATE);
                    	}
                	}
			else if(pChStatus->frameHeight == 240)
			{
                    	memset(&params, 0, sizeof(params));
                    	Venc_setInputFrameRate(chId, CIF_FPS_DEI_NTSC);

					switch(gInitSettings.camera[chId].camrec.iFps)
					{
						case FRAMERATE_30_25:
							params.frameRate = 30;
							break;
						case FRAMERATE_15_13:
							params.frameRate = 15;
							break;
						case FRAMERATE_08_06:
							params.frameRate = 8;
							break;
						case FRAMERATE_04_03:
							params.frameRate = 4;
							break;
						default:
							params.frameRate = 30;
							break;
					}
					params.targetBitRate = pVencParams->encChannelParams[chId].dynamicParam.targetBitRate;
				//printf("DVR_StartSystem_n:bitrate=%d,fps=%d\n",params.targetBitRate,params.frameRate);
                    	Venc_setDynamicParam(chId, 0, &params, VENC_FRAMERATE);

                    	if(enable_sec)
                    	{
                        		memset(&params, 0, sizeof(params));

                        		Venc_setInputFrameRate(chId+pVencParams->numPrimaryChn, CIF_FPS_DEI_NTSC);

                        		params.frameRate = CIF_FPS_ENC_NTSC;
                        		params.targetBitRate = CIF_BITRATE * 1000;
					//printf("DVR_StartSystem_cif_n:bitrate=%d,fps=%d\n",params.targetBitRate,params.frameRate);
                        		Venc_setDynamicParam(chId+pVencParams->numPrimaryChn, 0, &params, VENC_FRAMERATE);
                    	}
                	}	
			
		   printf("video detect ch%02d res[%dx%d]\n", chId, gCaptureInfo.chStatus[chId].frameWidth,gCaptureInfo.chStatus[chId].frameHeight);
        	}
            else
        	{
		printf (" [DVR_APP]: CH%2d: No video detected at [vipID:%d, vipCH: %d] and set PAL mode!!!\n",
					chId, pChStatus->vipInstId, pChStatus->chId);
        	}
    	}
}

int DVR_StartSystem(int isNTSC, int main_vout, int sub_vout, int venc_mode)
{
    int i;
    VSYS_PARAMS_S vsysParams;
    VCAP_PARAMS_S vcapParams;
    VENC_PARAMS_S vencParams;
    VDEC_PARAMS_S vdecParams;
    VDIS_PARAMS_S vdisParams;
	
    UInt64 wallTimeBase;
    AUDIO_STATE *pAudstate = &gAudio_state ;
    pAudstate->pbchannel = 0;

    VdecVdis_bitsRdInit();

    Vsys_params_init(&vsysParams);
    Vcap_params_init(&vcapParams);
    Venc_params_init(&vencParams);
    Vdec_params_init(&vdecParams);
    Vdis_params_init(&vdisParams);

    vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC;

    vcapParams.numChn           = 16;
    vencParams.numPrimaryChn    = 16;
    vdecParams.numChn           = 16;
    vdisParams.numChannels      = 32;
    vsysParams.numChs           = 16;

    //CIF output setting///
#ifdef RTSP_STREAM_ENABLE
    vsysParams.enableSecondaryOut   = TRUE;
    vsysParams.enableNsf            = TRUE;
    vencParams.numSecondaryChn      = 16;
#else
    vsysParams.enableSecondaryOut   = FALSE;
    vsysParams.enableNsf            = FALSE;
    vencParams.numSecondaryChn      = 0;
#endif
    ///////////////////////

    vsysParams.enableAVsync     = TRUE;
	vsysParams.enableMjpegEnc 	= FALSE;

    vsysParams.enableCapture = TRUE;
    vsysParams.enableNullSrc = FALSE;
    vsysParams.enableOsd     = TRUE;
	vsysParams.enableScd     = FALSE;

    vsysParams.numDeis       = 2;
    vsysParams.numSwMs       = 2;
    vsysParams.numDisplays   = 3;

    printf ("--------------- CHANNEL DETAILS-------------\n");
    printf ("Capture Channels => %d\n", vcapParams.numChn);
    printf ("Enc Channels => Primary %d, Secondary %d\n", vencParams.numPrimaryChn, vencParams.numSecondaryChn);
    printf ("Dec Channels => %d\n", vdecParams.numChn);
    printf ("Disp Channels => %d\n", vdisParams.numChannels);
    printf ("-------------------------------------------\n");

    /* Override the context here as needed */
    Vsys_init(&vsysParams);


    /* Override the context here as needed */
    for(i=0;i<MAX_DVR_CHANNELS;i++)
    {
		vcapParams.channelParams[i].dynamicParams.contrast 		= gInitSettings.camera[i].coloradjust.iContrast;
		vcapParams.channelParams[i].dynamicParams.satauration 	= gInitSettings.camera[i].coloradjust.iSaturation;
		vcapParams.channelParams[i].dynamicParams.brightness 	= gInitSettings.camera[i].coloradjust.iBrightness;
	}
	
    Vcap_init(&vcapParams);
    Venc_ParamSet(&vencParams);
    Venc_init(&vencParams);

	vdecParams.forceUseDecChannelParams = TRUE ; // Pal
    /* Override the context here as needed */
    Vdec_init(&vdecParams);

    Vdis_ParamSet(&vdisParams);
    /* Override the context here as needed */
    Vdis_init(&vdisParams);

    /* Init the application specific module which will handle bitstream exchange */
	WRITER_create();
    wallTimeBase = get_current_time_to_msec();
    Vdis_setWallTimeBase(wallTimeBase);

 	/* Configure display in order to start grpx before video */
 	Vsys_configureDisplay();

    printf("=====Going to Vsys_create=========\n");
    /* Create Link instances and connects compoent blocks */
    Vsys_create();


    if(vsysParams.enableSecondaryOut)
		SecOut_Set(vencParams.numPrimaryChn,&vdisParams);

    /* Start components in reverse order */
    Vdis_start();
    Vdec_start();
    Venc_start();
    Vcap_start();

    Get_SourceStatus(vsysParams.enableSecondaryOut,&vencParams);
    Osd_Setparam();

    for(i=0;i<MAX_DVR_CHANNELS;i++)
    {
        if(gInitSettings.camera[i].iEnable == 0)
            DVR_SetCameraEnable(i, gInitSettings.camera[i].iEnable);
        if(gInitSettings.playbackcamera[i].iEnable == 0)
			DVR_SetCameraEnable(MAX_CH+i, gInitSettings.playbackcamera[i].iEnable);
    }

    DVR_FBStart();

    DVR_SetSpotChannel(gInitSettings.iSpotOutputCh);

    DVR_setOff_GRPX() ;
#if 0
    DVR_SwitchDisplay(gInitSettings.layout.iMainOutput, gInitSettings.layout.iSubOutput, 
    DISP_PATH_SD, gInitSettings.maindisplayoutput.iResolution, 
    gInitSettings.subdisplayoutput.iResolution, DC_MODE_NTSC);
#endif    
    printf("\n ===============Switch Display Done========================= \n");

    if(gInitSettings.maindisplayoutput.iResolution != DC_MODE_1080P_60)
    {
        DVR_SetDisplayRes(gInitSettings.layout.iMainOutput, gInitSettings.maindisplayoutput.iResolution);
    }

    printf("\n ===============Main Output Resolution Switch Done========== \n");
    if(gInitSettings.subdisplayoutput.iResolution != DC_MODE_1080P_60)
    {
        DVR_SetDisplayRes(gInitSettings.layout.iSubOutput, gInitSettings.subdisplayoutput.iResolution);
    }

    printf("\n ===============Sub Output Resolution Switch Done=========== \n");

	DVR_setOff_GRPX() ;

	//REMOTEPLAY_create() ;
	
#ifdef AUDIO_STREAM_ENABLE
	DVR_setAudioInputParams(AUDIO_SAMPLE_RATE_DEFAULT, gInitSettings.audioin.iVolume);

	Audio_playStart(gInitSettings.audioout.iDevId);
	Audio_playSetVolume(gInitSettings.audioout.iVolume*10);
      Audio_captureStart();

    sleep(1);
#endif
	printf("\n ================DVR_StartSystem Done===================== \n");

    return 0;
}

int DVR_StopSystem()
{
    WRITER_stop();

    /* Stop components */
    Vcap_stop();
    Venc_stop();
    Vdec_stop();
    Vdis_stop();

    Vsys_delete();

 	/* De-configure display */
 	Vsys_deConfigureDisplay();

    WRITER_delete();

    /* De-initialize components */
    Vcap_exit();
    Venc_exit();
    Vdec_exit();
    Vdis_exit();
    Vsys_exit();

    return 0;
}

void Color_set(void)
{
	char brightness=128,saturation=128,contrast=128,value,value1,c,chId;
	int param,hue=0;

	while(1)
	{
	printf("\nPlease choose parameter to set: 1--brightness  2--saturation  3--contrast  4--hue  5--auto contrast  6--get brightness 7--get saturation  8--exit\n");
	//while ( (c = getchar()) != '\n' && c != EOF ) ;
	scanf("%d",&param);
	switch(param)
	{
		case 1:
			printf("\nPlease input brightness direction:1--add, 2--sub, 3--number  4--quit\n");
			while(1)
			{
				scanf("%c",&value);
				if(value == 1) 
					brightness++;
				else if(value == 2)
					brightness--;
				else if(value == 3)
				{
					scanf("%c",&value1);
					brightness = value1;
					while((c=getchar())!='\n');
				}
				else if(value == 4)
					break;
				//printf("set brightness=%d\n",brightness);
				DVR_SetBrightness(0,brightness);
			}
			break;
		case 2:
			printf("\nPlease input saturation direction:1--add, 2--sub, 3--number, 4--quit\n");
			while(1)
			{
				scanf("%c",&value);
				if(value == 1) 
					saturation++;
				else if(value == 2)
					saturation--;
				else if(value == 3)
				{
					scanf("%c",&value1);
					saturation = value1;
					while((c=getchar())!='\n');
				}
				else if(value == 4)
					break;
				//printf("set saturation=%d\n",saturation);
				DVR_SetSaturation(0,saturation);
			}
			break;
		case 3:
			printf("\nPlease input contrast direction:1--add, 2--sub, 3--number, 4--quit\n");
			while(1)
			{
				scanf("%c",&value);
				if(value == 1) 
					contrast++;
				else if(value == 2)
					contrast--;
				else if(value == 3)
				{
					scanf("%c",&value1);
					contrast = value1;
					while((c=getchar())!='\n');
				}
				else if(value == 4)
					break;
				//printf("set contrast=%d\n",saturation);
				DVR_SetContrast(0,contrast);
			}

		  case 4:
			printf("\nPlease input hue direction:1--add, 2--sub, 3--number, 4--quit\n");
			while(1)
			{
				scanf("%c",&value);
				if(value == 1) 
					hue++;
				else if(value == 2)
					hue--;
				else if(value == 3)
				{
					scanf("%c",&value1);
					hue = value1;
					while((c=getchar())!='\n');
				}
				else if(value == 4)
					break;
				//printf("set contrast=%d\n",saturation);
				DVR_SetHue(0,hue);
			}
			break;
		case 5:
			printf("\nPlease input on or off auto contrast mode:1--on, 2--off, 3--quit\n");
			char auto_mode=3;
			while(1)
			{
				scanf("%c",&value);
				if(value == 1)
					auto_mode = 0;
				else if(value == 2)
					auto_mode = 3;
				else if(value == 3)
					break;
				//while((c=getchar())!='\n');
				chId = 0;
				printf("value=%d,chId=%d\n",value,chId);
		   		Vcap_autoContrast(chId,auto_mode);
			}
                	 break;
		case 6:
			DVR_GetBrightness(0,&value);
			printf("birghtness=%d\n",value);
			break;
		case 7:
			DVR_GetSaturation(0,&value);
			printf("saturation=%d\n",value);
			break;
		case 8:
			exit(0);
		default:
			break;
	}		
	}
}	
int DVR_color_test()
{
    int retval = TRUE ;

    struct sched_param schedParam;
    pthread_t test ;
    pthread_attr_t     attr;

    if (pthread_attr_init(&attr)) 
   {
        retval = FALSE ;
    }

    /* Force the thread to use custom scheduling attributes */
    if (pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED)) {
        retval = FALSE ;
    }

    /* Set the thread to be fifo real time scheduled */
    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
        retval = FALSE ;
    }

    schedParam.sched_priority = 1;//sched_get_priority_max(SCHED_FIFO) - 20;
    if (pthread_attr_setschedparam(&attr, &schedParam))
    {
        retval = FALSE ;
    }
    if(pthread_create(&test, &attr, (void*)Color_set, (void*)NULL))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("Failed to create logon thread\n") ;
#endif
        retval = FALSE ;
    }
	return retval;
}

void ptzTest(void)
{
	int param,protocol,channel,ptzcmd;
	int baudrate;

	while(1)
	{
	printf("\nPlease choose parameter to set: 1--protocol and baudrate  2--address  3--command\n");
	//while ( (c = getchar()) != '\n' && c != EOF ) ;
	scanf("%d",&param);
	switch(param)
	{
		case 1:
			printf("\nPlease choose protocol and baudrate:1--pelco_d, 2--pelco_p:\n");
			scanf("%d %d",&protocol,&baudrate);
			DVR_RE_ptz_setInfo(protocol,baudrate);
			//printf("protocol=%d,baudrate=%d\n",protocol,baudrate);
			break;
		case 2:
			printf("\nPlease input address:\n");
			scanf("%d",&channel);
			//printf("channel=%d\n",channel);
			break;
		case 3:
			printf("\nPlease input command:1--R,2--L,3--U,4--D,5--ZI,6--ZO,7--FO,8--FI,9--S\n");
			scanf("%d",&ptzcmd);
			//printf("protocol=%d,channel=%d,ptzcmd=%d\n",protocol,channel,ptzcmd);
			DVR_RE_ptz_setInternal_Ctrl(protocol,channel,ptzcmd);	
			break;
		case 4:
			exit(0);
		default:
			break;
	}		
	}
}


//#ifndef RTSP_STREAM_ENABLE
int DVR_sock_startStream()
{
    int retval = TRUE ;

    struct sched_param schedParam;
    pthread_t logon ;
    pthread_t control ;
    pthread_t data ;
    pthread_t ipset ;
    pthread_attr_t     attr;

    if (pthread_attr_init(&attr)) {
        retval = FALSE ;
    }

    TcpServerInit() ;

	//netaddr(NETWORKMODIFYCFG) ;
    /* Force the thread to use custom scheduling attributes */
    if (pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED)) {
        retval = FALSE ;
    }

    /* Set the thread to be fifo real time scheduled */
    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
        retval = FALSE ;
    }

    schedParam.sched_priority = 2;//sched_get_priority_max(SCHED_FIFO) - 20;
    if (pthread_attr_setschedparam(&attr, &schedParam))
    {
        retval = FALSE ;
    }
    if(pthread_create(&logon, &attr, (void*)Lsock, (void*)NULL))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("Failed to create logon thread\n") ;
#endif
        retval = FALSE ;
    }
    schedParam.sched_priority = 2;//sched_get_priority_max(SCHED_FIFO) - 20 ;
    if (pthread_attr_setschedparam(&attr, &schedParam))
    {
        retval = FALSE ;
    }

    if(pthread_create(&control, &attr, (void*)Csock, (void*)NULL))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("Failed to create control thread\n") ;
#endif
        retval = -FALSE ;
    }

    schedParam.sched_priority = 10;//sched_get_priority_max(SCHED_FIFO) - 20 ;
    if (pthread_attr_setschedparam(&attr, &schedParam))
    {
        retval = FALSE ;
    }
    if(pthread_create(&data, &attr,  (void*)Dsock, (void*)NULL))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("Failed to create data thread\n") ;
#endif
        retval =FALSE ; 
    }

    schedParam.sched_priority = 1;//sched_get_priority_max(SCHED_FIFO) - 50 ;
    if (pthread_attr_setschedparam(&attr, &schedParam))
    {
        retval =FALSE ;
    }

    if(pthread_create(&ipset, &attr,  (void*)Ipset, (void*)NULL))
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("Failed to create data thread\n") ;
#endif
        retval = FALSE ;
    }

	//DVR_RE_ptz_setInfo(PTZCTRL_PELCO_D,2400);
	//DVR_RE_ptz_setInternal_Ctrl(PTZCTRL_PELCO_D,0,PAN_RIGHT);
	//DVR_RE_ptz_setInfo(PTZCTRL_PELCO_P,9600);
	//DVR_RE_ptz_setInternal_Ctrl(PTZCTRL_PELCO_P,0,PAN_RIGHT);
	//ptzTest();
	
    return retval ;
}
//#endif
void DVR_startRTSP(void)
{
#ifdef RTSP_STREAM_ENABLE
//  if(get_rtsp_status())

        int status, cnt;
        AVSERVER_Config config;
		
		if(get_rtsp_status())
		return;

        status = AVSERVER_init();
        if(status != 0)
            eprintf("AVSERVER_init() Fail!!! \n");

        config.numEncodeStream                          = AVSERVER_MAX_STREAMS;

        config.audioConfig.captureEnable                = FALSE;
        config.audioConfig.samplingRate                 = 16000;
        config.audioConfig.codecType                    = ALG_AUD_CODEC_G711_RTSP;
        config.audioConfig.fileSaveEnable               = FALSE;

    for(cnt = 0; cnt < config.numEncodeStream; cnt++)
    {
        config.encodeConfig[cnt].captureStreamId            = 0;
        config.encodeConfig[cnt].cropWidth                  = 352;//704;//
	    config.encodeConfig[cnt].cropHeight 				= 288;//576;//
        config.encodeConfig[cnt].frameRateBase              = 30000;
        config.encodeConfig[cnt].frameSkipMask              = 0xFFFFFFFF;
        config.encodeConfig[cnt].codecType                  = ALG_VID_CODEC_H264;
        config.encodeConfig[cnt].codecBitrate               = 2000000;
        config.encodeConfig[cnt].encryptEnable              = FALSE;
        config.encodeConfig[cnt].fileSaveEnable             = FALSE;
        config.encodeConfig[cnt].motionVectorOutputEnable   = FALSE;
        config.encodeConfig[cnt].qValue                     = 0;
    }
    config.streamConfig.mem_layou_set                = 1;

    status = AVSERVER_start(&config);
    if(status != 0)
    {
        eprintf("AVSERVER_start() Fail!!! \n");
        AVSERVER_exit();
    }

    AVSERVER_rtspServerStart();

    (void) set_rtsp_status(TRUE);

#endif
	//DVR_color_test();
	 DVR_sock_startStream();
}


void DVR_stopRTSP(void)
{
    if(!get_rtsp_status())
		return;
	
    AVSERVER_rtspServerStop() ;
    AVSERVER_stop() ;
    AVSERVER_exit() ;
	
	(void) set_rtsp_status(FALSE);
}

int DVR_SetMosaicLayout(int layoutMode)
{
    int ret = 1, layoutId, winId;
    VDIS_MOSAIC_S vdMosaicParam;
	AUDIO_STATE *pAudstate = &gAudio_state ;

    switch(layoutMode)
    {
        case LIB_LAYOUTMODE_1:
            layoutId = MULTICH_LAYOUT_MODE_1CH;         break;
        case LIB_LAYOUTMODE_4:
            layoutId = MULTICH_LAYOUT_MODE_2x2_4CH;     break;
        case LIB_LAYOUTMODE_9:
            layoutId = MULTICH_LAYOUT_MODE_3x3_9CH;     break;
        case LIB_LAYOUTMODE_8:
            layoutId = MULTICH_LAYOUT_MODE_1x1_8CH;     break;
        case LIB_LAYOUTMODE_8_1:
            layoutId = MULTICH_LAYOUT_MODE_2x2_8CH;     break;
        case LIB_LAYOUTMODE_12:
            layoutId = MULTICH_LAYOUT_MODE_4x3_12CH;    break;
        case LIB_LAYOUTMODE_12_1:
            layoutId = MULTICH_LAYOUT_MODE_12CH;        break;
        case LIB_LAYOUTMODE_14:
            layoutId = MULTICH_LAYOUT_MODE_14CH;        break;
        case LIB_LAYOUTMODE_16:
            layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;    break;
        default:
            layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
        break;
    }

    DVR_swMsGenerateLayouts(gLiveDspId, 0, MAX_CH*2, layoutId, &vdMosaicParam, FALSE,gInitSettings.maindisplayoutput.iResolution, 0);
	for (winId=0; winId < vdMosaicParam.numberOfWindows; winId++)
	{
		gMosaicInfo_Live.chMap[winId] = vdMosaicParam.chnMap[winId];
		if((gInitSettings.layout.iMainOutput == gInitSettings.layout.iSubOutput) && (pAudstate->state == LIB_PLAYBACK_MODE))
		{
			vdMosaicParam.chnMap[winId] = VDIS_CHN_INVALID;
		}
	}

	{
		Int32 i;

		for (i=0; i<vdMosaicParam.numberOfWindows; i++)
		{
			/* At capture level,YUV422 D1 stream id is 0. Set for D1 channels 0 ~ MAX_CH */
			Vcap_setFrameRate(vdMosaicParam.chnMap[i], 0, 60, 0);

			if((vdMosaicParam.chnMap[i] < Vcap_getNumChannels()) && (vdMosaicParam.useLowCostScaling[i] == FALSE))
				Vcap_setFrameRate(vdMosaicParam.chnMap[i],0,60,30);

			/* At capture level,CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
			Vcap_setFrameRate(vdMosaicParam.chnMap[i], 2, 30, 30);
		}

	}
	
    Vdis_setMosaicParams(gLiveDspId, &vdMosaicParam);
	
	gMosaicInfo_Live.layoutId 		= layoutId;
	gMosaicInfo_Live.numWin 		= vdMosaicParam.numberOfWindows;
	gMosaicInfo_Live.isCamPropMode 	= 0;


    return ret;
}

int DVR_SetPlaybackMosaicLayout(int layoutMode)
{
	int ret = 1, layoutId, winId;
    VDIS_MOSAIC_S vdMosaicParam;
	//AUDIO_STATE *pAudstate = &gAudio_state ;

    switch(layoutMode)
    {
        case LIB_LAYOUTMODE_1:
            layoutId = MULTICH_LAYOUT_MODE_1CH;         break;
        case LIB_LAYOUTMODE_4:
            layoutId = MULTICH_LAYOUT_MODE_2x2_4CH;     break;
        case LIB_LAYOUTMODE_9:
            layoutId = MULTICH_LAYOUT_MODE_3x3_9CH;     break;
        case LIB_LAYOUTMODE_8:
            layoutId = MULTICH_LAYOUT_MODE_1x1_8CH;     break;
        case LIB_LAYOUTMODE_8_1:
            layoutId = MULTICH_LAYOUT_MODE_2x2_8CH;     break;
        case LIB_LAYOUTMODE_12:
            layoutId = MULTICH_LAYOUT_MODE_4x3_12CH;    break;
        case LIB_LAYOUTMODE_12_1:
            layoutId = MULTICH_LAYOUT_MODE_12CH;        break;
        case LIB_LAYOUTMODE_14:
            layoutId = MULTICH_LAYOUT_MODE_14CH;        break;
        case LIB_LAYOUTMODE_16:
            layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;    break;
        default:
            layoutId = MULTICH_LAYOUT_MODE_4x4_16CH;
        break;

    }
    DVR_swMsGenerateLayouts(gPlayDspId, MAX_CH, MAX_CH*2, layoutId, &vdMosaicParam, FALSE,gInitSettings.subdisplayoutput.iResolution, 0);
	for (winId=0; winId < vdMosaicParam.numberOfWindows; winId++)
	{
		gMosaicInfo_Play.chMap[winId] = vdMosaicParam.chnMap[winId];

			//vdMosaicParam.chnMap[winId] = VDIS_CHN_INVALID;
	}
	
    Vdis_setMosaicParams(gPlayDspId, &vdMosaicParam);

	gMosaicInfo_Play.layoutId 		= layoutId;
	gMosaicInfo_Play.numWin 		= vdMosaicParam.numberOfWindows;
	gMosaicInfo_Play.isCamPropMode 	= 0;



//	Vsys_printDetailedStatistics();	//Arun

    return ret;
}

int DVR_SetRecording(int channel, int enableRec, int event_mode, int addAudio, char* camName)
{
    int status = 0;

    if(channel < MAX_CH)
    {
        status = WRITER_enableChSave(channel, enableRec, event_mode, addAudio, camName);
    }

    return status;

}

#if PREREC_ENABLE // BKKIM pre-record
void DVR_SetPreRecDuration(int channel, int presec)
{
	if(channel < MAX_CH)
	{
		//BKKIM test
		WRITER_setPreRecSec(channel, presec);
	}
}
#endif

int DVR_changeCamName(int channel, char* camName)
{
    return WRITER_changeCamName(channel, camName);
}

void DVR_closeBasket()
{
    WRITER_fileSaveExit();
}

////MULTI-HDD SKSUNG////
void DVR_setWriterMsgPipe(void* pipe_id)
{
    WRITER_setMsgPipe(pipe_id);
}

int DVR_GetCurPlaybackTime(struct tm *tp)
{

    T_BKT_TM tm1;
    long sec ;
    sec = BKTMD_GetCurPlayTime() ;
    //dprintf("sec = %ld\n",sec) ;
    BKT_GetLocalTime(sec, &tm1) ;
    /*
    dprintf("tm1.year = %d\n",tm1.year) ;
    dprintf("tm1.mon = %d\n",tm1.mon) ;
    dprintf("tm1.day = %d\n",tm1.day) ;
    dprintf("tm1.hour = %d\n",tm1.hour) ;
    dprintf("tm1.min = %d\n",tm1.min) ;
    dprintf("tm1.sec = %d\n",tm1.sec) ;
    */
    tp->tm_year = tm1.year ;
    tp->tm_mon = tm1.mon - 1 ;
    tp->tm_mday = tm1.day ;
    tp->tm_hour = tm1.hour ;
    tp->tm_min = tm1.min ;
    tp->tm_sec = tm1.sec ;

    return 1 ;
}


////MULTI-HDD SKSUNG////
long DVR_GetRecDays(long t_o, int* pRecMonTBL)
{

    char diskpath[30] ;

    T_BKT_TM tm1;
    BKT_GetLocalTime(t_o, &tm1);

    int i, bFindOK=0, j = 0;

    char path[LEN_PATH];

    for(j = 0; j < MAX_HDD_COUNT; j++)
    {
        if(gbkt_mount_info.hddInfo[j].isMounted)
        {
            sprintf(diskpath, "%s", gbkt_mount_info.hddInfo[j].mountDiskPath) ;

            for(i=0;i<31;i++)
            {
                sprintf(path, "%s/rdb/%04d%02d%02d", diskpath, tm1.year, tm1.mon, i+1);
                if(-1 != access(path, 0))
                {
                    *(pRecMonTBL+i) |= 1;

                    if(!bFindOK)
                        bFindOK=1;
                }
                else
                {
                    *(pRecMonTBL+i) |= 0;
                    if(!bFindOK)
                        bFindOK=1;
                }
            }
        }
    }

    return bFindOK;
}


long DVR_GetRecHour(int ch, long sec, int* pRecHourTBL)
{
    #ifdef BKT_SYSIO_CALL
    int fd;
    #else
    FILE *fd;
    #endif
    int i = 0, j = 0 ,Lastvalue = 0;
    char inRecHourTBL[TOTAL_MINUTES_DAY] ;
    char diskpath[30] ;


    T_BKT_TM tm1;
    BKT_GetLocalTime(sec, &tm1);

    char path[LEN_PATH];

    memset(pRecHourTBL, 0, TOTAL_MINUTES_DAY);
    /*
    for(i = 0; i < TOTAL_MINUTES_DAY; i++)
    {
        pRecHourTBL[i] = 0 ;
    }
    */

    for(j = 0; j < MAX_HDD_COUNT; j++)
    {
        if(gbkt_mount_info.hddInfo[j].isMounted)
        {
            sprintf(diskpath, "%s", gbkt_mount_info.hddInfo[j].mountDiskPath) ;

            sprintf(path, "%s/rdb/%04d%02d%02d", diskpath, tm1.year, tm1.mon, tm1.day);

            if(!OPEN_RDONLY(fd, path))
            {
                continue;
            }

            long offset = TOTAL_MINUTES_DAY*ch;
            if(!LSEEK_SET(fd, offset))
            {
                CLOSE_BKT(fd);
                continue;
            }

            if(!READ_PTSZ(fd, inRecHourTBL, TOTAL_MINUTES_DAY))
            {
                CLOSE_BKT(fd);
                continue;
            }

            for(i = 0; i < TOTAL_MINUTES_DAY; i++)
            {
                pRecHourTBL[i] |= (int)inRecHourTBL[i] ;
                if(pRecHourTBL[i] != 0)
                {
                    Lastvalue = i ;
                }
            }
            pRecHourTBL[Lastvalue] = 0 ;

            CLOSE_BKT(fd);
        }
    }

    return 0;
}

long DVR_GetLastRecTime(void)
{
    struct dirent *ent;
    DIR *dp;

    char buf[30];
    sprintf(buf, "%s/rdb", DEFAULT_DISK_MPOINT1);
    dp = opendir(buf);

    if (dp != NULL)
    {
        int last_rdb=0;

        for(;;)
        {
            ent = readdir(dp);
            if (ent == NULL)
                break;

            if (ent->d_name[0] == '.')
            {
                if (!ent->d_name[1] || (ent->d_name[1] == '.' && !ent->d_name[2]))
                {
                    continue;
                }
            }

            if(last_rdb==0)
                last_rdb = atoi(ent->d_name);
            else
                last_rdb = max(atoi(ent->d_name), last_rdb);

        }

        closedir(dp);

        if(last_rdb != 0)
        {
            int m, ch;
            int bFoundFirst=FALSE;
            #ifdef BKT_SYSIO_CALL
            int fd;
            #else
            FILE *fd;
            #endif
            char path[LEN_PATH];

            sprintf(path, "%s/rdb/%08d", DEFAULT_DISK_MPOINT1, last_rdb);

            if(!OPEN_RDONLY(fd, path))
            {
                eprintf("failed open rdb %s\n", path);
                return BKT_ERR;
            }

            char evts[TOTAL_MINUTES_DAY*BKT_MAX_CHANNEL];

            if(!READ_PTSZ(fd, evts, sizeof(evts)))
            {
                eprintf("failed read rec data\n");
                CLOSE_BKT(fd);
                return BKT_ERR;
            }
            CLOSE_BKT(fd);

            for(m=TOTAL_MINUTES_DAY-1;m>=0;m--)
            {
                for(ch=0;ch<BKT_MAX_CHANNEL;ch++)
                {
                    if(evts[TOTAL_MINUTES_DAY*ch+m] != 0)
                    {
                        if(bFoundFirst == TRUE)
                        {
                            char buff[64];
                            struct tm tm1;
                            time_t t1;
                            int yy = last_rdb/10000;
                            int mm = (last_rdb%10000)/100;
                            int dd = (last_rdb%10000)%100;

                            int HH = m/60;
                            int MM = m%60;

                            sprintf(buff, "%04d-%02d-%02d %02d:%02d:00", yy, mm, dd, HH, MM);
                            strptime(buff, "%Y-%m-%d %H:%M:%S", &tm1);

                            t1 = mktime(&tm1);

                            dprintf("get last -1 rec data. CH:%d, %04d-%02d-%02d %02d:%02d:00\n", ch, yy, mm, dd, HH, MM);

                            return t1;
                        }
                        bFoundFirst = TRUE;

                        dprintf("found real last rec time. %02d:%02d\n", m/60, m%60);
                    }
                }
            }
        }
    }

    dprintf("failed get last record time\n");
    return BKT_ERR;
}

////MULTI-HDD SKSUNG////
int DVR_FormatHDD(char* mntpath, char* devpath, int hddstatus)
{
    int status = 0;

    char buf[50];
    char* tmpName;

    WRITER_fileSaveExit();


    ////fdisk process/////

    if(hddstatus == DISK_IDLE)
    {
        if( dvr_fdisk(devpath) == -1 )
        {
            eprintf("perror says(fdisk) failed devpath: %s \n", devpath);
            return 0;
        }

        ////change dev path ex> /dev/sda -> /dev/sda1
        memset(buf, 0, 50);
        sprintf(buf, "%s1", devpath);
        strcpy(devpath, buf);

        hddstatus = DONE_FDISK;
    }

    if(hddstatus >= DONE_FDISK)
    {
        ////change dev path to mount path ex> dev/sda1 -> media/sda1
        if(hddstatus == DONE_FDISK)
        {
/*
			memset(buf, 0, 50) ;
        	sprintf(buf, "mkswap %s",devpath) ;
        	system_user(buf) ;
        	sleep(1) ;

        	memset(buf, 0, 50) ;
        	sprintf(buf, "mkswapon %s",devpath) ;
        	system_user(buf) ;
*/

            memset(buf, 0, 50);
            strcpy(buf, devpath);

            tmpName = strtok(buf, "/");
            while(tmpName != NULL)
            {
                strcpy(buf, tmpName);
                tmpName = strtok(NULL, "/");
            }

            sprintf(mntpath, "/media/%s", buf);
        }

        ////unmount process////
		fcloseall(); // ???
        sync();
        sleep(1);

		if( umount(mntpath) < 0){
			perror("failed umount");
			if( umount2(mntpath, MNT_FORCE) < 0)
			{
				perror("failed umount2");
				eprintf("perror says(format) failed umount mntpath: %s  devpath: %s status: %02d\n", mntpath, devpath, hddstatus);
				if(hddstatus == DONE_EXT3)
					return 0;
			}
        }

        sync();
        sleep(1);

        ////format process////
        memset(buf, 0, 50);
//        sprintf(buf, "mke2fs -T ext3 %s", devpath);
		sprintf(buf, "mke2fs -T ext3,largefile4 %s", devpath);
        system_user(buf);

        sync();

        ////mount process////
        if(hddstatus == DONE_FDISK)
        {
            memset(buf, 0, 50);
            sprintf(buf, "mkdir %s", mntpath);
            system_user(buf);
        }

        if(mount(devpath, mntpath, "ext3", 0, NULL ) < 0)
        {
            eprintf("mount error %s\n", mntpath);
            return FALSE;
        }

        sync();
        sleep(1);


        ///multi hdd update////
        WRITER_updateMountInfo();

        hddstatus = DONE_FORMAT;
    }

    //////BAKSET Create process/////
    if(hddstatus >= DONE_FORMAT && gbkt_mount_info.hddCnt < MAX_HDD_COUNT)
    {
        status = DVR_Basket_Create(mntpath) ;
        if(status < 0)
        {
            eprintf("Basket Create Fail!!! %s\n", mntpath);
        }
    }

    return 1;
}

int DVR_GetDiskInfo(dvr_disk_info_t* idisk)
{
    int ret=0, i, j, matchCnt;

    ret = get_dvr_disk_info(idisk);
    if(ret<0)
        return -1;

    if(idisk->cnt > 0)
    {
        ///multi hdd update////
        WRITER_updateMountInfo();

        memset(idisk->rec_used, 0, sizeof(idisk->rec_used));
        matchCnt = 0;
        for(i = 0; i < idisk->cnt; i++)
        {
            if((strlen(idisk->dev[i].dir)) && (gbkt_mount_info.hddCnt > 0))
            {
                for(j = 0; j < MAX_HDD_COUNT; j++)
                {
                    if(gbkt_mount_info.hddInfo[j].isMounted)
                    {
                        if(strcmp(idisk->dev[i].dir, gbkt_mount_info.hddInfo[j].mountDiskPath) == 0)
                        {
                            matchCnt++;
                            idisk->rec_used[i] = j+1;
                            if(matchCnt == gbkt_mount_info.hddCnt)
                                return 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

int DVR_GetRecDiskSize(unsigned long *total, unsigned long *used)
{
    if(gbkt_mount_info.hddCnt > 0)
    {
        #if 0
        printf("\n");
        printf("=================================================\n");
        printf("           GET DISK SIZE INFORMATION             \n");
        printf("=================================================\n");
        printf(" MNT-IDX      PATH       TOT-SIZE    USED-SZIE \n");
        printf("=================================================\n");
        #endif

        int i, ret;

        for(i = 0; i < MAX_HDD_COUNT; i++)
        {
            if(gbkt_mount_info.hddInfo[i].isMounted)
            {
                unsigned long mntTot, mntUsed;
                ret = get_dvr_disk_size(gbkt_mount_info.hddInfo[i].mountDiskPath, &mntTot, &mntUsed);
                if(ret != -1)
                {
//                  printf("    %02d    %s   %d   %d\n", i, gbkt_mount_info.hddInfo[i].mountDiskPath, mntTot, mntUsed);
                    *total = *total + mntTot;
                    *used = *used + (mntUsed-gbkt_mount_info.hddInfo[i].initUsedSize);
                }
            }
        }
        #if 0
        printf("=================================================\n");
        printf("               %d     %d\n", *total, *used);
        printf("\n");
        #endif
    }

    return 0;
}

int DVR_BasketInfo(long *bkt_count, long *bkt_size)
{
    return BKTREC_TotalBasketInfo( bkt_count, bkt_size ) ;
}

int DVR_Basket_Create(char *diskpath)
{
    int basket_cnt = 0 ;

    basket_cnt = BKTREC_CreateBasket(diskpath) ;

    sync() ;

    ////MULTI-HDD SKSUNG////
    if(basket_cnt > 0)
        BKTREC_getTargetDisk();

    printf("app: basket creation ... done %d\n",basket_cnt) ;

    return basket_cnt ;
}

int DVR_SetRecordingType(int iRecycle)
{
    return BKT_SetRecycleMode(iRecycle);
}

int DVR_set_encoder_request_key(int ch)
{
    int rv = 0;

//  rv = ENCODE_ctrl_req_key(pHandleParam, ch);

    return rv;
}

int DVR_get_encoder_request_key(int ch, int *pIsEnabled)
{
    int rv = 0;

//  rv = ENCODE_get_req_key_status(pHandleParam, ch, pIsEnabled);

    return rv;
}


int DVR_set_encoder_resolution(int ch, int width, int height)
{
    int rv = 0;

//  rv = ENCODE_ctrl_resolution(pHandleParam, ch, width, height);

    return rv;
}


int DVR_get_encoder_resolution(int ch, int *pWidth, int *pHeight)
{
    int rv = 0;
    Int32 status;
    VCODEC_BITSBUF_LIST_S bitsBuflist;
    status = Venc_getBitstreamBuffer(&bitsBuflist, 0);
    if(status==ERROR_NONE && bitsBuflist.numBufs)
    {
        *pWidth = bitsBuflist.bitsBuf[0].frameWidth;
        *pHeight = bitsBuflist.bitsBuf[0].frameHeight;
    }
    else
    {
        return -1;
    }

    return rv;
}

int DVR_set_encoder_fps(int ch, int fps)
{
    int rv = 0, palMode;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS && (fps >= 0 && fps <= FRAMERATE_04_03))
    {
		if(gCaptureInfo.chStatus[0].frameHeight == 288)
			palMode = 1;
		else
			palMode = 0;

    	switch(fps)
   		{
   			case FRAMERATE_30_25:
				if(!palMode)
					fps = 30*1000;
				else
					fps = 25*1000;
				break;
			case FRAMERATE_15_13:
				if(!palMode)
					fps = 15*1000;
				else
					fps = 13*1000;
				break;
			case FRAMERATE_08_06:
				if(!palMode)
					fps = 8*1000;
				else
					fps = 6*1000;
				break;
			case FRAMERATE_04_03:
				if(!palMode)
					fps = 4*1000;
				else
					fps = 3*1000;
				break;
			default:
				if(!palMode)
					fps = 30*1000;
				else
					fps = 25*1000;
				break;
   		}
        params.frameRate = fps;
        rv = Venc_setDynamicParam(ch, 0, &params, VENC_FRAMERATE);
    }

    return rv;
}

int DVR_get_encoder_fps(int ch, int *pFps)
{
    int rv = 0;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS)
    {
        Venc_getDynamicParam(ch, 0, &params, VENC_FRAMERATE);
        *pFps   = params.frameRate;
    }

    return rv;
}


int DVR_set_encoder_bps(int ch, int bps)
{
    int rv = 0;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS && (bps >= 64000 && bps <= 4000000))
    {
        params.targetBitRate = bps;
        rv = Venc_setDynamicParam(ch, 0, &params, VENC_BITRATE);
    }

    return rv;
}

int DVR_get_encoder_bps(int ch, int *pBps)
{
    int rv = 0;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS)
    {
        Venc_getDynamicParam(ch, 0, &params, VENC_BITRATE);
        *pBps   = params.targetBitRate;
    }

    return rv;
}

int DVR_set_encoder_iframeinterval(int ch, int value)
{
    int rv = 0;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS)
    {
        switch(value)
        {
            case IFRAME_INTERVAL_30:
                params.intraFrameInterval = 30;
                break;
            case IFRAME_INTERVAL_15:
                params.intraFrameInterval = 15;
                break;
            case IFRAME_INTERVAL_10:
                params.intraFrameInterval = 10;
                break;
            case IFRAME_INTERVAL_5:
                params.intraFrameInterval = 5;
                break;
            case IFRAME_INTERVAL_1:
                params.intraFrameInterval = 1;
                break;
        }
        rv = Venc_setDynamicParam(ch, 0, &params, VENC_IPRATIO);
    }

    return rv;
}

int DVR_get_encoder_iframeinterval(int ch, int *value)
{
    int rv = 0;
    VENC_CHN_DYNAMIC_PARAM_S params = { 0 };

    if(ch < MAX_DVR_CHANNELS)
    {
        Venc_getDynamicParam(ch, 0, &params, VENC_IPRATIO);
        *value  = params.intraFrameInterval;
    }

    return rv;
}

int DVR_set_encoder_codectype(int ch, int codectype)
{
    int rv = 0;

    return rv;
}

int DVR_get_encoder_codectype(int ch, int *codectype)
{
    int rv = 0;

    return rv;
}

int DVR_SetBrightness(int nChannelIndex, int ibrightness)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
	memset(&CapChnDynaParam,0,sizeof(CapChnDynaParam));
	printf("%s ch=%d val=%d\n",__FUNCTION__,nChannelIndex,ibrightness);
	CapChnDynaParam.brightness = ibrightness;

	//if(CapChnDynaParam.brightness != 0)
	{
		rv = Vcap_setDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_BRIGHTNESS);
	}
	return rv;
}
int DVR_SetContrast(int nChannelIndex, int icontrast)
{
	int rv = 0;
	VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
	memset(&CapChnDynaParam,0,sizeof(CapChnDynaParam));
	printf("%s ch=%d val=%d\n",__FUNCTION__,nChannelIndex,icontrast);
	CapChnDynaParam.contrast = icontrast;

	//if(CapChnDynaParam.contrast != 0)
	{
		rv = Vcap_setDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_CONTRAST);
	}
	return rv;
}
int DVR_SetSaturation(int nChannelIndex, int isaturation)
{
	int rv = 0;
	VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
	memset(&CapChnDynaParam,0,sizeof(CapChnDynaParam));
	printf("%s ch=%d val=%d\n",__FUNCTION__,nChannelIndex,isaturation);
	CapChnDynaParam.satauration = isaturation;

	//if(CapChnDynaParam.satauration != 0)
	{
		rv = Vcap_setDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_SATURATION);
	}
	return rv;
}
int DVR_SetHue(int nChannelIndex, int ihue)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
    printf("%s ch=%d val=%d\n",__FUNCTION__,nChannelIndex,ihue);
    Vcap_getDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_HUE);
    if(CapChnDynaParam.hue == ihue)
    {
        return 0;
    }
    CapChnDynaParam.hue = ihue-CapChnDynaParam.hue;
    if(CapChnDynaParam.hue != 0)
    {
        rv = Vcap_setDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_HUE);
    }
    return rv;
}
int DVR_SetSharpness(int nChannelIndex, int isharpness)
{
    int rv = 0;
    return rv;
}

int DVR_GetBrightness(int nChannelIndex, int *ibrightness)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
    rv=Vcap_getDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_BRIGHTNESS);
    *ibrightness = CapChnDynaParam.brightness;
    return rv;
}
int DVR_GetContrast(int nChannelIndex, int *icontrast)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
    rv=Vcap_getDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_CONTRAST);
    *icontrast = CapChnDynaParam.contrast;
    return rv;
}
int DVR_GetSaturation(int nChannelIndex, int *isaturation)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
    rv = Vcap_getDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_SATURATION);
    *isaturation = CapChnDynaParam.satauration;
    return rv;
}
int DVR_GetHue(int nChannelIndex, int *ihue)
{
    int rv = 0;
    VCAP_CHN_DYNAMIC_PARAM_S CapChnDynaParam;
    rv = Vcap_getDynamicParamChn(nChannelIndex, &CapChnDynaParam, VCAP_HUE);
    *ihue = CapChnDynaParam.hue;
    return rv;
}
int DVR_GetSharpness(int nChannelIndex, int *isharpness)
{
    int rv = 0;
    return rv;
}
int DVR_GetNoiseReduction(int nChannelIndex, int *inoisereduction)
{
    int rv = 0;
    return rv;
}
int DVR_GetDeInterlace(int nChannelIndex, int *ideinterlace)
{
    int rv = 0;
    return rv;
}

int DVR_SetCameraEnable(int chId, int bEnable)
{
    int rv=0;
	if(chId < MAX_CH)
	{
		if(bEnable)
		{
			rv = Vcap_enableChn(chId, 1);
			Vdis_enableChn(gLiveDspId,chId);
		}
		else
		{
			rv = Vcap_disableChn(chId, 1);
			Vdis_disableChn(gLiveDspId,chId);
		}
	}
	else
	{
		if(bEnable)
		{
			rv = Vdis_enableChn(gPlayDspId,chId);
		}
		else
		{
			rv = Vdis_disableChn(gPlayDspId,chId);
		}
	}

    return rv;
}

int DVR_SetCameraLayout(int iOutput, int nStartChannelIndex, int iLayoutMode, int m_iCameraSelectId)
{
    int rv=0;
	int i,idx=0;
//	UInt32 chMap[VDIS_MOSAIC_WIN_MAX];
	ST_MOSAIC_INFO* pMosaicInfo;
	VDIS_MOSAIC_S sVdMosaicParam;
	AUDIO_STATE *pAudstate = &gAudio_state ;

	pMosaicInfo = &gMosaicInfo_Live;
	//click the playback button, maintain live video before playback start 
	if((gInitSettings.layout.iMainOutput == gInitSettings.layout.iSubOutput) && (nStartChannelIndex >= MAX_CH))
	{
		pMosaicInfo = &gMosaicInfo_Play;
		if(pAudstate->state == LIB_LIVE_MODE)
			return rv;
	}

	Vdis_getMosaicParams(iOutput, &sVdMosaicParam);
	for(i=0; i<pMosaicInfo->numWin; i++)
		pMosaicInfo->chMap[i]=sVdMosaicParam.chnMap[i];

	for(i=0; i<pMosaicInfo->numWin; i++)
	{
		idx = nStartChannelIndex  + i;
		if((nStartChannelIndex<MAX_CH && idx<MAX_CH) || (nStartChannelIndex>=MAX_CH && idx<VDIS_CHN_MAX))
		{
			pMosaicInfo->chMap[i] = idx;
		}
		if(m_iCameraSelectId !=0xff)
		{
			if(pMosaicInfo->chMap[i] == m_iCameraSelectId)
			{
				pMosaicInfo->chMap[i] = VDIS_CHN_INVALID;
			}
		}
	}

	if(m_iCameraSelectId !=0xff)
	{
		pMosaicInfo->chMap[i] = m_iCameraSelectId;
	}
	
	if(nStartChannelIndex < MAX_CH)
	{
		//Int32 i;

		for (i=0; i<sVdMosaicParam.numberOfWindows; i++)
		{
			/* At capture level,YUV422 D1 stream id is 0. Set for D1 channels 0 ~ MAX_CH */
			Vcap_setFrameRate(pMosaicInfo->chMap[i], 0, 60, 0);

			if((pMosaicInfo->chMap[i] < Vcap_getNumChannels()) && (sVdMosaicParam.useLowCostScaling[i] == FALSE))
				Vcap_setFrameRate(pMosaicInfo->chMap[i],0,60,30);

			/* At capture level,CIF stream id is 2. Set for CIF channels 0 ~ MAX_CH */
			Vcap_setFrameRate(pMosaicInfo->chMap[i], 2, 30, 30);
		}
	}

	rv = Vdis_setMosaicChn(iOutput, pMosaicInfo->chMap);
	return rv;
}

int DVR_SetCovert(int nChannelIndex, int bEnable)
{
	int rv=0;
	if(nChannelIndex < MAX_CH)
	{
		if(bEnable)
		{
			rv = Vdis_disableChn(gLiveDspId,nChannelIndex);
		}
		else
		{
			rv = Vdis_enableChn(gLiveDspId,nChannelIndex);
		}
	}
	return rv;
}

//Cam-Property PIP//
VDIS_MOSAIC_S sBackVdMosaicParam;
void DVR_startCamProperty(int selectedCh, int startX, int startY, int width, int height)
{
	int i;
	
	if(gMosaicInfo_Live.isCamPropMode == 0)
	{
		VDIS_MOSAIC_S vdMosaicParam;

		gMosaicInfo_Live.selectedCh		= selectedCh;
		gMosaicInfo_Live.startX 		= startX;
		gMosaicInfo_Live.startY 		= startY;
		gMosaicInfo_Live.width			= width;
		gMosaicInfo_Live.height			= height;
		gMosaicInfo_Live.isCamPropMode 	= 1;

		Vdis_getMosaicParams(gLiveDspId, &vdMosaicParam);
		Vdis_getMosaicParams(gLiveDspId, &sBackVdMosaicParam);

		Int winIdMax = vdMosaicParam.numberOfWindows;
		for(i=0;i<winIdMax;i++)
		{
			if(vdMosaicParam.chnMap[i] == gMosaicInfo_Live.selectedCh)
			{
				vdMosaicParam.chnMap[i] = VDIS_CHN_INVALID;
				break;
			}
		}

		vdMosaicParam.numberOfWindows++;
		vdMosaicParam.winList[winIdMax].width 	 	= gMosaicInfo_Live.width;
		vdMosaicParam.winList[winIdMax].height	 	= gMosaicInfo_Live.height;
		vdMosaicParam.winList[winIdMax].start_X 	= gMosaicInfo_Live.startX;
		vdMosaicParam.winList[winIdMax].start_Y 	= gMosaicInfo_Live.startY;
		vdMosaicParam.chnMap[winIdMax]			 	= gMosaicInfo_Live.selectedCh;
		vdMosaicParam.useLowCostScaling[winIdMax] 	= 1;
		
	    Vdis_setMosaicParams(gLiveDspId, &vdMosaicParam);
	}
}

void DVR_endCamProperty()
{
	int i;
	if(gMosaicInfo_Live.isCamPropMode == 1)
	{
		gMosaicInfo_Live.isCamPropMode = 0;
		Vdis_setMosaicParams(gLiveDspId, &sBackVdMosaicParam);
	}
}

void DVR_selCamPropCh(int nCh)
{
	int i;

	if(gMosaicInfo_Live.isCamPropMode == 1)
	{
		VDIS_MOSAIC_S vdMosaicParam;
		Vdis_getMosaicParams(gLiveDspId, &vdMosaicParam);
		Int winIdMax = vdMosaicParam.numberOfWindows-1;
		for(i=0;i<winIdMax;i++)
		{		
			if(vdMosaicParam.chnMap[i] == VDIS_CHN_INVALID && sBackVdMosaicParam.chnMap[i] != VDIS_CHN_INVALID)
			{
				vdMosaicParam.chnMap[i] = sBackVdMosaicParam.chnMap[i];
				break;
			}
		}

		gMosaicInfo_Live.selectedCh = nCh;

		for(i=0;i<winIdMax;i++)
		{
			if(vdMosaicParam.chnMap[i] == gMosaicInfo_Live.selectedCh)
			{
				vdMosaicParam.chnMap[i] = VDIS_CHN_INVALID;
				break;
			}
		}

		vdMosaicParam.chnMap[winIdMax]			 	= gMosaicInfo_Live.selectedCh;
		vdMosaicParam.useLowCostScaling[winIdMax] 	= 1;
	    Vdis_setMosaicParams(gLiveDspId, &vdMosaicParam);
	}
}

UInt32 DVR_GetVideoLossStatus()
{
    UInt32 chId, VlossStatus;

    VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
    VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;

    Vcap_getVideoSourceStatus( &videoSourceStatus );

	VlossStatus = 0;
    for(chId=0; chId<videoSourceStatus.numChannels; chId++)
    {
        pChStatus = &videoSourceStatus.chStatus[chId];

        if(!pChStatus->isVideoDetect)
        {
        	VlossStatus += (1 << chId);
        }
    }
    return VlossStatus;

}

int DVR_GetSourceStatus()
{
	int rv=0;
	UInt32 chId;

	VCAP_VIDEO_SOURCE_STATUS_S videoSourceStatus;
    VCAP_VIDEO_SOURCE_CH_STATUS_S *pChStatus;

    Vcap_getVideoSourceStatus( &videoSourceStatus );
	gCaptureInfo.chCount = videoSourceStatus.numChannels;

    for(chId=0; chId<videoSourceStatus.numChannels; chId++)
    {
        pChStatus = &videoSourceStatus.chStatus[chId];

        if(pChStatus->isVideoDetect)
        {
        	gCaptureInfo.chStatus[chId].frameWidth = pChStatus->frameWidth;
        	if(pChStatus->isInterlaced)
        		gCaptureInfo.chStatus[chId].frameHeight = pChStatus->frameHeight;
        	else
        		gCaptureInfo.chStatus[chId].frameHeight = pChStatus->frameHeight;
        }
        else
        {
            printf (" [DVR_APP]: CH%2d: No video detected at [vipID:%d, vipCH: %d] !!!\n",
                 chId, pChStatus->vipInstId, pChStatus->chId);

        }

        printf("video detect ch%02d res[%dx%d]\n", chId, gCaptureInfo.chStatus[chId].frameWidth,gCaptureInfo.chStatus[chId].frameHeight);
    }

	return rv;
}


int DVR_SetBitrateType(int chId, int bitrateType)
{
	VENC_CHN_DYNAMIC_PARAM_S params;
	int type;
	if(bitrateType == BITRATE_TYPE_CBR)
	{
		type = VENC_RATE_CTRL_CBR;
	}
	else
	{
		type = VENC_RATE_CTRL_VBR;
	}
	params.rcAlg = type;
	return Venc_setDynamicParam(chId,0,&params,VENC_RCALG);
}

int DVR_SetDisplayRes(int displayId, int resType)
{
	int rv = 0,i;
	UInt32 layoutId, resolution, startCh = 0;
	Bool validRes = FALSE;
    VDIS_MOSAIC_S vdisMosaicParams;
    UInt32 outWidth, outHeight;
    int devId;

	printf("DVR_SetDisplayRes displayId = %d resType = %d \n", displayId, resType) ;

	switch(displayId)
	{
		case DISP_PATH_HDMI0:
			devId = VDIS_DEV_HDMI;
			break;
		case DISP_PATH_HDMI1:
			devId = VDIS_DEV_DVO2;
			break;
		case DISP_PATH_VGA:
			devId = VDIS_DEV_HDCOMP;
			break;
		case DISP_PATH_SD:
			devId = VDIS_DEV_SD;
			break;
	}
	if(displayId == gInitSettings.layout.iMainOutput)
	{
		gInitSettings.maindisplayoutput.iResolution=resType;
		Dvr_swMsGetOutSize(gInitSettings.maindisplayoutput.iResolution, &outWidth, &outHeight);
	}
	else if(displayId == gInitSettings.layout.iSubOutput)
	{
		gInitSettings.subdisplayoutput.iResolution=resType;
		Dvr_swMsGetOutSize(gInitSettings.subdisplayoutput.iResolution, &outWidth, &outHeight);
	}
	switch(resType)
	{
		case DC_MODE_1080I_60:
			if(devId != VDIS_DEV_SD)
			{
				resolution 	= VSYS_STD_1080I_60;
				validRes 	= TRUE;
			}
			break;
		case DC_MODE_1080P_60:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_1080P_60;
				validRes = TRUE;
			}
			break;
		case DC_MODE_1080P_50:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_1080P_50;
				validRes = TRUE ;
			}
			break;
		case DC_MODE_720P_60:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_720P_60;
				validRes = TRUE;
			}
			break;
		case DC_MODE_NTSC:
			if(devId == VDIS_DEV_SD)
			{
				resolution = VSYS_STD_NTSC;
				validRes = TRUE;
			}
			break;
		case DC_MODE_PAL:
			if(devId == VDIS_DEV_SD)
			{
				resolution = VSYS_STD_PAL;
				validRes = TRUE;
			}
			break;
		case DC_MODE_XGA_60:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_XGA_60;
				validRes = TRUE;
			}
			break;
		case DC_MODE_SXGA_60:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_SXGA_60;
				validRes = TRUE;
			}
			break ;
		case DC_MODE_480P:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_480P;
				validRes = TRUE ;
			}
			break;
		case DC_MODE_576P:
			if(devId != VDIS_DEV_SD)
			{
				resolution = VSYS_STD_576P;
				validRes = TRUE ;
			}
			break;

	}
	if(validRes)
	{
		if(displayId == gInitSettings.layout.iMainOutput)
		{
			layoutId = gMosaicInfo_Live.layoutId;
			gInitSettings.maindisplayoutput.iResolution = resType;
			/* Disable graphics through sysfs entries */
			if(devId == VDIS_DEV_HDMI)
			{
				Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);

		    }
		    else if(devId == VDIS_DEV_DVO2)
		  	{
				Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);

		  	}
		}
		else
		{
			startCh = MAX_CH;
			layoutId = gMosaicInfo_Play.layoutId;
			gInitSettings.subdisplayoutput.iResolution = resType;
            if(devId == VDIS_DEV_HDMI)
            {
				Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);

            }
            else if(devId == VDIS_DEV_DVO2)
            {
			    Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);
            }
        }
        Vdis_stopDrv(devId);
        memset(&vdisMosaicParams, 0, sizeof(VDIS_MOSAIC_S));

        /* Start with default layout */
        DVR_swMsGenerateLayouts(displayId, startCh, MAX_CH*2, layoutId, &vdisMosaicParams, FALSE, resType, 0);
		if(displayId == gInitSettings.layout.iMainOutput)
		{
	    	for (i=0; i<Vcap_getNumChannels(); i++)
	      		Vcap_setFrameRate(i, 0, 60, 0);

	    	for (i=0; i<vdisMosaicParams.numberOfWindows; i++)
	    	{
	            if((vdisMosaicParams.chnMap[i] < Vcap_getNumChannels()) && (vdisMosaicParams.useLowCostScaling[i] == FALSE))
	                Vcap_setFrameRate(vdisMosaicParams.chnMap[i],0,60,30);

	    	}
	  	}
        Vdis_setMosaicParams(displayId*2, &vdisMosaicParams);
        Vdis_setResolution(devId, resolution);

        Vdis_startDrv(devId);


        if(displayId == gInitSettings.layout.iMainOutput)
        {
            /* Enable graphics through sysfs entries */
            DVR_FBScale(devId, 0, 0, outWidth, outHeight);
            if(devId == VDIS_DEV_HDMI)
            {
				Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);

            }
            else if(devId == VDIS_DEV_DVO2)
            {
				Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_ON);

            }
        }
        if(displayId == gInitSettings.layout.iSubOutput)
        {
            /* Enable graphics through sysfs entries */
            DVR_FBScale(devId, 0, 0, outWidth, outHeight);

        }
    }
    else
    {
        rv = ERROR_FAIL;
    }
    return rv;
}

int DVR_SetSpotChannel(int chId)
{
	int rv = ERROR_FAIL;

	if(chId >= 0 && chId < MAX_CH)
		rv = Vdis_switchSDTVChId(VDIS_DEV_SD, chId);

	return rv;
}

int DVR_displaySwitchChn(UInt32 devId, UInt32 startChId)
{
    UInt32 rv, chMap[VDIS_MOSAIC_WIN_MAX];
    int i;

    for(i=0;i<VDIS_MOSAIC_WIN_MAX;i++)
    {
         if (i < MAX_CH*2)
            chMap[i] = (i+startChId)%(MAX_CH*2);
         else
            chMap[i] = VDIS_CHN_INVALID;

    }

    rv = Vdis_setMosaicChn(devId, chMap);

    /* wait for the info prints to complete */
    OSA_waitMsecs(100);

	return rv;
}

void DVR_VdisStartDrv(char displayId)
{

    int displaydevId;
    switch(displayId)
    {
         case DISP_PATH_HDMI0:
             displaydevId = VDIS_DEV_HDMI;
             break;
         case DISP_PATH_HDMI1:
             displaydevId = VDIS_DEV_DVO2;
             break;
         case DISP_PATH_VGA:
             displaydevId = VDIS_DEV_HDCOMP;
             break;
         case DISP_PATH_SD:
             displaydevId = VDIS_DEV_SD;
             break;
	  default:
	  	displaydevId = VDIS_DEV_HDMI;
             break;
    }    

    if(displayId == DISP_PATH_HDMI0)
    {
        sprintf(gBuff, "%d", (VDIS_SYSFS_HDMI|VDIS_SYSFS_HDCOMP));
        /* Tie HDMI and HDCOMP from A8 side */
        Vdis_sysfsCmd(2,VDIS_SYSFSCMD_SETTIEDVENCS, gBuff);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_ON);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_ON);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);

    }
    else if (displayId == DISP_PATH_HDMI1)
    {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_ON);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_ON);
    }
    Vdis_startDrv(displaydevId);
}

void DVR_VdisStopDrv(char displayId)
{
    int displaydevId;
    switch(displayId)
    {
         case DISP_PATH_HDMI0:
             displaydevId = VDIS_DEV_HDMI;
             break;
         case DISP_PATH_HDMI1:
             displaydevId = VDIS_DEV_DVO2;
             break;
         case DISP_PATH_VGA:
             displaydevId = VDIS_DEV_HDCOMP;
             break;
         case DISP_PATH_SD:
             displaydevId = VDIS_DEV_SD;
             break;
	   default:
	   	displaydevId = VDIS_DEV_HDMI;
             break;
    }    

    Vdis_stopDrv(displaydevId);

    if(displayId == DISP_PATH_HDMI0)
    {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDMI, VDIS_OFF);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_HDCOMP, VDIS_OFF);
    }
    else if (displayId == DISP_PATH_HDMI1)
    {
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF); 
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SETVENC, VDIS_SYSFS_DVO2, VDIS_OFF);
    }

}

int DVR_SwitchDisplay(int mainDspId, int subDspId, int spotDspId, int mainResolution, int subResolution, int spotResolution)
{
    int rv = 0, i, outWidth, outHeight, liveDevId, playDevId, spotDevId, mainDspRes, subDspRes, spotDspRes;
    AUDIO_STATE *pAudstate = &gAudio_state ;
    VDIS_MOSAIC_S vdMosaicParam;

    switch(mainDspId)
    {
         case DISP_PATH_HDMI0:
             liveDevId = VDIS_DEV_HDMI;
             break;
         case DISP_PATH_HDMI1:
             liveDevId = VDIS_DEV_DVO2;
             break;
         case DISP_PATH_VGA:
             liveDevId = VDIS_DEV_HDCOMP;
             break;
         case DISP_PATH_SD:
             liveDevId = VDIS_DEV_SD;
             break;
	   default:
             liveDevId = VDIS_DEV_HDMI;
		break;
    }    
    
    switch(subDspId)
    {
         case DISP_PATH_HDMI0:
             playDevId = VDIS_DEV_HDMI;
             break;
         case DISP_PATH_HDMI1:
             playDevId = VDIS_DEV_DVO2;
             break;
         case DISP_PATH_VGA:
             playDevId = VDIS_DEV_HDCOMP;
             break;
         case DISP_PATH_SD:
             playDevId = VDIS_DEV_SD;
             break;
	   default:
             liveDevId = VDIS_DEV_DVO2;
		break;
    }     

    switch(spotDspId)
    {
         case DISP_PATH_HDMI0:
             spotDevId = VDIS_DEV_HDMI;
             break;
         case DISP_PATH_HDMI1:
             spotDevId = VDIS_DEV_DVO2;
             break;
         case DISP_PATH_VGA:
             spotDevId = VDIS_DEV_HDCOMP;
             break;
         case DISP_PATH_SD:
             spotDevId = VDIS_DEV_SD;
             break;
         default:   
             spotDevId = VDIS_DEV_SD;
             break;
    }

    gInitSettings.maindisplayoutput.iResolution=mainResolution;
    gInitSettings.subdisplayoutput.iResolution=subResolution;
    gInitSettings.spotdisplayoutput.iResolution=spotResolution;

    switch(gInitSettings.maindisplayoutput.iResolution)
    {
        case DC_MODE_1080I_60:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes  = VSYS_STD_1080I_60;
            }
            break;
        case DC_MODE_1080P_60:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_1080P_60;
            }
            break;
        case DC_MODE_1080P_50:
            if(liveDevId!= VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_1080P_50;
            }
            break;
        case DC_MODE_720P_60:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_720P_60;
            }
            break;
        case DC_MODE_NTSC:
            if(liveDevId == VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_NTSC;
            }
            break;
        case DC_MODE_PAL:
            if(liveDevId == VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_PAL;
            }
            break;          
        case DC_MODE_XGA_60:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_XGA_60;
            }
            break;
        case DC_MODE_SXGA_60:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_SXGA_60;
            }
            break;
        case DC_MODE_480P:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_480P;
            }
            break;
        case DC_MODE_576P:
            if(liveDevId != VDIS_DEV_SD)
            {
                mainDspRes = VSYS_STD_576P;
            }
            break;
    
    }

    switch(gInitSettings.subdisplayoutput.iResolution)
    {
        case DC_MODE_1080I_60:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes  = VSYS_STD_1080I_60;
            }
            break;
        case DC_MODE_1080P_60:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_1080P_60;
            }
            break;
        case DC_MODE_1080P_50:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_1080P_50;
            }
            break;
        case DC_MODE_720P_60:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_720P_60;
            }
            break;
        case DC_MODE_NTSC:
            if(playDevId == VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_NTSC;
            }
            break;
        case DC_MODE_PAL:
            if(playDevId == VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_PAL;
            }
            break;          
        case DC_MODE_XGA_60:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_XGA_60;
            }
            break;
        case DC_MODE_SXGA_60:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_SXGA_60;
            }
            break;
        case DC_MODE_480P:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_480P;
            }
            break;
        case DC_MODE_576P:
            if(playDevId != VDIS_DEV_SD)
            {
                subDspRes = VSYS_STD_576P;
            }
            break;
    
    }

    switch(gInitSettings.spotdisplayoutput.iResolution)
    {
        case DC_MODE_NTSC:
            if(spotDevId == VDIS_DEV_SD)
            {
                spotDspRes = VSYS_STD_NTSC;
            }
            break;
        case DC_MODE_PAL:
            if(spotDevId == VDIS_DEV_SD)
            {
                spotDspRes = VSYS_STD_PAL;
            }
            break;          
        default:
            spotDspRes = VSYS_STD_PAL;
            break;            
    }
    
    if((mainDspId != subDspId))
    {

        gInitSettings.layout.iMainOutput = mainDspId;
        gInitSettings.layout.iSubOutput = subDspId;          
        gLiveDspId = mainDspId;
        gPlayDspId = subDspId;


        printf("gInitSettings.maindisplayoutput.iResolution: %d\n", gInitSettings.maindisplayoutput.iResolution);
        printf("gInitSettings.subdisplayoutput.iResolution : %d\n", gInitSettings.subdisplayoutput.iResolution);
        printf("gInitSettings.spotdisplayoutput.iResolution: %d\n", gInitSettings.spotdisplayoutput.iResolution);        
        printf("gInitSettings.layout.iMainOutput           : %d\n", gInitSettings.layout.iMainOutput);
        printf("gInitSettings.layout.iSubOutput            : %d\n", gInitSettings.layout.iSubOutput);
        printf("mainDspId : %d\n", mainDspId);
        printf("subDspId  : %d\n", subDspId);
        printf("spotDspId : %d\n", spotDspId);
        printf("liveDevId : %d\n", liveDevId);
        printf("playDevId : %d\n", playDevId); 
        printf("spotDevId : %d\n", spotDevId);

        /* Main Display */
        Vdis_stopDrv(liveDevId);

        /* Sub Display */

        Vdis_stopDrv(playDevId);

        /* Spot Display */

        Vdis_stopDrv(spotDevId);

        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);

        /* Spot Display */
        Vdis_setResolution(spotDevId, spotDspRes);

        memset(&vdMosaicParam, 0, sizeof(VDIS_MOSAIC_S));
        
        DVR_swMsGenerateLayouts(mainDspId, 0, MAX_CH*2, gMosaicInfo_Live.layoutId, 
                                &vdMosaicParam, FALSE,gInitSettings.maindisplayoutput.iResolution, 0);
        
        for(i = 0; i < gMosaicInfo_Live.numWin; i++)
            vdMosaicParam.chnMap[i] = gMosaicInfo_Live.chMap[i];

        for (i=0; i<Vcap_getNumChannels(); i++)
            Vcap_setFrameRate(i, 0, 60, 0);
    
        for (i=0; i<vdMosaicParam.numberOfWindows; i++)
        {
            if((vdMosaicParam.chnMap[i] < Vcap_getNumChannels()) && (vdMosaicParam.useLowCostScaling[i] == FALSE))
                Vcap_setFrameRate(vdMosaicParam.chnMap[i],0,60,30);
            
        }   
        
        Vdis_setMosaicParams(liveDevId, &vdMosaicParam);

        Vdis_setResolution(liveDevId, mainDspRes);



        memset(&vdMosaicParam, 0, sizeof(VDIS_MOSAIC_S));


        DVR_swMsGenerateLayouts(subDspId, MAX_CH, MAX_CH*2, gMosaicInfo_Play.layoutId, 
                                &vdMosaicParam, FALSE,gInitSettings.subdisplayoutput.iResolution, 0);
        for (i=0; i<vdMosaicParam.numberOfWindows; i++)
        {
            gMosaicInfo_Play.chMap[i] = vdMosaicParam.chnMap[i];
            if(pAudstate->state == LIB_LIVE_MODE)
                vdMosaicParam.chnMap[i] = VDIS_CHN_INVALID;
        }
        
        Vdis_setMosaicParams(playDevId, &vdMosaicParam);


        Vdis_setResolution(playDevId, subDspRes);
        

        Dvr_swMsGetOutSize(gInitSettings.maindisplayoutput.iResolution, &outWidth, &outHeight);
        
        /* Enable graphics through sysfs entries */
        DVR_FBScale(liveDevId, 1920, 1080, outWidth, outHeight);
        

        if(gInitSettings.layout.iMainOutput == DISP_PATH_HDMI0)
        {
            //enable HDMI0 Grp,
            Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);
        
        }
        else if(gInitSettings.layout.iMainOutput == DISP_PATH_HDMI1)
        {
            //enable HDMI1 Grp
            Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_ON);
        
        }

        Vdis_startDrv(spotDevId);
        

        Vdis_startDrv(playDevId);


        Vdis_startDrv(liveDevId);



    }
    else if(mainDspId == subDspId)
    {

        gInitSettings.layout.iMainOutput = mainDspId;
        gInitSettings.layout.iSubOutput = subDspId;  
        gLiveDspId = mainDspId;
        gPlayDspId = subDspId;

        printf("gInitSettings.maindisplayoutput.iResolution: %d\n", gInitSettings.maindisplayoutput.iResolution);
        printf("gInitSettings.spotdisplayoutput.iResolution: %d\n", gInitSettings.spotdisplayoutput.iResolution);        
        printf("gInitSettings.layout.iMainOutput           : %d\n", gInitSettings.layout.iMainOutput);
        printf("mainDspId : %d\n", mainDspId);
        printf("spotDspId : %d\n", spotDspId);
        printf("liveDevId : %d\n", liveDevId);
        printf("spotDevId : %d\n", spotDevId);

        /* Main Display */
        Vdis_stopDrv(liveDevId);

        /* Spot Display */

        Vdis_stopDrv(spotDevId);

        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);
        Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);


        /* Spot Display */
        Vdis_setResolution(spotDevId, spotDspRes); 

        memset(&vdMosaicParam, 0, sizeof(VDIS_MOSAIC_S));
                
        DVR_swMsGenerateLayouts(VDIS_DEV_HDCOMP, MAX_CH, MAX_CH*2, gMosaicInfo_Play.layoutId, 
                                &vdMosaicParam, FALSE,gInitSettings.maindisplayoutput.iResolution, 0);
        for(i = 0; i < vdMosaicParam.numberOfWindows; i++)
            vdMosaicParam.chnMap[i] = VDIS_CHN_INVALID;
        Vdis_setMosaicParams(VDIS_DEV_HDCOMP, &vdMosaicParam);

        DVR_SetMosaicLayout(gInitSettings.layout.iLayoutMode);
        if(pAudstate->state == LIB_PLAYBACK_MODE)
            DVR_SetPlaybackMosaicLayout(gInitSettings.layout.iLayoutMode);
        
        Vdis_setResolution(liveDevId, mainDspRes);

        Dvr_swMsGetOutSize(gInitSettings.maindisplayoutput.iResolution, &outWidth, &outHeight);
        
        /* Enable graphics through sysfs entries */
        DVR_FBScale(liveDevId, 1920, 1080, outWidth, outHeight);
        
        if(gInitSettings.layout.iMainOutput == DISP_PATH_HDMI0)
        {
            //enable HDMI0 Grp,
            Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_ON);
        
        }
        
        Vdis_startDrv(spotDevId);
        

        Vdis_startDrv(liveDevId);

    }

    return rv;
}


int DVR_setPlaybackChnMap(Bool bSet)
{
	UInt32 chMap[VDIS_MOSAIC_WIN_MAX];
	int winId;

	if(bSet)
	{
	    for (winId=0; winId < gMosaicInfo_Play.numWin; winId++)
			chMap[winId] = gMosaicInfo_Play.chMap[winId];
	}
	else
	{
	    for (winId=0; winId < VDIS_MOSAIC_WIN_MAX; winId++)
    		chMap[winId] = VDIS_CHN_INVALID;
	}

	Vdis_setMosaicChn(gPlayDspId, chMap);
	return 0;
}

int DVR_setCaptureResolution(int chId, int resId)
{
	int palMode = 0;
	VCAP_CHN_DYNAMIC_PARAM_S params;

	//if(gCaptureInfo.chStatus[chId].frameHeight == 288)
		palMode = 1;
	//else
		//palMode = 0;

	params.chDynamicRes.pathId = 1;	//primary stream.

	switch(resId)
	{
		case RESOLUTION_D1:
            params.chDynamicRes.width = 704;
            if (palMode)
                params.chDynamicRes.height = 576;
            else
                params.chDynamicRes.height = 480;
			break;
		case RESOLUTION_CIF:
			params.chDynamicRes.width = 352;
			if (palMode)
				params.chDynamicRes.height = 288;
			else
				params.chDynamicRes.height = 240;

			break;
		case RESOLUTION_HALFD1:
            params.chDynamicRes.width = 704;
            if (palMode)
                params.chDynamicRes.height = 288;
            else
                params.chDynamicRes.height = 240;
			break;
		default:
            params.chDynamicRes.width = 704;
            if (palMode)
                params.chDynamicRes.height = 576;
            else
                params.chDynamicRes.height = 480;
			break;
	}

	return Vcap_setDynamicParamChn(chId, &params, VCAP_RESOLUTION);
}

static int disp_fbinfo(int infd)
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo, org_varinfo;
	int size;
	int ret;
	int fd = infd;

	/* Get fix screen information. Fix screen information gives
	 * fix information like panning step for horizontal and vertical
	 * direction, line length, memory mapped start address and length etc.
	 */
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		eprintf("Error reading fixed information.\n");
		return -1;
	}

	{
		usleep(10000);
		printf("\n");
        printf("Fix Screen Info:\n");
		printf("----------------\n");
		printf("Line Length - %d\n", fixinfo.line_length);
		printf("Physical Address = %lx\n",fixinfo.smem_start);
		printf("Buffer Length = %d\n",fixinfo.smem_len);
	}

	/* Get variable screen information. Variable screen information
	 * gives informtion like size of the image, bites per pixel,
	 * virtual size of the image etc. */
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		perror("Error reading variable information.\n");
		return -1;
	}

	{
		usleep(10000);
   	    printf("\n");
	    printf("Var Screen Info:\n");
	    printf("----------------\n");
	    printf("Xres - %d\n", varinfo.xres);
	    printf("Yres - %d\n", varinfo.yres);
	    printf("Xres Virtual - %d\n", varinfo.xres_virtual);
	    printf("Yres Virtual - %d\n", varinfo.yres_virtual);
	    printf("Bits Per Pixel - %d\n", varinfo.bits_per_pixel);
	    printf("Pixel Clk - %d\n", varinfo.pixclock);
	    printf("Rotation - %d\n", varinfo.rotate);
	}

	memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

	/*
	 * Set the resolution which read before again to prove the
	 * FBIOPUT_VSCREENINFO ioctl.
	 */

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &org_varinfo);
	if (ret < 0) {
		perror("Error writing variable information.\n");
		return -1;
	}

	/* It is better to get fix screen information again. its because
	 * changing variable screen info may also change fix screen info. */
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		perror("Error reading fixed information.\n");
		return -1;
	}

	size = varinfo.xres*varinfo.yres*(varinfo.bits_per_pixel/8);
	printf("buf_size - %d Bytes\n", size);

	return size;
}

int DVR_FBStart()
{

  struct fb_var_screeninfo varinfo;
  fd0 = open(FBDEV_NAME_0, O_RDWR);
  if(fd0 < 0)
    printf("Could not open device [%s] \n", FBDEV_NAME_0);
  
  if (ioctl(fd0, FBIOGET_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB0: FBIOGET_VSCREENINFO !!!\n");
    }
    varinfo.xres           =  1920;
    varinfo.yres           =  1080;
    varinfo.xres_virtual   =  1920;
    varinfo.yres_virtual   =  1080;
    {
        varinfo.bits_per_pixel   =  16;
        varinfo.red.length       =  5;
        varinfo.green.length     =  6;
        varinfo.blue.length      =  5;

        varinfo.red.offset       =  11;
        varinfo.green.offset     =  5;
        varinfo.blue.offset      =  0;
    }

    if (ioctl(fd0, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB0: FBIOPUT_VSCREENINFO !!!\n");
    }
    DVR_FBScale(VDIS_DEV_HDMI, 0, 0, 1920, 1080);

    disp_fbinfo(fd0);

    fd1 = open(FBDEV_NAME_1, O_RDWR);
  if(fd1 < 0)
  	printf("Could not open device [%s] \n", FBDEV_NAME_1);

  if (ioctl(fd1, FBIOGET_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB1: FBIOGET_VSCREENINFO !!!\n");
    }
    varinfo.xres           =  1920;
    varinfo.yres           =  1080;
    varinfo.xres_virtual   =  1920;
    varinfo.yres_virtual   =  1080;
    {
        varinfo.bits_per_pixel   =  16;
        varinfo.red.length       =  5;
        varinfo.green.length     =  6;
        varinfo.blue.length      =  5;

        varinfo.red.offset       =  11;
        varinfo.green.offset     =  5;
        varinfo.blue.offset      =  0;
    }

    if (ioctl(fd1, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB1: FBIOPUT_VSCREENINFO !!!\n");
    }
    DVR_FBScale(VDIS_DEV_DVO2, 0, 0, 1920, 1080);
    
    disp_fbinfo(fd1); 

    return 0;
}

int DVR_FBScale(int devId, UInt32 inWidth, UInt32 inHeight, UInt32 outWidth, UInt32 outHeight)
{
#if 0
    int fd = 0, status = 0;

    struct ti81xxfb_scparams scparams;
    struct ti81xxfb_region_params  regp;
    int dummy;

    
  if (devId == VDIS_DEV_HDMI) {
        fd = fd0;
    }
    else if(devId == VDIS_DEV_DVO2)
    {
        fd = fd1;
    }
  

    /* Set Scalar Params for resolution conversion */
    scparams.inwidth = 1920;
    scparams.inheight = 1080;

    // this "-GRPX_SC_MARGIN_OFFSET" is needed since scaling can result in +2 extra pixels, so we compensate by doing -2 here
    scparams.outwidth = outWidth;// - GRPX_SC_MARGIN_OFFSET;
    scparams.outheight = outHeight;// - GRPX_SC_MARGIN_OFFSET;
    scparams.coeff = NULL;

    if ((status = ioctl(fd, TIFB_SET_SCINFO, &scparams)) < 0) {
        eprintf("TIFB_SET_SCINFO !!!\n");
    }



    if (ioctl(fd, TIFB_GET_PARAMS, &regp) < 0) {
        eprintf("TIFB_GET_PARAMS !!!\n");
    }

    regp.pos_x = 0;
    regp.pos_y = 0;
    regp.transen = TI81XXFB_FEATURE_ENABLE;
    regp.transcolor = RGB_KEY;//RGB_KEY_24BIT_GRAY;
    regp.scalaren = TI81XXFB_FEATURE_ENABLE;

    if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
        eprintf("TIFB_SET_PARAMS !!!\n");
    }

    if (ioctl(fd, FBIO_WAITFORVSYNC, &dummy)) {
        eprintf("FBIO_WAITFORVSYNC !!!\n");
//        return -1;
    }

    return (status);
#endif
	return 0;
}

int DVR_setAudioInputParams(int iSampleRate, int iVolume)
{
    return Vcap_setAudioModeParam(MAX_DVR_CHANNELS, AUDIO_SAMPLE_RATE_DEFAULT,iVolume);
}

int DVR_setAudioAIC3volume(int iVolume)
{
    int rv = ERROR_FAIL;
    
    if (Audio_playIsStart(AUDIO_PLAY_AIC3x))
        rv = Audio_playSetVolume(iVolume*10);

    Vsys_printDetailedStatistics(); //Arun
    return rv;
}

int DVR_setAudioOutputDev(int devId)
{
	int rv = 0 ;
	printf("Enable audio device: new=%d,old=%d\n",devId,gInitSettings.audioout.iDevId);
	if(devId != gInitSettings.audioout.iDevId)
	{
	rv = Audio_playStop(gInitSettings.audioout.iDevId);
	rv = Audio_playStart(devId);
    Audio_playSetVolume(gInitSettings.audioout.iVolume*10);
	gInitSettings.audioout.iDevId = devId ;
	}
	return rv ;
}

void DVR_getPbMosaicInfo(int *numWin, int* chnMap)
{
	int i;
	*numWin = gMosaicInfo_Play.numWin;
	for(i = 0; i < gMosaicInfo_Play.numWin; i++){
		chnMap[i] = gMosaicInfo_Play.chMap[i];
	}
}

void DVR_setOff_GRPX(void)
{
	if(gInitSettings.layout.iSubOutput == DISP_PATH_HDMI0)
    {
		Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX0, VDIS_OFF);

    }
    else
    {
		Vdis_sysfsCmd(3, VDIS_SYSFSCMD_SET_GRPX, VDIS_SYSFS_GRPX1, VDIS_OFF);

    }
}



