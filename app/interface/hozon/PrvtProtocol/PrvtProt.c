/******************************************************
文件名：	PrvtProt.c

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
#include "timer.h"
#include <sys/prctl.h>

#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
//#include "MessageData.h"
//#include "Appdatainfo.h"
#include "XcallReqinfo.h"
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
//#include "gb32960.h"
#include "PrvtProt.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
//static PrvtProt_heartbeat_t heartbeat;
static PrvtProt_task_t pp_task;
static PrvtProt_pack_t PP_RxPack;
static asn_TYPE_descriptor_t *pduType_Body = &asn_DEF_Bodyinfo;
static asn_TYPE_descriptor_t *pduType_XcallReq = &asn_DEF_XcallReqinfo;
//static uint8_t tboxEncodeData[PP_MSG_DATA_LEN];
//static int tboxEncodeLen;
static uint8_t tboxAppdata[PP_MSG_DATA_LEN];
static int tboxAppdataLen;
static uint8_t tboxDisBodydata[PP_MSG_DATA_LEN];
static int tboxDisBodydataLen;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
#if PP_THREAD
static void *PrvtProt_main(void);
#endif
static int PrvtPro_do_checksock(PrvtProt_task_t *task);
static int PrvtPro_do_rcvMsg(PrvtProt_task_t *task);
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task);
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PrvtPro_do_wait(PrvtProt_task_t *task);
static uint32_t PrvtPro_BSEndianReverse(uint32_t value);
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len);
static int PrvtPro_msgPackage(uint8_t type,PrvtProt_pack_t *pack,PrvtProt_DisptrBody_t *DisptrBody,PrvtProt_appData_t *Appchoice);
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task);
static int PrvtPro_writeout(const void *buffer,size_t size,void *key);
static long PrvtPro_getTimestamp(void);
static void PrvtPro_showMsgData(uint8_t type,Bodyinfo_t *RxBodydata,void *RxAppdata);

static void PrvtPro_decodeMsgData(uint8_t type,uint8_t *LeMessageData,int LeMessageDataLen);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PrvtProt_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
int PrvtProt_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			pp_task.heartbeat.ackFlag = 0;
			pp_task.heartbeat.state = 0;//
			pp_task.heartbeat.period = PP_HEART_BEAT_TIME;//
			pp_task.heartbeat.timer = tm_get_time();
			pp_task.waitSt = PP_IDLE;
			pp_task.waittime = 0;
			pp_task.suspend = 0;
			pp_task.ecall.req = 0;
			pp_task.nonce = 0;/* TCP会话ID 由TSP平台产生 */
			pp_task.version = 0x30;/* 大/小版本(由TSP平台定义)*/
			pp_task.tboxid = 204;/* 平台通过tboxID与tboxSN映射 */
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			PrvtProt_shell_init();
		}
        break;
    }

    return ret;
}


/******************************************************
*函数名：PrvtProt_run

*形  参：void

*返回值：void

*描  述：创建任务线程

*备  注：
******************************************************/
int PrvtProt_run(void)
{ 	
	int res = 0;
#if PP_THREAD
	int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)PrvtProt_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_HOZON, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }
#else
	res = 	PrvtPro_do_rcvMsg(&pp_task) ||
			PrvtPro_do_wait(&pp_task) || 
			PrvtProt_do_heartbeat(&pp_task);
#endif
    return res;
}

