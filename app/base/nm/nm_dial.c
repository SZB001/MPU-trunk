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
#include "nm_dial.h"

static pthread_mutex_t nm_regtbl_mutex;
static NM_REG_TBL      nm_tbl;
static NM_NET_INFO     nm_net_info;
static pthread_mutex_t nm_mutex;

/*******************************************************************************
function:     nm_sys_call
description:  call ds_system_call
input:        const char *cmd;
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************************/
static int nm_sys_call(const char *cmd)
{
    int ret, i = 0;

    log_o(LOG_NM, "%s", cmd);

    while (i < 3)
    {
        ret = ds_system_call(cmd, strlen(cmd));

        if (ret != 0)
        {
            log_e(LOG_NM, "ds_system_call failed, ret:%u", ret);
            i++;
        }
        else
        {
            break;
        }
    }

    return ret;
}

/*******************************************************************************
function:     nm_notify_changed
description:  if network status is changed,this function will be called
input:        type, network type;
              status, network status;
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************************/
static int  nm_notify_changed(NET_TYPE type, NM_STATE_MSG status)
{
    int i, ret;

    for (i = 0; i < nm_tbl.used_num; i++)
    {
        if (nm_tbl.item[i].type == type)
        {
            ret = nm_tbl.item[i].changed(type, status);

            if (ret != 0)
            {
                log_e(LOG_NM, "send net changed msg failed,ret:%d,type:%u", ret, type);
                return NM_SEND_MSG_FAILED;
            }
        }
    }

    return 0;
}

/*************************************************************************************
function:     nm_dcom_changed
description:  when dcom is set,this function will be called
input:        CFG_PARA_ITEM_ID id, cfg item id;
              unsigned char *old_para, old para value;
              unsigned char *new_para, new para value;
              unsigned int len, defined para length
output:       none
return:       0 indicates success;
              others indicates failed
*************************************************************************************/
static int nm_dcom_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                             unsigned char *new_para, unsigned int len)
{
    int ret;
    unsigned char dcom;
    TCOM_MSG_HEADER msghdr;

    if (CFG_ITEM_DCOM_SET != id)
    {
        log_e(LOG_NM, "invalid id, id:%u", id);
        return NM_INVALID_PARA;
    }

    dcom = *new_para;
    
    msghdr.sender    = MPU_MID_NM;
    msghdr.receiver  = MPU_MID_NM;
    msghdr.msgid     = NM_MSG_ID_DCOM_CHANGED;
    msghdr.msglen    = sizeof(dcom);

    ret = tcom_send_msg(&msghdr, &dcom);
    if (ret != 0)
    {
        log_e(LOG_NM, "tcom_send_msg failed, ret:%u", ret);
        return NM_SEND_MSG_FAILED;
    }

    return 0;
}


/*************************************************************************************
function:     nm_loc_apn_changed
description:  when set private apn,this function will be called
input:        CFG_PARA_ITEM_ID id, cfg item id;
              unsigned char *old_para, old para value;
              unsigned char *new_para, new para value;
              unsigned int len, defined para length
output:       none
return:       0 indicates success;
              others indicates failed
*************************************************************************************/
static int nm_loc_apn_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
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
        log_o(LOG_NM, "nm_net_local_apn_changed,there is no change!");
        return 0;
    }

    log_o(LOG_NM, "nm_net_local_apn_changed,old apn:%s,new apn:%s", old_para, new_para);

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

