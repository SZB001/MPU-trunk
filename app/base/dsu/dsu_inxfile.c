/**
 * @Title: dsu_inxfile.c
 * @author yuzhimin
 * @date Nov 9, 2017
 * @version V1.0
 */

#include "dsu_main.h"

#define INX_TMP_DATA_SIZE              (1024*640)
#define INX_TMP_DATA_RSV_SIZE          (1024*64)
#define INX_TMP_CNT                    (2)
#define INX_FILE_BUF                   (1024*1024)
#define INX_TMP_TIMEOUT                (2000)     // 2S
/* tmp data structure */
typedef struct
{
    unsigned char data[INX_TMP_CNT][INX_TMP_DATA_SIZE];
    unsigned int len[INX_TMP_CNT];
    unsigned char cur_idx;
    unsigned char sync_idx;
    pthread_mutex_t mutex;
} INX_TMP;

DSU_FILE inx_file;
INX_ATTR inx_attr;

static INX_TMP inx_tmp;
static unsigned char inx_fbuf[INX_FILE_BUF];
static INX_RT_DATA_T inx_rt;

static IN_CH_T ParaList[INX_CH_PARA_CNT] =
{
    /*  name  des  unit  bits  type   saveType     value */
    { "日期",       "",     "",     0, TYPE_UINT,   STYPE_FLOAT,    &inx_rt.date        },
    { "时间",       "",     "",     0, TYPE_DBLE,   STYPE_FLOAT,    &inx_rt.time        },
    { "经度",       "",     "",     5, TYPE_DBLE,   STYPE_DOUBLE,   &inx_rt.longitude   },
    { "纬度",       "",     "",     5, TYPE_DBLE,   STYPE_DOUBLE,   &inx_rt.latitude    },
    { "海拔",       "",     "m",    1, TYPE_DBLE,   STYPE_FLOAT,    &inx_rt.msl         },
    { "方向",       "",     "",     1, TYPE_DBLE,   STYPE_FLOAT,    &inx_rt.direction   },
    { "GPS车速",    "",     "kmph", 1, TYPE_DBLE,   STYPE_FLOAT,    &inx_rt.kms         },
    { "累计里程",   "",     "km",   1, TYPE_LLONG,  STYPE_DOUBLE,   &inx_rt.gps_odo     },
    { "充放电状态", "",     "",     0, TYPE_UINT,   STYPE_CHAR,     &inx_rt.powerStatus },
    { "累计运行时间", "",   "min",  1, TYPE_LLONG,  STYPE_DOUBLE,   &inx_rt.SYSRunTime  },
    { "CAN总线1",   "",     "",     0, TYPE_UINT,   STYPE_CHAR,     &inx_rt.canBusError[0] },
    { "CAN总线2",   "",     "",     0, TYPE_UINT,   STYPE_CHAR,     &inx_rt.canBusError[1] },
    { "CAN总线3",   "",     "",     0, TYPE_UINT,   STYPE_CHAR,     &inx_rt.canBusError[2] },
    { "GPRS流量",   "",     "Byte", 0, TYPE_LLONG,  STYPE_DOUBLE,   &inx_rt.INGPRSFlowCnt },
};

static IN_CH_T CanList[INX_CH_CAN_CNT];
static unsigned int canlist_cnt = 0;

static char *des_head[INX_EDI_CNT] =
{
    "TestName",
    "TestTime",
    "TestAddr",
    "TestMan",
    "TestTool",
    "Client",
    "ClientAddr",
    "UDTName",
    "",                 // Replace it with the SN Num
    "Notes"
};

