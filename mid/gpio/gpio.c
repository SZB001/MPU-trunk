#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "gpio_sysfs.h"
#include "gpio.h"

ST_GPIO_CFG gpio_cfgs[] =
{
    {GPIO_EN5_CTRL,         PINDIRECTION_OUT, PINLEVEL_HIGH, PINPULLSEL_DISABLE},
    {GPIO_PM_ENABLE,        PINDIRECTION_OUT, PINLEVEL_HIGH, PINPULLSEL_DISABLE},
    {GPIO_GPS_RESET,        PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_SUB_ANT_CTRL,     PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_MAIN_ANT_CTRL,    PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_RESET_EMMC,       PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_HOST_WAKE_BT,     PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_WAKEUP_MCU,       PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_MCU_WAKEUP,       PINDIRECTION_IN,  PINLEVEL_HIGH, PINPULLSEL_DISABLE},
    {GPIO_WLAN_WAKEUP,      PINDIRECTION_IN,  PINLEVEL_HIGH, PINPULLSEL_PULLUP},
    {GPIO_BT_WAKE_HOST,     PINDIRECTION_IN,  PINLEVEL_HIGH, PINPULLSEL_PULLUP},
    {GPIO_BT_CTS,           PINDIRECTION_OUT, PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_BT_RTS,           PINDIRECTION_IN,  PINLEVEL_LOW,  PINPULLSEL_DISABLE},
    {GPIO_BT_RST,           PINDIRECTION_OUT, PINLEVEL_HIGH, PINPULLSEL_DISABLE},
};

static ST_GPIO_CFG *gpio_cfg_by_pinName(const Enum_PinName pinName)
{
    int i;
    int nCnt = sizeof(gpio_cfgs) / sizeof(gpio_cfgs[0]);

    for (i = 0; i < nCnt; i++)
    {
        if (pinName == gpio_cfgs[i].pinName)
        {
            return &gpio_cfgs[i];
        }
    }

    return NULL;
}

/****************************************************************
function:     gpio_init
description:  init gpio by name
input:        Enum_PinName pinName
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_init(Enum_PinName pinName)
{
    ST_GPIO_CFG *p_cfg = gpio_cfg_by_pinName(pinName);

    if (!p_cfg)
    {
        return -1;
    }

    return gpio_sysfs_init(p_cfg->pinName, p_cfg->pinDirection, p_cfg->pinLevel, p_cfg->pinPullSel);
}

/****************************************************************
function:     gpio_uninit
description:  release gpio by name
input:        Enum_PinName pinName
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_uninit(Enum_PinName pinName)
{
    return gpio_sysfs_uninit(pinName);
}

/****************************************************************
function:     gpio_opt_all
description:  init gpio by name
input:        bool isInit
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int gpio_opt_all(bool isInit)
{
    int i, ret = 0;
    int nCnt = sizeof(gpio_cfgs) / sizeof(gpio_cfgs[0]);

    for (i = 0; i < nCnt; i++)
    {
        ret |= isInit ? gpio_sysfs_init(gpio_cfgs[i].pinName,
                                        gpio_cfgs[i].pinDirection,
                                        gpio_cfgs[i].pinLevel,
                                        gpio_cfgs[i].pinPullSel) :
               gpio_sysfs_uninit(gpio_cfgs[i].pinName);
    }

    return (ret < 0) ? -1 : 0;
}

/****************************************************************
function:     gpio_init_all
description:  init all gpio
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_init_all(void)
{
    return gpio_opt_all(true);
}

/****************************************************************
function:     gpio_uninit_all
description:  release all gpio
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_uninit_all(void)
{
    return gpio_opt_all(false);
}

/****************************************************************
function:     gpio_set_level
description:  set gpio by name
input:        Enum_PinName pinName,
              Enum_PinLevel level;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_set_level(Enum_PinName pinName, Enum_PinLevel level)
{
    return gpio_sysfs_write_value(pinName, level);
}

/****************************************************************
function:     gpio_get_level
description:  get gpio level by name
input:        Enum_PinName pinName
output:       none
return:       gpio level value
*****************************************************************/
int gpio_get_level(Enum_PinName pinName)
{
    return gpio_sysfs_read_value(pinName);
}

/****************************************************************
function:     gpio_set_direction
description:  set gpio direction by name
input:        Enum_PinName pinName
              Enum_PinDirection dir
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_set_direction(Enum_PinName pinName, Enum_PinDirection dir)
{
    return gpio_sysfs_write_direction(pinName, dir);
}

/****************************************************************
function:     gpio_get_direction
description:  get gpio direction by name
input:        Enum_PinName pinName
output:       none
return:       gpio direction
*****************************************************************/
int gpio_get_direction(Enum_PinName pinName)
{
    return gpio_sysfs_read_direction(pinName);
}

