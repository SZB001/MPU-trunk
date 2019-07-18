#include "remote_diag.h"
#include "log.h"
#include "uds_define.h"
#include "tcom_api.h"
#include "uds.h"

extern timer_t remote_diag_request_timeout;
extern UDS_T    uds_client;

/* 远程诊断时，当收到TBOX反馈的诊断响应消息，会执行此回调函数 */
static int remote_diag_set_client_callback(UDS_T *uds, int msg_id, int can_id, uint8_t *data, int len)
{
    switch (msg_id)
    {
        case MSG_ID_UDS_C_ACK:
            remote_diag_send_response(MPU_MID_UDS, REMOTE_DIAG_SET_MCU_RESPONSE, data, len);/* 向远程诊断发送设置MCU UDS ACK响应*/
            break;
        case MSG_ID_UDS_IND:
            remote_diag_send_response(MPU_MID_UDS, REMOTE_DIAG_MCU_RESPONSE, data, len);/*  MCU 返回的诊断响应 */
            break;
        default:
            break;
    }
    return 0;
}

/*0:发送诊断配置条件 已 满足
  1:发送诊断配置条件 未 满足*/
static int remote_diag_request_set_client_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = get_current_diag_cmd_request();
    remote_diag_response_t * response = get_current_diag_cmd_response();
    /* 前置条件以满足，当前命令状态为 REMOTE_DIAG_CMD_NOT_START */
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_NOT_START)
    {
        response->request_id = request->request_id;
        response->response_id = request->response_id;
        if(1 == is_remote_diag_tbox())
        {
            remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENING */
            remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENED */
            ret = 0;/* 发送诊断配置条件已满足 */
        }
        else
        {
            /* 发送 配置请求 给MCU */
            uds_set_client_ex(request->port, request->request_id,request->request_id,
                              request->response_id, remote_diag_set_client_callback);

            /* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_OPENING */
            remote_diag_cmd_state_update();


            ret = 1;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_OPENING)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*
    0:已满足接收到配置 MCU 响应 状态
    1:未满足接收到配置 MCU 响应 状态
*/
static int remote_diag_recieve_set_client_ack_process(unsigned int msgid)
{
    int ret = 0;

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_OPENING)
    {
        /* 接收到设置MCU UDS响应 */
        if(REMOTE_DIAG_SET_MCU_RESPONSE == msgid)
        {
            remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_OPENED */
            ret = 0;
        }
        else
        {
            ret = 1;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_OPENED)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*0:已满足
  1:未满足*/
static int remote_diag_request_session_control_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    
    request = get_current_diag_cmd_request();
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_OPENED)
    {
        /* 请求命令需要扩展会话*/
        if((SESSION_TYPE_EXTENDED == request->session))
        {
            StrToHex(request_tmp, (unsigned char *)EXTEND_SESSION_CMD, 2);/* 获取切换扩展会话命令 */
            remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(0x2E == request->diag_request[0])/* 请求为2E服务需切换 扩展会话*/
        {
            StrToHex(request_tmp, (unsigned char *)EXTEND_SESSION_CMD, 2);/* 获取切换扩展会话命令 */
            remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else/*不需要切换会话，不跳出switch，继续执行后续步骤 */
        {
            /* 直接更新会话控制为完成 */
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLED */
            ret = 0;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_SESSION_CONTROLLING)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*0:已满足
  1:未满足*/
static int remtoe_diag_recieve_session_control_ack_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SESSION_CONTROLLING)
    {
        if(REMOTE_DIAG_MCU_RESPONSE == msgid)
        {
            if(0 == is_remote_diag_tbox())/* 诊断其它ECU */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLED */
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SESSION_CONTROL_FAILED, 
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
        }
        else if(REMOTE_DIAG_UDS_RESPONSE == msgid)
        {
            if(1 == is_remote_diag_tbox())/* 诊断T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLED */
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SESSION_CONTROL_FAILED, 
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
            
        }
        else
        {
            ret = 1;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_SESSION_CONTROLLED)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }
    return ret;
}

/* 远程诊断，请求种子
   0:已请求
   1:未请求*/
static int remote_diag_request_seed_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    
    request = get_current_diag_cmd_request();
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);
    
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SESSION_CONTROLLED)
    {
        if(SecurityAccess_LEVEL1 == request->level)/*需要安全访问等级1*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL1_REQUEST_SEED, strlen(SECURITY_LEVEL1_REQUEST_SEED)/2);/* 获取切换扩展会话命令 */
            remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(SecurityAccess_LEVEL2 == request->level)/*需要安全访问等级3*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL3_REQUEST_SEED, strlen(SECURITY_LEVEL3_REQUEST_SEED)/2);/* 获取切换扩展会话命令 */
            remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(0x2E == request->diag_request[0])/* 请求为2E服务默认需 安全访问等级1*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL1_REQUEST_SEED, strlen(SECURITY_LEVEL1_REQUEST_SEED)/2);/* 获取切换扩展会话命令 */
            remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
            remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else/* 没有安全访问需要 */
        {
            /* 直接结束安全访问逻辑 */
            remote_diag_cmd_state_update();
            
            remote_diag_cmd_state_update();
            remote_diag_cmd_state_update();
            remote_diag_cmd_state_update();
            ret = 0;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTING)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int remote_diag_recieve_request_seed_ack_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTING)
    {
        if(REMOTE_DIAG_MCU_RESPONSE == msgid)
        {
            if(0 == is_remote_diag_tbox())/* 诊断T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* 诊断其它ECU */
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED */
                    ret = 0;
                }
                else
                {   
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED, 
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
        }
        else if(REMOTE_DIAG_UDS_RESPONSE == msgid)
        {
            if(1 == is_remote_diag_tbox())/* 诊断T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED */
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED, 
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
            
        }
        else
        {
            ret = 0;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED)
    {
        ret = 0;/*已满足该状态*/
    }
    else
    {
        ret = 1;/*不满足该状态*/
    }
    return ret;
}
/*0:已发送key
  1:未发送key*/
