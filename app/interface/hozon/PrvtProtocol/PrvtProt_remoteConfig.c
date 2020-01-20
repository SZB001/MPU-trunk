/******************************************************
�ļ�����	PrvtProt_remoteCfg.c

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
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
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
#include "dev_api.h"
#include "init.h"
#include "ble.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "../../../base/uds/server/uds_did.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "cfg_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "uds.h"
#include "PrvtProt_remoteConfig.h"

/*******************************************************
description�� global variable definitions
*******************************************************/



/*******************************************************
description�� static variable definitions
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
}__attribute__((packed))  PrvtProt_rmtCfg_t; /*�ṹ��*/

static PrvtProt_rmtCfg_t		PP_rmtCfg;
static App_rmtCfg_getResp_t 	AppDt_getResp;

static pthread_mutex_t cfgdtmtx = 	PTHREAD_MUTEX_INITIALIZER;
/*******************************************************
description�� function declaration
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
static int PP_rmtCfg_ReadCfgResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_DisptrBody_t *MsgDataBody);
static void PP_rmtCfg_reset(PrvtProt_rmtCfg_t *rmtCfg);
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_DisptrBody_t *MsgDataBody);

static void PP_rmtCfg_send_cb(void * para);
static void PP_rmtCfg_HexToStr(uint8_t *pbDest, uint8_t *pbSrc, int nLen);
static void getPP_rmtCfg_localConfig(PrvtProt_App_rmtCfg_t* appDt_rmtCfg);
static uint8_t PP_rmtCfg_is_empty(uint8_t *dt,int len);
static void PP_rmtCfg_settbox(App_rmtCfg_getResp_t *rmtCfg);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_rmtCfg_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_rmtCfg_init(void)
{
	memset(&PP_rmtCfg,0 , sizeof(PrvtProt_rmtCfg_t));

	PP_rmtCfg.state.avtivecheckflag = 0;
	PP_rmtCfg.state.iccidValid = 0;
	PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
}

/*
* 读取本地配置信息
*/
static void getPP_rmtCfg_localConfig(PrvtProt_App_rmtCfg_t* appDt_rmtCfg)
{
	unsigned int len;
	int res;
	uint8_t cfgsuccess;

	pthread_mutex_lock(&cfgdtmtx);
	len = 33;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_VER,appDt_rmtCfg->ReadResp.cfgVersion,&len);
	appDt_rmtCfg->ReadResp.cfgVersionlen = 32;
	len = 1;
	res = cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	if((res==0) && (cfgsuccess & RMTCFG_TSP_CONFIG))
	{
		pthread_mutex_lock(&cfgdtmtx);

		len = sizeof(App_rmtCfg_APN1_t);
		cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&appDt_rmtCfg->ReadResp.APN1,&len);
		len = sizeof(App_rmtCfg_APN2_t);
		cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN2,&appDt_rmtCfg->ReadResp.APN2,&len);
		len = sizeof(App_rmtCfg_FICM_t);
		cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&appDt_rmtCfg->ReadResp.FICM,&len);
		len = sizeof(App_rmtCfg_COMMON_t);
		cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&appDt_rmtCfg->ReadResp.COMMON,&len);
		len = sizeof(App_rmtCfg_EXTEND_t);
		cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_EXT,&appDt_rmtCfg->ReadResp.EXTEND,&len);

		pthread_mutex_unlock(&cfgdtmtx);
	}
	else
	{
		log_o(LOG_HOZON,"cfgsuccess = %d",cfgsuccess);
		appDt_rmtCfg->ReadResp.APN1.apn1ConfigValid = 0;
		appDt_rmtCfg->ReadResp.APN2.apn2ConfigValid = 0;
		appDt_rmtCfg->ReadResp.COMMON.commonConfigValid = 0;
		appDt_rmtCfg->ReadResp.EXTEND.extendConfigValid= 0;
		appDt_rmtCfg->ReadResp.FICM.ficmConfigValid = 0;
	}
}

