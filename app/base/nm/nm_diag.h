/****************************************************************
file:         nm_diag.h
description:  the header file of network diagnose function definition
date:         2018/06/30
author        liuzhongwen
****************************************************************/
#ifndef __NM_DIAG_H__
#define __NM_DIAG_H__

#include "mid_def.h"
#include "init.h"
#include "tcom_api.h"
#include "nm_api.h"

#define NM_MAX_OTA_DIAG_TIMES              3



//#define NM_MAX_DIAG_TIMES                3
#define NM_MAX_DIAG_TIMES                  12 //Liubinkui changed for T3R1,20190703

#if 1
    #define NM_MAX_OTA_LINK_DIAG_INTERVAL      (5*60*1000)     /* unit is ms, 5min   */
    //#define NM_MAX_DIAL_LINK_DIAG_INTERVAL     (120*60*1000)   /* unit is ms, 120min */
    #define NM_MAX_DIAL_LINK_DIAG_INTERVAL     (10*60*1000)   /* unit is ms, 10min,Liubinkui changed for T3R1,20190703 */
#else
    #define NM_MAX_OTA_LINK_DIAG_INTERVAL      (1*60*1000)     /* unit is ms, 1min for test only  */
    #define NM_MAX_DIAL_LINK_DIAG_INTERVAL     (1*60*1000)     /* unit is ms, 1min for test only */
#endif


#define NM_MAX_REG_OTA_TBL                 6

typedef struct NM_REG_OTA_TBL
{
    unsigned char     used_num;
    nm_ota_status_get get[NM_MAX_REG_OTA_TBL];
} NM_REG_OTA_TBL;

typedef enum
{
    NM_NET_NORMAL = 0 ,        /* the network is normal  */
    NM_NET_OTA_LINK_FAULT,     /* all ota link is fault  */
    NM_NET_DIAL_LINK_FAULT,    /* all dial link is fault */
} NET_STATUS_TYPE;

typedef struct
{
    NET_STATUS_TYPE     status;          /* the current network status */
    unsigned long long
    start_time;      /* if network is normal, it is not used, otherwise it is the starttime of the fault omit */
    unsigned int        diag_times;      /* diagnose times */
} NM_DIAG_CTL;

int  nm_diag_init(INIT_PHASE phase);
void nm_diag_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody);

#endif

