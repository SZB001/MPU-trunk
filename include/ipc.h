/*
file:         ipc.h
description:  the header file of inter-process communciation definition
date:         2016/11/29
author        liuzhongwen
modifier      liuzhenrong - 2017/11/1
*/

#ifndef __IPC_H__
#define __IPC_H__

#include <stdint.h>

#define IPC_PATH    "/tmp/"

extern int ipc_open(const char *local_name);
extern int ipc_close(int fd);
extern int ipc_send(int fd, const char *proc_name, const uint8_t *data, int len);
extern int ipc_recv(int fd, const char *proc_name, uint8_t *buf, int bufsz);

#endif

