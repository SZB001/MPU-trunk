/****************************************************************************
file:         ucom_msg.h
description:  the header file of ucom message en/decode definition
date:         2017/7/9
copyright     Wuhan Intest Electronic Technology,Co.,Ltd
author        wangqinglong
*****************************************************************************/

#ifndef __FAULT_SYNC_H__
#define __FAULT_SYNC_H__

#include "tbox_limit.h"

/*ant type definition */
typedef enum  ANT_TYPE
{
    ANT_4G_MAIN  =  0,
    ANT_4G_VICE,
    ANT_GNSS,
    ANT_MAX,
} ANT_TYPE;

/* GNSS Ant status */
enum
{
    ANT_UNKNOW = 0,
    ANT_OK,
    ANT_SHORT,
    ANT_OPEN,
};

typedef enum  DEV_FLT
{
    GPS_ANT = 0,
    CAN_NODE1,
    CAN_NODE2,
    CAN_NODE3,
    CAN_NODE4,
    CAN_NODE5,
    CAN_BUS1,
    CAN_BUS2,
    CAN_BUS3,
    CAN_BUS4,
    CAN_BUS5,
    POWER,
    BAT,                        /*BAT AGE*/
    MIC,
    MICSTATUS,      /*following 3, added by Cindy*/
    SOSBTN,
    SPK,
    GSENSE,
    BATVOL,
    GPS,
    EMMC,
    GPRS,
    SIM,
    WIFI,
    USB,
} DEV_FLT;


#pragma pack(1)
typedef struct DIAG_MPU_FAULT
{
    unsigned char gps;
    unsigned char emmc;
    unsigned char gprs;
    unsigned char sim;
    unsigned char wifi;
    unsigned char usb;
} DIAG_MPU_FAULT;
#pragma pack(0)

#pragma pack(1)
typedef struct DIAG_MCU_FAULT
{
    unsigned char gps_ant;
    unsigned char can_node[CAN_CHANNEL_NUM];
    unsigned char can_busoff[CAN_CHANNEL_NUM];
    unsigned char voltage;      /*power high or low*/
    unsigned char battery;      /*battery age*/
    unsigned char mic;          /*mic voltage*/
    unsigned char micstatus;    /*20180416 added by Cindy*/
    unsigned char sosbtn;       /*sos or ecall*/
    unsigned char spkstatus;    /*spk open/short*/
    unsigned char gsensestatus; /*gsense communication*/
    unsigned char batteryvol;   /*batteryvol low/high*/
} DIAG_MCU_FAULT;
#pragma pack(0)


int  flt_sync_init(void);

int flt_get_by_id(DEV_FLT id);

void flt_get_mcu(DIAG_MCU_FAULT *fault);
void flt_get_mpu(DIAG_MPU_FAULT *fault);

void flt_sync_to_mcu(DIAG_MPU_FAULT *fault);
void flt_sync_from_mcu(unsigned char *data, unsigned int len);

#endif




