/**
 * @Title: dsu_iwdfile.c
 * @author yuzhimin
 * @date Nov 21, 2017
 * @version V1.0
 */
#include "dsu_main.h"

#define DSU_IWDSTAT_LOCK()         do{pthread_mutex_lock(&iwd_attr.mutex);}while(0)
#define DSU_IWDSTAT_UNLOCK()       do{pthread_mutex_unlock(&iwd_attr.mutex);}while(0)


#define IWD_TMP_DATA_SIZE              (1024*1024)
#define IWD_TMP_DATA_RSV_SIZE          (1024*64)
#define IWD_FILE_BUF                   (64)

/* tmp data structure */
typedef struct
{
    unsigned char data[IWD_TMP_DATA_SIZE];
    unsigned int len;
    pthread_mutex_t mutex;
} IWD_TMP;

DSU_FILE iwd_file;
IWD_ATTR iwd_attr;
IWD_FAULT_INFO_T *iwd_fault_inf = NULL;
IWD_FAULT_INFO_T  iwd_fault_infmem[2];

static IWD_TMP  iwd_tmp;
static unsigned char iwd_fbuf[IWD_FILE_BUF];

static int iwd_file_close(void);
static int iwd_file_write(void);
static int iwd_file_sync(void);
static int iwd_file_rename(char *oldname, char *newname);

int iwd_file_create(void)
{
    RTCTIME localtime;
    unsigned char buf[64];
    unsigned int sn = 0, len;
    int ret;
    long offset;
    static unsigned int rebootIndex = 1;
    unsigned char stat;
    DSU_IWDSTAT_LOCK();
    stat = iwd_attr.stat;
    DSU_IWDSTAT_UNLOCK();

    if (stat == 0) // create tmp file
    {
        if (NULL != iwd_file.fp)
        {
            log_e(LOG_DSU, "stat[0],close file:%s!", iwd_file.name);
            dsu_fclose(&iwd_file);
        }

        tm_get_abstime(&localtime);
        len = sizeof(sn);
        cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char *) &sn, &len);
        snprintf(iwd_file.name, sizeof(iwd_file.name), "%s/%s", DSU_IWD_FILE_PATH, DSU_IWD_TMP_FILE_NAME);
        /* Open or create a file */
        ret = dsu_fcreate(&iwd_file, RAW_DATA, iwd_file.name, "wb");

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_fcreate failed!");
            DSU_LOCK();
            iwd_file.stat = FILE_ST_NONE;
            DSU_UNLOCK();
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 0;
            DSU_IWDSTAT_UNLOCK();
            return ret;
        }

        /*
         the structure of iwd file header:
         -------------------------------------
         | IWD Header(26 bytes, no compress) |
         -------------------------------------
        */
        memset(buf, 0, sizeof(buf));
        len = dsu_build_head_iwd(buf, sizeof(buf), IWD_VER_STR, sn, &localtime, MCU_TICK_CLK);
        ret |= dsu_write_fb(&iwd_file, buf, len);       // write to buf,and wait fsync()
        ret |= dsu_fsync(&iwd_file);
        iwd_attr.offset = ftell(iwd_file.fp);
        log_i(LOG_DSU, "iwd header offset %ld!", iwd_attr.offset);

        if (0 > iwd_attr.offset)
        {
            DSU_LOCK();
            iwd_file.stat = FILE_ST_NONE;
            DSU_UNLOCK();
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 0;
            DSU_IWDSTAT_UNLOCK();
            log_e(LOG_DSU, "ftell error:%s", strerror(ferror(iwd_file.fp)));
            return FR_ERR_FTELL;
        }

        DSU_IWDSTAT_LOCK();
        iwd_attr.stat = 1;
        DSU_IWDSTAT_UNLOCK();
    }
    else if (stat == 1) // 移动tmp文件写指针到IWD文件头信息的结尾
    {
        if (NULL != iwd_file.fp)
        {
            iwd_file_sync();
            offset = ftell(iwd_file.fp);
            dsu_fclose(&iwd_file);
            ret = truncate(iwd_file.name, offset);
            ret |= dsu_fcreate(&iwd_file, RAW_DATA, iwd_file.name, "rb+");

            if (FR_OK != ret)
            {
                DSU_LOCK();
                iwd_file.stat = FILE_ST_NONE;
                DSU_UNLOCK();
                DSU_IWDSTAT_LOCK();
                iwd_attr.stat = 0;
                DSU_IWDSTAT_UNLOCK();
                log_e(LOG_DSU, "dsu_fcreate failed!");
                return ret;
            }

            ret = fseek(iwd_file.fp, iwd_attr.offset, SEEK_SET);

            if (0 > ret)
            {
                DSU_LOCK();
                iwd_file.stat = FILE_ST_NONE;
                DSU_UNLOCK();
                DSU_IWDSTAT_LOCK();
                iwd_attr.stat = 0;
                DSU_IWDSTAT_UNLOCK();
                log_e(LOG_DSU, "fseek error:%s", strerror(ferror(iwd_file.fp)));
                return FR_ERR_FSEEK;
            }
        }
        else
        {
            DSU_LOCK();
            iwd_file.stat = FILE_ST_NONE;
            DSU_UNLOCK();
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 0;
            DSU_IWDSTAT_UNLOCK();
            log_e(LOG_DSU, "tmp.iwd fp is NULL");
            return FR_ERR;
        }
    }
    else if (stat == 2) //将tmp文件更名为iwd文件
    {
        if (NULL != iwd_file.fp)
        {
            memset(buf, 0, sizeof(buf));
            strncpy((char *)buf, iwd_file.name, sizeof(buf));
            RTCTIME *time = &iwd_attr.cantag;

            /* decide the file name */
            if (dev_is_time_syn())
            {
                snprintf(iwd_file.name, sizeof(iwd_file.name),
                         "%s/%04d%02d%02d_%02d%02d%02d(%u_%d).iwd", DSU_IWD_FILE_PATH, time->year,
                         time->mon, time->mday, time->hour, time->min, time->sec,
                         dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
                log_i(LOG_DSU, "Create %s!", iwd_file.name);
            }
            else
            {
                snprintf(iwd_file.name, sizeof(iwd_file.name), "%s/%d(%u@%d).iwd", DSU_IWD_FILE_PATH, sn,
                         dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
                log_i(LOG_DSU, "Create %s!", iwd_file.name);
            }

            iwd_file_rename((char *)buf, iwd_file.name);
            ret = dsu_fcreate(&iwd_file, RAW_DATA, iwd_file.name, "ab+");
        }
        else
        {
            DSU_LOCK();
            iwd_file.stat = FILE_ST_NONE;
            DSU_UNLOCK();
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 0;
            DSU_IWDSTAT_UNLOCK();
            log_e(LOG_DSU, "iwd fp is NULL");
            return FR_ERR;
        }
    }

    return ret;
}

static int iwd_file_write(void)
{
    int ret;
    pthread_mutex_lock(&iwd_tmp.mutex);

    if (0 < iwd_tmp.len)
    {
        ret = _dsu_fwrite(iwd_file.fp, iwd_tmp.data, iwd_tmp.len);

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_write_fb error, len=%u", iwd_tmp.len);
        }

        if (tmp_max_len[2] < iwd_tmp.len)
        {
            tmp_max_len[2] = iwd_tmp.len;
        }

        iwd_tmp.len = 0;
    }

    pthread_mutex_unlock(&iwd_tmp.mutex);
    return ret;
}

