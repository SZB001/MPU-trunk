#include <stdio.h>
#include <unistd.h>
#include "tcom_api.h"
#include "com_app_def.h"
#include "init.h"
#include "gpio.h"
#include "timer.h"
#include "pm_api.h"
#include "at_api.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "shell_api.h"
#include "pm_ctl.h"
#include "pm_state.h"
#include "ql_sleep_wakelock.h"
#include "scom_api.h"
#include "signal.h"
#include "netlink.h"
#include "ql_nw.h"
#include "ble.h"

static pthread_t     pm_tid;  /* thread id */
static unsigned char pm_msgbuf[TCOM_MAX_MSG_LEN];

static PM_MODE pm_mode = PM_RUNNING_MODE;
static int     pm_sleep_fd;
static timer_t pm_period_check_timer;

/****************************************************************
function:     pm_mode_proc
description:  when run mode is update,notify pm module
input:        ST_DEF_ITEM_ID id,
              unsigned char *old_mode,
              unsigned char *new_mode,
              unsigned int len;
              int len
output:       none
return:       none
*****************************************************************/
int pm_mode_proc(ST_DEF_ITEM_ID id, unsigned char *old_mode,
                 unsigned char *new_mode, unsigned int len)
{
    PM_MODE mode;

    if ((*new_mode < PM_RUNNING_MODE) || (*new_mode > PM_OFF_MODE))
    {
        log_e(LOG_PM, "new mode is invalid, new_mode:%d, old_mode:%d", *new_mode, *old_mode);
        return -1;
    }

    mode = *new_mode;
    pm_mode = mode;
    log_i(LOG_PM, "current mode:%d, old mode:%d", mode, *old_mode);

    switch (mode)
    {
        case PM_RUNNING_MODE:
            log_o(LOG_PM, "pm mode: running");
            pm_send_evt(MPU_MID_PM, PM_EVT_RUNNING);
            return 0;

        case PM_EMERGENCY_MODE:
            log_o(LOG_PM, "pm mode: emergency");
            pm_send_evt(MPU_MID_PM, PM_EVT_EMERGENCY);
            return 0;

        case PM_LISTEN_MODE:
            log_o(LOG_PM, "pm mode: listen");
            pm_send_evt(MPU_MID_PM, PM_EVT_SLEEP_REQ);
            return 0;

        case PM_SLEEP_MODE:
            log_o(LOG_PM, "pm mode: sleep");
            break;

        case PM_DEEP_SLEEP_MODE:
            log_o(LOG_PM, "pm mode: deep sleep");
            break;

        case PM_OFF_MODE:
            log_o(LOG_PM, "pm mode: power off");
            break;

        default:
            log_e(LOG_PM, "error pm mode");
            return PM_INVALID_PARA;
    }

    return pm_send_evt(MPU_MID_PM, PM_EVT_POWER_OFF);

}

/****************************************************************
function:     pm_get_mode
description:  get pm run mode
input:        none
output:       none
return:       mode
*****************************************************************/
PM_MODE pm_get_mode(void)
{
    return pm_mode;
}

/****************************************************************
function:     pm_vote_oppose
description:  oppose the 4G module to enter sleep status
input:        none
output:       none
return:       none
*****************************************************************/
void pm_vote_oppose(void)
{
    gpio_set_level(GPIO_EN5_CTRL, PINLEVEL_HIGH);
    system("echo 1 > /sys/devices/7864900.sdhci/mmc_host/mmc1/clk_scaling/enable_emmc");
    
    QL_NW_ForbidInd(0x0);
    Ql_SLP_WakeLock_Lock(pm_sleep_fd);
}

/****************************************************************
function:     pm_vote_agree
description:  agree the 4G module to enter sleep status
input:        none
output:       none
return:       none
*****************************************************************/
void pm_vote_agree(void)
{
    system("echo 0 > /sys/devices/7864900.sdhci/mmc_host/mmc1/clk_scaling/enable_emmc");
    gpio_set_level(GPIO_EN5_CTRL, PINLEVEL_LOW);

    /* add BT to control temporary */
    //gpio_set_level(GPIO_BT_RST, PINLEVEL_LOW);

    QL_NW_ForbidInd(0x1);

    /* vote enter sleep */
    Ql_SLP_WakeLock_Unlock(pm_sleep_fd);
}

/****************************************************************
function:       pm_sleep_request
description:    notity module ready for sleep
input:          none
output:         none
return:         1 indicates successful
                0 indicates failed
*****************************************************************/
int pm_shutdown(int argc, const char **argv)
{
    scom_tl_send_frame(SCOM_TL_CMD_SLEEP_REQ, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);
    sleep(1);
    return pm_send_evt(MPU_MID_PM, PM_EVT_SLEEP_REQ);
}

