/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_ptz_if.c
 * @brief
 * @author	phoong
 * @section	MODIFY history
 *     - 2011.03.01 : First Created
 */
/*****************************************************************************/

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <termios.h>

#include "app_ptz_if.h"
#include "app_manager.h"

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
int ptz_fd=-1;

static PTZLIST_serialInfo		gPTZLIST_serialInfo;
/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
#define eprintf(x...)	printf("err: " x);
//#define dprintf(x...)	printf(x);
#define dprintf(x...)

/*----------------------------------------------------------------------------
 local function
-----------------------------------------------------------------------------*/
void DVR_ptz_DRX500CheckSum(char* pBuf)
{
	int nSum = 0;
	int nNum = 0;
	int nCs = 0;

	nSum = pBuf[0];
	nSum += pBuf[1];
	nSum += pBuf[2];
	nSum += pBuf[3];
	nSum += pBuf[4];
	nSum += pBuf[5];
	nSum += pBuf[6];
	nSum += pBuf[7];
	nSum += pBuf[8];
	nSum += pBuf[9];

	nNum = 0x2020 - nSum;
	
	if(nNum & 1) { nCs += 1; }
	if(nNum & 2) { nCs += 2; }
	if(nNum & 4) { nCs += 4; }
	if(nNum & 8) { nCs += 8; }
	if(nNum & 16) { nCs += 16; }
	if(nNum & 32) { nCs += 32; }
	if(nNum & 64) { nCs += 64; }
	if(nNum & 128) { nCs += 128; }
	
	pBuf[10] = nCs;
}

void DVR_ptz_PelcoDCheckSum(char* pBuf)
{
	unsigned char bySum = 0;
	bySum = pBuf[1];
	bySum += pBuf[2];
	bySum += pBuf[3];
	bySum += pBuf[4];
	bySum += pBuf[5];
	
	pBuf[6] =  bySum;
}

void DVR_ptz_PelcoPCheckSum(char* pBuf)
{
	unsigned char bySum = 0;
	bySum = pBuf[1];
	bySum ^= pBuf[2];
	bySum ^= pBuf[3];
	bySum ^= pBuf[4];
	bySum ^= pBuf[5];
	
	pBuf[7] =  bySum;
}
unsigned int ptzGetBaudRate(unsigned int nBaud)
{
	unsigned int nBaudRate;
	
	switch(nBaud){
		case 1200:
			dprintf("===set baudrate B1200 ===\n");
			nBaudRate = B1200;
			break;
		case 1800:
			dprintf("===set baudrate B1800 ===\n");
			nBaudRate = B1800;
			break;
		case 2400:
			nBaudRate = B2400;
			dprintf("===set baudrate B2400 ===\n");
			break;
		case 4800:
			dprintf("===set baudrate B4800 ===\n");
			nBaudRate = B4800;
			break;
		case 9600:
			dprintf("===set baudrate B9600 ===\n");
			nBaudRate = B9600;
			break;
		case 19200:
			dprintf("===set baudrate B19200 ===\n");
			nBaudRate = B19200;
			break;
		case 38400:
			dprintf("===set baudrate B38400 ===\n");
			nBaudRate = B38400;
			break;
		case 57600:
			dprintf("===set baudrate B57600 ===\n");
			nBaudRate = B57600;
			break;
		case 115200:
			dprintf("===set baudrate B115200 ===\n");
			nBaudRate = B115200;
			break;
		default:
			dprintf("===set baudrate DEFAULT B9600 ===\n");
			nBaudRate = B9600;
			break;
		}
		
		return nBaudRate;
}

unsigned int ptzGetDataBit(unsigned int nBit)
{
	unsigned int nDataBit;
	
	switch(nBit){
		case 5:
			dprintf("===set Databit CS5 ===\n");
			nDataBit = CS5;
			break;
		case 6:
			dprintf("===set Databit CS6 ===\n");
			nDataBit = CS6;
			break;
		case 7:
			dprintf("===set Databit CS7 ===\n");
			nDataBit = CS7;
			break;
		case 8:
			dprintf("===set Databit CS8 ===\n");
			nDataBit = CS8;
			break;
		default:
			dprintf("===set Databit DEFAULT CS8 ===\n");
			nDataBit = CS8;
			break;
	}

	return nDataBit;
}