/************************************************************************************
function:     nm_wan_apn_changed
description:  when set the wan apn, this function will be called
input:        CFG_PARA_ITEM_ID id, cfg item id;
              unsigned char *old_para, old para value;
              unsigned char *new_para, new para value;
              unsigned int len, defined para length
output:       none
return:       0 indicates success;
              others indicates failed
************************************************************************************/
static int nm_wan_apn_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
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
function:     nm_loc_auth_changed
description:  when set the configuration, this function will be called
input:        CFG_PARA_ITEM_ID id, cfg item id;
              unsigned char *old_para, old para value;
              unsigned char *new_para, new para value;
              unsigned int len, defined para length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int nm_loc_auth_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                               unsigned char *new_para, unsigned int len)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    if ((CFG_ITEM_LOC_APN_AUTH != id) || (len != sizeof(CFG_DIAL_AUTH)))
    {
        log_e(LOG_NM, "invalid id, id:%u, len:%u", id, len);
        return NM_INVALID_PARA;
    }

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
function:     nm_dial_init_loc
description:  init private network
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int nm_dial_init_loc(void)
{
    int ret;
    unsigned int  len;
    NM_NET_ITEM   *net_item;
    CFG_DIAL_AUTH auth;

    /* get the APN and auth para of private network */
    net_item        = nm_net_info.item + NM_PRIVATE_NET;
    net_item->type  = NM_PRIVATE_NET;
    net_item->state = NM_NET_DISCONNECTED;

    len = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_LOC_APN_AUTH, &auth, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return ret;
    }

    if ((0 != strlen(auth.user)) && (0 != strlen(auth.pwd)))
    {
        memcpy(net_item->user, auth.user, strlen(auth.user));
        memcpy(net_item->pwd, auth.pwd, strlen(auth.pwd));
    }

    ret = cfg_register(CFG_ITEM_LOC_APN_AUTH, nm_loc_auth_changed);

    if (ret != 0)
    {
        log_e(LOG_NM, "reg apn auth changed callback failed,ret:0x%08x", ret);
        return ret;
    }

    len = sizeof(net_item->apn);
    ret = cfg_get_para(CFG_ITEM_LOCAL_APN, (unsigned char *)net_item->apn, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get local apn failed, ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_LOCAL_APN, nm_loc_apn_changed);

    if (ret != 0)
    {
        log_e(LOG_NM, "reg local apn changed callback failed,ret:0x%08x", ret);
        return ret;
    }
    
    return 0;
}

