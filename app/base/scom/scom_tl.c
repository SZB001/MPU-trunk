#include <stdlib.h>
#include "mid_def.h"
#include "com_app_def.h"
#include "scom.h"
#include "scom_dev.h"
#include "scom_tl.h"
#include "scom_api.h"
#include "scom_tl.h"
#include "msg_parse.h"
#include "ring_buffer.h"
#include "timer.h"

static int scom_tl_status = SCOM_TL_STATE_DISCONNECTED;

static unsigned char       scom_tl_tx_data[512 * 1024];
static struct ring_buffer  scom_tl_tx_rb;

static unsigned char       scom_tl_rx_data[2 * 1024];
static struct ring_buffer  scom_tl_rx_rb;
static SCOM_TL_RX_CTL      scom_tl_rx_data_ctl;

static scom_tx_fun        scom_tl_tx_fun;
static scom_msg_proc_fun  scom_tl_proc_fun[SCOM_TL_CHN_NUM];

static timer_t scom_tl_resend_timer;
static pthread_mutex_t scom_tx_mutex;

/*******************************************************************
function:     scom_tl_init
description:  initiaze transport layer
input:        INIT_PHASE phase, phase
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_init(INIT_PHASE phase)
{
    int ret = 0, i;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(&scom_tl_tx_data,     0, sizeof(scom_tl_tx_data));
            rb_init(&scom_tl_tx_rb, scom_tl_tx_data, sizeof(scom_tl_tx_data));

            memset(&scom_tl_rx_data_ctl, 0, sizeof(scom_tl_rx_data_ctl));
            scom_tl_rx_data_ctl.rx_fno = -1;

            memset(&scom_tl_rx_data,     0, sizeof(scom_tl_rx_data));
            rb_init(&scom_tl_rx_rb, scom_tl_rx_data, sizeof(scom_tl_rx_data));

            scom_tl_tx_fun   = NULL;
            pthread_mutex_init(&scom_tx_mutex, NULL);

            for (i = 0; i < SCOM_TL_CHN_NUM; i++)
            {
                scom_tl_proc_fun[i] = NULL;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:

            ret = tm_create(TIMER_REL, SCOM_MSG_ID_TIMER_NAME_RESEND, MPU_MID_SCOM, &scom_tl_resend_timer);

            if (ret != 0)
            {
                log_e(LOG_SCOM, "tm_create resend timer failed, ret:0x%08x", ret);
                return ret;
            }

            scom_tl_send_con_req();
            break;

        default:
            break;
    }

    return 0;
}

/*******************************************************************
function:     scom_tl_reg_tx_fun
description:  register sending function
input:        scom_tx_fun tx_fun, send message function
output:       none
return:       none
********************************************************************/
void scom_tl_reg_tx_fun(scom_tx_fun tx_fun)
{
    scom_tl_tx_fun = tx_fun;
}

/*******************************************************************
function:     scom_tl_reg_proc_fun
description:  register application message process function
input:        scom_msg_proc_fun proc_fun
output:       none
return:       none
********************************************************************/
int scom_tl_reg_proc_fun(SCOM_TL_CHAN chn, scom_msg_proc_fun proc_fun)
{
    if (chn >= SCOM_TL_CHN_NUM)
    {
        log_e(LOG_SCOM, "channel is valid, chn:%u", chn);
        return -1;
    }

    scom_tl_proc_fun[chn] = proc_fun;

    return 0;
}

/*******************************************************************
function:     scom_tl_get_state
description:  get transport layer state
input:        none
output:       none
return:       the current state
********************************************************************/
int scom_tl_get_state(void)
{
    return scom_tl_status;
}

