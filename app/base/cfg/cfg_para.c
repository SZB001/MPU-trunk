/****************************************************************
file:         cfg_para.c
description:  the source file of parameter definition implementation
date:         2016/9/26
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "cfg.h"
#include "cfg_api.h"
#include "cfg_para_api.h"
#include "cfg_para.h"
#include "uds_did_def.h"
#include "cfg_para_def.h"
#include "dev_rw.h"
#include "tcom_api.h"
#include "rds.h"
#include "tbox_limit.h"
#include "can_api.h"
#include "pm_api.h"
#include "at_api.h"
#include "uds.h"

static unsigned char cfg_para_buf[CFG_PARA_BUF_LEN];
static pthread_mutex_t cfg_para_mutex;
static bool cfg_para_status = true;
static CFG_REG_TBL cfg_regtbl;
static pthread_mutex_t cfg_regtbl_mutex;
static pthread_mutex_t cfg_set_mutex;

/*********************************************
function:     cfg_para_check
description:  init para
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_para_check(CFG_DATA_TYPE type,  unsigned int len)
{
    switch (type)
    {
        case CFG_DATA_UINT:
            if (len != sizeof(unsigned int))
            {
                return CFG_INVALID_PARAMETER;
            }

            break;

        case CFG_DATA_USHORT:
            if (len != sizeof(unsigned short))
            {
                return CFG_INVALID_PARAMETER;
            }

            break;

        case CFG_DATA_UCHAR:
            if (len != sizeof(unsigned char))
            {
                return CFG_INVALID_PARAMETER;
            }

            break;

        case CFG_DATA_DOUBLE:
            if (len != sizeof(double))
            {
                return CFG_INVALID_PARAMETER;
            }

            break;

        case CFG_DATA_STRING:
            if (len <= sizeof(unsigned char))
            {
                return CFG_INVALID_PARAMETER;
            }

            break;

        default:
            return CFG_INVALID_PARAMETER;
    }

    return 0;
}

/*********************************************
function:     cfg_init_para
description:  init para
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_init_para(void)
{
    int i, j;
    int offset = 0, size = 0;
    int ret;

    memset(cfg_para_buf, 0, sizeof(cfg_para_buf));
    pthread_mutex_init(&cfg_para_mutex, NULL);

    cfg_regtbl.used_num = 0;
    pthread_mutex_init(&cfg_regtbl_mutex, NULL);

    pthread_mutex_init(&cfg_set_mutex, NULL);

    for (i = 0; i < sizeof(cfg_table) / sizeof(CFG_ITEM_INFO); i++)
    {
        for (j = 0; j < CFG_PARA_MAX_MEMBER_CNT; j++)
        {
            if (cfg_table[i].member[j].type != CFG_DATA_INVALID)
            {
                /* check each field in one cfg item  */
                ret = cfg_para_check(cfg_table[i].member[j].type, cfg_table[i].member[j].len);

                if (ret != 0)
                {
                    log_e(LOG_CFG, "cfg_para_check cfg item(%u) member(%u) failed, type:%u, len:%u",
                          i, j, cfg_table[i].member[j].type, cfg_table[i].member[j].len);
                    return CFG_CHECK_CFG_TABLE_FAILED;
                }

                size = size + cfg_table[i].member[j].len;
            }
            else
            {
                break;
            }
        }

        /* compute each para length and offset */
        cfg_table[i].offset = offset;
        cfg_table[i].len    = size;  /* cfg item length */
        offset = offset + size;
        size = 0;
    }

    if ((cfg_table[i - 1].offset +  cfg_table[i - 1].len) > CFG_PARA_BUF_LEN)
    {
        log_e(LOG_CFG, "cfg para buf overflow, total len:%u",
              cfg_table[i - 1].offset +  cfg_table[i - 1].len);
        return CFG_PARA_OVERFLOW;
    }

    return 0;
}

