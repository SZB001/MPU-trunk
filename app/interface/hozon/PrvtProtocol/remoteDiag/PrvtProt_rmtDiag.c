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
#include "dev_api.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../../sockproxy/sockproxy_txdata.h"
#include "../../../support/protocol.h"
#include "cfg_api.h"
#include "gb32960_api.h"
#include "hozon_PP_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt_cfg.h"
#include "../PrvtProt.h"
#include "tbox_ivi_api.h"
#include "uds.h"
#include "PP_rmtDiag_cfg.h"
#include "../PrvtProt_lock.h"
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
//static PrvtProt_pack_t 			PP_rmtDiag_Pack;
static PrvtProt_rmtDiag_t		PP_rmtDiag;
static PP_App_rmtDiag_t 		AppData_rmtDiag;

static PP_rmtDiag_Fault_t		PP_rmtDiag_Fault;
typedef struct
{
	PP_rmtDiag_Fault_t	 		code[PP_DIAG_MAXECU];
	uint8_t 					currdiagtype;//当前诊断类型
	uint8_t 					rptfaultcnt[PP_DIAG_MAXECU];//当前诊断的ecu上报的故障码计数
	uint8_t						totalfaultCnt;
}__attribute__((packed))  PP_rmtDiag_allFault_t;
static PP_rmtDiag_allFault_t	PP_rmtDiag_allFault;

//static PrvtProt_TxInform_t 		diag_TxInform[PP_RMTDIAG_MAX_RESP];
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
extern void pm_ring_wakeup(void);
extern void PP_can_mcu_awaken(void);
extern void PP_can_mcu_sleep(void);

