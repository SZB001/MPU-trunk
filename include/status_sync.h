/****************************************************************************
file:         status_sync.h
description:  the header file of status sync definition
date:         2017/7/9
copyright     Wuhan Intest Electronic Technology,Co.,Ltd
author        wangqinglong
*****************************************************************************/
#ifndef __STATUS_SYNC_H__
#define __STATUS_SYNC_H__

typedef struct STATUS_MCU
{
    unsigned short  voltage;
    unsigned char   slow_sign;
    unsigned char   KL15_sign;
    unsigned char   KL75_sign;
    unsigned char   quick_sign;
    unsigned short  wakeup_src;
    unsigned char   icall_sign;
    unsigned char   bcall_sign;
    unsigned char   ecall_sign;
    unsigned char   run_mode;
    unsigned char   srs_sign;
    unsigned int    reboot_cnt;
    unsigned short  bat_vol;
    int             bat_temp;
    unsigned int    bat_rst;
    unsigned char   bat_status;
    unsigned char   can_status[4];
} __attribute__((packed)) STATUS_MCU;

typedef struct STATUS_MPU
{
    unsigned char ecall;
    unsigned char bcall;
    unsigned char icall;
    unsigned char app_status;
	unsigned char upg_status;   /* upgrade status, 0 indicates not upgrading, 1 indicates upgrading */
} __attribute__((packed)) STATUS_MPU;

void st_sync_to_mcu(STATUS_MPU *status);
void st_sync_from_mcu(unsigned char *data, unsigned int len);

#endif


