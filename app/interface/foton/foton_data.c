#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "gps_api.h"
#include "foton.h"
#include "foton_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "../support/protocol.h"
#include "dev_rw.h"
#include "dir.h"
#include "diag.h"
#include "rds.h"
#include "dev_api.h"

#define FT_MAX_PACK_CELL    800
#define FT_MAX_PACK_TEMP    800
#define FT_MAX_FUEL_TEMP    200
#define FT_MAX_FUEL_INFO    16
#define FT_MAX_VEHI_INFO    16
#define FT_MAX_WARN_INFO    32
#define FT_MAX_MOTOR_INFO   8
#define FT_MAX_ENGIN_INFO   4
#define FT_MAX_EXTR_INFO    16
#define FT_MAX_MOTOR        4

/* vehicle type */
#define FT_VEHITYPE_ELECT   0x01
#define FT_VEHITYPE_HYBIRD  0x02
#define FT_VEHITYPE_GASFUEL 0x03
/* vehicle information index */
#define FT_VINF_STATE       0x00
#define FT_VINF_CHARGE      0x01
#define FT_VINF_SPEED       0x02
#define FT_VINF_ODO         0x03
#define FT_VINF_VOLTAGE     0x04
#define FT_VINF_CURRENT     0x05
#define FT_VINF_SOC         0x06
#define FT_VINF_DCDC        0x07
#define FT_VINF_SHIFT       0x08
#define FT_VINF_INSULAT     0x09
#define FT_VINF_ACCPAD      0x0a
#define FT_VINF_BRKPAD      0x0b
#define FT_VINF_DRVIND      0x0c
#define FT_VINF_BRKIND      0x0d
#define FT_VINF_VEHIMODE    0x0e
#define FT_VINF_MAX         FT_VINF_VEHIMODE + 1
/* motor information index */
#define FT_MINF_STATE       0x00
#define FT_MINF_MCUTMP      0x01
#define FT_MINF_SPEED       0x02
#define FT_MINF_TORQUE      0x03
#define FT_MINF_MOTTMP      0x04
#define FT_MINF_VOLTAGE     0x05
#define FT_MINF_CURRENT     0x06
#define FT_MINF_MAX         FT_MINF_CURRENT + 1

/* fuel cell information index */
#define FT_FCINF_VOLTAGE    0x00
#define FT_FCINF_CURRENT    0x01
#define FT_FCINF_RATE       0x02
#define FT_FCINF_MAXTEMP    0x03
#define FT_FCINF_MAXTEMPID  0x04
#define FT_FCINF_MAXCCTT    0x05
#define FT_FCINF_MAXCCTTID  0x06
#define FT_FCINF_MAXPRES    0x07
#define FT_FCINF_MAXPRESID  0x08
#define FT_FCINF_HVDCDC     0x09
#define FT_FCINF_MAX        FT_FCINF_HVDCDC + 1
#define FT_FCINF_TEMPTAB    0x100

/* engine information index */
#define FT_EINF_STATE       0x00
#define FT_EINF_SPEED       0x01
#define FT_EINF_FUELRATE    0x02
#define FT_EINF_MAX         FT_EINF_FUELRATE + 1
/* extremum index */
#define FT_XINF_MAXVPID     0x00
#define FT_XINF_MAXVCID     0x01
#define FT_XINF_MAXV        0x02
#define FT_XINF_MINVPID     0x03
#define FT_XINF_MINVCID     0x04
#define FT_XINF_MINV        0x05
#define FT_XINF_MAXTPID     0x06
#define FT_XINF_MAXTCID     0x07
#define FT_XINF_MAXT        0x08
#define FT_XINF_MINTPID     0x09
#define FT_XINF_MINTCID     0x0a
#define FT_XINF_MINT        0x0b
#define FT_XINF_MAX         FT_XINF_MINT + 1
/* battery information index */
#define FT_BINF_VOLTAGE     0x3fe
#define FT_BINF_CURRENT     0x3ff
#define FT_TOTAL_CELLS      0x000   //GF000
#define FT_TOTAL_TEMPS      0x001   //GF001

/* FOTON data type */
#define FT_DATA_VEHIINFO    0x01
#define FT_DATA_MOTORINFO   0x02
#define FT_DATA_ENGINEINFO  0x04
#define FT_DATA_LOCATION    0x05
#define FT_DATA_EXTREMA     0x06
#define FT_DATA_WARNNING    0x07
#define FT_DATA_BATTVOLT    0x08
#define FT_DATA_BATTTEMP    0x09
#define FT_DATA_FUELCELL    0x0A
#define FT_DATA_VIRTUAL     0x0B
#define FT_DATA_ADAPTATION  0x0F

/* report data type */
#define FT_RPTTYPE_REALTM   0x02
#define FT_RPTTYPE_DELAY    0x03
/* report packets parameter */
#define FT_MAX_REPORT       2000

#define FT_ADAPT_TIMEOUT    10000  // 10S
typedef enum
{
    FT_ERR_CONFUSED = -3,
    FT_ERR_OVERFLOW = -2,
    FT_ERR_TIMEOUT  = -1,
    FT_INITIAL       = 0,
    FT_VALID         = 1,
    FT_CHANGED       = 2,
} FT_ADAPT_TYPE;

enum
{
    FT_ADAPT_CELLS = 0,
    FT_ADAPT_TEMPS,
    FT_ADAPT_MAX,
};

/* battery information adapt structure */
typedef struct
{
    FT_ADAPT_TYPE flg;
    int sig;
    uint32_t def_val;
    uint32_t cur_val;
    uint32_t cfg_val;
    uint32_t chg_cnt;
    unsigned long long tick;
    RTCTIME time;
} ft_adapt_t;

/* battery information structure */
typedef struct
{
    int voltage;
    int current;
    int cell[FT_MAX_PACK_CELL];
    int temp[FT_MAX_PACK_TEMP];
    uint32_t   cell_cnt;
    uint32_t   temp_cnt;
} ft_batt_t;

/* motor information structure */
typedef struct
{
    int info[FT_MAX_MOTOR_INFO];
    uint8_t state_tbl[256];
} ft_motor_t;

/* vehicle information structure */
typedef struct
{
    int info[FT_MAX_VEHI_INFO];
    uint8_t state_tbl[256];
    uint8_t mode_tbl[256];
    char    shift_tbl[256];
    uint8_t charge_tbl[256];
    uint8_t dcdc_tbl[256];
    uint8_t vehi_type;
} ft_vehi_t;

/* fuel cell information structure */
typedef struct
{
    int info[FT_MAX_FUEL_INFO];
    int temp[FT_MAX_FUEL_TEMP];
    int temp_cnt;
    uint8_t hvdcdc[8];
} ft_fuelcell_t;

/* engine information structure */
typedef struct
{
    int info[FT_MAX_ENGIN_INFO];
    uint8_t state_tbl[256];
} ft_engin_t;

/* FOTON information structure */
typedef struct _ft_info_t
{
    ft_vehi_t  vehi;
    ft_motor_t motor[FT_MAX_MOTOR];
    uint32_t   motor_cnt;
    ft_fuelcell_t fuelcell;
    ft_engin_t engin;
    ft_batt_t  batt;
    int warn[4][FT_MAX_WARN_INFO];
    int extr[FT_MAX_EXTR_INFO];
    int warntrig;
    int warntest;
    ft_adapt_t adapter[FT_ADAPT_MAX];   // [0]:cell_cnt; [1]:temp_cnt
    struct _ft_info_t *next;
} ft_info_t;

static ft_info_t  ft_infmem[2];
static ft_info_t *ft_inf;
static ft_pack_t  ft_datamem[FT_MAX_REPORT];
static ft_pack_t  ft_errmem[(FT_MAX_PACK_CELL + 199) / 200 * 30];
static list_t     ft_free_lst;
static list_t     ft_realtm_lst;
static list_t     ft_delay_lst;
static list_t     ft_trans_lst;
static list_t    *ft_errlst_head;
static int        ft_warnflag;
static int        ft_pendflag;

int 	          ft_is_allow_login = 0;
extern int 	      ft_allow_sleep;

static pthread_mutex_t ft_errmtx;
static pthread_mutex_t ft_datmtx;
static uint16_t ft_datintv;

static int ft_index_r, ft_index_w;

#define ERR_LOCK()          pthread_mutex_lock(&ft_errmtx)
#define ERR_UNLOCK()        pthread_mutex_unlock(&ft_errmtx)
#define DAT_LOCK()          pthread_mutex_lock(&ft_datmtx)
#define DAT_UNLOCK()        pthread_mutex_unlock(&ft_datmtx)
#define GROUP_SIZE(inf)     RDUP_DIV((inf)->batt.cell_cnt, 200)


