#include  <stdlib.h>
#include "timer.h"
#include "uds_request.h"
#include "uds_server.h"
#include "uds_define.h"
#include "key_access.h"

#define FIXED_SEED 0

#define ATT_CNT_LIMIT 3

#define SEED_SENT       1
#define SEED_NOT_SENT   0

const unsigned char RequestSeed_SubFunc[SecurityAccess_LEVEL1] =
{0x03};

static unsigned char get_request_security_level(unsigned char sub_function)
{
    unsigned char level = SecurityAccess_LEVEL0;

    if (sub_function == 0)
    {
        return SecurityAccess_LEVEL0;
    }

    if ((sub_function % 2) == 0)
    {
        sub_function = sub_function - 1;
    }

    for (level = 0; level < SecurityAccess_LEVEL1; level++)
    {
        if (sub_function == RequestSeed_SubFunc[level])
        {
            level = level + 1;
            return level;
        }
    }

    return SecurityAccess_LEVEL0;
}


static int uds_sa_cal_send_seed_msg(unsigned char *response, uint16_t seed, uint8_t sub_func)
{
    int len = 0;
    response[len++] =  0x27 + POS_RESPOND_SID_MASK ;
    response[len++] =  sub_func;
    response[len++] = (uint8_t)(seed >> 8);
    response[len++] = (uint8_t)seed;
    return len;
}



