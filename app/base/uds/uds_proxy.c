#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "log.h"
#include "timer.h"
#include "uds_proxy.h"
#include "uds_define.h"
#include "uds_request.h"
#include "uds_server.h"
#include "uds_client.h"
#include "uds_stack_def.h"


void UDS_SetDTCOn(void);

UDS_T    uds_server;
UDS_T    uds_client;
UDS_T    *uds_cfg;

static void (*uds_client_cb)(UDS_T *uds, int msg_id, int can_id, char *data, int len);
static int uds_hold_mode = 0;
static pthread_mutex_t uds_mtx = PTHREAD_MUTEX_INITIALIZER;  

#define UDS_PROXY_LOCK()    pthread_mutex_lock(&uds_mtx)
#define UDS_PROXY_UNLOCK()  pthread_mutex_unlock(&uds_mtx)

/*更改tbox uds server 模式，本地诊断 or 远程诊断，只有在默认会话，并未解锁安全等级时可更改模式*/
int uds_set_uds_server_mode(uint8_t        mode)
{
    int ret = 0;

    if(mode == uds_server.mode)
    {
        return 0;
    }
    else
    {
        if((g_u8CurrentSessionType == SESSION_TYPE_DEFAULT) && (g_u8SecurityAccess ==SecurityAccess_LEVEL0))
        {
            uds_server.mode = mode;
            ret = 0;
        }
        else
        {
            log_e(LOG_UDS, "uds server change mode error,g_u8CurrentSessionType:%d,g_u8SecurityAccess:%d",
                g_u8CurrentSessionType,g_u8SecurityAccess);
            ret = 1;
        }
    }

    return ret;
}


/*
    0:不是MCU返回的远程诊断响应
    1:是MCU返回的远程诊断响应*/
int is_remote_diag_response(unsigned char * msg)
{
    int ret = 0;
    int msg_type = msg[0];
    msg++;
    unsigned int can_id = 0;
    switch (msg_type)
    {
        case MSG_ID_UDS_S_ACK:/*设置UDS server*/
            ret = 0;
            break;
            
        case MSG_ID_UDS_C_ACK:/*设置UDS client*/
            ret = 1;
            break;
            
        case MSG_ID_UDS_CFM:
            can_id = msg[0] | (msg[1] << 8) | (msg[2] << 16) | (msg[3] << 24);
            if (can_id == uds_server.can_id_res)
            {
                ret = 0;
            }
            else if (can_id == uds_client.can_id_req)
            {
                ret = 1;
            }
            break;
            
        case MSG_ID_UDS_IND:
            can_id = msg[0] | (msg[1] << 8) | (msg[2] << 16) | (msg[3] << 24);
            if (can_id == uds_server.can_id_fun || can_id == uds_server.can_id_phy)
            {
                ret = 0;
            }
            else
            {
                ret = 1;
            }
            break;
        default:
            log_e(LOG_UDS, "unknown message: %x", msg_type);
            break;
    }
    return ret;
}

