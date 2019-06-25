/******************************************************
�ļ�����	PrvtProt_rmtCtrl.c

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
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "../sockproxy/sockproxy_txdata.h"
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
#include "PP_rmtCtrl.h"

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
}__attribute__((packed))  PrvtProt_rmtCtrl_pack_t; /**/

typedef struct
{
	PrvtProt_rmtCtrl_pack_t pack;
	PrvtProt_rmtCtrlSt_t	state[RMTCTRL_OBJ_MAX];
}__attribute__((packed))  PrvtProt_rmtCtrl_t; /*�ṹ��*/

static PrvtProt_rmtCtrl_t PP_rmtCtrl;
static PrvtProt_pack_t 		PP_rmtCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 		App_rmtCtrl;

static PrvtProt_RmtCtrlFunc_t PP_RmtCtrlFunc[RMTCTRL_OBJ_MAX] =
{
	{RMTCTRL_DOORLOCK,PP_doorLockCtrl_init,	PP_doorLockCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_AC,	  PP_ACCtrl_init, 		PP_ACCtrl_mainfunction},
	{RMTCTRL_CHARGE,  PP_ChargeCtrl_init,	PP_ChargeCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL}
};

static int PP_rmtCtrl_flag = 0;


/*******************************************************
description�� function declaration
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
description�� function code
******************************************************/
/******************************************************
*��������PP_rmtCtrl_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_rmtCtrl_init(void)
{
	int i;
	memset(&PP_rmtCtrl,0,sizeof(PrvtProt_rmtCtrl_t));

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
*��������PP_rmtCtrl_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_rmtCtrl_mainfunction(void *task)
{
	int res;
	int i;
	res = 		PP_rmtCtrl_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_rcvMsg((PrvtProt_task_t*)task);

	switch(PP_rmtCtrl_flag)
	{
		case RMTCTRL_IDLE://����
		{
			int ret ;
			ret  = PP_doorLockCtrl_start();
			if(ret == 1)
			{
				PP_rmtCtrl_flag = RMTCTRL_IDENTIFICAT_QUERY;
			}
		}
		break;
		case RMTCTRL_IDENTIFICAT_QUERY://�����֤
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
		case RMTCTRL_IDENTIFICAT_LAUNCH://��֤
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
    			//֪ͨtsp��֤ʧ��
   				//PP_rmtCtrl_StInformTsp
    			PP_rmtCtrl_flag = RMTCTRL_IDLE;
				log_o(LOG_HOZON,"-------identificat failed---------");
   			}
   			else
   			{}
		}
		break;
		case RMTCTRL_COMMAND_LAUNCH://Զ�̿���
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

			ret = PP_doorLockCtrl_end();
			if(ret == 1)
			{
				PP_rmtCtrl_flag = RMTCTRL_IDLE; //Զ������ִ���꣬�ص�����
			}
		}
		break;
		default:
		break;
	}

	PP_canSend_Mainfunction();//���Ϳ���ָ��

	return res;
}

/******************************************************
*��������PP_rmtCtrl_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}

	//�ͷ���Դ

	return -1;
}

/******************************************************
*��������PP_rmtCtrl_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
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
			(rlen <= 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
	{
		return 0;
	}
	PP_rmtCtrl_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_rmtCtrl_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
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

	if(PP_MID_RMTCTRL_REQ == MsgDataBody.mID)//�յ�����
	{
		switch((uint8_t)(Appdata.CtrlReq.rvcReqType >> 8))
		{
			case PP_RMTCTRL_DOORLOCK://���Ƴ�����
			{
				SetPP_doorLockCtrl_Request(RMTCTRL_TSP,&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_PNRSUNROOF://����ȫ���촰
			{
				log_i(LOG_HOZON, "remote PANORSUNROOF control req");
			}
			break;
			case PP_RMTCTRL_AUTODOOR://�����Զ���
			{
				log_i(LOG_HOZON, "remote AUTODOOR control req");
			}
			break;
			case PP_RMTCTRL_RMTSRCHVEHICLE://Զ����������
			{
				log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
			}
			break;
			case PP_RMTCTRL_DETECTCAMERA://��ʻԱ�������ͷ
			{
				log_i(LOG_HOZON, "remote DETECTCAMERA control req");
			}
			break;
			case PP_RMTCTRL_DATARECORDER://�г���¼��
			{
				log_i(LOG_HOZON, "remote DATARECORDER control req");
			}
			break;
			case PP_RMTCTRL_AC://�յ�
			{
				SetPP_ACCtrl_Request(&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_CHARGE://���
			{
				SetPP_ChargeCtrl_Request(&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
			}
			break;
			case PP_RMTCTRL_HIGHTENSIONCTRL://�ߵ�ѹ����
			{
				log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
			}
			break;
			case PP_RMTCTRL_ENGINECTRL://����������
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
*��������PP_rmtCtrl_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task)
{

	return 0;
}
#endif

/******************************************************
*��������PP_rmtCtrl_SetCtrlReq

*��  �Σ�

*����ֵ��

*��  �������� ����

*��  ע��
******************************************************/
void PP_rmtCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	switch((uint8_t)(reqType >> 8))
	{
		case PP_RMTCTRL_DOORLOCK://���Ƴ�����
		{
			PP_doorLockCtrl_SetCtrlReq(req,reqType);
		}
		break;
		case PP_RMTCTRL_PNRSUNROOF://����ȫ���촰
		{
			log_i(LOG_HOZON, "remote PANORSUNROOF control req");
		}
		break;
		case PP_RMTCTRL_AUTODOOR://�����Զ���
		{
			log_i(LOG_HOZON, "remote AUTODOOR control req");
		}
		break;
		case PP_RMTCTRL_RMTSRCHVEHICLE://Զ����������
		{
			log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
		}
		break;
		case PP_RMTCTRL_DETECTCAMERA://��ʻԱ�������ͷ
		{
			log_i(LOG_HOZON, "remote DETECTCAMERA control req");
		}
		break;
		case PP_RMTCTRL_DATARECORDER://�г���¼��
		{
			//PP_doorLockCtrl_StatusResp(reqType);
			log_i(LOG_HOZON, "remote DATARECORDER control req");
		}
		break;
		case PP_RMTCTRL_AC://�յ�
		{
			//PP_rmtCtrl_StatusResp(1,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_AC control req");
		}
		break;
		case PP_RMTCTRL_CHARGE://���
		{
			//PP_rmtCtrl_StatusResp(2,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
		}
		break;
		case PP_RMTCTRL_HIGHTENSIONCTRL://�ߵ�ѹ����
		{
			log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
		}
		break;
		case PP_RMTCTRL_ENGINECTRL://����������
		{
			log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
		}
		break;
		default:
		break;
	}
}

