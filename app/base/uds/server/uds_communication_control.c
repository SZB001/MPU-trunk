/*****************************************************************************
*   Include Files
*****************************************************************************/
#include "uds_request.h"
#include "uds_server.h"
#include "scom_msg_def.h"
#include "scom_api.h"

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
/**
 * 发送要往CAN总线上发送的TBOX状态信息给MCU
 * @param[in]    state 要发送的状态信息
 * @return       返回说明：0：发送成功             -1：参数错误 -2:MPU和MCU通信错误
 * @ref          scom_msg_def.h
 * @see
 * @note
*/
int uds_send_can_CommunicationControl_to_mcu(unsigned char mpu2mcu_msg_type,
        unsigned char mpu2mcu_ctrl_value)
{
    int len = 0;
    unsigned char buf[2];


    buf[len++] = mpu2mcu_msg_type;
    buf[len++] = mpu2mcu_ctrl_value;

    if (scom_tl_send_frame(SCOM_MPU_MCU_CAN_CTRL, SCOM_TL_SINGLE_FRAME, 0, buf, len))
    {
        log_e(LOG_UDS, "Fail to send comm ctrl msg to MCU");
        return -2;
    }

    return 0;
}

void UDS_SRV_CommunicationControl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[2];
    unsigned char mpu2mcu_msg_type = 0;/* 消息类型 */
    unsigned char mpu2mcu_ctrl_value = 0;/* 0 使能收发 3 禁止收发*/

    if (u16PDU_DLC != 3)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask)
    {
        case enableRxAndTx:
            {
                if (p_u8PDU_Data[2] == normalCommuMesg)
                {
                    g_TxFlag = 1;
                    g_RxFlag = 1;
                }

                mpu2mcu_ctrl_value = 0x00;
            }
            break;

        case disableRxAndTx:
            {
                if (p_u8PDU_Data[2] == normalCommuMesg)
                {
                    g_RxFlag = 0;
                    g_TxFlag = 0;
                }

                mpu2mcu_ctrl_value = 0x01;
            }
            break;

        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            return;
    }

    if ((p_u8PDU_Data[2] == normalCommuMesg) || (p_u8PDU_Data[2] == netManaCommuMesg)
        || (p_u8PDU_Data[2] == normalCommuMesgAndnetManaCommuMesg))
    {
        Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
        Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;
        mpu2mcu_msg_type = p_u8PDU_Data[2] - 1;/* 根据MCU定义赋值 */

        uds_send_can_CommunicationControl_to_mcu(mpu2mcu_msg_type, mpu2mcu_ctrl_value);

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


