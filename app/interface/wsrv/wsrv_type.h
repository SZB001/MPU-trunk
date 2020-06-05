/****************************************************************
file:         wsrv_type.h
description:  the header file of tbox web server data type
date:         2020/5/20
author        chengxingyu
****************************************************************/

#ifndef __WSRV_TYPE_H__
#define __WSRV_TYPE_H__

typedef struct WSRV_CLIENT
{
    int fd;
    unsigned char req_buf[WSRV_MAX_BUFF_SIZE]; // client request buff
    unsigned int  req_len;
} WSRV_CLIENT;

#endif

