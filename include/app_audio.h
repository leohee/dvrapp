/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _APP_AUDIO_H_
#define _APP_AUDIO_H_

#include <osa.h>
#include "app_manager.h"

#define AUDIO_MAX_CHANNELS                              (MAX_DVR_CHANNELS)///< Max number of audio channel supported for recording / playback

#define AUDIO_STATUS_OK                                 0  ///< Status : OK
#define AUDIO_STATUS_EFAIL                              -1  ///< Status : Generic error

#define AUDIO_SAMPLE_RATE_DEFAULT                       16000
#define AUDIO_VOLUME_DEFAULT                            5

typedef struct {
    int state ; // Use in Live 0 , Use in Playback 1 
    int livechannel ;
    int pbchannel ;
} AUDIO_STATE ;

extern AUDIO_STATE gAudio_state ;   // hwjun

/**
    \brief Start Audio capture

    - Start audio capture / file write

    \return AUDIO_STATUS_OK on success
*/
Int32 Audio_captureStart (Void);

/**
    \brief Stop Audio capture

    - Stop audio capture / file write

    \return AUDIO_STATUS_OK on success
*/
Int32 Audio_captureStop (Void);

Int32 Audio_captureAudio(Uint8 *buffer, Int32 numSamples);

Int8 Audio_captureGetIndexMap(Int8 chNum);

Int32 Audio_captureIsStart();

Int32 Audio_captureGetLastErr();

Int32 Audio_captureGetPeriods(Void);

Int32 Audio_captureGetSampleLength(Void);

Int32 Audio_captureGetFrameLength(Void);

/**
    \brief Start Audio playback

    - Start audio file read / playback

    \return AUDIO_STATUS_OK on success
*/
Int32 Audio_playStart (UInt32 playbackDevId);
Int32 Audio_playSetVolume(Int32 value);

/**
    \brief Stop Audio playback

    - Stop audio playback

    \return AUDIO_STATUS_OK on success
*/
Int32 Audio_playStop (Int32 id);

Int32 Audio_playAudio(Int32 id, Int8 *data, Int32 count);

Int32 Audio_playIsStart(Int32 id);

Int32 Audio_alsaSetControl(Int32 numid, Int8 *argv);

#endif  /* _AUDIO_H_ */

