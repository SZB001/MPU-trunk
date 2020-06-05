/****************************************************************
file:         nm_net.c
description:  the source file of data communciation dial implementation,
              this function is implemented base on qualcomm qcril library
date:         2016/12/13
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "timer.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcom_api.h"
#include "cfg_para_api.h"
#include "cfg_api.h"
#include "nm_api.h"
#include "at.h"
#include "hozon_ver_api.h"
#include "nm_dial.h"
#include "pm_api.h"


static NM_REG_TBL   nm_tbl;
static NM_NET_INFO nm_net_info[NM_NET_TYPE_NUM];

static pthread_mutex_t nm_mutex;
static pthread_mutex_t nm_regtbl_mutex;

static unsigned char nm_dcom_status = 0;

extern unsigned int dsi_client_init;
extern unsigned int g_call_id;
extern int nm_sleep_available(void);

int net_apn_config(NET_TYPE type)
{
    int ret = 0;
    unsigned char profile_idx = 0;
    NM_NET_INFO *info;
    ql_apn_info_s apn;
    ql_apn_add_s apn_add;

    info = nm_net_info + type;
    profile_idx = type + 3;

    if (0 == strlen(info->apn))
    {
        log_e(LOG_NM, "net_apn_config apn is NULL,type is %d.", type);
        return 0;
    }

    memset(&apn, 0, sizeof(ql_apn_info_s));
    memset(&apn_add, 0, sizeof(ql_apn_add_s));

    ret = QL_APN_Get(profile_idx, &apn);

    if (ret != 0)
    {
        strcpy(apn_add.apn_name, info->apn);
        ret = QL_APN_Add(&apn_add, &profile_idx);

        if (ret != 0)
        {
            log_e(LOG_NM, "QL_APN_Add apn %d failed,ret is %d.", profile_idx, ret);
            return -1;
        }

        log_e(LOG_NM, "QL_APN_Get apn failed,profile_idx is %d.", profile_idx);
    }

    log_o(LOG_NM, "QL_APN_Get apn %d is %s.", profile_idx, apn.apn_name);

    if (0 != strcmp(apn.apn_name, info->apn))
    {
        apn.profile_idx = profile_idx;
        apn.pdp_type = QL_APN_PDP_TYPE_IPV4;
        strcpy(apn.apn_name, info->apn);

        ret = QL_APN_Set(&apn);

        if (ret != 0)
        {
            log_e(LOG_NM, "QL_APN_Set apn %d failed,ret is %d.", profile_idx, ret);
            return -1;
        }
    }

    log_o(LOG_NM, "net_apn_config apn %d is %s.", profile_idx, info->apn);

    return 0;

}


/****************************************************************
function:     nm_dial_notify_changed
description:  if network status is changed,this function will be called
input:        nm_status_changed callback
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int  nm_dial_notify_changed(NET_TYPE type, NM_STATE_MSG status)
{
    int i, ret;

    for (i = 0 ; i < nm_tbl.used_num; i++)
    {
        ret = nm_tbl.changed[i](type , status);

        if (ret != 0)
        {
            log_e(LOG_NM, "send net changed msg failed,ret:%d", ret);
            return NM_SEND_MSG_FAILED;
        }
    }

    return 0;
}

/****************************************************************
function:     nm_dial_loc_apn_changed
description:  when set the configuration,
              this function will be called
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int nm_dial_loc_apn_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                   unsigned char *new_para, unsigned int len)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    log_o(LOG_NM, "nm_net_local_apn_changed,id:%u", id);

    if (CFG_ITEM_LOCAL_APN != id)
    {
        log_e(LOG_NM, "invalid id, id:%u", id);
        return NM_INVALID_PARA;
    }

    /* not changed */
    if (0 == strcmp((char *)old_para , (char *)new_para))
    {
        log_o(LOG_NM, "nm_net_local_apn_changed,there no change!");
        return 0;
    }

    log_o(LOG_NM, "nm_net_local_apn_changed,old apn:%s,new apn:%s", old_para, new_para);

    /* update apn */
    memcpy(nm_net_info[NM_PRIVATE_NET].apn, (char *)new_para,
           sizeof(nm_net_info[NM_PRIVATE_NET].apn));

    msghdr.sender    = MPU_MID_NM;
    msghdr.receiver  = MPU_MID_NM;
    msghdr.msgid     = NM_MSG_ID_LOCAL_APN_CHANGED;
    msghdr.msglen    = 0;

    /* notify recreate the connection */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_NM, "tcom_send_msg failed, ret:%u", ret);
        return NM_SEND_MSG_FAILED;
    }

    return 0;
}

/****************************************************************
function:     nm_dial_wan_apn_changed
description:  when set the configuration, this function will be called
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int nm_dial_wan_apn_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                   unsigned char *new_para, unsigned int len)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    log_o(LOG_NM, "nm_net_wan_apn_changed,apn type:%u", id - 1);

    if (CFG_ITEM_WAN_APN != id)
    {
        log_e(LOG_NM, "invalid id, id:%u", id);
        return NM_INVALID_PARA;
    }

    /* not changed */
    if (0 == strcmp((char *)old_para , (char *)new_para))
    {
        log_o(LOG_NM, "nm_net_wan_apn_changed , there no change!");
        return 0;
    }

    log_o(LOG_NM, "nm_net_wan_apn_changed , old apn:%s, new apn:%s", old_para, new_para);

    /* update apn */
    memcpy(nm_net_info[NM_PUBLIC_NET].apn, (char *)new_para,
           sizeof(nm_net_info[NM_PUBLIC_NET].apn));

    msghdr.sender    = MPU_MID_NM;
    msghdr.receiver  = MPU_MID_NM;
    msghdr.msgid     = NM_MSG_ID_WAN_APN_CHANGED;
    msghdr.msglen    = 0;

    /* notify recreate the connection */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_NM, "tcom_send_msg failed, ret:%u", ret);
        return NM_SEND_MSG_FAILED;
    }

    return 0;
}


