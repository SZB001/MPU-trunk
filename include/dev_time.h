#ifndef __DEV_TIME_H__
#define __DEV_TIME_H__

#include <stdbool.h>
#include "timer.h"

typedef enum
{
    UNSET_TIME = 0,
    TSP_TIME_SOURCE,
    GNSS_TIME_SOURCE,
    MCU_RTC_TIME_SOURCE,
    SHELL_TIME_SOURCE,
} SYN_TIME_SOURCE;

/* syn time state */
typedef struct
{
    RTCTIME time;
    SYN_TIME_SOURCE src;
} SYN_TIME_STR;

void dev_time_init(void);
int  dev_req_time(void);
int  dev_syn_time(RTCTIME *time, SYN_TIME_SOURCE src);
bool dev_is_time_syn(void);
bool dev_is_syn_by_gps(void);//lbk add 20180512


int dev_cmp_basetime(RTCTIME *base, RTCTIME *time);

/*Check the range: 2000.1.1 - 2999.12.31 */
static inline int dev_time_chk(unsigned int year,
                               unsigned int mon,
                               unsigned int day,
                               unsigned int hour,
                               unsigned int min,
                               unsigned int sec)
{
    char month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    char leap = 0;

    if ((year < 2000) || (year > 3000))
    {
        return -1;
    }

    if ((mon == 0) || (mon > 12))
    {
        return -1;
    }

    if (mon == 2)
    {
        leap = ((year % 4 == 0) && (year % 100 != 0 || year % 400 == 0));

        if ((day <= 0) || (day > (month[mon] + leap)))
        {
            return -1;
        }
    }
    else
    {
        if ((day <= 0) || day > (month[mon]))
        {
            return -1;
        }
    }

    if (hour >= 24 || min > 59 || sec > 59)
    {
        return -1;
    }

    return 0;
}

#endif

