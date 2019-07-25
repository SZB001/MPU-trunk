/**
 *@file     ql_sms.h
 *@date     2017-05-02
 *@author   
 *@brief    
 */
 
#ifndef __QL_SMS_H__
#define __QL_SMS_H__

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef uint32 sms_client_handle_type;

/**  Maximum length of an SMS. */
#define QL_SMS_MAX_MT_MSG_LENGTH    1440

/**  Maximum string length. */
#define QL_SMS_MAX_ADDR_LENGTH      252



typedef enum   
{    
    E_QL_SMS_FORMAT_GSM_7BIT        = 0,
    E_QL_SMS_FORMAT_BINARY_DATA     = 1,
    E_QL_SMS_FORMAT_UCS2            = 2,
    //<2017/12/28-QCM9XOL00004C001-P01-Vicent.Gao, <[SMS] Segment 1==> CharSet to Alpha implementation.>
    E_QL_SMS_FORMAT_IRA             = 3,
    //>2017/12/28-QCM9XOL00004C001-P01-Vicent.Gao

}E_QL_SMS_FORMAT_T;   

typedef enum   
{    
    E_QL_SMS_TYPE_RX                = 0,    ///< SMS mobile terminated message.  
    E_QL_SMS_TYPE_TX                = 1,    ///< SMS mobile originated message. 
    E_QL_SMS_TYPE_BROADCAST_RX      = 2     ///< SMS Cell Broadcast message.   
}E_QL_SMS_TYPE_T; 

typedef enum 
{
    E_QL_SMS_STORAGE_TYPE_NONE      = -1,   /**<  Message no need to store. */
    E_QL_SMS_STORAGE_TYPE_UIM       = 0,    /**<  Message store to UIM. */
    E_QL_SMS_STORAGE_TYPE_NV        = 1,    /**<  Message store to NV. */
}E_QL_SMS_STORAGE_TYPE_T;

typedef enum 
{
    E_QL_SMS_MESSAGE_MODE_UNKNOWN   = -1,   /**<  Message type CDMA */
    E_QL_SMS_MESSAGE_MODE_CDMA      = 0,    /**<  Message type CDMA */
    E_QL_SMS_MESSAGE_MODE_GW        = 1,    /**<  Message type GW. */
}E_QL_SMS_MODE_TYPE_T;

typedef struct
{
    /* If sms is stored, it won't parse, you need read it by yourself */
    E_QL_SMS_STORAGE_TYPE_T e_storage;                          ///specify where stored this msg
    
    E_QL_SMS_FORMAT_T       format;
    E_QL_SMS_TYPE_T         type;
    char                    src_addr[QL_SMS_MAX_ADDR_LENGTH]; ///Telephone number string.  
    int                     sms_data_len;
    char                    sms_data[QL_SMS_MAX_MT_MSG_LENGTH];   ///SMS content, data format depends on format
    char                    timestamp[21];                      ///Message time stamp (in text mode). string format: "yy/MM/dd,hh:mm:ss+/-TimeZone" 

    E_QL_SMS_MODE_TYPE_T    e_mode;                             ///specify where stored this msg cdma or gw area
    uint32_t                storage_index;                      ///storage index, -1 means not store
} ql_sms_info_t;
     

typedef struct
{
    E_QL_SMS_STORAGE_TYPE_T e_storage;
    E_QL_SMS_MODE_TYPE_T    e_mode;
    uint32_t                storage_idx;
} ql_sms_storage_info_t;


typedef enum 
{
    E_QL_SMS_MESSAGE_CLASS_0    = 0x00, //display only
    E_QL_SMS_MESSAGE_CLASS_1    = 0x01, //store in ME
    E_QL_SMS_MESSAGE_CLASS_2    = 0x02, //store in SIM
    E_QL_SMS_MESSAGE_CLASS_3    = 0x03, //
    E_QL_SMS_MESSAGE_CLASS_NONE = 0x04, 
    E_QL_SMS_MESSAGE_CLASS_CDMA = 0x05, 
    E_QL_SMS_MESSAGE_CLASS_MAX  = 0x06, 
}E_QL_SMS_MESSAGE_CLASS_TYPE_T;

