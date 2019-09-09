#include "remote_diag.h"
#include "log.h"
#include "uds_define.h"

#define UDS_P2_SERVER_T (50)

static remote_diag_state_t remote_diag_state;
extern timer_t remote_diag_request_timeout;

const unsigned int remote_diag_state_timeout[REMOTE_DIAG_CMD_NUM] = 
{
    0xffffffff,                                     //REMOTE_DIAG_CMD_NOT_START = 0,//未开始
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_OPENING,/* 已发送配置命令，等待响应*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_OPENED,/* 配置成功，尚未发送UDS诊断请求*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_SESSION_CONTROLLING,/* 会话控制 切换会话中*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_SESSION_CONTROLLED, /* 会话控制已满足 */
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTING,/* 安全访问请求种子中*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED,/* 安全访问已请求种子，并接收到种子*/
    UDS_P2_SERVER_T,                                //REMEOT_DIAG_CMD_SECURITY_KEY_SENDING,/* 安全访问，发送key中*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_SECURITY_KEY_SENDED,/* 安全访问已解锁 */
    REMOTE_DIAG_MSG_TIMEOUT_INTERVAL,               //REMOTE_DIAG_CMD_DIAGNOSING,/* 已发送诊断请求，等待诊断响应*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_DIAGNOSED,/* 已接收诊断响应，诊断结束*/
    UDS_P2_SERVER_T,                                //REMOTE_DIAG_CMD_ClOSING,/* 已发送关闭请求，等待响应*/
    0xffffffff                                      //REMOTE_DIAG_CMD_FINISHED,/* 已接收关闭，结束 */
};

extern const unsigned int REMOTE_DIAG_CAN_ID[REMOTE_DIAG_ECU_NUM][2];

void remote_diag_update_timer(unsigned int interval)
{
    if (get_current_diag_cmd_state() > REMOTE_DIAG_CMD_NOT_START)
    {
        tm_stop(remote_diag_request_timeout);
    }

    if (get_current_diag_cmd_state() < REMOTE_DIAG_CMD_FINISHED)
    {
        tm_start(remote_diag_request_timeout, interval, TIMER_TIMEOUT_REL_ONCE);
    }
}


/* 当前远程诊断命令状态更新
    0 更新成功
    1 更新失败*/
