/******************************************************
�ļ�����	PrvtProt.c

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
#include "dir.h"
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
#include "file.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../sockproxy/sockproxy_rxdata.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "cfg_api.h"
#include "udef_cfg_api.h"
#include "pm_api.h"
#include "dev_api.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt_callCenter.h"
#include "PrvtProt_xcall.h"
#include "PrvtProt_Mileage_sync.h"
#include "PrvtProt_remoteConfig.h"
#include "PP_rmtCtrl.h"
#include "PrvtProt_VehiSt.h"
#include "PrvtProt_fotaInfoPush.h"
#include "PrvtProt_SigParse.h"
#include "remoteDiag/PrvtProt_rmtDiag.h"
#include "PrvtProt_CertDownload.h"
#include "PrvtProt_netstatus.h"
#include "PrvtProt_lock.h"
#include "../../base/uds/server/uds_did.h"
#include "FileUpload/PrvtProt_FileUpload.h"
#include "CanMessageUL/CanMsgUL.h"
#include "PrvtProt.h"

/*******************************************************
description�� global variable definitions
*******************************************************/
extern uint8_t tbox_ivi_get_link_fault(uint64_t *timestamp);
/*******************************************************
description�� static variable definitions
*******************************************************/
static PrvtProt_heartbeat_t PP_heartbeat;
static PrvtProt_task_t 	pp_task;
static PrvtProt_TxInform_t HB_TxInform;
static char pp_tboxsn[PP_TBOXSN_LEN];

typedef struct
{
	PrvtProt_App_Xcall_t Xcall;//xcall
	PrvtProt_App_rmtCfg_t rmtCfg;//remote config
}PrvtProt_appData_t;

/* message data struct */
typedef struct
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t 		appData;
}PrvtProt_msgData_t;

static PrvtProt_RmtFunc_t PP_RmtFunc[PP_RMTFUNC_MAX] =
{
	{PP_RMTFUNC_XCALL,PP_xcall_init,	PP_xcall_mainfunction},
	{PP_RMTFUNC_CC,	  PrvtProt_CC_init, PrvtProt_CC_mainfunction},
	{PP_RMTFUNC_CFG,  PP_rmtCfg_init,	PP_rmtCfg_mainfunction},
	{PP_RMTFUNC_CTRL, PP_rmtCtrl_init,	PP_rmtCtrl_mainfunction},
	{PP_RMTFUNC_VS,   PP_VS_init,		PP_VS_mainfunction},
	{PP_RMTFUNC_DIAG, PP_rmtDiag_init,	PP_rmtDiag_mainfunction},
	{PP_RMTFUNC_SYNC,PP_Mileagesync_init,PP_Mileagesync_mainfunction},
	{PP_RMTFUNC_OTAPUSH,PP_FotaInfoPush_init,PP_FotaInfoPush_mainfunction}
};

static uint8_t PP_sleepflag = 0;
static uint16_t PP_hbtaskmpurtcwakeuptestflag = 0;
PrvtProt_TxInform_t 	PP_TxInform[PP_TXINFORM_NUM];

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
#if PP_THREAD
static void *PrvtProt_main(void);
#endif
static int PrvtPro_do_checksock(PrvtProt_task_t *task);
static int PrvtPro_do_rcvMsg(PrvtProt_task_t *task);
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task);
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PrvtPro_do_wait(PrvtProt_task_t *task);
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len);
static void PP_HB_send_cb(void * para);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
int PrvtProt_init(INIT_PHASE phase)
{
    int ret = 0;
    int obj;
    unsigned int cfglen;
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			//pp_task.heartbeat.ackFlag = 0;
			//PP_heartbeat.state = 1;
			PP_heartbeat.period = PP_HEART_BEAT_TIME;
			PP_heartbeat.timer = tm_get_time();
			PP_heartbeat.waitSt = 0;
			PP_heartbeat.waittime = 0;
			pp_task.nonce = 0;
			pp_task.version = 0x30;

			memset(pp_tboxsn,0 , sizeof(pp_tboxsn));
			PP_heartbeat.hbtaskflag = 1;
			PP_heartbeat.IGNoldst = 0xff;
			PP_heartbeat.IGNnewst = 0xff;
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			cfglen = 4;
			ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXID, &pp_task.tboxid, &cfglen);

			cfglen = 19;
			ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXSN,pp_tboxsn,&cfglen);

			PrvtProt_shell_init();
			PP_ntp_Init();
			for(obj = 0;obj < PP_RMTFUNC_MAX;obj++)
			{
				if(PP_RmtFunc[obj].Init != NULL)
				{
					PP_RmtFunc[obj].Init();
				}
			}
			InitPrvtProt_SignParse_Parameter();
			PP_CertDownload_init();
			InitPP_netstatus_Parameter();
		  	InitPP_lock_parameter();
			InitPP_FileUpload_Parameter();
			InitPP_CanMsgUL_Parameter();
		}
        break;
    }

    return ret;
}