/****************************************************************
function:     nm_dial_auth_changed
description:  when set the configuration, this function will be called
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int nm_dial_auth_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                unsigned char *new_para, unsigned int len)
{
    int ret;
    CFG_DIAL_AUTH *auth;
    TCOM_MSG_HEADER msghdr;

    if ((CFG_ITEM_LOC_APN_AUTH != id) || (len != sizeof(CFG_DIAL_AUTH)))
    {
        log_e(LOG_NM, "invalid id, id:%u", id);
        return NM_INVALID_PARA;
    }

    auth = (CFG_DIAL_AUTH *)new_para;

    /* update para */
    memset(nm_net_info[NM_PRIVATE_NET].user, 0,
           sizeof(nm_net_info[NM_PRIVATE_NET].user));
    memcpy(nm_net_info[NM_PRIVATE_NET].user, auth->user,
           sizeof(nm_net_info[NM_PRIVATE_NET].user));
    memset(nm_net_info[NM_PRIVATE_NET].pwd, 0,
           sizeof(nm_net_info[NM_PRIVATE_NET].pwd));
    memcpy(nm_net_info[NM_PRIVATE_NET].pwd, auth->pwd,
           sizeof(nm_net_info[NM_PRIVATE_NET].pwd));

    msghdr.sender    = MPU_MID_NM;
    msghdr.receiver  = MPU_MID_NM;
    msghdr.msgid     = NM_MSG_ID_LOC_AUTH_CHANGED;
    msghdr.msglen    = 0;

    /* notify recreate the connection */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_NM, "tcom_send_msg failed, ret:%u", ret);
        return NM_SEND_MSG_FAILED;
    }

    return 0;
}


/****************************************************************
function:     nm_dial_init
description:  init data communciation dial module
input:        INIT_PHASE phase, init phase
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_dial_init(INIT_PHASE phase)
{
    int ret, i;
    unsigned int  len;
    NM_NET_INFO   *info;
    CFG_DIAL_AUTH auth;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&nm_mutex, NULL);
            pthread_mutex_init(&nm_regtbl_mutex, NULL);
            memset(&nm_tbl, 0, sizeof(nm_tbl));
            memset(&nm_net_info, 0x00, sizeof(nm_net_info));

            for (i = 0; i < NM_NET_TYPE_NUM; i++)
            {
                nm_net_info[i].cdma_index = 1;
                nm_net_info[i].umts_index = 1;
                nm_net_info[i].ip_ver     = DSI_IP_VERSION_4;
                nm_net_info[i].auth_pref  = DSI_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
                nm_net_info[i].state      = NM_NET_IDLE;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* get the APN of private network */
            info = nm_net_info + NM_PRIVATE_NET;
            info->type = NM_PRIVATE_NET;

            if (0 != pm_reg_handler(MPU_MID_NM, (sleep_handler)nm_sleep_available))
            {
                log_e(LOG_NM, "pm_reg_handler failed!!!");
                return NM_STATUS_INVALID;
            }

    		len = sizeof(auth);
    		ret = cfg_get_para(CFG_ITEM_LOC_APN_AUTH, &auth, &len);

            if (ret != 0)
            {
                log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
                return ret;
            }

            if ((0 != strlen(auth.user)) && (0 != strlen(auth.pwd)))
            {
                memcpy(info->user, auth.user, strlen(auth.user));
                memcpy(info->pwd, auth.pwd, strlen(auth.pwd));
            }

    ret = cfg_register(CFG_ITEM_LOC_APN_AUTH, nm_dial_auth_changed);

            if (ret != 0)
            {
                log_e(LOG_NM, "reg apn auth changed callback failed,ret:0x%08x", ret);
                return ret;
            }

            len = sizeof(info->apn);
            ret = cfg_get_para(CFG_ITEM_LOCAL_APN, (unsigned char *)info->apn, &len);

            if (ret != 0)
            {
                log_e(LOG_NM, "get local apn failed, ret:0x%08x", ret);
                return ret;
            }

            ret = cfg_register(CFG_ITEM_LOCAL_APN, nm_dial_loc_apn_changed);

            if (ret != 0)
            {
                log_e(LOG_NM, "reg local apn changed callback failed,ret:0x%08x", ret);
                return ret;
            }

            info->timername = NM_MSG_ID_PRIVATE_TIMER_RECALL;
            ret = tm_create(TIMER_REL, info->timername, MPU_MID_NM, &info->recall_timer);

            if (ret != 0)
            {
                log_e(LOG_NM, "tm_create reconnect private network timer failed, ret:0x%08x", ret);
                return ret;
            }

            /* get the APN of public network */
            info = nm_net_info + NM_PUBLIC_NET;
            info->type = NM_PUBLIC_NET;

            len = sizeof(info->apn);
            ret = cfg_get_para(CFG_ITEM_WAN_APN, (unsigned char *)info->apn, &len);

            if (ret != 0)
            {
                log_e(LOG_NM, "get wan apn failed, ret:0x%08x", ret);
                return ret;
            }

            ret = cfg_register(CFG_ITEM_WAN_APN, nm_dial_wan_apn_changed);

            if (ret != 0)
            {
                log_e(LOG_NM, "reg wan apn changed callback failed,ret:0x%08x", ret);
                return ret;
            }

            info->timername = NM_MSG_ID_PUBLIC_TIMER_RECALL;
            ret = tm_create(TIMER_REL, info->timername, MPU_MID_NM, &info->recall_timer);

            if (ret != 0)
            {
                log_e(LOG_NM, "tm_create reconnect public network timer failed, ret:0x%08x", ret);
                return ret;
            }

            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
function:     nm_dial_init_cb_fun
description:  init finish callback function
input:        void *user_data, user input data
output:       none
return:       none
****************************************************************/
void nm_dial_init_cb_fun(void *user_data)
{
    dsi_client_init = 1;
    log_o(LOG_NM, "dsi_init_ex successfully,dsi_client_init is %d.", dsi_client_init);
}

/****************************************************************
function:     nm_dial_status_cb_fun
description:  network state changed callback function
input:        dsi_hndl_t hndl, handle;
              void *user_data, user input data;
              dsi_net_evt_t evt, event id;
              dsi_evt_payload_t *payload_ptr, para;
output:       none
return:       none
****************************************************************/
static void nm_dial_status_cb_fun(dsi_hndl_t handle, void *user_data, dsi_net_evt_t evt,
                                  dsi_evt_payload_t *payload_ptr)
{
    int ret;
    unsigned int size = 0;
    unsigned int call_id;
    unsigned char buf[32] = {0};
    TCOM_MSG_HEADER msghdr;

    call_id = (unsigned int)user_data;

    memcpy(buf, &call_id, sizeof(call_id));
    size += sizeof(call_id);

    memcpy(buf + size, &evt, sizeof(evt));
    size += sizeof(evt);

    log_o(LOG_NM, "call_id=%d,hndl=%p,evt=%d, payload_ptr=%p,enter %s.", call_id, handle, evt,
          payload_ptr, __FUNCTION__);

    /* send message to nm */
    msghdr.sender   = MPU_MID_NM;
    msghdr.receiver = MPU_MID_NM;
    msghdr.msgid    = NM_MSG_ID_DIAL_STATUS;
    msghdr.msglen   = size;

    ret = tcom_send_msg(&msghdr, buf);

    if (ret != 0)
    {
        log_e(LOG_NM, "send message(msgid:%u) to moudle(0x%04x) failed", msghdr.msgid, msghdr.receiver);
    }

    log_o(LOG_NM, "nm_dial_status_cb_fun send NM_MSG_ID_DIAL_STATUS ok.");
}