/*Static function declaration*/
static int PP_rmtDiag_do_checksock(PrvtProt_task_t *task);
static int PP_rmtDiag_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtDiag_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_rmtDiag_do_wait(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtDiag(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtImageReq(PrvtProt_task_t *task);
static int PP_rmtDiag_do_checkrmtLogReq(PrvtProt_task_t *task);
static int PP_rmtDiag_do_stoprmtLogReq(PrvtProt_task_t *task);

static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag,PP_rmtDiag_Fault_t *rmtDiag_Fault);
static int PP_remotDiagnosticStatus(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static int PP_rmtDiag_do_DiagActiveReport(PrvtProt_task_t *task);
//static int PP_remotImageAcquisitionReq(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static void PP_rmtDiag_send_cb(void * para);
static int PP_rmtDiag_do_FaultCodeClean(PrvtProt_task_t *task);
static int PP_rmtDiag_FaultCodeCleanResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static void getPPrmtDiag_tboxFaultcode(PP_rmtDiag_Fault_t *rmtDiag_Fault);
static int PP_rmtDiag_LogAcqReqResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
static int PP_rmtDiag_StopLogAcqResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag);
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
	memset(&PP_rmtDiag_Fault,0 , sizeof(PP_rmtDiag_Fault_t));

	PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
	PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
	PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_IDLE;
	PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_IDLE;
	PP_rmtDiag.state.sleepflag = 1;

	cfglen = 4;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_DIAGDATE,&rmtDiag_datetime.datetime,&cfglen);//读取诊断日期标志
	cfglen = 1;
	cfg_get_user_para(CFG_ITEM_HOZON_TSP_DIAGFLAG,&rmtDiag_datetime.diagflag,&cfglen);//读取诊断日期标志
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

	static char IGNnewSt,IGNoldSt = 0;

	IGNnewSt = dev_get_KL15_signal();
	if(IGNoldSt != IGNnewSt)
	{
		IGNoldSt = IGNnewSt;
		if(1 == IGNnewSt)//IGN ON
		{
			PP_rmtDiag.state.activeDiagFlag = 1;
		}
		else
		{
			PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
			PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
			PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_IDLE;
			PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_IDLE;
			PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_IDLE;
			PP_rmtDiag.state.diagReq = 0;
			PP_rmtDiag.state.ImageAcquisitionReq = 0;
			PP_rmtDiag.state.LogAcqReq = 0;
		}
	}

	res = 		PP_rmtDiag_do_checksock((PrvtProt_task_t*)task) 		||	\
				PP_rmtDiag_do_rcvMsg((PrvtProt_task_t*)task) 			||	\
				PP_rmtDiag_do_wait((PrvtProt_task_t*)task) 				||	\
				PP_rmtDiag_do_checkrmtDiag((PrvtProt_task_t*)task) 		||	\
				PP_rmtDiag_do_checkrmtImageReq((PrvtProt_task_t*)task) 	||	\
				PP_rmtDiag_do_checkrmtLogReq((PrvtProt_task_t*)task)   	||	\
				PP_rmtDiag_do_stoprmtLogReq((PrvtProt_task_t*)task)   	||	\
				PP_rmtDiag_do_FaultCodeClean((PrvtProt_task_t*)task);

	if(1 == sockproxy_socketState())//socket open
	{
		PP_rmtDiag_do_DiagActiveReport((PrvtProt_task_t*)task);//主动诊断上报
	}

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

	log_o(LOG_HOZON, "receive diag message");
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

	if(PP_OPERATETYPE_NGTP != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PP_App_rmtDiag_t Appdata;
	memset(&Appdata,0 , sizeof(PP_App_rmtDiag_t));
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
		case PP_MID_DIAG_REQ://
		{
			if((0 == PP_rmtDiag.state.diagReq) && (PP_DIAGRESP_IDLE == PP_rmtDiag.state.diagrespSt))
			{
				log_o(LOG_HOZON, "receive remote diag request\n");
				PP_rmtDiag.state.diagReq 	 = 1;
				PP_rmtDiag.state.diagType 	 = Appdata.DiagnosticReq.diagType;
				PP_rmtDiag.state.diageventId = MsgDataBody.eventId;
				PP_rmtDiag.state.diagexpTime = MsgDataBody.expTime;
				PP_rmtDiag.state.waittime = tm_get_time();
				PP_rmtDiag.state.sleepflag   = 0;
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
				log_o(LOG_HOZON, "receive remote ImageAcquisition request\n");
				PP_rmtDiag.state.ImageAcquisitionReq = 1;
				PP_rmtDiag.state.dataType    = Appdata.ImageAcquisitionReq.dataType;
				PP_rmtDiag.state.ImagedurationTime  =  Appdata.ImageAcquisitionReq.durationTime;
				PP_rmtDiag.state.cameraName = Appdata.ImageAcquisitionReq.cameraName;
				//PP_rmtDiag.state.sizeLimit   =  Appdata.ImageAcquisitionReq.sizeLimit;
				PP_rmtDiag.state.imagereqeventId = MsgDataBody.eventId;
			}
			else
			{
				log_e(LOG_HOZON, "repeat ImageAcq request\n");
			}
		}
		break;
		case PP_MID_DIAG_LOGACQREQ://tsp请求开始采集log
		{
			if(PP_LOGACQRESP_IDLE == PP_rmtDiag.state.LogAcqRespSt)
			{
				int i;
				log_o(LOG_HOZON, "receive remote LogAcquisition request\n");
				PP_rmtDiag.state.LogAcqReq  = 1;
				PP_rmtDiag.state.logeventId =  MsgDataBody.eventId;
				PP_rmtDiag.state.logexpTime = MsgDataBody.expTime;
				for(i = 0;i < PP_ECULOG_MAX;i++)
				{
					PP_rmtDiag.state.ecuLog[i].ecuType    	=  Appdata.LogAcquisitionReq[i].ecuType;
					PP_rmtDiag.state.ecuLog[i].logLevel  	=  Appdata.LogAcquisitionReq[i].logLevel;
					PP_rmtDiag.state.ecuLog[i].startTime 	=  Appdata.LogAcquisitionReq[i].startTime;
					PP_rmtDiag.state.ecuLog[i].durationTime =  Appdata.LogAcquisitionReq[i].durationTime;
				}
			}
			else
			{
				log_e(LOG_HOZON, "logAcq request is busy\n");
			}
		}
		break;
		case PP_MID_DIAG_STOPLOGREQ://tsp请求停止采集 log
		{
			log_o(LOG_HOZON, "receive stop log collect  request\n");
			if(PP_STOPLOG_IDLE == PP_rmtDiag.state.StopLogAcqSt)
			{
				PP_rmtDiag.state.StopLogAcqReq  =  1;
				PP_rmtDiag.state.StoplogeventId =  MsgDataBody.eventId;
				PP_rmtDiag.state.StoplogexpTime = MsgDataBody.expTime;
				PP_rmtDiag.state.StopLogResp.ecuType    =  Appdata.StopLogAcquisitionReq.ecuType;
			}
			else
			{
				log_e(LOG_HOZON, "stop log collect request is busy\n");
			}
		}
		break;
		case PP_MID_DIAG_FAULTCODECLEAN:
		{
			if(PP_FAULTCODECLEAN_IDLE == PP_rmtDiag.state.cleanfaultSt)
			{
				log_o(LOG_HOZON, "rcv clean fault request\n");
				PP_rmtDiag.state.cleanfaultReq = 1;
				PP_rmtDiag.state.cleanfaultType = Appdata.FaultCodeClearanceReq.diagType;
				PP_rmtDiag.state.cleanfaulteventId = MsgDataBody.eventId;
				PP_rmtDiag.state.cleanfaultexpTime = MsgDataBody.expTime;
				PP_rmtDiag.state.faultcleanwaittime = tm_get_time();
				PP_rmtDiag.state.sleepflag   = 0;
			}
			else
			{
				log_e(LOG_HOZON, "repeat clean fault request\n");
			}
		}
		break;
		case PP_MID_DIAG_CANBUSMSGCOLLREQ:
		{
			//TAP 向 TCU 请求车辆进行 CAN 总线报文采集
			log_o(LOG_HOZON, "rcv Can bus message collect request\n");
			PP_FileUpload_CanMsgRequest(Appdata.CanBusMessageCollectReq.durationTime);
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
	int ret = 0;
	int idlenode;
	int mtxlockst = 0;

	if(1 == get_factory_mode())
	{
		PP_rmtDiag.state.diagReq = 0;
		PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
		return 0;
	}

	switch(PP_rmtDiag.state.diagrespSt)
	{
		case PP_DIAGRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.diagReq)//接收到tsp查询故障请求
			{
				if(!dev_get_KL15_signal())
				{
					log_o(LOG_HOZON, "ign status：off\n");
					PP_rmtDiag.state.diagReq = 0;
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_IGNOFF;
					PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
					return 0;
				}

				if(0 == PP_rmtCfg_enable_dtcEnabled())
				{
					log_o(LOG_HOZON, "remote diag func unenable\n");
					PP_rmtDiag.state.diagReq = 0;
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_DIAGUNENABLE;
					PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
					return 0;
				}
				
				if(PP_canSend_weakupVehicle(RMTDIAG_VIRTUAL) == 0)
				{
					return 0;
				}


				log_o(LOG_HOZON, "start remote diag\n");
				memset(&PP_rmtDiag_Fault,0 , sizeof(PP_rmtDiag_Fault_t));
				mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_DIAG_TSPDIAG);
				if(PP_LOCK_OK == mtxlockst)
				{
					PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_VEHICOND;
				}
				else
				{
					log_e(LOG_HOZON, "mtxlockst = %d\n",mtxlockst);
					if(PP_LOCK_ERR_FOTAREADVER == mtxlockst)
					{
						log_e(LOG_HOZON, "In the fota ecu diag\n");
						PP_rmtDiag.state.result = 0;
						PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_FOTAECUDIAG;
						PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
					}
					else if(PP_LOCK_ERR_FOTAUPDATE == mtxlockst)
					{
						log_e(LOG_HOZON, "In the fota upgrade\n");
						PP_rmtDiag.state.result = 0;
						PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_FOTAING;
						PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
					}
					else
					{
						log_e(LOG_HOZON, "other diag ing\n");
						PP_rmtDiag.state.result = 0;
						PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_DIAGEVTCONFLICT;
						PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
					}
				}
				PP_rmtDiag.state.diagReq = 0;
			}
		}
		break;
		case PP_DIAGRESP_VEHICOND:
		{
			if(gb_data_vehicleSpeed() <= 50)//判断车速<=5km/h,满足诊断条件
			{
				PP_rmtDiag.state.faultquerySt = 0;
				PP_rmtDiag.state.waittime = tm_get_time();
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYFAILREQ;
			}
			else
			{
				log_e(LOG_HOZON, "vehicle speed > 5km/h");
				PP_rmtDiag.state.result = 0;
				PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_VEHISPEED;
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
			}
		}
		break;
		case PP_DIAGRESP_QUERYFAILREQ:
		{
			if((tm_get_time() - PP_rmtDiag.state.waittime) >= 30)
			{
				log_i(LOG_HOZON, "diagType = %d\n",PP_rmtDiag.state.diagType);
				setPPrmtDiagCfg_QueryFaultReq(PP_rmtDiag.state.diagType);
				PP_rmtDiag.state.waittime = tm_get_time();
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYWAIT;
			}
		}
		break;
		case PP_DIAGRESP_QUERYWAIT:
		{
			if((tm_get_time() - PP_rmtDiag.state.waittime) <= PP_DIAGQUERY_WAITTIME)
			{
				if(1 == PP_rmtDiag.state.faultquerySt)//查询完成
				{
					getPPrmtDiagCfg_Faultcode(PP_rmtDiag.state.diagType,&PP_rmtDiag_Fault);//读取故障码
					getPPrmtDiag_tboxFaultcode(&PP_rmtDiag_Fault);
					log_o(LOG_HOZON, "PP_rmtDiag.state.diagType = %d and PP_rmtDiag_Fault.failNum = %d\n",PP_rmtDiag.state.diagType,PP_rmtDiag_Fault.faultNum);
					PP_rmtDiag.state.result = PP_rmtDiag_Fault.sueecss;
					PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_NONE;
					PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
				}
			}
			else//超时
			{
				PP_rmtDiag.state.result = 0;
				PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_TIMEOUT;
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_QUERYUPLOAD;
			}
		}
		break;
		case PP_DIAGRESP_QUERYUPLOAD:
		{
			ret = PP_rmtDiag_DiagResponse(task,&PP_rmtDiag,&PP_rmtDiag_Fault);
			if(ret >= 0)
			{
				idlenode = PP_getIdleNode();
				PP_TxInform[idlenode].aid = PP_AID_DIAG;
				PP_TxInform[idlenode].mid = PP_MID_DIAG_RESP;
				PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
				PP_TxInform[idlenode].eventtime = tm_get_time();
				PP_TxInform[idlenode].idleflag = 1;
				PP_TxInform[idlenode].description = "diag req response";
				SP_data_write(PP_rmtDiag_Pack.Header.sign, \
						PP_rmtDiag_Pack.totallen,PP_rmtDiag_send_cb,&PP_TxInform[idlenode]);

				if(1 == ret)//上报完成
				{
					log_o(LOG_HOZON, "fault report finish\n");
					PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_END;
				}
			}
			else
			{
				PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_END;
			}
		}
		break;
		case PP_DIAGRESP_END:
		{
			clearPP_lock_odcmtxlock(PP_LOCK_DIAG_TSPDIAG);
			clearPP_canSend_virtualOnline(RMTDIAG_VIRTUAL);//清除虚拟on线
			PP_rmtDiag.state.diagrespSt = PP_DIAGRESP_IDLE;
			PP_rmtDiag.state.sleepflag = 1;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*PP_rmtDiag_do_FaultCodeClean

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_FaultCodeClean(PrvtProt_task_t *task)
{
	int mtxlockst = 0;

	if(1 == get_factory_mode())
	{
		PP_rmtDiag.state.cleanfaultReq = 0;
		PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_IDLE;
		return 0;
	}

	switch(PP_rmtDiag.state.cleanfaultSt)
	{
		case PP_FAULTCODECLEAN_IDLE:
		{
			if(1 == PP_rmtDiag.state.cleanfaultReq)
			{
				if(!dev_get_KL15_signal())
				{
					log_o(LOG_HOZON, "ign status：off\n");
					PP_rmtDiag.state.cleanfaultReq = 0;
					PP_rmtDiag.state.faultCleanResult	= 0;
					PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_IGNOFF;
					PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
					return 0;
				}

				if(0 == PP_rmtCfg_enable_dtcEnabled())
				{
					log_o(LOG_HOZON, "remote diag func unenable\n");
					PP_rmtDiag.state.cleanfaultReq = 0;
					PP_rmtDiag.state.faultCleanResult	= 0;
					PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_DIAGUNENABLE;
					PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
					return 0;
				}
				
				if(PP_canSend_weakupVehicle(RMTDIAG_VIRTUAL) == 0)
				{
					return 0;
				}


				log_o(LOG_HOZON, "rmt clean fault request\n");
				mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_DIAG_CLEAN);
				if(PP_LOCK_OK == mtxlockst)
				{
					PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_VEHICOND;
				}
				else
				{
					log_e(LOG_HOZON, "mtxlockst = %d\n",mtxlockst);
					if(PP_LOCK_ERR_FOTAREADVER == mtxlockst)
					{
						log_e(LOG_HOZON, "In the fota ecu diag\n");
						PP_rmtDiag.state.faultCleanResult = 0;
						PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_FOTAECUDIAG;
						PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
					}
					else if(PP_LOCK_ERR_FOTAUPDATE == mtxlockst)
					{
						log_e(LOG_HOZON, "In the fota upgrade\n");
						PP_rmtDiag.state.faultCleanResult	= 0;
						PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_FOTAING;
						PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
					}
					else
					{
						log_e(LOG_HOZON, "other diag ing\n");
						PP_rmtDiag.state.faultCleanResult	= 0;
						PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_DIAGEVTCONFLICT;
						PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
					}
				}
				PP_rmtDiag.state.cleanfaultReq = 0;
			}
		}
		break;
		case PP_FAULTCODECLEAN_VEHICOND:
		{
			if(gb_data_vehicleSpeed() <= 50)//判断车速<=5km/h,满足诊断条件
			{
				log_o(LOG_HOZON, "vehi speed <= 5km,start clean fault code\n");
				PP_rmtDiag.state.faultcleanwaittime = tm_get_time();
				PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_REQ;
			}
			else
			{
				log_e(LOG_HOZON,"vehicle speed > 5km/h,exit clean fault code\n");
				PP_rmtDiag.state.faultCleanResult	= 0;
				PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_VEHISPEED;
				PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
			}
		}
		break;
		case PP_FAULTCODECLEAN_REQ:
		{
			if((tm_get_time() - PP_rmtDiag.state.faultcleanwaittime) >= 30)
			{
				PP_rmtDiag.state.faultCleanFinish = 0;
				setPPrmtDiagCfg_ClearDTCReq(PP_rmtDiag.state.cleanfaultType);
				PP_rmtDiag.state.faultcleanwaittime = tm_get_time();
				PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_WAIT;
			}
		}
		break;
		case PP_FAULTCODECLEAN_WAIT:
		{
			if((tm_get_time() - PP_rmtDiag.state.faultcleanwaittime) < PP_FAULTCODECLEAN_WAITTIME)
			{
				if(1 == PP_rmtDiag.state.faultCleanFinish)
				{
					uint8_t failureType = 0;
					PP_rmtDiag.state.faultCleanResult = \
								getPPrmtDiagCfg_clearDTCresult(PP_rmtDiag.state.cleanfaultType,&failureType);
					PP_rmtDiag.state.faultCleanfailureType = failureType;
					PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
				}
			}
			else
			{
				log_e(LOG_HOZON, "clean faultcode timeout\n");
				PP_rmtDiag.state.faultCleanResult	= 0;
				PP_rmtDiag.state.faultCleanfailureType = PP_RMTDIAG_ERROR_TIMEOUT;
				PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_END;
			}
		}
		break;
		case PP_FAULTCODECLEAN_END:
		{
			PP_rmtDiag_FaultCodeCleanResp(task,&PP_rmtDiag);
			clearPP_lock_odcmtxlock(PP_LOCK_DIAG_CLEAN);
			clearPP_canSend_virtualOnline(RMTDIAG_VIRTUAL);//清除虚拟on线
			PP_rmtDiag.state.cleanfaultSt = PP_FAULTCODECLEAN_IDLE;
			PP_rmtDiag.state.sleepflag = 1;
		}
		break;
		default :
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
	if((1 == get_factory_mode())  || (!dev_get_KL15_signal()))
	{
		PP_rmtDiag.state.ImageAcquisitionReq = 0;
		PP_rmtDiag.state.ImageAcqRespSt = PP_IMAGEACQRESP_IDLE;
		return 0;
	}

	switch(PP_rmtDiag.state.ImageAcqRespSt)
	{
		case PP_IMAGEACQRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.ImageAcquisitionReq)
			{
				log_o(LOG_HOZON, "start remote ImageAcquisition\n");
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
			tspInformHU.eventid = PP_rmtDiag.state.imagereqeventId;
			tspInformHU.datatype = PP_rmtDiag.state.dataType;
			tspInformHU.effectivetime = PP_rmtDiag.state.ImagedurationTime;
			tspInformHU.cameraname = PP_rmtDiag.state.cameraName;
			//tspInformHU.sizelimit = PP_rmtDiag.state.sizeLimit;
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
*鍑芥暟鍚嶏細:PP_rmtDiag_do_checkrmtLogReq

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_checkrmtLogReq(PrvtProt_task_t *task)
{
	int i;
	if((1 == get_factory_mode())  || (!dev_get_KL15_signal()))
	{
		PP_rmtDiag.state.LogAcqReq = 0;
		PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_IDLE;
		return 0;
	}

	switch(PP_rmtDiag.state.LogAcqRespSt)
	{
		case PP_LOGACQRESP_IDLE:
		{
			if(1 == PP_rmtDiag.state.LogAcqReq)
			{
				log_o(LOG_HOZON, "start remote LogAcquisition\n");
				for(i = 0;i < PP_ECULOG_MAX;i++)
				{
					memset(&PP_rmtDiag.state.logReqResp[i],0,sizeof(PP_rmtDiag_logReqResp_t));
				}
				PP_rmtDiag.state.LogAcqReq = 0;
				PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_INFORM_UPLOADLOG;
			}
		}
		break;
		case PP_LOGACQRESP_INFORM_UPLOADLOG://通知上传log
		{
			if(1 == PP_rmtDiag.state.ecuLog[0].ecuType)
			{//tbox上传log
				log_o(LOG_HOZON, "inform tbox upload log\n");
				PP_rmtDiag.state.ecuLog[0].ecuType = 0;

				//通知采集tboxlog
				PP_log_upload_t acqlog_para;
				acqlog_para.log_stop_flag = 0;
				acqlog_para.log_grade 	= PP_rmtDiag.state.ecuLog[0].logLevel;
				acqlog_para.log_up_time = PP_rmtDiag.state.ecuLog[0].durationTime;
				acqlog_para.log_start_time = PP_rmtDiag.state.ecuLog[0].startTime;
				acqlog_para.log_eventId = PP_rmtDiag.state.logeventId;
				PP_FileUpload_LogRequest(acqlog_para);

				PP_rmtDiag.state.logReqResp[0].ecuType = 1;
				PP_rmtDiag.state.logReqResp[0].result = 1;
				PP_rmtDiag.state.logReqResp[0].failureType = 0;
			}
#if 0
			for(i = 1;i < PP_ECULOG_MAX;i++)
			{

			}

			if(PP_ECULOG_IHU == PP_rmtDiag.state.ecuType)
			{//通知HU 上传log
				log_o(LOG_HOZON, "inform HU upload log\n");
				ivi_logfile appointchargeSt;
				appointchargeSt.aid = PP_AID_DIAG;
				appointchargeSt.mid = PP_MID_DIAG_LOGACQRESP;
				appointchargeSt.eventid = PP_rmtDiag.state.logeventId;
				appointchargeSt.timestamp = PrvtPro_getTimestamp();
				appointchargeSt.level = PP_rmtDiag.state.logLevel;
				appointchargeSt.starttime = PP_rmtDiag.state.startTime;
				appointchargeSt.durationtime = PP_rmtDiag.state.durationTime;
				tbox_ivi_set_tsplogfile_InformHU(&appointchargeSt);
			}
#endif
			PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_INFORM_TSP;
		}
		break;
		case PP_LOGACQRESP_INFORM_WAITING:
		break;
		case PP_LOGACQRESP_INFORM_TSP:
		{
			PP_rmtDiag_LogAcqReqResp(task,&PP_rmtDiag);
			PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_END;
		}
		break;
		case PP_LOGACQRESP_END:
		{
			PP_rmtDiag.state.LogAcqRespSt = PP_LOGACQRESP_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*鍑芥暟鍚嶏細:PP_rmtDiag_do_stoprmtLogReq

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
static int PP_rmtDiag_do_stoprmtLogReq(PrvtProt_task_t *task)
{
	//int i;
	if((1 == get_factory_mode())  || (!dev_get_KL15_signal()))
	{
		PP_rmtDiag.state.StopLogAcqReq = 0;
		PP_rmtDiag.state.StopLogAcqSt = PP_STOPLOG_IDLE;
		return 0;
	}

	switch(PP_rmtDiag.state.StopLogAcqSt)
	{
		case PP_STOPLOG_IDLE:
		{
			if(1 == PP_rmtDiag.state.StopLogAcqReq)
			{
				PP_rmtDiag.state.StopLogAcqReq = 0;
				PP_rmtDiag.state.StopLogAcqSt = PP_STOPLOG_INFORM_STOP;
			}
		}
		break;
		case PP_STOPLOG_INFORM_STOP:
		{
			if(1 == PP_rmtDiag.state.StopLogResp.ecuType)
			{
				//通知停止采集tboxlog
				PP_log_upload_t acqlog_para;
				acqlog_para.log_stop_flag = 1;
				PP_FileUpload_LogRequest(acqlog_para);
				PP_rmtDiag.state.StopLogResp.result  = 1;
				PP_rmtDiag.state.StopLogResp.failureType  = 0;
			}
			else
			{
				PP_rmtDiag.state.StopLogResp.result  = 0;
				PP_rmtDiag.state.StopLogResp.failureType  = 1;
			}
			PP_rmtDiag.state.StopLogAcqSt = PP_STOPLOG_INFORM_TSP;
		}
		break;
		case PP_STOPLOG_INFORM_WAITING:
		break;
		case PP_STOPLOG_INFORM_TSP:
		{
			PP_rmtDiag_StopLogAcqResp(task,&PP_rmtDiag);
			PP_rmtDiag.state.StopLogAcqSt = PP_STOPLOG_END;
		}
		break;
		case PP_STOPLOG_END:
		{
			PP_rmtDiag.state.StopLogAcqSt = PP_STOPLOG_IDLE;
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

*澶�  娉細:每天仅成功上报1次
******************************************************/
static int PP_rmtDiag_do_DiagActiveReport(PrvtProt_task_t *task)
{
	static uint8_t tm_wday;
	static uint32_t tm_datetime;
	int i;
	int ret;
	int idlenode;
	int mtxlockst = 0;

	if(1 == get_factory_mode())
	{
		PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_IDLE;
		PP_rmtDiag.state.activeDiagFlag = 0;
		return 0;
	}

	switch(PP_rmtDiag.state.activeDiagSt)
	{
		case PP_ACTIVEDIAG_IDLE:
		{
			if((1 == PP_rmtDiag.state.activeDiagFlag) && \
								(0 == PP_rmtDiag.state.mcurtcflag))
			{
				PP_rmtDiag.state.activeDiagFlag = 0;
				if(0 == PP_rmtCfg_enable_dtcEnabled())
				{
					log_o(LOG_HOZON, "remote diag func unenable\n");
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_DIAGUNENABLE;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
					return 0;
				}

				memset(&PP_rmtDiag_allFault,0 , sizeof(PP_rmtDiag_allFault_t));
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_CHECKREPORTST;
			}
			else
			{
				PP_rmtDiag.state.activeDiagFlag = 0;
			}
		}
		break;
		case PP_ACTIVEDIAG_CHECKREPORTST:
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
				log_o(LOG_HOZON,"uploaded date : %d\n",rmtDiag_datetime.datetime);
			}
			else
			{
				log_o(LOG_HOZON,"start to daig report\n");
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_CHECKOTACOND;
			}
		}
		break;
		case PP_ACTIVEDIAG_CHECKOTACOND:
		{
			mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_DIAG_ACTIVE);
			if(PP_LOCK_OK == mtxlockst)
			{
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_CHECKVEHICOND;
				PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
			}
			else
			{
				log_e(LOG_HOZON, "mtxlockst = %d\n",mtxlockst);
				if(PP_LOCK_ERR_FOTAREADVER == mtxlockst)
				{
					log_e(LOG_HOZON, "In the fota ecu diag\n");
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_FOTAECUDIAG;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
				}
				else if(PP_LOCK_ERR_FOTAUPDATE == mtxlockst)
				{
					log_e(LOG_HOZON, "In the fota upgrade\n");
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_FOTAING;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
				}
				else
				{
					log_e(LOG_HOZON, "other diag ing\n");
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_DIAGEVTCONFLICT;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
				}
			}
		}
		break;
		case PP_ACTIVEDIAG_CHECKVEHICOND:
		{
			if((tm_get_time() - PP_rmtDiag.state.activeDiagdelaytime) >= PP_DIAGPWRON_WAITTIME)//延时5s
			{
				if(gb_data_vehicleSpeed() <= 50)//判断车速<=5km/h,满足诊断条件
				{
					log_o(LOG_HOZON,"vehicle speed <= 5km/h,start diag\n");
					PP_rmtDiag.state.faultquerySt = 0;
					setPPrmtDiagCfg_QueryFaultReq(PP_DIAG_ALL);//请求查询所有故障码
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUREYWAIT;
				}
				else
				{
					log_e(LOG_HOZON,"vehicle speed > 5km/h,exit active diag\n");
					PP_rmtDiag.state.result = 0;
					PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_VEHISPEED;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
				}
			}
		}
		break;
		case PP_ACTIVEDIAG_QUREYWAIT:
		{
			if((tm_get_time() - PP_rmtDiag.state.activeDiagdelaytime) <= PP_DIAGQUERYALL_WAITTIME)//
			{
				if(1 == PP_rmtDiag.state.faultquerySt)//查询完成
				{
					for(i=1;i <= PP_DIAG_MAXECU;i++)
					{
						getPPrmtDiagCfg_Faultcode(i,&PP_rmtDiag_allFault.code[i-1]);//读取故障码
						log_i(LOG_HOZON, "PP_rmtDiag_allFault.code[%d].sueecss = %d\n",i-1,PP_rmtDiag_allFault.code[i-1].sueecss);
						log_i(LOG_HOZON, "PP_rmtDiag_allFault.code[%d].faultNum = %d\n",i-1,PP_rmtDiag_allFault.code[i-1].faultNum);
					}
					PP_rmtDiag_allFault.currdiagtype = PP_DIAG_VCU;
					PP_rmtDiag.state.result = 1;
					PP_rmtDiag.state.failureType = PP_RMTDIAG_ERROR_NONE;
					PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
				}
			}
			else//超时
			{
				log_e(LOG_HOZON, "diag active report is timeout\n");
				PP_rmtDiag.state.result = 0;
				PP_rmtDiag.state.failureType  = PP_RMTDIAG_ERROR_TIMEOUT;
				PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
			}
		}
		break;
		case PP_ACTIVEDIAG_QUERYUPLOAD:
		{
			if((tm_get_time() - PP_rmtDiag.state.activeDiagdelaytime) <	50)//延时50ms
			{
				return 0;
			}

			ret = PP_remotDiagnosticStatus(task,&PP_rmtDiag);
			if(ret >= 0)
			{
				PP_rmtDiag.state.activeDiagdelaytime = tm_get_time();
				idlenode = PP_getIdleNode();
				PP_TxInform[idlenode].aid = PP_AID_DIAG;
				PP_TxInform[idlenode].mid = PP_MID_DIAG_STATUS;
				PP_TxInform[idlenode].pakgtype = PP_TXPAKG_CONTINUE;
				PP_TxInform[idlenode].eventtime = tm_get_time();
				PP_TxInform[idlenode].idleflag = 1;
				PP_TxInform[idlenode].description = "diag status response";
				SP_data_write(PP_rmtDiag_Pack.Header.sign,PP_rmtDiag_Pack.totallen, \
						PP_rmtDiag_send_cb,&PP_TxInform[idlenode]);

				if(ret ==1)//all数据打包发送完成
				{
					if(1 == PP_rmtDiag.state.result)//诊断成功
					{
						rmtDiag_datetime.diagflag = rmtDiag_weekmask[tm_wday].mask;
						if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGFLAG, &rmtDiag_datetime.diagflag, 1))
						{
							log_e(LOG_GB32960, "save rmtDiag_datetime.diagflag failed\n");
						}

						rmtDiag_datetime.datetime = tm_datetime;
						if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGDATE, &rmtDiag_datetime.datetime, 4))
						{
							log_e(LOG_GB32960, "save rmtDiag_datetime.datetime failed\n");
						}
					}
					PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_END;
				}
			}
			else
			{
				PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_END;
			}
		}
		break;
		case PP_ACTIVEDIAG_END:
		{
			clearPP_lock_odcmtxlock(PP_LOCK_DIAG_ACTIVE);
			PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*PP_rmtDiag_FaultCodeCleanResp

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細diag response

*澶�  娉細
******************************************************/
static int PP_rmtDiag_FaultCodeCleanResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	//int i;
	int idlenode;
	
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
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_FAULTCODECLEANRESP;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = rmtDiag->state.cleanfaulteventId;
	PP_rmtDiag.pack.DisBody.expTime = rmtDiag->state.cleanfaultexpTime;
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*appdata*/
	memset(&AppData_rmtDiag.FaultCodeClearanceResp,0,sizeof(PP_FaultCodeClearanceResp_t));
	AppData_rmtDiag.FaultCodeClearanceResp.diagType = rmtDiag->state.cleanfaultType;
	AppData_rmtDiag.FaultCodeClearanceResp.result = rmtDiag->state.faultCleanResult;
	AppData_rmtDiag.FaultCodeClearanceResp.failureType = rmtDiag->state.faultCleanfailureType;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_CLEANFAULTRESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.FaultCodeClearanceResp))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_DIAG;
	PP_TxInform[idlenode].mid = PP_MID_DIAG_FAULTCODECLEANRESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "diag cleanfaultcode response";
	SP_data_write(PP_rmtDiag_Pack.Header.sign, \
			PP_rmtDiag_Pack.totallen,PP_rmtDiag_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*PP_rmtDiag_StopLogAcqResp

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細diag response

*澶�  娉細
******************************************************/
static int PP_rmtDiag_StopLogAcqResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	//int i;
	int idlenode;
	
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
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_STOPLOGRESP;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = rmtDiag->state.StoplogeventId;
	PP_rmtDiag.pack.DisBody.expTime = rmtDiag->state.StoplogexpTime;
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*appdata*/
	memset(&AppData_rmtDiag.StopLogResp,0,sizeof(PP_LogAcqEcuResp_t));
	AppData_rmtDiag.StopLogResp.ecuType = rmtDiag->state.StopLogResp.ecuType;
	AppData_rmtDiag.StopLogResp.result = rmtDiag->state.StopLogResp.result;
	AppData_rmtDiag.StopLogResp.failureType = rmtDiag->state.StopLogResp.failureType;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_STOPLOGACQRESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.StopLogResp))
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_DIAG;
	PP_TxInform[idlenode].mid = PP_MID_DIAG_STOPLOGRESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "diag stop log response";
	SP_data_write(PP_rmtDiag_Pack.Header.sign, \
			PP_rmtDiag_Pack.totallen,PP_rmtDiag_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*PP_rmtDiag_LogAcqReqResp

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細diag response

*澶�  娉細
******************************************************/
static int PP_rmtDiag_LogAcqReqResp(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag)
{
	int msgdatalen;
	//int i;
	int idlenode;
	int i,j = 0;
	
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
	PP_rmtDiag.pack.DisBody.mID = PP_MID_DIAG_LOGACQRESP;
	PP_rmtDiag.pack.DisBody.eventTime = PrvtPro_getTimestamp();
	PP_rmtDiag.pack.DisBody.eventId = rmtDiag->state.logeventId;
	PP_rmtDiag.pack.DisBody.expTime = rmtDiag->state.logexpTime;
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*appdata*/
	AppData_rmtDiag.LogAcquisitionResp.ecuNum = 0;
	for(i = 0 ;i < PP_ECULOG_MAX;i++)
	{
		if(1 == PP_rmtDiag.state.logReqResp[i].ecuType)
		{
			AppData_rmtDiag.LogAcquisitionResp.EcuLog[j].ecuType = i+1;
			AppData_rmtDiag.LogAcquisitionResp.EcuLog[j].result = PP_rmtDiag.state.logReqResp[i].result;
			AppData_rmtDiag.LogAcquisitionResp.EcuLog[j].failureType = PP_rmtDiag.state.logReqResp[i].failureType;
			j++;
			AppData_rmtDiag.LogAcquisitionResp.ecuNum++;
		}
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_LOGACQRESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.LogAcquisitionResp))
	{
		log_e(LOG_HOZON, "encode error\n");
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	idlenode = PP_getIdleNode();
	PP_TxInform[idlenode].aid = PP_AID_DIAG;
	PP_TxInform[idlenode].mid = PP_MID_DIAG_LOGACQRESP;
	PP_TxInform[idlenode].pakgtype = PP_TXPAKG_SIGTIME;
	PP_TxInform[idlenode].eventtime = tm_get_time();
	PP_TxInform[idlenode].idleflag = 1;
	PP_TxInform[idlenode].description = "diag req log upload response";
	SP_data_write(PP_rmtDiag_Pack.Header.sign, \
			PP_rmtDiag_Pack.totallen,PP_rmtDiag_send_cb,&PP_TxInform[idlenode]);

	return 0;
}

/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_DiagResponse

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細diag response

*澶�  娉細
******************************************************/
static int PP_rmtDiag_DiagResponse(PrvtProt_task_t *task,PrvtProt_rmtDiag_t *rmtDiag,PP_rmtDiag_Fault_t *rmtDiag_Fault)
{
	int msgdatalen;
	int i;
	int ret = 0;
	static uint8_t faultUpdataCnt = 0;

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
	PP_rmtDiag.pack.DisBody.expTime = rmtDiag->state.diagexpTime;
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*appdata*/
	memset(&AppData_rmtDiag.DiagnosticResp,0,sizeof(PP_DiagnosticResp_t));
	AppData_rmtDiag.DiagnosticResp.diagType = rmtDiag->state.diagType;
	AppData_rmtDiag.DiagnosticResp.result = rmtDiag->state.result;
	AppData_rmtDiag.DiagnosticResp.failureType = rmtDiag->state.failureType;
	if((1 == AppData_rmtDiag.DiagnosticResp.result) && (rmtDiag_Fault->faultNum > 0))
	{
		for(i = 0;i < PP_DIAG_MAX_REPORT;i++)
		{
			memcpy(AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCode,rmtDiag_Fault->faultcode[faultUpdataCnt].diagcode,5);
			AppData_rmtDiag.DiagnosticResp.diagCode[i].diagCodelen = 5;
			AppData_rmtDiag.DiagnosticResp.diagCode[i].faultCodeType  = rmtDiag_Fault->faultcode[faultUpdataCnt].faultCodeType;
			AppData_rmtDiag.DiagnosticResp.diagCode[i].lowByte = rmtDiag_Fault->faultcode[faultUpdataCnt].lowByte;
			AppData_rmtDiag.DiagnosticResp.diagCode[i].diagTime = rmtDiag_Fault->faultcode[faultUpdataCnt].diagTime;
			AppData_rmtDiag.DiagnosticResp.diagcodenum++;
			faultUpdataCnt++;

			if(faultUpdataCnt == rmtDiag_Fault->faultNum)
			{
				faultUpdataCnt = 0;
				ret = 1;
				break;
			}
		}
	}
	else
	{
		ret = 1;
		faultUpdataCnt = 0;
		AppData_rmtDiag.DiagnosticResp.diagcodenum = 0;
	}

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_RESP,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
									   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.DiagnosticResp))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
	{
		log_e(LOG_HOZON, "encode error\n");
		faultUpdataCnt = 0;
		return -1;
	}

	PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
	PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));

	return ret;
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
	int ret = 0;

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
	PP_rmtDiag.pack.DisBody.eventId = 0;//tsp未指定eventid，传0，已沟通
	PP_rmtDiag.pack.DisBody.expTime = -1;
	PP_rmtDiag.pack.DisBody.ulMsgCnt++;	/* OPTIONAL */
	PP_rmtDiag.pack.DisBody.appDataProVer = 256;
	PP_rmtDiag.pack.DisBody.testFlag = 1;

	/*app data*/
	memset(&AppData_rmtDiag.DiagnosticSt,0,sizeof(PP_DiagnosticStatus_t));
	uint8_t index_i = 0;
	uint8_t index_j = 0;
	uint8_t diagType;
	for(diagType = PP_rmtDiag_allFault.currdiagtype;diagType <= PP_DIAG_MAXECU;diagType++)
	{
		AppData_rmtDiag.DiagnosticSt.diagobjnum++;
		AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagType = diagType;//
		AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].result = PP_rmtDiag_allFault.code[diagType-1].sueecss;
		AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].failureType = PP_rmtDiag.state.failureType;
		if((1 == PP_rmtDiag.state.result) && (PP_rmtDiag_allFault.code[diagType-1].faultNum > 0))
		{
			for(index_j = 0;index_j < 255;index_j++)
			{
				memcpy(AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagCode[index_j].diagCode, \
						PP_rmtDiag_allFault.code[diagType-1].faultcode[PP_rmtDiag_allFault.rptfaultcnt[diagType-1]].diagcode,5);
				AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagCode[index_j].diagCodelen = 5;

				AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagCode[index_j].faultCodeType = \
						PP_rmtDiag_allFault.code[diagType-1].faultcode[PP_rmtDiag_allFault.rptfaultcnt[diagType-1]].faultCodeType;

				AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagCode[index_j].lowByte  = \
						PP_rmtDiag_allFault.code[diagType-1].faultcode[PP_rmtDiag_allFault.rptfaultcnt[diagType-1]].lowByte;

				AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagCode[index_j].diagTime = \
						PP_rmtDiag_allFault.code[diagType-1].faultcode[PP_rmtDiag_allFault.rptfaultcnt[diagType-1]].diagTime;

				PP_rmtDiag_allFault.rptfaultcnt[diagType-1]++;
				PP_rmtDiag_allFault.totalfaultCnt++;
				AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagcodenum++;
				if(PP_rmtDiag_allFault.totalfaultCnt < PP_DIAG_MAX_REPORT)
				{
					if(PP_rmtDiag_allFault.rptfaultcnt[diagType-1] == PP_rmtDiag_allFault.code[diagType-1].faultNum)
					{
						break;
					}
				}
				else
				{//数据量大时，分多次上报，当前待上报数据打包完
					PP_rmtDiag_allFault.totalfaultCnt = 0;
					//PP_rmtDiag_allFault.currdiagtype = diagType;
					if(PP_rmtDiag_allFault.rptfaultcnt[diagType-1] < PP_rmtDiag_allFault.code[diagType-1].faultNum)
					{
						PP_rmtDiag_allFault.currdiagtype = diagType;
					}
					else
					{
						PP_rmtDiag_allFault.currdiagtype = diagType + 1;
					}

					if(PP_rmtDiag_allFault.currdiagtype > PP_DIAG_MAXECU)
					{
						ret = 1;//all数据处理完
					}
					goto rmtDiagStDataPacking;
				}
			}
		}
		else
		{
			AppData_rmtDiag.DiagnosticSt.diagStatus[index_i].diagcodenum = 0;
		}

		index_i++;

		if(diagType >= PP_DIAG_MAXECU)//
		{
			ret = 1;//all数据处理完
			goto rmtDiagStDataPacking;
		}
	}