/******************************************************
*��������PrvtProt_run

*��  �Σ�void

*����ֵ��void

*��  �������������߳�

*��  ע��
******************************************************/
int PrvtProt_run(void)
{ 	
	int res = 0;
#if PP_THREAD
	int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &ta, (void *)PrvtProt_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

	PP_ntp_run();
	PP_netstatus_run();
	PP_FileUpload_run();
	PP_CanMsgUL_run();
#else
	res = 	PrvtPro_do_rcvMsg(&pp_task) ||
			PrvtPro_do_wait(&pp_task) || 
			PrvtProt_do_heartbeat(&pp_task);
#endif
    return res;
}

#if PP_THREAD
/******************************************************
*��������PrvtProt_main

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static void *PrvtProt_main(void)
{
	int res;
	int obj;

	log_o(LOG_HOZON, "proprietary protocol  of hozon thread running");
    prctl(PR_SET_NAME, "HZ_PRVT_PROT");
    while (1)
    {
		usleep(5000);

#if 0
		log_set_level(LOG_HOZON, LOG_DEBUG);
		log_set_level(LOG_SOCK_PROXY, LOG_DEBUG);
		log_set_level(LOG_GB32960, LOG_DEBUG);
		log_set_level(LOG_UPER_ECDC, LOG_DEBUG);
#endif
		if(!gb32960_gbCanbusActiveSt())
		{
			PP_heartbeat.IGNnewst = dev_get_KL15_signal();
			if(PP_heartbeat.IGNoldst != PP_heartbeat.IGNnewst)
			{
				PP_heartbeat.IGNoldst = PP_heartbeat.IGNnewst;
				if(1 == PP_heartbeat.IGNnewst)//IGN ON
				{
					log_o(LOG_HOZON, "ign on,switch to normal heart rate\n");
					int hbtimeout;
					hbtimeout = getPP_rmtCfg_heartbeatTimeout();
					if(0 != hbtimeout)
					{
						PP_heartbeat.period = hbtimeout;
					}
					PP_heartbeat.hbtype = 1;//切换到正常通信心跳频率
				}
				else
				{
					log_o(LOG_HOZON, "ign off,switch to sleep heart rate\n");
					//PP_heartbeat.period = PP_HEART_BEAT_TIME_SLEEP;
					PP_heartbeat.hbtype = 2;//切换到休眠状态心跳频率
				}

				PP_heartbeat.hbtaskflag = 1;
			}
		}
		else
		{
			if(1 != PP_heartbeat.hbtype)
			{
				log_o(LOG_HOZON, "can msg on,switch to normal heart rate\n");
				int hbtimeout;
				hbtimeout = getPP_rmtCfg_heartbeatTimeout();
				if(0 != hbtimeout)
				{
					PP_heartbeat.period = hbtimeout;
				}
				PP_heartbeat.hbtype = 1;//切换到正常通信心跳频率
				PP_heartbeat.hbtaskflag = 1;
			}
		}

		PP_CertDownload_mainfunction(&pp_task);

		res = 	PrvtPro_do_checksock(&pp_task) ||
				PrvtPro_do_rcvMsg(&pp_task) ||
				PrvtPro_do_wait(&pp_task) || 
				PrvtProt_do_heartbeat(&pp_task);

		for(obj = 0;obj < PP_RMTFUNC_MAX;obj++)
		{
			if(PP_RmtFunc[obj].mainFunc != NULL)
			{
				PP_RmtFunc[obj].mainFunc(&pp_task);
			}
		}

		PP_sleepflag = PP_heartbeat.hbtasksleepflag &&	\
					   GetPP_rmtCtrl_Sleep();
    }
	(void)res;
    return NULL;
}
#endif

/******************************************************
*��������PrvtPro_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PrvtPro_do_checksock(PrvtProt_task_t *task)
{
	if((1 == sockproxy_socketState()) || \
			(sockproxy_sgsocketState()))//socket open
	{
		return 0;
	}
	else
	{
		PP_heartbeat.waitSt = 0;
		//PP_heartbeat.state = 0;
		PP_heartbeat.hbtaskflag = 0;
		PP_heartbeat.hbtasksleepflag = 1;
	}

	return -1;
}

/******************************************************
*��������PrvtPro_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PrvtPro_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	uint8_t rcvbuf[1456U] = {0};
	PrvtProt_pack_t 	RxPack;
	
	memset(&RxPack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = PrvtProtCfg_rcvMsg(rcvbuf,1456)) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "HOZON private protocol receive message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcvbuf, rlen, 0);
	if((rcvbuf[0] != 0x2A) || (rcvbuf[1] != 0x2A) || \
			(rlen < 18))//
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//
	{
		return 0;
	}
	PrvtPro_makeUpPack(&RxPack,rcvbuf,rlen);
	PrvtPro_RxMsgHandle(task,&RxPack,rlen);

	return 0;
}

/******************************************************
*��������PrvtPro_makeUpPack

*��  �Σ�void

*����ֵ��void

*��  �����������ݽ��

*��  ע��
******************************************************/
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len)
{
	int rlen = 0;
	int size = len;
	uint8_t rcvstep = 0;
	RxPack->Header.sign[0] = input[rlen++];
	RxPack->Header.sign[1] = input[rlen++];
	size = size-2;
	while(size--)
	{
		switch(rcvstep)
		{
			case 0:
			{
				RxPack->Header.ver.Byte = input[rlen++];
				rcvstep = 1;
			}
			break;
			case 1:
			{
				RxPack->Header.nonce = (RxPack->Header.nonce << 8) + input[rlen++];
				if(7 == rlen)
				{
					rcvstep = 2;
				}
			}
			break;	
			case 2:
			{
				RxPack->Header.commtype.Byte = input[rlen++];
				rcvstep = 3;
			}
			break;	
			case 3:
			{
				RxPack->Header.safetype.Byte = input[rlen++];
				rcvstep = 4;
			}
			break;
			case 4:
			{
				RxPack->Header.opera = input[rlen++];
				rcvstep = 5;
			}
			break;
			case 5:
			{
				RxPack->Header.msglen = (RxPack->Header.msglen << 8) + input[rlen++];
				if(14 == rlen)
				{
					rcvstep = 6;
				}
			}
			break;
			case 6://tboxid
			{
				RxPack->Header.tboxid = (RxPack->Header.tboxid << 8) + input[rlen++];
				if(18 == rlen)
				{
					rcvstep = 7;
				}
			}
			break;
			case 7://message data
			{
				RxPack->msgdata[rlen-18] = input[rlen];
				rlen += 1;
			}
			break;
			default:
			break;
		}
	}
}

