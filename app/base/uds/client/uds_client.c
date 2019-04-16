#include "uds_client.h"
#include "uds_proxy.h"
#include "log.h"

/**********************Macro Definition Declaration Start*******************************/
#define UDS_CLIENT_DEFAULT_CANPORT      (0)
#define UDS_CLIENT_DEFAULT_CANID        (0)
#define UDS_CLIENT_MAX_DATA_LEN         (512)
/**********************Macro Definition Declaration End********************************/

/**********************Data Type Declaration Start*******************************/
typedef enum
{
    UDS_CLIENT_RUNNING_ON  = 0,
    UDS_CLIENT_RUNNING_OFF = 1,
}UDS_CLIENT_RUNNING_STATUS_E;

typedef enum
{
    UDS_CLIENT_CMD_IDLE = 0,
    UDS_CLIENT_CMD_SEND,
    UDS_CLIENT_CMD_REQ,
}UDS_CLIENT_CMD_STATUS_E;


typedef struct
{
    uint32_t u32RequestID;
    uint8_t u8RequestSID;
    uint32_t u32RequestDLC;
    uint8_t au8RequestData[UDS_CLIENT_MAX_DATA_LEN + 10];

    //0:Response Info Invalid, 1:Response Info Valid
    uint8_t u8ResponseValid;

    uint32_t u32ResponseID;
    uint8_t u8ResponseSID;
    uint32_t u32ResponseDLC;
    uint8_t au8ResponseData[UDS_CLIENT_MAX_DATA_LEN + 10];
}UDS_CLIENT_DATA_LIST_T;
/**********************Data Type Declaration End********************************/

/**********************Parameter area Start*******************************/
static UDS_CLIENT_RUNNING_STATUS_E g_eUDSClientRunningStatus = UDS_CLIENT_RUNNING_OFF;
static UDS_CLIENT_CMD_STATUS_E g_eUDSClientCMDStatus = UDS_CLIENT_CMD_IDLE;
static UDS_CLIENT_DATA_LIST_T g_tUDSClientDataList;
static UDS_CLIENT_APPCALLBACKHANDLE g_phCallBackHandle;
/**********************Parameter area End********************************/

/****************************************************************
function:     UDS_Client_Open
description:  Open UDS Client
input:        uint8_t a_u8CanPort
              uint32_t a_u32CanIDPhy
              uint32_t a_u32CanIDFunc
              uint32_t a_u32CanIDRes
output:       none
return:       int
              0    :Command is Already Send To MCU
              other:Error
****************************************************************/
int UDS_Client_Open(uint8_t a_u8CanPort, 
                        uint32_t a_u32CanIDPhy, 
                        uint32_t a_u32CanIDFunc, 
                        uint32_t a_u32CanIDRes)
{
    g_eUDSClientCMDStatus = UDS_CLIENT_CMD_IDLE;
    g_eUDSClientRunningStatus = UDS_CLIENT_RUNNING_OFF;

    return uds_set_client_by_app(a_u8CanPort, a_u32CanIDPhy, a_u32CanIDFunc, a_u32CanIDRes);
}

/****************************************************************
function:     UDS_Client_GetOpenStatus
description:  Get UDS Client Open Status
input:        none
output:       none
return:       0:UDS Client Open Success;
              1:Fail
****************************************************************/
int UDS_Client_GetOpenStatus(void)
{
    return (int)g_eUDSClientRunningStatus;
}

/****************************************************************
function:     UDS_Client_SetOpenStatusOK
description:  MCU Return UDS Client Open OK
input:        none
output:       none
return:       none;
****************************************************************/
void UDS_Client_SetOpenStatusOK(void)
{
    g_eUDSClientRunningStatus = UDS_CLIENT_RUNNING_ON;
}

/****************************************************************
function:     UDS_Client_Close
description:  Close UDS Client
input:        none
output:       none
return:       none;
****************************************************************/
void UDS_Client_Close(void)
{
    g_eUDSClientCMDStatus = UDS_CLIENT_CMD_IDLE;
    g_eUDSClientRunningStatus = UDS_CLIENT_RUNNING_OFF;
}

