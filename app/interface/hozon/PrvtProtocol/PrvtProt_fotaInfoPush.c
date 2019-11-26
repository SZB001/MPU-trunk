/******************************************************
�ļ�����	PrvtProt_fotaInfoPush.c

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
#include "PrvtProt_fotaInfoPush.h"

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
}__attribute__((packed))  PP_FotaInfoPush_pack_t; /**/

typedef struct
{
	PP_FotaInfoPush_pack_t 	packResp;
	long					eventId;
	long					expTime;
	uint8_t 				req;
	uint8_t 				waitSt;
	uint64_t 				waittime;
}__attribute__((packed))  PP_FotaInfoPush_t;

static PrvtProt_pack_t 			PP_FIP_Pack;
static PP_FotaInfoPush_t 		PP_FotaInfoPush;
static PrvtProt_App_FIP_t		Appdata_FIP;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_FIP_do_checksock(PrvtProt_task_t *task);
static int PP_FIP_do_rcvMsg(PrvtProt_task_t *task);
static void PP_FIP_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_FIP_do_wait(PrvtProt_task_t *task);
static int PP_FIP_do_checkInfoPush(PrvtProt_task_t *task);
static int PP_FIP_Response(PrvtProt_task_t *task);

static void PP_FIP_send_cb(void * para);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*PP_FotaInfoPush_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_FotaInfoPush_init(void)
{
		memset(&PP_FotaInfoPush,0 , sizeof(PP_FotaInfoPush_t));
		memcpy(PP_FotaInfoPush.packResp.Header.sign,"**",2);
		PP_FotaInfoPush.packResp.Header.commtype.Byte = 0xe1;
		PP_FotaInfoPush.packResp.Header.ver.Byte = 0x30;
		PP_FotaInfoPush.packResp.Header.opera = 0x02;
		PP_FotaInfoPush.packResp.Header.tboxid = 27;

		memcpy(PP_FotaInfoPush.packResp.DisBody.aID,"180",3);
		PP_FotaInfoPush.packResp.DisBody.eventId = PP_INIT_EVENTID;
		PP_FotaInfoPush.packResp.DisBody.appDataProVer = 256;
		PP_FotaInfoPush.packResp.DisBody.testFlag = 1;
		PP_FotaInfoPush.req = 0;
}

/******************************************************
*PP_FotaInfoPush_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_FotaInfoPush_mainfunction(void *task)
{
	int res;
	res = 		PP_FIP_do_checksock((PrvtProt_task_t*)task) ||
				PP_FIP_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_FIP_do_wait((PrvtProt_task_t*)task) ||
				PP_FIP_do_checkInfoPush((PrvtProt_task_t*)task);
	return res;
}

/******************************************************
*��������PP_FIP_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_FIP_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}
	return -1;
}

/******************************************************
*��������PP_FIP_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_FIP_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_OTA_INFOPUSH,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_o(LOG_HOZON, "receive ota info push message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))
	{
		return 0;
	}
	PP_FIP_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_FIP_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_FIP_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	if(PP_OPERATETYPE_NGTP != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_FIP_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_OTAINFOPUSH != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	switch(MsgDataBody.mID)
	{
		case PP_MID_OTA_INFOPUSHREQ://请求
		{
			PP_FotaInfoPush.eventId = MsgDataBody.eventId;
			PP_FotaInfoPush.expTime = MsgDataBody.expTime;
			PP_FotaInfoPush.req = 1;
			log_o(LOG_HOZON, "recv fota info push request\n");
			PP_FIP_Response(task);
		}
		break;
		default:
		break;
	}
}

/******************************************************
*��������PP_FIP_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_FIP_do_wait(PrvtProt_task_t *task)
{
	return 0;
}

/******************************************************
*PP_FIP_do_checkInfoPush

*��  �Σ�

*����ֵ��

*

*��  ע��
******************************************************/
static int PP_FIP_do_checkInfoPush(PrvtProt_task_t *task)
{
	int idlenode;
	return 0;
}

/******************************************************
*PP_FIP_Response

*��  �Σ�

*����ֵ��

*��  ����xcall response

*��  ע��
******************************************************/
static int PP_FIP_Response(PrvtProt_task_t *task)
{
	int msgdatalen;
	int res = 0;

	/* header */
	memcpy(PP_FotaInfoPush.packResp.Header.sign,"**",2);
	PP_FotaInfoPush.packResp.Header.commtype.Byte = 0xe1;
	PP_FotaInfoPush.packResp.Header.ver.Byte = 0x30;
	PP_FotaInfoPush.packResp.Header.opera = 0x02;
	PP_FotaInfoPush.packResp.Header.ver.Byte = task->version;
	PP_FotaInfoPush.packResp.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_FotaInfoPush.packResp.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_FIP_Pack, &PP_FotaInfoPush.packResp.Header, sizeof(PrvtProt_pack_Header_t));

	/* disbody */
	memcpy(PP_FotaInfoPush.packResp.DisBody.aID,"180",3);
	PP_FotaInfoPush.packResp.DisBody.mID = PP_MID_OTA_INFOPUSHRESP;
	PP_FotaInfoPush.packResp.DisBody.eventId = PP_FotaInfoPush.eventId;
	PP_FotaInfoPush.packResp.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_FotaInfoPush.packResp.DisBody.expTime   = PP_FotaInfoPush.expTime;
	PP_FotaInfoPush.packResp.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_FotaInfoPush.packResp.DisBody.appDataProVer = 256;
	PP_FotaInfoPush.packResp.DisBody.testFlag = 1;

	/*appdata*/
	Appdata_FIP.sid = 0;
	Appdata_FIP.noticeStatus = 1;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_FIP_RESP,PP_FIP_Pack.msgdata,&msgdatalen,\
									   &PP_FotaInfoPush.packResp.DisBody,&Appdata_FIP))
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_FIP_Pack.totallen = 18 + msgdatalen;
	PP_FIP_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	int idlenode;
	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_OTAINFOPUSH;
	PP_TxInform[idlenode].mid = PP_MID_OTA_INFOPUSHRESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "the result of fota info push inform to tsp";
	SP_data_write(PP_FIP_Pack.Header.sign,PP_FIP_Pack.totallen,PP_FIP_send_cb,&PP_TxInform[idlenode]);

	return res;
}

/******************************************************
*PP_FIP_send_cb

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static void PP_FIP_send_cb(void * para)
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
*PP_FIP_shellReq

*��  �Σ�

*����ֵ��

*

*��  ע��
******************************************************/
void PP_FIP_shellReq(void)
{
	PP_FotaInfoPush.req = 1;
}
