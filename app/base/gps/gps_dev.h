/****************************************************************
 file:         gps_dev.h
 description:  the header file of gps device definition
 date:         2016/11/17
 author        liuzhongwen
 ****************************************************************/

#ifndef __GPS_DEV_H__
#define __GPS_DEV_H__

#include "init.h"
#include <time.h>

#define GPS_NMEA_PORT   "/dev/smd7"
#define GPS_UART_PORT   "/dev/ttyHSL1"

typedef struct gps_dev
{
    int dev_fd;
    timer_t data_timer;
    timer_t cfg_timer;
    timer_t imp_timer; // import ephemeris timer
    unsigned char cfg_step;
    unsigned char *eph_ptr; // ephemeris data ptr
    unsigned char init_ok;//Liu Binkui add for A-GPS
} GPS_DEV, *PGPS_DEV;


int gps_dev_init(INIT_PHASE phase);
int gps_dev_open(void);
int gps_dev_close(void);
int gps_dev_get_fd(void);
void gps_dev_send(unsigned char *data, unsigned int len);
void gps_dev_recv(void);
void gps_dev_timeout(unsigned int time_id);
void gps_set_fix_status(unsigned int status);
void gps_dev_reset(void);
void gps_dev_ubx_init(void);
extern int gps_dev_ubx_import_ehpemeris(const char *file);
int gps_get_ubx_init_sta(void);



#endif
