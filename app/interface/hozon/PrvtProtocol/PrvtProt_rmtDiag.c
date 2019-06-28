/******************************************************
文件名：	PrvtProt_rmtDiag.c

描述：	企业私有协议（浙江合众）

Data			Vasion			author

2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************

description： include the header file

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
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "tbox_ivi_api.h"
#include "PrvtProt_rmtDiag.h"

/*******************************************************
description： global variable definitions
*******************************************************/


/*******************************************************
description： static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PrvtProt_rmtDiag_pack_t; /**/

typedef struct
{
	PrvtProt_rmtDiag_pack_t 	pack;
	PrvtProt_rmtDiagSt_t	 	state;
}__attribute__((packed))  PrvtProt_rmtDiag_t; /*结构体*/


static PrvtProt_pack_t 			PP_rmtDiag_Pack;
static PrvtProt_rmtDiag_t		PP_rmtDiag;
static PP_App_rmtDiag_t 		AppData_rmtDiag;

static PrvtProt_TxInform_t 		diag_TxInform[PP_RMTDIAG_MAX_RESP];
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/


/*Static function declaration*/
static int PP_rmtDiag_do_checksock(PrvtProt_task_t *task);
static int PP_rmtDiag_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtDiag_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtDiag_do_wait(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtDiag(PrvtProt_task_t *task);

static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
//static int PP_remotImageAcquisitionReq(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static void PP_rmtDiag_send_cb(void * para);
/******************************************************
description： function code
******************************************************/

/******************************************************
*函数名：PP_rmtDiag_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_rmtDiag_init(void)
{
	memset(&PP_rmtDiag,0 , sizeof(PrvtProt_rmtDiag_t));
	memset(&AppData_rmtDiag,0 , sizeof(PP_App_rmtDiag_t));
	PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
	PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
}



/******************************************************
*函数名：PP_rmtDiag_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_rmtDiag_mainfunction(void *task)
{
	int res;

	res = 		PP_rmtDiag_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtDiag_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_rmtDiag_do_wait((PrvtProt_task_t*)task) ||
				PP_rmtDiag_do_checkrmtDiag((PrvtProt_task_t*)task);

	return res;
}

/******************************************************
*函数名：PP_rmtDiag_do_checksock

*形  参：void

*返回值：void

*描  述：检查socket连接

*备  注：
******************************************************/
static int PP_rmtDiag_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{
		return 0;
	}

	return -1;
}



/******************************************************
*函数名：PP_rmtDiag_do_rcvMsg

*形  参：void

*返回值：void

*描  述：接收数据函数

*备  注：
******************************************************/
static int PP_rmtDiag_do_rcvMsg(PrvtProt_task_t *task)
{
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;

	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_REMOTE_DIAG,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}

	log_i(LOG_HOZON, "receive diag message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//判断数据帧头有误或者数据长度不对
	{
		return 0;
	}

	if(rlen > (18 + PP_MSG_DATA_LEN))//接收数据长度超出缓存buffer长度
	{
		return 0;
	}

	PP_rmtDiag_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}


/******************************************************
*函数名：PP_rmtDiag_RxMsgHandle

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PP_rmtDiag_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;

	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PP_App_rmtDiag_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_DIAG != aid)
	{
		log_e(LOG_HOZON, "aid unmatch\n");
		return;
	}

	switch(MsgDataBody.mID)
	{
		case PP_MID_DIAG_REQ://收到tsp请求
		{
			if((0 == PP_rmtDiag.state.diagReq) && (PP_DIAGRESP_IDLE == PP_rmtDiag.state.diagrespSt))
			{
				log_i(LOG_HOZON, "receive remote diag request\n");
				PP_rmtDiag.state.diagReq = 1;
				PP_rmtDiag.state.diagType = Appdata.DiagnosticReq.diagType;
				PP_rmtDiag.state.diageventId = MsgDataBody.eventId;
			}
			else
			{
				log_e(LOG_HOZON, "repeat diag request\n");
			}
		}
		break;
		case PP_MID_DIAG_IMAGEACQREQ://
		{
			if(PP_IMAGEACQRESP_IDLE == PP_rmtDiag.state.ImageAcqRespSt)
			{
				log_i(LOG_HOZON, "receive remote ImageAcquisition request\n");
				PP_rmtDiag.state.ImageAcquisitionReq = 1;
				PP_rmtDiag.state.dataType    = Appdata.ImageAcquisitionReq.dataType;
				PP_rmtDiag.state.cameraName  =  Appdata.ImageAcquisitionReq.cameraName;
				PP_rmtDiag.state.effectiveTime = Appdata.ImageAcquisitionReq.effectiveTime;
				PP_rmtDiag.state.sizeLimit   =  Appdata.ImageAcquisitionReq.sizeLimit;
				PP_rmtDiag.state.diageventId = MsgDataBody.eventId;
			}
			else
			{
				log_e(LOG_HOZON, "repeat ImageAcq request\n");
			}
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_rmtDiag_do_wait

*形  参：void

*返回值：void

*描  述：检查是否有事件等待应答

*备  注：
******************************************************/
static int PP_rmtDiag_do_wait(PrvtProt_task_t *task)
{
	return 0;
}