int remote_diag_cmd_state_update(void)
{
    int ret = 0;

    if (remote_diag_state.current_cmd_state < REMOTE_DIAG_CMD_FINISHED)
    {

        remote_diag_state.current_cmd_state++;
        remote_diag_update_timer(remote_diag_state_timeout[get_current_diag_cmd_state()]);
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/*存储诊断消息*/
int remote_diag_cmd_store_diag_response(char *remote_diag_msg, unsigned int mlen)
{
    remote_diag_response_t *response = get_current_diag_cmd_response();
    memcpy(response->diag_response, remote_diag_msg, mlen);
    response->diag_response_len = mlen;

    return 0;
}

/* 结束当前的诊断命令 */
int remote_diag_cmd_set_finish(REMOTE_DIAG_CMD_RESULT_TYPE ret_type)
{
    if (remote_diag_state.remote_diag_response_arr.remote_diag_response_size
        < remote_diag_state.remote_diag_request_arr.remote_diag_request_size)
    {
        remote_diag_state.remote_diag_response_arr.remote_diag_response_size++;
    }

    remote_diag_response_t *response = get_current_diag_cmd_response();
    response->result_type = ret_type;

    while (remote_diag_cmd_state_update() == 0); /* 设置当前执行命令状态为已结束 */

    return 0;
}

/* 获取当前远程诊断命令执行状态 */
REMOTE_DIAG_CMD_STATE_TYPE get_current_diag_cmd_state(void)
{
    return remote_diag_state.current_cmd_state;
}

/*获取当前命令 ECU 类型*/
REMOTE_DIAG_ECUTYPE get_current_diag_cmd_ecu_type(void)
{

    return get_current_diag_cmd_request()->ecu_type;
}

/*获取当前命令 安全等级*/
unsigned char get_current_diag_cmd_security_level(void)
{
    return get_current_diag_cmd_request()->level;
}


/* 获取当前远程诊断命令请求*/
remote_diag_request_t *get_current_diag_cmd_request(void)
{
    return &(remote_diag_state.remote_diag_request_arr.remote_diag_request[remote_diag_state.current_cmd_no]);
}

/* 获取当前远程诊断命令结果 */
remote_diag_response_t *get_current_diag_cmd_response(void)
{
    return &(remote_diag_state.remote_diag_response_arr.remote_diag_response[remote_diag_state.current_cmd_no]);
}

remote_diag_response_arr_t *get_remote_diag_response(void)
{
    return &(remote_diag_state.remote_diag_response_arr);
}

remote_diag_request_arr_t *get_remote_diag_request(void)
{
    return &(remote_diag_state.remote_diag_request_arr);
}

remote_diag_state_t *get_remote_diag_state_t(void)
{
    return &(remote_diag_state);
}



/* 获取远程诊断模块状态 */
REMOTE_DIAG_STATE_TYPE get_remote_diag_state(void)
{
    return remote_diag_state.remote_diag_state;
}

/* 更新远程诊断模块状态
    0 更新成功
    1 更新失败*/
int update_remote_diag_state(void)
{
    int ret = 0;

    if (remote_diag_state.remote_diag_state < REMOTE_DIAG_FINISHED)
    {
        remote_diag_state.remote_diag_state++;
        ret = 0;
    }
    else
    {
        log_e(LOG_REMOTE_DIAG, "remote diag state update failed,current state:REMOTE_DIAG_FINISHED");
        ret = 1;
    }

    return ret;
}

/* 是否有远程诊断需求
    1 有
    0 没有*/
int is_remote_diag_need(void)
{
    int ret = 0;

    if (remote_diag_state.remote_diag_request_arr.remote_diag_request_size == 0)
    {
        ret = 0;
    }
    else if (remote_diag_state.remote_diag_response_arr.remote_diag_response_size
             < remote_diag_state.remote_diag_request_arr.remote_diag_request_size)
    {
        ret = 1;
    }
    else if (get_current_diag_cmd_state() < REMOTE_DIAG_CMD_FINISHED)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}



/* 跳转到下一条诊断命令执行 */
int update_remote_diag_cmd_no(void)
{
    int ret = 0;

    if (remote_diag_state.current_cmd_no <
        (remote_diag_state.remote_diag_request_arr.remote_diag_request_size - 1))
    {
        remote_diag_state.current_cmd_no++;
        remote_diag_state.current_cmd_state = REMOTE_DIAG_CMD_NOT_START;
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

/* 1：远程诊断Tbox
   0：诊断其它ECU*/
int is_remote_diag_tbox(void)
{
    remote_diag_request_t *request = NULL;
    request = get_current_diag_cmd_request();

    if (request->request_id == REMOTE_DIAG_CAN_ID[REMOTE_DIAG_TBOX][DIAG_REQUEST_ID_ROW])
    {
        return 1;/* 远程诊断Tbox*/
    }
    else
    {
        return 0;/* 远程诊断其它ECU*/
    }
}

void init_remote_diag_state(void)
{
    remote_diag_state.current_cmd_no = 0;
    remote_diag_state.current_cmd_state = REMOTE_DIAG_CMD_NOT_START;
    memset(&(remote_diag_state.remote_diag_response_arr), 0x00, sizeof(remote_diag_response_arr_t));
    //memcpy(&(remote_diag_state.remote_diag_request_arr), &request_arr, sizeof(remote_diag_request_arr_t));
    remote_diag_state.remote_diag_state = REMOTE_DIAG_IDLE;
}






