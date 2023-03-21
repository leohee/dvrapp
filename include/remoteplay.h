
#ifndef _REMOTEPLAY_H_
#define _REMOTEPLAY_H_

//#include <system.h>

int REMOTEPLAY_create(void);
int REMOTEPLAY_delete(void);
int REMOTEPLAY_start(int channel, unsigned long rectime, int sockchannel);
int RemotePlayback(int channel, int streamtype, int vtype, void *buffer, int framesize, unsigned long Timestamp, int resolution) ;
int PLAYBACK_pauseCh(int sockch) ;
int PLAYBACK_streamCh(int sockch) ;
int PLAYBACK_stopCh(int sockch) ;
int PLAYBACK_status(void) ;

#endif

