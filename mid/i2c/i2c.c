#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "log.h"

#define I2C_DEV  "/dev/i2c-2"

int Ql_I2C_Init(unsigned char slaveAddr)
{
    int fd_i2c = open(I2C_DEV, O_RDWR);
    log_i(LOG_MID, "open(I2C_DEV, O_RDWR)=%d", fd_i2c);

    if (fd_i2c < 0)
    {
        log_e(LOG_MID, "Fail to open i2c");
        return -1;
    }

    /*
     * Write slave address
     */
    int iRet = ioctl(fd_i2c, I2C_SLAVE, slaveAddr);

    if (iRet < 0)
    {
        log_e(LOG_MID, "ioctl error");
        return -2;
    }

    return fd_i2c;
}

int Ql_I2C_Read(int fd, unsigned short slaveAddr, unsigned char ofstAddr,  unsigned char *ptrBuff,
                unsigned short length)
{
    int iRet = 0;

    struct i2c_msg i2c_msgs[] =
    {
        [0] = {
            .addr  = slaveAddr,
            .flags = 0, // write
            .buf   = &ofstAddr,
            .len   = 1,
        },
        [1] = {
            .addr  = slaveAddr,
            .flags = I2C_M_RD,
            .buf   = ptrBuff,
            .len   = length,
        },
    };

    struct i2c_rdwr_ioctl_data msgset =
    {
        .msgs = i2c_msgs,
        .nmsgs = 2,
    };

    iRet = ioctl(fd, I2C_RDWR, &msgset);

    if (iRet < 0)
    {
        log_e(LOG_MID, "read failed rc : %d", iRet);
    }

    return iRet;
}


int Ql_I2C_Write(int fd, unsigned short slaveAddr, unsigned char ofstAddr,  unsigned char *ptrData,
                 unsigned short length)
{
    int iRet = 0;

    struct i2c_msg i2c_msgs =
    {
        .addr  = slaveAddr,
        .flags = 0,       // write
        .buf   = NULL,
        .len   = length + 1, // chenyin
    };

    i2c_msgs.buf = (unsigned char *)malloc(length + 1);
    i2c_msgs.buf[0] = ofstAddr; // target address
    memcpy(&i2c_msgs.buf[1], ptrData, length);

    struct i2c_rdwr_ioctl_data msgset =
    {
        .msgs  = &i2c_msgs,
        .nmsgs = 1,
    };

    iRet = ioctl(fd, I2C_RDWR, &msgset);
    free(i2c_msgs.buf);

    if (iRet < 0)
    {
        log_e(LOG_MID, "write failed, iRet=%d", iRet);
    }

    return iRet;
}
