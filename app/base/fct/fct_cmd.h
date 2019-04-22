
/****************************************************************
file:         fct_cmd.h
description:  the header file of fct cmd definition
date:         2016/9/25
author        liuzhongwen
****************************************************************/

#ifndef __FCT_CMD_H__
#define __FCT_CMD_H__
#include "init.h"

#define FCT_CMD_LEN        (2)
#define FCT_CMD_TYPE_LEN   (1)


typedef int (*cmd_proc)(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen);

typedef enum FCT_CMD_ENUM
{
    FCT_CMD_NG_RES = 0X1000,
    FCT_CMD_EMMC,
    FCT_4G_ICCID,
    FCT_4G_SIGNAL,
    FCT_4G_NET,
    FCT_4G_EX_M_ANT,
    FCT_4G_EX_S_ANT,
    FCT_4G_IN_ANT,
    FCT_GPS,
    FCT_4G_ACT,
    FCT_4G_RING,
    FCT_4G_AUDIOLOOP,
    FCT_4G_CFUN,
    FCT_WIFI_SSID,
    FCT_BLE_NAME,
    FCT_WIFI_ENBALE,
    FCT_LOCAL_APN,
    FCT_WAN_APN,
    FCT_URL,
    FCT_PORT,
    FCT_SET_SN,
} FCT_CMD_ENUM;

typedef enum FCT_CMD_TYPE_ENUM
{
    FCT_CMD_REQ = 0,
    FCT_CMD_RES,
} FCT_CMD_TYPE_ENUM;

typedef enum FCT_CMD_RESULT_ENUM
{
    FCT_CMD_NG = 0,
    FCT_CMD_OK,
} FCT_CMD_RESULT_ENUM;


typedef struct
{
    unsigned short cmd;
    cmd_proc proc;
} FCT_CMD;

int  fct_cmd_init(INIT_PHASE phase);
void fct_cmd_timetout_proc(unsigned int msgid);
int fct_cmd_proc(unsigned char *msg, int len);


#endif
