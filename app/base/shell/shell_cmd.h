/****************************************************************
file:         shell_cmd.h
description:  the header file of shell command definition
date:         2016/11/10
author        liuzhongwen
****************************************************************/

#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__

#include "shell_api.h"



extern int shell_cmd_init(INIT_PHASE phase);
extern int shell_cmd_proc(const char *cmd, int len);


#endif
