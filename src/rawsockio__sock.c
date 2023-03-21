#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <time.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <pthread.h>

#include <fstruct.h>
#include <fdefine.h>
#include <message.h>
#include <cmessage.h>
#include <dmessage.h>
#include <msghandler.h>
#include <rawsockio.h>
#include <fcommand.h>
#include <process.h>
//#include <networkcfg.h>
#include <osa.h>
//#include <setparam.h>

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif//max

extern SYSTEM_INFO SystemInfo ;

DUSERCONFIRMREQ Duserconfirmreq ;
IPCONFIRM *Ipconfirm ;
INTROPACKET intropacket ;

extern NETWORKCFG Networkcfg ;
extern IPSERVNETWORKCFG Ipservnetworkcfg ;
extern NETSETTINGCHANGE Netsettingchange ;
CONNECTON Connecton ;
extern SYSINFOCFG Sysinfocfg ;
extern SYSTEMREBOOT Systemreboot ;
//extern VSTREAMERROR Vstreamerror ;

char L_ReadBuffer[NORMALBUFFSIZE];
char C_ReadBuffer[NORMALBUFFSIZE];
char D_ReadBuffer[NORMALBUFFSIZE];
char m_TempBuf[NORMALBUFFSIZE] ;
unsigned char hostmacaddress[6] ;

extern int netaddr(int Filename) ;
//extern SETPARAM_Info gSETPARAM_info;

int InitVar(void) 
{
    int i = 0;    
    int m_Returnval = 0, bitrate = 0 ;   
   

    for (i = 0;i < MAXUSER;i ++)
    {
        SystemInfo.L_Channel[i] = 0;
        SystemInfo.C_Channel[i] = 0;
        SystemInfo.D_Channel[i] = 0;
        SystemInfo.Firstpacket[i] = 0;
        SystemInfo.videochannel[i] = 0 ;
        SystemInfo.headerrecv[i] = 0;
        SystemInfo.LQposition[i]= 0;   
        SystemInfo.CQposition[i]= 0;   
        SystemInfo.DQposition[i]= 0;   
        SystemInfo.StreamEnable[i] = 0 ;
   
        memset(SystemInfo.LQBUFF[i], 0, NORMALBUFFSIZE) ;
        memset(SystemInfo.CQBUFF[i], 0, NORMALBUFFSIZE) ;
        memset(SystemInfo.DQBUFF[i], 0, NORMALBUFFSIZE) ;
    }


        memset(L_ReadBuffer, 0, NORMALBUFFSIZE) ;
        memset(C_ReadBuffer, 0, NORMALBUFFSIZE) ;
        memset(D_ReadBuffer, 0, NORMALBUFFSIZE) ;

        SystemInfo.alternative = 0 ;
        SystemInfo.input = 0 ;
        SystemInfo.tvtype = 0 ; //air, catv
        SystemInfo.tvorcompchannel = 0 ;
        SystemInfo.datachannel = 0 ;
        SystemInfo.serial = 0 ;

        Netsettingchange.setting = 0 ;    
 
        Connecton.connect = 0 ;
        return m_Returnval ;
}

int NewDescriptor(int m_RootOneSock, int sockflag ) 
{
    SOCKETTYPE m_NewDesc;
    int i = 0, j = 0, k =0, m_Returnvalue = 0 ; 
    size_t m_SizeSocketPeer = 0;
    struct sockaddr_in m_SocketPeer;

    m_SizeSocketPeer = sizeof(m_SocketPeer);      
 
    if ((m_NewDesc  = accept(m_RootOneSock, (struct sockaddr *)&m_SocketPeer, &m_SizeSocketPeer)) == INVALID_SOCKET)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("\nNew Client Connecting Fail\n");
#endif
        return ERRNO;
    }
        
//    ChangeNonblock(m_NewDesc);

    switch(sockflag)
    {
        case LSOCKFLAG :
             for(i = 1; i < MAXUSER;i ++)
             {
                 if(SystemInfo.L_Channel[i] == 0) break;
             }
     
             if(i >= (MAXUSER-1))
             {
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("i >= MAXUSER -1\n") ;
#endif
                 close(m_NewDesc);
                 m_Returnvalue = FALSE ;
                 break ;
             }

             SystemInfo.L_Channel[i] = m_NewDesc;
             m_Returnvalue = i ;

             break ;

        case CSOCKFLAG :
             for(j = 1; j < MAXUSER;j ++)
             {
                 if(SystemInfo.C_Channel[j] == 0) break;
             }
     
             if(j >= (MAXUSER-1))
             {
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("j >= MAXUSER -1\n") ;
#endif
                 close(m_NewDesc);
                 m_Returnvalue = FALSE ;
                 break ;
             }

             SystemInfo.C_Channel[j] = m_NewDesc;
             m_Returnvalue = j ;

             break ;

        case DSOCKFLAG :
             for(k = 1; k < MAXUSER;k ++)
             {
                 if(SystemInfo.D_Channel[k] == 0) break;
             }
     
             if(k >= (MAXUSER-1))
             {
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("k >= MAXUSER -1\n") ;
#endif
                 close(m_NewDesc);
                 m_Returnvalue = FALSE ;
                 break ;
             }

             SystemInfo.D_Channel[k] = m_NewDesc ;
             m_Returnvalue = k ;

             break ;

        default :

             break ;
    }

    return m_Returnvalue;
}

int MainSocketListen( int sockflag )
{
    int reuse = 0, m_ListenSock = 0, SERVER_PORT = 0/*, opt_val = 1*/ ; 

    int recvBufferSize = 0 ;
    int sendBufferSize = 0, retvalue = 0 ;
    struct linger stLinger ;
    struct timeval tv ;

    struct sockaddr_in m_SocketAddress;

    tv.tv_sec = 5;
    tv.tv_usec = 0 ;
	//printf("Entering MainSocketListen: sockflag=%d\n",sockflag);
    stLinger.l_onoff = 1 ;
    stLinger.l_linger = 0 ;
    retvalue = netaddr(NETWORKMODIFYCFG) ;
   // Networkcfg.gateway.s_addr="192.168.0.1";
	//Networkcfg.subnetmask.s_addr="255.255.255.0";
	//Networkcfg.ipaddress.s_addr="192.168.0.200";
	//Networkcfg.port=7620;
	//Networkcfg.macaddress="fc:fc:fc:fc:fc:fc";
    if ((m_ListenSock = socket(PF_INET, SOCK_STREAM,0)) < 0)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("\ndon't create socket.\n");
#endif
        exit(1);
    }

    reuse = REUSEOK ;

    switch(sockflag)
    {
	case LSOCKFLAG :
		//printf("IP=%s,port=%d!!!!!!!!!!\n",Networkcfg.ipaddress.s_addr,Networkcfg.port);
             SERVER_PORT = Networkcfg.port ;
             recvBufferSize = NORMALBUFFSIZE ;
             sendBufferSize = NORMALBUFFSIZE ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize)) ;
             break ;

	case CSOCKFLAG :
		//printf("IP=%s,port=%d!!!!!!!!!!\n",Networkcfg.ipaddress.s_addr,Networkcfg.port+1);
             SERVER_PORT = Networkcfg.port + 1 ;
             recvBufferSize = NORMALBUFFSIZE ;
             sendBufferSize = NORMALBUFFSIZE*15 ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_LINGER, &stLinger, sizeof (stLinger)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize)) ;
             break ;

	case DSOCKFLAG :
		//printf("port=%d,IP=%s!!!!!!!!!!\n",Networkcfg.port,Networkcfg.ipaddress.s_addr+2);
             SERVER_PORT = Networkcfg.port + 2 ;
             recvBufferSize = NORMALBUFFSIZE;
             sendBufferSize = MAXBUFFSIZE ;