int ptzGetCmdCtrl(int ptzModelNum, int ctrlCmd, int targetID, char* pBuf)
{
	int ctrlSize = 0;
	
	switch(ptzModelNum)
	{
		case PTZCTRL_DRX_500:
			dprintf("DRX_500\n");
			ctrlSize = 11;
			pBuf[0]		= 0x55;
			pBuf[2]		= targetID;
			pBuf[9]		= 0xaa;
			switch(ctrlCmd)
			{
				case PAN_RIGHT:
					dprintf("PAN_RIGHT\n");
					pBuf[4] = 0x02;
					pBuf[5] = 0xB0;										
					break;
				case PAN_LEFT:
					dprintf("PAN_LEFT\n");
					pBuf[4] = 0x04;
					pBuf[5] = 0xB0;					
					break;					
				case TILT_UP:
					dprintf("TILT_UP\n");
					pBuf[4] = 0x08;
					pBuf[6] = 0xBF;					
					break;
				case TILT_DOWN:
					dprintf("TILT_DOWN\n");
					pBuf[4] = 0x10;
					pBuf[6] = 0xBF;										
					break;
				case ZOOM_IN:
					dprintf("ZOOM_IN\n");
					pBuf[4] = 0x20;
					pBuf[7] = 0x05;
					break;
				case ZOOM_OUT:
					dprintf("ZOOM_OUT\n");
					pBuf[4] = 0x40;
					pBuf[7] = 0x05;					
					break;											
				case FOCUS_OUT:
					dprintf("FOCUS_OUT\n");
					pBuf[3] = 0x01;
					pBuf[7] = 0x05;
					break;
				case FOCUS_IN:
					dprintf("FOCUS_IN\n");
					pBuf[3] = 0x02;
					pBuf[7] = 0x05;					
					break;
				case PTZ_STOP:	
					dprintf("PTZ_STOP\n");		
					break;				
			}
			DVR_ptz_DRX500CheckSum(pBuf);
			break;
		case PTZCTRL_PELCO_D:
			dprintf("PELCO_D\n");
			ctrlSize 	= 7;
			pBuf[0]	  = 0xFF;
			pBuf[1]	  = targetID;
			
			switch(ctrlCmd)
			{
				case PAN_RIGHT:
					dprintf("PAN_RIGHT\n");
					pBuf[3] = 0x02;
					pBuf[4] = 0x20;										
					break;
				case PAN_LEFT:
					dprintf("PAN_LEFT\n");
					pBuf[3] = 0x04;
					pBuf[4] = 0x20;					
					break;					
				case TILT_UP:
					dprintf("TILT_UP\n");
					pBuf[3] = 0x08;
					pBuf[5] = 0x20;					
					break;
				case TILT_DOWN:
					dprintf("TILT_DOWN\n");
					pBuf[3] = 0x10;
					pBuf[5] = 0x20;										
					break;
				case ZOOM_IN:
					dprintf("ZOOM_IN\n");
					pBuf[3] = 0x20;
					break;
				case ZOOM_OUT:
					dprintf("ZOOM_OUT\n");
					pBuf[3] = 0x40;
					break;											
				case FOCUS_OUT:
					dprintf("FOCUS_OUT\n");
					pBuf[3] = 0x80;
					break;
				case FOCUS_IN:
					dprintf("FOCUS_IN\n");
					pBuf[2] = 0x01;
					break;		
				case PTZ_STOP:	
					dprintf("PTZ_STOP\n");		
					break;				
			}
			DVR_ptz_PelcoDCheckSum(pBuf);
			break;
		case PTZCTRL_PELCO_P:
			dprintf("PELCO_P\n");
			ctrlSize 	= 8;
			pBuf[0]	= 0xA0;
			pBuf[1]	= targetID;
			pBuf[6] 	= 0xAF;
			switch(ctrlCmd)
			{
				case PAN_RIGHT:
					dprintf("PAN_RIGHT\n");
					pBuf[3] = 0x02;
					pBuf[4] = 0x20;										
					break;
				case PAN_LEFT:
					dprintf("PAN_LEFT\n");
					pBuf[3] = 0x04;
					pBuf[4] = 0x20;					
					break;					
				case TILT_UP:
					dprintf("TILT_UP\n");
					pBuf[3] = 0x08;
					pBuf[5] = 0x20;					
					break;
				case TILT_DOWN:
					dprintf("TILT_DOWN\n");
					pBuf[3] = 0x10;
					pBuf[5] = 0x20;										
					break;
				case ZOOM_IN:
					dprintf("ZOOM_IN\n");
					pBuf[3] = 0x20;
					break;
				case ZOOM_OUT:
					dprintf("ZOOM_OUT\n");
					pBuf[3] = 0x40;
					break;											
				case FOCUS_OUT:
					dprintf("FOCUS_OUT\n");
					pBuf[2] = 0x01;
					break;
				case FOCUS_IN:
					dprintf("FOCUS_IN\n");
					pBuf[2] = 0x02;
					break;		
				case PTZ_STOP:	
					dprintf("PTZ_STOP\n");		
					break;				
			}
			DVR_ptz_PelcoPCheckSum(pBuf);
			break;
	}
	
	return ctrlSize;
}

