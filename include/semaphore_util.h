/** ===========================================================================
* @file semaphore_util.h
*
* @path $(IPNCPATH)\multimedia\encode_stream\stream
*
* @desc 
* .
*
* =========================================================================== */

#ifndef _SEM_UTIL_H_
#define _SEM_UTIL_H_

int Sem_Creat( int index );
int Sem_unlock( int index );
int Sem_lock( int index );
int Sem_kill( int index );


#endif