//             setsockopt (m_ListenSock, IPPROTO_TCP, TCP_NODELAY, &opt_val, sizeof(opt_val)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) ;
    	     setsockopt (m_ListenSock, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) ;
             setsockopt (m_ListenSock, SOL_SOCKET, SO_LINGER, &stLinger, sizeof (stLinger)) ;
             
             break ;
       
        default :
             break ;
    }

    m_SocketAddress.sin_family = AF_INET;
    m_SocketAddress.sin_port = htons(SERVER_PORT);
    m_SocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(m_SocketAddress.sin_zero), 8); 
             
    if (bind(m_ListenSock,(struct sockaddr *) &m_SocketAddress, sizeof(m_SocketAddress)) < 0)
    {    
#ifdef NETWORK_DEBUG
       	DEBUG_PRI("\nBind Error! incorrect IP.\n\n");
#endif
       	close(m_ListenSock);
       	exit(1);
    }
    
//    if(sockflag != DSOCKFLAG)
//    { 
//        ChangeNonblock(m_ListenSock);
//    }
    if (listen(m_ListenSock,5) < 0)
    	{
    		printf("listen erro:%d!!!\n",sockflag);
		return -1;
    	}
    else
    		return m_ListenSock ; 
}


void ProcessSocket(SOCKETTYPE m_ListenSock, int sockflag)
{
    fd_set m_read_set,m_NULLSET;
    struct timeval m_NULLTIME;
    int i,m_NewDes,max_fd=0;

    int m_RetrunFailed;

    m_NewDes = 0;

    FD_ZERO(&m_read_set);
    FD_SET(m_ListenSock, &m_read_set);

	switch(sockflag)
	{
		case LSOCKFLAG :	
			for (i=1;i < MAXUSER;i++)
	    		{ 
	  			if(SystemInfo.L_Channel[i] > 0)
	        	 	{
	                     	FD_SET(SystemInfo.L_Channel[i],&m_read_set);
					max_fd = max(max_fd, SystemInfo.L_Channel[i]);
	        	 	}
	    		}
			max_fd = max(max_fd , m_ListenSock);
	    		if (select(max_fd+1/*FD_SETSIZE*/, &m_read_set, NULL,NULL/*&m_write_set, &m_spc_set*/, &m_NULLTIME) < 0)
	    		{
	          	return ;
	    		}

	     		if (FD_ISSET(m_ListenSock, &m_read_set))
	     		{
	      			m_NewDes = NewDescriptor(m_ListenSock, LSOCKFLAG);
#ifdef NETWORK_DEBUG
	                 DEBUG_PRI("Logon NewDescriptor = %d,m_ListenSock=%d\n", m_NewDes,m_ListenSock) ;
#endif
	      		}
		 
			for (i = 1;i < MAXUSER;i++)
	      		{
	       		if((SystemInfo.L_Channel[i]>0) && (m_NewDes != i))
	             	{
	              		if (FD_ISSET(SystemInfo.L_Channel[i],&m_read_set))
	                    	{
	                     		if ((m_RetrunFailed = ReadSocketData(i, LSOCKFLAG)) < FALSE)
	                         		{
	                             		CloseNowChannel(i, LSOCKFLAG);
	                         		}
	                     	}
	       		}
	    		}
			
			break;

		case CSOCKFLAG :
			for (i=1;i < MAXUSER;i++)
		    	{ 
		  		if(SystemInfo.C_Channel[i] > 0)
		        	{
		             	FD_SET(SystemInfo.C_Channel[i],&m_read_set);
					max_fd = max(max_fd, SystemInfo.L_Channel[i]);
		        	}
		    	}
			max_fd = max(max_fd , m_ListenSock);
		    	if (select(FD_SETSIZE, &m_read_set, NULL,NULL/*&m_write_set, &m_spc_set*/, &m_NULLTIME) < 0)
		    	{
		          	return ;
		    	}

		     	if (FD_ISSET(m_ListenSock, &m_read_set))
		     	{
		      		m_NewDes = NewDescriptor(m_ListenSock, CSOCKFLAG);
#ifdef NETWORK_DEBUG
		            DEBUG_PRI("Control NewDescriptor = %d,m_ListenSock=%d\n", m_NewDes,m_ListenSock) ;
#endif
		     	}
			 
			for (i = 1;i < MAXUSER;i++)
		      {
		       	if((SystemInfo.C_Channel[i]>0) && (m_NewDes != i))
		             {
		              	if (FD_ISSET(SystemInfo.C_Channel[i],&m_read_set))
		                   {
		                    	if ((m_RetrunFailed = ReadSocketData(i,CSOCKFLAG)) < FALSE)
		                         {
		                             CloseNowChannel(i, CSOCKFLAG);
		                         }
		                   }
		       	}
		    	}

			break;

		case DSOCKFLAG :
			for (i=1;i < MAXUSER;i++)
		    	{ 
		  		if(SystemInfo.D_Channel[i] > 0)
		        	 {
		                     FD_SET(SystemInfo.D_Channel[i],&m_read_set);
					max_fd = max(max_fd, SystemInfo.L_Channel[i]);
		        	 }
		    	}
			max_fd = max(max_fd , m_ListenSock);
		   	if (select(FD_SETSIZE, &m_read_set, NULL,NULL/*&m_write_set, &m_spc_set*/, &m_NULLTIME) < 0)
		    	{
		          return ;
		    	}

		     	if (FD_ISSET(m_ListenSock, &m_read_set))
		     	{
		      		m_NewDes = NewDescriptor(m_ListenSock, DSOCKFLAG);
#ifdef NETWORK_DEBUG
		             DEBUG_PRI("Data NewDescriptor = %d,m_ListenSock=%d\n", m_NewDes,m_ListenSock) ;
#endif
		      	}
			 
			for (i = 1;i < MAXUSER;i++)
		      {
		       	if((SystemInfo.D_Channel[i]>0) && (m_NewDes != i))
		             {
		              	if (FD_ISSET(SystemInfo.D_Channel[i],&m_read_set))
		                    {
		                     	if ((m_RetrunFailed = ReadSocketData(i, DSOCKFLAG)) < FALSE)
		                         {
		                             CloseNowChannel(i, DSOCKFLAG);
		                         }

		                     }
		       	}
		    	}

			break;

		default:
			break;

		}
}

