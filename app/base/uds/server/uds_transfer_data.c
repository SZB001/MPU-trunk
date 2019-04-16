#include  "uds_define.h"
#include  "uds_file.h"
#include  "string.h"
#include "uds_request.h"
#include "uds_server.h"

void UDS_SRV_TransferData(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6];

    if (u16PDU_DLC > maxNumberOfBlockLength)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
    }
    else
    {
        if (TransferDataConfig.blockSequenceCounter != p_u8PDU_Data[1])
        {
            if (TransferDataConfig.blockSequenceCounter >= p_u8PDU_Data[1])
            {
                Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
                return;
            }
            else
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_wrongBlockSequenceCounter);
                return;
            }
        }

        TransferDataConfig.blockSequenceCounter++;
        memcpy(TransferDataConfig.data + TransferDataConfig.datasize, &p_u8PDU_Data[2], (u16PDU_DLC - 2));
        TransferDataConfig.datasize += u16PDU_DLC - 2;

        Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
        Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
        uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
    }
}
