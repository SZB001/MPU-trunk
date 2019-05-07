/****************************************************************
 file:         at_task.c
 description:  the source file of at task implementation
 date:         2017/07/13
 author        yuzhimin
 ****************************************************************/
#include "com_app_def.h"
#include "cfg_api.h"
#include "tcom_api.h"
#include "at_queue.h"
#include "at_stat.h"
#include "timer.h"
#include "at_dev.h"
#include "at_task.h"
#include "at_data.h"
#include "at_packing.h"
#include "at_op.h"
#include "pm_api.h"

static AT_CMD_T cur_at;
static unsigned char at_modem_timeout_cnt = 0;
static unsigned char at_ap_timeout_cnt    = 0;

timer_t at_init_timer;
timer_t at_poll_timer;
timer_t at_retry_timer;

static pthread_mutex_t at_task_mutex;

/* commands to init at module */
static AT_CMD_SEND at_init_cmd[] =
{
    {ATE,           "ATE0\r\n" }, /* disable echo */
    {AT_QGPSCFG,    "AT+QGPSCFG=\"outport\",\"linuxsmd\"\r\n" }, /*GNSS*/
    {AT_QGPSCFG,    "AT+QGPSCFG=\"gpsnmeatype\",31\r\n" },
    {AT_QGPSCFG,    "AT+QGPSCFG=\"glonassnmeatype\",0\r\n" },
    {AT_QGPS,       "AT+QGPS=1,30,50,0,1\r\n" },
    {AT_QGPS,       "AT+QGPS?\r\n" },
    {ATI,           "ATI\r\n" }, /* version */
    {AT_IMEI,       "AT+EGMR=0,7\r\n" }, /* IMEI */
    {AT_IMSI,       "AT+QIMI\r\n" }, /* IMSI */
    {AT_QNWINFO,    "AT+QNWINFO\r\n" },
    {AT_COPS,       "AT+COPS=0\r\n" }, /* set rat auto */
    {AT_CSQ,        "AT+CSQ\r\n" }, /* signal quality */
    {AT_QCCID,      "AT+QCCID\r\n" },
    {AT_CLCC,       "AT+CLCC\r\n" },
    {AT_COPS,       "AT+COPS?\r\n" },
    {AT_QDAI,       "AT+QDAI=1,0,0,4,0\r\n" },
    {AT_QAUDMOD,    "AT+QAUDMOD=2\r\n" }, /*audio echo canceller*/
    {AT_QTONE,      "AT+QCFG=\"tone/incoming\",1\r\n" },
    {AT_CNMI,       "AT+CNMI=2,1,2,0,1\r\n" }, /*SMS*/
    {AT_CFUN,       "AT+CFUN?\r\n" },
    {AT_QWSSID,     "AT+QWSSID?\r\n" },
    {AT_QWAUTH,     "AT+QWAUTH?\r\n" },
    {AT_QWSETMAC,   "AT+QWSETMAC?\r\n" },
    {AT_QWMAXSTA,   "AT+QWMAXSTA?\r\n" },
    {AT_QWIFI,      "AT+QWIFI=1,0,0\r\n" }, /*wifi on after wifi setting*/
    {AT_QWIFI,      "AT+QWIFI?\r\n" }, /*wifi query*/
    {AT_CPMS,       "AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n" },/*Set SMS message storage as "ME"*/
    {AT_CMGD,       "AT+CMGD=1,4\r\n" }, /* delete all sms */
    {AT_CNUM,       "AT+CNUM\r\n" }, /*telnum*/
    {AT_QCFG,       "AT+QCFG=\"qoos\",0\r\n"},
    {AT_CGDCONT,    "AT+CGDCONT?\r\n"},
};

