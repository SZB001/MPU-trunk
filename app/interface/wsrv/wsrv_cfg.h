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

#endif

