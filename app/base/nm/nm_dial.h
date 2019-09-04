/****************************************************************
file:         nm_net.h
description:  the header file of data communciation dial function definition
date:         2016/12/12
author        liuzhongwen
****************************************************************/
#ifndef __NM_DIAL_H__
#define __NM_DIAL_H__

#include "dsi_netctrl.h"
#include "ds_util.h"
#include "tcom_api.h"
#include "nm_api.h"

#define NM_PLT_NUM      2

#define NM_RET_CHK(f)   do { if( f != 0 ) return f; } while( 0 )

#define SASTORAGE_FAMILY(addr)  (addr).ss_family
#define SASTORAGE_DATA(addr)    (addr).__ss_padding

typedef enum
{
    NM_NET_IDLE = 0,
    NM_NET_CONNECTING,
    NM_NET_CONNECTED,
    NM_NET_DISCONNECTING,
    DSI_STATE_MAX
} NM_NET_STATE;

typedef struct
{
    NET_TYPE type;
    dsi_hndl_t handle;
    volatile NM_NET_STATE state;
    int cdma_index;
    int umts_index;
    int ip_ver;
    dsi_ip_family_t ip_type;
    char apn[32];
    char user[32];
    char pwd[32];
    char interface[DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2];
    dsi_auth_pref_t auth_pref;
    struct in_addr ip_addr;
    struct in_addr gw_addr;
    struct in_addr pri_dns_addr;
    struct in_addr sec_dns_addr;
    unsigned short timername;
    unsigned int call_id;
    timer_t recall_timer;
} NM_NET_INFO;

#define NM_MAX_REG_TBL      6

typedef struct NM_REG_TBL
{
    unsigned char used_num;
    nm_status_changed changed[NM_MAX_REG_TBL];
} NM_REG_TBL;

int nm_dial_init(INIT_PHASE phase);
int nm_dial_start(void);
//int nm_dial_restart(void);
void nm_dial_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody);
bool nm_get_net_status_ex(NET_TYPE type);

#endif

