
/****************************************************************
file:         bcd.h
description:  the header file of bcd definition
date:         2016/9/20
author        liuzhongwen
****************************************************************/

#ifndef __BCD_H__
#define __BCD_H__

#include "timer.h"

unsigned char bin2bcd_2dit(unsigned long long bin);
unsigned char bcd2bin_2dit(unsigned char bcd);

unsigned short bin2bcd_4dit(unsigned long long bin, unsigned char *data);
unsigned short bcd2bin_4dit(unsigned char *data);

unsigned short bin2bcd_10dit(unsigned int bin, unsigned char *data);

unsigned short bin2bcd_long(double longitude, unsigned char *data, unsigned char is_east);
unsigned short bin2bcd_lat(double lat, unsigned char *data, unsigned char is_north);
unsigned short bin2bcd_time(RTCTIME *time, unsigned char *data);
unsigned short bcd2bin_time(unsigned char *data, RTCTIME *time);
unsigned short bin2bcd_time_msec(RTCTIME *time, unsigned char *data);
unsigned short bcd2bin_time_msec(unsigned char *data, RTCTIME *time);
unsigned short bin2bcd_time_year(RTCTIME *time, unsigned char *data);


#endif
