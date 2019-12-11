#ifndef __DEV_MCU_CFG_H__
#define __DEV_MCU_CFG_H__

#include "cfg_api.h"
#include "tbox_limit.h"

#define S_OFFSET(struc, e)    (int)&(((struc*)0)->e)

typedef struct MCU_PARA_T
{
    unsigned short ver;
    unsigned char  sys_mode;
    unsigned char  bat_type;
    unsigned char  anti_thief_enable;
    unsigned char  ecall_enbale;
    unsigned char  bcall_enbale;
    unsigned char  icall_enbale;
    unsigned int   sleep_time;
    unsigned int   deep_sleep_time;
    unsigned short acc_vecm_ths;
    unsigned char  reserved[21];
    unsigned char  cs;
} __attribute__((packed)) MCU_PARA_T;

typedef struct MCU_CFG_T
{
    MCU_PARA_T   para;
    unsigned int sn;
} __attribute__((packed)) MCU_CFG_T;

typedef enum MCU_CFG_ID
{
    MCU_CFG_ID_SYSMODE       = 0,
    MCU_CFG_ID_BAT_TYPE,
    MCU_CFG_ID_VTD,           // vehicle theft deterrent
    MCU_CFG_ID_ECALL,
    MCU_CFG_ID_BCALL,
    MCU_CFG_ID_ICALL,
    MCU_CFG_ID_STIME,
    MCU_CFG_ID_DSTIME,
    MCU_CFG_ID_ACCTHS,
    MCU_CFG_ID_SN,
    MCU_CFG_ID_MAX,
} MCU_CFG_ID;

#define MCU_CFG_ID_LEN      2

typedef struct MCU_CFG_ITEM
{
    MCU_CFG_ID       itemid;
    CFG_PARA_ITEM_ID alias_id;
    unsigned int     ilen;
    unsigned int     offset;
} MCU_CFG_ITEM;

int dev_set_from_mcu(unsigned char *msg, int len);
int dev_set_from_mpu(MCU_CFG_ID id, void *data, unsigned int len);
void scom_mcu_cfg_sync(unsigned char *msg, unsigned int len);

#endif

