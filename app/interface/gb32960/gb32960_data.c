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
#include "gb32960.h"
#include "gb32960_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "../support/protocol.h"

#define GB_MAX_PACK_CELL    800
#define GB_MAX_PACK_TEMP    800
#define GB_MAX_FUEL_TEMP    200
#define GB_MAX_FUEL_INFO    16
#define GB_MAX_VEHI_INFO    16
#define GB_MAX_WARN_INFO    32
#define GB_MAX_MOTOR_INFO   8
#define GB_MAX_ENGIN_INFO   4
#define GB_MAX_EXTR_INFO    16
#define GB_MAX_MOTOR        4

/* vehicle type */
#define GB_VEHITYPE_ELECT   0x01
#define GB_VEHITYPE_HYBIRD  0x02
#define GB_VEHITYPE_GASFUEL 0x03

/* vehicle information index */
#define GB_VINF_STATE       0x00
#define GB_VINF_CHARGE      0x01
#define GB_VINF_SPEED       0x02
#define GB_VINF_ODO         0x03
#define GB_VINF_VOLTAGE     0x04
#define GB_VINF_CURRENT     0x05
#define GB_VINF_SOC         0x06
#define GB_VINF_DCDC        0x07
#define GB_VINF_SHIFT       0x08
#define GB_VINF_INSULAT     0x09
#define GB_VINF_ACCPAD      0x0a
#define GB_VINF_BRKPAD      0x0b
#define GB_VINF_DRVIND      0x0c
#define GB_VINF_BRKIND      0x0d
#define GB_VINF_VEHIMODE    0x0e
#define GB_VINF_MAX         GB_VINF_VEHIMODE + 1

/* motor information index */
#define GB_MINF_STATE       0x00
#define GB_MINF_MCUTMP      0x01
#define GB_MINF_SPEED       0x02
#define GB_MINF_TORQUE      0x03
#define GB_MINF_MOTTMP      0x04
#define GB_MINF_VOLTAGE     0x05
#define GB_MINF_CURRENT     0x06
#define GB_MINF_MAX         GB_MINF_CURRENT + 1

/* fuel cell information index */
#define GB_FCINF_VOLTAGE    0x00
#define GB_FCINF_CURRENT    0x01
#define GB_FCINF_RATE       0x02
#define GB_FCINF_MAXTEMP    0x03
#define GB_FCINF_MAXTEMPID  0x04
#define GB_FCINF_MAXCCTT    0x05
#define GB_FCINF_MAXCCTTID  0x06
#define GB_FCINF_MAXPRES    0x07
#define GB_FCINF_MAXPRESID  0x08
#define GB_FCINF_HVDCDC     0x09
#define GB_FCINF_MAX        GB_FCINF_HVDCDC + 1
#define GB_FCINF_TEMPTAB    0x100


/* engine information index */
#define GB_EINF_STATE       0x00
#define GB_EINF_SPEED       0x01
#define GB_EINF_FUELRATE    0x02
#define GB_EINF_MAX         GB_EINF_FUELRATE + 1

/* extremum index */
#define GB_XINF_MAXVPID     0x00
#define GB_XINF_MAXVCID     0x01
#define GB_XINF_MAXV        0x02
#define GB_XINF_MINVPID     0x03
#define GB_XINF_MINVCID     0x04
#define GB_XINF_MINV        0x05
#define GB_XINF_MAXTPID     0x06
#define GB_XINF_MAXTCID     0x07
#define GB_XINF_MAXT        0x08
#define GB_XINF_MINTPID     0x09
#define GB_XINF_MINTCID     0x0a
#define GB_XINF_MINT        0x0b
#define GB_XINF_MAX         GB_XINF_MINT + 1

/* battery information index */
#define GB_BINF_VOLTAGE     0x3fe
#define GB_BINF_CURRENT     0x3ff

/* GB32960 data type */
#define GB_DATA_VEHIINFO    0x01
#define GB_DATA_MOTORINFO   0x02
#define GB_DATA_ENGINEINFO  0x04
#define GB_DATA_LOCATION    0x05
#define GB_DATA_EXTREMA     0x06
#define GB_DATA_WARNNING    0x07
#define GB_DATA_BATTVOLT    0x08
#define GB_DATA_BATTTEMP    0x09
#define GB_DATA_FUELCELL    0x0A
#define GB_DATA_VIRTUAL     0x0B

/* report data type */
#define GB_RPTTYPE_REALTM   0x02
#define GB_RPTTYPE_DELAY    0x03

/* report packets parameter */
#define GB_MAX_REPORT       2000

/* battery information structure */
typedef struct
{
    int voltage;
    int current;
    int cell[GB_MAX_PACK_CELL];
    int temp[GB_MAX_PACK_TEMP];
    uint32_t   cell_cnt;
    uint32_t   temp_cnt;
} gb_batt_t;
/* motor information structure */
typedef struct
{
    int info[GB_MAX_MOTOR_INFO];
    uint8_t state_tbl[256];
} gb_motor_t;
/* vehicle information structure */
typedef struct
{
    int info[GB_MAX_VEHI_INFO];
    uint8_t state_tbl[256];
    uint8_t mode_tbl[256];
    char    shift_tbl[256];
    uint8_t charge_tbl[256];
    uint8_t dcdc_tbl[256];
    uint8_t vehi_type;
} gb_vehi_t;

