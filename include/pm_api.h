#ifndef __PM_API_H__
#define __PM_API_H__

#include "mid_def.h"
#include "init.h"

#define PM_PERIOD_CHECK_TIMEOUT (1000)

/* pm strategy definition */
typedef enum  PM_STRATEGY
{
    PM_RUNNING_ONLY = 0,
    PM_LISTEN_ONLY,
    PM_SLEEP_ONLY,
    PM_AUTO,
} PM_STRATEGY;

/* pm mode definition */
typedef enum  PM_MODE
{
    PM_RUNNING_MODE = 0,
    PM_EMERGENCY_MODE,
    PM_LISTEN_MODE,
    PM_SLEEP_MODE,
    PM_DEEP_SLEEP_MODE,
    PM_OFF_MODE,
} PM_MODE;

/* the module of registed
 should take care of this event */
typedef enum PM_EVT_ID
{
    PM_MSG_OFF = MPU_MID_PM,
    PM_MSG_SLEEP,
    PM_MSG_EMERGENCY,
    PM_MSG_RUNNING,
    PM_MSG_RTC_WAKEUP,
    PM_MSG_ID_MAX,
} PM_EVT_ID;

/*module  message ID definition */
typedef enum  PM_MSG_ID
{
    PM_MSG_ID_EVT = PM_MSG_ID_MAX,
    PM_MSG_ID_CHECK_TIMER,
    PM_MSG_ID_RTC_TIMER,
    PM_MSG_ID_FOTA_UPDATE,
} PM_MSG_ID;

typedef enum PM_ERROR_CODE
{
    PM_INVALID_PARA  = (MPU_MID_PM << 16) | 0x01,
    PM_OPEN_DEV_FAILED,
    PM_TABLE_OVERFLOW,
} PM_PARA_ERROR_CODE;

typedef enum  PM_EVENT
{
    PM_EVT_RING,
    PM_EVT_RTC,
    PM_EVT_RUNNING,
    PM_EVT_TIMEOUT,
    PM_EVT_EMERGENCY,
    PM_EVT_POWER_OFF,
    PM_EVT_SLEEP_REQ,
    PM_EVT_RESTART_APP_REQ,
    PM_EVT_RESTART_4G_REQ,
    PM_EVT_FOTA_UPDATE_REQ,
    PM_EVT_NUM,
} PM_EVENT;

typedef int (*sleep_handler)(PM_EVT_ID);

/*Definition in pm_ctl.c file*/
int  pm_reg_handler(unsigned short  mid, sleep_handler handler);

int  pm_mcu_wakeup_check(void);

/*get sleep status*/
int pm_get_state(void);

/* get pm run mode */
PM_MODE pm_get_mode(void);

/*Definition in pm_main.c file */
void pm_vote_oppose(void);
void pm_vote_agree(void);

int pm_send_evt(unsigned short sender, PM_EVENT evt);

/* initiaze pm module */
int pm_init(INIT_PHASE phase);
/* starup pm module */
int pm_run(void);

#endif
