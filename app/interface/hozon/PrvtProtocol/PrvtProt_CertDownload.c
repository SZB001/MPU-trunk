/******************************************************
�ļ�����	PrvtProt_CertDownload.c

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
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "PrvtProt_CertDownload.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
}__attribute__((packed))  PP_CertDL_pack_t; /**/

typedef struct
{
	PP_CertDL_pack_t 			pack;
	PP_CertDownloadSt_t	 		state;
	PP_CertificateDownload_t	para;
}__attribute__((packed))  PP_CertDL_t;

static PP_CertDL_t 				PP_CertDL;
static PP_CertDownloadPara_t	PP_CertDownloadPara;//需要掉电保存的参数

static PrvtProt_TxInform_t CertDL_TxInform;
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_CertDL_do_checksock(PrvtProt_task_t *task);
static int PP_CertDL_do_rcvMsg(PrvtProt_task_t *task);
static void PP_CertDL_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_CertDL_do_wait(PrvtProt_task_t *task);
static int PP_CertDL_do_checkCertificate(PrvtProt_task_t *task);

static void PP_CertDL_send_cb(void * para);
static int PP_CertDL_CertDLReq(PrvtProt_task_t *task,PP_CertificateDownload_t *CertificateDownload);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_CertDownload_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_CertDownload_init(void)
{
	memset(&PP_CertDL,0 , sizeof(PP_CertDL_t));
	memcpy(PP_CertDL.pack.Header.sign,"**",2);
	PP_CertDL.pack.Header.commtype.Byte = 0xe1;
	PP_CertDL.pack.Header.ver.Byte = 0x30;
	PP_CertDL.pack.Header.opera = 0x05;
	PP_CertDL.state.dlSt = PP_CERTDL_IDLE;

	PP_CertDownloadPara.eventid = 1;

	if(1)//检查证书有效性
	{
		PP_CertDL.state.CertValid = 1;
	}
}