/****************************************************************
function:     nm_dial_init_wan
description:  init private network
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int nm_dial_init_wan(void)
{
    int ret;
    unsigned int  len;
    NM_NET_ITEM   *net_item;

    /* get the APN of public network */
    net_item = nm_net_info.item + NM_PUBLIC_NET;
    net_item->type  = NM_PUBLIC_NET;
    net_item->state = NM_NET_DISCONNECTED;

    len = sizeof(net_item->apn);
    ret = cfg_get_para(CFG_ITEM_WAN_APN, (unsigned char *)net_item->apn, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get wan apn failed, ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_WAN_APN, nm_wan_apn_changed);

    if (ret != 0)
    {
        log_e(LOG_NM, "reg wan apn changed callback failed,ret:0x%08x", ret);
        return ret;
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

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&nm_mutex, NULL);
            pthread_mutex_init(&nm_regtbl_mutex, NULL);
            memset(&nm_tbl, 0, sizeof(nm_tbl));
            memset(&nm_net_info, 0x00, sizeof(nm_net_info));

            nm_net_info.init_state = NM_NET_NOT_INITED;

            for (i = 0; i < NM_NET_TYPE_NUM; i++)
            {
                nm_net_info.item[i].cdma_index = 1;
                nm_net_info.item[i].umts_index = 1;
                nm_net_info.item[i].ip_ver     = DSI_IP_VERSION_4;
                nm_net_info.item[i].auth_pref  = DSI_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
                nm_net_info.item[i].state      = NM_NET_DISCONNECTED;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret  = nm_dial_init_loc();
            ret |= nm_dial_init_wan();

            if (ret != 0)
            {
                log_e(LOG_NM, "init networkr failed, ret:0x%08x", ret);
                return ret;
            }
            
            ret = cfg_register(CFG_ITEM_DCOM_SET, nm_dcom_changed);
            
            if (ret != 0)
            {
                log_e(LOG_NM, "reg dcom changed callback failed,ret:0x%08x", ret);
                return ret;
            }

            /* create dial timer */
            nm_net_info.timername = NM_MSG_ID_TIMER_RECALL;
            ret = tm_create(TIMER_REL, nm_net_info.timername, MPU_MID_NM, &(nm_net_info.recall_timer));

            if (ret != 0)
            {
                log_e(LOG_NM, "tm_create reconnect private network timer failed, ret:0x%08x", ret);
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
static void nm_dial_init_cb_fun(void *user_data)
{
    int ret;
    NET_TYPE type;
    TCOM_MSG_HEADER msghdr;

    type = *((NET_TYPE *)user_data);

    log_o(LOG_NM, "network init successfully, dial net(%u) first", type);

    /* send message to the nm */
    msghdr.sender   = MPU_MID_NM;
    msghdr.receiver = MPU_MID_NM;
    msghdr.msgid    = NM_MSG_ID_DIAL_INITED;
    msghdr.msglen   = sizeof(type);

    ret = tcom_send_msg(&msghdr, (unsigned char *)&type);

    if (ret != 0)
    {
        log_e(LOG_NM, "send message(msgid:%u) to moudle(0x%04x) failed, type:%u",
              msghdr.msgid, msghdr.receiver, type);
    }
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
    NET_TYPE type;
    unsigned char buf[32];
    TCOM_MSG_HEADER msghdr;

    log_o(LOG_NM, "hndl=%p,evt=%d, payload_ptr=%p", handle, evt, payload_ptr);

    type = *((NET_TYPE *)user_data);

    memcpy(buf, &type, sizeof(type));
    size += sizeof(type);

    memcpy(buf + size, &evt, sizeof(evt));
    size += sizeof(evt);

    if (DSI_EVT_WDS_CONNECTED == evt)
    {
        memcpy(buf + size, &payload_ptr->ip_type, sizeof(payload_ptr->ip_type));
        size += sizeof(payload_ptr->ip_type);
    }

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
    NM_NET_ITEM *item;

    item = nm_net_info.item + type;

    /* set data call param */
    param_info.buf_val = NULL;
    param_info.num_val = DSI_RADIO_TECH_UNKNOWN;
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_TECH_PREF, &param_info);

    param_info.buf_val = NULL;
    param_info.num_val = item->umts_index;
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_UMTS_PROFILE_IDX, &param_info);

    param_info.buf_val = NULL;
    param_info.num_val = item->cdma_index;
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_CDMA_PROFILE_IDX, &param_info);

    param_info.buf_val = NULL;
    param_info.num_val = item->ip_ver;
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_IP_VERSION, &param_info);

    param_info.buf_val = strdup(item->apn);
    param_info.num_val = strlen(param_info.buf_val);
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_APN_NAME, &param_info);
    log_o(LOG_NM, "apn=%s,apn len=%u", param_info.buf_val, param_info.num_val);

    free(param_info.buf_val);
    param_info.buf_val = NULL;

    param_info.buf_val = strdup(item->user);
    param_info.num_val = strlen(param_info.buf_val);
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_USERNAME, &param_info);
    free(param_info.buf_val);
    param_info.buf_val = NULL;

    param_info.buf_val = strdup(item->pwd);
    param_info.num_val = strlen(param_info.buf_val);
    dsi_set_data_call_param(item->handle, DSI_CALL_INFO_PASSWORD, &param_info);
    free(param_info.buf_val);

    if ((0 != strlen(item->user)) && (0 != strlen(item->pwd)))
    {
        item->auth_pref = DSI_AUTH_PREF_CHAP_ONLY_ALLOWED;
        param_info.buf_val = NULL;
        param_info.num_val = item->auth_pref;
        dsi_set_data_call_param(item->handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
    }
    else
    {
        param_info.buf_val = NULL;
        param_info.num_val = item->auth_pref;
        dsi_set_data_call_param(item->handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
    }
}

/****************************************************************
function:     nm_dial_add_default_route
description:  add ip as default route
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_add_default_route(NM_NET_ITEM *phndl)
{
    char command[200];
    memset(command, 0, sizeof(command));

    /*del defaut route from route*/
    snprintf(command, sizeof(command), "ip route del default");
    nm_sys_call(command);

    /*add defaut route as the public route*/
    snprintf(command, sizeof(command), "ip route add default via %s dev %s", inet_ntoa(phndl->gw_addr),
             phndl->interface);
    nm_sys_call(command);
}

/****************************************************************
function:     nm_dial_del_default_route
description:  add ip as default route
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_del_default_route(void)
{
    char command[200];
    memset(command, 0, sizeof(command));

    /*del defaut route from route*/
    snprintf(command, sizeof(command), "ip route del default");
    nm_sys_call(command);
}

