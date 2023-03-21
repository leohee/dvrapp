/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <app_audio.h>
#include <app_audio_priv.h>
#include <audio_alsa.h>

#define         MAX_STR_SIZE        256

static Int32 InitAudioCaptureDevice (Alsa_Env *pEnv);
static Int32    deInitAudioCaptureDevice(Void);
static Int32    recordAudioFlag = 0;

static Int8     audioPhyToDataIndexMap[AUDIO_MAX_CHANNELS];

static AudioStats captureStats;
static Alsa_Env captureEnv;

/**< Audio volume, Valid values: 0..8. Refer to TVP5158 datasheet for details */
UInt32 audioVolume = AUDIO_VOLUME_DEFAULT;

#ifdef AUDIO_G711CODEC_ENABLE
Uint8 audioG711codec = TRUE;
#endif

Int32 Audio_captureStart (Void)
{
    #if (AUDIO_MAX_CHANNELS == 4)
    /*
    Channel Mapping for 4-channels audio capture
    <-----------------64bits----------------->
    <-16bits->
    --------------------------------------------
    | S16-0  | S16-1  | S16-2  | S16-3 |
    --------------------------------------------
    | AIN 3 | AIN 0 | AIN 2 | AIN 1 |
    --------------------------------------------
    */
    audioPhyToDataIndexMap[0]   = 1;
    audioPhyToDataIndexMap[1]   = 3;
    audioPhyToDataIndexMap[2]   = 2;
    audioPhyToDataIndexMap[3]   = 0;
    
    #else
    /*
    Channel Mapping for 16-channels audio capture
    <---------------------------------------------------------------------------------256bits------------------------------------------------------------------------------------>
    <-16bits->
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    | S16-0  | S16-1 | S16-2 | S16-3 | S16-4 | S16-5 | S16-6  | S16-7  | S16-8  | S16-9 | S16-10 | S16-11 | S16-12 | S16-13 | S16-14 | S16-15 |
    ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    | AIN 16 | AIN 1 | AIN 3 | AIN 5 | AIN 7 | AIN 9 | AIN 11 | AIN 13 | AIN 15 | AIN 2 | AIN 4  | AIN 6  | AIN 8  | AIN 10 | AIN 12 | AIN 14 |
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    */

    audioPhyToDataIndexMap[0]   = 1;
    audioPhyToDataIndexMap[1]   = 9;
    audioPhyToDataIndexMap[2]   = 2;
    audioPhyToDataIndexMap[3]   = 10;
    
    audioPhyToDataIndexMap[4]   = 3;
    audioPhyToDataIndexMap[5]   = 11;
    audioPhyToDataIndexMap[6]   = 4;
    audioPhyToDataIndexMap[7]   = 12;
    
    audioPhyToDataIndexMap[8]   = 5;
    audioPhyToDataIndexMap[9]   = 13;
    audioPhyToDataIndexMap[10]  = 6;
    audioPhyToDataIndexMap[11]  = 14;

    audioPhyToDataIndexMap[12]  = 7;
    audioPhyToDataIndexMap[13]  = 15;
    audioPhyToDataIndexMap[14]  = 8;
    audioPhyToDataIndexMap[15]  = 0;
    #endif
    
    /* Initialize alsa device */
    memset(&captureEnv, 0, sizeof(Alsa_Env));
    
    captureEnv.device   = strdup(ALSA_CAPTURE_DEVICE);
    
    captureEnv.format   = SND_PCM_FORMAT_S16_LE;
    captureEnv.channels = AUDIO_MAX_CHANNELS;
    captureEnv.rate     = AUDIO_SAMPLE_RATE_DEFAULT;
    captureEnv.stream    = SND_PCM_STREAM_CAPTURE;
    captureEnv.start_delay  = 1; /* set to non-zero */
    
    InitAudioCaptureDevice(&captureEnv);
    sleep(1);
    recordAudioFlag = 1;
    
    return 0;
}

Int32 Audio_captureStop (Void)
{
    recordAudioFlag = 0;
    sleep(1);
    deInitAudioCaptureDevice();
    
    return 0;
}