/****************************************************************
function:     nm_sys_call
description:  call ds_system_call
input:        const char *command,  command;
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_sys_call(const char *command)
{
    int ret, i = 0;

    log_o(LOG_NM, "%s", command);

    while (i < 3)
    {
        ret = ds_system_call(command, strlen(command));

        if (ret != 0)
        {
            log_e(LOG_NM, "ds_system_call failed,ret:%x08x", ret);
            i++;
        }
        else
        {
            break;
        }
    }

    return ret;
}

/****************************************************************
function:     nm_dial_para_set
description:  set data call parameter
input:        NET_TYPE type, network type
output:       none
return:       none
****************************************************************/
void nm_dial_para_set(NET_TYPE type)
{
    dsi_call_param_value_t param_info;
    NM_NET_INFO *info;

    info = nm_net_info + type;

    info->umts_index = type + 1;
    info->cdma_index = type + 1;

    if (strlen(info->apn) != 0)
    {
        /* set data call param */
        param_info.buf_val = NULL;
        param_info.num_val = DSI_RADIO_TECH_UNKNOWN;
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_TECH_PREF, &param_info);

        param_info.buf_val = NULL;
        param_info.num_val = info->umts_index;
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_UMTS_PROFILE_IDX, &param_info);

        param_info.buf_val = NULL;
        param_info.num_val = info->cdma_index;
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_CDMA_PROFILE_IDX, &param_info);

        param_info.buf_val = NULL;
        param_info.num_val = info->ip_ver;
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_IP_VERSION, &param_info);

        param_info.buf_val = strdup(info->apn);
        param_info.num_val = strlen(param_info.buf_val);
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_APN_NAME, &param_info);
        free(param_info.buf_val);
        param_info.buf_val = NULL;

        param_info.buf_val = strdup(info->user);
        param_info.num_val = strlen(param_info.buf_val);
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_USERNAME, &param_info);
        free(param_info.buf_val);
        param_info.buf_val = NULL;

        param_info.buf_val = strdup(info->pwd);
        param_info.num_val = strlen(param_info.buf_val);
        dsi_set_data_call_param(info->handle, DSI_CALL_INFO_PASSWORD, &param_info);
        free(param_info.buf_val);

        if ((0 != strlen(info->user)) && (0 != strlen(info->pwd)))
        {
            info->auth_pref = DSI_AUTH_PREF_CHAP_ONLY_ALLOWED;
            param_info.buf_val = NULL;
            param_info.num_val = info->auth_pref;
            dsi_set_data_call_param(info->handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
        }
        else
        {
            param_info.buf_val = NULL;
            param_info.num_val = info->auth_pref;
            dsi_set_data_call_param(info->handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
        }
    }

}

/****************************************************************
function:     nm_dial_recall
description:  recall the data link
input:        NET_TYPE type, public network or private network
output:       none
return:       none
****************************************************************/
void nm_dial_recall(NET_TYPE type)
{
    int ret;
    NM_NET_INFO *info;

    info = nm_net_info + type;

    if (NM_NET_CONNECTED == nm_net_info[type].state)
    {
        log_o(LOG_NM, "net state is NM_NET_CONNECTED.");
        return;
    }

    if (NM_NET_CONNECTING == nm_net_info[type].state)
    {
        log_o(LOG_NM, "net state is NM_NET_CONNECTING.");
        return;
    }


    if (strlen(info->apn) != 0)
    {
        /* start recall timer */
        ret = tm_start(info->recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

        if (ret != 0)
        {
            log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x", type, ret);
        }

    }
    else
    {
        /* no apn,return,stop call */
        log_e(LOG_NM, "no apn,stop call,type = %d.", type);
        return ;
    }

    if (info->handle != NULL)
    {
        log_o(LOG_NM, "type = %d,dsi_rel_data_srvc_hndl,handle = %p. ", type, info->handle);
        dsi_rel_data_srvc_hndl(info->handle);
        info->handle = NULL;
    }

    if (NULL == info->handle)
    {
        /* acquire the handle. */
        g_call_id ++;

        /* over flow */
        if (g_call_id == 0)
        {
            g_call_id = 1;
        }

        info->call_id = g_call_id;
        info->handle = dsi_get_data_srvc_hndl(nm_dial_status_cb_fun, (void *)info->call_id);

        log_o(LOG_NM, "type = %d,dsi_get_data_srvc_hndl,handle = %p. ", type, info->handle);

        if (NULL == info->handle)
        {
            log_e(LOG_NM, "dsi_get_data_srvc_hndl fail,type:%u", type);
            return;
        }

        nm_dial_para_set(type);
    }

    if ((info->state != NM_NET_CONNECTED) &&
        (info->handle != NULL))
    {
        /* connecting WWAN */
        ret = dsi_start_data_call(info->handle);

        if (DSI_SUCCESS != ret)
        {
            log_e(LOG_NM, "dsi_start_data_call, type:%u,ret:0x%08x\n", type, ret);
        }
        else
        {
            info->state = NM_NET_CONNECTING;
            log_o(LOG_NM, "dsi_start_data_call successful,type is %d,state is DCOM_NET_CONNECTING.", type);
        }
    }
    else
    {
        log_e(LOG_NM, "info state is %d,handle is %p", info->state, info->handle);
    }
}

/****************************************************************
function:     nm_dial_call
description:  request data call
input:        NET_TYPE type, public network or private network
output:       none
return:       none
****************************************************************/
void nm_dial_call(NET_TYPE type)
{
    NM_NET_INFO *info;

    info = nm_net_info + type;

    if (NM_NET_CONNECTING == info->state)
    {
        log_o(LOG_NM, "net state is NM_NET_CONNECTING.");
        return;
    }

    if ((NULL == info->handle) &&
        (0 != strlen(info->apn)))
    {
        nm_dial_recall(type);
        log_o(LOG_NM, "nm_dial_recall,type:%u", type);
    }

    log_o(LOG_NM, "nm_dial_call,handle:%p,type:%u", info->handle, type);

    return ;
}

/****************************************************************
function:     nm_dial_add_default_route
description:  add ip as default route
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_add_default_route(NM_NET_INFO *phndl)
{
    char command[200];
    memset(command, 0, sizeof(command));

    /*add defaut route as the public route*/
    snprintf(command, sizeof(command), "ip route add default via %s dev %s", inet_ntoa(phndl->gw_addr),
             phndl->interface);
    nm_sys_call(command);

    nm_dcom_status = 1;
}
static void nm_dial_add_route(NM_NET_INFO *phndl)
{
    char command[200];
	char command1[200];
    memset(command, 0, sizeof(command));
	memset(command1, 0, sizeof(command1));

    /*add defaut route as the public route*/
	
	sprintf(command,"ip route add %s ",inet_ntoa(phndl->pri_dns_addr));
	sprintf(command1,"via %s dev %s ",inet_ntoa(phndl->gw_addr),phndl->interface);
	strcat(command,command1);
    
	nm_sys_call(command);

    nm_dcom_status = 1;
}

