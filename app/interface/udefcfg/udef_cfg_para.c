#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "tbox_limit.h"
#include "log.h"
#include "udef_cfg_api.h"
#include "com_app_def.h"
#include "udef_cfg.h"
#include "udef_para_def.h"
#include "rds.h"
#include "../../base/dev/dev_mcu_cfg.h"
#include "cfg_api.h"
#include "hozon_PP_api.h"
#include "dir.h"
#include "file.h"
#include "dev_api.h"
#include "ble.h"
#include "at.h"
#define USER_CFG_PARA_BUF_LEN     5*1024

typedef enum 
{
    udef_cfg_user_data          = 0,
} udef_cfg_foton_item;

static pthread_mutex_t clbt_cfg_para_mutex;
static pthread_mutex_t clbt_cfg_set_mutex;

static unsigned char clbt_cfg_para_buf[USER_CFG_PARA_BUF_LEN];

#define CFG_PARA_CLBT_LOCK()        pthread_mutex_lock(&clbt_cfg_para_mutex)  
#define CFG_PARA_CLBT_UNLOCK()      pthread_mutex_unlock(&clbt_cfg_para_mutex)

#define CFG_SET_CLBT_LOCK()         pthread_mutex_lock(&clbt_cfg_set_mutex)
#define CFG_SET_CLBT_UNLOCK()       pthread_mutex_unlock(&clbt_cfg_set_mutex)


/*********************************************
function:     user_cfg_para_check
description:  init para
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int user_cfg_para_check(CFG_DATA_TYPE type,  unsigned int len)
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
            break;
    }

    return 0;
}


int clbt_cfg_init_para(viod)
{
    int i, j;
    int offset = 0, size = 0;
    int ret;
    for (i = 0; i < sizeof(clbt_cfg_table) / sizeof(CFG_ITEM_INFO); i++)
    {
        for (j = 0; j < CFG_PARA_MAX_MEMBER_CNT; j++)
        {
            if (clbt_cfg_table[i].member[j].type != CFG_DATA_INVALID)
            {
                /* check each field in one cfg item  */
                ret = user_cfg_para_check(clbt_cfg_table[i].member[j].type, clbt_cfg_table[i].member[j].len);

                if (ret != 0)
                {
                    log_e(LOG_CFG, "user_cfg_para_check cfg item(%u) member(%u) failed, type:%u, len:%u",
                          i, j, clbt_cfg_table[i].member[j].type, clbt_cfg_table[i].member[j].len);
                    return CFG_CHECK_CFG_TABLE_FAILED;
                }

                size = size + clbt_cfg_table[i].member[j].len;
            }
            else
            {
                break;
            }
        }

        /* compute each para length and offset */
        clbt_cfg_table[i].offset = offset;
        clbt_cfg_table[i].len    = size;  /* cfg item length */
        offset = offset + size;
        size = 0;
    }

    if ((clbt_cfg_table[i - 1].offset +  clbt_cfg_table[i - 1].len) > USER_CFG_PARA_BUF_LEN)
    {
        log_e(LOG_CFG, "user cfg para buf overflow, total len:%u",
              clbt_cfg_table[i - 1].offset +  clbt_cfg_table[i - 1].len);
        return CFG_PARA_OVERFLOW;
    }
    
    return 0;
}


int udef_cfg_init_para(void)
{
    int ret = 0;
    memset(clbt_cfg_para_buf, 0 , sizeof(clbt_cfg_para_buf));

    pthread_mutex_init(&clbt_cfg_para_mutex,NULL);
    pthread_mutex_init(&clbt_cfg_set_mutex,NULL);

    ret |= clbt_cfg_init_para();

    return ret;
}


