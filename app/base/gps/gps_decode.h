/****************************************************************
 file:         gps_decode.h
 description:  the header file of gps decode definition
 date:         2016/11/17
 author        liuzhongwen
 ****************************************************************/

#ifndef __GPS_DECODE_H__
#define __GPS_DECODE_H__

#include "init.h"

int gps_decode_init(INIT_PHASE phase);
int gps_decode(unsigned char *buf);
int gps_decode_save(unsigned long long distance);
int gps_decode_restore(void);
void gps_acc_distance(int distance);
void gps_save_distance(void);
int gps_shell_init(void);

#endif

