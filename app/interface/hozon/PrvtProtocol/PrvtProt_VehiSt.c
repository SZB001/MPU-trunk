/******************************************************
文件名：	PrvtProt_VehiSt.c

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
#include "XcallReqinfo.h"
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
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "PrvtProt_VehiSt.h"

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
}__attribute__((packed))  PrvtProt_VS_pack_t; /**/

typedef struct
{
	PrvtProt_VS_pack_t 		pack;
	PrvtProt_VSSt_t	 		state;
}__attribute__((packed))  PrvtProt_VS_t; /*结构体*/

static PrvtProt_pack_t 		PP_VS_Pack;
static PrvtProt_VS_t		PP_rmtVS;
static PrvtProt_App_VS_t	PP_VS_appdata;

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_VS_do_checksock(PrvtProt_task_t *task);
static int PP_VS_do_rcvMsg(PrvtProt_task_t *task);
static void PP_VS_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_VS_do_wait(PrvtProt_task_t *task);
static int PP_VS_do_VehiStMainfunction(PrvtProt_task_t *task);
static int PP_VS_VehiStatusResp(PrvtProt_task_t *task,PrvtProt_VS_t *rmtVS);

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_VS_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_VS_init(void)
{
	memset(&PP_rmtVS,0 , sizeof(PrvtProt_VS_t));
	memcpy(PP_rmtVS.pack.Header.sign,"**",2);
	PP_rmtVS.pack.Header.ver.Byte = 0x30;
	PP_rmtVS.pack.Header.commtype.Byte = 0xe1;
	PP_rmtVS.pack.Header.opera = 0x02;
	PP_rmtVS.pack.Header.tboxid = 27;
	memcpy(PP_rmtVS.pack.DisBody.aID,"130",strlen("130"));
	PP_rmtVS.pack.DisBody.eventId = PP_INIT_EVENTID;
	PP_rmtVS.pack.DisBody.appDataProVer = 256;
	PP_rmtVS.pack.DisBody.testFlag = 1;
	PP_rmtVS.state.req = PP_VS_NOREQ;
}

/******************************************************
*函数名：PP_VS_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_VS_mainfunction(void *task)
{
	int res;
	res = 		PP_VS_do_checksock((PrvtProt_task_t*)task) ||
				PP_VS_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_VS_do_wait((PrvtProt_task_t*)task) ||
				PP_VS_do_VehiStMainfunction((PrvtProt_task_t*)task);
	return res;
}

/******************************************************
*函数名：PP_VS_do_checksock

*形  参：void

*返回值：void

*描  述：检查socket连接

*备  注：
******************************************************/
static int PP_VS_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}
	return -1;
}

