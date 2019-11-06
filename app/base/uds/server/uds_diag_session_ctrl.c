/*****************************************************************************
*   Include Files
*****************************************************************************/
#include "uds_diag.h"
#include "uds_request.h"
#include "uds_server.h"
#include "uds_proxy.h"

void UDS_SetDTCOn(void);

/*****************************************************************************
*   Function   :    UDS_SRV_DiagSessionCtrl
*   Description:
*   Inputs     :    None
*   Outputs    :    NULL
*   Notes      :
*****************************************************************************/
void UDS_SRV_DiagSessionCtrl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6];

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
    {
        case SESSION_TYPE_DEFAULT :
            Set_Seesion_Default();
            Clear_SecurityAccess();
            UDS_SetDTCOn();
            uds_send_can_CommunicationControl_to_mcu(2, 0);/*所有报文，允许收发*/
            //UDS_RecoverCANTxRxDefaultStatus();
            break;

        case SESSION_TYPE_EXTENDED :
            if (Get_Session_Current() != SESSION_TYPE_PROGRAM)
            {
                Set_Seesion_Extend();
                Clear_SecurityAccess();
                //UDS_SetDTCOn();
            }
            else
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ConditionsNotCorrect);
                return;
            }

            break;

        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            return ;
    }

    if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
    {
        g_u8suppressPosRspMsgIndicationFlag = 1;
    }

    Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[2] = (uint8_t)(tUDS->timer_t[P2SERVER].timer_value >> 8);
    Ar_u8RePDU_DATA[3] = (uint8_t)tUDS->timer_t[P2SERVER].timer_value;

    /*P2E 精度为10ms*/
    Ar_u8RePDU_DATA[4] = (uint8_t)(((tUDS->timer_t[P2EXT_SERVER].timer_value)/10) >> 8);
    Ar_u8RePDU_DATA[5] = (uint8_t)((tUDS->timer_t[P2EXT_SERVER].timer_value)/10);

    uds_positive_response(tUDS, tUDS->can_id_res, 6, Ar_u8RePDU_DATA);
}

