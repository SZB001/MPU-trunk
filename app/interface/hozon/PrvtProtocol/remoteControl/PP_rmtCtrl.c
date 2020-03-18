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
#include "ble.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "../../support/protocol.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt_cfg.h"
#include "PP_doorLockCtrl.h"
#include "PP_SendWakeUptime.h"
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
#include "PP_CameraCtrl.h"
#include "PP_SendWakeUptime.h"
#include "../PrvtProt_SigParse.h"
#include "../PrvtProt_remoteConfig.h"
#include "PP_bluetoothStart.h"
#include "../PrvtProt_lock.h"
#include "PP_rmtCtrl.h"

#define PP_TXINFORMNODE_NUM 100
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

	//uint8_t busyFlag;
	uint8_t rmtCtrlSt;
	uint8_t sleepflag;
	uint8_t fotaAuthReq;
	uint8_t fotaUpgradeSt;
	int 	fotaAuthResult;
}__attribute__((packed))  PrvtProt_rmtCtrl_t; /*结构体*/

static PrvtProt_rmtCtrl_t 		PP_rmtCtrl;
static PrvtProt_pack_t 			PP_rmtCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 	App_rmtCtrl;
static PrvtProt_task_t			PP_rmtCtrl_task;
static PrvtProt_RmtCtrlFunc_t PP_RmtCtrlFunc[RMTCTRL_OBJ_MAX] =
{
	{RMTCTRL_DOORLOCK,       PP_doorLockCtrl_init,	PP_doorLockCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,   PP_sunroofctrl_init,	PP_sunroofctrl_mainfunction},
	{RMTCTRL_AUTODOOR,       PP_autodoorCtrl_init,	PP_autodoorCtrl_mainfunction},
	{RMTCTRL_RMTSRCHVEHICLE, PP_searchvehicle_init,	PP_searchvehicle_mainfunction},
	{RMTCTRL_HIGHTENSIONCTRL,PP_startengine_init,   PP_startengine_mainfunction},
	{RMTCTRL_AC,	         PP_ACCtrl_init, 	    PP_ACCtrl_mainfunction},
	{RMTCTRL_CHARGE,         PP_ChargeCtrl_init,	NULL},
	{RMTCTRL_ENGINECTRL,	 PP_startforbid_init, 	PP_startforbid_mainfunction},
	{RMTCTRL_SEATHEATINGCTRL,PP_seatheating_init, 	PP_seatheating_mainfunction},
	{RMTCTRL_CAMERACTRL,     PP_CameraCtrl_init,    PP_CameraCtr_mainfunction},
	{RMTCTRL_BLUETOOTHSTART, PP_bluetoothstart_init,PP_bluetoothstart_mainfunction}, //蓝牙一键启动
	
};

