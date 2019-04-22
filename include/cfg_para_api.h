
/****************************************************************
file:         cfg_para_api.h
description:  the header file of para data structure definition
date:         2016/9/29
author        liuzhongwen
****************************************************************/

#ifndef __CFG_PARA_API_H__
#define __CFG_PARA_API_H__

#pragma pack(1)

typedef struct CFG_COMM_INTERVAL
{
    unsigned short  data_val;   /* unit is 200ms */
    unsigned short   hb_val;     /* unit is 200ms */
} CFG_COMM_INTERVAL;

typedef struct CFG_COMM_URL
{
    unsigned char URL[64];
    unsigned short port;
} CFG_COMM_URL;

typedef struct CFG_QSEND_INTERVAL
{
    unsigned short qsend_val;  /* unit is 1ms */
    unsigned short total_time;
} CFG_QSEND_INTERVAL;

typedef struct CFG_DIAL_AUTH
{
    char user[32];
    char pwd[32];
} CFG_DIAL_AUTH;

#pragma pack(0)

#endif