/****************************************************************
function:     nm_dial_add_dns_route
description:  add the dns ip to route list
input:        nm_NET_INFO *phndl
output:       none
return:       none
****************************************************************/
static void nm_dial_add_dns_route(NM_NET_ITEM *phndl)
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
	#if 0
    else
    {
        len = snprintf(command, sizeof(command), "ip route add %s via ", "8.8.8.8");
        snprintf(command + len, sizeof(command) - len , "%s dev %s", inet_ntoa(phndl->gw_addr),
                 phndl->interface);
        nm_sys_call(command);
    }
	#endif
}

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
}

/****************************************************************
function:     nm_dial_set_dns
description:  set dns configuration file
input:        nm_NET_INFO *phndl;
output:       none
return:       none
****************************************************************/
static void nm_dial_set_dns(NM_NET_ITEM *phndl)
{
    char command[200];

    memset(command, 0, sizeof(command));

    if (phndl->pri_dns_addr.s_addr)
    {
        snprintf(command, sizeof(command), "echo 'nameserver %s' > /etc/resolv.conf",
                 inet_ntoa(phndl->pri_dns_addr));
        nm_sys_call(command);

        if (phndl->sec_dns_addr.s_addr)
        {
            snprintf(command, sizeof(command), "echo 'nameserver %s' >> /etc/resolv.conf",
                     inet_ntoa(phndl->sec_dns_addr));
            nm_sys_call(command);
        }
    }
    else
    {
        snprintf(command, sizeof(command), "echo 'nameserver 8.8.8.8' > /etc/resolv.conf");
        nm_sys_call(command);
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
static int nm_dial_get_conf(NM_NET_ITEM *phndl)
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

    log_o(LOG_NM, "public_ip: %s",    inet_ntoa(phndl->ip_addr));
    log_o(LOG_NM, "gw_addr: %s",      inet_ntoa(phndl->gw_addr));
    log_o(LOG_NM, "pri_dns_addr: %s", inet_ntoa(phndl->pri_dns_addr));
    log_o(LOG_NM, "sec_dns_addr: %s", inet_ntoa(phndl->sec_dns_addr));

    /* if only one APN is configured or public networks is connected, set the router and dns */
    if ((NM_PUBLIC_NET == phndl->type) ||
        (NM_PRIVATE_NET == phndl->type && 0 == strlen(nm_net_info.item[NM_PUBLIC_NET].apn)))
    {
        char dcom = 1;
        unsigned int len = sizeof(dcom);
        
        if(0 != cfg_get_para(CFG_ITEM_DCOM_SET, &dcom, &len))
        {
           log_e(LOG_NM, "get dcom status failed"); 
        }
        if(dcom)
        {
            /*add defaut route used for the public net*/
            nm_dial_add_default_route(phndl);
        }
        /*add iptables rules*/
        nm_dial_set_iptable(iface);

        /*set DNS config file*/
        nm_dial_set_dns(phndl);

        nm_dial_add_dns_route(phndl);
    }

    return 0;
}

/****************************************************************
function:     nm_dial_wds_connected
description:  call data link
input:        NET_TYPE type, public network or private network;
              TCOM_MSG_HEADER *msghdr, msg header;
              unsigned char *msgbody, msg data;
output:       none
return:       none
****************************************************************/
void nm_dial_wds_connected(NET_TYPE type, TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    if (NULL != msgbody && msghdr->msglen > sizeof(dsi_net_evt_t) + sizeof(type))
    {
        nm_net_info.item[type].ip_type = *((dsi_ip_family_t *)(msgbody + sizeof(dsi_net_evt_t) + sizeof(type)));
    }
}

/****************************************************************
function:     nm_dial_get_next_dial_net
description:  get next network need to dial
input:        NET_TYPE type, public network or private network;
output:       none
return:       none
****************************************************************/
NET_TYPE nm_dial_get_next_dial_net(NET_TYPE type)
{
    NM_NET_ITEM *item;
    NET_TYPE next;

    if (type >= NM_NET_TYPE_NUM)
    {
        type =  NM_PRIVATE_NET;
    }

    next = (type + 1) % NM_NET_TYPE_NUM;

    while (next != type)
    {
        item = nm_net_info.item + next;

        if ((NM_NET_CONNECTED != item->state) && (0 != strlen(item->apn)))
        {
            return next;
        }

        next = (next + 1) % NM_NET_TYPE_NUM;
    }

    if ((NM_NET_CONNECTED != nm_net_info.item[type].state)
        && (0 != strlen(nm_net_info.item[type].apn)))
    {
        return type;
    }

    return NM_NET_TYPE_NUM;
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
    NM_NET_ITEM *item;
    NET_TYPE next;

    item = nm_net_info.item + type;

    if (item->state != NM_NET_CONNECTING)
    {
        log_e(LOG_NM, "invalid status:%u", item->state);
        return;
    }

    if (DSI_IP_FAMILY_V4 == item->ip_type)
    {
        ret = nm_dial_get_conf(item);

        if (ret != 0)
        {
            nm_dial_call(type);
            return;
        }
    }
    else if (DSI_IP_FAMILY_V6 == item->ip_type)
    {
        log_e(LOG_NM, "DSI_IP_FAMILY_V6 is not support,type:%u", type);
        nm_dial_call(type);
        return;
    }

    item->state = NM_NET_CONNECTED;
    log_o(LOG_NM, "network(%u) is connected", type);

    ret = nm_notify_changed(type, NM_REG_MSG_CONNECTED);

    if (0 != ret)
    {
        log_e(LOG_NM, "notify net status failed,type:%u", ret);
    }

    next = nm_dial_get_next_dial_net(type);

    if (next < NM_NET_TYPE_NUM)
    {
        nm_dial_call(next);
    }
    else  /* all networks is connected */
    {
        log_o(LOG_NM, "all networks is connected");
        nm_net_info.cur_type = NM_NET_TYPE_NUM;
    }
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
    NM_NET_ITEM *item;

    if (NM_NET_NOT_INITED == nm_net_info.init_state)
    {
        nm_dial_all_net();
        return;
    }

    item = nm_net_info.item + type;

    log_o(LOG_NM, "network(%u) is disconnected", type);

    if (NULL != item->handle)
    {
        if (dsi_get_call_end_reason(item->handle, &reason, item->ip_type) == DSI_SUCCESS)
        {
            log_e(LOG_NM, "dsi_get_call_end_reason type=%d reason code =%d", reason.reason_type,
                  reason.reason_code);
        }
    }

    if (NM_NET_CONNECTED == item->state)
    {
        item->state = NM_NET_DISCONNECTED;
        ret = nm_notify_changed(type, NM_REG_MSG_DISCONNECTED);

        if (0 != ret)
        {
            log_e(LOG_NM, "notify net status failed,type:%u", ret);
        }
    }
    else
    {
        item->state = NM_NET_DISCONNECTED;
    }

    /* start recall timer */
    ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

    if (ret != 0)
    {
        log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x", type, ret);
        return;
    }
}

/****************************************************************
function:     nm_dial_init_msg_proc
description:  init result message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
****************************************************************/
void nm_dial_init_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    NET_TYPE next, type;
    NM_NET_ITEM *item;

    if ((NULL == msgbody) || (msghdr->msglen < sizeof(NET_TYPE)))
    {
        log_e(LOG_NM, "invalid init msg, msgbody:%p,msglen:%u", msgbody, msghdr->msglen);
        return;
    }

    nm_net_info.init_state = NM_NET_INITED;

    type = *((NET_TYPE *)msgbody);
    item = nm_net_info.item + type;

    if (0 != strlen(item->apn))
    {
        nm_dial_call(type);
    }
    else
    {
        next = nm_dial_get_next_dial_net(type);

        if (next < NM_NET_TYPE_NUM)
        {
            nm_dial_call(next);
        }
        else
        {
            log_o(LOG_NM, "all networks is connected");
            nm_net_info.cur_type = NM_NET_TYPE_NUM;
        }
    }
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
    NET_TYPE type;
    dsi_net_evt_t evt;

    if ((NULL == msgbody) || (msghdr->msglen < sizeof(NET_TYPE) + sizeof(dsi_net_evt_t)))
    {
        log_e(LOG_NM, "invalid status changed msg, msgbody:%p,msglen:%u", msgbody, msghdr->msglen);
        return;
    }

    type = *((NET_TYPE *)msgbody);
    evt  = *((dsi_net_evt_t *)(msgbody + sizeof(type)));

    /* acquire successfully */
    if (DSI_EVT_WDS_CONNECTED == evt)
    {
        nm_dial_wds_connected(type, msghdr, msgbody);
    }
    else if (DSI_EVT_NET_IS_CONN == evt)
    {
        nm_dial_connected(type);
    }
    else if (DSI_EVT_NET_NO_NET == evt)
    {
        nm_dial_disconnected(type);
    }
}

