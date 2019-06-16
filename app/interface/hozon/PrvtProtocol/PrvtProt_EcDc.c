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
#include "CfgConnRespInfo.h"
#include "CfgReadReqInfo.h"
#include "CfgReadRespInfo.h"
#include "RmtCtrlReqInfo.h"
#include "RmtCtrlStRespInfo.h"
#include "BookingResp.h"
#include "VehicleStReqInfo.h"
#include "VehicleStRespInfo.h"
#include "VSgpspos.h"
#include "VSExtStatus.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt_xcall.h"
#include "PrvtProt_remoteConfig.h"
#include "PrvtProt.h"
#include "PrvtProt_EcDc.h"
#include "remoteControl/PP_rmtCtrl.h"
#include "PrvtProt_VehiSt.h"
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
static asn_TYPE_descriptor_t *pduType_Cfg_conn_resp = &asn_DEF_CfgConnRespInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_read_req = &asn_DEF_CfgReadReqInfo;
static asn_TYPE_descriptor_t *pduType_Cfg_read_resp = &asn_DEF_CfgReadRespInfo;
static asn_TYPE_descriptor_t *pduType_Ctrl_Req = &asn_DEF_RmtCtrlReqInfo;
static asn_TYPE_descriptor_t *pduType_Rmt_Ctrl_resp = &asn_DEF_RmtCtrlStRespInfo;
static asn_TYPE_descriptor_t *pduType_Rmt_Ctrl_Bookingresp = &asn_DEF_BookingResp;

