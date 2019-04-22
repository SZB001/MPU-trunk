/****************************************************************
 file:         ubx_cfg.c
 description:  ublox m8n configuration implemention
 date:         2018/04/10
 author        yuzhimin
 ****************************************************************/
#include "log.h"
#include "shell_api.h"
#include "gps_api.h"
#include "gps_dev.h"
#include "ubx_cfg.h"

unsigned char ubx_cfg_rate_1hz[] =
{
    /* 250ms 4HZ*/
    /*0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xFA, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x96,*/
    /* 1000ms 1HZ */
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x39,
};
unsigned char ubx_cfg_nav5[] =
{
    0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x13, 0x77,
};

unsigned char ubx_cfg_dis_gsv[] =
{
    /*dis GSV*/
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x38,
};

unsigned char ubx_cfg_dis_gsa[] =
{
    /*dis GSA*/
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x31,
};

unsigned char ubx_cfg_dis_gll[] =
{
    /* dis GLL */
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A,
};

unsigned char ubx_cfg_dis_vtg[] =
{
    /* dis VTG */
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x46,
};

unsigned char ubx_cfg_rst[] =
{
    /* cold start, hardware reset*/
    0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0xB9, 0x00, 0x00, 0xC6, 0x8B,
};

UBX_CMD_TBL ubx_tbl[UBX_CFG_MAX] =
{
    /*cmd id*/              /*cmd content*/             /*cmd length*/
    {UBX_CFG_RATE_1HZ,      ubx_cfg_rate_1hz,           sizeof(ubx_cfg_rate_1hz)},
    {UBX_CFG_NAV5,          ubx_cfg_nav5,               sizeof(ubx_cfg_nav5)},
    {UBX_CFG_MSG_GSV_OFF,   ubx_cfg_dis_gsv,            sizeof(ubx_cfg_dis_gsv)},
    {UBX_CFG_MSG_GSA_OFF,   ubx_cfg_dis_gsa,            sizeof(ubx_cfg_dis_gsa)},
    {UBX_CFG_MSG_GLL_OFF,   ubx_cfg_dis_gll,            sizeof(ubx_cfg_dis_gll)},
    {UBX_CFG_MSG_VTG_OFF,   ubx_cfg_dis_vtg,            sizeof(ubx_cfg_dis_vtg)},
    {UBX_CFG_RST,           ubx_cfg_rst,                sizeof(ubx_cfg_rst)},
};

int ubx_cmd_test(int argc, const char **argv)
{
    unsigned int id = 0;

    if (argc != 1)
    {
        shellprintf(" usage: ubx <id> \r\n");
        return -1;
    }

    if (GNSS_EXTERNAL != GNSS_TYPE)
    {
        shellprintf(" gnss on 4G module,not support ubx cmd!\r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &id);

    if (id < UBX_CFG_MAX)
    {
        log_buf_dump(LOG_GPS, "ubx_cfg", ubx_tbl[id].cmd, ubx_tbl[id].len);
        gps_dev_send(ubx_tbl[id].cmd, ubx_tbl[id].len);
    }
    else
    {
        shellprintf(" Unsupported command id:%u\r\n", id);
    }

    return 0;
}