rmtDiagStDataPacking:
	{
		if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTDIAG_STATUS,PP_rmtDiag_Pack.msgdata,&msgdatalen,\
										   &PP_rmtDiag.pack.DisBody,&AppData_rmtDiag.DiagnosticSt))//鏁版嵁缂栫爜鎵撳寘鏄惁瀹屾垚
		{
			log_e(LOG_HOZON, "encode error\n");
			return -1;
		}

		PP_rmtDiag_Pack.totallen = 18 + msgdatalen;
		PP_rmtDiag_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	}

	return ret;
}

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
			log_o(LOG_HOZON, "send remote diag response ok\n");
		}
		break;
		case PP_MID_DIAG_STATUS:
		{
			if(PP_TXPAKG_SUCCESS != TxInform_ptr->successflg)
			{
				rmtDiag_datetime.diagflag = 0;
				if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGFLAG, &rmtDiag_datetime.diagflag, 1))
				{
					log_e(LOG_GB32960, "save rmtDiag_datetime.diagflag failed\n");
				}

				rmtDiag_datetime.datetime = 0;
				if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGDATE, &rmtDiag_datetime.datetime, 4))
				{
					log_e(LOG_GB32960, "save rmtDiag_datetime.datetime failed\n");
				}
			}
		}
		break;
		default:
		break;
	}
	TxInform_ptr->idleflag = 0;
}

