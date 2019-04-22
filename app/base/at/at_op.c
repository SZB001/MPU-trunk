/****************************************************************
 file:         at_op.c
 description:  the source file of gprs call operation implementation
 date:         2017/07/17
 author        wangqinglong
 ****************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "at.h"
#include "at_queue.h"
#include "at_packing.h"
#include "at_task.h"
#include "at_stat.h"
#include "at_cmd.h"
#include "at_data.h"

/******************************************************************
 function:     makecall
 description:  use AT cmd to make call
 input:        char *num
 output:       none
 return:       none
 ******************************************************************/
void makecall(char *num)
{
    assert(num);
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_atd(&at, num);
    at_set_call_status(2);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     answercall
 description:  use AT cmd to answer call
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void answercall(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_ata(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     disconnectcall
 description:  use AT cmd to disconnect call
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void disconnectcall(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_chup(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_net
 description:  0x00:auto, 0x01:2G, 0x02 3G, 0x03 4G
 input:        unsigned char type
 output:       none
 return:       none
 ******************************************************************/
void at_set_net(unsigned char type)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_cops(&at, type);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_disable_gps
 description:  close GPS
 input:        unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_disable_gps(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qgpsend(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_enable_gps
 description:  open GPS
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_enable_gps(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qgps(&at, 1);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_query_gps
 description:  query GPS status
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_query_gps(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qgps(&at, 0);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_wifi
 description:  use AT cmd to set wifi on/off
 input:        unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_set_wifi(unsigned char op)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwifi(&at, op);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_wifi_ssid
 description:  use AT cmd to set wifi ssid
 input:        char *ssid
 output:       none
 return:       none
 ******************************************************************/
void at_set_wifi_ssid(char *ssid)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwssid(&at, ssid);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_wifi_key
 description:  use AT cmd to set wifi key
 input:        char *key
 output:       none
 return:       none
 ******************************************************************/
void at_set_wifi_key(char *key)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwauth(&at, key);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_wifi_maxassoc
 description:  use AT cmd to set wifi maxassoc
 input:        unsigned char num
 output:       none
 return:       none
 ******************************************************************/
void at_set_wifi_maxassoc(unsigned char num)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwmaxsta(&at, num);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_wifi_recover_cfg
 description:  recover wifi default config
 input:        unsigned int index
 output:       none
 return:       none
 ******************************************************************/
void at_wifi_recover_cfg(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwrstd(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_wifi_mac
 description:  use AT cmd to set wifi mac
 input:        char *mac
 output:       none
 return:       none
 ******************************************************************/
void at_set_wifi_mac(char *mac)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qwsetmac(&at, mac);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_sms_del
 description:  deleta short message by index
 input:        unsigned int index
 output:       none
 return:       none
 ******************************************************************/
void at_sms_del(unsigned int index)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_cmgd(&at, index);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_audioloop
 description:  set audio loop
 input:        unsigned int enable
 output:       none
 return:       none
 ******************************************************************/
void at_set_audioloop(unsigned int enable)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qaudloop(&at, &enable);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_audioloopGAIN
 description:  set audio loop
 input:        unsigned int gain
 output:       none
 return:       none
 ******************************************************************/
void at_set_audioloopGAIN(unsigned int gain)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qsidet(&at, &gain);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_cfun
 description:  set cfun
 input:        0 indicates Minimum functionality;
               1 indicates Full functionality (Default)
               4 Disable phone both transmit and receive RF circuits
 output:       none
 return:       none
 ******************************************************************/
void at_set_cfun(unsigned int enable)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_cfun(&at, &enable);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_query_audioloop
 description:  get audio loop
 input:        unsigned int enable
 output:       none
 return:       none
 ******************************************************************/
void at_query_audioloop(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qaudloop(&at, NULL);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_query_cfun
 description:  get phone function
 input:        unsigned int gain
 output:       none
 return:       none
 ******************************************************************/
void at_query_cfun(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_cfun(&at, NULL);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_get_audioloopGAIN
 description:  get audio loop
 input:        unsigned int gain
 output:       none
 return:       none
 ******************************************************************/
void at_query_audioloopGAIN(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qsidet(&at, NULL);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_fota_update
 description:  set audio loop
 input:        unsigned int gain
 output:       none
 return:       none
 ******************************************************************/
void at_fota_update(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qfotal(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_test
 description:  send test at command
 input:        unsigned char *atcmdtest
 output:       none
 return:       none
 ******************************************************************/
void at_test(const char *atcmdtest)
{
    assert(atcmdtest);
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at.id = AT_TEST;
    snprintf((char *)at.str, AT_CMD_SIZE_MAX, "%s\r\n", atcmdtest);
    at.len = strlen((const char *)at.str);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_qoos
 description:  set qoos mode
 input:        unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_set_qoos(unsigned char op)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_qoos(&at, op);
    at_send_enable(&at, AT_HIGH_PRI);
}

/******************************************************************
 function:     at_set_cid
 description:  set cid 1
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_set_cid(void)
{
    AT_CMD_T at;
    memset(&at, 0, sizeof(AT_CMD_T));
    at_cid(&at);
    at_send_enable(&at, AT_HIGH_PRI);
}


