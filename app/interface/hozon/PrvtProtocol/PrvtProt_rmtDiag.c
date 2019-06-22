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
#include "PrvtProt_data.h"
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

//static PrvtProt_pack_t 			PP_rmtDiag_Pack;
static PrvtProt_rmtDiag_t		PP_rmtDiag;
static PP_App_rmtDiag_t 		AppData_rmtDiag;


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

static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,uint8_t diagType,long eventid);
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
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	switch(MsgDataBody.mID)
	{
		case PP_MID_DIAG_REQ://收到tsp请求
		{
			PP_rmtDiag.state.diagReq = 1;
			PP_rmtDiag.state.diagType = Appdata.DiagnosticReq.diagType;
			PP_rmtDiag.state.diageventId = MsgDataBody.eventId;
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
	if(1 == PP_rmtDiag.state.diagReq)//
	{
		res = PP_rmtDiag_DiagResponse(task,PP_rmtDiag.state.diagType,PP_rmtDiag.state.diageventId);
		if(res < 0)//请求发送失败
		{
			log_e(LOG_HOZON, "socket send error, reset protocol");
			PP_rmtDiag.state.diagReq = 0;
			sockproxy_socketclose();//by liujian 20190514
		}
		else if(res > 0)
		{
			log_i(LOG_HOZON, "socket send ok");
			PP_rmtDiag.state.diagReq = 0;
		}
		else
		{}
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
static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,uint8_t diagType,long eventid)
{
	int msgdatalen;
	int res = 0;
	int i;
	PrvtProt_pack_t pp_pack;

	memset(&pp_pack,0 , sizeof(PrvtProt_pack_t));
	/* header */
	memcpy(PP_rmtDiag.pack.Header.sign,"**",2);
	PP_rmtDiag.pack.Header.commtype.Byte = 0xe1;
	PP_rmtDiag.pack.Header.ver.Byte = 0x30;
	PP_rmtDiag.pack.Header.opera = 0x02;
	PP_rmtDiag.pack.Header.ver.Byte = task->version;
	PP_rmtDiag.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_rmtDiag.pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&pp_pack, &PP_rmtDiag.pack.Header, sizeof(PrvtProt_pack_Header_t));

	/* disbody */
	memcpy(PP_rmtDiag.pack.DisBody.aID,"140",3);
	PP_rmtDiag.pack.DisBody.mID = 2;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = eventid;
	PP_rmtDiag.pack.DisBody.expTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*appdata*/
	AppData_rmtDiag.DiagnosticResp.diagType = diagType;
	AppData_rmtDiag.DiagnosticResp.result = 1;
	AppData_rmtDiag.DiagnosticResp.failureType = 0;
	for(i =0;i < 2;i++)
	{
		memcpy(AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCode,"12345",5);
		AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCodelen = 5;
		AppData_rmtDiag.DiagnosticResp.diagCode[i].diagTime = 123456+i;
		AppData_rmtDiag.DiagnosticResp.diagcodenum++;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_RESP,pp_pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag))//数据编码打包是否完成
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	pp_pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(pp_pack.Header.sign,(18 + msgdatalen),NULL);

	protocol_dump(LOG_HOZON, "xcall_response", pp_pack.Header.sign,(18 + msgdatalen),1);
	return res;
}

/******************************************************
*函数名：PP_rmtDiag_send_cb

*形  参：

*返回值：

*描  述：remote diag status response

*备  注：
******************************************************/
static void PP_rmtDiag_send_cb(void * para)
{
	log_e(LOG_HOZON, " resp send ok");
}


#if 0
/******************************************************
*函数名：PP_xcall_SetEcallResp

*形  参：

*返回值：

*描  述：设置ecall response

*备  注：
******************************************************/
void PP_xcall_SetEcallResp(unsigned char resp)
{
	PP_xcall[PP_ECALL].state.resp = resp;
}
#endif