static AT_CMD_SEND at_poll_cmd[] =
{
    {AT_CSQ,        "AT+CSQ\r\n" }, /* signal quality */
    {AT_QNWINFO,    "AT+QNWINFO\r\n" },  /*operator in numeric */
    {AT_COPS,       "AT+COPS?\r\n" }, /* china mobile/unicom */    
    {AT_QCCID,      "AT+QCCID\r\n" }, /* ICCID */
    {AT_CLCC,       "AT+CLCC\r\n" },  /* call status */
    {AT_CFUN,       "AT+CFUN?\r\n" },
    {AT_QGPS,       "AT+QGPS?\r\n" },
    {AT_QWIFI,      "AT+QWIFI?\r\n" },
    {AT_QWMAXSTA,   "AT+QWMAXSTA?\r\n" },
    {AT_QWSSID,     "AT+QWSSID?\r\n" },
    {AT_QWAUTH,     "AT+QWAUTH?\r\n" },
    {AT_QWSTATUS,   "AT+QWSTATUS?\r\n" },
    {AT_QWSTAINFO,  "AT+QWSTAINFO?\r\n" },
    {AT_QCFG,       "AT+QCFG=\"qoos\"\r\n"},
    {AT_IMEI,       "AT+EGMR=0,7\r\n" }, /* IMEI */
    {AT_IMSI,       "AT+QIMI\r\n"     }, /* IMSI */
    {AT_CGDCONT,    "AT+CGDCONT?\r\n"},
};

/******************************************************************
 function:     at_set_task_event
 description:  set event by send message
 input:        AT_MSG_EVENT event
 output:       none
 return:       0 indicates success;
 others indicates failed
 ******************************************************************/
int at_set_task_event(AT_MSG_EVENT event)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    /* send message to the nm */
    msghdr.sender = MPU_MID_AT;
    msghdr.receiver = MPU_MID_AT;
    msghdr.msgid = event;
    msghdr.msglen = 0;

    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_AT, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
              msghdr.msgid, msghdr.receiver, ret);
        return ret;
    }

    return 0;
}

/******************************************************************
 function:     at_send_enable
 description:  push the AT cmd to queue
 input:        const AT_CMD_T* at, AT_PRIORITY prior
 output:       none
 return:       none
 ******************************************************************/
void at_send_enable(const AT_CMD_T *at, AT_PRIORITY prior)
{
    if (NULL == at)
    {
        return;
    }

    if (0 == at->len)
    {
        log_e(LOG_AT, "AT cmd str is 0");
        return;
    }

    pthread_mutex_lock(&at_task_mutex);

    if (at_get_stat() < AT_INIT_IDLE_ST)
    {
        log_e(LOG_AT, "LTE is not on,not enqueue[%u]", prior);
        pthread_mutex_unlock(&at_task_mutex);
        return;
    }

    if (at_get_at_lock())
    {
        log_e(LOG_AT, "query be locked");
        pthread_mutex_unlock(&at_task_mutex);
        return;
    }

    if (prior == AT_HIGH_PRI && !at_is_full(&at_high))
    {
        at_enqueue(&at_high, at);
    }
    else if (prior == AT_NOR_PRI && !at_is_full(&at_normal))
    {
        at_enqueue(&at_normal, at);
    }
    else
    {
        log_e(LOG_AT, "at queue[%u] is full!", prior);
    }

    pthread_mutex_unlock(&at_task_mutex);
    return;
}

/******************************************************************
 function:     at_task_init
 description:  init task module
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ******************************************************************/
int at_task_init(void)
{
    int ret;

    ret = at_para_init();

    if (0 != ret)
    {
        log_e(LOG_AT, "init at config parameter error");
        return ret;
    }

    pthread_mutex_init(&at_task_mutex, NULL);

    at_init_stat();
    at_init_queue();
    memset(&cur_at, 0, sizeof(cur_at));

    ret = tm_create(TIMER_REL, AT_MSG_INIT_EVENT, MPU_MID_AT, &at_init_timer);

    if (ret != 0)
    {
        log_e(LOG_AT, "create init at timer failed, ret:0x%08x", ret);
        return ret;
    }

    ret = tm_create(TIMER_REL, AT_MSG_POLL_EVENT, MPU_MID_AT, &at_poll_timer);

    if (ret != 0)
    {
        log_e(LOG_AT, "create poll at timer failed, ret:0x%08x", ret);
        return ret;
    }

    ret = tm_create(TIMER_REL, AT_MSG_SEND_RETRY_EVENT, MPU_MID_AT, &at_retry_timer);

    if (ret != 0)
    {
        log_e(LOG_AT, "create retry at timer failed, ret:0x%08x", ret);
        return ret;
    }

    at_set_stat(AT_INIT_IDLE_ST);

    tm_start(at_init_timer, 100, TIMER_TIMEOUT_REL_ONCE);

    log_o(LOG_AT, "init over");
    return 0;
}

