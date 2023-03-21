#include "udbasket.h"
#include "app_motion.h"
#include "app_manager.h"
#include <osa.h>


VIDEOMOTION_Ctrl gVIDEOMOTION_ctrl ;
MD_Hndl_t gMDHndl ;
MD_RunPrm_t    mdRunPrm;

int VIDEO_motionTskCreate(VIDEOMOTION_CreatePrm *prm)
{
    int status = OSA_SOK, i = 0;

    status = OSA_mutexCreate(&gVIDEOMOTION_ctrl.mutexLock);
    if(status!=OSA_SOK)
    {
        OSA_ERROR("OSA_mutexCreate()\n");
    }

    memcpy(&gVIDEOMOTION_ctrl.createPrm, prm, sizeof(gVIDEOMOTION_ctrl.createPrm));

    gVIDEOMOTION_ctrl.bufNum = prm->numCh*VIDEOMOTION_MIN_IN_BUF_PER_CH;

    gVIDEOMOTION_ctrl.bufSize = (prm->frameWidth/16)*(prm->frameHeight/16)*VIDEOMOTION_BUF_MAX_FRAMES_IN_BUF;
    gVIDEOMOTION_ctrl.bufSize = OSA_align(gVIDEOMOTION_ctrl.bufSize, 32);

    printf("gVIDEOMOTION_ctrl.bufSize = %d\n",gVIDEOMOTION_ctrl.bufSize);
    printf("gVIDEOMOTION_ctrl.bufNum  = %d\n",gVIDEOMOTION_ctrl.bufNum);

    gVIDEOMOTION_ctrl.bufVirtAddr = OSA_memAlloc((gVIDEOMOTION_ctrl.bufSize * gVIDEOMOTION_ctrl.bufNum));

    if(gVIDEOMOTION_ctrl.bufVirtAddr==NULL)
    {
        
        goto error_exit;
    }

    gVIDEOMOTION_ctrl.bufCreate.numBuf = gVIDEOMOTION_ctrl.bufNum;
    for(i=0; i<gVIDEOMOTION_ctrl.bufNum; i++)
    {
        gVIDEOMOTION_ctrl.bufCreate.bufVirtAddr[i] = gVIDEOMOTION_ctrl.bufVirtAddr + i*gVIDEOMOTION_ctrl.bufSize;
    }

    status = OSA_bufCreate(&gVIDEOMOTION_ctrl.bufHndl, &gVIDEOMOTION_ctrl.bufCreate);
    if(status!=OSA_SOK)
    {
        OSA_ERROR("OSA_bufCreate()\n");
        goto error_exit;
    }

    for(i = 0; i < prm->numCh; i++)
    {
        gVIDEOMOTION_ctrl.motionstatus[i] = 0 ;
    }

    return OSA_SOK;

error_exit:

    OSA_mutexDelete(&gVIDEOMOTION_ctrl.mutexLock);

    if(gVIDEOMOTION_ctrl.bufVirtAddr)
    {
        OSA_memFree(gVIDEOMOTION_ctrl.bufVirtAddr);
        OSA_bufDelete(&gVIDEOMOTION_ctrl.bufHndl);
    }
    DM816X_MD_Delete() ;

    return status;
}

int VIDEO_motionTskDelete()
{
    int status = OSA_SOK;

    OSA_mutexDelete(&gVIDEOMOTION_ctrl.mutexLock);

    if(gVIDEOMOTION_ctrl.bufVirtAddr)
    {
        OSA_memFree(gVIDEOMOTION_ctrl.bufVirtAddr);
        OSA_bufDelete(&gVIDEOMOTION_ctrl.bufHndl);
    }
    DM816X_MD_Delete() ;

    return status;
}



int DM816X_MD_Create()
{
    int i ;

    for(i = 0 ; i < MAX_CH; i++)
    {
    	gMDHndl.motionenable[i]  = 0;
    	gMDHndl.motionlevel[i]   = ALG_MOTION_DETECT_SENSITIVITY_LOW;
    }
        gMDHndl.frameCnt      = 0;
        gMDHndl.startCnt      = 200;
        gMDHndl.detectCnt     = 0;
        gMDHndl.motioncenable = 0;
        gMDHndl.motioncvalue  = 0;
        gMDHndl.motionsens    = 0;
        gMDHndl.blockNum      = 4095;

    return 0;
}

int DM816X_MD_SetPrm(int streamId, int pPrm, int sensitivity, unsigned char* motionTable)
{
	gMDHndl.motionenable[streamId]  = pPrm;
	gMDHndl.motionlevel[streamId]	= sensitivity ;
	memcpy(mdRunPrm.windowEnableMap[streamId], motionTable, MAX_MOTION_CELL) ;

    return 0;
}

