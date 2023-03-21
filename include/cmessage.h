/**************
 * cmessage.h
 */

#ifndef cmessage_h
#define cmessage_h

#include <fdefine.h>

typedef struct __CMessage {
	int channel ;
	char buff [MAXBUFF] ;
	int len ;
} CMessage ;

typedef struct __CMessageManager {
	int cnt ;
	CMessage* Cmessage [MAXUSER] ;
} CMessageManager ;

CMessageManager* Cinit_message_manager () ;
void Czero_message (CMessage*) ;
void Czero_message_manager (CMessageManager*) ;
void Cset_message (CMessageManager*, int, char*, int) ;

CMessage* Cget_message (CMessageManager*, int) ;

extern CMessageManager* CgMessageManager ;

#endif // cmessage_h
