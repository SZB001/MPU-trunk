/******************************************************
�ļ�����	PrvtProt_xcall.c

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
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "PrvtProt_SigParse.h"
#include "PrvtProt_xcall.h"

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
}__attribute__((packed))  PrvtProt_xcall_pack_t; /**/

typedef struct
{
	//PrvtProt_xcall_pack_t 	packReq;
	PrvtProt_xcall_pack_t 	packResp;
	PrvtProt_xcallSt_t	 	state;
	char 					Type;
	long					eventId;
	char					activeflag;
}__attribute__((packed))  PrvtProt_xcall_t; /*xcall�ṹ��*/

static PrvtProt_pack_t 		PP_Xcall_Pack;

static PrvtProt_xcall_t	PP_xcall[PP_XCALL_MAX];

static PrvtProt_App_Xcall_t	Appdata_Xcall =
{
	/*xcallType engineSt totalOdoMr	gps{gpsSt latitude longitude altitude heading gpsSpeed hdop}   srsSt  updataTime	battSOCEx*/
		0,		0xff,	 0,		       {0,    0,       0,       0,        0,       0,       0  },	1,		0,			 0
};

//static PrvtProt_TxInform_t Xcall_TxInform[PP_XCALL_MAX];

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_xcall_do_checksock(PrvtProt_task_t *task);
static int PP_xcall_do_rcvMsg(PrvtProt_task_t *task);
static void PP_xcall_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_xcall_do_wait(PrvtProt_task_t *task);
static int PP_xcall_do_checkXcall(PrvtProt_task_t *task);
static int PP_xcall_xcallResponse(PrvtProt_task_t *task,unsigned char XcallType);

static void PP_xcall_send_cb(void * para);
static char PP_xcall_VivoDetection(void);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_xcall_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_xcall_init(void)
{
	int i;

	for(i = 0;i < PP_XCALL_MAX;i++)
	{
		memset(&PP_xcall[i],0 , sizeof(PrvtProt_xcall_t));
		memcpy(PP_xcall[i].packResp.Header.sign,"**",2);
		PP_xcall[i].packResp.Header.commtype.Byte = 0xe1;
		PP_xcall[i].packResp.Header.ver.Byte = 0x30;
		PP_xcall[i].packResp.Header.opera = 0x02;
		PP_xcall[i].packResp.Header.tboxid = 27;

		memcpy(PP_xcall[i].packResp.DisBody.aID,"170",3);
		PP_xcall[i].packResp.DisBody.mID = 2;
		PP_xcall[i].packResp.DisBody.eventId = PP_INIT_EVENTID;
		PP_xcall[i].packResp.DisBody.appDataProVer = 256;
		PP_xcall[i].packResp.DisBody.testFlag = 1;
		PP_xcall[i].Type = i + 1;
		PP_xcall[i].state.req = 0;

	}
}