static unsigned int inx_ch_baseinfo(IN_CH_T *ch, unsigned char *buf, unsigned int size)
{
    unsigned int len;
    unsigned int pos = 0;

    if (size < INX_CH_BASEINFO_SIZE)
    {
        return -1;
    }

    /* name  50 bytes, 50 */
    len = MIN(strlen((char const *) ch->name), 50);
    memcpy(buf + pos, ch->name, len);
    pos += 50;

    /* color 4 bytes, 54 */
    buf[pos++] = 0x0;
    buf[pos++] = 0x0;
    buf[pos++] = 0x0;
    buf[pos++] = 0xFF;

    /* des 50 bytes, 104 */
    len = MIN(strlen((char const *) ch->des), 50);
    memcpy(buf + pos, ch->name, len);
    pos += 50;

    /* unit 8 bytes 112 */
    len = MIN(strlen((char const *) ch->unit), 8);
    memcpy(buf + pos, ch->unit, len);
    pos += 8;

    /* bits(number of decimal places) 1 byte, 113 */
    buf[pos++] = ch->bits;

    /* type  1 byte, 114*/
    buf[pos++] = ch->saveType;
    return pos;
}

static void inx_rt_update(void)
{
    GPS_DATA gps_data;
    gps_get_snap(&gps_data);
    inx_rt.date             = gps_data.date;
    inx_rt.time             = gps_data.time;
    inx_rt.is_east          = gps_data.is_east;
    inx_rt.is_north         = gps_data.is_north;
    inx_rt.longitude        = gps_data.longitude;
    inx_rt.latitude         = gps_data.latitude;
    inx_rt.msl              = gps_data.msl;
    inx_rt.direction        = gps_data.direction;
    inx_rt.kms              = gps_data.kms;
    inx_rt.gps_odo          = gps_get_distance();
    inx_rt.powerStatus      = dev_get_KL15_signal() << 0 |
                              dev_get_slow_chg_signal() << 1 |
                              dev_get_quick_chg_signal() << 2;
    inx_rt.SYSRunTime       = tm_get_time();
    inx_rt.canBusError[0]   = flt_get_by_id(CAN_BUS1);
    inx_rt.canBusError[1]   = flt_get_by_id(CAN_BUS2);
    inx_rt.canBusError[2]   = flt_get_by_id(CAN_BUS3);
    inx_rt.INGPRSFlowCnt    = 0;    // TODO: no match item
}

static int inx_rt_ch_save(unsigned char *buf, unsigned int size)
{
    float tmpFloat;
    double tmpDouble;
    unsigned char tmpChar;
    unsigned long long tmpllong = 0;
    unsigned int i, num;
    unsigned int no = 0;
    RTCTIME localtime;    //use can tag time

    num = (sizeof(ParaList) / sizeof(ParaList[0]));

    if (num * 8 > size)
    {
        log_i(LOG_DSU, "buf not enough, need=[%u],now=[%u]!", num * 8, size);
        return -1;
    }

    for (i = 0; i < num; i++)
    {
        switch (ParaList[i].type)
        {
            case TYPE_UINT:
                tmpDouble = *(unsigned int *)(ParaList[i].valuePtr);
                break;

            case TYPE_SINT:
                tmpDouble = *(signed int *)(ParaList[i].valuePtr);
                break;

            case TYPE_FLAT:
                tmpDouble = *(float *)(ParaList[i].valuePtr);
                break;

            case TYPE_DBLE:
                tmpDouble = *(double *)(ParaList[i].valuePtr);
                break;

            case TYPE_LLONG:
                tmpllong = *(unsigned long long *)(ParaList[i].valuePtr);
                tmpDouble = *(unsigned long long *)(ParaList[i].valuePtr);
                break;

            default:
                log_w(LOG_DSU, "unknown paratype %d!", ParaList[i].type);
                break;
        }

        /* INTEST GPS distance */
        if (ParaList[i].valuePtr == (void *) &inx_rt.gps_odo)
        {
            tmpDouble = (tmpllong) / (1000 * 3600);
        }
        else if (ParaList[i].valuePtr == (void *) &inx_rt.SYSRunTime)
        {
            tmpDouble = tmpllong / (1000 * 60);
        }
        else if (ParaList[i].valuePtr == (void *) &inx_rt.date)
        {
            dsu_get_cantag_time(&localtime);
            tmpDouble = localtime.mday * 10000 + localtime.mon * 100 + (localtime.year - 2000);
        }
        else if (ParaList[i].valuePtr == (void *) &inx_rt.time)
        {
            dsu_get_cantag_time(&localtime);
            tmpDouble = localtime.hour * 10000 + localtime.min * 100 + localtime.sec;
        }
        else if (ParaList[i].valuePtr == (void *) &inx_rt.longitude)
        {
            if (!inx_rt.is_east)
            {
                tmpDouble = tmpDouble * -1;
            }
        }
        else if (ParaList[i].valuePtr == (void *) &inx_rt.latitude)
        {
            if (!inx_rt.is_north)
            {
                tmpDouble = tmpDouble * -1;
            }
        }

        switch (ParaList[i].saveType)
        {
            case STYPE_DOUBLE:
                memcpy(&buf[no], &tmpDouble, 8);
                no += 8;
                break;

            case STYPE_FLOAT:
                tmpFloat = (float) tmpDouble;
                memcpy(&buf[no], &tmpFloat, 4);
                no += 4;
                break;

            case STYPE_CHAR:
                tmpChar = (unsigned char) tmpDouble;
                memcpy(&buf[no], &tmpChar, 1);
                no += 1;
                break;

            default:
                log_w(LOG_DSU, "unknow savetype:%d!", ParaList[i].saveType);
                break;
        }
    }

    return no;
}