static int iwd_file_sync(void)
{
    int ret;
    ret = dsu_fsync(&iwd_file);  //sync file buf first;
    ret |= iwd_file_write();     //sync tmp buf second;
    return ret;
}

static int iwd_file_close(void)
{
    int ret;
    ret = iwd_file_sync();
    ret |= dsu_fclose(&iwd_file);
    return ret;
}

static int iwd_file_rename(char *oldname, char *newname)
{
    int ret;
    ret = iwd_file_sync();
    fflush(iwd_file.fp);
    fclose(iwd_file.fp);
    log_o(LOG_DSU, "file close:%s", oldname);
    iwd_file.fp = NULL;
    file_rename(oldname, newname);
    return ret;
}

static int iwd_file_check(void)
{
    int ret = IWD_CONTINUE_REC;
    unsigned char stat;
    DSU_IWDSTAT_LOCK();
    stat = iwd_attr.stat;
    DSU_IWDSTAT_UNLOCK();

    if ((IWD_LOGCAN_UNDEFINE != iwd_attr.countdown) && (0 > iwd_attr.countdown))
    {
        if (canbus_stat == CANBUS_TIMEOUT)
        {
            iwd_attr.countdown = IWD_LOGCAN_UNDEFINE;
        }
        else
        {
            iwd_attr.countdown = iwd_attr.logtime;
        }

        if (2 == stat)
        {
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 0;
            DSU_IWDSTAT_UNLOCK();
            ret = IWD_CLOSE_IWD_FILE;  // iwd file timeup, close it
        }
        else if (1 == stat)
        {
            ret = IWD_REWIND_TMP_FILE; // tmp file timeup, rewind it
        }
        else
        {
            ret = IWD_CONTINUE_REC;
        }
    }

    return ret;
}

int iwd_file_init(void)
{
    memset(&iwd_tmp, 0, sizeof(iwd_tmp));
    pthread_mutex_init(&iwd_tmp.mutex, NULL);
    iwd_file.create = iwd_file_create;
    iwd_file.write = iwd_file_write;
    iwd_file.sync = iwd_file_sync;
    iwd_file.close = iwd_file_close;
    return dsu_file_init(&iwd_file, iwd_fbuf, sizeof(iwd_fbuf), RAW_DATA);
}

