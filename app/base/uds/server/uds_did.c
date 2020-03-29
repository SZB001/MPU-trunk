#include <string.h>
#include "uds_did.h"
#include "uds_diag.h"
#include "diag.h"
#include "list.h"
#include "status_sync.h"
#include "timer.h"
#include "rds.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "gps_api.h"
#include "at.h"
#include "bcd.h"
#include "uds.h"
#include "uds_did_def.h"
#include "uds_define.h"
#include "gb32960_api.h"/* added by caoml 2019.06.28*/
#include "hozon_SP_api.h"/* added by caoml 2019.06.28*/
#include "nm_api.h"
#include "../../../../interface/hozon/PrvtProtocol/PrvtProt.h"
#include "hozon_SP_api.h"

int uds_did_get_wakup_src(unsigned char *did, unsigned int len)
{
    unsigned short wakeup_src = 0;
    unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "wakeup source len error,len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(unsigned short);
    st_get(ST_ITEM_WAKEUP_SRC, (unsigned char *)&wakeup_src, &length);
    memcpy(did, &wakeup_src, sizeof(unsigned char));
    return 0;
}

int uds_did_get_pow_voltage(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned short voltage = 0;
    unsigned char value;

    if (1 > len)
    {
        log_e(LOG_UDS, "power voltage len error,len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(unsigned short);
    st_get(ST_ITEM_POW_VOLTAGE, (unsigned char *)&voltage, &length);
    value = voltage / 100;
    did[0] = value;

    return 0;
}

int uds_did_get_time(unsigned char *did, unsigned int len)
{
    RTCTIME time;

    if (6 > len)
    {
        log_e(LOG_UDS, "get time len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    tm_get_abstime(&time);
    /*did[0] = bcd2bin_2dit(time.year - 2000);
    did[1] = bcd2bin_2dit(time.mon);
    did[2] = bcd2bin_2dit(time.mday);
    did[3] = bcd2bin_2dit(time.hour);
    did[4] = bcd2bin_2dit(time.min);
    did[5] = bcd2bin_2dit(time.sec);*/
    
    did[0] = time.year%256;
	did[1] = time.year/256;
    did[2] = time.mon;
    did[3] = time.mday;
    did[4] = time.hour;
    did[5] = time.min;
    did[6] = time.sec;
    
    return 0;
}

int uds_did_get_gps_ant(unsigned char *did, unsigned int len)       ////modified by Liuzw 20180320
{
    unsigned char status = 0;
    unsigned char uds_status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps ant len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_ant_status(ANT_GNSS);

    /* UDS DIAG STATUS:
    UNKNOW 0
    SHORT  1
    OPEN   2
    OK     3 */

    switch (status)
    {
        /* UNKNOW */
        case 0:
            uds_status = 0;
            break;

        /* OK */
        case 1:
            uds_status = 3;
            break;

        /* SHORT*/
        case 2:
            uds_status = 1;
            break;

        /* OPEN*/
        case 3:
            uds_status = 2;
            break;

        default:
            uds_status = 0;
            break;
    }

    did[0] = uds_status;

    return 0;
}


int uds_did_get_4G_main_ant(unsigned char *did, unsigned int len)       ////modified by Liuzw 20180320
{
    unsigned char status = 0;
    unsigned char uds_status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps ant len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_ant_status(ANT_4G_MAIN);

    /* UDS DIAG STATUS:
    UNKNOW 0
    SHORT  1
    OPEN   2
    OK     3 */

    switch (status)
    {
        /* UNKNOW */
        case 0:
            uds_status = 0;
            break;

        /* OK */
        case 1:
            uds_status = 3;
            break;

        /* SHORT */
        case 2:
            uds_status = 1;
            break;

        /*OPEN  */
        case 3:
            uds_status = 2;
            break;

        default:
            uds_status = 0;
            break;
    }

    did[0] = uds_status;

    return 0;
}


int uds_did_get_4G_vice_ant(unsigned char *did, unsigned int len)       ////modified by Liuzw 20180320
{
    unsigned char status = 0;
    unsigned char uds_status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps ant len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_ant_status(ANT_4G_VICE);

    /* UDS DIAG STATUS:
    UNKNOW 0
    SHORT  1
    OPEN   2
    OK     3 */

    switch (status)
    {
        /* UNKNOW */
        case 0:
            uds_status = 0;
            break;

        /* OK */
        case 1:
            uds_status = 3;
            break;

        /* SHORT*/
        case 2:
            uds_status = 1;
            break;

        /*OPEN */
        case 3:
            uds_status = 2;
            break;

        default:
            uds_status = 0;
            break;
    }

    did[0] = uds_status;

    return 0;
}

int uds_did_get_gps_module(unsigned char *did, unsigned int len)
{
    unsigned char status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps module len error,len: %d", len);
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_gps_status();

    if ((0 == status) || (1 == status))
    {
        did[0] = 0;
    }
    else
    {
        did[0] = 1;
    }

    return 0;
}

int uds_did_get_fix_status(unsigned char *did, unsigned int len)
{
    unsigned char status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps fix len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    if (GPS_FIX == gps_get_fix_status())    //modified by Liuzw 20180320
    {
        status = 0;
    }
    else
    {
        status = 1;
    }

    did[0] = status;
    return 0;
}

int uds_did_get_emmc(unsigned char *did, unsigned int len)  //modified by Liuzw 20180320
{
    unsigned char status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get gps module len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    #if 0
    typedef enum
    {
        SD_NOT_DETECT = 0xFF, /* there is no SD card */
        SD_IN_RECORDING = 10, /* there is one card, recording data */
        SD_IN_NOT_RECORDING = 11, /* there is one card, not record data */
        SD_IN_FULL = 21, /* there is one card, but card full */
        SD_IN_LOCK = 22, /* there is one card, but card locked */
        SD_IN_ERROR = 23, /* there is one card, but we got error */
    } SD_STATUS;
    #endif

    status = dev_diag_get_emmc_status();

    switch (status)
    {
        /* normal or unknown */
        case DIAG_EMMC_UNKNOWN:
        case DIAG_EMMC_OK:
            did[0] = 11;
            break;

        /* full */
        case DIAG_EMMC_FULL:
            did[0] = 21;
            break;

        /* unmont */
        case DIAG_EMMC_UMOUNT:
        case DIAG_EMMC_NOT_FORMAT:
        case DIAG_EMMC_UMOUNT_POINT_NOT_EXIST:
            did[0] = 23;
            break;

        /* not exist */
        case DIAG_EMMC_NOT_EXIST:
            did[0] = 0xff;
            break;

        default:
            did[0] = 11;
            break;
    }

    return 0;
}

int uds_did_get_4G_module(unsigned char *did, unsigned int len)
{
    unsigned char status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get 4G module len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_4G_status();

    if ((0 == status) || (1 == status))
    {
        did[0] = 0;
    }
    else
    {
        did[0] = 1;
    }

    return 0;
}

int uds_did_get_4G_comm(unsigned char *did, unsigned int len)
{
    unsigned char status = 0;
    unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get 4G comm len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(status);
    st_get(ST_ITEM_TSP_COMM, &status, &length);

    if (0 == status)
    {
        did[0] = 1;
    }
    else
    {
        did[0] = 0;
    }

    return 0;
}

int uds_did_get_ccid(unsigned char *did, unsigned int len)
{
    if (20 > len)
    {
        log_e(LOG_UDS, "get ccid len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_iccid((char *)did);
    return 0;
}

int uds_did_get_apn1(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned int ret =0;

    if (32 > len)
    {
        log_e(LOG_UDS, "get local apn len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 32;
    
    ret = cfg_get_para(CFG_ITEM_LOCAL_APN, did, &length);
   //length = strlen((const char *)did);

   //log_e(LOG_UDS, "did:%s len: %d", did,length);

   /*for( i = length; i < 32;i++ )
   {
       did[i] = 0;
   }*/

   return ret;
    //return cfg_get_para(CFG_ITEM_LOCAL_APN, did, &length);
}

int uds_did_get_4G_sig(unsigned char *did, unsigned int len)
{
    if (1 > len)
    {
        log_e(LOG_UDS, "get 4G signal len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    did[0] = at_get_signal();

    return 0;
}

int uds_did_get_can1(unsigned char *did, unsigned int len)
{
    unsigned char status;
    unsigned int para_len;
    int ret;

    if (1 > len)
    {
        log_e(LOG_UDS, "get can1 len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    para_len = sizeof(status);
    ret = st_get(ST_ITEM_CAN1_STATUS, &status, &para_len);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get can1 status error, ret:%d", ret);
        return UDS_STATUS_INVALID;
    }

    did[0] = status;

    return 0;
}

int uds_did_get_can2(unsigned char *did, unsigned int len)
{
    unsigned char status;
    unsigned int para_len;
    int ret;

    if (1 > len)
    {
        log_e(LOG_UDS, "get can2 len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    para_len = sizeof(status);
    ret = st_get(ST_ITEM_CAN2_STATUS, &status, &para_len);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get can2 status error, ret:%d", ret);
        return UDS_STATUS_INVALID;
    }

    did[0] = status;

    return 0;
}

int uds_did_get_pm_mode(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get pm mode len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(unsigned char);
    return cfg_get_para(CFG_ITEM_SLEEP_MODE, did, &length);
}

int uds_did_get_can3(unsigned char *did, unsigned int len)
{
    unsigned char status;
    unsigned int para_len;
    int ret;

    if (1 > len)
    {
        log_e(LOG_UDS, "get can3 len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    para_len = sizeof(status);
    ret = st_get(ST_ITEM_CAN3_STATUS, &status, &para_len);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get can3 status error, ret:%d", ret);
        return UDS_STATUS_INVALID;
    }

    did[0] = status;

    return 0;

}

#if 0
int uds_did_get_car_mode(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get car mode len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(unsigned char);
    return cfg_get_para(CFG_ITEM_UDS_CAR_MODE, did, &length);
}

int uds_did_set_car_mode(unsigned char *did, unsigned int len)
{
    if (1 > len)
    {
        log_e(LOG_UDS, "set car mode len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_UDS_CAR_MODE, did, 1);
}
#endif


int uds_did_get_apn2(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned int ret = 0;		

	  if (32 > len)
    {
        log_e(LOG_UDS, "get local apn len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 32;
    ret = cfg_get_para(CFG_ITEM_WAN_APN, did, &length);

   return ret;
}

int uds_did_get_wifi(unsigned char *did, unsigned int len)
{
    unsigned char status = 0;

    if (1 > len)
    {
        log_e(LOG_UDS, "get wifi status len error");
        return UDS_INVALID_PARA;
    }

    status = dev_diag_get_wifi_status();

    if ((0 == status) || (1 == status))
    {
        did[0] = 0;
    }
    else
    {
        did[0] = 1;
    }

    return 0;
}

int uds_did_get_dbc_name(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned char dbc_path[256] = {0};

    if (50 > len)
    {
        log_e(LOG_UDS, "get dbc name len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 256;
    cfg_get_para(CFG_ITEM_DBC_PATH, dbc_path, &length);
    memcpy(did, dbc_path, 50);

    return 0;
}


int uds_did_get_vin(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char VIN[18];
    memset(VIN, 0x00, 18);
    int ret = 0;
    if (DID_LEN_VIN > len)
    {
        log_e(LOG_UDS, "get vin len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 18;
    ret = cfg_get_user_para(CFG_ITEM_GB32960_VIN, VIN, &length);

    if(ret)
    {
        log_e(LOG_UDS, "get vin len error, Storage error");
        memset(did, 0x00, DID_LEN_VIN);
    }
    else
    {
        memcpy(did, VIN, DID_LEN_VIN);
    }
    return 0;
}

int uds_did_set_vin(unsigned char *did, unsigned int len)
{
    char VIN[18];
    int ret = 0;
    memset(VIN, 0x00, 18);
    
    if (DID_LEN_VIN != len)
    {
        log_e(LOG_UDS, "set vin len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    memcpy(VIN, did, len);
    ret = cfg_set_user_para(CFG_ITEM_GB32960_VIN, VIN, 18);
    if(ret)
    {
        return ret;
    }
    gb_set_vin(VIN);
    return 0;
}


int uds_did_set_installation_date(unsigned char *did, unsigned int len)
{
    if (DID_LEN_INSTALLATION_DATE != len)
    {
        log_e(LOG_UDS, "set installation date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_DID_INSTALL_DATE, did, DID_LEN_INSTALLATION_DATE); 
}


int uds_did_set_tester_sn(unsigned char *did, unsigned int len)
{
    if (DID_LEN_TESTER_SN != len)
    {
        log_e(LOG_UDS, "set SN len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_DID_TESTER_SN, did, DID_LEN_TESTER_SN); 

}




int uds_did_get_dev_no(unsigned char *did, unsigned int len)
{
    unsigned int length, j;
    unsigned int dev_no;
    unsigned char tempbuf[20];
    unsigned char tempbuf1[20];

    if (9 > len)
    {
        log_e(LOG_UDS, "get dev no len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(int);
    cfg_get_para(CFG_ITEM_DEV_NUM, (unsigned char *)&dev_no, &length);

    memset(tempbuf, 0, sizeof(tempbuf));
    memset(tempbuf, 0, sizeof(tempbuf));
    sprintf((char *)tempbuf, "%s", "E00000000");
    sprintf((char *)tempbuf1, "%u", dev_no);
    length = strlen((const char *)tempbuf1);

    if (length < 9)
    {
        for (j = 0; j < length; j++)
        {
            tempbuf[8 - j] = tempbuf1[length - j - 1];
        }
    }

    memcpy(did, tempbuf, 9);
    return 0;
}

int uds_did_get_btl_soft_finger(unsigned char *did, unsigned int len)
{
    if (10 > len)
    {
        log_e(LOG_UDS, "set soft finger len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

     did[0] = 'T';/*T*/
     did[1] = 'B';/*B*/
     did[2] = 'X';/*X*/
     did[3] = 0x20;
     did[4] = 0x18;
     did[5] = 0x04;
     did[6] = 0x21;
     did[7]= '0';
     did[8]= '0';
     did[9]= '1';
				
    return 0;
    //return cfg_set_para(CFG_ITEM_UDS_SOFT_FINGER, did, 9);
}

#if 0
int uds_did_get_app_soft_finger(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (9 > len)
    {
        log_e(LOG_UDS, "get local apn len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 9;
    return cfg_get_para(CFG_ITEM_UDS_SOFT_FINGER, did, &length);
}

int uds_did_set_app_soft_finger(unsigned char *did, unsigned int len)
{
    if (9 > len)
    {
        log_e(LOG_UDS, "set soft finger len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_UDS_SOFT_FINGER, did, 9);
}
#endif

int uds_did_get_supply_code(unsigned char *did, unsigned int len)
{
    if (10 > len)
    {
        log_e(LOG_UDS, "set supply code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memcpy(did, SUPPLYNAME, 10);

    return 0;
}

int uds_did_get_ecu_hard_ver(unsigned char *did, unsigned int len)
{
    if (9 > len)
    {
        log_e(LOG_UDS, "ecu hard version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return 0;
}

int uds_did_get_ecu_soft_ver(unsigned char *did, unsigned int len)
{
    if (9 > len)
    {
        log_e(LOG_UDS, "ecu soft version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return 0;
}

#if 0
int uds_did_get_pro_date(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (3 > len)
    {
        log_e(LOG_UDS, "get pro data len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 3;
    return cfg_get_para(CFG_ITEM_UDS_PROGRAM_DATA, did, &length);
}

int uds_did_set_pro_date(unsigned char *did, unsigned int len)
{
    if (3 > len)
    {
        log_e(LOG_UDS, "set pro date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_UDS_PROGRAM_DATA, did, 3);
}
#endif

int uds_did_get_tuid(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned int sn = 0;

    if (sizeof(int) > len)
    {
        log_e(LOG_UDS, "get tuid len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(int);
    cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char *)&sn, &length);
    did[0] = (uint8_t)(sn >> 24) & 0xff;
    did[1] = (uint8_t)(sn >> 16) & 0xff;
    did[2] = (uint8_t)(sn >> 8)  & 0xff;
    did[3] = (uint8_t)(sn >> 0)  & 0xff;

    return 0;
}

int uds_did_get_checksum_status(unsigned char *did, unsigned int len)
{
    if (1 > len)
    {
        log_e(LOG_UDS, "get uncfm dtc len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    did[0] = 0;

    //log_i(LOG_UDS, " DTC status is %s", uds_diag_get_uncfm_dtc_num() ? " DTCIsValid" : " DTCIsInvalid");

    return 0;
}

int uds_did_get_tbox_status(unsigned char *did, unsigned int len)
{
    if (1 > len)
    {
        log_e(LOG_UDS, "get uncfm dtc len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    did[0] = uds_diag_get_uncfm_dtc_num() ? 1 : 0;

    log_i(LOG_UDS, " DTC status is %s", uds_diag_get_uncfm_dtc_num() ? " DTCIsValid" : " DTCIsInvalid");

    return 0;
}


/* developed */
int uds_did_get_odometer_reading(unsigned char *did, unsigned int len)
{
    long vehicle_odograph = 0;
    if (DID_LEN_ODOMETER_READING > len)
    {
        log_e(LOG_UDS, "get Odometer Reading len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    vehicle_odograph = gb_data_vehicleOdograph();

    did[0] = (vehicle_odograph & 0xff000000) << 24;
    did[1] = (vehicle_odograph & 0x00ff0000) << 16;
    did[2] = (vehicle_odograph & 0x0000ff00) << 8;
    did[3] = (vehicle_odograph & 0x000000ff);
    return 0;
}

/* developed */
int uds_did_get_vehicle_speed(unsigned char *did, unsigned int len)
{
    long vehicle_speed = 0;
    if (DID_LEN_VEHICLE_SPEED > len)
    {
        log_e(LOG_UDS, "get vehicle speed len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    vehicle_speed = gb_data_vehicleSpeed() * 10;

    did[0] = (vehicle_speed & 0x0000ff00) >> 8;
    did[1] = (vehicle_speed & 0x000000ff);
    return 0;
}

/* developed */
int uds_did_get_ignition_status(unsigned char *did, unsigned int len)
{
    uint8_t sign;
    int sign_len = sizeof(sign);

    if (DID_LEN_IGNITION_STATUS > len)
    {
        log_e(LOG_UDS, "get ignition status len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    if (st_get(ST_ITEM_KL15_SIG, (void *)&sign, (void *)&sign_len) != 0)
    {
        log_e(LOG_UDS, "get ignition status len error, st_get error!");
        did[0] = 0;
        return 0;
    }

    if(1 ==  sign)
    {
        did[0] |= 0x04;
    }
    else
    {
        did[0] &= 0xFB;
    }
     
    return 0;
}

/* developed*/
int uds_did_get_platform_connection_status(unsigned char *did, unsigned int len)
{
    int platform_status = 0;
    
    if (DID_LEN_PLATFORM_CONNECTION_STATUS > len)
    {
        log_e(LOG_UDS, "get platform connection status len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    
    platform_status = sockproxy_socketState();

    if(1 == platform_status)/*connected*/
    {
        did[0] = 0;
    }
    else
    {
        did[0] = 1;
    }

    return 0;
}

/*  StoredData DID add by caoml*/
int uds_did_get_bootloader_identifier(unsigned char *did, unsigned int len)
{
    if (DID_LEN_BOOTLOADER_IDENTIFIER > len)
    {
        log_e(LOG_UDS, "get bootloader identifier len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0x00, DID_LEN_BOOTLOADER_IDENTIFIER);
    return 0;
}
int uds_did_get_diagnostic_session(unsigned char *did, unsigned int len)
{
    if (DID_LEN_DIAGNOSTIC_SESSION > len)
    {
        log_e(LOG_UDS, "get diagnostic session len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    did[0] = Get_Session_Current();

    return 0;
}
int uds_did_get_spare_part_number(unsigned char *did, unsigned int len)
{
    if (DID_LEN_SPARE_PART_NUMBER > len)
    {
        log_e(LOG_UDS, "get spare part number len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    memset(did,0x20,DID_LEN_SPARE_PART_NUMBER);
    memcpy(did, DID_F187_SPARE_PART_NO, 11);
    
    return 0;
}
int uds_did_get_software_upgrade_version(unsigned char *did, unsigned int len)
{
    #if 0
    unsigned int length;
    #endif
    
    if (DID_LEN_SOFTWARE_UPGRADE_VERSION > len)
    {
        log_e(LOG_UDS, "get software upgrade version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    #if 0
    length = DID_LEN_SOFTWARE_UPGRADE_VERSION;
    return cfg_get_para(CFG_ITEM_DID_SW_UPGRADE_VER, did, &length);
    #endif
    memcpy(did, DID_F1B0_SW_UPGRADE_VER, DID_LEN_SOFTWARE_UPGRADE_VERSION);
    return 0;
}
int uds_did_get_software_fixed_version(unsigned char *did, unsigned int len)
{
    if (DID_LEN_SOFTWARE_FIXED_VERSION > len)
    {
        log_e(LOG_UDS, "get software fixed version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    memcpy(did, DID_F1B0_SW_FIXED_VER, DID_LEN_SOFTWARE_FIXED_VERSION);
    
    return 0;
}
int uds_did_get_calibration_software_number(unsigned char *did, unsigned int len)
{
    if (DID_LEN_CALIBRATION_SOFTWARE_NUMBER > len)
    {
        log_e(LOG_UDS, "get calibration software number len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0x00, DID_LEN_CALIBRATION_SOFTWARE_NUMBER);
    return 0;
}
int uds_did_get_supplier_code(unsigned char *did, unsigned int len)
{
    if (DID_LEN_SUPPLIER_CODE > len)
    {
        log_e(LOG_UDS, "get supplier code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memcpy(did, DID_F18A_SUPPLIER_IDENTIFIER, DID_LEN_SUPPLIER_CODE);
    return 0;
}

unsigned char hex_to_BCD(unsigned char sdata)
{
    return (sdata/0x0A)*0x10 + (sdata%0x0A);
}
int uds_did_get_manufacture_date(unsigned char *did, unsigned int len)
{
    char SN[PP_TBOXSN_LEN];
    memset(SN,0x00,DID_LEN_SN);
    int ret = 0;
    int did_cur_len = 0;
    int mdate_day = 0;
    int mdate_month = 0;
    
    if (DID_LEN_MANUFACTURE_DATE > len)
    {
        log_e(LOG_UDS, "get manufacture date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    
    PrvtProt_gettboxsn((char *)SN);
    
    /*Conversion year according to the SN rules of the HOZON*/

    if(SN[9] < 'A')
    {
        did[did_cur_len] = 0x00;
        did_cur_len++;
        did[did_cur_len] = 0x00;
        did_cur_len++;
    }
    else if(SN[9] > 'Z')
    {
        did[did_cur_len] = 0x00;
        did_cur_len++;
        did[did_cur_len] = 0x00;
        did_cur_len++;
    }
    else if( (SN[9] == 'I') || (SN[9] == 'O') || (SN[9] == 'Q') || (SN[9] == 'X') || (SN[9] == 'Z') )
    {
        did[did_cur_len] = 0x00;
        did_cur_len++;
        did[did_cur_len] = 0x00;
        did_cur_len++;
    }
    else if(SN[9] < 'I')
    {
        did[did_cur_len] = hex_to_BCD(20);
        did_cur_len++;
        did[did_cur_len] = hex_to_BCD(SN[did_cur_len+8] - 0x41 + 19);
        did_cur_len++;
    }
    else if(SN[9] < 'O')
    {
        did[did_cur_len] = hex_to_BCD(20);
        did_cur_len++;
        did[did_cur_len] = hex_to_BCD(SN[did_cur_len+8] - 0x41 + 19 -1);
        did_cur_len++;
    }
    else if(SN[9] < 'Q')
    {
        did[did_cur_len] = hex_to_BCD(20);
        did_cur_len++;
        did[did_cur_len] = hex_to_BCD(SN[did_cur_len+8] - 0x41 + 19 - 2);
        did_cur_len++;
    }
    else if(SN[9] < 'X')
    {
        did[did_cur_len] = hex_to_BCD(20);
        did_cur_len++;
        did[did_cur_len] = hex_to_BCD(SN[did_cur_len+8] - 0x41 + 19 - 3);
        did_cur_len++;
    }
    else if(SN[9] < 'Z')
    {
        did[did_cur_len] = hex_to_BCD(20);
        did_cur_len++;
        did[did_cur_len] = hex_to_BCD(SN[did_cur_len+8] - 0x41 + 19 - 4);
        did_cur_len++;
    }
    


    /*Conversion month according to the SN rules of the HOZON*/
    if(sscanf(SN + 10, "%1x", &mdate_month) == 1)
    {
        if(mdate_month > 0x0C)
        {
            did[did_cur_len] = 0x00;
            did_cur_len++;
        }
        else
        {
            did[did_cur_len] = hex_to_BCD(mdate_month);
            did_cur_len++;
        }
    }
    else
    {
        did[did_cur_len] = 0x00;
        did_cur_len++;
    }

    
    /*Conversion day according to the SN rules of the HOZON*/
    if(sscanf(SN + 11, "%2x", &mdate_day) == 1)
    {
        if(mdate_day>0x31)
        {
            did[did_cur_len] = 0x00;
            did_cur_len++;
        }
        else
        {
            did[did_cur_len] = mdate_day;
            did_cur_len++;
        }
    }
    else
    {
        did[did_cur_len] = 0x00;
        did_cur_len++;

    }

    return ret;
}

#if 0
int uds_did_set_manufacture_date(unsigned char *did, unsigned int len)
{
    if (DID_LEN_MANUFACTURE_DATE != len)
    {
        log_e(LOG_UDS, "set manufacture date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_DID_MDATE, did, len);
}
#endif



int uds_did_get_sn(unsigned char *did, unsigned int len)
{

    char pp_tboxsn[PP_TBOXSN_LEN];
    memset(pp_tboxsn, 0x00, PP_TBOXSN_LEN);

    if (DID_LEN_SN > len)
    {
        log_e(LOG_UDS, "get sn len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    PrvtProt_gettboxsn((char *)pp_tboxsn);
    
    memcpy(did, pp_tboxsn, DID_LEN_SN);

    return 0;
}
int uds_did_get_hw_version(unsigned char *did, unsigned int len)
{
    if (DID_LEN_HW_VERSION > len)
    {
        log_e(LOG_UDS, "get hw version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    memset(did, 0x00, DID_LEN_HARD_NO);
    
    char cfgHW[32]= {0};
    unsigned int cfgHWLen = sizeof(cfgHW);
    int getCfgRet = cfg_get_para(CFG_ITEM_INTEST_HW, cfgHW, &cfgHWLen);
    if(getCfgRet == 0)
    {
        memcpy(did, cfgHW, DID_LEN_HARD_NO);
    }
    else
    {
        log_e(LOG_UDS, "get INTEST HW error ret:%d!", getCfgRet);
    }
    return 0;
}
int uds_did_get_tester_sn(unsigned char *did, unsigned int len)
{
    unsigned int length = DID_LEN_TESTER_SN;

    if (DID_LEN_TESTER_SN > len)
    {
        log_e(LOG_UDS, "get tester sn len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_get_para(CFG_ITEM_DID_TESTER_SN, did, &length);
}
int uds_did_get_programming_date(unsigned char *did, unsigned int len)
{
    if (DID_LEN_PROGRAMMING_DATE > len)
    {
        log_e(LOG_UDS, "get programming date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0x00, DID_LEN_PROGRAMMING_DATE);
    return 0;
}
int uds_did_get_installation_date(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (DID_LEN_INSTALLATION_DATE > len)
    {
        log_e(LOG_UDS, "get installation date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = DID_LEN_INSTALLATION_DATE;
    return cfg_get_para(CFG_ITEM_DID_INSTALL_DATE, did, &length);
}
int uds_did_get_configuration_code(unsigned char *did, unsigned int len)
{
    unsigned int length;
    
    if (DID_LEN_CONFIGURATION_CODE > len)
    {
        log_e(LOG_UDS, "get configuration code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = DID_LEN_CONFIGURATION_CODE;
    
    return cfg_get_para(CFG_ITEM_EN_BLE, did, &length);
}

int uds_did_set_configuration_code(unsigned char *did, unsigned int len)
{
    if (DID_LEN_CONFIGURATION_CODE != len)
    {
        log_e(LOG_UDS, "set configuration code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    if(did[0] > 0x07)
    {
        log_e(LOG_UDS, "set configuration code value error, value:%d", did[0]);
        return NRC_RequestOutOfRange;
    }
    return cfg_set_para(CFG_ITEM_EN_BLE, did, DID_LEN_CONFIGURATION_CODE);
}


int set_cfg_code(unsigned char cfg_code)
{
    if(cfg_code > 0x07)
    {
        log_e(LOG_UDS, "cfg code value is illegal!");
        return -1;
    }
    return cfg_set_para(CFG_ITEM_EN_BLE, &cfg_code, DID_LEN_CONFIGURATION_CODE); 
}


unsigned char get_ble_ctl(void)
{
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "get ble ctl error, storage error,ret:%d", ret_get_cfg);
        return 0x01;/* default value */
    }
    return (cfg_code[0] & 0x01);
}

int set_ble_ctl(unsigned char ble_ctl)
{
    if(ble_ctl > 0x01)
    {
        log_e(LOG_UDS, "Bluetooth configuration value is illegal!, ble_ctl:%x", ble_ctl);
        return -1;
    }
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "set ble ctl error, storage error,ret:%d", ret_get_cfg);
        return -1;
    }
    cfg_code[0] = (cfg_code[0] & 0xFE) | (ble_ctl & 0x01);
    return cfg_set_para(CFG_ITEM_EN_BLE, cfg_code, DID_LEN_CONFIGURATION_CODE); 
}

unsigned char get_ota_ctl(void)
{
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "get ota cfg error, storage error,ret:%d", ret_get_cfg);
        return 0;/* default value */
    }
    return (cfg_code[0] & 0x02) >> 1;
}

int set_ota_ctl(unsigned char ota_cfg)
{
    if(ota_cfg > 0x01)
    {
        log_e(LOG_UDS, "OTA Ctrl value is illegal!");
        return -1;
    }
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "set ota cfg error, storage error,ret:%d", ret_get_cfg);
        return 0;
    }
    cfg_code[0] = (cfg_code[0] & 0xFD) | ((ota_cfg & 0x01) << 1);
    return cfg_set_para(CFG_ITEM_EN_BLE, cfg_code, DID_LEN_CONFIGURATION_CODE); 
}

unsigned char get_factory_mode(void)
{
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "get factory mode error, Storage error,ret:%d", ret_get_cfg);
        return 1;/* default value */
    }
    return (cfg_code[0] & 0x04) >> 2;
}


int set_factory_mode(unsigned char factory_mode)
{
    if(factory_mode > 0x01)
    {
        log_e(LOG_UDS, "Factory mode value is illegal!");
        return -1;
    }
    unsigned char cfg_code[DID_LEN_CONFIGURATION_CODE] = {0x00};
    unsigned int length = DID_LEN_CONFIGURATION_CODE;

    int ret_get_cfg = cfg_get_para(CFG_ITEM_EN_BLE, cfg_code, &length);
    if(ret_get_cfg != 0)
    {
        log_e(LOG_UDS, "set factory mode error, Storage error,ret:%d", ret_get_cfg);
        return 1;
    }
    cfg_code[0] = (cfg_code[0] & 0xFB) | ((factory_mode & 0x01) << 2);
    return cfg_set_para(CFG_ITEM_EN_BLE, cfg_code, DID_LEN_CONFIGURATION_CODE); 
}

int uds_did_get_phone(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char tel[TBOX_TEL_LEN];

    if (DID_LEN_PHONE > len)
    {
        log_e(LOG_UDS, "get phone error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_telno(tel);
    memset(did, 0, len);

	length = DID_LEN_PHONE;
	if(strlen(tel))
	{
   		memcpy(did, tel, length);
	}else
	{
		length = 13;
		cfg_get_para(CFG_ITEM_FT_SIM, did, &length);
	}
	
    return 0;
}
int uds_did_get_iccid(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readiccid[CCID_LEN];

    if (DID_LEN_ICCID > len)
    {
        log_e(LOG_UDS, "get iccid error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    nm_get_iccid((char *)readiccid);
    memset(did, 0, len);

    length = DID_LEN_ICCID;
    memcpy(did, readiccid, length);

    return 0;

}
int uds_did_get_imsi(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readimsi[TBOX_IMSI_LEN];

    if (DID_LEN_IMSI > len)
    {
        log_e(LOG_UDS, "get IMSI error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_imsi(readimsi);
    memset(did, 0, len);

    length = DID_LEN_IMSI;
    memcpy(did, readimsi, length);

    return 0;

}
int uds_did_get_imei(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readimei[TBOX_IMEI_LEN];

    if (DID_LEN_IMEI > len)
    {
        log_e(LOG_UDS, "get IMEI error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_imei(readimei);

    memset(did, 0, len);

    length = DID_LEN_IMEI;
    memcpy(did, readimei, length);

    return 0;

}

int uds_did_get_hard_no(unsigned char *did, unsigned int len)
{
    if (DID_LEN_HARD_NO > len)
    {
        log_e(LOG_UDS, "get Vehicle Manufacturer ECU Hardware Number Data Identifier len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memcpy(did, DID_F1BF_HW_VERSION, DID_LEN_HARD_NO);
    
    return 0;
}

int uds_did_get_soft_no(unsigned char *did, unsigned int len)
{
    if (DID_LEN_SOFT_NO > len)
    {
        log_e(LOG_UDS, "get Vehicle Manufacturer ECU Software Assembly Number Data Identifier len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
    memcpy(did, DID_F1B0_SW_UPGRADE_VER, DID_LEN_SOFT_NO);
    
    return 0;
}

int uds_did_get_esk(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (DID_LEN_ESK > len)
    {
        log_e(LOG_UDS, "get ESK len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = DID_LEN_ESK;
    return cfg_get_para(CFG_ITEM_DID_ESK, did, &length);

}
int uds_did_get_pki_status(unsigned char *did, unsigned int len)
{
    if (DID_LEN_PKI_STATUS > len)
    {
        log_e(LOG_UDS, "get pki status len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0x01, DID_LEN_PKI_STATUS);
    return 0;

}

int uds_did_set_esk(unsigned char *did, unsigned int len)
{
    if (DID_LEN_ESK != len)
    {
        log_e(LOG_UDS, "set ESK len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_DID_ESK, did, DID_LEN_ESK); 

}