void DVR_RE_ptz_setInfo(char ptzIdx,int baudrate)
{
	struct termios	oldtio,newtio;


	//printf("DVR_ptz_setInternalCtrl:ptz_fd=0x%x\n",ptz_fd);
	tcgetattr(ptz_fd,&oldtio);
	bzero(&newtio, sizeof(newtio));

	
	newtio.c_cflag = CLOCAL | CREAD;
	newtio.c_cflag |= ptzGetBaudRate(baudrate);//ptzGetBaudRate((unsigned int)gPTZLIST_intList[ptzIdx].ptzBaudrate);
  	newtio.c_cflag |= CS8;//ptzGetDataBit((unsigned int)gPTZLIST_intList[ptzIdx].ptzDatabit);

  #if 0
  	//////////// parity bit /////////////////
  	if(gPTZLIST_intList[ptzIdx].ptzParitybit == PTZ_PARITY_EVEN)
  	{
  		dprintf("===set paritybit EVEN===\n");
  	  	newtio.c_cflag |= PARENB;
  	}
  	else if(gPTZLIST_intList[ptzIdx].ptzParitybit == PTZ_PARITY_ODD)
  	{
  		dprintf("===set paritybit ODD===\n");
  		newtio.c_cflag |= PARENB;
  		newtio.c_cflag |= PARODD;
  	}
  
  	//////////// stop bit //////////////
	if(gPTZLIST_intList[ptzIdx].ptzStopbit == 2)
	{
  		dprintf("===set stopbit 2 ===\n");
 		newtio.c_cflag |= CSTOPB;
		newtio.c_cflag |= CSIZE;
  	}
 #endif

  	newtio.c_iflag = IGNPAR | ICRNL;

  	newtio.c_oflag = 0;
  	newtio.c_lflag = ICANON;

  	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
  	newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */

  	tcflush(ptz_fd, TCIFLUSH);
  	tcsetattr(ptz_fd,TCSANOW,&newtio);	
}

int DVR_RE_ptz_setInternal_Ctrl(char ptzIdx, char targetAddr, char ctrlCmd)
{
	int ptzCtrlSize;
	char ptzBuf[25];

	
	//char ptzcmd[9]={PAN_RIGHT,PAN_LEFT,TILT_UP,TILT_DOWN,ZOOM_IN,ZOOM_OUT,FOCUS_OUT,FOCUS_IN,PTZ_STOP};
	//char ptzchannel[9]={0,2,4,6,8,10,12,14,15};
	//char j;
	
	bzero(ptzBuf, 25);
      //for(j=0;j<9;j++)
      	//{
      		ptzCtrlSize = ptzGetCmdCtrl(ptzIdx, ctrlCmd, targetAddr, ptzBuf);
	
  	//ptzCtrlSize = ptzGetCmdCtrl(ptzIdx, ctrlCmd, targetAddr, ptzBuf);
	  	if(ptzCtrlSize > 0)
	  	{
	  		/*int i;
			printf("ptzcmd:\t");
	  		for(i=0;i<ptzCtrlSize;i++)
	  			printf("%x\t", ptzBuf[i]);
			printf("\n");*/
	  		write(ptz_fd, (void *)ptzBuf, ptzCtrlSize);
	  	}
		//bzero(ptzBuf, 25);
		//usleep(1000);
      	//}

	return 0;
}

