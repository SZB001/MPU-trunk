/*****************************************************************************
*   Include Files
*****************************************************************************/
#include "uds_request.h"
#include "uds_server.h"

//ControlType
#define enableRxAndTx                       0x00
#define enableRxAnddisableTx                0x01
#define disableRxAndenableTx                0x02
#define disableRxAndTx                      0x03

//CommunicationType
#define normalCommuMesg                     0x01
#define netManaCommuMesg                    0x02 //don't support
#define normalCommuMesgAndnetManaCommuMesg  0x03 //don't support

static uint8_t g_TxFlag = 1, g_RxFlag = 1; // default enable

uint32_t UDS_GetCANTxStatus(void)
{
    return g_TxFlag;
}

void UDS_DisableCANTx(void)
{
    g_TxFlag = 0;
}

void UDS_EnableCANTx(void)
{
    g_TxFlag = 1;
}

void UDS_RecoverCANTxRxDefaultStatus(void)
{
    g_TxFlag = 1;
    g_RxFlag = 1;
}

void UDS_DisableCANTxRx(void)
{
    g_TxFlag = 0;
    g_RxFlag = 0;
}

uint32_t UDS_GetCANRxStatus(void)
{
    return g_RxFlag;
}

void UDS_SRV_CommunicationControl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[2];
    static unsigned char i = 0;

    if (u16PDU_DLC != 3)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
    {
        case enableRxAndTx:
            if (p_u8PDU_Data[2] == normalCommuMesg)
            {
                g_TxFlag = 1;
                g_RxFlag = 1;
            }

            break;

        case enableRxAnddisableTx:
            if (p_u8PDU_Data[2] == normalCommuMesg)
            {
                g_RxFlag = 1;
                g_TxFlag = 0;
            }

            break;

        case disableRxAndenableTx:
            if (p_u8PDU_Data[2] == normalCommuMesg)
            {
                g_RxFlag = 0;
                g_TxFlag = 1;
            }

            break;

        case disableRxAndTx:
            {
                if (p_u8PDU_Data[2] == normalCommuMesg)
                {
                    g_RxFlag = 0;
                    g_TxFlag = 0;
                }

                //if(1 == RTData.udsState255Flag)
                if (1)
                {
                    i = 0;
                }
                else if (i < 10)
                {
                    i++;
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);
                    return;
                }
                else//timeout
                {
                    i = 0;

                    if (p_u8PDU_Data[2] == normalCommuMesg)
                    {
                        g_RxFlag = 1;
                        g_TxFlag = 1;
                    }

                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ConditionsNotCorrect);

                    return;
                }
            }
            break;

        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            return;
    }

    if (p_u8PDU_Data[2] == normalCommuMesg)
    {
        Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
        Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;

        if (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBit)
        {
            g_u8suppressPosRspMsgIndicationFlag = 1;
        }

        uds_positive_response(tUDS, tUDS->can_id_res, 2, Ar_u8RePDU_DATA);
    }
    else
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
    }
}


