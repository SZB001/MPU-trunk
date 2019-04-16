/****************************************************************************
file:         scom_tl.h
description:  the header file of spi transport layer definition
date:         2017/7/9
copyright     Wuhan Intest Electronic Technology Co.,Ltd
author        liuzhongwen
*****************************************************************************/
#ifndef __SCOM_TL_H__
#define __SCOM_TL_H__

#include "init.h"
#include "scom_msg_def.h"

//#define SCOM_TL_FRAME_LEN       (480)
#define SCOM_TL_FRAME_LEN       (242)
#define SCOM_TL_RESEND_INTERVAL (500)

enum SCOM_TL_STATE
{
    SCOM_TL_STATE_DISCONNECTED = 0,
    SCOM_TL_STATE_CONNECTING,
    SCOM_TL_STATE_IDLE,
    SCOM_TL_STATE_SERVER_STARTED,
    SCOM_TL_STATE_CLIENT_STARTED,
};

enum SCOM_TL_FRAME_TYPE
{
    SCOM_TL_SINGLE_FRAME = 0,
    SCOM_TL_START_FRAME,
    SCOM_TL_MIDDLE_FRAME,
    SCOM_TL_LAST_FRAME,
};

enum SCOM_TL_RES_RESULT
{
    SCOM_TL_RET_OK = 0,
    SCOM_TL_RET_READ_FAILED,
    SCOM_TL_RET_SERVER_START_FAILED,
    SCOM_TL_RET_SERVER_NOT_START,
    SCOM_TL_RET_DATA_UNAVAILABLE,
    SCOM_TL_RET_INVALID_FNO,
};

typedef struct SCOM_TL_MSG_HDR
{
    unsigned char  msg_type;
    unsigned char  ftype;
    unsigned short fno;
    unsigned short flen;
} SCOM_TL_MSG_HDR;

typedef struct SCOM_TL_MSG
{
    SCOM_TL_MSG_HDR hdr;
    unsigned char data[1];
} SCOM_TL_MSG;

typedef struct SCOM_TL_DATA_REQ_MSG
{
    unsigned short fno;
    unsigned short fcnt;
} SCOM_TL_DATA_REQ_MSG;

typedef struct SCOM_TL_NG_RES_MSG
{
    unsigned short result;
    unsigned short reason;
} SCOM_TL_NG_RES_MSG;

typedef struct SCOM_TL_RX_CTL
{
    unsigned int  used;
    unsigned int  rx_len;
    unsigned int  rx_ftype;
    unsigned int  rx_flen;
    int           rx_fno;

    unsigned int  req_fno;
    unsigned int  req_fcnt;
} SCOM_TL_RX_CTL;

typedef int (*scom_tx_fun)(unsigned char *buf, unsigned int len);
typedef int (*scom_msg_proc_fun)(unsigned char *msg, unsigned int len);

int  scom_tl_init(INIT_PHASE phase);
void scom_tl_reg_tx_fun(scom_tx_fun tx_fun);
int  scom_tl_reg_proc_fun(SCOM_TL_CHAN chn, scom_msg_proc_fun proc_fun);
void scom_tl_msg_proc(unsigned char *msg, unsigned int len);
void scom_tl_timeout_proc();
void scom_tl_send_con_req(void);
int  scom_tl_send_msg(unsigned char msg_type, unsigned char *data, unsigned int len);
void scom_tl_stop_server(void);
int  scom_tl_start_server(unsigned char *data, unsigned int len);
#endif