/*******************************************************************
function:     scom_tl_send_frame
description:  send one frame to peer
input:        unsigned char type, message_type;
              unsigned char frame_type, frame type;
              unsigned short frame_no, frame no;
              unsigned char *data, message body;
              unsigned int len, message body length
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_send_frame(unsigned char msg_type, unsigned char frame_type,
                       unsigned short frame_no,
                       unsigned char *data, unsigned int len)
{
    int ret;
    unsigned char cs = 0;
    unsigned int msg_len;
    SCOM_TL_MSG_HDR msg_hdr;
    unsigned char omsg[512];

    msg_hdr.msg_type  = msg_type;
    msg_hdr.ftype     = frame_type;
    msg_hdr.fno       = frame_no;
    msg_hdr.flen      = len;

    ret = msg_encode((unsigned char *)&msg_hdr, sizeof(msg_hdr), omsg, sizeof(omsg), FIRST_SEG, &cs);

    if (ret <= 0)
    {
        log_e(LOG_SCOM, "msg_encode hdr failed");
        return -1;
    }

    msg_len = ret;

    ret = msg_encode(data, len, omsg + msg_len, sizeof(omsg) - msg_len, LAST_SEG, &cs);

    if (ret <= 0)
    {
        log_e(LOG_SCOM, "msg_encode failed");
        return -1;
    }
    else
    {
        msg_len += ret;
    }

    if (NULL != scom_tl_tx_fun)
    {
        pthread_mutex_lock(&scom_tx_mutex);
        scom_tl_tx_fun(omsg, msg_len);
        pthread_mutex_unlock(&scom_tx_mutex);
    }

    return 0;
}

/*******************************************************************
function:     scom_tl_send_con_req
description:  send connect request to peer
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_send_con_req(void)
{
    int ret;

    log_o(LOG_SCOM, "send connect request");

    ret = scom_tl_send_frame(SCOM_TL_CMD_CON_REQ, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send connect request failed, ret:0x%08x", ret);
    }

    tm_start(scom_tl_resend_timer, 1000, TIMER_TIMEOUT_REL_ONCE);

    scom_tl_status = SCOM_TL_STATE_CONNECTING;

    return;
}

/*******************************************************************
function:     scom_tl_send_con_res
description:  send connect response to peer
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_send_con_res(void)
{
    int ret;

    ret = scom_tl_send_frame(SCOM_TL_CMD_CON_RES, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send connect response failed, ret:0x%08x", ret);
    }

    return;
}

/*******************************************************************
function:     scom_tl_proc_con_req
description:  process connect request from peer
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_proc_con_req(void)
{
    static unsigned char get_time = 50;

    scom_tl_send_con_res();

    if (scom_tl_status < SCOM_TL_STATE_IDLE)
    {
        scom_tl_status = SCOM_TL_STATE_IDLE;
		log_o(LOG_SCOM, "spi is connected");
    }

    if (get_time)
    {
        scom_tl_send_frame(SCOM_TL_CMD_GET_TIME, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);
        get_time--;
    }
}

/*******************************************************************
function:     scom_tl_proc_con_res
description:  proc connect response from peer
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_proc_con_res(void)
{
    if (scom_tl_status <= SCOM_TL_STATE_IDLE)
    {
        tm_stop(scom_tl_resend_timer);
        scom_tl_status = SCOM_TL_STATE_IDLE;
        log_o(LOG_SCOM, "spi is connected");        
        scom_tl_send_frame(SCOM_TL_CMD_GET_TIME, SCOM_TL_SINGLE_FRAME, 0, NULL, 0);
    }
}

/*******************************************************************
function:     scom_tl_send_msg
description:  send command to peers immediately
input:        unsigned char msgid, message id;
              unsigned char *data, message body;
              unsigned int len,    message body length;
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_send_msg(unsigned char msg_type, unsigned char *data, unsigned int len)
{
    int ret;

    if (len > SCOM_TL_FRAME_LEN)
    {
        log_e(LOG_SCOM, "too long msg, len:%u", len);
        return -1;
    }

    ret = scom_tl_send_frame(msg_type, SCOM_TL_SINGLE_FRAME, 0, data, len);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send frame failed, ret:0x%08x", ret);
    }

    return ret;
}

/***************************************************************************
function:     scom_tl_send_data_req
description:  send data request to mcu
input:        none
output:       none
return:       none
***************************************************************************/
void scom_tl_send_data_req()
{
    SCOM_TL_DATA_REQ_MSG req_msg;

    /* all data is recevied */
    if (SCOM_TL_LAST_FRAME == scom_tl_rx_data_ctl.rx_ftype)
    {
        log_e(LOG_SCOM, "all data is received");
        return;
    }

    if (rb_unused_len(&scom_tl_rx_rb) >= SCOM_TL_FRAME_LEN)
    {
        req_msg.fno  = scom_tl_rx_data_ctl.rx_fno + 1;
        req_msg.fcnt = rb_unused_len(&scom_tl_rx_rb) / SCOM_TL_FRAME_LEN;

        if (req_msg.fcnt > 5)
        {
            req_msg.fcnt = 5;
        }

        scom_tl_rx_data_ctl.req_fcnt = req_msg.fcnt;
        scom_tl_rx_data_ctl.req_fno  = req_msg.fno;

        scom_tl_send_frame(SCOM_TL_DATA_REQ, SCOM_TL_SINGLE_FRAME, 0,
                           (unsigned char *)&req_msg, sizeof(req_msg));

        log_i(LOG_SCOM, "req data, fno:%u, fcnt:%u", req_msg.fno, req_msg.fcnt);

        tm_start(scom_tl_resend_timer, 1000, TIMER_TIMEOUT_REL_ONCE);
    }
}