/******************************************************
*��������PP_rmtCfg_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_rmtCfg_mainfunction(void *task)
{
	int res;

	if(!dev_get_KL15_signal())
	{
		PP_rmtCfg.state.avtivecheckflag = 0;
		PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
		return 0;
	}

	res = 		PP_rmtCfg_do_checksock((PrvtProt_task_t*)task) || \
				PP_rmtCfg_do_rcvMsg((PrvtProt_task_t*)task,&PP_rmtCfg) || \
				PP_rmtCfg_do_wait((PrvtProt_task_t*)task) || \
				PP_rmtCfg_do_checkConfig((PrvtProt_task_t*)task,&PP_rmtCfg);
	return res;
}

/******************************************************
*��������PP_rmtCfg_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_rmtCfg_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}

	PP_rmtCfg_reset(&PP_rmtCfg);
	return -1;
}

/******************************************************
*��������PP_rmtCfg_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
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
			(rlen <= 18))
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))
	{
		return 0;
	}
	PP_rmtCfg_RxMsgHandle(task,rmtCfg,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_rmtCfg_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	int i;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	if(PP_OPERATETYPE_NGTP != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	if(0 != PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&App_rmtCfg))
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
			if(rmtCfg->state.waitSt == PP_RMTCFG_CHECK_WAIT_RESP)
			{
				if(1 == App_rmtCfg.checkResp.needUpdate)
				{
					rmtCfg->state.needUpdata = 1;
					memset(rmtCfg->state.newCfgVersion,0,33);
					memcpy(rmtCfg->state.newCfgVersion,App_rmtCfg.checkResp.cfgVersion, \
														  App_rmtCfg.checkResp.cfgVersionlen);								  
				}
				App_rmtCfg.checkResp.needUpdate = 0;
				rmtCfg->state.waitSt = 0;
				log_o(LOG_HOZON, "\r\ncheck config req ok\r\n");
			}
		}
		break;
		case PP_MID_GET_CFG_RESP://get remote config response
		{
			if(rmtCfg->state.waitSt == PP_RMTCFG_GET_WAIT_RESP)
			{
				rmtCfg->state.waitSt = 0;
				memset(&AppDt_getResp,0,sizeof(App_rmtCfg_getResp_t));
				rmtCfg->state.getRespResult = App_rmtCfg.getResp.result;
				memcpy(&AppDt_getResp.FICM,&App_rmtCfg.getResp.FICM,sizeof(App_rmtCfg_FICM_t));
				memcpy(&AppDt_getResp.APN1,&App_rmtCfg.getResp.APN1,sizeof(App_rmtCfg_APN1_t));
				memcpy(&AppDt_getResp.APN2,&App_rmtCfg.getResp.APN2,sizeof(App_rmtCfg_APN2_t));
				memcpy(&AppDt_getResp.COMMON,&App_rmtCfg.getResp.COMMON,sizeof(App_rmtCfg_COMMON_t));
				memcpy(&AppDt_getResp.EXTEND,&App_rmtCfg.getResp.EXTEND,sizeof(App_rmtCfg_EXTEND_t));
				log_o(LOG_HOZON, "\r\nget config req ok\r\n");
			}
		}
		break;
		case PP_MID_CONN_CFG_REQ:
		{
			rmtCfg->state.req 	= 0;
			rmtCfg->state.reqCnt 	= 0;
			rmtCfg->state.waitSt  = PP_RMTCFG_WAIT_IDLE;
			rmtCfg->state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
			rmtCfg->state.cfgAccept = 1;
			PP_rmtCfg_ConnResp(task,rmtCfg,&MsgDataBody);
		}
		break;
		case PP_MID_READ_CFG_REQ:
		{
			memset(rmtCfg->state.readreq,0,PP_RMTCFG_SETID_MAX);
			for(i = 0; i < App_rmtCfg.ReadReq.SettingIdlen;i++)
			{
				rmtCfg->state.readreq[App_rmtCfg.ReadReq.SettingId[i] -1] = 1;
			}

			PP_rmtCfg_ReadCfgResp(task,rmtCfg,&MsgDataBody);

			for(i = 0; i < PP_RMTCFG_SETID_MAX;i++)
			{
				App_rmtCfg.ReadReq.SettingId[i] = 0;
			}
		}
		break;
		default:
		break;
	}
}

/******************************************************
*��������PP_rmtCfg_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_rmtCfg_do_wait(PrvtProt_task_t *task)
{
	if(!PP_rmtCfg.state.waitSt)
	{
		return 0;
	}

	if(PP_rmtCfg.state.waitSt == PP_RMTCFG_CHECK_WAIT_RESP)
	{
		if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)
		{
			log_e(LOG_HOZON, "check cfg response time out");
			PP_rmtCfg.state.reqCnt = 0;
			PP_rmtCfg.state.req = 0;
			PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
			PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
		}
	}
	else if(PP_rmtCfg.state.waitSt == PP_RMTCFG_GET_WAIT_RESP)
	{
		 if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)
		 {
			 log_e(LOG_HOZON, "get cfg response time out");
			 PP_rmtCfg.state.reqCnt = 0;
			 PP_rmtCfg.state.req = 0;
			 PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
			 PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
		 }
	}
	else if(PP_rmtCfg.state.waitSt == PP_RMTCFG_END_WAIT_SENDRESP)
	{
		if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)
		{
			PP_rmtCfg.state.reqCnt = 0;
			PP_rmtCfg.state.req = 0;
			PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
			PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
		}
	}
	return -1;
}

/******************************************************
*��������PP_rmtCfg_do_checkConfig

*��  �Σ�

*����ֵ��

*��  �����������

*��  ע��
******************************************************/
static int PP_rmtCfg_do_checkConfig(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	unsigned int len;
	uint8_t iccid[21] = {0};

	if(0 == rmtCfg->state.avtivecheckflag)
	{//上电主动查询配置
		rmtCfg->state.req  = 1;
		rmtCfg->state.reqCnt = 0;
		rmtCfg->state.period = tm_get_time();
		rmtCfg->state.delaytime = tm_get_time();
		rmtCfg->state.avtivecheckflag = 1;
	}

	(void)PrvtProtCfg_get_iccid((char *)(iccid));
	if(0 == PP_rmtCfg_is_empty(iccid,21))
	{
		rmtCfg->state.iccidValid = 1;
	}

	switch(rmtCfg->state.CfgSt)
	{
		case PP_RMTCFG_CFG_IDLE:
		{
			if(((1 == rmtCfg->state.iccidValid) && ((tm_get_time() - rmtCfg->state.delaytime) >= 15000)) || \
					((tm_get_time() - rmtCfg->state.delaytime) >= 30000))
			{
				if((1 == rmtCfg->state.req) && (rmtCfg->state.reqCnt < PP_RETRANSMIT_TIMES))
				{
					if((tm_get_time() - rmtCfg->state.period) >= RMTCFG_DELAY_TIME)
					{
						log_o(LOG_HOZON, "start request remote config\r\n");
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
		}
		break;
		case PP_CHECK_CFG_REQ:
		{
			if(0 == PP_rmtCfg_checkRequest(task,rmtCfg))
			{
				rmtCfg->state.waitSt 	= PP_RMTCFG_CHECK_WAIT_RESP;
				rmtCfg->state.CfgSt 	= PP_CHECK_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{
				rmtCfg->state.waitSt 	= PP_RMTCFG_WAIT_IDLE;
				rmtCfg->state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
			}
		}
		break;
		case PP_CHECK_CFG_RESP:
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
				log_o(LOG_HOZON, "noneed updata\n");
			}
		}
		break;
		case PP_GET_CFG_REQ:
		{
			if(0 == PP_rmtCfg_getRequest(task,rmtCfg))
			{
				rmtCfg->state.waitSt 	= PP_RMTCFG_GET_WAIT_RESP;
				rmtCfg->state.CfgSt 	= PP_GET_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{
				rmtCfg->state.waitSt 	= PP_RMTCFG_WAIT_IDLE;
				rmtCfg->state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
			}
		}
		break;
		case PP_GET_CFG_RESP:
		{
			rmtCfg->state.CfgSt = PP_RMTCFG_CFG_END;
		}
		break;
		case PP_RMTCFG_CFG_END://
		{
			App_rmtCfg_CfgReadResp_t app_ReadResp;
			if(rmtCfg->state.getRespResult == 1)
			{
				rmtCfg->state.getRespResult = 0;
				pthread_mutex_lock(&cfgdtmtx);
				len = sizeof(App_rmtCfg_APN1_t);
				cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&app_ReadResp.APN1,&len);
				len = sizeof(App_rmtCfg_APN2_t);
				cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN2,&app_ReadResp.APN2,&len);
				len = sizeof(App_rmtCfg_FICM_t);
				cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&app_ReadResp.FICM,&len);
				len = sizeof(App_rmtCfg_COMMON_t);
				cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&app_ReadResp.COMMON,&len);
				len = sizeof(App_rmtCfg_EXTEND_t);
				cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_EXT,&app_ReadResp.EXTEND,&len);
				pthread_mutex_unlock(&cfgdtmtx);
				//if(1 == AppDt_getResp.FICM.ficmConfigValid)
				{
				//	memcpy(&(AppData_rmtCfg.ReadResp.FICM),&AppDt_getResp.FICM,sizeof(App_rmtCfg_FICM_t));
				}

				if(1 == AppDt_getResp.APN1.apn1ConfigValid)
				{
					if((0 != strcmp((const char*)app_ReadResp.APN1.tspAddr,(const char*)AppDt_getResp.APN1.tspAddr)) || \
						(0 != strcmp((const char*)app_ReadResp.APN1.tspPort,(const char*)AppDt_getResp.APN1.tspPort)))
					{
						rmtCfg->state.apn1tspaddrchangedflag = 1;
					}

					if((0 != strcmp((const char*)app_ReadResp.APN1.certAddress,(const char*)AppDt_getResp.APN1.certAddress)) || \
						(0 != strcmp((const char*)app_ReadResp.APN1.certPort,(const char*)AppDt_getResp.APN1.certPort)))
					{
						rmtCfg->state.apn1certaddrchangeflag = 1;
					}

					if(0 != strcmp((const char*)app_ReadResp.APN1.tspSms, \
									(const char*)AppDt_getResp.APN1.tspSms))
					{
						rmtCfg->state.tspSMSchangeflag = 1;
					}
				}

				if(1 == AppDt_getResp.APN2.apn2ConfigValid)
				{
					//memcpy(&(AppData_rmtCfg.ReadResp.APN2),&(AppData_rmtCfg.getResp.APN2),sizeof(App_rmtCfg_APN2_t));
				}
		
				if(1 == AppDt_getResp.COMMON.commonConfigValid)
				{
					//memcpy(&(AppData_rmtCfg.ReadResp.COMMON),&(AppData_rmtCfg.getResp.COMMON),sizeof(App_rmtCfg_COMMON_t));
				}

				if(1 == AppDt_getResp.EXTEND.extendConfigValid)
				{
					//memcpy(&(AppData_rmtCfg.ReadResp.EXTEND),&(AppData_rmtCfg.getResp.EXTEND),sizeof(App_rmtCfg_EXTEND_t));
				}

				rmtCfg->state.cfgsuccess = 1;
			}

			if(0 == PP_rmtCfg_CfgEndRequest(task,rmtCfg))
			{
				rmtCfg->state.waitSt 	= PP_RMTCFG_END_WAIT_SENDRESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{
				rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_rmtCfg_checkRequest

*��  �Σ�

*����ֵ��

*��  ����check remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int idlenode;
	PrvtProt_pack_t 		rmtCfg_Pack;
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	memset(&rmtCfg_Pack,0,sizeof(PrvtProt_pack_t));
	memset(&DisBody,0,sizeof(PrvtProt_DisptrBody_t));
	memset(&App_rmtCfg,0,sizeof(PrvtProt_App_rmtCfg_t));
	
	/*header*/
	memcpy(rmtCfg_Pack.Header.sign,"**",2);
	rmtCfg_Pack.Header.commtype.Byte = 0xe1;
	rmtCfg_Pack.Header.opera = 0x02;
	rmtCfg_Pack.Header.ver.Byte = task->version;
	rmtCfg_Pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg_Pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/*body*/
	memcpy(DisBody.aID,"100",3);
	DisBody.mID = PP_MID_CHECK_CFG_REQ;
	DisBody.eventId = 0;//主动上行数据，eventid为0
	DisBody.eventTime = PrvtPro_getTimestamp();
	DisBody.expTime   = PP_rmtCfg.state.expTime;
	DisBody.ulMsgCnt++;	/* OPTIONAL */
	DisBody.appDataProVer = 256;
	DisBody.testFlag = 1;
	DisBody.appDataEncode = PP_APPDATA_ENCODING_UPER;
	/*appdata*/
	unsigned int len;
	memcpy(App_rmtCfg.checkReq.mcuSw,DID_F1B0_SW_UPGRADE_VER,strlen(DID_F1B0_SW_UPGRADE_VER));
	App_rmtCfg.checkReq.mcuSwlen = strlen(DID_F1B0_SW_UPGRADE_VER);
	memcpy(App_rmtCfg.checkReq.mpuSw,DID_F1B0_SW_UPGRADE_VER,strlen(DID_F1B0_SW_UPGRADE_VER));
	App_rmtCfg.checkReq.mpuSwlen = strlen(DID_F1B0_SW_UPGRADE_VER);

	pthread_mutex_lock(&cfgdtmtx);
	len = 18;
	cfg_get_user_para(CFG_ITEM_GB32960_VIN,App_rmtCfg.checkReq.vehicleVin,&len);//vin
	App_rmtCfg.checkReq.vehicleVinlen = 17;
	pthread_mutex_unlock(&cfgdtmtx);

	(void)PrvtProtCfg_get_iccid((char *)(App_rmtCfg.checkReq.iccID));
	App_rmtCfg.checkReq.iccIDlen = 20;

	unsigned char Mac[32];
	BleGetMac(Mac);
	PP_rmtCfg_HexToStr(App_rmtCfg.checkReq.btMacAddr,Mac,6);
	App_rmtCfg.checkReq.btMacAddrlen = 12;

	memcpy(App_rmtCfg.checkReq.configSw,"00000",strlen("00000"));
	App_rmtCfg.checkReq.configSwlen = strlen("00000");

	pthread_mutex_lock(&cfgdtmtx);
	len = 33;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_VER,App_rmtCfg.checkReq.cfgVersion,&len);
	App_rmtCfg.checkReq.cfgVersionlen = 32;
	pthread_mutex_unlock(&cfgdtmtx);

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CHECK_REQ,rmtCfg_Pack.msgdata,&msgdatalen,\
									   &DisBody,&App_rmtCfg))
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	rmtCfg_Pack.totallen = 18 + msgdatalen;
	rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_RMTCFG;
	PP_TxInform[idlenode].mid = PP_MID_CHECK_CFG_REQ;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "check cfg req";
	SP_data_write(rmtCfg_Pack.Header.sign,rmtCfg_Pack.totallen, \
			PP_rmtCfg_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_getRequest

*��  �Σ�

*����ֵ��

*��  ����get remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int idlenode;
	PrvtProt_pack_t 		rmtCfg_Pack;
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	memset(&rmtCfg_Pack,0,sizeof(PrvtProt_pack_t));
	memset(&DisBody,0,sizeof(PrvtProt_DisptrBody_t));
	memset(&App_rmtCfg,0,sizeof(PrvtProt_App_rmtCfg_t));
	
	/*header*/
	memcpy(rmtCfg_Pack.Header.sign,"**",2);
	rmtCfg_Pack.Header.commtype.Byte = 0xe1;
	rmtCfg_Pack.Header.opera = 0x02;
	rmtCfg_Pack.Header.ver.Byte = task->version;
	rmtCfg_Pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg_Pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/*body*/
	memcpy(DisBody.aID,"100",3);
	DisBody.mID = PP_MID_GET_CFG_REQ;
	DisBody.eventId = 0;//主动上行数据，eventid为0
	DisBody.eventTime = PrvtPro_getTimestamp();
	DisBody.expTime   = PP_rmtCfg.state.expTime;
	DisBody.ulMsgCnt++;	/* OPTIONAL */
	DisBody.appDataProVer = 256;
	DisBody.testFlag = 1;
	DisBody.appDataEncode = PP_APPDATA_ENCODING_UPER;
	/*appdata*/
	memcpy(App_rmtCfg.getReq.cfgVersion,rmtCfg->state.newCfgVersion,32);
	App_rmtCfg.getReq.cfgVersionlen = 32;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_GET_REQ,rmtCfg_Pack.msgdata,&msgdatalen,\
									   &DisBody,&App_rmtCfg))
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	rmtCfg_Pack.totallen = 18 + msgdatalen;
	rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_RMTCFG;
	PP_TxInform[idlenode].mid = PP_MID_GET_CFG_REQ;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "get cfg req";
	SP_data_write(rmtCfg_Pack.Header.sign,rmtCfg_Pack.totallen, \
			PP_rmtCfg_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_CfgEndRequest

*��  �Σ�

*����ֵ��

*��  ����remote config end request

*��  ע��
******************************************************/
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int idlenode;
	PrvtProt_pack_t 		rmtCfg_Pack;
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	memset(&rmtCfg_Pack,0,sizeof(PrvtProt_pack_t));
	memset(&DisBody,0,sizeof(PrvtProt_DisptrBody_t));
	memset(&App_rmtCfg,0,sizeof(PrvtProt_App_rmtCfg_t));

	/*header*/
	memcpy(rmtCfg_Pack.Header.sign,"**",2);
	rmtCfg_Pack.Header.commtype.Byte = 0xe1;
	rmtCfg_Pack.Header.opera = 0x02;
	rmtCfg_Pack.Header.ver.Byte = task->version;
	rmtCfg_Pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg_Pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/*body*/
	memcpy(DisBody.aID,"100",3);
	DisBody.mID = PP_MID_CFG_END;
	DisBody.eventId = 0;//主动上行数据，eventid为0
	DisBody.eventTime = PrvtPro_getTimestamp();
	DisBody.expTime   = PP_rmtCfg.state.expTime;
	DisBody.ulMsgCnt++;	/* OPTIONAL */
	DisBody.appDataProVer = 256;
	DisBody.testFlag = 1;
	DisBody.appDataEncode = PP_APPDATA_ENCODING_UPER;
	/*appdata*/
	App_rmtCfg.EndReq.configSuccess = rmtCfg->state.cfgsuccess;
	memcpy(App_rmtCfg.EndReq.mcuSw,DID_F1B0_SW_UPGRADE_VER,strlen(DID_F1B0_SW_UPGRADE_VER));
	App_rmtCfg.EndReq.mcuSwlen = strlen(DID_F1B0_SW_UPGRADE_VER);
	memcpy(App_rmtCfg.EndReq.mpuSw,DID_F1B0_SW_UPGRADE_VER,strlen(DID_F1B0_SW_UPGRADE_VER));
	App_rmtCfg.EndReq.mpuSwlen = strlen(DID_F1B0_SW_UPGRADE_VER);
	memcpy(App_rmtCfg.EndReq.configSw,"00000",strlen("00000"));
	App_rmtCfg.EndReq.configSwlen = strlen("00000");

	memcpy(App_rmtCfg.EndReq.cfgVersion,rmtCfg->state.newCfgVersion,\
							strlen((const char*)rmtCfg->state.newCfgVersion));
	App_rmtCfg.EndReq.cfgVersionlen = 32;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_END_REQ,rmtCfg_Pack.msgdata,&msgdatalen,\
									   &DisBody,&App_rmtCfg))
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	rmtCfg_Pack.totallen = 18 + msgdatalen;
	rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_RMTCFG;
	PP_TxInform[idlenode].mid = PP_MID_CFG_END;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "cfg end";
	SP_data_write(rmtCfg_Pack.Header.sign,rmtCfg_Pack.totallen, \
			PP_rmtCfg_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_ConnResp

*��  �Σ�

*����ֵ��

*��  ���� response of remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_DisptrBody_t *MsgDataBody)
{
	int msgdatalen;
	int idlenode;
	PrvtProt_pack_t 		rmtCfg_Pack;
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	memset(&rmtCfg_Pack,0,sizeof(PrvtProt_pack_t));
	memset(&DisBody,0,sizeof(PrvtProt_DisptrBody_t));
	memset(&App_rmtCfg,0,sizeof(PrvtProt_App_rmtCfg_t));

	/*header*/
	memcpy(rmtCfg_Pack.Header.sign,"**",2);
	rmtCfg_Pack.Header.commtype.Byte = 0xe1;
	rmtCfg_Pack.Header.opera = 0x02;
	rmtCfg_Pack.Header.ver.Byte = task->version;
	rmtCfg_Pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg_Pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/*body*/
	memcpy(DisBody.aID,"100",3);
	DisBody.eventId = MsgDataBody->eventId;
	DisBody.mID = PP_MID_CONN_CFG_RESP;
	DisBody.eventTime = PrvtPro_getTimestamp();
	DisBody.expTime   = MsgDataBody->expTime;
	DisBody.ulMsgCnt++;	/* OPTIONAL */
	DisBody.appDataProVer = 256;
	DisBody.testFlag = 1;
	DisBody.appDataEncode = PP_APPDATA_ENCODING_UPER;

	/*appdata*/
	App_rmtCfg.connResp.configAccepted = 0;
	if(1 == rmtCfg->state.cfgAccept)
	{
		App_rmtCfg.connResp.configAccepted = 1;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CONN_RESP,rmtCfg_Pack.msgdata,&msgdatalen,\
									   &DisBody,&App_rmtCfg))
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	rmtCfg_Pack.totallen = 18 + msgdatalen;
	rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_RMTCFG;
	PP_TxInform[idlenode].mid = PP_MID_CONN_CFG_RESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "resp of cfg req";
	SP_data_write(rmtCfg_Pack.Header.sign,rmtCfg_Pack.totallen, \
			PP_rmtCfg_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_ReadCfgResp

*��  �Σ�

*����ֵ��

*��  ���� response of remote read config request

*��  ע��
******************************************************/
static int PP_rmtCfg_ReadCfgResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,\
									PrvtProt_DisptrBody_t *MsgDataBody)
{
	int msgdatalen;
	int idlenode;
	int i;
	PrvtProt_pack_t 		rmtCfg_Pack;
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_App_rmtCfg_t	App_rmtCfg;

	memset(&rmtCfg_Pack,0,sizeof(PrvtProt_pack_t));
	memset(&DisBody,0,sizeof(PrvtProt_DisptrBody_t));
	memset(&App_rmtCfg,0,sizeof(PrvtProt_App_rmtCfg_t));
	
	/*header*/
	memcpy(rmtCfg_Pack.Header.sign,"**",2);
	rmtCfg_Pack.Header.commtype.Byte = 0xe1;
	rmtCfg_Pack.Header.opera = 0x02;
	rmtCfg_Pack.Header.ver.Byte = task->version;
	rmtCfg_Pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg_Pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	
	/*body*/
	memcpy(DisBody.aID,"100",3);
	DisBody.mID = PP_MID_READ_CFG_RESP;
	DisBody.eventId = MsgDataBody->eventId;
	DisBody.eventTime = PrvtPro_getTimestamp();
	DisBody.expTime   = MsgDataBody->expTime;
	DisBody.ulMsgCnt++;	/* OPTIONAL */
	DisBody.appDataProVer = 256;
	DisBody.testFlag = 1;
	DisBody.appDataEncode = PP_APPDATA_ENCODING_UPER;
	/*appdata*/
	App_rmtCfg.ReadResp.result = 1;

	for(i=0;i<PP_RMTCFG_SETID_MAX;i++)
	{
		App_rmtCfg.ReadResp.readreq[i] = rmtCfg->state.readreq[i];
	}

	//读取本地配置
	getPP_rmtCfg_localConfig(&App_rmtCfg);

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_READ_RESP,rmtCfg_Pack.msgdata,&msgdatalen,\
									   &DisBody,&App_rmtCfg))
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	rmtCfg_Pack.totallen = 18 + msgdatalen;
	rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_RMTCFG;
	PP_TxInform[idlenode].mid = PP_MID_READ_CFG_RESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "resp of cfg read";
	SP_data_write(rmtCfg_Pack.Header.sign,rmtCfg_Pack.totallen, \
			PP_rmtCfg_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*������:PP_rmtCfg_SetCfgReq

*��  �Σ�

*����ֵ��

*��  �������� ����

*��  ע��
******************************************************/
void PP_rmtCfg_SetCfgReq(unsigned char req)
{
	PP_rmtCfg.state.req  = req;
	PP_rmtCfg.state.reqCnt = 0;
	PP_rmtCfg.state.expTime = -1;
	PP_rmtCfg.state.period = tm_get_time();
}

/******************************************************
*������:PP_rmtCfg_setCfgEnable

*��  �Σ�

*����ֵ��

*��  �������� ����

*��  ע��
******************************************************/
void PP_rmtCfg_setCfgEnable(unsigned char obj,unsigned char enable)
{
	uint8_t cfgsuccess;
	unsigned int len;
	App_rmtCfg_COMMON_t rmtCfg_COMMON;

	len = sizeof(App_rmtCfg_COMMON_t);

	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmtCfg_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	switch(obj)
	{
		case 1:////T服务开启使能
		{
			rmtCfg_COMMON.actived = enable;
		}
		break;
		case 2://远程控制使能
		{
			rmtCfg_COMMON.rcEnabled = enable;
		}
		break;
		case 3://被盗追踪开关
		{
			rmtCfg_COMMON.svtEnabled = enable;
		}
		break;
		case 4://车辆状态开关
		{
			rmtCfg_COMMON.vsEnabled = enable;
		}
		break;
		case 5://呼叫中心开关
		{
			rmtCfg_COMMON.iCallEnabled = enable;
		}
		break;
		case 6://道路救援开关
		{
			rmtCfg_COMMON.bCallEnabled = enable;
		}
		break;
		case 7://紧急救援开关
		{
			rmtCfg_COMMON.eCallEnabled = enable;
		}
		break;
		case 8://数据采集开关
		{
			rmtCfg_COMMON.dcEnabled = enable;
		}
		break;
		case 9://远程诊断开关
		{
			rmtCfg_COMMON.dtcEnabled = enable;
		}
		break;
		case 10://行程开关
		{
			rmtCfg_COMMON.journeysEnabled = enable;
		}
		break;
		case 11://在线资讯开关
		{
			rmtCfg_COMMON.onlineInfEnabled = enable;
		}
		break;
		case 12://远程充电开关
		{
			rmtCfg_COMMON.rChargeEnabled = enable;
		}
		break;
		case 13://蓝牙钥匙开关
		{
			rmtCfg_COMMON.btKeyEntryEnabled = enable;
		}
		break;	
		case 14://车辆授权服务开关
		{
			rmtCfg_COMMON.carEmpowerEnabled = enable;
		}
		break;
		case 15://事件上报服务开关
		{
			rmtCfg_COMMON.eventReportEnabled = enable;
		}
		break;
		case 16://车辆报警服务开关
		{
			rmtCfg_COMMON.carAlarmEnabled = enable;
		}
		break;	
		case 17://心跳超时时间(s)
		{
			rmtCfg_COMMON.heartbeatTimeout = enable;
		}
		break;
		case 18://休眠心跳超时时间(s)
		{
			rmtCfg_COMMON.dormancyHeartbeatTimeout = enable;
		}
		break;
		case 19://国标报文采集打 包周期(s)
		{
			rmtCfg_COMMON.infoCollectCycle = enable;
		}
		break;
		case 20://国标报文定时上 报周期(s)
		{
			rmtCfg_COMMON.regularUpCycle = enable;
		}
		break;
		default:
		break;
	}
	pthread_mutex_lock(&cfgdtmtx);
	len = 1;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	cfgsuccess |= RMTCFG_MANUAL_CONFIG;
	rmtCfg_COMMON.commonConfigValid = 1;

	pthread_mutex_lock(&cfgdtmtx);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,1);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmtCfg_COMMON,sizeof(App_rmtCfg_COMMON_t));
	pthread_mutex_unlock(&cfgdtmtx);
}

