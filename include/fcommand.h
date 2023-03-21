/* 
 * fcommand.h
 *
 * this file contains command number
 */

#ifndef _FCOMMAND_H
#define _FCOMMAND_H_

#define CMD_USERKEY_REQ              0x1001      
#define CMD_USERKEY_RES              0x1002
#define CMD_USERAUTH_REQ             0x1011
#define CMD_USERAUTH_RES             0x1012
#define CMD_USERCONFIRM_REQ          0x1021
#define CMD_CLIENT_KICKOUT           0x1101


#define CMD_DUSERCONFIRM_REQ         0x9001

#define CMD_USERCONFIRM_RES          0x1022
#define CMD_SERVER_BUSY              0x1100
#define CMD_SDALARM_OPEN             0x6021

#define CMD_SYSTEMPARAM_REQ          0x2001
#define CMD_SYSTEMPARAM_RES          0x2002
#define CMD_VIDEOPARAM_REQ           0x2011
#define CMD_VIDEOPARAM_RES           0x2012
#define CMD_AUDIOPARAM_REQ           0x2021
#define CMD_AUDIOPARAM_RES           0x2022    
#define CMD_PTZCTRLPARAM_REQ         0x2031
#define CMD_PTZCTRLPARAM_RES         0x2032
#define CMD_PTZINFOPARAM_REQ         0x2041
#define CMD_PTZINFOPARAM_RES         0x2042
#define CMD_SENSORPARAM_REQ          0x2051
#define CMD_SENSORPARAM_RES          0x2052
#define CMD_SDALARMADDRESS_REQ       0x2061
#define CMD_SDALARMADDRESS_RES       0x2062
#define CMD_MOTIONPARAM_REQ          0x2071
#define CMD_MOTIONPARAM_RES          0x2072
#define CMD_MDALARMADDRESS_REQ       0x2081
#define CMD_MDALARMADDRESS_RES       0x2082 
#define CMD_BITRATEPARAM_REQ         0x2091
#define CMD_BITRATEPARAM_RES         0x2092
#define CMD_USERPARAM_REQ            0x20A1
#define CMD_USERPARAM_RES            0x20A2
#define CMD_BIOSPARAM_REQ            0x20B1
#define CMD_BIOSPARAM_RES            0x20B2
#define CMD_SERVERPARAM_REQ          0x20C1
#define CMD_SERVERPARAM_RES          0x20C2
#define CMD_NETWORKPARAM_REQ         0x20D1
#define CMD_NETWORKPARAM_RES         0x20D2
 
#define CMD_RESOLUTIONPARAM_REQ      0x2101
#define CMD_RESOLUTIONPARAM_RES      0x2102

#define CMD_IFRAMESETTINGPARAM_REQ   0x2161
#define CMD_IFRAMESETTINGPARAM_RES   0x2162

#define CMD_SVRPORTPARAM_REQ         0x2171
#define CMD_SVRPORTPARAM_RES         0x2172

#define CMD_FRAMERATEPARAM_REQ       0x2193
#define CMD_FRAMERATEPARAM_RES       0x2194

#define CMD_PARAM_ERR                0x2ff0


#define CMD_SYSTEMCFG_REQ            0x3001
#define CMD_SYSTEMCFG_RES            0x3002

#define CMD_SYSTIMECFG_REQ           0x3003
#define CMD_SYSTIMECFG_RES           0x3004

#define CMD_VIDEOCFG_REQ             0x3011
#define CMD_VIDEOCFG_RES             0x3012

#define CMD_AUDIOCFG_REQ             0x3021
#define CMD_AUDIOCFG_RES             0x3022

#define CMD_PTZCTRLCFG_REQ           0x3031
#define CMD_PTZCTRLCFG_RES           0x3032

#define CMD_PTZINFOCFG_REQ           0x3041
#define CMD_PTZINFOCFG_RES           0x3042

#define CMD_SENSORCFG_REQ            0x3051
#define CMD_SENSORCFG_RES            0x3052

#define CMD_SDALRAMADDRESSCFG_REQ    0x3061
#define CMD_SDALRAMADDRESSCFG_RES    0x3062

#define CMD_MOTIONCFG_REQ            0x3071
#define CMD_MOTIONCFG_RES            0x3072

#define CMD_MDALRAMADDRESSCFG_REQ    0x3081
#define CMD_MDALRAMADDRESSCFG_RES    0x3082

#define CMD_BITRATECFG_REQ           0x3091
#define CMD_BITRATECFG_RES           0x3092

#define CMD_USERCFG_REQ              0x30A1
#define CMD_USERCFG_RES              0x30A2