/*******************************************************************
function:     scom_tl_start_server
description:  start server for the client to download data
input:        unsigned char *data, message body;
              unsigned int len, message body length;
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_start_server(unsigned char *data, unsigned int len)
{
    if (SCOM_TL_STATE_SERVER_STARTED == scom_tl_status)
    {
        log_e(LOG_SCOM, "server is already started");
        return -1;
    }

    rb_clean(&scom_tl_tx_rb);
    log_o(LOG_SCOM, "rb_unused_len = %d", rb_unused_len(&scom_tl_tx_rb));

    if (rb_unused_len(&scom_tl_tx_rb) < len)
    {
        log_e(LOG_SCOM, "too long data");
        return -1;
    }

    rb_in(&scom_tl_tx_rb, data, len);

    scom_tl_status = SCOM_TL_STATE_SERVER_STARTED;

    log_o(LOG_SCOM, "server started");

    return 0;
}

/*******************************************************************
function:     scom_tl_stop_server
description:  stop server
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_stop_server(void)
{
    rb_clean(&scom_tl_tx_rb);

    scom_tl_status = SCOM_TL_STATE_IDLE;

    //log_o(LOG_SCOM, "server stoped");

    return;
}

/*******************************************************************
function:     scom_tl_start_client
description:  start client to down data from server
input:        none
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_start_client()
{
    if (SCOM_TL_STATE_CLIENT_STARTED == scom_tl_status)
    {
        log_e(LOG_SCOM, "client is already started");
        return -1;
    }

    rb_clean(&scom_tl_rx_rb);

    memset(&scom_tl_rx_data_ctl, 0, sizeof(scom_tl_rx_data_ctl));
    scom_tl_rx_data_ctl.rx_fno = -1;

    scom_tl_send_data_req();

    scom_tl_status = SCOM_TL_STATE_CLIENT_STARTED;

    log_o(LOG_SCOM, "client started");

    return 0;
}

/*******************************************************************
function:     scom_tl_stop_client
description:  stop client
input:        none
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
void scom_tl_stop_client()
{
    rb_clean(&scom_tl_rx_rb);
    memset(&scom_tl_rx_data_ctl, 0, sizeof(scom_tl_rx_data_ctl));
    scom_tl_rx_data_ctl.rx_fno = -1;

    scom_tl_status = SCOM_TL_STATE_IDLE;

    log_o(LOG_SCOM, "client stoped");
}

/*******************************************************************
function:     scom_tl_proc_data_req
description:  process data req
input:        none
output:       none
return:       0 indicates success;
              others indicate failed
********************************************************************/
int scom_tl_proc_data_req(unsigned char *msg, unsigned int len)
{
    int i, ret;
    unsigned char ftype;
    unsigned short max_fno;
    static unsigned char frame[SCOM_TL_FRAME_LEN];
    SCOM_TL_DATA_REQ_MSG *tl_msg;
    SCOM_TL_NG_RES_MSG res;

    /* the server is not startup */
    if (SCOM_TL_STATE_SERVER_STARTED != scom_tl_status)
    {
        log_e(LOG_SCOM, "data server is not start up");

        res.result = SCOM_TL_RET_SERVER_NOT_START;
        res.reason = 0;
        scom_tl_send_msg(SCOM_TL_DATA_NG_RES, (unsigned char *)&res, sizeof(res));
        return -1;
    }

    if (rb_empty(&scom_tl_tx_rb))
    {
        log_e(LOG_SCOM, "no data in data server");

        res.result = SCOM_TL_RET_DATA_UNAVAILABLE;
        res.reason = 0;
        scom_tl_send_msg(SCOM_TL_DATA_NG_RES, (unsigned char *)&res, sizeof(res));
        return -1;
    }

    if (rb_used_len(&scom_tl_tx_rb) % SCOM_TL_FRAME_LEN)
    {
        max_fno = rb_used_len(&scom_tl_tx_rb) / SCOM_TL_FRAME_LEN;
    }
    else
    {
        max_fno = rb_used_len(&scom_tl_tx_rb) / SCOM_TL_FRAME_LEN - 1;
    }

    tl_msg  = (SCOM_TL_DATA_REQ_MSG *)(msg + sizeof(SCOM_TL_MSG_HDR));

    if (tl_msg->fno > max_fno)
    {
        res.result = SCOM_TL_RET_INVALID_FNO;
        res.reason = max_fno;
        scom_tl_send_msg(SCOM_TL_DATA_NG_RES, (unsigned char *)&res, sizeof(res));
        return -1;
    }

    for (i = 0; i < tl_msg->fcnt && (tl_msg->fno + i <= max_fno); i++)
    {
        if (0 == tl_msg->fno + i)
        {
            if (0 == max_fno)
            {
                ftype = SCOM_TL_SINGLE_FRAME;
            }
            else
            {
                ftype = SCOM_TL_START_FRAME;
            }
        }
        else if (max_fno == tl_msg->fno + i)
        {
            ftype = SCOM_TL_LAST_FRAME;
        }
        else if (tl_msg->fno + i > max_fno)
        {
            break;
        }
        else
        {
            ftype = SCOM_TL_MIDDLE_FRAME;
        }

        ret = rb_get(&scom_tl_tx_rb, frame, (tl_msg->fno + i) * SCOM_TL_FRAME_LEN, SCOM_TL_FRAME_LEN);

        if (ret <= 0)
        {
            log_e(LOG_SCOM, "get data failed");
            return -1;
        }

        scom_tl_send_frame(SCOM_TL_DATA_OK_RES, ftype, tl_msg->fno + i, frame, ret);
    }

    return 0;
}

