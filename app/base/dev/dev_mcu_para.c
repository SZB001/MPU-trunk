#include "com_app_def.h"
#include "dev_mcu_para.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "scom_api.h"

static CUSTOM_PARA_T mcu_para;

/****************************************************************
 * function:     dev_send_para_to_mcu
 * description:  send the custom paramaters to mcu
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_send_custom_para_to_mcu(unsigned int index, const char *msg, int len)
{
    int ret = -1;
    unsigned char buf[64];

    if ((NULL == msg) || (len > sizeof(buf) - 1))
    {
        log_e(LOG_DEV, "invalid parameter,len:%d", len);
        return DEV_INVALID_PARA;
    }

    if ((index >= CUSTOM_ID_MAX) || (index < 0))
    {
        log_e(LOG_DEV, "invalid parameter,index:%d", index);
        return DEV_INVALID_PARA;
    }

    switch (index)
    {
        case CUSTOM_ID_VIN:
            if (len == VIN_BUF_LEN)
            {
                buf[0] = CUSTOM_ID_VIN;
                memcpy(&buf[1], msg, len);
                memcpy(mcu_para.vin, msg, sizeof(mcu_para.vin));
                ret = 0;
            }

            break;

        case CUSTOM_ID_TBOX_VER:
            if (len == SOFT_VER_LEN)
            {
                buf[0] = CUSTOM_ID_TBOX_VER;
                memcpy(&buf[1], msg, len);
                memcpy(mcu_para.ver, msg, sizeof(mcu_para.ver));
                ret = 0;
            }

            break;

        default:
            ret = DEV_INVALID_PARA;
            break;
    }

    if (ret == 0)
    {
        log_buf_dump(LOG_DEV, "send", buf, len + 1);
        ret = scom_tl_send_frame(SCOM_TL_CMD_CUSTOM_PARA, SCOM_TL_SINGLE_FRAME, 0, buf, len + 1);
    }

    return ret;
}

/****************************************************************
 * function:     scom_mcu_cfg_sync
 * description:  get the config about mcu
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_check_custom_para_from_mcu(unsigned int index, unsigned char *msg, int len)
{
    int ret = 0;

    if ((NULL == msg) || (0 >= len))
    {
        log_e(LOG_DEV, "invalid parameter,len:%d", len);
        return DEV_INVALID_PARA;
    }

    if ((index >= CUSTOM_ID_MAX) || (index < 0))
    {
        log_e(LOG_DEV, "invalid parameter,index:%d", index);
        return DEV_INVALID_PARA;
    }

    log_i(LOG_DEV, "recv index:%d", index);
    log_buf_dump(LOG_DEV, "recv", msg, len);

    switch (index)
    {
        case CUSTOM_ID_VIN:
            ret = memcmp((void *)mcu_para.vin, (void *)msg, len);
            break;

        case CUSTOM_ID_TBOX_VER:
            ret = memcmp((void *)mcu_para.ver, (void *)msg, len);
            break;

        default:
            break;
    }

    if (ret != 0)
    {
        log_e(LOG_DEV, "content not match,index:%d", index);
    }

    return ret;
}

