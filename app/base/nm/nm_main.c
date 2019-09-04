/****************************************************************
file:         nm_main.c
description:  the source file of data communciation implementation
date:         2016/10/11
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "nm_dial.h"
#include "nm_diag.h"
#include "nm_shell.h"
#include "pm_api.h"
#include "at_api.h"
#include "at.h"

static pthread_t nm_tid;    /* thread id */
static unsigned char nm_msgbuf[TCOM_MAX_MSG_LEN * 2];

unsigned int dsi_client_init = 0;
unsigned int g_call_id = 0;

extern int net_apn_config(NET_TYPE type);
extern void nm_dial_init_cb_fun(void *user_data);
extern int nm_dial_stop(void);
//extern int nm_dial_restart(void);

int nm_is_ready_sleep = 0;

int nm_sleep_available(PM_EVT_ID id)
{
    switch (id)
    {
        case PM_MSG_SLEEP:
        case PM_MSG_OFF:
            return nm_is_ready_sleep;

        case PM_MSG_EMERGENCY:
        case PM_MSG_RTC_WAKEUP:
            return 1;

        default:
            return 1;
    }

}

/****************************************************************
function:     nm_init
description:  initiaze data communciation module
input:        INIT_PHASE phase, init phase;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_init(INIT_PHASE phase)
{
    NM_RET_CHK(nm_dial_init(phase));
    NM_RET_CHK(nm_diag_init(phase));
    NM_RET_CHK(nm_shell_init(phase));

    return 0;
}


/****************************************************************
function:     nm_main
description:  data communciation module main function
input:        none
output:       none
return:       NULL
****************************************************************/
void *nm_main(void)
{
    int maxfd, tcom_fd, ret;
    TCOM_MSG_HEADER msghdr;
    ql_apn_info_list_s apn_list;
    unsigned int retry_cnt = 20;
    fd_set fds;

    prctl(PR_SET_NAME, "NM");

    tcom_fd = tcom_get_read_fd(MPU_MID_NM);

    if (tcom_fd  < 0)
    {
        log_e(LOG_NM, "tcom_get_read_fd failed");
        return NULL;
    }

    maxfd = tcom_fd;

    while (retry_cnt > 0)
    {
        ret = dsi_init_ex(DSI_MODE_GENERAL, nm_dial_init_cb_fun, (void *)&dsi_client_init);

        if (ret != DSI_SUCCESS)
        {
            retry_cnt --;
            sleep(1);
        }
        else
        {
            break;
        }
    }

    log_o(LOG_NM, "dsi_init_ex retry_cnt = %d.", retry_cnt);

    /* waitting for api service is ready */
    retry_cnt = 20;

    while (retry_cnt > 0)
    {
        memset(&apn_list, 0, sizeof(apn_list));
        ret = QL_APN_Get_Lists(&apn_list);

        if (ret > 8)
        {
            log_e(LOG_NM, "QL_APN_Get_Lists ret is %d.", ret);
        }

        if (ret > 0)
        {
            break;
        }

        retry_cnt --;
        usleep(500 * 1000);
    }

    log_o(LOG_NM, "QL_APN_Get_Lists retry_cnt = %d.", retry_cnt);

    if (ret < 0)
    {
        /* nerver happen */
        log_e(LOG_NM, "Unknow, failed to get apn list!!!");
    }

    retry_cnt = 20;

    while (retry_cnt > 0)
    {
        if (1 == dsi_client_init)
        {
            break;
        }

        retry_cnt --;
        usleep(500 * 1000);
    }

    if (net_apn_config(NM_PRIVATE_NET) != 0 || net_apn_config(NM_PUBLIC_NET) != 0)
    {
        /* nerver happen */
        log_e(LOG_NM, "Unknow, failed to set apn");
    }

    /* start dial data link */
    nm_dial_start();

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);

        /* monitor the incoming data */
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret > 0)
        {
            if (FD_ISSET(tcom_fd, &fds))
            {
                ret = tcom_recv_msg(MPU_MID_NM, &msghdr, nm_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_NM, "tcom_recv_msg failed,ret:0x%08x", ret);
                    continue;
                }

                if (MPU_MID_MID_PWDG == msghdr.msgid)
                {
                    pwdg_feed(MPU_MID_NM);
                }
                else if (NM_MSG_ID_DIAG_TIMER == msghdr.msgid)
                {
                    nm_diag_msg_proc(&msghdr, nm_msgbuf);
                }
                else if (PM_MSG_RUNNING == msghdr.msgid)
                {
                    // nm_dial_restart();    //JSQ
                    nm_is_ready_sleep = 0;

                    /* check if in cfun 4 */
                    if (4 == at_get_cfun_status())
                    {
                        at_set_cfun(1);//exit cfun 4
                    }
                }
                else if ((PM_MSG_SLEEP == msghdr.msgid) || (PM_MSG_OFF == msghdr.msgid))
                {
                    // nm_dial_stop();
                    nm_is_ready_sleep = 1;

                    log_o(LOG_NM, "nm recv pm sleep info...");
                }
                else
                {
                    nm_dial_msg_proc(&msghdr, nm_msgbuf);
                }
            }
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            log_e(LOG_NM, "nm_main exit, error:%s", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

/****************************************************************
function:     nm_run
description:  startup data communciation module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&nm_tid, &ta, (void *)nm_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_NM, "pthread_create failed, error:%s", strerror(errno));
        return NM_CREATE_THREAD_FAILED;
    }

    return 0;
}