/****************************************************************
function:     uds_proxy_init
description:  init uds config
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int uds_proxy_init(void)
{
  //  int ret = 0;

    memset(&uds_server, 0, sizeof(uds_server));

    uds_server.mode       = UDS_TYPE_SERVER;
    uds_server.canport    = UDS_CAN_PORT;
    uds_server.can_id_phy = UDS_CAN_ID_PHY;
    uds_server.can_id_fun = UDS_CAN_ID_FUN;
    uds_server.can_id_res = UDS_CAN_ID_RES;
    uds_server.timer_t[P2SERVER].timer_value     = UDS_P2_SERVER_T;
    uds_server.timer_t[P2EXT_SERVER].timer_value = UDS_P2E_SERVER_T;
    uds_server.timer_t[S3SERVER].timer_value     = UDS_S3_SERVER_T;
    uds_server.timer_t[SERVER_SET].timer_value   = 1000;

    memset(&uds_client, 0, sizeof(uds_client));
    uds_client.mode = UDS_TYPE_CLIENT;
    uds_client.canport = 0;
    uds_client.can_id_phy = 0;
    uds_client.can_id_fun = 0;
    uds_client.can_id_res = 0;
    uds_client.timer_t[P2CLINT].timer_value = 150;
    uds_client.timer_t[P2EXT_CLIENT].timer_value = 5000;
    uds_client.timer_t[S3CLIENT].timer_value = 4000;
    uds_client.timer_t[CLIENT_SET].timer_value = 1000;

    uds_timer_create(&uds_server.timer_t[P2SERVER].timer_fd, P2SERVER);
    uds_timer_create(&uds_server.timer_t[P2EXT_SERVER].timer_fd, P2EXT_SERVER);
    uds_timer_create(&uds_server.timer_t[S3SERVER].timer_fd, S3SERVER);
    uds_timer_create(&uds_server.timer_t[SERVER_SET].timer_fd, SERVER_SET);

    uds_timer_create(&uds_client.timer_t[P2CLINT].timer_fd, P2CLINT);
    uds_timer_create(&uds_client.timer_t[P2EXT_CLIENT].timer_fd, P2EXT_CLIENT);
    uds_timer_create(&uds_client.timer_t[S3CLIENT].timer_fd, S3CLIENT);
    uds_timer_create(&uds_client.timer_t[CLIENT_SET].timer_fd, CLIENT_SET);
    
#if 0    
    ret |= tm_create(TIMER_REL, P2SERVER, MPU_MID_UDS, &uds_server.timer_t[P2SERVER].timer_fd);
    ret |= tm_create(TIMER_REL, P2EXT_SERVER, MPU_MID_UDS, &uds_server.timer_t[P2EXT_SERVER].timer_fd);
    ret |= tm_create(TIMER_REL, S3SERVER, MPU_MID_UDS, &uds_server.timer_t[S3SERVER].timer_fd);
    ret |= tm_create(TIMER_REL, SERVER_SET, MPU_MID_UDS, &uds_server.timer_t[SERVER_SET].timer_fd);

    if (0 != ret)
    {
        log_e(LOG_UDS, "create uds server timer failed, ret:%08x", ret);
        return ret;
    }

    ret |= tm_create(TIMER_REL, P2CLINT, MPU_MID_UDS, &uds_client.timer_t[P2CLINT].timer_fd);
    ret |= tm_create(TIMER_REL, P2EXT_CLIENT, MPU_MID_UDS, &uds_client.timer_t[P2EXT_CLIENT].timer_fd);
    ret |= tm_create(TIMER_REL, S3CLIENT, MPU_MID_UDS, &uds_client.timer_t[S3CLIENT].timer_fd);
    ret |= tm_create(TIMER_REL, CLIENT_SET, MPU_MID_UDS, &uds_client.timer_t[CLIENT_SET].timer_fd);

    if (0 != ret)
    {
        log_e(LOG_UDS, "create uds client timer failed, ret:%08x", ret);
        return ret;
    }
#endif
    return 0;
}

/****************************************************************
function:     uds_set_server
description:  set transport layer server
input:        UDS_SL_CFG_T *uds_cfg
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_set_server(void)
{
    UDS_TL_CFG_T server_cfg;

    server_cfg.canport    = UDS_CAN_PORT;
    server_cfg.fill_value = UDS_FILL_VALUE;
    server_cfg.fc_stmin   = UDS_FC_STMIN;
    server_cfg.fc_bs      = UDS_FC_BS;
    server_cfg.n_bs       = UDS_N_BS;
    server_cfg.n_cr       = UDS_N_CR;
    server_cfg.can_id_phy = UDS_CAN_ID_PHY;
    server_cfg.can_id_fun = UDS_CAN_ID_FUN;
    server_cfg.can_id_res = UDS_CAN_ID_RES;

    uds_server.mode = UDS_TYPE_SERVER;
    uds_server.canport = server_cfg.canport;
    uds_server.can_id_phy = server_cfg.can_id_phy;
    uds_server.can_id_fun = server_cfg.can_id_fun;
    uds_server.can_id_res = server_cfg.can_id_res;

    uds_set_timer(&uds_server, SERVER_SET, 1);
    return uds_data_request(&uds_server, MSG_ID_UDS_SET_S, 0, (unsigned char *)&server_cfg,
                            sizeof(server_cfg));
}

/****************************************************************
function:     uds_set_client
description:  set transport layer client
input:        UDS_SL_CFG_T *uds_cfg
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_set_client(void)
{
    UDS_TL_CFG_T client_cfg;

    client_cfg.canport = 0;
    client_cfg.fill_value = 0x00;
    client_cfg.fc_stmin = 50;
    client_cfg.fc_bs = 8;
    client_cfg.n_bs = 150;
    client_cfg.n_cr = 150;
    client_cfg.can_id_phy = 0;
    client_cfg.can_id_fun = 0;
    client_cfg.can_id_res = 0;

    uds_client.mode = UDS_TYPE_CLIENT;
    uds_client.canport = client_cfg.canport;
    uds_client.can_id_phy = client_cfg.can_id_phy;
    uds_client.can_id_fun = client_cfg.can_id_fun;
    uds_client.can_id_res = client_cfg.can_id_res;

    uds_set_timer(&uds_client, CLIENT_SET, 1);
    return uds_data_request(&uds_client, MSG_ID_UDS_SET_C, 0, (unsigned char *)&client_cfg,
                            sizeof(client_cfg));
}

/****************************************************************
function:     uds_timeout
description:  uds module timeout handle
input:        UDS_SL_TIMER_E timer_id
output:       none
return:       NULL
****************************************************************/
void uds_timeout(UDS_TIMER_E timer_id, uint16_t seq)
{
    UDS_T *uds = (timer_id == P2SERVER || timer_id == P2EXT_SERVER ||
                  timer_id == S3SERVER || timer_id == SERVER_SET) ? 
                 &uds_server : &uds_client;

    //printf("timeout: %d, %d, %llu\n", timer_id, seq, tm_get_time());
    if (uds->timer_t[timer_id].timer_fd.seq != seq)
    {
        return;
    }
    
    UDS_PROXY_LOCK();

    switch (timer_id)
    {
        case P2SERVER:
            log_e(LOG_UDS, "uds p2_server timeout!!!");
            break;

        case P2EXT_SERVER:
            log_e(LOG_UDS, "uds p2_ext_server timeout!!!");
            break;

        case S3SERVER:
            Set_Seesion_Default();
            Clear_SecurityAccess();
            UDS_SetDTCOn();
            log_e(LOG_UDS, "uds s3_server timeout!!!");
            break;

        case SERVER_SET:
            uds_cfg = &uds_server;
            uds_set_server();
            log_e(LOG_UDS, "uds config server timeout!!!");
            break;

        case CLIENT_SET:
            log_e(LOG_UDS, "uds config client timeout!!!");
            if (uds_client_cb)
            {
                uds_client_cb(&uds_client, -1, 0, NULL, 0);
            }            
            break;
        case P2CLINT:
            log_e(LOG_UDS, "uds p2_client timeout!!!");
            if (uds_client_cb)
            {
                uds_client_cb(&uds_client, -1, 0, NULL, 0);
            }
            break;
        case P2EXT_CLIENT:
            log_e(LOG_UDS, "uds p2_ext_client timeout!!!");
            if (uds_client_cb)
            {
                uds_client_cb(&uds_client, -1, 0, NULL, 0);
            }
            break;        
        case S3CLIENT:
            log_e(LOG_UDS, "uds s3_ext_client timeout!!!");
            //uds_set_timer(&uds_client, S3CLIENT, uds_hold_mode);
            if (uds_hold_mode)
            {   
                //uds_data_request(&uds_client, MSG_ID_UDS_REQ, uds_client.can_id_fun, "\x3E\x80", 2);
            }
        default:
            break;
    }

    UDS_PROXY_UNLOCK();
}

