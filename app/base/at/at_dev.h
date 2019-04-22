/****************************************************************
 file:         at_dev.h
 description:  the header file of at device definition
 date:         2017/1/9
 author        liuzhongwen
 ****************************************************************/

#ifndef __AT_DEV_H__
#define __AT_DEV_H__

#include "init.h"

#define AT_UART_DEV    "/dev/ttyHSL1\0"
#define AT_SMD_DEV    "/dev/smd8"

#define AT_GNSS_DEV    "/dev/smd7"

typedef void (*at_handler)(unsigned char *buf, unsigned int len);

int at_dev_init(INIT_PHASE phase);

int at_port_dev_init(INIT_PHASE phase);
int at_port_dev_open(int *fd);
void at_port_dev_recv(void);
int at_port_dev_send(unsigned char *buf, unsigned int len);
int at_port_dev_clean(void);

#endif

