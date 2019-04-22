#ifndef __GPIO_H__
#define __GPIO_H__
#include "gpio_def.h"

#define GPIO_EN5_CTRL          PINNAME_NET_STATUS       // Purpose:Power Supply uart 1.8V and audio
#define GPIO_PM_ENABLE         PINNAME_PM_ENABLE_WIFI   // Purpose:WIFI Power 
#define GPIO_GPS_RESET         PINNAME_WLAN_EN          // Purpose:extern GPS reset
#define GPIO_SUB_ANT_CTRL      PINNAME_GPIO5            // PIN:RF_CTL2
#define GPIO_MAIN_ANT_CTRL     PINNAME_GPIO2            // PIN:RF_CTL1
#define GPIO_RESET_EMMC        PINNAME_SD_INT_DET       // PIN:MMC_nRST

#define GPIO_WAKEUP_MCU        PINNAME_GPIO6            // PIN:RING, Purpose: wakeup MCU by EC20,
#define GPIO_MCU_WAKEUP        PINNAME_GPIO3            // PIN:WAKEUP_EC20, Purpose:wakeup EC20 by MCU
#define GPIO_WLAN_WAKEUP       PINNAME_WAKE_WLAN        // PIN:WL_WAKE_HOST, Purpose:wakeup EC20 by wifi

#define GPIO_HOST_WAKE_BT      PINNAME_BT_EN            // Purpose:wakeup BT by EC20 device,dev_wake 
#define GPIO_BT_WAKE_HOST      PINNAME_GPIO1            // Purpose:wakeup EC20 by BT
#define GPIO_BT_RST            PINNAME_COEX_UART_RX     // Purpose:BT on/off,reset control 

#define GPIO_BT_CTS            PINNAME_MAIN_CTS
#define GPIO_BT_RTS            PINNAME_MAIN_RTS

/****************************************************************************
 * GPIO Config Items
 ***************************************************************************/
typedef struct
{
    Enum_PinName           pinName;
    Enum_PinDirection      pinDirection;
    Enum_PinLevel          pinLevel;
    Enum_PinPullSel        pinPullSel;
} ST_GPIO_CFG;

int gpio_init(Enum_PinName pinName);
int gpio_uninit(Enum_PinName pinName);

int gpio_init_all(void);
int gpio_uninit_all(void);

int gpio_get_level(Enum_PinName pinName);
int gpio_set_level(Enum_PinName pinName, Enum_PinLevel level);

int gpio_get_direction(Enum_PinName pinName);
int gpio_set_direction(Enum_PinName pinName, Enum_PinDirection dir);


#endif  // __GPIO_H__
