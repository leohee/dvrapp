
/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include "osa_mutex.h"
#include "osa_thr.h"
/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
#define GPIO_DEV	"/dev/dvr_io"
#define SENSOR_DURATION 	1	//60 Sec

typedef void (* DIO_Callback) (int event, void* param);

typedef struct {
	unsigned int	gpio;
	unsigned int 	dir;		//# 0:input, 1:output
	unsigned int	trigger;	//# 1:falling, 2:rising
	unsigned int 	val;		//# default value in case output
} dvrio_t;

//# ioctl command
#define DVRIO_CMD_INIT		_IOW('g', 1, dvrio_t)
#define DVRIO_CMD_CHG_IRQ	_IOW('g', 2, dvrio_t)
#define DVRIO_CMD_RD		_IOR('g', 3, dvrio_t)

#define MAX_SENSOR	16
#define MAX_ALARM	4

#define GPIO_INPUT	0
#define GPIO_OUTPUT	1
#define RISING		0x00000001
#define FALLING		0x00000002

#define SENSOR01	26+32
#define SENSOR02	4
#define SENSOR03	5
#define SENSOR04	6
#define SENSOR05	7
#define SENSOR06	21
#define SENSOR07	22
#define SENSOR08	23
#define SENSOR09	24
#define SENSOR10	25
#define SENSOR11	26
#define SENSOR12	27
#define SENSOR13	28
#define SENSOR14	29
#define SENSOR15	30
#define SENSOR16	31

#define ALARM1		12+32
#define ALARM2		13+32
#define ALARM3		14+32
#define ALARM4		15+32

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
unsigned char dvr_sensor_gpio[MAX_SENSOR] = {
	SENSOR01, SENSOR02, SENSOR03, SENSOR04, SENSOR05, SENSOR06, SENSOR07, SENSOR08,
	SENSOR09, SENSOR10, SENSOR11, SENSOR12, SENSOR13, SENSOR14, SENSOR15, SENSOR16
};
unsigned char dvr_alarm_gpio[MAX_ALARM] = {
	ALARM1, ALARM2, ALARM3, ALARM4
};

typedef struct
{
	int iSensorId;
	int iSensorEnable;
	int iSensorType;
	unsigned char status;
	unsigned int updatetime;
}SensorInfo,*pSensorInfo;
typedef struct
{
	int iAlarmId;
	int iAlarmEnable;
	int iAlarmType;
	int iAlarmDelay;
	unsigned char status;
	unsigned int updatetime;
}AlarmInfo,*pAlarmInfo;

SensorInfo sensorInfoArr[MAX_SENSOR];
AlarmInfo alarmInfoArr[MAX_ALARM];

int dio_fd;
DIO_Callback	pdioCbFn=NULL;
OSA_MutexHndl DIOMutex;
time_t cur_time;
/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
#define eprintf(x...) printf("!err-dio: " x);
//#define dprintf(x...) printf("dio: " x);
#define dprintf(x...)

static int dvr_io_init(void)
{
	int fd;
	dvrio_t io;
	int i;

	fd = open(GPIO_DEV, O_RDWR);
	if(fd < 0) {
		eprintf("fd open\n");
		return -1;
	}

	//# init dvr_io
	for(i=0; i<MAX_ALARM; i++)
	{
		io.gpio = dvr_alarm_gpio[i];
		io.dir = GPIO_OUTPUT;
		io.val = 0;
		ioctl(fd, DVRIO_CMD_INIT, &io);
		
		alarmInfoArr[i].iAlarmId = i;
		alarmInfoArr[i].iAlarmEnable = 0;
		alarmInfoArr[i].iAlarmType = 0;
		alarmInfoArr[i].iAlarmDelay = 0;
		alarmInfoArr[i].status = 0;
		alarmInfoArr[i].updatetime = 0;
	}
	for(i=0; i<MAX_SENSOR; i++)
	{
		io.gpio = dvr_sensor_gpio[i];
		io.dir = GPIO_INPUT;
		io.trigger = FALLING;
		ioctl(fd, DVRIO_CMD_INIT, &io);
		
		sensorInfoArr[i].iSensorId = i;
		sensorInfoArr[i].iSensorEnable = 0;
		sensorInfoArr[i].iSensorType = 0;
		sensorInfoArr[i].status = 0;
		sensorInfoArr[i].updatetime = 0;
	}

	return fd;
}

static void dvr_io_exit(int fd)
{
	if(fd) {
		close(fd);
	}
}

int getSensorId(char recv)
{
	int i;
	for(i=0; i<MAX_SENSOR; i++)
	{
		if(recv == dvr_sensor_gpio[i])
			return i;
	}
	return -1;
}

unsigned int readSensor(int SensorId)
{
	dvrio_t io;
	io.gpio = dvr_sensor_gpio[SensorId];
	ioctl(dio_fd, DVRIO_CMD_RD, &io);
	//printf("Read Sensor %d : val =  %d\n",SensorId,io.val);
	return io.val;
}

