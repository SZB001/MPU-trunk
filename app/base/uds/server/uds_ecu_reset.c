#include "uds_request.h"
#include "uds_server.h"
#include "ql_powerdown.h"
#include "com_app_def.h"
#include "pm_api.h"

#define HARDRESET      0x01
#define SOFTWARERESET  0x03

void UDS_SRV_EcuReset(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6];

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
    }
    else
    {
        switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
        {
            case HARDRESET:
                Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;

                if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
                {
                    g_u8suppressPosRspMsgIndicationFlag = 1;
                }

                uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
                pm_send_evt(MPU_MID_UDS, PM_EVT_RESTART_4G_REQ);
                break;

            case SOFTWARERESET:
                Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;

                if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
                {
                    g_u8suppressPosRspMsgIndicationFlag = 1;
                }

                uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
                pm_send_evt(MPU_MID_UDS, PM_EVT_RESTART_APP_REQ);
                break;

            default:
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
                break;
        }
    }
}