/* fuel cell information structure */
typedef struct
{
    int info[GB_MAX_FUEL_INFO];
    int temp[GB_MAX_FUEL_TEMP];
    int temp_cnt;
    uint8_t hvdcdc[8];
} gb_fuelcell_t;

/* engine information structure */
typedef struct
{
    int info[GB_MAX_ENGIN_INFO];
    uint8_t state_tbl[256];
} gb_engin_t;
/* GB32960 information structure */
typedef struct _gb_info_t
{
    gb_vehi_t  vehi;
    gb_motor_t motor[GB_MAX_MOTOR];
    uint32_t   motor_cnt;
    gb_fuelcell_t fuelcell;
    gb_engin_t engin;
    gb_batt_t  batt;
    int warn[4][GB_MAX_WARN_INFO];//index 3,as a relevance channel
    int extr[GB_MAX_EXTR_INFO];
    int warntrig;
    int warntest;
    struct _gb_info_t *next;
} gb_info_t;



static gb_info_t  gb_infmem[2];
static gb_info_t *gb_inf;
static gb_pack_t  gb_datamem[GB_MAX_REPORT];
static gb_pack_t  gb_errmem[(GB_MAX_PACK_CELL + 199) / 200 * 30];
static list_t     gb_free_lst;
static list_t     gb_realtm_lst;
static list_t     gb_delay_lst;
static list_t     gb_trans_lst;
static list_t    *gb_errlst_head;
static int        gb_warnflag;
static int        gb_pendflag;
static pthread_mutex_t gb_errmtx;
static pthread_mutex_t gb_datmtx;
static uint16_t gb_datintv;

#define ERR_LOCK()          pthread_mutex_lock(&gb_errmtx)
#define ERR_UNLOCK()        pthread_mutex_unlock(&gb_errmtx)
#define DAT_LOCK()          pthread_mutex_lock(&gb_datmtx)
#define DAT_UNLOCK()        pthread_mutex_unlock(&gb_datmtx)
#define GROUP_SIZE(inf)     RDUP_DIV((inf)->batt.cell_cnt, 200)



static uint32_t gb_data_save_vehi(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : vehicle information */
    buf[len++] = GB_DATA_VEHIINFO;

    /* vehicle state */
    if (gbinf->vehi.info[GB_VINF_STATE])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_STATE])->value;
        buf[len++] = gbinf->vehi.state_tbl[tmp] ? gbinf->vehi.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* charge state */
    if (gbinf->vehi.info[GB_VINF_CHARGE])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_CHARGE])->value;
        buf[len++] = gbinf->vehi.charge_tbl[tmp] ? gbinf->vehi.charge_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* vehicle type */
    if (gbinf->vehi.info[GB_VINF_VEHIMODE])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_VEHIMODE])->value;
        buf[len++] = gbinf->vehi.mode_tbl[tmp] ? gbinf->vehi.mode_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = gbinf->vehi.vehi_type;
    }

    /* vehicle speed, scale 0.1km/h */
    tmp = gbinf->vehi.info[GB_VINF_SPEED] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SPEED])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* odograph, scale 0.1km */
    tmp = gbinf->vehi.info[GB_VINF_ODO] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_ODO])->value * 10 : 0xffffffff;
    buf[len++] = tmp >> 24;
    buf[len++] = tmp >> 16;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total voltage, scale 0.1V */
    tmp = gbinf->vehi.info[GB_VINF_VOLTAGE] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_VOLTAGE])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total voltage, scale 0.1V, offset -1000A */
    tmp = gbinf->vehi.info[GB_VINF_CURRENT] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_CURRENT])->value * 10 + 10000 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total SOC */
    tmp = gbinf->vehi.info[GB_VINF_SOC] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SOC])->value : 0xff;
    buf[len++] = tmp;

    /* DCDC state */
    if (gbinf->vehi.info[GB_VINF_DCDC])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_DCDC])->value;
        buf[len++] = gbinf->vehi.dcdc_tbl[tmp] ? gbinf->vehi.dcdc_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* shift state */
    if (gbinf->vehi.info[GB_VINF_SHIFT])
    {
        uint8_t sft;

        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SHIFT])->value;
        sft = gbinf->vehi.shift_tbl[tmp];

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

        if (gbinf->vehi.info[GB_VINF_DRVIND])
        {
            if (dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_DRVIND])->value > 0)
            {
                tmp |= 0x20;
            }
        }
        else if (gbinf->vehi.info[GB_VINF_ACCPAD])
        {
            if (dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_ACCPAD])->value > 0)
            {
                tmp |= 0x20;
            }
        }

        if (gbinf->vehi.info[GB_VINF_BRKIND])
        {
            if (dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_BRKIND])->value > 0)
            {
                tmp |= 0x10;
            }
        }
        else if (gbinf->vehi.info[GB_VINF_BRKPAD])
        {
            if (dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_BRKPAD])->value > 0)
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
    tmp = gbinf->vehi.info[GB_VINF_INSULAT] ?
          MIN(dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_INSULAT])->value, 60000) : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* accelate pad value */
    tmp = gbinf->vehi.info[GB_VINF_ACCPAD] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_ACCPAD])->value : 0xff;
    buf[len++] = tmp;

    /* break pad value */
    tmp = gbinf->vehi.info[GB_VINF_BRKPAD] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_BRKPAD])->value : 0xff;
    buf[len++] = tmp;

    return len;
}