int DM816X_MD_Calc(MD_Hndl_t *hndl)
{
    unsigned int mbMV_data;

    int i,j,status;
    unsigned short mbSAD;

    mbMV_data = hndl->runPrm.mbMvInfo + hndl->MvDataOffset;

	//printf("hndl->MvDataOffset = %d\n",hndl->MvDataOffset) ;
//    hndl->warning_count = 0;

    mbSAD = *((unsigned short*)mbMV_data);
	//printf("%-7d",mbSAD) ;
			

    if(mbSAD > hndl->SAD_THRESHOLD)
    {
	//printf("%-7d",mbSAD) ;
        hndl->warning_count++;
    //}

	//if(hndl->warning_count > 0)
	//{	
		status = ALG_MOTION_S_DETECT ;
		printf("hndl->chId = %d ALG_MOTION_S_DETECT,warning_count=%d\n",hndl->chId,hndl->warning_count) ;
	}
	else
	{
		status = ALG_MOTION_S_NO_DETECT ;
	}

    return status;
}


int DM816X_MD_Start(MD_Hndl_t *hndl)
{
    int detect_cnt = 0, loopcount = 0;
    int i=0,j=0;

    hndl->MvDataOffset = 0;
	hndl->warning_count = 0 ;

    for(i = 0; i < ALG_MOTION_DETECT_VERT_REGIONS ;i++)
    {

        for(j = 0; j < ALG_MOTION_DETECT_HORZ_REGIONS ;j++)
        {

			loopcount++ ;
		//	printf("hndl->runPrm.windowEnableMap[%d][%d] = %d\n",hndl->chId, ALG_MOTION_DETECT_HORZ_REGIONS * i + j, hndl->runPrm.windowEnableMap[hndl->chId][ALG_MOTION_DETECT_HORZ_REGIONS * i + j]) ;

            if(hndl->runPrm.windowEnableMap[hndl->chId][ALG_MOTION_DETECT_HORZ_REGIONS * i + j] == 1) //1- True , 0 - False
            {

                if(DM816X_MD_Calc(hndl) == ALG_MOTION_S_DETECT )
                {
                    hndl->runStatus.windowMotionDetected[j][i] = 1; //Motion Detected
                    detect_cnt++;
                }
                else
                {
                    hndl->runStatus.windowMotionDetected[j][i] = 0; //Motion Not Detected
                }
                hndl->MvDataOffset += hndl->runPrm.mvJump*2 ;
            }
			else
			{
				if(j > 0)
                    hndl->MvDataOffset += hndl->runPrm.mvJump*2 ;
			}
        }
		if(j > 0)
			hndl->MvDataOffset += hndl->runPrm.mvJump*2 ;
    }
    return (( hndl->warning_count >= hndl->threshold ) ? ALG_MOTION_S_DETECT : ALG_MOTION_S_NO_DETECT) ;

}


/* DM816X Get Threshold */

int DM816X_MD_GetThres(MD_Hndl_t *hndl)
{
    int block_size;

    if(hndl == NULL)
    {
        return ALG_MOTION_S_FAIL;
    }

    hndl->frame_width   = (hndl->runPrm.ImageWidth  >> 4); // Number of macroblocks in frame width
    hndl->frame_height  = (hndl->runPrm.ImageHeight >> 4); // Number of macroblocks in frame height

    /* Set the motion block size base on image size */
    hndl->win_width     = (hndl->runPrm.windowWidth >> 4); //Window width in macroblocks
    hndl->win_height    = (hndl->runPrm.windowHeight >> 4); //Window height in macroblocks

    block_size = hndl->win_width * hndl->win_height;

    if(block_size >= 600) /* for 1080P */
    {
        hndl->SAD_THRESHOLD = 2500;
    }
    else if(block_size < 600 && block_size >= 300 ) /* for 720P */
    {
             hndl->SAD_THRESHOLD = 3000;
    }
    else if(block_size < 300 && block_size >= 100 ) /* for D1 */
    {
        hndl->SAD_THRESHOLD = 1500;
    }
    else if(block_size < 100 && block_size >= 20 ) /* for CIF */
    {
        hndl->SAD_THRESHOLD = 4000;
    }
    else /* less than CIF */
    {
        hndl->SAD_THRESHOLD = 4500;
    }

    return ALG_MOTION_S_OK;
}

/* DM816X Get Sensitivity */


