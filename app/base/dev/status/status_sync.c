/****************************************************************************
 file:         status_sync.c
 description:  the header file of status sync implemention
 date:         2017/7/9
 copyright     Wuhan Intest Electronic Technology Co.,Ltd
 author        wangqinglong
 *****************************************************************************/
#include "com_app_def.h"
#include "status_sync.h"
#include "dev_api.h"
#include "at_api.h"

/*******************************************************************
 function:     st_sync_to_mcu
 description:  sync mpu status to mcu
 input:        none
 output:       STATUS_MPU *status
 return:       none
 ********************************************************************/
void st_sync_to_mcu(STATUS_MPU *status)
{
    int ret;
    unsigned int ilen = 0;
    unsigned char call_status = 0;

    memset(status, 0, sizeof(STATUS_MPU));

    ilen = sizeof(call_status);
    ret = st_get(ST_ITEM_ECALL, &call_status, &ilen);

    if (0 == ret)
    {
        status->ecall = at_is_call_busy(call_status);
    }

    ilen = sizeof(call_status);
    ret = st_get(ST_ITEM_BCALL, &call_status, &ilen);

    if (0 == ret)
    {
        status->bcall = at_is_call_busy(call_status);
    }

    ilen = sizeof(call_status);
    ret = st_get(ST_ITEM_ICALL, &call_status, &ilen);

    if (0 == ret)
    {
        status->icall = at_is_call_busy(call_status);
    }

    ilen = sizeof(status->app_status);
    st_get(ST_ITEM_APP_SLEEP, &(status->app_status), &ilen);

	ilen = sizeof(status->upg_status);
    st_get(ST_ITEM_UPG_STATUS, &(status->upg_status), &ilen);

    return;
}

/*******************************************************************
 function:     status_sync_mcu
 description:  sync the mcu status from data
 input:        none
 output:       STATUS_MCU *status
 return:       none
 ********************************************************************/
void st_sync_from_mcu(unsigned char *data, unsigned int len)
{
    STATUS_MCU *mcu_status;

    if ((NULL == data) || (sizeof(STATUS_MCU) != len))
    {
        log_e(LOG_DEV, "para error");
        return;
    }

    mcu_status = (STATUS_MCU *) data;

    log_i(LOG_DEV, "recv status sync from mcu, %u", mcu_status->reboot_cnt);

    st_set(ST_ITEM_POW_VOLTAGE, (unsigned char *) & (mcu_status->voltage),
           sizeof(mcu_status->voltage));
    st_set(ST_ITEM_SLOW_CHG_SIG, &(mcu_status->slow_sign), sizeof(mcu_status->slow_sign));
    st_set(ST_ITEM_KL15_SIG, &(mcu_status->KL15_sign), sizeof(mcu_status->KL15_sign));
    st_set(ST_ITEM_KL75_SIG, &(mcu_status->KL75_sign), sizeof(mcu_status->KL75_sign));
    st_set(ST_ITEM_QUICK_CHG_SIG, &(mcu_status->quick_sign), sizeof(mcu_status->quick_sign));
    st_set(ST_ITEM_WAKEUP_SRC, (unsigned char *) & (mcu_status->wakeup_src),
           sizeof(mcu_status->wakeup_src));
    st_set(ST_ITEM_ICALL_SIG, &(mcu_status->icall_sign), sizeof(mcu_status->icall_sign));
    st_set(ST_ITEM_BCALL_SIG, &(mcu_status->bcall_sign), sizeof(mcu_status->bcall_sign));
    st_set(ST_ITEM_ECALL_SIG, &(mcu_status->ecall_sign), sizeof(mcu_status->ecall_sign));
    st_set(ST_ITEM_PM_MODE, &(mcu_status->run_mode), sizeof(mcu_status->run_mode));
    st_set(ST_ITEM_SRS_SIG, &(mcu_status->srs_sign), sizeof(mcu_status->srs_sign));
    st_set(ST_ITEM_RBT_CNT, (unsigned char *) & (mcu_status->reboot_cnt),
           sizeof(mcu_status->reboot_cnt));

    st_set(ST_ITEM_BAT_VOL, (unsigned char *) & (mcu_status->bat_vol), sizeof(mcu_status->bat_vol));
    st_set(ST_ITEM_BAT_TEMP, (unsigned char *) & (mcu_status->bat_temp), sizeof(mcu_status->bat_temp));
    st_set(ST_ITEM_BAT_RST, (unsigned char *) & (mcu_status->bat_rst), sizeof(mcu_status->bat_rst));
    st_set(ST_ITEM_BAT_STATUS, (unsigned char *) & (mcu_status->bat_status),
           sizeof(mcu_status->bat_status));

    st_set(ST_ITEM_CAN1_STATUS, (unsigned char *) & (mcu_status->can_status[0]),
           sizeof(mcu_status->can_status[0]));
    st_set(ST_ITEM_CAN2_STATUS, (unsigned char *) & (mcu_status->can_status[1]),
           sizeof(mcu_status->can_status[1]));
    st_set(ST_ITEM_CAN3_STATUS, (unsigned char *) & (mcu_status->can_status[2]),
           sizeof(mcu_status->can_status[2]));
    st_set(ST_ITEM_CAN4_STATUS, (unsigned char *) & (mcu_status->can_status[3]),
           sizeof(mcu_status->can_status[3]));

    return;
}