static int remote_diag_request_send_key_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    char request_tmp[DIAG_REQUEST_LEN];
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED)
    {
        request_tmp[0] = remote_diag_msg[0] - 0x40;
        request_tmp[1] = remote_diag_msg[1] + 0x01;
        remote_diag_calcKey(&(remote_diag_msg[2]), &(request_tmp[2]), 2);
        remote_diag_send_request((unsigned char *)request_tmp, 4);/*发送诊断命令*/
        remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMEOT_DIAG_CMD_SECURITY_KEY_SENDING */
        ret = 1;
    }
    else if(get_current_diag_cmd_state() >= REMEOT_DIAG_CMD_SECURITY_KEY_SENDING)
    {
        ret = 0;/*已满足该状态*/
    }
    else
    {
        ret = 1;
    }
    return ret;
}

/*
    1:已满足该状态
    0:不满足该状态*/
static int remote_diag_recieve_send_key_ack_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    if(get_current_diag_cmd_state() == REMEOT_DIAG_CMD_SECURITY_KEY_SENDING)
    {
        if(REMOTE_DIAG_MCU_RESPONSE == msgid)
        {
            if(0 == is_remote_diag_tbox())/* 诊断T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* 非消极响应 */
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SECURITY_KEY_SENDED */
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED,
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
        }
        else if(REMOTE_DIAG_UDS_RESPONSE == msgid)
        {
            if(1 == is_remote_diag_tbox())/* 诊断T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_SECURITY_KEY_SENDED */
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED,
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
            }
            else
            {
                ret = 1;
            }
            
        }
        else
        {
            ret = 1;
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_SECURITY_KEY_SENDED)
    {
        ret = 0;/*已满足该状态*/
    }
    else
    {
        ret = 1;/*不满足该状态*/
    }
    return ret;
}

/*0:已发送诊断命令
  1:未发送诊断命令*/
static int remote_diag_request_send_diag_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    request = get_current_diag_cmd_request();

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_KEY_SENDED)
    {
        /*发送诊断命令*/
        remote_diag_send_request(request->diag_request, request->diag_request_len);
        
        /* 开启响应超时定时器 */
        
        /* 更新诊断命令状态 */
        remote_diag_cmd_state_update();
        ret = 1;

    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_DIAGNOSING)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}


/* 处理UDS 返回的诊断Tbox的响应 */
static int remote_diag_recieve_request_diag_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    remote_diag_state_t * p_remote_diag_state = get_remote_diag_state_t();

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_DIAGNOSING)
    {
        if(1 == is_remote_diag_tbox())
        {
            if(msgid == REMOTE_DIAG_UDS_RESPONSE)
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* 非消极响应 */
                {
                    /* 处理UDS 返回的T-Box诊断结果 */
                    memcpy(&(p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].diag_response),
                            remote_diag_msg + 1, mlen - 1 );
                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].diag_response_len = mlen - 1;
                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].result_type = REMOTE_DIAG_CMD_RESULT_OK;

                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response_size++;
                    
                    remote_diag_cmd_state_update();
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_NEGATIVE,
                                                    remote_diag_msg, mlen);
                    ret = 1;
                }
                
            }
            else
            {
                ret = 1;
            }
            
        }
        else
        {
            if(msgid == REMOTE_DIAG_MCU_RESPONSE)
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* 非消极响应 */
                {
                    /* 处理MCU返回的其它ECU的诊断结果 */
                    memcpy(&(p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].diag_response),
                            remote_diag_msg, mlen);
                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].diag_response_len = mlen;
                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response[p_remote_diag_state->current_cmd_no].result_type = REMOTE_DIAG_CMD_RESULT_OK;

                    p_remote_diag_state->remote_diag_response_arr.remote_diag_response_size++;
            
                    remote_diag_cmd_state_update();
                    ret = 0;
                }
                else
                {
                    ret = remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_NEGATIVE,
                                                                        remote_diag_msg, mlen);
                    ret = 1;
                }
                
            }
            else
            {
                ret = 1;
            }
        }
    }
    else if(get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_DIAGNOSED)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*
    0:已正常启动单条诊断命令
    1:启动单条命令出错
