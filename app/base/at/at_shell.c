/****************************************************************
 file:         dcom_shell.c
 description:  the source file of data communciation cmd implementation
 date:         2017/7/14
 author        wangqinglong
 ****************************************************************/
#include "com_app_def.h"
#include "cfg_api.h"
#include "cfg_para_api.h"
#include "shell_api.h"
#include "at_api.h"
#include "at_op.h"
#include "tbox_limit.h"
#include "at.h"
#include "audio.h"
extern int audio_basic_ICall(void);
/****************************************************************
function:     at_shell_makecall
description:  make call
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int at_shell_makecall(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: makecall xxx\r\n");
        return -1;
    }
	
    if( strlen(argv[0]) > CALL_NUM_SIZE )
    {
        shellprintf(" the telephone num too long\r\n");
        return -1;
    }

    makecall((char *)argv[0]);
	audio_basic_ICall();
    shellprintf(" begin to make call\r\n");
    return 0;
}

/****************************************************************
function:     at_shell_seticall
description:  set icall
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int at_shell_seticall(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf("usage:seticall xxx\r\n");
        return -1;
    }

    ret = cfg_set_para(CFG_ITEM_ICALL, (unsigned char *)argv[0], 32);

    if (ret != 0)
    {
        shellprintf("set icall failed,ret=%d\r\n", ret);
    }
    else
    {
        shellprintf("set icall ok\r\n");
    }

    return ret;
}

/****************************************************************
function:     at_shell_setbcall
description:  set bcall
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int at_shell_setbcall(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf("usage:setbcall xxx\r\n");
        return -1;
    }

    ret = cfg_set_para(CFG_ITEM_BCALL, (unsigned char *)argv[0], 32);

    if (ret != 0)
    {
        shellprintf("set bcall failed,ret=%d\r\n", ret);
    }
    else
    {
        shellprintf("set bcall ok\r\n");
    }

    return ret;
}
/****************************************************************
function:     at_shell_setecall
description:  set ecall
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int at_shell_setecall(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf("usage:setbcall xxx\r\n");
        return -1;
    }

    ret = cfg_set_para(CFG_ITEM_ECALL, (unsigned char *)argv[0], 32);

    if (ret != 0)
    {
        shellprintf("set ecall failed,ret=%d\r\n", ret);
    }
    else
    {
        shellprintf("set ecall ok\r\n");
    }

    return ret;
}

/****************************************************************
function:     at_shell_setwhitelist
description:  set whitelist
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int at_shell_setwhitelist(int argc, const char **argv)
{
    int wlen;
    int ret;

    if (argc != 1)
    {
        shellprintf("usage:setwhitelist \"xxx;xxx;xxx;\"\r\n");
        return -1;
    }

    wlen = strlen((char *)argv[0]);

    if (wlen > 512)
    {
        shellprintf("failed,set whitelist length(%d) > 512\r\n", wlen);
        return -1;
    }

    ret = cfg_set_para(CFG_ITEM_WHITE_LIST, (unsigned char *)argv[0], 512);

    if (ret != 0)
    {
        shellprintf("set whitelist failed,ret=%d\r\n", ret);
    }
    else
    {
        shellprintf("set whitelist ok\r\n");
    }

    return ret;
}

/****************************************************************
 function:     at_shell_set_net_type
 description:  set net type
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_set_net_type(int argc, const char **argv)
{
    int ret;
    unsigned char net_type;

    if (argc != 1)
    {
        shellprintf(" usage:setnet 2G/3G/4G/auto\r\n");
        return AT_INVALID_PARA;
    }

    if (0 == strncmp(argv[0], "2G", 2))
    {
        net_type = 1;
    }
    else if (0 == strncmp(argv[0], "3G", 2))
    {
        net_type = 2;
    }
    else if (0 == strncmp(argv[0], "4G", 2))
    {
        net_type = 3;
    }
    else if (0 == strncmp(argv[0], "auto", 4))
    {
        net_type = 0;
    }
    else
    {
        shellprintf(" usage:setnet 2G/3G/4G/auto\r\n");
        return AT_INVALID_PARA;
    }

    
    ret = at_network_switch(net_type);    

//    ret = cfg_set_para(CFG_ITEM_NET_TYPE, &net_type, sizeof(net_type));

    if (ret != 0)
    {
        shellprintf(" set wan apn failed,ret:0x%08x\r\n", ret);
        return ret;
    }

    shellprintf(" set net ok\r\n");

    return 0;
}

/****************************************************************
 function:     at_shell_set_wifi
 description:  set wifi on/off
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_set_wifi(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf(" usage:setwifi on/off\r\n");
        return AT_INVALID_PARA;
    }

    if (0 == strncmp("on", argv[0], 2))
    {
        ret = wifi_enable();
    }
    else if (0 == strncmp("off", argv[0], 3))
    {
        ret = wifi_disable();
    }
    else
    {
        shellprintf(" usage:setwifi on/off\r\n");
        return -1;
    }

    if (0 != ret)
    {
        shellprintf(" set wifi failed\r\n");
        return ret;
    }

    shellprintf(" set wifi ok\r\n");

    return 0;
}

/****************************************************************
 function:     at_shell_set_ssid
 description:  set wifi on/off
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_set_ssid(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf(" usage:setssid xxxx\r\n");
        return AT_INVALID_PARA;
    }

    if (strlen(argv[0]) >= WIFI_SSID_SIZE)
    {
        shellprintf(" wifi ssid name length < 32\r\n");
        return AT_PARA_TOO_LONG;
    }

    ret = wifi_set_ssid(argv[0]);

    if (0 != ret)
    {
        shellprintf(" set wifi ssid failed\r\n");
        return ret;
    }

    shellprintf(" set wifi ssid ok\r\n");

    return 0;
}

/****************************************************************
 function:     at_shell_set_ap_key
 description:  set wifi on/off
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_set_ap_key(int argc, const char **argv)
{
    int ret;

    if (argc != 1)
    {
        shellprintf(" usage:setapkey xxxx\r\n");
        return AT_INVALID_PARA;
    }

    if ((strlen((char *)argv[0]) >= WIFI_PASSWD_SIZE) || (strlen((char *)argv[0]) < 8))
    {
        shellprintf(" 8 =< wifi key length < 32\r\n");
        return AT_INVALID_PARA;
    }

    ret = wifi_set_key(argv[0]);

    if (0 != ret)
    {
        shellprintf(" set wifi key failed\r\n");
        return ret;
    }

    shellprintf(" set wifi key ok\r\n");

    return 0;
}

/****************************************************************
 function:     at_shell_set_max_assoc
 description:  set wifi max assoc
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_set_max_assoc(int argc, const char **argv)
{
    int ret;
    unsigned int max_sta = 0;

    if (argc != 1)
    {
        shellprintf(" usage:setmaxsta xx\r\n");
        return AT_INVALID_PARA;
    }

    sscanf((char *) argv[0], "%d", &max_sta);

    if ((0 == max_sta) || (max_sta > 8))
    {
        shellprintf(" error: not support\r\n");
        return AT_INVALID_PARA;
    }

    log_o(LOG_AT, "set wifi max user: %d ", max_sta);

    ret = wifi_set_max_user(max_sta);

    if (0 != ret)
    {
        shellprintf(" set max station failed\r\n");
        return ret;
    }

    shellprintf(" set max station ok\r\n");

    return 0;
}

/****************************************************************
 function:     at_shell_at_test
 description:  set wifi max assoc
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int at_shell_at_test(int argc, const char **argv)
{
    if (argc != 1)
    {
        log_e(LOG_AT, "usage:attest xxx\r\n");
        return AT_INVALID_PARA;
    }

    at_test(argv[0]);
    shellprintf(" at test:[%s]\r\n", argv[0]);

    return 0;
}

int audio_shell_show_all_reg(int argc, const char **argv)
{
    if (argc != 0)
    {
        log_e(LOG_AT, "usage:audioreadallreg\r\n");
        return AT_INVALID_PARA;
    }

    audio_show_all_registers();

    return 0;

}

int audio_shell_set_reg(int argc, const char **argv)
{
    int ret;
    int reg, val;
    if (argc != 2)
    {
        log_e(LOG_AT, "usage:audiosetreg reg(0x) val(0x)\r\n");
        return AT_INVALID_PARA;
    }

    sscanf((char *) argv[0], "%x", &reg);
    sscanf((char *) argv[1], "%x", &val);

    if ( ((reg < 0)||(reg > 0xff)) || ((val < 0)||(val > 0xff)) )
    {
        shellprintf(" error: not support\r\n");
        return AT_INVALID_PARA;
    }
    ret = register_config((unsigned char)reg, (unsigned char)val);

    if(ret < 0)
    {
        shellprintf(" set reg error\r\n");        
    }
    else
    {
        shellprintf(" set reg ok\r\n");
    }
    return 0;

}

int audio_shell_4G_loop(int argc, const char **argv)
{
    if (argc != 0)
    {
        log_e(LOG_AT, "usage:audio4gloop\r\n");
        return AT_INVALID_PARA;
    }

    at_set_audioloop(1);
    at_set_audioloopGAIN(10000);

    at_query_audioloop();
    at_query_audioloopGAIN();   

    return 0;

}

int audio_shell_audio_to_micout(int argc, const char **argv)
{
    if (argc != 1)
    {
        log_e(LOG_AT, "usage:audioaudiotomicout on/off\r\n");
        return AT_INVALID_PARA;
    }

    /*note:lm49352 is bypass,but tlv320aic3104 is route 4G to HPLOUT.*/
    /*so,the result is the same when 4G is audioloop*/
    if(strcmp(argv[0],"on") == 0)
    {
        audio_route_mic_bypass(0);
    }
    else if(strcmp(argv[0],"off") == 0)
    {
        audio_route_mic_bypass(1);
    }
    else
    {
        log_e(LOG_AT, "usage:audioaudiotomicout on/off\r\n");
    }

    return 0;

}

