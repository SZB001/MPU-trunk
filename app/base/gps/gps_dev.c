/****************************************************************
 file:         gps_dev.c
 description:  the header file of ota upgrade manager implemention
 date:         2016/12/01
 author        Wuhan Intest Electronic Technology,Co.,Ltd,
 liuzhongwen
 ****************************************************************/
#include <termios.h>
#include "com_app_def.h"
#include "mid_def.h"
#include "log.h"
#include "file.h"
#include "ubx_cfg.h"
#include "gps_api.h"
#include "gps_dev.h"
#include "gps_decode.h"
#include "timer.h"
#include "ql_uart.h"
#include "dev_rw.h"
#include "pm_api.h"
#include "gpio.h"
#include "shell_api.h"
#include "at_api.h"
#include "../at/at_stat.h"

static GPS_DEV gps_dev;
static unsigned char gps_fix_status = GPS_UNCONNECTED;
static unsigned char gps_dev_init_status = 0;//liu bin kui add,0:not init 1:init ok

static pthread_mutex_t gps_dev_mutex;

static unsigned char ubx_init_tbl[] =
{
    UBX_CFG_RATE_1HZ,
    UBX_CFG_NAV5,
    UBX_CFG_MSG_GSV_OFF,
    UBX_CFG_MSG_GSA_OFF,
    //UBX_CFG_MSG_GLL_OFF,  // don't disable gngll, avoid the bug of UART2 receiving data exception.
    UBX_CFG_MSG_VTG_OFF,
};
	
static unsigned char ubx_ephemeris_data[8 * 1024];


/****************************************************************
 function:       gps_sleep_available
 description:    check at module can enter into sleep mode
 input:          none
 output:         none
 return:         1 indicates module can sleep
                 0 indicates module can not sleep
 *****************************************************************/
int gps_sleep_available(PM_EVT_ID id)
{
    return 1;
}

/****************************************************************
 function:     gps_dev_init
 description:  initiaze gps device
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int gps_dev_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&gps_dev_mutex, NULL);
            gpio_init(GPIO_GPS_RESET);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            if (0 != pm_reg_handler(MPU_MID_GPS, gps_sleep_available))
            {
                log_e(LOG_GPS, "pm_reg_handler failed!");
                return GPS_INIT_FAILED;
            }

            if (0 != tm_create(TIMER_REL, GPS_MSG_ID_DATA_TIMER, MPU_MID_GPS, &gps_dev.data_timer))
            {
                log_e(LOG_GPS, "create gps data failed!");
                return GPS_CREATE_TIMER_FAILED;
            }

            if (0 != tm_create(TIMER_REL, GPS_MSG_ID_CFG_TIMER, MPU_MID_GPS, &gps_dev.cfg_timer))
            {
                log_e(LOG_GPS, "create gps cfg failed!");
                return GPS_CREATE_TIMER_FAILED;
            }

            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
 function:     gps_dev_open
 description:  open gps device
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int gps_dev_open(void)
{
    log_o(LOG_GPS, "open gps device %u", GNSS_TYPE);

    if (GNSS_EXTERNAL == GNSS_TYPE)
    {
        gps_dev.dev_fd = Ql_UART_Open(GPS_UART_PORT, 9600, FC_NONE);
    }
    else if (GNSS_4G_MODULE == GNSS_TYPE)
    {
        if(gps_dev.dev_fd <= 0)
        {
            gps_dev.dev_fd = open(GPS_NMEA_PORT, O_RDWR | O_NONBLOCK | O_NOCTTY);
        }
    }

    if (gps_dev.dev_fd < 0)
    {
        log_e(LOG_GPS, "open gps device %u error", GNSS_TYPE);
        return GPS_OPEN_FAILED;
    }

    return 0;
}

/****************************************************************
 function:     gps_dev_close
 description:  close gps device
 input:        none
 output:       none
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int gps_dev_close(void)
{
    log_o(LOG_GPS, "close gps device %u", GNSS_TYPE);

    if (GNSS_EXTERNAL == GNSS_TYPE)
    {
        if (gps_dev.dev_fd >= 0)
        {
            close(gps_dev.dev_fd);
            gps_dev.dev_fd = -1;
            gps_dev.cfg_step = 0;
            tm_stop(gps_dev.data_timer);
        }
    }
    else if (GNSS_4G_MODULE == GNSS_TYPE)
    {
    }

    return 0;
}

/****************************************************************
 function:     gps_dev_reset
 description:  reset gps device
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void gps_dev_reset(void)
{
    if (GNSS_EXTERNAL == GNSS_TYPE)
    {
        gpio_set_level(GPIO_GPS_RESET, PINLEVEL_HIGH);
        usleep(10000);
        gpio_set_level(GPIO_GPS_RESET, PINLEVEL_LOW);
        gps_dev.cfg_step = 0;

        if (0 != tm_start(gps_dev.cfg_timer, 1000, TIMER_TIMEOUT_REL_ONCE))
        {
            log_e(LOG_GPS, "start GPS_MSG_ID_CFG_TIMER error");
        }

        log_o(LOG_GPS, "reset ubx gps");
    }
    else if (GNSS_4G_MODULE == GNSS_TYPE)
    {
        at_disable_gps();
        at_enable_gps();
        log_o(LOG_GPS, "reset inner gps");
    }
}

/****************************************************************
 function:     gps_dev_ubx_init
 description:  ublox gps init
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void gps_dev_ubx_init(void)
{
    UBX_CFG_ID id;

    if (gps_dev.cfg_step < sizeof(ubx_init_tbl))
    {
        id = ubx_init_tbl[gps_dev.cfg_step];
        log_buf_dump(LOG_GPS, "ubx_cfg", ubx_tbl[id].cmd, ubx_tbl[id].len);
        dev_write(gps_dev.dev_fd, ubx_tbl[id].cmd, ubx_tbl[id].len);
        gps_dev.cfg_step++;

        if (0 != tm_start(gps_dev.cfg_timer, UBX_CFG_INIT_TIMEOUT, TIMER_TIMEOUT_REL_ONCE))
        {
            log_e(LOG_GPS, "start GPS_MSG_ID_CFG_TIMER error");
            gps_dev.cfg_step = 0;
        }
    }
    else
    {
        gps_dev.cfg_step = 0;
        tm_stop(gps_dev.cfg_timer);
    }
}

/****************************************************************
 function:     gps_timeout
 description:  process timeout messages
 input:        unsigned int time_id, timeout msgid
 output:       none
 return:       none
 ****************************************************************/
