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
#include "PrvtProt.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
//static PrvtProt_heartbeat_t heartbeat;
static PrvtProt_task_t 	pp_task;
static PrvtProt_pack_t 	PP_RxPack;
static PrvtProt_pack_Header_t 	PP_PackHeader[PP_APP_MAX] =
{/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
	{"**",	0x30,	0,		0x70,		0,			0x01,	18,		  0	  },//heart beat
	{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  },//ecall req
	{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  } //ecall response
};

static PrvtProt_pack_t 	PP_Pack[PP_APP_MAX];

static PrvtProt_DisptrBody_t	PP_DisptrBody[PP_APP_MAX] =
{/*   AID  MID  EventTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
	{"000",	0,		0,		PP_INVALID,	   0,	   0,		0,       0,       0,      0,     0,		 0,         0   },//ecall req
	{"170",	1,		0,		PP_INVALID,	   0,	   0,		0,       0,       0,      0,     1,		 1,         0   },//ecall req
	{"170", 2,      0,      PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,		 1,			0   } //ecall response
};
static PrvtProt_appData_t 		PP_appData;

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
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task);
static int PrvtPro_ecallReq(PrvtProt_task_t *task);
static int PrvtPro_ecallResponse(PrvtProt_task_t *task);
static long PrvtPro_getTimestamp(void);
static long PrvtPro_BSEndianReverse(long value);
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

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			pp_task.heartbeat.ackFlag = 0;
			pp_task.heartbeat.state = 0;//
			pp_task.heartbeat.period = PP_HEART_BEAT_TIME;//
			pp_task.heartbeat.timer = tm_get_time();
			pp_task.waitSt = PP_IDLE;
			pp_task.waittime = 0;
			pp_task.suspend = 0;
			pp_task.xcall[PP_ECALL].req = 0;
			pp_task.xcall[PP_ECALL].resp = 0;
			pp_task.nonce = 0;/* TCP�ỰID ��TSPƽ̨���� */
			pp_task.version = 0x30;/* ��/С�汾(��TSPƽ̨����)*/
			pp_task.tboxid = 204;/* ƽ̨ͨ��tboxID��tboxSNӳ�� */
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			PrvtProt_shell_init();
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
	log_o(LOG_HOZON, "proprietary protocol  of hozon thread running");
	int res;
    prctl(PR_SET_NAME, "HZ_PRVT_PROT");
    while (1)
    {
		if(pp_task.suspend != 0)
		{
			continue;
		}
		log_set_level(LOG_HOZON, LOG_DEBUG);

		res = 	PrvtPro_do_checksock(&pp_task) ||
				PrvtPro_do_rcvMsg(&pp_task) ||
				PrvtPro_do_wait(&pp_task) || 
				PrvtProt_do_heartbeat(&pp_task) ||
				PrvtPro_do_checkXcall(&pp_task);
		
    }
	(void)res;
    return NULL;
}
#endif