#if PP_THREAD
/******************************************************
*函数名：PrvtProt_main

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
static void *PrvtProt_main(void)
{
	log_o(LOG_HOZON, "proprietary protocol  of hozon thread running");
	int res;
    prctl(PR_SET_NAME, "HZ_PRVT_PROT");
    while (1)
    {
		if(pp_task.suspend != 0)
		{
			continue;
		}
		log_set_level(LOG_HOZON, LOG_DEBUG);

		res = 	PrvtPro_do_checksock(&pp_task) ||
				PrvtPro_do_rcvMsg(&pp_task) ||
				PrvtPro_do_wait(&pp_task) || 
				PrvtProt_do_heartbeat(&pp_task) ||
				PrvtPro_do_checkXcall(&pp_task);
		
    }
	(void)res;
    return NULL;
}
#endif

/******************************************************
*函数名：PrvtProt_do_heartbeat

*形  参：void

*返回值：void

*描  述：心跳任务

*备  注：
******************************************************/
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task)
{
	PrvtProt_pack_Header_t pack_Header;

	if((tm_get_time() - task->heartbeat.timer) > (task->heartbeat.period*1000))
	{
		pack_Header.sign[0] = 0x2A;
		pack_Header.sign[1] = 0x2A;
		pack_Header.ver.Byte = task->version;
		pack_Header.nonce  = PrvtPro_BSEndianReverse(task->nonce);
		pack_Header.commtype.Byte = 0x70;
		pack_Header.safetype.Byte = 0x00;
		pack_Header.opera = 0x01;
		pack_Header.msglen = PrvtPro_BSEndianReverse((uint32_t)18);
		pack_Header.tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		
		if(sockproxy_MsgSend(pack_Header.sign, 18,NULL) > 0)//发送成功
		{
			protocol_dump(LOG_HOZON, "PRVT_PROT", pack_Header.sign, 18, 1);
			task->waitSt = PP_HEARTBEAT;
			task->heartbeat.ackFlag = PP_ACK_WAIT;
			task->waittime = tm_get_time();		
		}	
		task->heartbeat.timer = tm_get_time();
		return -1;
	}
	return 0;
}

/******************************************************
*函数名：PrvtPro_do_checksock

*形  参：void

*返回值：void

*描  述：检查socket连接

*备  注：
******************************************************/
static int PrvtPro_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{
		
		return 0;
	}
	PrvtPro_do_checkXcall(task);
	return -1;
}

/******************************************************
*函数名：PrvtPro_do_rcvMsg

*形  参：void

*返回值：void

*描  述：接收数据函数

*备  注：
******************************************************/
static int PrvtPro_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	uint8_t rcvbuf[1456U] = {0};
	
	if ((rlen = PrvtProt_rcvMsg(rcvbuf,1456)) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "HOZON private protocol receive message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcvbuf, rlen, 0);
	if((rcvbuf[0] != 0x2A) || (rcvbuf[1] != 0x2A) || \
			(rlen < 18))//判断数据帧头有误或者数据长度不对
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//接收数据长度超出缓存buffer长度
	{
		return 0;
	}
	PrvtPro_makeUpPack(&PP_RxPack,rcvbuf,rlen);
	protocol_dump(LOG_HOZON, "PRVT_PROT", PP_RxPack.packHeader.sign, rlen, 0);
	PrvtPro_RxMsgHandle(task,&PP_RxPack,rlen);

	return 0;
}

/******************************************************
*函数名：PrvtPro_makeUpPack

*形  参：void

*返回值：void

*描  述：接收数据解包

*备  注：
******************************************************/
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len)
{
	static int rlen = 0;
	uint8_t rcvstep = 0;

	rlen = 0;
	RxPack->packHeader.sign[0] = input[rlen++];
	RxPack->packHeader.sign[1] = input[rlen++];
	len = len-2;
	while(len--)
	{
		switch(rcvstep)
		{
			case 0://接收版本号
			{
				RxPack->packHeader.ver.Byte = input[rlen++];
				rcvstep = 1;
			}
			break;
			case 1://接收tcp会话id
			{
				RxPack->packHeader.nonce = PrvtPro_BSEndianReverse(*((uint32_t*)(&input[rlen])));
				rlen += 4;
				rcvstep = 2;
			}
			break;	
			case 2://编码、连接等方式
			{
				RxPack->packHeader.commtype.Byte = input[rlen++];
				rcvstep = 3;
			}
			break;	
			case 3://加密、签名方式
			{
				RxPack->packHeader.safetype.Byte = input[rlen++];
				rcvstep = 4;
			}
			break;
			case 4://操作类型
			{
				RxPack->packHeader.opera = input[rlen++];
				rcvstep = 5;
			}
			break;
			case 5://报文长度
			{
				RxPack->packHeader.msglen = PrvtPro_BSEndianReverse(*((uint32_t*)(&input[rlen])));
				rlen += 4;
				rcvstep = 6;
			}
			break;
			case 6://tboxid
			{
				RxPack->packHeader.tboxid = PrvtPro_BSEndianReverse(*((uint32_t*)(&input[rlen])));
				rlen += 4;
				rcvstep = 7;
			}
			break;
			case 7://message data
			{
				RxPack->msgdata[rlen-18] = input[rlen];
				rlen += 1;
			}
			break;
			default:
			break;
		}
	}
}