/****************************************************************
function:     uds_set_timer
description:  set the timer on/off
input:        UDS_T *uds,
              UDS_TIMER_E  name,
              uint8_t switch_value
output:       none
return:       NULL
****************************************************************/
void  uds_set_timer(UDS_T *uds, UDS_TIMER_E  name, uint8_t switch_value)
{
    if (name < TIMER_MAX)
    {
        uds->timer_t[name].timer_switch = switch_value;

        if (1 == switch_value)
        {
#if 0            
            tm_stop(uds->timer_t[name].timer_fd);
            tm_start(uds->timer_t[name].timer_fd, uds->timer_t[name].timer_value, TIMER_TIMEOUT_REL_ONCE);
#else
            uds_timer_settime(&uds->timer_t[name].timer_fd, uds->timer_t[name].timer_value);
#endif
            //printf("timer reset: %d, %d\n", name, uds->timer_t[name].timer_fd.seq);
        }
        else
        {
#if 0
            tm_stop(uds->timer_t[name].timer_fd);
#else
            uds_timer_settime(&uds->timer_t[name].timer_fd, 0);
#endif
            //printf("timer stop: %d\n", name);
        }
    }
}

/****************************************************************
function:     uds_confirm
description:  uds confirm frame handle
input:        UDS_T *uds,
              uint8_t result
output:       none
return:       NULL
****************************************************************/
static void uds_confirm(UDS_T *uds, uint8_t result)
{
    if (result == N_OK)
    {
        if (uds->mode == UDS_TYPE_CLIENT)
        {            
            uds_set_timer(uds, P2CLINT, 1);
        }
        else if (uds->mode == UDS_TYPE_SERVER)
        {
            uds_set_timer(uds, P2SERVER, 0);

            if (SID_NegativeResponse != uds->sid)
            {
                uds_set_timer(uds, P2EXT_SERVER, 0);
            }
        }
    }
}

