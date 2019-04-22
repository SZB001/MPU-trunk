
/****************************************************************
file:         mid_def.h
description:  the header file of module id definition
date:         2016/9/22
author        liuzhongwen
****************************************************************/

#ifndef __MID_DEF_H__
#define __MID_DEF_H__
#include <sys/prctl.h>
/*
   MID = cpuid + main module id + sub module id
   cpuid: bit15 and bit14, 0 indicate MPU, 1 indicate mcu
   main module id:bit13 to bit 8
   sub module id:bit7 to bit 0
*/

#define MAIN_MID_MASK              0x3f00   /* bit13 to bit 8 is main module id */
#define CPUID_MASK                 0xc000   /* bit15 and bit14 is cpuid */
#define SUB_MID_MASK               0x00ff   /* bit7 to bit0 is sub module id */
#define CPUID_MAIN_MID_MASK        0xff00   /* bit15 to bit 0 is main module id and cpuid */

#define MPU_ID                     (0)      /*cpuid for MPU*/
#define MCU_ID                     (1)      /*cpuid for MCU*/

#define MID_TO_INDEX(MID)      (((MID & MAIN_MID_MASK) >> 8) - 1)
#define INDEX_TO_MID(INDEX)    (((INDEX+1) << 8) & MAIN_MID_MASK)


/*----------------MPU main module definition begin---------------------------- */
#define MPU_MID_TIMER         0x0100   /* os timer main module */
#define MPU_MID_CFG           0x0200   /* configuration management main module */
#define MPU_MID_DEV           0x0300   /* OTA upgrade management main module */
#define MPU_MID_AT            0x0400   /* AT process main module */
#define MPU_MID_NM            0x0500   /* data forward main module */
#define MPU_MID_SHELL         0x0600   /* debug management main module */
#define MPU_MID_TCOM          0x0700   /* thread com management main module */
#define MPU_MID_PM            0x0800   /* power management module */
#define MPU_MID_GPS           0x0900   /* GPS management module */
#define MPU_MID_CAN           0x0A00   /* GPS management module */
#define MPU_MID_SCOM          0x0B00   /* spi com management module */
#define MPU_MID_UDS           0x0C00   /* UDS app stack module */
#define MPU_MID_ASSIST        0x0D00   /* tbox assist management module */
#define MPU_MID_GB32960       0x0E00   /* GB32960 protocal module */
#define MPU_MID_DSU           0x0F00   /* data Storage unit module*/
#define MPU_MID_FCT           0x1000   /* FCT */
#define MPU_MID_FOTON         0x1100   /* for Foton OTA */
#define MPU_MID_FOTA          0x1200
#define MPU_MID_GEELYHU       0x1300
#define MPU_MID_AUTO          0x1400   /* for autopilot tsp */

#define MPU_MIN_MID           MPU_MID_TIMER
#define MPU_MAX_MID           MPU_MID_AUTO

#define MPU_APP_MID_COUNT     (((MPU_MAX_MID - MPU_MIN_MID) >> 8) + 1)

#define MPU_MID_MID           0x3f00   /* middle module */
#define MPU_MID_MID_IPC       0x3f01   /* ipc middle module */
#define MPU_MID_MID_FILE      0x3f02   /* file middle module */
#define MPU_MID_MID_DIR       0x3f03   /* dir middle module */
#define MPU_MID_MID_DEV_RW    0x3f04   /* dev rw middle module */
#define MPU_MID_MID_RDS       0x3f05   /* rds middle module */
#define MPU_MID_MID_PWDG      0x3f06   /* middle module */



/*----------------main module definition end---------------------------- */

/*----------------MCU main module definition begin---------------------------- */
#define MCU_MID_PROXY         0x8100   /* configuration management main module */
/*----------------MCU main module definition end------------------------------ */

#endif

