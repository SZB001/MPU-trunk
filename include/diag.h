#ifndef __DIAG_H__
#define __DIAG_H__
#include "com_app_def.h"
#include "fault_sync.h"

#define DIAG_EMMC_MAX_FAULT_TIME   (60*1000)

typedef enum
{
    DIAG_EMMC_UNKNOWN = 0,
    DIAG_EMMC_OK,
    DIAG_EMMC_FULL,
    DIAG_EMMC_UMOUNT,
    DIAG_EMMC_NOT_EXIST,
    DIAG_EMMC_NOT_FORMAT,
    DIAG_EMMC_UMOUNT_POINT_NOT_EXIST,
    DIAG_EMMC_FORMATTING,
} DIAG_EMMC_STATUS;

void dev_diag_init(INIT_PHASE phase);
void dev_diag_4G_ant(void);
void dev_diag_emmc(void);
void dev_diag_mcu(void);
int  dev_diag_get_ant_status(ANT_TYPE type);
int  dev_diag_get_4G_status(void);
int  dev_diag_get_wifi_status(void);
int  dev_diag_get_bat_status(void);
int  dev_diag_get_pow_status(void);
int  dev_diag_get_can_status(int index);
int  dev_diag_get_usb_status(void);
int  dev_diag_get_sim_status(void);
int  dev_diag_get_emmc_status(void);
int  dev_diag_get_gps_status(void);
int  dev_diag_get_mic_status(void);
bool dev_diag_availbale(void);
void dev_diag_stop(void);
void dev_diag_start(void);
void dev_diag_emmc_set_format_flag(unsigned char flag);
bool dev_diag_emmc_get_format_flag(void);

#endif