/******************************************************************************
function:     nm_dial_loc_net_proc
description:  innner message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
*******************************************************************************/
void nm_dial_loc_net_proc(void)
{
    int ret;
    unsigned int  len;
    NM_NET_ITEM *net_item = NULL;
    CFG_DIAL_AUTH auth;

    /* update the APN of private network */
    net_item = nm_net_info.item + NM_PRIVATE_NET;
    net_item->type = NM_PRIVATE_NET;

    len = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_LOC_APN_AUTH, &auth, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return;
    }

    if ((0 != strlen(auth.user)) && (0 != strlen(auth.pwd)))
    {
        memcpy(net_item->user, auth.user, strlen(auth.user));
        memcpy(net_item->pwd, auth.pwd, strlen(auth.pwd));
    }

    len = sizeof(net_item->apn);
    ret = cfg_get_para(CFG_ITEM_LOCAL_APN, (unsigned char *)net_item->apn, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get local apn failed, ret:0x%08x", ret);
        return;
    }

    nm_dial_restart();
}

/******************************************************************************
function:     nm_dial_wan_net_proc
description:  innner message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
*******************************************************************************/
void nm_dial_wan_net_proc()
{
    int ret;
    unsigned int  len;
    NM_NET_ITEM *net_item = NULL;

    /* update the APN of public network */
    net_item = nm_net_info.item + NM_PUBLIC_NET;
    net_item->type = NM_PUBLIC_NET;

    len = sizeof(net_item->apn);
    ret = cfg_get_para(CFG_ITEM_WAN_APN, (unsigned char *)net_item->apn, &len);

    if (ret != 0)
    {
        log_e(LOG_NM, "get wan apn failed, ret:0x%08x", ret);
        return;
    }

    nm_dial_restart();
}