*/
static int remote_diag_single_cmd_start(void)
{
    int ret = 0;
    
    ret = remote_diag_request_set_client_process() 
        || remote_diag_request_session_control_process()
        ||remote_diag_request_seed_process()
        ||remote_diag_request_send_diag_process();
    
    return (ret?0:1);

}

void remote_diag_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    if(remote_diag_single_cmd_process(msgid, remote_diag_msg, mlen) == 0)/* 当前命令已结束 */
    {
         /* 更新到下一条诊断命令 */
        update_remote_diag_cmd_no();
        
        /* 所有诊断命令都已执行完毕 */
        if(0 == is_remote_diag_need())
        {
            update_remote_diag_state();/* 更新诊断状态 */
            PP_rmtDiag_queryInform_cb();/* 通知PP诊断已结束 */
            remote_diag_msg_output();/* 打包诊断结果，并输出*/
            log_o(LOG_REMOTE_DIAG, "remote diag state:%d", get_remote_diag_state());
        }
        else
        {
            remote_diag_single_cmd_start();
        }
    }
}

/* 处理单个远程诊断命令，27服务会调用安全算法计算key
    如果远程诊断命令是诊断Tbox，则调用UDS诊断服务
    如果远程诊断命令是诊断其它ECU，则先向MCU发送UDS client端配置信息，再向MCU发送远程诊断命令
    当前只支持物理寻址*/
int remote_diag_single_cmd_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    
    ret = remote_diag_request_set_client_process()
        ||remote_diag_recieve_set_client_ack_process(msgid)
        ||remote_diag_request_session_control_process()
        ||remtoe_diag_recieve_session_control_ack_process(msgid, remote_diag_msg, mlen)
        ||remote_diag_request_seed_process()
        ||remote_diag_recieve_request_seed_ack_process(msgid, remote_diag_msg, mlen)
        ||remote_diag_request_send_key_process(msgid, remote_diag_msg, mlen)
        ||remote_diag_recieve_send_key_ack_process(msgid, remote_diag_msg, mlen)
        ||remote_diag_request_send_diag_process()
        ||remote_diag_recieve_request_diag_process(msgid, remote_diag_msg, mlen);

    return ret;
}





/* 错误处理 */
int remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_TYPE ret_type, 
    char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    remote_diag_cmd_set_error(ret_type, remote_diag_msg, mlen);/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_DIAGNOSED */
    update_remote_diag_cmd_no();
    
    /* 所有诊断命令都已执行完毕 */
    if(0 == is_remote_diag_need())
    {
        update_remote_diag_state();/* 更新诊断状态 */
        remote_diag_msg_output();/* 打包诊断结果，并输出*/
        log_o(LOG_REMOTE_DIAG, "remote diag state:%d", get_remote_diag_state());
        ret = 0;
    }
    else/* 还有诊断命令需要执行 */
    {
        ret = remote_diag_single_cmd_start();
    }
    return ret;
}






/* 处理远程诊断请求中的全部命令，并返回所有命令的执行结果，
    依次执行每个诊断命令，不会自动为需要解锁安全等级的服务自动执行解锁服务*/
int remote_diag_process_start(void)
{
    int ret = 0;
    REMOTE_DIAG_STATE_TYPE remote_diag_state = REMOTE_DIAG_IDLE;
    
    remote_diag_state = get_remote_diag_state();
    switch (remote_diag_state)
    {
        case REMOTE_DIAG_IDLE:
        case REMOTE_DIAG_FINISHED:
            init_remote_diag_state();
            /* 有远程诊断需求，更新远程诊断状态*/
            if(is_remote_diag_need())
            {
                ret = update_remote_diag_state();
                if(ret == 0)
                {
                    ret = remote_diag_single_cmd_start();
                }
            }
            break;
            
        case REMOTE_DIAG_EXECUTING:
        {
            log_e(LOG_REMOTE_DIAG, "Remote diagnosis is in progress!");
            break;
        }
            
        default:
            break;
    }
    return ret;

}