/*********************************************
function:     cfg_register
description:  register callback
input:        unsigned short id, cfg item id;
              on_changed onchanged, callback for setting;
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_register(CFG_PARA_ITEM_ID id, on_changed onchanged)

{
    /* the paramerter is invalid */
    if (NULL == onchanged)
    {
        log_e(LOG_CFG, "onchanged is NULL");
        return CFG_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&cfg_regtbl_mutex);

    if (cfg_regtbl.used_num >= CFG_MAX_REG_ITEM_NUM)
    {
        pthread_mutex_unlock(&cfg_regtbl_mutex);
        log_e(LOG_CFG, "cfg register table overflow");
        return CFG_TABLE_OVERFLOW;
    }

    cfg_regtbl.cfgtbl[cfg_regtbl.used_num].id        = id;
    cfg_regtbl.cfgtbl[cfg_regtbl.used_num].onchanged = onchanged;
    cfg_regtbl.used_num++;
    pthread_mutex_unlock(&cfg_regtbl_mutex);

    return 0;
}

/*********************************************
function:     cfg_notify_changed
description:  notify the configuration is changed
input:        CFG_PARA_ITEM_ID id, cfg itme id
              const unsigned char *old_para, old para value
              unsigned int para_len, para length
output:       none
return:       none
*********************************************/
int cfg_notify_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para, unsigned int para_len)
{
    int j, ret;
    unsigned int new_para_len;
    static unsigned char newpara[CFG_MAX_PARA_LEN];

    for (j = 0; j < cfg_regtbl.used_num; j++)
    {
        if (cfg_regtbl.cfgtbl[j].id == id)   /* find the process callback */
        {
            new_para_len = CFG_MAX_PARA_LEN;
            memset(newpara, 0, sizeof(newpara));
            ret = cfg_get_para(id, newpara, &new_para_len);

            if ((ret != 0) || (new_para_len != para_len))
            {
                log_e(LOG_CFG, "cfg_get_para failed, para_len:%u, new_para_len:%u,ret:0x%08x",
                      para_len, new_para_len, ret);
                return ret;
            }
			
			if(cfg_regtbl.cfgtbl[j].onchanged != NULL)//by liujian
			{
				/* notify the data is changed */
				ret = cfg_regtbl.cfgtbl[j].onchanged(id, old_para, newpara, para_len);

				if (ret != 0)
				{
					log_e(LOG_CFG, "onchanged failed, ret:0x%08x", ret);
					continue;
				}
			}
        }
    }

    return 0;
}

/*********************************************
function:     cfg_get_para
description:  get para value
input:        CFG_PARA_ITEM_ID id, cfg item id
              unsigned int *len, the data buf size
output:       unsigned int *len, the para length
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_get_para(CFG_PARA_ITEM_ID id, void *data, unsigned int *len)
{
    if ((id >= CFG_ITEM_ID_MAX) || (NULL == data) || (NULL == len))
    {
        log_e(LOG_CFG, "invalid parameter");
        return CFG_INVALID_PARAMETER;
    }

    if (*len < cfg_table[id].len)
    {
        log_e(LOG_CFG, "invalid length:%u,%u,id=%u", *len, cfg_table[id].len, id);
        return CFG_INVALID_PARAMETER;
    }

    *len = cfg_table[id].len;

    pthread_mutex_lock(&cfg_para_mutex);
    memcpy(data, cfg_para_buf + cfg_table[id].offset, *len);
    pthread_mutex_unlock(&cfg_para_mutex);

    return 0;
}

/*********************************************
function:     cfg_set_para_im
description:  set para value immediatly
input:        none
output:       none
return:       0 indicates success;
              -1 indicates failed
*********************************************/
int cfg_set_para_im(CFG_PARA_ITEM_ID id, unsigned char *data, unsigned int len)
{
    if ((id >= CFG_ITEM_ID_MAX) || (NULL == data))
    {
        log_e(LOG_CFG, "invalid parameter");
        return CFG_INVALID_PARAMETER;
    }

    if (len != cfg_table[id].len)
    {
        log_e(LOG_CFG, "invalid length,id:%u,%u,%u", id, len, cfg_table[id].len);
        return CFG_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&cfg_para_mutex);
    memcpy(cfg_para_buf + cfg_table[id].offset, data, len);
    pthread_mutex_unlock(&cfg_para_mutex);

    return 0;
}