static uint32_t gb_data_save_cell(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp, i, cells;
    static uint32_t start = 0;

    buf[len++] = GB_DATA_BATTVOLT;
    buf[len++] = 1;
    buf[len++] = 1;

    /* packet voltage, scale 0.1V */
    tmp = gbinf->batt.voltage ?
          dbc_get_signal_from_id(gbinf->batt.voltage)->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* packet current, scale 0.1A, offset -1000A */
    tmp = gbinf->batt.current ?
          dbc_get_signal_from_id(gbinf->batt.current)->value * 10 + 10000 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total cell count */
    buf[len++] = gbinf->batt.cell_cnt >> 8;
    buf[len++] = gbinf->batt.cell_cnt;

    /* start cell of current frame */
    buf[len++] = (start + 1) >> 8;
    buf[len++] = (start + 1);

    /* cell count of current frame */
    cells = MIN(gbinf->batt.cell_cnt - start, 200);
    buf[len++] = cells;

    for (i = start; i < start + cells; i++)
    {
        tmp = gbinf->batt.cell[i] ?
              dbc_get_signal_from_id(gbinf->batt.cell[i])->value * 1000:0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    start = (start + cells) % gbinf->batt.cell_cnt;

    return len;
}

static uint32_t gb_data_save_temp(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i;

    buf[len++] = GB_DATA_BATTTEMP;
    buf[len++] = 1;
    buf[len++] = 1;

    /* total temp count */
    buf[len++] = gbinf->batt.temp_cnt >> 8;
    buf[len++] = gbinf->batt.temp_cnt;

    for (i = 0; i < gbinf->batt.temp_cnt; i++)
    {
        buf[len++] = gbinf->batt.temp[i]?
			(uint8_t)(dbc_get_signal_from_id(gbinf->batt.temp[i])->value + 40):0xff;
    }

    return len;
}

static uint32_t gb_data_save_motor(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmp;

    buf[len++] = GB_DATA_MOTORINFO;
    buf[len++] = gbinf->motor_cnt;

    for (i = 0; i < gbinf->motor_cnt; i++)
    {
        /* motor number */
        buf[len++] = i + 1;

        /* motor state */
        if (gbinf->motor[i].info[GB_MINF_STATE])
        {
            tmp = dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_STATE])->value;
            buf[len++] = gbinf->motor[i].state_tbl[tmp] ?
                         gbinf->motor[i].state_tbl[tmp] : 0xff;
        }
        else
        {
            buf[len++] = 0xff;
        }

        /* MCU temperature */
        tmp = gbinf->motor[i].info[GB_MINF_MCUTMP] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_MCUTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor speed, offset -20000rpm */
        tmp = gbinf->motor[i].info[GB_MINF_SPEED] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_SPEED])->value + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor torque, scale 0.1Nm, offset -2000Nm */
        tmp = gbinf->motor[i].info[GB_MINF_TORQUE] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_TORQUE])->value * 10 + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor temperature */
        tmp = gbinf->motor[i].info[GB_MINF_MOTTMP] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_MOTTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor voltage, scale 0.1V*/
        tmp = gbinf->motor[i].info[GB_MINF_VOLTAGE] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_VOLTAGE])->value * 10 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor current, scale 0.1A, offset -1000A */
        tmp = gbinf->motor[i].info[GB_MINF_CURRENT] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_CURRENT])->value * 10 + 10000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    return len;
}

static uint32_t gb_data_save_fuelcell(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp,i;

    /* data type : fuel cell information */
    buf[len++] = 0x03;

    /* fuel cell voltage value */
    tmp = gbinf->fuelcell.info[GB_FCINF_VOLTAGE] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_VOLTAGE])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell current value */
    tmp = gbinf->fuelcell.info[GB_FCINF_CURRENT] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_CURRENT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;    
    
    /* fuel cell consumption rate */
    tmp = gbinf->fuelcell.info[GB_FCINF_RATE] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_RATE])->value*100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell temperature needle number */
    buf[len++] = gbinf->fuelcell.temp_cnt >> 8;
    buf[len++] = gbinf->fuelcell.temp_cnt;

    for(i = 0;i < gbinf->fuelcell.temp_cnt; i++)
    {
        /*highest temperature of hydrogen system  */
        tmp = gbinf->fuelcell.temp[i] ? 
            (dbc_get_signal_from_id(gbinf->fuelcell.temp[i])->value + 40) : 0xff;
        buf[len++] = tmp;
    }
    
    /* highest temperature of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXTEMP] ? 
            (dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXTEMP])->value*10 + 400) : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest temperature of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXTEMPID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXTEMPID])->value : 0xff;
    buf[len++] = tmp;

    /* highest hydrogen rate  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXCCTT] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXCCTT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* the ID of highest hydrogen rate  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXCCTTID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXCCTTID])->value : 0xff;
    buf[len++] = tmp;

    /*highest pressure of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXPRES] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXPRES])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest pressure of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXPRESID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXPRESID])->value : 0xff;
    buf[len++] = tmp;

    /* High voltage DCDC state */
    if (gbinf->fuelcell.info[GB_FCINF_HVDCDC])
    {
        tmp = dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_HVDCDC])->value;
        buf[len++] = gbinf->fuelcell.hvdcdc[tmp] ? gbinf->fuelcell.hvdcdc[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }


    return len;
}

