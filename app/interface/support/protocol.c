#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "protocol.h"

int protocol_wait_msg(uint16_t mid, int pipe, TCOM_MSG_HEADER *msg, void *msgdata, int timeout)
{
    fd_set set;
    int res;
    struct timeval tv;

    FD_ZERO(&set);
    FD_SET(pipe, &set);
    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    res = select(pipe + 1, &set, NULL, NULL, &tv);

    if (res == 0 || (res < 0 && errno == EINTR))
    {
        memset(msg, 0, sizeof(TCOM_MSG_HEADER));
        return 0;
    }

    if (res > 0)
    {
        if (!FD_ISSET(pipe, &set) || tcom_recv_msg(mid, msg, msgdata))
        {
            memset(msg, 0, sizeof(TCOM_MSG_HEADER));
        }

        return 0;
    }

    return res;
}


void protocol_dump(uint16_t logid, const char *title, const uint8_t *buf, int len, int dir)
{
    #if 0
    unsigned int i, tlen = (strlen(title) + 1) / 2;
    const char *color, *direct;

    if (log_get_level(logid) < LOG_INFO || len <= 0)
    {
        return;
    }

    color  = dir ? "\033[47;32m" : "\033[47;35m";
    direct = dir ? ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" : "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    printf(" %s%-.*s%-*s%.*s\r\n", color, 29 - tlen, direct, tlen * 2, title, 30 - tlen, direct);

    if (save_log)
    {
        fprintf(sdsave, " %-.*s%-*s%.*s\r\n", 29 - tlen, direct, tlen * 2, title, 30 - tlen, direct);
    }

    for (i = 0; i < len; i++)
    {
        printf("%s%02X%s", (i % 20 == 0) ? " " : "", buf[i],
               (i % 20 == 19 || i == len - 1) ? "\r\n" : " ");

        if (save_log)
        {
            fprintf(sdsave, "%s%02X%s", (i % 20 == 0) ? " " : "", buf[i],
                    (i % 20 == 19 || i == len - 1) ? "\r\n" : " ");
        }
    }

    printf("\033[0m\r\n");

    if (save_log)
    {
        fprintf(sdsave, "\r\n");
    }

    #else
    const char *cover;
    uint8_t color;

    color = dir ? LOG_COLOR_GREEN : LOG_COLOR_PURPLE;
    cover = dir ? ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" : "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    log_dump(logid, LOG_INFO, color, title, cover, buf, len);
    #endif
}


