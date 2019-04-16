/****************************************************************
file:         shell_api.h
description:  the header file of shell api definition
date:         2016/11/10
author        liuzhongwen
****************************************************************/

#ifndef __SHELL_API_H__
#define __SHELL_API_H__

#include "mid_def.h"
#include "init.h"

typedef enum SHELL_ERROR_CODE
{
    SHELL_INVALID_PARAMETER = (MPU_MID_SHELL << 16) | 0x01,
    SHELL_FILE_BE_USING,
    SHELL_CREATE_IPC_FAILED,
    SHELL_READ_MSG_FAILED,
    SHELL_CMD_TABLE_OVERFLOW,
    SHELL_CMD_INVALID_LEN,
    SHELL_INVALID_CMD,
} SHELL_ERROR_CODE;

#define shell_cmd_register(name, func, help)    shell_cmd_register_ex(name, NULL, func, help)
#define shellprintf                             shell_cmd_print

extern int shell_init(INIT_PHASE phase);
extern int shell_run(void);
extern int shell_cmd_exec(const char *cmd, char *res, int resl);
extern int shell_cmd_register_ex(char const *name, char const *alis, void *func, char const *help);
extern void shell_cmd_print(const char *fmt, ...);

#endif