/***************************************************************************
function:     scom_tl_proc_data_ok_res
description:  send command to mcu
input:        unsigned char *msg, message body;
              unsigned int len, message body length
output:       none
return:       none
***************************************************************************/
void scom_tl_proc_data_ok_res(unsigned char *msg, unsigned int len)
{
    SCOM_TL_MSG *tl_msg = (SCOM_TL_MSG *)msg;

    if (tl_msg->hdr.fno != scom_tl_rx_data_ctl.rx_fno + 1)
    {
        log_e(LOG_SCOM, "invalid frame, expect frame:%u, recv frame:%u",
              tl_msg->hdr.fno, scom_tl_rx_data_ctl.rx_fno + 1);
        return;
    }

    log_i(LOG_SCOM, "receive data, fno:%u, flen:%u, ftype:%u",
          tl_msg->hdr.fno, tl_msg->hdr.flen, tl_msg->hdr.ftype);

    scom_tl_rx_data_ctl.used     = 1;
    scom_tl_rx_data_ctl.rx_fno   = tl_msg->hdr.fno;
    scom_tl_rx_data_ctl.rx_flen  = tl_msg->hdr.flen;
    scom_tl_rx_data_ctl.rx_ftype = tl_msg->hdr.ftype;
    scom_tl_rx_data_ctl.rx_len  += tl_msg->hdr.flen;

    rb_in(&scom_tl_rx_rb, tl_msg->data, tl_msg->hdr.flen);

    /* all data is received */
    if (SCOM_TL_LAST_FRAME == tl_msg->hdr.ftype)
    {
        tm_stop(scom_tl_resend_timer);
        return;
    }

    /* the last requested frame is received, reqest the next frames */
    if (tl_msg->hdr.fno == (scom_tl_rx_data_ctl.req_fno + scom_tl_rx_data_ctl.req_fcnt))
    {
        tm_stop(scom_tl_resend_timer);
        scom_tl_send_data_req();
    }
}

/*******************************************************************
function:     scom_tl_proc_data_ack
description:  send command to mcu
input:        unsigned char *msg, message body;
              unsigned int len, message body length
output:       none
return:       none
********************************************************************/
void scom_tl_proc_data_ng_res(unsigned char *msg, unsigned int len)
{
    SCOM_TL_NG_RES_MSG *ng_res_msg;

    tm_stop(scom_tl_resend_timer);

    ng_res_msg = (SCOM_TL_NG_RES_MSG *)(msg + sizeof(SCOM_TL_MSG_HDR));
    log_e(LOG_SCOM, "get data failed,%u,%u", ng_res_msg->result, ng_res_msg->reason);
}

