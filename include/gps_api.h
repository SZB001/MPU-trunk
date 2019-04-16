/****************************************************************
file:         gps_api.h
description:  the header file of ser_api definition
date:         2016/9/22
author        jinjun
****************************************************************/

#ifndef __GPS_API_H__
#define __GPS_API_H__

#include "mid_def.h"
#include "init.h"

/* GNSS Type */
typedef enum GPS_TYPE
{
    GNSS_EXTERNAL = 0,
    GNSS_4G_MODULE,
} GPS_TYPE;

#define  GNSS_TYPE_SEL  1

#if     GNSS_TYPE_SEL
    #define GNSS_TYPE    GNSS_4G_MODULE
#else
    #define GNSS_TYPE    GNSS_EXTERNAL
#endif

typedef enum GPS_ERROR_CODE
{
    GPS_INIT_FAILED = (MPU_MID_GPS << 16) | 0x01,
    GPS_OPEN_FAILED,
    GPS_INVALID_MSG,
    GPS_INVALID_CS,
    GPS_INVALID_PARAMETER,
    GPS_CREATE_TIMER_FAILED,
    GPS_GET_DISTANCE_FAILED,
    GPS_INVALID_DISTANCE_LEN,
} GPS_ERROR_CODE;

#define GPS_TIMEOUT_4S                  (4000)           // 4S
#define GPS_LOSS_LOCK_TIMEOUT           (300000)         // 300S

typedef enum GPS_MSG_ID
{
    GPS_MSG_ID_TIMER = MPU_MID_GPS,
    GPS_MSG_ID_DATA_TIMER,
    GPS_MSG_ID_CFG_TIMER
} GPS_MSG_ID;

/* GPS status */
typedef enum
{
    GPS_UNCONNECTED = 0,
    GPS_UNFIX,
    GPS_FIX,
    GPS_ERROR
} GPS_FIX_STATUS;

/* gps call back event */
enum
{
    GPS_EVENT_DATAIN = 1,    //p1:gps data(unsigned char*), p2:gps data length
};

typedef struct
{
    unsigned int date;
    double time;
    double longitude;
    unsigned int is_east;
    double latitude;
    unsigned int is_north;
    double direction;
    double knots;       // 1kn = 1 mile/h = 1.852 km/h
    double kms;         // 1km/h = 0.5399kn
    unsigned int stateUsed;
    double msl;
} GPS_DATA;

typedef void (*gps_cb_t)(unsigned int, unsigned int, unsigned int);

/* initiaze gps module */
int gps_init(INIT_PHASE phase);
/* startup gps module */
int gps_run(void);
unsigned int gps_get_fix_status(void);
double gps_get_speed(void);
void gps_reset_distance(void);
long long gps_get_distance(void);
void gps_get_snap(GPS_DATA *gps_data);
int gps_reg_callback(gps_cb_t cb);

#endif /* !__GPS_API_H__ */

