/******************************************************
文件名：	PrvtProt_remoteCfg.c

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
#include "cfg_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "PrvtProt_remoteConfig.h"

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
}__attribute__((packed))  PrvtProt_rmtCfg_pack_t; /**/

typedef struct
{
	PrvtProt_rmtCfg_pack_t 	pack;
	PrvtProt_rmtCfgSt_t	 	state;
}__attribute__((packed))  PrvtProt_rmtCfg_t; /*结构体*/

static PrvtProt_pack_t 		PP_rmtCfg_Pack;
static PrvtProt_rmtCfg_t	PP_rmtCfg;
static PrvtProt_App_rmtCfg_t AppData_rmtCfg;

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCfg_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCfg_do_rcvMsg(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static void PP_rmtCfg_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtCfg_do_wait(PrvtProt_task_t *task);
static int PP_rmtCfg_do_checkConfig(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_ReadCfgResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg);
//static int PP_rmtCfg_strcmp(unsigned char* str1,unsigned char* str2,int len);
static void PP_rmtCfg_reset(PrvtProt_rmtCfg_t *rmtCfg);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_rmtCfg_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_rmtCfg_init(void)
{
	memset(&PP_rmtCfg,0 , sizeof(PrvtProt_rmtCfg_t));
	memcpy(PP_rmtCfg.pack.Header.sign,"**",2);
	PP_rmtCfg.pack.Header.ver.Byte = 0x30;
	PP_rmtCfg.pack.Header.commtype.Byte = 0xe1;
	PP_rmtCfg.pack.Header.opera = 0x02;
	PP_rmtCfg.pack.Header.tboxid = 27;
	memcpy(PP_rmtCfg.pack.DisBody.aID,"100",strlen("100"));
	PP_rmtCfg.pack.DisBody.eventId = PP_INVALID;
	PP_rmtCfg.pack.DisBody.appDataProVer = 256;
	PP_rmtCfg.pack.DisBody.testFlag = 1;
	PP_rmtCfg.state.req = 1;
	PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
	PP_rmtCfg.state.period = tm_get_time();

	memset(&AppData_rmtCfg,0 , sizeof(PrvtProt_App_rmtCfg_t));
	memcpy(AppData_rmtCfg.checkReq.mcuSw,"00654",strlen("00654"));
	AppData_rmtCfg.checkReq.mcuSwlen = strlen("00654");
	memcpy(AppData_rmtCfg.checkReq.mpuSw,"00654",strlen("00654"));
	AppData_rmtCfg.checkReq.mpuSwlen = strlen("00654");
	memcpy(AppData_rmtCfg.checkReq.vehicleVin,"LUZAGAAA6JA000654",strlen("LUZAGAAA6JA000654"));
	AppData_rmtCfg.checkReq.vehicleVinlen = strlen("LUZAGAAA6JA000654");
	memcpy(AppData_rmtCfg.checkReq.iccID,"89860317452068729781",strlen("89860317452068729781"));
	AppData_rmtCfg.checkReq.iccIDlen = strlen("89860317452068729781");
	memcpy(AppData_rmtCfg.checkReq.btMacAddr,"000000000000",strlen("000000000000"));
	AppData_rmtCfg.checkReq.btMacAddrlen = strlen("000000000000");
	memcpy(AppData_rmtCfg.checkReq.configSw,"00000",strlen("00000"));
	AppData_rmtCfg.checkReq.configSwlen = strlen("00000");
	memcpy(AppData_rmtCfg.checkReq.cfgVersion,"00000000000000000000000000000001",strlen("00000000000000000000000000000001"));
	AppData_rmtCfg.checkReq.cfgVersionlen = strlen("00000000000000000000000000000001");
}

/******************************************************
*函数名：PP_rmtCfg_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_rmtCfg_mainfunction(void *task)
{
	int res;
	res = 		PP_rmtCfg_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCfg_do_rcvMsg((PrvtProt_task_t*)task,&PP_rmtCfg) ||
				PP_rmtCfg_do_wait((PrvtProt_task_t*)task) ||
				PP_rmtCfg_do_checkConfig((PrvtProt_task_t*)task,&PP_rmtCfg);
	return res;
}

/******************************************************
*函数名：PP_rmtCfg_do_checksock

*形  参：void

*返回值：void

*描  述：检查socket连接

*备  注：
******************************************************/
static int PP_rmtCfg_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}
	//释放资源
	PP_rmtCfg_reset(&PP_rmtCfg);
	return -1;
}