int audio_shell_4G_to_spkout(int argc, const char **argv)
{
    if (argc != 1)
    {
        log_e(LOG_AT, "usage:audio4gtospkout on/off\r\n");
        return AT_INVALID_PARA;
    }

    if(strcmp(argv[0],"on") == 0)
    {
        audio_route_4G_to_spkout(0);
    }
    else if(strcmp(argv[0],"off") == 0)
    {
        audio_route_4G_to_spkout(1);
    }
    else
    {
        log_e(LOG_AT, "usage:audio4gtospkout on/off\r\n");
    }

    return 0;

}

int audio_shell_4G_to_ols(int argc, const char **argv)
{
    if (argc != 1)
    {
        log_e(LOG_AT, "usage:audio4gtools on/off\r\n");
        return AT_INVALID_PARA;
    }

    if(strcmp(argv[0],"on") == 0)
    {
        audio_route_4G_to_ols(0);
    }
    else if(strcmp(argv[0],"off") == 0)
    {
        audio_route_4G_to_ols(1);
    }
    else
    {
        log_e(LOG_AT, "usage:audio4gtools on/off\r\n");
    }

    return 0;

}


/****************************************************************
 function:     at_shell_init
 description:  initiaze wireless communcaiton device
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_shell_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* register shell cmd */
            shell_cmd_register("makecall",     at_shell_makecall,    "make call");
            shell_cmd_register("seticall",     at_shell_seticall,    "set icall");
            shell_cmd_register("setbcall",     at_shell_setbcall,    "set bcall");
			shell_cmd_register("setecall",     at_shell_setecall,    "set ecall");
            shell_cmd_register("setwhitelist", at_shell_setwhitelist, "set whitelist");
            shell_cmd_register("setnet", at_shell_set_net_type, "set net type 2G/3G/4G/auto");
            shell_cmd_register("setwifi", at_shell_set_wifi, "set wifi on/off");
            shell_cmd_register("setssid", at_shell_set_ssid, "set wifi ssid");
            shell_cmd_register("setapkey",  at_shell_set_ap_key,    "set wifi password");
            shell_cmd_register("setmaxassoc", at_shell_set_max_assoc, "set wifi max assoc");
            shell_cmd_register("attest", at_shell_at_test, "at test");
            shell_cmd_register("audioreadallreg", audio_shell_show_all_reg, "read all audio register");
            shell_cmd_register("audiosetreg", audio_shell_set_reg, "set audio register");
            shell_cmd_register("audio4gloop", audio_shell_4G_loop, "set 4g audio loop");
            shell_cmd_register("audioaudiotomicout", audio_shell_audio_to_micout, "set audio to micout");
            shell_cmd_register("audio4gtospkout", audio_shell_4G_to_spkout, "set audio 4g to speakout");
            shell_cmd_register("audio4gtools", audio_shell_4G_to_ols, "set audio 4g to ols");
            break;

        default:
            break;
    }

    return 0;
}
