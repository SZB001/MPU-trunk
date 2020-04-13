#include "remote_diag.h"
#include "log.h"
#include "uds_define.h"
#include "tcom_api.h"
#include "uds.h"
#include "can_api.h"

typedef int (*remote_request_proc_func)(remote_diag_request_t *request,
                                        remote_diag_response_t *response);

extern timer_t remote_diag_request_timeout;
extern UDS_T    uds_client;
remote_diag_precondition_sig_id precondition_sig_id;

remote_diag_precondition_sig_id *get_remote_diag_precondition_sig_id(void)
{
    return (&precondition_sig_id);
}
/* 远程诊断时，当收到TBOX反馈的诊断响应消息，会执行此回调函数 */
static int remote_diag_set_client_callback(UDS_T *uds, int msg_id, int can_id, uint8_t *data,
        int len)
{
    switch (msg_id)
    {
        case MSG_ID_UDS_C_ACK:
            send_msg_to_remote_diag(MPU_MID_UDS, REMOTE_DIAG_SET_MCU_RESPONSE, data,
                                    len);/* 向远程诊断发送设置MCU UDS ACK响应*/
            break;

        case MSG_ID_UDS_IND:
            send_msg_to_remote_diag(MPU_MID_UDS, REMOTE_DIAG_MCU_RESPONSE, data, len);/*  MCU 返回的诊断响应 */
            break;

        default:
            break;
    }

    return 0;
}

/*发送远程诊断请求给MCU或者T-Box UDS模块，本函数进行异常处理，具体的发送任务在remote_request_proc 函数中处理*/
static int remote_diag_request_process(REMOTE_DIAG_CMD_STATE_TYPE pre_state,
                                       remote_request_proc_func remote_request_proc)
{
    int ret = 0;
    remote_diag_request_t *request = get_current_diag_cmd_request();
    remote_diag_response_t *response = get_current_diag_cmd_response();

    /* 前置状态已满足 */
    if (get_current_diag_cmd_state() == pre_state)
    {
        ret = remote_request_proc(request, response);
    }
    else if (get_current_diag_cmd_state() >= (pre_state + 1))
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*发送设置 MCU UDS client端参数 */
int remote_diag_request_set_client(remote_diag_request_t *request, remote_diag_response_t *response)
{
    int ret = 0;

    response->request_id = request->request_id;
    response->response_id = request->response_id;

    if (1 == is_remote_diag_tbox())
    {
        remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENING */
        remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENED */
        ret = 0;/* 发送诊断配置条件已满足 */
    }
    else
    {
        /* 发送 配置请求 给MCU */
        uds_set_client_ex(request->port, request->request_id, request->request_id,
                          request->response_id, remote_diag_set_client_callback);

        /* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_OPENING */
        remote_diag_cmd_state_update();


        ret = 1;
    }

    return ret;
}


static int remote_diag_request_session_control(remote_diag_request_t *request,
        remote_diag_response_t *response)
{
    int ret = 0;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);

    /* 请求命令需要扩展会话*/
    if ((SESSION_TYPE_EXTENDED == request->session))
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

    return ret;
}

/* ack处理 */
static int remtoe_diag_set_ack(REMOTE_DIAG_CMD_RESULT_TYPE error_result_type, char *remote_diag_msg,
                               unsigned int mlen)
{
    int ret = 0;

    /*肯定响应处理*/
    if (remote_diag_msg[0] != SID_NegativeResponse) /*肯定响应处理*/
    {
        remote_diag_cmd_state_update();

        if (REMOTE_DIAG_CMD_FINISHED == get_current_diag_cmd_state())
        {
            ret = remote_diag_cmd_set_finish(REMOTE_DIAG_CMD_RESULT_OK);
        }
        else if (REMOTE_DIAG_CMD_DIAGNOSED == get_current_diag_cmd_state())
        {
            remote_diag_cmd_store_diag_response(remote_diag_msg, mlen);
        }
        else
        {
            ret = 0;
        }
    }
    else/* 否定响应处理 */
    {
        if (remote_diag_msg[2] == 0x78) /*78延迟*/
        {
            ret = 1;/* 不继续执行 */
            remote_diag_update_timer(2000);
        }
        else
        {
            ret = remote_diag_cmd_set_finish(error_result_type);/* 设置当前命令结束 */
        }
    }

    return ret;
}

