/****************************************************************
file:         fct_main.c
description:  the source file of fct implementation
date:         2018/1/3
author        liuzhongwen
****************************************************************/

#include "com_app_def.h"
#include "tcom_api.h"
#include "log.h"
#include "fct.h"
#include "fct_cmd.h"

static pthread_t     fct_tid;  /* thread id */
static unsigned char fct_reqmsgbuf[TCOM_MAX_MSG_LEN];

/****************************************************************
function:     fct_init
description:  initiaze fct module main function
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int fct_init(INIT_PHASE phase)
{
    int ret;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            break;

        default:
            break;
    }

    ret = fct_cmd_init(phase);

    if (ret != 0)
    {
        log_e(LOG_FCT, "fct_cmd_init failed, ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     fct_main
description:  fct module main function
input:        none
output:       none
return:       NULL;
****************************************************************/
void *fct_main(void)
{
    int ret;
    int tcom_fd , max_fd = 0;
    fd_set fds;
    TCOM_MSG_HEADER reqmsghdr;

    prctl(PR_SET_NAME, "FCT");

    tcom_fd = tcom_get_read_fd(MPU_MID_FCT);

    if (tcom_fd < 0)
    {
        log_e(LOG_FCT, "get fct recv fd failed");
        return NULL;
    }

    max_fd = tcom_fd;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);

        /* monitor the incoming data */
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if ((ret > 0) && FD_ISSET(tcom_fd, &fds))
        {
            memset(fct_reqmsgbuf, 0, sizeof(fct_reqmsgbuf));
            ret = tcom_recv_msg(MPU_MID_FCT, &reqmsghdr, fct_reqmsgbuf);

            if (ret != 0)
            {
                log_e(LOG_FCT, "tcom_recv_msg failed,ret:0x%08x", ret);
                continue;
            }

            //log_i(LOG_FCT, "fct msg, sender:0x%04x, receiver:0x%04x, msgid:0x%08x, msglen:0x%08x",
            //      reqmsghdr.sender, reqmsghdr.receiver, reqmsghdr.msgid, reqmsghdr.msglen);

            if (MPU_MID_TIMER == reqmsghdr.sender)    /* timer message */
            {
                fct_cmd_timetout_proc(reqmsghdr.msgid);
            }
            else if (MPU_MID_SCOM == reqmsghdr.sender)
            {
                fct_cmd_proc(fct_reqmsgbuf, reqmsghdr.msglen);
            }
            else
            {
                /* do nothing */
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

            log_e(LOG_FCT, "fct exit, error:%s", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

/****************************************************************
function:     fct_run
description:  startup fct module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int fct_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&fct_tid, &ta, (void *)fct_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_FCT, "pthread_create failed, error:%s", strerror(errno));
        return FCT_CREATE_THREAD_FAILED;
    }

    return 0;
}