#define FT_CACHE_PATH   COM_SDCARD_DIR"/foton/"
#define FT_CACHE_SIZE   (1 << 16)
#define FT_CACHE_MASK   (FT_CACHE_SIZE - 1)
#define FT_CACHE_GRADE  6

int ft_cache_read(int fd, uint8_t *buf, int len)
{
    int rlen = 0;

    while (rlen < len)
    {
        /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

        if (dev_diag_emmc_get_format_flag())
        {
            return -2; //formatting
        }

        /* RF_BUG00118-yuzm end-> */

        int ret = read(fd, buf + rlen, len - rlen);

        if (ret < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }

            return -1;
        }

        if (ret == 0)
        {
            break;
        }

        rlen += ret;
    }

    return rlen;
}

int ft_cache_write(int fd, uint8_t *buf, int len)
{
    int wlen = 0;

    while (wlen < len)
    {
        /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

        if (dev_diag_emmc_get_format_flag())
        {
            return -2; //formatting
        }

        /* RF_BUG00118-yuzm end-> */

        int ret = write(fd, buf + wlen, len - wlen);

        if (ret < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }

            return -1;
        }

        if (ret == 0)
        {
            break;
        }

        wlen += ret;
    }

    return wlen;
}

static int ft_cache_check_sum(uint8_t *data, int len)
{
    uint8_t cs = 0;

    while (len--)
    {
        cs ^= *data++;
    }

    return cs;
}


static int ft_cache_load_info(int *index_r, int *index_w)
{
    uint8_t info[9];
    int fd;

    /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

    if (dev_diag_emmc_get_format_flag())
    {
        return -2; //formatting
    }

    /* RF_BUG00118-yuzm end-> */

    if ((fd = open(FT_CACHE_PATH"cache_info", O_RDONLY)) < 0)
    {
        log_e(LOG_FOTON, "open cache_info fail");
        *index_r = *index_w = 0;
        return -1;
    }

    if (ft_cache_read(fd, info, 9) != 9 || ft_cache_check_sum(info, 8) != info[8])
    {
        log_e(LOG_FOTON, "read cache_info fail");
        *index_r = *index_w = 0;
        close(fd);
        return -1;
    }

    memcpy(index_r, info, 4);
    memcpy(index_w, info + 4, 4);
    close(fd);
    log_o(LOG_FOTON, "read cache_info, index_r=%d, index_w=%d", *index_r, *index_w);
    return 0;
}

static int ft_cache_save_info(int index_r, int index_w)
{
    uint8_t info[9];
    int fd;

    /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

    if (dev_diag_emmc_get_format_flag())
    {
        return -2; //formatting
    }

    /* RF_BUG00118-yuzm end-> */

    if ((fd = open(FT_CACHE_PATH"cache_info", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
    {
        log_e(LOG_FOTON, "open cache_info fail");
        return -1;
    }

    memcpy(info, &index_r, 4);
    memcpy(info + 4, &index_w, 4);
    info[8] = ft_cache_check_sum(info, 8);

    if (ft_cache_write(fd, info, 9) != 9)
    {
        log_e(LOG_FOTON, "write cache_info fail");
        close(fd);
        return -1;
    }

    close(fd);
    log_i(LOG_FOTON, "write cache_info, index_r=%d, index_w=%d", index_r, index_w);
    return 0;
}


static int ft_cache_save_pack(int fd, int index, ft_pack_t *pack)
{
    int posi = index * sizeof(ft_pack_t), size = sizeof(ft_pack_t);

    return lseek(fd, posi, SEEK_SET) < 0 || ft_cache_write(fd, (uint8_t *)pack, size) != size;
}

static int ft_cache_load_pack(int fd, int index, ft_pack_t *pack)
{
    int posi = index * sizeof(ft_pack_t), size = sizeof(ft_pack_t);

    return lseek(fd, posi, SEEK_SET) < 0 || ft_cache_read(fd, (uint8_t *)pack, size) != size;
}

static int ft_cache_load(void)
{
    int cnt = 0, fd;

    if (ft_index_r == ft_index_w)
    {
        return 0;
    }

    /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

    if (dev_diag_emmc_get_format_flag())
    {
        return -2; //formatting
    }

    /* RF_BUG00118-yuzm end-> */

    if ((fd = open(FT_CACHE_PATH"cache_data", O_RDONLY)) < 0)
    {
        log_e(LOG_FOTON, "open cache_data fail, reset all index");
        ft_index_r = ft_index_w;
        return 0;
    }

    while (cnt < FT_CACHE_GRADE && ft_index_r != ft_index_w)
    {
        ft_pack_t *pack;
        list_t *node;

        //DAT_LOCK();
        if ((node = list_get_first(&ft_free_lst)) == NULL)
        {
            /* it should not be happened */
            log_e(LOG_FOTON, "BIG ERROR: no enough buffer to load cache.");

            while (1);
        }

        //DAT_UNLOCK();

        pack = list_entry(node, ft_pack_t, link);

        if (ft_cache_load_pack(fd, ft_index_r, pack) != 0)
        {
            log_e(LOG_FOTON, "read cache_data fail, index=%d", ft_index_r);
            log_e(LOG_FOTON, "reset all index, cache will lost");
            ft_index_r = ft_index_w = 0;
            //DAT_LOCK();
            list_insert_before(&ft_free_lst, node);
            //DAT_UNLOCK();
            break;
        }

        log_i(LOG_FOTON, "read data from cache_data, index=%d", ft_index_r);

        pack->list = &ft_delay_lst;
        pack->type = FT_RPTTYPE_DELAY;

        //DAT_LOCK();
        list_insert_before(&ft_delay_lst, node);
        //DAT_UNLOCK();

        ft_index_r = (ft_index_r + 1) & FT_CACHE_MASK;
        cnt++;
    }

    if (cnt > 0)
    {
        log_i(LOG_FOTON, "read %d data from cache_data", cnt);
        ft_cache_save_info(ft_index_r, ft_index_w);
    }

    close(fd);
    return cnt;
}

static int ft_cache_save(void)
{
    int cnt = 0, fd;

    if (dir_exists(FT_CACHE_PATH) == 0 &&
        dir_make_path(FT_CACHE_PATH, S_IRUSR | S_IWUSR, false) != 0)
    {
        log_e(LOG_FOTON, "open cache path fail, reset all index");
        ft_index_r = ft_index_w = 0;
        return 0;
    }

    /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

    if (dev_diag_emmc_get_format_flag())
    {
        return -2; //formatting
    }

    /* RF_BUG00118-yuzm end-> */

    if ((fd = open(FT_CACHE_PATH"cache_data", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
    {
        log_e(LOG_FOTON, "open cache_data fail, reset all index");
        ft_index_r = ft_index_w = 0;
        return 0;
    }

    while (cnt < FT_CACHE_GRADE)
    {
        ft_pack_t *pack;
        list_t *node;

        //DAT_LOCK();
        if ((node = list_get_first(&ft_delay_lst)) == NULL &&
            (node = list_get_first(&ft_realtm_lst)) == NULL)
        {
            /* it should not be happened */
            log_e(LOG_FOTON, "BIG ERROR: no enough data to save cache.");

            while (1);
        }

        //DAT_UNLOCK();

        pack = list_entry(node, ft_pack_t, link);

        if (ft_cache_save_pack(fd, ft_index_w, pack) != 0)
        {
            log_e(LOG_FOTON, "write cache_data fail, index=%d", ft_index_w);
            log_e(LOG_FOTON, "reset all index, cache will lost");
            ft_index_r = ft_index_w = 0;
            //DAT_LOCK();
            list_insert_before(pack->list, node);
            //DAT_UNLOCK();
            break;
        }

        log_i(LOG_FOTON, "write data to cache_data, index=%d", ft_index_w);

        //DAT_LOCK();
        list_insert_before(&ft_free_lst, node);
        //DAT_UNLOCK();

        ft_index_w = (ft_index_w + 1) & FT_CACHE_MASK;

        if (ft_index_r == ft_index_w)
        {
            log_e(LOG_FOTON, "cache_data overflow, lost index=%d", ft_index_r);
            ft_index_r = (ft_index_r + 1) & FT_CACHE_MASK;
        }

        cnt++;
    }

    if (cnt > 0)
    {
        log_i(LOG_FOTON, "write %d data to cache_data", cnt);
        ft_cache_save_info(ft_index_r, ft_index_w);
    }

    close(fd);
    return cnt;
}

int ft_cache_save_all(void)
{
    int cnt = 0, fd;

    if (dir_exists(FT_CACHE_PATH) == 0 &&
        dir_make_path(FT_CACHE_PATH, S_IRUSR | S_IWUSR, false) != 0)
    {
        log_e(LOG_FOTON, "open cache path fail, reset all index");
        ft_index_r = ft_index_w = 0;
        return 0;
    }

    /* <-begin RF_BUG00118-yuzm-2018/05/24-format failed when open/read/write. */

    if (dev_diag_emmc_get_format_flag())
    {
        return -2; //formatting
    }

    /* RF_BUG00118-yuzm end-> */

    if ((fd = open(FT_CACHE_PATH"cache_data", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
    {
        log_e(LOG_FOTON, "open cache_data fail, reset all index");
        ft_index_r = ft_index_w = 0;
        return 0;
    }

    DAT_LOCK();

    while (!list_empty(&ft_delay_lst) || !list_empty(&ft_realtm_lst))
    {
        ft_pack_t *pack;
        list_t *node = list_get_first(&ft_delay_lst);

        if (node == NULL)
        {
            node = list_get_first(&ft_realtm_lst);
        }

        pack = list_entry(node, ft_pack_t, link);

        if (ft_cache_save_pack(fd, ft_index_w, pack) != 0)
        {
            log_e(LOG_FOTON, "write cache_data fail, index=%d", ft_index_w);
            log_e(LOG_FOTON, "reset all index, cache will lost");
            ft_index_r = ft_index_w = 0;
            //DAT_LOCK();
            list_insert_before(pack->list, node);
            //DAT_UNLOCK();
            break;
        }

        log_i(LOG_FOTON, "write data to cache_data, index=%d", ft_index_w);

        //DAT_LOCK();
        list_insert_before(&ft_free_lst, node);
        //DAT_UNLOCK();

        ft_index_w = (ft_index_w + 1) & FT_CACHE_MASK;

        if (ft_index_r == ft_index_w)
        {
            log_e(LOG_FOTON, "cache_data overflow, lost index=%d", ft_index_r);
            ft_index_r = (ft_index_r + 1) & FT_CACHE_MASK;
        }

        cnt++;
    }

    DAT_UNLOCK();

    if (cnt > 0)
    {
        log_i(LOG_FOTON, "write %d data to cache_data", cnt);
        ft_cache_save_info(ft_index_r, ft_index_w);
    }

    close(fd);
    return cnt;
}

static uint32_t ft_data_save_vehi(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : vehicle information */
    buf[len++] = FT_DATA_VEHIINFO;

    /* vehicle state */
    if (ftinf->vehi.info[FT_VINF_STATE])
    {
        tmp = dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_STATE])->value;
        buf[len++] = ftinf->vehi.state_tbl[tmp] ? ftinf->vehi.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* charge state */
    if (ftinf->vehi.info[FT_VINF_CHARGE])
    {
        tmp = dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_CHARGE])->value;
        buf[len++] = ftinf->vehi.charge_tbl[tmp] ? ftinf->vehi.charge_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* vehicle type */
    if (ftinf->vehi.info[FT_VINF_VEHIMODE])
    {
        tmp = dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_VEHIMODE])->value;
        buf[len++] = ftinf->vehi.mode_tbl[tmp] ? ftinf->vehi.mode_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = ftinf->vehi.vehi_type;
    }

    /* vehicle speed, scale 0.1km/h */
    tmp = ftinf->vehi.info[FT_VINF_SPEED] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_SPEED])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* odograph, scale 0.1km */
    tmp = ftinf->vehi.info[FT_VINF_ODO] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_ODO])->value * 10 : 0xffffffff;
    buf[len++] = tmp >> 24;
    buf[len++] = tmp >> 16;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total voltage, scale 0.1V */
    tmp = ftinf->vehi.info[FT_VINF_VOLTAGE] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_VOLTAGE])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total voltage, scale 0.1V, offset -1000A */
    tmp = ftinf->vehi.info[FT_VINF_CURRENT] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_CURRENT])->value * 10 + 10000 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total SOC */
    tmp = ftinf->vehi.info[FT_VINF_SOC] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_SOC])->value : 0xff;
    buf[len++] = tmp;

    /* DCDC state */
    if (ftinf->vehi.info[FT_VINF_DCDC])
    {
        tmp = dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_DCDC])->value;
        buf[len++] = ftinf->vehi.dcdc_tbl[tmp] ? ftinf->vehi.dcdc_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* shift state */
    if (ftinf->vehi.info[FT_VINF_SHIFT])
    {
        uint8_t sft;

        tmp = dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_SHIFT])->value;
        sft = ftinf->vehi.shift_tbl[tmp];

        switch (sft)
        {
            case '1'...'6':
                tmp = sft - '0';
                break;

            case 'R':
                tmp = 13;
                break;

            case 'D':
                tmp = 14;
                break;

            case 'P':
                tmp = 15;
                break;

            default:
                tmp = 0;
                break;
        }

        if (ftinf->vehi.info[FT_VINF_DRVIND])
        {
            if (dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_DRVIND])->value > 0)
            {
                tmp |= 0x20;
            }
        }
        else if (ftinf->vehi.info[FT_VINF_ACCPAD])
        {
            if (dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_ACCPAD])->value > 0)
            {
                tmp |= 0x20;
            }
        }

        if (ftinf->vehi.info[FT_VINF_BRKIND])
        {
            if (dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_BRKIND])->value > 0)
            {
                tmp |= 0x10;
            }
        }
        else if (ftinf->vehi.info[FT_VINF_BRKPAD])
        {
            if (dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_BRKPAD])->value > 0)
            {
                tmp |= 0x10;
            }
        }

        buf[len++] = tmp;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* insulation resistance, scale 1k */
    tmp = ftinf->vehi.info[FT_VINF_INSULAT] ?
          MIN(dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_INSULAT])->value, 60000) : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* accelate pad value */
    tmp = ftinf->vehi.info[FT_VINF_ACCPAD] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_ACCPAD])->value : 0xff;
    buf[len++] = tmp;

    /* break pad value */
    tmp = ftinf->vehi.info[FT_VINF_BRKPAD] ?
          dbc_get_signal_from_id(ftinf->vehi.info[FT_VINF_BRKPAD])->value : 0xff;
    buf[len++] = tmp;

    return len;
}

