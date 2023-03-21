#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <fstruct.h>
#include <fdefine.h>
#include <networkcfg.h>
#include <rawsockio.h>
//#include <libdvr365.h>
#include "lib_dvr_app.h"
#define  MAXCFG_LENGTH   1024	
#define  S_OK     1
#define  S_ERR    0
#define  CREATE   malloc
#define  KEY_NUMERIC	256
#define	 KEY_STRING	257
#define	 KEY_ENCLOSED   258
#define  KEY_BOOLEAN	259
#define  KEY_NEWLINE    ('\n')
#define  KEY_EOF	(EOF)
#define  KEY_INVALID	260

static int	 gettokenvs(FILE *fp);
//static KEY_LIST  * search_cfg_keyvs(char * key);

struct TVALUE {
   int returnval ;
   int returnval1 ;
} Tvalue, Svalue ;

NETWORKCFG Networkcfg ;
NETSETTINGCHANGE Netsettingchange ;

int init_globalsvs( char *cfgfile, int filetype);
struct TVALUE read_cfg_filevs( char *filename, int filetype );
void init_cfgvs();
//int search_int_keyvs( char *key, int dft );
//char * search_str_keyvs( char *key, char *dft );
//int BlockNovs(int x, int y) ;	
static KEY_LIST		*key_head = NULL;

static unsigned int 	lineno ;
static int	lexval ;
static char		lexbuf[MAXCFG_LENGTH];

void init_cfgvs()
{
    KEY_LIST	*tmp, *prev;

    lineno = 1;

    tmp = key_head;
    while( tmp != NULL )
    {
        prev = tmp;
	tmp = tmp->next;
	free(prev);
    }
    key_head = NULL;
}

int netaddr(int filetype)
{
    int addrno = 0 ;
    char  cfg_file[1024];
    strcpy( cfg_file, "/etc/network/");	
    if(filetype == NETWORKMODIFYCFG)
    {
        strcat( cfg_file, "cfg-network" );
    }
    addrno = init_globalsvs( cfg_file, filetype ) ;
    return addrno ;
}

int init_globalsvs( char *cfgfile, int filetype)
{
    int cnt ;
    init_cfgvs(); /* KEY_LIST initation */
	//read_cfg_filevs(cfgfile, filetype) ;

    Svalue = read_cfg_filevs(cfgfile, filetype) ;
    if(Svalue.returnval1 != S_OK)
    {
//	my_log( LOG_WARN, "Cannot read given cfg file. ");
    }

//  loglevel = search_int_key("loglevel", 5);
//  printf("loglevel = %d\n", loglevel);

    cnt = Svalue.returnval ;

    return 0;//cnt ;
}

 struct TVALUE  read_cfg_filevs( char *filename , int filetype)
{
    FILE	*fp = NULL, *fd = NULL;
    int	token, retval = S_OK, intval = 0, i = 0, j = 0;
    char	*key, *strval = NULL;
//    KEY_LIST	*tmp;
    char HwAddr[6] ;
	
    char ipaddress[16] ;
    char subnetmask[16] ;
    char gateway[16] ;
    char broadcast[16] ;

    int port = 0, dhcp = 0;
    int fflag = 0 ;

    memset(ipaddress, 0, 16) ;
    memset(subnetmask, 0, 16) ;
    memset(gateway, 0, 16) ;
    memset(broadcast, 0, 16) ;

    if( filename == NULL )
    {
        retval = S_ERR;
    }