#if 0

/****************************************************************
function:     nm_dial_del_default_route
description:  add ip as default route
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_del_default_route(NM_NET_INFO *phndl)
{
    char command[200];
    memset(command, 0, sizeof(command));

    /*add defaut route as the public route*/
    snprintf(command, sizeof(command), "ip route del default via %s dev %s", inet_ntoa(phndl->gw_addr),
             phndl->interface);
    nm_sys_call(command);

    nm_dcom_status = 0;
}

#endif

/* adding DNS routing to add 8.8.8.8 when
   DNS is not obtained will cause the devices
   connected by USB to be unable to access the Internet */
#if 0

/****************************************************************
function:     nm_dial_add_dns_route
description:  add the dns ip to route list
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_add_dns_route(NM_NET_INFO *phndl)
{
    int len = 0;
    char command[200];

    memset(command, 0, sizeof(command));

    /*if dial DNS is not NULL,addr to route,otherwise addr "8.8.8.8" */
    if (phndl->pri_dns_addr.s_addr)
    {
        len = snprintf(command, sizeof(command), "ip route add %s via ", inet_ntoa(phndl->pri_dns_addr));
        snprintf(command + len, sizeof(command) - len , "%s dev %s", inet_ntoa(phndl->gw_addr),
                 phndl->interface);
        nm_sys_call(command);

        if (phndl->sec_dns_addr.s_addr)
        {
            len = snprintf(command, sizeof(command), "ip route add %s via ", inet_ntoa(phndl->sec_dns_addr));
            snprintf(command + len, sizeof(command) - len , "%s dev %s", inet_ntoa(phndl->gw_addr),
                     phndl->interface);
            nm_sys_call(command);
        }
    }
    else
    {
        len = snprintf(command, sizeof(command), "ip route add %s via ", "8.8.8.8");
        snprintf(command + len, sizeof(command) - len , "%s dev %s", inet_ntoa(phndl->gw_addr),
                 phndl->interface);
        nm_sys_call(command);
    }

}
#endif

/****************************************************************
function:     nm_dial_set_iptable
description:  set nat list of iptable
input:        char *iface
output:       none
return:       none
****************************************************************/
static void nm_dial_set_iptable(char *iface)
{
    char command[200];

    memset(command, 0, sizeof(command));

    /*add iptables rules*/
    snprintf(command, sizeof(command), "iptables -t nat -F POSTROUTING");
    nm_sys_call(command);

    snprintf(command, sizeof(command), "iptables -t nat -A POSTROUTING -o %s -j MASQUERADE --random" ,
             iface);
    nm_sys_call(command);
    snprintf(command, sizeof(command), "iptables -t filter -F");
    nm_sys_call(command);
}

/****************************************************************
function:     nm_dial_set_dns
description:  set dns configuration file
input:        nm_NET_INFO *phndl;
output:       none
return:       none
****************************************************************/
static int nm_dial_check_dns(char *dns_name)
{
	FILE *fp;
	char dns[50] = {0};
	int i = 0;
	fp = fopen("/etc/resolv.conf","r");
	
	if(fp == NULL)
	return 0;
	
	while(fgets(dns,50,fp) != NULL)
	{
		if(strncmp(dns,dns_name,strlen(dns_name)) == 0)
		{
			i++;
		}
		memset(dns,0 ,sizeof(dns));
	}
	fclose(fp);
	return i;
}

