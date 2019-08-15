/****************************************************************
file:         wsrv_api.h
description:  the header file of tbox web server api definition
date:         2018/08/08
author        chenyin
****************************************************************/

#ifndef __WSRV_API_H__
#define __WSRV_API_H__

#include "mid_def.h"
#include "init.h"
#include <netinet/in.h>    // for sockaddr_in

#define WEB_SERVER_PORT                 20003
#define WSRV_MAX_CLIENT_NUM             16
#define WSRV_MAX_BUFF_SIZE              1024
#define WSRV_RECREATE_INTERVAL          5000 // 5S
#define WSRV_NO_ACK_TIMEOUT             60000 // if gmobi da app no feeddog last 60S, restart da app

typedef enum
{
    WSRV_INIT_FAILED = (MPU_MID_WSRV << 16) | 0x01,
    // TODO

    WSRV_SOCKET_FAILED,
    WSRV_CREATE_TIMER_FAILED,
} WSRV_ERROR_CODE;

typedef enum
{
    TIMER_WSRV_RECREATE = MPU_MID_WSRV,
    TIMER_WSRV_RESTART_DA,
    // TODO

} WSRV_MSG_ID;

typedef struct WSRV_CLIENT
{
    int fd;
    unsigned char req_buf[WSRV_MAX_BUFF_SIZE]; // client request buff
    unsigned int  req_len;
} WSRV_CLIENT;

/* initiaze web server module */
int wsrv_init(INIT_PHASE phase);
/* startup web server module */
int wsrv_run(void);

#endif

