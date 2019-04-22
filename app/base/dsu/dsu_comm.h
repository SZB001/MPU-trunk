/**
 * @Title: dsu_comm.h
 * @author yuzhimin
 * @date Nov 9, 2017
 * @version V1.0
 */
#ifndef __DSU_COMM_H__
#define __DSU_COMM_H__

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <zlib.h>

#include "can_api.h"
#include "com_app_def.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "dev_time.h"
#include "diag.h"
#include "dir.h"
#include "dsu_api.h"
#include "file.h"
#include "gps_api.h"
#include "init.h"
#include "log.h"
#include "md5.h"
#include "shell_api.h"
#include "tcom_api.h"
#include "pm_api.h"

#define DSU_CAN_MAX_PORT            3           // This is the temporary definition
#define DSU_TBOX_REBOOT_CNT_MAX     1000000000  // This is the temporary definition

#define DSU_ZBUF_SIZE               (1024*64)

#define MCU_TICK_CLK                (5)         //80Mhz->5Mhz(DIV16)

#define DSU_INX_HEADER_SIZE         (33)
#define DSU_INZ_HEADER_SIZE         (23)
#define DSU_INR_HEADER_SIZE         (7)
#define DSU_IWD_HEADER_SIZE         (26)

#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/* support file type */
typedef enum
{
    DSU_FILE_INX = 0,
    DSU_FILE_IWDZ,
    DSU_FILE_IWD,
} DSU_FILE_TYPE;

/* parameter type */
enum
{
    TYPE_UINT = 0,
    TYPE_SINT,
    TYPE_FLAT,
    TYPE_DBLE,
    TYPE_LLONG,
};

/* save type */
enum
{
    STYPE_DOUBLE = 0,
    STYPE_CHAR,
    STYPE_FLOAT,
};

enum
{
    IWDZ_DATA_CAN = 0,
    IWDZ_DATA_GNSS,
    IWDZ_DATA_END,      // End record, clear cache data
};

/* fwrite result */
enum
{
    FR_ERR = -10,
    FR_ERR_FSEEK,
    FR_ERR_FTELL,
    FR_ERR_COMPRESS,
    FR_ERR_WRITE,
    FR_ERR_OVERFLOW,
    FR_ERR_OPEN,
    FR_OK = 0,
    FR_OK_WATER_MARK,
};

/* disk status */
enum
{
    DISK_OK = 1,
    DISK_FULL,
    DISK_ERROR,
};

/* disk status */
enum
{
    CANBUS_TIMEOUT = 0,
    CANBUS_ACTIVE,
};

#define DSU_CFG_INX_MASK                0x01u
#define DSU_CFG_IWDZ_MASK               0x02u
#define DSU_CFG_IWD_MASK                0x04u

typedef struct
{
    unsigned char sdhz;                 // inx Sampling frequency
    unsigned char hour;                 // bit0:inx, bit1:iwdz
    unsigned char loop;
    unsigned char canlog_mode;          // bit0:inx,bit1:iwdz,bit2:iwd; 0:diable,1:enable;
    short canlog_time;                  // the time before and after the failure. for iwd file
    unsigned char reserve[2];
} DSU_CFG_T;

/* message ID definition */
typedef enum
{
    DSU_MSG_CREATE_FILE = (MPU_MID_DSU << 16) | 0x01,
    DSU_MSG_WRITE_FILE,
    DSU_MSG_SYNC_FILE,
    DSU_MSG_CLOSE_FILE,
    DSU_MSG_MAX,
} DSU_MSG_ID;

#define DSU_TIMER                  1
#define DSU_TIMER_INTERVAL         1000
#define INX_FILE_TIMER             2
#define INX_FILE_TIMEOUT           2000

#define DSU_IWDZ_FILE_PATH          COM_SDCARD_DIR"/data"
#define DSU_INX_FILE_PATH           COM_SDCARD_DIR"/data"
#define DSU_IWD_FILE_PATH           COM_SDCARD_DIR"/canlog"
#define DSU_IWD_TMP_FILE_NAME       "tmp.iwd"

#define IWDZ_VER_STR                "INTEST IWD V003"
#define IWD_VER_STR                 "INTESTRIWD V003"

#define DSU_RESERVE_DISK_SIZE       (20)    // 20MB
#define DSU_G_CANTAG_HZ             (100)   // 100hz

int dsu_build_head_inx(unsigned char *header, unsigned int size, unsigned int sn);
int dsu_build_head_inz(unsigned char *header, unsigned int size, unsigned int sn);
int dsu_build_head_inr(unsigned char *header, unsigned int size);
int dsu_build_head_iwd(unsigned char *header, unsigned int size, char *ver,
                       unsigned int sn, RTCTIME *time, unsigned char clk);

#endif /* __DSU_COMM_H__ */