/******************************************************
*函数名：PP_rmtDiag_do_checkrmtDiag

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
static int PP_rmtDiag_do_checkrmtDiag(PrvtProt_task_t *task)
{
	int i;
	int res;

	switch(PP_rmtDiag.state.diagrespSt)
	{
		case PP_DIAGRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.diagReq)//远程诊断请求
			{
				log_i(LOG_HOZON, "start remote diag\n");
				PP_rmtDiag.state.diagReq = 0;
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_PENDING;
			}
		}
		break;
		case PP_DIAGRESP_PENDING:
		{
			if(0 == PP_rmtDiag_DiagResponse(task,&PP_rmtDiag))
			{
				memset(&diag_TxInform[PP_RMTDIAG_RESP_REQ],0,sizeof(PrvtProt_TxInform_t));
				diag_TxInform[PP_RMTDIAG_RESP_REQ].aid = PP_AID_DIAG;
				diag_TxInform[PP_RMTDIAG_RESP_REQ].mid = PP_MID_DIAG_RESP;
				diag_TxInform[PP_RMTDIAG_RESP_REQ].pakgtype = PP_TXPAKG_SIGTIME;
				diag_TxInform[PP_RMTDIAG_RESP_REQ].eventtime = tm_get_time();
				SP_data_write(PP_rmtDiag_Pack.Header.sign, \
						PP_rmtDiag_Pack.totallen,PP_rmtDiag_send_cb,&diag_TxInform[PP_RMTDIAG_RESP_REQ]);
				protocol_dump(LOG_HOZON, "diag_req_response", PP_rmtDiag_Pack.Header.sign,PP_rmtDiag_Pack.totallen,1);
			}
			PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_END;
		}
		break;
		case PP_DIAGRESP_END:
		{
			PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
		}
		break;
		default:
		break;
	}


	switch(PP_rmtDiag.state.ImageAcqRespSt)
	{
		case PP_IMAGEACQRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.ImageAcquisitionReq)
			{
				log_i(LOG_HOZON, "start remote ImageAcquisition\n");
				PP_rmtDiag.state.ImageAcquisitionReq = 0;
				PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_INFORM_HU;
			}
		}
		break;
		case PP_IMAGEACQRESP_INFORM_HU://通知HU
		{
			ivi_remotediagnos tspInformHU;
			tspInformHU.aid = PP_AID_DIAG;
			tspInformHU.mid = PP_MID_DIAG_IMAGEACQREQ;
			tspInformHU.eventid = PP_rmtDiag.state.diageventId;
			tspInformHU.datatype = PP_rmtDiag.state.dataType;
			tspInformHU.cameraname = PP_rmtDiag.state.cameraName;
			tspInformHU.effectivetime = PP_rmtDiag.state.effectiveTime;
			tspInformHU.sizelimit = PP_rmtDiag.state.sizeLimit;
			tbox_ivi_set_tspInformHU(&tspInformHU);
			PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}


/******************************************************
*函数名：PP_rmtDiag_DiagResponse

*形  参：

*返回值：

*描  述：diag response

*备  注：
******************************************************/
static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	int res = 0;
	int i;

	memset(&PP_rmtDiag_Pack,0 , sizeof(PrvtProt_pack_t));
	/* header */
	memcpy(PP_rmtDiag.pack.Header.sign,"**",2);
	PP_rmtDiag.pack.Header.commtype.Byte = 0xe1;
	PP_rmtDiag.pack.Header.ver.Byte = 0x30;
	PP_rmtDiag.pack.Header.opera = 0x02;
	PP_rmtDiag.pack.Header.ver.Byte = task->version;
	PP_rmtDiag.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtDiag.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtDiag_Pack, &PP_rmtDiag.pack.Header, sizeof(PrvtProt_pack_Header_t));

	/* disbody */
	memcpy(PP_rmtDiag.pack.DisBody.aID,"140",3);
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_RESP;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = rmtDiag->state.diageventId;
	PP_rmtDiag.pack.DisBody.expTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	memset(&AppData_rmtDiag.DiagnosticResp,0,sizeof(PP_DiagnosticResp_t));
	/*appdata*/
	switch(rmtDiag->state.diagType)
	{
		case PP_DIAG_TBOX:
		{
			log_i(LOG_HOZON, "diag tbox\n");
			AppData_rmtDiag.DiagnosticResp.diagType = rmtDiag->state.diagType;
			AppData_rmtDiag.DiagnosticResp.result = rmtDiag->state.result;
			AppData_rmtDiag.DiagnosticResp.failureType = rmtDiag->state.failureType;
			for(i =0;i < 2;i++)
			{
				memcpy(AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCode,"12345",5);
				AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCodelen = 5;
				AppData_rmtDiag.DiagnosticResp.diagCode[i].diagTime = PrvtPro_getTimestamp();
				AppData_rmtDiag.DiagnosticResp.diagcodenum++;
			}
		}
		break;
		case PP_DIAG_HU:
		{

		}
		break;
		case PP_DIAG_ICU:
		{

		}
		break;
		default:
		{
			AppData_rmtDiag.DiagnosticResp.diagType = rmtDiag->state.diagType;
			AppData_rmtDiag.DiagnosticResp.result = 0;
			AppData_rmtDiag.DiagnosticResp.diagcodenum = 0;
		}
		break;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_RESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.DiagnosticResp))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return res;
}

