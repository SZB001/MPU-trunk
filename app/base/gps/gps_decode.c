#include "com_app_def.h"
#include "rds.h"
#include "timer.h"
#include "gps_api.h"
#include "gps_decode.h"
#include "gps_dev.h"
#include "dev_time.h"
#include "shell_api.h"

static long long gps_r_distance = 0;  /* realtime distance */
static long long gps_s_distance = 0;  /* the distance already saved in flash */
static GPS_DATA gps_snap;
static volatile unsigned int gps_time_not_change = 0; /* check for the same time */
static pthread_mutex_t gps_snap_mutex;
static pthread_mutex_t gps_distance_mutex;

extern void gps_do_callback(unsigned int event, unsigned int arg1, unsigned int arg2);

/****************************************************************
 function:     gps_acc_distance
 description:  accumulate the distance
 input:        unsigned int distance, distance
 output:       none
 return:       none
 *****************************************************************/
void gps_acc_distance(int distance)
{
    /* only update realtime distance */
    pthread_mutex_lock(&gps_distance_mutex);
    gps_r_distance = gps_r_distance + distance;
    pthread_mutex_unlock(&gps_distance_mutex);
}

/****************************************************************
 function:     gps_get_distance
 description:  accumulate the distance
 input:        none
 output:       none
 return:       the distance
 *****************************************************************/
long long gps_get_distance(void)
{
    long long distance;

    /* only return the distance which is already saved in flash */
    pthread_mutex_lock(&gps_distance_mutex);
    distance = gps_s_distance;
    pthread_mutex_unlock(&gps_distance_mutex);

    return distance;
}

/****************************************************************
 function:     gps_reset_distance
 description:  reset the distance
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void gps_reset_distance(void)
{
    pthread_mutex_lock(&gps_distance_mutex);
    gps_r_distance = 0;
    gps_s_distance = 0;
    gps_decode_save(gps_r_distance);
    pthread_mutex_unlock(&gps_distance_mutex);

    return;
}

/****************************************************************
 function:     gps_save_distance
 description:  save the distance
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void gps_save_distance(void)
{
    pthread_mutex_lock(&gps_distance_mutex);
    gps_decode_save(gps_r_distance);
    gps_s_distance = gps_r_distance;
    pthread_mutex_unlock(&gps_distance_mutex);

    return;
}

/****************************************************************
 function:     gps_init_snap
 description:  initiaze the snapshot of gps to 0
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void gps_init_snap(void)
{
    pthread_mutex_lock(&gps_snap_mutex);
    memset(&gps_snap, 0, sizeof(gps_snap));
    pthread_mutex_unlock(&gps_snap_mutex);
}

/****************************************************************
 function:     gps_get_speed
 description:  get the vehicle speed
 input:        none
 output:       none
 return:       the speed
 *****************************************************************/
double gps_get_speed(void)
{
    unsigned int speed;

    pthread_mutex_lock(&gps_snap_mutex);
    speed = gps_snap.kms;
    pthread_mutex_unlock(&gps_snap_mutex);

    return speed;
}

/****************************************************************
 function:     gps_get_snap
 description:  get the position info
 input:        none
 output:       GPS_DATA *gps_data, the last gps info
 return:       none
 *****************************************************************/
void gps_get_snap(GPS_DATA *gps_data)
{
    pthread_mutex_lock(&gps_snap_mutex);
    memcpy(gps_data, &gps_snap, sizeof(GPS_DATA));
    pthread_mutex_unlock(&gps_snap_mutex);
}

/****************************************************************
 function:     gps_update_snap_by_rmc
 description:  update the snapshot by gmc message
 input:        GPS_DATA *data, gps data got from  gmc message
 output:       none
 return:       none
 *****************************************************************/
void gps_update_snap_by_rmc(GPS_DATA *data)
{
    pthread_mutex_lock(&gps_snap_mutex);

    gps_snap.time = data->time;
    gps_snap.date = data->date;
    gps_snap.latitude = data->latitude;
    gps_snap.is_north = data->is_north;
    gps_snap.longitude = data->longitude;
    gps_snap.is_east = data->is_east;
    gps_snap.knots = data->knots;
    gps_snap.direction = data->direction;

    pthread_mutex_unlock(&gps_snap_mutex);
}

/****************************************************************
 function:     gps_update_snap_by_gga
 description:  update the snapshot by gmc message
 input:        unsigned int stateUsed, num of state got from  gmc message;
 double msl, msl got from  gmc message;
 output:       none
 return:       none
 *****************************************************************/
