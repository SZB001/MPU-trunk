/*
file:         ipc.c
description:  the source file of inter-process communciation implementation
date:         2016/11/29
author        liuzhongwen
modifier      liuzhenrong - 2017/11/1
****************************************************************/
#include <sys/un.h>
#include <sys/socket.h>
#include "com_app_def.h"
#include "ipc.h"

/*
function:     ipc_open
description:  create af_unix socket and bind it
input:        [name]    local process name
return:       [<0]      failed
              [other]   file descripter
*/
int ipc_open(const char *local_name)
{
    int fd;
    struct sockaddr_un local;

    assert(local_name != NULL);
    assert(strlen(IPC_PATH) + strlen(local_name) < sizeof(local.sun_path));

    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        log_e(LOG_MID, "create socket failed, error: %s", strerror(errno));
    }
    else
    {

        int try;

        memset(&local, 0, sizeof(local));
        sprintf(local.sun_path, "%s%s", IPC_PATH, local_name);
        local.sun_family = AF_UNIX;
        unlink(local.sun_path);

        for (try = 3;
                 try > 0;
                     try--)
                    {
                        if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0)
                        {
                            if (EADDRINUSE == errno)
                            {
                                unlink(local.sun_path);
                            }
                            else
                            {
                                log_e(LOG_MID, "bind failed, name: %s, error: %s",
                                      local.sun_path, strerror(errno));
                                close(fd);
                                return -1;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
    }

    return fd;
}

/*
function:     ipc_close
description:  close af_unix socket
input:        [fd]      socket descriptor
return:       [0]       success
              [-1]      failed
*/
int ipc_close(int fd)
{
    return close(fd);
}

/*
function:     ipc_send
description:  send data to a process
input:        [fd]          socket descriptor
              [proc_name]   target process name
              [data]        data
              [len]         length of data
return:       [<0]          failed
              [other]       length that has been sent
*/
int ipc_send(int fd, const char *proc_name, const uint8_t *data, int len)
{
    int res, wlen = 0;
    struct sockaddr_un peer;

    assert(proc_name != NULL && data != NULL);
    assert(strlen(IPC_PATH) + strlen(proc_name) < sizeof(peer.sun_path));

    memset(&peer, 0, sizeof(peer));
    sprintf(peer.sun_path, "%s%s", IPC_PATH, proc_name);
    peer.sun_family = AF_UNIX;

    while (wlen < len)
    {
        res = sendto(fd, data, len - wlen, 0,
                     (const struct sockaddr *)(&peer), sizeof(peer));

        if (res < 0)
        {
            if (EINTR != errno) /* interrupted by signal */
            {
                log_e(LOG_MID, "send data failed, name: %s, error: %s",
                      proc_name, strerror(errno));
                return -1;
            }
        }
        else
        {
            wlen += res;
        }
    }

    return wlen;
}

/*
function:     ipc_recv
description:  receive data form a process
input:        [fd]          socket descriptor
              [proc_name]   source process name
              [buf]         data buffer
              [bufsz]       size of data buffer
return:       [<0]          failed
              [other]       length that has been received
*/
int ipc_recv(int fd, const char *proc_name, uint8_t *buf, int bufsz)
{
    int res;
    struct sockaddr_un peer;
    socklen_t socklen = sizeof(peer);

    assert(proc_name != NULL && buf != NULL);
    assert(strlen(IPC_PATH) + strlen(proc_name) < sizeof(peer.sun_path));

    memset(&peer, 0, sizeof(peer));
    sprintf(peer.sun_path, "%s%s", IPC_PATH, proc_name);
    peer.sun_family = AF_UNIX;

    if ((res = recvfrom(fd, buf, bufsz, 0, (struct sockaddr *)(&peer), &socklen)) < 0)
    {
        log_e(LOG_MID, "recv data failed, name: %s, error: %s",
              proc_name, strerror(errno));
        return -1;
    }

    return res;
}