static int inx_can_ch_save(unsigned char *buf, unsigned int size)
{
    float tmpFloat;
    unsigned char tmpChar;
    double tmpDouble;
    unsigned int i;
    unsigned int no = 0;

    if (canlist_cnt * 4 > size)
    {
        log_i(LOG_DSU, "buf not enough, need=[%u],now=[%u]!", canlist_cnt * 4, size);
        return -1;
    }

    for (i = 0; i < canlist_cnt; i++)
    {
        tmpDouble = *(double *) CanList[i].valuePtr;

        if (CanList[i].saveType == STYPE_CHAR)
        {
            tmpChar = (unsigned char) tmpDouble;
            memcpy(&buf[no], &tmpChar, 1);
            no += 1;
        }
        else
        {
            tmpFloat = (float)tmpDouble;
            memcpy(&buf[no], &tmpFloat, 4);
            no += 4;
        }
    }

    return no;
}

static int inx_file_create(void)
{
    static unsigned char buf[1024];
    unsigned char header[64];
    unsigned int sn = 0, len, i;
    int ret = 0;
    static unsigned int rebootIndex = 1;
    unsigned int channelnum;
    unsigned char ch_num[4];

    len = sizeof(sn);
    cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char *) &sn, &len);

    if (NULL != inx_file.fp)
    {
        dsu_fclose(&inx_file);
    }

    tm_get_abstime(&inx_file.c_time);

    /* decide the file name */
    if (dev_is_time_syn())
    {
        snprintf(inx_file.name, sizeof(inx_file.name),
                 "%s/%04d%02d%02d_%02d%02d%02d(%u_%d).inx", DSU_INX_FILE_PATH,
                 inx_file.c_time.year, inx_file.c_time.mon, inx_file.c_time.mday,
                 inx_file.c_time.hour, inx_file.c_time.min, inx_file.c_time.sec,
                 dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
        log_i(LOG_DSU, "Create %s!", inx_file.name);
    }
    else
    {
        snprintf(inx_file.name, sizeof(inx_file.name), "%s/GpsUnfix(%u_%d).inx",
                 DSU_INX_FILE_PATH, dev_get_reboot_cnt() % DSU_TBOX_REBOOT_CNT_MAX, rebootIndex++);
        log_i(LOG_DSU, "Create %s!", inx_file.name);
    }

    /* Open or create a file */
    ret = dsu_fcreate(&inx_file, RAW_DATA, inx_file.name, "wb+");

    if (FR_OK != ret)
    {
        log_e(LOG_DSU, "dsu_fcreate failed!");
        return FR_ERR_OPEN;
    }

    /*
     the structure of inx file header total 64 bytes:
     --------------------------------------
     | INX Header(33 bytes, no compress)  |
     | INZ Header(23 bytes, no compress)  |
     | INR Header(7 bytes, compress)      |
     | INX EDI CNT(1 byte, compress)      |
     --------------------------------------
     */
    memset(header, 0, sizeof(header));
    len = dsu_build_head_inx(header, sizeof(header), sn);
    len += dsu_build_head_inz(header + len, sizeof(header) - len, sn);
    ret = dsu_write_fb(&inx_file, header, len);      // write no compress

    ret |= dsu_change_fb_type(&inx_file, ZLIB_DATA);
    memset(header, 0, sizeof(header));
    len = dsu_build_head_inr(header, sizeof(header));
    header[len] = INX_EDI_CNT;
    len += 1;

    ret |= dsu_write_fb(&inx_file, header, len);      // write to buf,and wait fsync() to compress

    /* Add Experimental Description Information */
    memset(buf, ' ', sizeof(buf));
    len = 0;

    for (i = 0; i < INX_EDI_CNT; i++)
    {
        if (0 == strlen((const char *) des_head[i]))
        {
            snprintf((char *) &buf[len], 8, "%08X", sn);
        }
        else
        {
            memcpy(&buf[len], des_head[i], strlen((const char *) des_head[i]));
        }

        len += INX_EDI_PER_SIZE;
    }

    ret |= dsu_write_fb(&inx_file, buf,
                        INX_EDI_TOTAL_SIZE);      // write to buf,and wait fsync() to compress

    /* add channel total num */
    channelnum = (sizeof(ParaList) / sizeof(ParaList[0])) + dbc_get_signal_cnt();
    ch_num[0] = (unsigned char) channelnum;
    ch_num[1] = (unsigned char)(channelnum >> 8);
    ch_num[2] = (unsigned char)(channelnum >> 16);
    ch_num[3] = (unsigned char)(channelnum >> 24);
    ret |= dsu_write_fb(&inx_file, ch_num, sizeof(ch_num));
    ret |= dsu_fsync(&inx_file);      // write to file and sync

    /* add parameter des */
    memset(buf, ' ', sizeof(buf));
    len = 0;

    for (i = 0; i < INX_CH_PARA_CNT; i++)
    {
        if ((sizeof(buf) - len) < INX_CH_BASEINFO_SIZE)
        {
            ret |= dsu_write_fb(&inx_file, buf, len);
            memset(buf, ' ', sizeof(buf));
            len = 0;
        }

        if (1 == i % 50 && 1 != i)
        {
            ret |= dsu_fsync(&inx_file);      // write to file and sync
        }

        len += inx_ch_baseinfo(&ParaList[i], buf + len, sizeof(buf) - len);
    }

    ret |= dsu_write_fb(&inx_file, buf, len);
    memset(buf, ' ', sizeof(buf));
    len = 0;

    /* add can signal channel des */
    for (i = 0; i < canlist_cnt; i++)
    {
        if ((sizeof(buf) - len) < INX_CH_BASEINFO_SIZE)
        {
            ret |= dsu_write_fb(&inx_file, buf, len);
            memset(buf, ' ', sizeof(buf));
            len = 0;
        }

        if (1 == i % 50 && 1 != i)
        {
            ret |= dsu_fsync(&inx_file);      // write to file and sync
        }

        len += inx_ch_baseinfo(&CanList[i], buf + len, sizeof(buf) - len);
    }

    ret |= dsu_write_fb(&inx_file, buf, len);
    len = 0;

    /* date */
    if (dev_is_time_syn())
    {
        buf[0] = (inx_file.c_time.year - 2000) & 0xFF;
        buf[1] = inx_file.c_time.mon;
        buf[2] = inx_file.c_time.mday;
        buf[3] = inx_file.c_time.hour;
        buf[4] = inx_file.c_time.min;
        buf[5] = inx_file.c_time.sec;
        buf[6] = 0;
        buf[7] = 0x80;
    }
    else
    {
        memset(buf, 0, 8);
    }

    len += 8;

    /* samplerate */
    double tmpDouble = (double) inx_attr.sampling;
    memcpy(&buf[len], &tmpDouble, 8);
    len += 8;
    ret |= dsu_write_fb(&inx_file, buf, len);
    ret |= dsu_fsync(&inx_file);
    log_i(LOG_DSU, "SD is %d Hz!", inx_attr.sampling);
    return ret;
}

