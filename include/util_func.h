/********************************
** util_func.h
**
** by CSNAM, 2010/11/24
**
** Copyright nSolutions Co.
**
*********************************/

#ifndef __NSDVR_UTILITY_FUNC__
#define __NSDVR_UTILITY_FUNC__

/***************************************************************************** 
 ** Time fuinctions
 *****************************************************************************/
typedef enum {
	NSDVR_DATE_FORMAT_YYYYMMDD,
	NSDVR_DATE_FORMAT_MMDDYYYY,
	NSDVR_DATE_FORMAT_DDMMYYYY,
} enumNSDVR_DATE_FORMAT;

typedef enum {
	NSDVR_TIME_FORMAT_24HOUR,
	NSDVR_TIME_FORMAT_AMPM,
} enumNSDVR_TIME_FORMAT;

extern void nsDVR_util_set_time_format(int date_format, int time_format);
extern char* nsDVR_util_time_str_r(time_t lt, char* buf, int size);
extern char* nsDVR_util_time_str(time_t lt);
extern time_t nsDVR_util_make_time(int year, int mon, int day, int hour, int min, int sec);
extern time_t nsDVR_util_parse_time_str_with_format(char* date_str, char* time_str, int date_format, int time_format);
extern time_t nsDVR_util_parse_time_str(char* date_str, char* time_str);

/***************************************************************************** 
 ** Code name functions
 *****************************************************************************/
extern char* nsDVR_util_error_name(int code);
extern char* nsDVR_util_event_name(int code);
extern char* nsDVR_util_basket_status_name(int code);
extern char *nsDVR_util_play_mode_name(int mode);
extern char* nsDVR_util_play_direction_name(int direction);


#endif // __NSDVR_UTILITY_FUNC__