Int8 Audio_captureGetIndexMap(Int8 chNum)
{
    Int8 idxMap = -1;
    if(chNum < AUDIO_MAX_CHANNELS)
        idxMap = audioPhyToDataIndexMap[chNum];

    return idxMap;
}

Int32 Audio_captureIsStart()
{
    return recordAudioFlag;
}

Int32 Audio_captureGetLastErr()
{
    return captureStats.lastError;
}

Int32 Audio_captureGetPeriods(Void)
{
    return (Int32)captureEnv.periods;
}

Int32 Audio_captureGetSampleLength(Void)
{
    Int32 sampleLen;
    
    sampleLen = snd_pcm_format_physical_width(captureEnv.format)/8;
    
    return (Int32)sampleLen;
}

Int32 Audio_captureGetFrameLength(Void)
{
	Int32 ret = 0;
	
	ret = (captureEnv.bits_per_frame / 8);
	
	return (Int32)ret;
}

static Int32 InitAudioCaptureDevice (Alsa_Env *pEnv)
{
    Int32 err = -1; 
	Uint32 bits_per_sample;
    
    if (pEnv == NULL) {
        fprintf(stderr, "%s: invalid environment!!\n", __func__);
        return err;
    }
    
	/* blocking mode */
	err = Audio_alsaOpen(pEnv, AUDIO_MODE_NONBLOCK);
    if (err < 0) {
        fprintf (stderr, "AUDIO >>  cannot open audio device %s (%s)\n", pEnv->device, snd_strerror (err)); 
        return  err; 
    }
    
    err = Audio_alsaSetparams(pEnv, 0);
    if (err < 0) {
        fprintf (stderr, "AUDIO >> cannot set configuration device %s (%s)\n", pEnv->device, snd_strerror (err)); 
        return  err; 
    }
            
    /* 1 frame = # channels * sample bytes 
     * tvp5158 1 frames = 16CH*2 = 32bytes
     * 1 period = the number of frames = chunk_size (ALSA interrupt)
     *          = 256 frames = 8192 bytes
     * if 16Khz, 16ch captured = 16000*(16ch)*(2bytes/sample) = 512000 bytes/sec
     *          = 8192 / 512000 = 0.016 = 16ms
     * buffer size >= at least (2*period)
     */
    /* alloc capture buffer */ 
    bits_per_sample = snd_pcm_format_physical_width(pEnv->format);
	pEnv->bits_per_frame = bits_per_sample * pEnv->channels;
	
    return err;
}

Int32 Audio_captureAudio(Uint8 *buffer, Int32 numSamples)
{
    Int32 err = -1;
    snd_pcm_t *handle = captureEnv.handle;
    Int32 result = 0;
    Uint32 count = numSamples; 
    
    if (!handle) {
        return err;
    }
    
    if (count > captureEnv.periods) {
        count = captureEnv.periods;
    }
    
    while (count > 0) { 
        err = snd_pcm_readi(handle, buffer, count);
        if (err == -EAGAIN || (err >= 0 && (size_t)err < count)) {
            snd_pcm_wait(handle, 1000);
        } 
        else if (err == -EPIPE) {
            err = Audio_alsaXrun(handle, captureEnv.stream);
            if (err < 0) 
                goto err_exit;
        } 
        else if (err == -ESTRPIPE) {
            err = Audio_alsaSuspend(handle);
            if (err < 0) 
                goto err_exit;
        } 
        else if (err < 0) {
            fprintf(stderr, "read error: %s\n", snd_strerror(err));
            goto err_exit;
        }
        
        if (err > 0) {
            result += err;
            count -= err;
			buffer += (err * captureEnv.bits_per_frame / 8);
        }
    }
    
    return result;
        
err_exit:
    captureStats.errorCnt++;
    captureStats.lastError = err;
    
    return err;
}

static Int32 deInitAudioCaptureDevice(Void)
{
	Audio_alsaClose(&captureEnv, AUDIO_MODE_NONBLOCK);
    
    if (captureEnv.device) {
        free(captureEnv.device);
        captureEnv.device = NULL;
    }
    
    return 0;
}
