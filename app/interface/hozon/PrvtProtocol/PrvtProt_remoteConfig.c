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

static PrvtProt_pack_t 		PP_rmtCfg_Pack;

static PrvtProt_rmtCfg_t	PP_rmtCfg =
{
	{
			/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
			{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  }, //ecall response
		  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
			{"100", 1,      0,        0,	PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,	1,		0   } //bcall response
	},
	{1,PP_RMTCFG_CFG_IDLE,0,0,0}
};
static PrvtProt_msgData_t rmtCfgMsgData;
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCfg_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCfg_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtCfg_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtCfg_do_wait(PrvtProt_task_t *task);
static int PP_rmtCfg_do_checkConfig(PrvtProt_task_t *task);
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task);
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task);
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task);
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task);

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
	PP_rmtCfg.state.req = 1;
	PP_rmtCfg.state.period = tm_get_time();
	memset(&rmtCfgMsgData,0 , sizeof(PrvtProt_msgData_t));
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
	res = 		PP_rmtCfg_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCfg_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_rmtCfg_do_wait((PrvtProt_task_t*)task) ||
				PP_rmtCfg_do_checkConfig((PrvtProt_task_t*)task);
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
	return -1;
}

/******************************************************
*��������PP_rmtCfg_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_rmtCfg_do_rcvMsg(PrvtProt_task_t *task)
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
			(rlen <= 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
	{
		return 0;
	}
	PP_rmtCfg_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_rmtCfg_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_rmtCfg_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	int res;
	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	//PrvtProt_msgData_t MsgData;
	if(0 != PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&rmtCfgMsgData,1))
	{
		log_e(LOG_HOZON, "decode error\r\n");
		return;
	}

	aid = (rmtCfgMsgData.DisBody.aID[0] - 0x30)*100 +  (rmtCfgMsgData.DisBody.aID[1] - 0x30)*10 + \
			  (rmtCfgMsgData.DisBody.aID[2] - 0x30);
	if(PP_AID_RMTCFG != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}
	switch(rmtCfgMsgData.DisBody.mID)
	{
		case PP_MID_CHECK_CFG_RESP://remote config check response
		{
			if(PP_rmtCfg.state.waitSt == PP_RMTCFG_CHECK_WAIT_RESP)//���յ��ظ�
			{
				PP_rmtCfg.state.waitSt = 0;
				log_i(LOG_HOZON, "\r\ncheck config req ok\r\n");
			}
		}
		break;
		case PP_MID_GET_CFG_RESP://remote config check response
		{
			if(PP_rmtCfg.state.waitSt == PP_RMTCFG_GET_WAIT_RESP)//���յ��ظ�
			{
				PP_rmtCfg.state.waitSt = 0;
				log_i(LOG_HOZON, "\r\nget config req ok\r\n");
			}
		}
		break;
		case PP_MID_CONN_CFG_REQ://TAP �� TCU �������ø������� TCU ѡ����ʵ�ʱ��ȥ��̨��������
		{
			res = PP_rmtCfg_ConnResp(task);
			if(res > 0)
			{
				PP_rmtCfg.state.req  = PP_appData.rmtCfg.connCfgReq.configAccepted;
				PP_rmtCfg.state.period = tm_get_time();
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
    /*if (!PP_rmtCfg.state.waitSt)//û���¼��ȴ�Ӧ��
    {
        return 0;
    }

	 if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)
	 {
		 PP_rmtCfg.state.waitSt = 0;
		 if(PP_RMTCFG_CHECK_REQ == PP_rmtCfg.state.waitSt)
		 {
			 log_e(LOG_HOZON, "check cfg req time out");
		 }
		 return 0;
	 }
	 //*/

	return 0;
}

