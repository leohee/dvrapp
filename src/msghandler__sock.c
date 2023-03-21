//
// msghandler.c
//
//      This file contains command handling routine
//

#include <stdlib.h>
#include <fstruct.h>
#include <fdefine.h>
#include <fcommand.h>
#include <message.h>
#include <cmessage.h>
#include <dmessage.h>
#include <process.h>
#include <msghandler.h>

#define MSG_TRANS(msg)	(((msg)>>4) & 0xf00) + ((msg) & 0xff)
#define MSG_MAP(msg,func) msgfunc[MSG_TRANS(msg)]=(func);
#define MSG_MAP_SRV(msg,func) msgfunc_srv[(msg)]=(func);

//////////////////////////////////////////
// msgfunc

void (*msgfunc [MAXCOMMANDS])(int channel, char* data, int len) ;
void (*msgfunc_srv [MAXCOMMANDS])(char* data, int len) ;

void msginit (void)
{
    int i ;

    // Intialize the message handler map
    for (i = 0; i < MAXCOMMANDS; i++) 
    {
        msgfunc [i] = NULL ;
        msgfunc_srv [i] = NULL ;
    }

    MSG_MAP (CMD_USERKEY_REQ, userkeyreq)
    MSG_MAP (CMD_USERAUTH_REQ, userauthreq)
    MSG_MAP (CMD_USERCONFIRM_REQ, userconfirmreq)
    MSG_MAP (CMD_CLIENT_KICKOUT, kickout)
    MSG_MAP (CMD_DUSERCONFIRM_REQ, duserconfirmreq)
    MSG_MAP (CMD_SYSTEMPARAM_REQ, systemparamreq)
    MSG_MAP (CMD_PTZ_RAWDATA, ptzraw)

    MSG_MAP (CMD_VIDEOPARAM_REQ, videoparamreq)
    MSG_MAP (CMD_AUDIOPARAM_REQ, audioparamreq)

    MSG_MAP (CMD_BITRATEPARAM_REQ, bitrateparamreq)
    MSG_MAP (CMD_SERVERPARAM_REQ, serverparamreq)
    MSG_MAP (CMD_USERPARAM_REQ, userparamreq)

    MSG_MAP (CMD_RESOLUTIONPARAM_REQ, resolutionparamreq)

    MSG_MAP (CMD_SYSTEMCFG_REQ, systemcfgreq)
    MSG_MAP (CMD_SYSTIMECFG_REQ, systimecfgreq)
    MSG_MAP (CMD_SVRINFOCFG_REQ, svrinfocfgreq)
    MSG_MAP (CMD_SLIMITCFG_REQ, slimitcfgreq)

    MSG_MAP (CMD_SVRPORTPARAM_REQ, svrportparamreq)
    MSG_MAP (CMD_SVRPORTCFG_REQ, svrportcfgreq)


    MSG_MAP (CMD_VIDEOCFG_REQ, videocfgreq)
    MSG_MAP (CMD_AUDIOCFG_REQ, audiocfgreq)
    MSG_MAP (CMD_BITRATECFG_REQ, bitratecfgreq)
    MSG_MAP (CMD_USERCFG_REQ, usercfgreq)
    MSG_MAP (CMD_RESOLUTIONCFG_REQ, resolutioncfgreq)

    MSG_MAP (CMD_REBOOT_REQ, reboot)
    MSG_MAP (CMD_IDNPARAM_REQ, idnparamreq)
    MSG_MAP (CMD_IDNCFG_REQ, idncfgreq)

    MSG_MAP (CMD_STRHEADER_REQ, strheaderreq)
    MSG_MAP (CMD_VSTREAM_START, vstreamstart)
    MSG_MAP (CMD_ASTREAM_START, astreamstart)

    MSG_MAP (CMD_SDALARM_OPEN, sdalarmopen)
    MSG_MAP (CMD_FRAMERATEPARAM_REQ, framerateparamreq)
    MSG_MAP (CMD_FRAMERATECFG_REQ, frameratecfgreq)

    MSG_MAP (CMD_IFRAMESETTINGPARAM_REQ, iframesettingparamreq)
    MSG_MAP (CMD_IFRAMESETTINGCFG_REQ, iframesettingcfgreq)
    MSG_MAP (CMD_MACCHANGE_REQ, macchange)

    MSG_MAP (CMD_PTZCTRLCFG_REQ, ptzctrlcfgreq)
    MSG_MAP (CMD_PTZCTRLPARAM_REQ, ptzctrlparamreq)
    
    //MSG_MAP (CMD_PTZINFOCFG_REQ, ptzinfocfgreq)
    //MSG_MAP (CMD_PTZINFOPARAM_REQ, ptzinfoparamreq)
    
    
    MSG_MAP (CMD_PLAYBACKDATE_REQ, playbackdatereq)
    MSG_MAP (CMD_PLAYBACKTIME_REQ, playbacktimereq)

    MSG_MAP (CMD_MULTI_PB_START_REQ, multipbstartreq)
    MSG_MAP (CMD_MULTI_PB_STOP_REQ, multipbstopreq)
    MSG_MAP (CMD_MULTI_PB_PAUSE_REQ, multipbpausereq)
    MSG_MAP (CMD_MULTI_PB_PLAY_FWD_REQ, multipbplayfwdreq)

}

