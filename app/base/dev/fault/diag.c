#include <sys/vfs.h>
#include <dirent.h>
#include "com_app_def.h"
#include "com_app_def.h"
#include "fault_sync.h"
#include "at_api.h"
#include "dev_api.h"
#include "diag.h"
#include "gpio.h"
#include "log.h"
#include "file.h"
#include "dir.h"
#include "gps_api.h"
#include "pm_api.h"
#include "timer.h"

static unsigned char main_last_status = 0;
static unsigned char vice_last_status = 0;
static char main_ant_file[50];
static char vice_ant_file[50];
static unsigned char dev_diag_available = 1;
static pthread_mutex_t dev_diag_mutex;

static unsigned char is_emmc_formatting = 0; //0: normal;1: formatting
static pthread_mutex_t dev_emmc_format_mutex;
/****************************************************************
 function:     dev_diag_get_ant_name
 description:  get ant status
 input:        none
 output:       none
 return:       0 successful
               others unknow
 *****************************************************************/
static int dev_diag_get_ant_name(void)
{
    DIR *dp;
    struct dirent *cur;
    dp = opendir("/sys/devices/");

    if (dp == NULL)
    {
        log_e(LOG_DEV, "open \"/sys/devices/\"  failed, error:%s", strerror(errno));
        return DEV_DIR_READ_FAILED;
    }

    while ((cur = readdir(dp)) != NULL)
    {
        if (strstr(cur->d_name, "qpnp-vadc-"))
        {
            break;
        }
    }

    if (NULL == cur)
    {
        log_e(LOG_DEV, "thers is no adc dir");
        closedir(dp);
        return DEV_DIR_NOT_EXIST;
    }

    memset(main_ant_file, 0, sizeof(main_ant_file));
    memset(vice_ant_file, 0, sizeof(vice_ant_file));
    memcpy(main_ant_file, "/sys/devices/", strlen("/sys/devices/"));
    memcpy(vice_ant_file, "/sys/devices/", strlen("/sys/devices/"));
    strcat(main_ant_file, cur->d_name);
    strcat(vice_ant_file, cur->d_name);
    strcat(main_ant_file, "/mpp4_vadc");
    strcat(vice_ant_file, "/mpp6_vadc");
    closedir(dp);

    log_o(LOG_DEV, "main ant file: %s", main_ant_file);
    log_o(LOG_DEV, "vice ant file: %s", vice_ant_file);

    return 0;
}

/****************************************************************
 function:     dev_diag_get_ant_status
 description:  get ant status
 input:        ANT_TYPE type
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates short circuit;
 3 indicates open;
 *****************************************************************/
int dev_diag_get_ant_status(ANT_TYPE type)
{
    int ret = 0;
    int adc_value = 0;
    unsigned char buff[32];

    if (ANT_GNSS == type)
    {
        return flt_get_by_id(GPS_ANT);
    }

    memset(buff, '\0', sizeof(buff));

    if (ANT_4G_MAIN == type)
    {
        unsigned int len = 15;
        ret = file_read(main_ant_file, buff, &len);

        if (0 != ret)
        {
            log_e(LOG_DEV, "read %s failed !", main_ant_file);
            return ret;
        }
    }
    else if (ANT_4G_VICE == type)
    {
        unsigned int len = 15;
        file_read(vice_ant_file, buff, &len);

        if (0 != ret)
        {
            log_e(LOG_DEV, "read %s failed !", vice_ant_file);
            return ret;
        }
    }

    if (sscanf((char *)buff, "Result:%d ", &adc_value))
    {
        adc_value = adc_value / 1000;
        log_i(LOG_DEV, "adc-%d = %d ", type, adc_value);

        if ((adc_value > 1500) && (adc_value < 4096))
        {
            return ANT_OPEN;
        }
        else if ((adc_value > 0) && (adc_value < 300))
        {
            return ANT_SHORT;
        }
        else
        {
            if (ANT_4G_MAIN == type)
            {
                if ((adc_value > 400) && (adc_value < 800))
                {
                    return ANT_OK;
                }
                else
                {
                    return ANT_UNKNOW;
                }
            }
            else if (ANT_4G_VICE == type)
            {
                if ((adc_value > 700) && (adc_value < 1100))
                {
                    return ANT_OK;
                }
                else
                {
                    return ANT_UNKNOW;
                }
            }
        }

    }

    return ret;
}

