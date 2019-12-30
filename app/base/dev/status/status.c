/****************************************************************
 file:         status.c
 description:  the source file of status item definition implementation
 date:         2016/9/26
 author        liuzhongwen
 ****************************************************************/
#include "com_app_def.h"
#include "dev_api.h"
#include "pm_api.h"
#include "status_def.h"
#include "status.h"
#include "status_sync.h"
#include "tbox_limit.h"
#include "hozon_PP_api.h"

static unsigned char st_item_buf[ST_ITEM_BUF_LEN];
static pthread_mutex_t st_item_mutex;
static ST_REG_TBL st_regtbl;
static pthread_mutex_t st_regtbl_mutex;
static pthread_mutex_t st_set_mutex;

/***************************************************************************
 function:     st_para_check
 description:  init para
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************************/
int st_para_check(ST_DATA_TYPE type, unsigned int len)
{
    switch (type)
    {
        case ST_DATA_UINT:
            if (len != sizeof(unsigned int))
            {
                return DEV_INVALID_PARA;
            }

            break;

        case ST_DATA_INT:
            if (len != sizeof(int))
            {
                return DEV_INVALID_PARA;
            }

            break;

        case ST_DATA_USHORT:
            if (len != sizeof(unsigned short))
            {
                return DEV_INVALID_PARA;
            }

            break;

        case ST_DATA_UCHAR:
            if (len != sizeof(unsigned char))
            {
                return DEV_INVALID_PARA;
            }

            break;

        case ST_DATA_DOUBLE:
            if (len != sizeof(double))
            {
                return DEV_INVALID_PARA;
            }

            break;

        case ST_DATA_STRING:
            if (len <= sizeof(unsigned char))
            {
                return DEV_INVALID_PARA;
            }

            break;

        default:
            return DEV_INVALID_PARA;
    }

    return 0;
}

/***************************************************************************
 function:     st_init
 description:  init status
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************************/
int st_init(void)
{
    int i, j;
    int offset = 0, size = 0;
    int ret;

    memset(st_item_buf, 0, sizeof(st_item_buf));
    pthread_mutex_init(&st_item_mutex, NULL);

    st_regtbl.used_num = 0;
    pthread_mutex_init(&st_regtbl_mutex, NULL);

    pthread_mutex_init(&st_set_mutex, NULL);

    for (i = 0; i < sizeof(st_table) / sizeof(ST_ITEM_INFO); i++)
    {
        for (j = 0; j < ST_ITEM_MAX_MEMBER_CNT; j++)
        {
            if (st_table[i].member[j].type != ST_DATA_INVALID)
            {
                /* check each field in one status item  */
                ret = st_para_check(st_table[i].member[j].type, st_table[i].member[j].len);

                if (ret != 0)
                {
                    log_e(LOG_DEV, "st_para_check cfg item(%u) member(%u) failed, type:%u, len:%u",
                          i, j, st_table[i].member[j].type, st_table[i].member[j].len);
                    return DEV_CHECK_ITEM_FAILED;
                }

                size = size + st_table[i].member[j].len;
            }
            else
            {
                break;
            }
        }

        /* compute each para length and offset */
        st_table[i].offset = offset;
        st_table[i].len = size; /* status item length */
        offset = offset + size;
        size = 0;
    }

    if ((st_table[i - 1].offset + st_table[i - 1].len) > ST_ITEM_BUF_LEN)
    {
        log_e(LOG_DEV, "status para buf overflow, total len:%u",
              st_table[i - 1].offset + st_table[i - 1].len);
        return DEV_PARA_OVERFLOW;
    }

    return 0;
}

/*****************************************************************************
 function:     st_register
 description:  register callback
 input:        unsigned short id, status item id;
 st_on_changed onchanged, callback for setting;
 output:       none
 return:       0 indicates success;
 others indicates failed
 *******************************************************************************/
int st_register(ST_DEF_ITEM_ID id, st_on_changed onchanged)
{
    /* the paramerter is invalid */
    if (NULL == onchanged)
    {
        log_e(LOG_DEV, "onchanged is NULL");
        return DEV_INVALID_PARA;
    }

    pthread_mutex_lock(&st_regtbl_mutex);

    if (st_regtbl.used_num >= ST_MAX_REG_ITEM_NUM)
    {
        pthread_mutex_unlock(&st_regtbl_mutex);
        log_e(LOG_DEV, "st register table overflow");
        return DEV_ST_TABLE_OVERFLOW;
    }

    st_regtbl.sttbl[st_regtbl.used_num].id = id;
    st_regtbl.sttbl[st_regtbl.used_num].onchanged = onchanged;
    st_regtbl.used_num++;
    pthread_mutex_unlock(&st_regtbl_mutex);

    return 0;
}

