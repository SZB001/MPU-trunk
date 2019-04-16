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
 *   gpioSysfs.c
 *
 * Project:
 * --------
 *   OpenLinux
 *
 * Description:
 * ------------
 *   API implementation for GPIO driver.
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
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include "gpio_sysfs.h"
#include "gpio_oe.h"
#include "log.h"

#define DEBUG_EN  0
#if DEBUG_EN
    #define _DEBUG(fmtString, ...)     printf(fmtString, ##__VA_ARGS__)
#else
    #define _DEBUG(fmtString, ...)
#endif


//--------------------------------------------------------------------------------------------------
/**
 * GPIO signals have paths like /sys/class/gpio/gpio24/ (for GPIO #24)
 */
//--------------------------------------------------------------------------------------------------
#define SYSFS_GPIO_PATH    "/sys/class/gpio"

static ST_GPIO_SYSFS sysfs_gpio_pins[] =
{
    /*PIN-1*/   {PINNAME_GPIO1,             25,     "gpio25\0",     FALSE, 0, NULL, NULL},
    /*PIN-2*/   {PINNAME_GPIO2,             10,     "gpio10\0",     FALSE, 0, NULL, NULL},
    /*PIN-3*/   {PINNAME_GPIO3,             42,     "gpio42\0",     FALSE, 0, NULL, NULL},
    /*PIN-4*/   {PINNAME_GPIO4,             11,     "gpio11\0",     FALSE, 0, NULL, NULL},
    /*PIN-5*/   {PINNAME_GPIO5,             24,     "gpio24\0",     FALSE, 0, NULL, NULL},
    /*PIN-6*/   {PINNAME_NET_STATUS,        1018,   "gpio1018\0",   FALSE, 0, NULL, NULL},
    /*PIN-13*/  {PINNAME_USIM_PRESENCE,     34,     "gpio34\0",     FALSE, 0, NULL, NULL},
    /*PIN-23*/  {PINNAME_SD_INT_DET,        26,     "gpio26\0",     FALSE, 0, NULL, NULL},
    /*PIN-24*/  {PINNAME_PCM_IN,            76,     "gpio76\0",     FALSE, 0, NULL, NULL},
    /*PIN-25*/  {PINNAME_PCM_OUT,           77,     "gpio77\0",     FALSE, 0, NULL, NULL},
    /*PIN-26*/  {PINNAME_PCM_SYNC,          79,     "gpio79\0",     FALSE, 0, NULL, NULL},
    /*PIN-27*/  {PINNAME_PCM_CLK,           78,     "gpio78\0",     FALSE, 0, NULL, NULL},
    /*PIN-37*/  {PINNAME_SPI_CS_N,          22,     "gpio22\0",     FALSE, 0, NULL, NULL},
    /*PIN-38*/  {PINNAME_SPI_MOSI,          20,     "gpio20\0",     FALSE, 0, NULL, NULL},
    /*PIN-39*/  {PINNAME_SPI_MISO,          21,     "gpio21\0",     FALSE, 0, NULL, NULL},
    /*PIN-40*/  {PINNAME_SPI_CLK,           23,     "gpio23\0",     FALSE, 0, NULL, NULL},
    /*PIN-41*/  {PINNAME_I2C1_SCL,          7,      "gpio7\0",      FALSE, 0, NULL, NULL},
    /*PIN-42*/  {PINNAME_I2C1_SDA,          6,      "gpio6\0",      FALSE, 0, NULL, NULL},
    /*PIN-61*/  {PINNAME_STATUS,            1021,   "gpio1021\0",   FALSE, 0, NULL, NULL},
    /*PIN-62*/  {PINNAME_GPIO6,             75,     "gpio75\0",     FALSE, 0, NULL, NULL},
    /*PIN-63*/  {PINNAME_UART1_TX,          4,      "gpio4\0",      FALSE, 0, NULL, NULL},
    /*PIN-64*/  {PINNAME_MAIN_CTS,          3,      "gpio3\0",      FALSE, 0, NULL, NULL},
    /*PIN-65*/  {PINNAME_MAIN_RTS,          2,      "gpio2\0",      FALSE, 0, NULL, NULL},
    /*PIN-66*/  {PINNAME_UART1_RX,          5,      "gpio5\0",      FALSE, 0, NULL, NULL},
    /*PIN-73*/  {PINNAME_GPIO7,             49,     "gpio49\0",     FALSE, 0, NULL, NULL},
    /*PIN-119*/ {PINNAME_EPHY_RST_N,        29,     "gpio29\0",     FALSE, 0, NULL, NULL},
    /*PIN-120*/ {PINNAME_EPHY_INT_N,        30,     "gpio30\0",     FALSE, 0, NULL, NULL},
    /*PIN-121*/ {PINNAME_SGMII_DATA,        28,     "gpio28\0",     FALSE, 0, NULL, NULL},
    /*PIN-122*/ {PINNAME_SGMII_CLK,         27,     "gpio27\0",     FALSE, 0, NULL, NULL},
    /*PIN-127*/ {PINNAME_PM_ENABLE_WIFI,    1020,   "gpio1020\0",   FALSE, 0, NULL, NULL},
    /*PIN-129*/ {PINNAME_SDC1_DATA3,        12,     "gpio12\0",     FALSE, 0, NULL, NULL},
    /*PIN-130*/ {PINNAME_SDC1_DATA2,        13,     "gpio13\0",     FALSE, 0, NULL, NULL},
    /*PIN-131*/ {PINNAME_SDC1_DATA1,        14,     "gpio14\0",     FALSE, 0, NULL, NULL},
    /*PIN-132*/ {PINNAME_SDC1_DATA0,        15,     "gpio15\0",     FALSE, 0, NULL, NULL},
    /*PIN-133*/ {PINNAME_SDC1_CLK,          16,     "gpio16\0",     FALSE, 0, NULL, NULL},
    /*PIN-134*/ {PINNAME_SDC1_CMD,          17,     "gpio17\0",     FALSE, 0, NULL, NULL},
    /*PIN-135*/ {PINNAME_WAKE_WLAN,         59,     "gpio59\0",     FALSE, 0, NULL, NULL},
    /*PIN-136*/ {PINNAME_WLAN_EN,           38,     "gpio38\0",     FALSE, 0, NULL, NULL},
    /*PIN-137*/ {PINNAME_COEX_UART_RX,      37,     "gpio37\0",     FALSE, 0, NULL, NULL},
    /*PIN-138*/ {PINNAME_COEX_UART_TX,      36,     "gpio36\0",     FALSE, 0, NULL, NULL},
    /*PIN-139*/ {PINNAME_BT_EN,             1019,   "gpio1019\0",   FALSE, 0, NULL, NULL},
    /*PIN-141*/ {PINNAME_I2C2_SCL,          19,     "gpio19\0",     FALSE, 0, NULL, NULL},
    /*PIN-142*/ {PINNAME_I2C2_SDA,          18,     "gpio18\0",     FALSE, 0, NULL, NULL},
    /*PIN-143*/ {PINNAME_GPIO8,             53,     "gpio53\0",     FALSE, 0, NULL, NULL},
    /*PIN-144*/ {PINNAME_GPIO9,             52,     "gpio52\0",     FALSE, 0, NULL, NULL},
};