int DM816X_MD_GetSensitivity(MD_Hndl_t *hndl)
{
    if(hndl == NULL)
    {
        return ALG_MOTION_S_FAIL;
    }

    hndl->motionsens = 1; /* Intialize to avoid non-zero value */

    /* Get motion sensitivity from webpage according to user input*/
    if(hndl->motioncenable == 0)
    {
        if(hndl->frame_width >= 120) /* 1080P = 675MB/zone */
        {
            if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_LOW)                       //low level
            {
                hndl->motionsens = 35;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_MEDIUM)               //medium level
            {
                hndl->motionsens = 55;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_HIGH)            //high level
            {
                hndl->motionsens = 95;
            }
        }
		else if(hndl->frame_width < 120 && hndl->frame_width >= 80) /* 720P = 300MB/zone */
        {
            if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_LOW)                  //low level
            {
                hndl->motionsens = 15;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_MEDIUM)           //medium level
            {
                hndl->motionsens = 25;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_HIGH)         //high level
            {
                hndl->motionsens = 50;
            }
        }
		else if(hndl->frame_width < 80 && hndl->frame_width >= 40) /* D1 = 112.5MB/zone */
        {
            if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_LOW)                 //low level
            {
                hndl->motionsens = 7;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_MEDIUM)          //medium level
            {
                hndl->motionsens = 11;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_HIGH)         //high level
            {
                hndl->motionsens = 20;
            }
        }
		else /* CIF = 20MB/zone */
        {
            if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_LOW)           //low level
            {
                hndl->motionsens = 3;
            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_MEDIUM)          //medium level
            {
                hndl->motionsens = 5;

            }
            else if(hndl->motionlevel[hndl->chId] == ALG_MOTION_DETECT_SENSITIVITY_HIGH)            //high level
            {
                hndl->motionsens = 10;
            }
        }
	} // if(hndl->motioncenable == 0)
	else
	{
		if(hndl->motioncvalue == 0)
        {
            hndl->motionsens = 20;
        }
        else
        {
            if(hndl->frame_width > 45)
            {
                if(hndl->motioncvalue >= 20)
                {
                    hndl->motionsens = hndl->motioncvalue;
                }
                else
                {
                    hndl->motionsens = 20;
                }
            }
            else
            {
                hndl->motionsens = (hndl->motioncvalue / 5);

                if(hndl->motionsens < 10)
                {
                    hndl->motionsens = 10;
                }
            }
        }
    }

    if( hndl->motionsens == 0)
    {
        hndl->motionsens = 1;
    }

    /* Calculate the threshold value base on the motion sensitivity */
    hndl->threshold = (hndl->win_width * hndl->win_height)/hndl->motionsens;

    return ALG_MOTION_S_OK;
}
    
/* DM816X Motion Detection Print Run Time Parameters*/

static void DM816X_MD_PrintRunTimeParams(MD_RunPrm_t *prm)
{
    UInt8 horId, verId;
    printf("\n mbInfo | mvJump | ImgWidth | ImgHeight | winWidth | winHeight | ");
    printf("keyFrame | codecType | isDateTimeDraw \n");

    printf("\n %6d | %6d |   %3d   |    %3d   |   %3d   |    %3d   |     %1d   |      %1d     |     %1d     \n", 
            prm->mbMvInfo, 
            prm->mvJump, 
            prm->ImageWidth,
            prm->ImageHeight,
            prm->windowWidth,
            prm->windowHeight,
            prm->isKeyFrame,
            prm->codecType,
            prm->isDateTimeDraw);

    printf("\n    Window Map     \n");
    printf("\n    ");

    for(horId = 0; horId < ALG_MOTION_DETECT_HORZ_REGIONS; horId++)
    {
        printf(" %2d  ",horId);
    }
    
    for(verId = 0; verId < ALG_MOTION_DETECT_VERT_REGIONS; verId++)
    {
        printf("\n %2d ",verId); 
        for(horId = 0; horId < ALG_MOTION_DETECT_HORZ_REGIONS; horId++)
        {
            if(prm->windowEnableMap[horId][verId] == 1)
            {
                printf("  X  ");
            }
            else
            {
                printf("  O  ");
            }
        }
    }
    printf("\n");
}
/* DM816X Motion Detection Run */

int DM816X_MD_Run(MD_Hndl_t *hndl,MD_RunPrm_t *prm,MD_RunStatus_t *status)
{
    if(hndl == NULL)
    {
        return ALG_MOTION_S_FAIL;
    }

    /*Parm tranfer*/
    hndl->runPrm    = *prm;
    hndl->runStatus = *status;
    hndl->frameCnt++;

    if((hndl->runPrm.isKeyFrame == TRUE) || (hndl->frameCnt < hndl->startCnt))
    {
        return ALG_MOTION_S_NO_DETECT;
    }

    DM816X_MD_GetThres(hndl);
    DM816X_MD_GetSensitivity(hndl);

    return DM816X_MD_Start(hndl);
}

