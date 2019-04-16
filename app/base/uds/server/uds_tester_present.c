/*****************************************************************************
*   Include Files
*****************************************************************************/
#include "uds_request.h"
#include "uds_server.h"

#define ZeroSubFunction             0x00


void UDS_SRV_TesterPresent(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[2];

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
    {
        case ZeroSubFunction:

            Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
            Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;

            if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
            {
                g_u8suppressPosRspMsgIndicationFlag = 1;
            }

            uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
            break;

        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            break;
    }
}

