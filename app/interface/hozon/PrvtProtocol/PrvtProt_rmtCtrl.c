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
#include "PrvtProt_rmtCtrl.h"

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
}__attribute__((packed))  PrvtProt_rmtCtrl_t; /*结构体*/

static PrvtProt_rmtCtrl_t PP_rmtCtrl;

static PrvtProt_pack_t 		PP_rmtCtrl_Pack;

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_mainfunction(PrvtProt_task_t *task,PrvtProt_rmtCtrl_t *rmtCtrl);
static int PP_rmtCtrl_doorLockCtrl(PrvtProt_task_t *task,PrvtProt_rmtCtrl_t *rmtCtrl);
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

	for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
	{
		PP_rmtCtrl.state[i].reqType = PP_RMTCTRL_UNKNOW;
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
	res = 		PP_rmtCtrl_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_wait((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_mainfunction((PrvtProt_task_t*)task,&PP_rmtCtrl);
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

	if(PP_MID_RMTCTRL_REQ == MsgDataBody.mID)//收到请求
	{
		switch((uint8_t)(Appdata.CtrlReq.rvcReqType >> 8))
		{
			case PP_RMTCTRL_DOORLOCK://控制车门锁
			{
				PP_rmtCtrl.state[RMTCTRL_DOORLOCK].req = 1;
				PP_rmtCtrl.state[RMTCTRL_DOORLOCK].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_PNRSUNROOF://控制全景天窗
			{
				PP_rmtCtrl.state[RMTCTRL_PANORSUNROOF].req = 1;
				PP_rmtCtrl.state[RMTCTRL_PANORSUNROOF].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_AUTODOOR://控制自动门
			{
				PP_rmtCtrl.state[RMTCTRL_AUTODOOR].req = 1;
				PP_rmtCtrl.state[RMTCTRL_AUTODOOR].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_RMTSRCHVEHICLE://远程搜索车辆
			{
				PP_rmtCtrl.state[RMTCTRL_AUTODOOR].req = 1;
				PP_rmtCtrl.state[RMTCTRL_AUTODOOR].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_DETECTCAMERA://驾驶员检测摄像头
			{
				PP_rmtCtrl.state[DETECTCAMERA].req = 1;
				PP_rmtCtrl.state[DETECTCAMERA].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_DATARECORDER://行车记录仪
			{
				PP_rmtCtrl.state[DATARECORDER].req = 1;
				PP_rmtCtrl.state[DATARECORDER].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_AC://空调
			{
				PP_rmtCtrl.state[RMTCTRL_AC].req = 1;
				PP_rmtCtrl.state[RMTCTRL_AC].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_CHARGE://充电
			{
				PP_rmtCtrl.state[RMTCTRL_CHARGE].req = 1;
				PP_rmtCtrl.state[RMTCTRL_CHARGE].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_HIGHTENSIONCTRL://高电压控制
			{
				PP_rmtCtrl.state[RMTCTRL_HIGHTENSIONCTRL].req = 1;
				PP_rmtCtrl.state[RMTCTRL_HIGHTENSIONCTRL].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			case PP_RMTCTRL_ENGINECTRL://发动机控制
			{
				PP_rmtCtrl.state[RMTCTRL_ENGINECTRL].req = 1;
				PP_rmtCtrl.state[RMTCTRL_ENGINECTRL].reqType = Appdata.CtrlReq.rvcReqType;
			}
			break;
			default:
			break;
		}
	}
}

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

/******************************************************
*函数名：PP_rmtCtrl_do_mainfunction

*形  参：

*返回值：

*描  述：检查ecall等请求

*备  注：
******************************************************/
static int PP_rmtCtrl_do_mainfunction(PrvtProt_task_t *task,PrvtProt_rmtCtrl_t *rmtCtrl)
{

	PP_rmtCtrl_doorLockCtrl(task,rmtCtrl);

	return 0;
}

/******************************************************
*函数名：PP_rmtCtrl_doorLockCtrl

*形  参：

*返回值：

*描  述：门锁控制

*备  注：
******************************************************/
static int PP_rmtCtrl_doorLockCtrl(PrvtProt_task_t *task,PrvtProt_rmtCtrl_t *rmtCtrl)
{
	if(1 != sockproxy_socketState())//socket not open
	{
		return 0;
		//释放资源
	}

	switch(PP_rmtCtrl.state[RMTCTRL_DOORLOCK].CtrlSt)
	{
	case CTRLDOORLOCK_IDLE:
	{
		if(PP_RMTCTRL_DOORLOCKOPEN == \
				PP_rmtCtrl.state[RMTCTRL_DOORLOCK].reqType)//车门锁打开
		{

		}
	}
	break;
	default:
	break;
	}

	return 0;
}

#if 0
/******************************************************
*函数名：PP_rmtCtrl_SetEcallReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_rmtCtrl_SetEcallReq(unsigned char req)
{

}

#endif

