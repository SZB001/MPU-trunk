/******************************************************
�ļ�����	

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2020/6/2		V1.0			liujian
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
#include "scom_api.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt_SBattRechrg.h"

/*******************************************************
description�� global variable definitions
*******************************************************/
extern long PrvtPro_getTimestamp(void);

/*******************************************************
description�� static variable definitions
*******************************************************/
static PP_SBattRechrg_t  PP_SBRC;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPrvtPro_SBattRechrg

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void InitPrvtPro_SBattRechrg(void)
{
	unsigned int cfglen;
	PP_SBRC.IGNnewst = 0xff;
	PP_SBRC.IGNoldst = 0xff;
	PP_SBRC.tsktimer = tm_get_time();
	PP_SBRC.sleepflag = 1;
	PP_SBRC.TskSt = PP_SBRC_DETEC;

	cfglen = 4;
	cfg_get_para(CFG_ITEM_SLEEP_TS,&PP_SBRC.sleeptimestamp,&cfglen);//读取sleep timestamp
}

/******************************************************
*PrvtPro_SBattRechrgHandle

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
void PrvtPro_SBattRechrgHandle(void)
{
	long currTimestamp;

	PP_SBRC.IGNnewst = dev_get_KL15_signal();
	if(PP_SBRC.IGNoldst != PP_SBRC.IGNnewst)
	{
		if(1 == PP_SBRC.IGNnewst)
		{
			if(0xffffffff != PP_SBRC.sleeptimestamp)
			{
				PP_SBRC.sleeptimestamp = 0xffffffff;
				cfg_set_para(CFG_ITEM_SLEEP_TS, &PP_SBRC.sleeptimestamp, 4);
			}
		}
	}

	switch(PP_SBRC.TskSt)
	{
		case PP_SBRC_DETEC:
		{
			if((tm_get_time() - PP_SBRC.tsktimer) >= 100)
			{
				PP_SBRC.tsktimer = tm_get_time();
				if(0 == PP_SBRC.IGNnewst)
				{
					if((0 != PP_SBRC.sleeptimestamp)	&& \
					   (0xffffffff != PP_SBRC.sleeptimestamp))
					{
						currTimestamp = PrvtPro_getTimestamp();
						if((currTimestamp > PP_SBRC.sleeptimestamp) && \
						   ((currTimestamp - PP_SBRC.sleeptimestamp) >= PP_SBRC_SLEEPTIME))
						{
							PP_SBRC.sleepflag = 0;
							PP_SBRC.TskSt = PP_SBRC_WAKE_VEHI;
							log_o(LOG_HOZON,"SBRC wakeup vehi\n");
							break;
						}
					}
				}

				PP_SBRC.sleepflag = 1;
			}
		}
		break;
		case PP_SBRC_WAKE_VEHI:
		{
			if(PP_canSend_weakupVehicle(SMALLBATT_RCHRG) != 0)
			{
				PP_SBRC.waittimer = tm_get_time();
				PP_SBRC.TskSt = PP_SBRC_WAKE_WAIT;
			}
		}
		break;
		case PP_SBRC_WAKE_WAIT:
		{
			if((tm_get_time() - PP_SBRC.waittimer) >= 30000)
			{
				PP_SBRC.TskSt = PP_SBRC_END;
			}
		}
		break;
		case PP_SBRC_END:
		{
			clearPP_canSend_virtualOnline(SMALLBATT_RCHRG);//清除虚拟on线
			if(0xffffffff != PP_SBRC.sleeptimestamp)
			{
				PP_SBRC.sleeptimestamp = 0xffffffff;
				cfg_set_para(CFG_ITEM_SLEEP_TS, &PP_SBRC.sleeptimestamp, 4);
			}
			PP_SBRC.TskSt = PP_SBRC_DETEC;
			PP_SBRC.sleepflag = 1;
		}
		break;
		default:
		break;
	}
}

/*
	获取睡眠状态
*/
int GetPrvtPro_SBattRechrgSleepSt(void)
{
	return PP_SBRC.sleepflag;
}

/*
	睡眠时设置
*/
void setPrvtPro_SBattRechrgSleep(void)
{
	if((0 == PP_SBRC.sleeptimestamp) || \
	   (0xffffffff == PP_SBRC.sleeptimestamp))
	{
		PP_SBRC.sleeptimestamp = PrvtPro_getTimestamp();
		cfg_set_para(CFG_ITEM_SLEEP_TS, &PP_SBRC.sleeptimestamp, 4);
		log_o(LOG_HOZON,"set sleep timestamp ok");
	}
}

/*
	唤醒设置
*/
void setPrvtPro_SBattRechrgWakeup(void)
{
	PP_SBRC.sleepflag = 0;
}