/*  Reference ISO 14229-1 Annex I,page 379, added by caoml */
void UDS_SRV_SecrityAcess(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    static uint8_t is_seed_sent = SEED_NOT_SENT;/*是否已发送种子*/
    static uint8_t Att_Cnt = 0;/*尝试次数*/
    static uint32_t time = 0;
    static uint8_t sub_function = 0;/*保存的子功能*/
    static uint8_t counter = 0;/* 生成随机种子次数 */
    static uint16_t seed = 0;
    static uint32_t times_satisfy_delay = 0;/*modify for HOZON*/
    
    uint16_t key1 = 0, key2 = 0;

    uint8_t  Ar_u8RePDU_DATA[10], res_len = 0;

    uint32_t  current_time = tm_get_time() / 1000;/* 当前时间 */


    /* 请求长度判断 */
    if (u16PDU_DLC < 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }


    uint8_t sub_func_tmp = p_u8PDU_Data[1] & suppressPosRspMsgIndicationBitMask;

    /* sub-functionNotSupported */
    if (get_request_security_level(sub_func_tmp) == 0)
    {
        /* Transmit negative response NRC 0x12 */
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
        return;
    }


    /*All security levels locked*/
    if (Get_SecurityAccess() == SecurityAccess_LEVEL0)
    {
        /* No active seed */
        if (is_seed_sent == SEED_NOT_SENT)
        {
            /*(A) All security levels locked. No active seed*/

            /* SecurityAccess requestSeed received */
            if (1 == (sub_func_tmp % 2))
            {
                /* Determine if the message length is OK */
                if (u16PDU_DLC != 2)/* Message Length NOK*/
                {
                    /* step 4.1 */

                    /* Keep the original state A */

                    /* Transmit negative response NRC 0x13 */
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                    return;
                }
                /* Message length OK */
                else
                {

                    /* Delay expired */
                    if ((current_time - time) > 10) // 10S
                    {
                        times_satisfy_delay++;
                        
                        if(times_satisfy_delay == 1)
                        {
                            Att_Cnt = 0;
                        }
                        /* Step 2 */

                        /* with Seed as active, Jump to B state */
                        is_seed_sent = SEED_SENT;

                        /* Static_Seed = False, then generate new Seed*/
                        srand(current_time + (counter++));
                        
                        #if (FIXED_SEED)
                        seed = 0x1234;
                        #else
                        seed = rand();
                        #endif

                        /* Save sub-function: xx = securityAccess Type */
                        sub_function = sub_func_tmp;

                        /*Transmit SecurityAccess positive response on
                        requestSeed request for the
                        requested securityAccessType*/
                        res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                        uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                        return;
                    }
                    /* Delay NOT expired */
                    else
                    {
                        /* step 4.4 */

                        /* Keep the original state A */

                        /* Transmit negative response NRC 0x37 */
                        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequiredTimeDelayNotExpired);
                        return;
                    }
                }

            }
            /* SecurityAccess sendKey received.*/
            else if (0 == (sub_func_tmp % 2))
            {
                /* step 4.2 */

                /* Keep the original state A*/

                /* Transmit negative response NRC 0x24 */
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                return;
            }
        }
        /* Seed sent */
        else if (is_seed_sent == SEED_SENT)
        {
            /*(B) All Security Levels Locked. Seed sent. Waiting for key.*/

            /* SecurityAccess sendKey received and sub-function: yy == xx+1c.*/
            if (0 == (sub_func_tmp % 2))
            {

                /* sub-function: yy == xx+1 */
                if (sub_func_tmp == (sub_function + 1))
                {
                    /* Determine if the message length is OK */
                    if (u16PDU_DLC != 4)/*Message Length NOK*/
                    {
                        /* step 9.4 */

                        /*change to A*/
                        is_seed_sent = SEED_NOT_SENT;

                        /* Att_Cnt++ for sub-function xx (if applicable). */
                        Att_Cnt++;

                        /* Transmit negative response NRC 0x13 */
                        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                        return;
                    }
                    /* Message length OK */
                    else
                    {
                        /* Determine if the key is OK */
                        key1 = (p_u8PDU_Data[2] << 8) + p_u8PDU_Data[3];
                        key2 = calcKey(seed);

                        if (key1 == key2)/*Key OK*/
                        {
                            /*step 3*/

                            /* Unlock security level for subfunction xx, Jump to C state, step 3*/
                            Set_SecurityAccess_LEVEL(get_request_security_level(sub_function));
                            is_seed_sent = SEED_NOT_SENT;

                            /* Att_Cnt = 0 for subfunction xx (if applicable). */
                            Att_Cnt = 0;

                            /*Transmit SecurityAccess positive response on sendKey request*/
                            Ar_u8RePDU_DATA[res_len++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                            Ar_u8RePDU_DATA[res_len++] =  p_u8PDU_Data[1];
                            uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                            return;
                        }
                        /* Key NOK */
                        else
                        {

                            if ((Att_Cnt + 1) < ATT_CNT_LIMIT) /*(Att_Cnt+1) < Att_Cnt_Limit (if applicable).*/
                            {
                                /* step 9.1 */

                                /*change to A*/
                                is_seed_sent = SEED_NOT_SENT;

                                /* Att_Cnt++ for sub-function xx (if applicable) */
                                Att_Cnt++;

                                /* Transmit negative response NRC 0x35. */
                                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_InvalidKey);
                                return;
                            }
                            else
                            {
                                /* step 9.2 */

                                /*change to A*/
                                is_seed_sent = SEED_NOT_SENT;

                                /*Att_Cnt++ for sub-function xx (if applicable).*/
                                Att_Cnt++;

                                /* Start Delay_Timer for sub-function xx (if applicable) */
                                time = tm_get_time() / 1000;
                                times_satisfy_delay = 0;

                                /* Transmit negative response NRC 0x36. */
                                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ExceedNumberOfAttempts);
                                return;
                            }
                        }
                    }
                }

                /*sub-function: yy <> xx+1.*/
                else
                {
                    /*step 9.3 */

                    /*change to A*/
                    is_seed_sent = SEED_NOT_SENT;

                    /* Att_Cnt++ for sub-function xx (if applicable) */
                    Att_Cnt++;

                    /*Transmit negative response NRC 0x24*/
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                    return;
                }
            }
            /* SecurityAccess requestSeed received */
            else if (1 == (sub_func_tmp % 2))
            {
                /*step 5.1*/

                /* Keep the original state B*/

                /* Save sub-function: xx = securityAccess Type */
                sub_function = sub_func_tmp;

                /* Static_Seed = False, then generate new Seed*/
                srand(current_time + (counter++));
                
                #if (FIXED_SEED)
                seed = 0x1234;
                #else
                seed = rand();
                #endif

                /*Transmit SecurityAccess positive response on
                requestSeed request for the
                requested securityAccessType*/
                res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                return;
            }
        }
        else
        {

        }
    }
    /*One security level unlocked.*/
    else
    {
        /* No active seed */
        if (is_seed_sent == SEED_NOT_SENT)
        {
            /*(C) One security level unlocked. No active seed.*/

            /* SecurityAccess sendKey received. */
            if (0 == (sub_func_tmp % 2))
            {
                /* step 7.1 */

                /*Transmit negative response NRC 0x24*/
                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                return;
            }
            /* SecurityAccess requestSeed received */
            else if (1 == (sub_func_tmp % 2))
            {
                /* Requested level is unlocked */
                if (Get_SecurityAccess() == get_request_security_level(sub_func_tmp))
                {
                    /*step 7.2 */

                    /* Transmit SecurityAccess positive response with zero seed */
                    seed = 0;
                    res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                    uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                    return;
                }
                /* Requested level is NOT unlocked. */
                else
                {
                    /* Message length NOK. */
                    if (u16PDU_DLC != 2)
                    {
                        Att_Cnt++;

                        /* Transmit negative response NRC 0x13 */
                        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                        return;
                    }
                    /* Message length OK. */
                    else
                    {
                        /* Delay expired */
                        if ((current_time - time) > 10) // 10S
                        {
                            times_satisfy_delay++;
                        
                            if(times_satisfy_delay == 1)
                            {
                                Att_Cnt = 0;
                            }
                            
                            /*step 8*/

                            /* with Seed as active */
                            is_seed_sent = SEED_SENT;

                            /*Generate and store Seed for the requested
                            securityAccessType (if not previously generated and
                            stored during the current ECU operating cycle)*/
                            srand(current_time + (counter++));
                            
                            #if (FIXED_SEED)
                            seed = 0x1234;
                            #else
                            seed = rand();
                            #endif

                            /* Save sub-function: xx = securityAccess Type */
                            sub_function = sub_func_tmp;

                            /*Transmit SecurityAccess positive response on
                            requestSeed request with Seed as active for the
                            requested securityAccessType.*/
                            res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                            uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                            return;
                        }
                        else
                        {
                            /* Transmit negative response NRC 0x37 */
                            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequiredTimeDelayNotExpired);
                            return;
                        }
                    }
                }
            }
        }
        /* Seed sent */
        else
        {
            /*(D) One security level unlocked. Seed sent. Waiting for key.*/

            /* SecurityAccess sendKey received. */
            if (0 == (sub_func_tmp % 2))
            {
                /* sub-function: yy <> xx+1. */
                if (sub_func_tmp != (sub_function + 1))
                {
                    /*step 10.5*/

                    /*change to C*/
                    is_seed_sent = SEED_NOT_SENT;

                    /* Att_Cnt++ for sub-function xx (if applicable) */
                    Att_Cnt++;

                    /* Transmit negative response NRC 0x24 */
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequstSequenceError);
                    return;
                }
                /* sub-function: yy == xx+1. */
                else
                {
                    /* Message length NOK. */
                    if (u16PDU_DLC != 4)
                    {
                        /*step 10.5*/

                        /*change to C*/
                        is_seed_sent = SEED_NOT_SENT;

                        /* Message length NOK. */
                        Att_Cnt++;

                        /* Transmit negative response NRC 0x13 */
                        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
                        return;
                    }
                    /* Message length OK */
                    else
                    {
                        key1 = (p_u8PDU_Data[2] << 8) + p_u8PDU_Data[3];
                        key2 = calcKey(seed);

                        /* Key OK */
                        if (key1 == key2)
                        {
                            /*step 10.2*/
                            is_seed_sent = SEED_NOT_SENT;

                            /* Att_Cnt = 0 for subfunction xx (if applicable). */
                            Att_Cnt = 0;


                            /* Unlock security level for subfunction xx, Jump to C state, step 3*/
                            Set_SecurityAccess_LEVEL(get_request_security_level(sub_function));

                            /*Transmit SecurityAccess positive response on sendKey request*/
                            Ar_u8RePDU_DATA[res_len++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
                            Ar_u8RePDU_DATA[res_len++] =  p_u8PDU_Data[1];
                            uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                            return;
                        }
                        /* Key NOK */
                        else
                        {
                            /* (Att_Cnt+1) >= Att_Cnt_Limit */
                            if ((Att_Cnt + 1) >= ATT_CNT_LIMIT)
                            {
                                /*step 10.4 */

                                /*change to 3*/
                                is_seed_sent = SEED_NOT_SENT;

                                /* Att_Cnt++ for sub-function xx (if applicable). */
                                Att_Cnt++;

                                /* Start Delay_Timer for sub-function xx (if applicable). */
                                time = tm_get_time() / 1000;
                                times_satisfy_delay = 0;

                                /* Transmit negative response NRC 0x36. */
                                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_ExceedNumberOfAttempts);
                                return;
                            }
                            /*(Att_Cnt+1) < Att_Cnt_Limit (if applicable).*/
                            else
                            {
                                /* step 10.3 */

                                /*change to C*/
                                is_seed_sent = SEED_NOT_SENT;

                                /* Att_Cnt++ for sub-function xx (if applicable) */
                                Att_Cnt++;

                                /* Transmit negative response NRC 0x35. */
                                uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_InvalidKey);
                                return;
                            }
                        }
                    }
                }


            }
            /* SecurityAccess requestSeed received. */
            else if (1 == (sub_func_tmp % 2))
            {
                /* Requested level is unlocked */
                if (Get_SecurityAccess() == SecurityAccess_LEVEL1)
                {
                    /*step 10.1*/

                    /*change to C*/
                    is_seed_sent = SEED_NOT_SENT;

                    /* Transmit SecurityAccess positive response with zero seed */
                    seed = 0;
                    res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                    uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                    return;
                }
                /* Requested level is locked */
                else
                {
                    /* step 5.1 */

                    /*Keep the original state D*/

                    /*Generate new seed and transmit SecurityAccess
                    positive response with the new seed for the requested
                    securityAccessType.*/
                    srand(current_time + (counter++));
                    
                    #if (FIXED_SEED)
                    seed = 0x1234;
                    #else
                    seed = rand();
                    #endif

                    /* Save sub-function: xx = securityAccess Type */
                    sub_function = sub_func_tmp;

                    /*Transmit SecurityAccess positive response on
                    requestSeed request with Seed as active for the
                    requested securityAccessType.*/
                    res_len = res_len + (uds_sa_cal_send_seed_msg(Ar_u8RePDU_DATA, seed, sub_func_tmp));

                    uds_positive_response(tUDS, tUDS->can_id_res, res_len, Ar_u8RePDU_DATA);
                    return;
                }
            }
        }
    }
}


