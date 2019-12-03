/****************************************************************
file:         at.h
description:  the header file of at api definition
date:         2017/7/12
author        wangqinglong
****************************************************************/
#ifndef __BLE_H__
#define __BLE_H__

#include "mid_def.h"
#include "init.h"

#define MODULE_CM256					1
#define MODULE_NF3303					2
#define BLE_NAME_SIZE               	250
#define BLE_TIMEOUT_1S             		(1000)           //2000ms
#define MAX_LEN							1024

#define BLE_TIMER_CMD				    200
#define COM_APP_DATA_BT_DIR           		"/usrdata/bt/"
#define COM_APP_SERVER_DATA          		"bt-daemon-socket"




#define HIBYTE(X)                       ((unsigned char)((X) >> 8))
#define LOBYTE(X)                       ((unsigned char)((X) & 0xFF))
#define HIWORD(l)                       ((unsigned short)(((unsigned int)(l) >> 16) & 0xFFFF))
#define LOWORD(l)                       ((unsigned short)(l))


#define MAKEDWORD(FHiByte, SHiByte, FLoByte, SLoByte)	((((unsigned int)(FHiByte)<<24)&0xFF000000) | \
														(((unsigned int)(SHiByte)<<16)&0xFF0000) | \
														(((unsigned int)(FLoByte)<<8)&0xFF00) | (((unsigned int)(SLoByte))&0xFF))



/* return */
#define YT_OK							 				0
#define YT_ERR									(-1)  
#define YTERR_PARAMETER					(-2)  
#define YTERR_OUT_OF_MEMORY				(-3)  
#define YTERR_TIMEOUT						(-4)  
#define YTERR_DISCONCET					(-5)  



/* msg id definition */
typedef enum
{
    BLE_MSG_RECV_TYPE = MPU_MID_BLE,
    BLE_MSG_SEND_TYPE, 
    BLE_MSG_CONNECT, 
    BLE_MSG_DISCONNECT, 
    BLE_MSG_ID_TIMER_HEARTER, 
    BLE_MSG_ID_TIMER_DEFAULT,
    BLE_MSG_ID_RING,
    BLE_MSG_ID_CHECK_TIMEOUT,
    BLE_MSG_RECV_TEST,
    BLE_MSG_RECV_TO_APP,
    BLE_MSG_CONTROL,
    BLE_MSG_TIMER,
} BLE_MSG_TYPE;

typedef enum 
{
    BLE_CONNECT = 0,
    BLE_DISCONNECT,
    BLE_SELF_DISCONNECT,
} BLE_STATE;


typedef enum 
{
	BLE_INIT_STATUS = 0,
    BLE_RECV_STATUS,
    BLE_RECV_FINISH_STATUS,
    BLE_SEND_STATUS,
} BLE_TRANS_STATE;
	
typedef struct {
	unsigned char					aucTxPack[MAX_LEN];
	unsigned char					aucRxPack[MAX_LEN];
	unsigned int 					ulTxLen;
	unsigned int 					ulRxLen;
}BT_DATA;


typedef struct 
{
    unsigned char		 			aucBleName[BLE_NAME_SIZE];
	unsigned char       			ucBleEn;   
	unsigned char       			ucBleWakeMcu;
	unsigned char       			ucSleepCloseBle;
} BLE_CONTR;

typedef struct ST_BT_API_
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
}ST_BT_API, *PST_BT_API;

typedef struct BLE_MEMBER
{
   
	timer_t             Retimer;  
	int        			ucConnStatus;
	unsigned char       ucTransStatus;
	unsigned char		ucFct;
} BLE_MEMBER;

extern ST_BT_API           stBtApi; 


int ble_init(INIT_PHASE phase);

int ble_run(void);
int BleSendMsg(unsigned short usMid, int iDate);
int BleSetName(unsigned char *pucName, unsigned char unLen);
void set_ble_stats(unsigned char ucData);
void ApiTraceBuf(unsigned char *buf, unsigned int len);
void test_bt_hz(void);
int BleGetMac(unsigned char *Mac);

#endif
