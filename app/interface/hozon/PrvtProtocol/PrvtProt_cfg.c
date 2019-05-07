/******************************************************
文件名：	PrvtProt_cfg.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include "gps_api.h"
#include "../sockproxy/sockproxy_data.h"
#include "PrvtProt_cfg.h"
/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/


/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PrvtProt_rcvMsg
*形  参：
*返回值：
*描  述：读取数据
*备  注：
******************************************************/
int PrvtProtCfg_rcvMsg(unsigned char* buf,int buflen)
{
	return RdSockproxyData_Queue(SP_PRIV,buf,buflen);
}

/******************************************************
*函数名：PrvtProtCfg_ecallTriggerEvent
*形  参：
*返回值：
*描  述：读取ecall触发状态
*备  注：
******************************************************/
int PrvtProtCfg_ecallTriggerEvent(void)
{
	return 0;
}

/******************************************************
*函数名：PrvtProtCfg_engineSt
*形  参：
*返回值：
*描  述：读取发动机状态:1-熄火;2-启动
*备  注：
******************************************************/
int PrvtProtCfg_engineSt(void)
{
	return 2;
}

/******************************************************
*函数名：PrvtProtCfg_totalOdoMr
*形  参：
*返回值：
*描  述：读取里程
*备  注：
******************************************************/
int PrvtProtCfg_totalOdoMr(void)
{
	return 100;
}

/******************************************************
*函数名：PrvtProtCfg_gpsStatus
*形  参：
*返回值：
*描  述：读取gps状态
*备  注：
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
*函数名：PrvtProtCfg_gpsData
*形  参：
*返回值：
*描  述：读取gps数据
*备  注：
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