/* DM816X Motion Detection Algorithm delete */

int DM816X_MD_Delete()
{
    gMDHndl.frameCnt  = 0;
    gMDHndl.startCnt  = 200;
    gMDHndl.detectCnt = 0;

    return 0;
}

/* DM816X Motion Detection Apply */

int DM816X_MD_Apply(unsigned int mvBuf, unsigned int frameWidth, unsigned int frameHeight, unsigned int isKeyFrame )
{
    int mdStatus;
//    MD_RunPrm_t    mdRunPrm;
    MD_RunStatus_t mdRunStatus;
    AnalyticHeaderInfo *pAnalyticHeaderInfo = (AnalyticHeaderInfo*)mvBuf;
    int numElements = pAnalyticHeaderInfo->NumElements;


    if((numElements == 0) || (isKeyFrame == 1) || (gMDHndl.motionenable[gMDHndl.chId] == 0))
    {
        return 0;
    }

    mdRunPrm.mbMvInfo       = mvBuf + (pAnalyticHeaderInfo->elementInfoField0SAD.StartPos);
    mdRunPrm.mvJump         = pAnalyticHeaderInfo->elementInfoField0SAD.Jump;

    mdRunPrm.ImageWidth     = frameWidth;
    mdRunPrm.ImageHeight    = frameHeight;
    mdRunPrm.isKeyFrame     = isKeyFrame;
    mdRunPrm.codecType      = 0;
    mdRunPrm.isDateTimeDraw = 0;
	mdRunPrm.windowWidth    = (mdRunPrm.ImageWidth / 4);
	mdRunPrm.windowHeight   = (mdRunPrm.ImageHeight / 3);
//	memcpy(mdRunPrm.windowEnableMap[gMDHndl.chId], gMDHndl.runPrm.windowEnableMap[gMDHndl.chId], MAX_MOTION_CELL) ;


    mdStatus = DM816X_MD_Run(&gMDHndl,&mdRunPrm,&mdRunStatus);
/*
    if(mdStatus == ALG_MOTION_S_DETECT)
    {

        if(gMDHndl.detectCnt > ALG_MOTION_DETECT_MAX_DETECT_CNT)
        {
            gMDHndl.detectCnt = 0;
        }

        if(gMDHndl.detectCnt == 0)
        {
            return mdStatus ;
        }

        gMDHndl.detectCnt ++;

    }
    return 0;
*/
    return mdStatus ;
}

int VIDEO_motionTskRun()
{
    int status, inBufId, retval = 0, isKeyFrame;
    OSA_BufInfo *pMotionBufInfo ;
    status = OSA_bufGetFull(&gVIDEOMOTION_ctrl.bufHndl, &inBufId, OSA_TIMEOUT_FOREVER);
    if(status!=OSA_SOK)
    {
        OSA_ERROR("OSA_bufGetFull()\n");
        return status;
    }

    pMotionBufInfo = OSA_bufGetBufInfo(&gVIDEOMOTION_ctrl.bufHndl, inBufId);

    gMDHndl.chId = pMotionBufInfo->flags ;
    isKeyFrame = pMotionBufInfo->count ;

	if(gMDHndl.motionenable[gMDHndl.chId] != 0)
	{
		//printf("Input tskRun chid = %d\n",gMDHndl.chId) ;
		retval = DM816X_MD_Apply(pMotionBufInfo->virtAddr, gVIDEOMOTION_ctrl.createPrm.frameWidth, gVIDEOMOTION_ctrl.createPrm.frameHeight, isKeyFrame) ;
		if(retval == ALG_MOTION_S_DETECT)
		{
			gVIDEOMOTION_ctrl.motionstatus[gMDHndl.chId] = 1 ;
			printf("Detect tskRun chid = %d\n",gMDHndl.chId) ;
		}
		else
		{
			gVIDEOMOTION_ctrl.motionstatus[gMDHndl.chId] = 0 ;
		}
	}
    OSA_bufPutEmpty(&gVIDEOMOTION_ctrl.bufHndl, inBufId);

    return status;
}




