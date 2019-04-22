/****************************************************************
 file:         at_stat.h
 description:  the header file of AT status definition
 date:         2017/02/18
 author        yuzhimin
 ****************************************************************/
#ifndef __AT_STAT_H__
#define __AT_STAT_H__

#include "at_cmd.h"
#include "tbox_limit.h"

#define ATD_ERR_CNT_MAX                 (3)

/* AT moudle status */
typedef enum
{
    AT_OFF_ST = 0,
    AT_RESET_ST,
    AT_INIT_IDLE_ST,
    AT_INIT_SENDING_ST,
    AT_INIT_RETRY_ST,

    AT_NORM_IDLE_ST,
    AT_NORM_SENDING_ST,
    AT_NORM_RETRY_ST,
} AT_STAT_T;

void at_init_stat(void);

AT_STAT_T at_get_stat(void);
void at_set_stat(AT_STAT_T stat);
void at_clean_stat(void);

void at_set_at_lock(void);
void at_set_at_unlock(void);
unsigned int at_get_at_lock(void);

unsigned int at_get_ready_sleep(void);
void at_set_ready_sleep(unsigned char is_ready);

#endif