/* ack前置条件处理 */
static int remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_TYPE error_result_type,
                                   REMOTE_DIAG_CMD_STATE_TYPE pre_state,
                                   unsigned int msgid, char *remote_diag_msg, unsigned int mlen)
{
    int ret = 0;

    if (get_current_diag_cmd_state() == pre_state)
    {
        if (((0 == is_remote_diag_tbox())  && (REMOTE_DIAG_MCU_RESPONSE == msgid))
            || ((1 == is_remote_diag_tbox()) && (REMOTE_DIAG_UDS_RESPONSE == msgid)))
        {
            remtoe_diag_set_ack(error_result_type, remote_diag_msg, mlen);
        }
        else
        {
            ret = 1;
        }
    }
    else if (get_current_diag_cmd_state() >= (pre_state + 1))
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int remote_diag_request_seed(remote_diag_request_t *request,
                                    remote_diag_response_t *response)
{
    int ret = 0;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);

    if (SecurityAccess_LEVEL1 == request->level) /*需要安全访问等级1*/
    {
        StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL1_REQUEST_SEED,
                 strlen(SECURITY_LEVEL1_REQUEST_SEED) / 2); /* 安全访问请求种子 */
        remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
        remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_SESSION_CONTROLLING */
        ret = 1;
    }
    else if (SecurityAccess_LEVEL2 == request->level) /*需要安全访问等级2*/
    {
        StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL2_REQUEST_SEED,
                 strlen(SECURITY_LEVEL2_REQUEST_SEED) / 2); /* 获取切换扩展会话命令 */
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

    return ret;
}

static int remote_diag_request_send_diag(remote_diag_request_t *request,
        remote_diag_response_t *response)
{
    int ret = 0;
    /*发送诊断命令*/
    remote_diag_send_request(request->diag_request, request->diag_request_len);

    /* 更新诊断命令状态 */
    remote_diag_cmd_state_update();
    ret = 1;
    return ret;
}

static int remote_diag_request_closing(remote_diag_request_t *request,
        remote_diag_response_t *response)
{
    int ret = 0;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);

    StrToHex(request_tmp, (unsigned char *)DEFAULT_SESSION_CMD, 2);/* 获取切换默认会话命令 */
    remote_diag_send_request(request_tmp, 2);/*发送诊断命令*/
    remote_diag_cmd_state_update();/* 更新当前诊断状态为 REMOTE_DIAG_CMD_ClOSING */
    ret = 1;

    return ret;
}


/*0:发送诊断配置条件 已 满足
  1:发送诊断配置条件 未 满足*/
static int remote_diag_request_set_client_process(void)
{
    int ret = 0;
    ret = remote_diag_request_process(REMOTE_DIAG_CMD_NOT_START, remote_diag_request_set_client);
    return ret;
}

/*
    0:已满足接收到配置 MCU 响应 状态
    1:未满足接收到配置 MCU 响应 状态
*/
static int remote_diag_recieve_set_client_ack_process(unsigned int msgid)
{
    int ret = 0;

    if (get_current_diag_cmd_state() == REMOTE_DIAG_CMD_OPENING)
    {
        /* 接收到设置MCU UDS响应 */
        if (REMOTE_DIAG_SET_MCU_RESPONSE == msgid)
        {
            remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_OPENED */
            ret = 0;
        }
        else
        {
            ret = 1;
        }
    }
    else if (get_current_diag_cmd_state() >= REMOTE_DIAG_CMD_OPENED)
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
    ret = remote_diag_request_process(REMOTE_DIAG_CMD_OPENED, remote_diag_request_session_control);
    return ret;
}

/*0:已满足
  1:未满足*/
static int remtoe_diag_recieve_session_control_ack_process(unsigned int msgid,
        char *remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    ret = remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_SESSION_CONTROL_FAILED,
                                  REMOTE_DIAG_CMD_SESSION_CONTROLLING, msgid, remote_diag_msg, mlen);
    return ret;
}

/* 远程诊断，请求种子
   0:已请求
   1:未请求*/
static int remote_diag_request_seed_process(void)
{
    int ret = 0;
    ret = remote_diag_request_process(REMOTE_DIAG_CMD_SESSION_CONTROLLED, remote_diag_request_seed);
    return ret;
}

/* 远程诊断，请求种子响应处理
   0:已请求
   1:未请求*/