static uint32_t gb_data_save_extr(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmpv, tmpi;
    uint32_t maxvid = 0, maxtid = 0, minvid = 0, mintid = 0;
    double maxv, maxt, minv, mint;
    static uint32_t packnum = 0;

    maxvid = maxtid = minvid = mintid = 1;
    maxv = minv = gbinf->batt.cell_cnt > 0 ? dbc_get_signal_from_id(gbinf->batt.cell[0])->value : 0;
    maxt = mint = gbinf->batt.temp_cnt > 0 ? dbc_get_signal_from_id(gbinf->batt.temp[0])->value : 0;

    for (i = 0; i < gbinf->batt.cell_cnt; i++)
    {
        double value = dbc_get_signal_from_id(gbinf->batt.cell[i])->value;

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

    for (i = 0; i < gbinf->batt.temp_cnt; i++)
    {
        double value = dbc_get_signal_from_id(gbinf->batt.temp[i])->value;

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

    if (gbinf->extr[GB_XINF_MAXVCID])
    {
        maxvid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXVCID])->value;
    }

    if (gbinf->extr[GB_XINF_MAXV])
    {
        maxv = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXV])->value;
    }

    if (gbinf->extr[GB_XINF_MAXTCID])
    {
        maxtid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXTCID])->value;
    }

    if (gbinf->extr[GB_XINF_MAXT])
    {
        maxt = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXT])->value;
    }

    if (gbinf->extr[GB_XINF_MINVCID])
    {
        minvid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINVCID])->value;
    }

    if (gbinf->extr[GB_XINF_MINV])
    {
        minv = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINV])->value;
    }

    if (gbinf->extr[GB_XINF_MINTCID])
    {
        mintid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINTCID])->value;
    }

    if (gbinf->extr[GB_XINF_MINT])
    {
        mint = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINT])->value;
    }

    buf[len++] = GB_DATA_EXTREMA;

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

    packnum = (packnum + 1) % GROUP_SIZE(gbinf);

    return len;
}

static uint32_t gb_data_save_warn(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, j, warnbit = 0, warnlvl = 0;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if (gbinf->warn[i][j] && 
            (dbc_get_signal_from_id(gbinf->warn[i][j])->value || 
            (gbinf->warn[3][j] && 
            dbc_get_signal_from_id(gbinf->warn[3][j])->value)))
            // index 3,as a relevance channel,if the is two canid used for on warning
            {
                warnbit |= 1 << j;
                warnlvl  = i + 1;
            }
        }
    }

    if (gbinf->warntest)
    {
        warnbit |= 1;
        warnlvl  = 3;
    }

    buf[len++] = GB_DATA_WARNNING;
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

static uint32_t gb_data_save_engin(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : engine information */
    buf[len++] = GB_DATA_ENGINEINFO;

    /* engine state */
    if (gbinf->engin.info[GB_EINF_STATE])
    {
        tmp = dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_STATE])->value;
        buf[len++] = gbinf->engin.state_tbl[tmp] ? gbinf->engin.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* engine speed, scale 1rpm */
    tmp = gbinf->engin.info[GB_EINF_SPEED] ?
          dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_SPEED])->value : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fule rate, scale 0.01L/100km */
    tmp = gbinf->engin.info[GB_EINF_FUELRATE] ?
          dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_FUELRATE])->value * 100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    return len;
}


/* Convert dddmm.mmmm(double) To ddd.dd+(double)*/
static uint32_t gb_data_gpsconv(double dddmm)
{
    int deg;
    double min;

    deg = dddmm / 100.0;
    min = dddmm - deg * 100;

    return (uint32_t)((deg + min / 60 + 0.5E-6) * 1000000);
}


