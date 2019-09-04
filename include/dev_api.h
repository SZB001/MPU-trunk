/****************************************************************
file:         dev_api.h
description:  the header file of upg api definition
date:         2017/7/21
author        wangqinglong
****************************************************************/
#ifndef __DEV_API_H__
#define __DEV_API_H__
#include <stdint.h>
#include "mid_def.h"
#include "init.h"
#include "tbox_limit.h"

#define  DEV_SYNC_INTERVAL          (1*1000)
#define  DEV_DIAG_INTERVAL          (1*1000)
#define  DEV_PRINT_VER_INTERVAL     (10*1000)
#define  DEV_DIAG_LOGDIR_INTERVAL   (1*1000)


/* message ID definition */
typedef enum  DEV_MSG_ID
{
    DEV_MSG_PRINT_VER_TIMER = MPU_MID_DEV,
    DEV_MSG_SYNC_TIMER,
    DEV_MSG_DIAG_TIMER,
    DEV_SCOM_MSG_MCU_CFG_SYN,
    DEV_SCOM_MSG_CUSTOM_PARA,
    DEV_SCOM_MSG_MCU_SHELL,
    DEV_SCOM_MSG_MCU_CFG_SET,
    DEV_SCOM_MSG_MCU_VER,
    DEV_SCOM_MSG_MCU_BTL_VER,
    DEV_MSG_DIAG_LOGDIR,
    DEV_MSG_MAX,
} DEV_MSG_ID;

/* pkg upgrade cmd type */
typedef enum  DEV_UPG_TYPE
{
    MCU_APP_BIN = 0,
    MCU_BTL_BIN,
    MCU_CFG_FILE,
    MCU_SHELL_FILE,
    MPU_APP_BIN,
    MPU_SHELL_FILE,
    MPU_S32K_BIN,
    MPU_FIRMWARE,
    DEV_UPG_NUM_MAX,
} DEV_UPG_TYPE;

/* errcode definition */
typedef enum DEV_ERROR_CODE
{
    DEV_INVALID_PARA = (MPU_MID_DEV << 16) | 0x01,
    DEV_FILE_OPEN_FAILED,
    DEV_FILE_NOT_EXIST,
    DEV_DIR_NOT_EXIST,
    DEV_FILE_READ_FAILED,
    DEV_DIR_READ_FAILED,
    DEV_DATA_CHECK_FAILED,
    DEV_UNCOMPRESS_FAILED,
    DEV_DATA_OVERFLOW,
    DEV_INVALID_TYPE,
    DEV_GET_DEV_INFO_FAILED,
    DEV_CHECK_ITEM_FAILED,
    DEV_INVALID_DEV_INFO,
    DEV_PARA_OVERFLOW,
    DEV_ST_TABLE_OVERFLOW,
    DEV_ST_UPDATE_PARA_FAILED,
    DEV_MCU_UPDATING,
    DEV_UPDATING,
    DEV_ST_UNKNOWN,
    DEV_SAME_VER,
    DEV_IN_EMERGENCY,
} DEV_ERROR_CODE;

typedef enum ST_DEF_ITEM_ID
{
    ST_ITEM_POW_VOLTAGE = 0,
    ST_ITEM_KL15_SIG,
    ST_ITEM_KL75_SIG,
    ST_ITEM_BCALL_SIG,
    ST_ITEM_ICALL_SIG,
    ST_ITEM_ECALL_SIG,
    ST_ITEM_SLOW_CHG_SIG,
    ST_ITEM_QUICK_CHG_SIG,
    ST_ITEM_BCALL,
    ST_ITEM_ICALL,
    ST_ITEM_ECALL,
    ST_ITEM_WAKEUP_SRC,
    ST_ITEM_PM_MODE,
    ST_ITEM_SRS_SIG,
    ST_ITEM_RBT_CNT,
    ST_ITEM_TSP_COMM,
    ST_ITEM_BAT_VOL,
    ST_ITEM_BAT_TEMP,
    ST_ITEM_BAT_RST,
    ST_ITEM_BAT_STATUS,
    ST_ITEM_APP_SLEEP,
    ST_ITEM_CAN1_STATUS,
    ST_ITEM_CAN2_STATUS,
    ST_ITEM_CAN3_STATUS,
    ST_ITEM_CAN4_STATUS,
    ST_ITEM_UPG_STATUS,
    ST_ITEM_ID_MAX,
} ST_DEF_ITEM_ID;