static int remote_diag_recieve_request_seed_ack_process(unsigned int msgid, char *remote_diag_msg,
        unsigned int mlen)
{
    int ret = 0;
    ret = remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED,
                                  REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTING,
                                  msgid, remote_diag_msg, mlen);
    return ret;
}

/*0:已发送key
  1:未发送key*/
static int remote_diag_request_send_key_process(unsigned int msgid, char *remote_diag_msg,
        unsigned int mlen)
{
    int ret = 0;
    char request_tmp[DIAG_REQUEST_LEN];

    if (get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED)
    {
        request_tmp[0] = remote_diag_msg[0] - 0x40;
        request_tmp[1] = remote_diag_msg[1] + 0x01;

        remote_seed_To_Key(get_current_diag_cmd_ecu_type(), &(remote_diag_msg[2]), &(request_tmp[2]),
                           get_current_diag_cmd_security_level());
        remote_diag_send_request((unsigned char *)request_tmp, 6);/*发送诊断命令*/
        remote_diag_cmd_state_update();/* 更新当前诊断命令状态为 REMEOT_DIAG_CMD_SECURITY_KEY_SENDING */
        ret = 1;
    }
    else if (get_current_diag_cmd_state() >= REMEOT_DIAG_CMD_SECURITY_KEY_SENDING)
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
    0:已满足该状态
    1:不满足该状态*/
static int remote_diag_recieve_send_key_ack_process(unsigned int msgid, char *remote_diag_msg,
        unsigned int mlen)
{

    int ret = 0;
    ret = remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED,
                                  REMEOT_DIAG_CMD_SECURITY_KEY_SENDING, msgid, remote_diag_msg, mlen);
    return ret;
}

/*0:已发送诊断命令
  1:未发送诊断命令*/
static int remote_diag_request_send_diag_process(void)
{
    int ret = 0;
    ret = remote_diag_request_process(REMOTE_DIAG_CMD_SECURITY_KEY_SENDED,
                                      remote_diag_request_send_diag);
    return ret;
}

/*0:已发送诊断命令
  1:未发送诊断命令*/
static int remote_diag_request_closing_process(void)
{
    int ret = 0;
    ret = remote_diag_request_process(REMOTE_DIAG_CMD_DIAGNOSED,
                                      remote_diag_request_closing);
    return ret;
}


/* 处理UDS 返回的诊断Tbox的响应 */
static int remote_diag_recieve_request_diag_process(unsigned int msgid, char *remote_diag_msg,
        unsigned int mlen)
{
    int ret = 0;
    ret = remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_NEGATIVE, REMOTE_DIAG_CMD_DIAGNOSING, msgid,
                                  remote_diag_msg, mlen);
    return ret;
}

/* 处理切换默认会话命令响应 */
static int remote_diag_recieve_closing_process(unsigned int msgid, char *remote_diag_msg,
        unsigned int mlen)
{
    int ret = 0;
    ret = remtoe_diag_ack_process(REMOTE_DIAG_CMD_RESULT_CLOSING_FAILED, REMOTE_DIAG_CMD_ClOSING, msgid,
                                  remote_diag_msg, mlen);
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
          || remote_diag_request_seed_process()
          || remote_diag_request_send_diag_process();

    return (ret ? 0 : 1);

}

/* 远程诊断结束流程 */
static int remote_diag_finish_process(void)
{
    int ret = 0;

    /* 所有诊断命令都已执行完毕 */
    if (0 == is_remote_diag_need())
    {
        ret = update_remote_diag_state();/* 更新诊断状态 */

        if (ret == 0)
        {
            uds_set_client_ex(0, 0, 0, 0, NULL);
            remote_diag_excuted_result_output();/* 打包诊断结果，并输出*/
        }
    }
    else
    {
        ret = remote_diag_single_cmd_start();
    }

    return ret;
}

/*
    远程诊断处理函数 */
void remote_diag_process(unsigned int msgid, char *remote_diag_msg, unsigned int mlen)
{
    if (REMOTE_DIAG_EXECUTING == get_remote_diag_state())
    {
        if(remote_diag_single_cmd_process(msgid, remote_diag_msg, mlen) == 0) /* 当前命令已结束 */
        {
            /* 更新到下一条诊断命令 */
            if (update_remote_diag_cmd_no() == 1)
            {
                /* 更新到下一条命令失败，进行结束处理 */
                /* 判断是否所有远程诊断命令都已结束，若结束，将结果输出 */
                remote_diag_finish_process();
            }
            else
            {
                remote_diag_single_cmd_start();
            }
        }
    }
    else
    {
        log_o(LOG_REMOTE_DIAG, "remote diag state error:%d", get_remote_diag_state());
    }
}