/******************************************************
*��������PrvtPro_rcvMsgCallback

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	switch(rxPack->Header.opera)
	{
		case PP_OPERATETYPE_NS:
		{
			
		}
		break;
		case PP_OPERATETYPE_HEARTBEAT://心跳
		{
			if((19 == len) && ((2 == rxPack->msgdata[0]) || \
								(1 == rxPack->msgdata[0])))//接收到心跳响应
			{
				log_o(LOG_HOZON, "the heartbeat is ok");
				PP_heartbeat.hbtimeoutflag = 0;
				PP_heartbeat.timeoutCnt = 0;
				//PP_heartbeat.state = 1;
				PP_heartbeat.waitSt = 0;
				PP_heartbeat.hbtasksleepflag = 1;
			}
			else
			{}
		}
		break;
		case PP_OPERATETYPE_NGTP://ngtp
		{
			PrvtProt_DisptrBody_t MsgDataBody;
			PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,NULL);
			aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
					  (MsgDataBody.aID[2] - 0x30);
			switch(aid)
			{
				case PP_AID_XCALL://XCALL
				{
					WrPP_queue(PP_XCALL,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_RMTCFG://remote config
				{
					WrPP_queue(PP_REMOTE_CFG,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_RMTCTRL://remote control
				{
					WrPP_queue(PP_REMOTE_CTRL,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_VS://remote vehi status check
				{
					WrPP_queue(PP_REMOTE_VS,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_DIAG:
				{
					WrPP_queue(PP_REMOTE_DIAG,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_OTAINFOPUSH:
				{
					WrPP_queue(PP_OTA_INFOPUSH,rxPack->Header.sign,len);
				}
				break;
				default:
				{
					log_e(LOG_HOZON, "rcv unknow ngtp package\r\n");
				}
				break;
			}
		}
		break;
		case PP_OPERATETYPE_CERTDL:
		{
			WrPP_queue(PP_CERT_DL,rxPack->Header.sign,len);
		}
		break;
		default:
		{
			log_e(LOG_HOZON, "unknow package");
		}
		break;
	}
}

/******************************************************
*��������PrvtPro_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PrvtPro_do_wait(PrvtProt_task_t *task)
{
    if(!PP_heartbeat.waitSt)//
    {
        return 0;
    }

    if((tm_get_time() - PP_heartbeat.waittime) > PP_HB_WAIT_TIMEOUT)
    {
        if (PP_heartbeat.waitSt == 1)
        {
        	PP_heartbeat.waitSt = 0;
        	//PP_heartbeat.state = 0;//
        	PP_heartbeat.timeoutCnt++;
			if(0 != sockproxy_check_link_status())
			{
				sockproxy_socketclose((int)(PP_SP_COLSE_PP));
			}
			PP_heartbeat.hbtasksleepflag = 1;
			PP_heartbeat.hbtimeoutflag = 1;
            log_e(LOG_HOZON, "heartbeat timeout");
        }
        else
        {}
    }

	return -1;
}

/******************************************************
*��������PrvtProt_do_heartbeat

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task)
{
	PrvtProt_pack_t PP_PackHeader_HB;

	if(((tm_get_time() - PP_heartbeat.timer) > (PP_heartbeat.period*1000)) || 
		PP_heartbeat.hbtaskflag)
	{
		PP_heartbeat.hbtaskflag = 0;
		PP_heartbeat.hbtasksleepflag = 0;
		memset(&PP_PackHeader_HB,0 , sizeof(PrvtProt_pack_t));
		memcpy(PP_PackHeader_HB.Header.sign,"**",2);
		PP_PackHeader_HB.Header.ver.Byte = 0x30;
		PP_PackHeader_HB.Header.commtype.Byte = 0x70;
		PP_PackHeader_HB.Header.opera = 0x01;
		PP_PackHeader_HB.Header.ver.Byte = task->version;
		PP_PackHeader_HB.Header.nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader_HB.Header.tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		PP_PackHeader_HB.msgdata[0] = PP_heartbeat.hbtype;
		PP_PackHeader_HB.Header.msglen = PrvtPro_BSEndianReverse((long)19);

		HB_TxInform.pakgtype = PP_TXPAKG_SIGTRIG;
		HB_TxInform.eventtime = tm_get_time();
		HB_TxInform.description = "heartbeat";
		SP_data_write(PP_PackHeader_HB.Header.sign,19,PP_HB_send_cb,&HB_TxInform);

		PP_heartbeat.timer = tm_get_time();

		return -1;
	}

	if(PP_heartbeat.timeoutCnt >= 3)
	{
		PP_heartbeat.timeoutCnt = 0;
		log_i(LOG_HOZON, "heartbeat timeout too much!close socket\n");
		//sockproxy_socketclose((int)(PP_SP_COLSE_PP));
	}
	return 0;
}

/******************************************************
*��������PP_HB_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static void PP_HB_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)para;

	if(PP_TXPAKG_SUCCESS == TxInform_ptr->successflg)
	{
		PP_heartbeat.waitSt = 1;
		PP_heartbeat.waittime = tm_get_time();
	}
	else
	{
		log_e(LOG_HOZON, "send heartbeat frame fail\r\n");
		PP_heartbeat.waitSt = 0;
		PP_heartbeat.hbtasksleepflag = 1;
	}
}

/******************************************************
*��������PrvtPro_SetHeartBeatPeriod

*��  �Σ�

*����ֵ��

*��  ����������������

*��  ע��
******************************************************/
void PrvtPro_SetHeartBeatPeriod(unsigned char period)
{
	if(period != 0)
	{
		PP_heartbeat.period = period;
	}
}