void PP_rmtCfg_setCfgapn1(unsigned char obj,const void *data1,const void *data2)
{
	uint8_t cfgsuccess;
	unsigned int len;
	App_rmtCfg_APN1_t rmtCfg_APN1;

	len = sizeof(App_rmtCfg_APN1_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&rmtCfg_APN1,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	switch(obj)
	{
		case 1:  //TSP APN Address
		{
			strcpy((char *)rmtCfg_APN1.tspAddr,(const char*)data1);
			rmtCfg_APN1.tspAddrlen = strlen((const char*)data1);
			strcpy((char *)rmtCfg_APN1.tspPort,(const char*)data2);
			setsockproxy_bdlAddrPort((char*)rmtCfg_APN1.tspAddr, (char*)rmtCfg_APN1.tspPort);
		}
		break;
		case 2:
		{
			strcpy((char *)rmtCfg_APN1.tspUser,(const char*)data1);
		}
		break;
		case 3:
		{
			strcpy((char *)rmtCfg_APN1.tspPass,(const char*)data1);
		}
		break;
		case 4:
		{
			strcpy((char *)rmtCfg_APN1.tspIP,(const char*)data1);
			rmtCfg_APN1.tspIPlen = strlen((const char*)data1);
			strcpy((char *)rmtCfg_APN1.tspPort,(const char*)data2);
			setsockproxy_bdlAddrPort((char*)rmtCfg_APN1.tspIP,(char*)rmtCfg_APN1.tspPort);
		}
		break;
		case 5:
		{
			strcpy((char *)rmtCfg_APN1.tspSms,(const char*)data1);
		}
		break;
		case 6:
		{
			strcpy((char *)rmtCfg_APN1.certAddress,(const char*)data1);
			rmtCfg_APN1.certAddresslen = strlen((const char*)data1);
			strcpy((char *)rmtCfg_APN1.certPort,(const char*)data2);
			setsockproxy_sgAddrPort((char*)rmtCfg_APN1.certAddress, (char*)rmtCfg_APN1.certPort);
		}
		break;
		default:
		break;	
	}
	pthread_mutex_lock(&cfgdtmtx);
	len = 1;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	cfgsuccess |= RMTCFG_MANUAL_CONFIG;
	rmtCfg_APN1.apn1ConfigValid = 1;
	pthread_mutex_lock(&cfgdtmtx);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,1);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&rmtCfg_APN1,sizeof(App_rmtCfg_APN1_t));
	pthread_mutex_unlock(&cfgdtmtx);
}

