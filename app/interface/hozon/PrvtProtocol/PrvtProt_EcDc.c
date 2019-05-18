/******************************************************
�ļ�����	PrvtProt_EcDc.c

������	��ҵ˽��Э�飨�㽭���ڣ�,�����	
Data			Vasion			author
2019/4/29		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
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

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
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
static uint8_t tboxAppdata[PP_ECDC_DATA_LEN];
static int tboxAppdataLen;
static uint8_t tboxDisBodydata[PP_ECDC_DATA_LEN];
static int tboxDisBodydataLen;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PrvtPro_writeout(const void *buffer,size_t size,void *key);
#if 0
static void PrvtPro_showMsgData(uint8_t type,Bodyinfo_t *RxBodydata,void *RxAppdata);
#endif
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtPro_msgPackage

*��  �Σ�

*����ֵ��

*��  �������ݴ������

*��  ע��
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
	��� dispatcher body��application data
*********************************************/	
	Bodydata.aID.buf = DisptrBody->aID;
	Bodydata.aID.size = 3;
	Bodydata.mID = DisptrBody->mID;
	Bodydata.eventTime 		= DisptrBody->eventTime;
	Bodydata.expirationTime = &(DisptrBody->expTime);	/* OPTIONAL */;
	if(DisptrBody->eventId != PP_INVALID)//event ID��ʼ��Ϊ0xFFFFFFFF-��Ч
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
			Rvspos.gpsSt = XcallResp_ptr->gpsPos.gpsSt;//gps״̬ 0-��Ч��1-��Ч
			Rvspos.gpsTimestamp = XcallResp_ptr->gpsPos.gpsTimestamp;//gpsʱ���
			Rvspos.latitude = XcallResp_ptr->gpsPos.latitude;//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
			Rvspos.longitude = XcallResp_ptr->gpsPos.longitude;//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
			Rvspos.altitude = XcallResp_ptr->gpsPos.altitude;//�߶ȣ�m��
			Rvspos.heading = XcallResp_ptr->gpsPos.heading;//��ͷ����Ƕȣ�0Ϊ��������
			Rvspos.gpsSpeed = XcallResp_ptr->gpsPos.gpsSpeed;//�ٶ� x 10����λkm/h
			Rvspos.hdop = XcallResp_ptr->gpsPos.hdop;//ˮƽ�������� x 10
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

			Bodydata.dlMsgCnt 		= NULL;	/* OPTIONAL */
			CfgReadResp.result = rmtCfgReadResp_ptr->ReadResp.result;
			CfgReadResp.cfgVersion.buf = rmtCfgReadResp_ptr->checkReq.cfgVersion;
			CfgReadResp.cfgVersion.size = rmtCfgReadResp_ptr->checkReq.cfgVersionlen;
			if(1 == rmtCfgReadResp_ptr->ReadResp.result)
			{
				if(1 == rmtCfgReadResp_ptr->ReadResp.FICM.ficmConfigValid)
				{
					FICMConfigSet_t FICMCfgSet;
					FICMConfigSet_t *FICMCfgSet_ptr = &FICMCfgSet;
					struct ficmConfig ficmConfig;

					memset(&FICMCfgSet,0 , sizeof(FICMConfigSet_t));
					FICMCfgSet.token.buf = rmtCfgReadResp_ptr->ReadResp.FICM.token;
					FICMCfgSet.token.size = rmtCfgReadResp_ptr->ReadResp.FICM.tokenlen;
					FICMCfgSet.userID.buf =  rmtCfgReadResp_ptr->ReadResp.FICM.userID;
					FICMCfgSet.userID.size = rmtCfgReadResp_ptr->ReadResp.FICM.userIDlen;
					ficmConfig.list.array = &FICMCfgSet_ptr;
					ficmConfig.list.count = 1;
					ficmConfig.list.size = 1;
					CfgReadResp.ficmConfig = &ficmConfig;
				}
				else
				{
					CfgReadResp.ficmConfig  = NULL;
				}

				if(1 == rmtCfgReadResp_ptr->ReadResp.APN1.apn1ConfigValid)
				{
					APN1ConfigSet_t APN1ConfigSet;
					APN1ConfigSet_t *APN1ConfigSet_ptr = &APN1ConfigSet;
					struct apn1Config apn1Cfg;

					memset(&APN1ConfigSet,0 , sizeof(APN1ConfigSet_t));
					APN1ConfigSet.tspAddress.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspAddr;
					APN1ConfigSet.tspAddress.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspAddrlen;
					APN1ConfigSet.tspIp.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspIP;
					APN1ConfigSet.tspIp.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspIPlen;
					APN1ConfigSet.tspPass.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspPass;
					APN1ConfigSet.tspPass.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspPasslen;
					APN1ConfigSet.tspPort.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspPort;
					APN1ConfigSet.tspPort.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspPortlen;
					APN1ConfigSet.tspSms.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspSms;
					APN1ConfigSet.tspSms.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspSmslen;
					APN1ConfigSet.tspUser.buf = rmtCfgReadResp_ptr->ReadResp.APN1.tspUser;
					APN1ConfigSet.tspUser.size = rmtCfgReadResp_ptr->ReadResp.APN1.tspUserlen;
					apn1Cfg.list.array = &APN1ConfigSet_ptr;
					apn1Cfg.list.count = 1;
					apn1Cfg.list.size = 1;
					CfgReadResp.apn1Config = &apn1Cfg;
				}
				else
				{
					CfgReadResp.apn1Config = NULL;
				}

				if(1 == rmtCfgReadResp_ptr->ReadResp.APN2.apn2ConfigValid)
				{
					APN2ConfigSet_t APN2ConfigSet;
					APN2ConfigSet_t *APN2ConfigSet_ptr = &APN2ConfigSet;
					struct apn2Config apn2Cfg;

					memset(&APN2ConfigSet,0 , sizeof(APN2ConfigSet_t));
					APN2ConfigSet.tspAddress.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Address;
					APN2ConfigSet.tspAddress.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Addresslen;
					APN2ConfigSet.tspPass.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Pass;
					APN2ConfigSet.tspPass.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Passlen;
					APN2ConfigSet.tspUser.buf = rmtCfgReadResp_ptr->ReadResp.APN2.apn2User;
					APN2ConfigSet.tspUser.size = rmtCfgReadResp_ptr->ReadResp.APN2.apn2Userlen;
					apn2Cfg.list.array = &APN2ConfigSet_ptr;
					apn2Cfg.list.count = 1;
					apn2Cfg.list.size = 1;
					CfgReadResp.apn2Config = &apn2Cfg;
				}
				else
				{
					CfgReadResp.apn2Config = NULL;

				}

				if(1 == rmtCfgReadResp_ptr->ReadResp.COMMON.commonConfigValid)
				{
					CommonConfigSet_t CommonConfigSet;
					CommonConfigSet_t *CommonConfigSet_ptr = &CommonConfigSet;
					struct commonConfig commonCfg;

					memset(&CommonConfigSet,0 , sizeof(CommonConfigSet_t));
					CommonConfigSet.actived 		= rmtCfgReadResp_ptr->ReadResp.COMMON.actived;
					CommonConfigSet.rcEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.rcEnabled;
					CommonConfigSet.svtEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.svtEnabled;
					CommonConfigSet.vsEnabled 		= rmtCfgReadResp_ptr->ReadResp.COMMON.vsEnabled;
					CommonConfigSet.iCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.iCallEnabled;
					CommonConfigSet.bCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.bCallEnabled;
					CommonConfigSet.eCallEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.eCallEnabled;
					CommonConfigSet.dcEnabled 	 	= rmtCfgReadResp_ptr->ReadResp.COMMON.dcEnabled;
					CommonConfigSet.dtcEnabled 	 	= rmtCfgReadResp_ptr->ReadResp.COMMON.dtcEnabled;
					CommonConfigSet.journeysEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.journeysEnabled;
					CommonConfigSet.onlineInfEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.onlineInfEnabled;
					CommonConfigSet.rChargeEnabled 	= rmtCfgReadResp_ptr->ReadResp.COMMON.rChargeEnabled;
					CommonConfigSet.btKeyEntryEnabled = rmtCfgReadResp_ptr->ReadResp.COMMON.btKeyEntryEnabled;
					commonCfg.list.array = &CommonConfigSet_ptr;
					commonCfg.list.count =1;
					commonCfg.list.size = 1;
					CfgReadResp.commonConfig = &commonCfg;
				}
				else
				{
					CfgReadResp.commonConfig = NULL;
				}

				if(1 == rmtCfgReadResp_ptr->ReadResp.EXTEND.extendConfigValid)
				{
					ExtendConfigSet_t ExtendConfigSet;
					ExtendConfigSet_t *ExtendConfigSet_ptr = &ExtendConfigSet;
					struct extendConfig extendCfg;

					memset(&ExtendConfigSet,0 , sizeof(ExtendConfigSet_t));
					ExtendConfigSet.ecallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.ecallNO;
					ExtendConfigSet.ecallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.ecallNOlen;
					ExtendConfigSet.bcallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.bcallNO;
					ExtendConfigSet.bcallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.bcallNOlen;
					ExtendConfigSet.icallNO.buf = rmtCfgReadResp_ptr->ReadResp.EXTEND.ccNO;
					ExtendConfigSet.icallNO.size = rmtCfgReadResp_ptr->ReadResp.EXTEND.ccNOlen;
					extendCfg.list.array = &ExtendConfigSet_ptr;
					extendCfg.list.count =1;
					extendCfg.list.size = 1;
					CfgReadResp.extendConfig = &extendCfg;
				}
				else
				{
					CfgReadResp.extendConfig = NULL;
				}
			}
			else
			{
				CfgReadResp.ficmConfig 	= NULL;
				CfgReadResp.apn1Config 	= NULL;
				CfgReadResp.apn2Config 	= NULL;
				CfgReadResp.commonConfig = NULL;
				CfgReadResp.extendConfig = NULL;
			}

			ec = uper_encode(pduType_Cfg_read_resp,(void *) &CfgReadResp,PrvtPro_writeout,&key);
			if(ec.encoded  == -1)
			{
				log_e(LOG_HOZON,"encode:appdata Cfg_conn_req fail\n");
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
				����
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
				��� message data
*********************************************/	
	int tboxmsglen = 0;
	msgData[tboxmsglen++] = tboxDisBodydataLen +1;//��� dispatcher header
	for(i = 0;i < tboxDisBodydataLen;i++)
	{
		msgData[tboxmsglen++]= tboxDisBodydata[i];
	}
	for(i = 0;i < tboxAppdataLen;i++)
	{
		msgData[tboxmsglen++]= tboxAppdata[i];
	}
	*msgDataLen = 1 + tboxDisBodydataLen + tboxAppdataLen;//��� message data lengtn

/*********************************************
			����
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
*��������PrvtPro_decodeMsgData

*��  �Σ�

*����ֵ��

*��  ��������message data

*��  ע��
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

					((PrvtProt_App_Xcall_t*)appData)->xcallType = RxXcallReq.xcallType;
					log_i(LOG_HOZON, "appData->xcallType = %d\n",((PrvtProt_App_Xcall_t*)appData)->xcallType);
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

					((PrvtProt_App_Xcall_t*)appData)->xcallType 	= RxXcallResp.xcallType;
					((PrvtProt_App_Xcall_t*)appData)->updataTime 	= RxXcallResp.updataTime;
					((PrvtProt_App_Xcall_t*)appData)->battSOCEx 	= RxXcallResp.battSOCEx;
					((PrvtProt_App_Xcall_t*)appData)->engineSt 		= RxXcallResp.engineSt;
					((PrvtProt_App_Xcall_t*)appData)->srsSt 		= RxXcallResp.srsSt;
					((PrvtProt_App_Xcall_t*)appData)->totalOdoMr 	= RxXcallResp.ttOdoMeter;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.altitude 	= 	 (**(RxXcallResp.gpsPos.list.array)).altitude;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.gpsSpeed 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSpeed;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.gpsSt 	=	 (**(RxXcallResp.gpsPos.list.array)).gpsSt;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.gpsTimestamp = (**(RxXcallResp.gpsPos.list.array)).gpsTimestamp;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.hdop 		=	 (**(RxXcallResp.gpsPos.list.array)).hdop;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.heading 	=	 (**(RxXcallResp.gpsPos.list.array)).heading;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.latitude 	=	 (**(RxXcallResp.gpsPos.list.array)).latitude;
					((PrvtProt_App_Xcall_t*)appData)->gpsPos.longitude =	 (**(RxXcallResp.gpsPos.list.array)).longitude;
				}
				else
				{}
			}
			break;
			case PP_AID_RMTCFG:
			{
				PrvtProt_App_rmtCfg_t *appData_ptr = (PrvtProt_App_rmtCfg_t*)appData;
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

					appData_ptr->checkResp.needUpdate = cfgcheckResp.needUpdate;
					memcpy(appData_ptr->checkResp.cfgVersion,cfgcheckResp.cfgVersion->buf, \
																cfgcheckResp.cfgVersion->size);
					appData_ptr->checkResp.cfgVersionlen = cfgcheckResp.cfgVersion->size;
					log_i(LOG_HOZON, "checkCfgResp.needUpdate = %d\n",appData_ptr->checkResp.needUpdate);
					log_i(LOG_HOZON, "checkCfgResp.cfgVersion = %s\n",appData_ptr->checkResp.cfgVersion);
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

					appData_ptr->getResp.result = cfggetResp.result;
					log_i(LOG_HOZON, "getCfgResp.result = %d\n",appData_ptr->getResp.result);
					if(cfggetResp.ficmCfg != NULL)
					{
						appData_ptr->getResp.FICM.ficmConfigValid = 1;
						memcpy(appData_ptr->getResp.FICM.token,(**(cfggetResp.ficmCfg->list.array)).token.buf, \
								(**(cfggetResp.ficmCfg->list.array)).token.size);
						appData_ptr->getResp.FICM.tokenlen = (**(cfggetResp.ficmCfg->list.array)).token.size;
						memcpy(appData_ptr->getResp.FICM.userID,(**(cfggetResp.ficmCfg->list.array)).userID.buf, \
														(**(cfggetResp.ficmCfg->list.array)).userID.size);
						appData_ptr->getResp.FICM.userIDlen = (**(cfggetResp.ficmCfg->list.array)).userID.size;

						log_i(LOG_HOZON, "getCfgResp.token = %s\n",appData_ptr->getResp.FICM.token);
						log_i(LOG_HOZON, "getCfgResp.tokenlen = %d\n",appData_ptr->getResp.FICM.tokenlen);
						log_i(LOG_HOZON, "getCfgResp.userID = %s\n",appData_ptr->getResp.FICM.userID);
						log_i(LOG_HOZON, "getCfgResp.userIDlen = %d\n",appData_ptr->getResp.FICM.userIDlen);
					}

					if(cfggetResp.apn1Cfg != NULL)
					{
						appData_ptr->getResp.APN1.apn1ConfigValid =1;
						memcpy(appData_ptr->getResp.APN1.tspAddr,(**(cfggetResp.apn1Cfg->list.array)).tspAddress.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspAddress.size);
						appData_ptr->getResp.APN1.tspAddrlen = (**(cfggetResp.apn1Cfg->list.array)).tspAddress.size;
						log_i(LOG_HOZON, "getCfgResp.tspAddr = %s\n",appData_ptr->getResp.APN1.tspAddr);
						log_i(LOG_HOZON, "getCfgResp.tspAddrlen = %d\n",appData_ptr->getResp.APN1.tspAddrlen);

						memcpy(appData_ptr->getResp.APN1.tspPass,(**(cfggetResp.apn1Cfg->list.array)).tspPass.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspPass.size);
						appData_ptr->getResp.APN1.tspPasslen = (**(cfggetResp.apn1Cfg->list.array)).tspPass.size;
						log_i(LOG_HOZON, "getCfgResp.tspPass = %s\n",appData_ptr->getResp.APN1.tspPass);
						log_i(LOG_HOZON, "getCfgResp.tspPasslen = %d\n",appData_ptr->getResp.APN1.tspPasslen);

						memcpy(appData_ptr->getResp.APN1.tspIP,(**(cfggetResp.apn1Cfg->list.array)).tspIp.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspIp.size);
						appData_ptr->getResp.APN1.tspIPlen = (**(cfggetResp.apn1Cfg->list.array)).tspIp.size;
						log_i(LOG_HOZON, "getCfgResp.tspIP = %s\n",appData_ptr->getResp.APN1.tspIP);
						log_i(LOG_HOZON, "getCfgResp.tspIPlen = %d\n",appData_ptr->getResp.APN1.tspIPlen);

						memcpy(appData_ptr->getResp.APN1.tspSms,(**(cfggetResp.apn1Cfg->list.array)).tspSms.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspSms.size);
						appData_ptr->getResp.APN1.tspSmslen = (**(cfggetResp.apn1Cfg->list.array)).tspSms.size;
						log_i(LOG_HOZON, "getCfgResp.tspSms = %s\n",appData_ptr->getResp.APN1.tspSms);
						log_i(LOG_HOZON, "getCfgResp.tspSmslen = %d\n",appData_ptr->getResp.APN1.tspSmslen);

						memcpy(appData_ptr->getResp.APN1.tspPort,(**(cfggetResp.apn1Cfg->list.array)).tspPort.buf, \
															(**(cfggetResp.apn1Cfg->list.array)).tspPort.size);
						appData_ptr->getResp.APN1.tspPortlen = (**(cfggetResp.apn1Cfg->list.array)).tspPort.size;
						log_i(LOG_HOZON, "getCfgResp.tspPort = %s\n",appData_ptr->getResp.APN1.tspPort);
						log_i(LOG_HOZON, "getCfgResp.tspPortlen = %d\n",appData_ptr->getResp.APN1.tspPortlen);
					}

					if(cfggetResp.apn2Cfg != NULL)
					{
						appData_ptr->getResp.APN2.apn2ConfigValid = 1;
						memcpy(appData_ptr->getResp.APN2.apn2Address,(**(cfggetResp.apn2Cfg->list.array)).tspAddress.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspAddress.size);
						appData_ptr->getResp.APN2.apn2Addresslen = (**(cfggetResp.apn2Cfg->list.array)).tspAddress.size;
						log_i(LOG_HOZON, "getCfgResp.apn2Address = %s\n",appData_ptr->getResp.APN2.apn2Address);
						log_i(LOG_HOZON, "getCfgResp.apn2Addresslen = %d\n",appData_ptr->getResp.APN2.apn2Addresslen);

						memcpy(appData_ptr->getResp.APN2.apn2User,(**(cfggetResp.apn2Cfg->list.array)).tspUser.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspUser.size);
						appData_ptr->getResp.APN2.apn2Userlen = (**(cfggetResp.apn2Cfg->list.array)).tspUser.size;
						log_i(LOG_HOZON, "getCfgResp.apn2User = %s\n",appData_ptr->getResp.APN2.apn2User);
						log_i(LOG_HOZON, "getCfgResp.apn2Userlen = %d\n",appData_ptr->getResp.APN2.apn2Userlen);

						memcpy(appData_ptr->getResp.APN2.apn2Pass,(**(cfggetResp.apn2Cfg->list.array)).tspPass.buf, \
															(**(cfggetResp.apn2Cfg->list.array)).tspPass.size);
						appData_ptr->getResp.APN2.apn2Passlen = (**(cfggetResp.apn2Cfg->list.array)).tspPass.size;
						log_i(LOG_HOZON, "getCfgResp.apn2Pass = %s\n",appData_ptr->getResp.APN2.apn2Pass);
						log_i(LOG_HOZON, "getCfgResp.apn2Passlen = %d\n",appData_ptr->getResp.APN2.apn2Passlen);
					}

					if(cfggetResp.commonCfg != NULL)
					{
						appData_ptr->getResp.COMMON.commonConfigValid = 1;
						appData_ptr->getResp.COMMON.actived = (**(cfggetResp.commonCfg->list.array)).actived;
						appData_ptr->getResp.COMMON.bCallEnabled = (**(cfggetResp.commonCfg->list.array)).bCallEnabled;
						appData_ptr->getResp.COMMON.btKeyEntryEnabled = (**(cfggetResp.commonCfg->list.array)).btKeyEntryEnabled;
						appData_ptr->getResp.COMMON.dcEnabled = (**(cfggetResp.commonCfg->list.array)).dcEnabled;
						appData_ptr->getResp.COMMON.dtcEnabled = (**(cfggetResp.commonCfg->list.array)).dtcEnabled;
						appData_ptr->getResp.COMMON.eCallEnabled = (**(cfggetResp.commonCfg->list.array)).eCallEnabled;
						appData_ptr->getResp.COMMON.iCallEnabled = (**(cfggetResp.commonCfg->list.array)).iCallEnabled;
						appData_ptr->getResp.COMMON.journeysEnabled = (**(cfggetResp.commonCfg->list.array)).journeysEnabled;
						appData_ptr->getResp.COMMON.onlineInfEnabled = (**(cfggetResp.commonCfg->list.array)).onlineInfEnabled;
						appData_ptr->getResp.COMMON.rChargeEnabled = (**(cfggetResp.commonCfg->list.array)).rChargeEnabled;
						appData_ptr->getResp.COMMON.rcEnabled = (**(cfggetResp.commonCfg->list.array)).rcEnabled;
						appData_ptr->getResp.COMMON.svtEnabled = (**(cfggetResp.commonCfg->list.array)).svtEnabled;
						appData_ptr->getResp.COMMON.vsEnabled = (**(cfggetResp.commonCfg->list.array)).vsEnabled;

						log_i(LOG_HOZON, "getCfgResp.actived = %d\n",appData_ptr->getResp.COMMON.actived);
						log_i(LOG_HOZON, "getCfgResp.bCallEnabled = %d\n",appData_ptr->getResp.COMMON.bCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.btKeyEntryEnabled = %d\n",appData_ptr->getResp.COMMON.btKeyEntryEnabled);
						log_i(LOG_HOZON, "getCfgResp.dcEnabled = %d\n",appData_ptr->getResp.COMMON.dcEnabled);
						log_i(LOG_HOZON, "getCfgResp.dtcEnabled = %d\n",appData_ptr->getResp.COMMON.dtcEnabled);
						log_i(LOG_HOZON, "getCfgResp.eCallEnabled = %d\n",appData_ptr->getResp.COMMON.eCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.iCallEnabled = %d\n",appData_ptr->getResp.COMMON.iCallEnabled);
						log_i(LOG_HOZON, "getCfgResp.journeysEnabled = %d\n",appData_ptr->getResp.COMMON.journeysEnabled);
						log_i(LOG_HOZON, "getCfgResp.onlineInfEnabled = %d\n",appData_ptr->getResp.COMMON.onlineInfEnabled);
						log_i(LOG_HOZON, "getCfgResp.rChargeEnabled = %d\n",appData_ptr->getResp.COMMON.rChargeEnabled);
						log_i(LOG_HOZON, "getCfgResp.rcEnabled = %d\n",appData_ptr->getResp.COMMON.rcEnabled);
						log_i(LOG_HOZON, "getCfgResp.svtEnabled = %d\n",appData_ptr->getResp.COMMON.svtEnabled);
						log_i(LOG_HOZON, "getCfgResp.vsEnabled = %d\n",appData_ptr->getResp.COMMON.vsEnabled);
					}

					if(cfggetResp.extendCfg != NULL)
					{
						appData_ptr->getResp.EXTEND.extendConfigValid = 1;
						memcpy(appData_ptr->getResp.EXTEND.bcallNO,(**(cfggetResp.extendCfg->list.array)).bcallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).bcallNO.size);
						appData_ptr->getResp.EXTEND.bcallNOlen = (**(cfggetResp.extendCfg->list.array)).bcallNO.size;
						log_i(LOG_HOZON, "getCfgResp.bcallNO = %s\n",appData_ptr->getResp.EXTEND.bcallNO);
						log_i(LOG_HOZON, "getCfgResp.bcallNOlen = %d\n",appData_ptr->getResp.EXTEND.bcallNOlen);

						memcpy(appData_ptr->getResp.EXTEND.ecallNO,(**(cfggetResp.extendCfg->list.array)).ecallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).ecallNO.size);
						appData_ptr->getResp.EXTEND.ecallNOlen = (**(cfggetResp.extendCfg->list.array)).ecallNO.size;
						log_i(LOG_HOZON, "getCfgResp.ecallNO = %s\n",appData_ptr->getResp.EXTEND.ecallNO);
						log_i(LOG_HOZON, "getCfgResp.ecallNOlen = %d\n",appData_ptr->getResp.EXTEND.ecallNOlen);

						memcpy(appData_ptr->getResp.EXTEND.ccNO,(**(cfggetResp.extendCfg->list.array)).icallNO.buf, \
																(**(cfggetResp.extendCfg->list.array)).icallNO.size);
						appData_ptr->getResp.EXTEND.ccNOlen = (**(cfggetResp.extendCfg->list.array)).icallNO.size;
						log_i(LOG_HOZON, "getCfgResp.ccNONO = %s\n",appData_ptr->getResp.EXTEND.ccNO);
						log_i(LOG_HOZON, "getCfgResp.ccNOlen = %d\n",appData_ptr->getResp.EXTEND.ccNOlen);
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

					if(DecodeCRR.settingIds.list.count < PP_RMTCFG_SETID_MAX)
					{
						for(i = 0;i < DecodeCRR.settingIds.list.count;i++)
						{
							appData_ptr->ReadReq.SettingId[i] = DecodeCRR.settingIds.list.array[i]->id;
						}
					}
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
*��������PrvtPro_showMsgData

*��  �Σ�

*����ֵ��

*��  ����message data ��ӡ

*��  ע��
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
*��������static void PrvtPro_writeout

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
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
