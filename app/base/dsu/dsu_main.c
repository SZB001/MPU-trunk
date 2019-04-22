/**
 * @Title: dsu_main.c
 * @author yuzhimin
 * @date Nov 7, 2017
 * @version V1.0
 */

#include "dsu_main.h"

static pthread_t dsu_tid; /* thread id */
static unsigned char dsu_msgbuf[TCOM_MAX_MSG_LEN];
static timer_t dsu_timer;
static timer_t inx_timer;
DSU_CFG_T dsu_cfg;
int disk_stat = DISK_OK;
unsigned int canbus_stat = CANBUS_TIMEOUT;
static RTCTIME g_cantag_time;
volatile static unsigned int g_cantag_cnt = 0;       //100hz
pthread_mutex_t dsu_mutex;
static int dsu_sleep;

static int dsu_sleep_handler(PM_EVT_ID id)
{
    int ret = dsu_suspend_record();

    if (!ret)
    {
        dsu_sleep = 1;
    }
    else
    {
        log_e(LOG_DSU, "dsu file stat=%d!", ret);
    }

    return dsu_sleep;
}

static void dsu_do_sleep(void)
{
    int ret = dsu_suspend_record();

    if (!ret)
    {
        dsu_sleep = 1;
    }

    log_e(LOG_DSU, "dsu is sleeping,file stat=%d!", ret);
}

static void dsu_do_wakeup(void)
{
    dsu_sleep = 0;
    dsu_resume_record();
}

static int dsu_cfg_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                           unsigned char *new_para,
                           unsigned int len)
{
    int ret = 0;

    if (CFG_ITEM_DSU_CANLOG_TIME != id &&
        CFG_ITEM_DSU_CANLOG_MODE != id &&
        CFG_ITEM_DSU_LOOPFILE != id &&
        CFG_ITEM_DSU_HOURFILE != id &&
        CFG_ITEM_DSU_AUTHKEY != id &&
        CFG_ITEM_DSU_SDHZ != id)
    {
        log_e(LOG_DSU, "invalid id, id:%u", id);
        return -1;
    }

    /* not changed */
    if (0 == memcmp((const void *) old_para, (const void *) new_para, len))
    {
        log_o(LOG_DSU, "there no change!");
        return 0;
    }

    log_o(LOG_DSU, "id=%d,para=0x%x", id, *(unsigned int *) new_para);

    switch (id)
    {
        case CFG_ITEM_DSU_CANLOG_TIME:
            dsu_cfg.canlog_time = *(short *) new_para;
            iwd_attr.logtime = dsu_cfg.canlog_time;

            if (iwd_attr.stat)
            {
                iwd_attr.countdown = iwd_attr.logtime;
            }

            break;

        case CFG_ITEM_DSU_CANLOG_MODE:
            dsu_cfg.canlog_mode = *(unsigned char *) new_para;

            if (!(dsu_cfg.canlog_mode & DSU_CFG_INX_MASK)
                && (FILE_ST_OPEN == dsu_get_stat(&inx_file)))
            {
                // close inx file
                ret |= dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
                log_i(LOG_DSU, "close inx file");
            }

            if (!(dsu_cfg.canlog_mode & DSU_CFG_IWDZ_MASK)
                && (FILE_ST_OPEN == dsu_get_stat(&iwdz_file)))
            {
                // close iwdz file
                ret |= dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CLOSE);
                log_i(LOG_DSU, "close iwdz file");
            }

            if (!(dsu_cfg.canlog_mode & DSU_CFG_IWD_MASK)
                && (FILE_ST_OPEN == dsu_get_stat(&iwd_file)))
            {
                // close iwd file
                ret |= dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CLOSE);
                log_i(LOG_DSU, "close iwd file");
            }

            inx_attr.enable  = dsu_cfg.canlog_mode & DSU_CFG_INX_MASK;
            iwdz_attr.enable = dsu_cfg.canlog_mode & DSU_CFG_IWDZ_MASK;
            iwd_attr.enable  = dsu_cfg.canlog_mode & DSU_CFG_IWD_MASK;
            break;

        case CFG_ITEM_DSU_LOOPFILE:
            dsu_cfg.loop = *(unsigned char *) new_para;
            break;

        case CFG_ITEM_DSU_HOURFILE:
            dsu_cfg.hour = *(unsigned char *) new_para;
            iwdz_attr.hourfile = dsu_cfg.hour & DSU_CFG_IWDZ_MASK;
            inx_attr.hourfile  = dsu_cfg.hour & DSU_CFG_INX_MASK;
            break;

        case CFG_ITEM_DSU_AUTHKEY:

            // inx file for recording
            if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
            {
                // close current inx file
                ret |= dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
                log_i(LOG_DSU, "close inx file");
            }

            break;

        case CFG_ITEM_DSU_SDHZ:
            dsu_cfg.sdhz = *(unsigned char *) new_para;
            inx_attr.sampling = dsu_cfg.sdhz;

            // inx file for recording
            if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
            {
                // close current inx file
                ret |= dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
                log_i(LOG_DSU, "close inx file");
                inx_attr.suspend = 1;
                tm_start(inx_timer, INX_FILE_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);
            }

            break;

        default:
            return -1;
    }

    if (ret != 0)
    {
        log_e(LOG_DSU, "tcom_send_msg failed, ret:%u", ret);
        return -1;
    }

    return 0;
}