#define CMD_BIOSCFG_REQ              0x30B1
#define CMD_BIOSCFG_RES              0x30B2 
#define CMD_ALLCFG_REQ               0x30C1
#define CMD_ALLCFG_RES               0x30C2

#define CMD_FRAMERATECFG_REQ         0x30D1
#define CMD_FRAMERATECFG_RES         0x30D2


#define CMD_SLIMITCFG_REQ            0x3303
#define CMD_SLIMITCFG_RES            0x3304
#define CMD_SVRINFOCFG_REQ           0x3305
#define CMD_SVRINFOCFG_RES           0x3306
#define CMD_SVRPORTCFG_REQ           0x3315
#define CMD_SVRPORTCFG_RES           0x3316

#define CMD_NETWORKCFG_REQ           0x3311
#define CMD_NETWORKCFG_RES           0x3312

#define CMD_MACCHANGE_REQ            0x3411
#define CMD_MACCHANGE_RES            0x3412

#define CMD_RESOLUTIONCFG_REQ        0x3B01
#define CMD_RESOLUTIONCFG_RES        0x3B02

#define CMD_CHCHANGECFG_REQ          0x3B51
#define CMD_CHCHANGECFG_RES          0x3B52

#define CMD_IFRAMESETTINGCFG_REQ     0x3B81
#define CMD_IFRAMESETTINGCFG_RES     0x3B82


#define CMD_STRHEADER_REQ            0x4001
#define CMD_STRHEADER_RES            0x4002

#define CMD_VSTREAM_START            0x4003
#define CMD_VSTREAM_DATA             0x4004
#define CMD_VSTREAM_STOP             0x4005
#define CMD_VSTREAM_ERROR            0x400F
#define CMD_ASTREAM_START            0x4011
#define CMD_ASTREAM_DATA             0x4012
#define CMD_ASTREAM_STOP             0x4013
#define CMD_ASTREAM_ERROR            0x401F
#define CMD_MSTREAM_START            0x4021     
#define CMD_MSTREAM_DATA             0x4022
#define CMD_MSTREAM_STOP             0x4023
#define CMD_MSTREAM_ERROR            0x402F

#define CMD_PLAYBACKDATE_REQ         0x4067
#define CMD_PLAYBACKDATE_RES         0x4068

#define CMD_PLAYBACKTIME_REQ         0x4071
#define CMD_PLAYBACKTIME_RES         0x4072

#define CMD_MULTI_PB_START_REQ       0x4110
#define CMD_MULTI_PB_START_RES       0x4111
#define CMD_SINGLE_PB_VIDEO_DATA     0x4204
#define CMD_SINGLE_PB_AUDIO_DATA     0x4212

#define CMD_NOSIGNAL                 0x4240
#define CMD_INVALID_DEVICE           0x4010

#define CMD_PTZ_RAWDATA              0x5100

#define CMD_CFGCHANGE                0x3101
#define CMD_NETCHANGE                0x3102
#define CMD_REBOOT_REQ               0x3201
#define CMD_REBOOT_RES               0x3202

#define CMD_CLOSEBYSERVER            1000
#define CMD_VSERVADDRESS_REQ         0x100F      
#define CMD_VSERVADDRESS_RES         0x101F      //For relay-srv

#define CMD_AOUT_REQ                 0x4031   //11월 11일 추가 
#define CMD_AOUT_RES                 0x4032
#define CMD_AOUT_DATA                0x4033   


#define CMD_IDNPARAM_REQ             0xA011
#define CMD_IDNPARAM_RES             0xA012

#define CMD_IDNCFG_REQ               0xB011
#define CMD_IDNCFG_RES               0xB012

// playback command
#define CMD_PSTREAM_START            0x5001
#define CMD_PSTREAM_DATA             0x5002
#define CMD_PSTREAM_STOP             0x5003
#define CMD_PSTREAM_ERROR            0x5004

#define CMD_INTROPACKET              0x0002

#define CMD_UPDATE_REQ               0x7001
#define CMD_UPDATE_RES               0x7002

#define CMD_MULTI_PB_STOP_REQ        0x4112
#define CMD_MULTI_PB_STOP_RES        0x4113

#define CMD_MULTI_PB_END_RES         0x4114

#define CMD_MULTI_PB_PLAY_FWD_REQ    0x4120
#define CMD_MULTI_PB_PLAY_FWD_RES    0x4121

#define CMD_MULTI_PB_PAUSE_REQ       0x4124
#define CMD_MULTI_PB_PAUSE_RES       0x4125



#endif /* _FCOMMAND_H_ */
