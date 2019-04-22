#include  "uds_file.h"
#include "uds_server.h"
#include "uds_request.h"

void UDS_SRV_RequestTransferExit(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[2];

    TransferDataConfig.blockSequenceCounter = 1;
    TransferDataConfig.TransferStatus = TransferEnd;

    if (u16PDU_DLC != 1)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
    }
    else
    {
        Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
        uds_positive_response(tUDS, tUDS->can_id_res, 1, Ar_u8RePDU_DATA);
    }
}

