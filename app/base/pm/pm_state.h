#ifndef __PM_STATE_H__
#define __PM_STATE_H__

#include "mid_def.h"
#include "init.h"

typedef enum  PM_STATE
{
    PM_ST_NORMAL    = 0,
    PM_ST_MCK_SLEEP,    //module check sleep status
    PM_ST_MCK_POWER_OFF,//module check power off status
    PM_ST_MCK_EMERGENCY,//module check emergency status
    PM_ST_SLEEP,
    PM_ST_MCK_RESTART_APP,//module check RESTART APP status
    PM_ST_MCK_RESTART_4G, //module check RESTART 4G status
    PM_ST_MCK_FOTA_UPDATE,//module check fota update status
    PM_ST_NUM,
} PM_STATE;

typedef void (*evt_proc)(void);

void pm_init_state(void);

void pm_set_state(PM_STATE state);

void pm_st_ignore(void);

void pm_mcu_wakeup(void);
void pm_ring_wakeup(void);
void pm_rtc_wakeup(void);

void pm_st_s0e3(void);
void pm_st_s0e4(void);
void pm_st_s0e5(void);
void pm_st_s0e6(void);
void pm_st_s0e7(void);
void pm_st_s0e8(void);
void pm_st_s0e9(void);

void pm_st_s1e3(void);
void pm_st_s1e4(void);
void pm_st_s2e3(void);
void pm_st_s3e3(void);
void pm_st_s3e5(void);
void pm_st_s4e1(void);
void pm_st_s4e3(void);
void pm_st_s5e3(void);
void pm_st_s6e3(void);
void pm_st_s7e3(void);

int pm_send_evt(unsigned short sender, PM_EVENT evt);

void pm_evt_proc(PM_EVENT evt);

#endif

