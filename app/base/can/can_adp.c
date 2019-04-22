#include "scom_msg_def.h"
#include "scom_api.h"
#include "cfg_api.h"
#include "log.h"
#include <stdio.h>


static short auto_baud = 0;

void can_sync_autobaud_from_mcu(short *baud)
{
    int ret = 0;

    if (auto_baud != *baud)
    {
        auto_baud = *baud;
        ret = cfg_set_para(CFG_ITEM_CAN2_AUTO_BAUD, (void *)baud, 2);
    }

    if (ret == 0)
    {
        log_i(LOG_CAN, "autobaud = %d", auto_baud);
    }
    else
    {
        log_e(LOG_CAN, "CFG_ITEM_CAN2_AUTO_BAUD set error ");
    }

}

void can_auto_baud_rs(void)
{
    auto_baud = 0;

    if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_AUTO_BAUD, SCOM_TL_SINGLE_FRAME, 0,
                                (void *)&auto_baud, sizeof(auto_baud)))
    {
        log_e(LOG_CAN, "reset auto baud  error ");
    }
}

short can_get_auto_baud(void)
{
    return auto_baud;
}

int can2_init_read_autobaud(void)
{
    unsigned int length = 2;
    return cfg_get_para(CFG_ITEM_CAN2_AUTO_BAUD, (void *)&auto_baud, &length);
}

