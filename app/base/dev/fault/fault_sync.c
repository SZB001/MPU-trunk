/****************************************************************************
 file:         fault.h
 description:  the header file of ucom message en/decode implemention
 date:         2017/7/9
 copyright     Wuhan Intest Electronic Technology Co.,Ltd
 author        wangqinglong
 *****************************************************************************/
#include "com_app_def.h"
#include "fault_sync.h"
#include "diag.h"
#include "log.h"

DIAG_MCU_FAULT mcu_fault;
DIAG_MPU_FAULT mpu_fault;
static pthread_mutex_t fault_sycn_mutex;

/*******************************************************************
 function:     flt_sync_init
 description:  initize uart message module
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
int flt_sync_init(void)
{
    pthread_mutex_init(&fault_sycn_mutex, NULL);
    return 0;
}

/*******************************************************************
 function:     flt_get_mcu
 description:  get the mcu fault message
 input:        none
 output:       DIAG_MCU_FAULT *fault
 return:       none
 ********************************************************************/
void flt_get_mcu(DIAG_MCU_FAULT *fault)
{
    pthread_mutex_lock(&fault_sycn_mutex);
    memcpy(fault, &mcu_fault, sizeof(DIAG_MCU_FAULT));
    pthread_mutex_unlock(&fault_sycn_mutex);
}

/*******************************************************************
 function:     flt_get_mpu
 description:  get the mcu fault message
 input:        none
 output:       DIAG_MCU_FAULT *fault
 return:       none
 ********************************************************************/
void flt_get_mpu(DIAG_MPU_FAULT *fault)
{
    pthread_mutex_lock(&fault_sycn_mutex);
    memcpy(fault, &mpu_fault, sizeof(DIAG_MPU_FAULT));
    pthread_mutex_unlock(&fault_sycn_mutex);
}

/*******************************************************************
 function:     flt_get_by_id
 description:  get devices fault by id
 input:        DEV_FLT id
 output:       none
 return:       0 indicates unkown;
 1 indicates ok;
 others indicates failed
 ********************************************************************/
int flt_get_by_id(DEV_FLT id)
{
    switch (id)
    {
        case GPS_ANT:
            return mcu_fault.gps_ant;

        case CAN_NODE1:
            return mcu_fault.can_node[0];

        case CAN_NODE2:
            return mcu_fault.can_node[1];

        case CAN_NODE3:
            return mcu_fault.can_node[2];

        case CAN_NODE4:
            return mcu_fault.can_node[3];

        case CAN_NODE5:
            return mcu_fault.can_node[4];

        case CAN_BUS1:
            return mcu_fault.can_busoff[0];

        case CAN_BUS2:
            return mcu_fault.can_busoff[1];

        case CAN_BUS3:
            return mcu_fault.can_busoff[2];

        case CAN_BUS4:
            return mcu_fault.can_busoff[3];

        case CAN_BUS5:
            return mcu_fault.can_busoff[4];

        case POWER:
            return mcu_fault.voltage;

        case BAT:
            return mcu_fault.battery;

        case MIC:
            return mcu_fault.mic;

        case MICSTATUS:         //added by Cindy following 5
            return mcu_fault.micstatus;

        case SOSBTN:
            return mcu_fault.sosbtn;

        case SPK:
            return mcu_fault.spkstatus;

        case GSENSE:
            return mcu_fault.gsensestatus;

        case BATVOL:
            return mcu_fault.batteryvol;

        case GPS:
            return mpu_fault.gps;

        case EMMC:
            return mpu_fault.emmc;

        case GPRS:
            return mpu_fault.gprs;

        case SIM:
            return mpu_fault.sim;

        case WIFI:
            return mpu_fault.wifi;

        case USB:
            return mpu_fault.usb;

        default:
            log_e(LOG_DEV, "para error,id:%d", id);
            return -1;
    }
}

/*******************************************************************
 function:     flt_sync_to_mcu
 description:  sync mpu fault to mcu
 input:        none
 output:       DIAG_MCU_FAULT *fault
 return:       none
 ********************************************************************/
void flt_sync_to_mcu(DIAG_MPU_FAULT *fault)
{
    fault->emmc = dev_diag_get_emmc_status();
    fault->gprs = dev_diag_get_4G_status();
    fault->gps  = dev_diag_get_gps_status();
    fault->sim  = dev_diag_get_sim_status();
    fault->usb  = dev_diag_get_usb_status();
    fault->wifi = dev_diag_get_wifi_status();

    pthread_mutex_lock(&fault_sycn_mutex);
    memcpy(&mpu_fault, fault, sizeof(mpu_fault));
    pthread_mutex_unlock(&fault_sycn_mutex);
}

/*******************************************************************
 function:     fault_sync_mcu
 description:  get the mcu fault message
 input:        none
 output:       DIAG_MCU_FAULT *fault
 return:       none
 ********************************************************************/
void flt_sync_from_mcu(unsigned char *data, unsigned int len)
{
    if (sizeof(DIAG_MCU_FAULT) != len)
    {
        return;
    }

    pthread_mutex_lock(&fault_sycn_mutex);
    memcpy(&mcu_fault, data, sizeof(mcu_fault));
    pthread_mutex_unlock(&fault_sycn_mutex);
}

