/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_util.c
 * @brief
 * @author	phoong
 * @section	MODIFY history
 *     - 2011.03.28 : First Created
 */
/*****************************************************************************/

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

//# for network info
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

//# for mount info
#include <sys/vfs.h>
#include <sys/stat.h>

#include "app_util.h"
#include "app_manager.h"
#include "lib_dvr_app.h"

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
#define EEPROM_DEV	"/dev/eeprom"

#define ROM_MAC_ADDR0	0
#define ROM_MAC_SIZE	8
#define ROM_MAC_ADDR1	(ROM_MAC_ADDR0+ROM_MAC_SIZE)

#define ROM_HW_ADDR		(ROM_MAC_ADDR1+ROM_MAC_SIZE)
#define ROM_HW_SIZE		2

#define ROM_INFO_SIZE	(ROM_MAC_SIZE+ROM_MAC_SIZE+ROM_HW_SIZE)

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
static int gRtspStatus 	= 0;

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
#define eprintf(x...)	printf("err: " x);
//#define dprintf(x...)	printf(x);
#define dprintf(x...)

/*----------------------------------------------------------------------------
 local function
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Board H/W Info
-----------------------------------------------------------------------------*/
int get_hw_info(char *mac0, char *mac1, char *hwver)
{
	int fd;
	unsigned char a[ROM_INFO_SIZE];

	fd = open((const char *)EEPROM_DEV, O_RDWR);
	if (fd  < 0) {
		perror("EEPROM_DEV");
		eprintf("eeprom device open, %s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
		return -1;
	}
	read(fd, a, ROM_INFO_SIZE);
	close(fd);

	sprintf(mac0, "%02x:%02x:%02x:%02x:%02x:%02x",
		a[ROM_MAC_ADDR0+1], a[ROM_MAC_ADDR0+2], a[ROM_MAC_ADDR0+3], a[ROM_MAC_ADDR0+4], a[ROM_MAC_ADDR0+5], a[ROM_MAC_ADDR0+6]);
	sprintf(mac1, "%02x:%02x:%02x:%02x:%02x:%02x",
		a[ROM_MAC_ADDR1+1], a[ROM_MAC_ADDR1+2], a[ROM_MAC_ADDR1+3], a[ROM_MAC_ADDR1+4], a[ROM_MAC_ADDR1+5], a[ROM_MAC_ADDR1+6]);
	sprintf(hwver, "%x.%x",
		a[ROM_HW_ADDR], a[ROM_HW_ADDR+1]);

	return 0;
}

/*----------------------------------------------------------------------------
 Network Info
-----------------------------------------------------------------------------*/
static int gateway_info(char *dev, char *gateway, int set)
{
	FILE *fp;
	unsigned char buf[128], gate[16];
	unsigned char *find;

	//# get gateway
	sprintf(buf, "route -n | grep 'UG[ \t]' | grep %s | awk '{print $2}'", dev);

	fp = popen(buf, "r");
	if(NULL == fp) {
		eprintf("popen error (%s)\n", buf);
		return -1;
	}

	if(!fgets(gate, 16, fp))	{
		strcpy(gate, "0.0.0.0");
	}
	else {
		find = strchr(gate,'\n');	//# remove '\n'
		if(find) *find='\0';
	}
	pclose(fp);

	if(set)	//# set gateway
	{
		if(strcmp(gate, "0.0.0.0")) {
			sprintf(buf, "route del default gw %s %s", gate, dev);
			system_user(buf);
		}
		sprintf(buf, "route add default gw %s %s", gateway, dev);
		system_user(buf);
	}
	else
	{
		strcpy(gateway, gate);
	}

	return 0;
}

int get_net_info(int devno, dvr_net_info_t *inet)
{
	int ret, fd;
	char dev[8];
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	sprintf(dev, "eth%d", devno);
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

	//# check up/down
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	inet->state = ifr.ifr_flags & IFF_UP;

	#if 1
	if(!inet->state) {	//# down
		close(fd);
		strcpy(inet->ip, "0.0.0.0");
		strcpy(inet->mask, "255.255.255.0");
		strcpy(inet->gate, "0.0.0.0");
		return 0;
	}
	#endif

	ret = ioctl(fd, SIOCGIFADDR, &ifr);
	if(ret<0)	strcpy(inet->ip, "0.0.0.0");
	else		sprintf(inet->ip, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	ret = ioctl(fd, SIOCGIFNETMASK, &ifr);
	if(ret<0)	strcpy(inet->mask, "255.255.255.0");
	else		sprintf(inet->mask, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	close(fd);

	gateway_info(dev, inet->gate, 0);

	return 0;
}

int set_net_info(int devno, dvr_net_info_t *inet)
{
	int ret, fd;
	char dev[8], cmd[32];
	struct ifreq ifr;
  	struct sockaddr_in sin;

  	if(!strcmp(inet->ip, "0.0.0.0"))
  		return -1;

  	sprintf(dev, "eth%d", devno);

  	fd = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	//# check up/down
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	inet->state = ifr.ifr_flags & IFF_UP;
	if(!inet->state) {	//# down
		ifr.ifr_flags |= IFF_UP;
		ioctl(fd, SIOCSIFFLAGS, &ifr);
		sleep(1);
	}

  	if(inet->type == NET_DHCP)
  	{
  		sprintf(cmd, "udhcpc -n -i %s", dev);
  		ret = system_user(cmd);
  	}
  	else if(inet->type == NET_STATIC)
  	{
		//memset(&sin, 0, sizeof(struct sockaddr));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(inet->ip);
		memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
		ioctl(fd, SIOCSIFADDR, &ifr);

		sin.sin_addr.s_addr = inet_addr(inet->mask);
		memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
		ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
		if(ret < 0)
			dprintf("netmask: Invalid argument\n");

		gateway_info(dev, inet->gate, 1);
	}

	close(fd);

	return ret;
}

/*----------------------------------------------------------------------------
 RTSP setting
-----------------------------------------------------------------------------*/
void set_rtsp_status(int status)
{
	gRtspStatus = status;
}

int get_rtsp_status()
{
	return gRtspStatus;
}

/*--------------------------------------------
 Media size check for backup
---------------------------------------------*/

int get_media_capability(long backupsize, int media, char *mediapath)
{
    int i = 0, ret = 0 ;
	long cd_media_size = 0 ;

	unsigned long totalsize = 0, usedsize = 0, subsize = 0 ;
	subsize = backupsize / 100 ;



	if(media == TYPE_CDROM)
	{
		ret = LIB816x_CDROM_MEDIA(mediapath) ;
        if(ret == 0) // cdrom
            cd_media_size = CDROM_SIZE ; // KB
        else if(ret == 2)
        	cd_media_size = DVD_SIZE ;
        else if(ret == 1)
			return FALSE ; // INVALID MEDIA

		if(backupsize > cd_media_size)
			return FALSE ; // not sufficient size
		else
			return TRUE ;

	}
	else
	{
    	ret = LIB816x_disk_size(mediapath, &totalsize, &usedsize);

		if(ret == 0)
		{
			if((unsigned long)backupsize + subsize > totalsize - usedsize)
			{
				printf("backupsize = %ld totalsize = %ld usedsize = %ld\n",backupsize, totalsize, usedsize) ;
				return FALSE ;
			}
			else
			{
				printf("backupsize = %ld totalsize = %ld usedsize = %ld\n",backupsize, totalsize, usedsize) ;
				return TRUE ;
			}
		}
		else
		{
			printf("fail get disk size..\n") ;

		}
	}
	return FALSE ;

}

/*----------------------------------------------------------------------------
 SYSTEM function
-----------------------------------------------------------------------------*/
/* SYSTEM function implementation using fork() and execlp() calls */
/* vfork() can be used instead of fork() which avoids data space duplication for the child process and hence is faster */
/* vfork() is safe because child process in not going to modify the common data space */

int system_user(char *arg)
{
    int numArg,i,j,k,len,status;
    unsigned int chId;
    char exArg[10][64];

    if(arg[0] == '\0')
        return 0;

    j   = 0;
	k   = 0;
	len = strlen(arg);

    for(i = 0;i < len;i ++)
    {
        if(arg[i] == ' ')
        {
		    exArg[j][k] = '\0';
		    j ++;
		    k = 0;
		}
		else
		{
		    exArg[j][k] = arg[i];
		    k ++;
		}
	}

    if(exArg[j][k - 1] == '\n') {
	    exArg[j][k - 1] = '\0';
	}
	else {
	    exArg[j][k] = '\0';
	}

	numArg = j + 1;

	if(numArg > 10)
	{
	    printf("\nThe no of arguments are greater than 10,calling standard system function...\n");
	    return(system(arg));
	}

    chId = fork();

    if(chId == 0)
    {
	    // child process
	    switch(numArg)
	    {
		    case 1:
		        execlp(exArg[0],exArg[0],NULL);
		        break;
		    case 2:
		        execlp(exArg[0],exArg[0],exArg[1],NULL);
		        break;
		    case 3:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],NULL);
		        break;
		    case 4:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],NULL);
		        break;
		    case 5:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               NULL);
		        break;
		    case 6:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               exArg[5],NULL);
		        break;
		    case 7:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               exArg[5],exArg[6],NULL);
		        break;
		    case 8:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               exArg[5],exArg[6],exArg[7],NULL);
		        break;
		    case 9:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               exArg[5],exArg[6],exArg[7],exArg[8],NULL);
		        break;
		    case 10:
		        execlp(exArg[0],exArg[0],exArg[1],exArg[2],exArg[3],exArg[4],
		               exArg[5],exArg[6],exArg[7],exArg[8],exArg[9],NULL);
		        break;
		}

        printf("\nexeclp failed ...\n");
	    exit(0);
	}
	else if(chId < 0)
	{
		return -1;
	}
	else
	{
		// parent process
		// wait for the completion of the child process
		waitpid(chId,&status,0);
	}

    return 0;
}
