/****************************************************************
 file:          cm256_if.c
 description:   
 date:          2018/5/15
 author:        liuquanfu
 ****************************************************************/

#ifndef __CM256_IF_H__
#define __CM256_IF_H__
#include "ble.h"

//typedef int (*nm_status_changed)(NM_STATE_MSG status);
typedef int (*ble_status_changed)(void);


#define BLE_MAX_TBL						6
#define RX_BUFFER_SIZE            		4*1024
#define TX_BUFFER_SIZE					4*1024
#define UUID_SIZE						240
#define BLE_NAME_SIZE					250
#define BLE_UUID_CNT					2

void ApiBLETraceBuf(unsigned char *buf, unsigned int len);



typedef struct BLE_TBL
{
    unsigned char 		used_num;
    ble_status_changed 	BleMsgCallback[BLE_MAX_TBL];
} BLE_TBL;


typedef struct NF_BLE_MSG
{
	unsigned char    		aucRemoteAddress[6];
	unsigned char    		aucLocalAddress[6];
	int                     ulBleStatus;
	int                     ulHandle;
	int                     ulNotifyHandle;
} NF_BLE_MSG;

typedef struct BLE_CTL
{
    unsigned int  ulRecvTotalLen;
	unsigned int  ulRecvOffsetLen;
	unsigned int  ulSendTotalLen;
	unsigned int  ulSendOffsetLen;
} BLE_CTL;

typedef int (*SleepMsg)(void);

extern volatile NF_BLE_MSG				g_NfBleMsg;
#define IS_TIME_OUT(start, delta)        				((unsigned int)(tm_get_time() - (unsigned int)(start)) >= (unsigned int)(delta))
int cm256_init(void);
/******************************************************************************
* Function Name  : Nf3303Open
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_open(void);
/******************************************************************************
* Function Name  : Nf3303Close
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_close(void);
/******************************************************************************
* Function Name  : Cm256_Process
* Description    :  
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_process(unsigned char ucCycle);
/******************************************************************************
* Function Name  : Nf3303Disconnect
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_disconnect(void);
/******************************************************************************
* Function Name  : Nf3303GetState
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_get_state(void);
/******************************************************************************
* Function Name  : Nf3303GetName
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_get_name(unsigned char *paucOutBtName, unsigned char *pucOutLen);
/******************************************************************************
* Function Name  : Nf3303GetMac
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_get_mac(unsigned char *paucBtMac,unsigned char *pucLen);
/******************************************************************************
* Function Name  : Nf3303SetName
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_set_name(unsigned char *paucInBtName, unsigned char *pucInLen);
/******************************************************************************
* Function Name  : Nf3303_GetPkgSize
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
unsigned char cm256_get_pkgsize(unsigned short *pusLen);
/******************************************************************************
* Function Name  : Nf3303Send
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_send(unsigned char *pucBuf, unsigned int *pulLen);
/******************************************************************************
* Function Name  : Nf3303Recv
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_recv(unsigned char *pucBuf, unsigned int *pulLen);
/******************************************************************************
* Function Name  : Nf3303_test
* Description    :  open  
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_test(unsigned char *pucBuf, unsigned short usLen);
/******************************************************************************
* Function Name  : Nf3303EnPower
* Description    :   
* Input          :   
* Return         : NONE
******************************************************************************/
int cm256_enpower(unsigned char ucEn);
/******************************************************************************
* Function Name  : Nf3303GetUuid
* Description    :   
* Input          :   
* Return         : NONE
******************************************************************************/
int cm256_get_uuid(unsigned     short  *usUuid, unsigned char ucUuidCnt);
/******************************************************************************
* Function Name  : Nf3303GetRssi
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
int cm256_get_rssi(unsigned short *usRssi, unsigned char ucLen);

int cm256_apiget_recv(unsigned char *pucInBuf, unsigned int *ulInLen);

int cm256_apiset_send(unsigned char *pucOutBuf, unsigned int *ulOutLen);

int cm256_apiget_state(unsigned char *pucInRemot, int ulInState);

int cm256_check_len(void);


int cm256_apiset_notifyhandle(int *piHandle);

int PmCallbackhandler(SleepMsg handler);
void tm_ble_timeout(void);


#endif

