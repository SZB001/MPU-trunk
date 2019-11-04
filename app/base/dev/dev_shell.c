#include <sys/vfs.h>
#include <dirent.h>
#include "com_app_def.h"
#include "diag.h"
#include "shell_api.h"
#include "fault_sync.h"
#include "status_sync.h"
#include "status/status.h"
#include "tbox_limit.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "gps_api.h"
#include "can_api.h"
#include "dev_time.h"
#include "dev.h"
#include "file.h"
#include "dir.h"
#include "log.h"
#include "nm_api.h"
#include "scom_api.h"
#include "dev_mcu_cfg.h"
#include "dev_mcu_para.h"
#include "at.h"
#include "at_api.h"
#include "dsu_api.h"
#include "pm_api.h"
#include <mcheck.h>
#include <tbox_limit.h>
#include <ft_uds_tbox_rule.h>


/****************************************************************
 function:     dev_shell_show_verion
 description:  get mpu app version
 input:        unsigned int argc, para count;
 unsigned char **argv, para list
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
static int dev_shell_show_verion(int argc, const char **argv)
{
    int ret;
    unsigned char version[64];
    //unsigned int length = 14;      

    memset(version, 0, sizeof(version));
    upg_get_fw_ver(version, sizeof(version));
    shellprintf(" firmware version                 : %s\r\n", version);

    shellprintf(" mpu app version                  : %s\r\n", dev_get_version());

    memset(version, 0, sizeof(version));
    upg_get_mcu_run_ver(version, sizeof(version));
    shellprintf(" mcu app run version              : %s\r\n", version);

    memset(version, 0, sizeof(version));
    ret = upg_get_mcu_upg_ver(version, sizeof(version));

    if (0 == ret)
    {
        shellprintf(" mcu app upgrade version          : %s\r\n", version);
    }
    else
    {
        shellprintf(" mcu app upgrade version      : no upgrade version\r\n");
    }

    memset(version, 0, sizeof(version));
    upg_get_mcu_blt_ver(version, sizeof(version));
    shellprintf(" mcu bootloader version           : %s\r\n", version);

    //memset(version, 0, sizeof(version));
    //cfg_get_para(CFG_ITEM_FT_UDS_HW, version, &length);
    //shellprintf(" custom hw version                : %s\r\n", version);

    //memset(version, 0, sizeof(version));
    //get_uds_sw_ver((UDS_DID_VER*)version);
    //shellprintf(" custom sw version                : %s\r\n", version);
 
    shellprintf(" version show ok\r\n");
    shellprintf(" --------------------------------dataover--------------------------------\r\n");
    return 0;
}

/****************************************************************
 function:     dev_shell_upgrade
 description:  upgrade by shell cmd
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
static int dev_shell_upgrade(int argc, const char **argv)
{
    int ret;
    char *path = COM_APP_PKG_DIR"/"COM_PKG_FILE;

    if (!file_exists(path))
    {
        shellprintf(" error:�ļ�������!\r\n");
        return DEV_FILE_NOT_EXIST;
    }

    ret = upg_app_start(path);
    if (ret != 0)
    {
        shellprintf(" error:ִ������ʧ��!\r\n");
        return ret;
    }

    shellprintf(" ok:�����ļ�У��ͨ��,����ִ������!\r\n");
    return 0;
}

/****************************************************************
 function:     dev_shell_fw_upgrade
 description:  upgrade by shell cmd
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
static int dev_shell_fw_upgrade(int argc, const char **argv)
{
    return upg_fw_start((char *)argv[0]);
}

/*******************************************************************
 function:     dev_shell_show_wakeup_source
 description:  show wakeup source
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_show_wakeup_source(int argc, const char **argv)
{
    int ilen;
    unsigned short wakeup_src;

    ilen = sizeof(wakeup_src);
    st_get(ST_ITEM_WAKEUP_SRC, (unsigned char *) &wakeup_src, (unsigned int *)&ilen);

    switch (wakeup_src)
    {

        case 0x0001:
            shellprintf(" wakeup source: ON signal(%04x)\r\n", wakeup_src);
            break;

        case 0x0002:
            shellprintf(" wakeup source: ACC signal(%04x)\r\n", wakeup_src);
            break;

        case 0x0004:
            shellprintf(" wakeup source: USB signal(%04x)\r\n", wakeup_src);
            break;

        case 0x0008:
            shellprintf(" wakeup source: slow charge signal(%04x)\r\n", wakeup_src);
            break;

        case 0x0010:
            shellprintf(" wakeup source: quick charge signal(%04x)\r\n", wakeup_src);
            break;

        case 0x0020:
            shellprintf(" wakeup source: RING(%04x)\r\n", wakeup_src);
            break;

        case 0x0040:
            shellprintf(" wakeup source: RTC(%04x)\r\n", wakeup_src);
            break;

        case 0x0080:
            shellprintf(" wakeup source: ECU upgrade(%04x)\r\n", wakeup_src);
            break;

        case 0x0100:
            shellprintf(" wakeup source: BT(%04x)\r\n", wakeup_src);
            break;

        case 0x0200:
            shellprintf(" wakeup source: G-SENSOR(%04x)\r\n", wakeup_src);
            break;

        case 0x0400:
            shellprintf(" wakeup source: CAN1(%04x)\r\n", wakeup_src);
            break;

        case 0x0800:
            shellprintf(" wakeup source: CAN2(%04x)\r\n", wakeup_src);
            break;

        case 0x1000:
            shellprintf(" wakeup source: CAN3(%04x)\r\n", wakeup_src);
            break;

        default:
            shellprintf(" wakeup source: unknown(%04x)\r\n", wakeup_src);
            break;
    }

    return 0;
}

/*******************************************************************
function:     dev_shell_show_fault
description:  show tbox fault infomation
input:        unsigned int argc
              unsigned char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_show_fault(int argc, const char **argv)
{
    DIAG_MCU_FAULT mcu_fault;
    DIAG_MPU_FAULT mpu_fault;
    flt_get_mcu(&mcu_fault);
    flt_get_mpu(&mpu_fault);

    shellprintf(" DTC: \tCAN_ERR-%d\tGPS_ERR-%d\t4G_ERR-%d\tSIM_ERR-%d\tWIFI_ERR-%d\tBATTERY_ERR-%d\tEMMC_ERR-%d\r\n",
                (1 == mcu_fault.can_node[0]) ? 0 : 1,
                ((1 == mpu_fault.gps) && (1 == mcu_fault.gps_ant)) ? 0 : 1,
                (1 == mpu_fault.gprs) ? 0 : 1,
                (1 == mpu_fault.sim) ? 0 : 1,
                (1 == mpu_fault.wifi) ? 0 : 1,
                ((1 == mcu_fault.battery) || ((0 == mcu_fault.battery))) ? 0 : 1,
                (1 == mpu_fault.emmc) ? 0 : 1);
    shellprintf(" --------------------------------dataover--------------------------------\r\n");

    return 0;
}

/*******************************************************************
 function:     dev_shell_setdno
 description:  set devices number
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_setdno(int argc, const char **argv)
{
    int ret;
    unsigned int dev_no;

    if (argc != 1)
    {
        shellprintf(" usage: setdno <no>\r\n");
        return DEV_INVALID_PARA;
    }

    dev_no = atoi(argv[0]);
    ret = cfg_set_para(CFG_ITEM_DEV_NUM, (unsigned char *) &dev_no, sizeof(unsigned int));

    if (0 != ret)
    {
        shellprintf(" set devices number failed\r\n");
        return ret;
    }

    shellprintf(" set devices number ok !\r\n");
    return 0;
}

/*******************************************************************
 function:     dev_shell_setsn
 description:  set sn number
 input:        unsigned int argc
               unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 ********************************************************************/
