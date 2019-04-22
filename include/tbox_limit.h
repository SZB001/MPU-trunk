/****************************************************************
file:         tbox_limit.h
description:  the header file of size limitation definition
date:         2017/04/18
author        yuzhimin
****************************************************************/
#ifndef __TBOX_LIMIT_H__
#define __TBOX_LIMIT_H__

#define IP_SIZE                        (16)
#define MAC_SIZE                       (32)
#define APN_SIZE                       (32)
#define URL_SIZE                       (64)
#define HOSTNAME_SIZE                  (64)

#define TBOX_TEL_LEN                   (16)
#define TBOX_IMEI_LEN                  (16)
#define TBOX_IMSI_LEN                  (16)
#define TBOX_FW_VER_LEN                (64)
#define APP_VER_LEN                    (64)
#define APP_BIN_LEN                    (32)

/* wifi */
#define WIFI_CLI_MAX                   (16)
#define WIFI_PASSWD_SIZE               (32)
#define WIFI_SSID_SIZE                 (32)
#define WIFI_VERSION_SIZE              (32)

/* call */
#define CALL_NUM_SIZE                  (32)
#define WHITELIST_MAX_SIZE             (512)

/* at */
#define CCID_LEN                       (21)
#define OP_NAME_LEN                    (64)
#define SMS_MSG_LEN                    (64)

/* can */
#define CAN_CHANNEL_NUM                (5)

#define VIN_CODE_LEN                   (17)
#define SORFT_FINGER_LEN               (9)
#define PROGRAME_DATA_LEN              (3)

#define SUPPLIER_CODE                 "M0824"
//#define MODEL_NUM                     "7F9T4G"
#define MODEL_NUM                     "YLA02"

#define LOW_MODEL_CODE                "H4794010001A0"
#define HIGH_MODEL_CODE               "H4794010002A0"

#define SYSTEM_NAME                   "Tbox"
#define TBOX_CODE                     "TBD"

#define HW_VERSION                    "H4HA0002180814"

#endif

