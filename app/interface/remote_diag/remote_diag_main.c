#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "../support/protocol.h"
#include "pm_api.h"
#include "can_api.h"
#include "remote_diag.h"
#include "uds_did.h"
#include "dev_api.h"
#include "remote_diag_api.h"
#include "uds_define.h"
#include "uds.h"
#include "can_api.h"

extern UDS_T    uds_client;
timer_t remote_diag_request_timeout;

remote_diag_request_arr_t remote_diag_request_arr;
int remote_diag_fds[2];
static int remote_diag_sleep_flag;
pthread_mutex_t remote_diag_sleep_flag_mutex;
pthread_mutex_t remote_diag_mutex;


static int remote_diag_dbc_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    switch (event)
    {
        case DBC_EVENT_SURFIX:
            {
                int sigid = (int)arg1;
                const char *sfx = (const char *)arg2;
                uint32_t gbtype, gbindex;

                assert(sigid > 0 && sfx != NULL);

                remote_diag_precondition_sig_id *precondition_sig_id
                    = get_remote_diag_precondition_sig_id();

                if (2 != sscanf(sfx, "G%1x%3x", &gbtype, &gbindex))
                {
                    return 0;
                }

                if ((1 == gbtype) && (2 == gbindex))
                {
                    /*车速 sig id*/
                    precondition_sig_id->vehicle_speed_sig_id = sigid;
                }
                else if ((1 == gbtype) && (8 == gbindex))
                {
                    /*档位 sig id*/
                    precondition_sig_id->vehicle_gear_sig_id = sigid;
                }
                else if ((1 == gbtype) && (0 == gbindex))
                {
                    /*电源档位 sig id*/
                    precondition_sig_id->power_stall_sig_id = sigid;
                }
                else if ((1 == gbtype) && (1 == gbindex))
                {
                    /*充电状态 sig id*/
                    precondition_sig_id->charge_state_sig_id = sigid;
                }
                else
                {

                }

                break;
            }

        default:
            break;
    }

    return 0;
}



int remote_diag_init(INIT_PHASE phase)
{
    int ret = 0;
    ret |= remote_diag_shell_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            remote_diag_sleep_flag = 0;
            pthread_mutex_init(&remote_diag_sleep_flag_mutex, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            pipe(remote_diag_fds);
            ret = tm_create(TIMER_REL, REMOTE_DIAG_REQUEST_TIMEOUT, MPU_MID_REMOTE_DIAG,
                            &remote_diag_request_timeout);
            ret |= dbc_register_callback(remote_diag_dbc_cb);
            break;
    }


    return ret;
}



static void *remote_diag_main(void)
{
    int tcomfd;
    int ret = 0;
    log_o(LOG_REMOTE_DIAG, "REMOTE_DIAG thread running");

    prctl(PR_SET_NAME, "REMOTE_DIAG");

    if ((tcomfd = tcom_get_read_fd(MPU_MID_REMOTE_DIAG)) < 0)
    {
        log_e(LOG_REMOTE_DIAG, "get module pipe failed, thread exit");
        return NULL;
    }

    if (ret != 0)
    {
        log_e(LOG_REMOTE_DIAG, "start remote diag msg timeout timer failed!");
    }

    while (1)
    {
        TCOM_MSG_HEADER msg;

        char remote_diag_msg[2048];

        int res;

        res = protocol_wait_msg(MPU_MID_REMOTE_DIAG, tcomfd, &msg, &remote_diag_msg, 100);

        log_buf_dump(LOG_REMOTE_DIAG, ">>>>>>>>>>>>>remote diag recv msg>>>>>>>>>>>>>",
                     (const uint8_t *)remote_diag_msg,
                     (int)msg.msglen);

        if (res < 0)
        {
            log_e(LOG_REMOTE_DIAG, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            /* 处理其它模块发送的远程诊断请求 */
            case REMOTE_DIAG_REQUEST:
                if(0 == dev_get_KL15_signal())
                {
                    log_e(LOG_REMOTE_DIAG, "IG OFF, quit remote diag!");
                    remote_diag_process_quit();/*IG OFF 停止远程诊断 */
                }
                else
                {
                    remote_diag_process_start(remote_diag_msg, msg);
                }
                break;

            /* 远程诊断响应 */
            case REMOTE_DIAG_UDS_RESPONSE:
            case REMOTE_DIAG_SET_MCU_RESPONSE:
            case REMOTE_DIAG_MCU_RESPONSE:
                if(0 == dev_get_KL15_signal())
                {
                    log_e(LOG_REMOTE_DIAG, "IG OFF, quit remote diag!");
                    remote_diag_process_quit();/*IG OFF 停止远程诊断 */
                }
                else
                {
                    remote_diag_process(msg.msgid, remote_diag_msg, msg.msglen);/*逐次执行单个命令 */
                }
                break;

            /* 请求超时 */
            case REMOTE_DIAG_REQUEST_TIMEOUT:
                if(0 == dev_get_KL15_signal())
                {
                    log_e(LOG_REMOTE_DIAG, "IG OFF, quit remote diag!");
                    remote_diag_process_quit();/*IG OFF 停止远程诊断 */
                }
                else
                {
                    log_o(LOG_REMOTE_DIAG, "remote diag get REMOTE_DIAG_REQUEST_TIMEOUT");
                    remote_diag_timeout_process(remote_diag_msg, msg.msglen);
                }
                break;

            /* 喂狗消息 */
            case MPU_MID_MID_PWDG:
                pwdg_feed(MPU_MID_REMOTE_DIAG);
                break;

            default:
                break;
        }
    }

    return NULL;
}

int remote_diag_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)remote_diag_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_REMOTE_DIAG, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
}