static int dev_shell_setsn(int argc, const char **argv)
{
    int ret;
    unsigned int sn;

    if (argc != 1)
    {
        shellprintf(" usage: setsn <sn>\r\n");
        return DEV_INVALID_PARA;
    }

    sn = atoi(argv[0]);

    ret = dev_set_from_mpu(MCU_CFG_ID_SN, &sn, sizeof(sn));

    if (0 != ret)
    {
        shellprintf(" set sn number failed\r\n");
        return ret;
    }

    shellprintf(" set sn ok!\r\n");
    shellprintf(" please restart terminal!\r\n");
    return 0;
}

/*******************************************************************
 function:     dev_shell_show_task
 description:  show the pthread of current process
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_show_task(int argc, const char **argv)
{
    FILE *ptream;
    char buff[2048];
    char line[64];
    int len = 0;
    int i, pos;
    ptream = popen("ps -T | grep tbox_app.bin | grep -v \"grep\"", "r");

    if (!ptream)
    {
        log_e(LOG_DEV, "open dev failed , error:%s", strerror(errno));
        return 0;
    }

    len = fread(buff, sizeof(char), sizeof(buff), ptream);
    pclose(ptream);
    pos = 0;
    shellprintf("\n");

    for (i = 0; i < len; i++)
    {
        if ('\n' == buff[i] && pos < len)
        {
            memset(line, 0, sizeof(line));
            memcpy(line, &buff[pos], i - pos);
            shellprintf("%s\r\n", line);
            pos = i + 1;
        }
    }

    if (pos < len)
    {
        memset(line, 0, sizeof(line));
        memcpy(line, &buff[pos], i - pos);
        shellprintf("%s\r\n", line);
    }

    return 0;
}

/*******************************************************************
 function:     dev_shell_show_time
 description:  show the time of system
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_show_time(int argc, const char **argv)
{
    int ret;
    RTCTIME abstime;
    ret = tm_get_abstime(&abstime);

    if (0 != ret)
    {
        shellprintf(" get system time failed\r\n");
        return ret;
    }

    shellprintf(" system time:%04d/%02d/%02d %02d:%02d:%02d\r\n",
                abstime.year, abstime.mon, abstime.mday, abstime.hour, abstime.min, abstime.sec);
    return 0;
}

/*******************************************************************
 function:     dev_shell_set_time
 description:  set the time of system
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_set_time(int argc, const char **argv)
{
    int ret;
    RTCTIME time;
    unsigned int year, mon, day, hour, min, sec;

    if (argc != 2)
    {
        shellprintf(" usage: settime year-mon-day hour:min:sec\r\n");
        shellprintf(" example: settime 2018-1-29 11:30:30\r\n");
        return DEV_INVALID_PARA;
    }

    if (3 != sscanf((char *) argv[0], "%u-%u-%u", &year, &mon, &day))
    {
        shellprintf(" format error\r\n");
        return DEV_INVALID_PARA;
    }

    time.year = year;
    time.mon = mon;
    time.mday = day;

    if (3 != sscanf((char *) argv[1], "%u:%u:%u", &hour, &min, &sec))
    {
        shellprintf(" format error\r\n");
        return DEV_INVALID_PARA;
    }

    time.hour = hour;
    time.min = min;
    time.sec = sec;

    if (0 > dev_time_chk(year, mon, day, hour, min, sec))
    {
        shellprintf(" input date error:%d-%d-%d %d:%d:%d\r\n",
                    year, mon, day, hour, min, sec);
        return DEV_INVALID_PARA;
    }

    log_o(LOG_SHELL, "shell set time: %d-%d-%d %d:%d:%d",
          time.year, time.mon, time.mday, time.hour, time.min, time.sec);

    ret = dev_syn_time(&time, SHELL_TIME_SOURCE);

    if (0 != ret)
    {
        shellprintf(" set system time failed!\r\n");
        return ret;
    }

    shellprintf(" set system time ok!\r\n");
    return 0;
}

/*******************************************************************
 function:     dev_shell_format_emmc
 description:  format the emmc
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_format_emmc(int argc, const char **argv)
{
    FILE *ptream = NULL;
    unsigned char buf[64];
    unsigned long long base;
    dev_diag_stop();
    dsu_suspend_record();
    shellprintf(" format emmc begin!\r\n");
    log_o(LOG_SHELL, "close opened files,wait 3s!");
    dev_log_save_suspend();
    sleep(2);
    dev_diag_emmc_set_format_flag(1);
    sleep(1);
    memset(buf, '\0', sizeof(buf));
    base = tm_get_systick();
    log_o(LOG_SHELL, "[%llu]umount start!", base);
    ptream = popen("umount -l /media/sdcard/", "r");

    if (!ptream)
    {
        shellprintf(" format emmc failed!\r\n");
        dev_diag_emmc_set_format_flag(0);
        return DEV_INVALID_PARA;
    }

    log_o(LOG_SHELL, "[%llu]umount end!", tm_get_systick() - base);
    pclose(ptream);
    sleep(1);
    log_o(LOG_SHELL, "[%llu]mkdosfs start!", tm_get_systick() - base);
    ptream = popen("mkdosfs /dev/mmcblk0p1", "r");

    if (!ptream)
    {
        shellprintf(" format emmc failed!\r\n");
        dev_diag_emmc_set_format_flag(0);
        return DEV_INVALID_PARA;
    }

    log_o(LOG_SHELL, "[%llu]mkdosfs end!", tm_get_systick() - base);
    pclose(ptream);
    sleep(2);
    log_o(LOG_SHELL, "[%llu]mount start!", tm_get_systick() - base);
    ptream = popen("mount /dev/mmcblk0p1 /media/sdcard/", "r");

    if (!ptream)
    {
        shellprintf(" format emmc failed!\r\n");
        dev_diag_emmc_set_format_flag(0);
        return DEV_INVALID_PARA;
    }

    log_o(LOG_SHELL, "[%llu]mount end!", tm_get_systick() - base);
    pclose(ptream);

    log_o(LOG_SHELL, "wait 3s start record!");
    sleep(3);
    dev_diag_emmc_set_format_flag(0);
    shellprintf(" format emmc ok!\r\n");
    dev_log_save_resume();
    log_o(LOG_SHELL, "dsu_resume_record!");
    dsu_resume_record();
    dev_diag_start();
    return 0;
}

/*******************************************************************
 function:     dev_shell_reset
 description:  reset the system
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_reset(int argc, const char **argv)
{
    int i;
    dsu_suspend_record();

    for (i = 0; i < 3; i++)
    {
        system("reboot");
    }

    return 0;
}

/*******************************************************************
 function:     dev_shell_dump_status
 description:  dump the tbox status
 input:        unsigned int argc
 unsigned char **argv
 output:       none
 return:       0 indicates success;
 others indicates failed
 ********************************************************************/
