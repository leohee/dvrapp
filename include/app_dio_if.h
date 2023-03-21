/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_dio_if.h
 * @brief
 * @author	phoong
 * @section	MODIFY history
 *     - 2011.03.01 : First Created
 */
/*****************************************************************************/

#if !defined (_DIO_IF_H_)
#define _DIO_IF_H_

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
//# command to front

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
void DVR_start_dio_if(void);
int dvr_send_alarm(int alarmidx,int alarmOnOff);
void DVR_set_dio_callback(void* cbfn);
void DVR_setSensor(int iSensorId,int iSensorEnable,int iSensorType);
void DVR_setAlarm(int iAlarmId,int iAlarmEnable,int iAlarmType,int iAlarmDelay);
unsigned int DVR_getSensorStatus();
unsigned int DVR_getAlarmStatus();

#endif	//_DIO_IF_H_
