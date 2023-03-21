/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _APP_AUDIO_PRIV_H_
#define _APP_AUDIO_PRIV_H_

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>		
#include <sched.h>          

#include <app_audio_EncDec.h>
// WARNING - this is the only supported rate - dont change
#define AUDIO_SAMPLE_RATE_SUPPORTED                         16000   //< Only 16KHz supported
#define AUDIO_CHANNELS_SUPPORTED                            1       //< Only mono
#define AUDIO_CHANNEL_SAMPLE_INTERLEAVE_LEN                 1 
#define AUDIO_SAMPLES_TO_READ_PER_CHANNEL                   (250)       //< Ensure to align with hw param BUFFER_SIZE; only < 256 works right now
#define AUDIO_SAMPLE_LEN                                    2
#define AUDIO_PLAYBACK_SAMPLE_MULTIPLIER					16//64		/* For some reason, we have to read / feed this much data for playback */
#define AUDIO_SAMPLE_RATE_HDMI								32000	
#define AUDIO_MODE_NONBLOCK									1
/* We store huge audio samples one shot with a corresponding timestamp 
        == SAMPLES_TO_READ_PER_CHANNEL * PLAYBACK_SAMPLE_MULTIPLIER 
    Driver will consume lot of time in sending out this data, hence huge backend delay
    which we need to adjust when a comparison is done rendered video PTS
*/
#define AUDIO_BACKEND_DELAY_IN_MS           (((AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER) / AUDIO_SAMPLE_RATE_SUPPORTED) * 1000)

#define	ALSA_CAPTURE_DEVICE		"plughw:0,0"

#define ALSA_PLAY_AIC3x         "plughw:0,1"
#define ALSA_PLAY_ONCHIP_HDMI   "plughw:1,0"

#define ALSA_CONTROLS           "default"
 
#define AUDIO_ERROR(...) \
  fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); 

#define     AUD_DEVICE_PRINT_ERROR_AND_RETURN(str, err, hdl)        \
        fprintf (stderr, "AUDIO >> " str, snd_strerror (err));  \
        snd_pcm_close (hdl);    \
        hdl = NULL;             \
        return  -1;

#define AUDIO_CAPTURE_TSK_PRI                           18
#define AUDIO_CAPTURE_TSK_STACK_SIZE                    (10*1024)   

#define AUDIO_PLAY_TSK_PRI                              18
#define AUDIO_PLAY_TSK_STACK_SIZE                       (10*1024)   

#define AUDIO_BUFFER_SIZE                               ((8192*2)>>2)

#define	AUDIO_MAX_PLAYBACK_BUF_SIZE						(120*1024)	/* Max hd read for ~4 secs worth of data assuming 16KHz */

typedef struct  _AudioStats
{
    Int32   errorCnt;
    Int32   lastError;
} AudioStats;

typedef enum {
    AUDIO_PLAY_AIC3x = 0,
    AUDIO_PLAY_ONCHIP_HDMI,
    AUDIO_PLAY_DEVICE_MAX,
    
} ADUIO_PLAY_DEVCIE_t;



#endif  /* _AUDIO_PRIV_H_ */