/******************************************************
*鍑芥暟鍚嶏細PP_diag_SetdiagReq

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細璁剧疆璇锋眰

*澶�  娉細
******************************************************/
void PP_diag_SetdiagReq(unsigned char diagType,unsigned char reqtype)
{
	int i;
	if(0 == reqtype)
	{
		log_o(LOG_HOZON, "receive remote diag request\n");
		PP_rmtDiag.state.diagReq = 1;
		PP_rmtDiag.state.diagType = diagType;
		PP_rmtDiag.state.diageventId = 100;
		PP_rmtDiag.state.result = 1;
		PP_rmtDiag.state.failureType = 0;

	}
	else if(1 == reqtype)//主动上报所有故障码
	{
		log_o(LOG_HOZON, " diag fault code active report request\n");
		PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_IDLE;
		PP_rmtDiag.state.activeDiagFlag = 1;
		rmtDiag_datetime.diagflag = 0;
		if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGFLAG, &rmtDiag_datetime.diagflag, 1))
		{
			log_e(LOG_GB32960, "save rmtDiag_datetime.diagflag failed\n");
		}

		rmtDiag_datetime.datetime = 0;
		if(cfg_set_user_para(CFG_ITEM_HOZON_TSP_DIAGDATE, &rmtDiag_datetime.datetime, 4))
		{
			log_e(LOG_GB32960, "save rmtDiag_datetime.datetime failed\n");
		}
	}
	else//test
	{
		log_o(LOG_HOZON, " diag fault code active report test\n");
		PP_rmtDiag.state.activeDiagSt = PP_ACTIVEDIAG_QUERYUPLOAD;
		PP_rmtDiag.state.result = 1;
		PP_rmtDiag_allFault.currdiagtype = PP_DIAG_VCU;
		PP_rmtDiag_allFault.totalfaultCnt = 0;
		for(i = 0;i < PP_DIAG_MAXECU;i++)
		{
			PP_rmtDiag_allFault.rptfaultcnt[i] = 0;		
		}
	}
}