typedef enum DEV_UPG_STATUS
{
	DEV_UPG_IDLE = 0,     /* the tbox is not in upgrade status */
	DEV_UPG_BUSY,     /* the tbox is in upgrade status */	
}DEV_UPG_STATUS;


int upg_app_start(char *path);
int upg_fw_start(char *path);
int upg_set_status( DEV_UPG_STATUS status );

int upg_init_startup(void);
int upg_get_startup(char *status, unsigned int size);
int upg_set_startup(char *status, unsigned int size);

int upg_get_pkg_ver(unsigned char *ver, unsigned int size);
int upg_get_fw_ex_ver(char *ver, unsigned int size);
int upg_get_fw_ver(unsigned char *ver, unsigned int size);
int upg_get_app_ver(unsigned char *ver, unsigned int size);
int upg_get_mcu_upg_ver(unsigned char *ver, unsigned int size);
int upg_get_mcu_run_ver(unsigned char *ver, unsigned int size);
int upg_get_mcu_blt_ver(unsigned char *ver, unsigned int size);

int upg_is_mcu_exist(void);

int upg_proc_mcu_ver_msg(unsigned char *msg, unsigned int len);
int upg_proc_btl_ver_msg(unsigned char *msg, unsigned int len);

typedef int (*st_on_changed)(ST_DEF_ITEM_ID id, unsigned char *old_status,
                             unsigned char *new_status, unsigned int len);
int st_register(ST_DEF_ITEM_ID id, st_on_changed onchanged);
int st_get(ST_DEF_ITEM_ID id, unsigned char *data, unsigned int *len);
int st_set(ST_DEF_ITEM_ID id, unsigned char *data, unsigned int len);
static __inline uint16_t dev_get_power_voltage(void)
{
    uint16_t volt;
    int len = sizeof(volt);

    if (st_get(ST_ITEM_POW_VOLTAGE, (void *)&volt, (void *)&len) == 0)
    {
        return volt;
    }

    return 0;
}

static __inline uint8_t  dev_get_KL15_signal(void)
{
    uint8_t sign;
    int len = sizeof(sign);

    if (st_get(ST_ITEM_KL15_SIG, (void *)&sign, (void *)&len) == 0)
    {
        return sign;
    }

    return 0;
}

static __inline uint8_t  dev_get_KL75_signal(void)
{
    uint8_t sign;
    int len = sizeof(sign);

    if (st_get(ST_ITEM_KL75_SIG, (void *)&sign, (void *)&len) == 0)
    {
        return sign;
    }

    return 0;
}

static __inline uint8_t dev_get_slow_chg_signal(void)
{
    uint8_t sign;
    int len = sizeof(sign);

    if (st_get(ST_ITEM_SLOW_CHG_SIG, (void *)&sign, (void *)&len) == 0)
    {
        return sign;
    }

    return 0;
}

static __inline uint8_t dev_get_quick_chg_signal(void)
{
    uint8_t sign;
    int len = sizeof(sign);

    if (st_get(ST_ITEM_QUICK_CHG_SIG, (void *)&sign, (void *)&len) == 0)
    {
        return sign;
    }

    return 0;
}
uint8_t  dev_get_sos_signal(void);
static __inline uint16_t dev_get_wakeup_src(void)
{
    uint16_t src;
    int len = sizeof(src);

    if (st_get(ST_ITEM_WAKEUP_SRC, (void *)&src, (void *)&len) == 0)
    {
        return src;
    }

    return 0;
}

unsigned int dev_get_reboot_cnt(void);
void dev_print_softver_delay(void);
void dev_log_save_suspend(void);
void dev_log_save_resume(void);
char *dev_get_version(void);
void dev_sync_timeout(void);
extern int  dev_diag_get_emmc_status(void);
#endif