static int dev_shell_dump_status(int argc, const char **argv)
{
    unsigned char MD5[16];
    unsigned char ICCID[CCID_LEN];
    unsigned char IMEI[TBOX_IMEI_LEN];
    unsigned char IMSI[TBOX_IMSI_LEN];
    unsigned char telno[TBOX_TEL_LEN];
    unsigned char main_ant = 0;
    unsigned char vice_ant = 0;
    unsigned char gnss_ant;
    char op_name[OP_NAME_LEN];

    main_ant = dev_diag_get_ant_status(ANT_4G_MAIN);
    vice_ant = dev_diag_get_ant_status(ANT_4G_VICE);
    gnss_ant = dev_diag_get_ant_status(ANT_GNSS);

    shellprintf("\n");
    shellprintf(" MPU APP REBOOT CNT:%u\r\n", dev_get_reboot_cnt());
    st_dump(argc, argv);
    double km = gps_get_distance() / (1000 * 3600.0);
    shellprintf(" GPS ODO:%.3lf KMs\r\n", km);

    switch (gnss_ant)
    {
        case ANT_UNKNOW:
            shellprintf(" GNSS ant unkonw\r\n");
            break;

        case ANT_OK:
            shellprintf(" GNSS ant OK!\r\n");
            break;

        case ANT_SHORT:
            shellprintf(" GNSS ant short\r\n");
            break;

        case ANT_OPEN:
            shellprintf(" GNSS ant open\r\n");
            break;

        default:
            break;
    }

    switch (main_ant)
    {
        case ANT_UNKNOW:
            shellprintf(" 4G main ant unkonw\r\n");
            break;

        case ANT_OK:
            shellprintf(" 4G main ant OK!\r\n");
            break;

        case ANT_SHORT:
            shellprintf(" 4G main ant short\r\n");
            break;

        case ANT_OPEN:
            shellprintf(" 4G main ant open\r\n");
            break;

        default:
            break;
    }

    switch (vice_ant)
    {
        case ANT_UNKNOW:
            shellprintf(" 4G vice ant unkonw\r\n");
            break;

        case ANT_OK:
            shellprintf(" 4G vice ant OK!\r\n");
            break;

        case ANT_SHORT:
            shellprintf(" 4G vice ant short\r\n");
            break;

        case ANT_OPEN:
            shellprintf(" 4G vice ant open\r\n");
            break;

        default:
            break;
    }

    switch (dev_diag_get_emmc_status())
    {
        case DIAG_EMMC_OK:
            shellprintf(" eMMC OK!\r\n");
            struct statfs diskInfo;
            statfs(COM_SDCARD_DIR, &diskInfo);

            unsigned long long blocksize = diskInfo.f_bsize;                    //ÿ��block��������ֽ���
            unsigned long long totalsize = blocksize *
                                           diskInfo.f_blocks;                   //�ܵ��ֽ�����f_blocksΪblock����Ŀ
            unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //���ÿռ��С

            unsigned int total_size_mb = totalsize >> 20;
            unsigned int free_size_mb  = availableDisk >> 20;

            shellprintf(" eMMC total size:%u MB\r\n", total_size_mb);
            shellprintf(" eMMC free size:%u MB\r\n", free_size_mb);
			
            break;

        case DIAG_EMMC_FULL:
            shellprintf(" eMMC full\r\n");
            break;

        case DIAG_EMMC_UMOUNT:
            shellprintf(" eMMC umount\r\n");
            break;

        case DIAG_EMMC_NOT_EXIST:
            shellprintf(" eMMC no exist\r\n");
            break;

        case DIAG_EMMC_NOT_FORMAT:
            shellprintf(" eMMC no format\r\n");
            break;

        case DIAG_EMMC_UMOUNT_POINT_NOT_EXIST:
            shellprintf(" eMMC mount point is not exist\r\n");
            break;

        case DIAG_EMMC_FORMATTING:
            shellprintf(" eMMC is formatting\r\n");
            break;

        default:
            shellprintf(" eMMC unknow Err\r\n");
            break;
    }

    switch (gps_get_fix_status())
    {
        case GPS_UNCONNECTED:
            shellprintf(" GNSS unconnected!\r\n");
            break;

        case GPS_UNFIX:
            shellprintf(" GNSS unfix\r\n");
            break;

        case GPS_FIX:
            shellprintf(" GNSS fix\r\n");
            break;

        case GPS_ERROR:
            shellprintf(" GNSS fault\r\n");
            break;

        default:
            shellprintf(" GNSS unknow Err\r\n");
            break;
    }

    shellprintf(" CAN node 1: %s\r\n", (flt_get_by_id(CAN_NODE1) == 1) ? "normal" : "fault");
    shellprintf(" CAN node 2: %s\r\n", (flt_get_by_id(CAN_NODE2) == 1) ? "normal" : "fault");
    shellprintf(" CAN node 3: %s\r\n", (flt_get_by_id(CAN_NODE3) == 1) ? "normal" : "fault");
    shellprintf(" CAN node 4: %s\r\n", (flt_get_by_id(CAN_NODE4) == 1) ? "normal" : "fault");
    shellprintf(" CAN node 5: %s\r\n", (flt_get_by_id(CAN_NODE5) == 1) ? "normal" : "fault");
    shellprintf(" CAN busoff 1: %s\r\n", (flt_get_by_id(CAN_BUS1) == 1) ? "normal" : "fault");
    shellprintf(" CAN busoff 2: %s\r\n", (flt_get_by_id(CAN_BUS2) == 1) ? "normal" : "fault");
    shellprintf(" CAN busoff 3: %s\r\n", (flt_get_by_id(CAN_BUS3) == 1) ? "normal" : "fault");
    shellprintf(" CAN busoff 4: %s\r\n", (flt_get_by_id(CAN_BUS4) == 1) ? "normal" : "fault");
    shellprintf(" CAN busoff 5: %s\r\n", (flt_get_by_id(CAN_BUS5) == 1) ? "normal" : "fault");



    switch (flt_get_by_id(GPS_ANT))
    {
        case 0X00:
            shellprintf(" GPSANT unknow Err!\r\n");
            break;

        case 0X01:
            shellprintf(" GPSANT ok\r\n");
            break;

        case 0X02:
            shellprintf(" GPSANT short\r\n");
            break;

        case 0X03:
            shellprintf(" GPSANT open\r\n");
            break;

        default:
            shellprintf(" GPSANT unknow Err\r\n");
            break;
    }

    switch (flt_get_by_id(MIC))
    {
        case 0x00:
            shellprintf(" MIC power ok\r\n");
            break;

        case 0x01:
            shellprintf(" MIC power error!\r\n");
            break;

        default:
            shellprintf(" MIC power unknown!\r\n");
            break;
    }

    switch (flt_get_by_id(MICSTATUS))
    {
        case 0x00:
            shellprintf(" MIC status unknown\r\n");
            break;

        case 0x01:
            shellprintf(" MIC status short to gnd error!\r\n");
            break;

        case 0x02:
            shellprintf(" MIC status short to power error!\r\n");
            break;

        case 0x03:
            shellprintf(" MIC status open error!\r\n");
            break;

        case 0x04:
            shellprintf(" MIC status ok!\r\n");
            break;

        case 0xff:
            shellprintf(" MIC status not support!\r\n");
            break;

        default:
            shellprintf(" MIC status unknown!\r\n");
            break;
    }

    switch (flt_get_by_id(SOSBTN))
    {
        case 0x00:
            shellprintf(" SOSBTN status unknown error!\r\n");
            break;

        case 0x01:
            shellprintf(" SOSBTN UP: DISABLE!\r\n");
            break;

        case 0x02:
            shellprintf(" SOSBTN DOWN: ENABLE!\r\n");
            break;

        case 0xff:
            shellprintf(" SOSBTN status not support!\r\n");
            break;

        default:
            shellprintf(" SOSBTN status unknown!\r\n");
            break;
    }

    switch (flt_get_by_id(SPK))
    {
        case 0x00:
            shellprintf(" SPK status unknown error!\r\n");
            break;

        case 0x01:
            shellprintf(" SPK short!\r\n");
            break;

        case 0x02:
            shellprintf(" SPK open!\r\n");
            break;

        case 0x03:
            shellprintf(" SPK ok\r\n");
            break;

        case 0xff:
            shellprintf(" SPK status not support\r\n");
            break;

        default:
            shellprintf(" SPK status unknown!\r\n");
            break;

    }

    if (2 == flt_get_by_id(WIFI))
    {
        shellprintf(" Wifi status: fault\r\n");
    }
    else
    {
        shellprintf(" Wifi status: %s\r\n",  1 == at_get_wifi_status() ? "opened" : "closed");
    }

    shellprintf(" Battery aged[UDS]: %s\r\n", 2 != flt_get_by_id(BAT) ? "nomal" : "fault");

    shellprintf(" Battery voltage status[UDS]: %s\r\n",
                (2 != flt_get_by_id(BATVOL)
                 && 3 != flt_get_by_id(BATVOL)) ? "nomal" : "fault");
    
    shellprintf(" 4G signal: %d\r\n", nm_get_signal());
    shellprintf(" SIM status: %s\r\n", 1 == dev_diag_get_sim_status() ? "normal" : "fault");

    memset(ICCID, 0, sizeof(ICCID));
    nm_get_iccid((char *)ICCID);
    shellprintf(" ICCID                            : %s\r\n", ICCID);

    memset(IMEI, 0, sizeof(IMEI));
    at_get_imei((char *)IMEI);
    shellprintf(" IMEI                             : %s\r\n", IMEI);

    memset(IMSI, 0, sizeof(IMSI));
    at_get_imsi((char *)IMSI);
    shellprintf(" IMSI                             : %s\r\n", IMSI);

    memset(telno, 0, sizeof(telno));
    at_get_telno((char *)telno);
    shellprintf(" TELNO                            : %s\r\n", telno);

    memset(op_name, 0, sizeof(op_name));
    at_get_operator((unsigned char *)op_name);
    shellprintf(" operator: %s\r\n", op_name);

    switch (at_get_net_type())
    {
        case 0:
            shellprintf(" net type: 2G\r\n");
            break;

        case 2:
            shellprintf(" net type: 3G\r\n");
            break;

        case 7:
            shellprintf(" net type: 4G\r\n");
            break;

        case 100:  // CDMA
            shellprintf(" net type: 3G\r\n");
            break;

        default:
            shellprintf(" net type: unknown\r\n");
            break;
    }

    memset(MD5, 0, sizeof(MD5));
    dbc_lock();
    dbc_get_md5(MD5);
    dbc_unlock();
    shellprintf(" MD5: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                MD5[0], MD5[1], MD5[2], MD5[3], MD5[4], MD5[5], MD5[6], MD5[7], MD5[8], MD5[9], MD5[10], MD5[11],
                MD5[12], MD5[13], MD5[14], MD5[15]);

    shellprintf(" --------------------------------dataover--------------------------------\r\n");

    return 0;
}