int DVR_ptz_setInternalCtrl(int ptzIdx, int targetAddr, int ctrlCmd)
{
	char ptzBuf[25];
	struct termios	oldtio,newtio;
	int ptzCtrlSize;

	//printf("DVR_ptz_setInternalCtrl:ptzIdx=%d,targetAddr=%d,ctrlCmd=%d\n",ptzIdx,targetAddr,ctrlCmd);
	//printf("DVR_ptz_setInternalCtrl:ptzBaudrate=%d,ptzDatabit=%d,ptzParitybit=%d,ptzStopbit=%d\n",gPTZLIST_serialInfo.ptzBaudrate,gPTZLIST_serialInfo.ptzDatabit,gPTZLIST_intList[ptzIdx].ptzParitybit,gPTZLIST_intList[ptzIdx].ptzStopbit);
	tcgetattr(ptz_fd,&oldtio);
	
	bzero(ptzBuf, 25);
	bzero(&newtio, sizeof(newtio));
	
	newtio.c_cflag = CLOCAL | CREAD;
	newtio.c_cflag |= ptzGetBaudRate((unsigned int)gPTZLIST_serialInfo.ptzBaudrate);
  	newtio.c_cflag |= CS8;//ptzGetDataBit((unsigned int)gPTZLIST_serialInfo.ptzDatabit);

  #if 0
  	//////////// parity bit /////////////////
  	if(gPTZLIST_intList[ptzIdx].ptzParitybit == PTZ_PARITY_EVEN)
  	{
  		dprintf("===set paritybit EVEN===\n");
  	  	newtio.c_cflag |= PARENB;
  	}
  	else if(gPTZLIST_intList[ptzIdx].ptzParitybit == PTZ_PARITY_ODD)
  	{
  		dprintf("===set paritybit ODD===\n");
  		newtio.c_cflag |= PARENB;
  		newtio.c_cflag |= PARODD;
  	}
  
  	//////////// stop bit //////////////
	if(gPTZLIST_intList[ptzIdx].ptzStopbit == 2)
	{
  		dprintf("===set stopbit 2 ===\n");
 		newtio.c_cflag |= CSTOPB;
		newtio.c_cflag |= CSIZE;
  	}
#endif
  	newtio.c_iflag = IGNPAR | ICRNL;

  	newtio.c_oflag = 0;
  	newtio.c_lflag = ICANON;

  	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
  	newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */

  	tcflush(ptz_fd, TCIFLUSH);
  	tcsetattr(ptz_fd,TCSANOW,&newtio);	
	
      		ptzCtrlSize = ptzGetCmdCtrl(ptzIdx, ctrlCmd, targetAddr, ptzBuf);
	
  	//ptzCtrlSize = ptzGetCmdCtrl(ptzIdx, ctrlCmd, targetAddr, ptzBuf);

	  	if(ptzCtrlSize > 0)
	  	{
	  		/*int i;
			printf("ptzcmd:\t");
	  		for(i=0;i<ptzCtrlSize;i++)
	  			printf("%x\t", ptzBuf[i]);
			printf("\n");*/
	  		write(ptz_fd, (void *)ptzBuf, ptzCtrlSize);
	  	}
		bzero(ptzBuf, 25);
     

	return 0;
}

void DVR_start_ptz_if(void)
{
	ptz_fd = open((const char *)"/dev/ttyO0", O_RDWR);
	if (ptz_fd  < 0) {
		eprintf("uart0 device open\n");
		return ;
	}
	dprintf("DVR_start_ptz_if()\n");	
	bzero(gPTZLIST_intList, sizeof(gPTZLIST_intList));
  
  	gPTZLIST_intList[0].ptzIdx 				= PTZCTRL_DRX_500;
  	strcpy(gPTZLIST_intList[0].ptzName, "DRX-500");
  	gPTZLIST_intList[0].ptzBaudrate 	= 9600;
  	gPTZLIST_intList[0].ptzParitybit	= PARITYBIT_NONE;
  	gPTZLIST_intList[0].ptzDatabit		= 8;
  	gPTZLIST_intList[0].ptzStopbit		= 1;
  
  	gPTZLIST_intList[1].ptzIdx 				= PTZCTRL_PELCO_D;
  	strcpy(gPTZLIST_intList[1].ptzName, "PELCO-D");
  	gPTZLIST_intList[1].ptzBaudrate 	= 4800;
  	gPTZLIST_intList[1].ptzParitybit	= PARITYBIT_NONE;
  	gPTZLIST_intList[1].ptzDatabit		= 8;
  	gPTZLIST_intList[1].ptzStopbit		= 1;  
}