static uint32_t ft_data_save_cell(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, tmp, i, cells;
    static uint32_t start = 0;

    buf[len++] = FT_DATA_BATTVOLT;
    buf[len++] = 1;
    buf[len++] = 1;

    /* packet voltage, scale 0.1V */
    tmp = ftinf->batt.voltage ?
          dbc_get_signal_from_id(ftinf->batt.voltage)->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* packet current, scale 0.1A, offset -1000A */
    tmp = ftinf->batt.current ?
          dbc_get_signal_from_id(ftinf->batt.current)->value * 10 + 10000 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total cell count */
    buf[len++] = ftinf->batt.cell_cnt >> 8;
    buf[len++] = ftinf->batt.cell_cnt;

    /* start cell of current frame */
    buf[len++] = (start + 1) >> 8;
    buf[len++] = (start + 1);

    /* cell count of current frame */
    cells = MIN(ftinf->batt.cell_cnt - start, 200);
    buf[len++] = cells;

    for (i = start; i < start + cells; i++)
    {
        tmp = ftinf->batt.cell[i]?
			dbc_get_signal_from_id(ftinf->batt.cell[i])->value * 1000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    start = (start + cells) % ftinf->batt.cell_cnt;

    return len;
}

static uint32_t ft_data_save_temp(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, i;

    buf[len++] = FT_DATA_BATTTEMP;
    buf[len++] = 1;
    buf[len++] = 1;

    /* total temp count */
    buf[len++] = ftinf->batt.temp_cnt >> 8;
    buf[len++] = ftinf->batt.temp_cnt;

    for (i = 0; i < ftinf->batt.temp_cnt; i++)
    {
        buf[len++] = ftinf->batt.temp[i]?
			(uint8_t)(dbc_get_signal_from_id(ftinf->batt.temp[i])->value + 40):0xff;
    }

    return len;
}

static uint32_t ft_data_save_motor(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmp;

    buf[len++] = FT_DATA_MOTORINFO;
    buf[len++] = ftinf->motor_cnt;

    for (i = 0; i < ftinf->motor_cnt; i++)
    {
        /* motor number */
        buf[len++] = i + 1;

        /* motor state */
        if (ftinf->motor[i].info[FT_MINF_STATE])
        {
            tmp = dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_STATE])->value;
            buf[len++] = ftinf->motor[i].state_tbl[tmp] ?
                         ftinf->motor[i].state_tbl[tmp] : 0xff;
        }
        else
        {
            buf[len++] = 0xff;
        }

        /* MCU temperature */
        tmp = ftinf->motor[i].info[FT_MINF_MCUTMP] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_MCUTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor speed, offset -20000rpm */
        tmp = ftinf->motor[i].info[FT_MINF_SPEED] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_SPEED])->value + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor torque, scale 0.1Nm, offset -2000Nm */
        tmp = ftinf->motor[i].info[FT_MINF_TORQUE] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_TORQUE])->value * 10 + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor temperature */
        tmp = ftinf->motor[i].info[FT_MINF_MOTTMP] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_MOTTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor voltage, scale 0.1V*/
        tmp = ftinf->motor[i].info[FT_MINF_VOLTAGE] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_VOLTAGE])->value * 10 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor current, scale 0.1A, offset -1000A */
        tmp = ftinf->motor[i].info[FT_MINF_CURRENT] ?
              dbc_get_signal_from_id(ftinf->motor[i].info[FT_MINF_CURRENT])->value * 10 + 10000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    return len;
}

