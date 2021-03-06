#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include "gps_api.h"
#include "tcom_api.h"
#include "mid_def.h"
#include "ubx_cfg.h"
#include "gps_dev.h"
#include "gps_decode.h"
#include "log.h"
#include "timer.h"
#include "pwdg.h"
#include "pm_api.h"

#define GPS_MAX_CALLBACK    4
static gps_cb_t gps_cb_lst[GPS_MAX_CALLBACK];
static pthread_t gps_tid; /* thread id */
static unsigned char msgbuf[TCOM_MAX_MSG_LEN];

/****************************************************************
 function:     gps_init
 description:  initiaze thread communciation module
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int gps_init(INIT_PHASE phase)
{
    int ret;

    log_i(LOG_GPS, "init gps thread!");

    memset(gps_cb_lst, 0, sizeof(gps_cb_lst));

    ret = gps_dev_init(phase);

    if (ret != 0)
    {
        return ret;
    }

    ret = gps_decode_init(phase);

    if (ret != 0)
    {
        return ret;
    }

    return ret;
}

/****************************************************************
 function:     gps_main
 description:  gps module main function
 input:        none
 output:       none
 return:       NULL
 ****************************************************************/
static void *gps_main(void)
{
    int ret, msg_fd, gps_fd, max_fd;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;
    static unsigned int isInited = 0;

    prctl(PR_SET_NAME, "GPS");

    ret = gps_dev_open();

    if (ret != 0)
    {
        log_e(LOG_GPS, "open gps device failed");
        return 0;
    }

    msg_fd = tcom_get_read_fd(MPU_MID_GPS);

    if (msg_fd < 0)
    {
        log_e(LOG_GPS, "getcfg recv fd failed");
        return 0;
    }

    gps_fd = gps_dev_get_fd();

    while (1)
    {
        FD_ZERO(&fds);

        if (gps_fd >= 0)
        {
            FD_SET(gps_fd, &fds);
        }

        FD_SET(msg_fd, &fds);

        max_fd = msg_fd;

        if (gps_fd > max_fd)
        {
            max_fd = gps_fd;
        }

        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(gps_fd, &fds))
            {
                if (GNSS_EXTERNAL == GNSS_TYPE)
                {
                    if (! isInited)
                    {
                        isInited = 1;
                        /* set 1 Hz */
                        gps_dev_ubx_init();
                    }
                }

                gps_dev_recv();
            }

            if (FD_ISSET(msg_fd, &fds))
            {
                if (0 == tcom_recv_msg(MPU_MID_GPS, &msgheader, msgbuf))
                {
                    if (MPU_MID_TIMER == msgheader.sender)
                    {
                        gps_dev_timeout(msgheader.msgid);
                    }
                    else if (MPU_MID_MID_PWDG == msgheader.msgid)
                    {
                        pwdg_feed(MPU_MID_GPS);
                    }
                    else if (PM_MSG_RUNNING == msgheader.msgid)
                    {
                        if (0 == gps_dev_open())
                        {
                            gps_fd = gps_dev_get_fd();
                            gps_dev_reset();
                        }
                    }
                    else if (PM_MSG_SLEEP == msgheader.msgid || PM_MSG_OFF ==  msgheader.msgid)
                    {
                        gps_dev_close();
                        gps_fd = -1;
                    }
                }
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

    return 0;
}

/****************************************************************
 function:     gps_run
 description:  startup GPS module
 input:        none
 output:       none
 return:       positive value indicates success;
 -1 indicates failed
 *****************************************************************/
int gps_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&gps_tid, &ta, (void *) gps_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

int gps_reg_callback(gps_cb_t cb)
{
    int i = 0;

    while (i < GPS_MAX_CALLBACK && gps_cb_lst[i] != NULL)
    {
        i++;
    }

    if (i >= GPS_MAX_CALLBACK)
    {
        log_e(LOG_CAN, "no space for new callback!");
        return -1;
    }

    gps_cb_lst[i] = cb;
    return 0;
}

void gps_do_callback(unsigned int event, unsigned int arg1, unsigned int arg2)
{
    int i;

    for (i = 0; i < GPS_MAX_CALLBACK; i++)
    {
        if (gps_cb_lst[i])
        {
            gps_cb_lst[i](event, arg1, arg2);
        }
        else
        {
            break;
        }
    }
}

