/**************
 * dmessage.h
 */

#ifndef dmessage_h
#define dmessage_h

#include <fdefine.h>

typedef struct __DMessage {
	int channel ;
	char buff [MAXBUFF] ;
	int len ;
} DMessage ;

typedef struct __DMessageManager {
	int cnt ;
	DMessage* Dmessage [MAXUSER] ;
} DMessageManager ;

DMessageManager* Dinit_message_manager () ;
void Dzero_message (DMessage*) ;
void Dzero_message_manager (DMessageManager*) ;
void Dset_message (DMessageManager*, int, char*, int) ;
DMessage* Dget_message (DMessageManager*, int) ;

extern DMessageManager* DgMessageManager ;

#endif // dmessage_h
