/******************************************************************************
 * DVR Netra Board
 * Copyright by UDWorks, Incoporated. All Rights Reserved.
 *---------------------------------------------------------------------------*/
 /**
 * @file	app_front_if.c
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <termios.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "app_front_if.h"
#include "osa_thr.h"
/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
#define UART1_DEV	"/dev/ttyO1"		//# Front Micom
#define UINPUT_DEV	"/dev/input/uinput"

#define CMD_KEY		0x10
#define CMD_IR		0x20
#define CMD_JOG		0x40
#define CMD_SHU		0x80
#define CMD			0xff

#define PRESS		0x1
#define RELEASE		0x2
#define R_LONG		0x4

//# receive command define - never do not define 0x0
#define REC_ON		0x30		//# '0'
#define REC_OFF		0x31		//# '1'
#define REC_TOGGLE	0x32

#define REBOOT		0x40

#define SHU_OFFSET	8
#define SHU_LEFT	1
#define SHU_RIGHT	2
#define SHU_IDLE	3

typedef struct {
	unsigned char inkey;
	unsigned char outkey;
} key_map;

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
static unsigned char key_code[] = {
	KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8,//# channel
	KEY_9, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
	KEY_H,		//# 0x11, LIVE
	KEY_I,		//# 0x12, SEARCH
	KEY_J,		//# 0x13, SCRMODE
	KEY_K,		//# 0x14, FULLSCR
	KEY_L,		//# 0x15, PREV
	KEY_M,		//# 0x16, NEXT
	KEY_N,		//# 0x17, SEQ
	KEY_O,		//# 0x18, RF
	KEY_P,		//# 0x19, RSTEP
	KEY_Q,		//# 0x1A, PLAYPAUSE
	KEY_R,		//# 0x1B, FSTEP
	KEY_S,		//# 0x1C, FF
	KEY_T,		//# 0x1D, STOP
	KEY_U,		//# 0x1E, PTZ
	KEY_V,		//# 0x1F, REC
	KEY_W,		//# 0x20, SETUP

	KEY_UP,		//# 0x21
	KEY_LEFT,	//# 0x22
	KEY_ENTER,	//# 0x23
	KEY_RIGHT,	//# 0x24
	KEY_DOWN,	//# 0x25

	KEY_ESC,	//KEY_POWER->KEY_NUMLOCK,	//# 0x26, POWER
};

static key_map ir_key[] = {
	{0x7f, KEY_1},
	{0xbf, KEY_2},
	{0x3f, KEY_3},
	{0x5f, KEY_4},
	{0x9f, KEY_5},
	{0x1f, KEY_6},
	{0x6f, KEY_7},
	{0xaf, KEY_8},
	{0x2f, KEY_9},
	//{0xe7, KEY_0},

	{0xf7, KEY_UP},
	{0xb7, KEY_LEFT},
	{0xf, KEY_ENTER},
	{0x77, KEY_RIGHT},
	{0x37, KEY_DOWN},

	{0xd7, KEY_I},	//# SEARCH
	{0xc7, KEY_O},	//# RF
	{0x47, KEY_Q},	//# PLAYPAUSE
	{0x87, KEY_S},	//# FF
	{0x36, KEY_T},	//# STOP

	{0x3d, KEY_N},	//# SEQ

	{0xff, 0}
};

static key_map shu_key[] = {
	{70, KEY_HOME},
	{60, KEY_F1},
	{50, KEY_F2},
	{40, KEY_F3},
	{30, KEY_F4},
	{20, KEY_F5},
	{10, KEY_F6},
	{0xff, 0},

	{10, KEY_F7},
	{20, KEY_F8},
	{30, KEY_F9},
	{40, KEY_F10},
	{50, KEY_F11},
	{60, KEY_F12},
	{70, KEY_END},
	{0xff, 0},
};

struct uinput_user_dev   uinp;
struct input_event       event;

static int uart_fd;

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
#define eprintf(x...)	printf("err: " x);
//#define dprintf(x...)	printf(x);
#define dprintf(x...)

/*----------------------------------------------------------------------------
 local function
-----------------------------------------------------------------------------*/
static int uart_init(void)
{
	int fd;
	struct termios oldtio, newtio;

	fd = open((const char *)UART1_DEV, O_RDWR);
    dprintf("open %s returned %d\n", UART1_DEV, fd);
	if (fd  < 0) {
		eprintf("uart1 device open\n");
		return -1;
	}

	tcgetattr(fd, &oldtio);

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;
	newtio.c_cc[VMIN]     = 1;

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	return fd;
}

static int uinput_init(void)
{
	int fd;
	int ret=0, i;

	fd = open(UINPUT_DEV, O_WRONLY | O_NDELAY );
	dprintf("open %s returned %d\n", UINPUT_DEV, fd);
    if (fd == 0) {
		eprintf("Could not open uinput.\n");
		return -1;
    }

	memset(&uinp, 0, sizeof(uinp));
    strncpy(uinp.name, "dvr-keypad", 10);
    uinp.id.version = 4;
    uinp.id.bustype = BUS_VIRTUAL;	//BUS_USB//BUS_VIRTUAL

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    //ioctl(fd, UI_SET_EVBIT, EV_REL);

    for (i=0; i<KEY_MAX; i++) {
		ioctl(fd, UI_SET_KEYBIT, i);
    }

    //# create input device in input subsystem
    write(fd, &uinp, sizeof(uinp));
    //dprintf("First write returned %d.\n", ret);

    ret = (ioctl(fd, UI_DEV_CREATE));
    if (ret) {
		eprintf("Error create uinput device %d.\n", ret);
		return -1;
    }

	return fd;
}

