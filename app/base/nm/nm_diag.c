/****************************************************************
file:         nm_diag.c
description:  the source file of network diagnose implementation
date:         2018/06/30
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "timer.h"
#include "nm_api.h"
#include "at_api.h"
#include "pm_api.h"
#include "at.h"
#include "nm_dial.h"
#include "nm_diag.h"
#include "diag.h"

static NM_REG_OTA_TBL  nm_diag_tbl;
static pthread_mutex_t nm_diag_regtbl_mutex;
static timer_t         nm_diag_timer;
static NM_DIAG_CTL     nm_diag_ctl;

/****************************************************************
function:     nm_diag_init
description:  init data communciation diag module
input:        INIT_PHASE phase, init phase
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_diag_init(INIT_PHASE phase)
{
    int ret;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&nm_diag_regtbl_mutex, NULL);
            memset(&nm_diag_tbl, 0, sizeof(nm_diag_tbl));
            memset(&nm_diag_ctl, 0, sizeof(nm_diag_ctl));
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* create diag timer */
            ret = tm_create(TIMER_REL, NM_MSG_ID_DIAG_TIMER, MPU_MID_NM, &nm_diag_timer);

            if (ret != 0)
            {
                log_e(LOG_NM, "tm_create diag timer failed, ret:0x%08x", ret);
                return ret;
            }

            /* start diag timer */
            ret = tm_start(nm_diag_timer, NM_DIAG_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

            if (ret != 0)
            {
                log_e(LOG_NM, "tm_start diag timer failed, ret:0x%08x", ret);
            }

            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
function:     nm_diag_chk_ota_link
description:  check whether all the ota link are fault
input:        none
output:       none
return:       false indicates all the ota link are disconnected;
              true indicates at least one ota link is connected 
              or no ota link is used;
****************************************************************/
bool nm_diag_chk_ota_link(void)
{
    int i; 

    /* no ota link is used */
    if( 0 == nm_diag_tbl.used_num )
    {
        return true;   
    }

    /* at least one ota link is connected */
    for( i = 0; i < nm_diag_tbl.used_num; i++ )
    {
        if( NM_OTA_LINK_NORMAL == nm_diag_tbl.get[i]() )
        {
            return true;
        } 
    }

    return false;
}

/****************************************************************
function:     nm_diag_chk_dial_link
description:  check whether all the dial link are fault
input:        none
output:       none
return:       false indicates all the dial link are disconnected;
              true indicates at least one dial link is connected 
              or no dial link is used;
****************************************************************/
bool nm_diag_chk_dial_link(void)
{
    int i;

    for ( i = 0; i < NM_NET_TYPE_NUM; i++ )
    {
        if( nm_net_is_apn_valid(NM_PRIVATE_NET) )  
        {
            break;
        }
    }

    /* no dial link is used */
    if( i >= NM_NET_TYPE_NUM )
    {
        return true;    
    }

    /* at least one dial link is connected */
    for ( i = 0; i < NM_NET_TYPE_NUM; i++ )
    {
        if( nm_get_net_status(NM_PRIVATE_NET) )  
        {
            return true;
        }
    }

    return false;
}

/****************************************************************
function:     nm_diag_network
description:  diagnose the network
input:        none
output:       none
return:       none
****************************************************************/
void nm_diag_network(void)
{
    log_i(LOG_NM, "nm_diag_network, start time:%u, time:%u, diag times:%u,fun:%u,status:%u", 
                  (unsigned int)nm_diag_ctl.start_time, (unsigned int)tm_get_time(),
                  nm_diag_ctl.diag_times,at_get_cfun_status(), nm_diag_ctl.status);

    /* if sim card is fault, skip diagnose network */ 
    if (1 != dev_diag_get_sim_status() )
    {
        return;
    }
    
    /* all dial link is fault */
    if( !nm_diag_chk_dial_link() )
    {
        /* network status changed from NORMAL to NM_NET_DIAL_LINK_FAULT*/
        if (NM_NET_NORMAL == nm_diag_ctl.status)
        {
            nm_diag_ctl.status     = NM_NET_DIAL_LINK_FAULT;
            nm_diag_ctl.start_time = tm_get_time();
            nm_diag_ctl.diag_times = 0;
        }
        /* if fault is changed by nm_dial_restart, wait enough time for dialing network */
        else if (NM_NET_OTA_LINK_FAULT == nm_diag_ctl.status && nm_diag_ctl.diag_times > 0)
        {
            if( (tm_get_time() - nm_diag_ctl.start_time) > NM_MAX_OTA_LINK_DIAG_INTERVAL )
            {
                nm_diag_ctl.status     = NM_NET_DIAL_LINK_FAULT;
                nm_diag_ctl.start_time = tm_get_time();
                nm_diag_ctl.diag_times = 0; 
            }
        }
    }
    /* all ota link is fault */
    else if( !nm_diag_chk_ota_link() )
    {
        /* network status changed to NM_NET_OTA_LINK_FAULT*/
        if (NM_NET_OTA_LINK_FAULT != nm_diag_ctl.status)
        {
            nm_diag_ctl.status     = NM_NET_OTA_LINK_FAULT;
            nm_diag_ctl.start_time = tm_get_time();
            nm_diag_ctl.diag_times = 0;
        }    
    }
    /* the network is normal */
    else
    {
         nm_diag_ctl.status     = NM_NET_NORMAL;
         nm_diag_ctl.start_time = 0;
         nm_diag_ctl.diag_times = 0;    
    }

    if (NM_NET_DIAL_LINK_FAULT == nm_diag_ctl.status)
    {
        if( ((tm_get_time() - nm_diag_ctl.start_time) > NM_MAX_DIAL_LINK_DIAG_INTERVAL )
            && ( nm_diag_ctl.diag_times < NM_MAX_DIAG_TIMES) )
        {
            /* cfun full functionality */ 
            if( 1 == at_get_cfun_status() ) 
            {
  
                log_o(LOG_NM, "set cfun disable, start time:%u, time:%u, diag times:%u", 
                      (unsigned int)nm_diag_ctl.start_time, (unsigned int)tm_get_time(),
                      nm_diag_ctl.diag_times);
                
                at_set_cfun(4);  
            }
            else
            {
                log_o(LOG_NM, "set cfun enable, start time:%u, time:%u, diag times:%u,fun:%u", 
                      (unsigned int)nm_diag_ctl.start_time, (unsigned int)tm_get_time(),
                      nm_diag_ctl.diag_times,at_get_cfun_status());
                
                at_set_cfun(1); 
                nm_diag_ctl.start_time = tm_get_time();
                nm_diag_ctl.diag_times++;
            }
        }
        else if( ((tm_get_time() - nm_diag_ctl.start_time) > NM_MAX_DIAL_LINK_DIAG_INTERVAL )
                  && ( nm_diag_ctl.diag_times >= NM_MAX_DIAG_TIMES) )
        {
            pm_send_evt(MPU_MID_NM, PM_EVT_RESTART_4G_REQ);
            nm_diag_ctl.diag_times = 0;
            log_o(LOG_NM, "dial link is fault for a long time, reset 4G....");
        } 
    }
    else if (NM_NET_OTA_LINK_FAULT == nm_diag_ctl.status)
    {
         if( ((tm_get_time() - nm_diag_ctl.start_time) > NM_MAX_OTA_LINK_DIAG_INTERVAL )
            && ( nm_diag_ctl.diag_times < NM_MAX_DIAG_TIMES) )
         {
            log_o(LOG_NM, "restart dial network, start time:%u, time:%u, diag times:%u", 
                  (unsigned int)nm_diag_ctl.start_time, (unsigned int)tm_get_time(),
                   nm_diag_ctl.diag_times);
            
            nm_dial_restart();
            nm_diag_ctl.start_time = tm_get_time();
            nm_diag_ctl.diag_times++;
         }
    }
}

/****************************************************************
function:     nm_diag_msg_proc
description:  innner message process
input:        TCOM_MSG_HEADER *msghdr, message header;
              unsigned char *msgbody, message body
output:       none
return:       none
****************************************************************/
void nm_diag_msg_proc(TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    switch (msghdr->msgid)
    {
        case NM_MSG_ID_DIAG_TIMER:
            nm_diag_network();
            break;

        default:
            log_e(LOG_NM, "unknow msg id: %d", msghdr->msgid);
            break;
    }
}

/****************************************************************
function:     nm_register_get_ota_status
description:  register the callback to get the ota link status
input:        nm_ota_status_get callback
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int nm_register_get_ota_status(nm_ota_status_get callback)
{
    /* the paramerter is invalid */
    if (NULL == callback)
    {
        log_e(LOG_NM, "callback is NULL");
        return NM_INVALID_PARA;
    }

    pthread_mutex_lock(&nm_diag_regtbl_mutex);

    if (nm_diag_tbl.used_num >= NM_MAX_REG_OTA_TBL)
    {
        pthread_mutex_unlock(&nm_diag_regtbl_mutex);
        log_e(LOG_NM, "nm register table overflow");
        return NM_TABLE_OVERFLOW;
    }

    nm_diag_tbl.get[nm_diag_tbl.used_num] = callback;
    nm_diag_tbl.used_num++;
    pthread_mutex_unlock(&nm_diag_regtbl_mutex);
    
    return 0;
}