static PrvtProt_TxInform_t rmtCtrl_TxInform[PP_TXINFORMNODE_NUM];

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static void PP_rmtCtrl_send_cb(void * para);
static int PP_rmtCtrl_getIdleNode(void);
static uint8_t PP_rmtCtrl_request(void);
static uint8_t PP_rmtCtrl_end(void);
static void PP_rmtCtrl_clear(void);
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
	PP_identificat_init();
	PP_canSend_init();
	for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
	{
		PP_rmtCtrl.state[i].reqType = PP_RMTCTRL_UNKNOW;
		if(PP_RmtCtrlFunc[i].Init != NULL)
		{
			PP_RmtCtrlFunc[i].Init();
		}
	}

	for(i = 0;i < PP_TXINFORMNODE_NUM;i++)
	{
		memset(&rmtCtrl_TxInform[i],0,sizeof(PrvtProt_TxInform_t));
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
	PrvtProt_task_t* task_ptr = (PrvtProt_task_t*)task;
	PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;

	PP_rmtCtrl_task.nonce = task_ptr->nonce;
	PP_rmtCtrl_task.tboxid = task_ptr->tboxid;
	PP_rmtCtrl_task.version = task_ptr->version;

	res = 	PP_rmtCtrl_do_checksock(task_ptr) ||
			PP_rmtCtrl_do_rcvMsg(task_ptr);

	switch(PP_rmtCtrl.rmtCtrlSt)
	{
		case RMTCTRL_IDLE://空闲
		{
			int ret = 0;
			PP_Send_WakeUpTime_to_Mcu();//运行状态到listen模式，发一个最近的定时唤醒
			PP_rmtCtrl_checkenginetime();//15分钟之后下高压电
			//检测空调或座椅加热上电或下电
			PP_AcCtrl_acStMonitor(task_ptr);        //查询空调预约时间
			PP_SeatCtrl_SeatStMonitor(task_ptr);    //查询座椅加热是否满足睡眠条件
			PP_startforbid_acStMonitor(task_ptr);   //满足车控条件的时候是否有禁止启动的请求
			ret = PP_rmtCtrl_request();
			if((ret == 1) || (1 == PP_rmtCtrl.fotaAuthReq))
			{			
				if(PP_canSend_weakupVehicle(REMOTE_VIRTUAL) == 1)	
				{
					if(1 == PP_rmtCtrl.fotaAuthReq)
					{
						log_o(LOG_HOZON,"fota auth request\n");
						PP_rmtCtrl.fotaAuthReq = 0;
						PP_rmtCtrl.rmtCtrlSt = RMTCTRL_IDENTIFICAT_LAUNCH;
					}
					else
					{
						log_o(LOG_HOZON,"REMOTE CONTROL REQUEST\n");
						PP_rmtCtrl.rmtCtrlSt = RMTCTRL_IDENTIFICAT_QUERY;
					}
				}
								
			}
		}
		break;
		case RMTCTRL_IDENTIFICAT_QUERY://检查认证
		{
			if(PP_get_identificat_flag() == 1)	
			{
				PP_rmtCtrl.rmtCtrlSt = RMTCTRL_COMMAND_LAUNCH;
			}
			else
			{
				PP_rmtCtrl.rmtCtrlSt = RMTCTRL_IDENTIFICAT_LAUNCH;
			}
		}
		break;
		case RMTCTRL_IDENTIFICAT_LAUNCH://认证
		{
			int  authst;
			authst = PP_identificat_mainfunction();
   			if(PP_AUTH_SUCCESS == authst)
   			{
				PP_rmtCtrl.fotaAuthResult = 1;
				if(0 == PP_rmtCtrl.fotaUpgradeSt)
				{
					PP_rmtCtrl.rmtCtrlSt = RMTCTRL_COMMAND_LAUNCH;
				}
				else
				{
					PP_rmtCtrl.rmtCtrlSt = RMTCTRL_END;
				}
				log_o(LOG_HOZON,"-------identificat success---------");
  			}
  			else if(PP_AUTH_FAIL == authst)
  		    {
				PP_rmtCtrl.fotaAuthResult = -1;
				PP_rmtCtrl.fotaUpgradeSt = 0;
    			//通知tsp认证失败
    			//PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				rmtCtrl_Stpara.reqType = PP_rmtCtrl.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtCtrl.eventid;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
				rmtCtrl_Stpara.rvcFailureType = PP_RMTCTRL_BCDMAUTHFAIL;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
				log_o(LOG_HOZON,"-------identificat failed---------");
				// 清除远程控制请求
				PP_rmtCtrl_clear();
				PP_rmtCtrl.rmtCtrlSt = RMTCTRL_END;
   			}
   			else
   			{}
		}
		break;
		case RMTCTRL_COMMAND_LAUNCH://远程控制
		{
			int ifend;
			for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
			{
				if(PP_RmtCtrlFunc[i].mainFunc != NULL)
				{
					PP_RmtCtrlFunc[i].mainFunc(task_ptr);
				}
			}

			ifend = PP_rmtCtrl_end();
			if(ifend == 1)
			{
				PP_rmtCtrl.rmtCtrlSt = RMTCTRL_END; //远程命令执行完，回到空闲
			}
		}
		break;
		case RMTCTRL_END:
		{
			clearPP_canSend_virtualOnline(REMOTE_VIRTUAL);//清除虚拟on线
			PP_rmtCtrl.rmtCtrlSt = RMTCTRL_IDLE;
		}
		break;
		default:
		break;
	}

	PP_ChargeCtrl_mainfunction(task_ptr);//充电功能，不需要BDCM认证

	PP_can_send_cycle();//广播440 445报文
	
	PP_rmtCtrl.sleepflag = GetPP_ChargeCtrl_Sleep()&& \
						   GetPP_ACtrl_Sleep()     && \
						   GetPP_SeatCtrl_Sleep()  && \
						   GetPP_Wake_Sleep();

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
*函数名：PP_rmtCtrl_getTimestamp

*形  参：void

*返回值：void

*描  述：获取时间戳

*备  注：
******************************************************/
long PP_rmtCtrl_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return timestamp.tv_sec;
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
	
	log_o(LOG_HOZON, "receive rmt ctrl message");
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
	if(PP_OPERATETYPE_NGTP != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_rmtCtrl_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	if((MsgDataBody.expTime != (-1))&&(MsgDataBody.expTime < PP_rmtCtrl_getTimestamp()))//远控超时
	{
		PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
		log_e(LOG_HOZON,"This remote control timeout");
		rmtCtrl_Stpara.reqType = PP_rmtCtrl.reqType;
		rmtCtrl_Stpara.eventid = PP_rmtCtrl.eventid;
		rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
		rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
		rmtCtrl_Stpara.rvcFailureType = PP_RMTCTRL_INSTRTIMEOUT;
		PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);	
		return ;
	}
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
		PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
		if(PP_rmtCfg_enable_remotecontorl() != 1)
		{
			log_e(LOG_HOZON,"REMOTE CONTROL NOT ENABLED");
			rmtCtrl_Stpara.reqType = PP_rmtCtrl.reqType;
			rmtCtrl_Stpara.eventid = PP_rmtCtrl.eventid;
			rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
			rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
			rmtCtrl_Stpara.rvcFailureType = PP_RMTCTRL_NOTENABLE;
			PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			return;
		}

		int ret = 0;
		switch((uint8_t)(Appdata.CtrlReq.rvcReqType >> 8))
		{
			case PP_RMTCTRL_DOORLOCK://控制车门锁
			{
				ret = SetPP_doorLockCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP door lock control req");
			}
			break;
			case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
			{
				ret = SetPP_sunroofctrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP sunroof control req");
			}
			break;
			case PP_RMTCTRL_AUTODOOR://控制自动门
			{
				ret = SetPP_autodoorCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP electric tailgate control req");
			}
			break;
			case PP_RMTCTRL_RMTSRCHVEHICLE://远程搜索车辆
			{
				ret = SetPP_searchvehicle_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP car search req");
			}
			break;
			case PP_RMTCTRL_DETECTCAMERA://驾驶员检测摄像头
			{
				ret = SetPP_CameraCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote DETECTCAMERA control req");
			}
			break;
			case PP_RMTCTRL_DATARECORDER://行车记录仪
			{
				ret = SetPP_CameraCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote DATARECORDER control req");
			}
			break;
			case PP_RMTCTRL_AC://空调
			{
				switch(Appdata.CtrlReq.rvcReqType)
				{
					case PP_RMTCTRL_ACOPEN:
					case PP_RMTCTRL_ACCLOSE:
					case PP_RMTCTRL_ACAPPOINTOPEN:
					case PP_RMTCTRL_ACCANCELAPPOINT:
					case PP_RMTCTRL_SETTEMP:
					{
						ret = SetPP_ACCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
						log_i(LOG_HOZON, "TSP air conditioner control req");
					}
					break;
					case PP_RMTCTRL_MAINHEATOPEN:
					case PP_RMTCTRL_MAINHEATCLOSE:
					case PP_RMTCTRL_PASSENGERHEATOPEN:
					case PP_RMTCTRL_PASSENGERHEATCLOSE:
					{
						ret = SetPP_seatheating_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
						log_i(LOG_HOZON, "TSP seat heating control req");
					}
					break;
					default:
					break;
				}
			}
			break;
			case PP_RMTCTRL_CHARGE://充电
			{
				ret = SetPP_ChargeCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP charge control req");
			}
			break;
			case PP_RMTCTRL_HIGHTENSIONCTRL://高电压控制
			{
				ret = SetPP_startengine_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP high voltage electric control req");
			}
			break;
			case PP_RMTCTRL_ENGINECTRL://禁止启动
			{
				ret = SetPP_startforbid_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "TSP Disable Start Control req");
			}
			break;
			default:
			break;
		}

		if(PP_LOCK_ERR_FOTAUPDATE == ret)
		{
			log_e(LOG_HOZON,"fota updating");
			rmtCtrl_Stpara.reqType = PP_rmtCtrl.reqType;
			rmtCtrl_Stpara.eventid = PP_rmtCtrl.eventid;
			rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
			rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
			rmtCtrl_Stpara.rvcFailureType = PP_RMTCTRL_FOTA_UPGRADE;
			PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);	
			return;
		}
	}
	else if(PP_MID_RMTCTRL_HUBOOKBACKRESP == MsgDataBody.mID)//HU booking sync response
	{
		PP_ChargeCtrl_HUBookingBackResp(&Appdata.CtrlHUbookingBackResp);
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
void PP_rmtCtrl_BluetoothCtrlReq(unsigned char obj, unsigned char cmd)
{
	int mutex = 0;
	switch(obj)
	{
		case BT_VEhICLE_DOOR_REQ://控制车门锁
		{
			mutex = SetPP_doorLockCtrl_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth door lock control req");
		}
		break;
		case BT_PANORAMIC_SUNROOF_REQ://控制全景天窗
		{
			mutex = SetPP_sunroofctrl_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth sunroof control req");
		}
		break;
		case BT_ELECTRIC_DOOR_REQ://控制自动门
		{
			mutex = SetPP_autodoorCtrl_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth electric tailgate control req");
		}
		break;
		case BT_REMOTE_FIND_CAR_REQ://远程搜索车辆
		{
			mutex = SetPP_searchvehicle_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth car search req");
		}
		break;
		case BT_CHARGE_REQ://充电
		{
			mutex = SetPP_ChargeCtrl_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth charge control req");
		}
		break;
		case BT_POWER_CONTROL_REQ://高电压控制
		{
			mutex = SetPP_bluetoothstart_Request(RMTCTRL_BLUETOOTH,(void *)&cmd,NULL);
			log_i(LOG_HOZON, "Bluetooth one-button start control req");
		}
		break;
		case BT_VEHILCLE_STATUS_REQ:
		{
			PP_rmtCtrl_vehicle_status_InformBt(obj,cmd);
			log_i(LOG_HOZON, "Bluetooth Vehicle status req");
		}
		break;
		default:
		break;
	}
	if(mutex == PP_LOCK_ERR_FOTAUPDATE)  //fota升级
	{
		TCOM_MSG_HEADER msghdr;
		PrvtProt_respbt_t respbt;
		if(obj == BT_VEhICLE_DOOR_REQ)
		{
			respbt.msg_type = BT_VEhICLE_DOOR_RESP;
		}
		else if(obj == BT_PANORAMIC_SUNROOF_REQ)
		{
			respbt.msg_type = BT_PANORAMIC_SUNROOF_RESP;
		}
		else if(obj == BT_ELECTRIC_DOOR_REQ)
		{
			respbt.msg_type = BT_ELECTRIC_DOOR_RESP;
		}
		else if(obj == BT_REMOTE_FIND_CAR_REQ)
		{
			respbt.msg_type = BT_REMOTE_FIND_CAR_RESP;
		}
		else if(obj == BT_CHARGE_REQ)
		{
			respbt.msg_type = BT_CHARGE_RESP;
		}
		else if(obj == BT_POWER_CONTROL_REQ)
		{
			respbt.msg_type = BT_POWER_CONTROL_RESP;
		}
		else if(obj == BT_VEHILCLE_STATUS_REQ)
		{
			respbt.msg_type = BT_VEHILCLE_STATUS_RESP;
		}
		else
		{
		}
		respbt.cmd_state.execution_result = BT_FAIL;  //ִ执行失败
		respbt.cmd = cmd;
		respbt.failtype = 0;
		msghdr.sender    = MPU_MID_HOZON_PP;
		msghdr.receiver  = MPU_MID_BLE;
		msghdr.msgid     = BLE_MSG_CONTROL;
		msghdr.msglen    = sizeof(PrvtProt_respbt_t);
		tcom_send_msg(&msghdr, &respbt);
	}
}
 

//type  :回复蓝牙的消息类型
//cmd   :回复蓝牙的命令
//result:回复TBOX执行的结果
void PP_rmtCtrl_inform_tb(uint8_t type,uint8_t cmd,uint8_t result)
{
	TCOM_MSG_HEADER msghdr;
	PrvtProt_respbt_t respbt;
	respbt.msg_type = type;
	respbt.cmd = cmd;
	if(result == 1)//蓝牙执行成功
	{
		respbt.cmd_state.execution_result = BT_SUCCESS;
		respbt.cmd_state.state = cmd;  
	}
	else           //蓝牙执行失败
	{
		respbt.cmd_state.execution_result = BT_FAIL;
		switch(type)
		{
			case BT_VEhICLE_DOOR_RESP:
			{
				respbt.cmd_state.state = PrvtProtCfg_doorlockSt() ? 1 :2;
			}
			break;
			case BT_PANORAMIC_SUNROOF_RESP:
			{
				respbt.cmd_state.state = PP_rmtCtrl_cfg_bt_sunroofst();
			} 
			break;
			case BT_ELECTRIC_DOOR_RESP:
			{
				respbt.cmd_state.state = PP_rmtCtrl_cfg_bdmreardoorSt() ? 2 : 1;
				log_i(LOG_HOZON,"respbt.cmd_state.state = %d",respbt.cmd_state.state);	
			} 
			break;
			case BT_REMOTE_FIND_CAR_RESP:
			{
				respbt.cmd_state.state = 1;   //保留
			}	 
			break;
			case BT_CHARGE_RESP:  
			{
				respbt.cmd_state.state = PP_rmtCtrl_cfg_chargeOnOffSt() ? 2 : 1;
			} 
			break;
			case BT_POWER_CONTROL_RESP:
			{
				respbt.cmd_state.state = PP_rmtCtrl_cfg_RmtStartSt() ? 2 : 1;
			} 
			break;
			default:
			break;
		}
	}
	msghdr.sender    = MPU_MID_HOZON_PP;
	msghdr.receiver  = MPU_MID_BLE;
	msghdr.msgid     = BLE_MSG_CONTROL;
	msghdr.msglen    = sizeof(PrvtProt_respbt_t);
	tcom_send_msg(&msghdr, &respbt);
}

/******************************************************
*函数名：PP_rmtCtrl_HuCtrlReq

*形  参：

*返回值：

*描  述：设置 请求

*备  注：
******************************************************/
void PP_rmtCtrl_HuCtrlReq(unsigned char obj, void *cmdpara)
{
	switch(obj)
	{
		case PP_RMTCTRL_CHARGE://充电
		{
			SetPP_ChargeCtrl_Request(RMTCTRL_HU,cmdpara,NULL);
			log_i(LOG_HOZON, "HU charge req");
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
	if(PP_rmtCtrl.fotaUpgradeSt == 1)
	{
		log_e(LOG_HOZON,"fota updating");
		return;
	}
	
	switch((uint8_t)(reqType >> 8))
	{
		case PP_RMTCTRL_DOORLOCK://控制车门锁
		{
			log_o(LOG_HOZON,"DOORLOCK");
			PP_doorLockCtrl_SetCtrlReq(req,reqType);
		}
		break;
		case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
		{
			PP_sunroofctrl_SetCtrlReq(req,reqType);
			log_o(LOG_HOZON, "remote PANORSUNROOF control req");
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
			PP_CameraCtrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote DETECTCAMERA control req");
		}
		break;
		case PP_RMTCTRL_DATARECORDER://行车记录仪
		{
			PP_CameraCtrl_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote DATARECORDER control req");
		}
		break;
		case PP_RMTCTRL_AC://空调
		{
			switch(reqType)
			{
				case PP_RMTCTRL_ACOPEN:
				case PP_RMTCTRL_ACCLOSE:
				case PP_RMTCTRL_ACAPPOINTOPEN:
				case PP_RMTCTRL_ACCANCELAPPOINT:
				case PP_RMTCTRL_SETTEMP:
				{
					PP_ACCtrl_SetCtrlReq(req,reqType);
				}
				break;
				case PP_RMTCTRL_MAINHEATOPEN:
				case PP_RMTCTRL_MAINHEATCLOSE:
				case PP_RMTCTRL_PASSENGERHEATOPEN:
				case PP_RMTCTRL_PASSENGERHEATCLOSE:
				{
//					PP_seatheating_SetCtrlReq(req,reqType);
				}
				break;
				default:
				break;
			}
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
			PP_startforbid_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
		}
		break;
		case PP_RMTCTRL_BLUESTART:
		{
			PP_bluetoothstart_SetCtrlReq(req,reqType);
			log_i(LOG_HOZON, "BLUESTART control req");
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_rmtCtrl_vehicle_status_InformBt

*形  参：

*返回值：

*描  述：蓝牙查询车辆状态回复

*备  注：
******************************************************/

int PP_rmtCtrl_vehicle_status_InformBt(unsigned char obj, unsigned char cmd)
{
	TCOM_MSG_HEADER msghdr;
	PrvtProt_respbt_t respbt;
	respbt.cmd = cmd;
	respbt.failtype = 0;
	respbt.msg_type = BT_VEHILCLE_STATUS_RESP;
	respbt.cmd_state.execution_result = 0;
	PP_rmtAc_time acappoint_time;
	
	/**********************vihe_info****************************/
	
	respbt.state.vihe_info.electric_door_state = PP_rmtCtrl_cfg_bdmreardoorSt() ? 2 : 1; 
	log_i(LOG_HOZON,"electric_door_state = %d",respbt.state.vihe_info.electric_door_state);
	
	respbt.state.vihe_info.fine_car_state = 1;  //保留
	log_i(LOG_HOZON,"fine_car_state = %d",respbt.state.vihe_info.fine_car_state);
	
	respbt.state.vihe_info.power_control_state = PP_rmtCtrl_cfg_RmtStartSt() ? 2 : 1;
	log_i(LOG_HOZON,"power_control_state = %d",respbt.state.vihe_info.power_control_state);

	respbt.state.vihe_info.sunroof_state = PP_rmtCtrl_cfg_bt_sunroofst();
	log_i(LOG_HOZON,"sunroof_state = %d",respbt.state.vihe_info.sunroof_state);

	respbt.state.vihe_info.vehicle_door_state =PrvtProtCfg_doorlockSt() ? 1 :2;
	log_i(LOG_HOZON,"vehicle_door_state = %d",respbt.state.vihe_info.vehicle_door_state);

	respbt.state.vihe_info.remaining_capacity = PrvtProtCfg_vehicleSOC();
	log_i(LOG_HOZON,"remaining_capacity = %d",respbt.state.vihe_info.remaining_capacity);

	respbt.state.vihe_info.remaining_mileage = PrvtProtCfg_ResidualOdometer();
	log_i(LOG_HOZON,"remaining_mileage = %d",respbt.state.vihe_info.remaining_mileage);

	respbt.state.vihe_info.total_mileage = PrvtProtCfg_TotalOdometer()/10;
	log_i(LOG_HOZON,"total_mileage = %d",respbt.state.vihe_info.total_mileage);
	
	if(gb_data_gearPosition() == 15)
	{
		respbt.state.vihe_info.car_gear_position = 1; //P档
	}
	else if(gb_data_gearPosition() == 13)
	{
		respbt.state.vihe_info.car_gear_position = 2; //R档
	}
	else if(gb_data_gearPosition() == 0)
	{
		respbt.state.vihe_info.car_gear_position = 3; //N档
	}
	else if(gb_data_gearPosition() == 14)
	{
		respbt.state.vihe_info.car_gear_position = 4; //D档
	}
	log_i(LOG_HOZON,"car_gear_position = %d",respbt.state.vihe_info.car_gear_position);
	/**********************vihe_air****************************/
	if(PP_rmtCtrl_cfg_ACOnOffSt() == 1)  //空调开启
	{
		respbt.state.vihe_air.air_conditioning_mode = 3;           //远程控制开空调，auto命令开启
		
		respbt.state.vihe_air.vehicle_air_state = 2;
		
		respbt.state.vihe_air.air_temperature = PP_rmtCtrl_cfg_LHTemp();  //实际设置的空调温度
	}
	else   //空调关闭
	{
		respbt.state.vihe_air.air_conditioning_mode = 0;      
		
		respbt.state.vihe_air.vehicle_air_state = 1;
		
		respbt.state.vihe_air.air_temperature = 0;
	}
	log_i(LOG_HOZON,"air_conditioning_mode = %d",respbt.state.vihe_air.air_conditioning_mode);
	log_i(LOG_HOZON,"vehicle_air_state = %d",respbt.state.vihe_air.vehicle_air_state);
	log_i(LOG_HOZON,"air_temperature = %d",respbt.state.vihe_air.air_temperature);

	GetPP_ACCtrl_appointHour(&acappoint_time);
	
	respbt.state.vihe_air.air_condition_reservation = GetPP_ACCtrl_appointSt();
	
	respbt.state.vihe_air.reservation_hour1 = acappoint_time.actime[0].hour;
	log_i(LOG_HOZON,"reservation_hour1 = %d",respbt.state.vihe_air.reservation_hour1);
	
	respbt.state.vihe_air.reservation_minute1 = acappoint_time.actime[0].min;
	log_i(LOG_HOZON,"reservation_minute1 = %d",respbt.state.vihe_air.reservation_minute1);
	
	respbt.state.vihe_air.reservation_hour2 = acappoint_time.actime[1].hour;
	log_i(LOG_HOZON,"reservation_hour2 = %d",respbt.state.vihe_air.reservation_hour2);
	
	respbt.state.vihe_air.reservation_minute2 = acappoint_time.actime[1].min;
	log_i(LOG_HOZON,"reservation_minute2 = %d",respbt.state.vihe_air.reservation_minute2);
	
	respbt.state.vihe_air.reservation_hour3 = acappoint_time.actime[2].hour;
	log_i(LOG_HOZON,"reservation_hour3 = %d",respbt.state.vihe_air.reservation_hour3);
	
	respbt.state.vihe_air.reservation_minute3 = acappoint_time.actime[2].min;
	log_i(LOG_HOZON,"reservation_minute3 = %d",respbt.state.vihe_air.reservation_minute3);
	
	respbt.state.vihe_air.vehicle_mainseat_state = PP_rmtCtrl_cfg_DrivHeatingSt() + 1;
	log_i(LOG_HOZON,"vehicle_mainseat_state = %d",respbt.state.vihe_air.vehicle_mainseat_state);
	
	respbt.state.vihe_air.vehicle_secondseat_state = PP_rmtCtrl_cfg_PassHeatingSt() + 1;
	log_i(LOG_HOZON,"vehicle_secondseat_state = %d",respbt.state.vihe_air.vehicle_secondseat_state);

	respbt.state.vihe_air.vehicle_temperature =  gb_data_InnerTemp() - 80;
	log_i(LOG_HOZON,"vehicle_temperature = %d",respbt.state.vihe_air.vehicle_temperature);
	
	respbt.state.vihe_air.outside_temperature = gb_data_outTemp() - 80;
	log_i(LOG_HOZON,"outside_temperature = %d",respbt.state.vihe_air.outside_temperature);

	respbt.state.vihe_air.airwindshield = gb_data_BlowerGears();
	log_i(LOG_HOZON,"airwindshield = %d",respbt.state.vihe_air.airwindshield);
	
	/**********************vihe_charge****************************/
	respbt.state.vihe_charge.charge_reservation = GetPP_ChargeCtrl_appointSt()? 2 : 1;
	log_i(LOG_HOZON,"charge_reservation = %d",respbt.state.vihe_charge.charge_reservation);
	
	respbt.state.vihe_charge.charge_state = PP_rmtCtrl_cfg_chargeOnOffSt() ? 2 : 1;
	log_i(LOG_HOZON,"charge_state = %d",respbt.state.vihe_charge.charge_state);
	
	respbt.state.vihe_charge.remaining_charge_hour = PP_rmtCtrl_cfg_chargeOnOffSt() ? gb_data_ACChargeRemainTime()/60 :0;
	log_i(LOG_HOZON,"remaining_charge_hour = %d",respbt.state.vihe_charge.remaining_charge_hour);
	
	respbt.state.vihe_charge.remaining_charge_minute = PP_rmtCtrl_cfg_chargeOnOffSt() ? gb_data_ACChargeRemainTime()%60 :0;
	log_i(LOG_HOZON,"remaining_charge_minute = %d",respbt.state.vihe_charge.remaining_charge_minute);
	
	respbt.state.vihe_charge.reservation_hour = GetPP_ChargeCtrl_appointSt() ? GetPP_ChargeCtrl_appointHour():0;
	log_i(LOG_HOZON,"reservation_hour = %d",respbt.state.vihe_charge.reservation_hour);
	
	respbt.state.vihe_charge.reservation_minute = GetPP_ChargeCtrl_appointSt() ? GetPP_ChargeCtrl_appointMin():0;
	log_i(LOG_HOZON,"reservation_minute = %d",respbt.state.vihe_charge.reservation_minute);

	respbt.state.vihe_charge.battery_temperature = 1;

	/**********************胎压信息****************************/
	respbt.state.vihe_tire.ltire_pressure1 = PrvtProtCfg_TyrePre(2);//左前胎压
	log_i(LOG_HOZON,"ltire_pressure1 = %d",respbt.state.vihe_tire.ltire_pressure1);
	
	respbt.state.vihe_tire.ltire_temp1 = PrvtProtCfg_TyreTemp(2);//左前温度
	log_i(LOG_HOZON,"ltire_temp1 = %d",respbt.state.vihe_tire.ltire_temp1);
	
	respbt.state.vihe_tire.ltire_pressure2 = PrvtProtCfg_TyrePre(4);//左后胎压
	log_i(LOG_HOZON,"ltire_pressure2 = %d",respbt.state.vihe_tire.ltire_pressure2);
	
	respbt.state.vihe_tire.ltire_temp2 = PrvtProtCfg_TyreTemp(4);//左后温度
	log_i(LOG_HOZON,"ltire_temp2 = %d",respbt.state.vihe_tire.ltire_temp2);
	
	respbt.state.vihe_tire.rtire_pressure1 = PrvtProtCfg_TyrePre(1);//右前胎压
	log_i(LOG_HOZON,"rtire_pressure1 = %d",respbt.state.vihe_tire.rtire_pressure1);
	
	respbt.state.vihe_tire.rtire_temp1= PrvtProtCfg_TyreTemp(1);//右前温度
	log_i(LOG_HOZON,"rtire_temp1 = %d",respbt.state.vihe_tire.rtire_temp1);
	
	respbt.state.vihe_tire.rtire_pressure2 = PrvtProtCfg_TyrePre(3);//右后胎压
	log_i(LOG_HOZON,"rtire_pressure2 = %d",respbt.state.vihe_tire.rtire_pressure2);
	
	respbt.state.vihe_tire.rtire_temp2 =PrvtProtCfg_TyreTemp(3);//右后温度
	log_i(LOG_HOZON,"rtire_temp2 = %d",respbt.state.vihe_tire.rtire_temp2);
	
	/**********************车门状态****************************/
	respbt.state.vihe_door.ldoor1_state = getgb_data_LFDoorOpenSt()?2:1;
	log_i(LOG_HOZON,"ldoor1_state = %d",respbt.state.vihe_door.ldoor1_state);
	
	respbt.state.vihe_door.ldoor2_state = getgb_data_LRDoorOpenSt()?2:1;
	log_i(LOG_HOZON,"ldoor2_state = %d",respbt.state.vihe_door.ldoor2_state);
	
	respbt.state.vihe_door.rdoor1_state = getgb_data_RFDoorOpenSt()?2:1;
	log_i(LOG_HOZON,"rdoor1_state = %d",respbt.state.vihe_door.rdoor1_state);
	
	respbt.state.vihe_door.rdoor2_state = getgb_data_RRDoorOpenSt()?2:1;
	log_i(LOG_HOZON,"rdoor2_state = %d",respbt.state.vihe_door.rdoor2_state);
	
	msghdr.sender    = MPU_MID_HOZON_PP;
	msghdr.receiver  = MPU_MID_BLE;
	msghdr.msgid     = BLE_MSG_CONTROL;
	msghdr.msglen    = sizeof(PrvtProt_respbt_t);
	tcom_send_msg(&msghdr, &respbt);
	return 0;	
}

/******************************************************
*函数名：PP_rmtCtrl_StInformTsp

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
int PP_rmtCtrl_StInformTsp(PP_rmtCtrl_Stpara_t *CtrlSt_para)
{
	int msgdatalen;
	int res = 0;

	/*header*/
	memcpy(PP_rmtCtrl.pack.Header.sign,"**",2);
	PP_rmtCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtCtrl.pack.Header.opera = 0x02;
	PP_rmtCtrl.pack.Header.ver.Byte = PP_rmtCtrl_task.version;
	PP_rmtCtrl.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)PP_rmtCtrl_task.nonce);
	PP_rmtCtrl.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)PP_rmtCtrl_task.tboxid);
	memcpy(&PP_rmtCtrl_Pack, &PP_rmtCtrl.pack.Header, sizeof(PrvtProt_pack_Header_t));

	switch(CtrlSt_para->Resptype)
	{
		case PP_RMTCTRL_RVCSTATUSRESP://非预约
		{
			/*body*/
			memcpy(PP_rmtCtrl.pack.DisBody.aID,"110",3);
			PP_rmtCtrl.pack.DisBody.expTime = CtrlSt_para->expTime ;
			PP_rmtCtrl.pack.DisBody.mID = PP_MID_RMTCTRL_RESP;
			PP_rmtCtrl.pack.DisBody.eventId =  CtrlSt_para->eventid;
			PP_rmtCtrl.pack.DisBody.eventTime = PrvtPro_getTimestamp();
			//PP_rmtCtrl.pack.DisBody.expTime   = PrvtPro_getTimestamp();
			PP_UpMsgCnt++;
			PP_rmtCtrl.pack.DisBody.ulMsgCnt = PP_UpMsgCnt;	/* OPTIONAL */
			PP_rmtCtrl.pack.DisBody.appDataProVer = 256;
			PP_rmtCtrl.pack.DisBody.testFlag = 1;

			/*appdata*/
			PrvtProtcfg_gpsData_t gpsDt;
			App_rmtCtrl.CtrlResp.rvcReqType = CtrlSt_para->reqType;
			App_rmtCtrl.CtrlResp.rvcReqStatus = CtrlSt_para->rvcReqStatus;
			log_i(LOG_HOZON, "App_rmtCtrl.CtrlResp.rvcReqStatus = %d",App_rmtCtrl.CtrlResp.rvcReqStatus);
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
				//log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.latitude);
				//log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.longitude);
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

			App_rmtCtrl.CtrlResp.basicSt.driverDoor 	= getgb_data_LFDoorOpenSt()	? 1:0/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.driverLock 	= PrvtProtCfg_doorlockSt()?0:1;
			App_rmtCtrl.CtrlResp.basicSt.passengerDoor 	= getgb_data_RFDoorOpenSt()	? 1:0/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerLock 	= PrvtProtCfg_doorlockSt()?0:1;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftDoor 	= getgb_data_LRDoorOpenSt()	? 1:0/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftLock 	= PrvtProtCfg_doorlockSt()?0:1;
			App_rmtCtrl.CtrlResp.basicSt.rearRightDoor 	= getgb_data_RRDoorOpenSt()	? 1:0/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightLock 	= PrvtProtCfg_doorlockSt()?0:1;
			 
			App_rmtCtrl.CtrlResp.basicSt.bootStatus 	= PrvtProtCfg_reardoorSt();
			App_rmtCtrl.CtrlResp.basicSt.bootStatusLock = gb_data_reardoorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.driverWindow 	= 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerWindow = 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftWindow = 0	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightWinow = 0 /* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.sunroofStatus 	= PrvtProtCfg_sunroofSt()	/* OPTIONAL */;
			if(PP_rmtCtrl_cfg_RmtStartSt() == 0)
			{
				App_rmtCtrl.CtrlResp.basicSt.engineStatus = 0;
			}
			else
			{
				App_rmtCtrl.CtrlResp.basicSt.engineStatus = 1;
			}
			App_rmtCtrl.CtrlResp.basicSt.accStatus 			= PP_rmtCtrl_cfg_ACOnOffSt();
			App_rmtCtrl.CtrlResp.basicSt.accTemp 			= PrvtProtCfg_LHTemp()	/* OPTIONAL */;//16-32
			App_rmtCtrl.CtrlResp.basicSt.accMode 			= gb_data_ACMode()	/* OPTIONAL */;
			if(App_rmtCtrl.CtrlResp.basicSt.accMode > 3)
			{
				App_rmtCtrl.CtrlResp.basicSt.accMode = 3;
			}
			
			App_rmtCtrl.CtrlResp.basicSt.accBlowVolume		= gb_data_BlowerGears()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.innerTemp 			= gb_data_InnerTemp();  //室内温度
			if(App_rmtCtrl.CtrlResp.basicSt.innerTemp > 125)
			{
				App_rmtCtrl.CtrlResp.basicSt.innerTemp = 125;
			}
			
			App_rmtCtrl.CtrlResp.basicSt.outTemp 			= gb_data_outTemp();
			if(App_rmtCtrl.CtrlResp.basicSt.outTemp > 125)
			{
				App_rmtCtrl.CtrlResp.basicSt.outTemp = 125;
			}
			
			App_rmtCtrl.CtrlResp.basicSt.sideLightStatus 	= gb_data_PostionLampSt();//示位灯
			App_rmtCtrl.CtrlResp.basicSt.dippedBeamStatus 	= gb_data_NearLampSt();//近光灯
			App_rmtCtrl.CtrlResp.basicSt.mainBeamStatus 	= gb_data_HighbeamLampSt();//远光灯
			App_rmtCtrl.CtrlResp.basicSt.hazardLightStus 	= gb_data_TwinFlashLampSt();//双闪灯
			App_rmtCtrl.CtrlResp.basicSt.frtRightTyrePre	= PrvtProtCfg_TyrePre(1);/* 右前胎压 */
			
			App_rmtCtrl.CtrlResp.basicSt.frtRightTyreTemp	= PrvtProtCfg_TyreTemp(1);/*右前温度*/
			
			App_rmtCtrl.CtrlResp.basicSt.frontLeftTyrePre	= PrvtProtCfg_TyrePre(2);/* 左前胎压 */
			
			App_rmtCtrl.CtrlResp.basicSt.frontLeftTyreTemp	= PrvtProtCfg_TyreTemp(2);	/* OPTIONAL */
			
			App_rmtCtrl.CtrlResp.basicSt.rearRightTyrePre	= PrvtProtCfg_TyrePre(3)/* OPTIONAL */;
			
			App_rmtCtrl.CtrlResp.basicSt.rearRightTyreTemp	= PrvtProtCfg_TyreTemp(3)	/* OPTIONAL */;
			
			App_rmtCtrl.CtrlResp.basicSt.rearLeftTyrePre	= PrvtProtCfg_TyrePre(4)/* OPTIONAL */;
			
			App_rmtCtrl.CtrlResp.basicSt.rearLeftTyreTemp	= PrvtProtCfg_TyreTemp(4)/* OPTIONAL */;
			
			long VehicleSOC;
			VehicleSOC = PrvtProtCfg_vehicleSOC();
			App_rmtCtrl.CtrlResp.basicSt.batterySOCExact 	= VehicleSOC * 100;
			App_rmtCtrl.CtrlResp.basicSt.chargeRemainTim	= PrvtProtCfg_ACChargeRemainTime()/* OPTIONAL */;
			
			App_rmtCtrl.CtrlResp.basicSt.availableOdomtr	= PrvtProtCfg_ResidualOdometer();//续航里程;
			
			App_rmtCtrl.CtrlResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
			
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeSt	= GetPP_ChargeCtrl_appointSt();
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeHour	= GetPP_ChargeCtrl_appointHour()	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.bookingChargeMin	= GetPP_ChargeCtrl_appointMin()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.chargeMode			= PrvtProtCfg_chargeSt()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.chargeStatus		= gb_data_chargestatus()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.powerMode			= gb_data_powermode()/* OPTIONAL */;//0x1--纯电;0x2--混动;0x3--燃油
			App_rmtCtrl.CtrlResp.basicSt.speed				= PrvtProtCfg_vehicleSpeed();
			
			App_rmtCtrl.CtrlResp.basicSt.totalOdometer		= PrvtProtCfg_TotalOdometer();
			
			App_rmtCtrl.CtrlResp.basicSt.batteryVoltage		= PrvtProtCfg_TotalVoltage();
			
			App_rmtCtrl.CtrlResp.basicSt.batteryCurrent		= PrvtProtCfg_TotalCurrent();
			
			App_rmtCtrl.CtrlResp.basicSt.batterySOCPrc 		= VehicleSOC;
			App_rmtCtrl.CtrlResp.basicSt.dcStatus			= gb_data_dcdcstatus();
			App_rmtCtrl.CtrlResp.basicSt.gearPosition		= gb_data_gearPosition();
			App_rmtCtrl.CtrlResp.basicSt.insulationRstance	= gb_data_insulationResistance();
			if(App_rmtCtrl.CtrlResp.basicSt.insulationRstance > 60000)
			{
				App_rmtCtrl.CtrlResp.basicSt.insulationRstance = 60000;
			}
			
			App_rmtCtrl.CtrlResp.basicSt.acceleratePedalprc	= gb_data_acceleratePedalPrc();
			if(App_rmtCtrl.CtrlResp.basicSt.acceleratePedalprc > 100)
			{
				App_rmtCtrl.CtrlResp.basicSt.acceleratePedalprc = 100;
			}
			
			App_rmtCtrl.CtrlResp.basicSt.deceleratePedalprc	= gb_data_deceleratePedalPrc();
			App_rmtCtrl.CtrlResp.basicSt.canBusActive		= gb_data_CanbusActiveSt();
			App_rmtCtrl.CtrlResp.basicSt.bonnetStatus		= 0;//引擎盖，默认关
			App_rmtCtrl.CtrlResp.basicSt.lockStatus			= PrvtProtCfg_doorlockSt();
			App_rmtCtrl.CtrlResp.basicSt.gsmStatus			= gb32960_networkSt();
			App_rmtCtrl.CtrlResp.basicSt.wheelTyreMotrSt	= getgb_data_bdmsystemfailure();	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.vehicleAlarmSt		= getgb_data_warnSt();
			App_rmtCtrl.CtrlResp.basicSt.currentJourneyID	= gb_data_trip();
			App_rmtCtrl.CtrlResp.basicSt.journeyOdom		= PrvtProtCfg_trip();
			App_rmtCtrl.CtrlResp.basicSt.frtLeftSeatHeatLel	= PP_rmtCtrl_cfg_DrivHeatingSt()	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.frtRightSeatHeatLel= PP_rmtCtrl_cfg_PassHeatingSt()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.airCleanerSt		= PrvtProt_SignParse_pm25valid()/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.srsStatus			= PrvtProtCfg_CrashOutputSt();//安全气囊状态

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
			PP_rmtCtrl.pack.DisBody.expTime   = CtrlSt_para->expTime;
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
			PP_rmtCtrl.pack.DisBody.expTime   = -1;
			PP_rmtCtrl.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
			PP_rmtCtrl.pack.DisBody.appDataProVer = 256;
			PP_rmtCtrl.pack.DisBody.testFlag = 1;

			/*appdata*/
			App_rmtCtrl.CtrlHUbookingResp.rvcReqType 	= CtrlSt_para->rvcReqType;
			App_rmtCtrl.CtrlHUbookingResp.huBookingTime = CtrlSt_para->huBookingTime;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqHours 	= CtrlSt_para->rvcReqHours;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqMin 	= CtrlSt_para->rvcReqMin;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqEq 		= CtrlSt_para->rvcReqEq;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqCycle = CtrlSt_para->rvcReqCycle;
			App_rmtCtrl.CtrlHUbookingResp.rvcReqCyclelen = 1;
			App_rmtCtrl.CtrlHUbookingResp.bookingId = CtrlSt_para->HUbookingId;
			//log_o(LOG_HOZON,"App_rmtCtrl.CtrlHUbookingResp.bookingId = %d",App_rmtCtrl.CtrlHUbookingResp.bookingId);

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

	int i = PP_rmtCtrl_getIdleNode();
	rmtCtrl_TxInform[i].aid = PP_AID_RMTCTRL;
	rmtCtrl_TxInform[i].mid = PP_rmtCtrl.pack.DisBody.mID;
	rmtCtrl_TxInform[i].eventtime = tm_get_time();
	if(CtrlSt_para->Resptype == PP_RMTCTRL_HUBOOKINGRESP)
	{
		rmtCtrl_TxInform[i].pakgtype = PP_TXPAKG_SIGTIME;
	
}
	else
	{
		rmtCtrl_TxInform[i].pakgtype = PP_TXPAKG_CONTINUE;
	}
	rmtCtrl_TxInform[i].idleflag = 1;
	rmtCtrl_TxInform[i].description = "remote vehi control response";
	SP_data_write(PP_rmtCtrl_Pack.Header.sign,PP_rmtCtrl_Pack.totallen,PP_rmtCtrl_send_cb,&rmtCtrl_TxInform[i]);

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

	if(TxInform_ptr->mid == PP_MID_RMTCTRL_HUBOOKINGRESP)
	{
		if(TxInform_ptr->successflg == PP_TXPAKG_SUCCESS)
		{
			PP_ChargeCtrl_send_cb();
		}
	}

	TxInform_ptr->idleflag = 0;
}


static int PP_rmtCtrl_getIdleNode(void)
{
	int i;
	int res = 0;
	for(i = 0;i < PP_TXINFORMNODE_NUM;i++)
	{
		if(rmtCtrl_TxInform[i].idleflag == 0)
		{
			rmtCtrl_TxInform[i].idleflag = 1;
			res = i;
			break;
		}
	}
	return res;
}

/*
* 获取远程控制请求状态
*/
static uint8_t PP_rmtCtrl_request(void)
{
	uint8_t ret = 0;
	ret  = PP_doorLockCtrl_start() ||	\
		   PP_autodoorCtrl_start() ||	\
		   PP_searchvehicle_start()||	\
		   PP_sunroofctrl_start()  ||	\
		   PP_startengine_start()  ||	\
		   PP_seatheating_start()  ||	\
		   PP_ACCtrl_start()	   ||	\
		   PP_startforbid_start()  ||	\
		   PP_CameraCtrl_start()   ||	\
		   PP_bluetoothstart_start(); 
	return ret;
}

/*
* 获取远程控制执行结束状态
*/
static uint8_t PP_rmtCtrl_end(void)
{
	uint8_t ret = 0;
	ret = PP_doorLockCtrl_end()   &&	\
		  PP_autodoorCtrl_end()   &&	\
		  PP_searchvehicle_end()  &&	\
		  PP_sunroofctrl_end()    &&	\
		  PP_startengine_end()    &&	\
		  PP_seatheating_end()    &&	\
		  PP_ACCtrl_end()	      &&	\
		  PP_startforbid_end()    &&    \
		  PP_CameraCtrl_end()     &&    \
		  PP_bluetoothstart_end();
	return ret;
}

/*
* 清远程控制状态
*/
static void PP_rmtCtrl_clear(void)
{
	PP_autodoorCtrl_ClearStatus();
	PP_doorLockCtrl_ClearStatus();
	PP_searchvehicle_ClearStatus();
	PP_seatheating_ClearStatus();
	PP_startengine_ClearStatus();
	PP_startforbid_ClearStatus();
	PP_sunroofctrl_ClearStatus();
	PP_ACCtrl_ClearStatus();
	PP_CameraCtrl_ClearStatus();
	PP_bluetoothstart_ClearStatus();
}

/******************************************************
*������:SetPP_rmtCtrl_Awaken

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void SetPP_rmtCtrl_Awaken(void)
{
	PP_rmtCtrl.sleepflag = 0;
	//SetPP_ChargeCtrl_Awaken();
}

/******************************************************
*������:GetPrvtProt_Sleep

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
unsigned char GetPP_rmtCtrl_Sleep(void)
{
	return PP_rmtCtrl.sleepflag;
}


unsigned char GetPP_rmtCtrl_AuthResult(void)
{
	return PP_rmtCtrl.fotaAuthResult;
}

void SetPP_rmtCtrl_AuthRequest(void)
{
	PP_rmtCtrl.fotaAuthReq = 1;
	PP_rmtCtrl.fotaAuthResult = 0;
}

unsigned char GetPP_rmtCtrl_fotaUpgrade(void)
{
	return PP_rmtCtrl.fotaUpgradeSt;
}
/******************************************************
*函数名：SetPP_rmtCtrl_FOTA_startInform

*形  参：

*返回值：

*描  述：fota升级开始通知

*备  注：
******************************************************/
int SetPP_rmtCtrl_FOTA_startInform(void)
{
	int mtxlockst = 0;
	mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_OTA_FOTAUPDATE);
	if(PP_LOCK_OK == mtxlockst)
	{
		SetPP_ChargeCtrl_appointPara();
		PP_rmtCtrl.fotaAuthReq = 1;
		PP_rmtCtrl.fotaUpgradeSt = 1;
		PP_rmtCtrl.fotaAuthResult = 0;
		sockproxy_socketclose((int)(PP_SP_COLSE_OTA));
	}
	else
	{
		if(PP_LOCK_ERR_VEHICTRLING == mtxlockst)
		{
			return -1;
		}
		else
		{
			return -2;
		}
	}
	
	return 0;
}

/******************************************************
*函数名：SetPP_rmtCtrl_FOTA_endInform

*形  参：

*返回值：

*描  述：fota升级完成通知

*备  注：
******************************************************/
int SetPP_rmtCtrl_FOTA_endInform(void)
{
	int res = 0;
	PP_rmtCtrl.fotaUpgradeSt = 0;
	clearPP_lock_odcmtxlock(PP_LOCK_OTA_FOTAUPDATE);
	return res;
}

/******************************************************
*函数名：PP_rmtCtrl_ShellFotaUpdateReq

*形  参：

*返回值：

*描  述：shell设置 请求

*备  注：
******************************************************/
void PP_rmtCtrl_ShellFotaUpdateReq(unsigned char req)
{
	int ret;
	if(req == 1)
	{
		ret = SetPP_rmtCtrl_FOTA_startInform();
		log_o(LOG_HOZON, "start fota ret = %d\n",ret);
	}
	else
	{
		SetPP_rmtCtrl_FOTA_endInform();
		log_o(LOG_HOZON, "end fota\n");
	}	
}

/******************************************************
*PP_rmtCtrl_showSleepPara

*形  参：

*返回值：

*描  述：显示睡眠状态参数

*备  注：
******************************************************/
void PP_rmtCtrl_showSleepPara(void)
{
	log_o(LOG_HOZON, "GetPP_ChargeCtrl_Sleep = %d",GetPP_ChargeCtrl_Sleep());
	log_o(LOG_HOZON, "GetPP_ACtrl_Sleep = %d",GetPP_ACtrl_Sleep());
	log_o(LOG_HOZON, "GetPP_SeatCtrl_Sleep = %d",GetPP_SeatCtrl_Sleep());
	log_o(LOG_HOZON, "GetPP_Wake_Sleep = %d",GetPP_Wake_Sleep());
	log_o(LOG_HOZON, "PP_get_virtual_flag = %s",PP_get_virtual_flag()?"off":"on");
}