static unsigned char key_map_ir(unsigned char inputkey)
{
	key_map *kmap = &ir_key[0];

	while (kmap->inkey != 0xff)
	{
		if(kmap->inkey == inputkey) {
			dprintf("irkeymap 0x%x -> %d\n", kmap->inkey, kmap->outkey);
			return kmap->outkey;
		}
		kmap++;
	}

	return 0;
}

static unsigned char key_map_shu(unsigned char direct, unsigned char degree)
{
	key_map *kmap;

	if(direct ==  SHU_LEFT) {
		kmap = &shu_key[SHU_OFFSET];
	} else if(direct == SHU_RIGHT) {
		kmap = &shu_key[0];
	} else if(direct == SHU_IDLE) {
		return KEY_DELETE;
	}

	while (kmap->inkey != 0xff)
	{
		if(kmap->inkey == degree) {
			dprintf("shukeymap %d -> %d\n", kmap->inkey, kmap->outkey);
			return kmap->outkey;
		}
		kmap++;
	}

	return 0;
}

static void send_ev_input(int fd, int keycode, int type)
{
	memset(&event, 0, sizeof(event));
	gettimeofday(&event.time, NULL);

	event.type = EV_KEY;
	event.code = keycode;
	event.value = type;			//# 1:key pressed, 0:key released
	write(fd, &event, sizeof(event));

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	write(fd, &event, sizeof(event));
}

static void *front_interface(void* prm)
{
	int ret=0;
	int input_fd;
	int type=0;
	char send[2], recv[3];
	unsigned char keycode;

	input_fd = uinput_init();
	if(input_fd < 0)
		return;

	uart_fd = uart_init();
	if(uart_fd < 0)
		return;

	//# send command for linux booting done
	send[0] = REC_OFF;
	send[1] = 0x00;
	write(uart_fd, &send, sizeof(send));

	printf("front_interface start\n");

	while(1)
	{
		memset(recv, 0 , sizeof(recv));
		ret = read(uart_fd, &recv, sizeof(recv));
		if(ret < sizeof(recv)) {
			continue;
		}

		keycode = 0;
		switch(recv[0])
		{
			case CMD_KEY:
			{
				if(recv[1]==PRESS) {
					type = 1;
				}
				else if(recv[1]==RELEASE || recv[1]==R_LONG) {
					type = 0;
				}
				if(recv[2]!=0x26)
					send_ev_input(input_fd, key_code[recv[2]-1], type);
				printf("key \t0x%x recv[1]:%d (%d)\n", recv[2],recv[1], type);

				#if 1
				if(recv[2]==0x26 && recv[1]==R_LONG)
				{
					send_ev_input(input_fd, key_code[recv[2]-1], 1);
				}
				#endif
				break;
			}
			case CMD_IR:
			{
				keycode = key_map_ir(recv[2]);
				break;
			}
			case CMD_JOG:
			{
				if(recv[1]==1) {
					dprintf("Jog reft\n");
					keycode = KEY_PAGEDOWN;
				}
				else if(recv[1]==2) {
					dprintf("Jog right\n");
					keycode = KEY_PAGEUP;
				}
				break;
			}
			case CMD_SHU:
			{
				if(recv[1]==SHU_LEFT) {
					keycode = key_map_shu(SHU_LEFT, recv[2]);
					dprintf("shu reft\t");
				}
				else if(recv[1]==SHU_RIGHT) {
					keycode = key_map_shu(SHU_RIGHT, recv[2]);
					dprintf("shu right\t");
				}
				else if(recv[1]==SHU_IDLE) {
					keycode = key_map_shu(SHU_IDLE, recv[2]);
					dprintf("shu idle \t");
				}

				dprintf("%d\n", recv[2]);
				break;
			}
			case CMD:
			{
				break;
			}
			default:
				dprintf("0x%x, 0x%x, 0x%x\n", recv[0], recv[1], recv[2]);
				break;
		}

		if(keycode) {
			send_ev_input(input_fd, keycode, 1);
			send_ev_input(input_fd, keycode, 0);
		}
	}

    //# destroy the device
    ioctl(input_fd, UI_DEV_DESTROY);

    if(input_fd>0) {
		close(input_fd);
		dprintf("close input_fd\n");
	}
    if(uart_fd>0) {
		close(uart_fd);
		dprintf("close uart_fd\n");
	}
}

int dvr_send_front(int cmd)
{
	char send[2];

	if(!uart_fd) {
		return -1;
	}

	//# send command to front
	switch(cmd)
	{
		case RECORD_START:
		{
			send[0] = REC_TOGGLE;
			send[1] = 0x00;
			write(uart_fd, &send, sizeof(send));
			break;
		}
		case RECORD_STOP:
		{
			send[0] = REC_OFF;
			send[1] = 0x00;
			write(uart_fd, &send, sizeof(send));
			break;
		}
		case SYSTEM_REBOOT:
		{
			send[0] = REBOOT;
			send[1] = 0x00;
			write(uart_fd, &send, sizeof(send));
			system("reboot");
			break;
		}
		default:
			break;
	}

	return 0;
}

void DVR_start_front_if(void)
{
	OSA_ThrHndl task ;

	OSA_thrCreate (&task, front_interface, OSA_THR_PRI_DEFAULT,(50*1024), "front_if_task" );
}
