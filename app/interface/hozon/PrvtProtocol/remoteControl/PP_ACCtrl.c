/******************************************************
文件名：	PP_ACCtrl.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
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

#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "../sockproxy/sockproxy_data.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "PP_ACCtrl.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtACCtrl_pack_t; /**/

typedef struct
{
	PP_rmtACCtrl_pack_t 	pack;
	PP_rmtACCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtACCtrl_t; /*结构体*/

static PrvtProt_rmtACCtrl_t PP_rmtACCtrl;
static PrvtProt_pack_t 		PP_rmtACCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 		App_rmtACCtrl;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_ACCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtACCtrl_t *rmtCtrl);
static int PP_ACCtrl_BookingStatusResp(PrvtProt_task_t *task,PrvtProt_rmtACCtrl_t *rmtACCtrl);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_ACCtrl_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_ACCtrl_init(void)
{
	memset(&PP_rmtACCtrl,0,sizeof(PrvtProt_rmtACCtrl_t));
	memcpy(PP_rmtACCtrl.pack.Header.sign,"**",2);
	PP_rmtACCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtACCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtACCtrl.pack.Header.opera = 0x02;
	PP_rmtACCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtACCtrl.pack.DisBody.aID,"110",3);
	PP_rmtACCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtACCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtACCtrl.pack.DisBody.testFlag = 1;
}