void PP_rmtCfg_setCfgficm(unsigned char obj,const void *data)
{
	uint8_t cfgsuccess;
	unsigned int len;
	App_rmtCfg_FICM_t rmtCfg_FICM;

	len = sizeof(App_rmtCfg_FICM_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&rmtCfg_FICM,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	switch(obj)
	{
	  	case 1:
	  	{
	  		strcpy((char *)rmtCfg_FICM.token,(const char*)data);
	  	}
	  	break;
	  	case 2:
	  	{
	  		strcpy((char *)rmtCfg_FICM.userID,(const char*)data);
	  	}
	  	break;
	  	case 3:
	  	{
	  		rmtCfg_FICM.directConnEnable = atoi(data);
	  	}
	  	break;
	  	case 4:
	  	{
	  		strcpy((char *)rmtCfg_FICM.address,(const char*)data);
	  	}
	  	break;
	  	case 5:
	  	{
	  		strcpy((char *)rmtCfg_FICM.port,(const char*)data);
	  	}
	  	break;
		default:
		break;
	}

	pthread_mutex_lock(&cfgdtmtx);
	len = 1;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	cfgsuccess |= RMTCFG_MANUAL_CONFIG;
	rmtCfg_FICM.ficmConfigValid = 1;
	pthread_mutex_lock(&cfgdtmtx);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,1);
	(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&rmtCfg_FICM,sizeof(App_rmtCfg_FICM_t));
	pthread_mutex_unlock(&cfgdtmtx);
}