/******************************************************
*��������PP_xcall_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_xcall_mainfunction(void *task)
{
	int res;
	res = 		PP_xcall_do_checksock((PrvtProt_task_t*)task) ||
				PP_xcall_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_xcall_do_wait((PrvtProt_task_t*)task) ||
				PP_xcall_do_checkXcall((PrvtProt_task_t*)task);
	return res;
}

/******************************************************
*��������PP_xcall_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_xcall_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}
	return -1;
}

/******************************************************
*��������PP_xcall_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_xcall_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_XCALL,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive xcall message");
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
	PP_xcall_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_xcall_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_xcall_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	if(PP_OPERATETYPE_NGTP != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_Xcall_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_XCALL != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	switch(MsgDataBody.mID)
	{
		case PP_MID_XCALL_REQ://车辆状态查询请求
		{
			if((Appdata.xcallType >=1) && (Appdata.xcallType <= PP_XCALL_MAX))
			{
				PP_xcall[Appdata.xcallType-1].eventId = MsgDataBody.eventId;
				PP_xcall[Appdata.xcallType-1].state.req = 1;
				PP_xcall[Appdata.xcallType-1].activeflag = 0;
				log_i(LOG_HOZON, "recv xcall request\n");
			}
		}
		break;
		default:
		break;
	}
}

/******************************************************
*��������PP_xcall_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_xcall_do_wait(PrvtProt_task_t *task)
{
	return 0;
}

/******************************************************
*��������PP_xcall_do_checkXcall

*��  �Σ�

*����ֵ��

*��  �������ecall������

*��  ע��
******************************************************/
static int PP_xcall_do_checkXcall(PrvtProt_task_t *task)
{
	int idlenode;
	/* ecall */
	if(PrvtProtCfg_ecallTriggerEvent())//ecall����
	{
		PP_xcall[PP_ECALL].state.req = 1;
		PP_xcall[PP_ECALL].activeflag = 1;
	}

	if(PrvtProtCfg_bcallTriggerEvent())//ecall����
	{
		PP_xcall[PP_BCALL].state.req = 1;
		PP_xcall[PP_BCALL].activeflag = 1;
	}

	if(PP_xcall_VivoDetection())//Live detection warning
	{
		PP_xcall[PP_detection].state.req = 1;
		PP_xcall[PP_detection].activeflag = 1;
	}

	if(1 == PP_xcall[PP_ECALL].state.req)//ecall����
	{
		log_i(LOG_HOZON, "ecall trig\n");
		PP_xcall[PP_ECALL].state.req = 0;
		if(0 == PP_xcall_xcallResponse(task,PP_ECALL))
		{
			idlenode = PP_getIdleNode();
			//memset(&PP_TxInform[idlenode],0,sizeof(PrvtProt_TxInform_t));
			PP_TxInform[idlenode].aid = PP_AID_XCALL;
			PP_TxInform[idlenode].mid = PP_MID_XCALL_RESP;
			PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
			PP_TxInform[idlenode].idleflag = 1;

			SP_data_write(PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,PP_xcall_send_cb,&PP_TxInform[idlenode]);
			protocol_dump(LOG_HOZON, "xcall_response", PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,1);
		}
	}

	/* bcall */
	if(1 == PP_xcall[PP_BCALL].state.req)
	{
		log_i(LOG_HOZON, "bcall trig\n");
		PP_xcall[PP_BCALL].state.req = 0;
		if(0 == PP_xcall_xcallResponse(task,PP_BCALL))
		{
			idlenode = PP_getIdleNode();
			//memset(&PP_TxInform[idlenode],0,sizeof(PrvtProt_TxInform_t));
			PP_TxInform[idlenode].aid = PP_AID_XCALL;
			PP_TxInform[idlenode].mid = PP_MID_XCALL_RESP;
			PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
			PP_TxInform[idlenode].idleflag = 1;

			SP_data_write(PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,PP_xcall_send_cb,&PP_TxInform[idlenode]);
			protocol_dump(LOG_HOZON, "xcall_response", PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,1);
		}
	}

	/* icall */
	if(1 == PP_xcall[PP_ICALL].state.req)
	{
		log_i(LOG_HOZON, "icall trig\n");
		PP_xcall[PP_ICALL].state.req = 0;
		if(0 == PP_xcall_xcallResponse(task,PP_ICALL))
		{
			idlenode = PP_getIdleNode();
			//memset(&PP_TxInform[idlenode],0,sizeof(PrvtProt_TxInform_t));
			PP_TxInform[idlenode].aid = PP_AID_XCALL;
			PP_TxInform[idlenode].mid = PP_MID_XCALL_RESP;
			PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
			PP_TxInform[idlenode].idleflag = 1;

			SP_data_write(PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,PP_xcall_send_cb,&PP_TxInform[idlenode]);
			protocol_dump(LOG_HOZON, "xcall_response", PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,1);
		}
	}

	/* Live detection warning  */
	if(1 == PP_xcall[PP_detection].state.req)
	{
		log_i(LOG_HOZON, "detection trig\n");
		PP_xcall[PP_detection].state.req = 0;
		if(0 == PP_xcall_xcallResponse(task,PP_detection))
		{
			idlenode = PP_getIdleNode();
			//memset(&PP_TxInform[idlenode],0,sizeof(PrvtProt_TxInform_t));
			PP_TxInform[idlenode].aid = PP_AID_XCALL;
			PP_TxInform[idlenode].mid = PP_MID_XCALL_RESP;
			PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
			PP_TxInform[idlenode].idleflag = 1;

			SP_data_write(PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,PP_xcall_send_cb,&PP_TxInform[idlenode]);
			protocol_dump(LOG_HOZON, "xcall_response", PP_Xcall_Pack.Header.sign,PP_Xcall_Pack.totallen,1);
		}
	}

	return 0;
}