/******************************************************************
 function:     at_send_proc
 description:  at cmd sending process
 input:        none
 output:       none
 return:       none
 ******************************************************************/
static void at_send_proc(void)
{
    AT_CMD_T AT;
    memset(&AT, 0, sizeof(AT_CMD_T));

    switch (at_get_stat())
    {
        case AT_INIT_IDLE_ST:
        case AT_NORM_IDLE_ST:
            break;

        default:
            return;
    }

    if (!at_is_empty(&at_high))
    {
        at_dequeue(&at_high, &AT);
    }
    else if (!at_is_empty(&at_normal))
    {
        at_dequeue(&at_normal, &AT);
    }
    else
    {
        log_i(LOG_AT, "at queue is empty");
        return;
    }

    log_i(LOG_AT, "at queue num =[%d,%d]", at_queue_get_len(&at_normal),
          at_queue_get_len(&at_high));

    if (AT.len > sizeof(cur_at.str) - 1)
    {
        log_e(LOG_AT, "invalid at cmd, len:%u", cur_at.len);
        return;
    }

    cur_at.id  = AT.id;
    cur_at.len = AT.len;

    memset(cur_at.str, 0, sizeof(cur_at.str));
    memcpy(cur_at.str, AT.str, AT.len);

    at_port_dev_send((unsigned char *) AT.str, AT.len);

    if (AT_INIT_IDLE_ST == at_get_stat())
    {
        at_set_stat(AT_INIT_SENDING_ST);
    }
    else
    {
        at_set_stat(AT_NORM_SENDING_ST);
    }

    if (AT_TEST != AT.id)
    {
        tm_stop(at_retry_timer);
        tm_start(at_retry_timer, at_map[cur_at.id].timeout, TIMER_TIMEOUT_REL_ONCE);
    }
}

/******************************************************************
 function:     at_send_retry_proc
 description:  at cmd sending retry process
 input:        none
 output:       none
 return:       none
 ******************************************************************/
static void at_send_retry_proc(void)
{
    if (NULL == cur_at.str || 0 == cur_at.len)
    {
        return;
    }

    switch (at_get_stat())
    {
        case AT_INIT_SENDING_ST:
        case AT_INIT_RETRY_ST:
            at_set_stat(AT_INIT_RETRY_ST);
            break;

        case AT_NORM_SENDING_ST:
        case AT_NORM_RETRY_ST:
            at_set_stat(AT_NORM_RETRY_ST);
            break;

        default:
            return;
    }

    /* if CSQ,COPS,CLCC AT_QNWINFO is retry more than 15 times, 4G module is fault */
    if ((AT_CSQ == cur_at.id) || (AT_COPS == cur_at.id) || (AT_CLCC == cur_at.id) || (AT_QNWINFO == cur_at.id) )
    {
        at_modem_timeout_cnt++;

        if (at_modem_timeout_cnt > AT_MODEM_TIMEOUT_MAX_CNT)
        {
            at_info.at_timeout   = AT_4G_FAULT;
            at_modem_timeout_cnt = 0;
        }
    }

    at_ap_timeout_cnt++;

    if( at_ap_timeout_cnt > AT_AP_TIMEOUT_MAX_CNT )
    {
        pm_send_evt(MPU_MID_AT, PM_EVT_RESTART_4G_REQ);
        at_ap_timeout_cnt = 0;
        log_o(LOG_AT, "AT timeout for a long time, reset 4G....");
    }

    at_port_dev_send((unsigned char *) cur_at.str, cur_at.len);

    tm_stop(at_retry_timer);
    tm_start(at_retry_timer, at_map[cur_at.id].timeout, TIMER_TIMEOUT_REL_ONCE);
}

/******************************************************************
 function:     at_port_recv_result
 description:  process the result of recv
 input:        none
 output:       none
 return:       none
 ******************************************************************/
