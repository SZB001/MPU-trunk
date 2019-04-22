/****************************************************************
file:         main.c
description:  the source file of shell client implemention
date:         2016/11/10
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "ipc.h"

#define SHELL_MAX_CMD_LEN         (1024)

int main(int argc, char **argv)
{
    int ret, fd, i, cmd_len, para_len;
    char cmd[SHELL_MAX_CMD_LEN];

    if (argc <= 1)
    {
        printf("\r\n[shell client]parameter num must be more than 1 \r\n");
        return 1;
    }

    fd = ipc_open(COM_APP_SHELL_CTL_NAME);

    if (fd < 0)
    {
        printf("\r\n[shell client]open ipc(%s) failed, error:%s\r\n", COM_APP_SHELL_CTL_NAME,
               strerror(errno));
        return 1;
    }

    cmd_len = 0;

    for (i = 1; i < argc; i++)
    {
        para_len = strlen(argv[i]);

        if (cmd_len + para_len >= SHELL_MAX_CMD_LEN)
        {
            printf("\r\n[shell client]cmd is to long:%u\r\n", cmd_len + para_len);
            close(fd);
            return 1;
        }

        memcpy(cmd + cmd_len, argv[i], para_len);
        cmd_len = cmd_len + para_len;

        cmd[cmd_len] = '\0';
        cmd_len++;

        //printf("\r\n[shell client] index:%u,para:%s", i, argv[i] );
    }

    ret = ipc_send(fd, COM_APP_SHELL_CMD_NAME, (unsigned char *)cmd, cmd_len);

    if (ret != cmd_len)
    {
        printf("\r\n[shell client] ipc send failed\r\n");
    }
    else
    {
        //printf("\r\n[shell client] ipc send succeed\r\n" );
    }

    close(fd);

    return 0;
}
