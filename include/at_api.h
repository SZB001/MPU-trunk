/****************************************************************
file:         at_api.h
description:  the header file of network manager api definition
date:         2017/7/10
author        wangqinglong
****************************************************************/
#ifndef __AT_API_H__
#define __AT_API_H__

#include "mid_def.h"
#include "init.h"
#include "tbox_limit.h"

#define DEFAULT_SSID            "foton_tbox"
#define DEFAULT_PASSWORD        "12345678"

typedef enum AT_MSG_EVENT
{
    AT_MSG_INIT_EVENT = MPU_MID_AT,
    AT_MSG_POLL_EVENT,
    AT_MSG_SEND_EVENT,
    AT_MSG_SEND_RETRY_EVENT,
    AT_MSG_EVENT_MAX,
} AT_MSG_EVENT;

typedef enum AT_MSG_ID
{
    AT_MSG_ID_RING = AT_MSG_EVENT_MAX,
} AT_MSG_ID;



/* errcode definition */
typedef enum AT_ERROR_CODE
{
    AT_INVALID_PARA = (MPU_MID_AT << 16) | 0x01,
    AT_PARA_TOO_LONG,
    AT_OPEN_DEV_FAILED,
    AT_GET_DEV_ATTR_FAILED,
    AT_STATUS_INVALID,
    AT_CREATE_THREAD_FAILED,
    AT_SEND_MSG_FAILED,
    AT_TABLE_OVERFLOW,
} AT_ERROR_CODE;

typedef enum FOTA_ST
{
    FOTA_IDLE = 0,
    FOTA_ING = 1,
    FOTA_OK = 2,
    FOTA_ERROR = 3,
} FOTA_ST;

int wifi_enable(void);
int wifi_disable(void);
int wifi_set_ssid(const char *ssid);
int wifi_get_ssid(char *ssid);
int wifi_set_key(const char *key);
int wifi_get_key(char *key);
int wifi_set_max_user(unsigned char num);
int wifi_get_max_user(unsigned char *num);
int wifi_get_ap_mac(char *mac);
int wifi_get_sta(char *data,  unsigned int *length);

void fota_set_status(FOTA_ST status);
unsigned char fota_get_status(void);


/* get at status */
int at_get_status(void);

/* get wifi status */
int at_wifi_get_status(void);

/* get sim card status */
int at_get_sim_status(void);

/* get 4G firmware version */
int at_get_ati(char *ver, unsigned int size);

extern int at_is_call_busy(unsigned char status);

void at_set_audioloop(unsigned int enable);
void at_set_audioloopGAIN(unsigned int gain);
void at_query_audioloop(void);
void at_query_audioloopGAIN(void);
void at_query_cfun(void);
void at_set_cfun(unsigned int enable);
void at_set_wifi(unsigned char op);
int at_get_audioloop(void);
int at_get_gain(void);
void disconnectcall(void);
void answercall(void);
void makecall(char *num);
int at_get_call_status(void);
int at_get_wifi_status(void);
void at_disable_gps(void);
void at_enable_gps(void);

#endif