/******************************************************
*PP_rmtDiag_CleanFaultInform_cb

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
void PP_rmtDiag_CleanFaultInform_cb(void)
{
	PP_rmtDiag.state.faultCleanFinish = 1;
}

/******************************************************
*鍑芥暟鍚嶏細PP_rmtDiag_queryInform_cb

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
void PP_rmtDiag_queryInform_cb(void)
{
	PP_rmtDiag.state.faultquerySt = 1;
	PP_rmtDiag_CleanFaultInform_cb();
}

/******************************************************
*PP_diag_rmtdiagtest

*褰�  鍙傦細

*杩斿洖鍊硷細

*远程诊断测试

*澶�  娉細
******************************************************/
void PP_diag_rmtdiagtest(unsigned char diagType,unsigned char sueecss,unsigned char faultNum)
{
	uint8_t i,faulcnt;
	if(sueecss)
	{
		PP_rmtDiag_allFault.code[diagType].sueecss  = 1;
		PP_rmtDiag_allFault.code[diagType].faultNum = faultNum;
		for(faulcnt = 0;faulcnt < faultNum;faulcnt++)
		{
			memcpy(PP_rmtDiag_allFault.code[diagType].faultcode[faulcnt].diagcode,"12233",5);
			PP_rmtDiag_allFault.code[diagType].faultcode[faulcnt].faultCodeType	= 1;
			PP_rmtDiag_allFault.code[diagType].faultcode[faulcnt].lowByte		= 0x01;
	 		PP_rmtDiag_allFault.code[diagType].faultcode[faulcnt].diagTime		= PrvtPro_getTimestamp();
		}
	}
	else
	{
		PP_rmtDiag_allFault.code[diagType].sueecss  = 0;
		PP_rmtDiag_allFault.code[diagType].faultNum = 0;
	}

	for(i = 0;i < PP_DIAG_MAXECU;i++)
	{
		log_i(LOG_HOZON, "PP_rmtDiag_allFault.code[%d].sueecss = %d\n",i,\
					PP_rmtDiag_allFault.code[i].sueecss);
		log_i(LOG_HOZON, "PP_rmtDiag_allFault.code[%d].faultNum = %d\n",i,\
					PP_rmtDiag_allFault.code[i].faultNum);
	}
}