#if 0
/******************************************************
*函数名：

*形  参：

*返回值：

*描  述：远程诊断( MID=4)

*备  注：
******************************************************/
static int PP_remotImageAcquisitionReq(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	int res = 0;
	int i;

	memset(&PP_rmtDiag_Pack,0 , sizeof(PrvtProt_pack_t));
	/* header */
	memcpy(PP_rmtDiag.pack.Header.sign,"**",2);
	PP_rmtDiag.pack.Header.commtype.Byte = 0xe1;
	PP_rmtDiag.pack.Header.ver.Byte = 0x30;
	PP_rmtDiag.pack.Header.opera = 0x02;
	PP_rmtDiag.pack.Header.ver.Byte = task->version;
	PP_rmtDiag.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtDiag.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtDiag_Pack, &PP_rmtDiag.pack.Header, sizeof(PrvtProt_pack_Header_t));

	/* disbody */
	memcpy(PP_rmtDiag.pack.DisBody.aID,"140",3);
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_RESP;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = rmtDiag->state.diageventId;
	PP_rmtDiag.pack.DisBody.expTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;


	/*app data*/
	AppData_rmtDiag.ImageAcquisitionReq.dataType = 2;//
	AppData_rmtDiag.ImageAcquisitionReq.cameraName = 1;//
	AppData_rmtDiag.ImageAcquisitionReq.effectiveTime =123456;//
	AppData_rmtDiag.ImageAcquisitionReq.sizeLimit = 100;//

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_RESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return res;
}
#endif

/******************************************************
*函数名：PP_rmtDiag_send_cb

*形  参：

*返回值：

*描  述：remote diag status response

*备  注：
******************************************************/
static void PP_rmtDiag_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr =  (PrvtProt_TxInform_t*)para;

	switch(TxInform_ptr->mid)
	{
		case PP_MID_DIAG_RESP:
		{
			//PP_rmtDiag.state.diagReq = 0;
			log_i(LOG_HOZON, "send remote diag response ok\n");
		}
		break;
		default:
		break;
	}
}

/******************************************************
*函数名：PP_diag_SetdiagReq

*形  参：

*返回值：

*描  述：设置请求

*备  注：
******************************************************/
void PP_diag_SetdiagReq(unsigned char diagType)
{
	log_i(LOG_HOZON, "receive remote diag request\n");
	PP_rmtDiag.state.diagReq = 1;
	PP_rmtDiag.state.diagType = diagType;
	PP_rmtDiag.state.diageventId = 100;
	PP_rmtDiag.state.result = 1;
	PP_rmtDiag.state.failureType = 0;
}