/*******************************************************************
function:     dev_shell_set_vtd
description:  vehicle theft deterrent setting to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_vtd(int argc, const char **argv)
{
    unsigned char onoff;

    if (argc != 1)
    {
        shellprintf(" usage: setvtd on/off\r\n");
        return DEV_INVALID_PARA;
    }

    if (0 == strncmp(argv[0], "on", 2))
    {
        onoff = 1;
    }
    else if (0 == strncmp(argv[0], "off", 3))
    {
        onoff = 0;
    }
    else
    {
        shellprintf(" usage: setvtd on/off\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_VTD, &onoff, sizeof(onoff));

}

/*******************************************************************
function:     dev_shell_set_ecall
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_ecall(int argc, const char **argv)
{
    unsigned char onoff;

    if (argc != 1)
    {
        shellprintf(" usage: setecallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    if (0 == strncmp(argv[0], "on", 2))
    {
        onoff = 1;
    }
    else if (0 == strncmp(argv[0], "off", 3))
    {
        onoff = 0;
    }
    else
    {
        shellprintf(" usage: setecallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_ECALL, &onoff, sizeof(onoff));

}

/*******************************************************************
function:     dev_shell_set_bcall
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_bcall(int argc, const char **argv)
{
    unsigned char onoff;

    if (argc != 1)
    {
        shellprintf(" usage: setbcallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    if (0 == strncmp(argv[0], "on", 2))
    {
        onoff = 1;
    }
    else if (0 == strncmp(argv[0], "off", 3))
    {
        onoff = 0;
    }
    else
    {
        shellprintf(" usage: setbcallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_BCALL, &onoff, sizeof(onoff));

}

/*******************************************************************
function:     dev_shell_set_icall
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_icall(int argc, const char **argv)
{
    unsigned char onoff;

    if (argc != 1)
    {
        shellprintf(" usage: seticallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    if (0 == strncmp(argv[0], "on", 2))
    {
        onoff = 1;
    }
    else if (0 == strncmp(argv[0], "off", 3))
    {
        onoff = 0;
    }
    else
    {
        shellprintf(" usage: seticallfun on/off\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_ICALL, &onoff, sizeof(onoff));

}

/*******************************************************************
function:     pm_set_mode
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_stime(int argc, const char **argv)
{
    unsigned int time;

    if (argc != 1)
    {
        shellprintf(" usage: setstime <time(min)>\r\n");
        return DEV_INVALID_PARA;
    }

    time = atoi(argv[0]);

    return dev_set_from_mpu(MCU_CFG_ID_STIME, &time, sizeof(time));

}

/*******************************************************************
function:     pm_set_mode
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_dstime(int argc, const char **argv)
{
    unsigned int time;

    if (argc != 1)
    {
        shellprintf(" usage: setdstime <time(min)>\r\n");
        return DEV_INVALID_PARA;
    }

    time = atoi(argv[0]);

    return dev_set_from_mpu(MCU_CFG_ID_DSTIME, &time, sizeof(time));
}

/*******************************************************************
function:     dev_shell_set_accvecm
description:  set acc interrupt threshold
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_accvecm(int argc, const char **argv)
{
    unsigned short threshold;

    if (argc != 1)
    {
        shellprintf(" usage: setaccvecm <0~4095>\r\n");
        return DEV_INVALID_PARA;
    }

    threshold = atoi(argv[0]);

    if (threshold > 4096)
    {
        shellprintf(" the value is in 0~4095\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_ACCTHS, &threshold, sizeof(threshold));

}

/*******************************************************************
function:     dev_shell_set_mode
description:  set power manager mode and sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_mode(int argc, const char **argv)
{
    unsigned char mode;

    if (argc != 1)
    {
        shellprintf(" usage: setmode x(0:runing 1:listen 2:sleep 3:auto)\r\n");
        return DEV_INVALID_PARA;
    }

    mode = atoi(argv[0]);

    if (mode > PM_AUTO)
    {
        shellprintf(" invalid parameter: %u\r\n", mode);
        shellprintf(" usage: setmode x(0:runing 1:listen 2:sleep 3:auto)\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_SYSMODE, &mode, sizeof(mode));

}

/*******************************************************************
function:     pm_shell_set_bat
description:  set the battery type
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_bat(int argc, const char **argv)
{
    unsigned char bat_types;

    if (argc != 1)
    {
        shellprintf(" usage: setbat <x>\r\n");
        shellprintf(" 0:default,PANASONIC(500) 1:VARTA(450) 2:VARTA(500) 3:PANASONIC(500)\r\n");
        return DEV_INVALID_PARA;
    }

    bat_types = atoi(argv[0]);

    if (bat_types > 3)
    {
        shellprintf(" invalid parameter: %u\r\n", bat_types);
        shellprintf(" 0:default,PANASONIC(500) 1:VARTA(450) 2:VARTA(500) 3:PANASONIC(500)\r\n");
        return DEV_INVALID_PARA;
    }

    return dev_set_from_mpu(MCU_CFG_ID_BAT_TYPE, &bat_types, sizeof(bat_types));

}

/*******************************************************************
function:     pm_shell_set_log
description:  sync config to mcu
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_set_log(int argc, const char **argv)
{
    int en;

    if (argc != 1)
    {
        shellprintf(" usage: setlog <0/1>\r\n");
        return DEV_INVALID_PARA;
    }

    en = atoi(argv[0]) ? 1 : 0;

    if (en && !bfile_exists("/dev/mmcblk0p1"))
    {
        shellprintf(" eMMC has not been formated!!!\r\n");
        return DEV_FILE_NOT_EXIST;
    }

    if (en && !path_exists(COM_LOG_DIR) &&
        dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false) != 0)
    {
        shellprintf(" make log directory(%s) failed\r\n", COM_LOG_DIR);
        return -1;
    }

    if (cfg_set_para(CFG_ITEM_LOG_ENABLE, &en, 1) != 0)
    {
        shellprintf(" save configuration failed!\r\n");
        return -1;
    }

    log_save_ctrl(en ? dev_get_reboot_cnt() : 0, COM_LOG_DIR);

    if (en)
    {
        dev_print_softver_delay();
    }

    return 0;
}

/*******************************************************************
function:     dev_shell_sync_log
description:  sync log fd to save buffer
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_sync_log(int argc, const char **argv)
{
    log_sync();
    shellprintf(" sync ok\r\n");
    return 0;
}

/*******************************************************************
function:     dev_shell_dbg_level
description:  show the task log level
input:        int argc
              char **argv
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************/
static int dev_shell_dbg_level(int argc, const char **argv)
{
    log_level_show();
    return 0;
}

