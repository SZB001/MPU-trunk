/******************************************************
文件名：	PrvtProt_rmtCtrl.c

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

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt_cfg.h"
#include "PP_doorLockCtrl.h"
#include "PP_ACCtrl.h"
#include "PP_ChargeCtrl.h"
#include "PP_identificat.h"
#include "../PrvtProt.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "PP_autodoorCtrl.h"
#include "PP_searchvehicle.h"
#include "PP_sunroofCtrl.h"
#include "PP_StartEngine.h"
#include "PP_StartForbid.h"
#include "PP_SeatHeating.h"
#include "PP_rmtCtrl.h"

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
}__attribute__((packed))  PrvtProt_rmtCtrl_pack_t; /**/

typedef struct
{
	PrvtProt_rmtCtrl_pack_t pack;
	PrvtProt_rmtCtrlSt_t	state[RMTCTRL_OBJ_MAX];
	long reqType;//请求类型
	long eventid;//事件id
}__attribute__((packed))  PrvtProt_rmtCtrl_t; /*结构体*/

static PrvtProt_rmtCtrl_t PP_rmtCtrl;
static PrvtProt_pack_t 		PP_rmtCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 		App_rmtCtrl;

static PrvtProt_RmtCtrlFunc_t PP_RmtCtrlFunc[RMTCTRL_OBJ_MAX] =
{
	{RMTCTRL_DOORLOCK,PP_doorLockCtrl_init,	PP_doorLockCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,PP_sunroofctrl_init,	PP_sunroofctrl_mainfunction},
	{RMTCTRL_AUTODOOR,PP_autodoorCtrl_init,	PP_autodoorCtrl_mainfunction},
	{RMTCTRL_RMTSRCHVEHICLE,PP_searchvehicle_init,	PP_searchvehicle_mainfunction},
	{RMTCTRL_HIGHTENSIONCTRL,PP_startengine_init,PP_startengine_mainfunction},
	{RMTCTRL_AC,	  NULL, 		NULL},
	{RMTCTRL_CHARGE,  PP_ChargeCtrl_init,	PP_ChargeCtrl_mainfunction},
	{RMTCTRL_ENGINECTRL,	  PP_startforbid_init, 		PP_startforbid_mainfunction},
	{RMTCTRL_SEATHEATINGCTRL,	  PP_seatheating_init, 		PP_seatheating_mainfunction}
};

static int PP_rmtCtrl_flag = 0;

static PrvtProt_TxInform_t rmtCtrl_TxInform;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
//static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task);
//static int PP_rmtCtrl_StatusResp(long bookingId,unsigned int reqtype);

static void PP_rmtCtrl_send_cb(void * para);
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
void PP_rmtCtrl_init(void)
{
	int i;
	memset(&PP_rmtCtrl,0,sizeof(PrvtProt_rmtCtrl_t));
	PP_canSend_init();
	for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
	{
		PP_rmtCtrl.state[i].reqType = PP_RMTCTRL_UNKNOW;
		if(PP_RmtCtrlFunc[i].Init != NULL)
		{
			PP_RmtCtrlFunc[i].Init();
		}
	}

}