static uint32_t ft_data_save_fuelcell(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, tmp,i;

    /* data type : fuel cell information */
    buf[len++] = 0x03;

    /* fuel cell voltage value */
    tmp = ftinf->fuelcell.info[FT_FCINF_VOLTAGE] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_VOLTAGE])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell current value */
    tmp = ftinf->fuelcell.info[FT_FCINF_CURRENT] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_CURRENT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;    
    
    /* fuel cell consumption rate */
    tmp = ftinf->fuelcell.info[FT_FCINF_RATE] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_RATE])->value*100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell temperature needle number */
    buf[len++] = ftinf->fuelcell.temp_cnt >> 8;
    buf[len++] = ftinf->fuelcell.temp_cnt;

    for(i = 0;i < ftinf->fuelcell.temp_cnt; i++)
    {
        /*highest temperature of hydrogen system  */
        tmp = ftinf->fuelcell.temp[i] ? 
            (dbc_get_signal_from_id(ftinf->fuelcell.temp[i])->value + 40) : 0xff;
        buf[len++] = tmp;
    }
    
    /* highest temperature of hydrogen system  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXTEMP] ? 
            (dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXTEMP])->value*10 + 400) : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest temperature of hydrogen system  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXTEMPID] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXTEMPID])->value : 0xff;
    buf[len++] = tmp;

    /* highest hydrogen rate  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXCCTT] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXCCTT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* the ID of highest hydrogen rate  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXCCTTID] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXCCTTID])->value : 0xff;
    buf[len++] = tmp;

    /*highest pressure of hydrogen system  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXPRES] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXPRES])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest pressure of hydrogen system  */
    tmp = ftinf->fuelcell.info[FT_FCINF_MAXPRESID] ? 
          dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_MAXPRESID])->value : 0xff;
    buf[len++] = tmp;

    /* High voltage DCDC state */
    if (ftinf->fuelcell.info[FT_FCINF_HVDCDC])
    {
        tmp = dbc_get_signal_from_id(ftinf->fuelcell.info[FT_FCINF_HVDCDC])->value;
        buf[len++] = ftinf->fuelcell.hvdcdc[tmp] ? ftinf->fuelcell.hvdcdc[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }


    return len;
}

static uint32_t ft_data_save_extr(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmpv, tmpi;
    uint32_t maxvid = 0, maxtid = 0, minvid = 0, mintid = 0;
    double maxv, maxt, minv, mint;
    static uint32_t packnum = 0;

    maxvid = maxtid = minvid = mintid = 1;
    maxv = minv = ftinf->batt.cell_cnt > 0 ? dbc_get_signal_from_id(ftinf->batt.cell[0])->value : 0;
    maxt = mint = ftinf->batt.temp_cnt > 0 ? dbc_get_signal_from_id(ftinf->batt.temp[0])->value : 0;

    for (i = 0; i < ftinf->batt.cell_cnt; i++)
    {
        double value = dbc_get_signal_from_id(ftinf->batt.cell[i])->value;

        if (value > 15)
        {
            continue;
        }

        if (minv > 15 || value < minv)
        {
            minv   = value;
            minvid = i + 1;
        }

        if (maxv > 15 || value > maxv)
        {
            maxv   = value;
            maxvid = i + 1;
        }
    }

    for (i = 0; i < ftinf->batt.temp_cnt; i++)
    {
        double value = dbc_get_signal_from_id(ftinf->batt.temp[i])->value;

        if (value > 210)
        {
            continue;
        }

        if (mint > 210 || value < mint)
        {
            mint   = value;
            mintid = i + 1;
        }

        if (maxt > 210 || value > maxt)
        {
            maxt   = value;
            maxtid = i + 1;
        }
    }

    if (ftinf->extr[FT_XINF_MAXVCID])
    {
        maxvid = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MAXVCID])->value;
    }

    if (ftinf->extr[FT_XINF_MAXV])
    {
        maxv = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MAXV])->value;
    }

    if (ftinf->extr[FT_XINF_MAXTCID])
    {
        maxtid = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MAXTCID])->value;
    }

    if (ftinf->extr[FT_XINF_MAXT])
    {
        maxt = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MAXT])->value;
    }

    if (ftinf->extr[FT_XINF_MINVCID])
    {
        minvid = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MINVCID])->value;
    }

    if (ftinf->extr[FT_XINF_MINV])
    {
        minv = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MINV])->value;
    }

    if (ftinf->extr[FT_XINF_MINTCID])
    {
        mintid = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MINTCID])->value;
    }

    if (ftinf->extr[FT_XINF_MINT])
    {
        mint = dbc_get_signal_from_id(ftinf->extr[FT_XINF_MINT])->value;
    }

    buf[len++] = FT_DATA_EXTREMA;

    buf[len++] = 1;

    if (maxvid > (packnum + 1) * 200)
    {
        tmpi = 200;
        tmpv = 0xffff;
    }
    else if (maxvid > packnum * 200)
    {
        tmpi = maxvid - packnum * 200;
        tmpv = maxv * 1000;
    }
    else
    {
        tmpi = 0;
        tmpv = 0xffff;
    }

    buf[len++] = tmpi;
    buf[len++] = tmpv >> 8;
    buf[len++] = tmpv;

    buf[len++] = 1;

    if (minvid > (packnum + 1) * 200)
    {
        tmpi = 200;
        tmpv = 0xffff;
    }
    else if (minvid > packnum * 200)
    {
        tmpi = minvid - packnum * 200;
        tmpv = minv * 1000;
    }
    else
    {
        tmpi = 0;
        tmpv = 0xffff;
    }

    buf[len++] = tmpi;
    buf[len++] = tmpv >> 8;
    buf[len++] = tmpv;

    buf[len++] = 1;
    buf[len++] = maxtid;
    buf[len++] = maxt + 40;
    buf[len++] = 1;
    buf[len++] = mintid;
    buf[len++] = mint + 40;

    packnum = (packnum + 1) % GROUP_SIZE(ftinf);

    return len;
}

static uint32_t ft_data_save_warn(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, i, j, warnbit = 0, warnlvl = 0;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if (ftinf->warn[i][j] && 
            (dbc_get_signal_from_id(ftinf->warn[i][j])->value || 
            (ftinf->warn[3][j] && 
            dbc_get_signal_from_id(ftinf->warn[3][j])->value)))
            // index 3,as a relevance channel,if the is two canid used for on warning
            {
                warnbit |= 1 << j;
                warnlvl  = i + 1;
            }
        }
    }

    if (ftinf->warntest)
    {
        warnbit |= 1;
        warnlvl  = 3;
    }

    buf[len++] = FT_DATA_WARNNING;
    buf[len++] = warnlvl;
    buf[len++] = warnbit >> 24;
    buf[len++] = warnbit >> 16;
    buf[len++] = warnbit >> 8;
    buf[len++] = warnbit;

    buf[len++] = 0;     /* battery fault */
    buf[len++] = 0;     /* motor fault */
    buf[len++] = 0;     /* engin fault */
    buf[len++] = 0;     /* other fault */

    return len;
}

