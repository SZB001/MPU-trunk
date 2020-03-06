/****************************************************************
 file:         dev_main.c
 description:  the header file of device manager main function implemention
 date:         2017/07/20
 copyright     Wuhan Intest Electronic Technology Co.,Ltd
 author        wangqinglong
 ****************************************************************/
#include "com_app_def.h"
#include "tcom_api.h"
#include "timer.h"
#include "init.h"
#include "dev.h"
#include "diag.h"
#include "at_api.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "dev_shell.h"
#include "dev_time.h"
#include "status_sync.h"
#include "rds.h"
#include "log.h"
#include "status/status.h"
#include "scom_api.h"
#include "dev_mcu_cfg.h"
#include "dev_mcu_para.h"
#include "./upg/upg_ctl.h"
#include "pm_api.h"
#include "scom_msg_def.h"
#include "log_file_mgr.h"

static timer_t       dev_fault_sync_timer;
static pthread_t     dev_tid; /* thread id */
static unsigned char dev_msgbuf[TCOM_MAX_MSG_LEN];
static unsigned int  dev_reboot;
int log_disk_stat = LOG_DISK_OK;

static timer_t dev_fault_diag_timer;
static timer_t dev_print_ver_timer;
static timer_t dev_diag_logdir_timer;

/****************************************************************
 * function:     dev_info_save
 * description:  save tbox reboot count
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 ****************************************************************/
static int dev_info_save(unsigned int count)
{
    int ret;

    ret = rds_update_once(RDS_DATA_DEV_INFO, (unsigned char *) &count, sizeof(count));
    log_e(LOG_DEV, "update device info, ret:0x%08x", ret);

    if (ret != 0)
    {
        log_e(LOG_DEV, "update device info failed, ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 * function:     dev_info_restore
 * description:  restore device info
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 ****************************************************************/
static int dev_info_restore(void)
{
    int ret;
    unsigned int len = sizeof(dev_reboot);
    char ver[COM_APP_VER_LEN];

    ret = rds_get(RDS_DATA_DEV_INFO, (unsigned char *) &dev_reboot, &len, ver);

    if (ret != 0)
    {
        log_e(LOG_DEV, "get device info failed ret:0x%08x", ret);
        return DEV_GET_DEV_INFO_FAILED;
    }

    log_i(LOG_DEV, "device info ver:%s", ver);

    if (len != sizeof(dev_reboot))
    {
        log_e(LOG_DEV, "expect len:%u, actul len:%u", sizeof(dev_reboot), len);
        return DEV_INVALID_DEV_INFO;
    }

    dev_reboot++;
    return 0;
}

/****************************************************************
 * function:     dev_get_reboot_cnt
 * description:  get tbox reboot count
 * input:        none
 * output:       none
 * return:       tbox reboot count
 *****************************************************************/
unsigned int dev_get_reboot_cnt(void)
{
    return dev_reboot;
}

void dev_print_softver_delay(void)
{
    tm_stop(dev_print_ver_timer);
    tm_start(dev_print_ver_timer, DEV_PRINT_VER_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
}

/****************************************************************
 * function:     dev_log_save_init
 * description:  initiaze whether should save log
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_log_save_init(void)
{
    unsigned int len;
    unsigned char save_log;

    len = sizeof(save_log);

    if (cfg_get_para(CFG_ITEM_LOG_ENABLE, &save_log, &len) != 0)
    {
        log_e(LOG_DEV, "get save log flag failed");
        return -1;
    }

    if (save_log && !path_exists(COM_LOG_DIR) &&
        dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false) != 0)
    {
        log_e(LOG_DEV, "make log directory(%s) failed", COM_LOG_DIR);
        return -1;
    }

    log_save_ctrl(save_log ? dev_get_reboot_cnt() : 0, COM_LOG_DIR);
    return 0;
}

/****************************************************************
 * function:     dev_log_save_suspend
 * description:  suspend save log when eMMC formatting
 * input:        none
 * output:       none
 * return:       none
 *****************************************************************/
void dev_log_save_suspend(void)
{
    log_o(LOG_DEV, "stop recording log");
    log_save_ctrl(0, COM_LOG_DIR);
}

/****************************************************************
 * function:     dev_log_save_resume
 * description:  resump save log when eMMC format complete
 * input:        none
 * output:       none
 * return:       none
 *****************************************************************/
void dev_log_save_resume(void)
{
    unsigned int len;
    unsigned char en;
    len = sizeof(en);

    if (cfg_get_para(CFG_ITEM_LOG_ENABLE, &en, &len) != 0)
    {
        log_e(LOG_DEV, "get save log flag failed");
        return;
    }

    if (en)
    {
        dev_log_save_init();
        dev_print_softver_delay();
    }

    log_o(LOG_DEV, "start recording log");
}

/****************************************************************
 * function:     dev_pm_handler
 * description:  initiaze device manager module
 * input:        none
 * output:       none
 * return:       0 indicates can not enter sleep status;
 *               1 indicates can enter sleep status;
 *****************************************************************/
static int dev_sleep_available(PM_EVT_ID id)
{
    return 1;
}


/****************************************************************
function:     dev_scom_msg_proc
description:  process spi msg about dev
input:        unsigned char *msg, spi message;
              unsigned int len, message length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int dev_scom_msg_proc(unsigned char *msg, unsigned int len)
{
    RTCTIME *time = NULL;
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;

    if (len < sizeof(SCOM_TL_MSG_HDR))
    {
        log_e(LOG_DEV, "invalid message,len:%u", len);
        return -1;
    }

    if (((tl_hdr->msg_type & 0xf0) >> 4) != SCOM_TL_CHN_DEV)
    {
        log_e(LOG_DEV, "invalid message,msgtype:%02x, channel:%u", tl_hdr->msg_type, SCOM_TL_CHN_DEV);
        return -1;
    }

    switch ((tl_hdr->msg_type))
    {
        case SCOM_TL_CMD_MCU_FAULT_SYN:
            flt_sync_from_mcu(msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_MCU_STATUS_SYN:
            st_sync_from_mcu(msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_PARA_REPORT:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_MCU_CFG_SET,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_SET_TIME:
        //case SCOM_TL_CMD_REPORT_TIME:
            time = (RTCTIME *)&msg[sizeof(SCOM_TL_MSG_HDR)];
            time->year += 2000;
            dev_syn_time(time, MCU_RTC_TIME_SOURCE);
            break;

        case SCOM_TL_CMD_MCU_CFG_SYN:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_MCU_CFG_SYN,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_CUSTOM_PARA:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_CUSTOM_PARA,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_MCU_SHELL:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_MCU_SHELL,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        default:
            log_e(LOG_DEV, "invalid message,msgtype:%02x", tl_hdr->msg_type);
            break;
    }

    return 0;
}

/****************************************************************
 * function:     dev_init
 * description:  initiaze device manager module
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dev_init(INIT_PHASE phase)
{
    int ret = 0;

    dev_shell_init(phase);
    dev_diag_init(phase);
	upg_ctl_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            st_init();
            flt_sync_init();
            upg_init_startup();
            break;

        case INIT_PHASE_RESTORE:
            if (0 != dev_info_restore())
            {
                dev_reboot = 1;
            }

            dev_info_save(dev_reboot);
            break;

        case INIT_PHASE_OUTSIDE:
            dev_log_save_init();
            dev_time_init();

            ret = pm_reg_handler(MPU_MID_DEV, dev_sleep_available);

            if (ret != 0)
            {
                log_e(LOG_DEV, "reg pm handler failed, ret:0x%08x", ret);
                return ret;
            }

            ret = tm_create(TIMER_REL, DEV_MSG_SYNC_TIMER, MPU_MID_DEV, &dev_fault_sync_timer);

            if (ret != 0)
            {
                log_e(LOG_DEV, "tm_create sync timer failed, ret:0x%08x", ret);
                return ret;
            }

            tm_start(dev_fault_sync_timer, DEV_SYNC_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);


            ret = tm_create(TIMER_REL, DEV_MSG_DIAG_TIMER, MPU_MID_DEV, &dev_fault_diag_timer);

            if (ret != 0)
            {
                log_e(LOG_DEV, "tm_create diag timer failed, ret:0x%08x", ret);
                return ret;
            }

            tm_start(dev_fault_diag_timer, DEV_DIAG_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);


            ret = tm_create(TIMER_REL, DEV_MSG_PRINT_VER_TIMER, MPU_MID_DEV, &dev_print_ver_timer);

            if (ret != 0)
            {
                log_e(LOG_DEV, "tm_create printf version failed, ret:0x%08x", ret);
                return ret;
            }

            ret = tm_create(TIMER_REL, DEV_MSG_DIAG_LOGDIR, MPU_MID_DEV, &dev_diag_logdir_timer);

            if (ret != 0)
            {
                log_e(LOG_DEV, "tm_create diag the log dir failed, ret:0x%08x", ret);
                return ret;
            }

            tm_start(dev_diag_logdir_timer, DEV_DIAG_LOGDIR_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

            ret = scom_tl_reg_proc_fun(SCOM_TL_CHN_DEV, dev_scom_msg_proc);

            if (ret != 0)
            {
                log_e(LOG_DEV, "reg dev scom proc failed, ret:0x%08x", ret);
                return ret;
            }

            ret = scom_tl_reg_proc_fun(SCOM_TL_CHN_MGR, upg_ctl_scom_msg_proc);

            if (ret != 0)
            {
                log_e(LOG_DEV, "reg upg scom proc failed, ret:0x%08x", ret);
                return ret;
            }

            tm_start(dev_print_ver_timer, DEV_PRINT_VER_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
            break;

        default:
            break;
    }

    return ret;
}

/*******************************************************************
 function:     dev_diag_timeout
 description:  diagnose device periodically
 input:        none
 output:       none
 return:       none
 ********************************************************************/
void dev_diag_timeout(void)
{
    if (dev_diag_availbale())
    {
        /* diagnose 4G ant */
        dev_diag_4G_ant();

        /* diagnose emmc */
        dev_diag_emmc();

        /* diagnose mcu */
		dev_diag_mcu();
    }
}

/*******************************************************************
 function:     dev_sync_timeout
 description:  send mpu fault in time
 input:        none
 output:       none
 return:       none
 ********************************************************************/
void dev_sync_timeout(void)
{
    DIAG_MPU_FAULT mpu_fault;
    STATUS_MPU mpu_status;

    /*sync mpu fault to mcu */
    flt_sync_to_mcu(&mpu_fault);
    scom_tl_send_frame(SCOM_TL_CMD_MPU_FAULT_SYN, SCOM_TL_SINGLE_FRAME, 0,    
                       (unsigned char *) &mpu_fault, sizeof(mpu_fault));

    /*sync mpu status to mcu */
    st_sync_to_mcu(&mpu_status);
    scom_tl_send_frame(SCOM_TL_CMD_MPU_STATUS_SYN, SCOM_TL_SINGLE_FRAME, 0,
                       (unsigned char *) &mpu_status, sizeof(mpu_status));
}

/*******************************************************************
 function:     dev_print_ver_timeout
 description:  printf soft version delay
 input:        none
 output:       none
 return:       none
 ********************************************************************/
void dev_print_ver_timeout(void)
{
    int ret;
    unsigned char version[64];

    memset(version, 0, sizeof(version));
    upg_get_fw_ver(version, sizeof(version));
    log_o(LOG_DEV, "Firmware    Ver: %s", version);
    log_o(LOG_DEV, "MPU App     Ver: %s", dev_get_version());

    memset(version, 0, sizeof(version));
    upg_get_mcu_run_ver(version, sizeof(version));
    log_o(LOG_DEV, "MCU App Run Ver: %s", version);

    memset(version, 0, sizeof(version));
    ret = upg_get_mcu_upg_ver(version, sizeof(version));

    if (0 == ret)
    {
        log_o(LOG_DEV, "MCU App Upg Ver: %s", version);
    }
    else
    {
        log_o(LOG_DEV, "MCU App Upg Ver: no upgrade version");
    }

    log_o(LOG_DEV, "cnt: %u", dev_get_reboot_cnt());
}


void dev_diag_logdir_timeout(void)
{
    int disk_st;
    unsigned int len;
    unsigned char save_log;

    len = sizeof(save_log);
    if (cfg_get_para(CFG_ITEM_LOG_ENABLE, &save_log, &len) != 0)
    {
        log_e(LOG_DEV, "get save log flag failed");
        return ;
    }

    if(save_log)
    {
        // check disk status
        disk_st = log_disk_check(0);

        if (LOG_DISK_OK == disk_st && LOG_DISK_OK != log_disk_stat)
        {
            log_disk_stat = log_dir_check();
        }
        else
        {
            log_disk_stat = disk_st;
        }

        if (LOG_DISK_ERROR == log_disk_stat)
        {
            log_e(LOG_DEV, "disk error");
            return;
        }

        if (LOG_DISK_FULL == log_disk_stat)
        {

            log_o(LOG_DEV, "disk full, delete old file");
            log_do_diskfull();
        }

        if(LOG_DISK_OK == log_disk_stat)
        {
            log_dir_do_full();
            log_file_do_full();
        }
    }
 
}

/****************************************************************
 function:     dev_main
 description:  device manager module main function
 input:        none
 output:       none
 return:       NULL
 ****************************************************************/
static void *dev_main(void)
{
    int ret, msg_fd, max_fd = 0;
    TCOM_MSG_HEADER msgheader;
    STATUS_MPU mpu_status;
    fd_set fds;

    prctl(PR_SET_NAME, "DEV");

    msg_fd = tcom_get_read_fd(MPU_MID_DEV);

    if (msg_fd < 0)
    {
        log_e(LOG_DEV, "get at recv fd failed");
        return NULL;
    }

    if (msg_fd > max_fd)
    {
        max_fd = msg_fd;
    }

    /*sync mpu status to mcu */
    st_sync_to_mcu(&mpu_status);
    scom_tl_send_frame(SCOM_TL_CMD_MPU_STATUS_SYN, SCOM_TL_SINGLE_FRAME, 0,
                       (unsigned char *) &mpu_status, sizeof(mpu_status));

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(msg_fd, &fds);

        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(msg_fd, &fds))
            {
                if (0 == tcom_recv_msg(MPU_MID_DEV, &msgheader, dev_msgbuf))
                {
                    if (MPU_MID_DEV == msgheader.sender)
                    {
                        /* do nothing */
                    }
                    else if (MPU_MID_TIMER == msgheader.sender)
                    {
                        if(dev_get_KL15_signal())
                        {
                            if (DEV_MSG_DIAG_TIMER == msgheader.msgid)
                            {
                                dev_diag_timeout();
                            }
                            else if (DEV_MSG_SYNC_TIMER == msgheader.msgid)
                            {
                                dev_sync_timeout();
                            }
                            else if (DEV_MSG_PRINT_VER_TIMER == msgheader.msgid)
                            {
                                dev_print_ver_timeout();
                            }
                            else if ( DEV_MSG_DIAG_LOGDIR == msgheader.msgid )
                            {
                                dev_diag_logdir_timeout();
                            }
                        }
                    }
                    else if (MPU_MID_SCOM == msgheader.sender)
                    {
                        if (MPU_MID_MID_PWDG == msgheader.msgid)
                        {
                            pwdg_feed(MPU_MID_DEV);
                        }
                        else if (DEV_SCOM_MSG_MCU_CFG_SYN == msgheader.msgid)
                        {
                            scom_mcu_cfg_sync(dev_msgbuf, msgheader.msglen);
                        }
                        else if (DEV_SCOM_MSG_MCU_CFG_SET == msgheader.msgid)
                        {
                            dev_set_from_mcu(dev_msgbuf, msgheader.msglen);
                        }
                        else if (DEV_SCOM_MSG_CUSTOM_PARA == msgheader.msgid)
                        {
                            dev_check_custom_para_from_mcu(dev_msgbuf[0], &dev_msgbuf[1], msgheader.msglen - 1);
                        }
                        else if (DEV_SCOM_MSG_MCU_SHELL == msgheader.msgid)
                        {
                            dev_shell_mcu_shell_res(dev_msgbuf, msgheader.msglen);
                        }
                        else if (DEV_SCOM_MSG_MCU_VER == msgheader.msgid)
                        {
                            upg_proc_mcu_ver_msg(dev_msgbuf, msgheader.msglen);
                        }
                        else if (DEV_SCOM_MSG_MCU_BTL_VER == msgheader.msgid)
                        {
                            upg_proc_btl_ver_msg(dev_msgbuf, msgheader.msglen);
                        }
                    }
                    else if (MPU_MID_PM == msgheader.sender)
                    {
                        if (PM_MSG_SLEEP == msgheader.msgid
                            || PM_MSG_OFF == msgheader.msgid)
                        {
                            dev_log_save_suspend();
                            dev_diag_stop();
                        }
                        else if (PM_MSG_EMERGENCY  == msgheader.msgid
                                 || PM_MSG_RUNNING == msgheader.msgid)
                        {
                            dev_log_save_resume();
                            dev_diag_start();
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }
                }
            }
        }
        else if (0 == ret)  /* timeout */
        {
            continue; /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            break; /* thread exit abnormally */
        }

    }

    return 0;
}

/****************************************************************
 function:     dev_run
 description:  startup device manager module
 input:        none
 output:       none
 return:       positive value indicates success;
 -1 indicates failed
 *****************************************************************/
int dev_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&dev_tid, &ta, (void *) dev_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

char *dev_get_version(void)
{
    return COM_APP_SYS_VER;
}