void gps_update_snap_by_gga(unsigned int stateUsed, double msl)
{
    pthread_mutex_lock(&gps_snap_mutex);

    gps_snap.stateUsed = stateUsed;
    gps_snap.msl = msl;

    pthread_mutex_unlock(&gps_snap_mutex);
}

/****************************************************************
 function:     gps_checksum
 description:  check the received data
 input:        unsigned char * buf, data received from gps
 output:       none
 return:       0 indicates the cs is right;
 others indicates the cs is wrong;
 *****************************************************************/
static unsigned int gps_checksum(unsigned char *buf)
{
    unsigned int i, cs = 0;
    unsigned char datacs = 0;
    unsigned int len = strlen((char const *) buf);

    if (memcmp(buf, "$INACC", 6) == 0)
    {
        return 0;
    }

    if ((len < 9) || (memcmp(buf, "$G", 2) != 0) || (buf[len - 2 - 1] != '*'))
    {
        log_e(LOG_GPS, "received: %s", buf);
        log_e(LOG_GPS, "the GNSS header/tail is incorrect!");
        return GPS_INVALID_MSG;
    }

    /* get the checksum */
    sscanf((char const *) &buf[len - 2], "%02x", &cs);

    /* calculate the checksum of data */
    datacs = 0;

    for (i = 1; i < (len - 2 - 1); i++)
    {
        datacs ^= buf[i];
    }

    if (datacs != (cs & 0xFF))
    {
        log_e(LOG_GPS, "received: %s", buf);
        log_e(LOG_GPS, "checksum is incorrect: cs=%02X, datacs=%02X", cs, datacs);
        return GPS_INVALID_CS;
    }

    return 0;
}

/****************************************************************
 function:     gps_utc2local
 description:  convert UTC time to local time
 input:        unsigned int *date, UTC date received from gps
 double *time, UTC time received from gps
 output:       unsigned int *date, local date
 double *time, local time
 return:       0 indicates the cs is right;
 others indicates the cs is wrong;
 *****************************************************************/
static void gps_utc2local(unsigned int *date, double *time)
{
    double local_time = *time;
    unsigned int year = (*date) % 100;
    unsigned int month = ((*date) % 10000) / 100;
    unsigned int day = (*date) / 10000;
    unsigned int mday;

    /* Hour: +8 */
    local_time += 80000;

    if (local_time >= 240000)
    {
        local_time -= 240000;

        /* get how many days in current month */
        switch (month)
        {
            case 2:
                {
                    if (((year % 4 == 0) && (year % 100 != 0))
                        || (year % 400 == 0))
                    {
                        mday = 29;
                    }
                    else
                    {
                        mday = 28;
                    }
                }
                break;

            case 4:
            case 6:
            case 9:
            case 11:
                {
                    mday = 30;
                }
                break;

            default:
                {
                    mday = 31;
                }
                break;
        }

        day++;
        month += (day / (mday + 1));
        day = day % (mday + 1);

        if (day == 0)
        {
            day = 1;
        }

        if (month >= 13)
        {
            month = 1;
            year++;
        }
    }

    *time = local_time;
    *date = day * 10000 + month * 100 + year;
}

/****************************************************************
 function:     gps_get_time_diff
 description:  get the different of time,
 compared the last received message
 input:        double *now, the current time
 output:       none
 return:       time diff
 *****************************************************************/
static unsigned int gps_get_time_diff(double now)
{
    static unsigned int last_sec = 0xFFFF;
    unsigned int cur_sec = 0, time_diff = 0;

    /* get the second of the time */
    cur_sec = (((unsigned int)(now)) % 100);

    if (0xFFFF == last_sec)
    {
        time_diff = 1;
    }
    else
    {
        if (last_sec > cur_sec)
        {
            time_diff = 60 + cur_sec - last_sec;
        }
        else
        {
            time_diff = cur_sec - last_sec;
        }
    }

    log_i(LOG_GPS, "time %f, last_sec %d,cur_sec %d, time_diff %d!", now, last_sec, cur_sec,
          time_diff);

    last_sec = cur_sec;

    if (time_diff > 20)
    {
        time_diff = 1;
    }

    return time_diff;
}

/****************************************************************
 function:     gps_decode_init
 description:  initiaze the decode info
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed;
 *****************************************************************/
int gps_decode_init(INIT_PHASE phase)
{
    int ret;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&gps_snap_mutex, NULL);
            pthread_mutex_init(&gps_distance_mutex, NULL);
            gps_init_snap();
            break;

        case INIT_PHASE_RESTORE:
            ret = gps_decode_restore();

            if (ret != 0)
            {
                log_e(LOG_GPS, "restore gps distance failed!, ret:%u", ret);
            }

            break;

        case INIT_PHASE_OUTSIDE:
            gps_shell_init();
            break;

        default:
            break;
    }

    return 0;
}

