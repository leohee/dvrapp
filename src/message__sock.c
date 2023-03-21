/***********
 * message.c
 */

#include <message.h>
#include <stdlib.h>
#include <string.h>

MessageManager* gMessageManager ;

MessageManager* init_message_manager ()
{
	int i = 0 ;
	MessageManager* manager ;
	manager = (MessageManager*) malloc (sizeof (MessageManager)) ;

	for (i=0; i<MAXUSER; i++) {
		manager->message [i] = (Message*) malloc (sizeof (Message)) ;
		zero_message (manager->message [i]) ;
	}
	return manager ;
}

void zero_message (Message* msg)
{
	msg->channel  = 0 ;
	msg->buff [0] = 0 ;
	msg->len      = 0 ;
}

void zero_message_manager (MessageManager* manager)
{
	int i = 0 ;
	for (i=0; i<MAXUSER; i++) {
		zero_message (manager->message [i]) ;
	}
	manager->cnt = 0 ;
}

Message* get_message (MessageManager* manager, int i)
{
	return manager->message [i] ;
}

void set_message (MessageManager* manager, int channel, char* buff, int len)
{
	Message* msg = manager->message [manager->cnt] ;
	
	msg->channel = channel ;
	memcpy (msg->buff, buff, len) ;
	msg->len = len ;
	manager->cnt += 1 ;
}

// end of message.c