/******************************************************
*��������PrvtProt_do_heartbeat

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task)
{
	PrvtProt_pack_Header_t pack_Header;

	if((tm_get_time() - task->heartbeat.timer) > (task->heartbeat.period*1000))
	{
		PP_PackHeader[PP_APP_HEARTBEAT].ver.Byte = task->version;
		PP_PackHeader[PP_APP_HEARTBEAT].nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader[PP_APP_HEARTBEAT].msglen = PrvtPro_BSEndianReverse((long)18);
		PP_PackHeader[PP_APP_HEARTBEAT].tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		memcpy(&pack_Header, &PP_PackHeader[PP_APP_HEARTBEAT], sizeof(PrvtProt_pack_Header_t));

		if(sockproxy_MsgSend(pack_Header.sign, 18,NULL) > 0)//���ͳɹ�
		{
			protocol_dump(LOG_HOZON, "PRVT_PROT", pack_Header.sign, 18, 1);
			task->waitSt = PP_HEARTBEAT;
			task->heartbeat.ackFlag = PP_ACK_WAIT;
			task->waittime = tm_get_time();		
		}	
		task->heartbeat.timer = tm_get_time();
		return -1;
	}
	return 0;
}

/******************************************************
*��������PrvtPro_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PrvtPro_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{
		
		return 0;
	}
	PrvtPro_do_checkXcall(task);
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
	
	if ((rlen = PrvtProtCfg_rcvMsg(rcvbuf,1456)) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "HOZON private protocol receive message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcvbuf, rlen, 0);
	if((rcvbuf[0] != 0x2A) || (rcvbuf[1] != 0x2A) || \
			(rlen < 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
	{
		return 0;
	}
	PrvtPro_makeUpPack(&PP_RxPack,rcvbuf,rlen);
	PrvtPro_RxMsgHandle(task,&PP_RxPack,rlen);

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
	static int rlen = 0;
	uint8_t rcvstep = 0;

	rlen = 0;
	RxPack->Header.sign[0] = input[rlen++];
	RxPack->Header.sign[1] = input[rlen++];
	len = len-2;
	while(len--)
	{
		switch(rcvstep)
		{
			case 0://���հ汾��
			{
				RxPack->Header.ver.Byte = input[rlen++];
				rcvstep = 1;
			}
			break;
			case 1://����tcp�Ựid
			{
				RxPack->Header.nonce = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
				rlen += 4;
				rcvstep = 2;
			}
			break;	
			case 2://���롢���ӵȷ�ʽ
			{
				RxPack->Header.commtype.Byte = input[rlen++];
				rcvstep = 3;
			}
			break;	
			case 3://���ܡ�ǩ����ʽ
			{
				RxPack->Header.safetype.Byte = input[rlen++];
				rcvstep = 4;
			}
			break;
			case 4://��������
			{
				RxPack->Header.opera = input[rlen++];
				rcvstep = 5;
			}
			break;
			case 5://���ĳ���
			{
				RxPack->Header.msglen = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
				rlen += 4;
				rcvstep = 6;
			}
			break;
			case 6://tboxid
			{
				RxPack->Header.tboxid = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
				rlen += 4;
				rcvstep = 7;
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
	switch(rxPack->Header.opera)
	{
		case PP_NATIONALSTANDARD_TYPE:
		{
			
		}
		break;
		case PP_HEARTBEAT_TYPE://���յ�������
		{
			log_i(LOG_HOZON, "heart beat is ok");
			task->heartbeat.state = 1;//��������
			task->waitSt = 0;
		}
		break;
		case PP_NGTP_TYPE://ngtp
		{
			
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
    if (!task->waitSt)//û���¼��ȴ�Ӧ��
    {
        return 0;
    }

    if((tm_get_time() - task->waittime) > PP_WAIT_TIMEOUT)
    {
        if (task->waitSt == PP_HEARTBEAT)
        {
            task->waitSt = PP_IDLE;
			task->heartbeat.state = 0;//����������
            log_e(LOG_HOZON, "heartbeat time out");
        }
        else
        {
			
		}
    }
	 return -1;
}

/******************************************************
*��������PrvtPro_do_checkXcall

*��  �Σ�

*����ֵ��

*��  �������ecall������

*��  ע��
******************************************************/
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task)
{
	/* ecall */
	PrvtPro_ecallReq(task);
	PrvtPro_ecallResponse(task);

	/* bcall */

	/* icall */
	return 0;
}