/******************************************************
*getPP_rmtDiag_Idle

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
char getPP_rmtDiag_Idle(void)
{
	if((PP_rmtDiag.state.diagrespSt == PP_DIAGRESP_IDLE) && \
		(PP_rmtDiag.state.cleanfaultSt == PP_FAULTCODECLEAN_IDLE)	&&	\
		(PP_rmtDiag.state.activeDiagSt == PP_ACTIVEDIAG_IDLE) && (0 == PP_rmtDiag.state.activeDiagFlag))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/******************************************************
*PP_rmtDiag_mcuRTCweakup

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
void PP_rmtDiag_mcuRTCweakup(void)
{
	PP_rmtDiag.state.mcurtcflag = 1;
	log_o(LOG_HOZON, "mcu rtc weakup\n");
}

/******************************************************
*PP_rmtDiag_showPara

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
void PP_rmtDiag_showPara(void)
{
	log_o(LOG_HOZON, "mcu rtc weakup = %s\n",PP_rmtDiag.state.mcurtcflag?"ture":"false");
	log_o(LOG_HOZON, "PP_rmtDiag.state.sleepflag = %d\n",PP_rmtDiag.state.sleepflag);
}


/******************************************************
*clearPP_rmtDiag_para

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
void clearPP_rmtDiag_para(void)
{
	PP_rmtDiag.state.mcurtcflag = 0;
	log_i(LOG_HOZON, "clear mcu rtc weakup flag\n");
}

/******************************************************
*PP_rmtDiag_sleepflag

*褰�  鍙傦細

*杩斿洖鍊硷細

*鎻�  杩帮細

*澶�  娉細
******************************************************/
uint8_t PP_rmtDiag_sleepflag(void)
{
	return PP_rmtDiag.state.sleepflag;
}

/*
	读取tbox故障，非本地诊断
*/
static void getPPrmtDiag_tboxFaultcode(PP_rmtDiag_Fault_t *rmtDiag_Fault)
{
	uint64_t timestamp;

	if(1 == PP_netstatus_pubilcfaultsts(&timestamp))
	{
		memcpy(rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].diagcode,PP_DIAG_TBOX_PBLNETFAULTCODE,5);
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].faultCodeType = PP_DIAG_TBOX_CURRENTFAULT;
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].lowByte = PP_DIAG_TBOX_PBLNETFAULTLOWBYTE;
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].diagTime	= timestamp;
		rmtDiag_Fault->faultNum++;
	}

	if(1 == tbox_ivi_get_link_fault(&timestamp))
	{
		memcpy(rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].diagcode,PP_DIAG_TBOX_HULINKFAULTCODE,5);
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].faultCodeType = PP_DIAG_TBOX_CURRENTFAULT;
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].lowByte = PP_DIAG_TBOX_HULINKFAULTLOWBYTE;
		rmtDiag_Fault->faultcode[rmtDiag_Fault->faultNum].diagTime	= timestamp;
		rmtDiag_Fault->faultNum++;
	}

}