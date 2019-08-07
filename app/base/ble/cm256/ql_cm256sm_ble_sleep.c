#include <ql_oe.h>
#include "ql_qcmap_client_api.h"
#include "ql_eint.h"
#include "ql_cm256sm_ble_sleep.h"
#include "log.h"
#include "ble.h"
#include "tcom_api.h"

static int wakelock_fd = -1;
static int wakelock_flag = -1; /*lock or unlock flag. lock(wake):1 unlock(sleep):0*/
static int pin_value = 0;

Enum_PinName BT_HostWakePin = PINNAME_GPIO1;
/****************************************************************
 function:     ble_ring_wakeup
 description:  when there is message or ring, notify pm module
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
int ble_ring_wakeup( void )
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    msghdr.sender    = MPU_MID_BLE;
    msghdr.receiver  = MPU_MID_PM;
    msghdr.msgid     = BLE_MSG_ID_RING;
    msghdr.msglen    = 0;

    /* send msg id to pm */
    ret = tcom_send_msg(&msghdr, NULL);
    if (ret != 0)
    {
        log_e(LOG_BLE, "tcom_send_msg failed, ret:%u", ret);
    }

    return ret;
}


/*BT_HOST_WAKE Pin Interrupt Handler*/
static void bt_eint_callback(Enum_PinName PinName, int level)
{

#if 0
         if(0 == level)
             ql_bt_lock_wakelock();
#endif
    pin_value = level;
	log_i(LOG_BLE,"bt_eint_callback******************************1pin_value:%d\n", pin_value);

    if (1 == pin_value)
	{
		//pm_send_evt(MPU_MID_BLE, PM_EVT_RING);
		 ble_ring_wakeup();
	}
	 
}

/*Prepare for BLE sleep/wake*/
int ql_ble_sleep_init()
{
    int ret;
    Enum_EintType senseType = EINT_SENSE_FALLING;

    wakelock_fd = Ql_SLP_WakeLock_Create("low_power_bt", sizeof("low_power_bt"));
    if(wakelock_fd<0)
    {
        log_e(LOG_BLE,"Create Sleep Lock fail");
		return -1;
    }

    ret = Ql_GPIO_SetPullSelection(BT_HostWakePin, PINPULLSEL_PULLUP);
    if(ret<0)
    {
        log_e(LOG_BLE,"Set BT_HOST_WAKE fail\n");
		return -1;
    }

    ret = Ql_EINT_Enable(BT_HostWakePin, senseType, bt_eint_callback);
    if(ret<0)
    {
        log_e(LOG_BLE,"BT_HOST_WAKE Interrupt Enable fail\n");
		return -1;
    }

    return 0;
}

/*Lock wakeLock to keep wake*/
void ql_bt_lock_wakelock() //system awake
{
	
    if(wakelock_flag != 1)
    {
        ble_ring_wakeup();
        wakelock_flag = 1;
        if(Ql_SLP_WakeLock_Lock(wakelock_fd) != 0)
		{
		    Ql_SLP_WakeLock_Destroy(wakelock_fd);
		    log_i(LOG_BLE,"Lock wakelock failed!\n");
		    wakelock_flag = 0;
		    return;
		}
		log_i(LOG_BLE,"Lock wakelock Success!\n");
    }
	
}

/*Unlock wakeLock to sleep*/
void ql_bt_unlock_wakelock() //system sleep
{
    if(wakelock_flag == 1)
    {
        wakelock_flag = 0;
        if(Ql_SLP_WakeLock_Unlock(wakelock_fd) != 0)
        {
	    	log_i(LOG_BLE,"Unlock wakelock failed!\n");
	    	Ql_SLP_WakeLock_Destroy(wakelock_fd);
            wakelock_flag = 1;
	   	 	return;
        }
		log_i(LOG_BLE,"Unlock wakelock Success!\n");
    }
}


