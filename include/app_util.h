/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_util.h
 * @brief
 * @author	phoong
 * @section	MODIFY history
 *     - 2011.03.28 : First Created
 */
/*****************************************************************************/

#if !defined (_APP_UTIL_H_)
#define _APP_UTIL_H_

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <app_manager.h>
//#include "common_def.h"
/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/


//# for network detect ----------------
#define DEV_ETH0		0
#define DEV_ETH1		1
#define NET_STATIC		0
#define NET_DHCP		1
#define SIZE_NET_STR	16

// for backup 

#define CDROM_SIZE  700*1024
#define DVD_SIZE    4.7*1024*1024

typedef struct {
	char type;					//# 0: static, 1: dhcp
	char state;					//# 0: link down, 1: link up
	char ip[SIZE_NET_STR];		//# ip address
	char mask[SIZE_NET_STR];	//# netmask
	char gate[SIZE_NET_STR];	//# gateway
} dvr_net_info_t;

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
int get_hw_info(char *mac0, char *mac1, char *hwver);
int get_net_info(int devno, dvr_net_info_t *inet);
int set_net_info(int devno, dvr_net_info_t *inet);

int dvr_fdisk(char *dev);
int get_dvr_disk_info(dvr_disk_info_t *ddi);
int get_dvr_disk_size(const char *mount_name, unsigned long *total, unsigned long *used);

void set_rtsp_status(int status);
int get_rtsp_status();

int get_media_capability(long filesize, int media, char *mediapath) ;
int system_user(char *arg);

#endif	//_APP_UTIL_H_