unsigned int dsu_get_opened_filelist(char **list)
{
    unsigned int cnt = 0;

    if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
    {
        list[cnt++] = inx_file.name;
    }

    if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
    {
        list[cnt++] = iwd_file.name;
    }

    if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
    {
        list[cnt++] = iwdz_file.name;
    }

    return cnt;
}

static void dsu_do_diskfull(void)
{
    struct statfs diskfs;
    char *oldfile = NULL;
    unsigned long long block = 0;
    unsigned long long total = 0;
    unsigned long long avail = 0;
    unsigned long long folder_size = 0;    // canlog folder
    char *exfile[3];
    unsigned int ex_cnt = 0;

    ex_cnt = dsu_get_opened_filelist(exfile);

    do
    {
        statfs(COM_SDCARD_DIR, &diskfs);
        block = diskfs.f_bsize;
        total = diskfs.f_blocks * block;
        avail = diskfs.f_bavail * block;
        log_i(LOG_DSU, "total:%llu B;avail:%llu B", total, avail);

        if ((avail >> 20) <= DSU_RESERVE_DISK_SIZE)
        {
            folder_size = (unsigned long long) dir_get_size(DSU_IWD_FILE_PATH);

            if (folder_size > total / 2)
            {
                oldfile = dsu_find_oldfile(DSU_IWD_FILE_PATH, exfile, ex_cnt);
            }
            else
            {
                oldfile = dsu_find_oldfile(DSU_IWDZ_FILE_PATH, exfile, ex_cnt);
            }

            log_o(LOG_DSU, "del file:%s", oldfile);
            file_delete(oldfile);
            sync();
        }
        else
        {
            break;
        }
    }
    while (1);
}

static void dsu_timer_handler(unsigned int time_id)
{
    int disk_st;
    RTCTIME localtime;

    if (DSU_TIMER == time_id)
    {
        // check disk status
        disk_st = dsu_disk_check(0);

        if (DISK_OK == disk_st && DISK_OK != disk_stat)
        {
            disk_stat = dsu_dir_check();
        }
        else
        {
            disk_stat = disk_st;
        }

        if (DISK_ERROR == disk_stat)
        {
            log_e(LOG_DSU, "disk error");
            return;
        }

        if (DISK_FULL == disk_stat)
        {
            if (dsu_cfg.loop)
            {
                log_o(LOG_DSU, "disk full, delete old file");
                dsu_do_diskfull();
            }
            else
            {
                log_o(LOG_DSU, "disk full, stop record");

                if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
                {
                    iwdz_file.close();
                }

                if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
                {
                    iwd_file.close();
                }

                if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
                {
                    inx_file.close();
                }

                return;
            }
        }

        // check file time
        tm_get_abstime(&localtime);

        if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
        {
            if (iwdz_file.c_time.mday != localtime.mday
                || iwdz_file.c_time.mon != localtime.mon
                || iwdz_file.c_time.year != localtime.year)
            {
                iwdz_file.close();
            }
            else
            {
                if (iwdz_attr.hourfile && iwdz_file.c_time.hour != localtime.hour)
                {
                    iwdz_file.close();
                }
                else
                {
                    iwdz_file.sync();
                }
            }
        }

        if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
        {
            iwd_file.sync();
        }

        if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
        {
            if (inx_file.c_time.mday != localtime.mday
                || inx_file.c_time.mon != localtime.mon
                || inx_file.c_time.year != localtime.year)
            {
                inx_file.close();
            }
            else
            {
                if (inx_attr.hourfile && inx_file.c_time.hour != localtime.hour)
                {
                    inx_file.close();
                }
                else
                {
                    inx_file.sync();
                }
            }
        }
    }
    else if (INX_FILE_TIMER == time_id)
    {
        inx_attr.suspend = 0;
    }
}