/****************************************************************
function:     uds_get_client_info
description:  get uds client info struct
input:        uint32_t a_u32BroadcastFlag
              uint8_t a_u8Sid
              uint16_t a_u16Did
              uint8_t a_u32DLC
              uint8_t * a_pu8Data
              UDS_CLIENT_APPCALLBACKHANDLE a_phCallBackHandle
output:       none
return:       0:Request Is Send To MCU;
****************************************************************/
int UDS_Client_SendRequest(uint32_t a_u32BroadcastFlag, 
                                 uint8_t a_u8Sid, 
                                 uint16_t a_u16Did, 
                                 uint32_t a_u32DLC, 
                                 uint8_t * a_pu8Data, 
                                 UDS_CLIENT_APPCALLBACKHANDLE a_phCallBackHandle)
{
    UDS_T *  ptUDSClientInfo = NULL;
    uint32_t u32RequestID = 0;
    uint8_t  *data = g_tUDSClientDataList.au8RequestData;
    int      s32Ret = 1, len = 0;

    if((UDS_CLIENT_CMD_IDLE == g_eUDSClientCMDStatus) ||
       (UDS_CLIENT_CMD_REQ == g_eUDSClientCMDStatus))
    {
        ptUDSClientInfo = uds_get_client_info();
        //0 Means Do Not Need Broadcast, So Use Physics IDs
        if(0 == a_u32BroadcastFlag)
        {
            u32RequestID = ptUDSClientInfo->can_id_phy;
        }
        else
        {
            u32RequestID = ptUDSClientInfo->can_id_fun;
        }

        g_tUDSClientDataList.u32RequestID = u32RequestID;
        g_tUDSClientDataList.u8RequestSID = a_u8Sid;

        data[len++] = a_u8Sid;

        if (a_u16Did & 0xff00)
        {
            data[len++] = a_u16Did >> 8;
            data[len++] = a_u16Did;
        }
        else if (a_u16Did)
        {
            data[len++] = a_u16Did;
        }

        if (a_pu8Data && a_u32DLC > 0)
        {
            memcpy(data + len, a_pu8Data, a_u32DLC);
            len += a_u32DLC;
        }
        
        g_tUDSClientDataList.u32RequestDLC = len;

        if(UDS_CLIENT_MAX_DATA_LEN > g_tUDSClientDataList.u32RequestDLC)
        {
            g_phCallBackHandle = a_phCallBackHandle;

            //Call UDS request Function, Send Request
            s32Ret = (int)uds_data_request(ptUDSClientInfo, 
                                           MSG_ID_UDS_REQ, 
                                           u32RequestID, 
                                           g_tUDSClientDataList.au8RequestData, 
                                           (uint16_t)g_tUDSClientDataList.u32RequestDLC);
            if(0 == s32Ret)
            {
                g_eUDSClientCMDStatus = UDS_CLIENT_CMD_SEND;
                g_tUDSClientDataList.u8ResponseValid = 0;

                s32Ret = 0;
                log_i(LOG_UDS, "UDS Client Send Request.");
            }
            else
            {
                ;
            }
        }
        else
        {
            s32Ret = 1;
            log_e(LOG_UDS, "UDS Client Send Request Len Overflow!");
        }
    }
    else
    {
        log_w(LOG_UDS, "It Is Wait ECU Response, Can Not Send Request.");
    }

    return s32Ret;
}