#if 0
/******************************************************
*��������PP_rmtCtrl_StatusResp

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static int PP_rmtCtrl_StatusResp(long bookingId,unsigned int reqtype)
{
	int msgdatalen;

	PrvtProt_rmtCtrl_pack_t rmtCtrl_pack;
	memset(&rmtCtrl_pack.DisBody,0,sizeof(PrvtProt_rmtCtrl_pack_t));
	/*header*/
	memcpy(rmtCtrl_pack.Header.sign,"**",2);
	rmtCtrl_pack.Header.ver.Byte = 0x30;
	rmtCtrl_pack.Header.commtype.Byte = 0xe1;
	rmtCtrl_pack.Header.opera = 0x02;
	rmtCtrl_pack.Header.nonce  = PrvtPro_BSEndianReverse(0);
	rmtCtrl_pack.Header.tboxid = PrvtPro_BSEndianReverse(27);
	memcpy(&PP_rmtCtrl_Pack, &rmtCtrl_pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCtrl_pack.DisBody.aID,"110",3);
	rmtCtrl_pack.DisBody.mID = PP_MID_RMTCTRL_BOOKINGRESP;
	rmtCtrl_pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_BOOKINGRESP;
	rmtCtrl_pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCtrl_pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCtrl_pack.DisBody.appDataProVer = 256;
	rmtCtrl_pack.DisBody.testFlag = 1;
	rmtCtrl_pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	PrvtProt_App_rmtCtrl_t app_rmtCtrl;
	/*appdata*/
	app_rmtCtrl.CtrlbookingResp.bookingId = bookingId;
	app_rmtCtrl.CtrlbookingResp.rvcReqCode = (long)reqtype;
	app_rmtCtrl.CtrlbookingResp.oprTime = PrvtPro_getTimestamp();

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_BOOKINGRESP,PP_rmtCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtCtrl_pack.DisBody,&app_rmtCtrl))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	PP_rmtCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	//res = sockproxy_MsgSend(PP_rmtCtrl_Pack.Header.sign,18 + msgdatalen,NULL);
	SP_data_write(PP_rmtCtrl_Pack.Header.sign,18 + msgdatalen,PP_rmtCtrl_send_cb,NULL);
	return 0;
}
#endif

