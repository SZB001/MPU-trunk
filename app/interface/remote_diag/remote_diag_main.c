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

extern UDS_T    uds_client;
timer_t remote_diag_request_timeout;

remote_diag_request_arr_t remote_diag_request_arr;
int remote_diag_fds[2];
static int remote_diag_sleep_flag;
pthread_mutex_t remote_diag_sleep_flag_mutex;
pthread_mutex_t remote_diag_mutex;



/*发送远程诊断请求之后，等待响应条件*/
pthread_cond_t  remote_diag_waitresponse = PTHREAD_COND_INITIALIZER;


int remote_diag_init(INIT_PHASE phase)
{
    int ret = 0;
    ret |= remote_diag_shell_init(phase);
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            remote_diag_sleep_flag = 0;
            pthread_mutex_init(&remote_diag_sleep_flag_mutex, NULL);
            pthread_mutex_init(&remote_diag_mutex, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            pipe(remote_diag_fds);
            ret = tm_create(TIMER_REL, REMOTE_DIAG_REQUEST_TIMEOUT, MPU_MID_REMOTE_DIAG,
                            &remote_diag_request_timeout);
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

        if (res < 0)
        {
            log_e(LOG_REMOTE_DIAG, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            /* 处理平台发送的远程诊断消息 */
            case REMOTE_DIAG_REQUEST:
                if((get_remote_diag_state() == REMOTE_DIAG_IDLE)||(get_remote_diag_state() == REMOTE_DIAG_FINISHED))
                {
                    parsing_remote_diag_msg(remote_diag_msg, msg, &(get_remote_diag_state_t()->remote_diag_request_arr));
                    remote_diag_process_start();
                }
                else
                {
                    log_e(LOG_REMOTE_DIAG, "Remote diagnosis is in progress!");
                }
                
                break;

            /* 远程诊断响应 */
            case REMOTE_DIAG_UDS_RESPONSE:
            case REMOTE_DIAG_SET_MCU_RESPONSE:
            case REMOTE_DIAG_MCU_RESPONSE:
                remote_diag_process(msg.msgid, remote_diag_msg, msg.msglen);/*逐次执行单个命令 */
                break;
                
            /* 请求超时 */
            case REMOTE_DIAG_REQUEST_TIMEOUT:
                log_o(LOG_REMOTE_DIAG, "remote diag get REMOTE_DIAG_REQUEST_TIMEOUT");
                remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_TIMEOUT, remote_diag_msg, msg.msglen);
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



