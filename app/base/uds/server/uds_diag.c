#include <string.h>
#include "init.h"
#include "uds_diag_def.h"
#include "diag.h"
#include "status_sync.h"
#include "timer.h"
#include "rds.h"
#include "uds.h"
#include <pthread.h>
#include "hozon_PP_api.h"
#include "uds_diag.h"

unsigned int UDS_GetDTCSetting(void);
PP_rmtDiag_NodeFault_t g_PP_rmtDiag_NodeFault;




UDS_DIAG_ITEM_BUF_T uds_diag_item_buf_t;
IS_UDS_TRIGGER_FAULT is_uds_trigger_fault;


void set_is_uds_trigger_fault(IS_UDS_TRIGGER_FAULT_TYPE f_is_fault)
{
    pthread_mutex_lock(&is_uds_trigger_fault.is_fault_mtx);
    is_uds_trigger_fault.is_fault = f_is_fault;
    pthread_mutex_unlock(&is_uds_trigger_fault.is_fault_mtx);
}

IS_UDS_TRIGGER_FAULT_TYPE get_is_uds_trigger_fault(void)
{
    unsigned char ret_is_fault = 0;
    pthread_mutex_lock(&is_uds_trigger_fault.is_fault_mtx);
    ret_is_fault = is_uds_trigger_fault.is_fault;
    pthread_mutex_unlock(&is_uds_trigger_fault.is_fault_mtx);
    return ret_is_fault;
}

/* 处理 UDS 给 GB模块诊断信息处理 */
static void uds_to_gb_process(int dtc_sn)
{
    if(DTC_NUM_MISSING_BMS == dtc_sn)
    {
        g_PP_rmtDiag_NodeFault.BMSMiss = 1;
    }
    else if(DTC_NUM_MISSING_MCU == dtc_sn)
    {
        g_PP_rmtDiag_NodeFault.MCUMiss = 1;
    }
    g_PP_rmtDiag_NodeFault.tboxFault = 1;
}

PP_rmtDiag_NodeFault_t * get_PP_rmtDiag_NodeFault_t(void)
{
    return (&g_PP_rmtDiag_NodeFault);
}


//static unsigned char uds_diag_item_buf[DIAG_ITEM_BUF_LEN];

/**********************************************************************
function:     uds_diag_para_check
description:  check whether the para is right
input:        DIAG_DATA_TYPE type
              unsigned int len
output:       none
return:       0 indicates success;
              others indicates failed
***********************************************************************/
int uds_diag_para_check(DIAG_DATA_TYPE type, unsigned int len)
{
    switch (type)
    {
        case DIAG_DATA_UINT:
            if (len != sizeof(unsigned int))
            {
                return UDS_INVALID_PARA;
            }

            break;

        case DIAG_DATA_USHORT:
            if (len != sizeof(unsigned short))
            {
                return UDS_INVALID_PARA;
            }

            break;

        case DIAG_DATA_UCHAR:
            if (len != sizeof(unsigned char))
            {
                return UDS_INVALID_PARA;
            }

            break;

        case DIAG_DATA_DOUBLE:
            if (len != sizeof(double))
            {
                return UDS_INVALID_PARA;
            }

            break;

        case DIAG_DATA_STRING:
            if (len <= sizeof(unsigned char))
            {
                return UDS_INVALID_PARA;
            }

            break;

        case DIAG_DATA_TIME:
            if (len != sizeof(DIAG_RTCTIME))
            {
                return UDS_INVALID_PARA;
            }

            break;

        default:
            return UDS_INVALID_PARA;
    }

    return 0;
}

