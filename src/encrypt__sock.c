#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/time.h>
#include <encrypt.h>

static unsigned long int encrypt(unsigned long int nNum, unsigned long int nKeyE, unsigned long int nModN);
static unsigned long int decrypt(unsigned long int nNum, unsigned long int nKeyD, unsigned long int nModN);
static unsigned long int getModulo(unsigned long int nSub, unsigned long int nExp, unsigned long int nMod);
static unsigned long int getGDC(unsigned long int a, unsigned long int b);
static unsigned short int getPrime (unsigned short int maxLimit);
static void swapBytes (unsigned char *bufferPtr, unsigned long int size);
static void swapBits (unsigned char *bufferPtr, unsigned long int size);

#define NUMBER_PRIME	300
unsigned short int primeNumbers[NUMBER_PRIME] = {
	2,   3,   5,   7,  11,  13,  17,  19,  23,  29,		        /* 10 */
	31,  37,  41,  43,  47,  53,  59,  61,  67,  71,		/* 20 */
	73,  79,  83,  89,  97, 101, 103, 107, 109, 113,		/* 30 */
	127, 131, 137, 139, 149, 151, 157, 163, 167, 173,		/* 40 */
	179, 181, 191, 193, 197, 199, 211, 223, 227, 229,		/* 50 */
	233, 239, 241, 251, 257, 263, 269, 271, 277, 281,		/* 60 */
	283, 293, 307, 311, 313, 317, 331, 337, 347, 349,		/* 70 */
	353, 359, 367, 373, 379, 383, 389, 397, 401, 409,		/* 80 */
	419, 421, 431, 433, 439, 443, 449, 457, 461, 463,		/* 90 */
	467, 479, 487, 491, 499, 503, 509, 521, 523, 541,		/* 100 */
	547, 547, 563, 569, 571, 577, 587, 593, 599, 601,		/* 110 */
	607, 613, 617, 619, 631, 641, 643, 647, 653, 659,		/* 120 */
	661, 673, 677, 683, 691, 701, 709, 719, 727, 733,		/* 130 */
	739, 743, 751, 757, 761, 769, 773, 787, 797, 809,		/* 140 */
	811, 821, 823, 827, 829, 839, 853, 857, 859, 863,		/* 150 */
        877, 881, 883, 887, 907, 911, 919, 929, 937, 941,		/* 160 */
        947, 953, 967, 971, 977, 983, 991, 997,1009,1013,		/* 170 */
        1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,		/* 180 */
        1087,1091,1093,1097,1103,1109,1117,1123,1129,1151,		/* 190 */
	1153,1163,1171,1181,1187,1193,1201,1213,1217,1223,		/* 200 */
	1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,		/* 210 */
	1297,1301,1303,1307,1319,1321,1327,1361,1367,1373,		/* 220 */
	1381,1399,1409,1423,1427,1429,1433,1439,1447,1451,		/* 230 */
	1453,1459,1471,1481,1483,1487,1489,1493,1499,1511,		/* 240 */
	1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,		/* 250 */
	1597,1607,1607,1609,1613,1619,1621,1627,1637,1657,		/* 260 */
	1663,1667,1669,1693,1697,1699,1709,1721,1723,1733,		/* 270 */
	1741,1747,1753,1759,1777,1783,1787,1789,1801,1811,		/* 280 */
	1823,1831,1847,1861,1867,1871,1873,1877,1879,1889,		/* 290 */
	1901,1907,1913,1931,1933,1979,1987,1993,1997,1999};		/* 300 */
	


unsigned short CheckTimer()
{
    struct timeval t ;

    gettimeofday( &t, (int)NULL) ;
    return(t.tv_usec/1000) ;

}

