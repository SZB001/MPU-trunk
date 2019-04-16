/****************************************************************
file:         shell_cmd.c
description:  the header file of shell command implemention
date:         2016/11/10
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdarg.h>
#include <pthread.h>
#include "dir.h"
#include "file.h"
#include "ipc.h"
#include "timer.h"
#include "shell_cmd.h"
#include "shell_api.h"
#include "cfg_api.h"
#include "diag.h"
#include "fault_sync.h"
#include "status_sync.h"
#include "tcom_api.h"
#include "dev_time.h"
#include "log.h"
#include <ctype.h>

#define SHELL_MAX_ARG_NUM   7
#define SHELL_MAX_CMD_NUM   164
#define SHELL_MAX_CMD_LEN   TCOM_MAX_MSG_LEN


typedef struct
{
    char const *name;
    char const *alis;
    char const *help;
    int (*func)(int, char const **);
} shell_cmd_t;

typedef struct
{
    int cnt;
    shell_cmd_t cmd[SHELL_MAX_CMD_NUM];
} shell_tbl_t;

static shell_tbl_t     shell_tbl;
static pthread_mutex_t shell_resmtx;
static char *shell_resbuf;
static int   shell_resbsz;

/*
function:     shell_cmd_register
description:  register shell commmand
input:        name  - command name, must be static string constant
              func  - function pointer
              char  - help information, must be static string constant
return:       0     - success;
              other - failed
*/
int shell_cmd_register_ex(char const *name, char const *alis, void *func, char const *help)
{
    shell_cmd_t *cmd;

    assert(name != NULL && func != NULL && help != NULL);

    if (shell_tbl.cnt >= SHELL_MAX_CMD_NUM)
    {
        log_e(LOG_SHELL, "shell command table is full");
        return -1;
    }

    cmd = shell_tbl.cmd + shell_tbl.cnt++;
    cmd->name = name;
    cmd->alis = alis;
    cmd->help = help;
    cmd->func = func;
    return 0;
}

/****************************************************************
function:     shell_cmd_read
description:  read shell command from fd
input:        none
output:       none
return:       >0 read length;
              =0 read nothing
              <0 error happened
*****************************************************************/


/****************************************************************
function:     shell_cmd_help
description:  print the help info of all command
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static void shell_cmd_help(void)
{
    int i;

    shellprintf(" shell command list (total: %d)\r\n", shell_tbl.cnt);
    shellprintf("  command name      alias name        description\r\n");
    shellprintf("  ------------      ----------        -----------\r\n");

    for (i = 0; i < shell_tbl.cnt; i++)
    {
        shellprintf("  %-16s  %-16s  %s\r\n", shell_tbl.cmd[i].name,
                    shell_tbl.cmd[i].alis ? shell_tbl.cmd[i].alis : "N/A",
                    shell_tbl.cmd[i].help);
    }
}

/****************************************************************
function:     shell_cmd_proc
description:  process shell command
input:        unsigned char *cmd, cmd and para
output:       unsigned int len, cmd and para length
return:       none
*****************************************************************/
int shell_cmd_proc(char const *cmd, int len)
{
    int i, argc = 0, ret = -1;
    char const *argv[SHELL_MAX_ARG_NUM], *parg;

    assert(cmd != NULL);
    assert(len > 0);
    assert(cmd[len - 1] == 0);

    for (i = 0, argc = 0, parg = cmd; i < len; i++)
    {
        if ('\0' == cmd[i])
        {
            if (argc >= SHELL_MAX_ARG_NUM)
            {
                shellprintf(" too many arguments\r\n");
                return -1;
            }

            argv[argc++] = parg;
            parg = cmd + i + 1;
        }
    }

    for (i = 0; i < shell_tbl.cnt; i++)
    {
        if (strcmp(shell_tbl.cmd[i].name, argv[0]) == 0 ||
            (shell_tbl.cmd[i].alis != NULL && strcmp(shell_tbl.cmd[i].alis, argv[0]) == 0))
        {
            ret = shell_tbl.cmd[i].func(argc - 1, argv + 1);
            break;
        }
    }

    if (i >= shell_tbl.cnt)
    {
        shell_cmd_help();
    }

    return ret;
}



