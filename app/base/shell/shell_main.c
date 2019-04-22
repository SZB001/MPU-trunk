/****************************************************************
file:         shell_main.c
description:  the header file of shell main function implemention
date:         2016/11/10
author        liuzhongwen
****************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include "tcom_api.h"
#include "mid_def.h"
#include "log.h"
#include "shell_cmd.h"
#include "ipc.h"
#include "com_app_def.h"

/****************************************************************
function:     shell_init
description:  initiaze shell module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int shell_init(INIT_PHASE phase)
{
    return shell_cmd_init(phase);
}

/****************************************************************
function:     shell_main
description:  shell module main function
input:        none
output:       none
return:       NULL
*****************************************************************/
static void *shell_main(void)
{
    int tcomfd, ipcsock, maxfd;

    prctl(PR_SET_NAME, "SHELL");

    if ((ipcsock = ipc_open(COM_APP_SHELL_CMD_NAME)) < 0)
    {
        log_e(LOG_SHELL, "create ipc(%s) failed, error: %s",
              COM_APP_SHELL_CMD_NAME, strerror(errno));
        return NULL;
    }

    if ((tcomfd = tcom_get_read_fd(MPU_MID_SHELL)) < 0)
    {
        log_e(LOG_SHELL, "get tcom file descriptor failed");
        return NULL;
    }

    maxfd = tcomfd < ipcsock ? ipcsock : tcomfd;

    while (1)
    {
        fd_set fds;
        TCOM_MSG_HEADER msg;
        uint8_t msgdata[TCOM_MAX_MSG_LEN];
        int res, len;

        FD_ZERO(&fds);
        FD_SET(ipcsock, &fds);
        FD_SET(tcomfd, &fds);

        res = select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (res > 0)
        {
            if (FD_ISSET(ipcsock, &fds))
            {
                len = ipc_recv(ipcsock, COM_APP_SHELL_CTL_NAME, msgdata,
                               sizeof(msgdata));

                if (len > 0)
                {
                    shell_cmd_proc((const char *)msgdata, len);
                }
            }

            if (FD_ISSET(tcomfd, &fds))
            {
                res = tcom_recv_msg(MPU_MID_SHELL, &msg, msgdata);

                if (res != 0)
                {
                    log_e(LOG_NM, "tcom_recv_msg failed,ret:0x%08x", res);
                    continue;
                }

                if (MPU_MID_MID_PWDG == msg.msgid)
                {
                    pwdg_feed(MPU_MID_SHELL);
                }
            }
        }
        else if (res < 0 && EINTR != errno)
        {
            log_e(LOG_SHELL, "select error: %s", strerror(errno));
            break;
        }
    }

    return NULL;
}

/****************************************************************
function:     shell_run
description:  startup shell module
input:        none
output:       none
return:       positive value indicates success;
              -1 indicates failed
*****************************************************************/
int shell_run(void)
{
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    if (0 != pthread_create(&tid, &ta, (void *)shell_main, NULL))
    {
        log_e(LOG_SHELL, "create shell thread failed");
        return -1;
    }

    return 0;
}

