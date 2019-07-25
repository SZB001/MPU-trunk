/****************************************************************
 file:         bt_interface.c
 description:  the header file of bt_interface function implemention
 date:         2018/5/9
 author:       liuquanfu
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "com_app_def.h"
#include "init.h"
#include "bt_interface.h"
#include "tcom_api.h"
#include "init.h"
#include "gpio.h"
#include "timer.h"
#include "pm_api.h"
#include "at_api.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "shell_api.h"
#include "scom_api.h"
#include "signal.h"
#include "cm256_if.h"


typedef struct ST_BT_MODULE_
{
	int (*Init)(void);
	int (*Open)(void);
	int (*Close)(void);
	int (*LinkDrop)(void);						  
	
	int (*GetName)(unsigned char *paucOutBtName, unsigned char *pucOutLen);
	int (*SetName)(unsigned char *paucInBtName, unsigned char *pucInLen);
	int (*GetMac)(unsigned char *pucBuf, unsigned char *pucLen);
	int (*EnPower)(unsigned char ucEn);
	int (*GetUuid)(unsigned short *usUuid, unsigned char ucUuidCnt);
	int (*GetRssi)(unsigned short *usRssi, unsigned char ucLen);

	int (*GetState)(void);
	int (*Send)(unsigned char *pucBuf, unsigned int *pulLen);	
	int (*Recv)(unsigned char *pucBuf, unsigned int *pulLen);  

	unsigned char ucModuleType; 
}ST_BT_MI, *PST_BT_MI;

ST_BT_API					stBtApi; 
extern BLE_MEMBER			g_BleMember;
int ApiBleSend(unsigned char *pucBuf, unsigned int *ulLen);


ST_BT_MI stBtMi = 
{
    cm256_init, 
	cm256_open,
	cm256_close,
	cm256_disconnect,
	
	cm256_get_name,
	cm256_set_name,
	cm256_get_mac,
	cm256_enpower,
	cm256_get_uuid,
	cm256_get_rssi,
	
	cm256_get_state,
	cm256_send,
	cm256_recv,
    MODULE_CM256,
};


int ApiBleSend(unsigned char *pucBuf, unsigned int *ulLen)
{
	//if (BLE_RECV_FINISH_STATUS == g_BleMember.ucTransStatus)
	{
	    //log_e(LOG_BLE, "ApiBleSend\r\n");
		//log_e(LOG_BLE, "ApiBleSend=%d\r\n",*ulLen);
		stBtMi.Send(pucBuf, ulLen);
		g_BleMember.ucTransStatus = BLE_SEND_STATUS;
	}
	return YT_OK;
}

void BT_Interface_Init(void)
{
    //api init
    stBtApi.Init			= stBtMi.Init;
    stBtApi.Open			= stBtMi.Open;
    stBtApi.Close			= stBtMi.Close;
    stBtApi.LinkDrop		= stBtMi.LinkDrop;
    stBtApi.GetState		= stBtMi.GetState;
    stBtApi.GetName			= stBtMi.GetName;
	stBtApi.SetName			= stBtMi.SetName;
    stBtApi.GetMac			= stBtMi.GetMac;
    stBtApi.Send			= ApiBleSend;
    stBtApi.Recv			= stBtMi.Recv;
    stBtApi.EnPower			= stBtMi.EnPower;
    stBtApi.GetUuid			= stBtMi.GetUuid;
    stBtApi.GetRssi			= stBtMi.GetRssi;
    stBtApi.ucModuleType 	= stBtMi.ucModuleType;   
}


int ApiCheckLen(void)
{
	return cm256_check_len();
}