static void dsu_gps_callback(unsigned int event, unsigned int arg1, unsigned int arg2)
{
    if (DISK_OK != disk_stat)
    {
        return;
    }

    if (GPS_EVENT_DATAIN == event)
    {
        if (iwdz_attr.enable && CANBUS_ACTIVE == canbus_stat)
        {
            iwdz_file_append(IWDZ_DATA_GNSS, (unsigned char *) arg1, arg2);
        }
    }
}

static int dsu_can_callback(unsigned int event, unsigned int arg1, unsigned int arg2)
{
    CAN_MSG *canmsg = (CAN_MSG *) arg1;
    volatile static unsigned int g_cantag_bak1 = 0; //inx
    volatile static unsigned int g_cantag_bak2 = 0; //iwd
    static unsigned int uptime = 0;
    /* fix bug#79, start inx when recv raw can data. */
    //0:can timeout,1:can active,but not recv raw can,2: can active and recv raw can
    static int inx_start = 0;
    /* fix bug#85, record data to tmp_buf when disk full */
    bool is_rec = (DISK_OK == disk_stat) ? true : false;

    if (DISK_FULL == disk_stat && dsu_cfg.loop)
    {
        is_rec = true;
    }

    if (CAN_EVENT_DATAIN == event)
    {
        assert((int)arg2 > 0);

        if (canmsg[arg2 - 1].type == 'T')
        {
            if (canmsg[arg2 - 1].uptime - uptime > 1 && uptime != 0)
            {
                log_w(LOG_DSU, "can tag miss[%u,%u]", canmsg[arg2 - 1].uptime, uptime);
            }

            if (inx_start == 0)
            {
                inx_start = 1;
            }

            if (arg2 > 1)
            {
                inx_start = 2;
            }

            // handle iwdz file
            if (iwdz_attr.enable && is_rec)
            {
                if (arg2 > 1)
                {
                    iwdz_file_append(IWDZ_DATA_CAN, (unsigned char *) arg1, (arg2 - 1) * sizeof(CAN_MSG));
                }
            }

            // handle iwd file
            if (iwd_attr.enable && is_rec)
            {
                if ((IWD_LOGCAN_UNDEFINE != iwd_attr.countdown) && (0 <= iwd_attr.countdown))
                {
                    if (((g_cantag_cnt % 100) == 0) && (g_cantag_cnt != g_cantag_bak2))
                    {
                        log_i(LOG_DSU, "iwd countdown:%d,stat:%u", iwd_attr.countdown, iwd_attr.stat);
                        g_cantag_bak2 = g_cantag_cnt;
                        iwd_attr.countdown--;
                    }
                }

                iwd_file_append((unsigned char *) arg1, (arg2 - 1) * sizeof(CAN_MSG));
            }

            // handle inx file
            if (inx_attr.enable && is_rec)
            {
                assert(inx_attr.sampling != 0);

                if (((g_cantag_cnt % (DSU_G_CANTAG_HZ / inx_attr.sampling)) == 0)
                    && (g_cantag_cnt != g_cantag_bak1))
                {
                    g_cantag_bak1 = g_cantag_cnt;

                    if (inx_start != 1)
                    {
                        inx_file_append();
                    }
                }

                if (inx_attr.sampling > 10)
                {
                    if (g_cantag_cnt % 10 == 0)
                    {
                        dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_SYNC);
                    }
                }
            }

            g_cantag_cnt++;
            can_get_time(canmsg[arg2 - 1].uptime, &g_cantag_time);
        }
        else
        {
            inx_start = 2;

            // handle iwdz file
            if (iwdz_attr.enable && is_rec)
            {
                iwdz_file_append(IWDZ_DATA_CAN, (unsigned char *) arg1, arg2 * sizeof(CAN_MSG));
            }

            // handle iwd file
            if (iwd_attr.enable && is_rec)
            {
                iwd_file_append((unsigned char *) arg1, arg2 * sizeof(CAN_MSG));
            }
        }
    }
    else if (CAN_EVENT_ACTIVE == event)
    {
        canbus_stat = CANBUS_ACTIVE;
        iwd_attr.countdown = iwd_attr.logtime;
    }
    else if (CAN_EVENT_TIMEOUT == event)
    {
        canbus_stat = CANBUS_TIMEOUT;
        inx_start = 0;

        if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
        {
            iwdz_file_append(IWDZ_DATA_END, NULL, 0);
        }

        if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
        {
            inx_file_append();
        }
    }
    else if (CAN_EVENT_SLEEP == event)
    {
        inx_start = 0;

        if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
        {
            dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
        }

        if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
        {
            dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CLOSE);
        }

        if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
        {
            dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CLOSE);
        }
    }

    return 0;
}

