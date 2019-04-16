/****************************************************************
file:         nm_api.h
description:  the header file of network manager api definition
date:         2017/7/03
author        wangqinglong
****************************************************************/
#ifndef __NM_API_H__
#define __NM_API_H__

#include "mid_def.h"
#include "init.h"
#include <netdb.h>
#include <net/if.h>
#include <stdbool.h>

/* nm timer definition */
#define NM_RECALL_INTERVAL   (5000)
#define NM_DIAG_INTERVAL     (30000)   /* 30s */

/* define net type */
typedef enum NET_TYPE
{
    NM_PRIVATE_NET = 0 ,     /* private network */
    NM_PUBLIC_NET  ,         /* public network  */
    NM_NET_TYPE_NUM,
} NET_TYPE;

/* message ID definition */
typedef enum  NM_MSG_ID
{
    NM_MSG_ID_URL_CHANGED = MPU_MID_NM,
    NM_MSG_ID_PORT_CHANGED,
    NM_MSG_ID_LOCAL_APN_CHANGED,
    NM_MSG_ID_WAN_APN_CHANGED,
    NM_MSG_ID_LOC_AUTH_CHANGED,
    NM_MSG_ID_DCOM_CHANGED,
    NM_MSG_ID_COM_AVAILABLE,
    NM_MSG_ID_DIAL_INITED,
    NM_MSG_ID_DIAL_STATUS,
    NM_MSG_ID_TIMER_RECALL,
    NM_MSG_ID_DIAG_TIMER,
    NM_MSG_ID_MAX,
} NM_MSG_ID;

/* errcode definition */
typedef enum NM_ERROR_CODE
{
    NM_INVALID_PARA = (MPU_MID_NM << 16) | 0x01,
    NM_STATUS_INVALID,
    NM_INVALID_ADDR_TYPE,
    NM_INVALID_ADDR,
    NM_APN_INVALID,
    NM_CREATE_THREAD_FAILED,
    NM_REG_URL_CALLBACK_FAILED,
    NM_SEND_MSG_FAILED,
    NM_GET_URL_FAILED,
    NM_TABLE_OVERFLOW,
    NM_REG_SMS_FAILED,
} NM_ERROR_CODE;

typedef enum
{
    NM_OTA_LINK_FAULT = 0,
    NM_OTA_LINK_NORMAL,
} NM_OTA_LINK_STATE;

int nm_init(INIT_PHASE phase);
int nm_run(void);

typedef enum NM_STATE_MSG
{
    NM_REG_MSG_CONNECTED = 0,
    NM_REG_MSG_DISCONNECTED,
} NM_STATE_MSG;

typedef int (*nm_status_changed)(NET_TYPE type, NM_STATE_MSG status);
typedef int (*nm_ota_status_get)(void);
typedef int (*nm_sms_arrived)(unsigned char *msg, unsigned int len);

typedef struct NM_DIAL_INFO
{
    struct in_addr ip_addr;
    struct ifreq interface;
} NM_DIAL_INFO;


int  nm_get_dcom(unsigned char *state);
int  nm_set_dcom(unsigned char action);
int  nm_network_switch(unsigned char type);
int  nm_get_signal(void);
int  nm_get_iccid(char *iccid);
int  nm_get_net_type(void);
int  nm_get_operator(unsigned char *operator);
int  nm_bind(int sockfd, NET_TYPE type);
bool nm_get_net_status(NET_TYPE type);
bool nm_net_is_apn_valid( NET_TYPE type );
int  nm_reg_status_changed(NET_TYPE type, nm_status_changed callback);
int  nm_register_get_ota_status(nm_ota_status_get callback);


#endif



