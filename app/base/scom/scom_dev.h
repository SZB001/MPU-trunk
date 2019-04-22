/****************************************************************
file:         scom_dev.h
description:  the header file of spi device definition
date:         2017/07/18
copyright     Wuhan Intest Electronic Technology Co.,Ltd
author        liuzhongwen
****************************************************************/

#ifndef __SCOM_DEV_H__
#define __SCOM_DEV_H__

#include "init.h"

#define SCOM_SPI_DEVICE     "/dev/spichn_1"

int  scom_dev_init(INIT_PHASE phase);
int  scom_dev_open(int *fd);
int  scom_dev_close(void);
void scom_dev_recv(unsigned char *buf, unsigned int *len);
int  scom_dev_send(unsigned char *buf, unsigned int len);
int  scom_dev_clean(void);

#endif

