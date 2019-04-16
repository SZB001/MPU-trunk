
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "log.h"
#include "timer.h"
#include "uds_proxy.h"
#include "uds_define.h"
#include "uds_request.h"
#include "uds_server.h"
#include "uds_client.h"
#include "tcom_api.h"

static void uds_timer_notify(union sigval sigv)
{
    TCOM_MSG_HEADER msg;
    uint16_t seq = sigv.sival_int >> 16;
    
    msg.sender   = MPU_MID_UDS;
    msg.receiver = MPU_MID_UDS;
    msg.msgid    = sigv.sival_int & 0xffff;
    msg.msglen   = sizeof(seq);

    if (tcom_send_msg(&msg, &seq) != 0)
    {
        log_e(LOG_UDS, "send timer message fail");
    }
}


int uds_timer_create(uds_timer_t *tm, int idx)
{
    assert(tm != NULL);
    
    tm->idx = idx;
    tm->seq = 0;
    tm->tm  = (timer_t)-1;
    return 0;
}

int uds_timer_settime(uds_timer_t *tm, int time)
{    
    int res = 0;

    assert(tm != NULL);

    
    if (tm->tm != (timer_t)-1)
    {
        timer_delete(tm->tm);
        tm->tm = (timer_t)-1;
    }

    if (++(tm->seq) == 0)
    {
        tm->seq = 1;
    }

    if (time > 0)
    {
        struct sigevent evp;
        struct itimerspec ts;
        
        evp.sigev_notify            = SIGEV_THREAD;
        evp.sigev_notify_function   = uds_timer_notify;
        evp.sigev_value.sival_int   = tm->idx | (tm->seq << 16);
        evp.sigev_notify_attributes = NULL;

        ts.it_interval.tv_sec  = 0;
        ts.it_interval.tv_nsec = 0;
        ts.it_value.tv_sec  = time / 1000;
        ts.it_value.tv_nsec = (time % 1000) * 1000 * 1000;
        
        res = timer_create(CLOCK_MONOTONIC, &evp, &tm->tm) || 
              timer_settime(tm->tm, 0, &ts, NULL);
    }

    if (res)
    {
        log_e(LOG_UDS, "set timer's time to %d fail: %s", time, strerror(errno));
    }
    return res;
}
