#ifndef __UDS_TIMER_H__
#define __UDS_TIMER_H__
#include <sys/time.h>
#include <pthread.h>

typedef struct
{
    timer_t  tm;
    uint16_t idx;
    uint16_t seq;
} uds_timer_t;

extern int uds_timer_create(uds_timer_t *tm, int idx);
extern int uds_timer_settime(uds_timer_t *tm, int time);

#endif