/****************************************************************
function:     nm_dial_timer_msg_proc
description:  status changed message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
****************************************************************/
void nm_dial_timer_msg_proc()
{
    int ret;
	NET_TYPE next;

    if (NM_NET_NOT_INITED == nm_net_info.init_state)
    {
        nm_dial_all_net();
        return;
    }
    else
    {
        if( nm_net_info.cur_type < NM_NET_TYPE_NUM )
        {
        	log_i(LOG_NM, "wait connection result,%u,%u,%u", 
			  	  nm_net_info.item[nm_net_info.cur_type].wait_cnt, 
			  	  nm_net_info.cur_type,
                  nm_net_info.item[nm_net_info.cur_type].state);

			
			/* start recall timer */
			ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

			if (ret != 0)
			{
				log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x",
					  nm_net_info.cur_type, ret);
			}

		    if( (NM_NET_CONNECTING == nm_net_info.item[nm_net_info.cur_type].state) 
				&& nm_net_info.item[nm_net_info.cur_type].wait_cnt < NM_MAX_DIAL_TIMES )
		    {
		    	nm_net_info.item[nm_net_info.cur_type].wait_cnt++;
	            return;
		    }

			if( NM_NET_CONNECTING == nm_net_info.item[nm_net_info.cur_type].state )
	        {
	        	nm_dial_stop( nm_net_info.cur_type );
				nm_net_info.item[nm_net_info.cur_type].state = NM_NET_DISCONNECTED;
				return;
	        }
		}

        next = nm_dial_get_next_dial_net(nm_net_info.cur_type);

        if (next < NM_NET_TYPE_NUM)
        {
            nm_dial_call(next);
        }
        else
        {
            log_o(LOG_NM, "all networks is connected");
            nm_net_info.cur_type = NM_NET_TYPE_NUM;
        }
    }
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
    switch (msghdr->msgid)
    {
        case NM_MSG_ID_DIAL_INITED:
            nm_dial_init_msg_proc(msghdr, msgbody);
            break;

        case NM_MSG_ID_DIAL_STATUS:
            nm_dial_status_msg_proc(msghdr, msgbody);
            break;

        case NM_MSG_ID_TIMER_RECALL:
            nm_dial_timer_msg_proc();
            break;

        case NM_MSG_ID_LOC_AUTH_CHANGED:
        case NM_MSG_ID_LOCAL_APN_CHANGED:
            nm_dial_loc_net_proc();
            break;

        case NM_MSG_ID_WAN_APN_CHANGED:
            nm_dial_wan_net_proc();
            break;
            
        case NM_MSG_ID_DCOM_CHANGED:
            nm_set_dcom(msgbody[0]);
            break;
            
        default:
            log_e(LOG_NM, "unknow msg id: %d", msghdr->msgid);
            break;
    }
}

