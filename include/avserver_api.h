#ifndef _AV_SERVER_API_H_
#define _AV_SERVER_API_H_

#include <avserver_config.h>

int AVSERVER_init();
int AVSERVER_exit();

int AVSERVER_start(AVSERVER_Config *config);
int AVSERVER_stop();

int AVSERVER_rtspServerStart();
int AVSERVER_rtspServerStop();


#endif  /*  _AV_SERVER_H_  */