/******************************************************
*函数名：PrvtPro_rcvMsgCallback

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	switch(rxPack->packHeader.opera)
	{
		case PP_NATIONALSTANDARD_TYPE:
		{
			
		}
		break;
		case PP_HEARTBEAT_TYPE://接收到心跳包
		{
			log_i(LOG_HOZON, "heart beat is ok");
			task->heartbeat.state = 1;//正常心跳
			task->waitSt = 0;
		}
		break;
		case PP_NGTP_TYPE://ngtp
		{
			
		}
		break;
		default:
		{
			log_e(LOG_HOZON, "unknow package");
		}
		break;
	}
}

/******************************************************
*函数名：PrvtPro_do_wait

*形  参：void

*返回值：void

*描  述：检查是否有事件等待应答

*备  注：
******************************************************/
static int PrvtPro_do_wait(PrvtProt_task_t *task)
{
    if (!task->waitSt)//没有事件等待应答
    {
        return 0;
    }

    if((tm_get_time() - task->waittime) > PP_WAIT_TIMEOUT)
    {
        if (task->waitSt == PP_HEARTBEAT)
        {
            task->waitSt = PP_IDLE;
			task->heartbeat.state = 0;//心跳不正常
            log_e(LOG_HOZON, "heartbeat time out");
        }
        else
        {
			
		}
    }
	 return -1;
}