/******************************************************
*��������PrvtPro_ecallResponse

*��  �Σ�

*����ֵ��

*��  ����ecall response

*��  ע��
******************************************************/
static int PrvtPro_ecallResponse(PrvtProt_task_t *task)
{
	long msgdatalen;

	if(1 == sockproxy_socketState())//socket open
	{
		if((1 == pp_task.xcall[PP_ECALL].resp) || (PrvtProtCfg_ecallTriggerEvent()))//ecall����
		{
			PP_PackHeader[PP_ECALL_RESP].ver.Byte = task->version;
			PP_PackHeader[PP_ECALL_RESP].nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
			PP_PackHeader[PP_ECALL_RESP].tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
			memcpy(&PP_Pack[PP_ECALL_RESP], &PP_PackHeader[PP_ECALL_RESP], sizeof(PrvtProt_pack_Header_t));

			PP_DisptrBody[PP_ECALL_RESP].eventTime = PrvtPro_getTimestamp();
			PP_DisptrBody[PP_ECALL_RESP].ulMsgCnt++;	/* OPTIONAL */

			PrvtProtcfg_gpsData_t gpsDt;
			PP_appData.Xcall.xcallType = 2;//������Ԯecall
			PP_appData.Xcall.engineSt = PrvtProtCfg_engineSt();//����״̬��1-Ϩ��2-����
			PP_appData.Xcall.totalOdoMr = PrvtProtCfg_totalOdoMr();//�����Ч��Χ��0 - 1000000��km��
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
			PP_appData.Xcall.gpsPos.heading = (long)gpsDt.direction;//��ͷ����Ƕȣ�0Ϊ��������
			PP_appData.Xcall.gpsPos.gpsSpeed = (long)gpsDt.kms*10;//�ٶ� x 10����λkm/h
			PP_appData.Xcall.gpsPos.hdop = (long)gpsDt.hdop*10;//ˮƽ�������� x 10

			PP_appData.Xcall.srsSt = 1;//��ȫ����״̬ 1- ������2 - ����
			PP_appData.Xcall.updataTime = PrvtPro_getTimestamp();//����ʱ���
			PP_appData.Xcall.battSOCEx = 10;//�������ʣ�������0-10000��0%-100%��

			if(0 == PrvtPro_msgPackageEncoding(PP_ECALL_RESP,PP_Pack[PP_ECALL_RESP].msgdata,&msgdatalen,\
									   	   	   &PP_DisptrBody[PP_ECALL_RESP],&PP_appData))
			{
				PP_Pack[PP_ECALL_RESP].Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
				if(sockproxy_MsgSend(PP_Pack[PP_ECALL_RESP].Header.sign,18 + msgdatalen,NULL) > 0)//���ͳɹ�
				{
					protocol_dump(LOG_HOZON, "PRVT_PROT", PP_Pack[PP_ECALL_RESP].Header.sign, \
							18 + msgdatalen,1);
				}
			}
			pp_task.xcall[PP_ECALL].resp = 0;
		}
	}
	return 0;
}

/******************************************************
*��������PrvtPro_ecallReq

*��  �Σ�

*����ֵ��

*��  �������ecall������

*��  ע��
******************************************************/
static int PrvtPro_ecallReq(PrvtProt_task_t *task)
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
		
		PP_appData.Xcall.xcallType = 2;//������Ԯ
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

/******************************************************
*��������

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static long PrvtPro_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return (long)timestamp.tv_sec;
	
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
	pp_task.heartbeat.period = period;
}

/******************************************************
*��������PrvtPro_Setsuspend

*��  �Σ�

*����ֵ��

*��  ����������ͣ

*��  ע��
******************************************************/
void PrvtPro_Setsuspend(unsigned char suspend)
{
	pp_task.suspend = suspend;
}

/******************************************************
*��������PrvtPro_SetEcallReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PrvtPro_SetEcallReq(unsigned char req)
{
	pp_task.xcall[PP_ECALL].req = req;
}

/******************************************************
*��������PrvtPro_SetEcallResp

*��  �Σ�

*����ֵ��

*��  ��������ecall response

*��  ע��
******************************************************/
void PrvtPro_SetEcallResp(unsigned char resp)
{
	pp_task.xcall[PP_ECALL].resp = resp;
}

/******************************************************
*��������PrvtPro_BSEndianReverse

*��  �Σ�void

*����ֵ��void

*��  �������ģʽ��С��ģʽ����ת��

*��  ע��
******************************************************/
static long PrvtPro_BSEndianReverse(long value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
		   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

/******************************************************
*��������PrvtProt_FieldPadding

*��  �Σ�

*����ֵ��

*��  �����ֶ����

*��  ע��
******************************************************/
//static int PrvtProt_FieldPadding(PrvtProt_task_t *task)
//{

//}