static uint32_t gb_data_save_gps(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0;
    GPS_DATA gpsdata;

    static uint32_t  longitudeBak = 0;
    static uint32_t  latitudeBak  = 0;

    /* data type : location data */
    buf[len++] = GB_DATA_LOCATION;

    /* status bits */
    /* bit-0: 0-A,1-V */
    /* bit-1: 0-N,1-S */
    /* bit-2: 0-E,1-W */
    if (gps_get_fix_status() == 2)
    {
        gps_get_snap(&gpsdata);
        longitudeBak = gb_data_gpsconv(gpsdata.longitude);
        latitudeBak  = gb_data_gpsconv(gpsdata.latitude);
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

static uint32_t gb_data_save_all(gb_info_t *gbinf, uint8_t *buf, uint32_t uptime)
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

    len += gb_data_save_vehi(gbinf, buf + len);

    if (gbinf->vehi.vehi_type == GB_VEHITYPE_ELECT ||
        gbinf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
    {   
        uint8_t fuelcell;
        uint32_t paralen = sizeof(fuelcell);
        len += gb_data_save_motor(gbinf, buf + len);        
        if(0 == cfg_get_para(CFG_ITEM_FUELCELL, &fuelcell, &paralen) && fuelcell)
        {
            len += gb_data_save_fuelcell(gbinf, buf + len);
        }
        len += gb_data_save_cell(gbinf, buf + len);
        len += gb_data_save_temp(gbinf, buf + len);
        len += gb_data_save_extr(gbinf, buf + len);
    }

    if (gbinf->vehi.vehi_type == GB_VEHITYPE_GASFUEL ||
        gbinf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
    {
        len += gb_data_save_engin(gbinf, buf + len);
    }

    len += gb_data_save_gps(gbinf, buf + len);
    len += gb_data_save_warn(gbinf, buf + len);

    return len;
}

static void gb_data_save_realtm(gb_info_t *gbinf, uint32_t uptime)
{
    int i;

    DAT_LOCK();

    for (i = 0; i < GROUP_SIZE(gbinf); i++)
    {
        gb_pack_t *rpt;
        list_t *node;

        if ((node = list_get_first(&gb_free_lst)) == NULL)
        {
            if ((node = list_get_first(&gb_delay_lst)) == NULL &&
                (node = list_get_first(&gb_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_GB32960, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        rpt = list_entry(node, gb_pack_t, link);
        rpt->len  = gb_data_save_all(gbinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        rpt->list = &gb_realtm_lst;
        rpt->type = GB_RPTTYPE_REALTM;
        list_insert_before(&gb_realtm_lst, node);
    }

    DAT_UNLOCK();
}

static void gb_data_save_error(gb_info_t *gbinf, uint32_t uptime)
{
    int i;

    ERR_LOCK();

    for (i = 0; i < GROUP_SIZE(gbinf); i++)
    {
        gb_pack_t *rpt = list_entry(gb_errlst_head, gb_pack_t, link);

        rpt->len  = gb_data_save_all(gbinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        gb_errlst_head = gb_errlst_head->next;
    }

    ERR_UNLOCK();
}

static void gb_data_flush_error(void)
{
    list_t tmplst;

    list_init(&tmplst);
    ERR_LOCK();

    if (gb_errlst_head == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    while (list_entry(gb_errlst_head->prev, gb_pack_t, link)->len)
    {
        list_t *node;
        gb_pack_t *rpt, *err;

        DAT_LOCK();

        if ((node = list_get_first(&gb_free_lst)) == NULL)
        {
            log_e(LOG_GB32960, "report buffer is full, discard the oldest one");

            if ((node = list_get_first(&gb_delay_lst)) == NULL &&
                (node = list_get_first(&gb_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_GB32960, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        DAT_UNLOCK();

        gb_errlst_head = gb_errlst_head->prev;
        err = list_entry(gb_errlst_head, gb_pack_t, link);
        rpt = list_entry(node, gb_pack_t, link);
        memcpy(rpt, err, sizeof(gb_pack_t));
        err->len  = 0;
        rpt->list = &gb_delay_lst;
        rpt->type = GB_RPTTYPE_DELAY;
        list_insert_after(&tmplst, node);
    }

    ERR_UNLOCK();

    if (!list_empty(&tmplst))
    {
        DAT_LOCK();
        /* append error data to delay list */
        tmplst.next->prev = gb_delay_lst.prev;
        gb_delay_lst.prev->next = tmplst.next;
        tmplst.prev->next = &gb_delay_lst;
        gb_delay_lst.prev = tmplst.prev;
        DAT_UNLOCK();
    }
}

void gb_data_flush_sending(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_last(&gb_trans_lst)) != NULL)
    {
        gb_pack_t *pack = list_entry(node, gb_pack_t, link);

        list_insert_after(pack->list, &pack->link);
    }

    DAT_UNLOCK();
}

void gb_data_flush_realtm(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_first(&gb_realtm_lst)) != NULL)
    {
        gb_pack_t *pack = list_entry(node, gb_pack_t, link);
        pack->list = &gb_delay_lst;
        pack->type = GB_RPTTYPE_DELAY;
        list_insert_before(&gb_delay_lst, &pack->link);
    }

    DAT_UNLOCK();
}

void gb_data_clear_report(void)
{
    int i;

    DAT_LOCK();
    list_init(&gb_realtm_lst);
    list_init(&gb_delay_lst);
    list_init(&gb_trans_lst);
    list_init(&gb_free_lst);

    for (i = 0; i < GB_MAX_REPORT; i++)
    {
        list_insert_before(&gb_free_lst, &gb_datamem[i].link);
    }

    DAT_UNLOCK();
}

void gb_data_clear_error(void)
{
    list_t *node;

    ERR_LOCK();

    if ((node = gb_errlst_head) == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    do
    {
        list_entry(node, gb_pack_t, link)->len = 0;
        node = node->next;
    }
    while (node != gb_errlst_head);

    ERR_UNLOCK();
}

static void gb_data_parse_define(gb_info_t *gbinf, const char *str)
{
    assert(str != NULL);

    /* vehicle type */
    {
        uint32_t vtype;

        if (1 == sscanf(str, "BA_ \"IN_VEHITYPE\" %2x", &vtype) &&
            (vtype == GB_VEHITYPE_ELECT || vtype == GB_VEHITYPE_GASFUEL || GB_VEHITYPE_HYBIRD))
        {
            gbinf->vehi.vehi_type = vtype;
            return;
        }
    }

    /* engine state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_ENGINESTATE_%2x\" %2x", &index, &state))
        {
            gbinf->engin.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHISTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle mode */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHIMODE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.mode_tbl[index] = state;
            return;
        }
    }

    /* charge state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_CHGSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.charge_tbl[index] = state;
            return;
        }
    }

    /* DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_DCDCSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.dcdc_tbl[index] = state;
            return;
        }
    }

    /* High voltage DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_HVDCDCSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->fuelcell.hvdcdc[index] = state;
            return;
        }
    }

    /* shift state */
    {
        char shift;
        uint32_t index;

        if (2 == sscanf(str, "BA_ \"IN_SHIFT_%2x\" %c", &index, &shift))
        {
            gbinf->vehi.shift_tbl[index] = shift;
            return;
        }
    }

    /* motor state */
    {
        uint32_t state, index, motor;

        if (3 == sscanf(str, "BA_ \"IN_MOTSTATE%u_%2x\" %2x", &motor, &index, &state) &&
            motor < GB_MAX_MOTOR)
        {
            gbinf->motor[motor].state_tbl[index] = state;
            return;
        }
    }
}

static int gb_data_parse_surfix(gb_info_t *gbinf, int sigid, const char *sfx)
{
    uint32_t gbtype, gbindex;

    assert(sigid > 0 && sfx != NULL);

    if (2 != sscanf(sfx, "G%1x%3x", &gbtype, &gbindex))
    {
        return 0;
    }

    switch (gbtype)
    {
        case GB_DATA_VEHIINFO:
            if (gbindex >= GB_MAX_VEHI_INFO)
            {
                log_e(LOG_GB32960, "vehicle info over %d! ", gbindex);
                break;
            }

            gbinf->vehi.info[gbindex] = sigid;
            break;

        case GB_DATA_MOTORINFO:
            if ((gbindex >> 8) >= GB_MAX_MOTOR)
            {
                log_e(LOG_GB32960, "motor ID over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= GB_MAX_MOTOR_INFO)
            {
                log_e(LOG_GB32960, "motor info over %d! ", gbindex & 0xff);
                break;
            }

            gbinf->motor[gbindex >> 8].info[gbindex & 0xff] = sigid;

            if ((gbindex >> 8) + 1 > gbinf->motor_cnt)
            {
                gbinf->motor_cnt = (gbindex >> 8) + 1;
            }

            break;
            
        case GB_DATA_FUELCELL:
            if(gbindex >= GB_FCINF_TEMPTAB)
            {
                gbinf->fuelcell.temp_cnt++;
                gbinf->fuelcell.temp[gbindex-GB_FCINF_TEMPTAB] = sigid;
            }
            else
            {
                if ((gbindex & 0xf) >= GB_FCINF_MAX)
                {
                    log_e(LOG_GB32960, "fuel cell info over %d! ", gbindex & 0xf);
                    break;
                }
                gbinf->fuelcell.info[gbindex & 0xf] = sigid;
            }
            break;
            
        case GB_DATA_ENGINEINFO:
            if (gbindex >= GB_MAX_ENGIN_INFO)
            {
                log_e(LOG_GB32960, "engine info over %d! ", gbindex);
                break;
            }

            gbinf->engin.info[gbindex] = sigid;
            break;

        case GB_DATA_EXTREMA:
            if (gbindex >= GB_MAX_EXTR_INFO)
            {
                log_e(LOG_GB32960, "extrema info over %d! ", gbindex);
                break;
            }

            gbinf->extr[gbindex] = sigid;
            break;

        case GB_DATA_WARNNING:
            if ((gbindex >> 8) >= 4)
            {
                log_e(LOG_GB32960, "warnning level over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= GB_MAX_WARN_INFO)
            {
                log_e(LOG_GB32960, "warnning number over %d! ", gbindex & 0xff);
                break;
            }

            gbinf->warn[gbindex >> 8][gbindex & 0xff] = sigid;
            break;

        case GB_DATA_BATTVOLT:
            if ((gbindex >> 10) >= 1)
            {
                log_e(LOG_GB32960, "battery number over %u! ", gbindex >> 10);
            }
            else if ((gbindex & 0x3ff) == 0x3fe)
            {
                gbinf->batt.voltage = sigid;
            }
            else if ((gbindex & 0x3ff) == 0x3ff)
            {
                gbinf->batt.current = sigid;
            }
            else if ((gbindex & 0x3ff) >= GB_MAX_PACK_CELL)
            {
                log_e(LOG_GB32960, "battery cells count over %u! ", gbindex & 0x3ff);
            }
            else if (gbinf->batt.cell[gbindex & 0x3ff] == 0)
            {
                gbinf->batt.cell[gbindex & 0x3ff] = sigid;
                gbinf->batt.cell_cnt++;
            }

            break;

        case GB_DATA_BATTTEMP:
            if ((gbindex >> 8) >= 1)
            {
                log_e(LOG_GB32960, "battery number over %d! ", gbindex >> 8);
            }
            else if ((gbindex & 0xff) >= GB_MAX_PACK_TEMP)
            {
                log_e(LOG_GB32960, "battery temperature count over %d! ", gbindex & 0xff);
            }
            else if (gbinf->batt.temp[gbindex & 0xff] == 0)
            {
                gbinf->batt.temp[gbindex & 0xff] = sigid;
                gbinf->batt.temp_cnt++;
            }

            break;

        case GB_DATA_VIRTUAL:
            log_i(LOG_GB32960, "get virtual channe %s", sfx);
            break;

        default:
            log_o(LOG_GB32960, "unkonwn type %s", sfx);
            break;
    }

    return 5;
}


static int gb_data_dbc_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    static gb_info_t *gb_rld = NULL;
    int ret = 0;

    switch (event)
    {
        case DBC_EVENT_RELOAD:
            {
                gb_info_t *next;

                gb_rld = gb_inf == NULL ? gb_infmem : gb_inf->next;
                next = gb_rld->next;
                memset(gb_rld, 0, sizeof(gb_info_t));
                gb_rld->next = next;
                gb_rld->vehi.vehi_type = GB_VEHITYPE_ELECT;
                break;
            }

        case DBC_EVENT_FINISHED:
            if (gb_rld && arg1 == 0)
            {
                int i;

				for (i = 0; i < gb_rld->batt.cell_cnt && gb_rld->batt.cell[i]; i++);

				if (i < gb_rld->batt.cell_cnt)
				{
					log_e(LOG_GB32960, "battery cell defined in dbc is not incorrect");
					break;
				}

				for (i = 0; i < gb_rld->batt.temp_cnt && gb_rld->batt.temp[i]; i++);

				if (i < gb_rld->batt.temp_cnt)
				{
					log_e(LOG_GB32960, "temperature defined in dbc is not incorrect");
					break;
				}

                gb_inf = gb_rld;

                for (i = 0; i < GB_MAX_WARN_INFO; i++)
                {
                    if (gb_inf->warn[2][i] != 0)
                    {
                        dbc_set_signal_flag(gb_inf->warn[2][i], gb_warnflag);
                    }
                }

                ERR_LOCK();

                if (GROUP_SIZE(gb_inf) > 0)
                {
                    gb_errlst_head = &gb_errmem[0].link;
                    list_init(gb_errlst_head);

                    for (i = 1; i < GROUP_SIZE(gb_inf) * 30; i++)
                    {
                        gb_errmem[i].len  = 0;
                        gb_errmem[i].type = GB_RPTTYPE_DELAY;
                        gb_errmem[i].list = &gb_delay_lst;
                        list_insert_before(gb_errlst_head, &gb_errmem[i].link);
                    }
                }
                else
                {
                    gb_errlst_head = NULL;
                }

                ERR_UNLOCK();
                gb_data_clear_report();
            }

            gb_rld = NULL;
            break;

        case DBC_EVENT_DEFINE:
            if (gb_rld)
            {
                gb_data_parse_define(gb_rld, (const char *)arg1);
            }

            break;

        case DBC_EVENT_SURFIX:
            if (gb_rld)
            {
                ret = gb_data_parse_surfix(gb_rld, (int)arg1, (const char *)arg2);
            }

            break;

        case DBC_EVENT_UPDATE:
            if (gb_inf &&
                dbc_test_signal_flag((int)arg1, gb_warnflag, 0) &&
                dbc_get_signal_lastval((int)arg1) == 0 &&
                dbc_get_signal_value((int)arg1) != 0)
            {
                log_e(LOG_GB32960, "warnning triggered");
                gb_inf->warntrig = 1;
            }
        break;
        default:
            break;
    }

    return ret;
}

static void gb_data_periodic(gb_info_t *gbinf, int intv, uint32_t uptime)
{
    int period;
    static int ticks = 0, times = 0, triger = 0;

    if (gbinf->warntrig)
    {
        RTCTIME time;

        gbinf->warntrig = 0;
        times = 30 + 1;

        if (gb_pendflag)
        {
            gb_data_flush_realtm();
            gb_data_flush_error();
        }
        else
        {
            triger = 1;
        }

        can_get_time(uptime, &time);
        log_e(LOG_GB32960, "level 3 error start: %u, %02d/%02d/%02d, %02d:%02d:%02d",
              uptime, time.year, time.mon, time.mday, time.hour, time.min, time.sec);
    }

    ticks++;

    if (times == 0)
    {
        gb_data_save_error(gbinf, uptime);
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
        gb_data_save_realtm(gbinf, uptime);

        if (triger)
        {
            triger = 0;
            gb_data_flush_error();
        }
    }
}

static int gb_data_can_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
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
            gb_data_clear_error();
            break;

        case CAN_EVENT_DATAIN:
            {
                static int counter = 0;
                CAN_MSG *msg = (CAN_MSG *)arg1;

                while (canact && gb_inf && arg2--)
                {
                    if (msg->type == 'T' && ++counter == 100)
                    {
                        counter = 0;
                        gb_data_periodic(gb_inf, gb_datintv, msg->uptime);
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


#include "gb32960_disp.h"

static int gb_shell_dumpgb(int argc, const char **argv)
{
    dbc_lock();

    if (gb_inf == NULL)
    {
        shellprintf(" dbc file is not loaded\r\n");
    }
    else
    {
        shellprintf(" [车辆信息]\r\n");
        gb_disp_vinf(&gb_inf->vehi);

        if (gb_inf->vehi.vehi_type == GB_VEHITYPE_ELECT ||
            gb_inf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
        {
            int i;

            for (i = 0; i < gb_inf->motor_cnt; i++)
            {
                shellprintf(" [电机信息-%d]\r\n", i + 1);
                gb_disp_minf(&gb_inf->motor[i]);
            }
            
            shellprintf(" [燃料电池信息]\r\n");
            gb_disp_finf(&gb_inf->fuelcell);

            shellprintf(" [电池信息]\r\n");
            gb_disp_binf(&gb_inf->batt);
            shellprintf(" [极值信息]  (如果全部未定义，则由终端计算)\r\n");
            gb_disp_xinf(gb_inf->extr);
        }

        if (gb_inf->vehi.vehi_type == GB_VEHITYPE_GASFUEL ||
            gb_inf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
        {
            shellprintf(" [发动机信息]\r\n");
            gb_disp_einf(&gb_inf->engin);
        }

        shellprintf(" [报警信息-1级]\r\n");
        gb_disp_winf(gb_inf->warn[0]);
        shellprintf(" [报警信息-2级]\r\n");
        gb_disp_winf(gb_inf->warn[1]);
        shellprintf(" [报警信息-3级]\r\n");
        gb_disp_winf(gb_inf->warn[2]);
    }

    dbc_unlock();
    return 0;
}


static int gb_shell_setintv(int argc, const const char **argv)
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

    if (gb_set_datintv(intv))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    return 0;
}


static int gb_shell_testwarning(int argc, const const char **argv)
{
    int on;

    if (argc != 1 || sscanf(argv[0], "%d", &on) != 1)
    {
        shellprintf(" usage: gbtestwarn <0/1>\r\n");
        return -1;
    }

    dbc_lock();

    if (gb_inf)
    {
        if (!gb_inf->warntest && on)
        {
            gb_inf->warntrig = 1;
        }

        gb_inf->warntest = on;
    }

    dbc_unlock();
    return 0;
}

int gb_data_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            gb_infmem[0].next = gb_infmem + 1;
            gb_infmem[1].next = gb_infmem;
            gb_inf = NULL;
            gb_errlst_head = NULL;
            gb_datintv = 10;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                uint32_t cfglen;

                gb_warnflag = dbc_request_flag();
                ret |= dbc_register_callback(gb_data_dbc_cb);
                ret |= can_register_callback(gb_data_can_cb);
                ret |= shell_cmd_register_ex("gbdump", "dumpgb", gb_shell_dumpgb,
                                             "show GB32960 signals information");
                ret |= shell_cmd_register_ex("gbsetintv", "setintvd", gb_shell_setintv,
                                             "set GB32960 report period");
                ret |= shell_cmd_register_ex("gbtestwarn", "errtrig", gb_shell_testwarning,
                                             "test GB32960 warnning");
                ret |= pthread_mutex_init(&gb_errmtx, NULL);
                ret |= pthread_mutex_init(&gb_datmtx, NULL);

                cfglen = sizeof(gb_datintv);
                ret |= cfg_get_para(CFG_ITEM_GB32960_INTERVAL, &gb_datintv, &cfglen);
                break;
            }
    }

    return ret;
}

void gb_data_put_back(gb_pack_t *pack)
{
    DAT_LOCK();
    list_insert_after(pack->list, &pack->link);
    DAT_UNLOCK();
}

void gb_data_put_send(gb_pack_t *pack)
{
    DAT_LOCK();
    list_insert_before(&gb_trans_lst, &pack->link);
    DAT_UNLOCK();
}

void gb_data_ack_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&gb_trans_lst)) != NULL)
    {
        list_insert_before(&gb_free_lst, node);
    }

    DAT_UNLOCK();
}

static int gb_emerg;

void gb_data_emergence(int set)
{
    DAT_LOCK();
    gb_emerg = set;
    DAT_UNLOCK();
}

gb_pack_t *gb_data_get_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&gb_realtm_lst)) == NULL)
    {
        node = gb_emerg ? list_get_last(&gb_delay_lst) : list_get_first(&gb_delay_lst);
    }

    DAT_UNLOCK();

    return node == NULL ? NULL : list_entry(node, gb_pack_t, link);;
}

int gb_data_nosending(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&gb_trans_lst);
    DAT_UNLOCK();
    return ret;
}

int gb_data_noreport(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&gb_realtm_lst) && list_empty(&gb_delay_lst);
    DAT_UNLOCK();
    return ret;
}

void gb_data_set_intv(uint16_t intv)
{
    gb_datintv = intv;
}

int gb_data_get_intv(void)
{
    return gb_datintv;
}

void gb_data_set_pendflag(int flag)
{
    gb_pendflag = flag;
}

/*
 	 读取车辆状态
*/
uint8_t gb_data_vehicleState(void)
{
	uint32_t tmp;
	uint8_t vehicleState;
/* vehicle state */
   if (gb_inf->vehi.info[GB_VINF_STATE])
   {
       tmp = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_STATE])->value;
       vehicleState = gb_inf->vehi.state_tbl[tmp] ? gb_inf->vehi.state_tbl[tmp] : 0xff;
   }
   else
   {
	   vehicleState = 0xff;
   }
   return vehicleState;
}

/*
 	 读取车辆剩余电量
*/
long gb_data_vehicleSOC(void)
{
	long vehicleSOC;
	 /* total SOC */
	vehicleSOC = gb_inf->vehi.info[GB_VINF_SOC] ?
		  dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_SOC])->value : 0xff;
	return  vehicleSOC;
}

/*
 	 读取车辆总里程
*/
long gb_data_vehicleOdograph(void)
{
	long tmp;
    /* odograph, scale 0.1km */
    tmp = gb_inf->vehi.info[GB_VINF_ODO] ?
          dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_ODO])->value * 10 : 0xffffffff;
    return tmp;
}