/******************************************************
*������:PrvtPro_ShowPara

*��  �Σ�

*����ֵ��

*��  ����������ʾ

*��  ע��������
******************************************************/
void PrvtPro_ShowPara(void)
{
	log_o(LOG_HOZON, "/******************************/");
	log_o(LOG_HOZON, "     	  public parameters 	  ");
	log_o(LOG_HOZON, "/******************************/");
	log_o(LOG_HOZON, "IGN status = %s\n",dev_get_KL15_signal()?"on":"off");
	PP_rmtDiag_showPara();
	log_o(LOG_HOZON, "tboxid = %d\n",pp_task.tboxid);
	log_o(LOG_HOZON, "tboxsn = %s\n",pp_tboxsn);
	log_o(LOG_HOZON, "PP_heartbeat.period = %d\n",PP_heartbeat.period);
	showPP_lock_mutexlockstatus();
	log_o(LOG_HOZON, "/**********show remote config para**********/");
	PP_rmtCfg_ShowCfgPara();
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/**********show charge control para*********/");
	PP_ChargeCtrl_show();
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/**********show AC control para********/");
	PP_ACCtrl_show();
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/*************show sleep status*************/");
	sockproxy_showParameters();
	log_o(LOG_SOCK_PROXY, "gb32960 sleep = %d\n",gb32960_gbLogoutSt());
	log_o(LOG_SOCK_PROXY, "PrvtProt sleep = %d\n",GetPrvtProt_Sleep());
	log_o(LOG_HOZON, "PP_heartbeat.hbtasksleepflag = %d",PP_heartbeat.hbtasksleepflag);
	log_o(LOG_HOZON, "GetPP_rmtCtrl_Sleep = %d",GetPP_rmtCtrl_Sleep());
	PP_rmtCtrl_showSleepPara();
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/*************show cert download*************/");
	PP_CertDL_showPara();
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/*************show fault status*************/");
	uint64_t timestamp;
	log_o(LOG_SOCK_PROXY, "public net status = %s\n",PP_netstatus_pubilcfaultsts(&timestamp)?"fault":"normal");
	log_o(LOG_SOCK_PROXY, "HU link status = %s\n",tbox_ivi_get_link_fault(NULL)?"fault":"normal");
	log_o(LOG_HOZON, "/*****************data over*****************/");
	log_o(LOG_HOZON, "/****************show version***************/");
	log_o(LOG_HOZON, "hozon software version : %s\n",DID_F1B0_SW_UPGRADE_VER);
	char hw[32] = {0};
	unsigned int len;
    len = sizeof(hw);
    cfg_get_para(CFG_ITEM_INTEST_HW,hw,&len);
    if(hw[0] == 0)
    {
		memcpy(hw,"00.00",5);
    }
	log_o(LOG_HOZON, "hozon hardware version : %s\n",hw);
	log_o(LOG_HOZON, "/*****************data over*****************/");
}