/******************************************************
*函数名：PP_ACCtrl_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_ACCtrl_mainfunction(void *task)
{
	int res;
	static uint32_t stitch = 0;
	static char result = 0;
	if(PP_rmtACCtrl.state.req == 1)
	{
		stitch++;
		PP_rmtACCtrl.state.req = 0;
		if(PP_rmtACCtrl.state.bookingSt == 1)
		{
			PP_rmtACCtrl.state.executSt = 0;
		}
		else
		{
			PP_rmtACCtrl.state.executSt = 1;
		}

		res = PP_ACCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtACCtrl);
		if(res < 0)//请求发送失败
		{
			log_e(LOG_HOZON, "socket send error, reset protocol");
			sockproxy_socketclose();//by liujian 20190514
		}
		else if(res > 0)
		{
			log_e(LOG_HOZON, "socket send ok, reset");
			result = 1;
			PP_rmtACCtrl.state.period = tm_get_time();
		}
		else
		{}
	}

	if(1 == result)
	{
		if((tm_get_time() - PP_rmtACCtrl.state.period) > 3000)
		{
			result = 0;
			if(PP_rmtACCtrl.state.bookingSt == 1)
			{
				(void)PP_ACCtrl_BookingStatusResp((PrvtProt_task_t *)task,&PP_rmtACCtrl);
			}
			else
			{
				if(stitch%2)//返回执行成功
				{
					PP_rmtACCtrl.state.executSt = 2;
					(void)PP_ACCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtACCtrl);
				}
				else//返回执行失败
				{
					PP_rmtACCtrl.state.executSt = 3;
					(void)PP_ACCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtACCtrl);
				}
			}
		}
	}


	return res;
}


/******************************************************
*函数名：PP_ACCtrl_StatusResp

*形  参：

*返回值：

*描  述：remote control status response

*备  注：
******************************************************/
static int PP_ACCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtACCtrl_t *rmtACCtrl)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtACCtrl->pack.Header.ver.Byte = task->version;
	rmtACCtrl->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtACCtrl->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtACCtrl_Pack, &rmtACCtrl->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtACCtrl->pack.DisBody.mID = PP_MID_RMTCTRL_RESP;
	//rmtdoorCtrl->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtACCtrl->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PrvtProtcfg_gpsData_t gpsDt;
	App_rmtACCtrl.CtrlResp.rvcReqType = PP_rmtACCtrl.state.reqType;
	App_rmtACCtrl.CtrlResp.rvcReqStatus = PP_rmtACCtrl.state.executSt;
	App_rmtACCtrl.CtrlResp.rvcFailureType = 0;
	App_rmtACCtrl.CtrlResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps状态 0-无效；1-有效;
	App_rmtACCtrl.CtrlResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gps时间戳:系统时间(通过gps校时)

	PrvtProtCfg_gpsData(&gpsDt);

	if(App_rmtACCtrl.CtrlResp.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			App_rmtACCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//纬度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			App_rmtACCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//纬度 x 1000000,当GPS信号无效时，值为0
		}

		if(gpsDt.is_east)
		{
			App_rmtACCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//经度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			App_rmtACCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//经度 x 1000000,当GPS信号无效时，值为0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtACCtrl.CtrlResp.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtACCtrl.CtrlResp.gpsPos.longitude);
	}
	else
	{
		App_rmtACCtrl.CtrlResp.gpsPos.latitude  = 0;
		App_rmtACCtrl.CtrlResp.gpsPos.longitude = 0;
	}
	App_rmtACCtrl.CtrlResp.gpsPos.altitude = (long)(gpsDt.height);//高度（m）
	if(App_rmtACCtrl.CtrlResp.gpsPos.altitude > 10000)
	{
		App_rmtACCtrl.CtrlResp.gpsPos.altitude = 10000;
	}
	App_rmtACCtrl.CtrlResp.gpsPos.heading = (long)(gpsDt.direction);//车头方向角度，0为正北方向
	App_rmtACCtrl.CtrlResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//速度 x 10，单位km/h
	App_rmtACCtrl.CtrlResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//水平精度因子 x 10
	if(App_rmtACCtrl.CtrlResp.gpsPos.hdop > 1000)
	{
		App_rmtACCtrl.CtrlResp.gpsPos.hdop = 1000;
	}

	App_rmtACCtrl.CtrlResp.basicSt.driverDoor = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.driverLock = 1;
	App_rmtACCtrl.CtrlResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.passengerLock = 1;
	App_rmtACCtrl.CtrlResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearLeftLock = 1;
	App_rmtACCtrl.CtrlResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearRightLock = 1;
	App_rmtACCtrl.CtrlResp.basicSt.bootStatus = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.bootStatusLock = 1;
	App_rmtACCtrl.CtrlResp.basicSt.driverWindow = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.passengerWindow = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearLeftWindow = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearRightWinow = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.sunroofStatus = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.engineStatus = 1;
	App_rmtACCtrl.CtrlResp.basicSt.accStatus = 1;
	App_rmtACCtrl.CtrlResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
	App_rmtACCtrl.CtrlResp.basicSt.accMode = 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.accBlowVolume	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.innerTemp = 1;
	App_rmtACCtrl.CtrlResp.basicSt.outTemp = 1;
	App_rmtACCtrl.CtrlResp.basicSt.sideLightStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.dippedBeamStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.mainBeamStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.hazardLightStus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.frtRightTyrePre	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.frtRightTyreTemp	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.frontLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.frontLeftTyreTemp= 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearRightTyrePre= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearRightTyreTemp= 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.rearLeftTyreTemp	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.batterySOCExact= 1;
	App_rmtACCtrl.CtrlResp.basicSt.chargeRemainTim	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.availableOdomtr= 1;
	App_rmtACCtrl.CtrlResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.bookingChargeSt= 1;
	App_rmtACCtrl.CtrlResp.basicSt.bookingChargeHour= 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.bookingChargeMin	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.chargeMode	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.chargeStatus	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.powerMode	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.speed= 1;
	App_rmtACCtrl.CtrlResp.basicSt.totalOdometer= 1;
	App_rmtACCtrl.CtrlResp.basicSt.batteryVoltage= 1;
	App_rmtACCtrl.CtrlResp.basicSt.batteryCurrent= 1;
	App_rmtACCtrl.CtrlResp.basicSt.batterySOCPrc= 1;
	App_rmtACCtrl.CtrlResp.basicSt.dcStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.gearPosition= 1;
	App_rmtACCtrl.CtrlResp.basicSt.insulationRstance= 1;
	App_rmtACCtrl.CtrlResp.basicSt.acceleratePedalprc= 1;
	App_rmtACCtrl.CtrlResp.basicSt.deceleratePedalprc= 1;
	App_rmtACCtrl.CtrlResp.basicSt.canBusActive= 1;
	App_rmtACCtrl.CtrlResp.basicSt.bonnetStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.lockStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.gsmStatus= 1;
	App_rmtACCtrl.CtrlResp.basicSt.wheelTyreMotrSt= 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.vehicleAlarmSt= 1;
	App_rmtACCtrl.CtrlResp.basicSt.currentJourneyID= 1;
	App_rmtACCtrl.CtrlResp.basicSt.journeyOdom= 1;
	App_rmtACCtrl.CtrlResp.basicSt.frtLeftSeatHeatLel= 1	/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.frtRightSeatHeatLel	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.airCleanerSt	= 1/* OPTIONAL */;
	App_rmtACCtrl.CtrlResp.basicSt.srsStatus = 1;


	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_RESP,PP_rmtACCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtACCtrl->pack.DisBody,&App_rmtACCtrl))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtACCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtACCtrl_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_control_response", PP_rmtACCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}