/******************************************************
*������:PP_rmtCfg_SetmcuSw

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void PP_rmtCfg_SetmcuSw(const char *mcuSw)
{
	uint8_t rmt_mcuSw[11];
	memset(rmt_mcuSw,0 , 11);
	memcpy(rmt_mcuSw,mcuSw,strlen(mcuSw));
	pthread_mutex_lock(&cfgdtmtx);
	if (cfg_set_user_para(CFG_ITEM_HOZON_TSP_MCUSW, rmt_mcuSw, sizeof(rmt_mcuSw)))
	{
		log_e(LOG_HOZON, "save mcuSw failed");
	}
	pthread_mutex_unlock(&cfgdtmtx);
}

/******************************************************
*������:PP_rmtCfg_SetmpuSw

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void PP_rmtCfg_SetmpuSw(const char *mpuSw)
{
	uint8_t rmt_mpuSw[11];
	memset(rmt_mpuSw,0 , 11);
	memcpy(rmt_mpuSw,mpuSw,strlen(mpuSw));
	pthread_mutex_lock(&cfgdtmtx);
	if (cfg_set_user_para(CFG_ITEM_HOZON_TSP_MPUSW, rmt_mpuSw, sizeof(rmt_mpuSw)))
	{
		log_e(LOG_HOZON, "save mpuSw failed");
	}
	pthread_mutex_unlock(&cfgdtmtx);
}

/******************************************************
*������:PP_rmtCfg_ShowCfgPara

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��������
******************************************************/
void PP_rmtCfg_ShowCfgPara(void)
{
	unsigned int len;
	uint8_t cfgsuccess;
	uint8_t vehiclevin[18];
	char iccid[21];
	App_rmtCfg_FICM_t 	rmt_FICM;
	App_rmtCfg_APN1_t 	rmt_APN1;
	App_rmtCfg_APN2_t 	rmt_APN2;
	App_rmtCfg_COMMON_t rmt_COMMON;
	App_rmtCfg_EXTEND_t rmt_EXTEND;

	pthread_mutex_lock(&cfgdtmtx);
	len =18;
	cfg_get_user_para(CFG_ITEM_GB32960_VIN,vehiclevin,&len);
	len = 1;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&cfgsuccess,&len);
	len = sizeof(App_rmtCfg_APN1_t);			
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&rmt_APN1,&len);
	len = sizeof(App_rmtCfg_APN2_t);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN2,&rmt_APN2,&len);
	len = sizeof(App_rmtCfg_FICM_t);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&rmt_FICM,&len);
	len = sizeof(App_rmtCfg_COMMON_t);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	len = sizeof(App_rmtCfg_EXTEND_t);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_EXT,&rmt_EXTEND,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	log_o(LOG_HOZON, "/******************************/");
	log_o(LOG_HOZON, "       remote  cfg parameter    ");
	log_o(LOG_HOZON, "/******************************/");
	log_o(LOG_HOZON, "vehicleVin = %s",vehiclevin);
	log_o(LOG_HOZON, "mcuSw = %s",DID_F1B0_SW_UPGRADE_VER);
	log_o(LOG_HOZON, "mpuSw = %s",DID_F1B0_SW_UPGRADE_VER);
	(void)PrvtProtCfg_get_iccid(iccid);
	log_o(LOG_HOZON, "ICCID = %s",iccid);

	log_o(LOG_HOZON, "cfgsuccess = %d",cfgsuccess);
	char cfgversion[256] = {0};
	getPP_rmtCfg_cfgVersion(cfgversion);
	log_o(LOG_HOZON, "cfgVersion = %s",cfgversion);
	log_o(LOG_HOZON, "configSw = %s","00000");

	unsigned char Mac[32];
	unsigned char btMacAddr[13];
	BleGetMac(Mac);
	PP_rmtCfg_HexToStr(btMacAddr,Mac,6);
	log_o(LOG_HOZON, "btMacAddr = %s",btMacAddr);

	log_o(LOG_HOZON, "\n/* FICM info */");
	log_o(LOG_HOZON, "ficmConfigValid = %d",rmt_FICM.ficmConfigValid);
	log_o(LOG_HOZON, "FICM.token = %s",rmt_FICM.token);
	log_o(LOG_HOZON, "FICM.userID = %s",rmt_FICM.userID);
	log_o(LOG_HOZON, "FICM.directConnEnable = %d",rmt_FICM.directConnEnable);
	log_o(LOG_HOZON, "FICM.address = %s",rmt_FICM.address);
	log_o(LOG_HOZON, "FICM.port = %s",rmt_FICM.port);
	log_o(LOG_HOZON, "FICM info length = %d\n",sizeof(App_rmtCfg_FICM_t));

	log_o(LOG_HOZON, "\n/* APN1 info */");
	log_o(LOG_HOZON, "apn1ConfigValid = %d",rmt_APN1.apn1ConfigValid);
	log_o(LOG_HOZON, "APN1.tspAddr = %s",rmt_APN1.tspAddr);
	log_o(LOG_HOZON, "APN1.tspUser = %s",rmt_APN1.tspUser);
	log_o(LOG_HOZON, "APN1.tspPass = %s",rmt_APN1.tspPass);
	log_o(LOG_HOZON, "APN1.tspIP = %s",rmt_APN1.tspIP);
	log_o(LOG_HOZON, "APN1.tspSms = %s",rmt_APN1.tspSms);
	log_o(LOG_HOZON, "APN1.tspPort = %s",rmt_APN1.tspPort);
	log_o(LOG_HOZON, "APN1.certAddress = %s",rmt_APN1.certAddress);
	log_o(LOG_HOZON, "APN1.certPort = %s",rmt_APN1.certPort);
	log_o(LOG_HOZON, "APN1 info length = %d\n",sizeof(App_rmtCfg_APN1_t));

	log_o(LOG_HOZON, "\n/* APN2 info */");
	log_o(LOG_HOZON, "apn2ConfigValid = %d",rmt_APN2.apn2ConfigValid);
	log_o(LOG_HOZON, "APN2.apn2Address = %s",rmt_APN2.apn2Address);
	log_o(LOG_HOZON, "APN2.apn2User = %s",rmt_APN2.apn2User);
	log_o(LOG_HOZON, "APN2.apn2Pass = %s",rmt_APN2.apn2Pass);
	log_o(LOG_HOZON, "APN2 info length = %d\n",sizeof(App_rmtCfg_APN2_t));

	log_o(LOG_HOZON, "\n/* COMMON info */");
	log_o(LOG_HOZON, "commonConfigValid = %d",rmt_COMMON.commonConfigValid);
	log_o(LOG_HOZON, "COMMON.actived = %d",rmt_COMMON.actived);
	log_o(LOG_HOZON, "COMMON.rcEnabled = %d",rmt_COMMON.rcEnabled);
	log_o(LOG_HOZON, "COMMON.svtEnabled = %d",rmt_COMMON.svtEnabled);
	log_o(LOG_HOZON, "COMMON.vsEnabled = %d",rmt_COMMON.vsEnabled);
	log_o(LOG_HOZON, "COMMON.iCallEnabled = %d",rmt_COMMON.iCallEnabled);
	log_o(LOG_HOZON, "COMMON.bCallEnabled = %d",rmt_COMMON.bCallEnabled);
	log_o(LOG_HOZON, "COMMON.eCallEnabled = %d",rmt_COMMON.eCallEnabled);
	log_o(LOG_HOZON, "COMMON.dcEnabled = %d",rmt_COMMON.dcEnabled);
	log_o(LOG_HOZON, "COMMON.dtcEnabled = %d",rmt_COMMON.dtcEnabled);
	log_o(LOG_HOZON, "COMMON.journeysEnabled = %d",rmt_COMMON.journeysEnabled);
	log_o(LOG_HOZON, "COMMON.onlineInfEnabled = %d",rmt_COMMON.onlineInfEnabled);
	log_o(LOG_HOZON, "COMMON.rChargeEnabled = %d",rmt_COMMON.rChargeEnabled);
	log_o(LOG_HOZON, "COMMON.btKeyEntryEnabled = %d",rmt_COMMON.btKeyEntryEnabled);
	log_o(LOG_HOZON, "COMMON.carEmpowerEnabled  = %d",rmt_COMMON.carEmpowerEnabled);
	log_o(LOG_HOZON, "COMMON.eventReportEnabled  = %d",rmt_COMMON.eventReportEnabled);
	log_o(LOG_HOZON, "COMMON.carAlarmEnabled  = %d",rmt_COMMON.carAlarmEnabled);
	log_o(LOG_HOZON, "COMMON.heartbeatTimeout  = %d",rmt_COMMON.heartbeatTimeout);
	log_o(LOG_HOZON, "COMMON.dormancyHeartbeatTimeout  = %d",rmt_COMMON.dormancyHeartbeatTimeout);
	log_o(LOG_HOZON, "COMMON.infoCollectCycle  = %d",rmt_COMMON.infoCollectCycle);
	log_o(LOG_HOZON, "COMMON.regularUpCycle  = %d",rmt_COMMON.regularUpCycle);
	log_o(LOG_HOZON, "COMMON info length = %d\n",sizeof(App_rmtCfg_COMMON_t));

	log_o(LOG_HOZON, "\n/* EXTEND info */");
	log_o(LOG_HOZON, "extendConfigValid = %d",rmt_EXTEND.extendConfigValid);
	log_o(LOG_HOZON, "EXTEND.ecallNO = %s",rmt_EXTEND.ecallNO);
	log_o(LOG_HOZON, "EXTEND.bcallNO = %s",rmt_EXTEND.bcallNO);
	log_o(LOG_HOZON, "EXTEND.ccNO = %s",rmt_EXTEND.ccNO);
	log_o(LOG_HOZON, "EXTEND info length = %d\n",sizeof(App_rmtCfg_EXTEND_t));
}

