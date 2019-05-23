/******************************************************
文件名：	PP_doorLockCtrl.c

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
//#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "PP_doorLockCtrl.h"

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
}__attribute__((packed))  PP_rmtdoorCtrl_pack_t; /**/

typedef struct
{
	PP_rmtdoorCtrl_pack_t 	pack;
	PP_rmtdoorCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtdoorCtrl_t; /*结构体*/

static PrvtProt_rmtdoorCtrl_t PP_rmtdoorCtrl;
static PrvtProt_pack_t 		PP_rmtdoorCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 		App_rmtdoorCtrl;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_doorLockCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtdoorCtrl_t *rmtCtrl);

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_rmtCtrl_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_doorLockCtrl_init(void)
{
	memset(&PP_rmtdoorCtrl,0,sizeof(PrvtProt_rmtdoorCtrl_t));
	memcpy(PP_rmtdoorCtrl.pack.Header.sign,"**",2);
	PP_rmtdoorCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtdoorCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtdoorCtrl.pack.Header.opera = 0x02;
	PP_rmtdoorCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtdoorCtrl.pack.DisBody.aID,"110",3);
	PP_rmtdoorCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtdoorCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtdoorCtrl.pack.DisBody.testFlag = 1;
}

/******************************************************
*函数名：PP_doorLockCtrl_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_doorLockCtrl_mainfunction(void *task)
{
	int res;
	if(PP_rmtdoorCtrl.state.req == 1)
	{
		res = PP_doorLockCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtdoorCtrl);
		if(res < 0)//请求发送失败
		{
			log_e(LOG_HOZON, "socket send error, reset protocol");
			sockproxy_socketclose();//by liujian 20190514
			//PP_rmtdoorCtrl.state.req = 1;
		}
		else if(res > 0)
		{
			log_e(LOG_HOZON, "socket send ok, reset");
			PP_rmtdoorCtrl.state.req = 0;
		}
		else
		{
			PP_rmtdoorCtrl.state.req = 0;
		}
	}
	return res;
}


/******************************************************
*函数名：PP_doorLockCtrl_StatusResp

*形  参：

*返回值：

*描  述：remote control status response

*备  注：
******************************************************/
static int PP_doorLockCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtdoorCtrl_t *rmtdoorCtrl)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtdoorCtrl->pack.Header.ver.Byte = task->version;
	rmtdoorCtrl->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtdoorCtrl->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtdoorCtrl_Pack, &rmtdoorCtrl->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtdoorCtrl->pack.DisBody.mID = PP_MID_RMTCTRL_RESP;
	//rmtdoorCtrl->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtdoorCtrl->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtdoorCtrl->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtdoorCtrl->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PrvtProtcfg_gpsData_t gpsDt;
	App_rmtdoorCtrl.CtrlResp.rvcReqType = PP_rmtdoorCtrl.state.reqType;
	App_rmtdoorCtrl.CtrlResp.rvcReqStatus = 0;
	App_rmtdoorCtrl.CtrlResp.rvcFailureType = 0;
	App_rmtdoorCtrl.CtrlResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps状态 0-无效；1-有效;
	App_rmtdoorCtrl.CtrlResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gps时间戳:系统时间(通过gps校时)

	PrvtProtCfg_gpsData(&gpsDt);

	if(App_rmtdoorCtrl.CtrlResp.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			App_rmtdoorCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//纬度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			App_rmtdoorCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//纬度 x 1000000,当GPS信号无效时，值为0
		}

		if(gpsDt.is_east)
		{
			App_rmtdoorCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//经度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			App_rmtdoorCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//经度 x 1000000,当GPS信号无效时，值为0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtdoorCtrl.CtrlResp.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtdoorCtrl.CtrlResp.gpsPos.longitude);
	}
	else
	{
		App_rmtdoorCtrl.CtrlResp.gpsPos.latitude  = 0;
		App_rmtdoorCtrl.CtrlResp.gpsPos.longitude = 0;
	}
	App_rmtdoorCtrl.CtrlResp.gpsPos.altitude = (long)(gpsDt.height);//高度（m）
	if(App_rmtdoorCtrl.CtrlResp.gpsPos.altitude > 10000)
	{
		App_rmtdoorCtrl.CtrlResp.gpsPos.altitude = 10000;
	}
	App_rmtdoorCtrl.CtrlResp.gpsPos.heading = (long)(gpsDt.direction);//车头方向角度，0为正北方向
	App_rmtdoorCtrl.CtrlResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//速度 x 10，单位km/h
	App_rmtdoorCtrl.CtrlResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//水平精度因子 x 10
	if(App_rmtdoorCtrl.CtrlResp.gpsPos.hdop > 1000)
	{
		App_rmtdoorCtrl.CtrlResp.gpsPos.hdop = 1000;
	}

	App_rmtdoorCtrl.CtrlResp.basicSt.driverDoor = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.driverLock = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.passengerLock = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearLeftLock = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearRightLock = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.bootStatus = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.bootStatusLock = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.driverWindow = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.passengerWindow = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearLeftWindow = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearRightWinow = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.sunroofStatus = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.engineStatus = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.accStatus = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
	App_rmtdoorCtrl.CtrlResp.basicSt.accMode = 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.accBlowVolume	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.innerTemp = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.outTemp = 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.sideLightStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.dippedBeamStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.mainBeamStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.hazardLightStus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.frtRightTyrePre	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.frtRightTyreTemp	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.frontLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.frontLeftTyreTemp= 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearRightTyrePre= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearRightTyreTemp= 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.rearLeftTyreTemp	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.batterySOCExact= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.chargeRemainTim	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.availableOdomtr= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.bookingChargeSt= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.bookingChargeHour= 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.bookingChargeMin	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.chargeMode	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.chargeStatus	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.powerMode	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.speed= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.totalOdometer= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.batteryVoltage= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.batteryCurrent= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.batterySOCPrc= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.dcStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.gearPosition= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.insulationRstance= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.acceleratePedalprc= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.deceleratePedalprc= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.canBusActive= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.bonnetStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.lockStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.gsmStatus= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.wheelTyreMotrSt= 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.vehicleAlarmSt= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.currentJourneyID= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.journeyOdom= 1;
	App_rmtdoorCtrl.CtrlResp.basicSt.frtLeftSeatHeatLel= 1	/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.frtRightSeatHeatLel	= 1/* OPTIONAL */;
	App_rmtdoorCtrl.CtrlResp.basicSt.airCleanerSt	= 1/* OPTIONAL */;


	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_RESP,PP_rmtdoorCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtdoorCtrl->pack.DisBody,&App_rmtdoorCtrl))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtdoorCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtdoorCtrl_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_control_response", PP_rmtdoorCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}


/******************************************************
*函数名：SetPP_doorLockCtrl_Request

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
void SetPP_doorLockCtrl_Request(void *appdatarmtCtrl,void *disptrBody)
{
	PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
	PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
	log_i(LOG_HOZON, "remote door lock control req");
	PP_rmtdoorCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
	PP_rmtdoorCtrl.state.req = 1;
	PP_rmtdoorCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
}

/******************************************************
*函数名：PP_doorLockCtrl_SetCtrlReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_doorLockCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtdoorCtrl.state.reqType = (long)reqType;
	PP_rmtdoorCtrl.state.req = 1;
}

