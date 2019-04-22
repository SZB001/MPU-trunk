/**
 * @Title: dsu_iwdzfile.c
 * @author yuzhimin
 * @date Nov 9, 2017
 * @version V1.0
 */

#include "dsu_main.h"

#define IWDZ_TMP_DATA_SIZE              (1024*640)
#define IWDZ_TMP_DATA_RSV_SIZE          (1024*64)
#define IWDZ_TMP_CNT                    (2)
#define IWDZ_FILE_BUF                   (1024*1024)
#define IWDZ_TMP_TIMEOUT                (1000*2)        // 2S
/* tmp data structure */
typedef struct
{
    unsigned char data[IWDZ_TMP_CNT][IWDZ_TMP_DATA_SIZE];
    unsigned int len[IWDZ_TMP_CNT];
    unsigned char cur_idx;
    unsigned char sync_idx;
    pthread_mutex_t mutex;
} IWDZ_TMP;

DSU_FILE iwdz_file;
IWDZ_ATTR iwdz_attr;

static IWDZ_TMP iwdz_tmp;
static unsigned char iwdz_fbuf[IWDZ_FILE_BUF];

static int iwdz_file_create(void)
{
    unsigned char buf[128];
    unsigned int sn = 0, len;
    int ret;
    static unsigned int rebootIndex = 1;

    len = sizeof(sn);
    cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char *) &sn, &len);

    tm_get_abstime(&iwdz_file.c_time);

    if (NULL != iwdz_file.fp)
    {
        dsu_fclose(&iwdz_file);
    }

    /* decide the file name */
    if (dev_is_time_syn())
    {
        snprintf(iwdz_file.name, sizeof(iwdz_file.name),
                 "%s/%04u%02d%02d_%02d%02d%02d(%u_%d).iwdz", DSU_IWDZ_FILE_PATH,
                 iwdz_file.c_time.year, iwdz_file.c_time.mon, iwdz_file.c_time.mday,
                 iwdz_file.c_time.hour, iwdz_file.c_time.min, iwdz_file.c_time.sec,
                 dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
        log_i(LOG_DSU, "Create %s!", iwdz_file.name);
    }
    else
    {
        snprintf(iwdz_file.name, sizeof(iwdz_file.name), "%s/%d(%u@%d).iwdz", DSU_IWDZ_FILE_PATH,
                 sn, dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
        log_i(LOG_DSU, "Create %s!", iwdz_file.name);
    }

    /* Open or create a file */
    ret = dsu_fcreate(&iwdz_file, RAW_DATA, iwdz_file.name, "wb+");

    if (FR_OK != ret)
    {
        log_e(LOG_DSU, "dsu_fcreate failed!");
        return ret;
    }

    /*
     the structure of iwdz file header:
     -------------------------------------
     | INZ Header(23 byte, no compress)  |
     | IWD Header(26 bytes, compress)    |
     -------------------------------------
     */
    memset(buf, 0, sizeof(buf));
    len = dsu_build_head_inz(buf, sizeof(buf), sn);
    ret = dsu_write_fb(&iwdz_file, buf, len);      // write file no compress

    ret |= dsu_change_fb_type(&iwdz_file, ZLIB_DATA);
    memset(buf, 0, sizeof(buf));
    len = dsu_build_head_iwd(buf, sizeof(buf), IWDZ_VER_STR, sn, &iwdz_file.c_time, MCU_TICK_CLK);
    ret |= dsu_write_fb(&iwdz_file, buf, len);       // write to buf,and wait fsync() to compress
    return ret;
}

static int iwdz_file_write(void)
{
    int ret;
    unsigned char idx;
    unsigned int *len;
    unsigned char *ptr;

    pthread_mutex_lock(&iwdz_tmp.mutex);
    idx = iwdz_tmp.sync_idx;
    len = &iwdz_tmp.len[idx];
    ptr = iwdz_tmp.data[idx];

    if (0 < *len)
    {
        ret = dsu_write_fb(&iwdz_file, ptr, *len);

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_write_fb error, len=%u", *len);
        }

        if (tmp_max_len[1] < *len)
        {
            tmp_max_len[1] = *len;
        }

        *len = 0;
    }

    pthread_mutex_unlock(&iwdz_tmp.mutex);

    if (dsu_fb_used(&iwdz_file) + 1024 > DSU_ZBUF_SIZE)
    {
        dsu_fsync(&iwdz_file);
    }

    return ret;
}

static int iwdz_file_sync(void)
{
    return dsu_fsync(&iwdz_file);
}

static int iwdz_file_close(void)
{
    int ret = 0;
    unsigned char idx;
    unsigned int *len;
    unsigned char *ptr;

    ret |= dsu_fsync(&iwdz_file);
    pthread_mutex_lock(&iwdz_tmp.mutex);
    idx = iwdz_tmp.sync_idx;
    len = &iwdz_tmp.len[idx];
    ptr = iwdz_tmp.data[idx];

    if (0 < *len)
    {
        ret = dsu_write_fb(&iwdz_file, ptr, *len);

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_write_fb error, len=%u", *len);
        }

        *len = 0;
    }

    pthread_mutex_unlock(&iwdz_tmp.mutex);
    ret |= dsu_fclose(&iwdz_file);
    return ret;
}

