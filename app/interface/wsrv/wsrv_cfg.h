/****************************************************************
file:         wsrv_cfg.h
description:  the header file of tbox web server configuration
date:         2020/5/20
author        chengxingyu
****************************************************************/

#ifndef __WSRV_CFG_H__
#define __WSRV_CFG_H__

#define WEB_SERVER_PORT                 20003
#define WSRV_MAX_CLIENT_NUM             16
#define WSRV_RECREATE_INTERVAL          5000    /*Unit: ms*/
#define WSRV_NO_ACK_TIMEOUT             60000   /*Unit: ms*/

#define WSRV_MAX_BUFF_SIZE              1024

/* See Hozon email(2020-6-15 10:49) for detail. */
#define WSRV_F18C_SN_LEN_MAX            22
#define WSRV_F1C0_SW_LEN_MAX            8
#define WSRV_F180_BL_LEN_MAX            8
#define WSRV_F191_HW_LEN_MAX            5
#define WSRV_F187_PN_LEN_MAX            13
#define WSRV_F18A_SP_LEN_MAX            5

#endif

