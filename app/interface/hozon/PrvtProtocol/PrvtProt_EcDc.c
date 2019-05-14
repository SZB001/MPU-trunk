/******************************************************
文件名：	PrvtProt_EcDc.c

描述：	企业私有协议（浙江合众）,编解码	
Data			Vasion			author
2019/4/29		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/times.h>
#include "timer.h"

#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "XcallReqinfo.h"
#include "XcallRespinfo.h"
#include "Bodyinfo.h"
#include "RvsposInfo.h"
#include "CfgCheckReqInfo.h"
#include "CfgGetReqInfo.h"
#include "CfgGetRespInfo.h"
#include "CfgCheckRespInfo.h"
#include "CfgEndReqInfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "PrvtProt_EcDc.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static asn_TYPE_descriptor_t *pduType_Body = &asn_DEF_Bodyinfo;
static asn_TYPE_descriptor_t *pduType_XcallReq = &asn_DEF_XcallReqinfo;
static asn_TYPE_descriptor_t *pduType_XcallResp = &asn_DEF_XcallRespinfo;

static asn_TYPE_descriptor_t *pduType_Cfg_check_req = &asn_DEF_CfgCheckReqInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_check_resp = &asn_DEF_CfgCheckRespInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_get_req = &asn_DEF_CfgGetReqInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_get_resp = &asn_DEF_CfgGetRespInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_end_req = &asn_DEF_CfgEndReqInfo;

static uint8_t tboxAppdata[PP_ECDC_DATA_LEN];
static int tboxAppdataLen;
static uint8_t tboxDisBodydata[PP_ECDC_DATA_LEN];
static int tboxDisBodydataLen;

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PrvtPro_writeout(const void *buffer,size_t size,void *key);
static void PrvtPro_showMsgData(uint8_t type,Bodyinfo_t *RxBodydata,void *RxAppdata);

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PrvtPro_msgPackage

*形  参：

*返回值：

*描  述：数据打包编码

*备  注：
******************************************************/
int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,long *msgDataLen, \
					   void *disptrBody, void *appchoice)
{
	static uint8_t key;
	Bodyinfo_t Bodydata;
	PrvtProt_DisptrBody_t 	*DisptrBody = (PrvtProt_DisptrBody_t*)disptrBody;
	PrvtProt_appData_t		*Appchoice	= (PrvtProt_appData_t*)appchoice;
	int i;
	
	memset(&Bodydata,0 , sizeof(Bodyinfo_t));
	Bodydata.expirationTime = NULL;/* OPTIONAL */
	Bodydata.eventId 		= NULL;/* OPTIONAL */
	Bodydata.ulMsgCnt 		= NULL;	/* OPTIONAL */
	Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */
	Bodydata.msgCntAcked 	= NULL;/* OPTIONAL */
	Bodydata.ackReq			= NULL;/* OPTIONAL */
	Bodydata.appDataLen 	= NULL;/* OPTIONAL */
	Bodydata.appDataEncode	= NULL;/* OPTIONAL */
	Bodydata.appDataProVer	= NULL;/* OPTIONAL */
	Bodydata.testFlag		= NULL;/* OPTIONAL */
	Bodydata.result			= NULL;/* OPTIONAL */
/*********************************************
	填充 dispatcher body和application data
*********************************************/	
	Bodydata.aID.buf = DisptrBody->aID;
	Bodydata.aID.size = 3;
	Bodydata.mID = DisptrBody->mID;
	Bodydata.eventTime 		= DisptrBody->eventTime;
	Bodydata.expirationTime = &(DisptrBody->expTime);	/* OPTIONAL */;
	if(DisptrBody->eventId != PP_INVALID)//event ID初始化为0xFFFFFFFF-无效
	{
		Bodydata.eventId 	= &(DisptrBody->eventId);/* OPTIONAL */
	}
	Bodydata.ulMsgCnt 		= &(DisptrBody->ulMsgCnt);	/* OPTIONAL */
	Bodydata.dlMsgCnt 		= &(DisptrBody->dlMsgCnt);	/* OPTIONAL */
	Bodydata.msgCntAcked 	= &(DisptrBody->msgCntAcked);/* OPTIONAL */
	Bodydata.ackReq			= &(DisptrBody->ackReq);/* OPTIONAL */
	Bodydata.appDataLen 	= &(DisptrBody->appDataLen);/* OPTIONAL */
	Bodydata.appDataEncode	= &(DisptrBody->appDataEncode);/* OPTIONAL */
	Bodydata.appDataProVer	= &(DisptrBody->appDataProVer);/* OPTIONAL */
	Bodydata.testFlag		= &(DisptrBody->testFlag);/* OPTIONAL */
	Bodydata.result			= &(DisptrBody->result);/* OPTIONAL */
	
	asn_enc_rval_t ec;
	log_i(LOG_HOZON, "uper encode:appdata");
	key = PP_ENCODE_APPDATA;
	tboxDisBodydataLen = 0;
	tboxAppdataLen = 0;
	switch(type)
	{
		case ECDC_XCALL_REQ:
		{
			XcallReqinfo_t XcallReq;	
			memset(&XcallReq,0 , sizeof(XcallReqinfo_t));
			XcallReq.xcallType = Appchoice->Xcall.xcallType;
			
			ec = uper_encode(pduType_XcallReq,(void *) &XcallReq,PrvtPro_writeout,&key);
			log_i(LOG_HOZON, "uper encode appdata ec.encoded = %d",ec.encoded);
			if(ec.encoded  == -1) 
			{
				log_e(LOG_HOZON, "Could not encode MessageFrame");
				return -1;
			}
		}
		break;
		case ECDC_XCALL_RESP:
		{
			XcallRespinfo_t XcallResp;
			RvsposInfo_t Rvspos;
			RvsposInfo_t *Rvspos_ptr = &Rvspos;

			memset(&XcallResp,0 , sizeof(XcallRespinfo_t));
			memset(&Rvspos,0 , sizeof(RvsposInfo_t));

			XcallResp.xcallType = Appchoice->Xcall.xcallType;
			XcallResp.engineSt = Appchoice->Xcall.engineSt;
			XcallResp.ttOdoMeter = Appchoice->Xcall.totalOdoMr;
			Rvspos.gpsSt = Appchoice->Xcall.gpsPos.gpsSt;//gps状态 0-无效；1-有效
			Rvspos.gpsTimestamp = Appchoice->Xcall.gpsPos.gpsTimestamp;//gps时间戳
			Rvspos.latitude = Appchoice->Xcall.gpsPos.latitude;//纬度 x 1000000,当GPS信号无效时，值为0
			Rvspos.longitude = Appchoice->Xcall.gpsPos.longitude;//经度 x 1000000,当GPS信号无效时，值为0
			Rvspos.altitude = Appchoice->Xcall.gpsPos.altitude;//高度（m）
			Rvspos.heading = Appchoice->Xcall.gpsPos.heading;//车头方向角度，0为正北方向
			Rvspos.gpsSpeed = Appchoice->Xcall.gpsPos.gpsSpeed;//速度 x 10，单位km/h
			Rvspos.hdop = Appchoice->Xcall.gpsPos.hdop;//水平精度因子 x 10
			XcallResp.gpsPos.list.array = &Rvspos_ptr;
			XcallResp.gpsPos.list.size =0;
			XcallResp.gpsPos.list.count =1;

			XcallResp.srsSt = Appchoice->Xcall.srsSt;
			XcallResp.updataTime = Appchoice->Xcall.updataTime;
			XcallResp.battSOCEx = Appchoice->Xcall.battSOCEx;

			ec = uper_encode(pduType_XcallResp,(void *) &XcallResp,PrvtPro_writeout,&key);

			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON, "encode:appdata XcallResp fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCFG_CHECK_REQ:
		{
			log_i(LOG_HOZON, "encode Cfg_check_resp\n");
			CfgCheckReqInfo_t cfgcheckReq;
			memset(&cfgcheckReq,0 , sizeof(CfgCheckReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			cfgcheckReq.mcuSw.buf = Appchoice->rmtCfg.checkCfgReq.mcuSw;
			cfgcheckReq.mcuSw.size = Appchoice->rmtCfg.checkCfgReq.mcuSwlen;
			cfgcheckReq.mpuSw.buf = Appchoice->rmtCfg.checkCfgReq.mpuSw;
			cfgcheckReq.mpuSw.size = Appchoice->rmtCfg.checkCfgReq.mpuSwlen;
			cfgcheckReq.vehicleVIN.buf = Appchoice->rmtCfg.checkCfgReq.vehicleVin;
			cfgcheckReq.vehicleVIN.size = Appchoice->rmtCfg.checkCfgReq.vehicleVinlen;
			cfgcheckReq.iccID.buf = Appchoice->rmtCfg.checkCfgReq.iccID;
			cfgcheckReq.iccID.size = Appchoice->rmtCfg.checkCfgReq.iccIDlen;
			cfgcheckReq.btMacAddr.buf = Appchoice->rmtCfg.checkCfgReq.btMacAddr;
			cfgcheckReq.btMacAddr.size = Appchoice->rmtCfg.checkCfgReq.btMacAddrlen;
			cfgcheckReq.configSw.buf = Appchoice->rmtCfg.checkCfgReq.configSw;
			cfgcheckReq.configSw.size = Appchoice->rmtCfg.checkCfgReq.configSwlen;
			cfgcheckReq.cfgVersion.buf = Appchoice->rmtCfg.checkCfgReq.cfgVersion;
			cfgcheckReq.cfgVersion.size = Appchoice->rmtCfg.checkCfgReq.cfgVersionlen;
			//*/
			ec = uper_encode(pduType_Cfg_check_req,(void *) &cfgcheckReq,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON, "encode:appdata Cfg_check_req fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCFG_GET_REQ:
		{
			log_i(LOG_HOZON, "encode Cfg_check_req\n");
			CfgGetReqInfo_t cfgGetReq;
			memset(&cfgGetReq,0 , sizeof(CfgGetReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			cfgGetReq.cfgVersion.buf = Appchoice->rmtCfg.getCfgReq.cfgVersion_ptr;
			cfgGetReq.cfgVersion.size = Appchoice->rmtCfg.getCfgReq.cfgVersionlen;
			ec = uper_encode(pduType_Cfg_get_req,(void *) &cfgGetReq,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON, "encode:appdata Cfg_get_req fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCFG_END_REQ:
		{
			log_i(LOG_HOZON, "encode Cfg_end_request\n");
			CfgEndReqInfo_t CfgEndReq;
			memset(&CfgEndReq,0 , sizeof(CfgEndReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			CfgEndReq.configSuccess = Appchoice->rmtCfg.EndCfgReq.configSuccess;
			CfgEndReq.mcuSw.buf = Appchoice->rmtCfg.EndCfgReq.mcuSw_ptr;
			CfgEndReq.mcuSw.size = Appchoice->rmtCfg.EndCfgReq.mcuSwlen;
			CfgEndReq.cfgVersion.buf = Appchoice->rmtCfg.EndCfgReq.cfgVersion_ptr;
			CfgEndReq.cfgVersion.size = Appchoice->rmtCfg.EndCfgReq.cfgVersionlen;
			CfgEndReq.configSw.buf = Appchoice->rmtCfg.EndCfgReq.configSw_ptr;
			CfgEndReq.configSw.size = Appchoice->rmtCfg.EndCfgReq.configSwlen;
			CfgEndReq.mpuSw.buf = Appchoice->rmtCfg.EndCfgReq.mpuSw_ptr;
			CfgEndReq.mpuSw.size = Appchoice->rmtCfg.EndCfgReq.mpuSwlen;
			ec = uper_encode(pduType_Cfg_end_req,(void *) &CfgEndReq,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON,"encode:appdata Cfg_end_req fail\n");
				return -1;
			}
		}
		break;
		default:
		{
			log_e(LOG_HOZON, "unknow application request");
		}
		break;
	}
	
/*********************************************
				编码
*********************************************/
	DisptrBody->appDataLen = tboxAppdataLen;
	protocol_dump(LOG_HOZON, "uper encode:appdata", tboxAppdata,tboxAppdataLen, 0);
	log_i(LOG_HOZON, "uper encode appdata end");
	
	log_i(LOG_HOZON, "uper encode:dis body");
	key = PP_ENCODE_DISBODY;
	ec = uper_encode(pduType_Body,(void *) &Bodydata,PrvtPro_writeout,&key);
	log_i(LOG_HOZON, "uper encode dis body ec.encoded = %d",ec.encoded);
	if(ec.encoded  == -1)
	{
		log_e(LOG_HOZON, "Could not encode MessageFrame");
		return -1;
	}
	protocol_dump(LOG_HOZON, "uper encode:dis body", tboxDisBodydata, tboxDisBodydataLen, 0);
	log_i(LOG_HOZON, "uper encode dis body end");
	
/*********************************************
				填充 message data
*********************************************/	
	int tboxmsglen = 0;
	msgData[tboxmsglen++] = tboxDisBodydataLen +1;//填充 dispatcher header
	for(i = 0;i < tboxDisBodydataLen;i++)
	{
		msgData[tboxmsglen++]= tboxDisBodydata[i];
	}
	for(i = 0;i < tboxAppdataLen;i++)
	{
		msgData[tboxmsglen++]= tboxAppdata[i];
	}
	*msgDataLen = 1 + tboxDisBodydataLen + tboxAppdataLen;//填充 message data lengtn
	
/*********************************************
				解码
*********************************************/	
#if 1
	PrvtPro_decodeMsgData(msgData,tboxDisBodydataLen+1+tboxAppdataLen,NULL,1);
#endif	
	return 0;
}

/******************************************************
*函数名：PrvtPro_decodeMsgData

*形  参：

*返回值：

*描  述：解码message data

*备  注：
******************************************************/
int PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen,void *MsgData,int isdecodeAppdata)
{
	PrvtProt_msgData_t *msgData = (PrvtProt_msgData_t*)MsgData;
	asn_dec_rval_t dc;
	asn_codec_ctx_t *asn_codec_ctx = 0 ;
	Bodyinfo_t RxBodydata;
	Bodyinfo_t *RxBodydata_ptr = &RxBodydata;
	int i;
	memset(&RxBodydata,0 , sizeof(Bodyinfo_t));
	uint16_t AID;
	uint8_t MID;
	log_i(LOG_HOZON, "uper decode");
	log_i(LOG_HOZON, "uper decode:bodydata");
	log_i(LOG_HOZON, "dis header length = %d",LeMessageData[0]);
	dc = uper_decode(asn_codec_ctx,pduType_Body,(void *) &RxBodydata_ptr, \
					 &LeMessageData[1],LeMessageData[0] -1,0,0);
	if(dc.code  != RC_OK)
	{
		log_e(LOG_HOZON, "Could not decode dispatcher header Frame");
		return -1;
	}
	
	if(msgData != NULL)
	{
		msgData->DisBody.aID[0] = RxBodydata.aID.buf[0];
		msgData->DisBody.aID[1] = RxBodydata.aID.buf[1];
		msgData->DisBody.aID[2] = RxBodydata.aID.buf[2];
		msgData->DisBody.mID 	= RxBodydata.mID;
		msgData->DisBody.eventTime 	= RxBodydata.eventTime;
		log_i(LOG_HOZON, "RxBodydata.aid = %s",msgData->DisBody.aID);
		log_i(LOG_HOZON, "RxBodydata.mID = %d",RxBodydata.mID);
		log_i(LOG_HOZON, "RxBodydata.eventTime = %d",RxBodydata.eventTime);
		if(NULL != RxBodydata.expirationTime)
		{
			log_i(LOG_HOZON, "RxBodydata.expirationTime = %d",(*(RxBodydata.expirationTime)));
			msgData->DisBody.expTime = *(RxBodydata.expirationTime);/* OPTIONAL */
		}
		if(NULL != RxBodydata.eventId)
		{
			log_i(LOG_HOZON, "RxBodydata.eventId = %d",(*(RxBodydata.eventId)));
			msgData->DisBody.eventId = *(RxBodydata.eventId);/* OPTIONAL */
		}
		if(NULL != RxBodydata.ulMsgCnt)
		{
			msgData->DisBody.ulMsgCnt = *(RxBodydata.ulMsgCnt);/* OPTIONAL */
			log_i(LOG_HOZON, "RxBodydata.ulMsgCnt = %d",(*(RxBodydata.ulMsgCnt)));
		}
		if(NULL != RxBodydata.dlMsgCnt)
		{
			log_i(LOG_HOZON, "RxBodydata.dlMsgCnt = %d",(*(RxBodydata.dlMsgCnt)));
			msgData->DisBody.dlMsgCnt = *(RxBodydata.dlMsgCnt);/* OPTIONAL */
		}
		if(NULL != RxBodydata.msgCntAcked)
		{
			log_i(LOG_HOZON, "RxBodydata.msgCntAcked = %d",(*(RxBodydata.msgCntAcked)));
			msgData->DisBody.msgCntAcked = *(RxBodydata.msgCntAcked);/* OPTIONAL */
		}
		if(NULL != RxBodydata.ackReq)
		{
			log_i(LOG_HOZON, "RxBodydata.ackReq = %d",(*(RxBodydata.ackReq)));
			msgData->DisBody.ackReq	= *(RxBodydata.ackReq);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataLen)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataLen = %d",(*(RxBodydata.appDataLen)));
			msgData->DisBody.appDataLen	= *(RxBodydata.appDataLen);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataEncode)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataEncode = %d",(*(RxBodydata.appDataEncode)));
			msgData->DisBody.appDataEncode	= *(RxBodydata.appDataEncode);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataProVer)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataProVer = %d",(*(RxBodydata.appDataProVer)));
			msgData->DisBody.appDataProVer	= *(RxBodydata.appDataProVer);/* OPTIONAL */
		}
		if(NULL != RxBodydata.testFlag)
		{
			log_i(LOG_HOZON, "RxBodydata.testFlag = %d",(*(RxBodydata.testFlag)));
			msgData->DisBody.testFlag = *(RxBodydata.testFlag);/* OPTIONAL */
		}
		if(NULL != RxBodydata.result)
		{
			log_i(LOG_HOZON, "RxBodydata.result = %d",(*(RxBodydata.result)));
			msgData->DisBody.result	= *(RxBodydata.result);/* OPTIONAL */
		}
	}

	if((isdecodeAppdata == 1) && ((LeMessageDataLen-LeMessageData[0]) > 0))
	{
		AID = (RxBodydata.aID.buf[0] - 0x30)*100 +  (RxBodydata.aID.buf[1] - 0x30)*10 + \
			  (RxBodydata.aID.buf[2] - 0x30);
		MID = RxBodydata.mID;
		log_i(LOG_HOZON, "uper decode:appdata");
		log_i(LOG_HOZON, "application data length = %d",LeMessageDataLen-LeMessageData[0]);
		switch(AID)
		{
			case PP_AID_XCALL:
			{
				if(PP_MID_XCALL_REQ == MID)//xcall req
				{
					XcallReqinfo_t RxXcallReq;
					XcallReqinfo_t *RxXcallReq_ptr = &RxXcallReq;
					memset(&RxXcallReq,0 , sizeof(XcallReqinfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_XcallReq,(void *) &RxXcallReq_ptr, \
							 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON, "Could not decode application data Frame");
						return -1;
					}

					if(NULL != msgData)
					{
						msgData->appData.Xcall.xcallType = RxXcallReq.xcallType;
					}
					else
					{
						PrvtPro_showMsgData(ECDC_XCALL_REQ,RxBodydata_ptr,RxXcallReq_ptr);
					}
				}
				else if(PP_MID_XCALL_RESP == MID)//xcall response
				{
					XcallRespinfo_t RxXcallResp;
					XcallRespinfo_t *RxXcallResp_ptr = &RxXcallResp;
					memset(&RxXcallResp,0 , sizeof(XcallRespinfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_XcallResp,(void *) &RxXcallResp_ptr, \
									 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON, "Could not decode xcall application data Frame\n");
						return -1;
					}

					if(NULL != msgData)
					{
						msgData->appData.Xcall.xcallType 	= RxXcallResp.xcallType;
						msgData->appData.Xcall.updataTime 	= RxXcallResp.updataTime;
						msgData->appData.Xcall.battSOCEx 	= RxXcallResp.battSOCEx;
						msgData->appData.Xcall.engineSt 	= RxXcallResp.engineSt;
						msgData->appData.Xcall.srsSt 		= RxXcallResp.srsSt;
						msgData->appData.Xcall.totalOdoMr 	= RxXcallResp.ttOdoMeter;
						msgData->appData.Xcall.gpsPos.altitude 	= 	 (**(RxXcallResp.gpsPos.list.array)).altitude;
						msgData->appData.Xcall.gpsPos.gpsSpeed 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSpeed;
						msgData->appData.Xcall.gpsPos.gpsSt 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSt;
						msgData->appData.Xcall.gpsPos.gpsTimestamp = (**(RxXcallResp.gpsPos.list.array)).gpsTimestamp;
						msgData->appData.Xcall.gpsPos.hdop 		=	 (**(RxXcallResp.gpsPos.list.array)).hdop;
						msgData->appData.Xcall.gpsPos.heading 	=	 (**(RxXcallResp.gpsPos.list.array)).heading;
						msgData->appData.Xcall.gpsPos.latitude 	=	 (**(RxXcallResp.gpsPos.list.array)).latitude;
						msgData->appData.Xcall.gpsPos.longitude =	 (**(RxXcallResp.gpsPos.list.array)).longitude;
					}
					else
					{
						PrvtPro_showMsgData(ECDC_XCALL_RESP,RxBodydata_ptr,RxXcallResp_ptr);
					}
				}
				else
				{}
			}
			break;
			case PP_AID_RMTCFG:
			{
				if(PP_MID_CHECK_CFG_RESP == MID)//check cfg resp
				{
					log_i(LOG_HOZON, "config check response\n");
					CfgCheckRespInfo_t cfgcheckResp;
					CfgCheckRespInfo_t *cfgcheckResp_ptr = &cfgcheckResp;
					memset(&cfgcheckResp,0 , sizeof(CfgCheckRespInfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_Cfg_check_resp,(void *) &cfgcheckResp_ptr, \
									 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON,  "Could not decode cfg check response application data Frame\n");
						return -1;
					}

					if(NULL != msgData)
					{
						msgData->appData.rmtCfg.checkCfgResp.needUpdate = cfgcheckResp.needUpdate;
						for(i = 0;i < cfgcheckResp.cfgVersion->size;i++)
						{
							msgData->appData.rmtCfg.checkCfgResp.cfgVersion[i] = cfgcheckResp.cfgVersion->buf[i];
						}
						log_i(LOG_HOZON, "checkCfgResp.needUpdate = %d\n",msgData->appData.rmtCfg.checkCfgResp.needUpdate);
						log_i(LOG_HOZON, "checkCfgResp.cfgVersion = %s\n",msgData->appData.rmtCfg.checkCfgResp.cfgVersion);
					}
				}
				else if(PP_MID_GET_CFG_RESP == MID)//get cfg resp
				{
					log_i(LOG_HOZON,  "get config resp\n");
					CfgGetRespInfo_t cfggetResp;
					CfgGetRespInfo_t *cfggetResp_ptr = &cfggetResp;
					memset(&cfggetResp,0 , sizeof(CfgGetRespInfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_Cfg_get_resp,(void *) &cfggetResp_ptr, \
									 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON, "Could not decode get cfg resp application data Frame\n");
						return -1;
					}

					if(NULL != msgData)
					{
						msgData->appData.rmtCfg.getCfgResp.result = cfggetResp.result;
						log_i(LOG_HOZON, "getCfgResp.result = %d\n",msgData->appData.rmtCfg.getCfgResp.result);
						if(cfggetResp.ficmCfg != NULL)
						{
							for(i = 0;i < (**(cfggetResp.ficmCfg->list.array)).token.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.token[i] = (**(cfggetResp.ficmCfg->list.array)).token.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tokenlen = (**(cfggetResp.ficmCfg->list.array)).token.size;
							}
							msgData->appData.rmtCfg.getCfgResp.ficmConfigValid = 1;
							log_i(LOG_HOZON, "getCfgResp.token = %s\n",msgData->appData.rmtCfg.getCfgResp.token);
							log_i(LOG_HOZON, "getCfgResp.tokenlen = %d\n",msgData->appData.rmtCfg.getCfgResp.tokenlen);
						}

						if(cfggetResp.apn1Config != NULL)
						{
							msgData->appData.rmtCfg.getCfgResp.apn1ConfigValid =1;
							for(i = 0;i < (**(cfggetResp.apn1Config->list.array)).tspAddress.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.tspAddr[i] = (**(cfggetResp.apn1Config->list.array)).tspAddress.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tspAddrlen = (**(cfggetResp.apn1Config->list.array)).tspAddress.size;
							}
							log_i(LOG_HOZON, "getCfgResp.tspAddr = %s\n",msgData->appData.rmtCfg.getCfgResp.tspAddr);
							log_i(LOG_HOZON, "getCfgResp.tspAddrlen = %d\n",msgData->appData.rmtCfg.getCfgResp.tspAddrlen);
							for(i = 0;i < (**(cfggetResp.apn1Config->list.array)).tspPass.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.tspPass[i] = (**(cfggetResp.apn1Config->list.array)).tspPass.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tspPasslen = (**(cfggetResp.apn1Config->list.array)).tspPass.size;
							}
							log_i(LOG_HOZON, "getCfgResp.tspPass = %s\n",msgData->appData.rmtCfg.getCfgResp.tspPass);
							log_i(LOG_HOZON, "getCfgResp.tspPasslen = %d\n",msgData->appData.rmtCfg.getCfgResp.tspPasslen);
							for(i = 0;i < (**(cfggetResp.apn1Config->list.array)).tspIp.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.tspIP[i] = (**(cfggetResp.apn1Config->list.array)).tspIp.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tspIPlen = (**(cfggetResp.apn1Config->list.array)).tspIp.size;
							}
							log_i(LOG_HOZON, "getCfgResp.tspIP = %s\n",msgData->appData.rmtCfg.getCfgResp.tspIP);
							log_i(LOG_HOZON, "getCfgResp.tspIPlen = %d\n",msgData->appData.rmtCfg.getCfgResp.tspIPlen);
							for(i = 0;i < (**(cfggetResp.apn1Config->list.array)).tspSms.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.tspSms[i] = (**(cfggetResp.apn1Config->list.array)).tspSms.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tspSmslen = (**(cfggetResp.apn1Config->list.array)).tspSms.size;
							}
							log_i(LOG_HOZON, "getCfgResp.tspSms = %s\n",msgData->appData.rmtCfg.getCfgResp.tspSms);
							log_i(LOG_HOZON, "getCfgResp.tspSmslen = %d\n",msgData->appData.rmtCfg.getCfgResp.tspSmslen);
							for(i = 0;i < (**(cfggetResp.apn1Config->list.array)).tspPort.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.tspPort[i] = (**(cfggetResp.apn1Config->list.array)).tspPort.buf[i];
								msgData->appData.rmtCfg.getCfgResp.tspPortlen = (**(cfggetResp.apn1Config->list.array)).tspPort.size;
							}
							log_i(LOG_HOZON, "getCfgResp.tspPort = %s\n",msgData->appData.rmtCfg.getCfgResp.tspPort);
							log_i(LOG_HOZON, "getCfgResp.tspPortlen = %d\n",msgData->appData.rmtCfg.getCfgResp.tspPortlen);
						}

						if(cfggetResp.apn2Config != NULL)
						{
							msgData->appData.rmtCfg.getCfgResp.apn2ConfigValid = 1;
							for(i = 0;i < (**(cfggetResp.apn2Config->list.array)).tspAddress.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.apn2Address[i] = (**(cfggetResp.apn2Config->list.array)).tspAddress.buf[i];
								msgData->appData.rmtCfg.getCfgResp.apn2Addresslen = (**(cfggetResp.apn2Config->list.array)).tspAddress.size;
							}
							log_i(LOG_HOZON, "getCfgResp.apn2Address = %s\n",msgData->appData.rmtCfg.getCfgResp.apn2Address);
							log_i(LOG_HOZON, "getCfgResp.apn2Addresslen = %d\n",msgData->appData.rmtCfg.getCfgResp.apn2Addresslen);
							for(i = 0;i < (**(cfggetResp.apn2Config->list.array)).tspUser.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.apn2User[i] = (**(cfggetResp.apn2Config->list.array)).tspUser.buf[i];
								msgData->appData.rmtCfg.getCfgResp.apn2Userlen = (**(cfggetResp.apn2Config->list.array)).tspUser.size;
							}
							log_i(LOG_HOZON, "getCfgResp.apn2User = %s\n",msgData->appData.rmtCfg.getCfgResp.apn2User);
							log_i(LOG_HOZON, "getCfgResp.apn2Userlen = %d\n",msgData->appData.rmtCfg.getCfgResp.apn2Userlen);
							for(i = 0;i < (**(cfggetResp.apn2Config->list.array)).tspPass.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.apn2Pass[i] = (**(cfggetResp.apn2Config->list.array)).tspPass.buf[i];
								msgData->appData.rmtCfg.getCfgResp.apn2Passlen = (**(cfggetResp.apn2Config->list.array)).tspPass.size;
							}
							log_i(LOG_HOZON, "getCfgResp.apn2Pass = %s\n",msgData->appData.rmtCfg.getCfgResp.apn2Pass);
							log_i(LOG_HOZON, "getCfgResp.apn2Passlen = %d\n",msgData->appData.rmtCfg.getCfgResp.apn2Passlen);
						}

						if(cfggetResp.commonConfig != NULL)
						{
							msgData->appData.rmtCfg.getCfgResp.commonConfigValid = 1;
							msgData->appData.rmtCfg.getCfgResp.actived = (**(cfggetResp.commonConfig->list.array)).actived;
							msgData->appData.rmtCfg.getCfgResp.bCallEnabled = (**(cfggetResp.commonConfig->list.array)).bCallEnabled;
							msgData->appData.rmtCfg.getCfgResp.btKeyEntryEnabled = (**(cfggetResp.commonConfig->list.array)).btKeyEntryEnabled;
							msgData->appData.rmtCfg.getCfgResp.dcEnabled = (**(cfggetResp.commonConfig->list.array)).dcEnabled;
							msgData->appData.rmtCfg.getCfgResp.dtcEnabled = (**(cfggetResp.commonConfig->list.array)).dtcEnabled;
							msgData->appData.rmtCfg.getCfgResp.eCallEnabled = (**(cfggetResp.commonConfig->list.array)).eCallEnabled;
							msgData->appData.rmtCfg.getCfgResp.iCallEnabled = (**(cfggetResp.commonConfig->list.array)).iCallEnabled;
							msgData->appData.rmtCfg.getCfgResp.journeysEnabled = (**(cfggetResp.commonConfig->list.array)).journeysEnabled;
							msgData->appData.rmtCfg.getCfgResp.onlineInfEnabled = (**(cfggetResp.commonConfig->list.array)).onlineInfEnabled;
							msgData->appData.rmtCfg.getCfgResp.rChargeEnabled = (**(cfggetResp.commonConfig->list.array)).rChargeEnabled;
							msgData->appData.rmtCfg.getCfgResp.rcEnabled = (**(cfggetResp.commonConfig->list.array)).rcEnabled;
							msgData->appData.rmtCfg.getCfgResp.svtEnabled = (**(cfggetResp.commonConfig->list.array)).svtEnabled;
							msgData->appData.rmtCfg.getCfgResp.vsEnabled = (**(cfggetResp.commonConfig->list.array)).vsEnabled;

							log_i(LOG_HOZON, "getCfgResp.actived = %d\n",msgData->appData.rmtCfg.getCfgResp.actived);
							log_i(LOG_HOZON, "getCfgResp.bCallEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.bCallEnabled);
							log_i(LOG_HOZON, "getCfgResp.btKeyEntryEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.btKeyEntryEnabled);
							log_i(LOG_HOZON, "getCfgResp.dcEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.dcEnabled);
							log_i(LOG_HOZON, "getCfgResp.dtcEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.dtcEnabled);
							log_i(LOG_HOZON, "getCfgResp.eCallEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.eCallEnabled);
							log_i(LOG_HOZON, "getCfgResp.iCallEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.iCallEnabled);
							log_i(LOG_HOZON, "getCfgResp.journeysEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.journeysEnabled);
							log_i(LOG_HOZON, "getCfgResp.onlineInfEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.onlineInfEnabled);
							log_i(LOG_HOZON, "getCfgResp.rChargeEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.rChargeEnabled);
							log_i(LOG_HOZON, "getCfgResp.rcEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.rcEnabled);
							log_i(LOG_HOZON, "getCfgResp.svtEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.svtEnabled);
							log_i(LOG_HOZON, "getCfgResp.vsEnabled = %d\n",msgData->appData.rmtCfg.getCfgResp.vsEnabled);
						}

						if(cfggetResp.extendConfig != NULL)
						{
							msgData->appData.rmtCfg.getCfgResp.extendConfigValid = 1;
							for(i = 0;i < (**(cfggetResp.extendConfig->list.array)).bcallNO.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.bcallNO[i] = (**(cfggetResp.extendConfig->list.array)).bcallNO.buf[i];
								msgData->appData.rmtCfg.getCfgResp.bcallNOlen = (**(cfggetResp.extendConfig->list.array)).bcallNO.size;
							}
							log_i(LOG_HOZON, "getCfgResp.bcallNO = %s\n",msgData->appData.rmtCfg.getCfgResp.bcallNO);
							log_i(LOG_HOZON, "getCfgResp.bcallNOlen = %d\n",msgData->appData.rmtCfg.getCfgResp.bcallNOlen);
							for(i = 0;i < (**(cfggetResp.extendConfig->list.array)).ecallNO.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.ecallNO[i] = (**(cfggetResp.extendConfig->list.array)).ecallNO.buf[i];
								msgData->appData.rmtCfg.getCfgResp.ecallNOlen = (**(cfggetResp.extendConfig->list.array)).ecallNO.size;
							}
							log_i(LOG_HOZON, "getCfgResp.ecallNO = %s\n",msgData->appData.rmtCfg.getCfgResp.ecallNO);
							log_i(LOG_HOZON, "getCfgResp.ecallNOlen = %d\n",msgData->appData.rmtCfg.getCfgResp.ecallNOlen);
							for(i = 0;i < (**(cfggetResp.extendConfig->list.array)).icallNO.size;i++)
							{
								msgData->appData.rmtCfg.getCfgResp.ccNO[i] = (**(cfggetResp.extendConfig->list.array)).icallNO.buf[i];
								msgData->appData.rmtCfg.getCfgResp.ccNOlen = (**(cfggetResp.extendConfig->list.array)).icallNO.size;
							}
							log_i(LOG_HOZON, "getCfgResp.ccNONO = %s\n",msgData->appData.rmtCfg.getCfgResp.ccNO);
							log_i(LOG_HOZON, "getCfgResp.ccNOlen = %d\n",msgData->appData.rmtCfg.getCfgResp.ccNOlen);
						}
					}
				}
				else
				{

				}
			}
			break;
			default:
			break;
		}
	}
	
	log_i(LOG_HOZON, "uper decode end");
	return 0;
}

/******************************************************
*函数名：PrvtPro_showMsgData

*形  参：

*返回值：

*描  述：message data 打印

*备  注：
******************************************************/
static void PrvtPro_showMsgData(uint8_t type,Bodyinfo_t *RxBodydata,void *RxAppdata)
{
	uint16_t aid;
	
	aid = (RxBodydata->aID.buf[0] - 0x30)*100 +  (RxBodydata->aID.buf[1] - 0x30)*10 + \
		  (RxBodydata->aID.buf[2] - 0x30);
	log_i(LOG_HOZON, "RxBodydata.aid = %d",aid);
	log_i(LOG_HOZON, "RxBodydata.mID = %d",RxBodydata->mID);
	log_i(LOG_HOZON, "RxBodydata.eventTime = %d",RxBodydata->eventTime);
	if(NULL != RxBodydata->expirationTime)
	{
		log_i(LOG_HOZON, "RxBodydata.expirationTime = %d",(*(RxBodydata->expirationTime)));
	}
	if(NULL != RxBodydata->eventId)
	{
		log_i(LOG_HOZON, "RxBodydata.eventId = %d",(*(RxBodydata->eventId)));
	}
	if(NULL != RxBodydata->ulMsgCnt)
	{
		log_i(LOG_HOZON, "RxBodydata.ulMsgCnt = %d",(*(RxBodydata->ulMsgCnt)));
	}
	if(NULL != RxBodydata->dlMsgCnt)
	{
		log_i(LOG_HOZON, "RxBodydata.dlMsgCnt = %d",(*(RxBodydata->dlMsgCnt)));
	}
	if(NULL != RxBodydata->msgCntAcked)
	{
		log_i(LOG_HOZON, "RxBodydata.msgCntAcked = %d",(*(RxBodydata->msgCntAcked)));
	}
	if(NULL != RxBodydata->ackReq)
	{
		log_i(LOG_HOZON, "RxBodydata.ackReq = %d",(*(RxBodydata->ackReq)));
	}
	if(NULL != RxBodydata->appDataLen)
	{
		log_i(LOG_HOZON, "RxBodydata.appDataLen = %d",(*(RxBodydata->appDataLen)));
	}
	if(NULL != RxBodydata->appDataEncode)
	{
		log_i(LOG_HOZON, "RxBodydata.appDataEncode = %d",(*(RxBodydata->appDataEncode)));
	}
	if(NULL != RxBodydata->appDataProVer)
	{
		log_i(LOG_HOZON, "RxBodydata.appDataProVer = %d",(*(RxBodydata->appDataProVer)));
	}
	if(NULL != RxBodydata->testFlag)
	{
		log_i(LOG_HOZON, "RxBodydata.testFlag = %d",(*(RxBodydata->testFlag)));
	}
	if(NULL != RxBodydata->result)
	{
		log_i(LOG_HOZON, "RxBodydata.result = %d",(*(RxBodydata->result)));
	}
	
	if(NULL != RxAppdata)
	{
		switch(type)
		{
			case ECDC_XCALL_REQ:
			{
				log_i(LOG_HOZON, "xcallReq.xcallType = %d",((XcallReqinfo_t *)(RxAppdata))->xcallType);
			}
			break;
			case ECDC_XCALL_RESP:
			{
				log_i(LOG_HOZON,"RxAppdata.xcallType = %d\n",((XcallRespinfo_t *)(RxAppdata))->xcallType);
				log_i(LOG_HOZON,"RxAppdata.engineSt = %d\n",((XcallRespinfo_t *)(RxAppdata))->engineSt);
				log_i(LOG_HOZON,"RxAppdata.ttOdoMeter = %d\n",((XcallRespinfo_t *)(RxAppdata))->ttOdoMeter);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.gpsSt = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).gpsSt);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.gpsTimestamp = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).gpsTimestamp);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.latitude = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).latitude);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.longitude = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).longitude);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.altitude = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).altitude);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.heading = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).heading);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.gpsSpeed = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).gpsSpeed);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.gpsTimestamp = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).gpsTimestamp);
				log_i(LOG_HOZON,"RxAppdata.gpsPos.hdop = %d\n",(**(((XcallRespinfo_t *)(RxAppdata))->gpsPos.list.array)).hdop);	//
				log_i(LOG_HOZON,"RxAppdata.srsSt = %d\n",((XcallRespinfo_t *)(RxAppdata))->srsSt);
				log_i(LOG_HOZON,"RxAppdata.updataTime = %d\n",((XcallRespinfo_t *)(RxAppdata))->updataTime);
				log_i(LOG_HOZON,"RxAppdata.battSOCEx = %d\n",((XcallRespinfo_t *)(RxAppdata))->battSOCEx);
			}
			break;
			default:
			break;
		}
	}
}

/******************************************************
*函数名：static void PrvtPro_writeout

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
static int PrvtPro_writeout(const void *buffer,size_t size,void *key)
{
	int i;
	log_i(LOG_HOZON, "PrvtPro_writeout <<<");
	if(size > PP_ECDC_DATA_LEN)
	{
		log_i(LOG_HOZON, "the size of data greater than PP_MSG_DATA_LEN");
		return 0;
	}
	
	switch(*((uint8_t*)key))
	{
		case PP_ENCODE_DISBODY:
		{
			log_i(LOG_HOZON, "PP_ENCODE_DISBODY <<<");
			for(i = 0;i < size;i++)
			{
				tboxDisBodydata[i] = ((unsigned char *)buffer)[i];
			}
			tboxDisBodydataLen = size;
		}
		break;
		case PP_ENCODE_APPDATA:
		{
			log_i(LOG_HOZON, "PP_ENCODE_APPDATA <<<");
			for(i = 0;i < size;i++)
			{
				tboxAppdata[i] = ((unsigned char *)buffer)[i];
			}
			tboxAppdataLen = size;
		}
		break;
		default:
		break;
	}
	return 0;
}
