
/****************************************************************
file:         shm.h
description:  the header file of share memory operation definition
date:         2016/11/22
author        liuzhongwen
****************************************************************/

#ifndef __SHM_H__
#define __SHM_H__

#include <sys/file.h>
#include "com_app_def.h"

void *shm_create(const char *name, int flag, unsigned int size);
int shm_write(void *ptr, unsigned char *data, unsigned int size);
int shm_read(void *ptr, unsigned char *data, unsigned int size);
int shm_release(const char *name, void *ptr, unsigned int size);


#endif

