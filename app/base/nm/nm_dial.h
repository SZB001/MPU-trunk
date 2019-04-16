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

#define NM_NET_NUM          2
#define NM_MAX_REG_TBL      6
#define NM_MAX_DIAL_TIMES   6


#define NM_RET_CHK(f)   do { if( f != 0 ) return f; } while( 0 )

#define SASTORAGE_FAMILY(addr)  (addr).ss_family
#define SASTORAGE_DATA(addr)    (addr).__ss_padding

typedef enum
{
    NM_NET_NOT_INITED = 0,
    NM_NET_INITING,
    NM_NET_INITED,
    NM_NET_CONNECTING,
    NM_NET_CONNECTED,
    NM_NET_DISCONNECTED,
    DSI_STATE_MAX
} NM_NET_STATE;

typedef struct
{
    NET_TYPE type;
    dsi_hndl_t handle;
    volatile NM_NET_STATE state;
	int wait_cnt;
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
} NM_NET_ITEM;

typedef struct
{
    NET_TYPE cur_type;
    volatile NM_NET_STATE init_state;
    unsigned short timername;
    timer_t recall_timer;;
    NM_NET_ITEM item[NM_NET_TYPE_NUM];
} NM_NET_INFO;

typedef struct NM_REG_ITEM
{
    NET_TYPE type;
    nm_status_changed changed;
} NM_REG_ITEM;


typedef struct NM_REG_TBL
{
    unsigned char used_num;
    NM_REG_ITEM item[NM_MAX_REG_TBL];
} NM_REG_TBL;

int  nm_dial_init(INIT_PHASE phase);
int  nm_dial_all_net(void);
void nm_dial_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody);
void nm_dial_call(NET_TYPE type);
void nm_dial_stop(NET_TYPE type);
void nm_dial_restart(void);

#endif

