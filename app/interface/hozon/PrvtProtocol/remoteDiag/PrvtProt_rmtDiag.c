/******************************************************
鏂囦欢鍚嶏細	PrvtProt_rmtDiag.c

鎻忚堪锛�	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級

Data			Vasion			author

2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************

description锛� include the header file

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
#include "../../sockproxy/sockproxy_txdata.h"
#include "../../../support/protocol.h"
#include "cfg_api.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt_cfg.h"
#include "../PrvtProt.h"
#include "tbox_ivi_api.h"
#include "PrvtProt_rmtDiag.h"

/*******************************************************
description锛� global variable definitions
*******************************************************/


/*******************************************************
description锛� static variable definitions
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
}__attribute__((packed))  PrvtProt_rmtDiag_t; /*缁撴瀯浣�*/


static PrvtProt_pack_t 			PP_rmtDiag_Pack;
static PrvtProt_rmtDiag_t		PP_rmtDiag;
static PP_App_rmtDiag_t 		AppData_rmtDiag;

static PrvtProt_TxInform_t 		diag_TxInform[PP_RMTDIAG_MAX_RESP];
static PP_rmtDiag_datetime_t 	rmtDiag_datetime;
static PP_rmtDiag_weekmask_t rmtDiag_weekmask[7] =
{
	{0,0x80},//星期天
	{1,0x40},//星期1
	{2,0x20},//星期2
	{3,0x10},//星期3
	{4,0x08},//星期4
	{5,0x04},//星期5
	{6,0x02},//星期6
};
/*******************************************************
description锛� function declaration
*******************************************************/
/*Global function declaration*/


/*Static function declaration*/
static int PP_rmtDiag_do_checksock(PrvtProt_task_t *task);
static int PP_rmtDiag_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtDiag_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtDiag_do_wait(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtDiag(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtImageReq(PrvtProt_task_t *task);

static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static int PP_remotDiagnosticStatus(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static int PP_rmtDiag_do_DiagActiveReport(PrvtProt_task_t *task);
//static int PP_remotImageAcquisitionReq(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static void PP_rmtDiag_send_cb(void * para);
/******************************************************
description锛� function code
******************************************************/

/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_init

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細鍒濆鍖�

*澶�  娉細
******************************************************/
void PP_rmtDiag_init(void)
{
	unsigned int cfglen;
	memset(&PP_rmtDiag,0 , sizeof(PrvtProt_rmtDiag_t));
	memset(&AppData_rmtDiag,0 , sizeof(PP_App_rmtDiag_t));
	PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
	PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
	PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_PWRON;

	cfglen = 4;
	cfg_get_para(CFG_ITEM_HOZON_TSP_DIAGDATE,&rmtDiag_datetime.datetime,&cfglen);//读取诊断日期标志
	cfglen = 1;
	cfg_get_para(CFG_ITEM_HOZON_TSP_DIAGFLAG,&rmtDiag_datetime.diagflag,&cfglen);//读取诊断日期标志
}



/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_mainfunction

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細涓讳换鍔″嚱鏁�

*澶�  娉細
******************************************************/
int PP_rmtDiag_mainfunction(void *task)
{
	int res;

	res = 		PP_rmtDiag_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtDiag_do_rcvMsg((PrvtProt_task_t*)task) 	||
				PP_rmtDiag_do_wait((PrvtProt_task_t*)task) 		||
				PP_rmtDiag_do_checkrmtDiag((PrvtProt_task_t*)task) ||
				PP_rmtDiag_do_checkrmtImageReq((PrvtProt_task_t*)task);

	PP_rmtDiag_do_DiagActiveReport((PrvtProt_task_t*)task);//主动诊断上报

	return res;
}

/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_do_checksock

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細妫�鏌ocket杩炴帴

*澶�  娉細
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
*鍑芥暟鍚嶏細PP_rmtDiag_do_rcvMsg

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細鎺ユ敹鏁版嵁鍑芥暟

*澶�  娉細
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
			(rlen <= 18))//鍒ゆ柇鏁版嵁甯уご鏈夎鎴栬�呮暟鎹暱搴︿笉瀵�
	{
		return 0;
	}

	if(rlen > (18 + PP_MSG_DATA_LEN))//鎺ユ敹鏁版嵁闀垮害瓒呭嚭缂撳瓨buffer闀垮害
	{
		return 0;
	}

	PP_rmtDiag_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}


/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_RxMsgHandle

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細鎺ユ敹鏁版嵁澶勭悊

*澶�  娉細
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
		case PP_MID_DIAG_REQ://鏀跺埌tsp璇锋眰
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
*鍑芥暟鍚嶏細PP_rmtDiag_do_wait

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細妫�鏌ユ槸鍚︽湁浜嬩欢绛夊緟搴旂瓟

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_wait(PrvtProt_task_t *task)
{
	return 0;
}


/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_do_checkrmtDiag

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_checkrmtDiag(PrvtProt_task_t *task)
{
	switch(PP_rmtDiag.state.diagrespSt)
	{
		case PP_DIAGRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.diagReq)//杩滅▼璇婃柇璇锋眰
			{
				log_i(LOG_HOZON, "start remote diag\n");
				PP_rmtDiag.state.diagReq = 0;
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_ONGOING;
			}
		}
		break;
		case PP_DIAGRESP_ONGOING:
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

	return 0;
}