/*******************************************************************
 function:     dev_shell_set_user_para
 description:  set user para to mcu
 input:        unsigned int argc
               unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 ********************************************************************/
static int dev_shell_set_user_para(int argc, const char **argv)
{
    int ret, len;
    unsigned int index;

    if (argc != 2)
    {
        shellprintf(" usage: userpara <id> <content>\r\n");
        return DEV_INVALID_PARA;
    }

    index = atoi(argv[0]);
    len = strlen(argv[1]);
    ret = dev_send_custom_para_to_mcu(index & 0xFF, argv[1], len);

    if (0 != ret)
    {
        shellprintf(" set userpara failed\r\n");
        return ret;
    }

    shellprintf(" id:%d, content=%s, len=%d\r\n", index, argv[1], len);
    shellprintf(" set userpara ok!\r\n");
    return 0;
}

/*******************************************************************
 function:     dev_shell_mcu_shell_req
 description:  send mcu shell cmd to mcu
 input:        unsigned int argc
               unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 ********************************************************************/
static int dev_shell_mcu_shell_req(int argc, const char **argv)
{
    int i, ret;
    char cmd[64];

    if (argc < 1)
    {
        shellprintf(" usage: mcudbg cmd para ...\r\n");
        return DEV_INVALID_PARA;
    }

    memset(cmd, 0, sizeof(cmd));

    for (i = 0; i < argc; i++)
    {
        strcat(cmd, argv[i]);

        if (i < argc - 1)
        {
            strcat(cmd, " ");
        }
    }

    shellprintf("############mcu shell cmd:%s,wait ack############\r\n", cmd);
    ret = scom_tl_send_frame(SCOM_TL_CMD_MCU_SHELL, SCOM_TL_SINGLE_FRAME, 0, (unsigned char *)cmd,
                             strlen(cmd) + 1);

    if (0 != ret)
    {
        shellprintf("mcudbg cmd send to mcu failed:%s,ret:%u\r\n", cmd, ret);
        return ret;
    }

    return 0;
}

