/****************************************************************
 file:         ubx_cfg.h
 description:  ublox m8n configuration header
 date:         2018/04/10
 author        yuzhimin
 ****************************************************************/

#define UBX_CFG_INIT_TIMEOUT    (500)

typedef enum
{
    UBX_CFG_RATE_1HZ = 0,
    UBX_CFG_NAV5,
    UBX_CFG_MSG_GSV_OFF,
    UBX_CFG_MSG_GSA_OFF,
    UBX_CFG_MSG_GLL_OFF,
    UBX_CFG_MSG_VTG_OFF,
    UBX_CFG_RST,
    UBX_CFG_MAX,
} UBX_CFG_ID;

typedef struct
{
    UBX_CFG_ID id;
    unsigned char *cmd;
    unsigned int len;
} UBX_CMD_TBL;

extern UBX_CMD_TBL ubx_tbl[];
int ubx_cmd_test(int argc, const char **argv);

