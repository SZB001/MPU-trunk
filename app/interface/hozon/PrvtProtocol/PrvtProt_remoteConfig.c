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

static PrvtProt_pack_t 			PP_rmtCfg_Pack;
static PrvtProt_rmtCfg_t		PP_rmtCfg;
static PrvtProt_App_rmtCfg_t 	AppData_rmtCfg;

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
	int res;
	unsigned int len;
	memset(&PP_rmtCfg,0 , sizeof(PrvtProt_rmtCfg_t));
	memset(&AppData_rmtCfg,0 , sizeof(PrvtProt_App_rmtCfg_t));

	len = 11;
	res = cfg_get_para(CFG_ITEM_HOZON_TSP_MCUSW,AppData_rmtCfg.checkReq.mcuSw,&len);//
	if(AppData_rmtCfg.checkReq.mcuSw[0])
	{
		AppData_rmtCfg.checkReq.mcuSwlen = strlen((char*)AppData_rmtCfg.checkReq.mcuSw);
	}
	else
	{
		AppData_rmtCfg.checkReq.mcuSwlen = 10;
	}

	len = 11;
	res = cfg_get_para(CFG_ITEM_HOZON_TSP_MPUSW,AppData_rmtCfg.checkReq.mpuSw,&len);//
	if(AppData_rmtCfg.checkReq.mpuSw[0])
	{
		AppData_rmtCfg.checkReq.mpuSwlen = strlen((char*)AppData_rmtCfg.checkReq.mpuSw);
	}
	else
	{
		AppData_rmtCfg.checkReq.mpuSwlen = 10;
	}

	len = 18;
	res = cfg_get_para(CFG_ITEM_GB32960_VIN,AppData_rmtCfg.checkReq.vehicleVin,&len);//��ȡvin
	AppData_rmtCfg.checkReq.vehicleVinlen = 17;

	res = PrvtProtCfg_get_iccid((char *)(AppData_rmtCfg.checkReq.iccID));//��ȡiccid
	AppData_rmtCfg.checkReq.iccIDlen = 20;

	memcpy(AppData_rmtCfg.checkReq.btMacAddr,"000000000000",strlen("000000000000"));
	AppData_rmtCfg.checkReq.btMacAddrlen = strlen("000000000000");
	memcpy(AppData_rmtCfg.checkReq.configSw,"00000",strlen("00000"));
	AppData_rmtCfg.checkReq.configSwlen = strlen("00000");
	memcpy(AppData_rmtCfg.checkReq.cfgVersion,"00000000000000000000000000000000",strlen("00000000000000000000000000000000"));
	AppData_rmtCfg.checkReq.cfgVersionlen = strlen("00000000000000000000000000000000");

	//��ȡ����
	len = 512;
	res = cfg_get_para(CFG_ITEM_HOZON_TSP_RMTCFG,&AppData_rmtCfg.ReadResp,&len);
	if((res==0) && (AppData_rmtCfg.ReadResp.cfgsuccess == 1))
	{
		memcpy(AppData_rmtCfg.checkReq.cfgVersion,AppData_rmtCfg.ReadResp.cfgVersion,32);
		AppData_rmtCfg.checkReq.cfgVersionlen = 32;
	}
	else
	{
		memcpy(AppData_rmtCfg.ReadResp.cfgVersion,AppData_rmtCfg.checkReq.cfgVersion,32);
		AppData_rmtCfg.ReadResp.cfgVersionlen = 32;
		AppData_rmtCfg.ReadResp.APN1.apn1ConfigValid = 0;
		AppData_rmtCfg.ReadResp.APN2.apn2ConfigValid = 0;
		AppData_rmtCfg.ReadResp.COMMON.commonConfigValid = 0;
		AppData_rmtCfg.ReadResp.EXTEND.extendConfigValid= 0;
		AppData_rmtCfg.ReadResp.FICM.ficmConfigValid = 0;
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
	//�ͷ���Դ
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
			(rlen <= 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
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
			if(PP_rmtCfg.state.waitSt == PP_RMTCFG_CHECK_WAIT_RESP)//���յ��ظ�
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
			if(rmtCfg->state.waitSt == PP_RMTCFG_GET_WAIT_RESP)//���յ��ظ�
			{
				rmtCfg->state.waitSt = 0;

				log_i(LOG_HOZON, "\r\nget config req ok\r\n");
			}
		}
		break;
		case PP_MID_CONN_CFG_REQ://TAP �� TCU �������ø������� TCU ѡ����ʵ�ʱ��ȥ��̨��������
		{
			rmtCfg->state.cfgAccept = 1;//�������ø�������
			res = PP_rmtCfg_ConnResp(task,rmtCfg,&MsgDataBody);
			if(res > 0)
			{
				rmtCfg->state.req  = 1;
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
		case PP_MID_READ_CFG_REQ://TAP �� TCU ����������
		{
			memset(AppData_rmtCfg.ReadResp.readreq,0,PP_RMTCFG_SETID_MAX);
			for(i = 0; i < AppData_rmtCfg.ReadReq.SettingIdlen;i++)
			{
				AppData_rmtCfg.ReadResp.readreq[AppData_rmtCfg.ReadReq.SettingId[i] -1] = 1;
			}
			res = PP_rmtCfg_ReadCfgResp(task,rmtCfg,&MsgDataBody);
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
*��������PP_rmtCfg_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_rmtCfg_do_wait(PrvtProt_task_t *task)
{

	return 0;
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
	int res;

	if(1 != sockproxy_socketState())//socket open
	{
		return 0;
	}

	if(!AppData_rmtCfg.checkReq.iccID[0])
	{//iccid��Ч
		(void)PrvtProtCfg_get_iccid((char *)(AppData_rmtCfg.checkReq.iccID));//��ȡiccid
	}

	switch(rmtCfg->state.CfgSt)
	{
		case PP_RMTCFG_CFG_IDLE:
		{
			if((1 == rmtCfg->state.req) && (rmtCfg->state.reqCnt < PP_RETRANSMIT_TIMES))
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
			if(res < 0)//������ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 0;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				rmtCfg->state.waitSt 		= PP_RMTCFG_CHECK_WAIT_RESP;//�ȴ����������������Ӧ
				rmtCfg->state.CfgSt 		= PP_CHECK_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_CHECK_CFG_RESP:
		{
			if(rmtCfg->state.waitSt != PP_RMTCFG_WAIT_IDLE)//δ��Ӧ
			{
				 if((tm_get_time() - rmtCfg->state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//�ȴ���Ӧ��ʱ
				 {
					 log_e(LOG_HOZON, "check cfg response time out");
					 rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 rmtCfg->state.period = tm_get_time() - RMTCFG_DELAY_TIME;
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
			if(res < 0)//����ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 0;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)
			{
				rmtCfg->state.waitSt 		= PP_RMTCFG_GET_WAIT_RESP;//�ȴ�������Ӧ
				rmtCfg->state.CfgSt 		= PP_GET_CFG_RESP;
				rmtCfg->state.waittime 	= tm_get_time();
			}
			else
			{}
		}
		break;
		case PP_GET_CFG_RESP:
		{
			if(rmtCfg->state.waitSt != PP_RMTCFG_WAIT_IDLE)//δ��Ӧ
			{
				 if((tm_get_time() - rmtCfg->state.waittime) > PP_RMTCFG_WAIT_TIMEOUT)//�ȴ���Ӧ��ʱ
				 {
					 log_e(LOG_HOZON, "get cfg response time out");
					 rmtCfg->state.waitSt = PP_RMTCFG_WAIT_IDLE;
					 rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
					 rmtCfg->state.period = tm_get_time() - RMTCFG_DELAY_TIME;
				 }
			}
			else
			{
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_END;
			}
		}
		break;
		case PP_RMTCFG_CFG_END://��������
		{
			if(AppData_rmtCfg.getResp.result == 1)
			{//�������ã���֪ͨ�������������״̬
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
			if(res < 0)//����ʧ��
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				sockproxy_socketclose();//by liujian 20190514
				rmtCfg->state.req = 0;
				rmtCfg->state.reqCnt = 0;
				rmtCfg->state.period = tm_get_time();
				rmtCfg->state.CfgSt = PP_RMTCFG_CFG_IDLE;
			}
			else if(res > 0)//����ok
			{
				log_i(LOG_HOZON, "remote config take effect\r\n");
				if(rmtCfg->state.cfgsuccess == 1)
				{
					rmtCfg->state.cfgsuccess = 0;
					AppData_rmtCfg.ReadResp.cfgsuccess = 1;
					//�������ã�ʹ���µ�����
					(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTCFG,&AppData_rmtCfg.ReadResp,512);

					memcpy(AppData_rmtCfg.checkReq.cfgVersion,AppData_rmtCfg.checkResp.cfgVersion,AppData_rmtCfg.checkResp.cfgVersionlen);
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
*��������PP_rmtCfg_checkRequest

*��  �Σ�

*����ֵ��

*��  ����check remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_checkRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	memcpy(rmtCfg->pack.Header.sign,"**",2);
	rmtCfg->pack.Header.commtype.Byte = 0xe1;
	rmtCfg->pack.Header.opera = 0x02;
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCfg->pack.DisBody.aID,"100",3);
	rmtCfg->pack.DisBody.mID = PP_MID_CHECK_CFG_REQ;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_CHECK_CFG_REQ;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	rmtCfg->pack.DisBody.appDataProVer = 256;
	rmtCfg->pack.DisBody.testFlag = 1;
	/*appdata*/

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CHECK_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//���ݱ������Ƿ����
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
*��������PP_rmtCfg_getRequest

*��  �Σ�

*����ֵ��

*��  ����get remote config request

*��  ע��
******************************************************/
static int PP_rmtCfg_getRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	memcpy(rmtCfg->pack.Header.sign,"**",2);
	rmtCfg->pack.Header.commtype.Byte = 0xe1;
	rmtCfg->pack.Header.opera = 0x02;
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCfg->pack.DisBody.aID,"100",3);
	rmtCfg->pack.DisBody.mID = PP_MID_GET_CFG_REQ;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_GET_CFG_REQ;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	rmtCfg->pack.DisBody.appDataProVer = 256;
	rmtCfg->pack.DisBody.testFlag = 1;
	/*appdata*/
	memcpy(AppData_rmtCfg.getReq.cfgVersion,AppData_rmtCfg.checkResp.cfgVersion,AppData_rmtCfg.checkResp.cfgVersionlen);
	AppData_rmtCfg.getReq.cfgVersionlen = AppData_rmtCfg.checkResp.cfgVersionlen;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_GET_REQ,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//���ݱ������Ƿ����
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
*��������PP_rmtCfg_CfgEndRequest

*��  �Σ�

*����ֵ��

*��  ����remote config end request

*��  ע��
******************************************************/
static int PP_rmtCfg_CfgEndRequest(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg)
{
	int msgdatalen;
	int res;
	/*header*/
	memcpy(rmtCfg->pack.Header.sign,"**",2);
	rmtCfg->pack.Header.commtype.Byte = 0xe1;
	rmtCfg->pack.Header.opera = 0x02;
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCfg->pack.DisBody.aID,"100",3);
	rmtCfg->pack.DisBody.mID = PP_MID_CFG_END;
	rmtCfg->pack.DisBody.eventId = PP_AID_RMTCFG + PP_MID_CFG_END;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	rmtCfg->pack.DisBody.appDataProVer = 256;
	rmtCfg->pack.DisBody.testFlag = 1;
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
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtCfg_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
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
static int PP_rmtCfg_ConnResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_DisptrBody_t *MsgDataBody)
{
	int msgdatalen;
	int res;
	/*header*/
	memcpy(rmtCfg->pack.Header.sign,"**",2);
	rmtCfg->pack.Header.commtype.Byte = 0xe1;
	rmtCfg->pack.Header.opera = 0x02;
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCfg->pack.DisBody.aID,"100",3);
	rmtCfg->pack.DisBody.eventId = MsgDataBody->eventId;
	rmtCfg->pack.DisBody.mID = PP_MID_CONN_CFG_RESP;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	rmtCfg->pack.DisBody.appDataProVer = 256;
	rmtCfg->pack.DisBody.testFlag = 1;

	/*appdata*/
	AppData_rmtCfg.connResp.configAccepted = 0;
	if(1 == rmtCfg->state.cfgAccept)
	{
		AppData_rmtCfg.connResp.configAccepted = 1;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_CONN_RESP,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//���ݱ������Ƿ����
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
*��������PP_rmtCfg_ReadCfgResp

*��  �Σ�

*����ֵ��

*��  ���� response of remote read config request

*��  ע��
******************************************************/
static int PP_rmtCfg_ReadCfgResp(PrvtProt_task_t *task,PrvtProt_rmtCfg_t *rmtCfg,PrvtProt_DisptrBody_t *MsgDataBody)
{
	int msgdatalen;
	int res;
	/*header*/
	memcpy(rmtCfg->pack.Header.sign,"**",2);
	rmtCfg->pack.Header.commtype.Byte = 0xe1;
	rmtCfg->pack.Header.opera = 0x02;
	rmtCfg->pack.Header.ver.Byte = task->version;
	rmtCfg->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtCfg->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtCfg_Pack, &rmtCfg->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCfg->pack.DisBody.aID,"100",3);
	rmtCfg->pack.DisBody.mID = PP_MID_READ_CFG_RESP;
	rmtCfg->pack.DisBody.eventId = MsgDataBody->eventId;
	rmtCfg->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCfg->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	rmtCfg->pack.DisBody.appDataProVer = 256;
	rmtCfg->pack.DisBody.testFlag = 1;
	/*appdata*/
	AppData_rmtCfg.ReadResp.result = 1;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCFG_READ_RESP,PP_rmtCfg_Pack.msgdata,&msgdatalen,\
									   &rmtCfg->pack.DisBody,&AppData_rmtCfg))//���ݱ������Ƿ����
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
*������:PP_rmtCfg_SetCfgReq

*��  �Σ�

*����ֵ��

*��  �������� ����

*��  ע��
******************************************************/
void PP_rmtCfg_SetCfgReq(unsigned char req)
{
	PP_rmtCfg.state.req  = req;
	PP_rmtCfg.state.period = tm_get_time();
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
	memset(AppData_rmtCfg.checkReq.mcuSw,0 , 11);
	memcpy(AppData_rmtCfg.checkReq.mcuSw,mcuSw,strlen(mcuSw));
	AppData_rmtCfg.checkReq.mcuSwlen = strlen(mcuSw);
	if (cfg_set_para(CFG_ITEM_HOZON_TSP_MCUSW, AppData_rmtCfg.checkReq.mcuSw, sizeof(AppData_rmtCfg.checkReq.mcuSw)))
	{
		log_e(LOG_HOZON, "save mcuSw failed");
	}
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
	memset(AppData_rmtCfg.checkReq.mpuSw,0 , 11);
	memcpy(AppData_rmtCfg.checkReq.mpuSw,mpuSw,strlen(mpuSw));
	AppData_rmtCfg.checkReq.mpuSwlen = strlen(mpuSw);
	if (cfg_set_para(CFG_ITEM_HOZON_TSP_MPUSW, AppData_rmtCfg.checkReq.mpuSw, sizeof(AppData_rmtCfg.checkReq.mpuSw)))
	{
		log_e(LOG_HOZON, "save mpuSw failed");
	}
}

/******************************************************
*������:PP_rmtCfg_Seticcid

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��������
******************************************************/
void PP_rmtCfg_Seticcid(const char *iccid)
{
	memcpy(AppData_rmtCfg.checkReq.iccID,iccid,strlen(iccid));
	AppData_rmtCfg.checkReq.iccIDlen = strlen(iccid);
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
	log_i(LOG_HOZON, "/******************************/");
	log_i(LOG_HOZON, "       remote  cfg parameter    ");
	log_i(LOG_HOZON, "/******************************/");
	log_i(LOG_HOZON, "vehicleVin = %s",AppData_rmtCfg.checkReq.vehicleVin);
	log_i(LOG_HOZON, "mcuSw = %s",AppData_rmtCfg.checkReq.mcuSw);
	log_i(LOG_HOZON, "mpuSw = %s",AppData_rmtCfg.checkReq.mpuSw);
	log_i(LOG_HOZON, "ICCID = %s",AppData_rmtCfg.checkReq.iccID);
	log_i(LOG_HOZON, "cfgVersion = %s",AppData_rmtCfg.checkReq.cfgVersion);
	log_i(LOG_HOZON, "configSw = %s",AppData_rmtCfg.checkReq.configSw);
	log_i(LOG_HOZON, "btMacAddr = %s",AppData_rmtCfg.checkReq.btMacAddr);

	log_i(LOG_HOZON, "\n/* FICM info */");
	log_i(LOG_HOZON, "FICM.token = %s",AppData_rmtCfg.ReadResp.FICM.token);
	log_i(LOG_HOZON, "FICM.userID = %s",AppData_rmtCfg.ReadResp.FICM.userID);
	log_i(LOG_HOZON, "FICM.directConnEnable = %d",AppData_rmtCfg.ReadResp.FICM.directConnEnable);
	log_i(LOG_HOZON, "FICM.address = %s",AppData_rmtCfg.ReadResp.FICM.address);
	log_i(LOG_HOZON, "FICM.port = %s",AppData_rmtCfg.ReadResp.FICM.port);

	log_i(LOG_HOZON, "\n/* APN1 info */");
	log_i(LOG_HOZON, "APN1.tspAddr = %s",AppData_rmtCfg.ReadResp.APN1.tspAddr);
	log_i(LOG_HOZON, "APN1.tspUser = %s",AppData_rmtCfg.ReadResp.APN1.tspUser);
	log_i(LOG_HOZON, "APN1.tspPass = %s",AppData_rmtCfg.ReadResp.APN1.tspPass);
	log_i(LOG_HOZON, "APN1.tspIP = %s",AppData_rmtCfg.ReadResp.APN1.tspIP);
	log_i(LOG_HOZON, "APN1.tspSms = %s",AppData_rmtCfg.ReadResp.APN1.tspSms);
	log_i(LOG_HOZON, "APN1.tspPort = %s",AppData_rmtCfg.ReadResp.APN1.tspPort);

	log_i(LOG_HOZON, "\n/* APN2 info */");
	log_i(LOG_HOZON, "APN2.apn2Address = %s",AppData_rmtCfg.ReadResp.APN2.apn2Address);
	log_i(LOG_HOZON, "APN2.apn2User = %s",AppData_rmtCfg.ReadResp.APN2.apn2User);
	log_i(LOG_HOZON, "APN2.apn2Pass = %s",AppData_rmtCfg.ReadResp.APN2.apn2Pass);

	log_i(LOG_HOZON, "\n/* COMMON info */");
	log_i(LOG_HOZON, "COMMON.actived = %d",AppData_rmtCfg.ReadResp.COMMON.actived);
	log_i(LOG_HOZON, "COMMON.rcEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.rcEnabled);
	log_i(LOG_HOZON, "COMMON.svtEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.svtEnabled);
	log_i(LOG_HOZON, "COMMON.vsEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.vsEnabled);
	log_i(LOG_HOZON, "COMMON.iCallEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.iCallEnabled);
	log_i(LOG_HOZON, "COMMON.bCallEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.bCallEnabled);
	log_i(LOG_HOZON, "COMMON.eCallEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.eCallEnabled);
	log_i(LOG_HOZON, "COMMON.dcEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.dcEnabled);
	log_i(LOG_HOZON, "COMMON.dtcEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.dtcEnabled);
	log_i(LOG_HOZON, "COMMON.journeysEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.journeysEnabled);
	log_i(LOG_HOZON, "COMMON.onlineInfEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.onlineInfEnabled);
	log_i(LOG_HOZON, "COMMON.rChargeEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.rChargeEnabled);
	log_i(LOG_HOZON, "COMMON.btKeyEntryEnabled = %d",AppData_rmtCfg.ReadResp.COMMON.btKeyEntryEnabled);

	log_i(LOG_HOZON, "\n/* EXTEND info */");
	log_i(LOG_HOZON, "EXTEND.ecallNO = %s",AppData_rmtCfg.ReadResp.EXTEND.ecallNO);
	log_i(LOG_HOZON, "EXTEND.bcallNO = %s",AppData_rmtCfg.ReadResp.EXTEND.bcallNO);
	log_i(LOG_HOZON, "EXTEND.ccNO = %s",AppData_rmtCfg.ReadResp.EXTEND.ccNO);

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