/****************************************************************
function:       pm_timer_wake
description:    timer to wakeup
input:          none
output:         none
return:         1 indicates successful
                0 indicates failed
*****************************************************************/
int pm_timer_wake(int argc, const char **argv)
{
    int ret;
    unsigned int timer_wake;

    if (1 != argc)
    {
        log_e(LOG_PM, "para error,usage: timerwake x");
        return -1;
    }

    timer_wake = atoi(argv[0]);

    ret = scom_tl_send_frame(SCOM_TL_CMD_WAKE_TIME, SCOM_TL_SINGLE_FRAME, 0,
                             (unsigned char *)&timer_wake, sizeof(timer_wake));

    if (0 != ret)
    {
        log_e(LOG_PM, "set timer wake failed");
        return ret;
    }

    log_o(LOG_PM, "set timer wake successful");
    return 0;
}

/****************************************************************
function:       pm_restart
description:    restart app or 4G
input:          none
output:         none
return:         1 indicates successful
                0 indicates failed
*****************************************************************/
int pm_restart(int argc, const char **argv)
{
    if (1 != argc)
    {
        log_e(LOG_PM, "para error,usage: restart x");
        return -1;
    }

    if (0 == strncmp(argv[0], "app", strlen("app")))
    {
        pm_send_evt(MPU_MID_PM, PM_EVT_RESTART_APP_REQ);
    }
    else if (0 == strncmp(argv[0], "4g", strlen("4g")))
    {
        pm_send_evt(MPU_MID_PM, PM_EVT_RESTART_4G_REQ);
    }
    else
    {
        log_e(LOG_PM, "para invalid,%s", argv[0]);
    }

    return 0;
}


/****************************************************************
function:       pm_set_rtc_time
description:    set rtc wakeup time
input:          none
output:         none
return:         1 indicates successful
                0 indicates failed
*****************************************************************/
int pm_set_rtc_time(int argc, const char **argv)
{
    int ret;
    unsigned int time;

    if (argc != 1)
    {
        shellprintf(" usage: setrtc <x>(second)\r\n");
        return PM_INVALID_PARA;
    }

    time = atoi(argv[0]);

    ret = cfg_set_para(CFG_ITEM_RTC_WAKEUP_TIME, &time, sizeof(time));

    if (0 != ret)
    {
        shellprintf(" set rtc failed, ret:%08x\r\n", ret);
        return ret;
    }

    shellprintf(" set rtc ok!\r\n");

    return 0;
}

/****************************************************************
function:     pm_kill_handler
description:  while system power or kill process
input:        none
output:       none
return:       none
*****************************************************************/
static void pm_kill_handler(int s)
{
    pm_send_evt(MPU_MID_PM, PM_EVT_RESTART_APP_REQ);
}

/****************************************************************
function:     pm_init
description:  initiaze at module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int pm_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pm_init_para();
            pm_init_state();

            gpio_init(GPIO_EN5_CTRL);
            /* add BT to control temporary */
            //gpio_init(GPIO_BT_RST);
            gpio_init(GPIO_WAKEUP_MCU);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* create timer to check USB */
            ret = tm_create(TIMER_REL, PM_MSG_ID_CHECK_TIMER , MPU_MID_PM, &pm_period_check_timer);

            if (ret != 0)
            {
                log_e(LOG_PM, "tm_create pm send cmd timer failed, ret:0x%08x", ret);
                return ret;
            }

            /* check whether the sleep mode is valid */
            unsigned char mode;
			unsigned int  len;
			
			len = sizeof(mode);
			ret = cfg_get_para(CFG_ITEM_SLEEP_MODE, &mode, &len);

			if (ret != 0)
			{
				log_e(LOG_PM, "get power mode failed, ret:0x%08x", ret);
				return ret;
			}

			if( mode > PM_AUTO )
		    {
		    	mode = PM_AUTO;
		    	ret  = cfg_set_para_im(CFG_ITEM_SLEEP_MODE, &mode, len);	
				if (ret != 0)
				{
					log_e(LOG_PM, "set power mode failed, ret:0x%08x", ret);
					return ret;
				}
			}

            st_register(ST_ITEM_PM_MODE,    pm_mode_proc);
            shell_cmd_register("shutdown",  pm_shutdown,     "request sleep");
            shell_cmd_register("timerwake", pm_timer_wake,   "timer wake up");
            shell_cmd_register("restart",   pm_restart,      "restart 4G or APP");
            shell_cmd_register("setrtc",    pm_set_rtc_time, "set rct wakeup time");
            break;

        default:
            break;
    }

    if (ret != 0)
    {
        log_e(LOG_PM, "pm init failed");
        return ret;
    }

    return 0;
}

