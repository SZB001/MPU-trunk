/****************************************************************
file:         dcom_shell.c
description:  the source file of nm shell cmd implementation
date:         2017/07/09
author        wangqinglong
****************************************************************/
#include "com_app_def.h"
#include "nm_api.h"
#include "cfg_api.h"
#include "cfg_para_api.h"
#include "shell_api.h"
#include "nm_dial.h"
#include "at_api.h"
#include "at.h"

/****************************************************************
function:     nm_shell_set_local_apn
description:  set local private apn
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_shell_set_local_apn(int argc, const char **argv)
{
    int ret;
    char apn[32] = {0};
    char wanapn[32] = {0};
    unsigned int apn_len;

    if (0 == argc)
    {
        memset(apn, 0, sizeof(apn));
    }
    else
    {
        strcpy(apn, (char *)argv[0]);
    }

    apn_len = sizeof(wanapn);
    ret = cfg_get_para(CFG_ITEM_WAN_APN, (unsigned char *)wanapn, &apn_len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get wan apn failed,ret:0x%08x", ret);
        return ret;
    }

    /*if new apn is same as wan apn*/
    if (!strcmp(apn, wanapn))
    {
        log_e(LOG_NM, "wan apn and local apn is same");
        return NM_APN_INVALID;
    }

    ret = cfg_set_para(CFG_ITEM_LOCAL_APN, (unsigned char *)apn, sizeof(apn));

    if (ret != 0)
    {
        log_e(LOG_NM, "set local apn failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     nm_shell_set_wan_apn
description:  set public apn
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_shell_set_wan_apn(int argc, const char **argv)
{
    int ret;
    char apn[32] = {0};
    char localapn[32] = {32};
    unsigned int apn_len;

    if (0 == argc)
    {
        memset(apn, 0, sizeof(apn));
    }
    else
    {
        strcpy(apn, (char const *)argv[0]);
    }

    apn_len = sizeof(localapn);
    ret = cfg_get_para(CFG_ITEM_LOCAL_APN , (unsigned char *)localapn, &apn_len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get local apn failed,ret:0x%08x", ret);
        return ret;
    }

    /*if new apn is same as local apn*/
    if (!strcmp(apn, localapn))
    {
        log_e(LOG_NM, "wan apn and local apn is same");
        return NM_APN_INVALID;
    }

    ret = cfg_set_para(CFG_ITEM_WAN_APN, (unsigned char *)apn, sizeof(apn));

    if (ret != 0)
    {
        log_e(LOG_NM, "set wan apn failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     nm_shell_set_auth
description:  set local apn dial authenticate
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_shell_set_auth(int argc, const char **argv)
{
    int  ret;
    CFG_DIAL_AUTH  auth;

    if (argc != 2)
    {
        shellprintf(" usage: setauth <user> <password>\r\n");
        return NM_INVALID_PARA;
    }

    memset(&auth, 0, sizeof(auth));
    memcpy(auth.user, argv[0], strlen(argv[0]));
    memcpy(auth.pwd, argv[1], strlen(argv[1]));

    ret = cfg_set_para(CFG_ITEM_LOC_APN_AUTH, &auth, sizeof(auth));

    if (ret != 0)
    {
        log_e(LOG_NM, "set wan apn failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     nm_shell_set_dcom
description:  set dcom on/off
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_shell_set_dcom(int argc, const char **argv)
{
    int ret;
    unsigned char dcom_enable;

    if (argc != 1)
    {
        shellprintf("usage:setdcom on/off\r\n");
        return  -1;
    }

    if (0 == strncmp("on", (char *)argv[0], 2))
    {
        dcom_enable = 1;
    }
    else if (0 == strncmp("off", (char *)argv[0], 3))
    {
        dcom_enable = 0;
    }
    else
    {
        log_e(LOG_NM, "parameter error");
        return -1;
    }

    ret = nm_set_dcom(dcom_enable);

    if (0 != ret)
    {
        shellprintf("setdcom failed\r\n");
        return ret;
    }

    shellprintf("setdcom ok\r\n");
    return 0;
}

static int nm_shell_dumpnm(int argc, const char **argv)
{
    int status = at_get_cfun_status();

    shellprintf("private nm status:%s\r\n",
                (nm_get_net_status_ex(NM_PRIVATE_NET) ? "connected" : "disconnected"));
    shellprintf("public nm status :%s\r\n",
                (nm_get_net_status_ex(NM_PUBLIC_NET) ? "connected" : "disconnected"));
    shellprintf("cfun status      :");

    if (0 == status)
    {
        shellprintf("0,Minimum functionality\r\n");
    }
    else if (1 == status)
    {
        shellprintf("1,Full functionality\r\n");
    }
    else if (4 == status)
    {
        shellprintf("4,Disable phone both transmit and receive RF circuits\r\n");
    }

    return 0;
}

static int nm_shell_setcfun(int argc, const const char **argv)
{
    uint32_t fun;

    if (argc != 1 || sscanf(argv[0], "%u", &fun) != 1)
    {
        shellprintf(" usage: nmsetcfun <functionNO:0/1/4>\r\n");
        return -1;
    }

    if ((fun != 0) && (fun != 1) && (fun != 4))
    {
        shellprintf(" error: functionNO error,usage:0/1/4\r\n");
        return -1;
    }

    at_set_cfun(fun);
    return 0;
}

static int nm_shell_recall(int argc, const const char **argv)
{
	int ret;
    TCOM_MSG_HEADER msghdr;
	
	msghdr.sender    = MPU_MID_NM;
    msghdr.receiver  = MPU_MID_NM;
    msghdr.msgid     = NM_MSG_ID_RECALL;
    msghdr.msglen    = 0;

    /* notify recreate the connection */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        shellprintf("tcom_send_msg failed, ret:%u\r\n", ret);
        return -1;
    }

    return 0;
}


/****************************************************************
function:     nm_shell_init
description:  initiaze wireless communcaiton device
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_shell_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* register shell cmd */
            shell_cmd_register("setauth",     nm_shell_set_auth,      "set local apn dial auth");
            shell_cmd_register("setlocalapn", nm_shell_set_local_apn, "set local apn(private apn)");
            shell_cmd_register("setwanapn",   nm_shell_set_wan_apn,   "set public apn");
            shell_cmd_register("setdcom",     nm_shell_set_dcom,      "set public net on/off");
            shell_cmd_register("dumpnm",      nm_shell_dumpnm,        "dump NM status");
            shell_cmd_register("setcfun",     nm_shell_setcfun,       "set NM cfun state");
			shell_cmd_register("recall",      nm_shell_recall,        "recall nm");
            break;

        default:
            break;
    }

    return 0;
}