#if 0
/******************************************************
*������:PP_rmtCfg_strcmp

*��  �Σ�

*����ֵ��

*��  �����ַ����Ƚ�

*��  ע��
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
*������:PP_rmtCfg_reset

*��  �Σ�

*����ֵ��

*��  �����ַ����Ƚ�

*��  ע��
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

/******************************************************
*��������PP_rmtCfg_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static void PP_rmtCfg_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)para;
	log_i(LOG_HOZON, "aid = %d",TxInform_ptr->aid);
	log_i(LOG_HOZON, "mid = %d",TxInform_ptr->mid);
	log_i(LOG_HOZON, "pakgtype = %d",TxInform_ptr->pakgtype);
	log_i(LOG_HOZON, "eventtime = %d",TxInform_ptr->eventtime);
	log_i(LOG_HOZON, "successflg = %d",TxInform_ptr->successflg);
	log_i(LOG_HOZON, "failresion = %d",TxInform_ptr->failresion);
	log_i(LOG_HOZON, "txfailtime = %d",TxInform_ptr->txfailtime);

	switch(TxInform_ptr->mid)
	{
		case PP_MID_CONN_CFG_RESP:
		{
			if(PP_TXPAKG_SUCCESS == TxInform_ptr->successflg)
			{
				PP_rmtCfg.state.req  = 1;
				PP_rmtCfg.state.reqCnt = 0;
				PP_rmtCfg.state.period = tm_get_time();
			}
		}
		break;
		case PP_MID_READ_CFG_RESP:
		{

		}
		break;
		case PP_MID_CHECK_CFG_REQ:
		{
			if(PP_TXPAKG_FAIL == TxInform_ptr->successflg)//������ʧ��
			{
				PP_rmtCfg.state.req = 0;
				PP_rmtCfg.state.reqCnt = 0;
				PP_rmtCfg.state.period = tm_get_time();
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
				PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
			}
		}
		break;
		case PP_MID_GET_CFG_REQ:
		{
			if(PP_TXPAKG_FAIL == TxInform_ptr->successflg)//������ʧ��
			{
				PP_rmtCfg.state.req = 0;
				PP_rmtCfg.state.reqCnt = 0;
				PP_rmtCfg.state.period = tm_get_time();
				PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
		}
		break;
		case PP_MID_CFG_END:
		{
			if(PP_TXPAKG_SUCCESS == TxInform_ptr->successflg)//����ok
			{
				log_i(LOG_HOZON, "remote config take effect\r\n");
				if(PP_rmtCfg.state.cfgsuccess == 1)
				{
					//配置参数写入flash
					pthread_mutex_lock(&cfgdtmtx);
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_ST,&PP_rmtCfg.state.cfgsuccess,1);
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_VER,PP_rmtCfg.state.newCfgVersion,33);
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&AppDt_getResp.APN1,sizeof(App_rmtCfg_APN1_t));
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN2,&AppDt_getResp.APN2,sizeof(App_rmtCfg_APN2_t));
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&AppDt_getResp.FICM,sizeof(App_rmtCfg_FICM_t));
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&AppDt_getResp.COMMON,sizeof(App_rmtCfg_COMMON_t));
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_EXT,&AppDt_getResp.EXTEND,sizeof(App_rmtCfg_EXTEND_t));
					pthread_mutex_unlock(&cfgdtmtx);
					PP_rmtCfg.state.cfgsuccess = 0;
					PP_rmtCfg_settbox(&AppDt_getResp);
				}
			}
			PP_rmtCfg.state.waitSt  = PP_RMTCFG_WAIT_IDLE;
			PP_rmtCfg.state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
			PP_rmtCfg.state.req 	= 0;
			PP_rmtCfg.state.reqCnt 	= 0;
		}
		break;
		default:
		break;
	}

	TxInform_ptr->idleflag = 0;
}

/*
* 获取iccid
*/
uint8_t PP_rmtCfg_getIccid(uint8_t* iccid)
{
	(void)PrvtProtCfg_get_iccid((char *)(iccid));
	if(0 == PP_rmtCfg_is_empty(iccid,21))
	{
		return 1;
	}
	
	return 0;
}

