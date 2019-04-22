/****************************************************************
file:         upg.h
description:  the header file of at api definition
date:         2017/7/12
author        wangqinglong
****************************************************************/
#ifndef __UPG_H__
#define __UPG_H__

#include "com_app_def.h"
#include "mid_def.h"
#include "init.h"
#include "dev_api.h"


/* initiaze upgrade module*/
int dev_init(INIT_PHASE phase);

/* starup upgrade module*/
int dev_run(void);


int upg_ctl_scom_msg_proc(unsigned char *msg, unsigned int len);


#endif

