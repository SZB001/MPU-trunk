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

extern PP_rmtAC_AppointBook_t  PP_rmtac_AppointBook[ACC_APPOINT_NUM] ;
extern PP_rmtCharge_AppointBook_t PP_rmtCharge_AppointBook;
PP_rmtAc_Appointperiod_t PP_ACtrl_period[7] = {
	{0,0x01},//星期7
	{1,0x40}, //星期1
	{2,0x20},//星期2
	{3,0x10},//星期3
	{4,0x08},//星期4
	{5,0x04},//星期5
	{6,0x02},//星期6
};

/*******************************得到休眠之前时刻到预约充电的分钟******************************/
#if 0
int Get_Ac_Wake_Recent_time(void)
{
	int i;
	int temp_min = 0;
	int low_min = 0;
	time_t timep;
	struct tm *localdatetime;
	time(&timep);  //获取从1970.1.1 00:00:00到现在的秒数
	localdatetime = localtime(&timep);//获取本地时间
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
			
			if(PP_rmtac_AppointBook[i].hour < localdatetime->tm_hour )
			{
				continue;
			}
			else if(PP_rmtac_AppointBook[i].hour == localdatetime->tm_hour)
			{
				if(PP_rmtac_AppointBook[i].min <= localdatetime->tm_min)
				{
					continue;
				}
				else
				{
					temp_min = PP_rmtac_AppointBook[i].min - localdatetime->tm_min;
				}
			}
			else
			{
				if(PP_rmtac_AppointBook[i].min < localdatetime->tm_min)
				{
					temp_min = (PP_rmtac_AppointBook[i].hour - localdatetime->tm_hour - 1) * 60 + \
						PP_rmtac_AppointBook[i].min + 60 - localdatetime->tm_min;
				}
				else
				{
					temp_min = (PP_rmtac_AppointBook[i].hour - localdatetime->tm_hour ) * 60 + \
						PP_rmtac_AppointBook[i].min - localdatetime->tm_min;
				}
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


int Get_Ac_Wake_Today_Recent_time(void)
{
	int i;
	int temp_min = 0;
	int low_min = 0;
	time_t timep;
	struct tm *localdatetime;
	time(&timep);  //获取从1970.1.1 00:00:00到现在的秒数
	localdatetime = localtime(&timep);//获取本地时间
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
			if(PP_rmtac_AppointBook[i].period & 0x80)  //重复预约
			{
				if(PP_rmtac_AppointBook[i].hour < localdatetime->tm_hour )
				{
					continue;
				}
				else if(PP_rmtac_AppointBook[i].hour == localdatetime->tm_hour)
				{
					if(PP_rmtac_AppointBook[i].min <= localdatetime->tm_min)
					{
						continue;
					}
					else
					{
						temp_min = PP_rmtac_AppointBook[i].min - localdatetime->tm_min;
					}
				}
				else
				{
					if(PP_rmtac_AppointBook[i].min < localdatetime->tm_min)
					{
						temp_min = (PP_rmtac_AppointBook[i].hour - localdatetime->tm_hour - 1) * 60 + \
							PP_rmtac_AppointBook[i].min + 60 - localdatetime->tm_min;
					}
					else
					{
						temp_min = (PP_rmtac_AppointBook[i].hour - localdatetime->tm_hour ) * 60 + \
							PP_rmtac_AppointBook[i].min - localdatetime->tm_min;
					}
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

	}
	return low_min;
}
#endif

int Get_Ac_Wake_next_Recent_time(void)
{
	int i,j;
	int k = 0;
	int temp_min = 0;
	int low_min = 0;
	time_t timep;
	struct tm *localdatetime;
	time(&timep);  //获取从1970.1.1 00:00:00到现在的秒数
	localdatetime = localtime(&timep);//获取本地时间
	for( i =localdatetime->tm_wday ; i <= localdatetime->tm_wday+7 ;i++)
	{
		for(j=0;j<ACC_APPOINT_NUM;j++)
		{
			if(i>=7)
			{
				k = i-7;
			}
			else
			{
				k = i;
			}
			if(PP_rmtac_AppointBook[j].validFlg == 1)
			{
				if(PP_ACtrl_period[k].mask & PP_rmtac_AppointBook[j].period)
				{
					if((PP_rmtac_AppointBook[j].hour + (i - localdatetime->tm_wday) *24 ) < localdatetime->tm_hour)
					{
						continue;
					}
					else if((PP_rmtac_AppointBook[j].hour + (i - localdatetime->tm_wday) *24 ) == localdatetime->tm_hour)
					{
						if(PP_rmtac_AppointBook[j].min <= localdatetime->tm_min)
						{
							continue;
						}
						else
						{
							temp_min = PP_rmtac_AppointBook[j].min - localdatetime->tm_min;
						}
					}
					else
					{
						if(PP_rmtac_AppointBook[j].min < localdatetime->tm_min)
						{
							temp_min = (PP_rmtac_AppointBook[j].hour + (i - localdatetime->tm_wday) *24- localdatetime->tm_hour - 1) * 60 + \
								PP_rmtac_AppointBook[j].min + 60 - localdatetime->tm_min;
						}
						else
						{
							temp_min = (PP_rmtac_AppointBook[j].hour + (i - localdatetime->tm_wday) *24 - localdatetime->tm_hour ) * 60 + \
								PP_rmtac_AppointBook[j].min - localdatetime->tm_min;
						}
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
		}
	}
	return low_min;
}

/*******************************得到休眠之前时刻到预约充电的分钟******************************/
int Get_Charge_Wake_Recent_time(void)
{
	int k,i;
	int temp_min = 0;
	int low_min = 0;
	time_t timep;
	struct tm *localdatetime;
	time(&timep);  //获取从1970.1.1 00:00:00到现在的秒数
	localdatetime = localtime(&timep);//获取本地时间
	for( i =localdatetime->tm_wday ; i <=localdatetime->tm_wday+7 ;i++)
	{
			
		if(i>=7)
		{
			k = i-7;
		}
		else
		{
			k = i;
		}
		if(PP_rmtCharge_AppointBook.validFlg == 1)
		{
			if(PP_ACtrl_period[k].mask & PP_rmtCharge_AppointBook.period)
			{
				if((PP_rmtCharge_AppointBook.hour + (i - localdatetime->tm_wday) *24 ) < localdatetime->tm_hour)
				{
					continue;
				}
				else if((PP_rmtCharge_AppointBook.hour + (i - localdatetime->tm_wday) *24 ) == localdatetime->tm_hour)
				{
					if(PP_rmtCharge_AppointBook.min <= localdatetime->tm_min)
					{
						continue;
					}
					else
					{
						temp_min = PP_rmtCharge_AppointBook.min - localdatetime->tm_min;
					}
				}
				else
				{
					if(PP_rmtCharge_AppointBook.min < localdatetime->tm_min)
					{
						temp_min = (PP_rmtCharge_AppointBook.hour + (i - localdatetime->tm_wday) *24- localdatetime->tm_hour - 1) * 60 + \
									PP_rmtCharge_AppointBook.min + 60 - localdatetime->tm_min;
					}
					else
					{
						temp_min = (PP_rmtCharge_AppointBook.hour + (i - localdatetime->tm_wday) *24 - localdatetime->tm_hour ) * 60 + \
									PP_rmtCharge_AppointBook.min - localdatetime->tm_min;
					}
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
	}

	return low_min;
}

/******************************获取定时唤醒一个最近时间*********************************/
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
		if((Get_Ac_Wake_next_Recent_time() != 0) && (Get_Charge_Wake_Recent_time() == 0))
		{
			low_min = Get_Ac_Wake_next_Recent_time();
		}
		else if((Get_Ac_Wake_next_Recent_time() == 0) && (Get_Charge_Wake_Recent_time() != 0))
		{
			low_min = Get_Charge_Wake_Recent_time();
		}
		else if((Get_Ac_Wake_next_Recent_time() != 0) && (Get_Charge_Wake_Recent_time() != 0))
		{
			low_min = (Get_Ac_Wake_next_Recent_time() > Get_Charge_Wake_Recent_time())? Get_Charge_Wake_Recent_time() : Get_Ac_Wake_next_Recent_time();
		}
		else
		{
		}
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