int iwd_file_append(unsigned char *data, unsigned int len)
{
    int ret = iwd_file_check();

    // check iwd file
    if (IWD_CLOSE_IWD_FILE == ret)
    {
        dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close iwd file, tmp buf len:%u", iwd_tmp.len);
        return 0;
    }
    else if (IWD_REWIND_TMP_FILE == ret)
    {
        dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CREATE);
        log_i(LOG_DSU, "rewind iwd tmp file, tmp buf len:%u,", iwd_tmp.len);
    }
    else
    {
        if (FILE_ST_NONE == dsu_get_stat(&iwd_file))
        {
            dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CREATE);
            log_e(LOG_DSU, "creat iwd file stat:%u", iwd_attr.stat);
        }
    }

    pthread_mutex_lock(&iwd_tmp.mutex);

    if (iwd_tmp.len + len > IWD_TMP_DATA_SIZE)
    {
        dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_WRITE);
        log_e(LOG_DSU, "tmp_buf=[%d],need[%d] overflow", iwd_tmp.len, len);
        pthread_mutex_unlock(&iwd_tmp.mutex);
        return 0;
    }

    memcpy(iwd_tmp.data + iwd_tmp.len, data, len);
    iwd_tmp.len += len;
    pthread_mutex_unlock(&iwd_tmp.mutex);

    if (iwd_tmp.len > IWD_TMP_DATA_RSV_SIZE)
    {
        dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_WRITE);
    }

    return 0;
}


int iwd_fault_dbc_surfix(IWD_FAULT_INFO_T *faultinf, int sigid, const char *sfx)
{
    unsigned int flags;

    assert(sigid > 0 && sfx != NULL);

    if (*sfx == 'E' && sscanf(sfx + 1, "%2x", &flags) == 1)
    {
        log_i(LOG_DSU, "find EXX %d,flags=%d", sigid, flags);

        if (flags & 0x1)
        {
            if (faultinf->cnt >= IWD_FAULT_CH_MAX)
            {
                log_e(LOG_DSU, "iwd fault channel is overflow(maximum is %d)", IWD_FAULT_CH_MAX);
            }
            else
            {
                faultinf->siglist[faultinf->cnt++] = sigid;
            }
        }

        return 3;
    }

    return 0;
}

void iwd_fault_list_setting(IWD_FAULT_INFO_T *faultinf)
{
    int i;

    for (i = 0; i < faultinf->cnt; i++)
    {
        log_i(LOG_DSU, "iwd fault channel is %d,flag:0x%x", faultinf->siglist[i], iwd_attr.fault_flag);
        dbc_set_signal_flag(faultinf->siglist[i], iwd_attr.fault_flag);
    }
}

void iwd_fault_ch_happen(int id, unsigned int uptime)
{
    unsigned char stat;

    if (dbc_test_signal_flag(id, iwd_attr.fault_flag, 0) && 0 != dbc_get_signal_value(id)) //故障发生
    {

        DSU_IWDSTAT_LOCK();
        stat = iwd_attr.stat;
        DSU_IWDSTAT_UNLOCK();

        log_e(LOG_DSU, "iwd file stat:%u", stat);

        if (stat == 1) // 首次故障
        {
            DSU_IWDSTAT_LOCK();
            iwd_attr.stat = 2;
            DSU_IWDSTAT_UNLOCK();
            iwd_attr.countdown = iwd_attr.logtime;
            can_get_time(uptime, &iwd_attr.cantag);
            dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CREATE);
        }
        else if (stat == 2) //故障记录中,且新故障,刷新纪录时间
        {
            if (0 == dbc_get_signal_lastval(id))
            {
                iwd_attr.countdown = iwd_attr.logtime;
            }
        }
        else  // tmp.iwd 未创建
        {
            can_get_time(uptime, &iwd_attr.cantag);
            dsu_set_opt(&iwd_file, DSU_FILE_IWD, FILE_OPT_CREATE);
        }
    }
}

void iwd_attr_init(DSU_CFG_T *cfg)
{
    memset(&iwd_attr, 0, sizeof(iwd_attr));
    iwd_attr.enable = cfg->canlog_mode & DSU_CFG_IWD_MASK;
    iwd_attr.logtime = cfg->canlog_time;
    iwd_attr.countdown = IWD_LOGCAN_UNDEFINE;
    iwd_attr.fault_flag = dbc_request_flag();
    pthread_mutex_init(&iwd_attr.mutex, NULL);

    if (!iwd_attr.fault_flag)
    {
        log_e(LOG_DSU, "iwd get dbc_request_flag() failed");
    }
}

