/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2016
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gpioSysfs.h
 *
 * Project:
 * --------
 *   OpenLinux
 *
 * Description:
 * ------------
 *   API definition for GPIO driver.
 *
 * Author:
 * ------------
 * Stanley YONG
 * ------------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * WHO            WHEN                WHAT
 *----------------------------------------------------------------------------
 * Stanley.YONG   15/07/2016          Create
 * Stanley.YONG   26/07/2016          Add interrupt implementation.
 ****************************************************************************/

#ifndef __GPIO_SYSFS_H__
#define __GPIO_SYSFS_H__
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gpio_def.h"
#include "gpio_oe.h"

#if __LP64__
    #define strtoptr strtoull
#else
    #define strtoptr strtoul
#endif

#define off64_t long long

#define PAGE_SIZE 4096

//--------------------------------------------------------------------------------------------------
/**
 * The value of GPIO low or high.
 * Translates to the setting of "value" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    GPIO_LEVEL_LOW,   ///< GPIO Low Value
    GPIO_LEVEL_HIGH   ///< GPIO High Value
} Enum_GPIO_Level;

//--------------------------------------------------------------------------------------------------
/**
 * The polarity of GPIO level low or high.
 * Translates to the setting of "active_low" in sysfs
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    POLARITY_LEVEL_HIGH,  // GPIO Active-High, output signal is 1
    POLARITY_LEVEL_LOW    // GPIO Active-Low, output signal is 0
} Enum_GPIO_Polarity_Level;

typedef enum
{
    GPIO_DIR_INPUT,
    GPIO_DIR_OUTPUT
} Enum_GPIO_Dir;


//------------------------------------------------------------------------------
/**
 * The mode of GPIO Edge Sensivity.
 */
//------------------------------------------------------------------------------
typedef enum
{
    GPIO_EDGE_SENSE_NONE,   // pin is input, but no an interrupt pin.
    GPIO_EDGE_SENSE_RISING,
    GPIO_EDGE_SENSE_FALLING,
    GPIO_EDGE_SENSE_BOTH
} Enum_GpioEdgeSenseMode;


//--------------------------------------------------------------------------------------------------
/**
 * The struct of Sysfs object
 */
//--------------------------------------------------------------------------------------------------
typedef void (*_fdMonitor_CB)(Enum_PinName  pinName, boolean activeLevel, void *param);

typedef struct
{
    Enum_PinName    pinName;
    unsigned short  gpioNum;                      // Pin GPIO number
    const char gpioName[10];                    // Pin GPIO Signal Name
    boolean inUse;                              // Is the GPIO currently used
    int monitorFd;                              // The FD of the file bing monitored
    _fdMonitor_CB fdMonitor_proc;               // fdMonitor_cb Object associated to this GPIO
    void *param_cb;                             // the parameter can be passed into fdMonitor_cb
} ST_GPIO_SYSFS;


//--------------------------------------------------------------------------------------------------
/**
 * Export a GPIO in the sysfs.
 * @return
 * - RES_OK if exporting was successful
 * - RES_IO_NOT_SUPPORT pin is not supported
 * - RES_IO_ERROR if it failed
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_export(const Enum_PinName pinName);

//--------------------------------------------------------------------------------------------------
/**
 * Unexport a GPIO in the sysfs.
 * @return
 * - RES_OK if exporting was successful
 * - RES_IO_NOT_SUPPORT pin is not supported
 * - RES_IO_ERROR if it failed
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_unexport(const Enum_PinName pinName);

//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO Direction INPUT or OUTPUT mode.
 *
 * "direction" ... reads as either "in" or "out". This value may
 *        normally be written. Writing as "out" defaults to
 *        initializing the value as low. To ensure glitch free
 *        operation, values "low" and "high" may be written to
 *        configure the GPIO as an output with that initial value
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_write_direction(
    Enum_PinName  pinName,    // [IN] pin name
    Enum_GPIO_Dir dir         // [IN] gpio direction input/output mode
);

//--------------------------------------------------------------------------------------------------
/**
 * PINname get gpio
 *
 */
//--------------------------------------------------------------------------------------------------
ST_GPIO_SYSFS *get_gpio_item_by_Pin(const Enum_PinName pinName);


//--------------------------------------------------------------------------------------------------
/**
 * read GPIO Direction INPUT or OUTPUT mode.
 *
 *
 * "direction" ... reads as either "in" or "out". This value may
 *        normally be written. Writing as "out" defaults to
 *        initializing the value as low. To ensure glitch free
 *        operation, values "low" and "high" may be written to
 *        configure the GPIO as an output with that initial value
 *
 * @return
 *      Gpio direction mode, 0 (outpu) and 1 (input).
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_read_direction(Enum_PinName pinName);

//--------------------------------------------------------------------------------------------------
/**
 * write value to GPIO output, low or high
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_write_value(
    Enum_PinName pinName,           // [IN] pin name
    Enum_GPIO_Level level           // [IN] High or low
);

//--------------------------------------------------------------------------------------------------
/**
 * read level value from GPIO input mode.
 *
 *
 * "value" ... reads as either 0 (low) or 1 (high). If the GPIO
 *        is configured as an output, this value may be written;
 *        any nonzero value is treated as high.
 *
 * @return
 *      An active type, the status of pin: HIGH or LOW
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_read_value(Enum_PinName pinName);

//--------------------------------------------------------------------------------------------------
/**
 * Open the device file of level value.
 *
 *
 * "pinName" ... pin name.
 *
 * @return
 *      A handle of file descriptor for level value.
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_open_value(Enum_PinName pinName);

//--------------------------------------------------------------------------------------------------
/**
 * Rising or Falling of Edge sensitivity
 *
 * "edge" ... reads as either "none", "rising", "falling", or
 * "both". Write these strings to select the signal edge(s)
 * that will make poll(2) on the "value" file return.

 * This file exists only if the pin can be configured as an
 * interrupt generating input pin.

 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_set_edge_sense(
    Enum_PinName  pinName,                  // [IN] gpio name
    Enum_GpioEdgeSenseMode edgeSense        // [IN] The mode of GPIO Edge Sensivity.
);

//---------------------------------------------------------------------------------------------------
/*
*set gpio pull
*/
//----------------------------------------------------------------------------------------------------
int gpio_write_register(
    unsigned char gpio,
    unsigned char val
);

//---------------------------------------------------------------------------------------------------
/*
*set gpio config
*/
//----------------------------------------------------------------------------------------------------
int gpio_write_register_all(
    unsigned char gpio,
    unsigned char val
);

//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO polarity.
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_write_polarity(
    Enum_PinName  pinName,           // [IN] gpio object reference
    Enum_GPIO_Polarity_Level level   // [IN] Active-high or active-low
);


//---------------------------------------------------------------------------------------------------
/*
*gpio_sysfs_init
*/
//----------------------------------------------------------------------------------------------------
int gpio_sysfs_init(Enum_PinName  pinName,
                    Enum_PinDirection  dir,
                    Enum_PinLevel      level,
                    Enum_PinPullSel    pullSel
                   );

//---------------------------------------------------------------------------------------------------
/*
*   gpio_sysfs_uninit
*/
//---------------------------------------------------------------------------------------------------
int gpio_sysfs_uninit(Enum_PinName pinName);

#endif  //__GPIOSYSFS_H__
