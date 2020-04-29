#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include <pthread.h>
#include "dev_time.h"
#include "dev_api.h"
#include "log.h"
#include "scom_api.h"

static pthread_mutex_t dev_time_mutex;
static SYN_TIME_STR last_syn = { {0 }, 0 };
static bool dev_time_sync = false;  //Whether to synchronize the time by TSP or GNSS

/*********************************************
 function:     dev_time_init
 description:  init dev time mutex
 input:        none
 output:       none
 return:       0 indicates success,
               others indicates failed
 *********************************************/
void dev_time_init(void)
{
    pthread_mutex_init(&dev_time_mutex, NULL);
}

/*********************************************
 function:     dev_set_time
 description:  set systerm time
 input:        RTCTIME *time, the time need to set;
 output:       none
 return:       0 indicates success,
               others indicates failed
 *********************************************/
static int dev_set_time(RTCTIME *time)
{
    int ret;
    struct tm now;
    struct timeval tv;
    time_t t_sec;

    now.tm_year = time->year - 1900;
    now.tm_mon = time->mon - 1;
    now.tm_mday = time->mday;
    now.tm_hour = time->hour;
    now.tm_min = time->min;
    now.tm_sec = time->sec;

    t_sec = mktime(&now);

    tv.tv_sec = t_sec;
    tv.tv_usec = 0;

    ret = settimeofday(&tv, NULL);

    if (ret != 0)
    {
        log_e(LOG_DEV, "set time failed, ret:%s", strerror(errno));
        return -1;
    }

    log_o(LOG_DEV, "set time: %d-%d-%d %d:%d:%d",
          time->year, time->mon, time->mday, time->hour, time->min, time->sec);
    return 0;
}

/*********************************************
 function:     dev_syn_time
 description:  syn system time
 input:        RTCTIME *time, the time need to syn;
 output:       none
 return:       0 indicates success,
               others indicates failed
 *********************************************/
int dev_syn_time(RTCTIME *time, SYN_TIME_SOURCE src)
{
    unsigned char allowed = 0;

    /* Whether to allow the sync time */
    if (UNSET_TIME == src || NULL == time)
    {
        log_e(LOG_DEV, "INVALID PARAMETER!");
        return DEV_INVALID_PARA;
    }

    pthread_mutex_lock(&dev_time_mutex);

    switch (src)
    {
        // all sync source is allowed
        case UNSET_TIME:
        case MCU_RTC_TIME_SOURCE:
            if (UNSET_TIME == last_syn.src)
            {
                allowed = 1;
            }

            break;

        case SHELL_TIME_SOURCE:
            allowed = 1;
            break;

        // TSP
        case TSP_TIME_SOURCE:
            allowed = 1;
            break;

        // GNSS
        case GNSS_TIME_SOURCE:
            if (TSP_TIME_SOURCE != last_syn.src)
            {
                allowed = 1;
            }
            break;
        case NTP_TIME_SOURCE:
            if((GNSS_TIME_SOURCE != last_syn.src) && \
               (TSP_TIME_SOURCE != last_syn.src))
            {
                allowed = 1;
            }
            break;
    }

    if (allowed)
    {
        memcpy((char *) &last_syn.time, time, sizeof(RTCTIME));
        pthread_mutex_unlock(&dev_time_mutex);

        if (0 != dev_set_time(time))
        {
            return -1;
        }

        if (MCU_RTC_TIME_SOURCE != src)
        {
            if(0 != scom_tl_send_frame(SCOM_TL_CMD_SET_TIME, SCOM_TL_SINGLE_FRAME, 0, (unsigned char *) time,
                               sizeof(RTCTIME)))
            {
                log_e(LOG_DEV, "set time to mcu rtc failed\n");
            }
        }

        if((TSP_TIME_SOURCE == src)  || \
           (GNSS_TIME_SOURCE == src) || \
           (NTP_TIME_SOURCE == src))
        {
            dev_time_sync = true;          
        }

        last_syn.src = src;
        return 0;
    }

    pthread_mutex_unlock(&dev_time_mutex);
    return 0;
}

/************************************************************
 function:     dev_req_time
 description:  send cmd to get mcu time
 input:        none
 output:       none
 return:       0 indicates success,
               others indicates failed
 ************************************************************/
int dev_req_time(void)
{
    int ret;

    ret = scom_tl_send_frame(SCOM_TL_CMD_GET_TIME, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);

    if (0 != ret)
    {
        log_e(LOG_DEV, "Get mcu rtc time failed, ret:%d", ret);
        return ret;
    }

    return 0;
}

/*********************************************
 * function:     dev_is_time_syn
 * description:  whether system time is sync
 * input:        none
 * output:       none
 * return:       true indicates is sync;
 *               others indicates not sync;
 *********************************************/
bool dev_is_time_syn(void)
{
    return dev_time_sync;
}

/**********************************************
** function:     dev_last_syn_src
 * description:   
 * input:        none
 * output:       none
 * return:       
 *              
***********************************************/
unsigned char dev_last_syn_src(void)
{
    return last_syn.src;
}


int dev_cmp_basetime(RTCTIME *base, RTCTIME *time)
{
    struct tm tm1, tm2;
    time_t t_sec1, t_sec2;

    tm1.tm_year = base->year - 1900;
    tm1.tm_mon = base->mon - 1;
    tm1.tm_mday = base->mday;
    tm1.tm_hour = base->hour;
    tm1.tm_min = base->min;
    tm1.tm_sec = base->sec;

    tm2.tm_year = time->year - 1900;
    tm2.tm_mon = time->mon - 1;
    tm2.tm_mday = time->mday;
    tm2.tm_hour = time->hour;
    tm2.tm_min = time->min;
    tm2.tm_sec = time->sec;
    t_sec1 = mktime(&tm1);
    t_sec2 = mktime(&tm2);

    return (int)(t_sec2 - t_sec1);
}


