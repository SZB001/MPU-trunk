/****************************************************************
 file:         at_packing.c
 description:  the source file of gprs status implementation
 date:         2017/02/19
 author        yuzhimin
 ****************************************************************/
#include <stdio.h>
#include <string.h>
#include "tbox_limit.h"
#include "at_queue.h"
#include "at_stat.h"
#include "at_cmd.h"
#include "com_app_def.h"
#include "at_data.h"

/******************************************************************
 function:     at_atd
 description:  pack ATD cmd
 input:        AT_CMD_T* at,
 unsigned char* num
 output:       none
 return:       none
 ******************************************************************/
void at_atd(AT_CMD_T *at, char *num)
{
    /* ATD10086; */
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s%s;\r\n", at_map[ATD].name, num);
    at->id = ATD;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_ata
 description:  pack ATA cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_ata(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[ATA].name);
    at->id = ATA;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_chup
 description:  pack CHUP cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_chup(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_CHUP].name);
    at->id = AT_CHUP;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_cops
 description:  pack COPS cmd
 input:        AT_CMD_T* at,
 unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_cops(AT_CMD_T *at, unsigned char op)
{
    switch (op)
    {
        case 0: /* auto */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=0\r\n", at_map[AT_COPS].name);
            break;

        case 1: /* 2G */
            if (at_info.op_num > 0)
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,2,\"%d\",0\r\n", at_map[AT_COPS].name,at_info.op_num);
            else
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=,,,0\r\n", at_map[AT_COPS].name);
            break;

        case 2: /* 3G  */
            if (at_info.op_num > 0)
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,2,\"%d\",2\r\n", at_map[AT_COPS].name,at_info.op_num);
            else
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=,,,2\r\n", at_map[AT_COPS].name);
            break;

        case 3: /* 4G */
            if (at_info.op_num > 0)
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,2,\"%d\",7\r\n", at_map[AT_COPS].name,at_info.op_num);
            else
                snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=,,,7\r\n", at_map[AT_COPS].name);
            break;

        default: /* query network type */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_COPS].name);
            break;
    }

    at->id = AT_COPS;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_csq
 description:  pack CSQ cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_csq(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_CSQ].name);
    at->id = AT_CSQ;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_clcc
 description:  pack CLCC cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_clcc(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_CLCC].name);
    at->id = AT_CLCC;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_cmgd
 description:  pack CMGD cmd
 input:        AT_CMD_T* at,
 unsigned int index
 output:       none
 return:       none
 ******************************************************************/
void at_cmgd(AT_CMD_T *at, unsigned int index)
{
    /* AT+CMGD=1 */
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=%u\r\n", at_map[AT_CMGD].name, index);
    at->id = AT_CMGD;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qccid
 description:  pack get ccid cmd
 input:        AT_CMD_T* at
 output:       none
 return:       none
 ******************************************************************/
void at_qccid(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_QCCID].name);
    at->id = AT_QCCID;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qgps
 description:  pack open gps cmd
 input:        AT_CMD_T* at,
 unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_qgps(AT_CMD_T *at, unsigned char op)
{
    if (op)
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "AT+QGPS=1,30,50,0,1\r\n");
    }
    else
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "AT+QGPS?\r\n");
    }

    at->id = AT_QGPS;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwifi
 description:  pack close gps cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qgpsend(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_QGPSEND].name);
    at->id = AT_QGPSEND;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwifi
 description:  pack set wifi on/off cmd
 input:        AT_CMD_T* at,
 unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_qwifi(AT_CMD_T *at, unsigned char op)
{
    switch (op)
    {
        case 0: /* wifi off */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=0\r\n", at_map[AT_QWIFI].name);
            break;

        case 1: /* wifi on */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,0,0\r\n", at_map[AT_QWIFI].name);
            break;

        default: /* query wifi status */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWIFI].name);
            break;
    }

    at->id = AT_QWIFI;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_ata
 description:  pack get wifi status cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qwstatus(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWSTATUS].name);
    at->id = AT_QWSTATUS;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwssid
 description:  pack the set wifi ssid cmd
 input:        AT_CMD_T* at,
 unsigned char *ssid
 output:       none
 return:       none
 ******************************************************************/
void at_qwssid(AT_CMD_T *at, char *ssid)
{
    if (NULL == ssid)
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWSSID].name);

    }
    else
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=%s\r\n", at_map[AT_QWSSID].name, ssid);
    }

    at->id = AT_QWSSID;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwauth
 description:  pack the set wifi password cmd
 input:        AT_CMD_T* at,
 unsigned char *passwd
 output:       none
 return:       none
 ******************************************************************/