static int inx_file_write(void)
{
    int ret;
    unsigned char idx;
    unsigned int *len;
    unsigned char *ptr;

    pthread_mutex_lock(&inx_tmp.mutex);
    idx = inx_tmp.sync_idx;
    len = &inx_tmp.len[idx];
    ptr = inx_tmp.data[idx];

    if (0 < *len)
    {
        ret = dsu_write_fb(&inx_file, ptr, *len);

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_write_fb error, len=%u", *len);
        }

        if (tmp_max_len[0] < *len)
        {
            tmp_max_len[0] = *len;
        }

        *len = 0;
    }

    pthread_mutex_unlock(&inx_tmp.mutex);

    if (dsu_fb_used(&inx_file) + 1024 > DSU_ZBUF_SIZE)
    {
        dsu_fsync(&inx_file);
    }

    return ret;
}

static int inx_file_sync(void)
{
    return dsu_fsync(&inx_file);
}

static int inx_file_close(void)
{
    int ret = 0;
    unsigned char idx;
    unsigned int *len;
    unsigned char *ptr;

    ret |= dsu_fsync(&inx_file);
    pthread_mutex_lock(&inx_tmp.mutex);
    idx = inx_tmp.sync_idx;
    len = &inx_tmp.len[idx];
    ptr = inx_tmp.data[idx];

    if (0 < *len)
    {
        ret = dsu_write_fb(&inx_file, ptr, *len);

        if (FR_OK != ret)
        {
            log_e(LOG_DSU, "dsu_write_fb error, len=%u", *len);
        }

        *len = 0;
    }

    pthread_mutex_unlock(&inx_tmp.mutex);
    ret |= dsu_fclose(&inx_file);
    return ret;
}