/******************************************************
*��������PP_xcall_xcallResponse

*��  �Σ�

*����ֵ��

*��  ����xcall response

*��  ע��
******************************************************/
static int PP_xcall_xcallResponse(PrvtProt_task_t *task,unsigned char XcallType)
{
	int msgdatalen;
	int res = 0;

	/* header */
	memcpy(PP_xcall[XcallType].packResp.Header.sign,"**",2);
	PP_xcall[XcallType].packResp.Header.commtype.Byte = 0xe1;
	PP_xcall[XcallType].packResp.Header.ver.Byte = 0x30;
	PP_xcall[XcallType].packResp.Header.opera = 0x02;
	PP_xcall[XcallType].packResp.Header.ver.Byte = task->version;
	PP_xcall[XcallType].packResp.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_xcall[XcallType].packResp.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_Xcall_Pack, &PP_xcall[XcallType].packResp.Header, sizeof(PrvtProt_pack_Header_t));

	/* disbody */
	memcpy(PP_xcall[XcallType].packResp.DisBody.aID,"170",3);
	PP_xcall[XcallType].packResp.DisBody.mID = PP_MID_XCALL_RESP;
	PP_xcall[XcallType].packResp.DisBody.eventTime = PrvtPro_getTimestamp();

	if(1 == PP_xcall[XcallType].activeflag)
	{
		PP_xcall[XcallType].packResp.DisBody.eventId = 0;
	}
	else
	{
		PP_xcall[XcallType].packResp.DisBody.eventId = PP_xcall[XcallType].eventId;
	}
	PP_xcall[XcallType].activeflag = 0;
	PP_xcall[XcallType].packResp.DisBody.expTime = PrvtPro_getTimestamp();
	PP_xcall[XcallType].packResp.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_xcall[XcallType].packResp.DisBody.appDataProVer = 256;
	PP_xcall[XcallType].packResp.DisBody.testFlag = 1;

	/*appdata*/
	PrvtProtcfg_gpsData_t gpsDt;
	PrvtProtCfg_gpsData(&gpsDt);
	log_i(LOG_HOZON, "is_north = %d",gpsDt.is_north);
	log_i(LOG_HOZON, "is_east = %d",gpsDt.is_east);
	log_i(LOG_HOZON, "latitude = %lf",gpsDt.latitude);
	log_i(LOG_HOZON, "longitude = %lf",gpsDt.longitude);
	log_i(LOG_HOZON, "altitude = %lf",gpsDt.height);
	Appdata_Xcall.xcallType = PP_xcall[XcallType].Type;//xcall type:ecall/icall/bcall
	Appdata_Xcall.engineSt = PrvtProtCfg_engineSt();//����״̬��1-Ϩ��2-����
	Appdata_Xcall.totalOdoMr = PrvtProtCfg_totalOdoMr();//�����Ч��Χ��0 - 1000000��km��
	if(Appdata_Xcall.totalOdoMr > 1000000)
	{
		Appdata_Xcall.totalOdoMr = 1000000;
	}

	Appdata_Xcall.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps״̬ 0-��Ч��1-��Ч
	Appdata_Xcall.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gpsʱ���:ϵͳʱ��(ͨ��gpsУʱ)
	if(Appdata_Xcall.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			Appdata_Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000);//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			Appdata_Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}

		if(gpsDt.is_east)
		{
			Appdata_Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000);//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			Appdata_Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",Appdata_Xcall.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",Appdata_Xcall.gpsPos.longitude);
	}
	else
	{
		Appdata_Xcall.gpsPos.latitude  = 0;
		Appdata_Xcall.gpsPos.longitude = 0;
	}

	Appdata_Xcall.gpsPos.altitude = (long)gpsDt.height;//�߶ȣ�m��
	if(Appdata_Xcall.gpsPos.altitude > 10000)
	{
		Appdata_Xcall.gpsPos.altitude = 10000;
	}
	Appdata_Xcall.gpsPos.heading = (long)gpsDt.direction;//��ͷ����Ƕȣ�0Ϊ��������
	Appdata_Xcall.gpsPos.gpsSpeed = (long)gpsDt.kms*10;//�ٶ� x 10����λkm/h
	Appdata_Xcall.gpsPos.hdop = (long)gpsDt.hdop*10;//ˮƽ�������� x 10
	if(Appdata_Xcall.gpsPos.hdop > 1000)
	{
		Appdata_Xcall.gpsPos.hdop = 1000;
	}
	Appdata_Xcall.srsSt 		= PrvtProtCfg_CrashOutputSt();//��ȫ����״̬ 1- ������2 - ����
	Appdata_Xcall.updataTime 	= PrvtPro_getTimestamp();//����ʱ���
	Appdata_Xcall.battSOCEx 	= PrvtProtCfg_vehicleSOC();//�������ʣ�������0-10000��0%-100%��
	if (Appdata_Xcall.battSOCEx > 10000)
	{
  		Appdata_Xcall.battSOCEx = 10000;
 	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_XCALL_RESP,PP_Xcall_Pack.msgdata,&msgdatalen,\
									   &PP_xcall[XcallType].packResp.DisBody,&Appdata_Xcall))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_Xcall_Pack.totallen = 18 + msgdatalen;
	PP_Xcall_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return res;
}

