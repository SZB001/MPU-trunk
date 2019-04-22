#ifndef __GPIO_OE_H__
#define __GPIO_OE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <dirent.h>
#include <comdef.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <malloc.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef POLLRDHUP
#define POLLRDHUP       0x2000
#endif


/****************************************************************************
 * Error Code Definition
 ***************************************************************************/
enum
{
    RES_OK = 0,
    RES_BAD_PARAMETER  = -1,     ///< Parameter is invalid.
    RES_IO_NOT_SUPPORT = -2,
    RES_IO_ERROR = -3,
    RES_NOT_IMPLEMENTED = -4
};

#ifdef __cplusplus
}
#endif

#endif  //__QL_OE_H__