static bool inx_check_stop_record(void)
{
    /*
        1. DBC is not set
        2. DBC is set, and CanBus timeout, and gps.knots < 3
        3. inx not enable
        4. inx suspend
        The above satisfies any one, stop recording.
    */
    if (!dbc_is_load()
        || ((dbc_is_load()) && (CANBUS_TIMEOUT == canbus_stat) && (inx_rt.kms < 3))
        || !inx_attr.enable
        || inx_attr.suspend)
    {
        return true;
    }

    return false;
}

void inx_ch_init(void)
{
    int i, total;
    const dbc_sig_t *sig = NULL;

    canlist_cnt = 0;
    memset(&CanList[0], 0, sizeof(CanList) / sizeof(CanList[0]));

    total = MIN(INX_CH_CAN_CNT, dbc_get_signal_cnt());

    for (i = 0; i < total; i++)
    {
        sig = dbc_get_signal_from_id(i + 1);
        CanList[i].name = sig->name;
        CanList[i].des = sig->name;
        CanList[i].unit = sig->unit;
        CanList[i].bits = (sig->size == 1 ? 0 : 5);
        CanList[i].saveType = (sig->size == 1 ? STYPE_CHAR : STYPE_FLOAT);
        CanList[i].valuePtr = (void *)(&sig->value);
    }

    log_i(LOG_DSU, "inx signal cnt=%d", i);
    canlist_cnt = i;
}

int inx_file_init(void)
{
    memset(&inx_tmp, 0, sizeof(inx_tmp));
    inx_tmp.cur_idx = 0;
    inx_tmp.sync_idx = (inx_tmp.cur_idx + 1) % INX_TMP_CNT;
    pthread_mutex_init(&inx_tmp.mutex, NULL);
    memset(&inx_rt, 0, sizeof(inx_rt));
    inx_file.create = inx_file_create;
    inx_file.write = inx_file_write;
    inx_file.sync = inx_file_sync;
    inx_file.close = inx_file_close;
    return dsu_file_init(&inx_file, inx_fbuf, sizeof(inx_fbuf), RAW_DATA);
}