/******************************************************
*鍑芥暟鍚嶏細:PP_rmtDiag_do_checkrmtImageReq

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_checkrmtImageReq(PrvtProt_task_t *task)
{
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
		case PP_IMAGEACQRESP_INFORM_HU://閫氱煡HU
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
*鍑芥暟鍚嶏細:PP_rmtDiag_do_DiagActiveReport

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_DiagActiveReport(PrvtProt_task_t *task)
{
	static uint8_t tm_wday;
	static uint32_t tm_datetime;
	switch(PP_rmtDiag.state.activeDiagSt)
	{
		case PP_ACTIVEDIAG_PWRON:
		{
			PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_CHECK;
			PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
		}
		break;
		case PP_ACTIVEDIAG_CHECK:
		{
			char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
			time_t timep;
			struct tm *localdatetime;

			time(&timep);
			localdatetime = localtime(&timep);//取得当地时间
			log_i(LOG_HOZON,"%d-%d-%d ",(1900+localdatetime->tm_year), \
					(1 +localdatetime->tm_mon), localdatetime->tm_mday);
			log_i(LOG_HOZON,"%s %d:%d:%d\n", wday[localdatetime->tm_wday], \
					localdatetime->tm_hour, localdatetime->tm_min, localdatetime->tm_sec);
			tm_wday = localdatetime->tm_wday;
			tm_datetime = (1900+localdatetime->tm_year) * 10000 + (1 +localdatetime->tm_mon) * 100 + localdatetime->tm_mday;
			if((rmtDiag_datetime.diagflag & rmtDiag_weekmask[tm_wday].mask) && \
					(rmtDiag_datetime.datetime == tm_datetime))
			{//已上传过故障码
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_END;
				log_i(LOG_HOZON,"The fault code has been uploaded today\n");
				log_i(LOG_HOZON,"uploaded date : %d\n",rmtDiag_datetime.datetime);
			}
			else
			{
				log_i(LOG_HOZON,"start to daig report\n");
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_DIAGONGOING;
			}
		}
		break;
		case PP_ACTIVEDIAG_DIAGONGOING:
		{
			if((tm_get_time() - PP_rmtDiag.state.activeDiagdelaytime) >= PP_ACTIVEDIAG_WAITTIME)//延时5s
			{
				if(gb_data_vehicleSpeed() <= 50)//判断车速<=5km/h,满足诊断条件
				{
					log_i(LOG_HOZON,"vehicle speed <= 5km/h,start diag\n");
					PP_remotDiagnosticStatus(task,&PP_rmtDiag);

					rmtDiag_datetime.diagflag = rmtDiag_weekmask[tm_wday].mask;
					if(cfg_set_para(CFG_ITEM_HOZON_TSP_DIAGFLAG, &rmtDiag_datetime.diagflag, 1))
					{
						log_e(LOG_GB32960, "save rmtDiag_datetime.diagflag failed\n");
					}

					rmtDiag_datetime.datetime = tm_datetime;
					if(cfg_set_para(CFG_ITEM_HOZON_TSP_DIAGDATE, &rmtDiag_datetime.datetime, 4))
					{
						log_e(LOG_GB32960, "save rmtDiag_datetime.datetime failed\n");
					}
				}
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_END;
				log_i(LOG_HOZON,"exit diag\n");
			}
		}
		break;
		case PP_ACTIVEDIAG_END:
		{

		}
		break;
		default:
		break;
	}

	return 0;
}
/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_DiagResponse

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細diag response

*澶�  娉細
******************************************************/
static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
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
		case PP_DIAG_VCU:
		{
			log_i(LOG_HOZON, "diag tbox\n");
			AppData_rmtDiag.DiagnosticResp.diagType = rmtDiag->state.diagType;
			AppData_rmtDiag.DiagnosticResp.result = rmtDiag->state.result;
			AppData_rmtDiag.DiagnosticResp.failureType = rmtDiag->state.failureType;
			for(i =0;i < PP_DIAG_MAX_REPORT;i++)
			{
				memcpy(AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCode,"12345",5);
				AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCodelen = 5;
				AppData_rmtDiag.DiagnosticResp.diagCode[i].diagTime = PrvtPro_getTimestamp();
				AppData_rmtDiag.DiagnosticResp.diagcodenum++;
			}
		}
		break;
		case PP_DIAG_BMS:
		{

		}
		break;
		case PP_DIAG_MCUp:
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
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.DiagnosticResp))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return 0;
}