/**************************************************************************
function:     uds_diag_init
description:  init uds diag module,must used behind INIT_PHASE_RESTORE !!!
input:        DIAG_DATA_TYPE type
              unsigned int len
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
int uds_diag_init(void)
{
    int ret;
    int i, j, k;
    int offset = 0, size = 0;
    char ver[32];
    unsigned int len = 0;

    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    memset(uds_diag_item_buf_t.uds_diag_item_buf, 0, sizeof(uds_diag_item_buf_t.uds_diag_item_buf));  
    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);

    set_is_uds_trigger_fault(NO_FAULT);

    //memset(uds_diag_item_buf, 0, sizeof(uds_diag_item_buf));

    for (i = 0; i < sizeof(diag_table) / sizeof(DIAG_ITEM_INFO); i++)
    {
        for (j = 0; j < DIAG_MAX_DID_CNT; j++)
        {
            if (DID_INVALID != diag_table[i].freeze[j])
            {
                for (k = 0; k < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); k++)
                {
                    if (diag_did_table[k].id == diag_table[i].freeze[j])
                    {
                        size = size + diag_did_table[k].len;
                        break;
                    }
                }

                if (k >= sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID))
                {
                    log_e(LOG_UDS, "invalid did, did:%u", diag_table[i].freeze[j]);
                    return UDS_INVALID_PARA;
                }

            }
            else
            {
                break;
            }
        }

        /* compute each para length and offset */
        diag_table[i].offset    = offset;
        pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
        diag_table[i].confirmed = (unsigned int *)(uds_diag_item_buf_t.uds_diag_item_buf + offset);
        pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
        diag_table[i].len       = size + sizeof(unsigned int);  /* include confirmed flag */
        offset = offset + diag_table[i].len;
        size = 0;
    }

    if ((diag_table[i - 1].offset +  diag_table[i - 1].len) > DIAG_ITEM_BUF_LEN)
    {
        log_e(LOG_UDS, "diag para buf overflow, total len:%u",
              diag_table[i - 1].offset +  diag_table[i - 1].len);
        return UDS_PARA_OVERFLOW;
    }

    /* get history fault information */
    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    len = sizeof(uds_diag_item_buf_t.uds_diag_item_buf);
    ret = rds_get(RDS_DATA_UDS_DIAG, uds_diag_item_buf_t.uds_diag_item_buf, &len, ver);
    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    

    if (0 != ret)
    {
        log_e(LOG_UDS, "get uds fault diag info failed");
    }

    return 0;
}

/**************************************************************************
function:     uds_diag_gen_freeze
description:  diag devices and generate freeze
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       none
**************************************************************************/
void uds_diag_gen_freeze(DIAG_DEF_ITEM_ID id)
{
    int i, j, offset;
    offset = diag_table[id].offset + sizeof(unsigned int);

    for (i = 0; i < DIAG_MAX_DID_CNT; i++)
    {
        if (DID_INVALID == diag_table[id].freeze[i])
        {
            break;
        }
        else
        {
            for (j = 0; j < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); j++)
            {
                if (diag_did_table[j].id == diag_table[id].freeze[i])
                {
                    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
                    diag_did_table[j].get(uds_diag_item_buf_t.uds_diag_item_buf + offset, diag_did_table[j].len);
                    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
                    offset += diag_did_table[j].len;
                    break;
                }
            }
        }
    }
}

/**************************************************************************
function:     uds_diag_one_device
description:  diag a devices be appointed
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       none
**************************************************************************/
void uds_diag_one_device(DIAG_DEF_ITEM_ID id)
{
    unsigned char diag_ret;

    if ((DIAG_ITEM_NUM <= id) || (NULL == diag_table[id].fun))
    {
        log_e(LOG_UDS, "can not diag this devices,id: %d", id);
        return;
    }

    diag_ret = diag_table[id].fun();

    /* no fault */
    if (!diag_ret)
    {
        diag_table[id].counter = 0;
    }
    else
    {
        if (diag_table[id].counter < diag_table[id].thsd)
        {
            diag_table[id].counter++;
        }
        else
        {
            /* if freeze frame is not exist,generate it */
            if (!(*(diag_table[id].confirmed)))
            {
                *(diag_table[id].confirmed) = 1;
                uds_diag_gen_freeze(id);
            }
        }
    }
}

