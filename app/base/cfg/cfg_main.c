/****************************************************************
file:         cfg_main.c
description:  the source file of cfg main function implementation
date:         2016/9/25
author        liuzhongwen
****************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "com_app_def.h"
#include "cfg_api.h"
#include "cfg.h"
#include "cfg_para.h"
#include "tcom_api.h"
#include "shell_api.h"
#include "hozon_PP_api.h"
#include "file.h"

static pthread_t cfg_tid;  /* thread id */
static unsigned char cfg_reqmsgbuf[TCOM_MAX_MSG_LEN];

/****************************************************************
function:     cfg_init
description:  initiaze configuration manager module main function
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int cfg_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            cfg_init_para();
            break;

        case INIT_PHASE_RESTORE:

            if((access(PP_SYS_CFG_PATH,F_OK)) != 0)//文件不存在
            {
                if((access(PP_SYS_CFG_BKUP_PATH,F_OK)) == 0)//文件存在
                {
                    file_copy(PP_SYS_CFG_BKUP_PATH,PP_SYS_CFG_PATH);//还原配置文件
                }
            }

            cfg_restore_para();
            break;

        case INIT_PHASE_OUTSIDE:
            shell_cmd_register("dumpcfg",    cfg_dump_para,   "dump configuration");
            shell_cmd_register("setdefault", cfg_set_default, "set default configuration");
            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
function:     cfg_main
description:  configuration manager module main function
input:        none
output:       none
return:       NULL;
****************************************************************/
void *cfg_main(void)
{
    int maxfd = 0;
    int ret;
    fd_set fds;
    TCOM_MSG_HEADER reqmsghdr;

    prctl(PR_SET_NAME, "CFG");

    FD_ZERO(&fds);

    maxfd = tcom_get_read_fd(MPU_MID_CFG);

    if (maxfd < 0)
    {
        log_e(LOG_CFG, "get cfg recv fd failed");
        return NULL;
    }

    FD_SET(maxfd, &fds);

    while (1)
    {
        /* monitor the incoming data */
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret > 0 && FD_ISSET(maxfd, &fds))
        {
            memset(cfg_reqmsgbuf, 0, sizeof(cfg_reqmsgbuf));
            ret = tcom_recv_msg(MPU_MID_CFG, &reqmsghdr, cfg_reqmsgbuf);

            if (ret != 0)
            {
                log_e(LOG_CFG, "tcom_recv_msg failed,ret:0x%08x", ret);
                continue;
            }

            log_i(LOG_CFG, "cfg msg, sender:0x%04x, receiver:0x%04x, msgid:0x%08x, msglen:0x%08x",
                  reqmsghdr.sender, reqmsghdr.receiver, reqmsghdr.msgid, reqmsghdr.msglen);

            if (MPU_MID_TIMER == reqmsghdr.sender)    /* timer message */
            {
                /* do nothing */
            }
            else if (MPU_MID_MID_PWDG == reqmsghdr.msgid)
            {
                pwdg_feed(MPU_MID_CFG);
            }
            else
            {
                /* do nothing */
            }

            continue;
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

            log_e(LOG_CFG, "cfg exit, error:%s", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

/****************************************************************
function:     cfg_run
description:  startup configuration manager module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int cfg_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&cfg_tid, &ta, (void *)cfg_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_CFG, "pthread_create failed, error:%s", strerror(errno));
        return CFG_CREATE_THREAD_FAILED;
    }

    return 0;
}

