/****************************************************************
 file:         dev_shell.h
 description:  the header file of nm shell commmand definition
 date:         2017/07/09
 author        wangqinglong
 ****************************************************************/
#ifndef __DEV_SHELL_H__
#define __DEV_SHELL_H__

#include "init.h"

int dev_shell_init(INIT_PHASE phase);
int dev_shell_mcu_shell_res(unsigned char *msg, unsigned int len);

#endif
