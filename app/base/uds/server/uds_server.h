#ifndef __UDS_SERVER_H__
#define __UDS_SERVER_H__
#include "uds_define.h"

extern uint8_t   g_u8CurrentSessionType  ;
extern uint8_t   g_u8SecurityAccess  ;
extern uint8_t     g_u8suppressPosRspMsgIndicationFlag ;
extern uint32_t   g_u32DiagID;

typedef void (*uds_server_func)(UDS_T *, uint8_t *, uint16_t);

void UDS_SRV_ClearDTC(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_ControlDTCSetting(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_DiagSessionCtrl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_RequestDownload(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_RoutineControl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_SecrityAcess(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_TesterPresent(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_ReadDTC(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_TransferData(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_EcuReset(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_RequestTransferExit(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data);
void UDS_SRV_ReadDataByID(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void UDS_SRV_CommunicationControl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);


void uds_server_proc(UDS_T *uds, unsigned int can_id, unsigned char *pdu_data,
                     unsigned short lenth);

#endif