void gps_dev_timeout(unsigned int time_id)
{
    static AT_STAT_T last_at_stat = AT_OFF_ST;
    switch (time_id)
    {
        case GPS_MSG_ID_DATA_TIMER:
            log_e(LOG_GPS, "GPS data timer is timeout!");
            gps_dev_reset();
            if(last_at_stat >= AT_NORM_IDLE_ST)
            {
                gps_set_fix_status(GPS_ERROR);
                log_e(LOG_GPS, "at module has been initialized, GPS module error!");
            }
            last_at_stat = at_get_stat();
            if (0 != tm_start(gps_dev.data_timer, GPS_TIMEOUT_4S, TIMER_TIMEOUT_REL_ONCE))
            {
                log_e(LOG_GPS, "start GPS_MSG_ID_TIMER error");
                return;
            }

            break;

        case GPS_MSG_ID_CFG_TIMER:
            gps_dev_ubx_init();
            break;

        default:
            log_e(LOG_GPS, "timeid unknown:%d!", time_id);
            break;
    }
}

void gps_dev_send(unsigned char *data, unsigned int len)
{
    dev_write(gps_dev.dev_fd, data, len);

    if (GNSS_EXTERNAL == GNSS_TYPE)
    {
        tcflush(gps_dev.dev_fd, TCIOFLUSH);
    }
}

/***************************************************************
 function:     gps_recv
 description:  receive the raw gps data
 input:        none
 output:       none
 return:       none
 ****************************************************************/