/****************************************************************
function:     nm_dial_set_dns
description:  set dns configuration file
input:        nm_NET_INFO *phndl;
output:       none
return:       none
****************************************************************/
static void nm_dial_set_dns(NM_NET_INFO *phndl)
{
    char pub_command[200];
	char pri_command[200];
	char dns_name[50] = {0};

    if(phndl->type == NM_PUBLIC_NET) //
    {
        if(phndl->pri_dns_addr.s_addr)
        {
            memset(pub_command, 0, sizeof(pub_command));
            snprintf(pub_command, sizeof(pub_command), "echo 'nameserver %s' > /etc/resolv.conf",
                    inet_ntoa(phndl->pri_dns_addr));         //公网DNS
            nm_sys_call(pub_command);

            if(phndl->sec_dns_addr.s_addr)
            {
                memset(pub_command, 0, sizeof(pub_command));
                snprintf(pub_command, sizeof(pub_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                        inet_ntoa(phndl->sec_dns_addr));
                nm_sys_call(pub_command);
            }
        }
        else if(phndl->sec_dns_addr.s_addr)
        {
            memset(pub_command, 0, sizeof(pub_command));
            snprintf(pub_command, sizeof(pub_command), "echo 'nameserver %s' > /etc/resolv.conf",
                    inet_ntoa(phndl->sec_dns_addr));         //公网DNS
            nm_sys_call(pub_command);
        }
        else
        {}
		#if 0
        if(nm_net_info[NM_PRIVATE_NET].pri_dns_addr.s_addr)
        {
            memset(pri_command, 0, sizeof(pri_command));
            snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                inet_ntoa(nm_net_info[NM_PRIVATE_NET].pri_dns_addr)); //私网DNS
            nm_sys_call(pri_command);
        }
        
        if(nm_net_info[NM_PRIVATE_NET].sec_dns_addr.s_addr)
        {
            memset(pri_command, 0, sizeof(pri_command));
            snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                inet_ntoa(nm_net_info[NM_PRIVATE_NET].sec_dns_addr)); //私网DNS
            nm_sys_call(pri_command);
        }
		#endif
    }
    else if(phndl->type == NM_PRIVATE_NET)
    {
        if(phndl->pri_dns_addr.s_addr)
        {
            memset(dns_name,0,sizeof(dns_name));
            snprintf(dns_name, sizeof(dns_name), "nameserver %s",inet_ntoa(phndl->pri_dns_addr));
            if(nm_dial_check_dns(dns_name) == 0)
            {
                memset(pri_command, 0, sizeof(pri_command));
                snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                    inet_ntoa(phndl->pri_dns_addr)); //私网DNS 
                nm_sys_call(pri_command);
            }
            
        }
        
        if(phndl->sec_dns_addr.s_addr)
        {
            memset(dns_name,0,sizeof(dns_name));
            snprintf(dns_name, sizeof(dns_name), "nameserver %s",inet_ntoa(phndl->sec_dns_addr));
            if(nm_dial_check_dns(dns_name) == 0)
            {
                memset(pri_command, 0, sizeof(pri_command));
                snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                    inet_ntoa(phndl->sec_dns_addr)); //私网DNS 
                nm_sys_call(pri_command);
            }
        }
    }
    else
    {}
	 
	if((!(phndl->sec_dns_addr.s_addr)) && (!(phndl->pri_dns_addr.s_addr)))	
    {
    	if(phndl->type == NM_PUBLIC_NET)
        {
            memset(pub_command, 0, sizeof(pub_command));
            snprintf(pub_command, sizeof(pub_command), "echo 'nameserver 114.114.114.114' > /etc/resolv.conf");
            nm_sys_call(pub_command);

            memset(pub_command, 0, sizeof(pub_command));
            snprintf(pub_command, sizeof(pub_command), "echo 'nameserver 180.76.76.76' >> /etc/resolv.conf");
            nm_sys_call(pub_command);
        }
		#if 0
        if(nm_net_info[NM_PRIVATE_NET].pri_dns_addr.s_addr)
        {
            memset(pri_command, 0, sizeof(pri_command));
            snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                inet_ntoa(nm_net_info[NM_PRIVATE_NET].pri_dns_addr)); //私网DNS
            nm_sys_call(pri_command);
        }
        
        if(nm_net_info[NM_PRIVATE_NET].sec_dns_addr.s_addr)
        {
            memset(pri_command, 0, sizeof(pri_command));
            snprintf(pri_command, sizeof(pri_command), "echo 'nameserver %s' >> /etc/resolv.conf",
                inet_ntoa(nm_net_info[NM_PRIVATE_NET].sec_dns_addr)); //私网DNS
            nm_sys_call(pri_command);
        }
		#endif
    }
}

/****************************************************************
function:     nm_dial_get_conf
description:  get ipv4 network configuration
input:        nm_NET_INFO *phndl, dial info;
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int nm_dial_get_conf(NM_NET_INFO *phndl)
{
    int ret;
    int num_entries = 1;
    char iface[DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2];
    char ip_str[20];
    dsi_addr_info_t addr_info;

    phndl->ip_addr.s_addr      = 0;
    phndl->gw_addr.s_addr      = 0;
    phndl->pri_dns_addr.s_addr = 0;
    phndl->sec_dns_addr.s_addr = 0;

    memset(phndl->interface, 0, DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2);
    memset(iface, 0, DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2);
    memset(&addr_info, 0, sizeof(dsi_addr_info_t));

    ret = dsi_get_device_name(phndl->handle, iface, DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 1);

    if (ret != DSI_SUCCESS)
    {
        log_e(LOG_NM, "couldn't get ipv4 rmnet name. ret:0x%08x", ret);
        strncpy((char *)iface, "rmnet0", DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 1);
    }

    memcpy(phndl->interface, iface, DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 1);
    log_o(LOG_NM, "IPv4 WAN Connected, NET type : %d , device_name:%s", phndl->type, iface);

    ret = dsi_get_ip_addr(phndl->handle, &addr_info, num_entries);

    if (ret != DSI_SUCCESS)
    {
        log_o(LOG_NM, "couldn't get ipv4 ip address. ret:0x%08x", ret);
        return ret;
    }

    if (addr_info.iface_addr_s.valid_addr)
    {
        if (SASTORAGE_FAMILY(addr_info.iface_addr_s.addr) == AF_INET)
        {
            memset(ip_str, 0, 20);
            snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                     SASTORAGE_DATA(addr_info.iface_addr_s.addr)[0],
                     SASTORAGE_DATA(addr_info.iface_addr_s.addr)[1],
                     SASTORAGE_DATA(addr_info.iface_addr_s.addr)[2],
                     SASTORAGE_DATA(addr_info.iface_addr_s.addr)[3]);
            phndl->ip_addr.s_addr = inet_addr(ip_str);
        }
    }

    if (addr_info.gtwy_addr_s.valid_addr)
    {
        memset(ip_str, 0, 20);
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                 SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[0],
                 SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[1],
                 SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[2],
                 SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[3]);
        phndl->gw_addr.s_addr = inet_addr(ip_str);
    }

    if (addr_info.dnsp_addr_s.valid_addr)
    {
        memset(ip_str, 0, 20);
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                 SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[0],
                 SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[1],
                 SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[2],
                 SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[3]);
        phndl->pri_dns_addr.s_addr = inet_addr(ip_str);
    }

    if (addr_info.dnss_addr_s.valid_addr)
    {
        memset(ip_str, 0, 20);
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                 SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[0],
                 SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[1],
                 SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[2],
                 SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[3]);
        phndl->sec_dns_addr.s_addr = inet_addr(ip_str);
    }

    log_o(LOG_NM, "public_ip: %s", inet_ntoa(phndl->ip_addr));
    log_o(LOG_NM, "gw_addr: %s",   inet_ntoa(phndl->gw_addr));
    log_o(LOG_NM, "pri_dns_addr: %s", inet_ntoa(phndl->pri_dns_addr));
    log_o(LOG_NM, "sec_dns_addr: %s", inet_ntoa(phndl->sec_dns_addr));

    /* if only one APN is configured or public networks is connected, set the router and dns */
    if ((NM_PUBLIC_NET == phndl->type) ||
        (NM_PRIVATE_NET == phndl->type && 0 == strlen(nm_net_info[NM_PUBLIC_NET].apn)))
    {
        /*add defaut route used for the public net*/
        nm_dial_add_default_route(phndl);

        /*add iptables rules*/
        nm_dial_set_iptable(iface);

        /*set DNS config file*/
        nm_dial_set_dns(phndl);
        system("iptables -t filter -F");
    }

    if (NM_PRIVATE_NET == phndl->type && 0 != strlen(nm_net_info[NM_PUBLIC_NET].apn))
    {
        /*set dns config in the first */
		#ifdef HOZON_DNS
		nm_dial_add_route(phndl);
		#endif
        //nm_dial_set_dns(phndl);
		
        #if 0
        /*add PRIVATE net DNS ip to route*/
        nm_dial_add_dns_route(phndl);
        #endif
    }

    return 0;
}