static void *dio_interface(void *prm)
{
	char recv;
	
	if(dio_fd < 0)
	{
		eprintf("\nerr dvr_io_init\n");
		return NULL;
	}

	dprintf("data read start\n");
	while(1)
	{
		read(dio_fd, &recv, 1);
		int sensoridx = getSensorId(recv);
		if(sensoridx < 0)
		{
			usleep(100000);
		}
		else
		{
			//printf("inturrept gpio[SENSOR%d]\n", sensoridx);
			OSA_mutexLock(&DIOMutex);
			if(sensorInfoArr[sensoridx].iSensorEnable && pdioCbFn)
			{
				sensorInfoArr[sensoridx].status=0x01;
				pdioCbFn(sensoridx,(void*)1);
				sensorInfoArr[sensoridx].updatetime = cur_time;
			}
			OSA_mutexUnlock(&DIOMutex);
		}
	}

	dvr_io_exit(dio_fd);
	OSA_mutexDelete(&DIOMutex);
	return NULL;
}

static void *alarm_handler(void* prm)
{
	int i;
		
	if(dio_fd < 0)
	{
		eprintf("\nerr dvr_io_init\n");
		return NULL;
	}
	
	dprintf("alarm_handler start\n");
	while(1)
	{ 
		time(&cur_time);
		for(i=0;i<MAX_ALARM;i++)
		{
			if(alarmInfoArr[i].iAlarmEnable && (alarmInfoArr[i].status & 0x01))
			{
				if(cur_time - alarmInfoArr[i].updatetime >= (alarmInfoArr[i].iAlarmDelay+1))
				{
					dvrio_t io;
					io.gpio = dvr_alarm_gpio[i];
					alarmInfoArr[i].updatetime = cur_time;
					if(alarmInfoArr[i].iAlarmType==0)	//NO
					{
						io.val = 0;
					}
					else if(alarmInfoArr[i].iAlarmType == 1)	//NC
					{
						io.val = 1;
					}
					
					OSA_mutexLock(&DIOMutex);
					write(dio_fd, &io, sizeof(io));
					OSA_mutexUnlock(&DIOMutex);
					alarmInfoArr[i].status &= 0x10;	
					pdioCbFn(MAX_SENSOR+i,(void*)0);
				}
			}
		}
		for(i=0; i<MAX_SENSOR; i++)
		{
			if((sensorInfoArr[i].status) && (cur_time - sensorInfoArr[i].updatetime >= SENSOR_DURATION) && pdioCbFn)
			{
				int sensorval = readSensor(i);
				if(sensorInfoArr[i].iSensorType == 0)	//NO
				{
					if(sensorval == 1)
					{
						OSA_mutexLock(&DIOMutex);
						sensorInfoArr[i].status &= 0xF0;
						//printf("NO Reset gpio[SENSOR%d]\n", i);
						pdioCbFn(i,(void*)0);
						OSA_mutexUnlock(&DIOMutex);
					}
					else
					{
						sensorInfoArr[i].updatetime = cur_time;
						pdioCbFn(i,(void*)1);
						//printf("NO set gpio[SENSOR%d]\n", i);
					}
				}
				else if(sensorInfoArr[i].iSensorType == 1)	//NC
				{
					if(sensorval == 0)
					{
						OSA_mutexLock(&DIOMutex);
						sensorInfoArr[i].status &= 0xF0;
						//printf("NC Reset gpio[SENSOR%d]\n", i);
						pdioCbFn(i,(void*)0);
						OSA_mutexUnlock(&DIOMutex);
					}
					else
					{
						sensorInfoArr[i].updatetime = cur_time;
						pdioCbFn(i,(void*)1);
						//printf("NC set gpio[SENSOR%d]\n", i);
					}
				}
			}
		}
		
		usleep(200000);
	}
	return NULL;
}

int dvr_send_alarm(int alarmidx,int alarmOnOff)
{
	//# send command to front
	dvrio_t io;
	dprintf("dvr_send_alarm() alarmidx=%d alarmOnOff=%d\n",alarmidx,alarmOnOff);
	
	if(dio_fd < 0)
	{
		eprintf("dvr_send_alarm\n");
		return -1;
	}
	io.gpio = dvr_alarm_gpio[alarmidx];

	if(alarmOnOff && alarmInfoArr[alarmidx].iAlarmEnable)
	{
		//if(!(alarmInfoArr[alarmidx].status & 0x01))
		{
			OSA_mutexLock(&DIOMutex);
			if(alarmInfoArr[alarmidx].iAlarmType==0)	//NO
			{
				io.val = 1;
				alarmInfoArr[alarmidx].status |= 0x10;				
			}
			else if(alarmInfoArr[alarmidx].iAlarmType == 1)	//NC
			{
				io.val = 0;
				alarmInfoArr[alarmidx].status &= 0x01;
			}						
			write(dio_fd, &io, sizeof(io));
			alarmInfoArr[alarmidx].updatetime = cur_time;
			OSA_mutexUnlock(&DIOMutex);
		}
		alarmInfoArr[alarmidx].status |= 0x01;
	}
	else
	{
		if(alarmInfoArr[alarmidx].status & 0x01)
		{
			OSA_mutexLock(&DIOMutex);
			if(alarmInfoArr[alarmidx].iAlarmType==0)	//NO
			{
				io.val = 0;
				alarmInfoArr[alarmidx].status &= 0x01;
			}
			else if(alarmInfoArr[alarmidx].iAlarmType == 1)	//NC
			{
				io.val = 1;
				alarmInfoArr[alarmidx].status |= 0x10;				
			}			
			write(dio_fd, &io, sizeof(io));
			OSA_mutexUnlock(&DIOMutex);
		}
		alarmInfoArr[alarmidx].status &= 0x10;		
	}
	return 0;
}

