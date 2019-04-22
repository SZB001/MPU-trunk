/****************************************************************
file:         timer.c
description:  the source file of timer implementation
date:         2016/9/20
author        liuzhongwen
****************************************************************/
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/time.h>
#include "timer.h"
#include "tcom_api.h"
#include "mid_def.h"
#include "log.h"

static unsigned long long timebase = 0;

/*********************************************
function:     timer_timeout
description:  timeout callback
input:        union sigval value, the high 16 bytes is timername,
              and low 16 bytes is mid
output:       none
return:       none
*********************************************/
void tm_timeout(union sigval value)
{
    TCOM_MSG_HEADER msghdr;
    unsigned long long time;
    struct timespec now = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
    {
        log_e(LOG_MID, "get system monotonic clock fail");
        return;
    }

    time = now.tv_sec * 1000000 + now.tv_nsec / 1000;
    msghdr.sender   = MPU_MID_TIMER;
    msghdr.receiver = value.sival_int >> 16;
    msghdr.msgid    = value.sival_int & 0xffff;
    msghdr.msglen   = sizeof(time);

    if (tcom_send_msg(&msghdr, &time) != 0)
    {
        log_e(LOG_MID, "send message (0x%04x) to moudle(0x%04x) fail",
              msghdr.msgid, msghdr.receiver);
    }
}

/*********************************************
function:     timer_create
description:  create timer
input:        TIMER_TYPE type, timer type;
              unsigned short timerid, timername;
              unsigned short mid, moudle id;
output:       timer_t *timer, timer id
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_create(TIMER_TYPE type, unsigned short timername,
              unsigned short mid, timer_t *timer)
{
    int ret;
    struct sigevent evp;

    if (NULL == timer)
    {
        log_e(LOG_MID, "timer parameter is NULL");
        return TIMER_INVALID_PARAMETER;
    }

    if ((TIMER_REL != type) && (TIMER_ABS != type) && (TIMER_RTC != type))
    {
        log_e(LOG_MID, "type parameter is invalid, type:%u", type);
        return TIMER_INVALID_PARAMETER;
    }

    evp.sigev_notify            = SIGEV_THREAD;
    evp.sigev_notify_function   = tm_timeout;
    evp.sigev_value.sival_int   = timername | (mid << 16);
    evp.sigev_notify_attributes = NULL;

    /* if create relative timer */
    if (TIMER_REL == type)
    {
        ret = timer_create(CLOCK_MONOTONIC, &evp, timer);
    }
    else if (TIMER_ABS == type)
    {
        ret = timer_create(CLOCK_REALTIME, &evp, timer);
    }
    else
    {
        ret = timer_create(type, &evp, timer);
    }

    if (ret != 0)
    {
        log_e(LOG_MID, "create timer (0x%04x) in moudle(0x%04x) failed, ret:%s", timername, mid,
              strerror(errno));
        return TIMER_CREATE_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_start
description:  start timer
input:        timer_t timer, timer id;
              unsigned short interval, timer interval(the unit is ms);
              REL_TIMER_TYPE type, type of relative timer;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_start(timer_t timer, unsigned int interval, TIMER_TIMEOUT_TYPE type)
{
    struct itimerspec ts;
    struct timespec   now;
    int ret;

    if (0 == interval)
    {
        log_e(LOG_MID, "interval parameter is NULL,interval:%u", interval);
        return TIMER_INVALID_PARAMETER;
    }

    ts.it_interval.tv_sec  = 0;
    ts.it_interval.tv_nsec = 0;

    if ((TIMER_TIMEOUT_REL_ONCE == type) || (TIMER_TIMEOUT_REL_PERIOD == type))
    {
        /* the unit of interval is ms */
        ts.it_value.tv_sec  = interval / 1000;
        ts.it_value.tv_nsec = (interval % 1000) * 1000 * 1000;

        if (TIMER_TIMEOUT_REL_PERIOD == type)
        {
            ts.it_interval.tv_sec  = ts.it_value.tv_sec;
            ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
        }

        ret = timer_settime(timer, 0, &ts, NULL); /* start relative timer */
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &now);

        /* the unit of interval is ms */
        ts.it_value.tv_sec  = now.tv_sec + interval / 1000 +
                              (((interval % 1000) * 1000 * 1000 + now.tv_nsec) / (1000000000UL));
        ts.it_value.tv_nsec = ((interval % 1000) * 1000 * 1000 + now.tv_nsec) % (1000000000UL);

        ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL); /* start absolute timer */
    }

    if (ret != 0)
    {
        log_e(LOG_MID, "start timer with interval(0x%04x) failed, ret:%s", interval, strerror(errno));
        return TIMER_START_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_stop
description:  stop timer
input:        timer_t timer, timer id;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_stop(timer_t timer)
{
    struct itimerspec ts;
    int ret;

    memset(&ts, 0, sizeof(ts));

    ret = timer_settime(timer, 0, &ts, NULL);

    if (ret != 0)
    {
        log_e(LOG_MID, "stop timer failed, ret:%s", strerror(errno));
        return TIMER_STOP_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_destory
description:  destory timer, the timer will be deleted
input:        timer_t timer, timer id;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_destory(timer_t timer)
{
    int ret;

    ret = timer_delete(timer);

    if (ret != 0)
    {
        log_e(LOG_MID, "destory timer failed, ret:%s", strerror(errno));
        return TIMER_DESTORY_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     tm_get_systick
description:  get the system tick
input:        none
output:       none
return:       time
*********************************************/
unsigned long long tm_get_systick(void)
{
    int ret = -1;
    unsigned long long time;
    int cnt = 0;
    struct timespec  now = {0, 0};

    while (ret < 0 && cnt < 3)
    {
        ret = clock_gettime(CLOCK_MONOTONIC, &now);
        cnt++;
    }

    time = now.tv_sec * 1000 + now.tv_nsec / (1000 * 1000);

    return time;
}

/*********************************************
function:     tm_set_base
description:  set application base time
input:        none
output:       none
return:       time
*********************************************/
/********************************************
It can only be called once and one place !!!!
********************************************/
void tm_set_base(unsigned long long time)
{
    timebase = time;
}

/*********************************************
function:     tm_get_time
description:  get current relative time(unit is ms)
input:        none
output:       none
return:       the time since system startup
*********************************************/
unsigned long long tm_get_time(void)
{
    return tm_get_systick() - timebase;
}

/*********************************************
function:     tm_get_abstime
description:  get current absolute time(unit is ms)
input:        none,
output:       the current absolute time
return:       none
*********************************************/
int tm_get_abstime(RTCTIME *abstime)
{
    struct timeval  tv;
    struct timezone tz;
    struct tm timenow;

    if (NULL == abstime)
    {
        log_e(LOG_MID, "invalid time pointer");
        return TIMER_INVALID_PARAMETER;
    }

    gettimeofday(&tv, &tz);
    localtime_r(&tv.tv_sec, &timenow);

    abstime->msec  = tv.tv_usec / 1000;
    abstime->sec   = timenow.tm_sec;
    abstime->min   = timenow.tm_min;
    abstime->hour  = timenow.tm_hour;
    abstime->mday  = timenow.tm_mday;
    abstime->mon   = timenow.tm_mon + 1;
    abstime->year  = timenow.tm_year + 1900;

    return 0;
}