/****************************************************************
 function:     gps_decode_rmc
 description:  decode the message which begin with "$GPRMC"
 input:        unsigned char * buf, the message which begin with "$GPRMC"
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************/
static unsigned int gps_decode_rmc(unsigned char *buf)
{
    unsigned char status, is_north, is_east;
    unsigned int time_diff = 0;
    static unsigned int cur_date = 0;
    GPS_DATA snap;
    RTCTIME rtc_time;

    status = 0;
    gps_time_not_change = 0;

    if ((sscanf((char const *) &buf[3], "RMC,%lf,%c,%lf,%c,%lf,%c,%lf,%lf,%d,%*s,%*s,%*s",
                &snap.time, &status, &snap.latitude, &is_north, &snap.longitude, &is_east, &snap.knots,
                &snap.direction, &snap.date) == 9)
        || (sscanf((char const *) &buf[3], "RMC,%lf,%c,%lf,%c,%lf,%c,%lf,,%d,%*s,%*s,%*s",
                   &snap.time, &status, &snap.latitude, &is_north, &snap.longitude, &is_east, &snap.knots,
                   &snap.date) == 8))
    {
        /* data valid */
        if (status == 'A')
        {
            /* convert UTC time to local time */
            gps_utc2local(&snap.date, &snap.time);

            /* set RTC */
            if (cur_date != snap.date)
            {
                cur_date = snap.date;

                /* the year in NMEA message is base on 2000 */
                rtc_time.year = cur_date % 100 + 2000;
                rtc_time.mon = (cur_date / 100) % 100;
                rtc_time.mday = cur_date / 10000;
                rtc_time.hour = (unsigned int) snap.time / 10000;
                rtc_time.min = ((unsigned int) snap.time / 100) % 100;
                rtc_time.sec = (unsigned int) snap.time % 100;
                rtc_time.msec = 0;
                log_o(LOG_GPS, "gps has being fixed !");
                log_o(LOG_GPS, "set time by gps, %u-%u-%u %u:%u:%u",
                      rtc_time.year, rtc_time.mon, rtc_time.mday, rtc_time.hour, rtc_time.min,
                      rtc_time.sec);
                dev_syn_time(&rtc_time, GNSS_TIME_SOURCE);
            }

            if (snap.time == gps_snap.time)
            {
                gps_time_not_change = 1;
                log_i(LOG_GPS, "gps time is not changed!");
                return 0;
            }

            /* assign realtime data */
            snap.is_north = (is_north == 'N') ? 1 : 0;
            snap.is_east = (is_east == 'E') ? 1 : 0;
            time_diff = gps_get_time_diff(snap.time);

            if (snap.knots >= (3000.0 / 1852))
            {
                static unsigned int i = 0;

                i = (i + 1) % 60;
                gps_acc_distance(snap.knots * 1852 * time_diff);

                if (i == 30)
                {
                    gps_save_distance();
                }

                snap.kms = snap.knots * 1.852;
            }
            else
            {
                snap.knots = 0;
                snap.kms = 0;
                snap.direction = 0;
            }

            gps_update_snap_by_rmc(&snap);

            /* show debug log */
            log_i(LOG_GPS,
                  " date=%d, time=%lf, latitude=(%c)%lf, longitude=(%c)%lf, direction=%lf, knots=%lf",
                  snap.date, snap.time, is_north, snap.latitude, is_east, snap.longitude,
                  snap.direction, snap.knots);

            return 0;
        }
        else
        {
            gps_init_snap();
            return GPS_INVALID_MSG;
        }
    }
    else
    {
        gps_init_snap();
        return GPS_INVALID_MSG;
    }

    return 0;
}

void gps_update_snap_by_gsa(double Hdop, double Vdop)
{
    pthread_mutex_lock(&gps_snap_mutex);

    gps_snap.hdop = Hdop;
    gps_snap.vdop = Vdop;

    pthread_mutex_unlock(&gps_snap_mutex);
}

/****************************************************************
 function:     gps_decode_gga
 description:  decode the message which begin with "$GPGGA"
 input:        unsigned char * buf, the message which begin with "$GPGGA"
 output:       none
 return:       0 indicates success;
 others indicates failed;
 *****************************************************************/