/* append data into file */
int inx_file_append(void)
{
    int ret = 0;
    int rsv_need = 0;
    static int rsv_max = 1024;
    static long long time = 0;
    unsigned char *ptr;
    unsigned int pos;
    unsigned char idx;

    pthread_mutex_lock(&inx_tmp.mutex);

    // check close file
    if (inx_check_stop_record())
    {
        //log_i(LOG_DSU, "inx record stop");

        if (inx_tmp.len[inx_tmp.cur_idx])
        {
            inx_tmp.sync_idx = inx_tmp.cur_idx;
            inx_tmp.cur_idx = (inx_tmp.cur_idx + 1) % INX_TMP_CNT;
            inx_tmp.len[inx_tmp.cur_idx] = 0;
        }

        if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
        {
            dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
            log_i(LOG_DSU, "close inx file");
        }
        else
        {
            inx_tmp.len[inx_tmp.cur_idx] = 0;
            inx_tmp.len[inx_tmp.sync_idx] = 0;
        }

        rsv_max = 1024;
        pthread_mutex_unlock(&inx_tmp.mutex);
        return 0;
    }
    else
    {
        //log_i(LOG_DSU, "inx record");

        if (FILE_ST_NONE == dsu_get_stat(&inx_file))
        {
            dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CREATE);
            time = tm_get_time();
            log_i(LOG_DSU, "creat inx file");
        }
    }

    // check tmp buf is full
    if (inx_tmp.len[inx_tmp.cur_idx] > INX_TMP_DATA_RSV_SIZE
        || (tm_get_time() - time > INX_TMP_TIMEOUT))
    {
        dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_WRITE);
        time = tm_get_time();

        if (0 == inx_tmp.len[inx_tmp.sync_idx])
        {
            inx_tmp.sync_idx = inx_tmp.cur_idx;
            inx_tmp.cur_idx = (inx_tmp.cur_idx + 1) % INX_TMP_CNT;
        }
        else
        {
            if (inx_tmp.len[inx_tmp.cur_idx] + rsv_max > INX_TMP_DATA_SIZE)
            {
                log_e(LOG_DSU, "tmp_buf[%d]=[%d],need[%d] overflow",
                      inx_tmp.cur_idx, inx_tmp.len[inx_tmp.cur_idx], rsv_max);
                pthread_mutex_unlock(&inx_tmp.mutex);
                return 0;
            }
        }
    }

    pthread_mutex_unlock(&inx_tmp.mutex);

    idx = inx_tmp.cur_idx;
    pos = inx_tmp.len[idx];
    ptr = &inx_tmp.data[idx][pos];
    inx_rt_update();
    ret = inx_rt_ch_save(ptr, INX_TMP_DATA_SIZE - pos);

    if (ret <= 0)
    {
        inx_tmp.len[idx] = 0;
        log_e(LOG_DSU, "tmp_buf[%d] not enough rt_ch, ret=%d", inx_tmp.cur_idx, ret);
        return 0;
    }

    rsv_need += ret;
    pos += ret;
    ptr = &inx_tmp.data[idx][pos];
    ret = inx_can_ch_save(ptr, INX_TMP_DATA_SIZE - pos);

    if (ret <= 0)
    {
        inx_tmp.len[idx] = 0;
        log_e(LOG_DSU, "tmp_buf[%d] not enough can_ch, ret=%d", inx_tmp.cur_idx, ret);
        return 0;
    }

    pos += ret;
    rsv_need += ret;
    rsv_max = MAX(rsv_max, rsv_need);
    inx_tmp.len[idx] = pos;
    return 0;
}

void inx_attr_init(DSU_CFG_T *cfg)
{
    memset(&inx_attr, 0, sizeof(inx_attr));
    inx_attr.enable = cfg->canlog_mode & DSU_CFG_INX_MASK;
    inx_attr.hourfile = cfg->hour & DSU_CFG_INX_MASK;
    inx_attr.sampling = cfg->sdhz;
}

