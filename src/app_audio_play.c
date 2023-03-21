/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/            *
 *                        ALL RIGHTS RESERVED                                 *
 *                                                                             *
 ******************************************************************************/
#include <math.h>
#include <app_audio.h>
#include <app_audio_priv.h>
#include <audio_alsa.h>

static Int32        playAudioFlag[AUDIO_PLAY_DEVICE_MAX];
static AudioStats   playStats[AUDIO_PLAY_DEVICE_MAX];
static Alsa_Env     playbackEnv[AUDIO_PLAY_DEVICE_MAX];

#define check_range(val, min, max) \
	((val < min) ? (min) : (val > max) ? (max) : (val))
		
#define convert_prange1(val, min, max) \
	ceil((val) * ((max) - (min)) * 0.01 + (min))
	
static Int32 InitAudioRenderDevice(Alsa_Env *pEnv)
{
    Int32 err = -1;
	Uint32 bits_per_sample = 0;
    
    if (pEnv == NULL) {
        fprintf(stderr, "%s: invalid environment!!\n", __func__);
        return err;
    }
    
	err = Audio_alsaOpen(pEnv, AUDIO_MODE_NONBLOCK);
    if (err < 0) {
        fprintf (stderr, "AUDIO >>  cannot open audio device %s (%s)\n", pEnv->device, snd_strerror (err)); 
        return err; 
    }
    
    err = Audio_alsaSetparams(pEnv, 0);
    if (err < 0) {
        fprintf (stderr, "AUDIO >> cannot set configuration device %s (%s)\n", pEnv->device, snd_strerror (err)); 
        return err; 
    }
    
    /* alloc playback buffer */ 
    bits_per_sample = snd_pcm_format_physical_width(pEnv->format);
	pEnv->bits_per_frame = bits_per_sample * pEnv->channels;
    
    return err;
}

static void deInitAudioPlayback (Alsa_Env *pEnv)
{
	Audio_alsaClose(pEnv, AUDIO_MODE_NONBLOCK);
    
    if (pEnv->device) {
        free(pEnv->device);
        pEnv->device = NULL;
    }
}

Int32 Audio_playStart (UInt32 playbackDevId)
{
	Int32    *flags   = &playAudioFlag[playbackDevId];
	Alsa_Env *alsaenv = &playbackEnv[playbackDevId];

	/* Initialize alsa device */
	memset(flags, 0, sizeof(*flags));
	memset(alsaenv, 0, sizeof(*alsaenv));
	
	alsaenv->format      = SND_PCM_FORMAT_S16_LE;
	alsaenv->stream	     = SND_PCM_STREAM_PLAYBACK;
	alsaenv->start_delay = 0;
	alsaenv->rate     = AUDIO_SAMPLE_RATE_DEFAULT;
	alsaenv->channels = AUDIO_CHANNELS_SUPPORTED;
			
    if (playbackDevId == AUDIO_PLAY_ONCHIP_HDMI) {
		alsaenv->device   = strdup(ALSA_PLAY_ONCHIP_HDMI);
		alsaenv->resample = 0;
    } 
    else {
        /* default playback device -> aic3x */
		alsaenv->device   = strdup(ALSA_PLAY_AIC3x);
		alsaenv->resample = 1;
	}
	/* Set period size to 1000 frames. */
	// do not change this. This matches with period size printed using aplay
	alsaenv->periods = 1000;
			
	InitAudioRenderDevice(alsaenv);
	*flags = 1;
	
    sleep(1);
    
    return 0;
}

Int32 Audio_playStop (Int32 id)
{
    playAudioFlag[id] = 0;
    sleep(1);
    deInitAudioPlayback(&playbackEnv[id]);
        
    return 0;
}

static Int32 Audio_write(int id, char *data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;
	int err = 0;
	
	snd_pcm_t *handle = playbackEnv[id].handle;
	size_t bits_per_frame = playbackEnv[id].bits_per_frame;
	
	while (count > 0) {
		r = snd_pcm_writei(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 1000);
		} 
		else if (r == -EPIPE) {
			err = Audio_alsaXrun(handle, SND_PCM_STREAM_PLAYBACK);
			if (err < 0) {
				fprintf(stderr, "xrun error\n");
				return err;
			}
		} 
		else if (r == -ESTRPIPE) {
			err = Audio_alsaSuspend(handle);
			if (err < 0) {
				fprintf(stderr, "suspend error\n");
				return err;
			}
		} 
		else if (r < 0) {
			fprintf(stderr, "write error: %s", snd_strerror(r));
			return -1;
		}
		
		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	
	return result;
}

Int32 Audio_playAudio(Int32 id, Int8 *data, Int32 numBytes)
{
    Int32 err = -1;
	Int32 r, written;
	int count, l;
	
	Uint32 periods_bytes;
	
	char *buffer;
	
	snd_pcm_t *handle     = playbackEnv[id].handle;
	Uint32 periods        = playbackEnv[id].periods;
	Uint32 bits_per_frame = playbackEnv[id].bits_per_frame;
    
    if (!handle) {
        fprintf(stderr, "AUDIO >> Error - Device play_handle is NULL..\n");
        return err;
    }
    
	buffer = (char *)data;
	periods_bytes = (periods * bits_per_frame / 8); 
	
	written = 0;
	
	while (written < numBytes) 
	{
		l = numBytes - written;
			
		if (l > periods_bytes)
			l = periods_bytes; 
		/*
		 * input samples are tvp audio. 
		 * 1sample = 16bit. 1 frame 1/16(mono)->2 byte
		 */
		count = (l * 8 / 16 /*bits_per_frame*/);
		
		r = Audio_write(id, buffer, count);
		if (r != count)
			goto err_exit;
		
		r = r * 16 / 8; /* bytes */
		written += r;
		buffer += r;
	}
	
	return written;

err_exit:
    playStats[id].errorCnt++;
    playStats[id].lastError = err;
            
    return err;
}

Int32 Audio_playIsStart(Int32 id)
{
    return playAudioFlag[id];
}

Int32 Audio_playSetVolume(Int32 value)
{
	Int32 err = 0;
	
	long val, pmin, pmax;
	
	snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t* elem;
    snd_mixer_t *handle = NULL;
    
    snd_mixer_selem_id_alloca(&sid);
    /* set sset, index = 0(fixed) */
    snd_mixer_selem_id_set_index(sid, 0);
    /* name = PCM */
    snd_mixer_selem_id_set_name(sid, "PCM");
     /* mixer open */
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    /* mixer basic mode (abstraction mode-> need to mixer option */
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);
    
    elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		fprintf(stderr, "Unable to find simple control '%s',%i\n", snd_mixer_selem_id_get_name(sid), 
															snd_mixer_selem_id_get_index(sid));
		snd_mixer_close(handle);
		handle = NULL;
		return -1;
	}
	
	err = snd_mixer_selem_has_playback_volume(elem);
	if (!err) {
		fprintf(stderr, "Unable to get playback volume\n");
		snd_mixer_close(handle);
		return -1;
	}
	
	/* get range */
	snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
	
	val = (long)convert_prange1(value, pmin, pmax);
	val = check_range(val, pmin, pmax);
	
	snd_mixer_selem_set_playback_volume(elem, 0, val); /* left */
	snd_mixer_selem_set_playback_volume(elem, 1, val); /* right */
	snd_mixer_close(handle);
	
	return 0;
}