/****************************************************************
function:     uds_proxy
description:  distribute confirm、indication frame
input:        uint8_t* msg,
              uint16_t length
output:       none
return:       NULL
****************************************************************/
void uds_proxy(uint8_t *msg, uint16_t len)
{
    int msg_id;
    int can_id;
    //int dlc;

    if (len < 1)
    {
        log_e(LOG_UDS, "message length invalid: %d", len);
        return;
    }
    
    /* msg[0]: msgid */
    msg_id = *msg++;
    len--;

    switch (msg_id)
    {
        case MSG_ID_UDS_S_ACK:
            /* acknowlege of setting server */
            uds_set_timer(&uds_server, SERVER_SET, 0);
            break;
        case MSG_ID_UDS_C_ACK:
            /* acknowlege of setting client */
            uds_set_timer(&uds_client, CLIENT_SET, 0);
            if (uds_client_cb)
            {
                uds_client_cb(&uds_client, msg_id, 0, NULL, 0);
            }
            break;
        case MSG_ID_UDS_CFM:
            /* confirm of data request */
            if (len < 5)
            {
                log_e(LOG_UDS, "message length invalid");
                break;
            }

            can_id = msg[0] | (msg[1] << 8) | (msg[2] << 16) | (msg[3] << 24);
            msg += 4;
            
            if (can_id == uds_server.can_id_res)
            {
                uds_confirm(&uds_server, *msg);
            }
            else if (can_id == uds_client.can_id_req)
            {
                uds_confirm(&uds_client, *msg);
            }
            
            break;
        case MSG_ID_UDS_IND:
            /* data receive indication */
            if (len < 6 || len - 6 != msg[4] + msg[5] * 0xff)
            {
                log_e(LOG_UDS, "message length invalid");
                break;
            }
            
            can_id = msg[0] | (msg[1] << 8) | (msg[2] << 16) | (msg[3] << 24);
            len -= 6;
            msg += 6;

            if (can_id == uds_server.can_id_fun || can_id == uds_server.can_id_phy)
            {
                uds_set_timer(&uds_server, P2SERVER, 1);
                uds_set_timer(&uds_server, S3SERVER, 1);
                uds_server_proc(&uds_server, can_id, msg, len);
            }
            else
            {
                uint8_t srv_id = SID_NegativeResponse == msg[0] ? msg[1] : msg[0] - POS_RESPOND_SID_MASK;

                if (srv_id == uds_client.sid && srv_id != SID_TestPresent)
                {
                    uds_set_timer(&uds_client, P2CLINT, 0);

                    if ((SID_NegativeResponse == msg[0]) &&
                        (NRC_RequestCorrectlyReceivedResponsePending == msg[2]))
                    {
                        log_i(LOG_UDS, "pending reset");
                        uds_set_timer(&uds_client, P2EXT_CLIENT, 1);
                    }
                    else
                    {
                        uds_set_timer(&uds_client, P2EXT_CLIENT, 0);
                            
                    }
                    
                    if (uds_client_cb)
                    {
                        uds_client_cb(&uds_client, msg_id, can_id,(char *)msg, len);
                    }
                    
                }
            }
            
            break;
        default:
            log_e(LOG_UDS, "unknown message: %x", msg_id);
            break;
    }
    #if 0
    if (MSG_ID_UDS_S_ACK == msg_id)
    {
        uds_set_timer(&uds_server, SERVER_SET, 0);
        return ;
    }
    else if (MSG_ID_UDS_C_ACK == msg_id)
    {
        uds_set_timer(&uds_client, CLIENT_SET, 0);
        if (uds_client_open_cb)
        {
            uds_client_open_cb(0);
        }
        return ;
    }
    else if (MSG_ID_UDS_CFM == msg_id)
    {
        /* if it is server message */
        if ((uds_client.can_id_phy == can_id)  ||
            (uds_client.can_id_fun == can_id))
        {
            uds_cfg = &uds_client;
        }
        /* if it is client message */
        else if (uds_server.can_id_res == can_id)
        {
            uds_cfg = &uds_server;
        }

        /*
          msg[0]:   msgid;
          msg[1~4]: canid;
          msg[5]:   result;
         */
        memcpy(&result, &msg[sizeof(char) + sizeof(int)], sizeof(char));        
        uds_confirm(uds_cfg, result);
        return ;
    }
    else
    {
        /*
          msg[0]:   msgid;
          msg[1~4]: canid;
          msg[5~6]: data length;
          msg[7...]: data
         */

        /* if it is server message */
        if ((uds_server.can_id_phy == can_id)  ||
            (uds_server.can_id_fun == can_id))
        {
            memcpy(&pdu_dlc, &msg[sizeof(char) + sizeof(int)], sizeof(short));
            pdu_data = &msg[sizeof(char) + sizeof(int) + sizeof(short)];
            uds_cfg = &uds_server;
        }
        /* if it is client message */
        else if (uds_client.can_id_res == can_id)
        {
            memcpy(&pdu_dlc, &msg[sizeof(char) + sizeof(int)], sizeof(short));
            pdu_data = &msg[sizeof(char) + sizeof(int) + sizeof(short)];
            uds_cfg = &uds_client;
        }
        else
        {
            log_e(LOG_UDS, "message error!!!");
            return;
        }
    }

    if (UDS_TYPE_CLIENT == uds_cfg->mode)
    {
        uds_set_timer(uds_cfg, P2CLINT, 0);

        if ((SID_NegativeResponse == pdu_data[0]) &&
            (NRC_RequestCorrectlyReceivedResponsePending == pdu_data[2]))
        {
            uds_set_timer(uds_cfg, P2EXT_CLIENT, 1);
        }
        else
        {
            uds_set_timer(uds_cfg, P2EXT_CLIENT, 0);
            uds_set_timer(uds_cfg, S3CLIENT, 1);
            if (uds_client_resp_cb)
            {
                uds_client_resp_cb(uds_cfg, can_id, pdu_data, pdu_dlc);
            }
        }
    }
    else if (UDS_TYPE_SERVER == uds_cfg->mode)
    {
        uds_set_timer(uds_cfg, P2SERVER, 1);
        uds_set_timer(uds_cfg, S3SERVER, 1);
        uds_server_proc(uds_cfg, can_id, pdu_data, pdu_dlc);
    }
    #endif
}

