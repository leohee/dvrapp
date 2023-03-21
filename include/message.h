/**************
 * message.h
 */

#ifndef message_h
#define message_h

#include <fdefine.h>

typedef struct __Message {
	int channel ;
	char buff [MAXBUFF] ;
	int len ;
} Message ;

typedef struct __MessageManager {
	int cnt ;
	Message* message [MAXUSER] ;
} MessageManager ;

MessageManager* init_message_manager () ;
void zero_message (Message*) ;
void zero_message_manager (MessageManager*) ;
void set_message (MessageManager*, int, char*, int) ;
Message* get_message (MessageManager*, int) ;

extern MessageManager* gMessageManager ;

#endif // message_h