/* 单条诊断命令处理
   0：当前执行的单条命令已结束
   1：当前执行的单条命令未结束*/
int remote_diag_single_cmd_process(unsigned int msgid, char *remote_diag_msg, unsigned int mlen)
{
    int ret = 0;

    ret = remote_diag_request_set_client_process()
          || remote_diag_recieve_set_client_ack_process(msgid)
          || remote_diag_request_session_control_process()
          || remtoe_diag_recieve_session_control_ack_process(msgid, remote_diag_msg, mlen)
          || remote_diag_request_seed_process()
          || remote_diag_recieve_request_seed_ack_process(msgid, remote_diag_msg, mlen)
          || remote_diag_request_send_key_process(msgid, remote_diag_msg, mlen)
          || remote_diag_recieve_send_key_ack_process(msgid, remote_diag_msg, mlen)
          || remote_diag_request_send_diag_process()
          || remote_diag_recieve_request_diag_process(msgid, remote_diag_msg, mlen)
          || remote_diag_request_closing_process()
          || remote_diag_recieve_closing_process(msgid, remote_diag_msg, mlen);

    return ret;
}

/* 超时处理处理 */
int remote_diag_timeout_process(char *remote_diag_msg, unsigned int mlen)
{
    int ret = 0;

    if((get_current_diag_cmd_state() < REMOTE_DIAG_CMD_ClOSING)
     &&(get_current_diag_cmd_state() > REMOTE_DIAG_CMD_OPENING))
    {
        while (get_current_diag_cmd_state() < REMOTE_DIAG_CMD_DIAGNOSED) /* 设置当前执行命令状态为诊断结束 */
        {
            remote_diag_cmd_state_update();
        }
        ret = remote_diag_request_closing_process();
        return ret;
    }
    
    /* 更新当前诊断命令状态为 REMOTE_DIAG_CMD_FINISHED */
    remote_diag_cmd_set_finish(REMOTE_DIAG_CMD_RESULT_TIMEOUT);

    /* 更新到下一条诊断命令 */
    if (update_remote_diag_cmd_no() == 1)
    {
        /* 更新到下一条命令失败，进行结束处理 */
        /* 判断是否所有远程诊断命令都已结束，若结束，将结果输出 */
        remote_diag_finish_process();
    }
    else
    {
        ret = remote_diag_single_cmd_start();
    }

    return ret;
}
/* 启动远程诊断流程
   0：已成功启动
   1：未成功启动*/
int remote_diag_process_start(char *remote_diag_msg, TCOM_MSG_HEADER msg)
{
    int ret = 0;

    switch (get_remote_diag_state())
    {
        case REMOTE_DIAG_IDLE:
        case REMOTE_DIAG_FINISHED:
            /* 解析远程诊断消息 */
            ret = parsing_remote_diag_msg(remote_diag_msg, msg,
                                          &(get_remote_diag_state_t()->remote_diag_request_arr));

            /* 解析消息失败 */
            if (ret != 0)
            {
                break;
            }

            /* 初始化远程诊断状态 */
            init_remote_diag_state();

            /* 有远程诊断需求，更新远程诊断状态*/
            if (is_remote_diag_need())
            {

                ret = update_remote_diag_state();

                if (ret == 0)
                {
                    ret = remote_diag_single_cmd_start();
                }
            }

            break;

        case REMOTE_DIAG_EXECUTING:
            {
                /*系统忙未执行*/
                log_e(LOG_REMOTE_DIAG, "Remote diagnosis is in progress!");
                break;
                ret = 1;
            }

        default:
            ret = 1;
            break;
    }

    return ret;
}

/* 退出远程诊断流程
   0：已成功退出
   1：未成功退出*/
int remote_diag_process_quit(void)
{
    remote_diag_state_t * rd_state = get_remote_diag_state_t();
    if((rd_state->remote_diag_state != REMOTE_DIAG_IDLE)
        && (rd_state->current_cmd_state != REMOTE_DIAG_CMD_NOT_START))
    {
        memset(rd_state, 0x00, sizeof(remote_diag_state_t));
        uds_set_client_ex(0, 0, 0, 0, NULL);
    }
    
    return 0;
}