/******************************************************
*函数名：PP_rmtCtrl_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_rmtCtrl_mainfunction(void *task)
{
	int res;
	int i;
	res = 		PP_rmtCtrl_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_rcvMsg((PrvtProt_task_t*)task);

	switch(PP_rmtCtrl_flag)
	{
		case RMTCTRL_IDLE://空闲
		{
			int ret ;
			PP_rmtCtrl_checkenginetime();//15分钟之后下高压电
			//检测空调或座椅加热上电或下电

			PP_ChargeCtrl_chargeStMonitor();//监测充电状态

			ret  = PP_doorLockCtrl_start() ||
				   PP_autodoorCtrl_start() ||
				   PP_searchvehicle_start()||
				   PP_sunroofctrl_start()  ||
				   PP_startengine_start()  ||
				   PP_seatheating_start()  ||
				   PP_startforbid_start();
			if(ret == 1)
			{
				PP_rmtCtrl_flag = RMTCTRL_IDENTIFICAT_QUERY;
			}
		}
		break;
		case RMTCTRL_IDENTIFICAT_QUERY://检查认证
		{
			if(PP_get_identificat_flag() == 1)	
			{
				PP_rmtCtrl_flag = RMTCTRL_COMMAND_LAUNCH;
			}
			else
			{
				PP_rmtCtrl_flag = RMTCTRL_IDENTIFICAT_LAUNCH;
			}
		}
		break;
		case RMTCTRL_IDENTIFICAT_LAUNCH://认证
		{
			int  authst;
			authst = PP_identificat_mainfunction();
   			if(PP_AUTH_SUCCESS == authst)
   			{
  				PP_rmtCtrl_flag = RMTCTRL_COMMAND_LAUNCH;
				log_o(LOG_HOZON,"-------identificat success---------");
  			}
  			else if(PP_AUTH_FAIL == authst)
  		    {
    			//通知tsp认证失败
    			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				rmtCtrl_Stpara.reqType = PP_rmtCtrl.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtCtrl.eventid;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
				rmtCtrl_Stpara.rvcFailureType = PP_RMTCTRL_BCDMAUTHFAIL;
				PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
    			PP_rmtCtrl_flag = RMTCTRL_IDLE;
				log_o(LOG_HOZON,"-------identificat failed---------");
				// 清除远程控制请求
				ClearPP_autodoorCtrl_Request();
				ClearPP_doorLockCtrl_Request();
				ClearPP_searchvehicle_Request();
				ClearPP_seatheating_Request();
				ClearPP_startengine_Request();
				ClearPP_startforbid_Request();
				ClearPP_sunroofctrl_Request();
   			}
   			else
   			{}
		}
		break;
		case RMTCTRL_COMMAND_LAUNCH://远程控制
		{
			log_o(LOG_HOZON,"-------command---------");
			int ret;
			for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
			{
				if(PP_RmtCtrlFunc[i].mainFunc != NULL)
				{
					PP_RmtCtrlFunc[i].mainFunc((PrvtProt_task_t*)task);
				}
			}

			ret = PP_doorLockCtrl_end() ||
				  PP_autodoorCtrl_end() ||
				  PP_searchvehicle_end()||
				  PP_sunroofctrl_end()  ||
				  PP_startengine_end()  ||
				  PP_seatheating_end()  ||
				  PP_startforbid_end();
			if(ret == 0)
			{
				PP_rmtCtrl_flag = RMTCTRL_IDLE; //远程命令执行完，回到空闲
			}
		}
		break;
		default:
		break;
	}

	return res;
}

/******************************************************
*函数名：PP_rmtCtrl_do_checksock

*形  参：void

*返回值：void

*描  述：检查socket连接

*备  注：
******************************************************/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}

	//释放资源

	return -1;
}

/******************************************************
*函数名：PP_rmtCtrl_do_rcvMsg

*形  参：void

*返回值：void

*描  述：接收数据函数

*备  注：
******************************************************/
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_REMOTE_CTRL,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive rmt ctrl message");
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
	PP_rmtCtrl_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*函数名：PP_rmtCtrl_RxMsgHandle

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_rmtCtrl_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_RMTCTRL != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	PP_rmtCtrl.eventid = MsgDataBody.eventId;
	PP_rmtCtrl.reqType = Appdata.CtrlReq.rvcReqType;
	if(PP_MID_RMTCTRL_REQ == MsgDataBody.mID)//收到请求
	{
		switch((uint8_t)(Appdata.CtrlReq.rvcReqType >> 8))
		{
			case PP_RMTCTRL_DOORLOCK://控制车门锁
			{
				SetPP_doorLockCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
			{
				SetPP_sunroofctrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote PANORSUNROOF control req");
			}
			break;
			case PP_RMTCTRL_AUTODOOR://控制自动门
			{
				SetPP_autodoorCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote AUTODOOR control req");
			}
			break;
			case PP_RMTCTRL_RMTSRCHVEHICLE://远程搜索车辆
			{
				SetPP_searchvehicle_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
			}
			break;
			case PP_RMTCTRL_DETECTCAMERA://驾驶员检测摄像头
			{
				log_i(LOG_HOZON, "remote DETECTCAMERA control req");
			}
			break;
			case PP_RMTCTRL_DATARECORDER://行车记录仪
			{
				log_i(LOG_HOZON, "remote DATARECORDER control req");
			}
			break;
			case PP_RMTCTRL_AC://空调
			{
				//SetPP_ACCtrl_Request(&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_CHARGE://充电
			{
				SetPP_ChargeCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
			}
			break;
			case PP_RMTCTRL_HIGHTENSIONCTRL://高电压控制
			{
				SetPP_startengine_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
			}
			break;
			case PP_RMTCTRL_ENGINECTRL://发动机控制
			{
				log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
			}
			break;
			default:
			break;
		}
	}
}

#if 0
/******************************************************
*函数名：PP_rmtCtrl_do_wait

*形  参：void

*返回值：void

*描  述：检查是否有事件等待应答

*备  注：
******************************************************/
static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task)
{

	return 0;
}
#endif