int VIDEO_motionTskMain(struct OSA_TskHndl *pTsk, OSA_MsgHndl *pMsg, Uint32 curState )
{
    int status;
    Bool done=FALSE, ackMsg = FALSE;

    Uint16 cmd = OSA_msgGetCmd(pMsg);


    VIDEOMOTION_CreatePrm *pCreatePrm = (VIDEOMOTION_CreatePrm*)OSA_msgGetPrm(pMsg);

    if( cmd != VIDEOMOTION_CMD_CREATE || pCreatePrm==NULL)
    {

        return OSA_SOK;
    }

    status = VIDEO_motionTskCreate(pCreatePrm) ;

    OSA_tskAckOrFreeMsg(pMsg, status);

    if(status!=OSA_SOK)
    {
        OSA_ERROR("VIDEO_motionTskCreate()\n");
        return OSA_SOK;
    }

    while(!done)
    {
        status = OSA_tskWaitMsg(pTsk, &pMsg);

        if(status!=OSA_SOK)
            break;

        cmd = OSA_msgGetCmd(pMsg);

        switch(cmd)
        {
            case VIDEOMOTION_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
            break;

            case VIDEOMOTION_CMD_RUN:

                OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);

                VIDEO_motionTskRun();
            break;
            default:

            OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);
            break;
        }
    }

    status = VIDEO_motionTskDelete();

    if(ackMsg)
        OSA_tskAckOrFreeMsg(pMsg, OSA_SOK);

    return OSA_SOK;
}



int VIDEOMOTION_sendCmd(Uint16 cmd, void *prm, Uint16 flags )
{
    return OSA_mbxSendMsg(&gVIDEOMOTION_ctrl.tskHndl.mbxHndl, &gVIDEOMOTION_ctrl.mbxHndl, cmd, prm, flags);
}


int VIDEO_motionCreate(VIDEOMOTION_CreatePrm *prm)
{
    int status;

    DM816X_MD_Create() ;

    status = OSA_tskCreate( &gVIDEOMOTION_ctrl.tskHndl, VIDEO_motionTskMain, VIDEOMOTION_THR_PRI, VIDEOMOTION_STACK_SIZE, 0, &gVIDEOMOTION_ctrl);
    if(status!=OSA_SOK)
    {
        OSA_ERROR("OSA_tskCreate()\n");
        return status;
    }

    OSA_assertSuccess(status);

    status = OSA_mbxCreate(&gVIDEOMOTION_ctrl.mbxHndl);

    OSA_assertSuccess(status);

    status = VIDEOMOTION_sendCmd(VIDEOMOTION_CMD_CREATE, prm, OSA_MBX_WAIT_ACK );

    return status;

}



int VIDEO_motionDelete()
{
    int status;

    status = VIDEOMOTION_sendCmd(VIDEOMOTION_CMD_DELETE, NULL, OSA_MBX_WAIT_ACK) ;

    status = OSA_tskDelete( &gVIDEOMOTION_ctrl.tskHndl );

    OSA_assertSuccess(status) ;

    status = OSA_mbxDelete( &gVIDEOMOTION_ctrl.mbxHndl ) ;

    OSA_assertSuccess(status) ;

    return status;
}

OSA_BufInfo *VIDEOMOTION_getEmptyBuf(int *bufId, int timeout)
{
    int status;


    status = OSA_bufGetEmpty(&gVIDEOMOTION_ctrl.bufHndl, bufId, timeout);
    if(status!=OSA_SOK)
        return NULL;

    return OSA_bufGetBufInfo(&gVIDEOMOTION_ctrl.bufHndl, *bufId);
}

int VIDEOMOTION_putFullBuf(int chId, OSA_BufInfo *buf, int bufId)
{

    buf->flags = chId;

    OSA_bufPutFull(&gVIDEOMOTION_ctrl.bufHndl, bufId);

    VIDEOMOTION_sendCmd(VIDEOMOTION_CMD_RUN, NULL, 0);

    return OSA_SOK;
}


unsigned int getMotionStatus( int *MotionStatus)
{
	int i = 0 ;
    OSA_mutexLock(&gVIDEOMOTION_ctrl.mutexLock);

    memcpy(MotionStatus, gVIDEOMOTION_ctrl.motionstatus, sizeof(gVIDEOMOTION_ctrl.motionstatus));
/*
	for(i = 0; i < MAX_CH; i++)
	{
		printf("gVIDEOMOTION_ctrl.motionstatus[%d] = %d\n",i, gVIDEOMOTION_ctrl.motionstatus[i]) ;
	}
*/
    memset(gVIDEOMOTION_ctrl.motionstatus, 0, sizeof(gVIDEOMOTION_ctrl.motionstatus));

    OSA_mutexUnlock(&gVIDEOMOTION_ctrl.mutexLock);

    return OSA_SOK ;
}




