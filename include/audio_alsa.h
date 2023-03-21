/*
 * sound alsa library wrapper 
 */
#ifndef _AUDIO_ALSA_H_
#define _AUDIO_ALSA_H_

#include <app_audio.h>
#include <app_audio_priv.h>

// added udworks_jhhan
typedef struct {
	Int8 *device;
	snd_pcm_t *handle;         /* pcm handle */
	
	snd_pcm_format_t format;   /* sound format */
	snd_pcm_stream_t stream;   /* capture or playback */
	
	/* interrupt duration (the number of frames) */
	snd_pcm_uframes_t periods;
	
	Int32 resample;
		
	Uint32 channels;
	Uint32 rate;
	
	Uint32 bits_per_frame;
	
	int start_delay; /* set start threshold */
	int stop_delay;  /* set stop threshold */
	
} Alsa_Env;

Int32 Audio_alsaOpen(Alsa_Env *pEnv, int nonblock);
Int32 Audio_alsaSetparams(Alsa_Env *pEnv, int verbose);
Int32 Audio_alsaClose(Alsa_Env *pEnv, int nonblock);

Int32 Audio_alsaXrun(snd_pcm_t *handle, snd_pcm_stream_t stream);
Int32 Audio_alsaSuspend(snd_pcm_t *handle);
Int32 Audio_alsaState(snd_pcm_t *handle);

#endif