#if 0
void ChangeNonblock(SOCKETTYPE m_MainSocketNo)
{
    int flags;
    
    flags = fcntl(m_MainSocketNo,F_GETFL,0);
    flags |= O_NONBLOCK;

    if(fcntl(m_MainSocketNo,F_SETFL,flags) < 0)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("\nNoBlocking Error\n");
#endif
        exit(1);
    }   
}
#endif
void CloseAllChannel(void) 
{
    int i = 0, j = 0 ;

    for(i = 1; i < MAXUSER ; i++)
    {
        if(SystemInfo.C_Channel[i] != 0)
        {
            close(SystemInfo.C_Channel[i]) ;
            SystemInfo.C_Channel[i] = 0 ;
            SystemInfo.CQposition[i] = 0 ;
            memset(SystemInfo.CQBUFF[i],0,NORMALBUFFSIZE) ;
        }
        if(SystemInfo.D_Channel[i] != 0)
        {
            close(SystemInfo.D_Channel[i]) ;
            SystemInfo.D_Channel[i] = 0 ;
            SystemInfo.DQposition[i] = 0 ;
            j = SystemInfo.videochannel[i] ;
            SystemInfo.videochannel[i] = 0 ;
            SystemInfo.Firstpacket[i] = 0 ;
            SystemInfo.headerrecv[i] = 0 ;
            SystemInfo.encodingtype[i] = 0 ;
            Connecton.connect = 2 ;  // for LED
            memset(SystemInfo.DQBUFF[i],0,NORMALBUFFSIZE) ;
        }
    }
}

void CloseNowChannel(int m_NowChno, int sockflag)
{
    int alternative = 0, i = 0  ;

    switch(sockflag)
    {
	case LSOCKFLAG :
             if(SystemInfo.L_Channel[m_NowChno] != 0)
    	     {
        	 close (SystemInfo.L_Channel[m_NowChno]);
        	 SystemInfo.L_Channel[m_NowChno] = 0 ;
        	 SystemInfo.LQposition[m_NowChno] = 0 ;
        	 memset(SystemInfo.LQBUFF[m_NowChno],0,NORMALBUFFSIZE) ;
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("CloseNowChannel...Longon socket.....\n") ;
#endif
    	     } 

             break ;  

	case CSOCKFLAG :
             if(SystemInfo.C_Channel[m_NowChno] != 0)
    	     {
        	 close (SystemInfo.C_Channel[m_NowChno]);
        	 SystemInfo.C_Channel[m_NowChno] = 0 ;
        	 SystemInfo.CQposition[m_NowChno] = 0 ;
        	 memset(SystemInfo.CQBUFF[m_NowChno],0,NORMALBUFFSIZE) ;
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("CloseNowChannel...Control socket.....\n") ;
#endif

         
                 if(SystemInfo.D_Channel[m_NowChno] != 0)
                 {
                     close (SystemInfo.D_Channel[m_NowChno]);
                 }

                 for(i = 1; i < MAXUSER; i++)
                 {
                     if(SystemInfo.C_Channel[i] != 0)
                     {
                         alternative = 1 ;
                     }
                 }
                 if(alternative == 0)
                 {
                     //gSETPARAM_info.remoteConnectOn = FALSE ;
                 }


                 SystemInfo.D_Channel[m_NowChno] = 0 ;
                 SystemInfo.DQposition[m_NowChno] = 0 ;
                 SystemInfo.Firstpacket[m_NowChno] = 0 ;
                 SystemInfo.headerrecv[m_NowChno] = 0 ;
                 SystemInfo.encodingtype[m_NowChno] = 0 ; 
//                 VSTREAM_disableCh(SystemInfo.videochannel[m_NowChno] - 1) ;
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("CloseNowChannel...Data socket.....\n") ;
#endif

    	     } 

             break ;  

	case DSOCKFLAG :
             if(SystemInfo.D_Channel[m_NowChno] != 0)
    	     {
        	 close (SystemInfo.D_Channel[m_NowChno]);
        	 SystemInfo.D_Channel[m_NowChno] = 0 ;
        	 SystemInfo.DQposition[m_NowChno] = 0 ;
                 SystemInfo.Firstpacket[m_NowChno] = 0 ;
                 SystemInfo.headerrecv[m_NowChno] = 0 ;
                 SystemInfo.encodingtype[m_NowChno] = 0 ;
                 Connecton.connect = 2 ;
                 for(i = 1; i < MAXUSER; i++)
                 {
                     if(SystemInfo.D_Channel[i] != 0)
                     {
                         alternative = 1 ;
                     }
                 }
                 if(alternative == 0)
                 {
                     //gSETPARAM_info.remoteConnectOn = FALSE ;
                 }

//                 VSTREAM_disableCh(SystemInfo.videochannel[m_NowChno] - 1) ;
                 SystemInfo.videochannel[m_NowChno] = 0 ;
                 SystemInfo.alternative = alternative ;
        	 memset(SystemInfo.DQBUFF[m_NowChno],0,NORMALBUFFSIZE) ;
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("CloseNowChannel...Data socket.....\n") ;
#endif
    	     } 

             break ;  
        
        default :
          
             break ;
    }
}