/****************************************************************
function:     nm_dial_connected
description:  do the things when data link is connected
input:        NET_TYPE type, public network or private network;
output:       none
return:       none
****************************************************************/
void nm_dial_connected(NET_TYPE type)
{
    int ret;
    NM_NET_INFO *info;

    info = nm_net_info + type;

    if (info->state != NM_NET_CONNECTING)
    {
        log_e(LOG_NM, "invalid status:%u", info->state);
        return;
    }

    if (DSI_IP_FAMILY_V4 == info->ip_type)
    {
        ret = nm_dial_get_conf(info);

        if (ret != 0)
        {
            nm_dial_restart();
            return;
        }
    }
    else if (DSI_IP_FAMILY_V6 == info->ip_type)
    {
        log_e(LOG_NM, "DSI_IP_FAMILY_V6 is not support,type:%u", type);
        return;
    }

    info->state = NM_NET_CONNECTED;

    ret = nm_dial_notify_changed(type, NM_REG_MSG_CONNECTED);

    if (0 != ret)
    {
        log_e(LOG_NM, "notify net status failed,type:%u", ret);
    }

    return;
}

/****************************************************************
function:     nm_dial_disconnected
description:  do the things when data link is disconnected
input:        NET_TYPE type, the type of network
output:       none
return:       none
****************************************************************/
void nm_dial_disconnected(NET_TYPE type)
{
    int ret;
    dsi_ce_reason_t reason;
    NM_NET_INFO *info;

    info = nm_net_info + type;

    if (NULL != info->handle)
    {
        if (dsi_get_call_end_reason(info->handle, &reason, info->ip_type) == DSI_SUCCESS)
        {
            log_e(LOG_NM, "dsi_get_call_end_reason type=%d reason code =%d time=%u", reason.reason_type,
                  reason.reason_code, (unsigned int)tm_get_time());
        }

        /* start recall timer */
        ret = tm_start(info->recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

        if (ret != 0)
        {
            log_e(LOG_NM, "tm_start resend timer failed, ret:0x%08x, type:%u", ret, type);
        }

        info->state = NM_NET_IDLE;
    }

    ret = nm_dial_notify_changed(type, NM_REG_MSG_DISCONNECTED);

    if (0 != ret)
    {
        log_e(LOG_NM, "notify net status failed,type:%u", ret);
    }

    return;
}


/****************************************************************
function:     nm_dial_status_msg_proc
description:  status changed message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
****************************************************************/
void nm_dial_status_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    int i;
    unsigned int call_id;
    NET_TYPE type;
    dsi_net_evt_t evt;

    if ((NULL == msgbody) || (msghdr->msglen < sizeof(NET_TYPE) + sizeof(dsi_net_evt_t)))
    {
        log_e(LOG_NM, "invalid status changed msg, msgbody:%p,msglen:%u", msgbody, msghdr->msglen);
        return;
    }

    call_id = *((unsigned int *)msgbody);
    evt  = *((dsi_net_evt_t *)(msgbody + sizeof(type)));

    for (i = 0; i < NM_NET_TYPE_NUM; i ++)
    {
        if (nm_net_info[i].call_id == call_id)
        {
            type = nm_net_info[i].type;
            break;
        }
    }

    if (i >= NM_NET_TYPE_NUM)
    {
        log_o(LOG_NM, "not for current data call, ignor");
        return;
    }

    log_o(LOG_NM, "type = %d,call_id = %d,evt = %d,enter%s", type, call_id, evt, __FUNCTION__);

    /* acquire successfully */
    if (DSI_EVT_NET_IS_CONN == evt)
    {
        nm_dial_connected(type);
    }
    else if (DSI_EVT_NET_NO_NET == evt)
    {
        nm_dial_disconnected(type);
    }
}

