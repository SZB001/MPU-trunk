/******************************************************
�ļ�����	PrvtProt_cfg.c

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
#include "gps_api.h"
#include "at.h"
#include "../sockproxy/sockproxy_rxdata.h"
#include "gb32960_api.h"
#include "PrvtProt_SigParse.h"
#include "PrvtProt_cfg.h"
static uint8_t ecall_trigger = 0;
static uint8_t bcall_trigger = 0;

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_rcvMsg
*��  �Σ�
*����ֵ��
*��  ������ȡ����
*��  ע��
******************************************************/
int PrvtProtCfg_rcvMsg(unsigned char* buf,int buflen)
{
	return RdSockproxyData_Queue(SP_PRIV,buf,buflen);
}

/******************************************************
*��������PrvtProtCfg_ecallTriggerEvent
*��  �Σ�
*����ֵ��
*��  ������ȡecall����״̬
*��  ע��
******************************************************/
char PrvtProtCfg_ecallTriggerEvent(void)
{
	if(ecall_trigger == 1)
	{
		ecall_trigger = 0;
		return 1;
	}
	return 0;
}

char PrvtProtCfg_bcallTriggerEvent(void)
{
	if(bcall_trigger == 1)
	{
		bcall_trigger = 0;
		return 1;
	}
	return 0;
}

char PrvtProtCfg_detectionTriggerSt(void)
{
	if(PrvtProt_SignParse_CO2DensitySt() == 2)
	{
		return 1;
	}
	
	return 0;
}

/******************************************************
*��������PrvtProtCfg_engineSt
*��  �Σ�
*����ֵ��
*��  ������ȡ������״̬:1-Ϩ��;2-����
*��  ע��
******************************************************/
long PrvtProtCfg_engineSt(void)
{
	uint8_t st;
	st = gb_data_vehicleState();
	if(2 ==  st)//����1��Ӧ����
	{
		return 2;
	}

	return 1;
}

/******************************************************
*��������PrvtProtCfg_totalOdoMr
*��  �Σ�
*����ֵ��
*��  ������ȡ���
*��  ע��
******************************************************/
long PrvtProtCfg_totalOdoMr(void)
{
	return gb_data_vehicleOdograph();
}
/******************************************************
*��������PrvtProtCfg_vehicleSOC
*��  �Σ�
*����ֵ��
*��  ������ȡ����
*��  ע��
******************************************************/
long PrvtProtCfg_vehicleSOC(void)
{
	long soc;
	soc = gb_data_vehicleSOC();
	if(soc < 0)
	{
		soc = 0;
	}

	if(soc > 100)
	{
		soc = 0;
	}
	return soc;
}

/******************************************************
*��������PrvtProtCfg_gpsStatus
*��  �Σ�
*����ֵ��
*��  ������ȡgps״̬
*��  ע��
******************************************************/
int PrvtProtCfg_gpsStatus(void)
{
	int ret = 0;
	if(gps_get_fix_status() == 2)
	{
		ret = 1;
	}
	
	return ret;	
}

/******************************************************
*��������PrvtProtCfg_gpsData
*��  �Σ�
*����ֵ��
*��  ������ȡgps����
*��  ע��
******************************************************/
void PrvtProtCfg_gpsData(PrvtProtcfg_gpsData_t *gpsDt)
{
	GPS_DATA gps_snap;

	gps_get_snap(&gps_snap);
	gpsDt->time = gps_snap.time;
	gpsDt->date = gps_snap.date;
	gpsDt->latitude = gps_snap.latitude;
	gpsDt->is_north = gps_snap.is_north;
	gpsDt->longitude = gps_snap.longitude;
	gpsDt->is_east = gps_snap.is_east;
	gpsDt->knots = gps_snap.knots;
	gpsDt->direction = gps_snap.direction;
	gpsDt->height = gps_snap.msl;
	gpsDt->hdop = gps_snap.hdop;
	gpsDt->kms = gps_snap.kms;
}

