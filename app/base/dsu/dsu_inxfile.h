/**
 * @Title: dsu_inxfile.h
 * @author yuzhimin
 * @date Nov 9, 2017
 * @version V1.0
 */
#ifndef __DSU_INXFILE_H__
#define __DSU_INXFILE_H__

#include "dsu_comm.h"

#define INX_EDI_TOTAL_SIZE          (1000)
#define INX_EDI_CNT                 (10)
#define INX_EDI_PER_SIZE            (50)
#define INX_CH_BASEINFO_SIZE        (114)

#define INX_CH_TOTAL                (1024)
#define INX_CH_PARA_CNT             (14)
#define INX_CH_CAN_CNT              (INX_CH_TOTAL - INX_CH_PARA_CNT)

/* in channel structure */
typedef struct
{
    const char *name;
    const char *des;
    const char *unit;
    unsigned int bits;
    unsigned int type;
    unsigned int saveType;
    void *valuePtr;
} IN_CH_T;

typedef struct
{
    unsigned int date;
    unsigned int powerStatus;
    unsigned int is_east;
    unsigned int is_north;
    int canBusError[DSU_CAN_MAX_PORT];
    double time;
    double longitude;
    double latitude;
    double msl;
    double direction;
    double kms;
    long long gps_odo;
    unsigned long long SYSRunTime;
    unsigned long long INGPRSFlowCnt;
} INX_RT_DATA_T;    // realtime data

/* inx status structure */
typedef struct
{
    unsigned char enable;
    unsigned char sampling;      // same as sdhz
    unsigned char hourfile;
    unsigned char suspend;         // 0:not suspend,1:suspend;
} INX_ATTR;

int inx_file_init(void);
int inx_file_append(void);
void inx_ch_init(void);
void inx_attr_init(DSU_CFG_T *cfg);

#endif /* __DSU_INXFILE_H__ */