/******************************************************
*函数名：PP_ACCtrl_BookingStatusResp

*形  参：

*返回值：

*描  述：remote control booking status response

*备  注：
******************************************************/
static int PP_ACCtrl_BookingStatusResp(PrvtProt_task_t *task,PrvtProt_rmtACCtrl_t *rmtACCtrl)
{
	int msgdatalen;
	int res;

	/*header*/
	rmtACCtrl->pack.Header.ver.Byte = task->version;
	rmtACCtrl->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtACCtrl->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtACCtrl_Pack, &rmtACCtrl->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtACCtrl->pack.DisBody.mID = PP_MID_RMTCTRL_BOOKINGRESP;
	//rmtdoorCtrl->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtACCtrl->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	PrvtProt_App_rmtCtrl_t app_rmtCtrl;
	/*appdata*/
	app_rmtCtrl.CtrlbookingResp.bookingId = 1;
	app_rmtCtrl.CtrlbookingResp.rvcReqCode = rmtACCtrl->state.rvcReqCode;
	app_rmtCtrl.CtrlbookingResp.oprTime = PrvtPro_getTimestamp();

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_BOOKINGRESP,PP_rmtACCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtACCtrl->pack.DisBody,&app_rmtCtrl))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtACCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtACCtrl_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_control_booking_response", PP_rmtACCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}



/******************************************************
*函数名：SetPP_ACCtrl_Request

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
void SetPP_ACCtrl_Request(void *appdatarmtCtrl,void *disptrBody)
{
	PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
	PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
	static char rvcReqCode = 0;
	log_i(LOG_HOZON, "remote AC control req");
	PP_rmtACCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
	PP_rmtACCtrl.state.req = 1;
	PP_rmtACCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
	PP_rmtACCtrl.state.bookingSt = 0;
	if(appdatarmtCtrl_ptr->CtrlReq.rvcReqParamslen > 0)//有预约
	{
		PP_rmtACCtrl.state.bookingSt = 1;
		switch(rvcReqCode)
		{
			case 0:
			{
				PP_rmtACCtrl.state.rvcReqCode = 0x0610;
			}
			break;
			case 1:
			{
				PP_rmtACCtrl.state.rvcReqCode = 0x0611;
			}
			break;
			case 2:
			{
				PP_rmtACCtrl.state.rvcReqCode = 0x0600;
			}
			break;
			case 3:
			{
				PP_rmtACCtrl.state.rvcReqCode = 0x0601;
			}
			break;
			default:
			{
				rvcReqCode = 0;
				PP_rmtACCtrl.state.rvcReqCode = 0x0610;
			}
			break;
		}
		rvcReqCode++;
	}
}

/******************************************************
*函数名：PP_ACCtrl_SetCtrlReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_ACCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtACCtrl.state.reqType = (long)reqType;
	PP_rmtACCtrl.state.req = 1;
}