/******************************************************
*��������PP_rmtCfg_do_checkConfig

*��  �Σ�

*����ֵ��

*��  �����������

*��  ע��
******************************************************/
static int PP_rmtCfg_do_checkConfig(PrvtProt_task_t *task)
{
	int res;

	if(1 != sockproxy_socketState())//socket open
	{
		return 0;
	}

	switch(PP_rmtCfg.state.CfgSt)
	{
		case PP_RMTCFG_CFG_IDLE:
		{
			if(1 == PP_rmtCfg.state.req)
			{
				if((tm_get_time() - PP_rmtCfg.state.period) >= RMTCFG_DELAY_TIME)
				{
					log_i(LOG_HOZON, "start request remote config\r\n");
					//PP_rmtCfg.state.period = 0;
					PP_rmtCfg.state.CfgSt = PP_CHECK_CFG_REQ;
					PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
					PP_rmtCfg.state.req = 0;
					memset(&rmtCfgMsgData,0 , sizeof(PrvtProt_msgData_t));
				}
			}
		}
		break;
		case PP_CHECK_CFG_REQ:
		{
			res = PP_rmtCfg_checkRequest(task);
			if(res < 0)//������ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				PP_rmtCfg.state.req = 1;
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				PP_rmtCfg.state.waitSt 		= PP_RMTCFG_CHECK_WAIT_RESP;//�ȴ����������������Ӧ
				PP_rmtCfg.state.CfgSt 		= PP_CHECK_CFG_RESP;
				PP_rmtCfg.state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_CHECK_CFG_RESP:
		{
			if(PP_rmtCfg.state.waitSt != PP_RMTCFG_WAIT_IDLE)//δ��Ӧ
			{
				 if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//�ȴ���Ӧ��ʱ
				 {
					 log_e(LOG_HOZON, "check cfg response time out");
					 PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 PP_rmtCfg.state.req = 1;
					 PP_rmtCfg.state.period = tm_get_time();
				 }
			}
			else
			{
				if(rmtCfgMsgData.appData.rmtCfg.checkCfgResp.needUpdate == 1)
				{
					PP_rmtCfg.state.CfgSt = PP_GET_CFG_REQ;
				}
				else
				{
					PP_rmtCfg.state.req = 0;
					PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
					PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;

				}
			}
		}
		break;
		case PP_GET_CFG_REQ:
		{
			res = PP_rmtCfg_getRequest(task);
			if(res < 0)//����ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				PP_rmtCfg.state.req = 1;
				PP_rmtCfg.state.period = tm_get_time();
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				PP_rmtCfg.state.waitSt 		= PP_RMTCFG_GET_WAIT_RESP;//�ȴ�������Ӧ
				PP_rmtCfg.state.CfgSt 		= PP_GET_CFG_RESP;
				PP_rmtCfg.state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_GET_CFG_RESP:
		{
			if(PP_rmtCfg.state.waitSt != PP_RMTCFG_WAIT_IDLE)//δ��Ӧ
			{
				 if((tm_get_time() - PP_rmtCfg.state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//�ȴ���Ӧ��ʱ
				 {
					 log_e(LOG_HOZON, "get cfg response time out");
					 PP_rmtCfg.state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 PP_rmtCfg.state.req = 1;
					 PP_rmtCfg.state.period = tm_get_time();
				 }
			}
			else
			{
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_END;
			}
		}
		break;
		case PP_RMTCFG_CFG_END://��������
		{
			res = PP_rmtCfg_CfgEndRequest(task);
			if(res < 0)//����ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				PP_rmtCfg.state.req = 1;
				PP_rmtCfg.state.period = tm_get_time();
				PP_rmtCfg.state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				log_i(LOG_HOZON, "remote config take effect\r\n");
				//������Ч��ʹ���µ�����

				PP_rmtCfg.state.waitSt  = PP_RMTCFG_WAIT_IDLE;
				PP_rmtCfg.state.CfgSt 	= PP_RMTCFG_CFG_IDLE;
				PP_rmtCfg.state.req 	= 0;
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
*��������PP_rmtCfg_checkRequest

*��  �Σ�

*����ֵ��

*��  ����check remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task)
{
	long msgdatalen;
	int res;
	/*header*/
	PP_rmtCfg.pack.Header.ver.Byte = task->version;
	PP_rmtCfg.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtCfg.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &PP_rmtCfg.pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	PP_rmtCfg.pack.DisBody.mID = PP_MID_CHECK_CFG_REQ;
	PP_rmtCfg.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.expTime   = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CHECK_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &PP_rmtCfg.pack.DisBody,&PP_appData))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "check_remote_config_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*��������PP_rmtCfg_getRequest

*��  �Σ�

*����ֵ��

*��  ����get remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task)
{
	long msgdatalen;
	int res;
	/*header*/
	PP_rmtCfg.pack.Header.ver.Byte = task->version;
	PP_rmtCfg.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtCfg.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &PP_rmtCfg.pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	PP_rmtCfg.pack.DisBody.mID = PP_MID_GET_CFG_REQ;
	PP_rmtCfg.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.expTime   = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PP_appData.rmtCfg.getCfgReq.cfgVersion_ptr = rmtCfgMsgData.appData.rmtCfg.checkCfgResp.cfgVersion;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_GET_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &PP_rmtCfg.pack.DisBody,&PP_appData))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_config_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*��������PP_rmtCfg_CfgEndRequest

*��  �Σ�

*����ֵ��

*��  ����remote config end request

*��  ע��
******************************************************/
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task)
{
	long msgdatalen;
	int res;
	/*header*/
	PP_rmtCfg.pack.Header.ver.Byte = task->version;
	PP_rmtCfg.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtCfg.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &PP_rmtCfg.pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	PP_rmtCfg.pack.DisBody.mID = PP_MID_CFG_END;
	PP_rmtCfg.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.expTime   = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PP_appData.rmtCfg.EndCfgReq.configSuccess = 1;
	PP_appData.rmtCfg.EndCfgReq.mcuSw_ptr = PP_appData.rmtCfg.EndCfgReq.mcuSw;
	PP_appData.rmtCfg.EndCfgReq.mpuSw_ptr = PP_appData.rmtCfg.EndCfgReq.mpuSw;
	PP_appData.rmtCfg.EndCfgReq.cfgVersion_ptr = PP_appData.rmtCfg.EndCfgReq.cfgVersion;
	PP_appData.rmtCfg.EndCfgReq.configSw_ptr = PP_appData.rmtCfg.EndCfgReq.configSw;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_END_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &PP_rmtCfg.pack.DisBody,&PP_appData))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);//����

	protocol_dump(LOG_HOZON, "remote_config_end_request", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}

/******************************************************
*��������PP_rmtCfg_ConnResp

*��  �Σ�

*����ֵ��

*��  ���� response of remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task)
{
	long msgdatalen;
	int res;
	/*header*/
	PP_rmtCfg.pack.Header.ver.Byte = task->version;
	PP_rmtCfg.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtCfg.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &PP_rmtCfg.pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	PP_rmtCfg.pack.DisBody.mID = PP_MID_CONN_CFG_RESP;
	PP_rmtCfg.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.expTime   = PrvtPro_getTimestamp();
	PP_rmtCfg.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PP_appData.rmtCfg.connCfgReq.configAccepted = 1;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CONN_RESP,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &PP_rmtCfg.pack.DisBody,&PP_appData))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
	res = sockproxy_MsgSend(PP_rmtCfg_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "remote_config_conn_resp", PP_rmtCfg_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}
/******************************************************
*������:PP_rmtCfg_SetCfgReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PP_rmtCfg_SetCfgReq(unsigned char req)
{
	PP_rmtCfg.state.req  = req;
	PP_rmtCfg.state.period = tm_get_time();
}



