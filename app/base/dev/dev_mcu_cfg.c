#include "com_app_def.h"
#include "dev_mcu_cfg.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "can_api.h"
#include "scom_api.h"

MCU_CFG_ITEM mcu_cfg_info[] =
{
    {MCU_CFG_ID_SYSMODE, CFG_ITEM_SLEEP_MODE,   sizeof(char),   S_OFFSET(MCU_PARA_T, sys_mode),},
    {MCU_CFG_ID_BAT_TYPE, CFG_ITEM_BAT_TYPE,    sizeof(char),   S_OFFSET(MCU_PARA_T, bat_type),},
    {MCU_CFG_ID_VTD,     CFG_ITEM_VTD,          sizeof(char),   S_OFFSET(MCU_PARA_T, anti_thief_enable),},
    {MCU_CFG_ID_ECALL,   CFG_ITEM_ECALL_F,      sizeof(char),   S_OFFSET(MCU_PARA_T, ecall_enbale),},
    {MCU_CFG_ID_BCALL,   CFG_ITEM_BCALL_F,      sizeof(char),   S_OFFSET(MCU_PARA_T, bcall_enbale),},
    {MCU_CFG_ID_ICALL,   CFG_ITEM_ICALL_F,      sizeof(char),   S_OFFSET(MCU_PARA_T, icall_enbale),},
    {MCU_CFG_ID_STIME,   CFG_ITEM_SLEEP_TIME,   sizeof(int),    S_OFFSET(MCU_PARA_T, sleep_time),},
    {MCU_CFG_ID_DSTIME,  CFG_ITEM_DSLEEP_TIME,  sizeof(int),    S_OFFSET(MCU_PARA_T, deep_sleep_time)},
    {MCU_CFG_ID_ACCTHS,  CFG_ITEM_ACC_THS,      sizeof(short),  S_OFFSET(MCU_PARA_T, acc_vecm_ths),},
    {MCU_CFG_ID_SN,      CFG_ITEM_SN_NUM,       sizeof(int),    S_OFFSET(MCU_CFG_T,  sn),},
};


/****************************************************************
 * function:     dev_set_cfg
 * description:  set the config about mcu
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_set_from_mcu(unsigned char *msg, int len)
{
    short id;
    int ret;

    if ((NULL == msg) || (len < MCU_CFG_ID_LEN))
    {
        log_e(LOG_DEV, "invalid parameter,len:%d", len);
        return DEV_INVALID_PARA;
    }

    memcpy(&id, msg, MCU_CFG_ID_LEN);

    if ((id >= MCU_CFG_ID_MAX) || (id < 0))
    {
        log_e(LOG_DEV, "invalid parameter, id:%d", id);
        return DEV_INVALID_PARA;
    }

    if ((len - MCU_CFG_ID_LEN) != mcu_cfg_info[id].ilen)
    {
        log_e(LOG_DEV, "invalid length,id:%u, ilen:%u, len:%u", id, mcu_cfg_info[id].ilen, len);
        return DEV_INVALID_PARA;
    }

    ret = cfg_set_para(mcu_cfg_info[id].alias_id, &msg[MCU_CFG_ID_LEN], mcu_cfg_info[id].ilen);

    if (0 != ret)
    {
        shellprintf(" set (%d) failed\r\n", mcu_cfg_info[id].alias_id);
    }

    return ret;
}

/****************************************************************
 * function:     dev_set_mcu_cfg
 * description:  set the config about mcu
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_set_from_mpu(MCU_CFG_ID id, void *data, unsigned int len)
{
    int ret, pos = 0;
    unsigned char buff[6];

    if ((id >= MCU_CFG_ID_MAX) || (NULL == data))
    {
        log_e(LOG_DEV, "invalid parameter");
        return DEV_INVALID_PARA;
    }

    if (len != mcu_cfg_info[id].ilen)
    {
        log_e(LOG_DEV, "invalid length,id:%u, ilen:%u, len:%u", id, mcu_cfg_info[id].ilen, len);
        return DEV_INVALID_PARA;
    }

    ret = cfg_set_para(mcu_cfg_info[id].alias_id, data, len);

    if (0 != ret)
    {
        shellprintf(" set (%d) failed\r\n", mcu_cfg_info[id].alias_id);
        return ret;
    }

    pos = 0;
    memcpy(&buff[pos], &id, MCU_CFG_ID_LEN);
    pos += MCU_CFG_ID_LEN;
    memcpy(&buff[pos], data, len);
    pos += len;
    ret = scom_tl_send_frame(SCOM_TL_CMD_PARA_CFG, SCOM_TL_SINGLE_FRAME, 0, buff, pos);

    if (0 != ret)
    {
        log_e(LOG_DEV, " %d set failed\r\n", id);
        return ret;
    }

    return 0;
}

/****************************************************************
 * function:     scom_mcu_cfg_sync
 * description:  get the config about mcu
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
void scom_mcu_cfg_sync(unsigned char *msg, unsigned int len)
{
    int i, ret, offset, pos = 0;
    unsigned int ilen;
    static unsigned char temp[16];
    static unsigned char buff[32];

    if (len != sizeof(MCU_CFG_T))
    {
        log_e(LOG_DEV, "mcu config sync failed, len:%d", len);
        return ;
    }

    for (i = 0; i < MCU_CFG_ID_MAX; i++)
    {
        ilen   = mcu_cfg_info[i].ilen;
        offset = mcu_cfg_info[i].offset;

        if (CFG_ITEM_INVALID == mcu_cfg_info[i].alias_id)
        {
            continue;
        }

        ret = cfg_get_para(mcu_cfg_info[i].alias_id, temp, &ilen);

        if (0 != ret)
        {
            log_e(LOG_DEV, "get mcu cfg failed:id: %u, ret:0x%08x", i, ret);
            continue;
        }

        if (0 != memcmp(&msg[offset], temp, ilen))
        {
            log_o(LOG_DEV, " %d is changed\r\n", i);
            pos = 0;
            memcpy(&buff[pos], &i, MCU_CFG_ID_LEN);
            pos += MCU_CFG_ID_LEN;
            memcpy(&buff[pos], temp, ilen);
            pos += ilen;
            scom_tl_send_frame(SCOM_TL_CMD_PARA_CFG, SCOM_TL_SINGLE_FRAME, 0, buff, pos);
        }
    }
}