int ReadSocketData(int m_NowReadChannel, int sockflag) 
{
    int m_ReadBuffSize = 0, m_Returnvalue = 0 ;

//#ifdef NETWORK_DEBUG
 //   DEBUG_PRI("ReadSocketData sockflag = %d\n",sockflag) ;
//#endif
    switch(sockflag)
    {
        case LSOCKFLAG :
             m_ReadBuffSize = recv(SystemInfo.L_Channel[m_NowReadChannel], L_ReadBuffer,NORMALBUFFSIZE,0);
#ifdef NETWORK_DEBUG
             DEBUG_PRI("logon socket recv......size = %d,read data=0x%0x 0x%0x\n",m_ReadBuffSize,*(int*)L_ReadBuffer,*(int*)(L_ReadBuffer+4)) ;
#endif
             break ;
        
        case CSOCKFLAG :
             m_ReadBuffSize = recv(SystemInfo.C_Channel[m_NowReadChannel], C_ReadBuffer,NORMALBUFFSIZE,0);
#ifdef NETWORK_DEBUG
             DEBUG_PRI("control socket recv......size = %d\n",m_ReadBuffSize) ;
#endif
             break ;


        case DSOCKFLAG :
             m_ReadBuffSize = recv(SystemInfo.D_Channel[m_NowReadChannel], D_ReadBuffer,NORMALBUFFSIZE,0);
#ifdef NETWORK_DEBUG
             DEBUG_PRI("data socket recv......size = %d\n",m_ReadBuffSize) ;
#endif
             break ;
 
        default :
             break ;
    }

    if(m_ReadBuffSize < FALSE)
    {
	switch(errno)
        {
	    case EWOULDBLOCK : 
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("ReadSocketData EWOULDBLOCK..\n") ;
#endif
                 m_Returnvalue = FALSE ; // FALSE == 0
                 break ;

            case EPIPE :
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("ReadSocketData EPIPE..\n") ;
#endif
                 m_Returnvalue = ERRNO ; // ERRNO == -1
                 break ;

            case ENOTSOCK :
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("ReadSocketData ENOTSOCK\n") ;
#endif
                 m_Returnvalue = ERRNO ; 
                 break ;

            case ECONNRESET :
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("ReadSocketData ECONNRESET\n") ;
#endif
                 m_Returnvalue = ERRNO ; 
                 break ;

            default :
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("ReadSocketData errno = %d\n",errno) ;
#endif
                 m_Returnvalue = ERRNO ; 
                 break ;
        }
        return m_Returnvalue ;
        
    }
    else
    {
        if(m_ReadBuffSize == FALSE) 
        {
            m_Returnvalue = ERRNO ;
        }
        if(m_ReadBuffSize > FALSE)
        {
            m_Returnvalue = ReadSocketQue(m_NowReadChannel, m_ReadBuffSize, sockflag) ;
            if(m_Returnvalue < FALSE)
            {
                m_Returnvalue = ERRNO ;
            }
            else
            {
                if(sockflag == LSOCKFLAG)
                {
                    memset(L_ReadBuffer, 0, NORMALBUFFSIZE) ;
                    m_Returnvalue = FALSE ;
                }
                if(sockflag == CSOCKFLAG)
                {
                    memset(C_ReadBuffer, 0, NORMALBUFFSIZE) ;
                    m_Returnvalue = FALSE ;
                }
                if(sockflag == DSOCKFLAG)
                {    
                    memset(D_ReadBuffer, 0, NORMALBUFFSIZE) ;
                    m_Returnvalue = FALSE ;
                } 
            }
        }
    }
    return m_Returnvalue;//m_ReadBuffSize;//
}

int ReadSocketQue(int m_NowChannel, int m_BuffSize, int sockflag) 
{
    int m_Returnvalue = 0 ;

    if(m_BuffSize < FALSE) 
    {
        m_Returnvalue = ERRNO ; 
    }
    if(m_BuffSize == FALSE) 
    {
        m_Returnvalue = FALSE ;
    }
    else
    {
        switch(sockflag)
        {
	    case LSOCKFLAG :
		//printf("LSOCKFLAG receive: LQposition=%d,m_BuffSize=%d\n",SystemInfo.LQposition[m_NowChannel],m_BuffSize);
    	         if(SystemInfo.LQposition[m_NowChannel] >= NORMALBUFFSIZE)
                 {
                     SystemInfo.LQposition[m_NowChannel] = 0;
                     memset(SystemInfo.LQBUFF[m_NowChannel], 0, NORMALBUFFSIZE);
                     m_Returnvalue = FALSE ;
                 }
                 if(SystemInfo.LQposition[m_NowChannel] + m_BuffSize >= NORMALBUFFSIZE)
                 {
                     SystemInfo.LQposition[m_NowChannel] = 0;
                     memset(SystemInfo.LQBUFF[m_NowChannel], 0, NORMALBUFFSIZE );
                     m_Returnvalue = FALSE ;
                 }
                 else
                 {
                     memcpy(SystemInfo.LQBUFF[m_NowChannel] + SystemInfo.LQposition[m_NowChannel],L_ReadBuffer,m_BuffSize);
                     SystemInfo.LQposition[m_NowChannel] += m_BuffSize;

                     if((m_BuffSize < 2) || (SystemInfo.LQposition[m_NowChannel] < 2)) return 0;

                     m_Returnvalue = GetQue(m_NowChannel, sockflag);
                 } 
                 break ;

	    case CSOCKFLAG :
			//printf("CSOCKFLAG receive: CQposition=%d,m_BuffSize=%d\n",SystemInfo.CQposition[m_NowChannel],m_BuffSize);
    	         if(SystemInfo.CQposition[m_NowChannel] >= NORMALBUFFSIZE)
                 {
                     SystemInfo.CQposition[m_NowChannel] = 0;
                     memset(SystemInfo.CQBUFF[m_NowChannel], 0, NORMALBUFFSIZE);
                     m_Returnvalue = FALSE ;
                 }
                 if(SystemInfo.CQposition[m_NowChannel] + m_BuffSize >= NORMALBUFFSIZE)
                 {
                     SystemInfo.CQposition[m_NowChannel] = 0;
                     memset(SystemInfo.CQBUFF[m_NowChannel], 0, NORMALBUFFSIZE );
                     m_Returnvalue = FALSE ;
                 }
                 else
                 {
                     memcpy(SystemInfo.CQBUFF[m_NowChannel] + SystemInfo.CQposition[m_NowChannel],C_ReadBuffer,m_BuffSize);
                     SystemInfo.CQposition[m_NowChannel] += m_BuffSize;

                     if((m_BuffSize < 2) || (SystemInfo.CQposition[m_NowChannel] < 2)) return 0;

                     m_Returnvalue = GetQue(m_NowChannel, sockflag);
                 } 
                 break ;

	    case DSOCKFLAG :
			//printf("DSOCKFLAG receive: DQposition=%d,m_BuffSize=%d\n",SystemInfo.DQposition[m_NowChannel],m_BuffSize);
    	         if(SystemInfo.DQposition[m_NowChannel] >= NORMALBUFFSIZE)
                 {
                     SystemInfo.DQposition[m_NowChannel] = 0;
                     memset(SystemInfo.DQBUFF[m_NowChannel], 0, NORMALBUFFSIZE);
                     m_Returnvalue = FALSE ;
                 }
                 if(SystemInfo.DQposition[m_NowChannel] + m_BuffSize >= NORMALBUFFSIZE)
                 {
                     SystemInfo.DQposition[m_NowChannel] = 0;
                     memset(SystemInfo.DQBUFF[m_NowChannel], 0, NORMALBUFFSIZE );
                     m_Returnvalue = FALSE ;
                 }
                 else
                 {
                     memcpy(SystemInfo.DQBUFF[m_NowChannel] + SystemInfo.DQposition[m_NowChannel],D_ReadBuffer,m_BuffSize);
                     SystemInfo.DQposition[m_NowChannel] += m_BuffSize;

                     if((m_BuffSize < 2) || (SystemInfo.DQposition[m_NowChannel] < 2)) return 0;

                     m_Returnvalue = GetQue(m_NowChannel, sockflag);
                 } 
                 break ;

            default :
                 break ;
        }
    }

        return m_Returnvalue ;
}