/****************************************************************
 function:     dev_diag_get_4G_status
 description:  get 4G status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_4G_status(void)
{
    return at_get_status();
}

/****************************************************************
 function:     dev_diag_get_wifi_status
 description:  get wifi status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_wifi_status(void)
{
    return at_wifi_get_status();
}

/****************************************************************
 function:     dev_diag_get_bat_status
 description:  get battery status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 3 indicates no install;
 *****************************************************************/
int dev_diag_get_bat_status(void)
{
    return flt_get_by_id(BAT);
}

/****************************************************************
 function:     dev_diag_get_pow_status
 description:  get power voltage status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_pow_status(void)
{
    return flt_get_by_id(POWER);
}

/****************************************************************
 function:     dev_diag_get_can_status
 description:  get can status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_can_status(int index)
{
    return 0;
}

/****************************************************************
 function:     dev_diag_get_usb_status
 description:  get usb status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_usb_status(void)
{
    return 0;
}

/****************************************************************
 function:     dev_diag_get_gps_status
 description:  get gps status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_gps_status(void)
{
    if (GPS_ERROR == gps_get_fix_status())
    {
        return 2;
    }

    return 1;
}

/****************************************************************
 function:     dev_diag_get_emmc_status
 description:  get emmc status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates full;
 3 indicates umount;
 4 indicates not exist;
 5 indicates not formated;
 6 indicates mount ponit not exist;
 *****************************************************************/
int dev_diag_get_emmc_status(void)
{
    struct statfs st_fs;
    unsigned long long total_bytes = 0;
    unsigned long long free_bytes = 0;
    unsigned long long avail_bytes;

    if (dev_diag_emmc_get_format_flag())
    {
        return DIAG_EMMC_FORMATTING;
    }

    if (!bfile_exists("/dev/mmcblk0"))
    {
        return DIAG_EMMC_NOT_EXIST;  //emmc not exist
    }

    if (!bfile_exists("/dev/mmcblk0p1"))
    {
        return DIAG_EMMC_NOT_FORMAT;  //emmc not formated
    }

    if (statfs(COM_SDCARD_DIR, &st_fs) == -1)
    {
        return DIAG_EMMC_UMOUNT_POINT_NOT_EXIST;    // mount ponit not exist
    }

    if (0x4d44 != st_fs.f_type)
    {
        return DIAG_EMMC_UMOUNT;    // umount
    }

    total_bytes = (unsigned long long)(st_fs.f_blocks * st_fs.f_bsize);
    free_bytes  = (unsigned long long)(st_fs.f_bfree * st_fs.f_bsize);
    avail_bytes = (unsigned long long)(st_fs.f_bavail * st_fs.f_bsize);

    if (((free_bytes >> 20) < 10) || ((avail_bytes >> 20) < 10)
        || ((total_bytes >> 20) < 10))             //if less than 10M
    {
        return DIAG_EMMC_FULL;    // full
    }

    return DIAG_EMMC_OK;       // ok
}

