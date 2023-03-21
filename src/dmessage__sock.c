/***********
 * dmessage.c
 */

#include <dmessage.h>
#include <stdlib.h>
#include <string.h>

DMessageManager* DgMessageManager ;

DMessageManager* Dinit_message_manager ()
{
	int i = 0 ;
	DMessageManager* Dmanager ;
	Dmanager = (DMessageManager*) malloc (sizeof (DMessageManager)) ;

	for (i=0; i<MAXUSER; i++) {
		Dmanager->Dmessage [i] = (DMessage*) malloc (sizeof (DMessage)) ;
		Dzero_message (Dmanager->Dmessage [i]) ;
	}
	return Dmanager ;
}

void Dzero_message (DMessage* Dmsg)
{
	Dmsg->channel  = 0 ;
	Dmsg->buff [0] = 0 ;
	Dmsg->len      = 0 ;
}

void Dzero_message_manager (DMessageManager* Dmanager)
{
	int i = 0 ;
	for (i=0; i<MAXUSER; i++) {
		Dzero_message (Dmanager->Dmessage [i]) ;
	}
	Dmanager->cnt = 0 ;
}

DMessage* Dget_message (DMessageManager* Dmanager, int i)
{
	return Dmanager->Dmessage [i] ;
}

void Dset_message (DMessageManager* Dmanager, int channel, char* buff, int len)
{
	DMessage* Dmsg = Dmanager->Dmessage [Dmanager->cnt] ;
	
	Dmsg->channel = channel ;
	memcpy (Dmsg->buff, buff, len) ;
	Dmsg->len = len ;
	Dmanager->cnt += 1 ;
}

// end of dmessage.c