/****************************************************************
function:     nm_dial_wan_switch
description:  control public network on/off
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_dial_wan_switch(unsigned char *msgbody)
{
    int ret;
    NM_NET_INFO *info;
    info = nm_net_info + NM_PUBLIC_NET;

    if (1 == *msgbody)
    {
        log_o(LOG_NM, "NM_PRIVATE_NET sta = %d.", nm_net_info[NM_PUBLIC_NET].state);

        if (nm_net_info[NM_PUBLIC_NET].state != NM_NET_CONNECTED)
        {
            nm_dial_call(NM_PUBLIC_NET);
        }
    }
    else if (0 == *msgbody)
    {
        if (info->handle != NULL)
        {
            ret = dsi_stop_data_call(info->handle);

            if (ret != 0)
            {
                log_e(LOG_NM, "dsi_stop_data_call failed,ret:0x%08x", ret);
                return ret;
            }

            dsi_rel_data_srvc_hndl(info->handle);
            info->handle = NULL;
            log_o(LOG_NM, "dsi_stop_data_call ,apn is %s.", info->apn);
        }

        tm_stop(info->recall_timer);
        info->state = NM_NET_IDLE;
    }
    else
    {
        return NM_INVALID_PARA;
    }

    return 0;
}

/****************************************************************
function:     nm_dial_msg_proc
description:  innner message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
****************************************************************/
void nm_dial_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    int ret = 0;
    int retry_cnt = 0;
    ql_apn_info_list_s apn_list;

    if ((NM_MSG_ID_DIAL_STATUS == msghdr->msgid) && (MPU_MID_NM == msghdr->sender))
    {
        nm_dial_status_msg_proc(msghdr, msgbody);
    }
    else if ((NM_MSG_ID_PRIVATE_TIMER_RECALL == msghdr->msgid) && (MPU_MID_TIMER == msghdr->sender))
    {
        log_o(LOG_NM, "NM_MSG_ID_PRIVATE_TIMER_RECALL...");
        nm_dial_recall(NM_PRIVATE_NET);
    }
    else if ((NM_MSG_ID_PUBLIC_TIMER_RECALL == msghdr->msgid) && (MPU_MID_TIMER == msghdr->sender))
    {
        log_o(LOG_NM, "NM_MSG_ID_PUBLIC_TIMER_RECALL...");
        nm_dial_recall(NM_PUBLIC_NET);
    }
    else if ((NM_MSG_ID_LOC_AUTH_CHANGED == msghdr->msgid) && (MPU_MID_NM == msghdr->sender))
    {
        nm_dial_restart();
    }
	else if( ( NM_MSG_ID_RECALL == msghdr->msgid ) && (MPU_MID_NM == msghdr->sender) )
	{
		nm_dial_restart();
	}
    else if ((NM_MSG_ID_WAN_APN_CHANGED == msghdr->msgid) && (MPU_MID_NM == msghdr->sender))
    {
        int retry_cnt = 0;

        /* waitting for api service is ready */
        retry_cnt = 20;

        while (retry_cnt > 0)
        {
            memset(&apn_list, 0, sizeof(apn_list));
            ret = QL_APN_Get_Lists(&apn_list);

            if (ret > 8)
            {
                log_e(LOG_NM, "QL_APN_Get_Lists ret is %d.", ret);
            }

            if (ret > 0)
            {
                break;
            }

            retry_cnt --;
            usleep(500 * 1000);
        }

        if (ret < 0)
        {
            /* nerver happen */
            log_e(LOG_NM, "Unknow, failed to get apn list");
            return;
        }

        ret = net_apn_config(NM_PUBLIC_NET);

        if (ret != 0)
        {
            log_e(LOG_NM, "at change pub apn failed.");
        }

        nm_dial_restart();
    }
    else if ((NM_MSG_ID_LOCAL_APN_CHANGED == msghdr->msgid) && (MPU_MID_NM == msghdr->sender))
    {
        /* waitting for api service is ready */
        retry_cnt = 20;

        while (retry_cnt > 0)
        {
            memset(&apn_list, 0, sizeof(apn_list));
            ret = QL_APN_Get_Lists(&apn_list);

            if (ret > 8)
            {
                log_e(LOG_NM, "QL_APN_Get_Lists ret is %d.", ret);
            }

            if (ret > 0)
            {
                break;
            }

            retry_cnt --;
            usleep(500 * 1000);
        }

        if (ret < 0)
        {
            /* nerver happen */
            log_e(LOG_NM, "Unknow, failed to get apn list");
            return;
        }

        ret = net_apn_config(NM_PRIVATE_NET);

        if (ret != 0)
        {
            log_e(LOG_NM, "change pri apn failed.");
        }

        nm_dial_restart();
    }
    else if ((NM_MSG_ID_WAN_SWITCH == msghdr->msgid) && (MPU_MID_NM == msghdr->sender))
    {
        nm_dial_wan_switch(msgbody);
    }
    else
    {
        log_e(LOG_NM, "unknow msg id: %d", msghdr->msgid);
    }

    return;
}

