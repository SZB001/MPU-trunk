#include "remote_diag.h"
#include "log.h"
#include "uds_define.h"
#include "uds_stack_def.h"

static remote_diag_state_t remote_diag_state;
extern timer_t remote_diag_request_timeout;

/* ��ǰԶ���������״̬���� 
    0 ���³ɹ�
    1 ����ʧ��*/
int remote_diag_cmd_state_update(void)
{
    int ret = 0;
    if(remote_diag_state.current_cmd_state < REMOTE_DIAG_CMD_DIAGNOSED)
    {
        if(remote_diag_state.current_cmd_state > REMOTE_DIAG_CMD_NOT_START)
        {
            tm_stop(remote_diag_request_timeout);
        }
        remote_diag_state.current_cmd_state++;
        if(remote_diag_state.current_cmd_state < REMOTE_DIAG_CMD_DIAGNOSED)
        {
            tm_start(remote_diag_request_timeout, REMOTE_DIAG_MSG_TIMEOUT_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
        }
        
        ret = 0;
    }
    else
    {
        ret = 1;
    }
    return ret;
}

/*������ǰ�����������ڷ��������ĳ�ʱ����*/
int remote_diag_cmd_set_error(REMOTE_DIAG_CMD_RESULT_TYPE ret_type, char * remote_diag_msg, unsigned int mlen)
{
    
    if(remote_diag_state.remote_diag_response_arr.remote_diag_response_size
        < remote_diag_state.remote_diag_request_arr.remote_diag_request_size)
    {
        remote_diag_state.remote_diag_response_arr.remote_diag_response_size++;
    }
    remote_diag_response_t * response = get_current_diag_cmd_response();
    memcpy(response->diag_response, remote_diag_msg, mlen);
    response->diag_response_len = mlen;
    response->result_type = ret_type;
    
    while(remote_diag_cmd_state_update() == 0);/* ���õ�ǰִ������״̬Ϊ�ѽ��� */
    return 0;
}

/* ��ȡ��ǰԶ���������ִ��״̬ */
REMOTE_DIAG_CMD_STATE_TYPE get_current_diag_cmd_state(void)
{
    return remote_diag_state.current_cmd_state;
}

/* ��ȡ��ǰԶ�������������*/
remote_diag_request_t * get_current_diag_cmd_request(void)
{
    return &(remote_diag_state.remote_diag_request_arr.remote_diag_request[remote_diag_state.current_cmd_no]);
}

/* ��ȡ��ǰԶ����������� */
remote_diag_response_t * get_current_diag_cmd_response(void)
{
    return &(remote_diag_state.remote_diag_response_arr.remote_diag_response[remote_diag_state.current_cmd_no]);
}

remote_diag_response_arr_t * get_remote_diag_response(void)
{
    return &(remote_diag_state.remote_diag_response_arr);
}

remote_diag_request_arr_t * get_remote_diag_request(void)
{
    return &(remote_diag_state.remote_diag_request_arr);
}

remote_diag_state_t * get_remote_diag_state_t(void)
{
    return &(remote_diag_state);
}



/* ��ȡԶ�����ģ��״̬ */
REMOTE_DIAG_STATE_TYPE get_remote_diag_state(void)
{
    return remote_diag_state.remote_diag_state;
}

/* ����Զ�����ģ��״̬ 
    0 ���³ɹ�
    1 ����ʧ��*/
int update_remote_diag_state(void)
{
    int ret = 0;
    if(remote_diag_state.remote_diag_state < REMOTE_DIAG_FINISHED)
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

/* �Ƿ���Զ��������� 
    1 ��
    0 û��*/
int is_remote_diag_need(void)
{
    int ret = 0;
    if(remote_diag_state.remote_diag_request_arr.remote_diag_request_size == 0)
    {
        ret = 0;
    }
    else if(remote_diag_state.remote_diag_response_arr.remote_diag_response_size 
            < remote_diag_state.remote_diag_request_arr.remote_diag_request_size)
    {
        ret = 1;
    }else if(get_current_diag_cmd_state() < REMOTE_DIAG_CMD_DIAGNOSED)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }
    return ret;
}



/* ��ת����һ���������ִ�� */
int update_remote_diag_cmd_no(void)
{
    int ret = 0;
    if(remote_diag_state.current_cmd_no < (remote_diag_state.remote_diag_request_arr.remote_diag_request_size - 1))
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

/* 1��Զ�����Tbox
   0���������ECU*/
int is_remote_diag_tbox(void)
{
    remote_diag_request_t * request = NULL;
    request = get_current_diag_cmd_request();
    if(request->request_id == UDS_CAN_ID_PHY)
    {
        return 1;/* Զ�����Tbox*/
    }
    else
    {
        return 0;/* Զ���������ECU*/
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