void DVR_start_dio_if(void)
{
	OSA_ThrHndl task;
	OSA_ThrHndl alarm_task;
	
	dio_fd = dvr_io_init();
	
	OSA_mutexCreate(&DIOMutex);
	OSA_thrCreate (&task, dio_interface, OSA_THR_PRI_DEFAULT,(50*1024),	"dio_if_task" );
	
	OSA_thrCreate (&alarm_task, alarm_handler, OSA_THR_PRI_DEFAULT, (50*1024), "alarm_task" );
}

void DVR_set_dio_callback(void* cbfn)
{
	pdioCbFn = (DIO_Callback)cbfn;
}
void DVR_setSensor(int iSensorId,int iSensorEnable,int iSensorType)
{
	if(iSensorId >= MAX_SENSOR)
		return;
	
	dprintf("DVR_setSensor() iSensorId=%d iSensorEnable=%d iSensorType=%d\n",iSensorId,iSensorEnable,iSensorType);
		
	sensorInfoArr[iSensorId].iSensorEnable = iSensorEnable;
	if(sensorInfoArr[iSensorId].iSensorType != iSensorType)
	{
		//Reset Sensor Type
		dvrio_t io;
		sensorInfoArr[iSensorId].iSensorType = iSensorType;
		io.gpio = dvr_sensor_gpio[iSensorId];
		io.dir = GPIO_INPUT;
		if(sensorInfoArr[iSensorId].iSensorType == 0)	//NO
		{
			io.trigger = FALLING;
		}
		else if(sensorInfoArr[iSensorId].iSensorType == 1)	//NC
		{
			io.trigger = RISING;
		}
		ioctl(dio_fd, DVRIO_CMD_CHG_IRQ, &io);
	}
}

void DVR_setAlarm(int iAlarmId,int iAlarmEnable,int iAlarmType,int iAlarmDelay)
{
	if(iAlarmId >= MAX_ALARM)
		return;
	
	dprintf("DVR_setAlarm() iAlarmId=%d iAlarmEnable=%d iAlarmType=%d iAlarmDelay=%d\n",iAlarmId,iAlarmEnable,iAlarmType,iAlarmDelay);
	alarmInfoArr[iAlarmId].iAlarmEnable=iAlarmEnable;
	if(alarmInfoArr[iAlarmId].iAlarmEnable && alarmInfoArr[iAlarmId].iAlarmType != iAlarmType)
	{
		//Reset Alarm Type
		dvrio_t io;
		io.gpio = dvr_alarm_gpio[iAlarmId];
		io.dir = GPIO_OUTPUT;
		alarmInfoArr[iAlarmId].iAlarmType = iAlarmType;
		if(alarmInfoArr[iAlarmId].iAlarmType==0)	//NO
		{
			io.val = 0;
		}
		else if(alarmInfoArr[iAlarmId].iAlarmType == 1)	//NC
		{
			io.val = 1;
		}
		write(dio_fd, &io, sizeof(io));
	}
	else if(!alarmInfoArr[iAlarmId].iAlarmEnable)
	{
		dvrio_t io;
		io.gpio = dvr_alarm_gpio[iAlarmId];
		io.val = 0;
		write(dio_fd, &io, sizeof(io));
	}
	alarmInfoArr[iAlarmId].iAlarmDelay=iAlarmDelay;
}

unsigned int DVR_getSensorStatus()
{
	unsigned int ret=0;
	int i;
	for(i=0;i<MAX_SENSOR;i++)
	{
		if(sensorInfoArr[i].status & 0x01)
		{
			ret|=(0x1<<i);
		}
	}
	
	return ret;
}

unsigned int DVR_getAlarmStatus()
{
	unsigned int ret=0;
	int i;
	for(i=0;i<MAX_ALARM;i++)
	{
		if(alarmInfoArr[i].status & 0x01)
		{
			ret|=(0x1<<i);
		}
	}
	
	return ret;
}