/****************************************************************
function:     pm_main
description:  pm module main function
input:        none
output:       none
return:       NULL
****************************************************************/
static void *pm_main(void)
{
    int ret, msg_fd, max_fd;
    int netlink_fd = -1;
    fd_set fds;
    sigset_t set;
    TCOM_MSG_HEADER msgheader;

    prctl(PR_SET_NAME, "PM");

    msg_fd = tcom_get_read_fd(MPU_MID_PM);
    if (msg_fd < 0)
    {
        log_e(LOG_PM, "get pm recv fd failed");
        return NULL;
    }

    netlink_fd = netlink_sock();
    if (netlink_fd < 0)
    {
        log_e(LOG_PM, "get netlink fd failed");
    }
    if(netlink_bind(netlink_fd,NONBLOCK) < 0)
    {
        log_e(LOG_PM, "bind netlink fd failed");
    }
    netlink_send(netlink_fd, "register", 9, 0, 0);

    if(netlink_fd >= msg_fd)
    {
        max_fd = netlink_fd;
    }
    else
    {
        max_fd = msg_fd;
    }

    pm_sleep_fd = Ql_SLP_WakeLock_Create("intest-proc", strlen("intest-proc"));

    if (pm_sleep_fd < 0)
    {
        log_e(LOG_PM, "Create wakelock failed, error:%s", strerror(errno));
        return NULL;
    }

    /*force 4G module not enter sleep */
    pm_vote_oppose();
    Ql_Autosleep_Enable(1);

    ret = tm_start(pm_period_check_timer, PM_PERIOD_CHECK_TIMEOUT, TIMER_TIMEOUT_REL_PERIOD);

    if (ret != 0)
    {
        log_e(LOG_PM, "tm_start check sleep timer failed, ret:0x%08x", ret);
    }

    /* capture the TERM signal and notify
    the PM module to do the relative work */
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    signal(SIGTERM, pm_kill_handler);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
	
    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(msg_fd, &fds);
		if(netlink_fd >= 0)
		{
        	FD_SET(netlink_fd, &fds);
		}

		ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(msg_fd, &fds))
            {
                if (0 == tcom_recv_msg(MPU_MID_PM, &msgheader, pm_msgbuf))
                {
                    if (AT_MSG_ID_RING == msgheader.msgid)
                    {
                        pm_send_evt(MPU_MID_AT, PM_EVT_RING);
                    }
					else if(BLE_MSG_ID_RING == msgheader.msgid)
					{
						pm_send_evt(MPU_MID_BLE, PM_EVT_RING);
					}
                    else if (PM_MSG_ID_EVT == msgheader.msgid)
                    {
                        PM_EVENT evt;
                        memcpy(&evt, pm_msgbuf, sizeof(evt));
                        pm_evt_proc(evt);
                    }
                    else if (PM_MSG_ID_RTC_TIMER == msgheader.msgid)
                    {
                        pm_send_evt(MPU_MID_PM, PM_EVT_RTC);
                    }
                    else if (PM_MSG_ID_CHECK_TIMER == msgheader.msgid)
                    {
                        pm_evt_proc(PM_EVT_TIMEOUT);
                    }
                    else if (MPU_MID_MID_PWDG == msgheader.msgid)
                    {
                        pwdg_feed(MPU_MID_PM);
                    }
                    else
                    {
                        log_e(LOG_PM, "unknow msg, msgid:0x%08x", msgheader.msgid);
                    }
                }
            }

            else if(FD_ISSET(netlink_fd, &fds))
            {
                int  len;
                char data[20] = {0};
                len = sizeof(data);
                netlink_recv(netlink_fd, data, &len);                
                log_o(LOG_PM, "netlink recv: %s ,len: %d",data,len);
                if(0 == strcmp(data,"re_modem"))
                {
                    pm_send_evt(MPU_MID_PM, PM_EVT_RESTART_4G_REQ);
                }
            }
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)   /* interrupted by signal */
            {
                continue;
            }

            break;  /* thread exit abnormally */
        }

    }

    return 0;
}

/****************************************************************
function:     pm_run
description:  startup pm module
input:        none
output:       none
return:       positive value indicates success;
              -1 indicates failed
*****************************************************************/
int pm_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor sleep and wakeup */
    ret = pthread_create(&pm_tid, &ta, (void *)pm_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

