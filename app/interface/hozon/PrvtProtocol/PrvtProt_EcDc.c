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
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "PrvtProt_EcDc.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static asn_TYPE_descriptor_t *pduType_Body = &asn_DEF_Bodyinfo;
static asn_TYPE_descriptor_t *pduType_XcallReq = &asn_DEF_XcallReqinfo;
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

*描  述：数据打包

*备  注：
******************************************************/
int PrvtPro_msgPackage(uint8_t type,uint8_t *msgData,long *msgDataLen, \
					   PrvtProt_DisptrBody_t *DisptrBody, PrvtProt_appData_t *Appchoice)
{
	static uint8_t key;
	Bodyinfo_t Bodydata;
	//Appdatainfo_t Appdata;
	
	int i;
	
	memset(&Bodydata,0 , sizeof(Bodyinfo_t));
/*********************************************
	填充 dispatcher body和application data
*********************************************/	
	Bodydata.aID.buf = DisptrBody->aID;
	Bodydata.aID.size = 3;
	Bodydata.mID = DisptrBody->mID;
	Bodydata.eventTime 		= DisptrBody->eventTime;
	Bodydata.eventId 		= &(DisptrBody->eventId);/* OPTIONAL */
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
		case PP_ECALL_REQ:
		{
			XcallReqinfo_t XcallReq;	
			memset(&XcallReq,0 , sizeof(XcallReqinfo_t));
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
			XcallReq.xcallType = Appchoice->xcallType;
			
			ec = uper_encode(pduType_XcallReq,(void *) &XcallReq,PrvtPro_writeout,&key);
			log_i(LOG_HOZON, "uper encode appdata ec.encoded = %d",ec.encoded);
			if(ec.encoded  == -1) 
			{
				log_e(LOG_HOZON, "Could not encode MessageFrame");
				return -1;
			}
		}
		break;
		case PP_ECALL_RESP:
		{
			
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
	Bodydata.appDataLen = &(DisptrBody->appDataLen);
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
	msgData[tboxmsglen++] = tboxDisBodydataLen;//填充 dispatcher header
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
	PrvtPro_decodeMsgData(msgData,tboxDisBodydataLen+1+tboxAppdataLen,NULL);
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
void PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen,PrvtProt_msgData_t *msgData)
{
	asn_dec_rval_t dc;
	asn_codec_ctx_t *asn_codec_ctx = 0 ;
	Bodyinfo_t RxBodydata;
	Bodyinfo_t *RxBodydata_ptr = &RxBodydata;
	memset(&RxBodydata,0 , sizeof(Bodyinfo_t));
	uint16_t AID;
	uint8_t MID;
	log_i(LOG_HOZON, "uper decode");
	log_i(LOG_HOZON, "uper decode:bodydata");
	log_i(LOG_HOZON, "dis header length = %d",LeMessageData[0]);
	dc = uper_decode(asn_codec_ctx,pduType_Body,(void *) &RxBodydata_ptr, \
					 &LeMessageData[1],LeMessageData[0],0,0);
	if(dc.code  != RC_OK)
	{
		log_e(LOG_HOZON, "Could not decode dispatcher header Frame");
	}
	
	AID = (RxBodydata.aID.buf[0] - 0x30)*100 +  (RxBodydata.aID.buf[1] - 0x30)*10 + \
		  (RxBodydata.aID.buf[2] - 0x30);
	MID = RxBodydata.mID;
	log_i(LOG_HOZON, "uper decode:appdata");
	log_i(LOG_HOZON, "application data length = %d",LeMessageDataLen-LeMessageData[0]-1);
	switch(AID)
	{
		case PP_AID_ECALL:
		{
			if(PP_MID_ECALL_REQ == MID)//ecall req
			{
				XcallReqinfo_t RxXcallReq;
				XcallReqinfo_t *RxXcallReq_ptr = &RxXcallReq;
				memset(&RxXcallReq,0 , sizeof(XcallReqinfo_t));
				dc = uper_decode(asn_codec_ctx,pduType_XcallReq,(void *) &RxXcallReq_ptr, \
						 &LeMessageData[LeMessageData[0] +1],LeMessageDataLen - LeMessageData[0]-1,0,0);
				if(dc.code  != RC_OK) 
				{
					log_e(LOG_HOZON, "Could not decode application data Frame");
				}
				if(NULL == msgData)
				{
					PrvtPro_showMsgData(PP_ECALL_REQ,RxBodydata_ptr,RxXcallReq_ptr);
				}
			}
			else if(PP_MID_ECALL_RESP == MID)//ecall response
			{
				
			}
			else
			{}
		}
		break;
		default:
		break;
	}
	
	log_i(LOG_HOZON, "uper decode end");
	if(msgData != NULL)
	{
		msgData->DisBody.aID[0] = RxBodydata.aID.buf[0];
		msgData->DisBody.aID[1] = RxBodydata.aID.buf[1];
		msgData->DisBody.aID[2] = RxBodydata.aID.buf[2];
		msgData->DisBody.mID 	= RxBodydata.mID;
		msgData->DisBody.eventTime 	= RxBodydata.eventTime;
		log_i(LOG_HOZON, "RxBodydata.aid = %d",AID);
		log_i(LOG_HOZON, "RxBodydata.mID = %d",RxBodydata.mID);
		log_i(LOG_HOZON, "RxBodydata.eventTime = %d",RxBodydata.eventTime);
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
			case PP_ECALL_REQ:
			{
				log_i(LOG_HOZON, "xcallReq.xcallType = %d",((XcallReqinfo_t *)(RxAppdata))->xcallType);
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