/******************************************************
*��������PP_rmtCtrl_StInformTsp

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
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
		case PP_RMTCTRL_RVCSTATUSRESP://��ԤԼ
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
			App_rmtCtrl.CtrlResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps״̬ 0-��Ч��1-��Ч;
			App_rmtCtrl.CtrlResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gpsʱ���:ϵͳʱ��(ͨ��gpsУʱ)

			PrvtProtCfg_gpsData(&gpsDt);

			if(App_rmtCtrl.CtrlResp.gpsPos.gpsSt == 1)
			{
				if(gpsDt.is_north)
				{
					App_rmtCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
				}
				else
				{
					App_rmtCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
				}

				if(gpsDt.is_east)
				{
					App_rmtCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
				}
				else
				{
					App_rmtCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
				}
				log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.latitude);
				log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtCtrl.CtrlResp.gpsPos.longitude);
			}
			else
			{
				App_rmtCtrl.CtrlResp.gpsPos.latitude  = 0;
				App_rmtCtrl.CtrlResp.gpsPos.longitude = 0;
			}
			App_rmtCtrl.CtrlResp.gpsPos.altitude = (long)(gpsDt.height);//�߶ȣ�m��
			if(App_rmtCtrl.CtrlResp.gpsPos.altitude > 10000)
			{
				App_rmtCtrl.CtrlResp.gpsPos.altitude = 10000;
			}
			App_rmtCtrl.CtrlResp.gpsPos.heading = (long)(gpsDt.direction);//��ͷ����Ƕȣ�0Ϊ��������
			App_rmtCtrl.CtrlResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//�ٶ� x 10����λkm/h
			App_rmtCtrl.CtrlResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//ˮƽ�������� x 10
			if(App_rmtCtrl.CtrlResp.gpsPos.hdop > 1000)
			{
				App_rmtCtrl.CtrlResp.gpsPos.hdop = 1000;
			}

			App_rmtCtrl.CtrlResp.basicSt.driverDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.driverLock = 1;
			App_rmtCtrl.CtrlResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerLock = 1;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftLock = 1;
			App_rmtCtrl.CtrlResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightLock = 1;
			App_rmtCtrl.CtrlResp.basicSt.bootStatus = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.bootStatusLock = 1;
			App_rmtCtrl.CtrlResp.basicSt.driverWindow = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.passengerWindow = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearLeftWindow = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.rearRightWinow = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.sunroofStatus = 1	/* OPTIONAL */;
			App_rmtCtrl.CtrlResp.basicSt.engineStatus = 1;
			App_rmtCtrl.CtrlResp.basicSt.accStatus = 1;
			App_rmtCtrl.CtrlResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
			App_rmtCtrl.CtrlResp.basicSt.accMode = 1	/* OPTIONAL */;
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
											   &PP_rmtCtrl.pack.DisBody,&App_rmtCtrl))//���ݱ������Ƿ����
			{
				log_e(LOG_HOZON, "uper error");
				return -1;
			}
		}
		break;
		case PP_RMTCTRL_RVCBOOKINGRESP://ԤԼ
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
											   &PP_rmtCtrl.pack.DisBody,&App_rmtCtrl))//���ݱ������Ƿ����
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

	static PrvtProt_TxInform_t rmtCtrl_TxInform;
	memset(&rmtCtrl_TxInform,0,sizeof(PrvtProt_TxInform_t));
	rmtCtrl_TxInform.aid = PP_AID_RMTCTRL;
	rmtCtrl_TxInform.mid = PP_MID_RMTCTRL_RESP;
	rmtCtrl_TxInform.pakgtype = PP_TXPAKG_CONTINUE;

	SP_data_write(PP_rmtCtrl_Pack.Header.sign,PP_rmtCtrl_Pack.totallen,PP_rmtCtrl_send_cb,&rmtCtrl_TxInform);

	protocol_dump(LOG_HOZON, "get_remote_control_response", PP_rmtCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;

}

/******************************************************
*��������PP_rmtCtrl_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
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