static void gps_decode_gga(unsigned char *buf)
{
    double msl = 0;
    unsigned int fs = 0, state;

    if (gps_time_not_change)
    {
        log_i(LOG_GPS, "ignore GPGGA!");
        return;
    }

    if (sscanf((char const *) &buf[3], "GGA,%*f,%*f,%*c,%*f,%*c,%d,%d,%*f,%lf", &fs, &state, &msl)
        == 3)
    {
        if (fs != 0)
        {
            log_i(LOG_GPS, " msl = %lf", msl);
            gps_update_snap_by_gga(state, msl);
        }
    }

    return;
}

static void gps_decode_gsa(unsigned char *buf)
{
    double Hdop,Vdop;
	unsigned int fs = 1;

    if (gps_time_not_change)
    {
        log_i(LOG_GPS, "ignore GPGSA!");
        return;
    }

    if (sscanf((char const *) &buf[3], "GSA,%*c,%d",&fs) == 1)
    {
        log_i(LOG_GPS, " fs = %d", fs);
        if (fs != 1)
        {
			if (sscanf((char const *) &buf[strlen((const char *) buf)-14], "%lf,%lf",&Hdop,&Vdop) == 2)
			{
				log_i(LOG_GPS, " Hdop = %lf", Hdop);
            	gps_update_snap_by_gsa(Hdop, Vdop);
			}
        }
    }

    return;
}

/****************************************************************
 function:     gps_decode_gga
 description:  decode the message received from gps
 input:        unsigned char * buf, the message received from gps
 output:       none
 return:       none
 *****************************************************************/
int gps_decode(unsigned char *buf)
{
    int ret;
    static unsigned long long llt = 0; // GPS loss lock time

    if (gps_checksum(buf))
    {
        return 0;
    }

    log_i(LOG_GPS, "received: %s", buf);
    gps_do_callback(GPS_EVENT_DATAIN, (unsigned int) buf, strlen((const char *) buf));

    /* decode it */
    if ((memcmp(buf, "$GPRMC", 6) == 0) || (memcmp(buf, "$GNRMC", 6) == 0))
    {
        ret = gps_decode_rmc(buf);

        if (0 == ret)
        {
            gps_set_fix_status(GPS_FIX);
            llt = 0;
        }
        else
        {
            gps_set_fix_status(GPS_UNFIX);

            if (0 != llt)
            {
                if (tm_get_time() - llt > GPS_LOSS_LOCK_TIMEOUT)
                {
                    llt = 0;

                    if (GNSS_4G_MODULE == GNSS_TYPE)
                    {
                        log_o(LOG_GPS, "GPS signal lost or too long unfixed.");
                        gps_dev_reset();
                    }
                }
            }
            else
            {
                llt = tm_get_time();
            }
        }

        return ret;
    }
    else if ((memcmp(buf, "$GPGGA", 6) == 0) || (memcmp(buf, "$GNGGA", 6) == 0))
    {
        gps_decode_gga(buf);
        return 0;
    }
    else if((memcmp(buf, "$GPGSA", 6) == 0) || (memcmp(buf, "$GNGSA", 6) == 0))
	{
		gps_decode_gsa(buf);
		return 0;
	}
    else
    {
        return 0;
    }
}

/****************************************************************
 function:     gps_decode_save
 description:  save distance
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int gps_decode_save(unsigned long long distance)
{
    int ret;

    ret = rds_update_once(RDS_DATA_DISTANCE, (unsigned char *) &distance, sizeof(distance));

    if (ret != 0)
    {
        log_e(LOG_GPS, "update distance failed, ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     opt_decode_restore
 description:  restore distance
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int gps_decode_restore(void)
{
    int ret;
    unsigned int len = sizeof(gps_s_distance);
    char ver[COM_APP_VER_LEN];

    pthread_mutex_lock(&gps_distance_mutex);
    ret = rds_get(RDS_DATA_DISTANCE, (unsigned char *) &gps_s_distance, &len, ver);

    if (ret != 0)
    {
        log_e(LOG_GPS, "get distance failed ret:0x%08x", ret);
        gps_s_distance = 0;
        gps_r_distance = 0;
        gps_decode_save(gps_r_distance);
        pthread_mutex_unlock(&gps_distance_mutex);
        return GPS_GET_DISTANCE_FAILED;
    }

    log_o(LOG_GPS, "gps distance ver:%s", ver);

    if (len != sizeof(gps_s_distance))
    {
        log_e(LOG_GPS, "expect len:%u, actul len:%u", sizeof(gps_s_distance), len);
        gps_s_distance = 0;
        gps_r_distance = 0;
        gps_decode_save(gps_r_distance);
        pthread_mutex_unlock(&gps_distance_mutex);
        return GPS_INVALID_DISTANCE_LEN;
    }

    gps_r_distance = gps_s_distance;

    pthread_mutex_unlock(&gps_distance_mutex);

    return 0;
}