/*****************************************************************************
 function:     st_notify_changed
 description:  notify the configuration is changed
 input:        ST_ITEM_ITEM_ID id, cfg itme id
 const unsigned char *old_para, old para value
 unsigned int para_len, para length
 output:       none
 return:       none
 ******************************************************************************/
int st_notify_changed(ST_DEF_ITEM_ID id, unsigned char *old_para, unsigned int para_len)
{
    int j, ret;
    unsigned int new_para_len;
    unsigned char newpara[ST_MAX_STATUS_LEN];

    for (j = 0; j < st_regtbl.used_num; j++)
    {
        if (st_regtbl.sttbl[j].id == id)  /* find the process callback */
        {
            new_para_len = ST_MAX_STATUS_LEN;
            memset(newpara, 0, sizeof(newpara));
            ret = st_get(id, newpara, &new_para_len);

            if ((ret != 0) || (new_para_len != para_len))
            {
                log_e(LOG_DEV, "st_get failed, para_len:%u, new_para_len:%u,ret:0x%08x",
                      para_len, new_para_len, ret);
                return ret;
            }

            /* notify the registrants that the status is changed */
            ret = st_regtbl.sttbl[j].onchanged(id, old_para, newpara, para_len);

            if (ret != 0)
            {
                log_e(LOG_DEV, "onchanged failed, ret:0x%08x", ret);
                continue;
            }
        }
    }

    return 0;
}

/************************************************************************
 function:     st_get_para
 description:  get para value
 input:        ST_ITEM_ITEM_ID id, status item id
 unsigned int *len, the data buf size
 output:       unsigned int *len, the para length
 return:       0 indicates success;
 others indicates failed,
 DEV_ST_UNKNOWN indicates the status is unknown
 *************************************************************************/
int st_get(ST_DEF_ITEM_ID id, unsigned char *data, unsigned int *len)
{
    if ((id >= ST_ITEM_ID_MAX) || (NULL == data) || (NULL == len))
    {
        log_e(LOG_DEV, "invalid parameter");
        return DEV_INVALID_PARA;
    }

    if (*len < st_table[id].len)
    {
        log_e(LOG_DEV, "invalid length:%u,%u,id:%u", *len, st_table[id].len, id);
        return DEV_INVALID_PARA;
    }

    /* if the status is invalid, return DEV_ST_UNKNOWN */
    if (!st_table[id].is_valid)
    {
        *len = st_table[id].len;
        log_i(LOG_DEV, "status unknown, id: %d", id);
        return DEV_ST_UNKNOWN;
    }

    *len = st_table[id].len;

    pthread_mutex_lock(&st_item_mutex);
    memcpy(data, st_item_buf + st_table[id].offset, *len);
    pthread_mutex_unlock(&st_item_mutex);

    return 0;
}

/************************************************************************
 function:     st_set_para
 description:  setting parameter from ehu or app
 input:        const TCOM_MSG_HEADER *msgheader, message header
 const unsigned char *msgbody, message body
 output:       none
 return:       0 indicates success;
 -1 indicates failed
 ************************************************************************/
int st_set(ST_DEF_ITEM_ID id, unsigned char *data, unsigned int len)
{
    int ret;
    unsigned int paralen;
    static unsigned char oldpara[ST_MAX_STATUS_LEN];

    if (id >= ST_ITEM_ID_MAX)
    {
        log_e(LOG_DEV, "invalid id, id:%u", id);
        return DEV_INVALID_PARA;
    }

    pthread_mutex_lock(&st_set_mutex);

    /* get the current status */
    memset(oldpara, 0, sizeof(oldpara));
    paralen = ST_MAX_STATUS_LEN;
    ret = st_get(id, oldpara, &paralen);

    if (((ret != 0) && (ret != DEV_ST_UNKNOWN)) || (paralen != len))
    {
        pthread_mutex_unlock(&st_set_mutex);
        log_e(LOG_DEV, "req len is invalid, reqlen:%u, paralen:%u, id:%u,%u", len, paralen, id,
              st_table[id].len);
        return DEV_INVALID_PARA;
    }

    /* if setting status at the first time or setting with a different value,
     update the value and notify the registrant */
    if ((DEV_ST_UNKNOWN == ret) || (0 != memcmp(data, oldpara, len)))
    {
        pthread_mutex_lock(&st_item_mutex);
        memcpy(st_item_buf + st_table[id].offset, data, len);
        st_table[id].is_valid = 1;
        pthread_mutex_unlock(&st_item_mutex);

        if(ST_ITEM_WAKEUP_SRC == id)
        {
             unsigned short wakesrc;
            wakesrc = *(unsigned short *)data;
            if( wakesrc & ( 1 << 6 ) )
            {
                PP_rmtDiag_mcuRTCweakup();
            }
        }

        st_notify_changed(id, oldpara, len);
    }

    pthread_mutex_unlock(&st_set_mutex);

    if((ST_ITEM_ECALL == id) || (ST_ITEM_UPG_STATUS == id)  || \
       (ST_ITEM_APP_SLEEP == id))
    {
        dev_sync_timeout();
    }

    return 0;
}