void at_qwauth(AT_CMD_T *at, char *passwd)
{
    if (NULL == passwd)
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWAUTH].name);
    }
    else
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=5,4,\"%s\"\r\n", at_map[AT_QWAUTH].name,
                 passwd);
    }

    at->id = AT_QWAUTH;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwsetmac
 description:  pack set wifi mac cmd
 input:        AT_CMD_T* at,
 unsigned char *mac
 output:       none
 return:       none
 ******************************************************************/
void at_qwsetmac(AT_CMD_T *at, char *mac)
{
    if (NULL == mac)
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWSETMAC].name);
    }
    else
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"%s\"\r\n", at_map[AT_QWSETMAC].name,
                 mac);
    }

    at->id = AT_QWSETMAC;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwmaxsta
 description:  pack ATA cmd
 input:        AT_CMD_T* at,
 unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_qwmaxsta(AT_CMD_T *at, unsigned char op)
{
    if (op > WIFI_CLI_MAX)
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWMAXSTA].name);
    }
    else
    {
        snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=%u\r\n", at_map[AT_QWMAXSTA].name, op);
    }

    at->id = AT_QWMAXSTA;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwrstd
 description:  pack AT cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qwrstd(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s\r\n", at_map[AT_QWRSTD].name);
    at->id = AT_QWRSTD;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qwstainfo
 description:  pack get wifi station information cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qwstainfo(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QWSTAINFO].name);
    at->id = AT_QWSTAINFO;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_qaudloop
 description:  pack set/get audio loop
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qaudloop(AT_CMD_T *at, unsigned int *op)
{
    if (NULL == op)
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QAUDLOOP].name);
    }
    else
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s=%u\r\n", at_map[AT_QAUDLOOP].name, *op);
    }

    at->id  = AT_QAUDLOOP;
    at->len = strlen((const char *)at->str);
}

/******************************************************************
 function:     at_qaudloop
 description:  pack set/get audio gain
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qsidet(AT_CMD_T *at, unsigned int *op)
{
    if (NULL == op)
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_QSIDET].name);
    }
    else
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s=%u\r\n", at_map[AT_QSIDET].name, *op);
    }

    at->id  = AT_QSIDET;
    at->len = strlen((const char *)at->str);
}

/******************************************************************
 function:     at_qaudloop
 description:  pack set/get phone functionality
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_cfun(AT_CMD_T *at, unsigned int *op)
{
    if (NULL == op)
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s?\r\n", at_map[AT_CFUN].name);
    }
    else
    {
        snprintf((char *)at->str, AT_CMD_SIZE_MAX, "%s=%u\r\n", at_map[AT_CFUN].name, *op);
    }

    at->id  = AT_CFUN;
    at->len = strlen((const char *)at->str);
}

/******************************************************************
 function:     at_qfotal
 description:  pack AT cmd
 input:        AT_CMD_T* at,
 output:       none
 return:       none
 ******************************************************************/
void at_qfotal(AT_CMD_T *at)
{
    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"%s\"\r\n", at_map[AT_QFOTA].name,
             COM_APP_UPG_FW_DIR"/"COM_FW_UPDATE);
    at->id = AT_QFOTA;
    at->len = strlen(at->str);
}


/******************************************************************
 function:     at_qoos
 description:  pack set qoos mode
 input:        AT_CMD_T* at,
 unsigned char op
 output:       none
 return:       none
 ******************************************************************/
void at_qoos(AT_CMD_T *at, unsigned char op)
{
    switch (op)
    {
        case 0: /* qoos off */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"qoos\",0\r\n", at_map[AT_QCFG].name);
            break;

        case 1: /* qoos on */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"qoos\",1,120,900,30,5,0,0,5,1\r\n", at_map[AT_QCFG].name);
            break;

        case 2: /* qoos on */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"qoos\",2,120,900,30,5,0,0,5,1\r\n", at_map[AT_QCFG].name);
            break;

        default: /* query qoos status */
            snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=\"qoos\"\r\n", at_map[AT_QCFG].name);
            break;
    }

    at->id = AT_QCFG;
    at->len = strlen(at->str);
}

/******************************************************************
 function:     at_cid
 description:  pack set cid
 input:        AT_CMD_T* at
 output:       none
 return:       none
 ******************************************************************/
void at_cid(AT_CMD_T *at)
{
    //snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,%s\r\n", at_map[AT_CGDCONT].name, at_info.cid_info);

    snprintf((char *) at->str, AT_CMD_SIZE_MAX, "%s=1,\"IPV4V6\",\"3gnet\",\"0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0\",0,0,0,0\r\n", at_map[AT_CGDCONT].name);

    at->id = AT_CGDCONT;
    at->len = strlen(at->str);
}