static int dsu_dbc_callback(unsigned int event, unsigned int arg1, unsigned int arg2)
{
    int ret = 0;
    static IWD_FAULT_INFO_T *fault_rld = NULL;

    switch (event)
    {
        case DBC_EVENT_RELOAD:
            fault_rld = iwd_fault_inf == NULL ? iwd_fault_infmem : iwd_fault_inf->next;
            fault_rld->cnt = 0;
            break;

        case DBC_EVENT_FINISHED:
            if (0 == arg1)
            {
                if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
                {
                    // inx file record mode
                    ret |= dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
                    log_i(LOG_DSU, "dbc load finish,close inx file");
                    inx_attr.suspend = 1;
                    tm_start(inx_timer, INX_FILE_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);
                }

                inx_ch_init();

                if (fault_rld)
                {
                    iwd_fault_list_setting(fault_rld);

                    if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
                    {
                        // iwd file record mode
                        ret |= dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CLOSE);
                        log_i(LOG_DSU, "dbc load finish,close iwd file");
                    }
                }
            }

            break;

        case DBC_EVENT_SURFIX:
            if (fault_rld)
            {
                ret = iwd_fault_dbc_surfix(fault_rld, (int)arg1, (const char *)arg2);
            }

            break;

        case DBC_EVENT_UPDATE:
            if (iwd_attr.enable)
            {
                iwd_fault_ch_happen((int)arg1, arg2);
            }

            break;

        default:
            break;
    }

    return ret;
}

static void dsu_file_disk_ok(unsigned int msg_id, DSU_FILE *file)
{
    switch (msg_id)
    {
        case DSU_MSG_CREATE_FILE:

            if (FR_OK != file->create())
            {
                disk_stat = DISK_ERROR;
            }

            break;

        case DSU_MSG_CLOSE_FILE:
            if (file->fp)
            {
                file->close();
            }

            break;

        case DSU_MSG_WRITE_FILE:
            if (file->fp)
            {
                file->write();
            }

            break;

        case DSU_MSG_SYNC_FILE:
            if (file->fp)
            {
                file->sync();
            }

            break;

        default:
            break;
    }

}

static void dsu_file_disk_err(unsigned int msg_id, DSU_FILE *file)
{
    switch (msg_id)
    {
        case DSU_MSG_CREATE_FILE:
        case DSU_MSG_CLOSE_FILE:
        case DSU_MSG_WRITE_FILE:
        case DSU_MSG_SYNC_FILE:
            if (file->fp)
            {
                file->close();
            }

            DSU_LOCK();
            file->stat = FILE_ST_NONE;
            DSU_UNLOCK();
            break;

        default:
            break;
    }

}

static void dsu_file_handler(unsigned int msg_id, DSU_FILE_TYPE type)
{
    DSU_FILE *file;

    if (DSU_FILE_INX == type)
    {
        file = &inx_file;
    }
    else if (DSU_FILE_IWD == type)
    {
        file = &iwd_file;
    }
    else if (DSU_FILE_IWDZ == type)
    {
        file = &iwdz_file;
    }
    else
    {
        return;
    }

    if (DISK_ERROR == disk_stat)
    {
        log_e(LOG_DSU, "disk error, msgid:0x%x", msg_id);
        dsu_file_disk_err(msg_id, file);
    }
    else
    {
        dsu_file_disk_ok(msg_id, file);
    }
}