/*********************************************
function:     cfg_set_by_id
description:  setting parameter from ehu or app
input:        const TCOM_MSG_HEADER *msgheader, message header
              const unsigned char *msgbody, message body
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_set_by_id(CFG_PARA_ITEM_ID id, void *data, unsigned int len, unsigned char silent)
{
    int ret;
    unsigned int paralen;
    static unsigned char oldpara[CFG_MAX_PARA_LEN];

    if (id >= CFG_ITEM_ID_MAX)
    {
        log_e(LOG_CFG, "invalid id, id:%u", id);
        return CFG_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&cfg_set_mutex);
    
    /* update configuration */
    memset(oldpara, 0, sizeof(oldpara));
    paralen = CFG_MAX_PARA_LEN;
    ret = cfg_get_para(id, oldpara, &paralen);

    if ((ret != 0) || (paralen != len))
    {
        pthread_mutex_unlock(&cfg_set_mutex);
        log_e(LOG_CFG, "req len is invalid, reqlen:%u, paralen:%u", len, paralen);
        return CFG_INVALID_PARAMETER;
    }

    ret = cfg_set_para_im(id, data, len);

    if (ret != 0)
    {
        pthread_mutex_unlock(&cfg_set_mutex);
        log_e(LOG_CFG, "cfg_set_para_im failed, id:0x%04x", id);
        return CFG_TABLE_UPDATE_PARA_FAILED;
    }

    if (!silent)
    {
        /* save it into flash or eMMC */
        cfg_save_para();

        cfg_notify_changed(id, oldpara, len);
    }

    pthread_mutex_unlock(&cfg_set_mutex);

    return 0;
}

