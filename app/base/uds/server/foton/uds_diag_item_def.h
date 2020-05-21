/****************************************************************
file:         uds_diag_item_def.h
description:  the source file of uds diagnose item definition
date:         2016/9/26
author        liuzhongwen
****************************************************************/
#ifndef UDS_DIAG_ITEM_DEF_H
#define UDS_DIAG_ITEM_DEF_H

typedef enum DIAG_DEF_ITEM_ID
{
    DTC_NUM_ECALL_SWITCH = 0,
    DTC_NUM_GPS_ANTENNA_SHORT_TO_GND,
    DTC_NUM_GPS_ANTENNA_OPEN,
    DTC_NUM_GPS_MODULE_FAULT,
    DTC_NUM_WAN_ANTENNA_SHORT_TO_GND,
    DTC_NUM_WAN_ANTENNA_OPEN,
    DTC_NUM_GSM_MODULE,
    DTC_NUM_SIM_FAULT,
    DTC_NUM_BATTERY_TOO_LOW,
    DTC_NUM_BATTERY_TOO_HIGH,
    DTC_NUM_BATTERY_AGED,
    DTC_NUM_POWER_VOLTAGE_HIGH,
    DTC_NUM_POWER_VOLTAGE_LOW,
    DTC_NUM_BUSOFF,
    DTC_NUM_MISSING_ACU,
    DTC_NUM_MISSING_BMS,
    DTC_NUM_MISSING_CDU,
    DTC_NUM_MISSING_MCU,
    DTC_NUM_MISSING_VCU1,
    DTC_NUM_MISSING_EPS,
    DTC_NUM_MISSING_ESC,
    //DTC_NUM_MISSING_EHB,
    DTC_NUM_MISSING_EACP,
    DTC_NUM_MISSING_PTC,
    //DTC_NUM_MISSING_PLG,
    DTC_NUM_MISSING_CLM,
    DTC_NUM_MISSING_BDCM,
    //DTC_NUM_MISSING_ALM,
    DTC_NUM_MISSING_ICU,
    DTC_NUM_MISSING_IHU,
    //DTC_NUM_MISSING_TAP,
    DIAG_ITEM_NUM,
} DIAG_DEF_ITEM_ID;

#endif