int uds_set_client_ex(int port, int pid, int fid, int rid, void *cb)
{
    UDS_TL_CFG_T client_cfg;

    client_cfg.canport = port;
    client_cfg.fill_value = 0x00;
    client_cfg.fc_stmin = 50;
    client_cfg.fc_bs = 8;
    client_cfg.n_bs = 150;
    client_cfg.n_cr = 150;
    client_cfg.can_id_phy = pid;
    client_cfg.can_id_fun = fid;
    client_cfg.can_id_res = rid;

    uds_client.mode = UDS_TYPE_CLIENT;
    uds_client.canport = client_cfg.canport;
    uds_client.can_id_phy = client_cfg.can_id_phy;
    uds_client.can_id_fun = client_cfg.can_id_fun;
    uds_client.can_id_res = client_cfg.can_id_res;

    uds_client_cb = cb;

    uds_set_timer(&uds_client, CLIENT_SET, 1);
    return uds_data_request(&uds_client, MSG_ID_UDS_SET_C, 0, (unsigned char *)&client_cfg,
                            sizeof(client_cfg));
}

int uds_clr_client(void)
{
    uds_set_timer(&uds_client, CLIENT_SET, 0);
    uds_set_timer(&uds_client, P2CLINT, 0);
    uds_set_timer(&uds_client, P2EXT_CLIENT, 0);
    UDS_PROXY_LOCK();
    uds_client_cb = NULL;
    UDS_PROXY_UNLOCK();
    return 0;
}

void uds_hold_client(int en)
{
    UDS_PROXY_LOCK();
    uds_hold_mode = en;
    //uds_set_timer(&uds_client, S3CLIENT, en);
    if (en)
    {
       //uds_data_request(&uds_client, MSG_ID_UDS_REQ, uds_client.can_id_fun, "\x3E\x80", 2);
    }
    UDS_PROXY_UNLOCK();
}