/*********************************************
function:     cfg_set_para
description:  setting parameter from ehu or app
input:        const TCOM_MSG_HEADER *msgheader, message header
              const unsigned char *msgbody, message body
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_set_para(CFG_PARA_ITEM_ID id, void *data, unsigned int len)
{
    return cfg_set_by_id(id, data, len, CFG_SET_UNSILENT);
}

/*********************************************
function:     cfg_set_default_para
description:  get para value
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_set_default_para(CFG_SET_TYPE type)
{
    CFG_DIAL_AUTH  apn_auth;
    unsigned char dcom_enable;
    unsigned char wifi_enable;
    unsigned char net_type;//0x00:auto, 0x01:2G, 0x02 3G, 0x03 4G
    unsigned char wifi_maxassoc;
    unsigned char sleep_mode;
    unsigned char car_type;
    short tmp_short;
    int   tmp_int;
    char  tmp_char;

    tmp_short = 0;
    cfg_set_by_id(CFG_ITEM_CAN_DEFAULT_BAUD_0,  &tmp_short, sizeof(short), type);
    cfg_set_by_id(CFG_ITEM_CAN_DEFAULT_BAUD_1,  &tmp_short, sizeof(short), type);
    cfg_set_by_id(CFG_ITEM_CAN_DEFAULT_BAUD_2,  &tmp_short, sizeof(short), type);
    cfg_set_by_id(CFG_ITEM_CAN_DEFAULT_BAUD_3,  &tmp_short, sizeof(short), type);
    cfg_set_by_id(CFG_ITEM_CAN_DEFAULT_BAUD_4,  &tmp_short, sizeof(short), type);
    
//    tmp_short = 0;
//    cfg_set_by_id(CFG_ITEM_CAN2_AUTO_BAUD, &tmp_short, sizeof(short), type);
    
//    can_baud_reset();
//    can_auto_baud_rs();

    car_type = 0xff;
    cfg_set_by_id(CFG_ITEM_FT_UDS_VEHICLE_TYPE,  &car_type, 1, type);
    
    tmp_char = UDS_12V_POWER;
    cfg_set_by_id(CFG_ITEM_FT_UDS_POWER,  &tmp_char, 1, type);
    
    tmp_char = 0;
    cfg_set_by_id(CFG_ITEM_DBC_PATH, &tmp_char, 256, type);

    sleep_mode = 3;
    cfg_set_by_id(CFG_ITEM_SLEEP_MODE, &sleep_mode, sizeof(sleep_mode), type);

    /* China telecom */
    cfg_set_by_id(CFG_ITEM_WAN_APN, (unsigned char *) "ctnet", 32, type);
    
    memset(&apn_auth, 0, sizeof(apn_auth));
    cfg_set_by_id(CFG_ITEM_WAN_APN_AUTH, &apn_auth, sizeof(apn_auth), type);

    /* China telecom, no private apn */
    cfg_set_by_id(CFG_ITEM_LOCAL_APN, (unsigned char *) "", 32, type);

    memset(&apn_auth, 0, sizeof(apn_auth));
    cfg_set_by_id(CFG_ITEM_LOC_APN_AUTH, &apn_auth, sizeof(apn_auth), type);

    unsigned char dbc_path[256] = "/usrdata/dbc/GB-EP30_CAN_v1.5.dbc";
    cfg_set_by_id(CFG_ITEM_DBC_PATH, dbc_path, 256, type);

    wifi_enable = 0;
    cfg_set_by_id(CFG_ITEM_WIFI_SET, &wifi_enable, sizeof(wifi_enable), type);

    dcom_enable = 1;
    cfg_set_by_id(CFG_ITEM_DCOM_SET, &dcom_enable, sizeof(dcom_enable), type);

    /* auto */
    net_type = 0;
    cfg_set_by_id(CFG_ITEM_NET_TYPE, &net_type, sizeof(net_type), type);

    wifi_maxassoc = 8;
    cfg_set_by_id(CFG_ITEM_WIFI_MAXASSOC, &wifi_maxassoc, sizeof(wifi_maxassoc), type);

    cfg_set_by_id(CFG_ITEM_WIFI_SSID, (unsigned char *)DEFAULT_SSID, 32, type);
    cfg_set_by_id(CFG_ITEM_WIFI_KEY, (unsigned char *)DEFAULT_PASSWORD, 32, type);

    /*set icall bcall and white list*/
    cfg_set_by_id(CFG_ITEM_ICALL, (unsigned char *)"95190738", 32, type);
    cfg_set_by_id(CFG_ITEM_BCALL, (unsigned char *)"95190737", 32, type);
    cfg_set_by_id(CFG_ITEM_WHITE_LIST, (unsigned char *) "95190737;95190738;01080287000;", 512, type);

    unsigned char auth[256];
    memset(auth, 0, sizeof(auth));
    strcpy((char *) auth, "intestadmin");
    cfg_set_by_id(CFG_ITEM_DSU_AUTHKEY, (unsigned char *) &auth, sizeof(auth), type);

    unsigned short canlog_time = 30;
    cfg_set_by_id(CFG_ITEM_DSU_CANLOG_TIME, (unsigned char *) &canlog_time, sizeof(canlog_time), type);

    unsigned char canlog_mode = 0; // record iwd file by default
    cfg_set_by_id(CFG_ITEM_DSU_CANLOG_MODE, (unsigned char *) &canlog_mode, sizeof(canlog_mode), type);

    unsigned char sdhz = 1;
    cfg_set_by_id(CFG_ITEM_DSU_SDHZ, (unsigned char *) &sdhz, sizeof(sdhz) , type);

    unsigned char hourfile = 3;
    cfg_set_by_id(CFG_ITEM_DSU_HOURFILE, (unsigned char *) &hourfile, sizeof(hourfile), type);

    unsigned char loopfile = 1;
    cfg_set_by_id(CFG_ITEM_DSU_LOOPFILE, (unsigned char *) &loopfile, sizeof(loopfile), type);

    char gbvin[18];
    memset(gbvin, 0, sizeof(gbvin));
    strcpy((char *) gbvin, "00000000000000000");
    cfg_set_by_id(CFG_ITEM_GB32960_VIN, gbvin, 18, type);

    char gb_url[256];
    memset(gb_url, 0, sizeof(gb_url));
    strcpy((char *) gb_url, "60.12.185.130");
    cfg_set_by_id(CFG_ITEM_GB32960_URL, gb_url, sizeof(gb_url), type);

    tmp_short = 20000;
    cfg_set_by_id(CFG_ITEM_GB32960_PORT, &tmp_short, sizeof(short), type);

    tmp_short = 0;
    cfg_set_by_id(CFG_ITEM_GB32960_REGINTV, &tmp_short, sizeof(short), type);
    tmp_short = 10;
    cfg_set_by_id(CFG_ITEM_GB32960_INTERVAL, &tmp_short, sizeof(short), type);
    tmp_short = 5;
    cfg_set_by_id(CFG_ITEM_GB32960_TIMEOUT, &tmp_short, sizeof(short), type);
    tmp_int = 0;
    cfg_set_by_id(CFG_ITEM_GB32960_REGSEQ, &tmp_int, sizeof(int), type);

    cfg_set_by_id(CFG_ITEM_HOZON_TSP_TBOXID, &tmp_int, sizeof(int), type);

    char mxuSw[11];
    memset(mxuSw, 0, sizeof(mxuSw));
    strcpy((char *) mxuSw, "0");
    cfg_set_by_id(CFG_ITEM_HOZON_TSP_MCUSW, mxuSw, sizeof(mxuSw), type);
    cfg_set_by_id(CFG_ITEM_HOZON_TSP_MPUSW, mxuSw, sizeof(mxuSw), type);

