#ifndef __PM_CTL_H__
#define __PM_CTL_H__

#include "pm_api.h"

#define PM_MAX_REG_ITEM_NUM     16

typedef struct PM_REG_ITEM
{
    unsigned short  mid;
    sleep_handler handler;
} PM_REG_ITEM;

typedef struct PM_REG_TBL
{
    unsigned short used_num;
    PM_REG_ITEM  pmtbl[PM_MAX_REG_ITEM_NUM];
} PM_REG_TBL;


void pm_init_para(void);
int  pm_usb_wakeup_check(void);
int  pm_mcu_wakeup_check(void);
int  pm_is_sleep_ready(PM_EVT_ID id);
int  pm_notify_moudle(PM_EVT_ID msgid);


#endif
