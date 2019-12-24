/****************************************************************
file:         pm_state.c
description:  the source file of power management
              state machine implementation
date:         2017/06/02
author        liuzhongwen wangqinglong
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include "com_app_def.h"
#include "tcom_api.h"
#include "pm_api.h"
#include "dev_api.h"
#include "pm_state.h"
#include "pm_ctl.h"
#include "timer.h"
#include "init.h"
#include "gpio.h"
#include "cfg_api.h"
#include "hozon_PP_api.h"
#include "ql_powerdown.h"
#include "sock_api.h"
#include "at_api.h"
#include "dir.h"

static timer_t pm_rtc_data_timer;

static PM_STATE pm_state = PM_ST_NORMAL;
static pthread_mutex_t pm_state_mutex;

static int pm_ring_status = 0;

static evt_proc state_tbl[PM_ST_NUM][PM_EVT_NUM] =
{
    /*   RING           MPU_RTC      RUNNING         TIMER    EMERGENCY      POWER_OFF      SLEEP      RESTART APP     RESTART 4G     FOTA UPDATE */
    {pm_ring_wakeup,   pm_st_ignore,  pm_st_ignore,  pm_st_s0e3, pm_st_s0e4,   pm_st_s0e5,   pm_st_s0e6,   pm_st_s0e7,   pm_st_s0e8,   pm_st_s0e9  },
    {pm_ring_wakeup, pm_rtc_wakeup, pm_mcu_wakeup, pm_st_s1e3, pm_st_s1e4,   pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_ring_wakeup, pm_st_ignore,  pm_mcu_wakeup, pm_st_s2e3, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_st_ignore,   pm_st_ignore,  pm_mcu_wakeup, pm_st_s3e3, pm_st_ignore, pm_st_s3e5,   pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_ring_wakeup, pm_st_s4e1,    pm_st_ignore,  pm_st_s4e3, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_st_ignore,   pm_st_ignore,  pm_st_ignore,  pm_st_s5e3, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_st_ignore,   pm_st_ignore,  pm_st_ignore,  pm_st_s6e3, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
    {pm_st_ignore,   pm_st_ignore,  pm_st_ignore,  pm_st_s7e3, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore, pm_st_ignore},
};

void pm_init_state(void)
{
    pm_state = PM_ST_NORMAL;
    pthread_mutex_init(&pm_state_mutex, NULL);

    tm_create(TIMER_RTC, PM_MSG_ID_RTC_TIMER , MPU_MID_PM, &pm_rtc_data_timer);
}

/**************************************************************************
function:     pm_get_state
description:  get current power status
input:        none.
output:       none
return:       power state, refer to PM_STATE
***************************************************************************/
int pm_get_state(void)
{
    PM_STATE state;
    pthread_mutex_lock(&pm_state_mutex);
    state = pm_state;
    pthread_mutex_unlock(&pm_state_mutex);

    return (int)state;
}

/**************************************************************************
function:     pm_set_state
description:  set current power status
input:        PM_STATE state
output:       none
return:       none
***************************************************************************/
void pm_set_state(PM_STATE state)
{
    pthread_mutex_lock(&pm_state_mutex);
    pm_state = state;
    pthread_mutex_unlock(&pm_state_mutex);
}

/**************************************************************************
function:     pm_st_ignore
description:  ignore the event,do nothing
input:        none.
output:       none
return:       none
***************************************************************************/
void pm_st_ignore(void)
{
    return;
}

static void pm_wakeup_low(void)
{
    gpio_set_level(GPIO_WAKEUP_MCU, PINLEVEL_LOW);
    pm_ring_status = 0;
}

static void pm_wakeup_high(void)
{
    gpio_set_level(GPIO_WAKEUP_MCU, PINLEVEL_HIGH);
    pm_ring_status = 1;
}

/****************************************************************
function:     pm_mcu_wakeup
description:  wakeup by mcu
input:        none
output:       none
return:       none
*****************************************************************/
void pm_mcu_wakeup(void)
{
    unsigned char app_status =  0;

    pm_vote_oppose();
    pm_set_state(PM_ST_NORMAL);
    pm_notify_moudle(PM_MSG_RUNNING);

    /* app keep wakeup status */
    st_set(ST_ITEM_APP_SLEEP, &app_status, sizeof(app_status));

    /*power on 1.8V*/
    //gpio_set_level(GPIO_EN5_CTRL, PINLEVEL_HIGH);
}