void gps_dev_recv(void)
{
    int recv_cnt;
    int i, j;
    static int last_recv_cnt = 0;
    static char recv_buf[1024];
    static char msg[512];

    recv_cnt = read(gps_dev.dev_fd, recv_buf + last_recv_cnt, sizeof(recv_buf) - last_recv_cnt);

    if (recv_cnt < 0)
    {
        log_e(LOG_GPS, "get data from gps serial error! %d", recv_cnt);
        return;
    }

    log_i(LOG_GPS, "gps received cnt:%d,last_cnt:%d", recv_cnt, last_recv_cnt);
    log_hex_dump(LOG_GPS, (unsigned char *)(recv_buf + last_recv_cnt), recv_cnt);
    j = 0;

    for (i = 0; i < recv_cnt + last_recv_cnt; i++)
    {
        if (('\r' == recv_buf[i - 1]) && ('\n' == recv_buf[i]))
        {
            msg[j - 1] = '\0';
            gps_decode((unsigned char *) msg);

            if (0 != tm_start(gps_dev.data_timer, GPS_TIMEOUT_4S, TIMER_TIMEOUT_REL_ONCE))
            {
                log_e(LOG_GPS, "start GPS_MSG_ID_TIMER error");
                return;
            }

            j = 0;
            memset(msg, 0, sizeof(msg));
        }
        else
        {
            if ('\0' != recv_buf[i])
            {
                msg[j++] = recv_buf[i];
            }
            else
            {
                log_i(LOG_GPS, "ignore NUL at [%d]", i);
            }
        }
    }

    last_recv_cnt = j;
    memset(recv_buf, 0, sizeof(recv_buf));

    if (j > 0)
    {
        memcpy(recv_buf, msg, j);
    }
}

/****************************************************************
 function:     gps_dev_get_fd
 description:  get gps ant status
 input:        none
 output:       none
 return:       fd;
 *****************************************************************/
int gps_dev_get_fd(void)
{
    return gps_dev.dev_fd;
}


int gps_get_ubx_init_sta(void)
{
    unsigned char state;
    pthread_mutex_lock(&gps_dev_mutex);
    state = gps_dev_init_status;
    pthread_mutex_unlock(&gps_dev_mutex);

    return state;
}
void gps_dev_ubx_write_ehpemeris_data(void)
{
    unsigned int len;

    if (gps_dev.eph_ptr && *gps_dev.eph_ptr)
    {
        while ((0xB5 != gps_dev.eph_ptr[0]) || (0x62 != gps_dev.eph_ptr[1]))
        {
            gps_dev.eph_ptr++;
        }

        len = gps_dev.eph_ptr[4] + gps_dev.eph_ptr[5] * 256 + 8;

        dev_write(gps_dev.dev_fd, gps_dev.eph_ptr, len);
        //log_e(LOG_GPS, "***************************************************************");
        gps_dev.eph_ptr += len;

        if (0 != tm_start(gps_dev.imp_timer, UBX_WRITE_INTERVAL, TIMER_TIMEOUT_REL_ONCE))
        {
            log_e(LOG_GPS, "start GPS_MSG_ID_IMP_TIMER error");
            gps_dev.eph_ptr = (unsigned char *)ubx_ephemeris_data;
        }
    }
    else
    {
        gps_dev.eph_ptr = NULL;
        tm_stop(gps_dev.imp_timer);
        log_o(LOG_GPS, "ehpemeris data write success");
    }
}

int gps_dev_ubx_import_ehpemeris(const char *file)
{
    int ret;
    unsigned int len;

    /* if fixed,don't write */
    if (gps_get_fix_status() == 2)
    {
        log_e(LOG_GPS, "***** GPS has fixed,stop write *****");
        return 0;
    }

    if (!file)
    {
        log_e(LOG_GPS, "ublox ephemeris file is null!");
        return -1;
    }

    len = sizeof(ubx_ephemeris_data);

    ret = file_read(file, ubx_ephemeris_data, &len);

    if (0 != ret)
    {
        log_e(LOG_GPS, "read ephemeris file failed!");
        return -1;
    }

    gps_dev.eph_ptr = (unsigned char *)ubx_ephemeris_data;

    gps_dev_ubx_write_ehpemeris_data();

    log_o(LOG_GPS, "startup import ehpemeris data to ublox success!");

    return 0;
}



/****************************************************************
 function:     gps_get_fix_status
 description:  get gps status
 input:        0 indicates unconnected;
 1 indicates unfix;
 2 indicates fix;
 3 indicates error;
 output:       none
 return:       none
 *****************************************************************/
void gps_set_fix_status(unsigned int status)
{
    pthread_mutex_lock(&gps_dev_mutex);
    gps_fix_status = status;
    pthread_mutex_unlock(&gps_dev_mutex);
}

/****************************************************************
 function:     gps_get_fix_status
 description:  get gps status
 input:        none
 output:       none
 return:       0 indicates unconnected;
 1 indicates unfix;
 2 indicates fix;
 3 indicates error;
 *****************************************************************/
unsigned int gps_get_fix_status(void)
{
    unsigned int status;

    pthread_mutex_lock(&gps_dev_mutex);
    status = gps_fix_status;
    pthread_mutex_unlock(&gps_dev_mutex);

    return status;
}