/******************************************************
*��������PP_xcall_send_cb

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static void PP_xcall_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)para;
	log_i(LOG_HOZON, "aid = %d",TxInform_ptr->aid);
	log_i(LOG_HOZON, "mid = %d",TxInform_ptr->mid);
	log_i(LOG_HOZON, "pakgtype = %d",TxInform_ptr->pakgtype);
	log_i(LOG_HOZON, "eventtime = %d",TxInform_ptr->eventtime);
	log_i(LOG_HOZON, "successflg = %d",TxInform_ptr->successflg);
	log_i(LOG_HOZON, "failresion = %d",TxInform_ptr->failresion);
	log_i(LOG_HOZON, "txfailtime = %d",TxInform_ptr->txfailtime);

	TxInform_ptr->idleflag = 0;
}

/******************************************************
*��������PP_xcall_SetXcallReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PP_xcall_SetXcallReq(unsigned char req)
{

	PP_xcall[(req-1)].state.req = 1;
}

/*
* 活体检测
*/
static char PP_xcall_VivoDetection(void)
{
	static uint64_t lastsendtime;
	static uint8_t cnt;
	if(PrvtProtCfg_detectionTriggerSt() == 1)
	{
		if(cnt < 3)
		{
			if(tm_get_time() - lastsendtime > 10000)
			{
				cnt++;
				lastsendtime = tm_get_time() ;
				return 1;
			}
		}
		else
		{
			if(tm_get_time() - lastsendtime > 60000)
			{
				lastsendtime = tm_get_time() ;
				return 1;
			}
		}
	}
	else
	{
		cnt = 0;
	}
	return 0;
}