static asn_TYPE_descriptor_t *pduType_VS_req = &asn_DEF_VehicleStReqInfo;
static asn_TYPE_descriptor_t *pduType_VS_resp = &asn_DEF_VehicleStRespInfo;

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
#if 0
static void PrvtPro_showMsgData(uint8_t type,Bodyinfo_t *RxBodydata,void *RxAppdata);
#endif
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
int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,int *msgDataLen, \
					   void *disptrBody, void *appchoice)
{
	static uint8_t key;
	Bodyinfo_t Bodydata;
	PrvtProt_DisptrBody_t 	*DisptrBody = (PrvtProt_DisptrBody_t*)disptrBody;
	//PrvtProt_appData_t		*Appchoice	= (PrvtProt_appData_t*)appchoice;
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
	log_i(LOG_HOZON, "Bodydata.aID = %s\n",Bodydata.aID.buf);
	Bodydata.aID.size = 3;

	Bodydata.mID = DisptrBody->mID;
	log_i(LOG_HOZON, "Bodydata.mID = %d\n",Bodydata.mID);

	Bodydata.eventTime 		= DisptrBody->eventTime;
	log_i(LOG_HOZON, "Bodydata.eventTime = %d\n",Bodydata.eventTime);

	Bodydata.expirationTime = &DisptrBody->expTime;	/* OPTIONAL */;
	log_i(LOG_HOZON, "Bodydata.expirationTime = %d\n",*Bodydata.expirationTime);

	Bodydata.eventId 		= &DisptrBody->eventId;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.eventId = %d\n",*Bodydata.eventId);

	Bodydata.ulMsgCnt 		= &DisptrBody->ulMsgCnt;	/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.ulMsgCnt = %d\n",*Bodydata.ulMsgCnt);

	Bodydata.dlMsgCnt 		= &DisptrBody->dlMsgCnt;	/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.dlMsgCnt = %d\n",*Bodydata.dlMsgCnt);

	Bodydata.msgCntAcked 	= &DisptrBody->msgCntAcked;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.msgCntAcked = %d\n",*Bodydata.msgCntAcked);

	Bodydata.ackReq			= &DisptrBody->ackReq;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.ackReq = %d\n",*Bodydata.ackReq);

	Bodydata.appDataLen 	= &DisptrBody->appDataLen;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.appDataLen = %d\n",*Bodydata.appDataLen);

	Bodydata.appDataEncode	= &DisptrBody->appDataEncode;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.appDataEncode = %d\n",*Bodydata.appDataEncode);

	Bodydata.appDataProVer	= &DisptrBody->appDataProVer;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.appDataProVer = %d\n",*Bodydata.appDataProVer);

	Bodydata.testFlag		= &DisptrBody->testFlag;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.testFlag = %d\n",*Bodydata.testFlag);

	Bodydata.result			= &DisptrBody->result;/* OPTIONAL */
	log_i(LOG_HOZON, "Bodydata.result = %d\n",*Bodydata.result);
	
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
			PrvtProt_App_Xcall_t *XcallReq_ptr = (PrvtProt_App_Xcall_t*)appchoice;
			memset(&XcallReq,0 , sizeof(XcallReqinfo_t));
			XcallReq.xcallType = XcallReq_ptr->xcallType;
			
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
			PrvtProt_App_Xcall_t *XcallResp_ptr = (PrvtProt_App_Xcall_t*)appchoice;
			memset(&XcallResp,0 , sizeof(XcallRespinfo_t));
			memset(&Rvspos,0 , sizeof(RvsposInfo_t));

			XcallResp.xcallType = XcallResp_ptr->xcallType;
			XcallResp.engineSt = XcallResp_ptr->engineSt;
			XcallResp.ttOdoMeter = XcallResp_ptr->totalOdoMr;
			Rvspos.gpsSt = XcallResp_ptr->gpsPos.gpsSt;//gps状态 0-无效；1-有效
			Rvspos.gpsTimestamp = XcallResp_ptr->gpsPos.gpsTimestamp;//gps时间戳
			Rvspos.latitude = XcallResp_ptr->gpsPos.latitude;//纬度 x 1000000,当GPS信号无效时，值为0
			Rvspos.longitude = XcallResp_ptr->gpsPos.longitude;//经度 x 1000000,当GPS信号无效时，值为0
			Rvspos.altitude = XcallResp_ptr->gpsPos.altitude;//高度（m）
			Rvspos.heading = XcallResp_ptr->gpsPos.heading;//车头方向角度，0为正北方向
			Rvspos.gpsSpeed = XcallResp_ptr->gpsPos.gpsSpeed;//速度 x 10，单位km/h
			Rvspos.hdop = XcallResp_ptr->gpsPos.hdop;//水平精度因子 x 10
			XcallResp.gpsPos.list.array = &Rvspos_ptr;
			XcallResp.gpsPos.list.size =0;
			XcallResp.gpsPos.list.count =1;

			XcallResp.srsSt = XcallResp_ptr->srsSt;
			XcallResp.updataTime = XcallResp_ptr->updataTime;
			XcallResp.battSOCEx = XcallResp_ptr->battSOCEx;

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
			log_i(LOG_HOZON, "encode Cfg_check_req\n");
			CfgCheckReqInfo_t cfgcheckReq;
			PrvtProt_App_rmtCfg_t *rmtCfgCheckReq_ptr = (PrvtProt_App_rmtCfg_t*)appchoice;
			memset(&cfgcheckReq,0 , sizeof(CfgCheckReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			cfgcheckReq.mcuSw.buf = rmtCfgCheckReq_ptr->checkReq.mcuSw;
			cfgcheckReq.mcuSw.size = rmtCfgCheckReq_ptr->checkReq.mcuSwlen;
			cfgcheckReq.mpuSw.buf = rmtCfgCheckReq_ptr->checkReq.mpuSw;
			cfgcheckReq.mpuSw.size = rmtCfgCheckReq_ptr->checkReq.mpuSwlen;
			cfgcheckReq.vehicleVIN.buf = rmtCfgCheckReq_ptr->checkReq.vehicleVin;
			cfgcheckReq.vehicleVIN.size = rmtCfgCheckReq_ptr->checkReq.vehicleVinlen;
			cfgcheckReq.iccID.buf = rmtCfgCheckReq_ptr->checkReq.iccID;
			cfgcheckReq.iccID.size = rmtCfgCheckReq_ptr->checkReq.iccIDlen;
			cfgcheckReq.btMacAddr.buf = rmtCfgCheckReq_ptr->checkReq.btMacAddr;
			cfgcheckReq.btMacAddr.size = rmtCfgCheckReq_ptr->checkReq.btMacAddrlen;
			cfgcheckReq.configSw.buf = rmtCfgCheckReq_ptr->checkReq.configSw;
			cfgcheckReq.configSw.size = rmtCfgCheckReq_ptr->checkReq.configSwlen;
			cfgcheckReq.cfgVersion.buf = rmtCfgCheckReq_ptr->checkReq.cfgVersion;
			cfgcheckReq.cfgVersion.size = rmtCfgCheckReq_ptr->checkReq.cfgVersionlen;

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
			log_i(LOG_HOZON, "encode Cfg_get_req\n");
			CfgGetReqInfo_t cfgGetReq;
			PrvtProt_App_rmtCfg_t *rmtCfgGetReq_ptr = (PrvtProt_App_rmtCfg_t*)appchoice;
			memset(&cfgGetReq,0 , sizeof(CfgGetReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			cfgGetReq.cfgVersion.buf = rmtCfgGetReq_ptr->getReq.cfgVersion;
			cfgGetReq.cfgVersion.size = rmtCfgGetReq_ptr->getReq.cfgVersionlen;
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
			log_i(LOG_HOZON, "encode Cfg_end_req\n");
			CfgEndReqInfo_t CfgEndReq;
			//PrvtProt_App_rmtCfg_t appdata;
			PrvtProt_App_rmtCfg_t *rmtCfgEndReq_ptr = (PrvtProt_App_rmtCfg_t*)appchoice;
			memset(&CfgEndReq,0 , sizeof(CfgEndReqInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			CfgEndReq.configSuccess = rmtCfgEndReq_ptr->EndReq.configSuccess;
			CfgEndReq.mcuSw.buf 	= rmtCfgEndReq_ptr->EndReq.mcuSw;
			CfgEndReq.mcuSw.size 	= rmtCfgEndReq_ptr->EndReq.mcuSwlen;
			CfgEndReq.cfgVersion.buf = rmtCfgEndReq_ptr->EndReq.cfgVersion;
			CfgEndReq.cfgVersion.size = rmtCfgEndReq_ptr->EndReq.cfgVersionlen;
			CfgEndReq.configSw.buf 	 = rmtCfgEndReq_ptr->EndReq.configSw;
			CfgEndReq.configSw.size  = rmtCfgEndReq_ptr->EndReq.configSwlen;
			CfgEndReq.mpuSw.buf  = rmtCfgEndReq_ptr->EndReq.mpuSw;
			CfgEndReq.mpuSw.size = rmtCfgEndReq_ptr->EndReq.mpuSwlen;
			ec = uper_encode(pduType_Cfg_end_req,(void *) &CfgEndReq,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON,"encode:appdata Cfg_end_req fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCFG_CONN_RESP:
		{
			log_i(LOG_HOZON, "encode Cfg_conn_req\n");
			CfgConnRespInfo_t CfgConnResp;
			//PrvtProt_App_rmtCfg_t appdata;
			PrvtProt_App_rmtCfg_t *rmtCfgConnResp_ptr = (PrvtProt_App_rmtCfg_t*)appchoice;
			memset(&CfgConnResp,0 , sizeof(CfgConnRespInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */
			CfgConnResp.configAccepted = rmtCfgConnResp_ptr->connResp.configAccepted;
			ec = uper_encode(pduType_Cfg_conn_resp,(void *) &CfgConnResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON,"encode:appdata Cfg_conn_req fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCFG_READ_RESP:
		{
			log_i(LOG_HOZON, "encode Cfg_read_resp\n");
			CfgReadRespInfo_t CfgReadResp;
			PrvtProt_App_rmtCfg_t *rmtCfgReadResp_ptr = (PrvtProt_App_rmtCfg_t*)appchoice;
			memset(&CfgReadResp,0 , sizeof(CfgReadRespInfo_t));

			//Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			CfgReadResp.result = rmtCfgReadResp_ptr->ReadResp.result;
			log_i(LOG_HOZON, "CfgReadResp.result = %d\n",CfgReadResp.result);
			CfgReadResp.cfgVersion.buf = rmtCfgReadResp_ptr->ReadResp.cfgVersion;
			CfgReadResp.cfgVersion.size = rmtCfgReadResp_ptr->ReadResp.cfgVersionlen;
			log_i(LOG_HOZON, "CfgReadResp.cfgVersion.buf = %s\n",CfgReadResp.cfgVersion.buf);
			log_i(LOG_HOZON, "CfgReadResp.cfgVersion.size = %d\n",CfgReadResp.cfgVersion.size);
			CfgReadResp.ficmConfig 	= NULL;
			CfgReadResp.apn1Config 	= NULL;
			CfgReadResp.apn2Config 	= NULL;
			CfgReadResp.commonConfig = NULL;
			CfgReadResp.extendConfig = NULL;

			FICMConfigSet_t FICMCfgSet;
			FICMConfigSet_t *FICMCfgSet_ptr = &FICMCfgSet;
			struct ficmConfig ficmConfig;
			APN1ConfigSet_t APN1ConfigSet;
			APN1ConfigSet_t *APN1ConfigSet_ptr = &APN1ConfigSet;
			struct apn1Config apn1Config;
			APN2ConfigSet_t APN2ConfigSet;
			APN2ConfigSet_t *APN2ConfigSet_ptr = &APN2ConfigSet;
			struct apn2Config apn2Config;
			CommonConfigSet_t CommonConfigSet;
			CommonConfigSet_t *CommonConfigSet_ptr = &CommonConfigSet;
			struct commonConfig commonConfig;
			ExtendConfigSet_t ExtendConfigSet;
			ExtendConfigSet_t *ExtendConfigSet_ptr = &ExtendConfigSet;
			struct extendConfig extendConfig;

			memset(&ExtendConfigSet,0 , sizeof(ExtendConfigSet_t));
			memset(&CommonConfigSet,0 , sizeof(CommonConfigSet_t));
			memset(&APN2ConfigSet,0 , sizeof(APN2ConfigSet_t));
			memset(&APN1ConfigSet,0 , sizeof(APN1ConfigSet_t));
			memset(&FICMCfgSet,0 , sizeof(FICMConfigSet_t));
			if(1 == CfgReadResp.result)
			{
				if((rmtCfgReadResp_ptr->ReadResp.readreq[0] == 1) && \
							(1 == rmtCfgReadResp_ptr->ReadResp.FICM.ficmConfigValid))
				{
					FICMCfgSet.token.buf = rmtCfgReadResp_ptr->ReadResp.FICM.token;
					FICMCfgSet.token.size = rmtCfgReadResp_ptr->ReadResp.FICM.tokenlen;
					FICMCfgSet.userID.buf =  rmtCfgReadResp_ptr->ReadResp.FICM.userID;
					FICMCfgSet.userID.size = rmtCfgReadResp_ptr->ReadResp.FICM.userIDlen;
					FICMCfgSet.directConnEnable = rmtCfgReadResp_ptr->ReadResp.FICM.directConnEnable;
					FICMCfgSet.address.buf = rmtCfgReadResp_ptr->ReadResp.FICM.address;
					FICMCfgSet.address.size = rmtCfgReadResp_ptr->ReadResp.FICM.addresslen;
					FICMCfgSet.port.buf = rmtCfgReadResp_ptr->ReadResp.FICM.port;
					FICMCfgSet.port.size = rmtCfgReadResp_ptr->ReadResp.FICM.portlen;

					log_i(LOG_HOZON, "FICMCfgSet.token.buf = %s\n",FICMCfgSet.token.buf);
					log_i(LOG_HOZON, "FICMCfgSet.token.size = %d\n",FICMCfgSet.token.size);
					log_i(LOG_HOZON, "FICMCfgSet.userID.buf = %s\n",FICMCfgSet.userID.buf);
					log_i(LOG_HOZON, "FICMCfgSet.userID.size = %d\n",FICMCfgSet.userID.size);
					log_i(LOG_HOZON, "FICMCfgSet.directConnEnable = %d\n",FICMCfgSet.directConnEnable);
					log_i(LOG_HOZON, "FICMCfgSet.address.buf = %s\n",FICMCfgSet.address.buf);
					log_i(LOG_HOZON, "FICMCfgSet.address.size = %d\n",FICMCfgSet.address.size);
					log_i(LOG_HOZON, "FICMCfgSet.port.buf = %s\n",FICMCfgSet.port.buf);
					log_i(LOG_HOZON, "FICMCfgSet.port.size = %d\n",FICMCfgSet.port.size);
					ficmConfig.list.array = &FICMCfgSet_ptr;
					ficmConfig.list.count = 1;
					ficmConfig.list.size = 1;
					CfgReadResp.ficmConfig = &ficmConfig;
				}

				if((rmtCfgReadResp_ptr->ReadResp.readreq[1] == 1) && \
						(1 == rmtCfgReadResp_ptr->ReadResp.APN1.apn1ConfigValid))
				{
					APN1ConfigSet.tspAddress.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspAddr;
					APN1ConfigSet.tspAddress.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspAddrlen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspAddress.buf = %s\n",APN1ConfigSet.tspAddress.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspAddress.size = %d\n",APN1ConfigSet.tspAddress.size);
					APN1ConfigSet.tspIp.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspIP;
					APN1ConfigSet.tspIp.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspIPlen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspIp.buf = %s\n",APN1ConfigSet.tspIp.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspIp.size = %d\n",APN1ConfigSet.tspIp.size);
					APN1ConfigSet.tspPass.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspPass;
					APN1ConfigSet.tspPass.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspPasslen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspPass.buf = %s\n",APN1ConfigSet.tspPass.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspPass.size = %d\n",APN1ConfigSet.tspPass.size);
					APN1ConfigSet.tspPort.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspPort;
					APN1ConfigSet.tspPort.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspPortlen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspPort.buf = %s\n",APN1ConfigSet.tspPort.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspPort.size = %d\n",APN1ConfigSet.tspPort.size);
					APN1ConfigSet.tspSms.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspSms;
					APN1ConfigSet.tspSms.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspSmslen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspSms.buf = %s\n",APN1ConfigSet.tspSms.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspSms.size = %d\n",APN1ConfigSet.tspSms.size);
					APN1ConfigSet.tspUser.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspUser;
					APN1ConfigSet.tspUser.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspUserlen;
					log_i(LOG_HOZON, "APN1ConfigSet.tspUser.buf = %s\n",APN1ConfigSet.tspUser.buf);
					log_i(LOG_HOZON, "APN1ConfigSet.tspUser.size = %d\n",APN1ConfigSet.tspUser.size);
					apn1Config.list.array = &APN1ConfigSet_ptr;
					apn1Config.list.count = 1;
					apn1Config.list.size = 1;
					CfgReadResp.apn1Config = &apn1Config;
				}

				if((rmtCfgReadResp_ptr->ReadResp.readreq[2] == 1) && \
						(1 == rmtCfgReadResp_ptr->ReadResp.APN2.apn2ConfigValid))
				{
					APN2ConfigSet.tspAddress.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Address;
					APN2ConfigSet.tspAddress.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Addresslen;
					log_i(LOG_HOZON, "APN2ConfigSet.tspAddress.buf = %s\n",APN2ConfigSet.tspAddress.buf);
					log_i(LOG_HOZON, "APN2ConfigSet.tspAddress.size = %d\n",APN2ConfigSet.tspAddress.size);
					APN2ConfigSet.tspPass.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Pass;
					APN2ConfigSet.tspPass.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Passlen;
					log_i(LOG_HOZON, "APN2ConfigSet.tspPass.buf = %s\n",APN2ConfigSet.tspPass.buf);
					log_i(LOG_HOZON, "APN2ConfigSet.tspPass.size = %d\n",APN2ConfigSet.tspPass.size);
					APN2ConfigSet.tspUser.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2User;
					APN2ConfigSet.tspUser.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Userlen;
					log_i(LOG_HOZON, "APN2ConfigSet.tspUser.buf = %s\n",APN2ConfigSet.tspUser.buf);
					log_i(LOG_HOZON, "APN2ConfigSet.tspUser.size = %d\n",APN2ConfigSet.tspUser.size);
					apn2Config.list.array = &APN2ConfigSet_ptr;
					apn2Config.list.count = 1;
					apn2Config.list.size = 1;
					CfgReadResp.apn2Config = &apn2Config;
				}

				if((rmtCfgReadResp_ptr->ReadResp.readreq[3] == 1) && \
						(1 == rmtCfgReadResp_ptr->ReadResp.COMMON.commonConfigValid))
				{
					CommonConfigSet.actived 		= rmtCfgReadResp_ptr->ReadResp.COMMON.actived;
					log_i(LOG_HOZON, "CommonConfigSet.actived = %d\n",CommonConfigSet.actived);
					CommonConfigSet.rcEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.rcEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.rcEnabled = %d\n",CommonConfigSet.rcEnabled);
					CommonConfigSet.svtEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.svtEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.svtEnabled = %d\n",CommonConfigSet.svtEnabled);
					CommonConfigSet.vsEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.vsEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.vsEnabled = %d\n",CommonConfigSet.vsEnabled);
					CommonConfigSet.iCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.iCallEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.iCallEnabled = %d\n",CommonConfigSet.iCallEnabled);
					CommonConfigSet.bCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.bCallEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.bCallEnabled = %d\n",CommonConfigSet.bCallEnabled);
					CommonConfigSet.eCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.eCallEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.eCallEnabled = %d\n",CommonConfigSet.eCallEnabled);
					CommonConfigSet.dcEnabled 	 	= rmtCfgReadResp_ptr->ReadResp.COMMON.dcEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.dcEnabled = %d\n",CommonConfigSet.dcEnabled);
					CommonConfigSet.dtcEnabled 	 	= rmtCfgReadResp_ptr->ReadResp.COMMON.dtcEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.dtcEnabled = %d\n",CommonConfigSet.dtcEnabled);
					CommonConfigSet.journeysEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.journeysEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.journeysEnabled = %d\n",CommonConfigSet.journeysEnabled);
					CommonConfigSet.onlineInfEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.onlineInfEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.onlineInfEnabled = %d\n",CommonConfigSet.onlineInfEnabled);
					CommonConfigSet.rChargeEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.rChargeEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.rChargeEnabled = %d\n",CommonConfigSet.rChargeEnabled);
					CommonConfigSet.btKeyEntryEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.btKeyEntryEnabled;
					log_i(LOG_HOZON, "CommonConfigSet.btKeyEntryEnabled = %d\n",CommonConfigSet.btKeyEntryEnabled);

					commonConfig.list.array = &CommonConfigSet_ptr;
					commonConfig.list.count =1;
					commonConfig.list.size = 1;
					CfgReadResp.commonConfig = &commonConfig;
				}

				if((rmtCfgReadResp_ptr->ReadResp.readreq[4] == 1) && \
						(1 == rmtCfgReadResp_ptr->ReadResp.EXTEND.extendConfigValid))
				{
					ExtendConfigSet.ecallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.ecallNO;
					ExtendConfigSet.ecallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.ecallNOlen;
					log_i(LOG_HOZON, "ExtendConfigSet.ecallNO.buf = %s\n",ExtendConfigSet.ecallNO.buf);
					log_i(LOG_HOZON, "ExtendConfigSet.ecallNO.size = %d\n",ExtendConfigSet.ecallNO.size);
					ExtendConfigSet.bcallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.bcallNO;
					ExtendConfigSet.bcallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.bcallNOlen;
					log_i(LOG_HOZON, "ExtendConfigSet.bcallNO.buf = %s\n",ExtendConfigSet.bcallNO.buf);
					log_i(LOG_HOZON, "ExtendConfigSet.bcallNO.size = %d\n",ExtendConfigSet.bcallNO.size);
					ExtendConfigSet.icallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.ccNO;
					ExtendConfigSet.icallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.ccNOlen;
					log_i(LOG_HOZON, "ExtendConfigSet.icallNO.buf = %s\n",ExtendConfigSet.icallNO.buf);
					log_i(LOG_HOZON, "ExtendConfigSet.icallNO.size = %d\n",ExtendConfigSet.icallNO.size);
					extendConfig.list.array = &ExtendConfigSet_ptr;
					extendConfig.list.count =1;
					extendConfig.list.size = 1;
					CfgReadResp.extendConfig = &extendConfig;
				}
			}

			ec = uper_encode(pduType_Cfg_read_resp,(void *) &CfgReadResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON,"encode:appdata Cfg_read_resp fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCTRL_RESP:
		{
			log_i(LOG_HOZON, "encode remote_control_resp\n");
			PrvtProt_App_rmtCtrl_t *rmtCtrlResp_ptr = (PrvtProt_App_rmtCtrl_t*)appchoice;
			RmtCtrlStRespInfo_t RmtCtrlResp;
			RmtRvsposInfo_t rmtCtrlRvspos;
			RmtRvsposInfo_t *rmtCtrlRvspos_ptr = &rmtCtrlRvspos;

			RvsBasicStatus_t	RvsBasicSt;
			RvsBasicStatus_t *RvsBasicSt_ptr= &RvsBasicSt;
			memset(&RmtCtrlResp,0 , sizeof(RmtCtrlStRespInfo_t));
			memset(&rmtCtrlRvspos,0 , sizeof(RmtRvsposInfo_t));
			memset(&RvsBasicSt,0 , sizeof(RvsBasicStatus_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			RmtCtrlResp.rvcReqType 			= rmtCtrlResp_ptr->CtrlResp.rvcReqType;
			RmtCtrlResp.rvcReqStatus 		= rmtCtrlResp_ptr->CtrlResp.rvcReqStatus;
			RmtCtrlResp.rvcFailureType 		= &(rmtCtrlResp_ptr->CtrlResp.rvcFailureType);

			rmtCtrlRvspos.gpsSt 			= rmtCtrlResp_ptr->CtrlResp.gpsPos.gpsSt;//gps状态 0-无效；1-有效
			rmtCtrlRvspos.gpsTimestamp 		= rmtCtrlResp_ptr->CtrlResp.gpsPos.gpsTimestamp;//gps时间戳
			rmtCtrlRvspos.latitude 			= rmtCtrlResp_ptr->CtrlResp.gpsPos.latitude;//纬度 x 1000000,当GPS信号无效时，值为0
			rmtCtrlRvspos.longitude 		= rmtCtrlResp_ptr->CtrlResp.gpsPos.longitude;//经度 x 1000000,当GPS信号无效时，值为0
			rmtCtrlRvspos.altitude 			= rmtCtrlResp_ptr->CtrlResp.gpsPos.altitude;//高度（m）
			rmtCtrlRvspos.heading 			= rmtCtrlResp_ptr->CtrlResp.gpsPos.heading;//车头方向角度，0为正北方向
			rmtCtrlRvspos.gpsSpeed 			= rmtCtrlResp_ptr->CtrlResp.gpsPos.gpsSpeed;//速度 x 10，单位km/h
			rmtCtrlRvspos.hdop 				= rmtCtrlResp_ptr->CtrlResp.gpsPos.hdop;//水平精度因子 x 10
			RmtCtrlResp.gpsPosition.list.array = &rmtCtrlRvspos_ptr;
			RmtCtrlResp.gpsPosition.list.size = 0;
			RmtCtrlResp.gpsPosition.list.count =1;

			RvsBasicSt.driverDoor 			= &rmtCtrlResp_ptr->CtrlResp.basicSt.driverDoor	/* OPTIONAL */;
			RvsBasicSt.driverLock 			= rmtCtrlResp_ptr->CtrlResp.basicSt.driverLock;
			RvsBasicSt.passengerDoor 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.passengerDoor	/* OPTIONAL */;
			RvsBasicSt.passengerLock 		= rmtCtrlResp_ptr->CtrlResp.basicSt.passengerLock;
			RvsBasicSt.rearLeftDoor 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearLeftDoor	/* OPTIONAL */;
			RvsBasicSt.rearLeftLock 		= rmtCtrlResp_ptr->CtrlResp.basicSt.rearLeftLock;
			RvsBasicSt.rearRightDoor 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearRightDoor	/* OPTIONAL */;
			RvsBasicSt.rearRightLock 		= rmtCtrlResp_ptr->CtrlResp.basicSt.rearRightLock;
			RvsBasicSt.bootStatus 			= &rmtCtrlResp_ptr->CtrlResp.basicSt.bootStatus	/* OPTIONAL */;
			RvsBasicSt.bootStatusLock 		= rmtCtrlResp_ptr->CtrlResp.basicSt.bootStatusLock;
			RvsBasicSt.driverWindow 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.driverWindow	/* OPTIONAL */;
			RvsBasicSt.passengerWindow 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.passengerWindow	/* OPTIONAL */;
			RvsBasicSt.rearLeftWindow 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearLeftWindow	/* OPTIONAL */;
			RvsBasicSt.rearRightWinow 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearRightWinow	/* OPTIONAL */;
			RvsBasicSt.sunroofStatus 		= &rmtCtrlResp_ptr->CtrlResp.basicSt.sunroofStatus	/* OPTIONAL */;
			RvsBasicSt.engineStatus 		= rmtCtrlResp_ptr->CtrlResp.basicSt.engineStatus;
			RvsBasicSt.accStatus 			= rmtCtrlResp_ptr->CtrlResp.basicSt.accStatus;
			RvsBasicSt.accTemp 				= &rmtCtrlResp_ptr->CtrlResp.basicSt.accTemp	/* OPTIONAL */;
			RvsBasicSt.accMode 				= &rmtCtrlResp_ptr->CtrlResp.basicSt.accMode	/* OPTIONAL */;
			RvsBasicSt.accBlowVolume		= &rmtCtrlResp_ptr->CtrlResp.basicSt.accBlowVolume/* OPTIONAL */;
			RvsBasicSt.innerTemp 			= rmtCtrlResp_ptr->CtrlResp.basicSt.innerTemp;
			RvsBasicSt.outTemp 				= rmtCtrlResp_ptr->CtrlResp.basicSt.outTemp;
			RvsBasicSt.sideLightStatus		= rmtCtrlResp_ptr->CtrlResp.basicSt.sideLightStatus;
			RvsBasicSt.dippedBeamStatus		= rmtCtrlResp_ptr->CtrlResp.basicSt.dippedBeamStatus;
			RvsBasicSt.mainBeamStatus		= rmtCtrlResp_ptr->CtrlResp.basicSt.mainBeamStatus;
			RvsBasicSt.hazardLightStus		= rmtCtrlResp_ptr->CtrlResp.basicSt.hazardLightStus;
			RvsBasicSt.frtRightTyrePre		= &rmtCtrlResp_ptr->CtrlResp.basicSt.frtRightTyrePre/* OPTIONAL */;
			RvsBasicSt.frtRightTyreTemp		= &rmtCtrlResp_ptr->CtrlResp.basicSt.frtRightTyreTemp/* OPTIONAL */;
			RvsBasicSt.frontLeftTyrePre		= &rmtCtrlResp_ptr->CtrlResp.basicSt.frontLeftTyrePre/* OPTIONAL */;
			RvsBasicSt.frontLeftTyreTemp	= &rmtCtrlResp_ptr->CtrlResp.basicSt.frontLeftTyreTemp	/* OPTIONAL */;
			RvsBasicSt.rearRightTyrePre		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearRightTyrePre/* OPTIONAL */;
			RvsBasicSt.rearRightTyreTemp	= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearRightTyreTemp	/* OPTIONAL */;
			RvsBasicSt.rearLeftTyrePre		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearLeftTyrePre/* OPTIONAL */;
			RvsBasicSt.rearLeftTyreTemp		= &rmtCtrlResp_ptr->CtrlResp.basicSt.rearLeftTyreTemp/* OPTIONAL */;
			RvsBasicSt.batterySOCExact		= rmtCtrlResp_ptr->CtrlResp.basicSt.batterySOCExact;
			RvsBasicSt.chargeRemainTim		= &rmtCtrlResp_ptr->CtrlResp.basicSt.chargeRemainTim/* OPTIONAL */;
			RvsBasicSt.availableOdomtr		= rmtCtrlResp_ptr->CtrlResp.basicSt.availableOdomtr;
			RvsBasicSt.engineRunningTime	= &rmtCtrlResp_ptr->CtrlResp.basicSt.engineRunningTime/* OPTIONAL */;
			RvsBasicSt.bookingChargeSt		= rmtCtrlResp_ptr->CtrlResp.basicSt.bookingChargeSt;
			RvsBasicSt.bookingChargeHour	= &rmtCtrlResp_ptr->CtrlResp.basicSt.bookingChargeHour	/* OPTIONAL */;
			RvsBasicSt.bookingChargeMin		= &rmtCtrlResp_ptr->CtrlResp.basicSt.bookingChargeMin/* OPTIONAL */;
			RvsBasicSt.chargeMode			= &rmtCtrlResp_ptr->CtrlResp.basicSt.chargeMode/* OPTIONAL */;
			RvsBasicSt.chargeStatus			= &rmtCtrlResp_ptr->CtrlResp.basicSt.chargeStatus/* OPTIONAL */;
			RvsBasicSt.powerMode			= &rmtCtrlResp_ptr->CtrlResp.basicSt.powerMode/* OPTIONAL */;
			RvsBasicSt.speed				= rmtCtrlResp_ptr->CtrlResp.basicSt.speed;
			RvsBasicSt.totalOdometer		= rmtCtrlResp_ptr->CtrlResp.basicSt.totalOdometer;
			RvsBasicSt.batteryVoltage		= rmtCtrlResp_ptr->CtrlResp.basicSt.batteryVoltage;
			RvsBasicSt.batteryCurrent		= rmtCtrlResp_ptr->CtrlResp.basicSt.batteryCurrent;
			RvsBasicSt.batterySOCPrc		= rmtCtrlResp_ptr->CtrlResp.basicSt.batterySOCPrc;
			RvsBasicSt.dcStatus				= rmtCtrlResp_ptr->CtrlResp.basicSt.dcStatus;
			RvsBasicSt.gearPosition			= rmtCtrlResp_ptr->CtrlResp.basicSt.gearPosition;
			RvsBasicSt.insulationRstance	= rmtCtrlResp_ptr->CtrlResp.basicSt.insulationRstance;
			RvsBasicSt.acceleratePedalprc	= rmtCtrlResp_ptr->CtrlResp.basicSt.acceleratePedalprc;
			RvsBasicSt.deceleratePedalprc	= rmtCtrlResp_ptr->CtrlResp.basicSt.deceleratePedalprc;
			RvsBasicSt.canBusActive			= rmtCtrlResp_ptr->CtrlResp.basicSt.canBusActive;
			RvsBasicSt.bonnetStatus			= rmtCtrlResp_ptr->CtrlResp.basicSt.bonnetStatus;
			RvsBasicSt.lockStatus			= rmtCtrlResp_ptr->CtrlResp.basicSt.lockStatus;
			RvsBasicSt.gsmStatus			= rmtCtrlResp_ptr->CtrlResp.basicSt.gsmStatus;
			RvsBasicSt.wheelTyreMotrSt		= &rmtCtrlResp_ptr->CtrlResp.basicSt.wheelTyreMotrSt	/* OPTIONAL */;
			RvsBasicSt.vehicleAlarmSt		= rmtCtrlResp_ptr->CtrlResp.basicSt.vehicleAlarmSt;
			RvsBasicSt.currentJourneyID		= rmtCtrlResp_ptr->CtrlResp.basicSt.currentJourneyID;
			RvsBasicSt.journeyOdom			= rmtCtrlResp_ptr->CtrlResp.basicSt.journeyOdom;
			RvsBasicSt.frtLeftSeatHeatLel	= &rmtCtrlResp_ptr->CtrlResp.basicSt.frtLeftSeatHeatLel	/* OPTIONAL */;
			RvsBasicSt.frtRightSeatHeatLel	= &rmtCtrlResp_ptr->CtrlResp.basicSt.frtRightSeatHeatLel/* OPTIONAL */;
			RvsBasicSt.airCleanerSt			= &rmtCtrlResp_ptr->CtrlResp.basicSt.airCleanerSt/* OPTIONAL */;
			RvsBasicSt.srsStatus			= rmtCtrlResp_ptr->CtrlResp.basicSt.srsStatus;
			RmtCtrlResp.basicVehicleStatus.list.array = &RvsBasicSt_ptr;
			RmtCtrlResp.basicVehicleStatus.list.size = 0;
			RmtCtrlResp.basicVehicleStatus.list.count =1;

			ec = uper_encode(pduType_Rmt_Ctrl_resp,(void *) &RmtCtrlResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_i(LOG_HOZON,  "encode:appdata remote_control_resp fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTCTRL_BOOKINGRESP:
		{
			log_i(LOG_HOZON, "encode Ctrl_booking_resp\n");
			BookingResp_t ctrlBookingResp;
			PrvtProt_App_rmtCtrl_t *rmtCtrlBookingResp_ptr = (PrvtProt_App_rmtCtrl_t*)appchoice;
			memset(&ctrlBookingResp,0 , sizeof(BookingResp_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */
			ctrlBookingResp.bookingId = rmtCtrlBookingResp_ptr->CtrlbookingResp.bookingId;
			ctrlBookingResp.oprTime = rmtCtrlBookingResp_ptr->CtrlbookingResp.oprTime;
			ctrlBookingResp.rvcReqCode = rmtCtrlBookingResp_ptr->CtrlbookingResp.rvcReqCode;
			ec = uper_encode(pduType_Rmt_Ctrl_Bookingresp,(void *) &ctrlBookingResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_i(LOG_HOZON,  "encode:appdata Ctrl_booking_resp fail\n");
				return -1;
			}
		}
		break;
		case ECDC_RMTVS_RESP:
		{
			log_i(LOG_HOZON, "encode remote_check_vs_resp\n");
			PrvtProt_App_VS_t *rmtVSResp_ptr = (PrvtProt_App_VS_t*)appchoice;
			VehicleStRespInfo_t VSResp;
			memset(&VSResp,0 , sizeof(VehicleStRespInfo_t));

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */

			VSgpspos_t VSgpspos;
			VSgpspos_t *VSgpspos_ptr = &VSgpspos;

			RvsBasicStatus_t	VSBasicSt;
			RvsBasicStatus_t *VSBasicSt_ptr= &VSBasicSt;

			struct exVehicleSt exVehicleSt;
			VSExtStatus_t VSExtSt;
			VSExtStatus_t *VSExtSt_ptr = &VSExtSt;
			OCTET_STRING_t	alertIds;

			memset(&VSResp,0 , sizeof(VehicleStRespInfo_t));
			memset(&VSgpspos,0 , sizeof(VSgpspos_t));
			memset(&VSBasicSt,0 , sizeof(RvsBasicStatus_t));
			memset(&VSExtSt,0 , sizeof(VSExtStatus_t));

			VSResp.statusTime = rmtVSResp_ptr->VSResp.statusTime;

			VSgpspos.gpsSt = rmtVSResp_ptr->VSResp.gpsPos.gpsSt;//gps状态 0-无效；1-有效
			VSgpspos.gpsTimestamp = rmtVSResp_ptr->VSResp.gpsPos.gpsTimestamp;//gps时间戳
			VSgpspos.latitude = rmtVSResp_ptr->VSResp.gpsPos.latitude;//纬度 x 1000000,当GPS信号无效时，值为0
			VSgpspos.longitude = rmtVSResp_ptr->VSResp.gpsPos.longitude;//经度 x 1000000,当GPS信号无效时，值为0
			VSgpspos.altitude = rmtVSResp_ptr->VSResp.gpsPos.altitude;//高度（m）
			VSgpspos.heading = rmtVSResp_ptr->VSResp.gpsPos.heading;//车头方向角度，0为正北方向
			VSgpspos.gpsSpeed = rmtVSResp_ptr->VSResp.gpsPos.gpsSpeed;//速度 x 10，单位km/h
			VSgpspos.hdop = rmtVSResp_ptr->VSResp.gpsPos.hdop;//水平精度因子 x 10
			VSResp.vsgpsPos.list.array = &VSgpspos_ptr;
			VSResp.vsgpsPos.list.size =0;
			VSResp.vsgpsPos.list.count =1;

			VSBasicSt.driverDoor 		= &rmtVSResp_ptr->VSResp.basicSt.driverDoor	/* OPTIONAL */;
			VSBasicSt.driverLock 		= rmtVSResp_ptr->VSResp.basicSt.driverLock;
			VSBasicSt.passengerDoor 	= &rmtVSResp_ptr->VSResp.basicSt.passengerDoor	/* OPTIONAL */;
			VSBasicSt.passengerLock 	= rmtVSResp_ptr->VSResp.basicSt.passengerLock;
			VSBasicSt.rearLeftDoor 		= &rmtVSResp_ptr->VSResp.basicSt.rearLeftDoor	/* OPTIONAL */;
			VSBasicSt.rearLeftLock 		= rmtVSResp_ptr->VSResp.basicSt.rearLeftLock;
			VSBasicSt.rearRightDoor 	= &rmtVSResp_ptr->VSResp.basicSt.rearRightDoor	/* OPTIONAL */;
			VSBasicSt.rearRightLock 	= rmtVSResp_ptr->VSResp.basicSt.rearRightLock;
			VSBasicSt.bootStatus 		= &rmtVSResp_ptr->VSResp.basicSt.bootStatus	/* OPTIONAL */;
			VSBasicSt.bootStatusLock 	= rmtVSResp_ptr->VSResp.basicSt.bootStatusLock;
			VSBasicSt.driverWindow 		= &rmtVSResp_ptr->VSResp.basicSt.driverWindow	/* OPTIONAL */;
			VSBasicSt.passengerWindow 	= &rmtVSResp_ptr->VSResp.basicSt.passengerWindow	/* OPTIONAL */;
			VSBasicSt.rearLeftWindow 	= &rmtVSResp_ptr->VSResp.basicSt.rearLeftWindow	/* OPTIONAL */;
			VSBasicSt.rearRightWinow 	= &rmtVSResp_ptr->VSResp.basicSt.rearRightWinow	/* OPTIONAL */;
			VSBasicSt.sunroofStatus 	= &rmtVSResp_ptr->VSResp.basicSt.sunroofStatus	/* OPTIONAL */;
			VSBasicSt.engineStatus 		= rmtVSResp_ptr->VSResp.basicSt.engineStatus;
			VSBasicSt.accStatus 		= rmtVSResp_ptr->VSResp.basicSt.accStatus;
			VSBasicSt.accTemp 			= &rmtVSResp_ptr->VSResp.basicSt.accTemp	/* OPTIONAL */;
			VSBasicSt.accMode 			= &rmtVSResp_ptr->VSResp.basicSt.accMode	/* OPTIONAL */;
			VSBasicSt.accBlowVolume		= &rmtVSResp_ptr->VSResp.basicSt.accBlowVolume/* OPTIONAL */;
			VSBasicSt.innerTemp			= rmtVSResp_ptr->VSResp.basicSt.innerTemp;
			VSBasicSt.outTemp 			= rmtVSResp_ptr->VSResp.basicSt.outTemp;
			VSBasicSt.sideLightStatus	= rmtVSResp_ptr->VSResp.basicSt.sideLightStatus;
			VSBasicSt.dippedBeamStatus	= rmtVSResp_ptr->VSResp.basicSt.dippedBeamStatus;
			VSBasicSt.mainBeamStatus	= rmtVSResp_ptr->VSResp.basicSt.mainBeamStatus;
			VSBasicSt.hazardLightStus	= rmtVSResp_ptr->VSResp.basicSt.hazardLightStus;
			VSBasicSt.frtRightTyrePre	= &rmtVSResp_ptr->VSResp.basicSt.frtRightTyrePre/* OPTIONAL */;
			VSBasicSt.frtRightTyreTemp	= &rmtVSResp_ptr->VSResp.basicSt.frtRightTyreTemp/* OPTIONAL */;
			VSBasicSt.frontLeftTyrePre	= &rmtVSResp_ptr->VSResp.basicSt.frontLeftTyrePre/* OPTIONAL */;
			VSBasicSt.frontLeftTyreTemp	= &rmtVSResp_ptr->VSResp.basicSt.frontLeftTyreTemp	/* OPTIONAL */;
			VSBasicSt.rearRightTyrePre	= &rmtVSResp_ptr->VSResp.basicSt.rearRightTyrePre/* OPTIONAL */;
			VSBasicSt.rearRightTyreTemp	= &rmtVSResp_ptr->VSResp.basicSt.rearRightTyreTemp	/* OPTIONAL */;
			VSBasicSt.rearLeftTyrePre	= &rmtVSResp_ptr->VSResp.basicSt.rearLeftTyrePre/* OPTIONAL */;
			VSBasicSt.rearLeftTyreTemp	= &rmtVSResp_ptr->VSResp.basicSt.rearLeftTyreTemp/* OPTIONAL */;
			VSBasicSt.batterySOCExact	= rmtVSResp_ptr->VSResp.basicSt.batterySOCExact;
			VSBasicSt.chargeRemainTim	= &rmtVSResp_ptr->VSResp.basicSt.chargeRemainTim/* OPTIONAL */;
			VSBasicSt.availableOdomtr	= rmtVSResp_ptr->VSResp.basicSt.availableOdomtr;
			VSBasicSt.engineRunningTime	= &rmtVSResp_ptr->VSResp.basicSt.engineRunningTime/* OPTIONAL */;
			VSBasicSt.bookingChargeSt	= rmtVSResp_ptr->VSResp.basicSt.bookingChargeSt;
			VSBasicSt.bookingChargeHour	= &rmtVSResp_ptr->VSResp.basicSt.bookingChargeHour	/* OPTIONAL */;
			VSBasicSt.bookingChargeMin	= &rmtVSResp_ptr->VSResp.basicSt.bookingChargeMin/* OPTIONAL */;
			VSBasicSt.chargeMode		= &rmtVSResp_ptr->VSResp.basicSt.chargeMode/* OPTIONAL */;
			VSBasicSt.chargeStatus		= &rmtVSResp_ptr->VSResp.basicSt.chargeStatus/* OPTIONAL */;
			VSBasicSt.powerMode			= &rmtVSResp_ptr->VSResp.basicSt.powerMode/* OPTIONAL */;
			VSBasicSt.speed				= rmtVSResp_ptr->VSResp.basicSt.speed;
			VSBasicSt.totalOdometer		= rmtVSResp_ptr->VSResp.basicSt.totalOdometer;
			VSBasicSt.batteryVoltage	= rmtVSResp_ptr->VSResp.basicSt.batteryVoltage;
			VSBasicSt.batteryCurrent	= rmtVSResp_ptr->VSResp.basicSt.batteryCurrent;
			VSBasicSt.batterySOCPrc		= rmtVSResp_ptr->VSResp.basicSt.batterySOCPrc;
			VSBasicSt.dcStatus			= rmtVSResp_ptr->VSResp.basicSt.dcStatus;
			VSBasicSt.gearPosition		= rmtVSResp_ptr->VSResp.basicSt.gearPosition;
			VSBasicSt.insulationRstance	= rmtVSResp_ptr->VSResp.basicSt.insulationRstance;
			VSBasicSt.acceleratePedalprc= rmtVSResp_ptr->VSResp.basicSt.acceleratePedalprc;
			VSBasicSt.deceleratePedalprc= rmtVSResp_ptr->VSResp.basicSt.deceleratePedalprc;
			VSBasicSt.canBusActive		= rmtVSResp_ptr->VSResp.basicSt.canBusActive;
			VSBasicSt.bonnetStatus		= rmtVSResp_ptr->VSResp.basicSt.bonnetStatus;
			VSBasicSt.lockStatus		= rmtVSResp_ptr->VSResp.basicSt.lockStatus;
			VSBasicSt.gsmStatus			= rmtVSResp_ptr->VSResp.basicSt.gsmStatus;
			VSBasicSt.wheelTyreMotrSt	= &rmtVSResp_ptr->VSResp.basicSt.wheelTyreMotrSt	/* OPTIONAL */;
			VSBasicSt.vehicleAlarmSt	= rmtVSResp_ptr->VSResp.basicSt.vehicleAlarmSt;
			VSBasicSt.currentJourneyID	= rmtVSResp_ptr->VSResp.basicSt.currentJourneyID;
			VSBasicSt.journeyOdom		= rmtVSResp_ptr->VSResp.basicSt.journeyOdom;
			VSBasicSt.frtLeftSeatHeatLel= &rmtVSResp_ptr->VSResp.basicSt.frtLeftSeatHeatLel	/* OPTIONAL */;
			VSBasicSt.frtRightSeatHeatLel	= &rmtVSResp_ptr->VSResp.basicSt.frtRightSeatHeatLel/* OPTIONAL */;
			VSBasicSt.airCleanerSt		= &rmtVSResp_ptr->VSResp.basicSt.airCleanerSt/* OPTIONAL */;
			VSBasicSt.srsStatus 		= rmtVSResp_ptr->VSResp.basicSt.srsStatus;
			VSResp.vehiclebasicSt.list.array = &VSBasicSt_ptr;
			VSResp.vehiclebasicSt.list.size =1;
			VSResp.vehiclebasicSt.list.count =1;

			if((rmtVSResp_ptr->VSResp.ExtSt.validFlg == 1) && \
					(rmtVSResp_ptr->VSResp.ExtSt.alertSize != 0))
			{
				VSExtSt.alertSize = rmtVSResp_ptr->VSResp.ExtSt.alertSize;
				alertIds.size = rmtVSResp_ptr->VSResp.ExtSt.alertSize;
				alertIds.buf = rmtVSResp_ptr->VSResp.ExtSt.alertIds;
				VSExtSt.alertIds = &alertIds;
				exVehicleSt.list.array = &VSExtSt_ptr;
				exVehicleSt.list.count = 1;
				exVehicleSt.list.size = 1;
				VSResp.exVehicleSt = &exVehicleSt;
			}
			else
			{
				VSResp.exVehicleSt = NULL;
			}

			ec = uper_encode(pduType_VS_resp,(void *) &VSResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_i(LOG_HOZON,  "encode:appdata rmt_VS_resp fail\n");
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
	PrvtProt_DisptrBody_t disbody;
	switch(type)
	{
		case ECDC_XCALL_REQ:
		case ECDC_XCALL_RESP:
		{
			PrvtProt_App_Xcall_t xcallappdata;
			PrvtPro_decodeMsgData(msgData,*msgDataLen,&disbody,&xcallappdata);
		}
		break;
		case ECDC_RMTCFG_CHECK_REQ:
		case ECDC_RMTCFG_GET_REQ:
		case ECDC_RMTCFG_END_REQ:
		case ECDC_RMTCFG_CONN_RESP:
		{
			PrvtProt_App_rmtCfg_t rmtCfgappdata;
			PrvtPro_decodeMsgData(msgData,*msgDataLen,&disbody,&rmtCfgappdata);
		}
		break;
	}
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
int PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen,void *DisBody,void *appData)
{
	PrvtProt_DisptrBody_t *msgDataBody = (PrvtProt_DisptrBody_t*)DisBody;
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
	
	if(msgDataBody != NULL)
	{
		memcpy(msgDataBody->aID,RxBodydata.aID.buf,sizeof(char)*3);
		msgDataBody->mID 	= RxBodydata.mID;
		msgDataBody->eventTime 	= RxBodydata.eventTime;
		log_i(LOG_HOZON, "RxBodydata.aid = %s\n",msgDataBody->aID);
		log_i(LOG_HOZON, "RxBodydata.mID = %d\n",RxBodydata.mID);
		log_i(LOG_HOZON, "RxBodydata.eventTime = %d\n",RxBodydata.eventTime);
		if(NULL != RxBodydata.expirationTime)
		{
			log_i(LOG_HOZON, "RxBodydata.expirationTime = %d\n",(*(RxBodydata.expirationTime)));
			msgDataBody->expTime = *(RxBodydata.expirationTime);/* OPTIONAL */
		}
		if(NULL != RxBodydata.eventId)
		{
			log_i(LOG_HOZON, "RxBodydata.eventId = %d\n",(*(RxBodydata.eventId)));
			msgDataBody->eventId = *(RxBodydata.eventId);/* OPTIONAL */
		}
		if(NULL != RxBodydata.ulMsgCnt)
		{
			msgDataBody->ulMsgCnt = *(RxBodydata.ulMsgCnt);/* OPTIONAL */
			log_i(LOG_HOZON, "RxBodydata.ulMsgCnt = %d\n",(*(RxBodydata.ulMsgCnt)));
		}
		if(NULL != RxBodydata.dlMsgCnt)
		{
			log_i(LOG_HOZON, "RxBodydata.dlMsgCnt = %d\n",(*(RxBodydata.dlMsgCnt)));
			msgDataBody->dlMsgCnt = *(RxBodydata.dlMsgCnt);/* OPTIONAL */
		}
		if(NULL != RxBodydata.msgCntAcked)
		{
			log_i(LOG_HOZON, "RxBodydata.msgCntAcked = %d\n",(*(RxBodydata.msgCntAcked)));
			msgDataBody->msgCntAcked = *(RxBodydata.msgCntAcked);/* OPTIONAL */
		}
		if(NULL != RxBodydata.ackReq)
		{
			log_i(LOG_HOZON, "RxBodydata.ackReq = %d\n",(*(RxBodydata.ackReq)));
			msgDataBody->ackReq	= *(RxBodydata.ackReq);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataLen)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataLen = %d\n",(*(RxBodydata.appDataLen)));
			msgDataBody->appDataLen	= *(RxBodydata.appDataLen);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataEncode)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataEncode = %d\n",(*(RxBodydata.appDataEncode)));
			msgDataBody->appDataEncode	= *(RxBodydata.appDataEncode);/* OPTIONAL */
		}
		if(NULL != RxBodydata.appDataProVer)
		{
			log_i(LOG_HOZON, "RxBodydata.appDataProVer = %d\n",(*(RxBodydata.appDataProVer)));
			msgDataBody->appDataProVer	= *(RxBodydata.appDataProVer);/* OPTIONAL */
		}
		if(NULL != RxBodydata.testFlag)
		{
			log_i(LOG_HOZON, "RxBodydata.testFlag = %d\n",(*(RxBodydata.testFlag)));
			msgDataBody->testFlag = *(RxBodydata.testFlag);/* OPTIONAL */
		}
		if(NULL != RxBodydata.result)
		{
			log_i(LOG_HOZON, "RxBodydata.result = %d\n",(*(RxBodydata.result)));
			msgDataBody->result	= *(RxBodydata.result);/* OPTIONAL */
		}
	}

	if((appData != NULL) && ((LeMessageDataLen-LeMessageData[0]) > 0))
	{
		AID = (RxBodydata.aID.buf[0] - 0x30)*100 +  (RxBodydata.aID.buf[1] - 0x30)*10 + \
			  (RxBodydata.aID.buf[2] - 0x30);
		MID = RxBodydata.mID;
		log_i(LOG_HOZON, "uper decode:appdata");
		log_i(LOG_HOZON, "application data length = %d\n",LeMessageDataLen-LeMessageData[0]);
		switch(AID)
		{
			case PP_AID_XCALL:
			{
				PrvtProt_App_Xcall_t *app_xcall_ptr = (PrvtProt_App_Xcall_t*)appData;
				if(PP_MID_XCALL_REQ == MID)//xcall req
				{
					XcallReqinfo_t RxXcallReq;
					XcallReqinfo_t *RxXcallReq_ptr = &RxXcallReq;
					memset(&RxXcallReq,0 , sizeof(XcallReqinfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_XcallReq,(void *) &RxXcallReq_ptr, \
							 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON, "Could not decode application data Frame\n");
						return -1;
					}

					app_xcall_ptr->xcallType = RxXcallReq.xcallType;
					log_i(LOG_HOZON, "appData->xcallType = %d\n",app_xcall_ptr->xcallType);
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

					app_xcall_ptr->xcallType 	= RxXcallResp.xcallType;
					app_xcall_ptr->updataTime 	= RxXcallResp.updataTime;
					app_xcall_ptr->battSOCEx 	= RxXcallResp.battSOCEx;
					app_xcall_ptr->engineSt 		= RxXcallResp.engineSt;
					app_xcall_ptr->srsSt 		= RxXcallResp.srsSt;
					app_xcall_ptr->totalOdoMr 	= RxXcallResp.ttOdoMeter;
					app_xcall_ptr->gpsPos.altitude 	= 	 (**(RxXcallResp.gpsPos.list.array)).altitude;
					app_xcall_ptr->gpsPos.gpsSpeed 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSpeed;
					app_xcall_ptr->gpsPos.gpsSt 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSt;
					app_xcall_ptr->gpsPos.gpsTimestamp = (**(RxXcallResp.gpsPos.list.array)).gpsTimestamp;
					app_xcall_ptr->gpsPos.hdop 		=	 (**(RxXcallResp.gpsPos.list.array)).hdop;
					app_xcall_ptr->gpsPos.heading 	=	 (**(RxXcallResp.gpsPos.list.array)).heading;
					app_xcall_ptr->gpsPos.latitude 	=	 (**(RxXcallResp.gpsPos.list.array)).latitude;
					app_xcall_ptr->gpsPos.longitude =	 (**(RxXcallResp.gpsPos.list.array)).longitude;
				}
				else
				{}
			}
			break;
			case PP_AID_RMTCFG:
			{
				PrvtProt_App_rmtCfg_t *app_rmtCfg_ptr = (PrvtProt_App_rmtCfg_t*)appData;
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
					log_i(LOG_HOZON, "decode appdata ok\n");
					app_rmtCfg_ptr->checkResp.needUpdate = cfgcheckResp.needUpdate;
					log_i(LOG_HOZON, "checkCfgResp.needUpdate = %d\n",app_rmtCfg_ptr->checkResp.needUpdate);
					if(cfgcheckResp.cfgVersion != NULL)
					{
						log_i(LOG_HOZON, "checkCfgResp.cfgVersionlen = %d\n",cfgcheckResp.cfgVersion->size);
						app_rmtCfg_ptr->checkResp.cfgVersionlen = cfgcheckResp.cfgVersion->size;
						memcpy(app_rmtCfg_ptr->checkResp.cfgVersion,cfgcheckResp.cfgVersion->buf, \
																	cfgcheckResp.cfgVersion->size);
						log_i(LOG_HOZON, "checkCfgResp.cfgVersion = %s\n",app_rmtCfg_ptr->checkResp.cfgVersion);
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

					app_rmtCfg_ptr->getResp.result = cfggetResp.result;
					log_i(LOG_HOZON, "getCfgResp.result = %d\n",app_rmtCfg_ptr->getResp.result);
					if(cfggetResp.ficmCfg != NULL)
					{
						app_rmtCfg_ptr->getResp.FICM.ficmConfigValid = 1;
						memcpy(app_rmtCfg_ptr->getResp.FICM.token,(**(cfggetResp.ficmCfg->list.array)).token.buf, \
								(**(cfggetResp.ficmCfg->list.array)).token.size);
						app_rmtCfg_ptr->getResp.FICM.tokenlen = (**(cfggetResp.ficmCfg->list.array)).token.size;

						memcpy(app_rmtCfg_ptr->getResp.FICM.userID,(**(cfggetResp.ficmCfg->list.array)).userID.buf, \
														(**(cfggetResp.ficmCfg->list.array)).userID.size);
						app_rmtCfg_ptr->getResp.FICM.userIDlen = (**(cfggetResp.ficmCfg->list.array)).userID.size;

						app_rmtCfg_ptr->getResp.FICM.directConnEnable = (**(cfggetResp.ficmCfg->list.array)).directConnEnable;

						memcpy(app_rmtCfg_ptr->getResp.FICM.address,(**(cfggetResp.ficmCfg->list.array)).address.buf, \
																				(**(cfggetResp.ficmCfg->list.array)).address.size);
						app_rmtCfg_ptr->getResp.FICM.addresslen = (**(cfggetResp.ficmCfg->list.array)).address.size;

						memcpy(app_rmtCfg_ptr->getResp.FICM.port,(**(cfggetResp.ficmCfg->list.array)).port.buf, \
																				(**(cfggetResp.ficmCfg->list.array)).port.size);
						app_rmtCfg_ptr->getResp.FICM.portlen = (**(cfggetResp.ficmCfg->list.array)).port.size;

						log_i(LOG_HOZON, "getCfgResp.token = %s\n",app_rmtCfg_ptr->getResp.FICM.token);
						log_i(LOG_HOZON, "getCfgResp.tokenlen = %d\n",app_rmtCfg_ptr->getResp.FICM.tokenlen);
						log_i(LOG_HOZON, "getCfgResp.userID = %s\n",app_rmtCfg_ptr->getResp.FICM.userID);
						log_i(LOG_HOZON, "getCfgResp.userIDlen = %d\n",app_rmtCfg_ptr->getResp.FICM.userIDlen);

						log_i(LOG_HOZON, "getCfgResp.directConnEnable = %d\n",app_rmtCfg_ptr->getResp.FICM.directConnEnable);
						log_i(LOG_HOZON, "getCfgResp.address = %s\n",app_rmtCfg_ptr->getResp.FICM.address);
						log_i(LOG_HOZON, "getCfgResp.addresslen = %d\n",app_rmtCfg_ptr->getResp.FICM.addresslen);
						log_i(LOG_HOZON, "getCfgResp.port = %s\n",app_rmtCfg_ptr->getResp.FICM.port);
						log_i(LOG_HOZON, "getCfgResp.portlen = %d\n",app_rmtCfg_ptr->getResp.FICM.portlen);
					}

					if(cfggetResp.apn1Cfg != NULL)
					{
						app_rmtCfg_ptr->getResp.APN1.apn1ConfigValid =1;
						memcpy(app_rmtCfg_ptr->getResp.APN1.tspAddr,(**(cfggetResp.apn1Cfg->list.array)).tspAddress.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspAddress.size);
						app_rmtCfg_ptr->getResp.APN1.tspAddrlen = (**(cfggetResp.apn1Cfg->list.array)).tspAddress.size;
						log_i(LOG_HOZON, "getCfgResp.tspAddr = %s\n",app_rmtCfg_ptr->getResp.APN1.tspAddr);
						log_i(LOG_HOZON, "getCfgResp.tspAddrlen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspAddrlen);

						memcpy(app_rmtCfg_ptr->getResp.APN1.tspUser,(**(cfggetResp.apn1Cfg->list.array)).tspUser.buf, \
																	(**(cfggetResp.apn1Cfg->list.array)).tspUser.size);
						app_rmtCfg_ptr->getResp.APN1.tspUserlen = (**(cfggetResp.apn1Cfg->list.array)).tspUser.size;
						log_i(LOG_HOZON, "getCfgResp.tspUser = %s\n",app_rmtCfg_ptr->getResp.APN1.tspUser);
						log_i(LOG_HOZON, "getCfgResp.tspUserlen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspUserlen);

						memcpy(app_rmtCfg_ptr->getResp.APN1.tspPass,(**(cfggetResp.apn1Cfg->list.array)).tspPass.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspPass.size);
						app_rmtCfg_ptr->getResp.APN1.tspPasslen = (**(cfggetResp.apn1Cfg->list.array)).tspPass.size;
						log_i(LOG_HOZON, "getCfgResp.tspPass = %s\n",app_rmtCfg_ptr->getResp.APN1.tspPass);
						log_i(LOG_HOZON, "getCfgResp.tspPasslen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspPasslen);

						memcpy(app_rmtCfg_ptr->getResp.APN1.tspIP,(**(cfggetResp.apn1Cfg->list.array)).tspIp.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspIp.size);
						app_rmtCfg_ptr->getResp.APN1.tspIPlen = (**(cfggetResp.apn1Cfg->list.array)).tspIp.size;
						log_i(LOG_HOZON, "getCfgResp.tspIP = %s\n",app_rmtCfg_ptr->getResp.APN1.tspIP);
						log_i(LOG_HOZON, "getCfgResp.tspIPlen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspIPlen);

						memcpy(app_rmtCfg_ptr->getResp.APN1.tspSms,(**(cfggetResp.apn1Cfg->list.array)).tspSms.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspSms.size);
						app_rmtCfg_ptr->getResp.APN1.tspSmslen = (**(cfggetResp.apn1Cfg->list.array)).tspSms.size;
						log_i(LOG_HOZON, "getCfgResp.tspSms = %s\n",app_rmtCfg_ptr->getResp.APN1.tspSms);
						log_i(LOG_HOZON, "getCfgResp.tspSmslen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspSmslen);

						memcpy(app_rmtCfg_ptr->getResp.APN1.tspPort,(**(cfggetResp.apn1Cfg->list.array)).tspPort.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspPort.size);
						app_rmtCfg_ptr->getResp.APN1.tspPortlen = (**(cfggetResp.apn1Cfg->list.array)).tspPort.size;
						log_i(LOG_HOZON, "getCfgResp.tspPort = %s\n",app_rmtCfg_ptr->getResp.APN1.tspPort);
						log_i(LOG_HOZON, "getCfgResp.tspPortlen = %d\n",app_rmtCfg_ptr->getResp.APN1.tspPortlen);
					}

					if(cfggetResp.apn2Cfg != NULL)
					{
						app_rmtCfg_ptr->getResp.APN2.apn2ConfigValid = 1;
						memcpy(app_rmtCfg_ptr->getResp.APN2.apn2Address,(**(cfggetResp.apn2Cfg->list.array)).tspAddress.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspAddress.size);
						app_rmtCfg_ptr->getResp.APN2.apn2Addresslen = (**(cfggetResp.apn2Cfg->list.array)).tspAddress.size;
						log_i(LOG_HOZON, "getCfgResp.apn2Address = %s\n",app_rmtCfg_ptr->getResp.APN2.apn2Address);
						log_i(LOG_HOZON, "getCfgResp.apn2Addresslen = %d\n",app_rmtCfg_ptr->getResp.APN2.apn2Addresslen);

						memcpy(app_rmtCfg_ptr->getResp.APN2.apn2User,(**(cfggetResp.apn2Cfg->list.array)).tspUser.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspUser.size);
						app_rmtCfg_ptr->getResp.APN2.apn2Userlen = (**(cfggetResp.apn2Cfg->list.array)).tspUser.size;
						log_i(LOG_HOZON, "getCfgResp.apn2User = %s\n",app_rmtCfg_ptr->getResp.APN2.apn2User);
						log_i(LOG_HOZON, "getCfgResp.apn2Userlen = %d\n",app_rmtCfg_ptr->getResp.APN2.apn2Userlen);

						memcpy(app_rmtCfg_ptr->getResp.APN2.apn2Pass,(**(cfggetResp.apn2Cfg->list.array)).tspPass.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspPass.size);
						app_rmtCfg_ptr->getResp.APN2.apn2Passlen = (**(cfggetResp.apn2Cfg->list.array)).tspPass.size;
						log_i(LOG_HOZON, "getCfgResp.apn2Pass = %s\n",app_rmtCfg_ptr->getResp.APN2.apn2Pass);
						log_i(LOG_HOZON, "getCfgResp.apn2Passlen = %d\n",app_rmtCfg_ptr->getResp.APN2.apn2Passlen);
					}

					if(cfggetResp.commonCfg != NULL)
					{
						app_rmtCfg_ptr->getResp.COMMON.commonConfigValid = 1;
						app_rmtCfg_ptr->getResp.COMMON.actived = (**(cfggetResp.commonCfg->list.array)).actived;
						app_rmtCfg_ptr->getResp.COMMON.bCallEnabled = (**(cfggetResp.commonCfg->list.array)).bCallEnabled;
						app_rmtCfg_ptr->getResp.COMMON.btKeyEntryEnabled = (**(cfggetResp.commonCfg->list.array)).btKeyEntryEnabled;
						app_rmtCfg_ptr->getResp.COMMON.dcEnabled = (**(cfggetResp.commonCfg->list.array)).dcEnabled;
						app_rmtCfg_ptr->getResp.COMMON.dtcEnabled = (**(cfggetResp.commonCfg->list.array)).dtcEnabled;
						app_rmtCfg_ptr->getResp.COMMON.eCallEnabled = (**(cfggetResp.commonCfg->list.array)).eCallEnabled;
						app_rmtCfg_ptr->getResp.COMMON.iCallEnabled = (**(cfggetResp.commonCfg->list.array)).iCallEnabled;
						app_rmtCfg_ptr->getResp.COMMON.journeysEnabled = (**(cfggetResp.commonCfg->list.array)).journeysEnabled;
						app_rmtCfg_ptr->getResp.COMMON.onlineInfEnabled = (**(cfggetResp.commonCfg->list.array)).onlineInfEnabled;
						app_rmtCfg_ptr->getResp.COMMON.rChargeEnabled = (**(cfggetResp.commonCfg->list.array)).rChargeEnabled;
						app_rmtCfg_ptr->getResp.COMMON.rcEnabled = (**(cfggetResp.commonCfg->list.array)).rcEnabled;
						app_rmtCfg_ptr->getResp.COMMON.svtEnabled = (**(cfggetResp.commonCfg->list.array)).svtEnabled;
						app_rmtCfg_ptr->getResp.COMMON.vsEnabled = (**(cfggetResp.commonCfg->list.array)).vsEnabled;

						log_i(LOG_HOZON, "getCfgResp.actived = %d\n",app_rmtCfg_ptr->getResp.COMMON.actived);
						log_i(LOG_HOZON, "getCfgResp.bCallEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.bCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.btKeyEntryEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.btKeyEntryEnabled);
						log_i(LOG_HOZON, "getCfgResp.dcEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.dcEnabled);
						log_i(LOG_HOZON, "getCfgResp.dtcEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.dtcEnabled);
						log_i(LOG_HOZON, "getCfgResp.eCallEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.eCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.iCallEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.iCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.journeysEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.journeysEnabled);
						log_i(LOG_HOZON, "getCfgResp.onlineInfEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.onlineInfEnabled);
						log_i(LOG_HOZON, "getCfgResp.rChargeEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.rChargeEnabled);
						log_i(LOG_HOZON, "getCfgResp.rcEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.rcEnabled);
						log_i(LOG_HOZON, "getCfgResp.svtEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.svtEnabled);
						log_i(LOG_HOZON, "getCfgResp.vsEnabled = %d\n",app_rmtCfg_ptr->getResp.COMMON.vsEnabled);
					}

					if(cfggetResp.extendCfg != NULL)
					{
						app_rmtCfg_ptr->getResp.EXTEND.extendConfigValid = 1;
						memcpy(app_rmtCfg_ptr->getResp.EXTEND.bcallNO,(**(cfggetResp.extendCfg->list.array)).bcallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).bcallNO.size);
						app_rmtCfg_ptr->getResp.EXTEND.bcallNOlen = (**(cfggetResp.extendCfg->list.array)).bcallNO.size;
						log_i(LOG_HOZON, "getCfgResp.bcallNO = %s\n",app_rmtCfg_ptr->getResp.EXTEND.bcallNO);
						log_i(LOG_HOZON, "getCfgResp.bcallNOlen = %d\n",app_rmtCfg_ptr->getResp.EXTEND.bcallNOlen);

						memcpy(app_rmtCfg_ptr->getResp.EXTEND.ecallNO,(**(cfggetResp.extendCfg->list.array)).ecallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).ecallNO.size);
						app_rmtCfg_ptr->getResp.EXTEND.ecallNOlen = (**(cfggetResp.extendCfg->list.array)).ecallNO.size;
						log_i(LOG_HOZON, "getCfgResp.ecallNO = %s\n",app_rmtCfg_ptr->getResp.EXTEND.ecallNO);
						log_i(LOG_HOZON, "getCfgResp.ecallNOlen = %d\n",app_rmtCfg_ptr->getResp.EXTEND.ecallNOlen);

						memcpy(app_rmtCfg_ptr->getResp.EXTEND.ccNO,(**(cfggetResp.extendCfg->list.array)).icallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).icallNO.size);
						app_rmtCfg_ptr->getResp.EXTEND.ccNOlen = (**(cfggetResp.extendCfg->list.array)).icallNO.size;
						log_i(LOG_HOZON, "getCfgResp.ccNONO = %s\n",app_rmtCfg_ptr->getResp.EXTEND.ccNO);
						log_i(LOG_HOZON, "getCfgResp.ccNOlen = %d\n",app_rmtCfg_ptr->getResp.EXTEND.ccNOlen);
					}
				}
				else if(PP_MID_READ_CFG_REQ == MID)//read config req
				{
					log_i(LOG_HOZON,  "read config req\n");
					CfgReadReqInfo_t DecodeCRR;
					CfgReadReqInfo_t *DecodeCRR_ptr = &DecodeCRR;
					memset(&DecodeCRR,0 , sizeof(CfgReadReqInfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_Cfg_read_req,(void *) &DecodeCRR_ptr, \
							&LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON,  "Could not decode cfg read req application data Frame\n");
						return -1;
					}

					memset(&(app_rmtCfg_ptr->ReadReq),0,sizeof(App_rmtCfg_CfgReadReq_t));
					if(DecodeCRR.settingIds.list.count <= PP_RMTCFG_SETID_MAX)
					{
						for(i = 0;i < DecodeCRR.settingIds.list.count;i++)
						{
							app_rmtCfg_ptr->ReadReq.SettingId[i] = DecodeCRR.settingIds.list.array[i]->id;
							log_i(LOG_HOZON, "ReadReq.SettingId = %d\n",app_rmtCfg_ptr->ReadReq.SettingId[i]);
						}
						app_rmtCfg_ptr->ReadReq.SettingIdlen = DecodeCRR.settingIds.list.count;
						log_i(LOG_HOZON, "ReadReq.SettingIdlen = %d\n",app_rmtCfg_ptr->ReadReq.SettingIdlen);
					}
				}
			}
			break;
			case PP_AID_RMTCTRL:
			{
				PrvtProt_App_rmtCtrl_t *app_rmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t*)appData;
				if(PP_MID_RMTCTRL_REQ == MID)//check cfg resp
				{
					log_i(LOG_HOZON, "remote control req\n");
					RmtCtrlReqInfo_t RmtCtrlReq;
					RmtCtrlReqInfo_t *RmtCtrlReq_ptr = &RmtCtrlReq;
					memset(&RmtCtrlReq,0 , sizeof(RmtCtrlReqInfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_Ctrl_Req,(void *) &RmtCtrlReq_ptr, \
									 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON,  "Could not decode remote control req application data Frame\n");
						return -1;
					}

					app_rmtCtrl_ptr->CtrlReq.rvcReqType = RmtCtrlReq.rvcReqType;
					log_i(LOG_HOZON, "RmtCtrlReq.rvcReqType = %d\n",app_rmtCtrl_ptr->CtrlReq.rvcReqType);
					app_rmtCtrl_ptr->CtrlReq.rvcReqParamslen = 0;
					if(RmtCtrlReq.rvcReqParams != NULL)
					{
						memcpy(app_rmtCtrl_ptr->CtrlReq.rvcReqParams,RmtCtrlReq.rvcReqParams->buf, \
																	RmtCtrlReq.rvcReqParams->size);
						app_rmtCtrl_ptr->CtrlReq.rvcReqParamslen = RmtCtrlReq.rvcReqParams->size;
						log_i(LOG_HOZON, "RmtCtrlReq.rvcReqParams = %s\n",app_rmtCtrl_ptr->CtrlReq.rvcReqParams);
						log_i(LOG_HOZON, "RmtCtrlReq.rvcReqParamslen = %d\n",app_rmtCtrl_ptr->CtrlReq.rvcReqParamslen);
					}
				}
			}
			break;
			case PP_AID_VS:
			{
				PrvtProt_App_VS_t *app_VS_ptr = (PrvtProt_App_VS_t*)appData;
				if(PP_MID_VS_REQ == MID)//check Vehi status req
				{
					log_i(LOG_HOZON, "remote vehi status req\n");
					VehicleStReqInfo_t VSReq;
					VehicleStReqInfo_t *VSReq_ptr = &VSReq;
					memset(&VSReq,0 , sizeof(VehicleStReqInfo_t));
					dc = uper_decode(asn_codec_ctx,pduType_VS_req,(void *) &VSReq_ptr, \
									 &LeMessageData[LeMessageData[0]],LeMessageDataLen - LeMessageData[0],0,0);
					if(dc.code  != RC_OK)
					{
						log_e(LOG_HOZON,  "Could not decode remote vehi status check  req application data Frame\n");
						return -1;
					}

					app_VS_ptr->VSReq.vehStatusReqType = VSReq.vehStatusReqType;
					log_i(LOG_HOZON, "VSReq.vehStatusReqType = %d\n",app_VS_ptr->VSReq.vehStatusReqType);
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

#if 0
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
#endif
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
