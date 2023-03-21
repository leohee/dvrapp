/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_ptz_if.h
 * @brief
 * @author	phoong
 * @section	MODIFY history
 *     - 2011.03.01 : First Created
 */
/*****************************************************************************/

#if !defined (_PTZ_IF_H_)
#define _PTZ_IF_H_

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
#define INTERNAL_PTZ_COUNT							6

#define MAX_PARITYBIT                               3
#define PARITYBIT_NONE                              0
#define PARITYBIT_ODD                               1
#define PARITYBIT_EVEN                              2

#define PTZCTRL_DRX_500                             0
#define PTZCTRL_PELCO_D                             1
#define PTZCTRL_PELCO_P                             2

#define PTZ_BAUDRATE_1200                           1200
#define PTZ_BAUDRATE_2400                           2400
#define PTZ_BAUDRATE_4800                           4800
#define PTZ_BAUDRATE_9600                           9600
#define PTZ_BAUDRATE_38400                          38400
#define PTZ_BAUDRATE_57600                          57600
#define PTZ_BAUDRATE_115200                         115200

#define PTZ_DATBIT_7                                7
#define PTZ_DATBIT_8                                8

#define PTZ_STOPBIT_1                               1
#define PTZ_STOPBIT_2                               2

#define PTZ_PARITY_NONE                             0
#define PTZ_PARITY_ODD                              1
#define PTZ_PARITY_EVEN                             2

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
typedef struct
{
	int ptzIdx;
	char ptzName[10];
	int ptzBaudrate;
	int ptzParitybit;
	int ptzDatabit;
	int ptzStopbit;
}PTZLIST_internalInfo;
typedef struct
{
	int ptzDatabit;
	int ptzParitybit;
	int ptzStopbit;
	int ptzBaudrate;
}PTZLIST_serialInfo;

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
 PTZLIST_internalInfo gPTZLIST_intList[INTERNAL_PTZ_COUNT];
/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
void DVR_start_ptz_if(void);
int DVR_ptz_getIntPtzCount();
int DVR_ptz_getIntPtzInfo(int ptzIdx, char* pPtzName);
int DVR_ptz_setInternalCtrl(int ptzIdx, int targetAddr, int ctrlCmd);
int DVR_RE_ptz_setInternal_Ctrl(char ptzIdx, char targetAddr, char ctrlCmd);
void DVR_RE_ptz_setInfo(char ptzIdx,int baudrate);

int	DVR_ptz_setSerialInfo(int dataBit, int parityBit, int stopBit, int baudRate);
int DVR_ptz_ptzSendBypass(char* ptzBuf, int bufSize);

void DVR_ptz_PelcopDCheckSum(char* pBuf);
void DVR_ptz_PelcopPCheckSum(char* pBuf);
#endif	//_PTZ_IF_H_
