#include "remote_diag.h"
#include "log.h"
#include "uds_define.h"
#include "tcom_api.h"
#include "uds.h"

extern timer_t remote_diag_request_timeout;
extern UDS_T    uds_client;

/* Զ�����ʱ�����յ�TBOX�����������Ӧ��Ϣ����ִ�д˻ص����� */
static int remote_diag_set_client_callback(UDS_T *uds, int msg_id, int can_id, uint8_t *data, int len)
{
    switch (msg_id)
    {
        case MSG_ID_UDS_C_ACK:
            remote_diag_send_response(MPU_MID_UDS, REMOTE_DIAG_SET_MCU_RESPONSE, data, len);/* ��Զ����Ϸ�������MCU UDS ACK��Ӧ*/
            break;
        case MSG_ID_UDS_IND:
            remote_diag_send_response(MPU_MID_UDS, REMOTE_DIAG_MCU_RESPONSE, data, len);/*  MCU ���ص������Ӧ */
            break;
        default:
            break;
    }
    return 0;
}

/*0:��������������� �� ����
  1:��������������� δ ����*/
static int remote_diag_request_set_client_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = get_current_diag_cmd_request();
    remote_diag_response_t * response = get_current_diag_cmd_response();
    /* ǰ�����������㣬��ǰ����״̬Ϊ REMOTE_DIAG_CMD_NOT_START */
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_NOT_START)
    {
        response->request_id = request->request_id;
        response->response_id = request->response_id;
        if(1 == is_remote_diag_tbox())
        {
            remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENING */
            remote_diag_cmd_state_update();/* REMOTE_DIAG_CMD_OPENED */
            ret = 0;/* ��������������������� */
        }
        else
        {
            /* ���� �������� ��MCU */
            uds_set_client_ex(request->port, request->request_id,request->request_id,
                              request->response_id, remote_diag_set_client_callback);

            /* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_OPENING */
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
    0:��������յ����� MCU ��Ӧ ״̬
    1:δ������յ����� MCU ��Ӧ ״̬
*/
static int remote_diag_recieve_set_client_ack_process(unsigned int msgid)
{
    int ret = 0;

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_OPENING)
    {
        /* ���յ�����MCU UDS��Ӧ */
        if(REMOTE_DIAG_SET_MCU_RESPONSE == msgid)
        {
            remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_OPENED */
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

/*0:������
  1:δ����*/
static int remote_diag_request_session_control_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    
    request = get_current_diag_cmd_request();
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_OPENED)
    {
        /* ����������Ҫ��չ�Ự*/
        if((SESSION_TYPE_EXTENDED == request->session))
        {
            StrToHex(request_tmp, (unsigned char *)EXTEND_SESSION_CMD, 2);/* ��ȡ�л���չ�Ự���� */
            remote_diag_send_request(request_tmp, 2);/*�����������*/
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(0x2E == request->diag_request[0])/* ����Ϊ2E�������л� ��չ�Ự*/
        {
            StrToHex(request_tmp, (unsigned char *)EXTEND_SESSION_CMD, 2);/* ��ȡ�л���չ�Ự���� */
            remote_diag_send_request(request_tmp, 2);/*�����������*/
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else/*����Ҫ�л��Ự��������switch������ִ�к������� */
        {
            /* ֱ�Ӹ��»Ự����Ϊ��� */
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLED */
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

/*0:������
  1:δ����*/
static int remtoe_diag_recieve_session_control_ack_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SESSION_CONTROLLING)
    {
        if(REMOTE_DIAG_MCU_RESPONSE == msgid)
        {
            if(0 == is_remote_diag_tbox())/* �������ECU */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLED */
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
            if(1 == is_remote_diag_tbox())/* ���T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLED */
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

/* Զ����ϣ���������
   0:������
   1:δ����*/
static int remote_diag_request_seed_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    unsigned char request_tmp[DIAG_REQUEST_LEN];
    
    request = get_current_diag_cmd_request();
    memset(request_tmp, 0x00, DIAG_REQUEST_LEN);
    
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SESSION_CONTROLLED)
    {
        if(SecurityAccess_LEVEL1 == request->level)/*��Ҫ��ȫ���ʵȼ�1*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL1_REQUEST_SEED, strlen(SECURITY_LEVEL1_REQUEST_SEED)/2);/* ��ȡ�л���չ�Ự���� */
            remote_diag_send_request(request_tmp, 2);/*�����������*/
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(SecurityAccess_LEVEL2 == request->level)/*��Ҫ��ȫ���ʵȼ�3*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL3_REQUEST_SEED, strlen(SECURITY_LEVEL3_REQUEST_SEED)/2);/* ��ȡ�л���չ�Ự���� */
            remote_diag_send_request(request_tmp, 2);/*�����������*/
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else if(0x2E == request->diag_request[0])/* ����Ϊ2E����Ĭ���� ��ȫ���ʵȼ�1*/
        {
            StrToHex(request_tmp, (unsigned char *)SECURITY_LEVEL1_REQUEST_SEED, strlen(SECURITY_LEVEL1_REQUEST_SEED)/2);/* ��ȡ�л���չ�Ự���� */
            remote_diag_send_request(request_tmp, 2);/*�����������*/
            remote_diag_cmd_state_update();/* ���µ�ǰ���״̬Ϊ REMOTE_DIAG_CMD_SESSION_CONTROLLING */
            ret = 1;
        }
        else/* û�а�ȫ������Ҫ */
        {
            /* ֱ�ӽ�����ȫ�����߼� */
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
            if(0 == is_remote_diag_tbox())/* ���T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* �������ECU */
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED */
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
            if(1 == is_remote_diag_tbox())/* ���T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED */
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
        ret = 0;/*�������״̬*/
    }
    else
    {
        ret = 1;/*�������״̬*/
    }
    return ret;
}
/*0:�ѷ���key
  1:δ����key*/
static int remote_diag_request_send_key_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    char request_tmp[DIAG_REQUEST_LEN];
    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED)
    {
        request_tmp[0] = remote_diag_msg[0] - 0x40;
        request_tmp[1] = remote_diag_msg[1] + 0x01;
        remote_diag_calcKey(&(remote_diag_msg[2]), &(request_tmp[2]), 2);
        remote_diag_send_request((unsigned char *)request_tmp, 4);/*�����������*/
        remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMEOT_DIAG_CMD_SECURITY_KEY_SENDING */
        ret = 1;
    }
    else if(get_current_diag_cmd_state() >= REMEOT_DIAG_CMD_SECURITY_KEY_SENDING)
    {
        ret = 0;/*�������״̬*/
    }
    else
    {
        ret = 1;
    }
    return ret;
}

/*
    1:�������״̬
    0:�������״̬*/
static int remote_diag_recieve_send_key_ack_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    if(get_current_diag_cmd_state() == REMEOT_DIAG_CMD_SECURITY_KEY_SENDING)
    {
        if(REMOTE_DIAG_MCU_RESPONSE == msgid)
        {
            if(0 == is_remote_diag_tbox())/* ���T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)/* ��������Ӧ */
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SECURITY_KEY_SENDED */
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
            if(1 == is_remote_diag_tbox())/* ���T-Box */
            {
                if(remote_diag_msg[0] != SID_NegativeResponse)
                {
                    remote_diag_cmd_state_update();/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_SECURITY_KEY_SENDED */
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
        ret = 0;/*�������״̬*/
    }
    else
    {
        ret = 1;/*�������״̬*/
    }
    return ret;
}

/*0:�ѷ����������
  1:δ�����������*/
static int remote_diag_request_send_diag_process(void)
{
    int ret = 0;
    remote_diag_request_t * request = NULL;
    request = get_current_diag_cmd_request();

    if(get_current_diag_cmd_state() == REMOTE_DIAG_CMD_SECURITY_KEY_SENDED)
    {
        /*�����������*/
        remote_diag_send_request(request->diag_request, request->diag_request_len);
        
        /* ������Ӧ��ʱ��ʱ�� */
        
        /* �����������״̬ */
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


/* ����UDS ���ص����Tbox����Ӧ */
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
                if(remote_diag_msg[0] != SID_NegativeResponse)/* ��������Ӧ */
                {
                    /* ����UDS ���ص�T-Box��Ͻ�� */
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
                if(remote_diag_msg[0] != SID_NegativeResponse)/* ��������Ӧ */
                {
                    /* ����MCU���ص�����ECU����Ͻ�� */
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
    0:���������������������
    1:���������������
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
    if(remote_diag_single_cmd_process(msgid, remote_diag_msg, mlen) == 0)/* ��ǰ�����ѽ��� */
    {
         /* ���µ���һ��������� */
        update_remote_diag_cmd_no();
        
        /* ������������ִ����� */
        if(0 == is_remote_diag_need())
        {
            update_remote_diag_state();/* �������״̬ */
            PP_rmtDiag_queryInform_cb();/* ֪ͨPP����ѽ��� */
            remote_diag_msg_output();/* �����Ͻ���������*/
            log_o(LOG_REMOTE_DIAG, "remote diag state:%d", get_remote_diag_state());
        }
        else
        {
            remote_diag_single_cmd_start();
        }
    }
}

/* ������Զ��������27�������ð�ȫ�㷨����key
    ���Զ��������������Tbox�������UDS��Ϸ���
    ���Զ������������������ECU��������MCU����UDS client��������Ϣ������MCU����Զ���������
    ��ǰֻ֧������Ѱַ*/
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





/* ������ */
int remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_TYPE ret_type, 
    char * remote_diag_msg, unsigned int mlen)
{
    int ret = 0;
    remote_diag_cmd_set_error(ret_type, remote_diag_msg, mlen);/* ���µ�ǰ�������״̬Ϊ REMOTE_DIAG_CMD_DIAGNOSED */
    update_remote_diag_cmd_no();
    
    /* ������������ִ����� */
    if(0 == is_remote_diag_need())
    {
        update_remote_diag_state();/* �������״̬ */
        remote_diag_msg_output();/* �����Ͻ���������*/
        log_o(LOG_REMOTE_DIAG, "remote diag state:%d", get_remote_diag_state());
        ret = 0;
    }
    else/* �������������Ҫִ�� */
    {
        ret = remote_diag_single_cmd_start();
    }
    return ret;
}






/* ����Զ����������е�ȫ��������������������ִ�н����
    ����ִ��ÿ�������������Զ�Ϊ��Ҫ������ȫ�ȼ��ķ����Զ�ִ�н�������*/
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
            /* ��Զ��������󣬸���Զ�����״̬*/
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