/**************************************************************************
function:     uds_diag_all_devices
description:  diag all the devices about tbox
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       none
**************************************************************************/
void uds_diag_all_devices(void)
{
    int i;
    unsigned int  pre_confirmed;
    unsigned char is_gen_freeze = 0;

    if (0 == UDS_GetDTCSetting())
    {
        return;
    }

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        pre_confirmed = *(diag_table[i].confirmed);
        uds_diag_one_device(i);

        /* new confirmed fault generated */
        if ((0 == pre_confirmed) && (1 == *(diag_table[i].confirmed)))
        {
            is_gen_freeze = 1;
            uds_to_gb_process(i);
        }
    }

    if (is_gen_freeze)
    {
        /* save freeze frame and confirmed flag */
        pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
        rds_update_once(RDS_DATA_UDS_DIAG, uds_diag_item_buf_t.uds_diag_item_buf, 
            sizeof(uds_diag_item_buf_t.uds_diag_item_buf));
        pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    }
}

/**************************************************************************
function:     uds_diag_devices
description:  Diagnostic devices based on 
              the range of diagnostic serial numbers
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       none
**************************************************************************/
void uds_diag_devices(int startno, int endno)
{
    int i;
    unsigned int  pre_confirmed;
    unsigned char is_gen_freeze = 0;
    IS_UDS_TRIGGER_FAULT_TYPE is_fault = NO_FAULT;

    if (0 == UDS_GetDTCSetting())
    {
        return;
    }
    
    for (i = startno; i < endno; i++)
    {
        pre_confirmed = *(diag_table[i].confirmed);
        uds_diag_one_device(i);

        /* new confirmed fault generated */
        if ((0 == pre_confirmed) && (1 == *(diag_table[i].confirmed)))
        {
            is_gen_freeze = 1;
        }
    }

    if (is_gen_freeze)
    {
        /* save freeze frame and confirmed flag */
        pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
        rds_update_once(RDS_DATA_UDS_DIAG, uds_diag_item_buf_t.uds_diag_item_buf, 
            sizeof(uds_diag_item_buf_t.uds_diag_item_buf));
        pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    }

    
    for(i = 0; i < DIAG_ITEM_NUM; i++)
    {
        if(1 == *(diag_table[i].confirmed))
        {
            is_fault = FAULT;
            break;
        }
    }
    set_is_uds_trigger_fault(is_fault);
    //log_i(LOG_UDS, "get_is_uds_trigger_fault:%d", get_is_uds_trigger_fault());
}


/**************************************************************************
function:     uds_diag_get_dtc_num
description:  get confirm and unconfirm dtc number
input:        none
output:       none
return:       counter
**************************************************************************/
unsigned int uds_diag_get_dtc_num(void)
{
    int i, counter = 0;

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        if (*(diag_table[i].confirmed) || diag_table[i].counter > 0)
        {
            counter++;
        }
    }

    return counter;
}

/**************************************************************************
function:     uds_diag_get_dtc_num
description:  get confirm dtc number
input:        none
output:       none
return:       counter
**************************************************************************/
unsigned int uds_diag_get_cfm_dtc_num(void)
{
    int i, counter = 0;

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        if (*(diag_table[i].confirmed))
        {
            counter++;
        }
    }

    return counter;
}