/*
* 下发配置
*/
static void PP_rmtCfg_settbox(App_rmtCfg_getResp_t *rmtCfg)
{
	int ret;	
	//FICMConfigSettings
	if(rmtCfg->FICM.ficmConfigValid == 1)
	{
		
	}
	
	//APN1ConfigSettings APN1 
	if(PP_rmtCfg.state.apn1tspaddrchangedflag)
	{
		PP_rmtCfg.state.apn1tspaddrchangedflag = 0;
		setsockproxy_bdlAddrPort((char*)rmtCfg->APN1.tspAddr, \
				(char*)rmtCfg->APN1.tspPort);
	}

	if(PP_rmtCfg.state.apn1certaddrchangeflag)
	{
		PP_rmtCfg.state.apn1certaddrchangeflag = 0;
		setsockproxy_sgAddrPort((char*)rmtCfg->APN1.certAddress, \
				(char*)rmtCfg->APN1.certPort);
	}

	if(PP_rmtCfg.state.tspSMSchangeflag)
	{
		PP_rmtCfg.state.tspSMSchangeflag = 0;
		ret = cfg_set_para(CFG_ITEM_WHITE_LIST, (unsigned char *)rmtCfg->APN1.tspSms, 512);
		if (ret != 0)
		{
			log_e(LOG_HOZON,"set whitelist failed,ret=%d\r\n", ret);
		}
	}

	//APN2ConfigSettings APN2
	if(rmtCfg->APN2.apn2ConfigValid == 1)
	{
		
	}

	//ExtendConfigSettings
	if(rmtCfg->EXTEND.extendConfigValid == 1)
	{
		unsigned char xcall[32];
		uint32_t len = 32;
		if(PP_rmtCfg_is_empty(rmtCfg->EXTEND.bcallNO,17) == 0)  //设置BCALL
		{
			cfg_get_para(CFG_ITEM_BCALL,xcall,&len);
			if(strncmp(( char *)rmtCfg->EXTEND.bcallNO,( char *)xcall,17) != 0)
			{
				memset(xcall,0,32);
				strncpy((char *)xcall,(char *)rmtCfg->EXTEND.bcallNO,17);
				(void)cfg_set_para(CFG_ITEM_BCALL,(void *)xcall,32);
			}
		}
		if(PP_rmtCfg_is_empty(rmtCfg->EXTEND.ecallNO,17) == 0) //设置ECALL
		{
			cfg_get_para(CFG_ITEM_ECALL,xcall,&len);
			if(strncmp(( char *)rmtCfg->EXTEND.ecallNO,(char *)xcall,17) != 0)
			{
				memset(xcall,0,32);
				strncpy((char *)xcall,(char *)rmtCfg->EXTEND.ecallNO,17);
				cfg_set_para(CFG_ITEM_ECALL,(void *)xcall,32);
			}
		}
		if(PP_rmtCfg_is_empty(rmtCfg->EXTEND.ccNO,17) == 0) //设置ICALL
		{
			cfg_get_para(CFG_ITEM_ICALL,xcall,&len);
			if(strncmp((char *)rmtCfg->EXTEND.ccNO,( char *)xcall,17) != 0)
			{
				memset(xcall,0,32);
				strncpy((char *)xcall,(char *)rmtCfg->EXTEND.ccNO,17);
				(void)cfg_set_para(CFG_ITEM_ICALL,(void *)xcall,32);
			}
		}
	}

	if(rmtCfg->COMMON.commonConfigValid == 1)
	{
		PrvtPro_SetHeartBeatPeriod((uint8_t)rmtCfg->COMMON.heartbeatTimeout);

		uint8_t btKeyEntryEn;
		btKeyEntryEn = get_ble_ctl();
		if(rmtCfg->COMMON.btKeyEntryEnabled != btKeyEntryEn)
		{
			set_ble_ctl((unsigned char)rmtCfg->COMMON.btKeyEntryEnabled);
			sleep(3);
			system("reboot");
		}
	}
	
}