/****************************************************************
 function:     dev_diag_get_sim_status
 description:  get sim status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_sim_status(void)
{
    return at_get_sim_status();
}

/****************************************************************
 function:     dev_diag_get_mic_status
 description:  get mic status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int dev_diag_get_mic_status(void)
{
    return flt_get_by_id(MIC);
}

/****************************************************************
 function:     dev_diag_init
 description:  init devices manage moudule
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&dev_diag_mutex, NULL);
            pthread_mutex_init(&dev_emmc_format_mutex, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            gpio_init(GPIO_MAIN_ANT_CTRL);
            gpio_init(GPIO_SUB_ANT_CTRL);

            if (0 != dev_diag_get_ant_name())
            {
                log_e(LOG_DEV, "get 4G ant status file failed");
                return ;
            }

            /*if 4G main ant is not normal, switch to internal ant*/
            main_last_status = dev_diag_get_ant_status(ANT_4G_MAIN);

            if (1 != main_last_status)
            {
                log_e(LOG_DEV, "set 4G main ant to internal");
                gpio_set_level(GPIO_MAIN_ANT_CTRL, PINLEVEL_HIGH);
            }
            else
            {
                log_e(LOG_DEV, "set 4G main ant to external");
                gpio_set_level(GPIO_MAIN_ANT_CTRL, PINLEVEL_LOW);
            }

            /*if 4G vice ant is not normal, switch to internal ant*/
            vice_last_status = dev_diag_get_ant_status(ANT_4G_VICE);

            if (1 != vice_last_status)
            {
                log_e(LOG_DEV, "set 4G vice ant to internal");
                gpio_set_level(GPIO_SUB_ANT_CTRL, PINLEVEL_HIGH);
            }
            else
            {
                log_e(LOG_DEV, "set 4G vice ant to external");
                gpio_set_level(GPIO_SUB_ANT_CTRL, PINLEVEL_LOW);
            }

            dev_diag_start();

            break;
    }
}

/****************************************************************
 function:     dev_diag_start()
 description:  start to diag devices
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_start(void)
{
    pthread_mutex_lock(&dev_diag_mutex);
    dev_diag_available = 1;
    pthread_mutex_unlock(&dev_diag_mutex);
}

/****************************************************************
 function:     dev_diag_stop()
 description:  start to diag devices
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_stop(void)
{
    pthread_mutex_lock(&dev_diag_mutex);
    dev_diag_available = 0;
    pthread_mutex_unlock(&dev_diag_mutex);
}

/****************************************************************
 function:     dev_diag_stop()
 description:  start to diag devices
 input:        none
 output:       none
 return:       none
 *****************************************************************/
bool dev_diag_availbale(void)
{
    unsigned char avail;

    pthread_mutex_lock(&dev_diag_mutex);
    avail = dev_diag_available;
    pthread_mutex_unlock(&dev_diag_mutex);

    return avail;
}

/****************************************************************
 function:     dev_diag_emmc_set_format_flag()
 description:  set emmc format flag
 input:        unsigned char flag, 0 not format, 1 formatting
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_emmc_set_format_flag(unsigned char flag)
{
    pthread_mutex_lock(&dev_emmc_format_mutex);
    is_emmc_formatting = flag;
    pthread_mutex_unlock(&dev_emmc_format_mutex);
}

/****************************************************************
 function:     dev_diag_emmc_get_format_flag
 description:  get emmc format flag
 input:        none
 output:       none
 return:       bool  true formatting, false not formatting
 *****************************************************************/
bool dev_diag_emmc_get_format_flag(void)
{
    unsigned char flag;
    pthread_mutex_lock(&dev_emmc_format_mutex);
    flag = is_emmc_formatting;
    pthread_mutex_unlock(&dev_emmc_format_mutex);
    return flag ? true : false;
}

