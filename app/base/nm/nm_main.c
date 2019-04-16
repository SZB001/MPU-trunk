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

static pthread_t nm_tid;    /* thread id */
static unsigned char nm_msgbuf[TCOM_MAX_MSG_LEN * 2];

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
    fd_set fds;

    prctl(PR_SET_NAME, "NM");

    tcom_fd = tcom_get_read_fd(MPU_MID_NM);

    if (tcom_fd  < 0)
    {
        log_e(LOG_NM, "tcom_get_read_fd failed");
        return NULL;
    }

    maxfd = tcom_fd;

    /* start dial data link */
    nm_dial_all_net();

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
                else if( NM_MSG_ID_DIAG_TIMER == msghdr.msgid )
                {
                    nm_diag_msg_proc(&msghdr, nm_msgbuf);
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

