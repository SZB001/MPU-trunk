
/****************************************************************
file:         tcom.h
description:  the header file of thread communciation definition
date:         2016/9/22
author        liuzhongwen
****************************************************************/

#ifndef __TCOM_H__
#define __TCOM_H__

#define TCOM_QUEUE_BUF_SIZE   (100*1024)
//#define TCOM_QUEUE_BUF_SIZE     (20)


#define TCOM_PIPE_READ      0    /*  the read endian  */
#define TCOM_PIPE_WRITE     1    /*  the write endian */

typedef struct TCOM_PIPE
{
    int fd[2];  /* fd[0] is used for reading and fd[1] is used for writing */
} TCOM_PIPE;

typedef struct TCOM_QUEUE
{
    int head;
    int tail;
    unsigned char buf[TCOM_QUEUE_BUF_SIZE];
} TCOM_QUEUE;

#endif
