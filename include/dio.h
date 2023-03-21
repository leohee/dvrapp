
#ifndef _GPIO_H_
#define _GPIO_H_


#define ALARM_PORT_MAX_NUM	4
#define SENSOR_PORT_MAX_NUM	8

#define SENSOR_START_GPIO	85
#define ALARM_GPIO_NUM1		30
#define ALARM_GPIO_NUM2		32
#define ALARM_GPIO_NUM3 	33
#define ALARM_GPIO_NUM4		36
#define REBOOT_GPIO_NUM		43

#define GPIO_PINMUX1	1
#define GPIO_PINMUX4	4

#define DIO_TYPE_NO	0
#define DIO_TYPE_NC 1

#define ALARM_DEFAULT_TIME 	(10*1000)
#define DIO_DETECT_DELAY	(100)
#define DIO_RESET_TIME		(2*1000)
#define STATUS_MAX_COUNT	(DIO_RESET_TIME/DIO_DETECT_DELAY)

typedef struct {

 int alarmType;
 int alarmTime;
 int alarmEnable;
 int alarmGPIO;
 
 int runFlag;
 int runTime;

} ALARM_info;

typedef struct {

 int sensorType;
 int sensorEnable;
 int sensorGPIO;
 
} SENSOR_info;


int DIO_create();
int DIO_delete();
int DIO_start();
int DIO_stop();

int DIO_runAlarm(int alarmId, int alarmRun);
int DIO_setAlarmInfo(int alarmId, int alarmEnable, int alarmType, int alarmTime);
int DIO_setSensorInfo(int sensorId, int sensorEnable, int sensorType);

int DIO_getNoSignalStatus(int *chSignalStatus);
int DIO_getAlarmStatus(int *chAlarmStatus);
int DIO_getSensorStatus(int *chSensorStatus);

void DIO_setReboot(void);
#endif 