static uint32_t ft_data_save_engin(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : engine information */
    buf[len++] = FT_DATA_ENGINEINFO;

    /* engine state */
    if (ftinf->engin.info[FT_EINF_STATE])
    {
        tmp = dbc_get_signal_from_id(ftinf->engin.info[FT_EINF_STATE])->value;
        buf[len++] = ftinf->engin.state_tbl[tmp] ? ftinf->engin.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* engine speed, scale 1rpm */
    tmp = ftinf->engin.info[FT_EINF_SPEED] ?
          dbc_get_signal_from_id(ftinf->engin.info[FT_EINF_SPEED])->value : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fule rate, scale 0.01L/100km */
    tmp = ftinf->engin.info[FT_EINF_FUELRATE] ?
          dbc_get_signal_from_id(ftinf->engin.info[FT_EINF_FUELRATE])->value * 100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    return len;
}


/* Convert dddmm.mmmm(double) To ddd.dd+(double)*/
static uint32_t ft_data_gpsconv(double dddmm)
{
    int deg;
    double min;

    deg = dddmm / 100.0;
    min = dddmm - deg * 100;

    return (uint32_t)((deg + min / 60 + 0.5E-6) * 1000000);
}


static uint32_t ft_data_save_gps(ft_info_t *ftinf, uint8_t *buf)
{
    uint32_t len = 0;
    GPS_DATA gpsdata;

    static uint32_t  longitudeBak = 0;
    static uint32_t  latitudeBak  = 0;

    /* data type : location data */
    buf[len++] = FT_DATA_LOCATION;

    /* status bits */
    /* bit-0: 0-A,1-V */
    /* bit-1: 0-N,1-S */
    /* bit-2: 0-E,1-W */
    if (gps_get_fix_status() == 2)
    {
        gps_get_snap(&gpsdata);
        longitudeBak = ft_data_gpsconv(gpsdata.longitude);
        latitudeBak  = ft_data_gpsconv(gpsdata.latitude);
        buf[len++]   = (gpsdata.is_north ? 0 : 0x02) | (gpsdata.is_east ? 0 : 0x04);
    }
    else
    {
        buf[len++] = 0x01;
    }

    /* longitude */
    buf[len++] = longitudeBak >> 24;
    buf[len++] = longitudeBak >> 16;
    buf[len++] = longitudeBak >> 8;
    buf[len++] = longitudeBak;
    /* latitude */
    buf[len++] = latitudeBak >> 24;
    buf[len++] = latitudeBak >> 16;
    buf[len++] = latitudeBak >> 8;
    buf[len++] = latitudeBak;

    return len;
}

static uint32_t ft_data_save_all(ft_info_t *ftinf, uint8_t *buf, uint32_t uptime)
{
    uint32_t len = 0;
    RTCTIME time;

    can_get_time(uptime, &time);

    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;

    len += ft_data_save_vehi(ftinf, buf + len);

    if (ftinf->vehi.vehi_type == FT_VEHITYPE_ELECT ||
        ftinf->vehi.vehi_type == FT_VEHITYPE_HYBIRD)
    {
        uint8_t  fuelcell;
        uint32_t paralen = sizeof(fuelcell);
        
        len += ft_data_save_motor(ftinf, buf + len);
        if(0 == cfg_get_para(CFG_ITEM_FUELCELL, &fuelcell, &paralen) && fuelcell)
        {
            len += ft_data_save_fuelcell(ftinf, buf + len);
        }
        len += ft_data_save_cell(ftinf, buf + len);
        len += ft_data_save_temp(ftinf, buf + len);
        len += ft_data_save_extr(ftinf, buf + len);
    }

    if (ftinf->vehi.vehi_type == FT_VEHITYPE_GASFUEL ||
        ftinf->vehi.vehi_type == FT_VEHITYPE_HYBIRD)
    {
        len += ft_data_save_engin(ftinf, buf + len);
    }

    len += ft_data_save_gps(ftinf, buf + len);
    len += ft_data_save_warn(ftinf, buf + len);

    return len;
}

static void ft_data_save_realtm(ft_info_t *ftinf, uint32_t uptime)
{
    int i;

    DAT_LOCK();

    for (i = 0; i < GROUP_SIZE(ftinf); i++)
    {
        ft_pack_t *rpt;
        list_t *node;

        if (list_empty(&ft_free_lst) && ft_cache_save() == 0)
        {
            log_e(LOG_FOTON, "cache data fail, oldest data will lost");
        }

        if ((node = list_get_first(&ft_free_lst)) == NULL)
        {
            if ((node = list_get_first(&ft_delay_lst)) == NULL &&
                (node = list_get_first(&ft_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_FOTON, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        rpt = list_entry(node, ft_pack_t, link);
        rpt->len  = ft_data_save_all(ftinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        rpt->list = &ft_realtm_lst;
        rpt->type = FT_RPTTYPE_REALTM;
        list_insert_before(&ft_realtm_lst, node);
    }

    DAT_UNLOCK();
}

static void ft_data_save_error(ft_info_t *ftinf, uint32_t uptime)
{
    int i;

    ERR_LOCK();

    for (i = 0; i < GROUP_SIZE(ftinf); i++)
    {
        ft_pack_t *rpt = list_entry(ft_errlst_head, ft_pack_t, link);

        rpt->len  = ft_data_save_all(ftinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        ft_errlst_head = ft_errlst_head->next;
    }

    ERR_UNLOCK();
}

static void ft_data_flush_error(void)
{
    list_t tmplst;

    list_init(&tmplst);
    ERR_LOCK();

    if (ft_errlst_head == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    while (list_entry(ft_errlst_head->prev, ft_pack_t, link)->len)
    {
        list_t *node;
        ft_pack_t *rpt, *err;

        DAT_LOCK();

        if ((node = list_get_first(&ft_free_lst)) == NULL)
        {
            log_e(LOG_FOTON, "report buffer is full, discard the oldest one");

            if ((node = list_get_first(&ft_delay_lst)) == NULL &&
                (node = list_get_first(&ft_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_FOTON, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        DAT_UNLOCK();

        ft_errlst_head = ft_errlst_head->prev;
        err = list_entry(ft_errlst_head, ft_pack_t, link);
        rpt = list_entry(node, ft_pack_t, link);
        memcpy(rpt, err, sizeof(ft_pack_t));
        err->len  = 0;
        rpt->list = &ft_delay_lst;
        rpt->type = FT_RPTTYPE_DELAY;
        list_insert_after(&tmplst, node);
    }

    ERR_UNLOCK();

    if (!list_empty(&tmplst))
    {
        DAT_LOCK();
        /* append error data to delay list */
        tmplst.next->prev = ft_delay_lst.prev;
        ft_delay_lst.prev->next = tmplst.next;
        tmplst.prev->next = &ft_delay_lst;
        ft_delay_lst.prev = tmplst.prev;
        DAT_UNLOCK();
    }
}

void ft_data_flush_sending(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_last(&ft_trans_lst)) != NULL)
    {
        ft_pack_t *pack = list_entry(node, ft_pack_t, link);

        list_insert_after(pack->list, &pack->link);
    }

    DAT_UNLOCK();
}

void ft_data_flush_realtm(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_first(&ft_realtm_lst)) != NULL)
    {
        ft_pack_t *pack = list_entry(node, ft_pack_t, link);
        pack->list = &ft_delay_lst;
        pack->type = FT_RPTTYPE_DELAY;
        list_insert_before(&ft_delay_lst, &pack->link);
    }

    DAT_UNLOCK();
}

void ft_data_clear_report(void)
{
    int i;

    DAT_LOCK();
    list_init(&ft_realtm_lst);
    list_init(&ft_delay_lst);
    list_init(&ft_trans_lst);
    list_init(&ft_free_lst);

    for (i = 0; i < FT_MAX_REPORT; i++)
    {
        list_insert_before(&ft_free_lst, &ft_datamem[i].link);
    }

    DAT_UNLOCK();
}

void ft_data_clear_error(void)
{
    list_t *node;

    ERR_LOCK();

    if ((node = ft_errlst_head) == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    do
    {
        list_entry(node, ft_pack_t, link)->len = 0;
        node = node->next;
    }
    while (node != ft_errlst_head);

    ERR_UNLOCK();
}

static void ft_data_parse_define(ft_info_t *ftinf, const char *str)
{
    assert(str != NULL);

    /* vehicle type */
    {
        uint32_t vtype;

        if (1 == sscanf(str, "BA_ \"IN_VEHITYPE\" %2x", &vtype) &&
            (vtype == FT_VEHITYPE_ELECT || vtype == FT_VEHITYPE_GASFUEL || FT_VEHITYPE_HYBIRD))
        {
            ftinf->vehi.vehi_type = vtype;
            return;
        }
    }

    /* engine state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_ENGINESTATE_%2x\" %2x", &index, &state))
        {
            ftinf->engin.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHISTATE_%2x\" %2x", &index, &state))
        {
            ftinf->vehi.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle mode */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHIMODE_%2x\" %2x", &index, &state))
        {
            ftinf->vehi.mode_tbl[index] = state;
            return;
        }
    }

    /* charge state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_CHGSTATE_%2x\" %2x", &index, &state))
        {
            ftinf->vehi.charge_tbl[index] = state;
            return;
        }
    }

    /* DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_DCDCSTATE_%2x\" %2x", &index, &state))
        {
            ftinf->vehi.dcdc_tbl[index] = state;
            return;
        }
    }

    /* High voltage DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_HVDCDCSTATE_%2x\" %2x", &index, &state))
        {
            ftinf->fuelcell.hvdcdc[index] = state;
            return;
        }
    }

    /* shift state */
    {
        char shift;
        uint32_t index;

        if (2 == sscanf(str, "BA_ \"IN_SHIFT_%2x\" %c", &index, &shift))
        {
            ftinf->vehi.shift_tbl[index] = shift;
            return;
        }
    }

    /* motor state */
    {
        uint32_t state, index, motor;

        if (3 == sscanf(str, "BA_ \"IN_MOTSTATE%u_%2x\" %2x", &motor, &index, &state) &&
            motor < FT_MAX_MOTOR)
        {
            ftinf->motor[motor].state_tbl[index] = state;
            return;
        }
    }
}

static int ft_data_parse_surfix(ft_info_t *ftinf, int sigid, const char *sfx)
{
    uint32_t gbtype, gbindex;

    assert(sigid > 0 && sfx != NULL);

    if (2 != sscanf(sfx, "G%1x%3x", &gbtype, &gbindex))
    {
        return 0;
    }

    switch (gbtype)
    {
        case FT_DATA_VEHIINFO:
            if (gbindex >= FT_MAX_VEHI_INFO)
            {
                log_e(LOG_FOTON, "vehicle info over %d! ", gbindex);
                break;
            }

            ftinf->vehi.info[gbindex] = sigid;
            break;

        case FT_DATA_MOTORINFO:
            if ((gbindex >> 8) >= FT_MAX_MOTOR)
            {
                log_e(LOG_FOTON, "motor ID over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= FT_MAX_MOTOR_INFO)
            {
                log_e(LOG_FOTON, "motor info over %d! ", gbindex & 0xff);
                break;
            }

            ftinf->motor[gbindex >> 8].info[gbindex & 0xff] = sigid;

            if ((gbindex >> 8) + 1 > ftinf->motor_cnt)
            {
                ftinf->motor_cnt = (gbindex >> 8) + 1;
            }

            break;
            
        case FT_DATA_FUELCELL:
        {
            if(FT_FCINF_CURRENT == gbindex)
            {
            	unsigned char flag = 0;
            	unsigned int  len = sizeof(flag);
				cfg_get_para(CFG_ITEM_FUELCELL, &flag, &len);
				if(!flag)
				{
					flag = 1;
					cfg_set_para(CFG_ITEM_FUELCELL, &flag, sizeof(flag));
				}
            }

            if(gbindex >= FT_FCINF_TEMPTAB)
            {
                ftinf->fuelcell.temp_cnt++;
                ftinf->fuelcell.temp[gbindex-FT_FCINF_TEMPTAB] = sigid;
            }
            else
            {
                if ((gbindex & 0xf) >= FT_FCINF_MAX)
                {
                    log_e(LOG_FOTON, "fuel cell info over %d! ", gbindex & 0xf);
                    break;
                }
                ftinf->fuelcell.info[gbindex & 0xf] = sigid;
            }
            break;
        }

        case FT_DATA_ENGINEINFO:
            if (gbindex >= FT_MAX_ENGIN_INFO)
            {
                log_e(LOG_FOTON, "engine info over %d! ", gbindex);
                break;
            }

            ftinf->engin.info[gbindex] = sigid;
            break;

        case FT_DATA_EXTREMA:
            if (gbindex >= FT_MAX_EXTR_INFO)
            {
                log_e(LOG_FOTON, "extrema info over %d! ", gbindex);
                break;
            }

            ftinf->extr[gbindex] = sigid;
            break;

        case FT_DATA_WARNNING:
            if ((gbindex >> 8) >= 4)
            {
                log_e(LOG_FOTON, "warnning level over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= FT_MAX_WARN_INFO)
            {
                log_e(LOG_FOTON, "warnning number over %d! ", gbindex & 0xff);
                break;
            }

            ftinf->warn[gbindex >> 8][gbindex & 0xff] = sigid;
            break;

        case FT_DATA_BATTVOLT:
            if ((gbindex >> 10) >= 1)
            {
                log_e(LOG_FOTON, "battery number over %u! ", gbindex >> 10);
            }
            else if ((gbindex & 0x3ff) == 0x3fe)
            {
                ftinf->batt.voltage = sigid;
            }
            else if ((gbindex & 0x3ff) == 0x3ff)
            {
                ftinf->batt.current = sigid;
            }
            else if ((gbindex & 0x3ff) >= FT_MAX_PACK_CELL)
            {
                log_e(LOG_FOTON, "battery cells count over %u! ", gbindex & 0x3ff);
            }
            else if (ftinf->batt.cell[gbindex & 0x3ff] == 0)
            {
                ftinf->batt.cell[gbindex & 0x3ff] = sigid;
                ftinf->batt.cell_cnt++;
                ftinf->adapter[FT_ADAPT_CELLS].def_val++;
            }

            break;

        case FT_DATA_BATTTEMP:
            if ((gbindex >> 8) >= 1)
            {
                log_e(LOG_FOTON, "battery number over %d! ", gbindex >> 8);
            }
            else if ((gbindex & 0xff) >= FT_MAX_PACK_TEMP)
            {
                log_e(LOG_FOTON, "battery temperature count over %d! ", gbindex & 0xff);
            }
            else if (ftinf->batt.temp[gbindex & 0xff] == 0)
            {
                ftinf->batt.temp[gbindex & 0xff] = sigid;
                ftinf->batt.temp_cnt++;
                ftinf->adapter[FT_ADAPT_TEMPS].def_val++;
            }

            break;

        case FT_DATA_VIRTUAL:
            log_i(LOG_FOTON, "get virtual channe %s", sfx);
            break;

        case FT_DATA_ADAPTATION:
            if (gbindex == FT_TOTAL_CELLS)
            {
                ftinf->adapter[FT_ADAPT_CELLS].sig = sigid;
            }
            else if (gbindex == FT_TOTAL_TEMPS)
            {
                ftinf->adapter[FT_ADAPT_TEMPS].sig = sigid;
            }
            else
            {
                log_e(LOG_FOTON, "get data adaptation %s,unused", sfx);
            }

            break;

        default:
            log_e(LOG_FOTON, "unkonwn type %s", sfx);
            break;
    }

    return 5;
}

static void ft_adapt_init(void)
{
    if (!ft_inf)
    {
        return;
    }

    char ver[COM_APP_VER_LEN];
    unsigned int value[FT_ADAPT_MAX];
    unsigned int len = sizeof(value);
    int i, ret;
    RTCTIME *time;
    ret = rds_get(RDS_ADAPTIVE_CFG, (unsigned char *) value, &len, ver);

    if (ret != 0)
    {
        log_o(LOG_FOTON, "get adaptive cfg failed, ret:0x%08x", ret);
        memset(value, 0, sizeof(value));
        ret = rds_update_once(RDS_ADAPTIVE_CFG, (unsigned char *) value, sizeof(value));
        log_o(LOG_FOTON, "init adaptive cfg, ret:0x%08x", ret);
    }

    for (i = 0; i < FT_ADAPT_MAX; i++)
    {
        if (value[i])
        {
            ft_inf->adapter[i].flg = FT_VALID;
            ft_inf->adapter[i].cur_val = ft_inf->adapter[i].cfg_val = value[i];
            ft_inf->adapter[i].tick = tm_get_time();
            tm_get_abstime(&ft_inf->adapter[i].time);
            time = &ft_inf->adapter[i].time;
            log_e(LOG_FOTON, "adapter[%d] from config value=%u", i, value[i]);
            log_e(LOG_FOTON, "uptick=%llu, time:%04d/%02d/%02d %02d:%02d:%02d", ft_inf->adapter[i].tick,
                  time->year, time->mon, time->mday, time->hour, time->min, time->sec);

            if (FT_ADAPT_CELLS == i)
            {
                ft_inf->batt.cell_cnt = value[i];
            }
            else
            {
                ft_inf->batt.temp_cnt = value[i];
            }
        }
        else
        {
            ft_inf->adapter[i].flg = FT_INITIAL;
        }
    }
}

static void ft_adapt_value_check(int sigid)
{
    if (!ft_inf)
    {
        return;
    }

    RTCTIME *time;
    int i, ret, is_save = 0;
    uint32_t temp_val = (uint32_t)dbc_get_signal_value(sigid);
    unsigned int value[FT_ADAPT_MAX];

    for (i = 0; i < FT_ADAPT_MAX; i++)
    {
        if (0 != ft_inf->adapter[i].sig && sigid == ft_inf->adapter[i].sig)
        {
            if (temp_val == 0)
            {
                ft_inf->adapter[i].flg = FT_ERR_CONFUSED;
                log_e(LOG_FOTON, "adapter[%d] ERR_CONFUSED!", i);
            }
            else if (temp_val > ft_inf->adapter[i].def_val)
            {
                ft_inf->adapter[i].flg = FT_ERR_OVERFLOW;
                log_e(LOG_FOTON, "adapter[%d] ERR_OVERFLOW, value=%u!", i, temp_val);
            }
            else
            {
                if (ft_inf->adapter[i].flg == FT_INITIAL)
                {
                    ft_inf->adapter[i].tick = tm_get_time();
                    tm_get_abstime(&ft_inf->adapter[i].time);
                    ft_inf->adapter[i].flg = FT_VALID;
                    time = &ft_inf->adapter[i].time;
                    log_e(LOG_FOTON, "adapter[%d] first adaptation value=%u", i, temp_val);
                    log_e(LOG_FOTON, "uptick=%llu, time:%04d/%02d/%02d %02d:%02d:%02d", ft_inf->adapter[i].tick,
                          time->year, time->mon, time->mday, time->hour, time->min, time->sec);
                    is_save = 1;

                    if (FT_ADAPT_CELLS == i)
                    {
                        ft_inf->batt.cell_cnt = ft_inf->adapter[i].cfg_val = temp_val;
                    }
                    else        // FT_ADAPT_TEMPS
                    {
                        ft_inf->batt.temp_cnt = ft_inf->adapter[i].cfg_val = temp_val;
                    }
                }
                else
                {
                    if (temp_val != ft_inf->adapter[i].cur_val)
                    {
                        ft_inf->adapter[i].flg = FT_CHANGED;
                        ft_inf->adapter[i].tick = tm_get_time();
                        tm_get_abstime(&ft_inf->adapter[i].time);
                        ft_inf->adapter[i].chg_cnt++;
                        time = &ft_inf->adapter[i].time;
                        log_e(LOG_FOTON, "adapter[%d] changed value=%u", i, temp_val);
                        log_e(LOG_FOTON, "uptick=%llu, time:%04d/%02d/%02d %02d:%02d:%02d", ft_inf->adapter[i].tick,
                              time->year, time->mon, time->mday, time->hour, time->min, time->sec);
                    }
                }
            }

            ft_inf->adapter[i].cur_val = temp_val;
        }
    }

    if (is_save)
    {
        for (i = 0; i < FT_ADAPT_MAX; i++)
        {
            value[i] = ft_inf->adapter[i].cfg_val;
        }

        ret = rds_update_once(RDS_ADAPTIVE_CFG, (unsigned char *) value, sizeof(value));
        log_e(LOG_FOTON, "save adaptive cfg, ret:0x%08x", ret);
    }
}

static void ft_adapt_timeout_check(void)
{
    int i, ret, is_save = 0;
    uint32_t temp_value;
    RTCTIME *time;
    unsigned int value[FT_ADAPT_MAX];

    for (i = 0; i < FT_ADAPT_MAX; i++)
    {
        if (ft_inf->adapter[i].flg == FT_CHANGED)
        {
            if (tm_get_time() - ft_inf->adapter[i].tick > FT_ADAPT_TIMEOUT)
            {
                temp_value = ft_inf->adapter[i].cur_val;
                ft_inf->adapter[i].tick = tm_get_time();
                tm_get_abstime(&ft_inf->adapter[i].time);
                ft_inf->adapter[i].flg = FT_VALID;
                time = &ft_inf->adapter[i].time;
                log_e(LOG_FOTON, "adapter[%d] take effect value=%u", i, temp_value);
                log_e(LOG_FOTON, "uptick=%llu, time:%04d/%02d/%02d %02d:%02d:%02d", ft_inf->adapter[i].tick,
                      time->year, time->mon, time->mday, time->hour, time->min, time->sec);
                is_save = 1;

                if (FT_ADAPT_CELLS == i)
                {
                    ft_inf->batt.cell_cnt = ft_inf->adapter[i].cfg_val = temp_value;
                }
                else     // FT_ADAPT_TEMPS
                {
                    ft_inf->batt.temp_cnt = ft_inf->adapter[i].cfg_val = temp_value;
                }
            }
        }

    }

    if (is_save)
    {
        for (i = 0; i < FT_ADAPT_MAX; i++)
        {
            value[i] = ft_inf->adapter[i].cfg_val;
        }

        ret = rds_update_once(RDS_ADAPTIVE_CFG, (unsigned char *) value, sizeof(value));
        log_e(LOG_FOTON, "save adaptive cfg, ret:0x%08x", ret);
    }
}

static void ft_shell_adapt_show(int argc, const char **argv)
{
    int i;
    RTCTIME *time;
    dbc_lock();

    if (ft_inf == NULL)
    {
        shellprintf(" dbc file is not loaded\r\n");
    }
    else
    {
        for (i = 0; i < FT_ADAPT_MAX; i++)
        {
            time = &ft_inf->adapter[i].time;
            shellprintf(" adapter[%d]:stat=%02d,rpt_val=%03u,cur_val=%03u,def_val=%03u,cfg_val=%03u,chg=%u\r\n",
                        i, ft_inf->adapter[i].flg, i ? ft_inf->batt.temp_cnt : ft_inf->batt.cell_cnt,
                        ft_inf->adapter[i].cur_val, ft_inf->adapter[i].def_val, ft_inf->adapter[i].cfg_val,
                        ft_inf->adapter[i].chg_cnt);
            shellprintf(" \tuptick=%llu,time:%04d/%02d/%02d %02d:%02d:%02d\r\n",
                        ft_inf->adapter[i].tick,
                        time->year, time->mon, time->mday, time->hour, time->min, time->sec);
        }
    }

    dbc_unlock();
}

static int ft_data_dbc_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    static ft_info_t *ft_rld = NULL;
    int ret = 0;
    int i;

    switch (event)
    {
        case DBC_EVENT_RELOAD:
            {
                ft_info_t *next;
                
                ft_rld = ft_inf == NULL ? ft_infmem : ft_inf->next;
                next = ft_rld->next;
                memset(ft_rld, 0, sizeof(ft_info_t));
                ft_rld->next = next;
                ft_rld->vehi.vehi_type = FT_VEHITYPE_ELECT;
                break;
            }

        case DBC_EVENT_FINISHED:
            if (ft_rld && arg1 == 0)
            {

				for (i = 0; i < ft_rld->batt.cell_cnt && ft_rld->batt.cell[i]; i++);

				if (i < ft_rld->batt.cell_cnt)
				{
					log_e(LOG_FOTON, "battery cell defined in dbc is not incorrect");
					break;
				}

				for (i = 0; i < ft_rld->batt.temp_cnt && ft_rld->batt.temp[i]; i++);

				if (i < ft_rld->batt.temp_cnt)
				{
					log_e(LOG_FOTON, "temperature defined in dbc is not incorrect");
					break;
				}
				
                ft_inf = ft_rld;
                ft_adapt_init();

                for (i = 0; i < FT_MAX_WARN_INFO; i++)
                {
                    if (ft_inf->warn[2][i] != 0)
                    {
                        dbc_set_signal_flag(ft_inf->warn[2][i], ft_warnflag);
                    }
                }

                ERR_LOCK();

                if (GROUP_SIZE(ft_inf) > 0)
                {
                    ft_errlst_head = &ft_errmem[0].link;
                    list_init(ft_errlst_head);

                    for (i = 1; i < GROUP_SIZE(ft_inf) * 30; i++)
                    {
                        ft_errmem[i].len  = 0;
                        ft_errmem[i].type = FT_RPTTYPE_DELAY;
                        ft_errmem[i].list = &ft_delay_lst;
                        list_insert_before(ft_errlst_head, &ft_errmem[i].link);
                    }
                }
                else
                {
                    ft_errlst_head = NULL;
                }

                ERR_UNLOCK();
                //ft_data_clear_report();
            }

            ft_rld = NULL;
            break;

        case DBC_EVENT_DEFINE:
            if (ft_rld)
            {
                ft_data_parse_define(ft_rld, (const char *)arg1);
            }

            break;

        case DBC_EVENT_SURFIX:
            if (ft_rld)
            {
                ret = ft_data_parse_surfix(ft_rld, (int)arg1, (const char *)arg2);
            }

            break;

        case DBC_EVENT_UPDATE:
            if (ft_inf &&
                dbc_test_signal_flag((int)arg1, ft_warnflag, 0) &&
                dbc_get_signal_lastval((int)arg1) == 0 &&
                dbc_get_signal_value((int)arg1) != 0)
            {
                log_e(LOG_FOTON, "warnning triggered");
                ft_inf->warntrig = 1;
            }

            ft_adapt_value_check((int)arg1);
            break;

        default:
            break;
    }

    return ret;
}

static void ft_data_periodic(ft_info_t *ftinf, int intv, uint32_t uptime)
{
    int period;
    static int ticks = 0, times = 0, triger = 0;

    ft_adapt_timeout_check();

    if (ftinf->warntrig)
    {
        RTCTIME time;

        ftinf->warntrig = 0;
        times = 30 + 1;

        if (ft_pendflag)
        {
            ft_data_flush_realtm();
            ft_data_flush_error();
        }
        else
        {
            triger = 1;
        }

        can_get_time(uptime, &time);
        log_e(LOG_FOTON, "level 3 error start: %u, %02d/%02d/%02d, %02d:%02d:%02d",
              uptime, time.year, time.mon, time.mday, time.hour, time.min, time.sec);
    }

    ticks++;

    if (times == 0)
    {
        ft_data_save_error(ftinf, uptime);
        period = intv;
    }
    else if (--times == 0)
    {
        period = intv;
    }
    else
    {
        period = 1;
    }

    if (ticks >= period)
    {
        ticks = 0;
        ft_data_save_realtm(ftinf, uptime);

        if (triger)
        {
            triger = 0;
            ft_data_flush_error();
        }
    }
}

static int ft_data_can_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    static int canact = 0;

    switch (event)
    {
        case CAN_EVENT_ACTIVE:
            canact = 1;
            break;

        case CAN_EVENT_SLEEP:
        case CAN_EVENT_TIMEOUT:
            canact = 0;
			//ft_is_allow_login = 0;
            ft_data_clear_error();
            break;

        case CAN_EVENT_DATAIN:
            {
                static int counter = 0;
                CAN_MSG *msg = (CAN_MSG *)arg1;

                while (canact && ft_inf && arg2--)
                {
                    if (msg->type == 'T' && ++counter == 100)
                    {
                    	uint8_t onvalue; 
						unsigned int len = 1;
                        counter = 0;

					    st_get(ST_ITEM_KL15_SIG, &onvalue, &len);
						
						if((0 < dbc_get_signal_value(ft_inf->vehi.info[FT_VINF_CHARGE])) || onvalue)
						{
							//ft_is_allow_login = 1;
							ft_allow_sleep = 0;
							ft_data_periodic(ft_inf, ft_datintv, msg->uptime);
						}
						else
						{
							//ft_is_allow_login = 0;
							//continue;
						}
						
                        
                    }

                    msg++;
                }

                break;
            }

        default:
            break;
    }

    return 0;
}

#include "foton_disp.h"

static int ft_shell_dumpgb(int argc, const char **argv)
{
    dbc_lock();

    if (ft_inf == NULL)
    {
        shellprintf(" dbc file is not loaded\r\n");
    }
    else
    {
        shellprintf(" [车辆信息]\r\n");
        ft_disp_vinf(&ft_inf->vehi);

        if (ft_inf->vehi.vehi_type == FT_VEHITYPE_ELECT ||
            ft_inf->vehi.vehi_type == FT_VEHITYPE_HYBIRD)
        {
            int i;

            for (i = 0; i < ft_inf->motor_cnt; i++)
            {
                shellprintf(" [电机信息-%d]\r\n", i + 1);
                ft_disp_minf(&ft_inf->motor[i]);
            }

            shellprintf(" [燃料电池信息]\r\n");
            ft_disp_finf(&ft_inf->fuelcell);

            shellprintf(" [电池信息]\r\n");
            ft_disp_binf(&ft_inf->batt);
            shellprintf(" [极值信息]  (如果全部未定义，则由终端计算)\r\n");
            ft_disp_xinf(ft_inf->extr);
        }

        if (ft_inf->vehi.vehi_type == FT_VEHITYPE_GASFUEL ||
            ft_inf->vehi.vehi_type == FT_VEHITYPE_HYBIRD)
        {
            shellprintf(" [发动机信息]\r\n");
            ft_disp_einf(&ft_inf->engin);
        }

        shellprintf(" [报警信息-1级]\r\n");
        ft_disp_winf(ft_inf->warn[0]);
        shellprintf(" [报警信息-2级]\r\n");
        ft_disp_winf(ft_inf->warn[1]);
        shellprintf(" [报警信息-3级]\r\n");
        ft_disp_winf(ft_inf->warn[2]);
    }

    dbc_unlock();
    return 0;
}


static int ft_shell_setintv(int argc, const const char **argv)
{
    uint16_t intv;

    if (argc != 1 || sscanf(argv[0], "%hu", &intv) != 1)
    {
        shellprintf(" usage: gbsetintv <interval seconds>\r\n");
        return -1;
    }

    if (intv == 0)
    {
        shellprintf(" error: data interval can't be 0\r\n");
        return -1;
    }

    if (ft_set_datintv(intv))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    return 0;
}


static int ft_shell_testwarning(int argc, const const char **argv)
{
    int on;

    if (argc != 1 || sscanf(argv[0], "%d", &on) != 1)
    {
        shellprintf(" usage: gbtestwarn <0/1>\r\n");
        return -1;
    }

    dbc_lock();

    if (ft_inf)
    {
        if (!ft_inf->warntest && on)
        {
            ft_inf->warntrig = 1;
        }

        ft_inf->warntest = on;
    }

    dbc_unlock();
    return 0;
}

int ft_data_init(INIT_PHASE phase)
{
    int ret = 0, i;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            ft_infmem[0].next = ft_infmem + 1;
            ft_infmem[1].next = ft_infmem;
            ft_inf = NULL;
            ft_errlst_head = NULL;
            ft_datintv = 10;

            list_init(&ft_realtm_lst);
            list_init(&ft_delay_lst);
            list_init(&ft_trans_lst);
            list_init(&ft_free_lst);

            for (i = 0; i < FT_MAX_REPORT; i++)
            {
                list_insert_before(&ft_free_lst, &ft_datamem[i].link);
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                uint32_t cfglen;

                if (ft_cache_load_info(&ft_index_r, &ft_index_w) != 0)
                {
                    log_e(LOG_FOTON, "load cache_info fail");
                }

                ft_warnflag = dbc_request_flag();
                ret |= dbc_register_callback(ft_data_dbc_cb);
                ret |= can_register_callback(ft_data_can_cb);
                ret |= shell_cmd_register("ftdump", ft_shell_dumpgb, "show FOTON signals information");
                ret |= shell_cmd_register("ftsetintv", ft_shell_setintv, "set FOTON report period");
                ret |= shell_cmd_register("fttestwarn", ft_shell_testwarning, "test FOTON warnning");
                ret |= shell_cmd_register_ex("ftadp", NULL, ft_shell_adapt_show, "show FOTON adaptive variable");
                ret |= pthread_mutex_init(&ft_errmtx, NULL);
                ret |= pthread_mutex_init(&ft_datmtx, NULL);

                cfglen = sizeof(ft_datintv);
                ret |= cfg_get_para(CFG_ITEM_FOTON_INTERVAL, &ft_datintv, &cfglen);
                break;
            }
    }

    return ret;
}


void ft_data_put_back(ft_pack_t *pack)
{
    DAT_LOCK();
    list_insert_after(pack->list, &pack->link);
    DAT_UNLOCK();
}

void ft_data_put_send(ft_pack_t *pack)
{
    DAT_LOCK();
    list_insert_before(&ft_trans_lst, &pack->link);
    DAT_UNLOCK();
}

void ft_data_ack_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&ft_trans_lst)) != NULL)
    {
        list_insert_before(&ft_free_lst, node);
    }

    DAT_UNLOCK();
}

static int ft_emerg;

void ft_data_emergence(int set)
{
    DAT_LOCK();
    ft_emerg = set;
    DAT_UNLOCK();
}

ft_pack_t *ft_data_get_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&ft_realtm_lst)) == NULL)
    {
        if (list_empty(&ft_delay_lst))
        {
            ft_cache_load();
        }

        node = ft_emerg ? list_get_last(&ft_delay_lst) : list_get_first(&ft_delay_lst);
    }

    DAT_UNLOCK();

    return node == NULL ? NULL : list_entry(node, ft_pack_t, link);;
}

int ft_data_nosending(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&ft_trans_lst);
    DAT_UNLOCK();
    return ret;
}

int ft_data_noreport(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&ft_realtm_lst) && list_empty(&ft_delay_lst);
    DAT_UNLOCK();
    return ret;
}

void ft_data_set_intv(uint16_t intv)
{
    ft_datintv = intv;
}

int ft_data_get_intv(void)
{
    return ft_datintv;
}

void ft_data_set_pendflag(int flag)
{
    ft_pendflag = flag;
}

