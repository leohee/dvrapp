/***********
 * cmessage.c
 */
#include <stdlib.h>
#include <string.h>
#include <cmessage.h>

CMessageManager* CgMessageManager ;

CMessageManager* Cinit_message_manager ()
{
	int i = 0 ;
	CMessageManager* Cmanager ;
	Cmanager = (CMessageManager*) malloc (sizeof (CMessageManager)) ;

	for (i=0; i<MAXUSER; i++) {
		Cmanager->Cmessage [i] = (CMessage*) malloc (sizeof (CMessage)) ;
		Czero_message (Cmanager->Cmessage [i]) ;
	}
	return Cmanager ;
}

void Czero_message (CMessage* Cmsg)
{
	Cmsg->channel  = 0 ;
	Cmsg->buff [0] = 0 ;
	Cmsg->len      = 0 ;
}

void Czero_message_manager (CMessageManager* Cmanager)
{
	int i = 0 ;
	for (i=0; i<MAXUSER; i++) {
		Czero_message (Cmanager->Cmessage [i]) ;
	}
	Cmanager->cnt = 0 ;
}

CMessage* Cget_message (CMessageManager* Cmanager, int i)
{
	return Cmanager->Cmessage [i] ;
}

void Cset_message (CMessageManager* Cmanager, int channel, char* buff, int len)
{
	CMessage* Cmsg = Cmanager->Cmessage [Cmanager->cnt] ;
	
	Cmsg->channel = channel ;
	memcpy (Cmsg->buff, buff, len) ;
	Cmsg->len = len ;
	Cmanager->cnt += 1 ;
}

// end of cmessage.c