void  print_gpio_tbl(void)
{
    int i;
    int nCnt = sizeof(sysfs_gpio_pins) / sizeof(sysfs_gpio_pins[0]);

    for (i = 0; i < nCnt; i++)
    {
        _DEBUG("PIN[%d]: pinName=%d, gpioNum=%d, gpioName=%s, inUse=%d, monitorFd=%d proc=%X\n", i,
               sysfs_gpio_pins[i].pinName,
               sysfs_gpio_pins[i].gpioNum,
               sysfs_gpio_pins[i].gpioName,
               sysfs_gpio_pins[i].inUse,
               sysfs_gpio_pins[i].monitorFd,
               (uint32_t)sysfs_gpio_pins[i].fdMonitor_proc);
    }
}

ST_GPIO_SYSFS *get_gpio_item_by_Pin(const Enum_PinName pinName)
{
    int nCnt = sizeof(sysfs_gpio_pins) / sizeof(sysfs_gpio_pins[0]);
    int i;

    for (i = 0; i < nCnt; i++)
    {
        if (pinName == sysfs_gpio_pins[i].pinName)
        {
            return &sysfs_gpio_pins[i];
        }
    }

    return NULL;
}

ST_GPIO_SYSFS *get_gpio_item_by_fd(const int monFd)
{
    int i;
    int nCnt = sizeof(sysfs_gpio_pins) / sizeof(sysfs_gpio_pins[0]);

    _DEBUG("%s, monFd=%d\n", __func__, monFd);

    for (i = 0; i < nCnt; i++)
    {
        if (monFd == sysfs_gpio_pins[i].monitorFd)
        {
            _DEBUG("found: pin=%d, gpio=%d,fd=%d, ptr=%X\n",
                   sysfs_gpio_pins[i].pinName,
                   sysfs_gpio_pins[i].gpioNum,
                   sysfs_gpio_pins[i].monitorFd,
                   (uint32_t)sysfs_gpio_pins[i].fdMonitor_proc);
            return &sysfs_gpio_pins[i];
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if sysfs gpio path exists.
 * @return
 * - true: gpio path exists
 * - FALSE: gpio path does not exist
 */
//--------------------------------------------------------------------------------------------------
static boolean gpio_sysfs_check_exist(const char *path)
{
    DIR *dir;

    dir = opendir(path);

    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        return FALSE;
    }

    return TRUE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Export a GPIO in the sysfs.
 * @return
 * - RES_OK if exporting was successful
 * - RES_IO_NOT_SUPPORT pin is not supported
 * - RES_IO_ERROR if it failed
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_export(const Enum_PinName pinName)
{
    char path[128];
    char export[128];
    char gpioStr[8];
    FILE *fp = NULL;
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    // First check if the GPIO has already been exported
    snprintf(path, sizeof(path), "%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName);

    if (gpio_sysfs_check_exist(path))
    {
        return RES_OK;
    }

    // Write the GPIO number to the export file
    snprintf(export, sizeof(export), "%s/%s", SYSFS_GPIO_PATH, "export");
    snprintf(gpioStr, sizeof(gpioStr), "%d", pstGpioItem->gpioNum);

    do
    {
        fp = fopen(export, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {

        log_e(LOG_MID, "Error opening file %s for writing.", export);
        return RES_IO_ERROR;
    }

    ssize_t written = fwrite(gpioStr, 1, strlen(gpioStr), fp);
    fflush(fp);

    int file_err = ferror(fp);
    fclose(fp);

    if (file_err != 0)
    {
        log_e(LOG_MID, "Failed to export GPIO %s. Error %s", gpioStr, strerror(file_err));
        return RES_IO_ERROR;
    }

    if (written < strlen(gpioStr))
    {
        log_e(LOG_MID, "Data truncated while exporting GPIO %s.", gpioStr);
        return RES_IO_ERROR;
    }

    // Now check again that it has been exported
    if (gpio_sysfs_check_exist(path))
    {
        return RES_OK;
    }

    log_e(LOG_MID, "Failed to export GPIO %s", gpioStr);
    return RES_IO_ERROR;
}

int gpio_sysfs_unexport(const Enum_PinName pinName)
{
    char path[128];
    char export[128];
    char gpioStr[8];
    FILE *fp = NULL;
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    // First check if the GPIO has already been exported
    snprintf(path, sizeof(path), "%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName);

    if (!gpio_sysfs_check_exist(path))
    {
        return RES_OK;
    }

    // Write the GPIO number to the unexport file
    snprintf(export, sizeof(export), "%s/%s", SYSFS_GPIO_PATH, "unexport");
    snprintf(gpioStr, sizeof(gpioStr), "%d", pstGpioItem->gpioNum);

    do
    {
        fp = fopen(export, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {
        log_e(LOG_MID, "Error opening file %s for writing", export);
        return RES_IO_ERROR;
    }

    ssize_t written = fwrite(gpioStr, 1, strlen(gpioStr), fp);
    fflush(fp);

    int file_err = ferror(fp);
    fclose(fp);

    if (file_err != 0)
    {
        log_e(LOG_MID, "Failed to export GPIO %s. Error %s", gpioStr, strerror(file_err));
        return RES_IO_ERROR;
    }

    if (written < strlen(gpioStr))
    {
        log_e(LOG_MID, "Data truncated while exporting GPIO %s", gpioStr);
        return RES_IO_ERROR;
    }

    // Now check again that it has been unexported
    if (!gpio_sysfs_check_exist(path))
    {
        return RES_OK;
    }

    log_e(LOG_MID, "Failed to unexport GPIO %s", gpioStr);
    return RES_IO_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set sysfs GPIO signals attributes
 *
 * GPIO signals have paths like /sys/class/gpio/gpioN/
 * and have the following read/write attributes:
 * - "direction"
 * - "value"
 * - "edge"
 * - "active_low"
 * - "pull"
 *
 * @return
 * - RES_IO_ERROR: write sysfs gpio error
 * - RES_OK: successfully
 */
//--------------------------------------------------------------------------------------------------
static int gpio_sysfs_write_attr(
    const char *path,        // [IN] path to sysfs gpio signal
    const char *attr         // [IN] GPIO signal write attribute
)
{
    FILE *fp = NULL;

    if (!gpio_sysfs_check_exist(path))
    {
        log_e(LOG_MID, "GPIO %s does not exist (probably not exported)", path);
        return RES_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "w");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {
        log_e(LOG_MID, "Error opening file %s for writing", path);
        return RES_IO_ERROR;
    }

    ssize_t written = fwrite(attr, 1, strlen(attr), fp);
    fflush(fp);

    int file_err = ferror(fp);

    if (file_err != 0)
    {
        log_e(LOG_MID, "Failed to write %s to GPIO config %s. Error %s", attr, path, strerror(file_err));
        fclose(fp);
        return RES_IO_ERROR;
    }

    if (written < strlen(attr))
    {
        log_e(LOG_MID, "Data truncated while writing %s to GPIO config %s", path, attr);
        fclose(fp);
        return RES_IO_ERROR;
    }

    fclose(fp);
    return RES_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get sysfs GPIO attribute
 *
 * GPIO signals have paths like /sys/class/gpio/gpioN/
 * and have the following read/write attributes:
 * - "direction"
 * - "value"
 * - "edge"
 * - "active_low"
 * - "pull"
 *
 * @return
 * - -1: write sysfs gpio error
 * -  0: successfully
 */
//--------------------------------------------------------------------------------------------------
static int gpio_sysfs_read_attr(
    const char *path,        // [IN] path to sysfs gpio
    int attr_size,           // [IN] the size of attribute content
    char *attr               // [OUT] GPIO signal read attribute content
)
{
    int i;
    char c;
    FILE *fp = NULL;
    char *result = attr;

    if (!gpio_sysfs_check_exist(path))
    {
        log_e(LOG_MID, "File %s does not exist", path);
        return RES_BAD_PARAMETER;
    }

    do
    {
        fp = fopen(path, "r");
    }
    while ((fp == NULL) && (errno == EINTR));

    if (!fp)
    {
        log_e(LOG_MID, "Error opening file %s for reading", path);
        return RES_IO_ERROR;
    }

    i = 0;

    while (((c = fgetc(fp)) != EOF) && (i < (attr_size - 1)))
    {
        result[i] = c;
        i++;
    }

    result[i] = '\0';
    fclose(fp);

    log_e(LOG_MID, "Read result: %s from %s", result, path);

    return RES_OK;
}

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
)
{
    char path[64];
    char attr[16];
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "direction");
    snprintf(attr, sizeof(attr), "%s", (GPIO_DIR_OUTPUT == dir) ? "out" : "in");
    log_o(LOG_MID, "path:%s, attr:%s", path, attr);

    return gpio_sysfs_write_attr(path, attr);
}

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
int gpio_sysfs_read_direction(Enum_PinName pinName)
{
    char path[64];
    char result[17];
    int dir_value;
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "direction");
    memset(result, 0x0, sizeof(result));
    gpio_sysfs_read_attr(path, sizeof(result), result);
    dir_value = (result[0] == 'o') ? PINDIRECTION_OUT : ((result[0] == 'i') ? PINDIRECTION_IN : -1);
    log_o(LOG_MID, "result:%s Dir:%s", result, (dir_value == PINDIRECTION_IN) ? "input" : "output");

    return dir_value;
}

//--------------------------------------------------------------------------------------------------
/**
 * write value to GPIO output, low or high
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_write_value(
    Enum_PinName pinName,           // [IN] pin name
    Enum_GPIO_Level level           // [IN] High or low
)
{
    char path[64];
    char attr[16];
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "value");
    snprintf(attr, sizeof(attr), "%d", level);
    log_i(LOG_MID, "path:%s, attr:%s", path, attr);

    return gpio_sysfs_write_attr(path, attr);
}

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
int gpio_sysfs_read_value(Enum_PinName pinName)
{
    char path[64];
    char result[17];
    int level_value;
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "value");
    memset(result, 0x0, sizeof(result));
    gpio_sysfs_read_attr(path, sizeof(result), result);
    level_value = atoi(result);
    log_o(LOG_MID, "result:%s, value:%s", result, (level_value == 1) ? "high" : "low");

    return level_value;
}

//--------------------------------------------------------------------------------------------------
/**

 * Open the device file of level value.
 *
 *
 * "pinName" ... pin name.

 *
 * @return
 *      A handle for file descriptor.

 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_open_value(Enum_PinName pinName)
{
    char monFile[128];
    int valueFd = 0;

    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    // Start monitoring the fd for the correct GPIO
    snprintf(monFile, sizeof(monFile), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "value");

    do
    {
        valueFd = open(monFile, O_RDONLY);
    }
    while ((0 == valueFd) && (EINTR == errno));

    if (0 == valueFd)
    {
        log_e(LOG_MID, "Fail to open GPIO file for monitoring");
        return -2;
    }

    log_o(LOG_MID, "open(monFile..)=%d", valueFd);
    return valueFd;
}

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
)
{
    char path[64];
    char attr[11];

    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "edge");

    switch (edgeSense)
    {
        case GPIO_EDGE_SENSE_RISING:
            snprintf(attr, 10, "rising");
            break;

        case GPIO_EDGE_SENSE_FALLING:
            snprintf(attr, 10, "falling");
            break;

        case GPIO_EDGE_SENSE_BOTH:
            snprintf(attr, 10, "both");
            break;

        default:
            snprintf(attr, 10, "none");
            break;
    }

    log_o(LOG_MID, "path:%s, attr:%s", path, attr);

    return gpio_sysfs_write_attr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * setup GPIO polarity.
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_write_polarity(
    Enum_PinName  pinName,           // [IN] gpio name
    Enum_GPIO_Polarity_Level level   // [IN] Active-high or active-low
)
{
    char path[64];
    char attr[16];
    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "active_low");
    snprintf(attr, sizeof(attr), "%d", level);
    log_o(LOG_MID, "path:%s, attr:%s\n", path, attr);

    return gpio_sysfs_write_attr(path, attr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current value the pin polarity.
 *
 * @return The current configured value
 */
//--------------------------------------------------------------------------------------------------
int gpio_sysfs_read_polarity(Enum_PinName  pinName)
{
    char path[64];
    char result[17];
    Enum_GpioEdgeSenseMode type;

    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        // pin not support
        return RES_IO_NOT_SUPPORT;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", SYSFS_GPIO_PATH, pstGpioItem->gpioName, "active_low");
    gpio_sysfs_read_attr(path, sizeof(result), result);
    type = atoi(result);
    log_o(LOG_MID, "result:%s", result);
    return type;
}

/****************************************************************
function:     gpio_write_register
description:  write gpio register
input:        unsigned char gpio
              unsigned char val
              val 0-----gpio no pull
                  1-----gpio pull down
                  3-----gpio pull up
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_write_register(unsigned char gpio, unsigned char val)
{
    uintptr_t addr = 0x1000000 + 0x1000 * gpio;
    uintptr_t endaddr = addr + 3;
    uint32_t reg_val, temp;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0)
    {
        log_e(LOG_MID, "cannot open /dev/mem");
        return -1;
    }

    off64_t mmap_start = addr & ~(PAGE_SIZE - 1);
    size_t mmap_size = endaddr - mmap_start + 1;
    mmap_size = (mmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    void *page = mmap(0, mmap_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, mmap_start);

    if (page == MAP_FAILED)
    {
        //fprintf(stderr,"cannot mmap region\n");
        log_e(LOG_MID, "cannot mmap region");
        return -1;
    }

    uint32_t *x = (uint32_t *)(((uintptr_t) page) + (addr & 4095));
    reg_val = *x;
    temp = reg_val >> 2;
    *x = (temp << 2) + val;
    log_o(LOG_MID, "width 4 ---%08"PRIxPTR": %08x\n", addr, *x);
    return 1;
}

/****************************************************************
function:    gpio_sysfs_uninit
description: bit        9 8 76 54 32 10
             ctl_reg    0|0 00|00 00|00
                      dir| drv| func|pull
             bit                   1 0
             io_reg                0|0
                                 out|in
             gpio : gpio no
             pull : no pull(0), pull down(1), keeper(2), pull up(3)
             dir  : input(0), output(1)
             lvl  : output level
input:       unsigned char gpio
             unsigned char val
output:      none
return:      0 indicates success;
             others indicates failed
*****************************************************************/
int gpio_write_register_all(unsigned char gpio, unsigned char val)
{
    uintptr_t addr = 0x1000000 + 0x1000 * gpio;
    uintptr_t endaddr = addr + 3;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0)
    {
        log_e(LOG_MID, "open failed, fd:%d", fd);
        return -1;
    }

    off64_t mmap_start = addr & ~(PAGE_SIZE - 1);
    size_t mmap_size = endaddr - mmap_start + 1;
    mmap_size = (mmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    void *page = mmap(0, mmap_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, mmap_start);

    if (page == MAP_FAILED)
    {
        close(fd);
        log_e(LOG_MID, "mmap failed");
        return -1;
    }

    uint32_t *x = (uint32_t *)(((uintptr_t) page) + (addr & 4095));
    *x = val;

    if (munmap(page, mmap_size) == -1)
    {
        close(fd);
        log_e(LOG_MID, "munmap failed");
        return -1;
    }

    close(fd);
    return 0;
}

/****************************************************************
function:     gpio_sysfs_init
description:  Converts a set of poll(2) event flags into a set of epoll(7) event flags.
              @return Bit map containing epoll(7) events flags.
input:        short  pollFlags
output:       none
return:       epollFlags
*****************************************************************/
unsigned int poll_to_epoll(short  pollFlags)
{
    unsigned int epollFlags = 0;

    if (pollFlags & POLLIN)
    {
        epollFlags |= EPOLLIN;
    }

    if (pollFlags & POLLPRI)
    {
        epollFlags |= EPOLLPRI;
    }

    if (pollFlags & POLLOUT)
    {
        epollFlags |= EPOLLOUT;
    }

    if (pollFlags & POLLHUP)
    {
        epollFlags |= EPOLLHUP;
    }

    if (pollFlags & POLLRDHUP)
    {
        epollFlags |= EPOLLRDHUP;
    }

    if (pollFlags & POLLERR)
    {
        epollFlags |= EPOLLERR;
    }

    return epollFlags;
}

/****************************************************************
function:     gpio_sysfs_init
description:  init gpio by name
input:        Enum_PinName pinName
              Enum_PinDirection dir,
              Enum_PinLevel level,
              Enum_PinPullSel pullSel;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_sysfs_init(Enum_PinName  pinName,
                    Enum_PinDirection  dir,
                    Enum_PinLevel      level,
                    Enum_PinPullSel    pullSel
                   )
{
    int ret;

    ST_GPIO_SYSFS *pstGpioItem = get_gpio_item_by_Pin(pinName);

    if (!pstGpioItem)
    {
        log_e(LOG_MID, "pin not support");
        return RES_IO_NOT_SUPPORT;
    }

    if (pstGpioItem->gpioNum < 1018)
    {
        ret = gpio_write_register_all(pstGpioItem->gpioNum, 0);

        if (0 != ret)
        {
            log_e(LOG_MID, "register, ret:%d", ret);
            return ret;
        }
    }

    ret = gpio_sysfs_export(pinName);

    if (0 != ret)
    {
        log_e(LOG_MID, "register, ret:%d", ret);
        return ret;
    }

    ret = gpio_sysfs_write_direction(pinName, dir);

    if (0 != ret)
    {
        log_e(LOG_MID, "register, ret:%d", ret);
        return ret;
    }

    if (PINDIRECTION_OUT == dir)
    {
        ret = gpio_sysfs_write_value(pinName, level);
        log_o(LOG_MID, "GpioSysfs_WriteValue(%d, %d)=%d\n", pinName, level, ret);
    }

    return 0;
}

/****************************************************************
function:     gpio_sysfs_uninit
description:  init gpio by name
input:        Enum_PinName pinName
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int gpio_sysfs_uninit(Enum_PinName pinName)
{
    int iRet;
    iRet = gpio_sysfs_unexport(pinName);
    log_o(LOG_MID, "GpioSysfs_UnexportGpio(%d)=%d\n", pinName, iRet);
    return iRet;
}