/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: createKeys
* INPUT		: pPrivateKey - pointer to 32bit unsigned integer
*             pPublicKey - pointer to 32bit unsigned integer
*             pModNumber - pointer to 32bit unsigned integer
* OUTPUT	: pPrivateKey - private key
*             pPublicKey - public key
*             pModNumber - mod number
* RETURN	: 0 if successful / -1 if failed
* COMMENTS	: 
*
*END*--------------------------------------------------------------*/
int createKeys(unsigned long int *pPrivateKey, unsigned long int *pPublicKey, unsigned long int *pModNumber)
{
	unsigned long int primeP, primeQ;
	unsigned long int temp, cnt;

	
	for (;;) {

		/* Make the first prime number */
		primeP = getPrime(0x0FFF);
		if (primeP == 0) return (-1);			/* failed to get prime number */

//		sched_yield();

		/* Make the second prime number */
		primeQ = getPrime(0xFFFF / (unsigned short int)primeP);
		if (primeQ == 0) return (-1);			/* failed to get prime number */

//		sched_yield();

		/* CAUTION!!!  
		 * primeQ must not the same value as the primeP
		 * primeP * primeQ must be lower than 0xFFFF
		 */
		if ((primeP * primeQ < 0xFFFF) && (primeP != primeQ)) break;
		
	}

	*pModNumber = primeP * primeQ;
	temp = (primeP-1) * (primeQ-1);

	/* Create public Key */
	for (cnt=2; cnt<temp; cnt++) {
		if (getGDC(cnt, temp) == 1) {
			*pPublicKey = cnt;
			break;
		}
		if (cnt == temp-1) return (-1);
	}

//	sched_yield();

	/* Create private Key */
	for (cnt=2; cnt<temp; cnt++) {
		if (getModulo(cnt * *pPublicKey, 1, temp) == 1) {
			*pPrivateKey = cnt;
			break;
		}
		if (cnt == temp-1) return (-1);
	}

//	sched_yield();

	return (0);
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: ss_encrypt
* INPUT		: publicKey - key to encrypt
*             modNumber - mod number
*             srcBufPtr - encrypt할 코드가 들어있는 buffer pointer
*             srcSize	- encrypt할 코드의 byte 크기(buffer의 크기가 아님)
*             outBufPtr - encrypt 결과가 저장될 buffer pointer
*             outBufSize - encrypt 결과가 저장될 buffer의 크기
* OUTPUT	: outBufPtr - encrypted code
* RETURN	: encrypt된 cypher text의 byte size / 0 if failed or error
* COMMENTS	: outBufSize가 encrypt된 코드의 크기보다 작으면 error
*
*END*--------------------------------------------------------------*/
unsigned long int ss_encrypt (	unsigned long int publicKey,	unsigned long int modNumber, 
								unsigned char *srcBufPtr,		unsigned long int srcSize,
								unsigned char *outBufPtr,		unsigned long int outBufSize)
{
	unsigned long int i;
	unsigned long int cypher;

//        printf("ss_encrypt...function.. reached..1\n") ;
	if (srcSize == 0) return 0;					/* invalid plain text */
	
	/* 1-byte character is encypted to 4-byte code */
	if (srcSize * 4 > outBufSize) return 0;		/* insufficient output buffer room */
		
//        printf("ss_encrypt...function.. reached..2\n") ;
	for (i=0; i < srcSize; i++) {
		cypher = encrypt(srcBufPtr[i], publicKey, modNumber);
		outBufPtr[i*4 + 3]	= (unsigned char)( cypher & 0x000000FF);
		outBufPtr[i*4 + 2]	= (unsigned char)((cypher & 0x0000FF00) >> 8);
		outBufPtr[i*4 + 1]	= (unsigned char)((cypher & 0x00FF0000) >> 16);
		outBufPtr[i*4 + 0]	= (unsigned char)((cypher & 0xFF000000) >> 24);
//		sched_yield();
	}

	/* shuffle bytes & bits */
	swapBytes (outBufPtr, srcSize*4);
	swapBits (outBufPtr, srcSize*4);

	return srcSize * 4;
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: ss_decrypt
* INPUT		: privateKey - key to decrypt
*             modNumber - mod number
*             srcBufPtr - decrypt할 코드가 들어있는 buffer pointer
*             srcSize	- decrypt할 코드의 byte 크기(buffer의 크기가 아님)
*             outBufPtr - decrypt 결과가 저장될 buffer pointer
*             outBufSize - decrypt 결과가 저장될 buffer의 크기
* OUTPUT	: outBufPtr - decrypted code
* RETURN	: decrypt된 plain text의 byte size / 0 if failed or error
* COMMENTS	: outBufSize가 decrypt된 코드의 크기보다 작으면 error
*
*END*--------------------------------------------------------------*/
unsigned long int ss_decrypt (	unsigned long int privateKey,	unsigned long int modNumber, 
				unsigned char *srcBufPtr,		unsigned long int srcSize,
				unsigned char *outBufPtr,		unsigned long int outBufSize)
{
	unsigned long int i;
	unsigned long int plain, cypher, temp32;

//        printf("ss_decrypt...function.. reached..srcSize = %ld\n",srcSize) ;

	if ((srcSize == 0) || (srcSize % 4 != 0)) return 0;		/* invalid cipher text */
		
	/* 4-byte cipher text is decypted to 1-byte character */
	if ((srcSize / 4) > outBufSize) return 0;		/* insufficient output buffer room */

	/* de-shuffle bytes & bits */
	swapBits (srcBufPtr, srcSize);
	swapBytes (srcBufPtr, srcSize);

	for (i=0; i < srcSize/4; i++) 
        {
		temp32 = srcBufPtr[i*4 + 3];	cypher = temp32;
		temp32 = srcBufPtr[i*4 + 2];	cypher |= (temp32 << 8);
		temp32 = srcBufPtr[i*4 + 1];	cypher |= (temp32 << 16);
		temp32 = srcBufPtr[i*4 + 0];	cypher |= (temp32 << 24);

		plain = decrypt(cypher, privateKey, modNumber);
		outBufPtr[i] = (unsigned char)plain; 

//		if (i%2) sched_yield();
	}
	
	return (srcSize/4);

}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: getModulo
* INPUT		: 
* OUTPUT	: 
* RETURN	: modulo
* COMMENTS	: 
*
*END*--------------------------------------------------------------*/
static unsigned long int getModulo(unsigned long int nSub, unsigned long int nExp, unsigned long int nMod)
{
	long int i; 
	unsigned long int d = 1;

	/* the beginning value of i is the max bit index of integer type  
	   except the sign bit (leftmost bit) */
	for (i=sizeof(int)*8-1; i>=0; i--) {
		d = (d*d) % nMod;
		if (nExp & (1<<i)) d = (d*nSub) % nMod;
	}
	
	return d;
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: getGDC
* INPUT		: 
* OUTPUT	: 
* RETURN	: gdc
* COMMENTS	: 최대공약수
*
*END*--------------------------------------------------------------*/
static unsigned long int getGDC(unsigned long int a, unsigned long int b)
{
	if (b==0) return a;
	else return getGDC(b, a % b);
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: getPrime
* INPUT		: maxLimit - limit of prime number
* OUTPUT	: 
* RETURN	: prime number under maxLimit / 0 if failed
* COMMENTS	: maxLimit보다 작은 수 중에서 Prime Number를 찾는다.
*
*END*--------------------------------------------------------------*/
static unsigned short int getPrime (unsigned short int maxLimit) 
{
	unsigned short int prime;
	unsigned short int cnt = 0x0FFF;

	while (--cnt) {
		srand(CheckTimer());
		prime = primeNumbers[rand()%NUMBER_PRIME];
		if (prime <= maxLimit) return prime;
	}

	return 0;	/* cannot find prime number */
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: encrypt
* INPUT		: 
* OUTPUT	: 
* RETURN	: 
* COMMENTS	: 
*
*END*--------------------------------------------------------------*/
static unsigned long int encrypt(unsigned long int nNum, unsigned long int nKeyE, unsigned long int nModN)
{
	return (getModulo(nNum, nKeyE, nModN));
}

/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: decrypt
* INPUT		: 
* OUTPUT	: 
* RETURN	: 
* COMMENTS	: 
*
*END*--------------------------------------------------------------*/
static unsigned long int decrypt(unsigned long int nNum, unsigned long int nKeyD, unsigned long int nModN)
{
	return (getModulo(nNum, nKeyD, nModN));
}


/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: swapBytes
* INPUT		: bufferPtr - pointer to buffer
*             size - size of data
* OUTPUT	: N/A
* RETURN	: N/A
* COMMENTS	: 
*
*END*--------------------------------------------------------------*/
static void swapBytes (unsigned char *bufferPtr, unsigned long int size) 
{
	unsigned long int i;
	unsigned char tempByte;
		
	for (i=1; i<size-1 ; i+=2) {
		tempByte = bufferPtr[i];
		bufferPtr[i] = bufferPtr[i+1];
		bufferPtr[i+1] = tempByte;
	}
}

/*FUNCTION*---------------------------------------------------------
*
* FUNCTION	: swapBits
* INPUT		: bufferPtr - pointer to buffer
*             size - size of data
* OUTPUT	: N/A
* RETURN	: N/A
* COMMENTS	: 1바이트를 4비트씩 swap.
*
*END*--------------------------------------------------------------*/
static void swapBits (unsigned char *bufferPtr, unsigned long int size) 
{
	unsigned long int i;
	unsigned char tempBits;
	
	for (i=0; i<size; i++) {
		tempBits = bufferPtr[i] & 0xF;
		bufferPtr[i] = bufferPtr[i] >> 4;
		bufferPtr[i] |= (tempBits << 4);
	}
}
