/****************************************************************
file:         app_shell.h
description:  the header file of app shell api definition
date:         2017/07/12
author        liuwei
****************************************************************/

#ifndef __ASSIST_SHELL_H__
#define __ASSIST_SHELL_H__

#include "mid_def.h"
#include "init.h"


/* initiaze assist shell module */
int assist_shell_init(INIT_PHASE phase);
int app_shell_get_ver(unsigned int argc, unsigned char **argv);
#endif

