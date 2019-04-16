#include "uds_server.h"
#include "uds_request.h"
#include  "uds_file.h"

void UDS_SRV_RequestDownload(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6], i;
    uint8_t memoryAddressLength, memorySizeLength;

    memoryAddressLength = p_u8PDU_Data[2] & 0x0f;
    memorySizeLength = (p_u8PDU_Data[2] >> 4) & 0x0f;

    if (u16PDU_DLC != (3 + memoryAddressLength + memorySizeLength))
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
    }
    else
    {
        if (p_u8PDU_Data[1] != 0x00)
        {
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
        }
        else
        {
            for (i = 0; i < memoryAddressLength; i++)
            {
                TransferDataConfig.ProgrammemoryAddress = (TransferDataConfig.ProgrammemoryAddress << 8);
                TransferDataConfig.ProgrammemoryAddress |= p_u8PDU_Data[3 + i];
            }

            for (i = 0; i < memorySizeLength; i++)
            {
                TransferDataConfig.ProgrammemorySize = (TransferDataConfig.ProgrammemorySize << 8);
                TransferDataConfig.ProgrammemorySize |= p_u8PDU_Data[3 + memoryAddressLength + i];
            }

            #if 0

            if (TransferDataConfig.ProgrammemoryAddress > 0x80000)
            {
                UDS_NegativeResponse(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
                return;
            }

            #endif
            TransferDataConfig.blockSequenceCounter = 1;
            TransferDataConfig.TransferStatus = TransferStart;

            Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
            Ar_u8RePDU_DATA[1] =  0x20;//length (number of bytes) of the maxNumberOfBlockLength parameter
            Ar_u8RePDU_DATA[2] = (maxNumberOfBlockLength >> 8) & 0xFF;
            Ar_u8RePDU_DATA[3] =  maxNumberOfBlockLength & 0xFF;

            uds_positive_response(tUDS, tUDS->can_id_res, 4, Ar_u8RePDU_DATA);

        }
    }
}

