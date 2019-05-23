
/****************************************************************
file:         cfg_api.h
description:  the header file of configuration manager api definition
date:         2016/9/25
author        liuzhongwen
****************************************************************/

#ifndef __CFG_API_H__
#define __CFG_API_H__

#include "mid_def.h"
#include "init.h"
#include <stdbool.h>

#define CFG_ITEM_INVALID    0xffffffff

typedef enum CFG_PARA_ITEM_ID
{
    /* General Configuration */
    CFG_ITEM_SN_NUM = 0,
    CFG_ITEM_DEV_NUM,
    CFG_ITEM_SLEEP_MODE,
    CFG_ITEM_LOCAL_APN,
    CFG_ITEM_LOC_APN_AUTH,
    CFG_ITEM_WAN_APN,
    CFG_ITEM_WAN_APN_AUTH,
    CFG_ITEM_DBC_PATH,
    CFG_ITEM_WIFI_SET,
    CFG_ITEM_DCOM_SET,
    CFG_ITEM_NET_TYPE,
    CFG_ITEM_WIFI_MAXASSOC,
    CFG_ITEM_WIFI_SSID,
    CFG_ITEM_WIFI_KEY,
    CFG_ITEM_ICALL,
    CFG_ITEM_BCALL,
	 CFG_ITEM_ECALL,
    CFG_ITEM_WHITE_LIST,
    CFG_ITEM_SLEEP_TIME,
    CFG_ITEM_DSLEEP_TIME,
    CFG_ITEM_VTD,
    CFG_ITEM_ACC_THS,
    CFG_ITEM_ICALL_F,
    CFG_ITEM_BCALL_F,
    CFG_ITEM_ECALL_F,
    CFG_ITEM_CAN_DEFAULT_BAUD_0,
    CFG_ITEM_CAN_DEFAULT_BAUD_1,
    CFG_ITEM_CAN_DEFAULT_BAUD_2,
    CFG_ITEM_CAN_DEFAULT_BAUD_3,
    CFG_ITEM_CAN_DEFAULT_BAUD_4,
    CFG_ITEM_LOG_ENABLE,
    CFG_ITEM_RTC_WAKEUP_TIME,
    CFG_ITEM_BAT_TYPE,

    /* GB32960 Configuration */
    CFG_ITEM_GB32960_VIN,
    CFG_ITEM_GB32960_URL,
    CFG_ITEM_GB32960_PORT,
    CFG_ITEM_GB32960_INTERVAL,
    CFG_ITEM_GB32960_REGINTV,
    CFG_ITEM_GB32960_TIMEOUT,
    CFG_ITEM_GB32960_REGSEQ,

    /* Data save Configuration */
    CFG_ITEM_DSU_AUTHKEY,
    CFG_ITEM_DSU_CANLOG_TIME,
    CFG_ITEM_DSU_CANLOG_MODE,
    CFG_ITEM_DSU_SDHZ,
    CFG_ITEM_DSU_HOURFILE,
    CFG_ITEM_DSU_LOOPFILE,

    /* FOTON Configuration */
    CFG_ITEM_FOTON_VIN,
    CFG_ITEM_FOTON_URL,
    CFG_ITEM_FOTON_PORT,
    CFG_ITEM_FOTON_INTERVAL,
    CFG_ITEM_FOTON_REGINTV,
    CFG_ITEM_FOTON_TIMEOUT,

    /* EOL Configuration */
    CFG_ITEM_FT_UDS_REPAIR_SHOP_CODE,   /* 4S shop code */
    CFG_ITEM_FT_UDS_AVN_SERIAL_NUMBER,  /* Reserved */
    CFG_ITEM_FT_UDS_VEHICLE_TYPE,       /* Vehicle type */

    CFG_ITEM_CAN2_AUTO_BAUD,            /*CAN2 ADP auto baud*/
    CFG_ITEM_FT_UDS_POWER,
    
    CFG_ITEM_FUELCELL,
	CFG_ITEM_PART_NUM,
	
    CFG_ITEM_FT_UDS_HW,
    CFG_ITEM_INTEST_HW,

	CFG_ITEM_FT_HBINTV,
    CFG_ITEM_FT_REGISTER,

    CFG_ITEM_FT_TID,
    CFG_ITEM_FT_SIM,
    CFG_ITEM_FT_DTN,
    CFG_ITEM_FT_DEV_TYPE,
    CFG_ITEM_FT_DEV_SN,
    CFG_ITEM_FT_TYPE,
    CFG_ITEM_FT_PORT,

   	CFG_ITEM_FTTSP_VIN,
    CFG_ITEM_FTTSP_URL,
    CFG_ITEM_FTTSP_PORT,
    CFG_ITEM_FTTSP_INTERVAL,
    CFG_ITEM_FTTSP_TUKEY,

	/* HOZON TSP */
	//CFG_ITEM_HOZON_TSP_CFGVER,


    CFG_ITEM_ID_MAX,
} CFG_PARA_ITEM_ID;

typedef enum CFG_PARA_ERROR_CODE
{
    CFG_INVALID_PARAMETER = (MPU_MID_CFG << 16) | 0x01,
    CFG_TABLE_OVERFLOW,
    CFG_TABLE_UPDATE_PARA_FAILED,
    CFG_CREATE_THREAD_FAILED,
    CFG_CHECK_CFG_TABLE_FAILED,
    CFG_PARA_OVERFLOW,
    CFG_FN_NOT_FOUND,
    CFG_INVALID_STRING,
    CFG_OPEN_FAILED,
    CFG_STAT_FAILED,
    CFG_INVALID_DATA,
} CFG_PARA_ERROR_CODE;

typedef int (*on_changed)(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                          unsigned char *new_para, unsigned int len);
/* initiaze configuration manager module*/
int cfg_init(INIT_PHASE phase);

/* starup configuration manager module*/
int cfg_run(void);

/* register configuration changed function */
int cfg_register(CFG_PARA_ITEM_ID id, on_changed onchanged);

/* get para value */
int cfg_get_para(CFG_PARA_ITEM_ID id, void *data, unsigned int *len);

/* set para value */
int cfg_set_para(CFG_PARA_ITEM_ID id, void *data, unsigned int len);

/* get para restore staus */
bool cfg_get_para_status(void);

/* set para value immediatly */
int cfg_set_para_im(CFG_PARA_ITEM_ID id, unsigned char *data, unsigned int len);

#endif