/****************************************************************
function:     pm_ring_wakeup
description:  wakeup by ring
input:        none
output:       none
return:       none
*****************************************************************/
void pm_ring_wakeup(void)
{
    if( pm_get_state() != PM_ST_NORMAL )
    {
	    pm_vote_oppose();
	    pm_set_state(PM_ST_NORMAL);
	    pm_notify_moudle(PM_MSG_RUNNING);

	    /*power on 1.8V*/
	    //gpio_set_level(GPIO_EN5_CTRL, PINLEVEL_HIGH);
	    pm_wakeup_high();

	}
	else
    {
        /* MCU in sleep state, wakeup it */
    	if(!pm_mcu_wakeup_check())
    	{
			pm_wakeup_high();
    	}
	}
}

/****************************************************************
function:     pm_rtc_wakeup
description:  wakeup by mpu rtc
input:        none
output:       none
return:       none
*****************************************************************/
void pm_rtc_wakeup(void)
{
    log_o(LOG_PM, "--------------------------------------------------> rtc wakeup");
    pm_vote_oppose();
    pm_notify_moudle(PM_MSG_RTC_WAKEUP);
    SetPrvtProt_Awaken((int)PM_MSG_RTC_WAKEUP);
}

/****************************************************************
function:     pm_st_s0e3
description:  when is in running mode, timeout is hanppened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e3(void)
{
    unsigned char mode = PM_RUNNING_MODE;
    unsigned int len = sizeof(mode);
    int ret, is_allow_sleep = 0;
    static int pm_check_cnt = 0;

    is_allow_sleep |= pm_usb_wakeup_check();
    is_allow_sleep |= (pm_mcu_wakeup_check() << 1);
    log_i(LOG_PM, "running mode, is_allow_sleep: %x", is_allow_sleep);

    if (is_allow_sleep >= 1)
    {
        pm_check_cnt   = 0;

        if (0 != pm_ring_status)
        {
            pm_wakeup_low();
        }
    }
    else
    {
        pm_check_cnt++;

        if (pm_check_cnt >= 10)
        {
            ret = st_get(ST_ITEM_PM_MODE, &mode, &len);

            if (ret != 0)
            {
                log_e(LOG_PM, "get pm mode failed, ret:%u", ret);
                return;
            }

            if ((PM_SLEEP_MODE == mode) || (PM_DEEP_SLEEP_MODE == mode) || (PM_OFF_MODE == mode))
            {
                pm_set_state(PM_ST_MCK_POWER_OFF);
                pm_notify_moudle(PM_MSG_OFF);
            }
            else
            {
                pm_set_state(PM_ST_MCK_SLEEP);
                pm_notify_moudle(PM_MSG_SLEEP);
            }

            pm_check_cnt = 0;
        }
    }
}

/****************************************************************
function:     pm_st_s0e4
description:  when is in running mode, requeset emergency
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e4(void)
{
    pm_set_state(PM_ST_MCK_EMERGENCY);
    pm_notify_moudle(PM_MSG_EMERGENCY);
}

/****************************************************************
function:     pm_st_s0e5
description:  when is in running mode, requeset shutdown
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e5(void)
{
    pm_set_state(PM_ST_MCK_POWER_OFF);
    pm_notify_moudle(PM_MSG_OFF);
}

/****************************************************************
function:     pm_st_s0e6
description:  when is in running mode, requeset sleep
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e6(void)
{
    pm_set_state(PM_ST_MCK_SLEEP);
    pm_notify_moudle(PM_MSG_SLEEP);
}

/****************************************************************
function:     pm_st_s0e6
description:  when is in running mode, requeset restart app
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e7(void)
{
    pm_set_state(PM_ST_MCK_RESTART_APP);
    pm_notify_moudle(PM_MSG_OFF);
}

/****************************************************************
function:     pm_st_s0e6
description:  when is in running mode, requeset restart 4G
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e8(void)
{
    pm_set_state(PM_ST_MCK_RESTART_4G);
    pm_notify_moudle(PM_MSG_OFF);
}

/****************************************************************
function:     pm_st_s0e9
description:  when is in running mode, requeset fota update
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s0e9(void)
{
    pm_set_state(PM_ST_MCK_FOTA_UPDATE);
    pm_notify_moudle(PM_MSG_OFF);
}

/****************************************************************
function:     pm_st_s1e3
description:  when module check status, timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s1e3(void)
{
    int is_allow_sleep = 0;

    is_allow_sleep |= pm_usb_wakeup_check();
    is_allow_sleep |= (pm_mcu_wakeup_check() << 1);
    log_o(LOG_PM, "module check sleep status, is_allow_sleep: %x", is_allow_sleep);

    if (is_allow_sleep >= 1)
    {
        pm_mcu_wakeup();
        pm_wakeup_low();
    }
    else
    {
        if ((1 == pm_is_sleep_ready(PM_MSG_SLEEP)) && (0 == pm_ring_status))
        {
            unsigned int time = 0, len;
            pm_set_state(PM_ST_SLEEP);
            log_o(LOG_PM, "-----------------going to sleep----------------");
            clearPP_rmtDiag_para();
            fflush(stdout);
            len = sizeof(time);
            cfg_get_para(CFG_ITEM_RTC_WAKEUP_TIME, &time, &len);

            if (0 != time)
            {
                tm_stop(pm_rtc_data_timer);
                tm_start(pm_rtc_data_timer, time * 1000, TIMER_TIMEOUT_REL_ONCE);
            }
            
            pm_vote_agree();
        }
    }
}

/****************************************************************
function:     pm_st_s1e4
description:  when module check status, emergency is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s1e4(void)
{
    pm_set_state(PM_ST_MCK_EMERGENCY);
    pm_notify_moudle(PM_MSG_EMERGENCY);
}

/****************************************************************
function:     pm_st_s2e3
description:  when module shutdown check status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s2e3(void)
{
    int is_allow_off = 0;

    if ((PM_OFF_MODE != pm_get_mode()) &&
        (PM_DEEP_SLEEP_MODE != pm_get_mode()))
    {
        is_allow_off |= pm_usb_wakeup_check();
    }

    is_allow_off |= (pm_mcu_wakeup_check() << 1);
    log_o(LOG_PM, "module check power off status, status: %d", is_allow_off);

    if (is_allow_off >= 1)
    {
        pm_mcu_wakeup();
        pm_wakeup_low();
    }
    else
    {
        if ((1 == pm_is_sleep_ready(PM_MSG_OFF)) && (0 == pm_ring_status))
        {
            log_o(LOG_PM, "-----------------going to power off------------------");
            Ql_Powerdown(0);
            //system("halt");
        }
    }

}

/****************************************************************
function:     pm_st_s3e3
description:  when module emergency check status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s3e3(void)
{
    pm_wakeup_low();

    if (1 == pm_is_sleep_ready(PM_MSG_EMERGENCY))
    {
        /* app can enter sleep status */
        unsigned char app_status =  1;
        st_set(ST_ITEM_APP_SLEEP, &app_status, sizeof(app_status));
        log_i(LOG_PM, "app can enter sleep status");
    }
}