/**************************************************************************
function:     uds_diag_get_uncfm_dtc_num
description:  get unconfirm dtc num
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
unsigned int uds_diag_get_uncfm_dtc_num(void)
{
    int i, counter = 0;

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        if (diag_table[i].counter > 0)
        {
            counter++;
        }
    }

    return counter;
}

/**************************************************************************
function:     uds_diag_is_cfm_dtc_valid
description:  verify id whether is confirm dtc
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
unsigned int uds_diag_is_cfm_dtc_valid(DIAG_DEF_ITEM_ID id)
{
    return *(diag_table[id].confirmed);
}

/**************************************************************************
function:     uds_diag_is_uncfm_dtc_valid
description:  verify id whether is unconfirm dtc
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
unsigned int uds_diag_is_uncfm_dtc_valid(DIAG_DEF_ITEM_ID id)
{
    if (diag_table[id].counter > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**************************************************************************
function:     uds_diag_get_dtc
description:  get the dtc information by appointed
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
unsigned char *uds_diag_get_dtc(DIAG_DEF_ITEM_ID id)
{
    return (unsigned char *)diag_table[id].dtc;
}

/**************************************************************************
function:     uds_diag_get_dtc_name
description:  get the dtc information by appointed
input:        DIAG_DEF_ITEM_ID id
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
unsigned char *uds_diag_get_dtc_name(DIAG_DEF_ITEM_ID id)
{
    return (unsigned char *)diag_table[id].name;
}

/**************************************************************************
function:     uds_diag_get_did_value
description:  set did value
input:        unsigned short did
output:       unsigned char* value,
              unsigned int *len
return:       0 indicates success;
              others indicates failed
**************************************************************************/
int uds_diag_get_did_value(unsigned short did, unsigned char *value, unsigned int *len)
{
    int i, ret;
    unsigned short uint16_value;
    unsigned int   uint32_value;

    for (i = 0; i < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); i++)
    {
        if (diag_did_table[i].id == did)
        {
            break;
        }
    }

    if (i >= sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID))
    {
        log_e(LOG_UDS, "invalid did, did:%u, len:%u", did, *len);
        return UDS_INVALID_PARA;
    }

    if (*len < diag_did_table[i].len)
    {
        log_e(LOG_UDS, "buffer overlflow, did:%u, len:%u", did, *len);
        return UDS_INVALID_PARA;
    }

    ret = diag_did_table[i].get(value, *len);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get did failed, did:%u, ret:0x%08x", did, ret);
        return ret;
    }

    if (DIAG_DATA_USHORT == diag_did_table[i].type)
    {
        uint16_value = (value[0] >> 8) +  value[1];
        memcpy(value, &uint16_value, sizeof(uint16_value));
    }

    if (DIAG_DATA_UINT == diag_did_table[i].type)
    {
        uint32_value = (value[0] >> 24) + (value[1] >> 16) + (value[2] >> 8) + value[3];
        memcpy(value, &uint32_value, sizeof(uint32_value));
    }

    *len = diag_did_table[i].len;

    return 0;
}

int is_uds_diag_set_did_invalue(unsigned short did, unsigned char *value, unsigned int len)
{
    int i;

    for (i = 0; i < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); i++)
    {
        if (diag_did_table[i].id == did)
        {
            break;
        }
    }

    if (i >= sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID))
    {
        log_e(LOG_UDS, "invalid did, did:%u, len:%u", did, len);
        return NRC_RequestOutOfRange;
    }

    if (len != diag_did_table[i].len)
    {
        log_e(LOG_UDS, "invalid did length, did:%u, len:%u", did, len);
        return NRC_IncorrectMessageLengthOrInvailFormat;
    }

    if (g_u8SecurityAccess != diag_did_table[i].level)
    {
        log_e(LOG_UDS, "level type is not match");
        return NRC_SecurityAccessDenied;
    }
    return 0;
}


