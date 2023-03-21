/********************************
** avi.h
**
** by CSNAM, 2010/11/24
**
** Copyright nSolutions Co.
**
*********************************/

#ifndef __NSDVR_AVI_DEF__
#define __NSDVR_AVI_DEF__

#include <common_def.h>

/********************************
** Constants
*********************************/

/********************************
** Structures
*********************************/

typedef struct {
	char file_name[NSDVR_MAX_PATH_LENGTH];	
	int smi_flag;		
	int video_encoding;	// NSDVR_ENCODING_MPEG4/NSDVR_ENCODING_H264
	int video_fps;	// 30(NTSC)/25(PAL)
	int video_width;
	int video_height ;
	int audio_flag;
} stNSDVR_AVIInfo;

/********************************
** Functions
*********************************/

handle nsDVR_AVI_create(stNSDVR_AVIInfo* pInfo, int *error_code);
int nsDVR_AVI_add_frame(handle hAVI, stNSDVR_FrameInfo* pFrame);
void nsDVR_AVI_close(handle hAVI);

#endif // __NSDVR_AVI_DEF__
