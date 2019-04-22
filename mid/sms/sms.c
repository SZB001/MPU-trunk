/****************************************************************
file:         sms.c
description:  the source file of sms operation
date:         2017/10/10
author        wangqinglong
****************************************************************/
#include "ql_sms_parser.h"
#include "ql_sms.h"
#include "sms.h"
#include "log.h"

ST_SMS_MsgRef h_msg;
static pthread_mutex_t sms_mutex;

/*****************************************************************************
function:     sms_init
description:  apply a session, init only one times by pthread !!!!!!!!!!!!!
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int sms_init(void)
{
    int ret;

    pthread_mutex_init(&sms_mutex, NULL);

    h_msg = QL_SMS_Create();

    if (NULL == h_msg)
    {
        log_e(LOG_MID, "create sms failed");
        return -1;
    }

    ret = QL_SMS_SetTimeout(h_msg, 3600);

    if (0 != ret)
    {
        log_e(LOG_MID, "set timeout failed, ret:%d", ret);
        return ret;
    }

    return 0;
}

/*****************************************************************************
function:     sms_send
description:  check whether the path refers to a file
input:        SMS_CONTENT_MODE mode, the mode of content
              char *phone_num , the phone number of destination
              char *msg , the message will be send
              int len , the length of message
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int sms_send(SMS_CONTENT_MODE mode, char *phone_num, char *msg, int len)
{
    int ret;

    pthread_mutex_lock(&sms_mutex);
    ret = QL_SMS_SetDestination(h_msg, phone_num);

    if (0 != ret)
    {
        pthread_mutex_unlock(&sms_mutex);
        log_e(LOG_MID, "set destination number failed, ret:%d", ret);
        return ret;
    }

    switch (mode)
    {
        case SMS_PDU:
            ret = QL_SMS_SetPDU(h_msg, msg, len);
            break;

        case SMS_TEXT:
            ret = QL_SMS_SetText(h_msg, msg);
            break;

        case SMS_UCS2:
            ret =  QL_SMS_SetUCS2(h_msg, (uint16_t *)msg, len / 2);
            break;

        case SMS_BINARY:
            ret = QL_SMS_SetBinary(h_msg, msg, len);
            break;

        default:
            break;
    }

    if (0 != ret)
    {
        pthread_mutex_unlock(&sms_mutex);
        log_e(LOG_MID, "set msg mode failed, ret:%d", ret);
        return ret;
    }

    ret = QL_SMS_Encode(h_msg);

    if (ret < 0)
    {
        pthread_mutex_unlock(&sms_mutex);
        log_e(LOG_MID, "Encode msg failed, ret:%d", ret);
        return ret;
    }

    ret = QL_SMS_Send(h_msg);

    if (0 != ret)
    {
        pthread_mutex_unlock(&sms_mutex);
        log_e(LOG_MID, "send msg failed, ret:%d", ret);
        return ret;
    }

    pthread_mutex_unlock(&sms_mutex);
    return 0;
}

/*****************************************************************************
function:     sms_release
description:  release a session
input:        none
output:       none
return:       none
*****************************************************************************/
void sms_release(void)
{
    pthread_mutex_lock(&sms_mutex);
    QL_SMS_Delete(h_msg);
    h_msg = NULL;
    pthread_mutex_unlock(&sms_mutex);
}

