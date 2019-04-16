#ifndef __DEV_MCU_PARA_H__
#define __DEV_MCU_PARA_H__


#define SOFT_VER_LEN         (6)
#define VIN_BUF_LEN          (17)

typedef struct CUSTOM_PARA_T
{
    unsigned char  vin[VIN_BUF_LEN];
    unsigned char  ver[SOFT_VER_LEN];
} CUSTOM_PARA_T;

typedef enum CUSTOM_PARA_ID
{
    CUSTOM_ID_VIN       = 0,
    CUSTOM_ID_TBOX_VER,
    CUSTOM_ID_MAX,
} CUSTOM_PARA_ID;

int dev_send_custom_para_to_mcu(unsigned int index, const char *msg, int len);
int dev_check_custom_para_from_mcu(unsigned int index, unsigned char *msg, int len);

#endif