/******************************************************
*��������PP_CertDownload_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_CertDownload_mainfunction(void *task)
{
	int res;
	PrvtProt_task_t *task_ptr = (PrvtProt_task_t*)task;

	res = 		PP_CertDL_do_checksock(task_ptr) ||
				PP_CertDL_do_rcvMsg(task_ptr) ||
				PP_CertDL_do_wait(task_ptr) ||
				PP_CertDL_do_checkCertificate(task_ptr);

	return res;
}

/******************************************************
*��������PP_CertDL_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_CertDL_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}
	return -1;
}

/******************************************************
*��������PP_CertDL_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_CertDL_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_CERT_DL,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive cert download message\n");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//
	{
		return 0;
	}
	PP_CertDL_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_CertDL_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_CertDL_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{

	if(PP_OPERATETYPE_CERTDL != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	if(rxPack->msgdata[0] == PP_CERTDL_MID_RESP)//mid == 2,cert download response
	{
		PP_CertDownloadPara.eventid = 0;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID] << 24;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+1] << 16;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+2] << 8;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+3];


		if((rxPack->msgdata[PP_CERTDL_RESP_RESULT] == 1) && \
				(rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE] == 1))//成功
		{
			PP_CertDL.state.dlsuccess = PP_CERTDL_SUCCESS;
			PP_CertDL.para.CertDLResp.certLength = 0;
			PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 24;
			PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 16;
			PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 8;
			PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN];

			log_i(LOG_HOZON, "PP_CertDL.para.CertDLResp.certLength = %d\n",PP_CertDL.para.CertDLResp.certLength);
			log_i(LOG_HOZON, "certContent = %s\n",rxPack->msgdata[PP_CERTDL_RESP_CERTCONTENT]);


			//保存证书内容

			if(1)//检查证书有效性
			{
				PP_CertDL.state.CertValid = 1;
			}
		}
		else
		{
			PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
		}
	}
}

/******************************************************
*��������PP_CertDL_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_CertDL_do_wait(PrvtProt_task_t *task)
{

	return 0;
}

/******************************************************
*��������PP_CertDL_do_checkCertificate

*��  �Σ�

*����ֵ��

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_checkCertificate(PrvtProt_task_t *task)
{
	switch(PP_CertDL.state.dlSt)
	{
		case PP_CERTDL_IDLE:
		{
			if(1 != PP_CertDL.state.CertValid)//
			{
				PP_CertDL.state.dlSt = PP_CERTDL_DLREQ;
			}
		}
		break;
		case PP_CERTDL_DLREQ:
		{
			PP_CertDL.para.CertDLReq.mid = PP_CERTDL_MID_REQ;
			PP_CertDL.para.CertDLReq.eventid = PP_CertDownloadPara.eventid;
			PP_CertDL.para.CertDLReq.cerType = 1;//tbox
			//生成消息列表,VIN &&密文信息 &&密钥编号&& 证书申请文件内容 CSR，其中密文信息中包含 TBOXSN、IMISI、随机数信息

			PP_CertDL_CertDLReq(task,&PP_CertDL.para);
			PP_CertDL.state.dlsuccess = PP_CERTDL_INITVAL;
			PP_CertDL.state.waittime = tm_get_time();
			PP_CertDL.state.dlSt = PP_CERTDL_DLREQWAIT;
		}
		break;
		case PP_CERTDL_DLREQWAIT:
		{
			if((tm_get_time() - PP_CertDL.state.waittime) <= PP_CERTDL_DLTIMEOUT)
			{
				if(PP_CERTDL_INITVAL != PP_CertDL.state.dlsuccess)
				{
					PP_CertDL.state.dlSt = PP_CERTDL_END;
				}
			}
			else//timeout
			{
				PP_CertDL.state.dlSt = PP_CERTDL_END;
			}
		}
		break;
		case PP_CERTDL_END:
		{
			PP_CertDL.state.dlSt = PP_CERTDL_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static int PP_CertDL_CertDLReq(PrvtProt_task_t *task,PP_CertificateDownload_t *CertificateDownload)
{
	int i;
	PrvtProt_pack_t	PP_CertDL_pack;

	/* header */
	memcpy(PP_CertDL.pack.Header.sign,"**",2);
	PP_CertDL.pack.Header.commtype.Byte = 0xe1;
	PP_CertDL.pack.Header.ver.Byte = 0x30;
	PP_CertDL.pack.Header.opera = 0x05;
	PP_CertDL.pack.Header.ver.Byte = task->version;
	PP_CertDL.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_CertDL.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_CertDL_pack, &PP_CertDL.pack.Header, sizeof(PrvtProt_pack_Header_t));

	/* message data */
	PP_CertDL_pack.msgdata[0] = CertificateDownload->CertDLReq.mid;
	PP_CertDL_pack.msgdata[1] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 24);
	PP_CertDL_pack.msgdata[2] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 16);
	PP_CertDL_pack.msgdata[3] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 8);
	PP_CertDL_pack.msgdata[4] = (uint8_t)CertificateDownload->CertDLReq.eventid;
	PP_CertDL_pack.msgdata[5] = CertificateDownload->CertDLReq.cerType;//证书类型：1- tbox
	PP_CertDL_pack.msgdata[6] = (uint8_t)(CertificateDownload->CertDLReq.infoListLength >> 8);
	PP_CertDL_pack.msgdata[7] = (uint8_t)CertificateDownload->CertDLReq.infoListLength;
	for(i = 0;i < CertificateDownload->CertDLReq.infoListLength;i++)
	{
		PP_CertDL_pack.msgdata[8+i] = CertificateDownload->CertDLReq.infoList[i];
	}

	log_i(LOG_HOZON, "cert infoList Length = %d\n",CertificateDownload->CertDLReq.infoListLength);
	log_i(LOG_HOZON, "cert infoList = %s\n",CertificateDownload->CertDLReq.infoList);

	PP_CertDL_pack.totallen = 18 + 8 + CertificateDownload->CertDLReq.infoListLength;
	PP_CertDL_pack.Header.msglen = PrvtPro_BSEndianReverse((long)PP_CertDL_pack.totallen);

	memset(&CertDL_TxInform,0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform.mid = CertificateDownload->CertDLReq.mid;
	CertDL_TxInform.pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform);
	protocol_dump(LOG_HOZON, "CertDownload_request", PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,1);

	return 1;
}


/******************************************************
*��������PP_CertDL_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static void PP_CertDL_send_cb(void * para)
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

/******************************************************
*��������PP_CertDL_SetCertDLReq

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
void PP_CertDL_SetCertDLReq(unsigned char req)
{

}

/******************************************************
*��������GetPP_CertDL_CertValid

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
unsigned char GetPP_CertDL_CertValid(void)
{
	return PP_CertDL.state.CertValid;
}