/**************************************************************************
function:     uds_diag_set_did_value
description:  set did value
input:        unsigned short did,
              unsigned char* value,
              unsigned int len
output:       none
return:       0 indicates success;
              others indicates failed
**************************************************************************/
int uds_diag_set_did_value(unsigned short did, unsigned char *value, unsigned int len)
{
    int i, ret;
    unsigned short temp1;
    unsigned int   temp2;

    for (i = 0; i < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); i++)
    {
        if (diag_did_table[i].id == did)
        {
            break;
        }
    }


    if (DIAG_DATA_USHORT == diag_did_table[i].type)
    {
        temp1 = (value[0] >> 8) +  value[1];
        memcpy(value, &temp1, sizeof(temp1));
    }

    if (DIAG_DATA_UINT == diag_did_table[i].type)
    {
        temp2 = (value[0] >> 24) + (value[1] >> 16) + (value[2] >> 8) + value[3];
        memcpy(value, &temp2, sizeof(temp2));
    }

    ret = diag_did_table[i].set(value, len);

    if (ret != 0)
    {
        return NRC_IncorrectMessageLengthOrInvailFormat;
    }

    return 0;
}


/**************************************************************************
function:     uds_diag_get_freeze
description:  get the freeze information、len、and num
input:        DIAG_DEF_ITEM_ID id,
output:       unsigned char *freeze,
              unsigned int *len,
              unsigned int *did_num
return:       0 indicates success;
              others indicates failed
**************************************************************************/
int uds_diag_get_freeze(DIAG_DEF_ITEM_ID id, unsigned char *freeze,
                        unsigned int *len, unsigned int *did_num)
{
    int i, j,  offset = 0, did_cnt = 0;
    int current_index = 0;
    
    *did_num = 0;

    if (DID_INVALID == diag_table[id].freeze[0])
    {
        *len = 0;
        *did_num = 0;
        return 0;
    }

    for (i = 0; i < DIAG_MAX_DID_CNT; i++)
    {
        if (DID_INVALID != diag_table[id].freeze[i])
        {
            did_cnt++;
            freeze[offset++] = (unsigned char)(diag_table[id].freeze[i] >> 8);
            freeze[offset++] = (unsigned char)(diag_table[id].freeze[i]);

            for (j = 0; j < sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID); j++)
            {
                if (diag_did_table[j].id == diag_table[id].freeze[i])
                {
                    if (diag_did_table[j].len + offset > *len)
                    {
                        log_e(LOG_UDS, "buffer overflow ,did_cnt = %d , len = %d,", did_cnt , *len);
                        return NRC_RequestOutOfRange;
                    }

                           
                    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
                    memcpy(freeze + offset, uds_diag_item_buf_t.uds_diag_item_buf + diag_table[id].offset + sizeof(unsigned int) + current_index,
                            diag_did_table[j].len);  
                    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
                    
                    offset += diag_did_table[j].len;
                    current_index += diag_did_table[j].len;
                    break;
                }
            }

            if (j >= sizeof(diag_did_table) / sizeof(DIAG_ITEM_DID))
            {
                log_e(LOG_UDS, "invalid id ,id = %d , freeze id = %d,", id , diag_table[id].freeze[i]);
                return NRC_RequestOutOfRange;
            }
        }
        else
        {
            break;
        }
    }

    *len = diag_table[id].len - sizeof(unsigned int) + did_cnt * sizeof(unsigned short);
    *did_num  = did_cnt;

    log_i(LOG_UDS, "did_cnt = %d , len = %d ", did_cnt , *len);

    return 0;
}

/**************************************************************************
function:     uds_diag_dtc_clear
description:  clear all the dtc information
input:        none
output:       none
return:       none
**************************************************************************/
void uds_diag_dtc_clear(void)
{
    int i;

    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    memset(uds_diag_item_buf_t.uds_diag_item_buf, 0, sizeof(uds_diag_item_buf_t.uds_diag_item_buf));  
    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);


    for (i = 0; i < sizeof(diag_table) / sizeof(DIAG_ITEM_INFO); i++)
    {
        diag_table[i].counter = 0;
    }

    pthread_mutex_lock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
    rds_update_once(RDS_DATA_UDS_DIAG, uds_diag_item_buf_t.uds_diag_item_buf, sizeof(uds_diag_item_buf_t.uds_diag_item_buf));
    pthread_mutex_unlock(&uds_diag_item_buf_t.uds_diag_item_buf_mtx);
}

