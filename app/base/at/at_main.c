/****************************************************************
 file:         at_main.c
 description:  the header file of at main function implemention
 date:         2017/1/9
 author        liuzhongwen
 ****************************************************************/
#include "com_app_def.h"
#include "tcom_api.h"
#include "init.h"
#include "at.h"
#include "at_stat.h"
#include "at_shell.h"
#include "at_task.h"
#include "at_data.h"
#include "at_dev.h"
#include "pm_api.h"
#include "dev_api.h"
#include "at_op.h"
#include "audio.h"
#include "ql_mcm_sim.h"

static PM_MODE mode;
static int port_fd;
static pthread_t at_tid; /* thread id */
static unsigned char at_msgbuf[TCOM_MAX_MSG_LEN];
sim_client_handle_type h_sim = 0;

/****************************************************************
 function:       at_sleep_available
 description:    check at module can enter into sleep mode
 input:          none
 output:         none
 return:         1 indicates module can sleep
 0 indicates module can not sleep
 *****************************************************************/
int at_sleep_available(PM_EVT_ID id)
{
    switch (id)
    {
        case PM_MSG_SLEEP:
            return at_get_ready_sleep();

        case PM_MSG_OFF:
        case PM_MSG_EMERGENCY:
        case PM_MSG_RUNNING:
        case PM_MSG_RTC_WAKEUP:
            return 1;

        default:
            return 1;
    }
}

/****************************************************************
function:     at_mode_proc
description:  when run mode is update,notify at module
input:        ST_DEF_ITEM_ID id,
              unsigned char *old_mode,
              unsigned char *new_mode,
              unsigned int len;
              int len
output:       none
return:       none
*****************************************************************/
int at_mode_proc(ST_DEF_ITEM_ID id, unsigned char *old_mode,
                 unsigned char *new_mode, unsigned int len)
{
    if ((*new_mode < PM_RUNNING_MODE) || (*new_mode > PM_OFF_MODE))
    {
        log_e(LOG_AT, "new mode is invalid, new_mode:%d, old_mode:%d", *new_mode, *old_mode);
        return -1;
    }

    mode = *new_mode;

    return 0;
}

/****************************************************************
 function:     at_get_pm_mode
 description:  get current pm mode
 input:        none
 output:       none
 return:       pm mode define in PM_MODE
 *****************************************************************/
int at_get_pm_mode(void)
{
    return mode;
}

/****************************************************************
 function:     at_init
 description:  initiaze at module
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_init(INIT_PHASE phase)
{
    int ret = 0;

    log_o(LOG_AT, "init at thread");

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            ret |= at_dev_init(phase);
            ret |= at_port_dev_open(&port_fd);

            if (ret != 0)
            {
                log_e(LOG_AT, "init and open dev failed, ret:0x%08x", ret);
                return ret;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= at_shell_init(phase);
            ret |= at_para_register();
            ret |= st_register(ST_ITEM_PM_MODE, at_mode_proc);
            ret |= pm_reg_handler(MPU_MID_AT, at_sleep_available);
            ret |= at_task_init();

            if (0 != ret)
            {
                log_e(LOG_AT, "init at task failed,ret:%d", ret);
                return ret;
            }

            audio_open();
            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
 function:     at_main
 description:  at module main function
 input:        none
 output:       none
 return:       NULL
 ****************************************************************/
static void *at_main(void)
{
    int ret, tcom_fd, max_fd;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;

    prctl(PR_SET_NAME, "AT");

    tcom_fd = tcom_get_read_fd(MPU_MID_AT);

    if (tcom_fd < 0)
    {
        log_e(LOG_AT, "get at recv fd failed");
        return NULL;
    }

    max_fd = tcom_fd;

    if (port_fd > max_fd)
    {
        max_fd = port_fd;
    }

    ret = QL_MCM_SIM_Client_Init(&h_sim);

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);
        FD_SET(port_fd, &fds);

        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(port_fd, &fds))
            {
                at_port_dev_recv();
                continue;
            }
            else if (FD_ISSET(tcom_fd, &fds))
            {
                ret = tcom_recv_msg(MPU_MID_AT, &msgheader, at_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_AT, "tcom_recv_msg failed,ret:0x%08x", ret);
                    continue;
                }

                if ((MPU_MID_AT == msgheader.sender) || (MPU_MID_TIMER == msgheader.sender))
                {
                    at_task(msgheader.msgid);
                }
                else if (MPU_MID_PM == msgheader.sender)
                {
                    if (PM_MSG_SLEEP == msgheader.msgid)
                    {
                        at_sleep_proc();
                    }
                    else if (PM_MSG_RUNNING == msgheader.msgid)
                    {
                        at_wakeup_proc();
                    }
                    else if (PM_MSG_EMERGENCY == msgheader.msgid)
                    {
                        at_wakeup_proc();
                        disconnectcall();
                    }
                    else if (PM_MSG_ID_FOTA_UPDATE == msgheader.msgid)
                    {
                        at_fota_update();
                        fota_set_status(FOTA_ING);
                    }
                }
                else if (MPU_MID_MID_PWDG == msgheader.msgid)
                {
                    pwdg_feed(MPU_MID_AT);
                }

                continue;
            }

        }
        else if (0 == ret)  /* timeout */
        {
            continue; /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            break; /* thread exit abnormally */
        }

    }

    return NULL;
}

/****************************************************************
 function:     gps_run
 description:  startup GPS module
 input:        none
 output:       none
 return:       positive value indicates success;
 -1 indicates failed
 *****************************************************************/
int at_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&at_tid, &ta, (void *) at_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