/*******************************************************************
function:     scom_tl_msg_proc
description:  send command to mcu
input:        unsigned char *msg, message body;
output:       unsigned int len, message length;
return:       none
********************************************************************/
void scom_tl_msg_proc(unsigned char *msg, unsigned int len)
{
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;
    
    log_i(LOG_SCOM, "recv msg from mcu, %u", tl_hdr->msg_type);

    switch (tl_hdr->msg_type)
    {
        case SCOM_TL_DATA_REQ:
            scom_tl_proc_data_req(msg, len);
            break;

        case SCOM_TL_DATA_OK_RES:
            scom_tl_proc_data_ok_res(msg, len);
            break;

        case SCOM_TL_DATA_NG_RES:
            scom_tl_proc_data_ng_res(msg, len);
            break;

        case SCOM_TL_CMD_CON_REQ:
            scom_tl_proc_con_req();
            break;

        case SCOM_TL_CMD_CON_RES:
            scom_tl_proc_con_res();
            break;

        default:
            if (((tl_hdr->msg_type & 0xf0) >> 4) >= SCOM_TL_CHN_NUM)
            {
                log_e(LOG_SCOM, "invalid msg type, type:%u", tl_hdr->msg_type);
            }
            else
            {
                if (NULL != scom_tl_proc_fun[(tl_hdr->msg_type & 0xf0) >> 4])
                {
                    scom_tl_proc_fun[(tl_hdr->msg_type & 0xf0) >> 4](msg, len);
                }
                else
                {
                    log_e(LOG_SCOM, "scom_tl_proc_fun is null,type:%u", tl_hdr->msg_type);
                }
            }

            break;
    }
}

/*******************************************************************
function:     scom_tl_timeout_proc
description:  resend timer timeout message
input:        none
output:       none
return:       none
********************************************************************/
void scom_tl_timeout_proc()
{
    if (SCOM_TL_STATE_DISCONNECTED == scom_tl_status
        || SCOM_TL_STATE_CONNECTING == scom_tl_status)
    {
        scom_tl_send_con_req();
    }
    else
    {
        if (SCOM_TL_STATE_SERVER_STARTED == scom_tl_status)
        {
        }
    }
}

/*******************************************************************
function:     scom_tl_get_data
description:  get data
input:        unsigned char *buffer, the data dl from server;
              unsigned int len, the data length dl from server;
output:       unsigned char *fin, 1 indicates receiving the data is finished;
                                  0 indicates receiving the data is not finished;
return:       the size of the data;
              -1 indicates get data error;
              others the size of the data
********************************************************************/
int scom_tl_get_data(unsigned char *buffer, unsigned int len, unsigned char *fin)
{
    int ret;

    *fin = 0;

    /* the server is not startup */
    if (SCOM_TL_STATE_CLIENT_STARTED != scom_tl_status)
    {
        log_e(LOG_SCOM, "client is not startup,status:%u", scom_tl_status);
        *fin = 1;
        return -1;
    }

    /* no more data,  there are two reason:
       1, all data is reveived;
       2, the data dl from peer is too slowly, call this function later */
    if (rb_empty(&scom_tl_rx_rb))
    {
        log_e(MPU_MID_SCOM, "ring buffer is empty");

        if (SCOM_TL_LAST_FRAME == scom_tl_rx_data_ctl.rx_ftype)
        {
            *fin = 1;
        }

        return 0;
    }

    ret = rb_out(&scom_tl_rx_rb, buffer, len);

    if (ret <= 0)
    {
        log_e(LOG_SCOM, "get data failed");
        return 0;
    }

    if (SCOM_TL_LAST_FRAME != scom_tl_rx_data_ctl.rx_ftype)
    {
        scom_tl_send_data_req();
    }

    /* all data is reveived, stop the client */
    if ((SCOM_TL_LAST_FRAME == scom_tl_rx_data_ctl.rx_ftype) && rb_empty(&scom_tl_rx_rb))
    {
        log_o(LOG_SCOM, "get all data");

        *fin = 1;
        scom_tl_stop_client();
    }

    return ret;
}

