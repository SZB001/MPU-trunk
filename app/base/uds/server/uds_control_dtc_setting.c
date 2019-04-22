#include "uds_request.h"
#include "uds_server.h"

/***DTCSettingType***/
#define ON               0x01
#define OFF             0x02


static uint8_t DTCSwitch = 1; //default on


void UDS_SRV_ControlDTCSetting(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6];

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
    {
        case ON:
            DTCSwitch = 1;
            break;

        case OFF:
            DTCSwitch = 0;
            break;

        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            return;
    }

    Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;

    if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
    {
        g_u8suppressPosRspMsgIndicationFlag = 1;
    }

    uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
}

unsigned int UDS_GetDTCSetting(void)
{
    return DTCSwitch;
}

void UDS_SetDTCOff(void)
{
    DTCSwitch = 0;
}

void UDS_SetDTCOn(void)
{
    DTCSwitch = 1;
}


