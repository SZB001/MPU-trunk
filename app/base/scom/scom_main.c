#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "com_app_def.h"
#include "tcom_api.h"
#include "scom_api.h"
#include "dev_api.h"
#include "scom_dev.h"
#include "scom.h"
#include "scom_tl.h"
#include "timer.h"
#include "file.h"
#include "msg_parse.h"
#include "pm_api.h"
#include "tbox_limit.h"
#include "can_api.h"
#include "dev_time.h"
#include "dev_api.h"
#include "fault_sync.h"
#include "status_sync.h"
#include "pwdg.h"
#include "fct.h"
#include "scom_msg_def.h"

static pthread_t     scom_tid;  /* thread id */
static timer_t       scom_heart_timer;
static timer_t       scom_open_timer;
static timer_t       scom_close_spi_timer;
static int           scom_heart_count = 0;
static int           scom_spifd = -1;

/*soft watchdog feed status(4 bytes) and mcu version(32 bytes) */
static SCOM_HEART_T  scom_heart_data;
static unsigned char scom_msgbuf[TCOM_MAX_MSG_LEN * 2];
static unsigned char scom_is_ready_sleep = 0;

/****************************************************************
function:       scom_sleep_available
description:    check at module can enter into sleep mode
input:          none
output:         none
return:         1 indicates module can sleep
                0 indicates module can not sleep
*****************************************************************/
int scom_sleep_available(PM_EVT_ID id)
{
    switch (id)
    {
        case PM_MSG_SLEEP:
        case PM_MSG_OFF:
            return scom_is_ready_sleep;

        case PM_MSG_EMERGENCY:
        case PM_MSG_RTC_WAKEUP:
            return 1;

        default:
            return 1;
    }
}

/******************************************************************
function:     scom_forward_msg
description:  forward spi message
input:        unsigned int msg_id
              unsigned char *msg, message body;
              unsigned int len, message body length
output:       none
return:       0 indicates success;
              others indicates failed
******************************************************************/
int scom_forward_msg(unsigned short receiver, unsigned int msg_id, unsigned char *msg,
                     unsigned int len)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    /* send message to the receiver */
    msghdr.sender     = MPU_MID_SCOM;
    msghdr.receiver   = receiver;
    msghdr.msgid      = msg_id;
    msghdr.msglen     = len;

    ret = tcom_send_msg(&msghdr, msg);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
              msghdr.msgid, msghdr.receiver, ret);

        return ret;
    }

    return 0;
}



/****************************************************************
function:     scom_init
description:  initiaze spi communcaiton device
input:        INIT_PHASE phase, phase;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int scom_init(INIT_PHASE phase)
{
    int ret;

    log_o(LOG_SCOM, "init scom thread");

    ret = scom_dev_init(phase);

    if (ret != 0)
    {
        return ret;
    }

    ret = scom_tl_init(phase);

    if (ret != 0)
    {
        return ret;
    }

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /*timer use for spi dev open retry*/
            ret = tm_create(TIMER_REL, SCOM_MSG_ID_TIMER_OPEN, MPU_MID_SCOM, &scom_open_timer);

            if (ret != 0)
            {
                log_e(LOG_SCOM, "tm_create spi open timer failed, ret:0x%08x", ret);
                return ret;
            }

            if (0 != scom_dev_open(&scom_spifd))
            {
                tm_start(scom_open_timer, SCOM_OPEN_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
            }

            /*timer use for send heart beat data*/
            ret = tm_create(TIMER_REL, SCOM_MSG_ID_TIMER_HEARTER, MPU_MID_SCOM, &scom_heart_timer);

            if (ret != 0)
            {
                log_e(LOG_SCOM, "tm_create heart timer failed, ret:0x%08x", ret);
                return ret;
            }

            tm_start(scom_heart_timer, SCOM_HEARTER_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

			/*timer use for delay close spi*/
            ret = tm_create(TIMER_REL, SCOM_MSG_ID_TIMER_CLOSE_SPI, MPU_MID_SCOM, &scom_close_spi_timer);

            if (ret != 0)
            {
                log_e(LOG_SCOM, "tm_create scom timer failed, ret:0x%08x", ret);
                return ret;
            }

            scom_tl_reg_tx_fun(scom_dev_send);

            memset(scom_heart_data.version, 0, sizeof(scom_heart_data.version));
            ret = upg_get_mcu_upg_ver((unsigned char *)scom_heart_data.version,
                                      sizeof(scom_heart_data.version));

            if (ret != 0 || !upg_is_mcu_exist())
            {
                memset(scom_heart_data.version, 0, sizeof(scom_heart_data.version));
                log_e(LOG_SCOM, "read mcu version failed, ret:0x%08x", ret);
            }

            pm_reg_handler(MPU_MID_SCOM, scom_sleep_available);
            break;

        default:
            break;
    }

    return 0;
}

/*******************************************************************
function:     scom_timeout_proc
description:  timer timeout message
input:        none
output:       none
return:       none
********************************************************************/
void scom_timeout_proc( void )
{
    int ret;
    unsigned char mode;
    unsigned int len;

    len = sizeof(mode);	
    ret = st_get(ST_ITEM_PM_MODE, &mode, &len);
	if( ret != 0 )
	{
		log_e(LOG_SCOM , "get pm mode failed:%u", ret);	
		return;
	}
	
	log_o(LOG_SCOM , "pm mode:%u", mode);
	
	if(  (PM_LISTEN_MODE == mode) || (PM_SLEEP_MODE == mode)
		|| (PM_DEEP_SLEEP_MODE == mode) || (PM_OFF_MODE == mode) )
	{
	    if (scom_spifd >= 0)
		{
		    scom_dev_close();
            scom_spifd = -1;
            log_o(LOG_SCOM , "close spi devices");
		}
		
	    scom_is_ready_sleep = 1;
	}
}