/****************************************************************
function:     nm_dial_start
description:  start to dial data communciation
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_dial_start(void)
{
    nm_dial_call(NM_PRIVATE_NET);
    nm_dial_call(NM_PUBLIC_NET);

    return 0;
}

int nm_dial_stop(void)
{
    int i, ret;
    NM_NET_INFO *info;

    for (i = NM_PRIVATE_NET; i <= NM_PUBLIC_NET; i++)
    {
        info = nm_net_info + i;

        if ((info->handle != NULL) &&
            (info->apn[0] != 0))
        {
            log_e(LOG_NM, "nm_dial_stop,i:%d,dsi_stop_data_call.", i);
            ret = dsi_stop_data_call(info->handle);

            if (ret != 0)
            {
                log_e(LOG_NM, "dsi_stop_data_call failed,ret:0x%08x", ret);
            }

            dsi_rel_data_srvc_hndl(info->handle);
            info->handle = NULL;
        }

        tm_stop(info->recall_timer);
        info->state = NM_NET_IDLE;
    }

    return 0;
}


/****************************************************************
function:     nm_dial_restart
description:  redial network
input:        NET_TYPE type, public network or private network;
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_dial_restart(void)
{
    int i, ret;
    NM_NET_INFO *info;

    for (i = NM_PRIVATE_NET; i <= NM_PUBLIC_NET; i++)
    {
        info = nm_net_info + i;

        if ((info->handle != NULL) &&
            (info->apn[0] != 0))
        {
            log_e(LOG_NM, "nm_dial_restart,i:%d,dsi_stop_data_call.", i);
            ret = dsi_stop_data_call(info->handle);

            if (ret != 0)
            {
                log_e(LOG_NM, "dsi_stop_data_call failed,ret:0x%08x", ret);
            }

            dsi_rel_data_srvc_hndl(info->handle);
            info->handle = NULL;
        }

        tm_stop(info->recall_timer);
        info->state = NM_NET_IDLE;
    }

    nm_dial_start();
    return 0;
}

/****************************************************************
function:     nm_set_dcom
description:  set public network state
input:        unsigned char action
output:       none
return:       0 indicates successful;
              other indicates failed;
****************************************************************/
int nm_set_dcom(unsigned char action)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    ret = cfg_set_para(CFG_ITEM_DCOM_SET, &action, sizeof(char));

    if (0 != ret)
    {
        log_e(LOG_NM, "set dcom failed,ret:0x%08x", ret);
        return ret;
    }

    /*if public net is empty,set invalid*/
    pthread_mutex_lock(&nm_mutex);

    if (0 == strlen(nm_net_info[NM_PUBLIC_NET].apn))
    {
        pthread_mutex_unlock(&nm_mutex);
        return 0;
    }

    pthread_mutex_unlock(&nm_mutex);

    /* send message to the nm */
    msghdr.sender   = MPU_MID_NM;
    msghdr.receiver = MPU_MID_NM;
    msghdr.msgid    = NM_MSG_ID_WAN_SWITCH;
    msghdr.msglen   = sizeof(char);

    ret = tcom_send_msg(&msghdr, &action);

    if (ret != 0)
    {
        log_e(LOG_NM, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
              msghdr.msgid, msghdr.receiver, ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     nm_get_dcom
description:  get public network state
input:        unsigned char *state
output:       none
return:       1 indicates network is available;
              0 indicates network is unavailable
****************************************************************/
unsigned char nm_get_dcom(void)
{
    int ret;
    unsigned char state = 0;;
    unsigned int len = sizeof(unsigned char);

    ret = cfg_get_para(CFG_ITEM_DCOM_SET, &state, &len);

    if (0 != ret)
    {
        log_e(LOG_NM, "get dcom set state failed,ret:0x%08x", ret);
        return ret;
    }

    return state;
}

/****************************************************************
function:     nm_get_dial_info
description:  get network ip and interface
input:        NET_TYPE type
              NM_PRIVATE_DATA *data
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_get_dial_info(NET_TYPE type, NM_DIAL_INFO *data)
{
    NM_NET_INFO *info;

    if (type >= NM_NET_TYPE_NUM)
    {
        log_e(LOG_NM, "net type error, type:%d", type);
        return NM_INVALID_PARA;
    }

    pthread_mutex_lock(&nm_mutex);
    info = nm_net_info + type;

    if (NULL == data)
    {
        log_e(LOG_NM, "para error:data is NULL");
        pthread_mutex_unlock(&nm_mutex);
        return NM_INVALID_PARA;
    }

    if (NM_NET_CONNECTED == info->state)
    {
        memcpy(&data->ip_addr, &info->ip_addr, sizeof(struct in_addr));
        memcpy(data->interface.ifr_ifrn.ifrn_name, info->interface, sizeof(info->interface));
        pthread_mutex_unlock(&nm_mutex);
        return 0;
    }

    pthread_mutex_unlock(&nm_mutex);
    log_o(LOG_NM, "the data link is disconnnected,status:%u", nm_net_info[type].state);
    return NM_STATUS_INVALID;
}

/****************************************************************
function:     nm_set_net
description:  bind the different sock to different interface
input:        int sockfd
              NET_TYPE type
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_set_net(int sockfd, NET_TYPE type)
{
    int ret;
    NM_NET_INFO *info;

    if ((type >= NM_NET_TYPE_NUM) || (sockfd < 0))
    {
        log_e(LOG_NM, "para error,sockfd:%d, type:%d", sockfd, type);
        return NM_INVALID_PARA;
    }

    pthread_mutex_lock(&nm_mutex);
    info = nm_net_info + type;

    if (NM_NET_CONNECTED == info->state)
    {
        ret = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, info->interface, sizeof(info->interface));

        if (ret < 0)
        {
            log_e(LOG_NM, "set local interface failed,error:%s", strerror(errno));
            pthread_mutex_unlock(&nm_mutex);
            return ret;
        }

        pthread_mutex_unlock(&nm_mutex);
        return 0;
    }

    pthread_mutex_unlock(&nm_mutex);
    log_o(LOG_NM, "the data link is disconnnected,status:%u", nm_net_info[type].state);
    return NM_STATUS_INVALID;

}

/****************************************************************
function:     nm_network_switch
description:  0x00:auto, 0x01:2G, 0x02:3G, 0x03:4G
input:        unsigned char type
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_network_switch(unsigned char type)
{
    int ret;
    ret = at_network_switch(type);
    return ret;
}

/****************************************************************
function:     nm_get_signal
description:  get the signal of 4G module
input:        none
output:       none
return:       the signal value of 4G module
*****************************************************************/
int nm_get_signal(void)
{
    return  at_get_signal();
}

/****************************************************************
function:     nm_get_iccid
description:  get the ccid of sim
input:        none
output:       char *iccid
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_get_iccid(char *iccid)
{
    int ret;
    ret =  at_get_iccid(iccid);
    return ret;
}

/****************************************************************
function:     nm_get_net_type
description:  get network type
input:        none
output:       none
return:       the net type of 4G module
*****************************************************************/
int nm_get_net_type(void)
{
    return at_get_net_type();
}

/****************************************************************
function:     nm_get_operator
description:  get network operator
input:        none
output:       unsigned char *operator
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int nm_get_operator(unsigned char *operator)
{
    int ret;
    ret = at_get_operator(operator);
    return ret;
}

/****************************************************************
function:     nm_get_net_status
description:  get network status
input:        none
return:       true indicates  CONNECTED;
              false indicates DISCONNECTED
*****************************************************************/
bool nm_get_net_status(void)
{
    pthread_mutex_lock(&nm_mutex);

    if (NM_NET_CONNECTED == nm_net_info[NM_PRIVATE_NET].state)
    {
        pthread_mutex_unlock(&nm_mutex);
        return true;
    }
    else
    {
        pthread_mutex_unlock(&nm_mutex);
        return false;
    }
}

/****************************************************************
function:     nm_net_is_apn_valid
description:  get network status
input:        NET_TYPE type
return:       true indicates  the apn is not null;
              false indicates the apn is null;
*****************************************************************/
bool nm_net_is_apn_valid(NET_TYPE type)
{
    pthread_mutex_lock(&nm_mutex);

    if (0 == strlen(nm_net_info[type].apn))
    {
        pthread_mutex_unlock(&nm_mutex);
        return false;
    }
    else
    {
        pthread_mutex_unlock(&nm_mutex);
        return true;
    }
}

/****************************************************************
function:     nm_get_net_status_ex
description:  get network status
input:        NET_TYPE type
return:       true indicates  CONNECTED;
              false indicates DISCONNECTED
*****************************************************************/
bool nm_get_net_status_ex(NET_TYPE type)
{
    pthread_mutex_lock(&nm_mutex);

    if (NM_NET_CONNECTED == nm_net_info[type].state)
    {
        pthread_mutex_unlock(&nm_mutex);
        return true;
    }
    else
    {
        pthread_mutex_unlock(&nm_mutex);
        return false;
    }
}

/****************************************************************
function:     nm_register_status_changed
description:  if network status is changed,notify callback
input:        nm_status_changed callback
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_register_status_changed(nm_status_changed callback)
{
    /* the paramerter is invalid */
    if (NULL == callback)
    {
        log_e(LOG_NM, "callback is NULL");
        return NM_INVALID_PARA;
    }

    pthread_mutex_lock(&nm_regtbl_mutex);

    if (nm_tbl.used_num >= NM_MAX_REG_TBL)
    {
        pthread_mutex_unlock(&nm_regtbl_mutex);
        log_e(LOG_NM, "nm register table overflow");
        return NM_TABLE_OVERFLOW;
    }

    nm_tbl.changed[nm_tbl.used_num] = callback;
    nm_tbl.used_num++;
    pthread_mutex_unlock(&nm_regtbl_mutex);
    return 0;
}
void nm_get_dns_ip(char *dns)
{
	strcpy(dns,inet_ntoa(nm_net_info[0].pri_dns_addr));
}

void nm_dial_add_ip_route(char *ip)
{
   char command[200];
    memset(command, 0, sizeof(command));

    /*add defaut route as the public route*/
    snprintf(command, sizeof(command), "ip route add %s via %s dev %s", ip,inet_ntoa(nm_net_info[0].gw_addr),
             nm_net_info[0].interface);
    nm_sys_call(command);

    nm_dcom_status = 1;
}