int DVR_ptz_getIntPtzCount()
{
	return INTERNAL_PTZ_COUNT;
}

int DVR_ptz_getIntPtzInfo(int ptzIdx, char* pPtzName)
{
	int status = 0;
	
	if(ptzIdx < 3)//INTERNAL_PTZ_COUNT) 
	{
	  strcpy(pPtzName, gPTZLIST_intList[ptzIdx].ptzName);
//	  *pBaudRate	= gPTZLIST_intList[ptzIdx].ptzBaudrate;
//	  *pParityBit	= gPTZLIST_intList[ptzIdx].ptzParitybit;
//	  *pDataBit		= gPTZLIST_intList[ptzIdx].ptzDatabit;
//	  *pStopBit		= gPTZLIST_intList[ptzIdx].ptzStopbit;
	  
	  status = 1;
	}
  	return status;
}

int	DVR_ptz_setSerialInfo(int dataBit, int parityBit, int stopBit, int baudRate)
{
	gPTZLIST_serialInfo.ptzDatabit 		= dataBit;
	gPTZLIST_serialInfo.ptzParitybit	= parityBit;
	gPTZLIST_serialInfo.ptzStopbit		= stopBit;
	gPTZLIST_serialInfo.ptzBaudrate		= baudRate;
	
	return 1;
}

int DVR_ptz_ptzSendBypass(char* ptzBuf, int bufSize)
{
	int sendSize = 0;
	
	struct termios	oldtio,newtio;
	int ptzCtrlSize;
	
	tcgetattr(ptz_fd,&oldtio);
	
	bzero(ptzBuf, 25);
	bzero(&newtio, sizeof(newtio));
	
	newtio.c_cflag = CLOCAL | CREAD;
	newtio.c_cflag |= ptzGetBaudRate((unsigned int)gPTZLIST_serialInfo.ptzBaudrate);
  	newtio.c_cflag |= ptzGetDataBit((unsigned int)gPTZLIST_serialInfo.ptzDatabit);

  	//////////// parity bit /////////////////
  	if(gPTZLIST_serialInfo.ptzParitybit == PARITYBIT_EVEN)
  	{
  		dprintf("===set paritybit EVEN===\n");
  	  	newtio.c_cflag |= PARENB;
  	}
  	else if(gPTZLIST_serialInfo.ptzParitybit == PARITYBIT_ODD)
  	{
  		dprintf("===set paritybit ODD===\n");
  		newtio.c_cflag |= PARENB;
  		newtio.c_cflag |= PARODD;
  	}
  
  	//////////// stop bit //////////////
	if(gPTZLIST_serialInfo.ptzStopbit == 2)
	{
  		dprintf("===set stopbit 2 ===\n");
 		newtio.c_cflag |= CSTOPB;
		newtio.c_cflag |= CSIZE;
  	}

  	newtio.c_iflag = IGNPAR | ICRNL;

  	newtio.c_oflag = 0;
  	newtio.c_lflag = ICANON;

  	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
  	newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */

  	tcflush(ptz_fd, TCIFLUSH);
  	tcsetattr(ptz_fd,TCSANOW,&newtio);	
	
  	if(bufSize > 0)
  	{
  		ptzCtrlSize = bufSize+1;
//  	int i;
//  	for(i=0;i<ptzCtrlSize;i++)
//  		printf("%x\n", ptzBuf[i]);
  		
  		sendSize = write(ptz_fd, (void *)ptzBuf, ptzCtrlSize);
  	}	
  
  	return sendSize;
}