static int dsu_cfg_init(void)
{
    int ret = 0;
    unsigned int len;

    len = sizeof(dsu_cfg.canlog_time);

    if (cfg_get_para(CFG_ITEM_DSU_CANLOG_TIME, &dsu_cfg.canlog_time, &len))
    {
        dsu_cfg.canlog_time = 30;
        cfg_set_para_im(CFG_ITEM_DSU_CANLOG_TIME, (unsigned char *) &dsu_cfg.canlog_time,
                        sizeof(dsu_cfg.canlog_time));
    }

    len = sizeof(dsu_cfg.canlog_mode);

    if (cfg_get_para(CFG_ITEM_DSU_CANLOG_MODE, &dsu_cfg.canlog_mode, &len))
    {
        dsu_cfg.canlog_mode = 0x0;
        cfg_set_para_im(CFG_ITEM_DSU_CANLOG_MODE, (unsigned char *) &dsu_cfg.canlog_mode,
                        sizeof(dsu_cfg.canlog_mode));
    }

    len = sizeof(dsu_cfg.hour);

    if (cfg_get_para(CFG_ITEM_DSU_HOURFILE, &dsu_cfg.hour, &len))
    {
        dsu_cfg.hour = 0x2;     // iwdz one hour a file
        cfg_set_para_im(CFG_ITEM_DSU_HOURFILE, (unsigned char *) &dsu_cfg.hour,
                        sizeof(dsu_cfg.hour));
    }

    len = sizeof(dsu_cfg.loop);

    if (cfg_get_para(CFG_ITEM_DSU_LOOPFILE, &dsu_cfg.loop, &len))
    {
        dsu_cfg.loop = 1;
        cfg_set_para_im(CFG_ITEM_DSU_LOOPFILE, (unsigned char *) &dsu_cfg.loop,
                        sizeof(dsu_cfg.loop));
    }

    len = sizeof(dsu_cfg.sdhz);

    if (cfg_get_para(CFG_ITEM_DSU_SDHZ, &dsu_cfg.sdhz, &len) || 0 == dsu_cfg.sdhz || 100 < dsu_cfg.sdhz)
    {
        dsu_cfg.sdhz = 1;
        cfg_set_para_im(CFG_ITEM_DSU_SDHZ, (unsigned char *) &dsu_cfg.sdhz, sizeof(dsu_cfg.sdhz));
    }

    /* monitor dsu file Config */
    ret = cfg_register(CFG_ITEM_DSU_CANLOG_TIME, dsu_cfg_changed);
    ret |= cfg_register(CFG_ITEM_DSU_CANLOG_MODE, dsu_cfg_changed);
    ret |= cfg_register(CFG_ITEM_DSU_HOURFILE, dsu_cfg_changed);
    ret |= cfg_register(CFG_ITEM_DSU_LOOPFILE, dsu_cfg_changed);
    ret |= cfg_register(CFG_ITEM_DSU_AUTHKEY, dsu_cfg_changed);
    ret |= cfg_register(CFG_ITEM_DSU_SDHZ, dsu_cfg_changed);

    inx_attr_init(&dsu_cfg);
    iwd_attr_init(&dsu_cfg);
    iwdz_attr_init(&dsu_cfg);

    return ret;
}

static int dsu_timer_init(void)
{
    int ret = 0;
    ret = tm_create(TIMER_REL, DSU_TIMER, MPU_MID_DSU, &dsu_timer);

    if (ret != 0)
    {
        log_e(LOG_DSU, "create timer failed, ret:0x%08x", ret);
        return ret;
    }

    ret = tm_create(TIMER_REL, INX_FILE_TIMER, MPU_MID_DSU, &inx_timer);

    if (ret != 0)
    {
        log_e(LOG_DSU, "create timer failed, ret:0x%08x", ret);
        return ret;
    }

    ret = tm_start(dsu_timer, DSU_TIMER_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);
    return ret;
}

