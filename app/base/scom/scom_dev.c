/*******************************************************************
file:         scom_dev.c
description:  the header file of spi device implemention
date:         2017/1/9
Copyright     Wuhan Intest Electronic Technology Co.,Ltd
author        liuzhongwen
*******************************************************************/
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "com_app_def.h"
#include "dev_rw.h"
#include "scom_dev.h"
#include "scom_api.h"

static int scom_dev_fd = -1;  /* this spi is used to connect with MCU */
static pthread_mutex_t scom_dev_mutex;

/******************************************************************
function:     scom_dev_init
description:  initiaze spi device
input:        INIT_PHASE phase, init phase
output:       none
return:       0 indicates success;
              others indicates failed
*******************************************************************/
int scom_dev_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&scom_dev_mutex, NULL);
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
function:     scom_dev_open
description:  open spi device
input:        none
output:       int *fd, the file descriptor
return:       0 indicates success;
              others indicates failed
*******************************************************************/
int scom_dev_open(int *fd)
{
    pthread_mutex_lock(&scom_dev_mutex);
    scom_dev_fd = open(SCOM_SPI_DEVICE, O_RDWR | O_NONBLOCK | O_NOCTTY);

    if (scom_dev_fd < 0)
    {
        pthread_mutex_unlock(&scom_dev_mutex);
        log_e(LOG_SCOM, "can't open device,ret:%s", strerror(errno));
        return SCOM_OPEN_FAILED;
    }

    *fd = scom_dev_fd;
    pthread_mutex_unlock(&scom_dev_mutex);

    return 0;
}

/******************************************************************
function:     scom_dev_close
description:  close spi device
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*******************************************************************/
int scom_dev_close(void)
{
    pthread_mutex_lock(&scom_dev_mutex);
    close(scom_dev_fd);

    scom_dev_fd = -1;
    pthread_mutex_unlock(&scom_dev_mutex);

    return 0;
}

/******************************************************************
function:     scom_dev_recv
description:  receive spi message from MCU
input:        unsigned char *buf, the message buffer;
              unsigned int *len, the message buffer length
output:       unsigned int *len, the received message length
return:       none
******************************************************************/
void scom_dev_recv(unsigned char *buf, unsigned int *len)
{
    int ret;

    pthread_mutex_lock(&scom_dev_mutex);
    ret = read(scom_dev_fd, buf, *len);
    pthread_mutex_unlock(&scom_dev_mutex);

    if (ret <= 0)
    {
        *len = 0;
    }
    else
    {
        *len = ret;
    }

    log_buf_dump(LOG_SCOM, ">>>>>>>>>>>>>>>>>>(recv)>>>>>>>>>>>>>>>>>", buf, *len);
}

/*******************************************************************
function:     scom_dev_send
description:  send a message to MCU
input:        unsigned char *buf, msg buffer;
              unsigned int len, msg length
output:       none
return:       0 indicates success;
              others indicates failed
*******************************************************************/
int scom_dev_send(unsigned char *buf, unsigned int len)
{
    int ret;

    log_buf_dump(LOG_SCOM, "<<<<<<<<<<<<<(send)<<<<<<<<<<<<<", buf, len);
    pthread_mutex_lock(&scom_dev_mutex);
    ret = dev_write(scom_dev_fd, buf, len);
    pthread_mutex_unlock(&scom_dev_mutex);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send data failed,data:%s,ret:%s", buf, strerror(errno));
    }

    return ret;
}