int GetQue(int m_NowChno, int sockflag)
{
    int m_PhaseLen = 0, m_PhasePos = 0, phasecmd = 0, m_Returnvalue = 0;

    switch(sockflag)
    {
        case LSOCKFLAG :
		do
		{
             if (SystemInfo.L_Channel[m_NowChno] <= 0) 
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
             m_PhaseLen = ntohs(((SystemInfo.LQBUFF[m_NowChno][m_PhasePos+5] << 8) & 0xff00)
                        | ((SystemInfo.LQBUFF[m_NowChno][m_PhasePos + 4]) & 0xff));

             phasecmd = ntohs(((SystemInfo.LQBUFF[m_NowChno][m_PhasePos+3] << 8) & 0xff00)
                        | ((SystemInfo.LQBUFF[m_NowChno][m_PhasePos + 2]) & 0xff));
		//printf("GetQue: LSOCKFLAG--CMD=0x%x,m_PhaseLen=%d,LQBUFF=0x%x,LQBUFF+2=0x%x\n",phasecmd,m_PhaseLen,SystemInfo.LQBUFF[m_NowChno],SystemInfo.LQBUFF[m_NowChno]+2);

             if((m_PhaseLen <= 0) || (m_PhaseLen > NORMALBUFFSIZE -10))
             {
                 SystemInfo.LQposition[m_NowChno] = 0;
                 memset(SystemInfo.LQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("m_PhaseLen == 0 \n") ;
#endif
                 m_Returnvalue = FALSE ;
                 break ;
             }

             if(m_PhaseLen > SystemInfo.LQposition[m_NowChno])
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
    
             set_message (gMessageManager, m_NowChno, SystemInfo.LQBUFF[m_NowChno]+2, m_PhaseLen - 2) ;

             memset(SystemInfo.LQBUFF[m_NowChno],0,m_PhaseLen );
             m_PhasePos = m_PhaseLen ;
    
             SystemInfo.LQposition[m_NowChno] = SystemInfo.LQposition[m_NowChno] - m_PhasePos;
   
             if(m_PhasePos + SystemInfo.LQposition[m_NowChno] >= NORMALBUFFSIZE )
             {
                 SystemInfo.LQposition[m_NowChno] = 0;
                 memset(SystemInfo.LQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }

             if(SystemInfo.LQposition[m_NowChno] <= FALSE)
             {
                 SystemInfo.LQposition[m_NowChno] = 0;
                 memset(SystemInfo.LQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }
             else
             {
                 memcpy(m_TempBuf, SystemInfo.LQBUFF[m_NowChno] + m_PhasePos, SystemInfo.LQposition[m_NowChno]);
                 memcpy(SystemInfo.LQBUFF[m_NowChno],m_TempBuf, SystemInfo.LQposition[m_NowChno]);
             }
		//m_Returnvalue = m_PhaseLen;
             m_PhasePos = 0;
		}while(SystemInfo.LQposition[m_NowChno]);
             break ;
        
        case CSOCKFLAG :
		do
		{
             if (SystemInfo.C_Channel[m_NowChno] <= 0) 
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
             m_PhaseLen = ntohs(((SystemInfo.CQBUFF[m_NowChno][m_PhasePos+5] << 8) & 0xff00)
                        | ((SystemInfo.CQBUFF[m_NowChno][m_PhasePos + 4]) & 0xff));

             phasecmd = ntohs(((SystemInfo.CQBUFF[m_NowChno][m_PhasePos+3] << 8) & 0xff00)
                        | ((SystemInfo.CQBUFF[m_NowChno][m_PhasePos + 2]) & 0xff));
		//printf("GetQue: CSOCKFLAG--CMD=0x%x,m_PhaseLen=%d,CQBUFF=0x%x,CQBUFF+2=0x%x\n",phasecmd,m_PhaseLen,SystemInfo.CQBUFF[m_NowChno],SystemInfo.CQBUFF[m_NowChno]+2);

/*             
             if(phasecmd != CMD_USERCONFIRM_REQ && phasecmd != CMD_SYSTEMPARAM_REQ)
             {
                 memset(SystemInfo.CQBUFF[m_NowChno], 0 ,NORMALBUFFSIZE) ;
                 m_Returnvalue = FALSE ;
                 break ;
             }
*/
             if((m_PhaseLen <= 0) || (m_PhaseLen > NORMALBUFFSIZE -10))
             {
                 SystemInfo.CQposition[m_NowChno] = 0;
                 memset(SystemInfo.CQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("m_PhaseLen == 0 \n") ;
#endif
                 m_Returnvalue = FALSE ;
                 break ;
             }
             if(phasecmd == 24609)
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }

             if(m_PhaseLen > SystemInfo.CQposition[m_NowChno])
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
 
             Cset_message (CgMessageManager, m_NowChno, SystemInfo.CQBUFF[m_NowChno]+2, m_PhaseLen - 2) ;

             memset(SystemInfo.CQBUFF[m_NowChno],0,m_PhaseLen );
             m_PhasePos = m_PhaseLen ;
    
             SystemInfo.CQposition[m_NowChno] = SystemInfo.CQposition[m_NowChno] - m_PhasePos;
   
             if(m_PhasePos + SystemInfo.CQposition[m_NowChno] >= NORMALBUFFSIZE )
             {
                 SystemInfo.CQposition[m_NowChno] = 0;
                 memset(SystemInfo.CQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }

             if(SystemInfo.CQposition[m_NowChno] <= FALSE)
             {
                 SystemInfo.CQposition[m_NowChno] = 0;
                 memset(SystemInfo.CQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }
             else
             {
                 memcpy(m_TempBuf, SystemInfo.CQBUFF[m_NowChno] + m_PhasePos, SystemInfo.CQposition[m_NowChno]);
                 memcpy(SystemInfo.CQBUFF[m_NowChno],m_TempBuf, SystemInfo.CQposition[m_NowChno]);
             }
		//m_Returnvalue = m_PhaseLen;
             m_PhasePos = 0;
		}while(SystemInfo.CQposition[m_NowChno]);
             break ;

        case DSOCKFLAG :
		do
		{
             if (SystemInfo.D_Channel[m_NowChno] <= 0)
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
             m_PhaseLen = ntohs(((SystemInfo.DQBUFF[m_NowChno][m_PhasePos+5] << 8) & 0xff00)
                        | ((SystemInfo.DQBUFF[m_NowChno][m_PhasePos + 4]) & 0xff));

             phasecmd = ntohs(((SystemInfo.DQBUFF[m_NowChno][m_PhasePos+3] << 8) & 0xff00)
                        | ((SystemInfo.DQBUFF[m_NowChno][m_PhasePos + 2]) & 0xff));
		//printf("GetQue: DSOCKFLAG--CMD=0x%x,m_PhaseLen=%d,DQBUFF=0x%x,DQBUFF+2=0x%x\n",phasecmd,m_PhaseLen,SystemInfo.DQBUFF[m_NowChno],SystemInfo.DQBUFF[m_NowChno]+2);
             if((m_PhaseLen <= 0) || (m_PhaseLen > NORMALBUFFSIZE -10))
             {
                 SystemInfo.DQposition[m_NowChno] = 0;
                 memset(SystemInfo.DQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
#ifdef NETWORK_DEBUG
                 DEBUG_PRI("m_PhaseLen == 0 \n") ;
#endif
                 m_Returnvalue = FALSE ;
                 break ;
             }
/*
             if(phasecmd == 16401)
             {
                 m_Returnvalue = FALSE ;
                 break ;
             }
*/
             if(m_PhaseLen > SystemInfo.DQposition[m_NowChno])
             {
                 m_Returnvalue = FALSE ;
		//printf("DSOCKFLAG: m_PhaseLen = %d,DQposition=%d\n",m_PhaseLen,SystemInfo.DQposition[m_NowChno]);
                 break ;
             }
  
             if(phasecmd == CMD_USERCONFIRM_REQ)
             {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("receive CMD_USERCONFIRM_REQ in DSOCKFLAG!!!\n") ;
#endif
		    Duserconfirmreq.cmd = htons(CMD_DUSERCONFIRM_REQ) ;
                 memcpy(SystemInfo.DQBUFF[m_NowChno] + 2, &Duserconfirmreq, 2) ;
                 Dset_message (DgMessageManager, m_NowChno, SystemInfo.DQBUFF[m_NowChno]+2, m_PhaseLen - 2);
             }  
             else
             {
                 Dset_message (DgMessageManager, m_NowChno, SystemInfo.DQBUFF[m_NowChno]+2, m_PhaseLen - 2) ;
             }
             memset(SystemInfo.DQBUFF[m_NowChno],0,m_PhaseLen );
             m_PhasePos = m_PhaseLen ;
    
             SystemInfo.DQposition[m_NowChno] = SystemInfo.DQposition[m_NowChno] - m_PhasePos;
   
             if(m_PhasePos + SystemInfo.DQposition[m_NowChno] >= NORMALBUFFSIZE )
             {
                 SystemInfo.DQposition[m_NowChno] = 0;
                 memset(SystemInfo.DQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }

             if(SystemInfo.DQposition[m_NowChno] <= FALSE)
             {
                 SystemInfo.DQposition[m_NowChno] = 0;
                 memset(SystemInfo.DQBUFF[m_NowChno], 0, NORMALBUFFSIZE );
                 m_Returnvalue = FALSE ;
                 break ;
             }
             else
             {
                 memcpy(m_TempBuf, SystemInfo.DQBUFF[m_NowChno] + m_PhasePos, SystemInfo.DQposition[m_NowChno]);
                 memcpy(SystemInfo.DQBUFF[m_NowChno],m_TempBuf, SystemInfo.DQposition[m_NowChno]);
             }

             m_PhasePos = 0;
		//m_Returnvalue = m_PhaseLen;
		//printf("DSOCKFLAG: m_Returnvalue = %d\n",m_Returnvalue);
		}while(SystemInfo.DQposition[m_NowChno]);
             break ;

        default :
             break ;
    }
             return m_Returnvalue ;
}


int GetGlobalMacAddr(char *pstHwAddr)
{
    int sock, ret;
    struct ifreq ifr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return 0;
    }

    strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));

    // Get HW Adress 
    ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
    if (ret < 0) {
        close(sock);
        return 0;
    }

    memcpy(pstHwAddr, &ifr.ifr_hwaddr.sa_data, 6);
    close(sock);

    return  0;
}

int IpCamRead(void)
{
	int ipcam_socket,rev_len,reuse;
	struct sockaddr_in ipcam_sockaddr;
	char *test_msg= "Hello, This is DM8168 DVR!!!";
	char recv_msg[28];
	int len = strlen(test_msg);	
	reuse=REUSEOK;
	
	printf("Enter IpCamRead !!!\n");	
	if((ipcam_socket=socket(PF_INET, SOCK_STREAM, 0))<0)
	{
		printf("IPcam socket create failed!\n");
		return 0;
	}		
	setsockopt(ipcam_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) ;
	ipcam_sockaddr.sin_family = AF_INET;
	ipcam_sockaddr.sin_port = 0;
	ipcam_sockaddr.sin_addr.s_addr = inet_addr("192.168.0.201");
	bzero(&(ipcam_sockaddr.sin_zero),8);
	if(bind(ipcam_socket, (struct sockaddr*)&ipcam_sockaddr, sizeof(ipcam_sockaddr))<0)
	{
		printf("bind to own ip 192.168.0.201 failed!!!\n");
		close(ipcam_socket);
		return 0;
	}	
	
	//ipcam_sockaddr.sin_family = AF_INET;
	ipcam_sockaddr.sin_port = htons(2000);
	ipcam_sockaddr.sin_addr.s_addr = inet_addr("192.168.0.238");
	bzero(&(ipcam_sockaddr.sin_zero),8);
	if(connect(ipcam_socket, (struct sockaddr*)&ipcam_sockaddr, sizeof(ipcam_sockaddr))<0)
	{
		printf("connect to server 192.168.0.238 failed!\n");
		close(ipcam_socket);
		return 0;
	}

	while(1)
	{
		rev_len=recv(ipcam_socket, recv_msg,len,MSG_DONTWAIT);
		if(rev_len>0)
		{
			printf("Receive date from 192.168.0.238: %s\n",recv_msg);
			rev_len=send(ipcam_socket,test_msg,len,MSG_DONTWAIT);
			printf("Send data %d bytes!!!\n", rev_len);
		}
		//else
			//printf("Receive data failed,errno=%d\n",errno);
		sleep(5);
	}	
	return 1;
}

void broadreceive()
{
    FILE *fp ;
    int recv_sock = 0, recv_len = 0, cmd = 0, retvalue = 0, compareresult = 0 ;
    int broadsock = 0, so_broadcast = 1, status = 0, reuse = REUSEOK ;
    struct sockaddr_in addr, svraddr ;

    char HwAddr[6];

    char recvbuff[NORMALBUFFSIZE] ;
    char cmparebuff[20] ;

//    retvalue = netaddr(NETWORKMODIFYCFG) ;
				
    recv_sock = socket(PF_INET, SOCK_DGRAM, 0) ;
    if(recv_sock == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadcastrecv sock error\n") ;
#endif
    }

    memset(&addr, 0, sizeof(addr)) ;
    addr.sin_family = AF_INET ;
    addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    addr.sin_port = htons(20529) ;

    if(bind(recv_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
#ifdef NETWORK_DEBUG
//        DEBUG_PRI("broadcast recv bind error\n") ;
#endif
    }

    broadsock = socket(PF_INET, SOCK_DGRAM, 0) ;
    if(broadsock == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadsock open error\n") ;
#endif
    }
    memset(&svraddr, 0, sizeof(svraddr)) ;
    svraddr.sin_family = AF_INET ;
    svraddr.sin_addr.s_addr = inet_addr("255.255.255.255") ;
    svraddr.sin_port = htons(20528) ;

    status = setsockopt(broadsock, SOL_SOCKET, SO_BROADCAST, (void*)&so_broadcast, sizeof(so_broadcast)) ;
    status = setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) ;

    while(1)
    {
        if(Netsettingchange.setting == 4)
        {
            Netsettingchange.setting = 0 ;
            break ;
        }
        usleep(100) ;
        memset(recvbuff, 0, NORMALBUFFSIZE) ;
        cmd = 0 ;
        recv_len = recvfrom(recv_sock, recvbuff, NORMALBUFFSIZE, 0, NULL, 0) ;
        if(recv_len < 0)
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("recv errno = %d\n",errno) ;
#endif
        }

        Ipconfirm = (IPCONFIRM *)recvbuff ;
        cmd = ntohs(Ipconfirm->cmd) ;

        sprintf(cmparebuff, "%02x:%02x:%02x:%02x:%02x:%02x\n",Ipconfirm->macaddress[0],Ipconfirm->macaddress[1],
        Ipconfirm->macaddress[2], Ipconfirm->macaddress[3],Ipconfirm->macaddress[4], Ipconfirm->macaddress[5]) ;

        if(cmd == STATICINSERT) // ip setting
        {
   //         retvalue = netaddr(NETWORKMODIFYCFG) ;
#ifdef NETWORK_DEBUG
            DEBUG_PRI("compare = %s\n",cmparebuff) ;
            DEBUG_PRI("Networkcfg = %s\n",Networkcfg.macaddress) ;
#endif
            compareresult = strncasecmp(Networkcfg.macaddress, cmparebuff, 17) ;
            if(compareresult == 0)
            {
                if(( fp = fopen("/etc/network/cfg-network" , "w")) == NULL)
//                if(( fp = fopen("./cfg-network" , "w")) == NULL)
                {
#ifdef NETWORK_DEBUG
                    DEBUG_PRI("cfg-network file open error\n") ;
#endif
                }
                Networkcfg.dhcp = 0 ;
#ifdef NETWORK_DEBUG
                DEBUG_PRI("same macaddress\n") ;
                DEBUG_PRI("DHCP=%d\n",Networkcfg.dhcp) ;
                DEBUG_PRI("MAC=%s\n",Networkcfg.macaddress) ;
                DEBUG_PRI("IPADDR=%s\n",inet_ntoa(Ipconfirm->ipaddress)) ;
                DEBUG_PRI("NETMASK=%s\n",inet_ntoa(Ipconfirm->subnetmask)) ;
                DEBUG_PRI("GATEWAY=%s\n",inet_ntoa(Ipconfirm->gateway)) ;
                DEBUG_PRI("BROADCAST=%s\n",inet_ntoa(Networkcfg.broadcast)) ;
#endif

                fprintf(fp,"DHCP=%d\n",Networkcfg.dhcp) ;
                fprintf(fp,"MAC=\"%s\"\n",Networkcfg.macaddress) ;
                fprintf(fp,"IPADDR=\"%s\"\n",inet_ntoa(Ipconfirm->ipaddress)) ;
                fprintf(fp,"NETMASK=\"%s\"\n",inet_ntoa(Ipconfirm->subnetmask)) ;
                fprintf(fp,"GATEWAY=\"%s\"\n",inet_ntoa(Ipconfirm->gateway)) ;
                fprintf(fp,"BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
                fprintf(fp,"PORT=%d\n",Networkcfg.port) ;
                fprintf(fp,"NETWORK=\"\"\n") ;
                fclose(fp) ;

//                sleep(1) ;
//                Netsettingchange.setting = 1 ;
            }
            else
            {
#ifdef NETWORK_DEBUG
                DEBUG_PRI("different mac\n") ;
#endif
            }
        }
        else if(cmd == INTROREQUEST) // local cast packet
        {
        //    retvalue = netaddr(NETWORKMODIFYCFG) ;
            retvalue = netaddr(SYSINFOMODIFYCFG) ;

            GetGlobalMacAddr(HwAddr);

            intropacket.identifier = htons(0x2818) ;
            intropacket.cmd = htons(INTRORESULT) ;

            memcpy(intropacket.macaddress, HwAddr, 6) ;
            intropacket.reserved1 = htons(0x00) ;
            intropacket.ipaddress.s_addr = Networkcfg.ipaddress.s_addr ;
            intropacket.subnetmask.s_addr = Networkcfg.subnetmask.s_addr ;
            intropacket.gateway.s_addr = Networkcfg.gateway.s_addr ;
            intropacket.versionmajor = htons(VERSIONMAJOR) ;
            intropacket.versionminor = VERSIONMINOR ;
            intropacket.versiontext = VERSIONTEXT ;
#ifdef NETWORK_DEBUG
//            DEBUG_PRI("intropacket.. Version %d.%d%c\n",VERSIONMAJOR, VERSIONMINOR, VERSIONTEXT) ;
#endif
            intropacket.boardtype = htons(0x0000) ;
            intropacket.reserved2 = htons(0x0000) ;
            intropacket.Tlogonport = htons(Networkcfg.port) ;
            intropacket.Tcontrolport = htons(Networkcfg.port + 1) ;
            intropacket.Tdataport = htons(Networkcfg.port + 2) ;
            intropacket.Ulogonport = htons(Networkcfg.port) ;
            intropacket.Ucontrolport = htons(Networkcfg.port + 1) ;
            intropacket.Udataport = htons(Networkcfg.port + 2) ;
            strncpy(intropacket.servertitle,"DVR_RDK",10) ;
            strncpy(intropacket.serverlocation,"DVR_RDK",9) ;
            strncpy(intropacket.serverdescription, "DVR_RDK",32) ;
            strncpy(intropacket.cameratitle1,"DVR_RDK",32) ;
            strncpy(intropacket.cameratitle2,"DVR_RDK",32) ;
            strncpy(intropacket.cameratitle3,"DVR_RDK",32) ;
            strncpy(intropacket.cameratitle4,"DVR_RDK",32) ;

            sendto(broadsock, &intropacket, sizeof(INTROPACKET), 0, (struct sockaddr*)&svraddr, sizeof(svraddr)) ;

        }
    }
    close(recv_sock) ;
    pthread_exit(NULL) ;
}



#if 0
void broadreceive()
{
    FILE *fp ;
    int recv_sock = 0, recv_len = 0, cmd = 0, retvalue = 0, compareresult = 0 ;
    int broadsock = 0, so_broadcast = 1, status = 0 ;
    TIMECHECK *Timecheck ;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0 ;

    struct tm tm_ptr ;
    time_t m_time ;

    struct sockaddr_in addr, svraddr ;


    char recvbuff[NORMALBUFFSIZE] ;
    char cmparebuff[20] ;

    recv_sock = socket(PF_INET, SOCK_DGRAM, 0) ;
    if(recv_sock == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadcastrecv sock error\n") ;
#endif
    }

    memset(&addr, 0, sizeof(addr)) ;
    addr.sin_family = AF_INET ;
    addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    addr.sin_port = htons(20601) ;

    if(bind(recv_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadcast recv bind error\n") ;
#endif
    }

    while(1)
    {
        usleep(2000000) ;
        memset(recvbuff, 0, NORMALBUFFSIZE) ;
        cmd = 0 ;
        recv_len = recvfrom(recv_sock, recvbuff, NORMALBUFFSIZE, 0, NULL, 0) ;
        if(recv_len > 0)
        {
#ifdef NETWORK_DEBUG
            DEBUG_PRI("recvelen = %d\n",recv_len) ;
#endif

            Timecheck = (TIMECHECK *)recvbuff ;
            year = ntohs(Timecheck->year) ;
            month = ntohs(Timecheck->month) ;
            day = ntohs(Timecheck->day) ;
            hour = ntohs(Timecheck->hour) ;
            minute = ntohs(Timecheck->minute) ;
            second = ntohs(Timecheck->second) ;

            tm_ptr.tm_year = year - 1900 ;
            tm_ptr.tm_mon  = month - 1;
            tm_ptr.tm_mday = day ;
            tm_ptr.tm_hour = hour ;
            tm_ptr.tm_min  = minute ;
            tm_ptr.tm_sec  = second ;

            m_time = mktime(&tm_ptr) ;
            stime(&m_time) ;

#ifdef NETWORK_DEBUG
            DEBUG_PRI("%d, %d, %d, %d, %d, %d\n",month, day, hour, minute, second, year) ;
#endif

            break ;
        }
    }
    close(recv_sock) ;
}        

void ipserv() 
{
    char HwAddr[6] ;
    int udpsock = 0, retvalue = 0 ;

    struct sockaddr_in  svraddr ;

    udpsock = socket(PF_INET, SOCK_DGRAM, 0) ;
    if(udpsock == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadsock open error\n") ;
#endif
    }
    
    while(1)
    {
        if(Systemreboot.reboot == 1)
        {
            pthread_exit(NULL) ;
        }    
        retvalue = netaddr(NETWORKMODIFYCFG) ;
        retvalue = netaddr(IPSERVMODIFYNETWORKCFG) ;
        retvalue = netaddr(SYSINFOMODIFYCFG) ;
        
        memset(&svraddr, 0, sizeof(svraddr)) ;
        svraddr.sin_family = AF_INET ;
        svraddr.sin_addr.s_addr = Ipservnetworkcfg.ipaddress.s_addr ;
        
        svraddr.sin_port = htons(Ipservnetworkcfg.port) ;

        GetGlobalMacAddr(HwAddr);

        intropacket.identifier = htons(IDENTIFIER) ;
        intropacket.cmd = htons(CMD_INTROPACKET) ;
        memcpy(intropacket.macaddress, HwAddr, 6) ;
        intropacket.reserved1 = htons(0x00) ;
        intropacket.ipaddress.s_addr = Networkcfg.ipaddress.s_addr ;
        intropacket.subnetmask.s_addr = Networkcfg.subnetmask.s_addr ;
        intropacket.gateway.s_addr = Networkcfg.gateway.s_addr ;
        intropacket.versionmajor = htons(VERSIONMAJOR) ;
        intropacket.versionminor = VERSIONMINOR ;
        intropacket.versiontext = VERSIONTEXT ;
        intropacket.boardtype = htons(0x0000) ;
        intropacket.reserved2 = htons(0x0000) ;
        intropacket.Tlogonport = htons(Networkcfg.port) ;
        intropacket.Tcontrolport = htons(Networkcfg.port+1) ;
        intropacket.Tdataport = htons(Networkcfg.port+2 ) ;
        intropacket.Ulogonport = htons(Networkcfg.port) ;
        intropacket.Ucontrolport = htons(Networkcfg.port+1) ;
        intropacket.Udataport = htons(Networkcfg.port+2) ;
        strncpy(intropacket.servertitle,"boooom box",10) ;
        strncpy(intropacket.serverlocation,"worldwide",9) ;
        strncpy(intropacket.serverdescription, Sysinfocfg.boooomboxname,32) ;
        strncpy(intropacket.cameratitle1,Sysinfocfg.boooomboxname,32) ;
        strncpy(intropacket.cameratitle2,Sysinfocfg.composite1,32) ;
        strncpy(intropacket.cameratitle3,Sysinfocfg.composite2,32) ;
        strncpy(intropacket.cameratitle4,Sysinfocfg.svideo,32) ;
 
        sendto(udpsock, &intropacket, sizeof(INTROPACKET), 0, (struct sockaddr*)&svraddr, sizeof(svraddr)) ;
        sleep(Ipservnetworkcfg.interval) ;
    }
}



void updateserv()
{
    FILE *fp ;
    int update_sock = 0, recv_len = 0, recvflag = 0 ;
    char recvbuff[UPDATEBUFFSIZE] ;

    struct sockaddr_in servaddr ;

    update_sock = socket(AF_INET, SOCK_DGRAM, 0) ;

    if(update_sock == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("update socket create error\n") ; 
#endif
    }

    memset(&servaddr, 0, sizeof(servaddr)) ;
    servaddr.sin_family = AF_INET ;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    servaddr.sin_port = htons(8401) ;

    if(bind(update_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
#ifdef NETWORK_DEBUG
        DEBUG_PRI("broadcast recv bind error\n") ;
#endif
    }

    memset(recvbuff, 0, UPDATEBUFFSIZE) ;

    fp = fopen("/mnt/settings/initrd.image","w") ;

    while(1)
    {
        recv_len = recvfrom(update_sock, recvbuff, UPDATEBUFFSIZE, 0, NULL, 0) ;
        if(recvflag == 0 && recv_len > 0)
        {
            fp = fopen("/mnt/settings/initrd.image","w") ;
            recvflag = 1 ;
        }
        if(recv_len > 0)
        {
            fwrite(recvbuff, recv_len, 1, fp) ;
            memset(recvbuff, 0, recv_len) ;
        }
        else 
        {
            fclose(fp) ;
            recvflag = 0 ;
        }
    }

}

void invalidserv(void)
{
    int sendlen = 0 ;

    while(1)
    {
        if(SystemInfo.Nosignal == 1)
        {
            Vstreamerror.identifier = htons(IDENTIFIER) ;
            Vstreamerror.cmd = htons(CMD_VSTREAM_ERROR) ;
            Vstreamerror.length = htons(10) ;
            Vstreamerror.channel = htons(1) ;
            Vstreamerror.errcode = htons(CMD_INVALID_DEVICE) ; // initCaptureDevice is not initialized
            if(SystemInfo.D_Channel[SystemInfo.datachannel] != 0)
            {
                sendlen = send(SystemInfo.D_Channel[SystemInfo.datachannel], &Vstreamerror, 10,0) ;
#ifdef NETWORK_DEBUG
                DEBUG_PRI("initCaptureDevice initialize fail.. Invalid Device....sendlen = %d\n",sendlen) ;
#endif
            }
        }
        sleep(1) ;
    }
}
#endif