/*******************************************************************
 function:     dev_shell_mcu_shell_res
 description:  process shell response from mcu
 input:        unsigned int argc
               unsigned char **argv
 output:       none
 return:       0 indicates success;
               others indicates failed
 ********************************************************************/
int dev_shell_mcu_shell_res(unsigned char *msg, unsigned int len)
{
    msg[len] = 0;

    shellprintf("mcu shell res:\r\n");
    shellprintf("%s\r\n", (char *)msg);

    return 0;
}

int dev_shell_intest_hw_set(int argc, const char **argv)
{
    int ret = 0;
    char intest_hw[32];
    unsigned int length = 0;

    if (argc != 2)
    {
        shellprintf(" usage: set intset hw ver. format: shell_cli.bin sethw XXXXX XXXXX\r\n");
        return -1;
    }

    if (strlen(argv[0] + strlen(argv[1])) > 32)
    {
        shellprintf(" error: set intset hw ver more than 32\r\n");
        return -1;
    }

    memset(intest_hw, 0, 32);

    length = strlen(argv[0]);
    
    memcpy(intest_hw, (void *)argv[0], strlen(argv[0]));
    
    intest_hw[length -1] = ' ';

    memcpy(intest_hw + length, (void *)argv[1], strlen(argv[1]));
     
    ret =  cfg_set_para(CFG_ITEM_INTEST_HW, intest_hw, 32);

    return ret;  
}


