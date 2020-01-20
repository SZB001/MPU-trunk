/****************************************************************
file:         uds_shell.c
description:  the source file of nm shell cmd implementation
date:         2017/07/09
author        wangqinglong
****************************************************************/
#include "com_app_def.h"
#include "shell_api.h"
#include "uds_proxy.h"
#include "uds_define.h"
#include "log.h"
#include "server/uds_diag.h"
#include "J1939.h"
#include <stdlib.h>
#include "cfg_api.h"
#include "can_api.h"
#include "uds_shell_vendor.h"
#include "uds_did.h"
/****************************************************************
function:     uds_shell_set_uds
description:  set uds server/client
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int uds_shell_set_tl(int argc, const char **argv)
{
    if (argc != 1)
    {
        log_e(LOG_UDS, "usage:setuds server/client");
        return  -1;
    }

    if (0 == strncmp("server", (char *)argv[0], 6))
    {
        uds_set_server();
    }
    else if (0 == strncmp("client", (char *)argv[0], 6))
    {
        uds_set_client();
    }
    else
    {
        log_e(LOG_UDS, "parameter error");
        return -1;
    }

    shellprintf("set uds mode ok\r\n");
    return 0;
}

/****************************************************************
function:     uds_dtc_hex2dec
description:  convert dtc from character to long
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static uint32_t uds_dtc_hex2dec(uint8_t *dtcchar)
{
    uint8_t tempbuf[20];
    uint32_t result;

    memset(tempbuf, 0x00, sizeof(tempbuf));
    strcat((char *)tempbuf, "0x");
    strcat((char *)tempbuf, (const char *)dtcchar);
    result = strtoul((const char *)tempbuf, 0, 16);

    return result;

}

/****************************************************************
function:     uds_shell_dump_uds
description:  dump uds information
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int uds_shell_dump_uds(int argc, const char **argv)
{
    int i, j, ret;
    unsigned char pre_char[4] = { 'P', 'C', 'B', 'U' };
    unsigned int  did_num, len, dtc = 0;
    unsigned char dtc_high;
    unsigned char dtc_type;
    unsigned char freeze[256];

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        dtc      = uds_dtc_hex2dec(uds_diag_get_dtc(i)) & 0x3FFFFF;
        dtc_high = (uds_dtc_hex2dec(uds_diag_get_dtc(i)) & 0xC00000) >> 22;

        if (dtc_high >= 4)
        {
            dtc_type = '?';
        }
        else
        {
            dtc_type =  pre_char[dtc_high];
        }

        if (uds_diag_is_cfm_dtc_valid(i))
        {
            shellprintf(" DTC %s(confirm): %c%06X\r\n", uds_diag_get_dtc_name(i), dtc_type, dtc);
        }

        if (uds_diag_is_uncfm_dtc_valid(i))
        {
            shellprintf(" DTC %s(current): %c%06X\r\n", uds_diag_get_dtc_name(i), dtc_type, dtc);
        }
    }

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        did_num = 0;
        len = sizeof(freeze);
        memset(freeze, 0, sizeof(freeze));
        ret = uds_diag_get_freeze(i, freeze, &len, &did_num);

        if ((ret == 0) && (len != 0) && (did_num != 0))
        {
            shellprintf("DTC: %s, comfirm:%d, counter:%d, did_num:%d\r\n", uds_diag_get_dtc(i),
                        uds_diag_is_cfm_dtc_valid(i), uds_diag_is_uncfm_dtc_valid(i), did_num);

            for (j = 0; j < len; j++)
            {
                printf("%02x ", freeze[j]);
            }

            printf("\r\n");
        }
    }

    shellprintf("dump uds ok\r\n");
    return 0;
}

/****************************************************************
function:     uds_shell_cleardt
description:  dump uds information
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int uds_shell_cleardt(int argc, const char **argv)
{
    uds_diag_dtc_clear();
    shellprintf("clear uds fault ok\r\n");
    return 0;
}

static int uds_shell_set_cfg(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: udssetcfg <value>\r\n");
        return -1;
    }
    int cfg_value = 0;
    sscanf((char *) argv[0], "%d", &cfg_value);
    return set_cfg_code((unsigned char)cfg_value);
}

static int uds_shell_get_cfg_test(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: udsgetcftest <cfg type>\r\n");
        return -1;
    }
    int ret = 0;
    int cfg_type = 0;
    sscanf((char *) argv[0], "%d", &cfg_type);
    switch(cfg_type)
    {
        case 1:
        {
            unsigned char ble_ctl = get_ble_ctl();
            log_o(LOG_UDS, "ble_ctl:%d", ble_ctl);
            break;
        }
        case 2:
        {
            unsigned char ota_ctl = get_ota_ctl();
            log_o(LOG_UDS, "ota_ctl:%d", ota_ctl);
            break;
        }
        case 3:
        {
            unsigned char factory_mode = get_factory_mode();
            log_o(LOG_UDS, "factory_mode:%d", factory_mode);
            break;
        }
        default:
            log_e(LOG_UDS, "cfg type error,cfg_type:%d", cfg_type);
            ret = -1;
            break;
    }
    return ret;
}


static int uds_shell_set_cfg_test(int argc, const char **argv)
{
    if (argc != 2)
    {
        shellprintf(" usage: udssetcftest <cfg type> <value>\r\n");
        return -1;
    }
    int cfg_type = 0;
    int cfg_value = 0;
    sscanf((char *) argv[0], "%d", &cfg_type);
    sscanf((char *) argv[1], "%d", &cfg_value);
    int ret = 0;
    switch(cfg_type)
    {
        case 1:
        {
            set_ble_ctl((unsigned char)cfg_value);
            if(ret == 0)
            {
                log_o(LOG_UDS, "set bluetooth ctrl success!");
            }
            else
            {
                log_e(LOG_UDS, "set bluetooth ctrl failed, ret:%d", ret);
            }
            break;
        }
        case 2:
        {
            set_ota_ctl((unsigned char)cfg_value);
            if(ret == 0)
            {
                log_o(LOG_UDS, "set OTA ctrl success!");
            }
            else
            {
                log_e(LOG_UDS, "set OTA ctrl failed, ret:%d", ret);
            }
            break;
        }
        case 3:
        {
            set_factory_mode((unsigned char)cfg_value);
            if(ret == 0)
            {
                log_o(LOG_UDS, "set factory mode success!");
            }
            else
            {
                log_e(LOG_UDS, "set factory mode failed, ret:%d", ret);
            }
            break;
        }
        default:
            log_e(LOG_UDS, "cfg type error,cfg_type:%d", cfg_type);
            ret = -1;
            break;
    }
    return ret;
}

/****************************************************************
function:     uds_shell_init
description:  initiaze wireless communcaiton device
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_shell_init(INIT_PHASE phase)
{
	int ret;
	
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* register shell cmd */
            shell_cmd_register("setuds",     uds_shell_set_tl,           "set uds server/client)");
            shell_cmd_register("dumpuds",    uds_shell_dump_uds,         "dump uds diag information");
            shell_cmd_register("cleardt",    uds_shell_cleardt,          "clear uds diag fault information)");
            shell_cmd_register("dumpvf",     J1939_shell_dump_vhflt,     "dump fault of current generate");
            shell_cmd_register("udssetcfg",  uds_shell_set_cfg,     "uds set cfg code");
            shell_cmd_register("udsgetcfgtest",  uds_shell_get_cfg_test,     "uds get cfg test");
            shell_cmd_register("udssetcfgtest",  uds_shell_set_cfg_test,     "uds set cfg code");
            break;

        default:
            break;
    }

	ret = uds_shell_vendor_init( phase );

    return ret;
}



