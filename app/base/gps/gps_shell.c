/****************************************************************
 file:         gps_shell.c
 description:  the source file of gps shell implementation
 date:         2018/04/25
 author        yuzhimin
 ****************************************************************/
#include <stdlib.h>
#include "log.h"
#include "ubx_cfg.h"
#include "gps_api.h"
#include "gps_dev.h"
#include "gps_decode.h"
#include "shell_api.h"

/****************************************************************
 function:     gps_shell_clean_distance
 description:  shell cmd clean gps distance(test only)
 input:        int argc, para count;
               const char **argv, para array
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************/
static int gps_shell_clean_distance(int argc, const char **argv)
{
    gps_reset_distance();
    shellprintf(" clean gps odo,distance:%lld!!!\r\n", gps_get_distance());
    return 0;
}

/****************************************************************
 function:     gps_shell_acc_distance
 description:  shell cmd used to acc distance(test only)
 input:        int argc, para count;
               const char **argv, para array
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************/
static int gps_shell_acc_distance(int argc, const char **argv)
{
    int distance;

    if (argc != 1)
    {
        shellprintf(" para error,usage: gpsaccdis x (unit:m)\r\n");
        return GPS_INVALID_PARAMETER;
    }

    distance = atoi(argv[0]);

    gps_acc_distance(distance * 3600);
    gps_save_distance();
    shellprintf(" acc:%d,distance:%lld!!!\r\n", distance, gps_get_distance());
    return 0;
}

/****************************************************************
 function:     gps_shell_get_distance
 description:  shell cmd used to get distance(test only)
 input:        int argc, para count;
               const char **argv, para array
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************/
static int gps_shell_get_distance(int argc, const char **argv)
{
    unsigned long long distance;

    distance = gps_get_distance();

    shellprintf(" gps distance:%llu!!!\r\n", distance);

    return 0;
}

/****************************************************************
 function:     gps_shell_clean_distance
 description:  shell cmd clean gps distance(test only)
 input:        int argc, para count;
               const char **argv, para array
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************/
static int gps_shell_show_data(int argc, const char **argv)
{
    GPS_DATA snap;
    char dir[] = {'N', 'S', 'E', 'W'};
    gps_get_snap(&snap);
    shellprintf(" date=%u, time=%lf, Lat=(%c)%lf, Lng=(%c)%lf, Dir=%lf, knots=%lf,kms=%lf,state=%d, msl=%lf\r\n",
                snap.date, snap.time,
                snap.is_north ? dir[0] : dir[1], snap.latitude,
                snap.is_east ? dir[2] : dir[3], snap.longitude,
                snap.direction, snap.knots, snap.kms, snap.stateUsed, snap.msl);
    return 0;
}

static int gps_shell_reset(int argc, const char **argv)
{
    gps_dev_reset();
    shellprintf(" gps reset!!!\r\n");
    return 0;
}

static int gps_shell_ubx_init(int argc, const char **argv)
{
    if (GNSS_EXTERNAL == GNSS_TYPE)
    {
        gps_dev_ubx_init();
        shellprintf(" ublox config init!!!\r\n");
    }
    else
    {
        shellprintf(" gnss on 4G module,not support ubx cmd!\r\n");
    }

    return 0;
}

int gps_shell_init(void)
{
    int ret = 0;
    ret |= shell_cmd_register_ex("gpsubxinit",  "ubxinit",      gps_shell_ubx_init,
                                 "ubx config init.");
    ret |= shell_cmd_register_ex("gpsubxtest",  "ubx",          ubx_cmd_test,
                                 "ubx cmd send test.");
    ret |= shell_cmd_register_ex("gpsreset",    "gpsreset",     gps_shell_reset,
                                 "reset gps.");
    ret |= shell_cmd_register_ex("gpsclean",    "gpsclean",     gps_shell_clean_distance,
                                 "clean gps distance");
    ret |= shell_cmd_register_ex("gpsaccdis",   "gpsclean",     gps_shell_acc_distance,
                                 "acc gps distance");
    ret |= shell_cmd_register_ex("gpsgetdis",   "gpsgetdis",    gps_shell_get_distance,
                                 "get gps distance");
    ret |= shell_cmd_register_ex("gpsdata",     "gpsdata",      gps_shell_show_data,
                                 "show gps data.");

    return ret;
}