/****************************************************************
 function:     dev_shell_init
 description:  initiaze dev module shell cmd
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int dev_shell_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* register shell cmd */
            shell_cmd_register("version", dev_shell_show_verion, "show software version");
            shell_cmd_register("pkgupgrade", dev_shell_upgrade, "pkg upgrade");
            shell_cmd_register("fwupdate", dev_shell_fw_upgrade, "firmware upgrade");
            shell_cmd_register("dumpw", dev_shell_show_wakeup_source, "show wakeup source");
            shell_cmd_register("dumperr", dev_shell_show_fault, "show system fault");
            shell_cmd_register("setdno", dev_shell_setdno, "set devices number");
            shell_cmd_register("setsn", dev_shell_setsn, "set sn number");
            shell_cmd_register("task", dev_shell_show_task, "show system task info");
            shell_cmd_register("time", dev_shell_show_time,          "show system time info");
            shell_cmd_register("settime", dev_shell_set_time, "set system time");
            shell_cmd_register("format", dev_shell_format_emmc, "format eMMC card");
            shell_cmd_register("reset", dev_shell_reset, "reset 4G module");
            shell_cmd_register("dumpst", dev_shell_dump_status, "dump status");

            shell_cmd_register("setvtd", dev_shell_set_vtd, "set vehicle theft deterrent to mcu");
            shell_cmd_register("seticallfun", dev_shell_set_icall, "set icall fun to mcu");
            shell_cmd_register("setbcallfun", dev_shell_set_bcall, "set bcall fun to mcu");
            shell_cmd_register("setecallfun", dev_shell_set_ecall, "set ecall fun to mcu");
            shell_cmd_register("setstime", dev_shell_set_stime, "set sleep time");
            shell_cmd_register("setdstime", dev_shell_set_dstime, "set deep sleep time");
            shell_cmd_register("setaccvecm", dev_shell_set_accvecm, "set G-sensor threshold");
            shell_cmd_register("setmode",   dev_shell_set_mode, "sync power config strategy to mcu");
            shell_cmd_register("setbat",   dev_shell_set_bat, "set battery type");
            shell_cmd_register("setlog",   dev_shell_set_log, "set whether the log will be saved");
            shell_cmd_register("sync",   dev_shell_sync_log, "save the log in buffer");
            shell_cmd_register("showdbg",   dev_shell_dbg_level, "show the task log level");
            shell_cmd_register("userpara", dev_shell_set_user_para, "set user para to mcu");
            shell_cmd_register("mcudbg",   dev_shell_mcu_shell_req, "mcu shell cmd");
            shell_cmd_register("sethw", dev_shell_intest_hw_set, "set intest hw");
            break;

        default:
            break;
    }

    return 0;
}