/******************************************************
*��������PrvtProtCfg_get_iccid
*��  �Σ�
*����ֵ��
*��  ������ȡgps����
*��  ע��
******************************************************/
int PrvtProtCfg_get_iccid(char *iccid)
{
	return at_get_iccid(iccid);
}

/*
 * 安全气囊状态
 */
uint8_t PrvtProtCfg_CrashOutputSt(void)
{
	if( gb_data_CrashOutputSt())
	{
		return 1;
	}
	return 0;
}

/*
 	快慢充电状态״̬
*/
uint8_t PrvtProtCfg_chargeSt(void)
{
	uint8_t tmp;
	uint8_t chargeSt = 0;
	tmp = gb_data_chargeSt();
	if((tmp == 0x1) || (tmp == 0x6))
	{
		chargeSt = 1;//慢充
	}
	else if(tmp == 0x2)
	{
		chargeSt = 2;//快充
	}
	else
	{}

	return chargeSt;
}
void PrvtProtCfg_ecallSt(uint8_t st)
{
	if(st == 1)
	{
		ecall_trigger = 1;
	}
}
void PrvtProtCfg_bcallSt(uint8_t st)
{
	if(st == 1)
	
{
		bcall_trigger = 1;
	}
}

/*
 	天窗状态״̬：打开、关闭、翘起
*/
uint8_t PrvtProtCfg_sunroofSt(void)
{
	unsigned char st;
	st = PrvtProt_SignParse_sunroofSt();
	if(st == 2)
	{
		return 0;//关闭
	}
	else if((st == 3) || (st == 4))
	{
		return 1;//开启
	}
	else if((st == 0) || (st == 1))
	{
		return 2;//翘起
	}

	 return 0;
}

uint8_t PrvtProtCfg_reardoorSt(void)
{
	unsigned char st;
	st = gb_data_reardoorSt();
	if(st)
	{
		return 1;
	}

	return 0;
}

/*续航里程*/
long PrvtProtCfg_ResidualOdometer(void)
{
	long Odometer;
	Odometer = gb_data_ResidualOdometer();
	return (Odometer > 2000)?2000:Odometer;
}

/*充电保持时间*/
long PrvtProtCfg_ACChargeRemainTime(void)
{
	long ChargeRemainTime;

	ChargeRemainTime = gb_data_ACChargeRemainTime();

	return (ChargeRemainTime > 2000)?2000:ChargeRemainTime;
}

/*胎压*/
long PrvtProtCfg_TyrePre(uint8_t obj)
{
	long TyrePre = 0;
	switch(obj)
	{
		case 1:
		{
			TyrePre = gb_data_frontRightTyrePre();/* 右前胎压 */
		}
		break;
		case 2:
		{
			TyrePre = gb_data_frontLeftTyrePre();/* 左前胎压 */
		}
		break;
		case 3:
		{
			TyrePre = gb_data_rearRightTyrePre()/* 右后胎压 */;
		}
		break;
		case 4:
		{
			TyrePre = gb_data_rearLeftTyrePre()/* 左后胎压 */;
		}
		break;
		default:
		break;
	}

	return (TyrePre > 45)?45:TyrePre;
}

/*胎温*/
long PrvtProtCfg_TyreTemp(uint8_t obj)
{
	long TyreTemp = 0;
	
	switch(obj)
	{
		case 1:
		{
			TyreTemp = gb_data_frontRightTyreTemp();
		}
		break;
		case 2:
		{
			TyreTemp = gb_data_frontLeftTyreTemp();
		}
		break;
		case 3:
		{
			TyreTemp = gb_data_rearRightTyreTemp();
		}
		break;
		case 4:
		{
			TyreTemp = gb_data_rearLeftTyreTemp()/* OPTIONAL */;
		}
		break;
		default:
		break;
	}

	return (TyreTemp > 165)?165:TyreTemp;
}