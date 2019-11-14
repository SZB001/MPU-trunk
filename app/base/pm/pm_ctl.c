#include <stdio.h>
#include <unistd.h>
#include "com_app_def.h"
#include "tcom_api.h"
#include "dev_api.h"
#include "pm_ctl.h"
#include "file.h"
#include "pm_api.h"

static PM_REG_TBL pm_regtbl;
static pthread_mutex_t pm_regtbl_mutex;

/****************************************************************
function:       pm_init_para
description:    init para
input:          none
output:         none
return:         none
*****************************************************************/
void pm_init_para(void)
{
    int i;

    pthread_mutex_init(&pm_regtbl_mutex, NULL);

    pm_regtbl.used_num = 0;

    for (i = 0; i < PM_MAX_REG_ITEM_NUM; i++)
    {
        pm_regtbl.pmtbl[i].handler = NULL;
        pm_regtbl.pmtbl[i].mid     = 0;
    }
}

/****************************************************************
function:       pm_reg_handler
description:    register sleep module
input:          none
output:         none
return:         0 indicates register successful
                other indicates register failed
*****************************************************************/
int pm_reg_handler(unsigned short mid, sleep_handler handler)
{
    /* the paramerter is invalid */
    if (NULL == handler)
    {
        log_e(LOG_PM, "handler is NULL");
        return PM_INVALID_PARA;
    }

    pthread_mutex_lock(&pm_regtbl_mutex);

    if (pm_regtbl.used_num >= PM_MAX_REG_ITEM_NUM)
    {
        pthread_mutex_unlock(&pm_regtbl_mutex);
        log_e(LOG_PM, "pm register table overflow");
        return PM_TABLE_OVERFLOW;
    }

    pm_regtbl.pmtbl[pm_regtbl.used_num].mid     = mid;
    pm_regtbl.pmtbl[pm_regtbl.used_num].handler = handler;
    pm_regtbl.used_num++;
    pthread_mutex_unlock(&pm_regtbl_mutex);

    return 0;
}

/****************************************************************
function:       pm_notify_moudle
description:    notify module to sleep or wakeup
input:          none
output:         none
return:         0 indicates successful
                other indicates failed
*****************************************************************/
int pm_notify_moudle(PM_EVT_ID msgid)
{
    int ret, i;
    TCOM_MSG_HEADER msghdr;

    log_o(LOG_PM, "*****************pm_notify_moudle:0x%x****************", msgid);

    for (i = 0; i < pm_regtbl.used_num; i++)
    {
        msghdr.sender     = MPU_MID_PM ;
        msghdr.receiver   = pm_regtbl.pmtbl[i].mid;
        msghdr.msgid      = msgid;
        msghdr.msglen     = 0;

        ret = tcom_send_msg(&msghdr, NULL);

        if (ret != 0)
        {
            log_e(LOG_PM, "send message to pm failed ");
            return ret;
        }
    }

    return 0;
}

/****************************************************************
function:       pm_is_sleep_ready
description:    check the modules are ready for sleeping
input:          none
output:         none
return:         1 indicates module can sleep
                0 indicates module can not sleep
*****************************************************************/
int pm_is_sleep_ready(PM_EVT_ID id)
{
    int i, count = 0;

    for (i = 0; i < pm_regtbl.used_num; i++)
    {
		if(pm_regtbl.pmtbl[i].handler != NULL)//by liujian ,���ú���ָ��һ���ж��Ƿ�ΪNULL
		{
			if (1 == pm_regtbl.pmtbl[i].handler(id))
			{
				count++;
			}
			else
			{
				log_e(LOG_PM, "moudle %u is not ready for sleep", pm_regtbl.pmtbl[i].mid);
			}
		}
    }

    if (count == pm_regtbl.used_num)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/****************************************************************
function:       pm_usb_wakeup_check
description:    check pm module can enter into sleep mode
input:          none
output:         none
return:         0 indicates module can sleep
                1 indicates module can not sleep
*****************************************************************/
int  pm_usb_wakeup_check(void)
{
    int ret;
    unsigned int len;
    char status;

    len = sizeof(status);
    ret = file_read("/sys/devices/78d9000.usb/in_lpm", (unsigned char *)&status, &len);

    if (0 != ret)
    {
        log_e(LOG_PM, "open dev failed , error:%d", ret);
        return -1;
    }

    /*voltage of vbus is low */
    if ('1' == status)
    {
        return 0;
    }
    /* voltage of vbus is high */
    else if ('0' == status)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}


/****************************************************************
function:       pm_mcu_wakeup_check
description:    check pm module can enter into sleep mode
input:          none
output:         none
return:         0 indicates module can sleep
                1 indicates module can not sleep
*****************************************************************/
int  pm_mcu_wakeup_check(void)
{
    int ret;
    unsigned int len;
    char status;

    len = sizeof(status);
    ret = file_read("/sys/devices/soc:quec,quectel-power-manager/dtr_in", (unsigned char *)&status,
                    &len);

    if (0 != ret)
    {
        log_e(LOG_PM, "open dev failed , ret:%d", ret);
        return -1;
    }

    /*voltage of vbus is low */
    if ('1' == status)
    {
        return 0;
    }
    /* voltage of vbus is high */
    else if ('0' == status)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}


