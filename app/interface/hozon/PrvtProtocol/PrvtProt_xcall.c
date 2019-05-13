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
	PrvtProt_xcall_pack_t 	packReq;
	PrvtProt_xcall_pack_t 	packResp;
	PrvtProt_xcallSt_t	 	state;
	char 					Type;
}__attribute__((packed))  PrvtProt_xcall_t; /*xcall�ṹ��*/

static PrvtProt_pack_t 		PP_Xcall_Pack;

static PrvtProt_xcall_t	PP_xcall[PP_XCALL_MAX] =
{
		{
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					 {"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  },//ecall req
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					 {"170",1,		0,		  0,	PP_INVALID,	   0,	   0,		 0,       0,       0,      0,     256,	 1,        0   },//bcall req

			},
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  }, //ecall response
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					{"170", 2,      0,        0,		PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,	1,		0   } //bcall response
			},
			{0,0,0,0},
			PP_BCALL_TYPE
		},
		{
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					 {"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  },//ecall req
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					 {"170",1,		0,		  0,	PP_INVALID,	   0,	   0,		 0,       0,       0,      0,     256,	 1,        0   },//ecall req

			},
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  }, //ecall response
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					{"170", 2,      0,        0,		PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,	1,		0   } //ecall response
			},
			{0,0,0,0},
			PP_ECALL_TYPE
		},
		{
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					 {"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  },//ecall req
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					 {"170",1,		0,		  0,	PP_INVALID,	   0,	   0,		 0,       0,       0,      0,     256,	 1,        0   },//icall req

			},
			{
					/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
					{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  }, //ecall response
				  /*   AID  MID  EventTime	ExpTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
					{"170", 2,      0,        0,		PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,	1,		0   } //icall response
			},
			{0,0,0,0},
			PP_ICALL_TYPE
		},

};

//static PrvtProt_xcallSt_t PP_xcall[PP_XCALL_MAX];

//const char PP_xcallType[PP_XCALL_MAX] =
//{
//	PP_BCALL_TYPE,PP_ECALL_TYPE,PP_ICALL_TYPE
//};

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
//static int PrvtPro_ecallReq(PrvtProt_task_t *task);
static int PP_xcall_xcallResponse(PrvtProt_task_t *task,unsigned char XcallType);

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
	PP_xcall[PP_ECALL].state.req = 0;
	PP_xcall[PP_ECALL].state.resp = 0;
	PP_xcall[PP_ECALL].state.waitSt = 0;
	PP_xcall[PP_ECALL].state.waittime= 0;
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
	switch(rxPack->Header.opera)
	{
		case PP_NGTP_TYPE://ngtp
		{
			PrvtProt_msgData_t MsgData;
			PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgData,1);
			aid = (MsgData.DisBody.aID[0] - 0x30)*100 +  (MsgData.DisBody.aID[1] - 0x30)*10 + \
					  (MsgData.DisBody.aID[2] - 0x30);
			switch(aid)
			{
				case PP_AID_XCALL://XCALL
				{
					if(PP_MID_XCALL_RESP == MsgData.DisBody.mID)//�յ�xcall response�ظ�
					{
						if(PP_xcall[PP_ECALL].state.waitSt == 1)//���յ��ظ�
						{
							PP_xcall[PP_ECALL].state.waitSt = 0;
							log_i(LOG_HOZON, "\r\necall ok\r\n");
						}
					}
				}
				break;
				default:
				break;
			}
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
*��������PP_xcall_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_xcall_do_wait(PrvtProt_task_t *task)
{
    //if (!task->waitSt[PP_APP_XCALL])//û���¼��ȴ�Ӧ��
    //{
    //    return 0;
    //}

    if(PP_xcall[PP_ECALL].state.waitSt == 1)
    {
    	 if((tm_get_time() - PP_xcall[PP_ECALL].state.waittime) > PP_XCALL_WAIT_TIMEOUT)
    	 {
    		 PP_xcall[PP_ECALL].state.waitSt = 0;
    		 log_e(LOG_HOZON, "ecall time out");
    		 return 0;
    	 }
    	 return -1;
    }

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
	int res;
	/* ecall */
	if(1 == sockproxy_socketState())//socket open
	{
		if((1 == PP_xcall[PP_ECALL].state.resp) || (PrvtProtCfg_ecallTriggerEvent()))//ecall����
		{
			PP_xcall[PP_ECALL].state.resp = 0;
			res = PP_xcall_xcallResponse(task,PP_ECALL);
		}

	/* bcall */

	/* icall */

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
	long msgdatalen;

	PP_xcall[XcallType].packResp.Header.ver.Byte = task->version;
	PP_xcall[XcallType].packResp.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_xcall[XcallType].packResp.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_Xcall_Pack, &PP_xcall[XcallType].packResp.Header, sizeof(PrvtProt_pack_Header_t));

	PP_xcall[XcallType].packResp.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_xcall[XcallType].packResp.DisBody.expTime   = PrvtPro_getTimestamp();
	PP_xcall[XcallType].packResp.DisBody.ulMsgCnt++;	/* OPTIONAL */

	PrvtProtcfg_gpsData_t gpsDt;
	PP_appData.Xcall.xcallType = (long)(PP_xcall[XcallType].Type);//xcall type:ecall/icall/bcall
	PP_appData.Xcall.engineSt = PrvtProtCfg_engineSt();//����״̬��1-Ϩ��2-����
	PP_appData.Xcall.totalOdoMr = PrvtProtCfg_totalOdoMr();//�����Ч��Χ��0 - 1000000��km��
	if(PP_appData.Xcall.totalOdoMr > 1000000)
	{
		PP_appData.Xcall.totalOdoMr = 1000000;
	}
	PP_appData.Xcall.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps״̬ 0-��Ч��1-��Ч
	PP_appData.Xcall.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gpsʱ���:ϵͳʱ��(ͨ��gpsУʱ)

	PrvtProtCfg_gpsData(&gpsDt);
	log_i(LOG_HOZON, "is_north = %d",gpsDt.is_north);
	log_i(LOG_HOZON, "is_east = %d",gpsDt.is_east);
	log_i(LOG_HOZON, "latitude = %lf",gpsDt.latitude);
	log_i(LOG_HOZON, "longitude = %lf",gpsDt.longitude);
	log_i(LOG_HOZON, "altitude = %lf",gpsDt.height);

	if(PP_appData.Xcall.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			PP_appData.Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000);//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			PP_appData.Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}

		if(gpsDt.is_east)
		{
			PP_appData.Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000);//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			PP_appData.Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",PP_appData.Xcall.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",PP_appData.Xcall.gpsPos.longitude);
	}
	else
	{
		PP_appData.Xcall.gpsPos.latitude  = 0;
		PP_appData.Xcall.gpsPos.longitude = 0;
	}
	PP_appData.Xcall.gpsPos.altitude = (long)gpsDt.height;//�߶ȣ�m��
	if(PP_appData.Xcall.gpsPos.altitude > 10000)
	{
		PP_appData.Xcall.gpsPos.altitude = 10000;
	}
	PP_appData.Xcall.gpsPos.heading = (long)gpsDt.direction;//��ͷ����Ƕȣ�0Ϊ��������
	PP_appData.Xcall.gpsPos.gpsSpeed = (long)gpsDt.kms*10;//�ٶ� x 10����λkm/h
	PP_appData.Xcall.gpsPos.hdop = (long)gpsDt.hdop*10;//ˮƽ�������� x 10
	if(PP_appData.Xcall.gpsPos.hdop > 1000)
	{
		PP_appData.Xcall.gpsPos.hdop = 1000;
	}
	PP_appData.Xcall.srsSt = 1;//��ȫ����״̬ 1- ������2 - ����
	PP_appData.Xcall.updataTime = PrvtPro_getTimestamp();//����ʱ���
	PP_appData.Xcall.battSOCEx = PrvtProtCfg_vehicleSOC();//�������ʣ�������0-10000��0%-100%��

	if(0 != PrvtPro_msgPackageEncoding(PP_XCALL_RESP,PP_Xcall_Pack.msgdata,&msgdatalen,\
									   &PP_xcall[XcallType].packResp.DisBody,&PP_appData))//���ݱ������Ƿ����
	{
		return -1;
	}

	PP_Xcall_Pack.Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
	if(sockproxy_MsgSend(PP_Xcall_Pack.Header.sign,18 + msgdatalen,NULL) <= 0)//����ʧ��
	{
		return -2;
	}

	if(1 == PP_xcall[XcallType].packResp.DisBody.ackReq)
	{
		PP_xcall[XcallType].state.waitSt 		= 1;
		PP_xcall[XcallType].state.waittime 	= tm_get_time();
		//task->waitSt[PP_APP_XCALL] 		= 1;
	}
	protocol_dump(LOG_HOZON, "PRVT_PROT", PP_Xcall_Pack.Header.sign, \
					18 + msgdatalen,1);
	return 0;
}
#if 0
/******************************************************
*��������PP_xcall_ecallReq

*��  �Σ�

*����ֵ��

*��  �������ecall������

*��  ע��
******************************************************/
static int PP_xcall_ecallReq(PrvtProt_task_t *task)
{

	long msgdatalen;

	if(1 == pp_task.xcall[PP_ECALL].req)
	{	
		PP_PackHeader[PP_ECALL_REQ].ver.Byte = task->version;
		PP_PackHeader[PP_ECALL_REQ].nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader[PP_ECALL_REQ].tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		memcpy(&PP_Pack[PP_ECALL_REQ], &PP_PackHeader[PP_ECALL_REQ], sizeof(PrvtProt_pack_Header_t));

		PP_DisptrBody[PP_ECALL_REQ].eventTime = PrvtPro_getTimestamp();
		PP_DisptrBody[PP_ECALL_REQ].ulMsgCnt++;	/* OPTIONAL */
		
		PP_appData.Xcall.xcallType = PP_ECALL_TYPE;//������Ԯ
		if(0 == PrvtPro_msgPackageEncoding(PP_ECALL_REQ,PP_Pack[PP_ECALL_REQ].msgdata,&msgdatalen, \
											&PP_DisptrBody[PP_ECALL_REQ],&PP_appData))
		{
			PP_Pack[PP_ECALL_REQ].Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
			if(sockproxy_MsgSend(PP_Pack[PP_ECALL_REQ].Header.sign,(18 + msgdatalen),NULL) > 0)//���ͳɹ�
			{
				pp_task.xcall[PP_ECALL].req = 0;
				protocol_dump(LOG_HOZON, "PRVT_PROT", PP_Pack[PP_ECALL_REQ].Header.sign, \
							  (18 + msgdatalen),1);
			}
		}
		pp_task.xcall[PP_ECALL].req = 0;
	}
	
	return 0;
}
#endif

/******************************************************
*��������PP_xcall_SetEcallReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PP_xcall_SetEcallReq(unsigned char req)
{
	PP_xcall[PP_ECALL].state.req = req;
}

/******************************************************
*��������PP_xcall_SetEcallResp

*��  �Σ�

*����ֵ��

*��  ��������ecall response

*��  ע��
******************************************************/
void PP_xcall_SetEcallResp(unsigned char resp)
{
	PP_xcall[PP_ECALL].state.resp = resp;
}


