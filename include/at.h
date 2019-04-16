/****************************************************************
file:         at.h
description:  the header file of at api definition
date:         2017/7/12
author        wangqinglong
****************************************************************/
#ifndef __AT_H__
#define __AT_H__

#include "mid_def.h"
#include "init.h"

typedef int (*at_sms_arrived)(unsigned char *msg, unsigned int len);


int at_init(INIT_PHASE phase);
int at_run(void);

int at_network_switch(unsigned char type);
int at_get_signal(void);
int at_get_iccid(char *iccid);
int at_get_net_type(void);
int at_get_operator(unsigned char *operator);
int at_get_cfun_status(void);
int at_get_imei(char *imei);
int at_get_imsi(char *imsi);
int at_get_telno(char *telno);
int at_get_pm_mode(void);

#endif
