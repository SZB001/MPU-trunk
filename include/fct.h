
/****************************************************************
file:         fct.h
description:  the header file of fct definition
date:         2016/9/25
author        liuzhongwen
****************************************************************/

#ifndef __FCT_H__
#define __FCT_H__
#include "init.h"

typedef enum FCT_ERROR_CODE
{
    FCT_INVALID_PARAMETER = (MPU_MID_FCT << 16) | 0x01,
    FCT_TABLE_OVERFLOW,
    FCT_TABLE_UPDATE_PARA_FAILED,
    FCT_CREATE_THREAD_FAILED,
} FCT_ERROR_CODE;

/* message ID definition */
typedef enum  FCT_MSG_ID
{
    FCT_MSG_FCT_IND = MPU_MID_FCT,
    FCT_MSG_MAX,
} FCT_MSG_ID;

#define  FCT_IN_ANT_TEST_TIMER      1
#define  FCT_AUDIOLOOP_TEST_TIMER   2

#define  FCT_IN_ANT_TEST_VAL        (10*1000)
#define  FCT_AUDIOLOOP_TEST_VAL     (5*1000)

int fct_save_log(unsigned char *msg, unsigned int len);

int fct_init(INIT_PHASE phase);
int fct_run(void);


#endif