    if(( fp = fopen( filename, "r")) == NULL)
    {
//        printf("file open error \n") ;

        if(filetype == NETWORKMODIFYCFG)
        {
            if((fd = fopen("/etc/network/cfg-network","w")) == NULL)
            {
                printf("cfg-network file open error\n") ;
            }
            GetGlobalMacAddr(HwAddr);
            dhcp = 0 ;  // 0 static 1 dhcp
            strncpy(ipaddress, "192.168.0.202",13) ;
            strncpy(subnetmask, "255.255.255.0",13) ;
            strncpy(gateway, "192.168.0.1", 11) ;
            strncpy(broadcast, "192.168.0.255", 15) ;
            port = 7620 ;

            fprintf(fd, "DHCP=\"%d\"\n",dhcp) ;
            fprintf(fd, "MAC=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",HwAddr[0], HwAddr[1], HwAddr[2], HwAddr[3], HwAddr[4], HwAddr[5]) ;
            fprintf(fd, "IPADDR=\"%s\"\n",ipaddress) ;
            fprintf(fd, "NETMASK=\"%s\"\n",subnetmask) ;
    	    fprintf(fd, "GATEWAY=\"%s\"\n",gateway) ;
            fprintf(fd, "BROADCAST=\"%s\"\n",broadcast) ;
            fprintf(fd, "PORT=%d\n",port) ;
            fprintf(fd, "NETWORK=\"\"\n") ;
            fclose(fd) ;
           Netsettingchange.setting = 1 ;
           usleep(30000) ;
           Netsettingchange.setting = 0 ;
        }
   
        fflag = 1 ;
    }

    if(fflag == 1)
    {
        if(( fp = fopen( filename, "r")) == NULL)
        {
            printf("file reopen error %s\n",filename) ;
        }
            fflag = 0 ;
    }

    for(;;)
    {   
	token = gettokenvs( fp );
	if( token == KEY_NEWLINE ) continue;

	if( token == KEY_EOF )
		break;
	if( token != KEY_STRING )
	{
	    retval = S_ERR;
	    continue;
	}
 	key = strdup (lexbuf );
	//printf("lexbuf=%s\n",lexbuf);
	if(gettokenvs(fp) != '=')
	{
	    exit(0);
	    retval = S_ERR;
	    free(key);
	    continue;
	}
	token = gettokenvs(fp);

        if(filetype == NETWORKMODIFYCFG)
        {
	    switch(token)
	    {
	        case  KEY_STRING:
//                    strcpy(&Buf[j][i], lexbuf) ;
                      i++ ;
                      if( i == 7  )
                      {
                          i = 0;
                          j++ ;
                      }
                      break ;
                case  KEY_ENCLOSED:
		      strval = strdup(lexbuf);
			//printf("KEY_ENCLOSED:lexbuf=%s,i=%d\n",lexbuf,i);
			 /*  if(i == 0)
			   {
				if(key == "DHCP")
				 	Networkcfg.dhcp = lexval;	
			   }*/
                      if(i == 1)
                      {
                          strcpy(Networkcfg.macaddress, strval) ;
                      } 
                      if(i == 2)
                      {
                          Networkcfg.ipaddress.s_addr = inet_addr(strval) ;
                      }
                      if(i == 3)
                      {
                          Networkcfg.subnetmask.s_addr = inet_addr(strval) ;
                      }
                      if(i == 4)
                      {
                          Networkcfg.gateway.s_addr = inet_addr(strval) ;
                      }
                      if(i == 5)
                      {
                          Networkcfg.broadcast.s_addr = inet_addr(strval) ;
                      }
                      i++ ;
                      if(i == 7)
                      {  
                          i = 0;
                          j++ ;
                      }
	              token  = KEY_STRING;
		      break;
	        case  KEY_NUMERIC:
				//printf("KEY_NUMERIC:lexbuf=%s,i=%d\n",lexbuf,i);
                      if(i == 0)
                      {
                          Networkcfg.dhcp = lexval ;
                      }
                      if(i == 6)
                      { 
                          Networkcfg.port = lexval ; 
                      }                    
                      i++ ;
	   	      if( i == 7 )
		      {
		          i = 0;
	                  j++;
		      }
                      break ;
		case  KEY_BOOLEAN:
		      intval = lexval;
		      break;
		      default:
		      printf(" file '%s': Data values required on line %d",
				filename, lineno );
		      exit(0);
		      retval = S_ERR;
		      continue;
            }
        } 
    }
	//printf("close fp now!!!!\n");
	fclose(fp);	
	//printf("close fp : i=%d!!!!\n",i);
        Tvalue.returnval = j ;
        Tvalue.returnval1 = retval ;
       //printf("read_cfg_filevs: port=%d!!\n",,Networkcfg.port);
        return Tvalue;
	
}