/******************************************************
*函数名：PP_rmtCfg_do_rcvMsg

*形  参：void

*返回值：void

*描  述：接收数据函数

*备  注：
******************************************************/
static int PP_rmtCfg_do_rcvMsg(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_REMOTE_CFG,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive remote config message");
	protocol_dump(LOG_HOZON, "remote_config_message", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//判断数据帧头有误或者数据长度不对
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//接收数据长度超出缓存buffer长度
	{
		return 0;
	}
	PP_rmtCfg_RxMsgHandle(task,rmtCfg,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*函数名：PP_rmtCfg_RxMsgHandle

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PP_rmtCfg_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	int res;
	int i;
	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	if(0 != PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&AppData_rmtCfg))
	{
		log_e(LOG_HOZON, "decode error\r\n");
		return;
	}

	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_RMTCFG != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}
	switch(MsgDataBody.mID)
	{
		case PP_MID_CHECK_CFG_RESP://check remote config response
		{
			if(PP_rmtCfg.state.waitSt == PP_RMTCFG_CHECK_WAIT_RESP)//接收到回复
			{
				if((1 == AppData_rmtCfg.checkResp.needUpdate) \
						/*&& (PP_rmtCfg_strcmp(AppData_rmtCfg.checkResp.cfgVersion,AppData_rmtCfg.checkReq.cfgVersion,32)!=0)*/)
				{
					rmtCfg->state.needUpdata = 1;
				}
				AppData_rmtCfg.checkResp.needUpdate = 0;
				rmtCfg->state.waitSt = 0;
				log_i(LOG_HOZON, "\r\ncheck config req ok\r\n");
			}
		}
		break;
		case PP_MID_GET_CFG_RESP://get remote config response
		{
			if(rmtCfg->state.waitSt == PP_RMTCFG_GET_WAIT_RESP)//接收到回复
			{
				rmtCfg->state.waitSt = 0;

				log_i(LOG_HOZON, "\r\nget config req ok\r\n");
			}
		}
		break;
		case PP_MID_CONN_CFG_REQ://TAP 向 TCU 发送配置更新请求， TCU 选择合适的时机去后台请求数据
		{
			rmtCfg->pack.DisBody.eventId = MsgDataBody.eventId;
			rmtCfg->state.cfgAccept = 1;//接受配置更新请求
			res = PP_rmtCfg_ConnResp(task,rmtCfg);
			if(res > 0)
			{
				rmtCfg->state.req  = AppData_rmtCfg.connResp.configAccepted;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
			}
			else if(res < 0)
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
			}
			else
			{}
		}
		break;
		case PP_MID_READ_CFG_REQ://TAP 向 TCU 读配置请求
		{
			memset(AppData_rmtCfg.ReadResp.readreq,0,PP_RMTCFG_SETID_MAX);
			for(i = 0; i < AppData_rmtCfg.ReadReq.SettingIdlen;i++)
			{
				AppData_rmtCfg.ReadResp.readreq[AppData_rmtCfg.ReadReq.SettingId[i] -1] = 1;
			}
			rmtCfg->pack.DisBody.eventId = MsgDataBody.eventId;
			res = PP_rmtCfg_ReadCfgResp(task,rmtCfg);
			if(res < 0)
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
			}
			for(i = 0; i < PP_RMTCFG_SETID_MAX;i++)
			{
				AppData_rmtCfg.ReadReq.SettingId[i] = 0;
			}
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_rmtCfg_do_wait

*形  参：void

*返回值：void

*描  述：检查是否有事件等待应答

*备  注：
******************************************************/
static int PP_rmtCfg_do_wait(PrvtProt_task_t *task)
{

	return 0;
}

/******************************************************
*函数名：PP_rmtCfg_do_checkConfig

*形  参：

*返回值：

*描  述：检查配置

*备  注：
******************************************************/
static int PP_rmtCfg_do_checkConfig(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int res;

	if(1 != sockproxy_socketState())//socket open
	{
		return 0;
	}

	switch(rmtCfg->state.CfgSt)
	{
		case PP_RMTCFG_CFG_IDLE:
		{
			if((1 == rmtCfg->state.req) && (rmtCfg->state.reqCnt < 10))
			{
				if((tm_get_time() - rmtCfg->state.period) >= RMTCFG_DELAY_TIME)
				{
					log_i(LOG_HOZON, "start request remote config\r\n");
					rmtCfg->state.CfgSt = PP_CHECK_CFG_REQ;
					rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					rmtCfg->state.reqCnt++;
				}
			}
			else
			{
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.req = 0;
			}
		}
		break;
		case PP_CHECK_CFG_REQ:
		{
			res = PP_rmtCfg_checkRequest(task,rmtCfg);
			if(res < 0)//请求发送失败
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 1;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				rmtCfg->state.waitSt 		= PP_RMTCFG_CHECK_WAIT_RESP;//等待检查检查配置请求响应
				rmtCfg->state.CfgSt 		= PP_CHECK_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_CHECK_CFG_RESP:
		{
			if(rmtCfg->state.waitSt != PP_RMTCFG_WAIT_IDLE)//未响应
			{
				 if((tm_get_time() - rmtCfg->state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//等待响应超时
				 {
					 log_e(LOG_HOZON, "check cfg response time out");
					 rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 rmtCfg->state.period = tm_get_time();
				 }
			}
			else
			{
				if(rmtCfg->state.needUpdata == 1)
				{
					rmtCfg->state.needUpdata = 0;
					rmtCfg->state.CfgSt = PP_GET_CFG_REQ;
				}
				else
				{
					rmtCfg->state.req = 0;
					rmtCfg->state.reqCnt = 0;
					rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;

				}
			}
		}
		break;
		case PP_GET_CFG_REQ:
		{
			memset(&(AppData_rmtCfg.getResp),0,sizeof(App_rmtCfg_getResp_t));
			res = PP_rmtCfg_getRequest(task,rmtCfg);
			if(res < 0)//请求失败
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 1;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				rmtCfg->state.waitSt 		= PP_RMTCFG_GET_WAIT_RESP;//等待请求响应
				rmtCfg->state.CfgSt 		= PP_GET_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_GET_CFG_RESP:
		{
			if(rmtCfg->state.waitSt != PP_RMTCFG_WAIT_IDLE)//未响应
			{
				 if((tm_get_time() - rmtCfg->state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//等待响应超时
				 {
					 log_e(LOG_HOZON, "get cfg response time out");
					 rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 rmtCfg->state.period = tm_get_time();
				 }
			}
			else
			{
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_END;
			}
		}
		break;
		case PP_RMTCFG_CFG_END://结束配置
		{
			if(AppData_rmtCfg.getResp.result == 1)
			{//缓存配置，并通知服务器配置完成状态
				AppData_rmtCfg.getResp.result = 0;
				memcpy(AppData_rmtCfg.ReadResp.cfgVersion,AppData_rmtCfg.checkResp.cfgVersion, \
														  AppData_rmtCfg.checkResp.cfgVersionlen);
				AppData_rmtCfg.ReadResp.cfgVersionlen = AppData_rmtCfg.checkResp.cfgVersionlen;
				//AppData_rmtCfg.ReadResp.FICM.ficmConfigValid = AppData_rmtCfg.getResp.FICM.ficmConfigValid;
				memcpy(&(AppData_rmtCfg.ReadResp.FICM),&(AppData_rmtCfg.getResp.FICM),sizeof(App_rmtCfg_FICM_t));
				memcpy(&(AppData_rmtCfg.ReadResp.APN1),&(AppData_rmtCfg.getResp.APN1),sizeof(App_rmtCfg_APN1_t));
				memcpy(&(AppData_rmtCfg.ReadResp.APN2),&(AppData_rmtCfg.getResp.APN2),sizeof(App_rmtCfg_APN2_t));
				memcpy(&(AppData_rmtCfg.ReadResp.COMMON),&(AppData_rmtCfg.getResp.COMMON),sizeof(App_rmtCfg_COMMON_t));
				memcpy(&(AppData_rmtCfg.ReadResp.EXTEND),&(AppData_rmtCfg.getResp.EXTEND),sizeof(App_rmtCfg_EXTEND_t));

				rmtCfg->state.cfgsuccess = 1;
			}

			res = PP_rmtCfg_CfgEndRequest(task,rmtCfg);
			if(res < 0)//发送失败
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 1;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)//发送ok
			{
				log_i(LOG_HOZON, "remote config take effect\r\n");
				if(rmtCfg->state.cfgsuccess == 1)
				{
					rmtCfg->state.cfgsuccess = 0;
					//保存配置，使用新的配置
					//
					//
					memcpy(AppData_rmtCfg.checkReq.cfgVersion,AppData_rmtCfg.checkResp.cfgVersion, \
																			  AppData_rmtCfg.checkResp.cfgVersionlen);
					AppData_rmtCfg.checkReq.cfgVersionlen = AppData_rmtCfg.checkResp.cfgVersionlen;
				}
				rmtCfg->state.waitSt  = PP_RMTCFG_WAIT_IDLE;
				rmtCfg->state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
				rmtCfg->state.req 	= 0;
				rmtCfg->state.reqCnt 	= 0;
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
*函数名：PP_rmtCfg_checkRequest

*形  参：

*返回值：

*描  述：check remote config request

*备  注：
******************************************************/
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtCfg->pack.DisBody.mID = PP_MID_CHECK_CFG_REQ;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_CHECK_CFG_REQ;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CHECK_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "check_remote_config_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名：PP_rmtCfg_getRequest

*形  参：

*返回值：

*描  述：get remote config request

*备  注：
******************************************************/
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtCfg->pack.DisBody.mID = PP_MID_GET_CFG_REQ;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_GET_CFG_REQ;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	memcpy(AppData_rmtCfg.getReq.cfgVersion,AppData_rmtCfg.checkResp.cfgVersion,AppData_rmtCfg.checkResp.cfgVersionlen);
	AppData_rmtCfg.getReq.cfgVersionlen = AppData_rmtCfg.checkResp.cfgVersionlen;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_GET_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_config_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名：PP_rmtCfg_CfgEndRequest

*形  参：

*返回值：

*描  述：remote config end request

*备  注：
******************************************************/
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtCfg->pack.DisBody.mID = PP_MID_CFG_END;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_CFG_END;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	AppData_rmtCfg.EndReq.configSuccess = rmtCfg->state.cfgsuccess;
	memcpy(AppData_rmtCfg.EndReq.mcuSw,AppData_rmtCfg.checkReq.mcuSw,AppData_rmtCfg.checkReq.mcuSwlen);
	AppData_rmtCfg.EndReq.mcuSwlen = AppData_rmtCfg.checkReq.mcuSwlen;
	memcpy(AppData_rmtCfg.EndReq.mpuSw,AppData_rmtCfg.checkReq.mpuSw,AppData_rmtCfg.checkReq.mpuSwlen);
	AppData_rmtCfg.EndReq.mpuSwlen = AppData_rmtCfg.checkReq.mpuSwlen;
	memcpy(AppData_rmtCfg.EndReq.configSw,AppData_rmtCfg.checkReq.configSw,AppData_rmtCfg.checkReq.configSwlen);
	AppData_rmtCfg.EndReq.configSwlen = AppData_rmtCfg.checkReq.configSwlen;
	memcpy(AppData_rmtCfg.EndReq.cfgVersion,AppData_rmtCfg.checkReq.cfgVersion,AppData_rmtCfg.checkReq.cfgVersionlen);
	AppData_rmtCfg.EndReq.cfgVersionlen = AppData_rmtCfg.checkReq.cfgVersionlen;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_END_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);//发送

	protocol_dump(LOG_HOZON, "remote_config_end_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名：PP_rmtCfg_ConnResp

*形  参：

*返回值：

*描  述： response of remote config request

*备  注：
******************************************************/
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtCfg->pack.DisBody.mID = PP_MID_CONN_CFG_RESP;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	AppData_rmtCfg.connResp.configAccepted = 0;
	if(1 == rmtCfg->state.cfgAccept)
	{
		AppData_rmtCfg.connResp.configAccepted = 1;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CONN_RESP,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "remote_config_conn_resp", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名：PP_rmtCfg_ReadCfgResp

*形  参：

*返回值：

*描  述： response of remote read config request

*备  注：
******************************************************/
static int PP_rmtCfg_ReadCfgResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtCfg->pack.DisBody.mID = PP_MID_READ_CFG_RESP;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	AppData_rmtCfg.ReadResp.result = 1;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_READ_RESP,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "remote_config_conn_resp", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*函数名:PP_rmtCfg_SetCfgReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_rmtCfg_SetCfgReq(unsigned char req)
{
	PP_rmtCfg.state.req  = req;
	PP_rmtCfg.state.period = tm_get_time();
}

#if 0
/******************************************************
*函数名:PP_rmtCfg_strcmp

*形  参：

*返回值：

*描  述：字符串比较

*备  注：
******************************************************/
static int PP_rmtCfg_strcmp(unsigned char* str1,unsigned char* str2,int len)
{
	int i;
	for(i = 0;i < len;i++)
	{
		if(str1[i] != str2[i])
		{
			return -1;
		}
	}
	return 0;
}
#endif

/******************************************************
*函数名:PP_rmtCfg_reset

*形  参：

*返回值：

*描  述：字符串比较

*备  注：
******************************************************/
static void PP_rmtCfg_reset(PrvtProt_rmtCfg_t *rmtCfg)
{
	rmtCfg->state.CfgSt = 0;
	rmtCfg->state.cfgAccept = 0;
	rmtCfg->state.cfgsuccess = 0;
	rmtCfg->state.needUpdata = 0;
	rmtCfg->state.req = 0;
	rmtCfg->state.period = 0;
	rmtCfg->state.reqCnt = 0;
	rmtCfg->state.waitSt = 0;
	rmtCfg->state.waittime = 0;
}