/*
* 检查参数是否全0
*/
static uint8_t PP_rmtCfg_is_empty(uint8_t *dt,int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		if(dt[i] != 0)
		{
			return 0;
		}
	}

	return 1;
}

//国标直连开关
uint8_t PP_rmtCfg_enable_directConnEnable(void)
{
	unsigned int len;
	App_rmtCfg_FICM_t rmt_FICM;
	len = sizeof(App_rmtCfg_FICM_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_FICM,&rmt_FICM,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	return rmt_FICM.directConnEnable;
}

uint8_t PP_rmtCfg_enable_actived(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	return rmt_COMMON.actived;
}

uint8_t PP_rmtCfg_enable_remotecontorl(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);

	return rmt_COMMON.rcEnabled;
}

uint8_t PP_rmtCfg_enable_icall(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.iCallEnabled;
}

uint8_t PP_rmtCfg_enable_bcall(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.bCallEnabled;
}

uint8_t PP_rmtCfg_enable_ecall(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.eCallEnabled;
}

uint8_t PP_rmtCfg_enable_datacollect(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.dcEnabled;
}
uint8_t PP_rmtCfg_enable_dtcEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.dtcEnabled;
}

uint8_t PP_rmtCfg_enable_dcEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.dcEnabled;
}

uint8_t PP_rmtCfg_enable_rChargeEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.rChargeEnabled;
}
uint8_t PP_rmtCfg_enable_svtEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.svtEnabled;
}
uint8_t PP_rmtCfg_enable_vsEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.vsEnabled;
}
uint8_t PP_rmtCfg_enable_btKeyEntryEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.btKeyEntryEnabled;
}
uint8_t PP_rmtCfg_enable_journeysEnabled(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.journeysEnabled;
}

uint8_t PP_rmtCfg_enable_online(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.onlineInfEnabled;
}

uint8_t PP_rmtCfg_enable_carEmpower(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.carEmpowerEnabled;
}

uint8_t PP_rmtCfg_enable_evtReport(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.eventReportEnabled;
}

uint8_t PP_rmtCfg_enable_carAlarm(void)
{
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	return rmt_COMMON.carAlarmEnabled;
}

/*
* 获取心跳超时时间配置
*/
int getPP_rmtCfg_heartbeatTimeout(void)
{
	int hbtimeout = 0;
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	if(1 == rmt_COMMON.commonConfigValid)
	{
		hbtimeout = rmt_COMMON.heartbeatTimeout;
	}
	
	return hbtimeout;
}

/*
* 国标报文采集打包周期
*/
int getPP_rmtCfg_infoCollectCycle(void)
{
	int infoCollectCycle = 0;
	unsigned int len;
	App_rmtCfg_COMMON_t rmt_COMMON;
	len = sizeof(App_rmtCfg_COMMON_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_COMM,&rmt_COMMON,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	
	if(1 == rmt_COMMON.commonConfigValid)
	{
		infoCollectCycle = rmt_COMMON.infoCollectCycle;
	}
	
	return infoCollectCycle;
}

/*
* 获取tsp ip addr和port
*/
void getPP_rmtCfg_tspAddrPort(char* addr,int* port)
{
	unsigned int len;
	App_rmtCfg_APN1_t rmt_APN1;
	len = sizeof(App_rmtCfg_APN1_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&rmt_APN1,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	if(1 == rmt_APN1.apn1ConfigValid)
	{
		memcpy(addr,(char*)rmt_APN1.tspAddr,rmt_APN1.tspAddrlen);
		*port = atoi((const char*)rmt_APN1.tspPort);
	}
}

/*
* 获取cert ip addr和port
*/
void getPP_rmtCfg_certAddrPort(char* addr,int* port)
{
	unsigned int len;
	App_rmtCfg_APN1_t rmt_APN1;
	len = sizeof(App_rmtCfg_APN1_t);
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_APN1,&rmt_APN1,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	if(1 == rmt_APN1.apn1ConfigValid)
	{
		memcpy(addr,(char*)rmt_APN1.certAddress,rmt_APN1.certAddresslen);
		*port = atoi((const char*)rmt_APN1.certPort);
	}
}

/*无符号长整形转字符型*/
int PP_rmtCfg_ultoa(unsigned long value, char *string, int radix)
{
	char tmp[33] = {0};
	char *tp = tmp;
	int i;

	if (radix > 36 || radix <= 1)
	{
		return -1;
	}

	if(value != 0)
	{
		while(value)
		{
			i = value % radix;
			value = value / radix;
			if (i < 10)
			*tp++ = i +'0';
			else
			*tp++ = i + 'a' - 10;
		}

		while(tp > tmp)
		{
			*string++ = *(--tp);
		}
	}
	else
	{
		*string++ = '0';
	}

	*string = 0;

	return 0;
}

/*
* 获取配置版本
*/
void getPP_rmtCfg_cfgVersion(char* ver)
{
	int i,j;
	unsigned int len;
	uint32_t tempVal;
	uint8_t  cfgVersion[33];
	char stringVal[33] = {0};
	char *ver_tp = ver;

	len = 33;
	pthread_mutex_lock(&cfgdtmtx);
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTCFG_VER,cfgVersion,&len);
	pthread_mutex_unlock(&cfgdtmtx);
	for(i = 0;i < sizeof(cfgVersion)/4;i++)
	{
		tempVal = 0;
		memset(stringVal,0,sizeof(stringVal));
		tempVal |= ((uint32_t)cfgVersion[4*i]) << 24;
		tempVal |= ((uint32_t)cfgVersion[4*i+1]) << 16;
		tempVal |= ((uint32_t)cfgVersion[4*i+2]) << 8;
		tempVal |= (uint32_t)cfgVersion[4*i+3];
		PP_rmtCfg_ultoa(tempVal,stringVal,10);

		for(j=0;j<strlen(stringVal);j++)
		{
			*ver_tp++ = stringVal[j];
		}
		
		if(i < 7)
		{
			*ver_tp++ = '.';
		}
	}

	*ver_tp = 0;
}

static void PP_rmtCfg_HexToStr(uint8_t *pbDest, uint8_t *pbSrc, int nLen)
{
	char ddl,ddh;
	int i;
	for (i=0; i<nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;
		pbDest[i*2] = ddh;
		pbDest[i*2+1] = ddl;
	}

	pbDest[nLen*2] = 0;
}