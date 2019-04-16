/****************************************************************
file:         uds_main.c
description:  the header file of at main function implemention
date:         2017/10/19
author        wangqinglong
****************************************************************/
#include "com_app_def.h"
#include "tcom_api.h"
#include "scom_api.h"
#include "cfg_api.h"
#include "init.h"
#include "log.h"
#include "uds.h"
#include "uds_diag.h"
#include "uds_proxy.h"
#include "uds_define.h"
#include "uds_shell.h"
#include "J1939.h"
#include "pm_api.h"
#include "scom_msg_def.h"
#include "uds_diag_cond.h"

static timer_t       uds_diag_timer;
static pthread_t     uds_tid;  /* thread id */
static unsigned char uds_msgbuf[TCOM_MAX_MSG_LEN];
static unsigned char uds_is_ready_sleep = 1;

/****************************************************************
function:     uds_sleep_available
description:  whether uds module can sleep or other operation
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_sleep_available(PM_EVT_ID id)
{
    switch (id)
    {
        case PM_MSG_SLEEP:
        case PM_MSG_OFF:
            if (uds_is_ready_sleep)
            {
                return 0;
            }
            else
            {
                return 1;
            }

        case PM_MSG_EMERGENCY:
        case PM_MSG_RTC_WAKEUP:
            return 1;

        default:
            return 1;
    }
}

/****************************************************************
function:     uds_scom_msg_proc
description:  process spi msg about uds
input:        unsigned char *msg, spi message;
              unsigned int len, message length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_scom_msg_proc(unsigned char *msg, unsigned int len)
{
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;

    if (len < sizeof(SCOM_TL_MSG_HDR))
    {
        log_e(LOG_UDS, "invalid message,len:%u", len);
        return -1;
    }

    if (((tl_hdr->msg_type & 0xf0) >> 4) != SCOM_TL_CHN_UDS)
    {
        log_e(LOG_UDS, "invalid message,msgtype:%u, fct:%u", tl_hdr->msg_type, SCOM_TL_CHN_UDS);
        return -1;
    }

    switch (tl_hdr->msg_type)
    {
        case SCOM_TL_CMD_UDS_MSG:
            scom_forward_msg(MPU_MID_UDS, UDS_SCOM_MSG_IND,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        default:
            log_e(LOG_UDS, "invalid message,msgtype:%u", tl_hdr->msg_type);
            break;
    }

    return 0;
}

/****************************************************************
function:     uds_init
description:  initiaze uds module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int uds_init(INIT_PHASE phase)
{
    int ret = 0;

    log_o(LOG_UDS, "init uds thread");

    uds_shell_init(phase);
    J1939_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            uds_proxy_init();
            ret |= uds_diag_init();

            if (ret != 0)
            {
                log_e(LOG_UDS, "uds diag init failed, ret:0x%08x", ret);
            }

            ret = tm_create(TIMER_REL, UDS_DIAG_TIMER, MPU_MID_UDS, &uds_diag_timer);

            if (ret != 0)
            {
                log_e(LOG_UDS, "tm_create heart timer failed, ret:0x%08x", ret);
                return ret;
            }

            ret = pm_reg_handler(MPU_MID_UDS, uds_sleep_available);

            if (0 != ret)
            {
                log_e(LOG_UDS, "pm register sleep failed , ret:0x%08x" , ret);
                return ret;
            }

            ret = scom_tl_reg_proc_fun(SCOM_TL_CHN_UDS, uds_scom_msg_proc);

            if (ret != 0)
            {
                log_e(LOG_UDS, "reg scom proc failed, ret:0x%08x", ret);
                return ret;
            }

			unsigned char power_thred;
			unsigned int len = sizeof(power_thred);

		    ret = cfg_get_para(CFG_ITEM_FT_UDS_POWER, (unsigned char *)&power_thred, &len);

		    if (ret != 0)
		    {
		        log_e(LOG_UDS, "get supply vol failed, ret:0x%08x", ret);
		        return ret;
		    }

			if( (UDS_24V_POWER != power_thred) && (UDS_12V_POWER != power_thred) )
			{
			    power_thred = UDS_12V_POWER;
				cfg_set_para(CFG_ITEM_FT_UDS_POWER, (unsigned char *)&power_thred, len);	
			}

            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
function:     uds_main
description:  uds module main function
input:        none
output:       none
return:       NULL
****************************************************************/
static void *uds_main(void)
{
    int ret, tcom_fd;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;

    prctl(PR_SET_NAME, "UDS");

    tcom_fd = tcom_get_read_fd(MPU_MID_UDS);

    if (tcom_fd < 0)
    {
        log_e(LOG_UDS, "get uds fd failed");
        return NULL;
    }

    uds_set_server();
    tm_start(uds_diag_timer, UDS_DIAG_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);

        ret = select(tcom_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(tcom_fd, &fds))
            {
                ret = tcom_recv_msg(MPU_MID_UDS, &msgheader, uds_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_UDS, "tcom_recv_msg failed,ret:0x%08x", ret);
                    continue;
                }

                if (MPU_MID_SCOM == msgheader.sender)
                {
                    if (UDS_SCOM_MSG_IND == msgheader.msgid)
                    {
                        log_buf_dump(LOG_UDS, ">>>>>>>>>>>>>uds recv>>>>>>>>>>>>>", uds_msgbuf, msgheader.msglen);
                        uds_proxy(uds_msgbuf, msgheader.msglen);
                    }
                    else if (MPU_MID_MID_PWDG == msgheader.msgid)
                    {
                        pwdg_feed(MPU_MID_UDS);
                    }
                }
                else if (MPU_MID_PM == msgheader.sender)
                {
                    if (PM_MSG_SLEEP == msgheader.msgid
                        || PM_MSG_OFF == msgheader.msgid)
                    {
                        uds_is_ready_sleep = J1939_fault_save();
                    }
                }
                else if (MPU_MID_TIMER == msgheader.sender)
                {
                    /* used for uds diag */
                    if (UDS_DIAG_TIMER == msgheader.msgid)
                    {
                        if (uds_diag_available())
                        {
                            uds_diag_all_devices();
                        }
                    }
                    /* used for J1939 application */
                    else if (TIMER_MAX < msgheader.msgid)
                    {
                        J1939_timeout(msgheader.msgid);
                    }
                }
				else if (MPU_MID_UDS == msgheader.sender)
                {/* used for uds application */
                    if (msgheader.msgid < TIMER_MAX)
                    {
                        uint16_t seq = uds_msgbuf[0] + uds_msgbuf[1] * 256;
                        uds_timeout(msgheader.msgid, seq);
                    }
                }

            }
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)   /* interrupted by signal */
            {
                continue;
            }

            break;  /* thread exit abnormally */
        }

    }

    return NULL;
}

/****************************************************************
function:     uds_run
description:  startup uds module
input:        none
output:       none
return:       positive value indicates success;
              -1 indicates failed
*****************************************************************/
int uds_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&uds_tid, &ta, (void *)uds_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

