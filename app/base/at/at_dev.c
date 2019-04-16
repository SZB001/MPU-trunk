/*******************************************************************
 file:         at_dev.c
 description:  the header file of at device implemention
 date:         2017/1/9
 Copyright     Wuhan Intest Electronic Technology,Co.,Ltd
 author        liuzhongwen
 *******************************************************************/
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "com_app_def.h"
#include "at_dev.h"
#include "at_task.h"
#include "at_api.h"
#include "dev_rw.h"

static int at_port_dev_fd = -1;
static int at_port_recv_cnt = 0;

/******************************************************************
 function:     at_dev_init
 description:  initiaze at device
 input:        INIT_PHASE phase, init phase
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************/
int at_dev_init(INIT_PHASE phase)
{
    int ret;

    ret = at_port_dev_init(phase);

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

/******************************************************************
 function:     at_dev_recv
 description:  receive at commmand from MCU
 and process it
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_dev_recv(int fd, at_handler handler, int *last_recv_cnt, char *recv_buf, int buf_len)
{
    int i, j;
    int recv_cnt;

    static char atcmd[512];

    recv_cnt = read(fd, recv_buf + *last_recv_cnt, buf_len - *last_recv_cnt);

    if (recv_cnt <= 0)
    {
        log_e(LOG_AT, "get data from device error,ret:%s", strerror(errno));
        return;
    }

    log_i(LOG_AT, "recv:%.*s , cnt:%u", recv_cnt + *last_recv_cnt , recv_buf , recv_cnt);
    //log_i(LOG_AT, "at received cnt:%d,last_cnt:%d", recv_cnt, *last_recv_cnt);
    //log_buf_dump(LOG_AT, "AT DATA", (unsigned char *)(recv_buf + *last_recv_cnt), recv_cnt);

    j = 0;

    for (i = 0; i < recv_cnt + *last_recv_cnt && j < sizeof(atcmd); i++)
    {
        /* one at commmand is found */
        if (('\r' == recv_buf[i]) && ('\n' == recv_buf[i + 1])
            && (i + 1 < recv_cnt + *last_recv_cnt))
        {
            atcmd[j++] = '\r';
            atcmd[j++] = '\n';
            handler((unsigned char *) atcmd, j);
            j = 0;
            memset(atcmd, 0, sizeof(atcmd));
            i++; /* skip \n */
        }
        else
        {
            atcmd[j++] = recv_buf[i];
        }
    }

    /* invalid at command */
    if (j >= sizeof(atcmd))
    {
        memset(recv_buf, 0, buf_len);
        *last_recv_cnt = 0;
    }
    else
    {
        /* this is part of one commmand */
        *last_recv_cnt = j;
        memset(recv_buf, 0, buf_len);

        if (j > 0)
        {
            memcpy(recv_buf, atcmd, j);
            memset(atcmd, 0, sizeof(atcmd));
        }
    }
}

/******************************************************************
 function:     at_port_dev_init
 description:  initiaze at device
 input:        INIT_PHASE phase, init phase
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************/
int at_port_dev_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            break;

        default:
            break;
    }

    return 0;
}

/******************************************************************
 function:     at_port_dev_open
 description:  open at uart device
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************/
int at_port_dev_open(int *fd)
{
    *fd = -1;

    at_port_dev_fd = open(AT_SMD_DEV, O_RDWR | O_NONBLOCK | O_NOCTTY);

    if (at_port_dev_fd <= 0)
    {
        log_e(LOG_AT, "open dev failed, ret:%s", strerror(errno));
        return AT_OPEN_DEV_FAILED;
    }

    *fd = at_port_dev_fd;

    return 0;
}

/******************************************************************
 function:     at_port_dev_recv
 description:  receive at commmand from MCU
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_port_dev_recv(void)
{
    static char recv_buf[1024];

    at_dev_recv(at_port_dev_fd, at_handler_from_port, &at_port_recv_cnt, recv_buf,
                sizeof(recv_buf));

    return;
}

/*******************************************************************
 function:     at_port_dev_send
 description:  send at command to 4G module
 input:        unsigned char *buf, at cmd buffer;
 unsigned int len, at cmd length
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************/
int at_port_dev_send(unsigned char *buf, unsigned int len)
{
    log_i(LOG_AT, "write to sdm8:%.*s", len, buf);

    return dev_write(at_port_dev_fd, buf, len);
}

/******************************************************************
 function:     at_port_dev_clean
 description:  clean the buffer in the port device
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************/
int at_port_dev_clean(void)
{
    int bytes = 0;
    static char recv_buf[1024];

    ioctl(at_port_dev_fd, FIONREAD, &bytes);

    while (bytes > 0)
    {
        log_o(LOG_AT, " data len in port buf:%u", bytes);
        read(at_port_dev_fd, recv_buf, bytes);
        ioctl(at_port_dev_fd, FIONREAD, &bytes);
    }

    at_port_recv_cnt = 0;

    return 0;
}