/****************************************************************
function:     scom_main
description:  spi communciation module main function
input:        none
output:       none
return:       NULL
****************************************************************/
static void *scom_main(void)
{
    unsigned int len;
    int ret, maxfd, tcom_fd;
    static int r_pos = 0, w_pos = 0;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;
    static unsigned char read_buff[TCOM_MAX_MSG_LEN] = {0};
    prctl(PR_SET_NAME, "SCOM");
    pwdg_init(MPU_MID_SCOM);
    tcom_fd = tcom_get_read_fd(MPU_MID_SCOM);

    if (tcom_fd < 0)
    {
        log_e(LOG_SCOM, "get tcom fd failed");
        return 0;
    }

    while (1)
    {
        FD_ZERO(&fds);

        if (scom_heart_count >= 10)
        {
            log_e(LOG_SCOM, "restart spi devices");
            scom_spifd = -1;
            scom_heart_count = 0;
            scom_dev_close();

            if (0 != scom_dev_open(&scom_spifd))
            {
                tm_start(scom_open_timer, SCOM_OPEN_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
            }
        }

        if (scom_spifd >= 0)
        {
            FD_SET(scom_spifd, &fds);
        }

        FD_SET(tcom_fd, &fds);

        maxfd = tcom_fd;

        if (scom_spifd > maxfd)
        {
            maxfd = scom_spifd;
        }

        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (ret > 0)
        {
            if (scom_spifd >= 0)
            {
                if (FD_ISSET(scom_spifd, &fds))
                {
                    if (w_pos >= sizeof(scom_msgbuf))
                    {
                        r_pos = 0;
                        w_pos = 0;
                    }

                    len = sizeof(scom_msgbuf) - w_pos;
                    scom_dev_recv(scom_msgbuf + w_pos, &len);

                    if (len > 0)
                    {
                        w_pos += len;

                        scom_heart_count = 0;
                        msg_decode(&r_pos, &w_pos, scom_msgbuf, scom_tl_msg_proc);
                    }
                }
            }

            if (FD_ISSET(tcom_fd, &fds))
            {
                if (0 == tcom_recv_msg(MPU_MID_SCOM, &msgheader, read_buff))
                {
                    switch (msgheader.msgid)
                    {
                        case PM_MSG_SLEEP:
                        case PM_MSG_OFF:
						    log_o(LOG_SCOM , "start delay timer");
                            tm_start(scom_close_spi_timer, SCOM_DELAY_CLOSE_SCOM_INTERVAL, TIMER_TIMEOUT_REL_ONCE);

                            break;

                        case PM_MSG_EMERGENCY:
							scom_is_ready_sleep = 0;
							tm_stop(scom_close_spi_timer);
                            if (scom_spifd < 0)
                            {
                                if (0 != scom_dev_open(&scom_spifd))
                                {
                                    tm_start(scom_open_timer, SCOM_OPEN_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
                                }

                                log_e(LOG_SCOM , "open spi devices");
                            }

                            break;

                        case PM_MSG_RUNNING:
                            scom_is_ready_sleep = 0;
                            tm_stop(scom_close_spi_timer);
                            if (scom_spifd < 0)
                            {
                                if (0 != scom_dev_open(&scom_spifd))
                                {
                                    tm_start(scom_open_timer, SCOM_OPEN_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
                                }

                                log_e(LOG_SCOM , "open spi devices");
                            }

                            break;

                        case MPU_MID_MID_PWDG:
                            pwdg_timeout(MPU_MID_SCOM);
                            break;

                        case SCOM_MSG_ID_TIMER_HEARTER:
                            scom_heart_data.pwdg = pwdg_food();
                            scom_heart_count++;
                            scom_tl_send_msg(SCOM_TL_CMD_HEARTBEAT, (unsigned char *)&scom_heart_data, sizeof(scom_heart_data));
                            break;

                        case SCOM_MSG_ID_TIMER_OPEN:
                            if (0 == scom_is_ready_sleep && 0 != scom_dev_open(&scom_spifd))
                            {
                                tm_start(scom_open_timer, SCOM_OPEN_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
                            }

                            log_e(LOG_SCOM , "open spi devices timeout");

                            break;

                        case SCOM_MSG_ID_TIMER_NAME_RESEND:
                            scom_tl_timeout_proc();
							
                            break;

						case SCOM_MSG_ID_TIMER_CLOSE_SPI:
                            scom_timeout_proc();
							
                            break;	

                        default:
                            log_e(LOG_SCOM, "unknow msg, sender:0x%04x, recver:0x%04x, msgid:0x%04x",
                                  msgheader.sender, msgheader.receiver, msgheader.msgid);
                            break;
                    }

                }
            }
        }
        else if (0 == ret)  /* timeout */
        {
            continue;
            /* continue to monitor the incomging data */
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

    return 0;
}

/****************************************************************
function:     scom_run
description:  startup scom module
input:        none
output:       none
return:       positive value indicates success;
              -1 indicates failed
*****************************************************************/
int scom_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&scom_tid, &ta, (void *)scom_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