/******************************************************
*函数名：PP_VS_do_rcvMsg

*形  参：void

*返回值：void

*描  述：接收数据函数

*备  注：
******************************************************/
static int PP_VS_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_REMOTE_VS,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive vehi status message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//判断数据帧头有误或者数据长度不对
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//接收数据长度超出缓存buffer长度
	{
		return 0;
	}
	PP_VS_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*函数名：PP_VS_RxMsgHandle

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PP_VS_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_VS_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_VS != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	switch(MsgDataBody.mID)
	{
		case PP_MID_VS_REQ://收到tsp查询请求
		{
			PP_rmtVS.state.req = Appdata.VSReq.vehStatusReqType;
			PP_rmtVS.pack.DisBody.eventId = MsgDataBody.eventId;
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_xcall_do_wait

*形  参：void

*返回值：void

*描  述：检查是否有事件等待应答

*备  注：
******************************************************/
static int PP_VS_do_wait(PrvtProt_task_t *task)
{
	return 0;
}

/******************************************************
*函数名：PP_xcall_do_checkXcall

*形  参：

*返回值：

*描  述：检查ecall等请求

*备  注：
******************************************************/
static int PP_VS_do_VehiStMainfunction(PrvtProt_task_t *task)
{
	int res;

	if(1 != sockproxy_socketState())//socket not open
	{
		return 0;
		PP_rmtVS.state.req = PP_VS_NOREQ;
	}

	switch(PP_rmtVS.state.req)
	{
		case PP_VS_NOREQ:
		break;
		case PP_VS_BASICSTATUS:
		{
			PP_VS_appdata.VSResp.ExtSt.validFlg = 0;//不上报ext status数据
			res = PP_VS_VehiStatusResp(task,&PP_rmtVS);
			if(res < 0)//请求发送失败
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				PP_rmtVS.state.req = PP_VS_NOREQ;
				sockproxy_socketclose();//by liujian 20190514
			}
			else if(res > 0)
			{
				log_e(LOG_HOZON, "socket send ok, reset");
				PP_rmtVS.state.req = PP_VS_NOREQ;
			}
			else
			{}
		}
		break;
		case PP_VS_EXTSTATUS:
		{
			PP_VS_appdata.VSResp.ExtSt.validFlg = 1;//上报ext status数据
			res = PP_VS_VehiStatusResp(task,&PP_rmtVS);
			if(res < 0)//请求发送失败
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				PP_rmtVS.state.req = PP_VS_NOREQ;
				sockproxy_socketclose();//by liujian 20190514
			}
			else if(res > 0)
			{
				log_e(LOG_HOZON, "socket send ok, reset");
				PP_rmtVS.state.req = PP_VS_NOREQ;
			}
			else
			{}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*函数名：PP_VS_VehiStatusResp

*形  参：

*返回值：

*描  述：remote vehi status response

*备  注：
******************************************************/
static int PP_VS_VehiStatusResp(PrvtProt_task_t *task,PrvtProt_VS_t *rmtVS)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtVS->pack.Header.ver.Byte = task->version;
	rmtVS->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtVS->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_VS_Pack, &rmtVS->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtVS->pack.DisBody.mID = PP_MID_VS_RESP;
	//rmtVS->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtVS->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtVS->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtVS->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PrvtProtcfg_gpsData_t gpsDt;
	PP_VS_appdata.VSResp.statusTime = PrvtPro_getTimestamp();//0表示立即执行

	PP_VS_appdata.VSResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps状态 0-无效；1-有效;
	PP_VS_appdata.VSResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gps时间戳:系统时间(通过gps校时)

	PrvtProtCfg_gpsData(&gpsDt);

	if(PP_VS_appdata.VSResp.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			PP_VS_appdata.VSResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//纬度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			PP_VS_appdata.VSResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//纬度 x 1000000,当GPS信号无效时，值为0
		}

		if(gpsDt.is_east)
		{
			PP_VS_appdata.VSResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//经度 x 1000000,当GPS信号无效时，值为0
		}
		else
		{
			PP_VS_appdata.VSResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//经度 x 1000000,当GPS信号无效时，值为0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",PP_VS_appdata.VSResp.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",PP_VS_appdata.VSResp.gpsPos.longitude);
	}
	else
	{
		PP_VS_appdata.VSResp.gpsPos.latitude  = 0;
		PP_VS_appdata.VSResp.gpsPos.longitude = 0;
	}
	PP_VS_appdata.VSResp.gpsPos.altitude = (long)(gpsDt.height);//高度（m）
	if(PP_VS_appdata.VSResp.gpsPos.altitude > 10000)
	{
		PP_VS_appdata.VSResp.gpsPos.altitude = 10000;
	}
	PP_VS_appdata.VSResp.gpsPos.heading = (long)(gpsDt.direction);//车头方向角度，0为正北方向
	PP_VS_appdata.VSResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//速度 x 10，单位km/h
	PP_VS_appdata.VSResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//水平精度因子 x 10
	if(PP_VS_appdata.VSResp.gpsPos.hdop > 1000)
	{
		PP_VS_appdata.VSResp.gpsPos.hdop = 1000;
	}

	PP_VS_appdata.VSResp.basicSt.driverDoor = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.driverLock = 1;
	PP_VS_appdata.VSResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.passengerLock = 1;
	PP_VS_appdata.VSResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearLeftLock = 1;
	PP_VS_appdata.VSResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearRightLock = 1;
	PP_VS_appdata.VSResp.basicSt.bootStatus = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.bootStatusLock = 1;
	PP_VS_appdata.VSResp.basicSt.driverWindow = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.passengerWindow = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearLeftWindow = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearRightWinow = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.sunroofStatus = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.engineStatus = 1;
	PP_VS_appdata.VSResp.basicSt.accStatus = 1;
	PP_VS_appdata.VSResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
	PP_VS_appdata.VSResp.basicSt.accMode = 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.accBlowVolume	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.innerTemp = 1;
	PP_VS_appdata.VSResp.basicSt.outTemp = 1;
	PP_VS_appdata.VSResp.basicSt.sideLightStatus= 1;
	PP_VS_appdata.VSResp.basicSt.dippedBeamStatus= 1;
	PP_VS_appdata.VSResp.basicSt.mainBeamStatus= 1;
	PP_VS_appdata.VSResp.basicSt.hazardLightStus= 1;
	PP_VS_appdata.VSResp.basicSt.frtRightTyrePre	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.frtRightTyreTemp	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.frontLeftTyrePre	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.frontLeftTyreTemp= 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearRightTyrePre= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearRightTyreTemp= 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearLeftTyrePre	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.rearLeftTyreTemp	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.batterySOCExact= 1;
	PP_VS_appdata.VSResp.basicSt.chargeRemainTim	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.availableOdomtr= 1;
	PP_VS_appdata.VSResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.bookingChargeSt= 1;
	PP_VS_appdata.VSResp.basicSt.bookingChargeHour= 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.bookingChargeMin	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.chargeMode	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.chargeStatus	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.powerMode	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.speed= 1;
	PP_VS_appdata.VSResp.basicSt.totalOdometer= 1;
	PP_VS_appdata.VSResp.basicSt.batteryVoltage= 1;
	PP_VS_appdata.VSResp.basicSt.batteryCurrent= 1;
	PP_VS_appdata.VSResp.basicSt.batterySOCPrc= 1;
	PP_VS_appdata.VSResp.basicSt.dcStatus= 1;
	PP_VS_appdata.VSResp.basicSt.gearPosition= 1;
	PP_VS_appdata.VSResp.basicSt.insulationRstance= 1;
	PP_VS_appdata.VSResp.basicSt.acceleratePedalprc= 1;
	PP_VS_appdata.VSResp.basicSt.deceleratePedalprc= 1;
	PP_VS_appdata.VSResp.basicSt.canBusActive= 1;
	PP_VS_appdata.VSResp.basicSt.bonnetStatus= 1;
	PP_VS_appdata.VSResp.basicSt.lockStatus= 1;
	PP_VS_appdata.VSResp.basicSt.gsmStatus= 1;
	PP_VS_appdata.VSResp.basicSt.wheelTyreMotrSt= 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.vehicleAlarmSt= 1;
	PP_VS_appdata.VSResp.basicSt.currentJourneyID= 1;
	PP_VS_appdata.VSResp.basicSt.journeyOdom= 1;
	PP_VS_appdata.VSResp.basicSt.frtLeftSeatHeatLel= 1	/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.frtRightSeatHeatLel	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.airCleanerSt	= 1/* OPTIONAL */;
	PP_VS_appdata.VSResp.basicSt.srsStatus = 1;

	if(PP_VS_appdata.VSResp.ExtSt.validFlg == 1)//上报ext status数据
	{
		PP_VS_appdata.VSResp.ExtSt.alertSize = 1;//
		PP_VS_appdata.VSResp.ExtSt.alertIds[0] = 1;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTVS_RESP,PP_VS_Pack.msgdata,&msgdatalen,\
									   &rmtVS->pack.DisBody,&PP_VS_appdata))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_VS_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_VS_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_VS_response", PP_VS_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名：PP_VS_SetVSReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_VS_SetVSReq(unsigned char req)
{
	PP_rmtVS.state.req = req;
	PP_rmtVS.pack.DisBody.eventId = 130+2;
}

