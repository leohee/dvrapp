#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef __cplusplus
extern "C"{
#endif
#include <stdio.h>


#define eprintf(x...) printf("err: " x);
//#define eprintf(x...)

//#define dprintf(x...) printf(x);
#define dprintf(x...)

#define MAX_CH					(16)

typedef enum {
	IDX_VFDC_LIVE = 0,
	IDX_VFDC_PB,
	IDX_VFDC_SPOT,
} VfdcIdx_e;

#define IDX_VFPC_DEIM			(0)
#define IDX_VFPC_DEIH			(1)

#define KEY_FRAME_NAL_SIZE		(24)

#define FRAME_TYPE_I			(0)
#define FRAME_TYPE_P			(1)


/************************/
/* Possible combination */
/* Young(20110629)      */
/************************/
/*
 1. Only CMUX_DMUX_LOOPBACK is available.
 2. Only SUPPORT_HW_TEST_FUNCTION is not available.
 3. Both CMUX_DMUX_LOOPBACK + SUPPORT_HW_TEST_FUNCTION are available for hardware stretching test.
*/
//#define CMUX_DMUX_LOOPBACK
//#define SUPPORT_HW_TEST_FUNCTION

//#define MULTI_HDD_SUPPORT					(1)

//#define CMUX_WRITER_TEST

typedef struct DVR_CONTEXT_T
{
	int		bNTSC;
	void 	*qt_pipe_id;
} DVR_CONTEXT;

DVR_CONTEXT gDvrCtx;

////MULTI-HDD SKSUNG////
char gMac0[20];
char gMac1[20];
char gHWver[20];

#ifdef __cplusplus
}
#endif

#endif