/****************************************************************
function:     pm_st_s3e5
description:  when module emergency check status,request shutdown
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s3e5(void)
{
    pm_set_state(PM_ST_MCK_POWER_OFF);
    pm_notify_moudle(PM_MSG_OFF);
}

/****************************************************************
function:     pm_st_s4e1
description:  when stay sleep status,mpu rtc is wakeup
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s4e1(void)
{
    pm_rtc_wakeup();
    pm_set_state(PM_ST_MCK_SLEEP);
}

/****************************************************************
function:     pm_st_s4e3
description:  when stay sleep status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s4e3(void)
{
    int is_allow_sleep = 0;
    int sock_enable;

    is_allow_sleep |= pm_usb_wakeup_check();
    is_allow_sleep |= (pm_mcu_wakeup_check() << 1);

    log_o(LOG_PM, "from sleep status wakeup, is_allow_sleep: %x", is_allow_sleep);

    if (is_allow_sleep >= 1)
    {
        pm_mcu_wakeup();
        pm_wakeup_low();
        return ;
    }

    sock_enable = sock_recv_check();

    if (sock_enable >= 0)
    {
        log_o(LOG_PM, "--------------------------------------------------> data wakeup, tsp:%d",
              sock_enable);
        pm_ring_wakeup();
    }
}

/****************************************************************
function:     pm_st_s2e3
description:  when module restart app check status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s5e3(void)
{
    static int cnt = 0;

    if ((1 == pm_is_sleep_ready(PM_MSG_OFF)) || (cnt > 5))
    {
        log_o(LOG_PM, "-----------------kill app and wait to restart it-----------------");
        system("pkill -9 tbox_app.bin");
    }
    else
    {
        cnt++;
    }
}

/****************************************************************
function:     pm_st_s2e3
description:  when module restart 4G check status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s6e3(void)
{
    static int cnt = 0;

    if ((1 == pm_is_sleep_ready(PM_MSG_OFF)) || (cnt > 5))
    {
        log_o(LOG_PM, "-----------------going to restart 4G------------------");
        Ql_Powerdown(0);
    }
    else
    {
        cnt++;
    }
}

/****************************************************************
function:     pm_st_s7e3
description:  when fota update check status,timeout is happened
input:        none
output:       none
return:       none
*****************************************************************/
void pm_st_s7e3(void)
{
    int ret;
    int is_allow = 0;
    unsigned char fota_status;
    static int cnt = 0;
    TCOM_MSG_HEADER msghdr;

    if (1 == pm_is_sleep_ready(PM_MSG_OFF))
    {
        is_allow = 1;
    }
    else
    {
        cnt++;

        if (cnt > 10)
        {
            is_allow = 1;
        }
    }

    fota_status = fota_get_status();

    if ((FOTA_OK == fota_status) || (FOTA_ING == fota_status))
    {
        return;
    }
    else if (FOTA_ERROR == fota_status)
    {
        static int error_cnt = 0;
        error_cnt++;
        log_e(LOG_PM, "fota update error, cnt:%d", error_cnt);

        if (error_cnt > 3)
        {
            upg_set_status( DEV_UPG_IDLE );
            error_cnt = 0;
            pm_set_state(PM_ST_NORMAL);
            pm_notify_moudle(PM_MSG_RUNNING);

            if (dir_exists("/usrdata/cache"))
            {
                ret = dir_remove_path("/usrdata/cache");
                if (ret != 0)
                {
                    log_e(LOG_DEV, "remove fota cache dir failed, path:%s, ret:0x%08x",
                            "/usrdata/cache", ret);
                }
            }

            ret = dir_remove_path(COM_APP_UPG_DIR);
            if (ret != 0)
            {
                log_e(LOG_APPL, "remove upgrade dir failed, path:%s, ret:0x%08x",
                      COM_APP_UPG_DIR, ret);
            }

            log_o(LOG_PM, "-----------------stop fota update------------------");
            return;
        }
    }

    if (is_allow)
    {
        if (dir_exists("/usrdata/cache"))
        {
            dir_remove_path("/usrdata/cache");
        }

        log_o(LOG_PM, "-----------------going to fota update------------------");
        msghdr.sender    = MPU_MID_PM;
        msghdr.receiver  = MPU_MID_AT;
        msghdr.msgid     = PM_MSG_ID_FOTA_UPDATE;
        msghdr.msglen    = 0;

        /* send event to at */
        ret = tcom_send_msg(&msghdr, NULL);

        if (ret != 0)
        {
            log_e(LOG_PM, "tcom_send_msg failed, ret:%u", ret);
        }
    }
}

/****************************************************************
function:     pm_send_event
description:  send event to pm module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int pm_send_evt(unsigned short sender, PM_EVENT evt)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    msghdr.sender    = MPU_MID_PM;
    msghdr.receiver  = MPU_MID_PM;
    msghdr.msgid     = PM_MSG_ID_EVT;
    msghdr.msglen    = sizeof(evt);

    /* send event to pm */
    ret = tcom_send_msg(&msghdr, (unsigned char *)&evt);

    if (ret != 0)
    {
        log_e(LOG_PM, "tcom_send_msg failed, ret:%u", ret);
    }

    return ret;
}

/****************************************************************
function:     pm_evt_proc
description:  proc the event
input:        PM_EVENT evt
output:       none
return:       none
*****************************************************************/
void pm_evt_proc(PM_EVENT evt)
{
    if ((evt >= PM_EVT_NUM) || (evt < PM_EVT_RING))
    {
        log_e(LOG_PM, "invalid event, evt:%u", evt);
        return;
    }

    log_i(LOG_PM, "proc event, state:%u, evt:%u", pm_state, evt);

    state_tbl[pm_state][evt]();
}


