/**********************************************************************************
	
	用于远控预约功能定时唤醒MCU和MPU

*********************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>
#include "log.h"	
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "scom_api.h"
#include "at.h"
#include "pm_api.h"
#include "scom_msg_def.h"
#include "../../../../base/scom/scom_tl.h"
#include "PP_ACCtrl.h"
#include "PP_ChargeCtrl.h"
#include "PP_SendWakeUptime.h"

extern unsigned int wsrv_Get_WakeTime(void);

static PP_wake_t wake_period[7] =
{
	{0,0x40},//星期1
	{1,0x20},//星期2
	{2,0x10},//星期3
	{3,0x08},//星期4
	{4,0x04},//星期5
	{5,0x02},//星期6
	{6,0x01},//星期7
};

static char localweekToAlgoweek[7] = {6,0,1,2,3,4,5};

/*
* 计算最小唤醒时间（单位：min）
*/
int PP_waketime_to_min(waketime * pt)
{
	uint8_t wday;
	int temp_min = 0;
	int low_min = 0;
	time_t timep;
	struct tm *localdatetime;
	time(&timep);  //获取从1970.1.1 00:00:00到现在的秒数
	localdatetime = localtime(&timep);//获取本地时间

	for(wday = 0;wday < 7;wday++)//逐一检查周一到周天
	{
		if(wake_period[wday].mask & pt->period)//检查到当前星期有预约
		{	
			if(wday < localweekToAlgoweek[localdatetime->tm_wday])//下周的预约
			{
				temp_min = ((7-localweekToAlgoweek[localdatetime->tm_wday])*24*60 - \
							(localdatetime->tm_hour *60 + localdatetime->tm_min)) +	\
							(wday*24*60 + pt->hour*60 + pt->min);	
			}
			else if(wday == localweekToAlgoweek[localdatetime->tm_wday])//
			{
				if(localdatetime->tm_hour > pt->hour)//下周的预约
				{
					temp_min = ((7-localweekToAlgoweek[localdatetime->tm_wday])*24*60 - \
							(localdatetime->tm_hour *60 + localdatetime->tm_min)) +	\
							(wday*24*60 + pt->hour*60 + pt->min);			
				}
				else if(localdatetime->tm_hour == pt->hour)
				{
					if(localdatetime->tm_min > pt->min)//下周的预约
					{
						temp_min = ((7-localweekToAlgoweek[localdatetime->tm_wday])*24*60 - \
							(localdatetime->tm_hour *60 + localdatetime->tm_min)) +	\
							(wday*24*60 + pt->hour*60 + pt->min);			
					}
					else//本周的预约
					{
						temp_min = pt->min - localdatetime->tm_min;		
					}
				}
				else//本周的预约
				{
					temp_min = (pt->hour - localdatetime->tm_hour)*60 - localdatetime->tm_min + pt->min;		
				}
			}
			else//本周的预约
			{
				temp_min = (wday - localweekToAlgoweek[localdatetime->tm_wday])*24*60 + pt->hour*60 + \
							pt->min - localdatetime->tm_hour * 60 - localdatetime->tm_min;
			}

			if(low_min == 0)
			{
				low_min = temp_min;
			}
			else
			{
				if(low_min > temp_min)
				{
					low_min = temp_min;
				}
			}
		}
	}

	return low_min;
}

int pp_get_low_waketime(void)
{
	int ac_time = 0 ;
	int charge_time = 0;
	int fota_time = 0;
	int low_time = 0;
	ac_time = PP_ACCtrl_waketime();
	charge_time = PP_ChargeCtrl_waketime();
	fota_time = wsrv_Get_WakeTime();
	low_time = ac_time;
	if(low_time == 0)
	{
		if(charge_time != 0)
		{
			low_time = charge_time;
			if((low_time > fota_time ) &&(fota_time != 0))
			{
				low_time = fota_time;
			}
		}
		else
		{
			low_time = fota_time;
		}
	}
	else
	{
		if((low_time > charge_time ) &&(charge_time != 0))
		{
			low_time = charge_time;
		}
		if((low_time > fota_time ) &&(fota_time != 0))
		{
			low_time = fota_time;
		}
	}
	return low_time;
}

uint8_t  GetPP_Wake_Sleep()
{
	if(pp_get_low_waketime() <= 2)
		return 0;
	return 1;
}
void PP_Send_WakeUpTime_to_Mcu(void)
{
	int low_min = 0;
	static int sleep_flag = 0;
	if(at_get_pm_mode() == PM_RUNNING_MODE)
	{
		sleep_flag = 0;
	}

	if(at_get_pm_mode() == PM_LISTEN_MODE)
	{	
		low_min = pp_get_low_waketime();
		if(sleep_flag == 0)
		{
			if(low_min != 0)
			{
				scom_tl_send_frame(SCOM_TL_CMD_WAKE_TIME, SCOM_TL_SINGLE_FRAME, 0,
                           		 (unsigned char *)&low_min, sizeof(low_min));
				sleep_flag = 1;
				log_o(LOG_HOZON,"low_time = %d\n",low_min);
			}
		}
	}
}