/******************************************************
*函数名：PP_rmtCtrl_BluetoothSetCtrlReq

*形  参：

*返回值：

*描  述：设置 请求

*备  注：
******************************************************/
void PP_rmtCtrl_BluetoothSetCtrlReq(unsigned char obj, unsigned char cmd)
{
	switch(obj)
	{
		case PP_RMTCTRL_DOORLOCK://控制车门锁
		{
			SetPP_doorLockCtrl_Request(RMTCTRL_BLUETOOTH,&cmd,NULL);
		}
		break;
		case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
		{
			//PP_sunroofctrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote PANORSUNROOF control req");
		}
		break;
		case PP_RMTCTRL_AUTODOOR://控制自动门
		{
			//PP_autodoorCtrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote AUTODOOR control req");
		}
		break;
		case PP_RMTCTRL_RMTSRCHVEHICLE://远程搜索车辆
		{
			//PP_searchvehicle_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
		}
		break;
		case PP_RMTCTRL_DETECTCAMERA://驾驶员检测摄像头
		{
			log_i(LOG_HOZON, "remote DETECTCAMERA control req");
		}
		break;
		case PP_RMTCTRL_DATARECORDER://行车记录仪
		{
			//PP_doorLockCtrl_StatusResp(reqType);
			log_i(LOG_HOZON, "remote DATARECORDER control req");
		}
		break;
		case PP_RMTCTRL_AC://空调
		{
			//PP_rmtCtrl_StatusResp(1,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_AC control req");
		}
		break;
		case PP_RMTCTRL_CHARGE://充电
		{
			//PP_rmtCtrl_StatusResp(2,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
		}
		break;
		case PP_RMTCTRL_HIGHTENSIONCTRL://高电压控制
		{
			//PP_startengine_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
		}
		break;
		case PP_RMTCTRL_ENGINECTRL://发动机控制
		{
			log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
		}
		break;
		default:
		break;
	}
}