/****************************************************************
function:     nm_dial_stop
description:  stop data call
input:        NET_TYPE type, public network or private network
output:       none
return:       none
****************************************************************/
void nm_dial_stop(NET_TYPE type)
{
    int ret;
    NM_NET_ITEM *item;

    item = nm_net_info.item + type;

    if (item->handle != NULL)
    {
        if (NM_NET_CONNECTED == item->state)
        {
            ret = dsi_stop_data_call(item->handle);

            if (ret != 0)
            {
                log_e(LOG_NM, "dsi_stop_data_call failed,ret:0x%08x", ret);
            }
        }

        dsi_rel_data_srvc_hndl(item->handle);
        item->handle = NULL;
    }

    if (NM_NET_CONNECTED == item->state)
    {
        item->state = NM_NET_DISCONNECTED;
        log_o(LOG_NM, "network(%u) is disconnected", type);
        ret = nm_notify_changed(type, NM_REG_MSG_DISCONNECTED);

        if (0 != ret)
        {
            log_e(LOG_NM, "notify net status failed,type:%u", ret);
        }
    }
    else
    {
        item->state = NM_NET_DISCONNECTED;
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
    int ret;
    NM_NET_ITEM *item;

    /* start recall timer */
    ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

    if (ret != 0)
    {
        log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x", type, ret);
        return;
    }

    item = nm_net_info.item + type;
	item->wait_cnt = 0;

    if (0 == strlen(item->apn))
    {
        return;
    }
	
	if (NM_NET_INITED != nm_net_info.init_state)
	{
		return;
	}

    if (NULL != item->handle)
    {
        nm_dial_stop(type);
		return;
    }

    /* acquire the handle. */
    item->handle = dsi_get_data_srvc_hndl(nm_dial_status_cb_fun, &(item->type));

    if (NULL == item->handle)
    {
        log_e(LOG_NM, "dsi_get_data_srvc_hndl fail,type:%u", type);
        return;
    }

    nm_dial_para_set(type);

    log_i(LOG_NM, "start data call, handle:%p,type:%u", item->handle, type);
    /* connecting WWAN */
    ret = dsi_start_data_call(item->handle);

    if (DSI_SUCCESS != ret)
    {
        log_e(LOG_NM, "dsi_start_data_call, type:%u,ret:0x%08x\n", type, ret);
    }
    else
    {
        item->state = NM_NET_CONNECTING;
		nm_net_info.cur_type = type;
    }
}

/****************************************************************
function:     nm_dial_restart
description:  start to dial data communciation
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
void nm_dial_restart(void)
{
    int i,ret;

     /* start recall timer */
    ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

    if (ret != 0)
    {
        log_e(LOG_NM, "tm_start resend timer failed, ret:0x%08x", ret);
        return;
    }

    for (i = NM_PRIVATE_NET; i < NM_NET_TYPE_NUM; i++)
    {
        nm_dial_stop(i);
    }

	nm_net_info.cur_type = NM_NET_TYPE_NUM;

    #if 0
	int ret;
    ret = dsi_release(DSI_MODE_GENERAL);

    if (ret != 0)
    {
        log_e(LOG_NM, "dsi_release failed,ret:0x%08x", ret);
    }

	nm_net_info.init_state = NM_NET_NOT_INITED;
	#endif

    //nm_dial_all_net();
}