static void at_port_recv_result(void)
{
    switch (at_get_stat())
    {
        case AT_INIT_SENDING_ST:
        case AT_INIT_RETRY_ST:
            at_set_stat(AT_INIT_IDLE_ST);
            tm_stop(at_init_timer);
            tm_start(at_init_timer, AT_SEND_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
            break;

        case AT_NORM_SENDING_ST:
        case AT_NORM_RETRY_ST:
            at_set_stat(AT_NORM_IDLE_ST);

            if (at_is_empty(&at_high))
            {
                tm_stop(at_poll_timer);
                tm_start(at_poll_timer, AT_SEND_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
            }

            break;

        default:
            log_e(LOG_AT, "not expect the state=%d", at_get_stat());
            break;
    }

    at_send_proc();
}

/******************************************************************
 function:     at_handler_from_port
 description:  handle the data receive from port
 input:        unsigned char *buf,
 unsigned int len
 output:       none
 return:       none
 ******************************************************************/
void at_handler_from_port(unsigned char *buf, unsigned int len)
{
    unsigned int i = 0;
    char *p = NULL;

    if (NULL == buf || (strlen("\r\n") >= len))
    {
        return;
    }

    /*if CSQ,COPS,CLCC AT_QNWINFO is return, wo can think 4G module is good*/
    if ((AT_CSQ == cur_at.id) || (AT_COPS == cur_at.id) || (AT_CLCC == cur_at.id) || (AT_QNWINFO == cur_at.id) )
    {
        at_info.at_timeout = AT_4G_NORMAL;
        at_modem_timeout_cnt = 0;
    }

    at_ap_timeout_cnt = 0;

    /* at result parse */
    for (i = 0; i < RST_MAX; i++)
    {
        p = strstr((char *) buf, (char *) at_rst[i].rst_str);

        if (p)
        {
            at_rst[i].rst_fn(p, cur_at.id);
            at_port_recv_result();
            /* at cmd is over */
            return;
        }
    }

    /* at respone parse */
    for (i = 0; i < AT_CMD_ID_MAX; i++)
    {
        if (NULL != at_map[i].rsp_fn && NULL != at_map[i].rsp_str)
        {
            p = strstr((char *) buf, (char *) at_map[i].rsp_str);

            if (p)
            {
                at_map[i].rsp_fn(p, cur_at.id);
                break;
            }
        }
    }

    /* urc parse */
    if (i == AT_CMD_ID_MAX)
    {
        at_recv_urc((char const *) buf);
    }

    return;
}

/******************************************************************
 function:     at_sleep_proc
 description:  process the sleep message
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_sleep_proc(void)
{
    if (at_get_at_lock())
    {
        return;
    }

    /* clean queue*/
    at_clear_queue(&at_high);
    at_clear_queue(&at_normal);

    /* add close GPS and WIFI*/
    at_set_wifi(0);       //disable wifi
    at_set_wifi(2);       //query wifi
    at_set_qoos(2);      //enable qoos
    at_set_qoos(3);      //enable qoos
    at_disable_gps();    //disable gps
    at_query_gps();     //query gps

    /* lock queue*/
    at_set_at_lock();
}

/******************************************************************
 function:     at_wakeup_proc
 description:  process the wakeup message
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_wakeup_proc(void)
{
    at_set_at_unlock();
    at_set_ready_sleep(0);
}

/******************************************************************
 function:     at_init_cmd_proc
 description:  the init process thread
 input:        none
 output:       none
 return:       none
 ******************************************************************/
void at_init_cmd_proc(void)
{
    static unsigned char step_max = sizeof(at_init_cmd) / sizeof(at_init_cmd[0]);
    static unsigned char init_step = 0;
    AT_CMD_T AT;

    memset(&AT, 0, sizeof(AT_CMD_T));

    log_i(LOG_AT, "init_step=[%d/%d] stat=%d", init_step, step_max, at_get_stat());

    if (at_get_stat() != AT_INIT_IDLE_ST)
    {
        return;
    }

    if (!at_para.wifi_enable)     // wifi not enable, skip wifi setting
    {
        while (init_step < step_max && (AT_QWIFI == at_init_cmd[init_step].id ||
                                        AT_QWSSID == at_init_cmd[init_step].id ||
                                        AT_QWAUTH == at_init_cmd[init_step].id ||
                                        AT_QWSETMAC == at_init_cmd[init_step].id ||
                                        AT_QWMAXSTA == at_init_cmd[init_step].id))
        {
            init_step++;
        }
    }

    if (init_step < step_max)
    {
        switch (at_init_cmd[init_step].id)
        {
            case AT_COPS:
                at_cops(&AT, at_para.net_type);
                break;

            case AT_QWSSID:
                at_qwssid(&AT, at_para.wifi_ssid);
                break;

            case AT_QWAUTH:
                at_qwauth(&AT, at_para.wifi_key);
                break;

            case AT_QWMAXSTA:
                at_qwmaxsta(&AT, at_para.wifi_maxassoc);
                break;

            default:
                AT.id = at_init_cmd[init_step].id;
                AT.len = strlen((char const *) at_init_cmd[init_step].name);
                snprintf((char *) AT.str, AT_CMD_SIZE_MAX,
                         (char const *) at_init_cmd[init_step].name);
                break;
        }

        at_send_enable(&AT, AT_NOR_PRI);
        at_set_task_event(AT_MSG_SEND_EVENT);

        init_step++;

        tm_stop(at_init_timer);
        tm_start(at_init_timer, AT_INIT_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
    }
    else
    {
        at_set_stat(AT_NORM_IDLE_ST);
        init_step = 0;

        tm_stop(at_poll_timer);
        tm_start(at_poll_timer, AT_POLL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
    }

    return;
}

/******************************************************************
 function:     at_poll_cmd_proc
 description:  the poll process thread
 input:        none
 output:       none
 return:       none
 ******************************************************************/
static void at_poll_cmd_proc(void)
{
    static unsigned char step_max = sizeof(at_poll_cmd) / sizeof(at_poll_cmd[0]);
    static unsigned char poll_step = 0;

    AT_CMD_T AT;
    memset(&AT, 0, sizeof(AT_CMD_T));

    if (at_get_stat() < AT_NORM_IDLE_ST)
    {
        return;
    }

    if (!at_para.wifi_enable)     // wifi not enable, skip wifi check
    {
        while (AT_QWSSID == at_poll_cmd[poll_step].id ||
               AT_QWAUTH == at_poll_cmd[poll_step].id ||
               AT_QWMAXSTA == at_poll_cmd[poll_step].id ||
               AT_QWSTATUS == at_poll_cmd[poll_step].id ||
               AT_QWSTAINFO == at_poll_cmd[poll_step].id)
        {
            poll_step = (poll_step + 1) % step_max;
        }
    }

    AT.id  = at_poll_cmd[poll_step].id;
    AT.len = strlen((char const *) at_poll_cmd[poll_step].name);
    snprintf((char *) AT.str, AT_CMD_SIZE_MAX, (char const *) at_poll_cmd[poll_step].name);

    if (6 != at_get_call_status())
    {
        /* query network type one time in each polling cycle while call is active */
        if (AT_CLCC == AT.id)
        {
            at_cops(&AT, 0xff);
        }
        else
        {
            at_clcc(&AT);
        }
    }

    if (AT_QWSTAINFO == AT.id)
    {
        at_wifi.cli_idx = 0;
    }

    if (AT_CLCC == AT.id)
    {
        at_call.temp_status = 6;
    }

    at_send_enable(&AT, AT_NOR_PRI);
    at_set_task_event(AT_MSG_SEND_EVENT);

    poll_step = (poll_step + 1) % step_max;

    tm_stop(at_poll_timer);
    tm_start(at_poll_timer, AT_POLL_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

    return;
}

/******************************************************************
 function:     at_task
 description:  process event by different message
 input:        AT_MSG_EVENT msg_id
 output:       none
 return:       none
 ******************************************************************/
void at_task(AT_MSG_EVENT msg_id)
{
    if (AT_MSG_INIT_EVENT == msg_id)
    {
        at_init_cmd_proc();
    }
    else if (AT_MSG_POLL_EVENT == msg_id)
    {
        at_poll_cmd_proc();
    }
    else if (AT_MSG_SEND_EVENT == msg_id)
    {
        at_send_proc();
    }
    else if (AT_MSG_SEND_RETRY_EVENT == msg_id)
    {
        at_send_retry_proc();
    }
}

