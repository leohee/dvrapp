/***************
 * msghandler.h
 */
#include <fdefine.h>
#include <process.h>

#ifndef msghandler_h
#define msghandler_h

void msginit(void) ;
void init_msgfunc () ;
extern void (*msgfunc [MAXCOMMANDS])(int, char*, int) ;

#endif // msghandler_h