/****************************************************************
function:     nm_dial_start
description:  start to dial data communciation
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_dial_all_net(void)
{
    int type, ret = 0;
    NM_NET_ITEM *item;

    for (type = 0; type < NM_NET_TYPE_NUM; type++)
    {
        item = nm_net_info.item + type;

        if (0 != strlen(item->apn))
        {
            break;
        }
    }

    if (type >= NM_NET_TYPE_NUM)
    {
        log_e(LOG_NM, "all apn is null");
        return NM_APN_INVALID;
    }

    log_o(LOG_NM, "dial network, type:%u, apn:%s", type, item->apn);

    if( NM_NET_NOT_INITED == nm_net_info.init_state )
	{
	    nm_net_info.init_state = NM_NET_INITING;

	    ret = dsi_init_ex(DSI_MODE_GENERAL, nm_dial_init_cb_fun, &(item->type));

	    if (DSI_SUCCESS != ret)
	    {
	        log_e(LOG_NM, "dsi_init_ex failed,ret:0x%08x,type:%u", ret, type);
	        nm_net_info.init_state = NM_NET_NOT_INITED;

	        /* start recall timer and retry to reinit when the timer timeout */
	        ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

	        if (ret != 0)
	        {
	            log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x", type, ret);
	        }
	    }
	}
	else if( NM_NET_INITING == nm_net_info.init_state ) 
	{
		/* start recall timer and retry to reinit when the timer timeout */
        ret = tm_start(nm_net_info.recall_timer, NM_RECALL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

        if (ret != 0)
        {
            log_e(LOG_NM, "tm_start resend timer failed, type:%u,ret:0x%08x", type, ret);
        }	
    }
	else
	{
		nm_dial_call(type);
	}

    return ret;
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
    NM_NET_ITEM *phndl = nm_net_info.item + NM_PUBLIC_NET;

    if(action == 0)
    {
        nm_dial_del_default_route();
    }
    else if(action > 0)
    {
        nm_dial_add_default_route(phndl);
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
int nm_get_dcom(unsigned char *state)
{
    int ret;
    unsigned int len = sizeof(unsigned char);

    ret = cfg_get_para(CFG_ITEM_DCOM_SET, state, &len);

    if (0 != ret)
    {
        log_e(LOG_NM, "get dcom set state failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     nm_bind
description:  bind the socket to specified interface
input:        int sockfd      
              NET_TYPE type
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_bind(int sockfd, NET_TYPE type)
{
    int ret;
    NM_NET_ITEM *item;

    if ((type >= NM_NET_TYPE_NUM) || (sockfd < 0))
    {
        log_e(LOG_NM, "para error,sockfd:%d, type:%d", sockfd, type);
        return NM_INVALID_PARA;
    }

    pthread_mutex_lock(&nm_mutex);
    item = nm_net_info.item + type;

    if (NM_NET_CONNECTED == item->state)
    {
        ret = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, item->interface, sizeof(item->interface));

        if (ret < 0)
        {
            log_e(LOG_NM, "bind interface failed,error:%s", strerror(errno));
            pthread_mutex_unlock(&nm_mutex);
            return ret;
        }

        pthread_mutex_unlock(&nm_mutex);
        return 0;
    }

    pthread_mutex_unlock(&nm_mutex);
    log_o(LOG_NM, "the data link is disconnnected,status:%u", nm_net_info.item[type].state);
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
bool nm_get_net_status(NET_TYPE type)
{
    pthread_mutex_lock(&nm_mutex);

    if (NM_NET_CONNECTED == nm_net_info.item[type].state)
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
bool nm_net_is_apn_valid( NET_TYPE type )
{
    pthread_mutex_lock(&nm_mutex);

    if (0 == strlen(nm_net_info.item[type].apn))
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
function:     nm_reg_status_changed
description:  if network status is changed,notify callback
input:        nm_status_changed callback
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_reg_status_changed(NET_TYPE type, nm_status_changed callback)
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

    nm_tbl.item[nm_tbl.used_num].type    = type;
    nm_tbl.item[nm_tbl.used_num].changed = callback;
    nm_tbl.used_num++;
    pthread_mutex_unlock(&nm_regtbl_mutex);

    return 0;
}