int cfg_get_user_para(USER_CFG_PARA_ITEM_ID id, void *data, unsigned int *len)
{
    if ((id >= USER_CFG_ITEM_ID_MAX) || (NULL == data) || (NULL == len))
    {
        log_e(LOG_CFG, "invalid parameter");
        return CFG_INVALID_PARAMETER;
    }

    if (*len > clbt_cfg_table[id].len)
    {
        log_e(LOG_CFG, "invalid length:%u,%u,id=%u", *len, clbt_cfg_table[id].len, id);
        return CFG_INVALID_PARAMETER;
    }

    //*len = clbt_cfg_table[id].len;

    CFG_PARA_CLBT_LOCK();
    memcpy(data, clbt_cfg_para_buf + clbt_cfg_table[id].offset, *len);
    CFG_PARA_CLBT_UNLOCK();

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
int cfg_set_clbt_para_im(USER_CFG_PARA_ITEM_ID id, unsigned char *data, unsigned int len)
{
    if ((id >= USER_CFG_ITEM_ID_MAX) || (NULL == data))
    {
        log_e(LOG_CFG, "invalid parameter");
        return CFG_INVALID_PARAMETER;
    }

    if (len > clbt_cfg_table[id].len)
    {
        log_e(LOG_CFG, "invalid length,id:%u,%u,%u", id, len, clbt_cfg_table[id].len);
        return CFG_INVALID_PARAMETER;
    }
    
    CFG_PARA_CLBT_LOCK();
    memcpy(clbt_cfg_para_buf + clbt_cfg_table[id].offset, data, len);
    CFG_PARA_CLBT_UNLOCK();
    
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
int udef_cfg_save_para(udef_cfg_foton_item item)
{
    int ret = 0;
    switch (item)
    {
        case udef_cfg_user_data:
        {
            CFG_PARA_CLBT_LOCK();
            ret = rds_update_once(RDS_USER_CFG, (unsigned char *)&clbt_cfg_para_buf, sizeof(clbt_cfg_para_buf));
            
            if(1 == dev_diag_get_emmc_status())//emmc挂载成功
            {
                if (dir_exists("/media/sdcard/usrdata/bkup/") == 0 &&
                dir_make_path("/media/sdcard/usrdata/bkup/", S_IRUSR | S_IWUSR, false) != 0)
                {
                    log_e(LOG_CFG, "creat path:/media/sdcard/usrdata/bkup/ fail");
                    //return;
                }
                else
                {
                    file_copy(PP_USER_CFG_PATH,PP_USER_CFG_BKUP_PATH);//备份配置文件
                }
            }

            CFG_PARA_CLBT_UNLOCK();
        }
        break;
        default:
        break;
    }
    
    return ret;
}


int clbt_cfg_set_by_id(USER_CFG_PARA_ITEM_ID id, void *data, unsigned int len, unsigned char silent)
{
    int ret;
    //unsigned int paralen;
    //static unsigned char oldpara[CFG_MAX_PARA_LEN];

    if (id >= USER_CFG_ITEM_ID_MAX)
    {
        log_e(LOG_CFG, "invalid id, id:%u", id);
        return CFG_INVALID_PARAMETER;
    }

    CFG_SET_CLBT_LOCK();
    
    /* update configuration */
#if 0
    memset(oldpara, 0, sizeof(oldpara));
    paralen = CFG_MAX_PARA_LEN;
    ret = cfg_get_user_para(id, oldpara, &paralen);

    if ((ret != 0) || (paralen != len))
    {
        CFG_SET_CLBT_UNLOCK();
        log_e(LOG_CFG, "req len is invalid, reqlen:%u, paralen:%u", len, paralen);
        return CFG_INVALID_PARAMETER;
    }
#endif
    ret = cfg_set_clbt_para_im(id, data, len);

    if (ret != 0)
    {
        CFG_SET_CLBT_UNLOCK();
        log_e(LOG_CFG, "cfg_set_para_im failed, id:0x%04x", id);
        return CFG_TABLE_UPDATE_PARA_FAILED;
    }

    if (!silent)
    {
        /* save it into flash or eMMC */
        udef_cfg_save_para(udef_cfg_user_data);
    }

    CFG_SET_CLBT_UNLOCK();

    return 0;
}



int cfg_set_user_para(USER_CFG_PARA_ITEM_ID id, void *data, unsigned int len)
{
    return clbt_cfg_set_by_id(id, data, len, CFG_SET_UNSILENT);
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

int clbt_cfg_dump_para(void )
{
    int i, j, k;
    unsigned char *para, *array_para;
    
    shellprintf(" -----------------------------user data------------------------------\r\n");
    for (i = 0; i < sizeof(clbt_cfg_table)/ sizeof(CFG_ITEM_INFO); i++)
    {
        /* dump cfg item id with index i */
        para = clbt_cfg_para_buf + clbt_cfg_table[i].offset;
        
        for (j = 0; j < CFG_PARA_MAX_MEMBER_CNT; j++)
        {
            if (CFG_DATA_INVALID == clbt_cfg_table[i].member[j].type)
            {
                break;
            }

            /* dump all para */
            switch (clbt_cfg_table[i].member[j].type)
            {
                case CFG_DATA_UCHAR:
                    shellprintf(" %-32s : %u\r\n", clbt_cfg_table[i].member[j].name, *para);
                    break;

                case CFG_DATA_USHORT:
                    shellprintf(" %-32s : %u\r\n", clbt_cfg_table[i].member[j].name, *((unsigned short *)para));
                    break;

                case CFG_DATA_UINT:
                    shellprintf(" %-32s : %u\r\n", clbt_cfg_table[i].member[j].name, *((unsigned int *)para));
                    break;

                case CFG_DATA_STRING:
					if((strcmp("BCM_TBOX_ID_KEY", clbt_cfg_table[i].member[j].name) == 0) ||
						(strcmp("FTTSP TUKEY", clbt_cfg_table[i].member[j].name) == 0))
					{
						shellprintf(" %-32s :", clbt_cfg_table[i].member[j].name);
						array_para = para; 
						for(k = 0; k < clbt_cfg_table[i].member[j].len; k++)
						{	
							shellprintf(" %02X",*(char *)array_para);
							array_para++;
						}
						shellprintf("\r\n");					
					}else
					{
						 shellprintf(" %-32s : %.*s\r\n",
                                clbt_cfg_table[i].member[j].name, clbt_cfg_table[i].member[j].len, (char *)para);
					}  
                    break;

                case CFG_DATA_DOUBLE:
                    shellprintf(" %-32s : %f\r\n", clbt_cfg_table[i].member[j].name, *((double *)para));
                    break;

                default:
                    shellprintf(" unknow type\r\n");
                    shellprintf(" --------------------------------data over--------------------------\r\n");
                    return  CFG_INVALID_DATA;
                    break;
            }

            para = para + clbt_cfg_table[i].member[j].len;
        }
    }
    

    char plbuff[256] = {0};
    char *bufftp;
    int port;
    port = 0;
    memset(plbuff, 0, sizeof(plbuff));
    getPP_rmtCfg_certAddrPort(plbuff,&port);
    shellprintf(" %-32s : %.*s\r\n","sg addr", strlen(plbuff), (char *)plbuff);
    shellprintf(" %-32s : %u\r\n", "sg port", port);

    port = 0;
    memset(plbuff, 0, sizeof(plbuff));
    getPP_rmtCfg_tspAddrPort(plbuff,&port);
    shellprintf(" %-32s : %.*s\r\n","bdl addr", strlen(plbuff), (char *)plbuff);
    shellprintf(" %-32s : %u\r\n", "bdl port", port);

    memset(plbuff, 0, sizeof(plbuff));
    PP_rmtCfg_getIccid((uint8_t*)plbuff);
    shellprintf(" %-32s : %.*s\r\n","iccid", 21, (char *)plbuff);
    memset(plbuff, 0, sizeof(plbuff));
    shellprintf(" %-32s : %.*s\r\n","simno", 12, (char *)plbuff);

    memset(plbuff, 0, sizeof(plbuff));
    at_get_imei((char *)plbuff);
    shellprintf(" %-32s : %.*s\r\n","imei", 16, (char *)plbuff);

    memset(plbuff, 0, sizeof(plbuff));
    at_get_imsi((char *)plbuff);
    shellprintf(" %-32s : %.*s\r\n","imsi", 16, (char *)plbuff);

    shellprintf(" %-32s : %.*s\r\n","tboxmodel", 4, (char *)"EP30");

    memset(plbuff, 0, sizeof(plbuff));
    bufftp = plbuff;
    BleGetMac((uint8_t*)plbuff);
    shellprintf(" %-32s :","bluetooth mac");
    for(k = 0; k < 6; k++)
    {	
        shellprintf("%x",*bufftp);
        bufftp++;
    }
    shellprintf("\r\n");

    char cipherbuff[1024] = {0};
    bufftp = cipherbuff;
    int len = 0;
    PP_CertDL_getCipher(cipherbuff,&len);
    shellprintf(" %-32s :","cipher");
    for(k = 0; k < len; k++)
    {	
        shellprintf("%u",*bufftp);
        bufftp++;
    }
    shellprintf("\r\n");

    shellprintf(" ------------------------------data over------------------------------\r\n");
    sleep(1);
    return 0;
}

int udef_cfg_para_back(int argc,const char **argv)
{
    return 0;
}

int udef_cfg_dump_para(int argc, const char **argv)
{
    clbt_cfg_dump_para();
    return 0;
}

int udef_cfg_para_restor(int argc , const char **argv)
{
    
    return 0;
}

int clbt_cfg_set_default_para(CFG_SET_TYPE type)
{
    short tmp_short;
    int   tmp_int;
    char gbvin[18];
    memset(gbvin, 0, sizeof(gbvin));
    strcpy((char *) gbvin, "00000000000000000");
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_VIN, gbvin, 18, type);

    char gb_url[256];
    memset(gb_url, 0, sizeof(gb_url));
    strcpy((char *) gb_url, "60.12.185.130");
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_URL, gb_url, sizeof(gb_url), type);

    tmp_short = 20000;
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_PORT, &tmp_short, sizeof(short), type);

    tmp_short = 0;
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_REGINTV, &tmp_short, sizeof(short), type);
    tmp_short = 10;
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_INTERVAL, &tmp_short, sizeof(short), type);
    tmp_short = 5;
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_TIMEOUT, &tmp_short, sizeof(short), type);
    tmp_int = 0;
    clbt_cfg_set_by_id(CFG_ITEM_GB32960_REGSEQ, &tmp_int, sizeof(int), type);

    clbt_cfg_set_by_id(CFG_ITEM_HOZON_TSP_TBOXID, &tmp_int, sizeof(int), type);

    char mxuSw[11];
    memset(mxuSw, 0, sizeof(mxuSw));
    strcpy((char *) mxuSw, "0");
    clbt_cfg_set_by_id(CFG_ITEM_HOZON_TSP_MCUSW, mxuSw, sizeof(mxuSw), type);
    clbt_cfg_set_by_id(CFG_ITEM_HOZON_TSP_MPUSW, mxuSw, sizeof(mxuSw), type);
    
    return 0;
}


