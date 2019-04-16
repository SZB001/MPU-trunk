/****************************************************************
 file:         at_task.h
 description:  the header file of AT task definition
 date:         2017/07/11
 author        wangqinglong
 ****************************************************************/
#ifndef __AT_TASK_H__
#define __AT_TASK_H__

#include "at.h"
#include "at_cmd.h"
#include "at_api.h"

#define AT_SEND_INTERVAL   (100)

#define AT_INIT_INTERVAL   (3000)
#define AT_POLL_INTERVAL   (3000)

#define AT_TIMEOUT1                     3000            //(3S)
#define AT_TIMEOUT2                     5000            //(5S)
#define AT_TIMEOUT3                     8000            //(8S)
#define AT_TIMEOUT4                     10000           //(10S) 

#define AT_MODEM_TIMEOUT_MAX_CNT        15

#if 1
#define AT_AP_TIMEOUT_MAX_CNT           600             //  at least 30min
#else
#define AT_AP_TIMEOUT_MAX_CNT           20              //  at least 1min for test only
#endif

/* 4G module at status */
typedef enum AT_4G_STATUS
{
    AT_4G_UNKNOW = 0,
    AT_4G_NORMAL,
    AT_4G_FAULT,
} AT_4G_STATUS;

int at_task_init(void);
void at_task(AT_MSG_EVENT msg_id);
void at_sleep_proc(void);
void at_wakeup_proc(void);
void at_send_enable(const AT_CMD_T *at, AT_PRIORITY prior);
void at_handler_from_port(unsigned char *buf, unsigned int len);

#endif