/******************************************************
*��������PrvtPro_getTimestamp

*��  �Σ�

*����ֵ��

*��  ������ȡʱ���

*��  ע��
******************************************************/
long PrvtPro_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return timestamp.tv_sec;
}

/******************************************************
*PP_rmtCtrl_usTimestamp

*形  参：void

*返回值：void

*描  述：获取时间戳 (us)

*备  注：
******************************************************/
long PP_rmtCtrl_usTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return timestamp.tv_usec;
}

/******************************************************
*��������PrvtPro_SettboxId

*��  �Σ�

*����ֵ��

*��  ����������ͣ

*��  ע��
******************************************************/
void PrvtPro_SettboxId(char obj,unsigned int tboxid)
{
	log_o(LOG_HOZON, "set tboxid = %d\n",tboxid);

	if(0 == obj)
	{
		pp_task.tboxid = tboxid;
		if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_TBOXID, &pp_task.tboxid, 4))
		{
			log_e(LOG_HOZON, "save tboxid failed");
		}

		return;
	}


	if(0 != tboxid)
	{
		pp_task.tboxid = tboxid;
		if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_TBOXID, &pp_task.tboxid, 4))
		{
			log_e(LOG_HOZON, "save tboxid failed");
		}
	}
	else
	{
		log_e(LOG_HOZON, "invalid tboxid\n");
	}
}