#if 0
    cfg_set_by_id(CFG_ITEM_FOTON_VIN, "00000000000000000", 18, type);
    
    cfg_set_by_id(CFG_ITEM_FOTON_URL, "211.94.119.48", 256, type);

    tmp_short = 39091;
    cfg_set_by_id(CFG_ITEM_FOTON_PORT, &tmp_short, sizeof(short), type);

    tmp_short = 30;
    cfg_set_by_id(CFG_ITEM_FOTON_REGINTV, &tmp_short, sizeof(short), type);

    tmp_short = 10;
    cfg_set_by_id(CFG_ITEM_FT_HBINTV, &tmp_short, sizeof(short), type);

    tmp_short = 10;
    cfg_set_by_id(CFG_ITEM_FOTON_INTERVAL, &tmp_short, sizeof(short), type);

    tmp_short = 5;
    cfg_set_by_id(CFG_ITEM_FOTON_TIMEOUT, &tmp_short, sizeof(short), type);

	tmp_short = 15;
    cfg_set_by_id(CFG_ITEM_FT_HBINTV, &tmp_short, sizeof(short), type);
#endif
    unsigned int sleep_time = 7 * 24 * 60;
    cfg_set_by_id(CFG_ITEM_SLEEP_TIME, &sleep_time, sizeof(sleep_time), type);
    unsigned int dsleep_time = 14 * 24 * 60;
    cfg_set_by_id(CFG_ITEM_DSLEEP_TIME, &dsleep_time, sizeof(dsleep_time), type);

    unsigned char vtd_enable = 0;
    cfg_set_by_id(CFG_ITEM_VTD, &vtd_enable, sizeof(vtd_enable), type);

    unsigned char icall_enable = 0;
    cfg_set_by_id(CFG_ITEM_ICALL_F, &icall_enable, sizeof(icall_enable), type);

    unsigned char bcall_enable = 0;
    cfg_set_by_id(CFG_ITEM_BCALL_F, &bcall_enable, sizeof(bcall_enable), type);

    unsigned char ecall_enable = 0;
    cfg_set_by_id(CFG_ITEM_ECALL_F, &ecall_enable, sizeof(ecall_enable), type);

    unsigned char log_enable = 0;
    cfg_set_by_id(CFG_ITEM_LOG_ENABLE, &log_enable, sizeof(log_enable), type);

    unsigned int wakeup_time = 0;
    cfg_set_by_id(CFG_ITEM_RTC_WAKEUP_TIME, &wakeup_time, sizeof(wakeup_time), type);

    unsigned char bat_type = 2;
    cfg_set_by_id(CFG_ITEM_BAT_TYPE, &bat_type, sizeof(bat_type), type);

    char ft_shop_code[10];
    memset(ft_shop_code, 0, sizeof(ft_shop_code));
    cfg_set_by_id(CFG_ITEM_FT_UDS_REPAIR_SHOP_CODE, ft_shop_code, sizeof(ft_shop_code), type);

    char ft_avn_sn[16];
    memset(ft_avn_sn, 0, sizeof(ft_avn_sn));
    cfg_set_by_id(CFG_ITEM_FT_UDS_AVN_SERIAL_NUMBER, ft_avn_sn, sizeof(ft_avn_sn), type);

    unsigned char fuelcell = 0;
    cfg_set_by_id(CFG_ITEM_FUELCELL, &fuelcell, sizeof(fuelcell), type);

	char ft_part_num[14];
    memset(ft_part_num, 0, sizeof(ft_part_num));
	strcpy(ft_part_num, HIGH_MODEL_CODE);
    cfg_set_by_id(CFG_ITEM_PART_NUM, ft_part_num, sizeof(ft_part_num), type);

    char ft_hw_ver[14];
    memset(ft_hw_ver, 0, sizeof(ft_hw_ver));
    strcpy(ft_hw_ver, HW_VERSION);
    cfg_set_by_id(CFG_ITEM_FT_UDS_HW, ft_hw_ver, sizeof(ft_hw_ver), type);

	char model_num[10];
	memset(model_num, 0, sizeof(model_num));
	strcpy(model_num, MODEL_NUM);
	cfg_set_by_id(CFG_ITEM_FT_DEV_TYPE, model_num, sizeof(model_num), type);

	unsigned char brand = 0;
	cfg_set_by_id(CFG_ITEM_FT_PORT, &brand, sizeof(brand), type);

	unsigned char status = 0;
	cfg_set_by_id(CFG_ITEM_FT_REGISTER, &status, sizeof(status), type);
	
  
  //  printf("wang wang wang\r\n");
    return 0;
}