/****************************************************************
function:     UDS_Client_GetResponse
description:  Get Response From MCU
input:        UDS_T * a_ptUDSClientInfo
              uint32_t a_u32ResponseID
              uint8_t * a_pu8Data
              uint32_t a_u32DLC
output:       none
return:       none;
****************************************************************/
void UDS_Client_GetResponse(UDS_T * a_ptUDSClientInfo, 
                                  uint32_t a_u32ResponseID, 
                                  uint8_t * a_pu8Data, 
                                  uint32_t a_u32DLC)
{
    uint8_t  u8ErrorCode = 0;
    uint16_t u16DataLen = 0;
    uint8_t *pu8Data = NULL;

    if(a_u32ResponseID == a_ptUDSClientInfo->can_id_res)
    {
        if((3 == a_u32DLC) &&
           (0x7F == a_pu8Data[0]) &&
           (g_tUDSClientDataList.u8RequestSID == a_pu8Data[1]))
        {
            log_w(LOG_UDS, "UDS Client Get Negative Response Codes, 0x%x!", a_pu8Data[2]);
            u8ErrorCode = a_pu8Data[2];
        }
        else if((g_tUDSClientDataList.u8RequestSID + POS_RESPOND_SID_MASK) ==  a_pu8Data[0])
        {
            g_tUDSClientDataList.u32ResponseID = a_u32ResponseID;
            g_tUDSClientDataList.u8ResponseSID = a_pu8Data[0];
            g_tUDSClientDataList.u32ResponseDLC = a_u32DLC;
            memset(g_tUDSClientDataList.au8ResponseData, 0, sizeof(g_tUDSClientDataList.au8ResponseData));
            memcpy(g_tUDSClientDataList.au8ResponseData, a_pu8Data, a_u32DLC);
            g_tUDSClientDataList.u8ResponseValid = 1;

            //if Response Data Len Bigger Than Request Data Len, We Send Some Data To App
            //Example:
            //Request Data : 10 03
            //Response Data: 50 03 00 32 00 C8
            //So We Send "00 32 00 C8" To App
            if((0x34 + POS_RESPOND_SID_MASK) == a_pu8Data[0])
            {
                //临时修改方案，后续需要完善
                if(g_tUDSClientDataList.au8ResponseData[2] > 0x02)
                {
                    g_tUDSClientDataList.au8ResponseData[1] = 0x20;
                    g_tUDSClientDataList.au8ResponseData[2] = 0x02;
                    g_tUDSClientDataList.au8ResponseData[3] = 0x00;
                }
                u16DataLen = g_tUDSClientDataList.u32ResponseDLC - 1;
                pu8Data = &g_tUDSClientDataList.au8ResponseData[1];
            }
            else if(g_tUDSClientDataList.u32ResponseDLC > g_tUDSClientDataList.u32RequestDLC)
            {
                //Get Response Data
                u16DataLen = g_tUDSClientDataList.u32ResponseDLC - g_tUDSClientDataList.u32RequestDLC;
                pu8Data = &g_tUDSClientDataList.au8ResponseData[g_tUDSClientDataList.u32RequestDLC];
            }
        }
        else
        {
            log_w(LOG_UDS, "UDS Client Response SID() Is Not Match Request SID() !", a_pu8Data[0], g_tUDSClientDataList.u8RequestSID);
            u8ErrorCode = 1;
        }
    }
    else
    {
        log_e(LOG_UDS, "UDS Client Get Response Fail, ID Is Not Match!");
        u8ErrorCode = 2;
    }
    
    g_eUDSClientCMDStatus = UDS_CLIENT_CMD_REQ;
    if (g_phCallBackHandle)
    {
        g_phCallBackHandle(u8ErrorCode, u16DataLen, pu8Data);
    }
}

void UDS_Client_Timeout(void)
{
    g_eUDSClientCMDStatus = UDS_CLIENT_CMD_REQ;
    if (g_phCallBackHandle)
    {
        g_phCallBackHandle(-1, 0, NULL);
    }
}

/****************************************************************
function:     UDS_Client_SendTesterPresent
description:  Use To Send Tester Present
input:        none
output:       none
return:       none;
****************************************************************/
void UDS_Client_SendTesterPresent(void)
{
    UDS_T * ptUDSClientInfo = NULL;
    uint8_t u8RequestData[3] = {0x02, SID_TestPresent, suppressPosRspMsgIndicationBit};

    ptUDSClientInfo = uds_get_client_info();

    log_i(LOG_UDS, "UDS Client Send Tester Present");
    uds_data_request(ptUDSClientInfo, MSG_ID_UDS_REQ, ptUDSClientInfo->can_id_fun, u8RequestData, sizeof(u8RequestData));
}