/******************************************************
*��������PrvtPro_BSEndianReverse

*��  �Σ�void

*����ֵ��void

*��  �������ģʽ��С��ģʽ����ת��

*��  ע��
******************************************************/
long PrvtPro_BSEndianReverse(long value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
		   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

/******************************************************
*������:PrvtProt_Settboxsn

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void PrvtProt_Settboxsn(const char *tboxsn)
{
	memset(pp_tboxsn, 0 , PP_TBOXSN_LEN);
	memcpy(pp_tboxsn,tboxsn,strlen(tboxsn));
	if (cfg_set_user_para(CFG_ITEM_HOZON_TSP_TBOXSN, pp_tboxsn , PP_TBOXSN_LEN))
	{
		log_e(LOG_HOZON, "save tboxsn failed");
	}
}

/******************************************************
*������:PrvtProt_defaultsettings

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void PrvtProt_defaultsettings(void)
{
	clbt_cfg_set_default_para(UDEF_CFG_SET_SILENT);
	cfg_set_para(CFG_ITEM_INTEST_HW, "H1.11", 32);
	PP_rmtCfg_setCfgapn1(1,"tboxgw-uat.chehezhi.cn","21000");
	PP_rmtCfg_setCfgapn1(6,"tboxgw-uat.chehezhi.cn","22000");

	PP_rmtCfg_setCfgEnable(1,1);
	PP_rmtCfg_setCfgEnable(2,1);
	PP_rmtCfg_setCfgEnable(3,1);
	PP_rmtCfg_setCfgEnable(4,1);
	PP_rmtCfg_setCfgEnable(5,1);
	PP_rmtCfg_setCfgEnable(6,1);
	PP_rmtCfg_setCfgEnable(7,1);
	PP_rmtCfg_setCfgEnable(8,1);
	PP_rmtCfg_setCfgEnable(9,1);
	PP_rmtCfg_setCfgEnable(10,1);
	PP_rmtCfg_setCfgEnable(11,1);
	PP_rmtCfg_setCfgEnable(12,1);
	PP_rmtCfg_setCfgEnable(13,1);
	PP_rmtCfg_setCfgEnable(14,1);
	PP_rmtCfg_setCfgEnable(15,1);
	PP_rmtCfg_setCfgEnable(16,1);

	unsigned char pkien = 1;
	cfg_set_para(CFG_ITEM_EN_PKI, &pkien, 1);
	pkien = 0;
	cfg_set_para(CFG_ITEM_EN_HUPKI,&pkien, 1);
}


/******************************************************
*������:PrvtProt_tboxsnValidity

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
uint8_t PrvtProt_tboxsnValidity(void)
{
	if((strlen(pp_tboxsn) == 0) || \
	   (strcmp(pp_tboxsn,"000000000000000000") == 0) || \
	   18 != strlen(pp_tboxsn)
	   )
	{
		return 0;
	}

	return 1;
}

/******************************************************
*������:PrvtProt_gettboxsn

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void PrvtProt_gettboxsn(char *tboxsn)
{
	memcpy(tboxsn,pp_tboxsn,strlen(pp_tboxsn));
}

/******************************************************
*������:SetPrvtProt_Awaken

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void SetPrvtProt_Awaken(int type)
{
	PP_sleepflag = 0;
	SetPP_rmtCtrl_Awaken();
	switch(type)
	{
		case PM_MSG_RTC_WAKEUP://mpu rtc wake up
		{
			PP_heartbeat.hbtaskflag = 1;
			PP_heartbeat.hbtasksleepflag = 0;
			PP_hbtaskmpurtcwakeuptestflag++;
			PP_heartbeat.timer = tm_get_time();
		}
		break;
		default:
		break;
	}
}

/******************************************************
*������:setPrvtProt_sendHeartbeat

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void setPrvtProt_sendHeartbeat(void)
{
	PP_heartbeat.hbtaskflag = 1;
	PP_heartbeat.hbtasksleepflag = 0;
	PP_heartbeat.hbtimeoutflag = 0;
	//PP_heartbeat.resettimer = tm_get_time();
}

/******************************************************
*������:GetPrvtProt_Sleep

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
unsigned char GetPrvtProt_Sleep(void)
{
	return PP_sleepflag;
}

/*
 * 获取空闲节点
 */
int PP_getIdleNode(void)
{
	int i;
	int res = 0;
	for(i = 0;i < PP_TXINFORM_NUM;i++)
	{
		if(PP_TxInform[i].idleflag == 0)
		{
			res = i;
			PP_TxInform[i].idleflag = 1;
			break;
		}
	}
	return res;
}

/*获取心跳超时状态*/
unsigned int PP_hbTimeoutStatus(void)
{
	return PP_heartbeat.hbtimeoutflag;
}