#ifndef __UDS_CLIENT_H__
#define __UDS_CLIENT_H__

#include "uds_define.h"

/**********************Data Type Declaration Start*******************************/
typedef void (*UDS_CLIENT_APPCALLBACKHANDLE)(uint8_t a_u8ErrorCode, uint16_t a_u16DataLen, uint8_t * a_pu8Data);
/**********************Data Type Declaration End********************************/

int  UDS_Client_Open(uint8_t a_u8CanPort, 
                        uint32_t a_u32CanIDPhy, 
                        uint32_t a_u32CanIDFunc, 
                        uint32_t a_u32CanIDRes);
int  UDS_Client_GetOpenStatus(void);
void UDS_Client_SetOpenStatusOK(void);
void UDS_Client_Close(void);


int  UDS_Client_SendRequest(uint32_t a_u32BroadcastFlag, 
                                  uint8_t a_u8Sid, 
                                  uint16_t a_u16Did,
                                  uint32_t a_u32DLC, 
                                  uint8_t * a_pu8Data, 
                                  UDS_CLIENT_APPCALLBACKHANDLE a_phCallBackHandle);
void UDS_Client_GetResponse(UDS_T * a_ptUDSClientInfo, 
                                  uint32_t a_u32ResponseID, 
                                  uint8_t * a_pu8Data, 
                                     uint32_t a_u32DLC);
void UDS_Client_SendTesterPresent(void);
uint32_t uds_data_request(UDS_T *uds, UDS_TL_ID msg_id, uint32_t can_id, uint8_t *pdu_data,
                          uint16_t pdu_dlc);
#endif