int shell_cmd_exec(const char *cmd, char *res, int resl)
{
    int ret = -1, argc, i;
    const char *argv[SHELL_MAX_ARG_NUM];
    char *argp, tmp[512];
    shell_cmd_t *pcmd = NULL;

    assert(cmd != NULL);
    assert(strlen(cmd) < 512);

    for (argc = 0, argp = tmp; argc < SHELL_MAX_ARG_NUM; argc++)
    {
        while (*cmd && isspace(*cmd))
        {
            cmd++;
        }

        if (*cmd)
        {
            const char *pos = cmd;

            while (*cmd && !isspace(*cmd))
            {
                cmd++;
            }

            argv[argc] = argp;
            strncpy(argp, pos, cmd - pos);
            argp[cmd - pos] = 0;
            argp += cmd - pos + 1;
        }
        else
        {
            break;
        }
    }

    while (*cmd && isspace(*cmd))
    {
        cmd++;
    }

    if (*cmd)
    {
        return -1;
    }

    for (i = 0; i < shell_tbl.cnt; i++)
    {
        if (strcmp(shell_tbl.cmd[i].name, argv[0]) == 0 ||
            (shell_tbl.cmd[i].alis && strcmp(shell_tbl.cmd[i].alis, argv[0]) == 0))
        {
            pcmd = shell_tbl.cmd + i;
            break;
        }
    }

    if (!pcmd)
    {
        return -1;
    }

    if (res && resl > 0)
    {
        pthread_mutex_lock(&shell_resmtx);
        shell_resbuf = res;
        shell_resbsz = resl;

        ret = pcmd->func(argc - 1, argv + 1);

        if (shell_resbsz == resl)
        {
            res[0] = 0;
        }

        shell_resbuf = NULL;
        pthread_mutex_unlock(&shell_resmtx);
    }
    else
    {
        ret = pcmd->func(argc - 1, argv + 1);
    }

    return ret;
}



void shell_cmd_print(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    log_raw(LOG_COLOR_BLUE, fmt, args);
    va_end(args);

    if (shell_resbuf && shell_resbsz > 1)
    {
        int size;

        va_start(args, fmt);
        size = vsnprintf(shell_resbuf, shell_resbsz, fmt, args);
        va_end(args);

        if (size >= shell_resbsz)
        {
            size = shell_resbsz - 1;
        }

        if (size > 0)
        {
            shell_resbuf += size;
            shell_resbsz -= size;
        }
    }
}

/****************************************************************
function:     shell_cmd_set_loglvl
description:  set the specified module log level
input:        unsigned char *cmd, cmd and para
output:       unsigned int len, cmd and para length
return:       none
*****************************************************************/
static int shell_cmd_setlvl(int argc, char const **argv)
{
    int mid, level;

    if (argc != 2)
    {
        shellprintf(" usage: loglv <module name> <log level>\r\n");
        shellprintf("     log level: e  - error level\r\n");
        shellprintf("                w  - warnning level\r\n");
        shellprintf("                i  - information level\r\n");
        shellprintf("                d  - debug level\r\n");
        return -1;
    }

    switch (argv[1][0])
    {
        case 'd':
        case 'D':
            level = LOG_DEBUG;
            break;

        case 'i':
        case 'I':
            level = LOG_INFO;
            break;

        case 'e':
        case 'E':
            level = LOG_ERROR;
            break;

        case 'w':
        case 'W':
        case 'l':
        case 'L':
            level = LOG_WARNING;
            break;

        default:
            level = -1;
            break;
    }

    if (level < 0 || argv[1][1] != 0)
    {
        shellprintf(" error: unknown log level \"%s\"\r\n", argv[1]);
        shellprintf(" usage: loglv <module name> <log level>\r\n");
        shellprintf("     log level: e  - error level\r\n");
        shellprintf("                w  - warnning level\r\n");
        shellprintf("                i  - information level\r\n");
        shellprintf("                d  - debug level\r\n");
        return -1;
    }

    if ((mid = log_get_id(argv[0])) < 0)
    {
        shellprintf(" error: module name \"%s\" not found\r\n", argv[0]);
        return -1;
    }

    return log_set_level(mid, level);
}

/****************************************************************
function:     shell_cmd_init
description:  initiaze shell cmd sub module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int shell_cmd_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(&shell_tbl, 0 , sizeof(shell_tbl));
            pthread_mutex_init(&shell_resmtx, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            shell_cmd_register_ex("loglv", "setdbg", shell_cmd_setlvl, "set module log level");

            break;

        default:
            break;

    }

    return 0;
}

