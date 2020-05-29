/******************************************************
�ļ�����	PrvtProt.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
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
#include "dir.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "XcallReqinfo.h"
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"
#include "file.h"
#include "init.h"
#include "log.h"
#include "uds.h"
#include "list.h"
#include "../sockproxy/sockproxy_rxdata.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "at_api.h"
#include "cfg_api.h"
#include "udef_cfg_api.h"
#include "pm_api.h"
#include "dev_api.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "hozon_ver_api.h"
#include "shell_api.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt_mpuAbnor.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
static PP_mpuAbnor_t  PP_mpuAbnor;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void PrvtPro_mpureboot(void);
static int PrvtPro_check_icciderr(void);
static int PrvtPro_mpucycleReboot(void);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPrvtPro_mpuAbnor

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void InitPrvtPro_mpuAbnor(void)
{
	unsigned int cfglen;
	PP_mpuAbnor.IGNnewst = 0;
	PP_mpuAbnor.mpurebootflag = 0;
	PP_mpuAbnor.iccidchktimer = tm_get_time();
	PP_mpuAbnor.pwrontimer = tm_get_time();
	PP_mpuAbnor.cycreboottsktimer = tm_get_time();
	PP_mpuAbnor.sleepflag = 1;
	cfglen = 4;
	cfg_get_para(CFG_ITEM_MPUREBOOT_DATE,&PP_mpuAbnor.datetime,&cfglen);//读取重启日期
	cfglen = 1;
	cfg_get_para(CFG_ITEM_MPUREBOOT_TIMES,&PP_mpuAbnor.reboottimes,&cfglen);//读取重启次数
}

/******************************************************
*PrvtPro_mpuAbnorHandle

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
void PrvtPro_mpuAbnorHandle(void)
{
	if((tm_get_time() - PP_mpuAbnor.pwrontimer) >= PP_PWRON_TIMEOUT)
	{
		PP_mpuAbnor.IGNnewst = dev_get_KL15_signal();
		if(1 == PrvtPro_check_icciderr())
		{
			PP_mpuAbnor.mpurebootflag = 1;
		}
#if 0
		if(1 == PrvtPro_mpucycleReboot())
		{
			PP_mpuAbnor.mpurebootflag = 1;
		}
#endif
		if(1 == PP_mpuAbnor.mpurebootflag)
		{
			PP_mpuAbnor.mpurebootflag = 0;
			PrvtPro_mpureboot();//重启mpu
		}
	}
}

/******************************************************
*PrvtPro_mpureboot

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static void PrvtPro_mpureboot(void)
{
	time_t timep;
	struct tm *localdatetime;
    uint32_t tm_datetime;

	time(&timep);
	localdatetime = localtime(&timep);//取得当地时间
	log_o(LOG_HOZON,"%d-%d-%d ",(1900+localdatetime->tm_year), \
			(1 +localdatetime->tm_mon), localdatetime->tm_mday);
	log_o(LOG_HOZON," %d:%d:%d\n", \
			localdatetime->tm_hour, localdatetime->tm_min, localdatetime->tm_sec);
	tm_datetime = (1900+localdatetime->tm_year) * 10000 + \
							(1 +localdatetime->tm_mon) * 100 + localdatetime->tm_mday;
	if(PP_mpuAbnor.datetime != tm_datetime)
	{
		PP_mpuAbnor.reboottimes = 1;
		cfg_set_para(CFG_ITEM_MPUREBOOT_TIMES, &PP_mpuAbnor.reboottimes, 1);
		PP_mpuAbnor.datetime = tm_datetime;
		cfg_set_para(CFG_ITEM_MPUREBOOT_DATE, &PP_mpuAbnor.datetime, 4);
		log_o(LOG_HOZON,"today,the %d time to reboot mpu\n",PP_mpuAbnor.reboottimes);
		//sleep(1);
		system("reboot");
	}
	else
	{
		if(PP_mpuAbnor.reboottimes < PP_MPUREBOOT_TIMES)
		{
			PP_mpuAbnor.reboottimes++;
			cfg_set_para(CFG_ITEM_MPUREBOOT_TIMES, &PP_mpuAbnor.reboottimes, 1);
			PP_mpuAbnor.datetime = tm_datetime;
			cfg_set_para(CFG_ITEM_MPUREBOOT_DATE, &PP_mpuAbnor.datetime, 4);
			log_o(LOG_HOZON,"today,the %d time to reboot mpu\n",PP_mpuAbnor.reboottimes);
			//sleep(1);
			system("reboot");
		}
		else
		{
			log_o(LOG_HOZON,"mpu have to reboot %d times\n",PP_MPUREBOOT_TIMES);
		}
	}
}


void PrvtPro_mpureboottest(void)
{
	PP_mpuAbnor.mpurebootflag = 1;
}


/******************************************************
*PrvtPro_check_icciderr

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static int PrvtPro_check_icciderr(void)
{
	uint8_t	iccid[21] = {0};
	if(1 == PP_mpuAbnor.IGNnewst)//IGN ON
	{
		if(1 != PP_rmtCfg_getIccid(iccid))
		{
			if((tm_get_time() - PP_mpuAbnor.iccidchktimer) > PP_ICCIDCHECK_TIMEOUT)
			{
				PP_mpuAbnor.iccidchktimer = tm_get_time();
				return 1;
			}
		}
		else
		{
			PP_mpuAbnor.iccidchktimer = tm_get_time();
		}
	}
	else
	{
		PP_mpuAbnor.iccidchktimer = tm_get_time();
	}

	return 0;
}

/******************************************************
*PrvtPro_mpucycleReboot

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static int PrvtPro_mpucycleReboot(void)
{
	time_t timep;
	struct tm *localdatetime;
    uint32_t tm_datetime;

	if((tm_get_time() - PP_mpuAbnor.cycreboottsktimer) >= 100)
	{
		PP_mpuAbnor.cycreboottsktimer = tm_get_time();

		if((0 == PP_mpuAbnor.IGNnewst)	&& \
		(0 == gb32960_gbCanbusActiveSt()))//IGN OFF && can bus inactive
		{
			time(&timep);
			localdatetime = localtime(&timep);//取得当地时间
			tm_datetime = (1900+localdatetime->tm_year) * 10000 + \
									(1 +localdatetime->tm_mon) * 100 + localdatetime->tm_mday;
			if(PP_mpuAbnor.datetime != tm_datetime)
			{
				PP_mpuAbnor.sleepflag = 0;
				log_o(LOG_HOZON,"cycle reboot mpu\n");
				return 1;
			}
			else
			{
				PP_mpuAbnor.sleepflag = 1;
			}
		}
	}

	return 0;
}

/*
	获取睡眠状态
*/
int GetPrvtPro_mpuAbnorsleepSt(void)
{
	//return PP_mpuAbnor.sleepflag;
	return 1;
}

/*
	唤醒
*/
void ClearPrvtPro_mpuAbnorsleep(void)
{
	PP_mpuAbnor.sleepflag = 0;
}

