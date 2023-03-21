#ifndef _encrypt_h__
#define _encrypt_h__


extern int createKeys(unsigned long int *pPrivateKey, unsigned long int *pPublicKey, unsigned long int *pModNumber);
extern unsigned long int ss_encrypt(unsigned long int publicKey,unsigned long int modNumber,
                                    unsigned char *srcBufPtr,	unsigned long int srcSize,
				    unsigned char *outBufPtr,	unsigned long int outBufSize);
extern unsigned long int ss_decrypt (unsigned long int privateKey, unsigned long int modNumber, 
				     unsigned char *srcBufPtr,	unsigned long int srcSize,
				     unsigned char *outBufPtr,	unsigned long int outBufSize);
unsigned short CheckTimer(void) ;

#endif