/****************************************************************
 * function:     dsu_init
 * description:  initiaze DataStorageUnit module
 * input:        none
 * output:       none
 * return:       0 indicates success;
 *               others indicates failed
 *****************************************************************/
int dsu_init(INIT_PHASE phase)
{
    int ret = 0;
    log_o(LOG_DSU, "init dsu thread");

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&dsu_mutex, NULL);
            dsu_compress_init();
            ret |= inx_file_init();
            ret |= iwdz_file_init();
            ret |= iwd_file_init();
            dsu_sleep = 0;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            disk_stat = dsu_dir_check();
            ret |= dsu_cfg_init();
            ret |= dsu_timer_init();
            ret |= can_register_callback(dsu_can_callback);
            ret |= dbc_register_callback(dsu_dbc_callback);
            ret |= gps_reg_callback(dsu_gps_callback);
            ret |= dsu_shell_init();
            ret |= pm_reg_handler(MPU_MID_DSU, dsu_sleep_handler);
            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
 * function:     dsu_main
 * description:  DataStorageUnit module main function
 * input:        none
 * output:       none
 * return:       NULL
 ****************************************************************/
static void *dsu_main(void)
{
    int maxfd = 0;
    int ret;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;

    prctl(PR_SET_NAME, "DSU");

    maxfd = tcom_get_read_fd(MPU_MID_DSU);

    if (maxfd < 0)
    {
        return NULL;
    }

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(maxfd, &fds);

        /* monitor the incoming data */
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret)
        {
            if (FD_ISSET(maxfd, &fds))
            {
                memset(dsu_msgbuf, 0, sizeof(dsu_msgbuf));

                if (0 == tcom_recv_msg(MPU_MID_DSU, &msgheader, dsu_msgbuf))
                {
                    if (MPU_MID_TIMER == msgheader.sender)
                    {
                        dsu_timer_handler(msgheader.msgid);
                    }
                    else if (MPU_MID_MID_PWDG == msgheader.msgid)
                    {
                        pwdg_feed(MPU_MID_DSU);
                    }
                    else if (MPU_MID_PM == msgheader.sender)
                    {
                        switch (msgheader.msgid)
                        {
                            case PM_MSG_SLEEP:
                            case PM_MSG_EMERGENCY:
                            case PM_MSG_OFF:
                                dsu_do_sleep();
                                break;

                            case PM_MSG_RUNNING:
                                dsu_do_wakeup();
                                break;

                            default:
                                break;
                        }
                    }
                    else
                    {
                        dsu_file_handler(msgheader.msgid, (DSU_FILE_TYPE)dsu_msgbuf[0]);
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

            log_e(LOG_DSU, "DSU thread exit abnormally!");
            break; /* thread exit abnormally */
        }
    }

    return 0;
}

/****************************************************************
 * function:     dsu_run
 * description:  startup dsu module
 * input:        none
 * output:       none
 * return:       positive value indicates success;
 *               -1 indicates failed
 *****************************************************************/
int dsu_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&dsu_tid, &ta, (void *) dsu_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

void dsu_get_cantag_time(RTCTIME *time)
{
    memcpy((void *) time, (void *) &g_cantag_time, sizeof(RTCTIME));
}

unsigned int dsu_get_cantag_cnt(void)
{
    return g_cantag_cnt;
}

int dsu_suspend_record(void)
{
    int all_closed = 0;
    inx_attr.enable = iwdz_attr.enable = iwd_attr.enable = 0;

    if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
    {
        all_closed |= DSU_CFG_IWDZ_MASK;
        dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close iwdz file");
    }

    if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
    {
        all_closed |= DSU_CFG_INX_MASK;
        dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close inx file");
    }

    if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
    {
        all_closed |= DSU_CFG_IWD_MASK;
        dsu_set_opt(&inx_file, DSU_FILE_IWD, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close iwd file");
    }

    return all_closed;
}

void dsu_resume_record(void)
{
    dsu_dir_check();
    inx_attr.enable  = dsu_cfg.canlog_mode & DSU_CFG_INX_MASK;
    iwdz_attr.enable = dsu_cfg.canlog_mode & DSU_CFG_IWDZ_MASK;
    iwd_attr.enable  = dsu_cfg.canlog_mode & DSU_CFG_IWD_MASK;
}

