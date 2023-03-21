#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include <app_audio_priv.h>
#include <audio_alsa.h>

Int32 Audio_alsaOpen(Alsa_Env *pEnv, int nonblock)
{
    Int32 err = 0;
    snd_pcm_t *handle;
	int open_mode = 0;
	
	if (nonblock)
		open_mode |= SND_PCM_NONBLOCK;
	
	err = snd_pcm_open (&handle, pEnv->device, pEnv->stream, open_mode);
    if (err < 0)
        return err;
    
	if (nonblock) {
		err = snd_pcm_nonblock(handle, 1);
		if (err < 0) {
			fprintf(stderr, "nonblock setting error: %s", snd_strerror(err));
			snd_pcm_close(handle);
			pEnv->handle = NULL;
			return err;
		}
	}
	
    pEnv->handle = handle;
    return 0;
}

Int32 Audio_alsaSetparams(Alsa_Env *pEnv, int verbose)
{
    Int32 err = 0;
    Uint32 rate, n;
    
    snd_pcm_t *handle;
    snd_output_t *log;
    
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams; 
    
    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t start_threshold, stop_threshold;
    
	unsigned int buffer_time, period_time;
	
    handle = pEnv->handle;
    
    err = snd_output_stdio_attach(&log, stderr, 0);
    OSA_assert(err >= 0);
    
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);
    
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) { 
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("Broken configuration for this PCM:"
                                          "no configurations available(%s)\n", err, handle); 
    } 
    
    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set access type (%s)\n", err, handle); 
    } 
    
    err = snd_pcm_hw_params_set_format(handle, params, pEnv->format);
    if (err < 0) { 
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set sample format (%s)\n", err, handle); 
    }
    
    err = snd_pcm_hw_params_set_channels(handle, params, pEnv->channels); 
    if (err < 0) { 
        AUD_DEVICE_PRINT_ERROR_AND_RETURN("cannot set channel count (%s)\n", err, handle); 
    }
    
    rate = pEnv->rate;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &pEnv->rate, 0);
    OSA_assert(err >= 0);
    
    if ((float)rate * 1.05 < pEnv->rate || (float)rate * 0.95 > pEnv->rate) {
        fprintf(stderr, "Warning: rate is not accurate"
                        "(requested = %iHz, got = %iHz)\n", rate, pEnv->rate);
    }
    rate = pEnv->rate;
    
    /* following setting of period size is done only for AIC3X. Leaving default for HDMI */
    if (pEnv->resample) {
        /* Restrict a configuration space to contain only real hardware rates. */
        snd_pcm_hw_params_set_rate_resample(handle, params, 1);
    } 
    
	buffer_time = 0;
	period_time = 0;
	if (pEnv->periods == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
		OSA_assert(err >= 0);
		
		/* in microsecond */
		if (buffer_time > 500000)
			buffer_time = 500000; /* 500ms */ 
	}
	
	if (buffer_time > 0)
		period_time = buffer_time / 4;
	
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle, params,
							     &period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle, params,
							     &pEnv->periods, 0);
	OSA_assert(err >= 0);
	
	if (period_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
							     &buffer_time, 0);
	} else {
		buffer_size = pEnv->periods * 4;
		err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
							     &buffer_size);
    }
    OSA_assert(err >= 0);
    
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) { 
		fprintf(stderr, "cannot set alsa hw parameters %d\n", err);
		return err; 
    } 
    
    /* Get alsa interrupt duration */
    snd_pcm_hw_params_get_period_size(params, &pEnv->periods, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (pEnv->periods == buffer_size) {
        fprintf(stderr, "Can't use period equal to buffer size (%lu == %lu)\n",
                                                pEnv->periods, buffer_size);
        return -1;
    }
    
    /* set software params */
    snd_pcm_sw_params_current(handle, swparams);
    
    n = pEnv->periods;
    /* set minimum avil size -> 1 period size */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);
    OSA_assert(err >= 0);
    
    n = buffer_size;
    /* in microsecond -> divide 1000000 */
    if (pEnv->start_delay <= 0) 
        start_threshold = n + (double)rate * pEnv->start_delay / 1000000;
    else
        start_threshold = (double)rate * pEnv->start_delay / 1000000;
    
    if (start_threshold < 1)
        start_threshold = 1;
        
    if (start_threshold > n)
        start_threshold = n;
    
    /* set pcm auto start condition */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 
                                            start_threshold);
    OSA_assert(err >= 0);
    
    /* in microsecond -> divide 1000000 */
    if (pEnv->stop_delay <= 0) 
        stop_threshold = buffer_size + (double)rate * pEnv->stop_delay / 1000000;
    else
        stop_threshold = (double)rate * pEnv->stop_delay / 1000000;
    
    err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, 
                                            stop_threshold);
    OSA_assert(err >= 0);

    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        fprintf(stderr, "unable to install sw params\n");
        return err;
    }
    
    if (verbose)
        snd_pcm_dump(handle, log);
    
    snd_output_close(log);
    
    return err;
}

Int32 Audio_alsaClose(Alsa_Env *pEnv, int nonblock)
{
    Int32 err = 0;
    snd_pcm_t *handle = pEnv->handle;
    
    if (handle) {
        if (pEnv->stream == SND_PCM_STREAM_PLAYBACK) {
            snd_pcm_nonblock(handle, 0);
            snd_pcm_drain(handle);
            /* blocking->0 non-blocking -> 1*/
			snd_pcm_nonblock(handle, nonblock); 
        }
        snd_pcm_close(handle); 
    }
    
    return err;
}

Int32 Audio_alsaXrun(snd_pcm_t *handle, snd_pcm_stream_t stream)
{
    snd_pcm_status_t *status;
    Int32 err;
    
    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(handle, status)) < 0) {
        fprintf(stderr,"status error: %s\n", snd_strerror(err));
        return err;
    }
    
    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		fprintf(stderr, "%s: %s !!!\n", snd_pcm_name(handle),
				(stream == SND_PCM_STREAM_PLAYBACK)?"underrun":"overrun");
		
        if ((err = snd_pcm_prepare(handle))< 0) {
            fprintf(stderr, "xrun: prepare error: %s\n", snd_strerror(err));
            return err;
        }
        /* ok, data should be accepted again */
        return 0;       
    } 
    
    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        if (stream == SND_PCM_STREAM_CAPTURE) {
            fprintf(stderr, "capture stream format change? attempting recover...\n");
            if ((err = snd_pcm_prepare(handle)) < 0) {
                fprintf(stderr, "xrun(DRAINING): prepare error: %s\n", snd_strerror(err));
                return err;
            }
            return 0;
        }
    }
    
    fprintf(stderr, "read/write error, state = %s", 
                        snd_pcm_state_name(snd_pcm_status_get_state(status)));
    return -1;
}

Int32 Audio_alsaSuspend(snd_pcm_t *handle)
{
    Int32 err = 0;
    
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
        sleep(1);   /* wait until suspend flag is released */
    
    if (err < 0) {
        if ((err = snd_pcm_prepare(handle)) < 0) {
            fprintf(stderr, "suspend: prepare error: %s\n", snd_strerror(err));
            return err;
        }
    }
    return err;
}

Int32 Audio_alsaState(snd_pcm_t *handle)
{
    snd_pcm_state_t state;
    Int32 ret = 0;
        
    if (!handle)
        return ret;
        
    state = snd_pcm_state(handle);
    if (state == SND_PCM_STATE_RUNNING) {
        ret = 1;
    }
    
    return ret;
}