void iwdz_attr_init(DSU_CFG_T *cfg)
{
    memset(&iwdz_attr, 0, sizeof(iwdz_attr));
    iwdz_attr.enable = cfg->canlog_mode & DSU_CFG_IWDZ_MASK;
    iwdz_attr.hourfile = cfg->hour & DSU_CFG_IWDZ_MASK;
}

int iwdz_file_init(void)
{
    memset(&iwdz_tmp, 0, sizeof(iwdz_tmp));
    iwdz_tmp.cur_idx = 0;
    iwdz_tmp.sync_idx = (iwdz_tmp.cur_idx + 1) % IWDZ_TMP_CNT;

    pthread_mutex_init(&iwdz_tmp.mutex, NULL);
    iwdz_file.create = iwdz_file_create;
    iwdz_file.write = iwdz_file_write;
    iwdz_file.sync = iwdz_file_sync;
    iwdz_file.close = iwdz_file_close;
    return dsu_file_init(&iwdz_file, iwdz_fbuf, sizeof(iwdz_fbuf), RAW_DATA);
}

/* append data into file */
int iwdz_file_append(unsigned int type, unsigned char *data, unsigned int len)
{
    static unsigned char gnss_header[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 'G', 0 };
    static long long time = 0;
    pthread_mutex_lock(&iwdz_tmp.mutex);

    // check close file
    if (!iwdz_attr.enable || type == IWDZ_DATA_END)
    {
        if (iwdz_tmp.len[iwdz_tmp.cur_idx])
        {
            iwdz_tmp.sync_idx = iwdz_tmp.cur_idx;
            iwdz_tmp.cur_idx = (iwdz_tmp.cur_idx + 1) % IWDZ_TMP_CNT;
            iwdz_tmp.len[iwdz_tmp.cur_idx] = 0;
        }

        if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
        {
            dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CLOSE);
            log_i(LOG_DSU, "close iwdz file");
        }
        else
        {
            iwdz_tmp.len[iwdz_tmp.cur_idx] = 0;
            iwdz_tmp.len[iwdz_tmp.sync_idx] = 0;
        }

        pthread_mutex_unlock(&iwdz_tmp.mutex);
        return 0;
    }
    else
    {
        if (FILE_ST_NONE == dsu_get_stat(&iwdz_file) && IWDZ_DATA_CAN == type)
        {
            dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CREATE);
            time = tm_get_time();
            log_i(LOG_DSU, "creat iwdz file");
        }
    }

    // check tmp buf
    if (iwdz_tmp.len[iwdz_tmp.cur_idx] > IWDZ_TMP_DATA_RSV_SIZE
        || (tm_get_time() - time > IWDZ_TMP_TIMEOUT))
    {
        dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_WRITE);
        time = tm_get_time();

        if (0 == iwdz_tmp.len[iwdz_tmp.sync_idx])
        {
            iwdz_tmp.sync_idx = iwdz_tmp.cur_idx;
            iwdz_tmp.cur_idx = (iwdz_tmp.cur_idx + 1) % IWDZ_TMP_CNT;
        }
        else
        {
            if (iwdz_tmp.len[iwdz_tmp.cur_idx] + len > IWDZ_TMP_DATA_SIZE)
            {
                log_e(LOG_DSU, "tmp_buf[%d]=[%d],need[%d], overflow",
                      iwdz_tmp.cur_idx, iwdz_tmp.len[iwdz_tmp.cur_idx], len);
                pthread_mutex_unlock(&iwdz_tmp.mutex);
                return 0;
            }
        }
    }

    pthread_mutex_unlock(&iwdz_tmp.mutex);

    // save data
    if (type == IWDZ_DATA_CAN)
    {
        memcpy(gnss_header, data, 8);      // CAN_MSG timestamp
        memcpy(iwdz_tmp.data[iwdz_tmp.cur_idx] + iwdz_tmp.len[iwdz_tmp.cur_idx], data, len);
        iwdz_tmp.len[iwdz_tmp.cur_idx] += len;
    }
    else    // gnss data
    {
        gnss_header[9] = (unsigned char)(len & 0xFF);
        memcpy(iwdz_tmp.data[iwdz_tmp.cur_idx] + iwdz_tmp.len[iwdz_tmp.cur_idx], gnss_header,
               sizeof(gnss_header));
        iwdz_tmp.len[iwdz_tmp.cur_idx] += sizeof(gnss_header);
        memcpy(iwdz_tmp.data[iwdz_tmp.cur_idx] + iwdz_tmp.len[iwdz_tmp.cur_idx], data, len);
        iwdz_tmp.len[iwdz_tmp.cur_idx] += len;
    }

    return 0;
}