/*********************************************
function:    cfg_upgrade_new_para
description: if you want to set default value,
             you must add here when make a new version
input:       none
output:      none
return:      0 indicates success;
             others indicates failed
*********************************************/
int cfg_upgrade_new_para(void)
{
    char ft_tukey[17] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00};
    cfg_set_by_id(CFG_ITEM_FTTSP_TUKEY, ft_tukey, sizeof(ft_tukey), 1);
    return 0;
}

/*********************************************
function:     cfg_save_para
description:  save para value
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_save_para(void)
{
    int ret;

    pthread_mutex_lock(&cfg_para_mutex);
    ret = rds_update_once(RDS_SYS_CFG, (unsigned char *)&cfg_para_buf, sizeof(cfg_para_buf));
    pthread_mutex_unlock(&cfg_para_mutex);

    return ret;
}

/*********************************************
function:     cfg_restore_para
description:  restore para value
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_restore_para(void)
{
    unsigned int len = 0;
    char ver[COM_APP_VER_LEN];
    int ret;

    len = sizeof(cfg_para_buf);
    pthread_mutex_lock(&cfg_para_mutex);
    ret = rds_get(RDS_SYS_CFG, cfg_para_buf, &len, ver);
    pthread_mutex_unlock(&cfg_para_mutex);

    if (ret != 0)
    {
        /* file is not exist, set default data */
        cfg_set_default_para(CFG_SET_SILENT);
        cfg_save_para();
        cfg_para_status = false;
    }
    else
    {
        log_o(LOG_CFG, "cfg data ver:%s", ver);
    }

    /* if add new para in the new version, upgrade the para */
    if (0 != strcmp(COM_APP_MAIN_VER, ver))
    {
        log_e(LOG_CFG, "cfg ver:%s", ver);
        cfg_upgrade_new_para();
    }

    return 0;
}