static int gettokenvs( FILE * fp )
{
    int ch;
	
    while((( ch = getc(fp))) != EOF )
    {
	//printf("ch = %d\n", ch);
	if(isspace(ch)) continue;

	if( ch == '\n' )
	{ 
	    lineno++;
	    return KEY_NEWLINE;
	}

	if( ch == '#' )
        {
	fgets( lexbuf, MAXCFG_LENGTH, fp );
	return KEY_NEWLINE;
	}
        if( ch == ':')
        {
            fgets( lexbuf, TITLESIZE, fp );
            return KEY_STRING ;
        }

	if(isdigit(ch))
	{
	    ungetc(ch, fp);
	    fscanf(fp, "%i", &lexval);
	    return    KEY_NUMERIC;
	}
	if(isalpha(ch) || ch == '_') 
	{
	    ungetc( ch, fp );
	    fscanf( fp, "%[a-zA-Z0-9_]", lexbuf );
	    if( !strcasecmp( lexbuf, "y") || !strcasecmp( lexbuf, "yes")
				|| !strcasecmp( lexbuf, "on"))
	    {
	        lexval = 1;
	        return	KEY_BOOLEAN;
	    }
	    if( !strcasecmp( lexbuf, "n" ) || !strcasecmp( lexbuf, "no")
					|| !strcasecmp( lexbuf, "off")) 
				
	    {
		lexval = 0;

		return	KEY_BOOLEAN;
	    }
		return	KEY_STRING;
	 }
         if(ch =='-')
         {
             ungetc(ch, fp);
	     fscanf(fp, "%i", &lexval);
	     return    KEY_NUMERIC;
         }
       

	if( ch == '"')
	{
	    register int		i = 0;
	    while(( ch = fgetc(fp)) !=EOF && ch != '\n' )
	    {
		if( ch == '"' )
		{
		    lexbuf[i] = '\0';
		    return		KEY_ENCLOSED;
		}
                if( ch == '\\')
		{
		    lexbuf[i++] = (int) fgets( lexbuf, MAXCFG_LENGTH, fp );
		}
		else
		lexbuf[i++] = ch;
	    }
		return		KEY_INVALID;
	}
	return		ch;
    }
	return		KEY_EOF;
}	

#if 0
char * search_str_keyvs( char *key, char *dft )
{
    KEY_LIST		*tmp;

    tmp = search_cfg_keyvs(key);

    if(tmp != NULL ) return tmp->strval;

    return dft;
}

int search_int_keyvs( char *key, int dft )
{
    KEY_LIST	*tmp;

    tmp = search_cfg_keyvs(key);

    if( tmp != NULL ) return tmp->intval;

    return  dft;

}

static KEY_LIST * search_cfg_keyvs(char *key)
{
    KEY_LIST	*tmp;

    tmp = key_head;

    while(tmp != NULL ) 
    {
        if(!strcasecmp( key, tmp->key))
        {
	    return  tmp;
	}
	tmp = tmp->next;
    }

    return NULL;
}


int GetIPtype()
{
    
    netaddr(NETWORKMODIFYCFG) ;
    return Networkcfg.dhcp ;
    
}

char* GetMacAddr()
{

    netaddr(NETWORKMODIFYCFG) ;
    return Networkcfg.macaddress ;

}

char* GetIPaddr()
{
    netaddr(NETWORKMODIFYCFG) ;
    return inet_ntoa(Networkcfg.ipaddress) ;
}

char* GetNetMask()
{
    netaddr(NETWORKMODIFYCFG) ;
    return inet_ntoa(Networkcfg.subnetmask) ;

}

char* GetGateWay()
{
    netaddr(NETWORKMODIFYCFG) ;
    return inet_ntoa(Networkcfg.gateway) ;

}

int GetPort()
{
    netaddr(NETWORKMODIFYCFG) ;
    return Networkcfg.port ;

}