/****************************************************************
 function:     dev_diag_emmc
 description:  remount emmc
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_emmc(void)
{
    int ret, emmc_status;
    FILE *ptream;
    RTCTIME abstime;
    static unsigned long long err_time = 0;

    if (dev_diag_emmc_get_format_flag())
    {
        log_o(LOG_DEV, "emmc is formatting!!!!");
        return;
    }

    emmc_status = dev_diag_get_emmc_status();

    /* maybe driver not load */
    if(DIAG_EMMC_NOT_EXIST == emmc_status)  
    {
        log_e(LOG_DEV, "emmc driver is loaded failed!");
        if( 0 == err_time )
        {
            err_time = tm_get_time();
        }

        if( tm_get_time() - err_time > DIAG_EMMC_MAX_FAULT_TIME )
        {
            pm_send_evt(MPU_MID_DEV, PM_EVT_RESTART_4G_REQ);
            err_time = 0;
            log_o(LOG_DEV, "emmc is fault for a long time, reset 4G....");
            return;
        }
    }
    /* mount ponit not exist */
    else if (DIAG_EMMC_UMOUNT_POINT_NOT_EXIST == emmc_status)
    {
        err_time = 0;
        ret = dir_make_path(COM_SDCARD_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);

        if (0 != ret)
        {
            log_e(LOG_DEV, "create mount point failed, ret:%08x", ret);
            return;
        }

        log_o(LOG_DEV, "create mount point OK!!!!");
    }
    else
    {
        err_time = 0;    
    }
    
    /* umount */
    if (DIAG_EMMC_UMOUNT == dev_diag_get_emmc_status())
    {
        tm_get_abstime(&abstime);

        log_o(LOG_DEV, "mount emmc,date:%u.%u.%u %u:%u:%u",
              abstime.year, abstime.mon, abstime.mday,
              abstime.hour, abstime.min, abstime.sec);
        ptream = popen("mount /dev/mmcblk0p1 /media/sdcard/", "r");

        if (!ptream)
        {
            log_e(LOG_DEV, "mount emmc failed:%s", strerror(errno));
            return;
        }

        pclose(ptream);

        log_o(LOG_DEV, "remount eMMC OK!!!!");
    }
}

/****************************************************************
 function:     dev_diag_mcu
 description:  diagnose MCU
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_mcu(void)
{
	int           ret;
	unsigned int  ilen;
	static int    cnt = 0;
	char          status[16];
	unsigned char upg_status = DEV_UPG_IDLE;

    cnt++;
	
	if( !pm_mcu_wakeup_check() )
	{
        ilen = sizeof(upg_status);
    	st_get(ST_ITEM_UPG_STATUS, &upg_status, &ilen);
		if( DEV_UPG_BUSY == upg_status )
	    {   
	        /* wakeup mcu */
			log_o(LOG_DEV, "wankeup mcu tp upgrade");
	    	pm_send_evt( MPU_MID_DEV, PM_EVT_RING );
			return;
	    }

        /* if MCU is sleeping and not in upgrade status */
	    if( cnt > 10 )
	    {
			ret = upg_get_startup( status, sizeof(status) );

	        if (ret != 0)
	        {
	            log_e(LOG_DEV, "read startup shm failed, ret:%08x", ret);
	        }
	        else
	        {
	            if (0 != strncmp(status, "OK", strlen("OK")))
	            {
	                log_o(LOG_DEV, "begin write startup shm");
			        upg_set_startup("OK", strlen("OK") + 1);
			        log_o(LOG_DEV, "end write startup shm");
	            }
	        }

			cnt = 0;
		}
	}
}

/****************************************************************
 function:     dev_diag_4G_ant
 description:  switch 4G ant when ant status is changed
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void dev_diag_4G_ant(void)
{
    unsigned char status = 0;

    /*if main ant status is changed, switch ant*/
    status = dev_diag_get_ant_status(ANT_4G_MAIN);

    if (status != main_last_status)
    {
        main_last_status = status;

        if (1 != status)
        {
            log_e(LOG_DEV, "change 4G main ant to internal");
            gpio_set_level(GPIO_MAIN_ANT_CTRL, PINLEVEL_HIGH);
        }
        else
        {
            log_e(LOG_DEV, "change 4G main ant to external");
            gpio_set_level(GPIO_MAIN_ANT_CTRL, PINLEVEL_LOW);
        }
    }

    /*if vice ant status is changed, switch ant*/
    status = dev_diag_get_ant_status(ANT_4G_VICE);

    if (status != vice_last_status)
    {
        vice_last_status = status;

        if (1 != status)
        {
            log_e(LOG_DEV, "change 4G vice ant to internal");
            gpio_set_level(GPIO_SUB_ANT_CTRL, PINLEVEL_HIGH);
        }
        else
        {
            log_e(LOG_DEV, "change 4G vice ant to external");
            gpio_set_level(GPIO_SUB_ANT_CTRL, PINLEVEL_LOW);
        }
    }
}