/********************************************************************************
 function:     st_dump_para
 description:  dump all para
 input:        unsigned int argc, para count;
 unsigned char **argv, para value
 output:       none
 return:       0 indicates success;
 others indicates failed;
 *********************************************************************************/
int st_dump(int argc, const char **argv)
{
    int i, j;
    unsigned char *para;

    for (i = 0; i < sizeof(st_table) / sizeof(ST_ITEM_INFO); i++)
    {
        /* dump status item id with index i */
        para = st_item_buf + st_table[i].offset;

        if( ST_ITEM_PM_MODE == st_table[i].itemid )
        {
            if (!st_table[i].is_valid)
            {
                shellprintf(" %s:unknown\r\n", st_table[i].member[0].name);
                continue;
            }

            switch( *para )
            {
                case PM_RUNNING_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "running mode");
                    break;
                    
                case PM_EMERGENCY_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "emergency mode");
                    break;  
                    
                case PM_LISTEN_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "listen mode");
                    break; 
                    
                case PM_SLEEP_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "sleep mode");
                    break; 

                case PM_DEEP_SLEEP_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "deep sleep mode");
                    break;   

                case PM_OFF_MODE:
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, "off mode");
                    break; 

                default:  
                    shellprintf(" %s:unknown\r\n", st_table[i].member[0].name);
                    break; 
            }

            continue;
        }

        if( ST_ITEM_WAKEUP_SRC == st_table[i].itemid )
        {
            unsigned short wakesrc, k;
            
            if (!st_table[i].is_valid)
            {
                shellprintf(" %s:unknown\r\n", st_table[i].member[0].name);
                continue;
            }

            char *src[] = { "KL15", "KL75", "USB", "SLOW CHARGE" , "FAST CHARGE",   
                            "RING", "RTC", "ECU UPGRADE", "BLE", "G-SENSOR",        
                            "G-CAN1", "G-CAN2", "G-CAN3", "KL30 DISCONNECTED", 
                            "4G ANT DISCONNECTED", "UNKNOWN" };

            wakesrc = *(unsigned short *)para;
            for( k = 0; k < 16; k++ )
            {
                if( wakesrc & ( 1 << k ) )
                {
                    shellprintf(" %s:%s\r\n", st_table[i].member[0].name, src[k]);        
                }   
            }
           
            continue;
        }

        for (j = 0; j < ST_ITEM_MAX_MEMBER_CNT; j++)
        {
            if (ST_DATA_INVALID == st_table[i].member[j].type)
            {
                break;
            }

            if (!st_table[i].is_valid)
            {
                shellprintf(" %s:unknown\r\n", st_table[i].member[j].name);
            }
            else
            {
                /* dump all status item */
                switch (st_table[i].member[j].type)
                {
                    case ST_DATA_UCHAR:
                        shellprintf(" %s:%u\r\n", st_table[i].member[j].name, *para);
                        break;

                    case ST_DATA_USHORT:
                        shellprintf(" %s:%u\r\n", st_table[i].member[j].name, *((unsigned short *)para));
                        break;

                    case ST_DATA_UINT:
                        shellprintf(" %s:%u\r\n", st_table[i].member[j].name, *((unsigned int *)para));
                        break;

                    case ST_DATA_INT:
                        shellprintf(" %s:%d\r\n", st_table[i].member[j].name, *((unsigned int *)para));
                        break;

                    case ST_DATA_STRING:
                        shellprintf(" %s:%s\r\n", st_table[i].member[j].name, (char *)para);
                        break;

                    case ST_DATA_DOUBLE:
                        shellprintf(" %s:%f\r\n", st_table[i].member[j].name, *((double *)para));
                        break;

                    default:
                        shellprintf(" unknown type\r\n");
                        return  ST_DATA_INVALID;
                        break;
                }
            }

            para = para + st_table[i].member[j].len;
        }
    }

    return 0;
}

