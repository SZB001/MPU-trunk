
/****************************************************************
file:         timer.h
description:  the header file of timer definition
date:         2016/9/20
author        liuzhongwen
****************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <signal.h>
#include <time.h>
#include <errno.h>
#include "mid_def.h"

/* RTCTIME structure */
typedef struct
{
    unsigned short    msec;   /* micro second value - [0,999] */
    unsigned char     sec;    /* second value - [0,59] */
    unsigned char     min;    /* minute value - [0,59] */
    unsigned char     hour;   /* hour value - [0,23] */
    unsigned char     mday;   /* day of the month value - [1,31] */
    unsigned char     mon;    /* month value - [1,12] */
    unsigned char     week;  /* unsed */
    unsigned short    year;  /* year value - [0,4095] */
} RTCTIME;

/* time is formated as bytes */
typedef struct
{
    unsigned int  sec: 6;
    unsigned int  min: 6;
    unsigned int  hour: 5;
    unsigned int  mday: 5;
    unsigned int  mon: 4;
    unsigned int  year: 6;
    unsigned char msec: 8;
} BYTESTIME;

typedef enum TIMER_TYPE
{
    TIMER_REL,     /* relative timer */
    TIMER_ABS,      /* absolute timer */
    TIMER_RTC = 8,  /* rtc timer£¬ defined by quectel sdk */
} TIMER_TYPE;

typedef enum TIMER_ERROR_CODE
{
    TIMER_INVALID_PARAMETER  = (MPU_MID_TIMER << 16) | 0x01,
    TIMER_CREATE_TIMER_FAILED,
    TIMER_START_TIMER_FAILED,
    TIMER_STOP_TIMER_FAILED,
    TIMER_DESTORY_TIMER_FAILED,
    TIMER_SET_TIME_FAILED,
} TIMER_ERROR_CODE;

typedef enum TIMER_TIMEOUT_TYPE
{
    TIMER_TIMEOUT_REL_ONCE,    /* one time relative timer */
    TIMER_TIMEOUT_REL_PERIOD,  /* period relative timer */
    TIMER_TIMEOUT_ABS_ONCE,    /* one time absolute timer */
} TIMER_TIMEOUT_TYPE;


#define TIME_TO_BYTES( time, bytes) \
    do{                           \
        ((BYTESTIME*)bytes)->sec  = time.sec;    \
        ((BYTESTIME*)bytes)->min  = time.min;    \
        ((BYTESTIME*)bytes)->hour = time.hour;   \
        ((BYTESTIME*)bytes)->mday = time.mday;   \
        ((BYTESTIME*)bytes)->mon  = time.mon ;   \
        ((BYTESTIME*)bytes)->year = (time.year - 2000);   \
        ((BYTESTIME*)bytes)->msec = (time.msec/10);   \
    }while(0)

int tm_create(TIMER_TYPE type, unsigned short timername, unsigned short mid, timer_t *timer);

int tm_start(timer_t timer, unsigned int interval, TIMER_TIMEOUT_TYPE type);

int tm_stop(timer_t timer);

int tm_destory(timer_t timer);

unsigned long long  tm_get_time(void);

int tm_get_abstime(RTCTIME *abstime);

unsigned long long tm_get_systick(void);

void tm_set_base(unsigned long long time);

#endif

