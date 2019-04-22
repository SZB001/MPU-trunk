#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "dev_api.h"
#include "at.h"
#include "at_api.h"
#include "hu.h"

static int hu_status = 0;


static int hu_send_10h(hu_pack_t *pack)
{
    if (sizeof(pack->dat) < 22)
    {
        return HU_ERR_DENIED;
    }

    pack->len = 0;
    pack->dat[pack->len++] = at_get_net_type();
    pack->dat[pack->len++] = 1;
    strncpy((char *)pack->dat + pack->len, "00000", 5);
    pack->len += 5;
    strncpy((char *)pack->dat + pack->len, "000000000000000", 15);
    pack->len += 15;

    return 0;
}

static int hu_send_40h(hu_pack_t *pack)
{
    if (sizeof(pack->dat) < 1)
    {
        return HU_ERR_DENIED;
    }

    pack->len = 1;
    pack->dat[0] = at_get_signal();

    return 0;
}

static int hu_send_51h(hu_pack_t *pack)
{
    pack->len = 0;
    return 0;
}

static int hu_recv_51h(hu_pack_t *pack)
{
    if (pack->len != 1)
    {
        return HU_ERR_CHECKSUM;
    }

    hu_status = pack->dat[0];
    return 0;
}

static int hu_recv_70h(hu_pack_t *pack)
{
    if (pack->len != 22)
    {
        return HU_ERR_CHECKSUM;
    }

    return 0;
}

static int hu_send_80h(hu_pack_t *pack)
{
    return HU_ERR_DENIED;
}

static int hu_send_90h(hu_pack_t *pack)
{
    if (sizeof(pack->dat) < 75)
    {
        return HU_ERR_DENIED;
    }

    upg_get_app_ver(pack->dat, 20);
    upg_get_mcu_run_ver(pack->dat + 20, 20);
    upg_get_fw_ver(pack->dat + 40, 20);

    pack->len = 75;
    return 0;
}

static int hu_send_a0h(hu_pack_t *pack)
{
    if (sizeof(pack->dat) < 8)
    {
        return HU_ERR_DENIED;
    }

    memset(pack->dat, 0, 8);
    pack->len = 8;
    return 0;
}

static int hu_send_b0h(hu_pack_t *pack)
{
    if (sizeof(pack->dat) < 2)
    {
        return HU_ERR_DENIED;
    }

    pack->dat[0] = at_get_sim_status() == 1;
    pack->dat[1] = 0;//nm_dial_status();
    pack->len = 2;
    return 0;
}

static hu_cmd_t hu_cmd_10h =
{
    .cmd    = HU_CMD_STATION_INFO,
    .uptype = HU_CMD_TYPE_PERIOD,
    .period = 5000,
    .send_proc = hu_send_10h,
};

static hu_cmd_t hu_cmd_40h =
{
    .cmd    = HU_CMD_SIGNAL_LEVEL,
    .uptype = HU_CMD_TYPE_PERIOD,
    .period = 5000,
    .send_proc = hu_send_40h,
};


static hu_cmd_t hu_cmd_51h =
{
    .cmd     = HU_CMD_HU_STATUS,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_send_51h,
    .recv_proc = hu_recv_51h,
};

static hu_cmd_t hu_cmd_70h =
{
    .cmd     = HU_CMD_GPS_INFO,
    .dwtype  = HU_CMD_TYPE_PERIOD,
    .recv_proc = hu_recv_70h,
};

static hu_cmd_t hu_cmd_80h =
{
    .cmd     = HU_CMD_UPG_STATUS,
    .uptype  = HU_CMD_TYPE_NEEDACK,
    .timeout = 500,
    .send_proc = hu_send_80h,
};

static hu_cmd_t hu_cmd_90h =
{
    .cmd     = HU_CMD_TBOX_INFO,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_send_90h,
};

static hu_cmd_t hu_cmd_a0h =
{
    .cmd    = HU_CMD_DATA_FLOW,
    .uptype = HU_CMD_TYPE_PERIOD,
    .period = 2000,
    .send_proc = hu_send_a0h,
};

static hu_cmd_t hu_cmd_b0h =
{
    .cmd     = HU_CMD_NET_STATUS,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_send_b0h,
};

int hu_common_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= hu_register_cmd(&hu_cmd_10h);
            ret |= hu_register_cmd(&hu_cmd_40h);
            ret |= hu_register_cmd(&hu_cmd_51h);
            ret |= hu_register_cmd(&hu_cmd_70h);
            ret |= hu_register_cmd(&hu_cmd_80h);
            ret |= hu_register_cmd(&hu_cmd_90h);
            ret |= hu_register_cmd(&hu_cmd_a0h);
            ret |= hu_register_cmd(&hu_cmd_b0h);
            break;
    }

    return ret;
}