/*********************************************
function:     cfg_get_para_status
description:  get para restore status
input:        none
output:       none
return:       false indicates abnormal;
              true indicates normal;
*********************************************/
bool cfg_get_para_status(void)
{
    return cfg_para_status;
}

/************************************************************
function:     cfg_dump_para
description:  dump all para
input:        unsigned int argc, para count;
              unsigned char **argv, para value
output:       none
return:       0 indicates success;
              others indicates failed;
***********************************************************/
int cfg_dump_para(int argc, const char **argv)
{
    int i, j;
    unsigned char *para;

    for (i = 0; i < sizeof(cfg_table) / sizeof(CFG_ITEM_INFO); i++)
    {
        /* dump cfg item id with index i */
        para = cfg_para_buf + cfg_table[i].offset;

        for (j = 0; j < CFG_PARA_MAX_MEMBER_CNT; j++)
        {
            if (CFG_DATA_INVALID == cfg_table[i].member[j].type)
            {
                break;
            }

            /* dump all para */
            switch (cfg_table[i].member[j].type)
            {
                case CFG_DATA_UCHAR:
                    shellprintf(" %-32s : %u\r\n", cfg_table[i].member[j].name, *para);
                    break;

                case CFG_DATA_USHORT:
                    shellprintf(" %-32s : %u\r\n", cfg_table[i].member[j].name, *((unsigned short *)para));
                    break;

                case CFG_DATA_UINT:
                    shellprintf(" %-32s : %u\r\n", cfg_table[i].member[j].name, *((unsigned int *)para));
                    break;

                case CFG_DATA_STRING:
                    shellprintf(" %-32s : %.*s\r\n",
                                cfg_table[i].member[j].name, cfg_table[i].member[j].len, (char *)para);                             
                    break;

                case CFG_DATA_DOUBLE:
                    shellprintf(" %-32s : %f\r\n", cfg_table[i].member[j].name, *((double *)para));
                    break;

                default:
                    shellprintf(" unknow type\r\n");
                    shellprintf(" --------------------------------dataover--------------------------------\r\n");
                    return  CFG_INVALID_DATA;
                    break;
            }

            para = para + cfg_table[i].member[j].len;
        }
    }

    shellprintf(" --------------------------------dataover--------------------------------\r\n");
    return 0;
}

/*********************************************
function:     cfg_set_default
description:  get para value
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int cfg_set_default(int argc, const char **argv)
{
//    short tmp_short = 0;
//    cfg_set_by_id(CFG_ITEM_CAN2_AUTO_BAUD, &tmp_short, sizeof(short), CFG_SET_SILENT);
    
//   can_auto_baud_rs();
//    can_baud_reset();

    rds_set_default();

    shell_cmd_exec("mcudbg clrbrt", NULL, 0);

    sleep(2);

    #if 0
    pm_send_evt(MPU_MID_CFG, PM_EVT_RESTART_APP_REQ);
	#endif

	log_o(LOG_CFG, "-----------------kill app and restart it-----------------");
    system("pkill -9 tbox_app.bin");

    shellprintf(" set default OK\r\n");

    return 0;
}