int clbt_cfg_restore_para(void)
{
    unsigned int len = 0;
    char ver[COM_APP_VER_LEN];
    int ret;

    len = sizeof(clbt_cfg_para_buf);

    CFG_PARA_CLBT_LOCK();
    ret = rds_get(RDS_USER_CFG, clbt_cfg_para_buf, &len, ver);
    CFG_PARA_CLBT_UNLOCK();

    if (ret != 0)
    {
        /* file is not exist, set default data */
        clbt_cfg_set_default_para(CFG_SET_SILENT); 
        udef_cfg_save_para(udef_cfg_user_data);
    }
    else
    {
        log_o(LOG_CFG, "cfg data ver:%s", ver);
    }
    return 0;
}

/****************************************************************
function:     cfg_init
description:  initiaze configuration manager module main function
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int udef_cfg_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            udef_cfg_init_para();
            break;

        case INIT_PHASE_RESTORE:

            if((access(PP_USER_CFG_PATH,F_OK)) != 0)//文件不存在
            {
                if((access(PP_USER_CFG_BKUP_PATH,F_OK)) == 0)//文件存在
                {
                    file_copy(PP_USER_CFG_BKUP_PATH,PP_USER_CFG_PATH);//还原配置文件
                }
            }

            clbt_cfg_restore_para();
            break;

        case INIT_PHASE_OUTSIDE:
            shell_cmd_register("dumpcfgu",    udef_cfg_dump_para,   "dump user define configuration");
            shell_cmd_register("ucfgbak",     udef_cfg_para_back,   "user define config parameter back");
            shell_cmd_register("ucfgrestor",  udef_cfg_para_restor, "user define config parameter restor");
            break;

        default:
            break;
    }

    return 0;
}




