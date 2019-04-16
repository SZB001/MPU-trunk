/****************************************************************
file:         bcd.c
description:  the source file of bcd code implementation
date:         2016/9/28
author        liuzhongwen
****************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include "log.h"
#include "timer.h"

/****************************************************************
function:     bin2bcd_2dit
description:  convert the last 2 digit number of bin into bcd code
input:        none
output:       none
return:       bcd code with 1 byte length
****************************************************************/
unsigned char bin2bcd_2dit(unsigned long long bin)
{
    unsigned char bcd = 0;

    bin = bin % 100;
    bcd = bin % 10;
    bcd = bcd + (bin / 10) * 0x10;

    return bcd;
}

/****************************************************************
function:     bcd2bin_2dit
description:  convert bcd code into the 2 digit number
input:        none
output:       none
return:       the 2 digit number
****************************************************************/
unsigned char bcd2bin_2dit(unsigned char bcd)
{
    unsigned char bin = 0;

    bin  = (bcd >> 4) * 10;
    bin += (bcd & 0x0F);

    return bin;
}

/****************************************************************
function:     bin2bcd_4dit
description:  convert the last 4 digit number of bin into bcd code
input:        none
output:       none
return:       bcd code with 2 byte length
****************************************************************/
unsigned short bin2bcd_4dit(unsigned long long bin, unsigned char *data)
{
    int i = 0;

    data[i++] = bin2bcd_2dit(bin / 100);
    data[i++] = bin2bcd_2dit(bin);

    return i;
}

/****************************************************************
function:     bcd2bin_4dit
description:  convert bcd code into the 4 digit number
input:        none
output:       none
return:       the 4 digit number
****************************************************************/
unsigned short bcd2bin_4dit(unsigned char *data)
{
    unsigned short bin;

    bin =  bcd2bin_2dit(data[0]) * 100;
    bin += bcd2bin_2dit(data[1]);

    return bin;
}

/****************************************************************
function:     bin2bcd_10dit
description:  convert the last 10 digit number of bin into bcd code
input:        none
output:       none
return:       bcd code buffer length(5 bytes )
****************************************************************/
unsigned short bin2bcd_10dit(unsigned int bin, unsigned char *data)
{
    int i = 0;

    data[i++] = bin2bcd_2dit(bin / 100000000);
    data[i++] = bin2bcd_2dit(bin / 1000000);
    data[i++] = bin2bcd_2dit(bin / 10000);
    data[i++] = bin2bcd_2dit(bin / 100);
    data[i++] = bin2bcd_2dit(bin);

    return i;
}

/****************************************************************
function:     bin2bcd_long
description:  convert longitude into bcd code
input:        none
output:       none
return:       bcd code buffer length(5 bytes )
****************************************************************/
unsigned short bin2bcd_long(double longitude, unsigned char *data, unsigned char is_east)
{
    unsigned int int_pos, i;

    int_pos = (unsigned int)(longitude * 100000);

    i = bin2bcd_10dit(int_pos, data);
    data[0] = data[0] | (is_east ? (0 << 6) : (1 << 6));

    return i;
}

/****************************************************************
function:     bin2bcd_lat
description:  convert latitude into bcd code
input:        none
output:       none
return:       bcd code buffer length(5 bytes )
****************************************************************/
unsigned short bin2bcd_lat(double lat, unsigned char *data, unsigned char is_north)
{
    unsigned int int_pos, i;

    int_pos = (unsigned int)(lat * 100000);

    i = bin2bcd_10dit(int_pos, data);
    data[0] = data[0] | (is_north ? (1 << 6) : (0 << 6));

    return i;
}

/****************************************************************
function:     bin2bcd_time
description:  convert time into bcd code
input:        none
output:       none
return:       bcd code buffer length(6 bytes )
****************************************************************/
unsigned short bin2bcd_time(RTCTIME *time, unsigned char *data)
{
    unsigned int i = 0;

    data[i++] = bin2bcd_2dit(time->year - 2000);
    data[i++] = bin2bcd_2dit(time->mon);
    data[i++] = bin2bcd_2dit(time->mday);
    data[i++] = bin2bcd_2dit(time->hour);
    data[i++] = bin2bcd_2dit(time->min);
    data[i++] = bin2bcd_2dit(time->sec);

    return i;
}

/****************************************************************
function:     bcd2bin_time
description:  convert bcd code into time
input:        unsigned char *data, time with bcd code
output:       RTCTIME *time, binary time
return:       the used data length
****************************************************************/
unsigned short bcd2bin_time(unsigned char *data, RTCTIME *time)
{
    unsigned int i = 0;

    time->year = bcd2bin_2dit(data[i++]) + 2000;
    time->mon  = bcd2bin_2dit(data[i++]);
    time->mday = bcd2bin_2dit(data[i++]);
    time->hour = bcd2bin_2dit(data[i++]);
    time->min  = bcd2bin_2dit(data[i++]);
    time->sec  = bcd2bin_2dit(data[i++]);

    return i ;
}

/****************************************************************
function:     bin2bcd_time_msec
description:  convert time with msec into bcd code
input:        RTCTIME *time, binary time
output:       unsigned char *data, time with bcd code
return:       bcd code buffer length(7 bytes )
****************************************************************/
unsigned short bin2bcd_time_msec(RTCTIME *time, unsigned char *data)
{
    unsigned int i = 0;

    data[i++] = bin2bcd_2dit(time->year - 2000);
    data[i++] = bin2bcd_2dit(time->mon);
    data[i++] = bin2bcd_2dit(time->mday);
    data[i++] = bin2bcd_2dit(time->hour);
    data[i++] = bin2bcd_2dit(time->min);
    data[i++] = bin2bcd_2dit(time->sec);
    data[i++] = bin2bcd_2dit(time->msec / 10);

    return i;
}

/****************************************************************
function:     bcd2bin_time
description:  convert bcd code into time
input:        unsigned char *data, time with bcd code
output:       RTCTIME *time, binary time
return:       the used data length
****************************************************************/
unsigned short bcd2bin_time_msec(unsigned char *data, RTCTIME *time)
{
    unsigned int i = 0;

    time->year = bcd2bin_2dit(data[i++]) + 2000;
    time->mon  = bcd2bin_2dit(data[i++]);
    time->mday = bcd2bin_2dit(data[i++]);
    time->hour = bcd2bin_2dit(data[i++]);
    time->min  = bcd2bin_2dit(data[i++]);
    time->sec  = bcd2bin_2dit(data[i++]);
    time->msec = bcd2bin_2dit(data[i++]) * 10;

    return i;
}

/****************************************************************
function:     bin2bcd_time_year
description:  convert time with complete year into bcd code
input:        none
output:       none
return:       bcd code buffer length(7 bytes )
****************************************************************/
unsigned short bin2bcd_time_year(RTCTIME *time, unsigned char *data)
{
    unsigned int i = 0;

    data[i++] = bin2bcd_2dit(time->year / 100);
    data[i++] = bin2bcd_2dit(time->year);
    data[i++] = bin2bcd_2dit(time->mon);
    data[i++] = bin2bcd_2dit(time->mday);
    data[i++] = bin2bcd_2dit(time->hour);
    data[i++] = bin2bcd_2dit(time->min);
    data[i++] = bin2bcd_2dit(time->sec);

    return i;
}