typedef enum 
{
    E_QL_SMS_UNKNOWN            = -1, 
    E_QL_SMS_DISCARD            = 0x00, /*  Incoming messages for this route are discarded by the WMS service without 
                                            notifying QMI_WMS clients */
    E_QL_SMS_STORE_AND_NOTIFY   = 0x01, /*  Incoming messages for this route are stored to the specified device 
                                            memory, and new message notifications */
    E_QL_SMS_TRANSFER_ONLY      = 0x02, /*  Incoming messages for this route are transferred to the client, and the
                                            client is expected to send ACK to the network */
    E_QL_SMS_TRANSFER_AND_ACK   = 0x03, /*  Incoming messages for this route are transferred to the client, and ACK is
                                            sent to the network */
}E_QL_SMS_RECEPTION_ACTION_TYPE_T;

typedef struct 
{
  E_QL_SMS_MESSAGE_CLASS_TYPE_T     message_class;
  E_QL_SMS_STORAGE_TYPE_T           route_storage;
  E_QL_SMS_RECEPTION_ACTION_TYPE_T  receipt_action;
}ql_sms_route_info_t;  /* Type */

typedef struct 
{
    ql_sms_route_info_t     route_list[E_QL_SMS_MESSAGE_CLASS_MAX];
}ql_sms_route_info_list_t;  /* Message */



typedef ql_sms_info_t       QL_SMS_Msg_t; 
typedef ql_sms_info_t*      QL_SMS_MsgRef; 

/* Callback function registered to QL_SMS_AddRxMsgHandler, msgRef contains the detail msg infor */
typedef void (*QL_SMS_RxMsgHandlerFunc_t)   
(    
    QL_SMS_MsgRef       msgRef,  
    void*               contextPtr    
);

/* Init SMS module and return h_sms, this should be called before any othe ones */
int QL_SMS_Client_Init(sms_client_handle_type  *ph_sms);

/* Add callback function, if any new msg arrived, it will notify app */
int QL_SMS_AddRxMsgHandler(QL_SMS_RxMsgHandlerFunc_t handlerPtr, void* contextPtr);

/* Send sms, you just need to fill format/src_addr/sms_data_len/sms_data,
   if format is UCS2, the data should be Unicode-BE format.
*/
int QL_SMS_Send_Sms(sms_client_handle_type h_sms, ql_sms_info_t *pt_sms_info);

/* DeInit SMS module and release resource, this should be called in the last one. */
int QL_SMS_Client_Deinit(sms_client_handle_type h_sms);
     
/* Delete the SMS specified in the  pt_sms_storage */
int QL_SMS_DeleteFromStorage(sms_client_handle_type  h_sms, ql_sms_storage_info_t  *pt_sms_storage);

/* Set route list, normally, sms won't be stored, the route set to transfer_and_ack by default.
   If you want to save to storage, you can specify it with this api.
*/
int QL_SMS_SetRouteList(sms_client_handle_type  h_sms,  ql_sms_route_info_list_t   *pt_route_list);


/*
Usage 1 (register callback and wait for new sms arrive): 
1, QL_SMS_Client_Init
2, QL_SMS_AddRxMsgHandler(pf_cb)
3, wait for sms arrive, pf_cb will pass the detail sms info to app.
4, QL_SMS_Client_Deinit


Usage 2 (Send sms): 
1, QL_SMS_Client_Init
2, QL_SMS_AddRxMsgHandler(pf_cb)
3, QL_SMS_Send_Sms
4, QL_SMS_Client_Deinit


Usage 3 (store sms): 
1, QL_SMS_Client_Init
2, QL_SMS_AddRxMsgHandler(pf_cb)
3, QL_SMS_SetRouteList (Notice: class-0 not allowed to store, class-2 must be stored to UIM)
4, pf_cb will be called when new sms arrived, and tell you where the sms stored
5, you can call QL_SMS_DeleteFromStorage to delete specified sms, the location can be got from pf_cb.
6, QL_SMS_Client_Deinit
*/

#endif // __QL_SMS_H__      
     
