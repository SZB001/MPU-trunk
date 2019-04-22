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
    FT_ITEM_DIAG_ECALL_SWITCH = 0,
    FT_ITEM_DIAG_GPS_ANTENNA_SHORT_TO_GND,
    FT_ITEM_DIAG_GPS_ANTENNA_SHORT_TO_BAT,
    FT_ITEM_DIAG_GPS_ANTENNA_OPEN,
    FT_ITEM_DIAG_GPS_MODULE_FAULT,
    FT_ITEM_DIAG_WAN_ANTENNA_SHORT_TO_GND,
    FT_ITEM_DIAG_WAN_ANTENNA_SHORT_TO_BAT,
    FT_ITEM_DIAG_WAN_ANTENNA_OPEN,
    FT_ITEM_DIAG_GSM_MODULE,
    FT_ITEM_DIAG_MIC_OUTPUT_SHORT_TO_GROUND,
    FT_ITEM_DIAG_SIM_FAULT,
    FT_ITEM_DIAG_BATTERY_TOO_LOW,
    FT_ITEM_DIAG_BATTERY_TOO_HIGH,
    FT_ITEM_DIAG_BATTERY_AGED,
    FT_ITEM_DIAG_GSENS_MODULE,
    FT_ITEM_DIAG_POWER_VOLTAGE_HIGH,
    FT_ITEM_DIAG_POWER_VOLTAGE_LOW,
    DIAG_ITEM_NUM,
} DIAG_DEF_ITEM_ID;

#endif

