#include  <stdlib.h>
#include "timer.h"
#include "uds_request.h"
#include "uds_server.h"
#include "key_access.h"

#define RequestSeed_Secrity_Level1          0x03
#define SeedKey_Secrity_Level1              0x04

#if 0
#define RequestSeed_Secrity_Level2          0x03
#define RequestSeed_Secrity_Level3          0x11

#define SeedKey_Secrity_Level2              0x04
#define SeedKey_Secrity_Level3              0x12
#endif
void UDS_SRV_SecrityAcess(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[10], i = 0;
    static uint8_t flag, counter = 0;
    static uint32_t SecrityAccessFailCounter = 0;
    static uint32_t time = 0;
    static uint16_t seed, key1, key2;
    static uint32_t  current_time = 0;

    if (u16PDU_DLC < 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    if (tm_get_time() < (900))  /*refer to wanshuai*/
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequiredTimeDelayNotExpired);
        return;
    }

    current_time = tm_get_time() / 1000;

    if (SecrityAccessFailCounter >= 3)
    {
        if ((current_time - time) < 10) // 10S
        {
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequiredTimeDelayNotExpired);
            return;
        }
        else
        {
            SecrityAccessFailCounter--;
        }
    }

    switch (p_u8PDU_Data[1])
    {
        case RequestSeed_Secrity_Level1:
        #if 0
        case RequestSeed_Secrity_Level2:
        #endif
            if (u16PDU_DLC != 2)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                return;
            }
            /*合众项目要求2703不支持编程会话*/
            if (Get_Session_Current() == SESSION_TYPE_PROGRAM)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFunctionNotSupportedInActiveSession);
                return;
            }
            if (flag == 1)
            {
                SecrityAccessFailCounter++;

                if (SecrityAccessFailCounter == 3)
                {
                    time = current_time;
                }
            }
            else
            {
                srand(current_time + (counter++));

                if (Get_SecurityAccess() == SecurityAccess_LEVEL0)
                {
                    seed = rand();//0x1234;
                    flag = 1;/*已发送种子标志*/
                }
                else
                {
                    seed = 0;
                }
            }

            Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
            Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1];
            
            /* Modified by caoml for HOZON */
            #if 0
            Ar_u8RePDU_DATA[i++] = (uint8_t)(seed >> 24);
            Ar_u8RePDU_DATA[i++] = (uint8_t)(seed >> 16);
            #endif

            Ar_u8RePDU_DATA[i++] = (uint8_t)(seed >> 8);
            Ar_u8RePDU_DATA[i++] = (uint8_t)seed;

            #if 0
            log_o(LOG_UDS, "seed = 0X%X, key1 = 0X%X", seed, saGetKey(seed, 2));
            log_o(LOG_UDS, "seed = 0X%X, key2 = 0X%X", seed, saGetKey(seed, 4));
            #endif
            
            break;

        case SeedKey_Secrity_Level1:
            if (u16PDU_DLC != 4)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                return;
            }

            if (flag == 0)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                return;
            }


            key1 = (p_u8PDU_Data[2] << 8) + p_u8PDU_Data[3];
            key2 = calcKey(seed);

            log_o(LOG_UDS, "SeedKey_Secrity_Level1 key1 = %X, key2 = %X\r\n", key1, key2);

            if (key1 == key2)
            {
                Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1];
                flag = 0;

                if (p_u8PDU_Data[1] == SeedKey_Secrity_Level1)
                {
                    Set_SecurityAccess_LEVEL1();
                    SecrityAccessFailCounter = 0;
                }
            }
            else
            {
                SecrityAccessFailCounter++;

                if (SecrityAccessFailCounter == 3)
                {
                    time = current_time;
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ExceedNumberOfAttempts);
                }
                else
                {
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_InvalidKey);
                }

                flag = 0;/*延时以满足，清除已发送种子标志*/
                return;
            }

            break;
        #if 0
        case SeedKey_Secrity_Level2:
            if (u16PDU_DLC != 6)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                return;
            }

            if (flag == 0)
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                return;
            }

            if (attempt >= 3)
            {
                flag = 0;
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ExceedNumberOfAttempts);
                return;
            }

            key1 = (p_u8PDU_Data[2] << 24) + (p_u8PDU_Data[3] << 16) + (p_u8PDU_Data[4] << 8) + p_u8PDU_Data[5];
            key2 = saGetKey(seed, 4);

            log_o(LOG_UDS, "SeedKey_Secrity_Level2 key1 = %X, key2 = %X\r\n", key1, key2);

            if (key1 == key2)
            {
                Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1];
                flag = 0;

                if (p_u8PDU_Data[1] == SeedKey_Secrity_Level3)
                {
                    Set_SecurityAccess_LEVEL2();
                    SecrityAccessFailCounter = 0;
                }
            }
            else
            {
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_InvalidKey);
                SecrityAccessFailCounter++;

                if (SecrityAccessFailCounter == 3)
                {
                    time = current_time;
                }

                attempt++;
                return;
            }

            break;
        #endif
        
        default:
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            return;
    }


    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);

}