/******************************************************
*函数名：PP_rmtCtrl_SetCtrlReq

*形  参：

*返回值：

*描  述：设置 请求

*备  注：
******************************************************/
void PP_rmtCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	switch((uint8_t)(reqType >> 8))
	{
		case PP_RMTCTRL_DOORLOCK://控制车门锁
		{
			PP_doorLockCtrl_SetCtrlReq(req,reqType);
		}
		break;
		case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
		{
			PP_sunroofctrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote PANORSUNROOF control req");
		}
		break;
		case PP_RMTCTRL_AUTODOOR://控制自动门
		{
			PP_autodoorCtrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote AUTODOOR control req");
		}
		break;
		case PP_RMTCTRL_RMTSRCHVEHICLE://远程搜索车辆
		{
			PP_searchvehicle_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
		}
		break;
		case PP_RMTCTRL_DETECTCAMERA://驾驶员检测摄像头
		{
			log_i(LOG_HOZON, "remote DETECTCAMERA control req");
		}
		break;
		case PP_RMTCTRL_DATARECORDER://行车记录仪
		{
			//PP_doorLockCtrl_StatusResp(reqType);
			log_i(LOG_HOZON, "remote DATARECORDER control req");
		}
		break;
		case PP_RMTCTRL_AC://空调
		{
			//PP_rmtCtrl_StatusResp(1,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_AC control req");
		}
		break;
		case PP_RMTCTRL_CHARGE://充电
		{
			//PP_rmtCtrl_StatusResp(2,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
		}
		break;
		case PP_RMTCTRL_HIGHTENSIONCTRL://高电压控制
		{
			PP_startengine_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
		}
		break;
		case PP_RMTCTRL_ENGINECTRL://发动机控制
		{
			log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_rmtCtrl_StInformTsp

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
int PP_rmtCtrl_StInformTsp(void *task,PP_rmtCtrl_Stpara_t *CtrlSt_para)
{
	int msgdatalen;
	int res = 0;

	PrvtProt_task_t* task_ptr = (PrvtProt_task_t*)task;
	/*header*/
	memcpy(PP_rmtCtrl.pack.Header.sign,"**",2);
	PP_rmtCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtCtrl.pack.Header.opera = 0x02;
	PP_rmtCtrl.pack.Header.ver.Byte = task_ptr->version;
	PP_rmtCtrl.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task_ptr->nonce);
	PP_rmtCtrl.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task_ptr->tboxid);
	memcpy(&PP_rmtCtrl_Pack, &PP_rmtCtrl.pack.Header, sizeof(PrvtProt_pack_Header_t));

	switch(CtrlSt_para->Resptype)
	{
		case PP_RMTCTRL_RVCSTATUSRESP://非预约
		{
			/*body*/
			memcpy(PP_rmtCtrl.pack.DisBody.aID,"110",3);
			PP_rmtCtrl.pack.DisBody.mID = PP_MID_RMTCTRL_RESP;
			PP_rmtCtrl.pack.DisBody.eventId =  CtrlSt_para->eventid;
			PP_rmtCtrl.pack.DisBody.eventTime = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.expTime   = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
			PP_rmtCtrl.pack.DisBody.appDataProVer = 256;
			PP_rmtCtrl.pack.DisBody.testFlag = 1;

			/*appdata*/
			PrvtProtcfg_gpsData_t gpsDt;
			App_rmtCtrl.CtrlResp.rvcReqType = CtrlSt_para->reqType;
			App_rmtCtrl.CtrlResp.rvcReqStatus = CtrlSt_para->rvcReqStatus;
			App_rmtCtrl.CtrlResp.rvcFailureType = CtrlSt_para->rvcFailureType;
			App_rmtCtrl.CtrlResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps状态 0-无效；1-有效;
			App_rmtCtrl.CtrlResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gps时间戳:系统时间(通过gps校时)

			PrvtProtCfg_gpsData(&gpsDt);

			if(App_rmtCtrl.CtrlResp.gpsPos.gpsSt == 1)
			{
				if(gpsDt.is_north)
				{
					App_rmtCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//纬度 x 1000000,当GPS信号无效时，值为0
				}
				else
				{
					App_rmtCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//纬度 x 1000000,当GPS信号无效时，值为0
				}

				if(gpsDt.is_east)
				{
					App_rmtCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//经度 x 1000000,当GPS信号无效时，值为0
				}
				else
				{
					App_rmtCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//经度 x 1000000,当GPS信号无效时，值为0
				}
				log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.latitude);
				log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.longitude);
			}
			else
			{
				App_rmtCtrl.CtrlResp.gpsPos.latitude  = 0;
				App_rmtCtrl.CtrlResp.gpsPos.longitude = 0;
			}
			App_rmtCtrl.CtrlResp.gpsPos.altitude = (long)(gpsDt.height);//高度（m）
			if(App_rmtCtrl.CtrlResp.gpsPos.altitude > 10000)
			{
				App_rmtCtrl.CtrlResp.gpsPos.altitude = 10000;
			}
			App_rmtCtrl.CtrlResp.gpsPos.heading = (long)(gpsDt.direction);//车头方向角度，0为正北方向
			App_rmtCtrl.CtrlResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//速度 x 10，单位km/h
			App_rmtCtrl.CtrlResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//水平精度因子 x 10
			if(App_rmtCtrl.CtrlResp.gpsPos.hdop > 1000)
			{
				App_rmtCtrl.CtrlResp.gpsPos.hdop = 1000;
			}

			App_rmtCtrl.CtrlResp.basicSt.driverDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.driverLock = PP_rmtCtrl_cfg_doorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerLock = PP_rmtCtrl_cfg_doorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftLock = PP_rmtCtrl_cfg_doorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightLock = PP_rmtCtrl_cfg_doorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.bootStatus = gb_data_reardoorSt()	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.bootStatusLock = gb_data_reardoorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.driverWindow = 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerWindow = 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftWindow = 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightWinow = 0 /* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.sunroofStatus = PP_rmtCtrl_cfg_sunroofSt()	/* OPTIONAL */;
			if(PP_rmtCtrl_cfg_RmtStartSt() ==0)
			{
				App_rmtCtrl.CtrlResp.basicSt.engineStatus = 0;
			}
			else
			{
				App_rmtCtrl.CtrlResp.basicSt.engineStatus = 1;
			}
			App_rmtCtrl.CtrlResp.basicSt.accStatus = PP_rmtCtrl_cfg_ACOnOffSt();
			App_rmtCtrl.CtrlResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
			App_rmtCtrl.CtrlResp.basicSt.accMode = gb_data_ACMode()	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.accBlowVolume	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.innerTemp = 1;
			App_rmtCtrl.CtrlResp.basicSt.outTemp = 1;
			App_rmtCtrl.CtrlResp.basicSt.sideLightStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.dippedBeamStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.mainBeamStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.hazardLightStus= 1;
			App_rmtCtrl.CtrlResp.basicSt.frtRightTyrePre	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.frtRightTyreTemp	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.frontLeftTyrePre	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.frontLeftTyreTemp= 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightTyrePre= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightTyreTemp= 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftTyrePre	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftTyreTemp	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.batterySOCExact= 1;
			App_rmtCtrl.CtrlResp.basicSt.chargeRemainTim	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.availableOdomtr= 1;
			App_rmtCtrl.CtrlResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeSt= 1;
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeHour= 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeMin	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.chargeMode	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.chargeStatus	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.powerMode	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.speed= 1;
			App_rmtCtrl.CtrlResp.basicSt.totalOdometer= 1;
			App_rmtCtrl.CtrlResp.basicSt.batteryVoltage= 1;
			App_rmtCtrl.CtrlResp.basicSt.batteryCurrent= 1;
			App_rmtCtrl.CtrlResp.basicSt.batterySOCPrc= 1;
			App_rmtCtrl.CtrlResp.basicSt.dcStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.gearPosition= 1;
			App_rmtCtrl.CtrlResp.basicSt.insulationRstance= 1;
			App_rmtCtrl.CtrlResp.basicSt.acceleratePedalprc= 1;
			App_rmtCtrl.CtrlResp.basicSt.deceleratePedalprc= 1;
			App_rmtCtrl.CtrlResp.basicSt.canBusActive= 1;
			App_rmtCtrl.CtrlResp.basicSt.bonnetStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.lockStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.gsmStatus= 1;
			App_rmtCtrl.CtrlResp.basicSt.wheelTyreMotrSt= 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.vehicleAlarmSt= 1;
			App_rmtCtrl.CtrlResp.basicSt.currentJourneyID= 1;
			App_rmtCtrl.CtrlResp.basicSt.journeyOdom= 1;
			App_rmtCtrl.CtrlResp.basicSt.frtLeftSeatHeatLel= 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.frtRightSeatHeatLel	= 1/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.airCleanerSt	= 1/* OPTIONAL */;

			if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_RESP,PP_rmtCtrl_Pack.msgdata,&msgdatalen,\
											   &PP_rmtCtrl.pack.DisBody,&App_rmtCtrl))//数据编码打包是否完成
			{
				log_e(LOG_HOZON, "uper error");
				return -1;
			}
		}
		break;
		case PP_RMTCTRL_RVCBOOKINGRESP://预约
		{
			/*body*/
			memcpy(PP_rmtCtrl.pack.DisBody.aID,"110",3);
			PP_rmtCtrl.pack.DisBody.mID = PP_MID_RMTCTRL_BOOKINGRESP;
			PP_rmtCtrl.pack.DisBody.eventId =  CtrlSt_para->eventid;
			PP_rmtCtrl.pack.DisBody.eventTime = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.expTime   = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
			PP_rmtCtrl.pack.DisBody.appDataProVer = 256;
			PP_rmtCtrl.pack.DisBody.testFlag = 1;

			/*appdata*/
			App_rmtCtrl.CtrlbookingResp.bookingId = CtrlSt_para->bookingId;
			App_rmtCtrl.CtrlbookingResp.rvcReqCode = CtrlSt_para->rvcReqCode;
			App_rmtCtrl.CtrlbookingResp.oprTime = PrvtPro_getTimestamp();

			if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_BOOKINGRESP,PP_rmtCtrl_Pack.msgdata,&msgdatalen,\
											   &PP_rmtCtrl.pack.DisBody,&App_rmtCtrl))//数据编码打包是否完成
			{
				log_e(LOG_HOZON, "uper error");
				return -1;
			}
		}
		break;
		case PP_RMTCTRL_HUBOOKINGRESP://HU 预约
		{
			/*body*/
			memcpy(PP_rmtCtrl.pack.DisBody.aID,"110",3);
			PP_rmtCtrl.pack.DisBody.mID = PP_MID_RMTCTRL_HUBOOKINGRESP;
			PP_rmtCtrl.pack.DisBody.eventId =  CtrlSt_para->eventid;
			PP_rmtCtrl.pack.DisBody.eventTime = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.expTime   = PrvtPro_getTimestamp();
			PP_rmtCtrl.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
			PP_rmtCtrl.pack.DisBody.appDataProVer = 256;
			PP_rmtCtrl.pack.DisBody.testFlag = 1;

			/*appdata*/
			App_rmtCtrl.CtrlHUbookingResp.rvcReqType 	= CtrlSt_para->rvcReqType;
			App_rmtCtrl.CtrlHUbookingResp.huBookingTime = CtrlSt_para->huBookingTime;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqHours 	= CtrlSt_para->rvcReqHours;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqMin 	= CtrlSt_para->rvcReqMin;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqEq 		= CtrlSt_para->rvcReqEq;
			memcpy(App_rmtCtrl.CtrlHUbookingResp.rvcReqCycle,CtrlSt_para->rvcReqCycle,8);
			App_rmtCtrl.CtrlHUbookingResp.rvcReqCyclelen = 8;
			App_rmtCtrl.CtrlHUbookingResp.bookingId = CtrlSt_para->bookingId;

			if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_HUBOOKINGRESP,PP_rmtCtrl_Pack.msgdata,&msgdatalen,\
											   &PP_rmtCtrl.pack.DisBody,&App_rmtCtrl))//数据编码打包是否完成
			{
				log_e(LOG_HOZON, "uper error");
				return -1;
			}
		}
		break;
		default:
		break;
	}

	PP_rmtCtrl_Pack.totallen = 18 + msgdatalen;
	PP_rmtCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	rmtCtrl_TxInform.aid = PP_AID_RMTCTRL;
	rmtCtrl_TxInform.mid = PP_MID_RMTCTRL_RESP;
	rmtCtrl_TxInform.pakgtype = PP_TXPAKG_CONTINUE;

	SP_data_write(PP_rmtCtrl_Pack.Header.sign,PP_rmtCtrl_Pack.totallen,PP_rmtCtrl_send_cb,&rmtCtrl_TxInform);

	protocol_dump(LOG_HOZON, "get_remote_control_response", PP_rmtCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;

}

/******************************************************
*函数名：PP_rmtCtrl_send_cb

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
static void PP_rmtCtrl_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)para;
	log_i(LOG_HOZON, "aid = %d",TxInform_ptr->aid);
	log_i(LOG_HOZON, "mid = %d",TxInform_ptr->mid);
	log_i(LOG_HOZON, "pakgtype = %d",TxInform_ptr->pakgtype);
	log_i(LOG_HOZON, "eventtime = %d",TxInform_ptr->eventtime);
	log_i(LOG_HOZON, "successflg = %d",TxInform_ptr->successflg);
	log_i(LOG_HOZON, "failresion = %d",TxInform_ptr->failresion);
	log_i(LOG_HOZON, "txfailtime = %d",TxInform_ptr->txfailtime);
}
