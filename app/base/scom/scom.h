/****************************************************************************
file:         scom.h
description:  the header file of spi communciation definition
date:         2017/07/18
copyright     Wuhan Intest Electronic Technology Co.,Ltd
author        liuzhongwen
*****************************************************************************/

#ifndef __SCOM_H__
#define __SCOM_H__

#include "mid_def.h"
#include "init.h"


/* initiaze scom module*/
int scom_init(INIT_PHASE phase);

/* starup scom module*/
int scom_run(void);

#endif

