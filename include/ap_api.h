#ifndef __AP_API_H__
#define __AP_API_H__
#include "mid_def.h"
#include "init.h"


typedef enum
{
    AP_MSG_ID_REPOAT = MPU_MID_AUTO,
    AP_MSG_ID_CANON,
    AP_MSG_TIMER_EVENT,
    AP_MSG_NET_CONNECT,
    AP_MSG_NET_DISCONNECT,
} AP_MSG_TYPE;


#define AP_CHECK_INTERVAL   (1000)

#define AP_MAX_REG_TBL      6


typedef int (*ap_update_notify)(char *,int);

typedef struct AP_REG_ITEM
{
    ap_update_notify changed;
} AP_REG_ITEM;

typedef struct AP_REG_TBL
{
    unsigned char used_num;
    AP_REG_ITEM   item[AP_MAX_REG_TBL];
} AP_REG_TBL;

int ap_init(INIT_PHASE phase);

int ap_run(void);

int ap_update_evt_reg(ap_update_notify callback);


#endif
