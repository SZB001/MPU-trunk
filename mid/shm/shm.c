/*****************************************************************************
file:         shm.c
description:  the source file of share memory implementation
date:         2016/11/22
author        liuzhongwen
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "shm.h"

/*****************************************************************************
function:     shm_create
description:  create share memory
input:        const char* name, the share memory name
              unsigned int size, the share memory size
output:       none
return:       others indicates success.
              NULL indicates failed.
*****************************************************************************/
void *shm_create(const char *name, int flag, unsigned int size)
{
    int fd, ret;
    void *ptr;

    /* O_CREAT | O_TRUNC | O_RDWR */
    fd = shm_open(name, flag, 0666);

    if (fd < 0)
    {
        log_e(LOG_MID, "create shm(%s) failed, error:%s", name, strerror(errno));
        return NULL;
    }

    ret = ftruncate(fd, size);

    if (ret < 0)
    {
        log_e(LOG_MID, "ftruncate shm(%s) failed, error:%s", name, strerror(errno));
        close(fd);
        return NULL;
    }

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (ptr == MAP_FAILED)
    {
        log_e(LOG_MID, "mmap shm(%s) failed, error:%s", name, strerror(errno));
        close(fd);
        return NULL;
    }

    return ptr;
}

/*****************************************************************************
function:     shm_write
description:  write data into share memory
input:        void *ptr, the share memroy address;
              unsigned char *data, the data buffer;
              unsigned int size, the data size to write;
output:       none
return:       0 indicates success.
              -1 indicates failed.
*****************************************************************************/
int shm_write(void *ptr, unsigned char *data, unsigned int size)
{
    if (NULL == ptr)
    {
        log_e(LOG_MID, "invalid address");
        return -1;
    }

    memcpy(ptr, data, size);

    return 0;
}

/*****************************************************************************
function:     shm_read
description:  read data from share memory
input:        void *ptr, the share memroy address;
              unsigned char *data, the data buffer;
              unsigned int size, the data size to read;
output:       none
return:       0 indicates success.
              -1 indicates failed.
*****************************************************************************/
int shm_read(void *ptr, unsigned char *data, unsigned int size)
{
    if (NULL == ptr)
    {
        log_e(LOG_MID, "invalid address");
        return -1;
    }

    memcpy(data, ptr, size);

    return 0;
}

/*****************************************************************************
function:     shm_release
description:  release the share memory
input:        const char* name, the share memory name;
              void *ptr, the share memory address;
              unsigned int size, the share memory size
output:       none
return:       0 indicates success.
              -1 indicates failed.
*****************************************************************************/
int shm_release(const char *name, void *ptr, unsigned int size)
{
    int ret;

    if (NULL == ptr)
    {
        log_e(LOG_MID, "invalid address");
        return -1;
    }

    ret = munmap(ptr, size);

    if (ret < 0)
    {
        log_e(LOG_MID, "munmap shm(%s) failed, error:%s", name, strerror(errno));
    }

    ret = shm_unlink(name);

    if (ret < 0)
    {
        log_e(LOG_MID, "unlink shm(%s) failed, error:%s", name, strerror(errno));
        return -1;
    }

    return 0;
}