/******************************************************
*函数名：

*形  参：

*返回值：

*描  述：远程诊断( MID=3)

*备  注：
******************************************************/
static int PP_remotDiagnosticStatus(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	int i,j;

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
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_STATUS;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = PP_AID_DIAG + PP_MID_DIAG_STATUS;//rmtDiag->state.diageventId;
	PP_rmtDiag.pack.DisBody.expTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;
	/*app data*/
	for(i=0;i<(PP_DIAG_MAXECU -1);i++)
	{
		AppData_rmtDiag.DiagnosticSt.diagStatus[i].diagType = i+1;//
		AppData_rmtDiag.DiagnosticSt.diagStatus[i].result = 1;
		AppData_rmtDiag.DiagnosticSt.diagStatus[i].failureType = 0;
		for(j=0;j<2;j++)
		{
			memcpy(AppData_rmtDiag.DiagnosticSt.diagStatus[i].diagCode[j].diagCode,"12345",5);
			AppData_rmtDiag.DiagnosticSt.diagStatus[i].diagCode[j].diagCodelen = 5;
			AppData_rmtDiag.DiagnosticSt.diagStatus[i].diagCode[j].diagTime = PrvtPro_getTimestamp();
			AppData_rmtDiag.DiagnosticSt.diagStatus[i].diagcodenum++;
		}
		AppData_rmtDiag.DiagnosticSt.diagobjnum++;
	}


	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_STATUS,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.DiagnosticSt))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return 0;
}



#if 0
/******************************************************
*鍑芥暟鍚嶏細

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細杩滅▼璇婃柇( MID=4)

*澶�  娉細
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
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
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
*鍑芥暟鍚嶏細PP_rmtDiag_send_cb

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細remote diag status response

*澶�  娉細
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
*鍑芥暟鍚嶏細PP_diag_SetdiagReq

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細璁剧疆璇锋眰

*澶�  娉細
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