void SetIPtype(int type)
{
    FILE *fd = NULL ;

    netaddr(NETWORKMODIFYCFG) ;

    if((fd = fopen("/etc/network/cfg-network","w")) == NULL)
    {
         printf("cfg-network file open error\n") ;
    }
 
    fprintf(fd, "DHCP=%d\n",type) ;
    fprintf(fd, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
    fprintf(fd, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
    fprintf(fd, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
    fprintf(fd, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
    fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
    fprintf(fd, "PORT=%d\n",Networkcfg.port) ;
    fprintf(fd, "NETWORK=\"\"\n") ;
    fclose(fd) ;

    if(type == 1)
    {
        // dhcp
       // LIBDVR365_setSysstop() ;
        system("/etc/rc.d/rc3.d/S98network") ;
        sleep(5) ;
        //LIBDVR365_setSysstart() ;
    }
}

void SetMacInfo(char *macaddress)
{
    FILE *fd = NULL ;
    netaddr(NETWORKMODIFYCFG) ;
  
    if((fd = fopen("/etc/network/cfg-network","w")) == NULL)
    {
         printf("cfg-network file open error\n") ;
    }

    fprintf(fd, "DHCP=%d\n",Networkcfg.dhcp) ;
    fprintf(fd, "MAC=\"%s\"\n",macaddress) ;
    fprintf(fd, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
    fprintf(fd, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
    fprintf(fd, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
    fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
    fprintf(fd, "PORT=%d\n",Networkcfg.port) ;
    fprintf(fd, "NETWORK=\"\"\n") ;
    fclose(fd) ;

}

void SetIPInfo(char* ipaddr, char* submask, char* gateway)
{
    FILE *fd = NULL ;
    netaddr(NETWORKMODIFYCFG) ;
  
    if((fd = fopen("/etc/network/cfg-network","w")) == NULL)
    {
         printf("cfg-network file open error\n") ;
    }

    fprintf(fd, "DHCP=%d\n",Networkcfg.dhcp) ;
    fprintf(fd, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
    fprintf(fd, "IPADDR=\"%s\"\n",ipaddr) ;
    fprintf(fd, "NETMASK=\"%s\"\n",submask) ;
    fprintf(fd, "GATEWAY=\"%s\"\n",gateway) ;
    fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
    fprintf(fd, "PORT=%d\n",Networkcfg.port) ;
    fprintf(fd, "NETWORK=\"\"\n") ;
    fclose(fd) ;

    //LIBDVR365_setSysstop() ;
    system("/etc/rc.d/rc3.d/S98network") ;
    sleep(5) ;
 //   LIBDVR365_setSysstart() ;
}

void SetPort(int port)
{
    FILE *fd = NULL ;
    netaddr(NETWORKMODIFYCFG) ;

    if((fd = fopen("/etc/network/cfg-network","w")) == NULL)
    {
         printf("cfg-network file open error\n") ;
    }

    fprintf(fd, "DHCP=%d\n",Networkcfg.dhcp) ;
    fprintf(fd, "MAC=\"%s\"\n",Networkcfg.macaddress) ;
    fprintf(fd, "IPADDR=\"%s\"\n",inet_ntoa(Networkcfg.ipaddress)) ;
    fprintf(fd, "NETMASK=\"%s\"\n",inet_ntoa(Networkcfg.subnetmask)) ;
    fprintf(fd, "GATEWAY=\"%s\"\n",inet_ntoa(Networkcfg.gateway)) ;
    fprintf(fd, "BROADCAST=\"%s\"\n",inet_ntoa(Networkcfg.broadcast)) ;
    fprintf(fd, "PORT=%d\n",port) ;
    fprintf(fd, "NETWORK=\"\"\n") ;
    fclose(fd) ;

    LIBDVR365_setSysstop() ;
    system("/etc/rc.d/rc3.d/S98network") ;
    Netsettingchange.setting = 1 ;
    sleep(5) ;

    //LIBDVR365_setSysstart() ;
    //LIBDVR365_startStream() ;
    DVR_sock_startStream();
    
}

#endif


