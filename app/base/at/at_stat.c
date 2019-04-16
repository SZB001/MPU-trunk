/****************************************************************
 file:         at_stat.c
 description:  the source file of at status implementation
 date:         2017/02/21
 author        yuzhimin
 ****************************************************************/
#include "stdio.h"
#include "string.h"
#include "at_stat.h"
#include "at_data.h"
#include "at_queue.h"
#include "at.h"

static AT_STAT_T at_stat;

/******************************************************************
 function:     at_get_stat
 description:  get the state of at thread
 input:        none
 output:       none
 return:       AT_STAT_T
 ******************************************************************/
AT_STAT_T at_get_stat(void)
{
    return at_stat;
}

/******************************************************************
 function:     at_set_stat
 description:  set the state of at thread
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_set_stat(AT_STAT_T stat)
{
    at_stat = stat;
}

/******************************************************************
 function:     at_init_stat
 description:  init the state of at thread
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_init_stat(void)
{
    at_stat = AT_OFF_ST;
}

/******************************************************************
 function:     at_clean_stat
 description:  clean the state of at thread
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_clean_stat(void)
{
    memset(&at_para, 0, sizeof(AT_CFG_T));
    memset(&at_info, 0, sizeof(AT_INFO_T));
    memset(&at_call, 0, sizeof(AT_CALL_T));
    memset(&at_wifi, 0, sizeof(AT_WIFI_T));
    memset(&at_fct, 0, sizeof(AT_FCT_T));
}

/******************************************************************
 function:     at_get_ready_sleep
 description:  get whether is ready to sleep
 input:        none
 output:       none
 return:       unsigned int
 ******************************************************************/
unsigned int at_get_ready_sleep(void)
{
    return at_info.is_ready_sleep;
}

/******************************************************************
 function:     at_set_ready_sleep
 description:  set whether is ready to sleep
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_set_ready_sleep(unsigned char is_ready)
{
    at_info.is_ready_sleep = is_ready;
}

/******************************************************************
 function:     at_set_at_lock
 description:  set the queue lock state
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_set_at_lock(void)
{
    at_info.at_lock = 1;
}

/******************************************************************
 function:     at_set_at_lock
 description:  set the queue lock state
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_set_at_unlock(void)
{
    at_info.at_lock = 0;
}

/******************************************************************
 function:     at_get_at_lock
 description:  get the queue whether is locked
 input:        none
 output:       none
 return:       unsigned int
 ******************************************************************/
unsigned int at_get_at_lock(void)
{
    return at_info.at_lock;
}