/******************************************************
*函数名：PrvtPro_BSEndianReverse

*形  参：void

*返回值：void

*描  述：大端模式和小端模式互相转换

*备  注：
******************************************************/
static uint32_t PrvtPro_BSEndianReverse(uint32_t value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
			(value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

/******************************************************
*函数名：PrvtPro_do_checkXcall

*形  参：

*返回值：

*描  述：检查ecall等请求

*备  注：
******************************************************/
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task)
{
	PrvtProt_pack_t pack;
	PrvtProt_DisptrBody_t	PP_DisptrBody;
	PrvtProt_appData_t PP_appData;
	
	if(1 == pp_task.ecall.req)
	{
		uint8_t LeMsgdata[26] = {0x18,0xdf,0xd8,0xb7,0x60,0x03,0x73,0x14,0x38,0x64, \
							 0x00,0x00,0x1f,0x40,0x00,0x08,0x00,0x08,0x00,0x04, \
							 0x01,0x00,0x00,0x00,0x40,0x01};
		int LeMsgdataLen = 26;
		log_i(LOG_HOZON, "decode server data");
		PrvtPro_decodeMsgData(PP_ECALL_REQ,LeMsgdata,LeMsgdataLen);
	}
	
	if(1 == pp_task.ecall.req)
	{	
		pack.packHeader.sign[0] = 0x2A;
		pack.packHeader.sign[1] = 0x2A;
		pack.packHeader.ver.Byte = task->version;
		pack.packHeader.nonce  = PrvtPro_BSEndianReverse(task->nonce);
		pack.packHeader.commtype.Byte = 0xe1;
		pack.packHeader.safetype.Byte = 0x00;
		pack.packHeader.opera = 0x02;
		pack.packHeader.msglen = PrvtPro_BSEndianReverse((uint32_t)18);
		pack.packHeader.tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		
		PP_DisptrBody.aID[0] = '1';
		PP_DisptrBody.aID[1] = '7';
		PP_DisptrBody.aID[2] = '0';
		PP_DisptrBody.mID = 1;
		PP_DisptrBody.eventTime = PrvtPro_getTimestamp();
		PP_DisptrBody.eventId = 1000;
		PP_DisptrBody.ulMsgCnt = 1;	/* OPTIONAL */
		PP_DisptrBody.dlMsgCnt = 0;	/* OPTIONAL */
		PP_DisptrBody.msgCntAcked	= 0;/* OPTIONAL */
		PP_DisptrBody.ackReq = 1;	/* OPTIONAL */
		PP_DisptrBody.appDataLen = 0;	/* OPTIONAL */
		PP_DisptrBody.appDataEncode = 0;	/* OPTIONAL */
		PP_DisptrBody.appDataProVer = 1;	/* OPTIONAL */
		PP_DisptrBody.testFlag	= 0;/* OPTIONAL */
		PP_DisptrBody.result = 0;	/* OPTIONAL */
		
		PP_appData.xcallType = 1;
		if(0 == PrvtPro_msgPackage(PP_ECALL_REQ,&pack,&PP_DisptrBody,&PP_appData))
		{
			if(sockproxy_MsgSend(pack.packHeader.sign,PrvtPro_BSEndianReverse(pack.packHeader.msglen),NULL) > 0)//发送成功
			{
				pp_task.ecall.req = 0;
				protocol_dump(LOG_HOZON, "PRVT_PROT", pack.packHeader.sign, \
							  PrvtPro_BSEndianReverse(pack.packHeader.msglen),1);	
			}
		}
	}
	
	return 0;
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
	if(size > PP_MSG_DATA_LEN)
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

/******************************************************
*函数名：

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
static long PrvtPro_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return (long)timestamp.tv_sec;
	
}

/******************************************************
*函数名：PrvtPro_SetHeartBeatPeriod

*形  参：

*返回值：

*描  述：设置心跳周期

*备  注：
******************************************************/
void PrvtPro_SetHeartBeatPeriod(unsigned char period)
{
	pp_task.heartbeat.period = period;
}

/******************************************************
*函数名：PrvtPro_Setsuspend

*形  参：

*返回值：

*描  述：设置暂停

*备  注：
******************************************************/
void PrvtPro_Setsuspend(unsigned char suspend)
{
	pp_task.suspend = suspend;
}

/******************************************************
*函数名：PrvtPro_SetEcallReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PrvtPro_SetEcallReq(unsigned char req)
{
	pp_task.ecall.req = req;
}

/******************************************************
*函数名：PrvtPro_msgPackage

*形  参：

*返回值：

*描  述：数据打包

*备  注：
******************************************************/
static int PrvtPro_msgPackage(uint8_t type,PrvtProt_pack_t *pack,PrvtProt_DisptrBody_t \
							  *DisptrBody, PrvtProt_appData_t *Appchoice)
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
	pack->msgdata[tboxmsglen++] = tboxDisBodydataLen;//填充 dispatcher header
	for(i = 0;i < tboxDisBodydataLen;i++)
	{
		pack->msgdata[tboxmsglen++]= tboxDisBodydata[i];
	}
	for(i = 0;i < tboxAppdataLen;i++)
	{
		pack->msgdata[tboxmsglen++]= tboxAppdata[i];
	}
	pack->packHeader.msglen = PrvtPro_BSEndianReverse((uint32_t)(18 + 1 + \
							 tboxDisBodydataLen + tboxAppdataLen));//填充 message lengtn
	
/*********************************************
				解码
*********************************************/	
#if 1
	PrvtPro_decodeMsgData(type,pack->msgdata,tboxDisBodydataLen+1+tboxAppdataLen);
#endif	
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
	
	(void)type;
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
*函数名：PrvtPro_decodeMsgData

*形  参：

*返回值：

*描  述：解码message data

*备  注：
******************************************************/
static void PrvtPro_decodeMsgData(uint8_t type,uint8_t *LeMessageData,int LeMessageDataLen)
{
	asn_dec_rval_t dc;
	asn_codec_ctx_t *asn_codec_ctx = 0 ;
	
	Bodyinfo_t RxBodydata;
	Bodyinfo_t *RxBodydata_ptr = &RxBodydata;
	memset(&RxBodydata,0 , sizeof(Bodyinfo_t));

	log_i(LOG_HOZON, "uper decode");
	log_i(LOG_HOZON, "uper decode:bodydata");
	log_i(LOG_HOZON, "dis header length = %d",LeMessageData[0]);
	dc = uper_decode(asn_codec_ctx,pduType_Body,(void *) &RxBodydata_ptr, \
					 &LeMessageData[1],LeMessageData[0],0,0);
	if(dc.code  != RC_OK)
	{
		log_e(LOG_HOZON, "Could not decode dispatcher header Frame");
	}
	
	log_i(LOG_HOZON, "uper decode:appdata");
	log_i(LOG_HOZON, "application data length = %d",LeMessageDataLen-LeMessageData[0]-1);
	switch(type)
	{
		case PP_ECALL_REQ:
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
			PrvtPro_showMsgData(type,RxBodydata_ptr,RxXcallReq_ptr);
		}
		break;
		default:
		break;
	}
	log_i(LOG_HOZON, "uper decode end");
}

