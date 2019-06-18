/******************************************************
�ļ�����	PrvtProt_cfg.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include "gps_api.h"
#include "at.h"
#include "../sockproxy/sockproxy_data.h"
#include "PrvtProt_cfg.h"
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
int PrvtProtCfg_ecallTriggerEvent(void)
{
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
	long st;
	st = gb_data_vehicleState();
	if(1 ==  st)//����1��Ӧ����
	{
		st = 2;
	}
	else
	{
		st = 1;
	}
	return st;
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
	if(soc > 100)
	{
		soc = 100;
	}
	return (long)(soc*100);
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
