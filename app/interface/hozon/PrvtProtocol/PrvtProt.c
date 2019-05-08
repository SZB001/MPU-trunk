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

#include "../sockproxy/sockproxy_data.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "PrvtProt.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
//static PrvtProt_heartbeat_t heartbeat;
static PrvtProt_task_t 	pp_task;
static PrvtProt_pack_t 	PP_RxPack;
static PrvtProt_pack_Header_t 	PP_PackHeader[PP_APP_MAX] =
{/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
	{"**",	0x30,	0,		0x70,		0,			0x01,	18,		  0	  },//heart beat
	{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  },//ecall req
	{"**",	0x30,	0,		0xe1,		0,			0x02,	0,		  0	  } //ecall response
};

static PrvtProt_pack_t 	PP_Pack[PP_APP_MAX];

static PrvtProt_DisptrBody_t	PP_DisptrBody[PP_APP_MAX] =
{/*   AID  MID  EventTime	EventID		ulMsgCnt  dlMsgCnt	AckedCnt ackReq	 Applen	 AppEc  AppVer  TestFlg  result*/
	{"000",	0,		0,		PP_INVALID,	   0,	   0,		0,       0,       0,      0,     0,		 0,         0   },//ecall req
	{"170",	1,		0,		PP_INVALID,	   0,	   0,		0,       0,       0,      0,     1,		 1,         0   },//ecall req
	{"170", 2,      0,      PP_INVALID,    0,      0,		0,		 1,		  0,	  0,	 256,		 1,			0   } //ecall response
};
static PrvtProt_appData_t 		PP_appData;

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
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len);
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task);
static int PrvtPro_ecallReq(PrvtProt_task_t *task);
static int PrvtPro_ecallResponse(PrvtProt_task_t *task);
static long PrvtPro_getTimestamp(void);
static long PrvtPro_BSEndianReverse(long value);
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
			pp_task.xcall[PP_ECALL].req = 0;
			pp_task.xcall[PP_ECALL].resp = 0;
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
		PP_PackHeader[PP_APP_HEARTBEAT].ver.Byte = task->version;
		PP_PackHeader[PP_APP_HEARTBEAT].nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader[PP_APP_HEARTBEAT].msglen = PrvtPro_BSEndianReverse((long)18);
		PP_PackHeader[PP_APP_HEARTBEAT].tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		memcpy(&pack_Header, &PP_PackHeader[PP_APP_HEARTBEAT], sizeof(PrvtProt_pack_Header_t));

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
	
	if ((rlen = PrvtProtCfg_rcvMsg(rcvbuf,1456)) <= 0)
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
	RxPack->Header.sign[0] = input[rlen++];
	RxPack->Header.sign[1] = input[rlen++];
	len = len-2;
	while(len--)
	{
		switch(rcvstep)
		{
			case 0://接收版本号
			{
				RxPack->Header.ver.Byte = input[rlen++];
				rcvstep = 1;
			}
			break;
			case 1://接收tcp会话id
			{
				RxPack->Header.nonce = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
				rlen += 4;
				rcvstep = 2;
			}
			break;	
			case 2://编码、连接等方式
			{
				RxPack->Header.commtype.Byte = input[rlen++];
				rcvstep = 3;
			}
			break;	
			case 3://加密、签名方式
			{
				RxPack->Header.safetype.Byte = input[rlen++];
				rcvstep = 4;
			}
			break;
			case 4://操作类型
			{
				RxPack->Header.opera = input[rlen++];
				rcvstep = 5;
			}
			break;
			case 5://报文长度
			{
				RxPack->Header.msglen = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
				rlen += 4;
				rcvstep = 6;
			}
			break;
			case 6://tboxid
			{
				RxPack->Header.tboxid = PrvtPro_BSEndianReverse(*((long*)(&input[rlen])));
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
	switch(rxPack->Header.opera)
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
*函数名：PrvtPro_do_checkXcall

*形  参：

*返回值：

*描  述：检查ecall等请求

*备  注：
******************************************************/
static int PrvtPro_do_checkXcall(PrvtProt_task_t *task)
{
	/* ecall */
	PrvtPro_ecallReq(task);
	PrvtPro_ecallResponse(task);

	/* bcall */

	/* icall */
	return 0;
}

/******************************************************
*函数名：PrvtPro_ecallResponse

*形  参：

*返回值：

*描  述：ecall response

*备  注：
******************************************************/
static int PrvtPro_ecallResponse(PrvtProt_task_t *task)
{
	long msgdatalen;

	if(1 == sockproxy_socketState())//socket open
	{
		if((1 == pp_task.xcall[PP_ECALL].resp) || (PrvtProtCfg_ecallTriggerEvent()))//ecall触发
		{
			PP_PackHeader[PP_ECALL_RESP].ver.Byte = task->version;
			PP_PackHeader[PP_ECALL_RESP].nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
			PP_PackHeader[PP_ECALL_RESP].tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
			memcpy(&PP_Pack[PP_ECALL_RESP], &PP_PackHeader[PP_ECALL_RESP], sizeof(PrvtProt_pack_Header_t));

			PP_DisptrBody[PP_ECALL_RESP].eventTime = PrvtPro_getTimestamp();
			PP_DisptrBody[PP_ECALL_RESP].ulMsgCnt++;	/* OPTIONAL */

			PrvtProtcfg_gpsData_t gpsDt;
			PP_appData.Xcall.xcallType = 2;//紧急救援ecall
			PP_appData.Xcall.engineSt = PrvtProtCfg_engineSt();//启动状态；1-熄火；2-启动
			PP_appData.Xcall.totalOdoMr = PrvtProtCfg_totalOdoMr();//里程有效范围：0 - 1000000（km）
			PP_appData.Xcall.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps状态 0-无效；1-有效
			PP_appData.Xcall.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gps时间戳:系统时间(通过gps校时)

			PrvtProtCfg_gpsData(&gpsDt);
			log_i(LOG_HOZON, "is_north = %d",gpsDt.is_north);
			log_i(LOG_HOZON, "is_east = %d",gpsDt.is_east);
			log_i(LOG_HOZON, "latitude = %lf",gpsDt.latitude);
			log_i(LOG_HOZON, "longitude = %lf",gpsDt.longitude);
			log_i(LOG_HOZON, "altitude = %lf",gpsDt.height);

			if(PP_appData.Xcall.gpsPos.gpsSt == 1)
			{
				if(gpsDt.is_north)
				{
					PP_appData.Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000);//纬度 x 1000000,当GPS信号无效时，值为0
				}
				else
				{
					PP_appData.Xcall.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//纬度 x 1000000,当GPS信号无效时，值为0
				}

				if(gpsDt.is_east)
				{
					PP_appData.Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000);//经度 x 1000000,当GPS信号无效时，值为0
				}
				else
				{
					PP_appData.Xcall.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//经度 x 1000000,当GPS信号无效时，值为0
				}
				log_i(LOG_HOZON, "PP_appData.latitude = %lf",PP_appData.Xcall.gpsPos.latitude);
				log_i(LOG_HOZON, "PP_appData.longitude = %lf",PP_appData.Xcall.gpsPos.longitude);
			}
			else
			{
				PP_appData.Xcall.gpsPos.latitude  = 0;
				PP_appData.Xcall.gpsPos.longitude = 0;
			}
			PP_appData.Xcall.gpsPos.altitude = (long)gpsDt.height;//高度（m）
			PP_appData.Xcall.gpsPos.heading = (long)gpsDt.direction;//车头方向角度，0为正北方向
			PP_appData.Xcall.gpsPos.gpsSpeed = (long)gpsDt.kms*10;//速度 x 10，单位km/h
			PP_appData.Xcall.gpsPos.hdop = (long)gpsDt.hdop*10;//水平精度因子 x 10

			PP_appData.Xcall.srsSt = 1;//安全气囊状态 1- 正常；2 - 弹出
			PP_appData.Xcall.updataTime = PrvtPro_getTimestamp();//数据时间戳
			PP_appData.Xcall.battSOCEx = 10;//车辆电池剩余电量：0-10000（0%-100%）

			if(0 == PrvtPro_msgPackageEncoding(PP_ECALL_RESP,PP_Pack[PP_ECALL_RESP].msgdata,&msgdatalen,\
									   	   	   &PP_DisptrBody[PP_ECALL_RESP],&PP_appData))
			{
				PP_Pack[PP_ECALL_RESP].Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
				if(sockproxy_MsgSend(PP_Pack[PP_ECALL_RESP].Header.sign,18 + msgdatalen,NULL) > 0)//发送成功
				{
					protocol_dump(LOG_HOZON, "PRVT_PROT", PP_Pack[PP_ECALL_RESP].Header.sign, \
							18 + msgdatalen,1);
				}
			}
			pp_task.xcall[PP_ECALL].resp = 0;
		}
	}
	return 0;
}

/******************************************************
*函数名：PrvtPro_ecallReq

*形  参：

*返回值：

*描  述：检查ecall等请求

*备  注：
******************************************************/
static int PrvtPro_ecallReq(PrvtProt_task_t *task)
{

	long msgdatalen;

	if(1 == pp_task.xcall[PP_ECALL].req)
	{	
		PP_PackHeader[PP_ECALL_REQ].ver.Byte = task->version;
		PP_PackHeader[PP_ECALL_REQ].nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader[PP_ECALL_REQ].tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		memcpy(&PP_Pack[PP_ECALL_REQ], &PP_PackHeader[PP_ECALL_REQ], sizeof(PrvtProt_pack_Header_t));

		PP_DisptrBody[PP_ECALL_REQ].eventTime = PrvtPro_getTimestamp();
		PP_DisptrBody[PP_ECALL_REQ].ulMsgCnt++;	/* OPTIONAL */
		
		PP_appData.Xcall.xcallType = 2;//紧急救援
		if(0 == PrvtPro_msgPackageEncoding(PP_ECALL_REQ,PP_Pack[PP_ECALL_REQ].msgdata,&msgdatalen, \
											&PP_DisptrBody[PP_ECALL_REQ],&PP_appData))
		{
			PP_Pack[PP_ECALL_REQ].Header.msglen = PrvtPro_BSEndianReverse(18 + msgdatalen);
			if(sockproxy_MsgSend(PP_Pack[PP_ECALL_REQ].Header.sign,(18 + msgdatalen),NULL) > 0)//发送成功
			{
				pp_task.xcall[PP_ECALL].req = 0;
				protocol_dump(LOG_HOZON, "PRVT_PROT", PP_Pack[PP_ECALL_REQ].Header.sign, \
							  (18 + msgdatalen),1);
			}
		}
		pp_task.xcall[PP_ECALL].req = 0;
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
	pp_task.xcall[PP_ECALL].req = req;
}

/******************************************************
*函数名：PrvtPro_SetEcallResp

*形  参：

*返回值：

*描  述：设置ecall response

*备  注：
******************************************************/
void PrvtPro_SetEcallResp(unsigned char resp)
{
	pp_task.xcall[PP_ECALL].resp = resp;
}

/******************************************************
*函数名：PrvtPro_BSEndianReverse

*形  参：void

*返回值：void

*描  述：大端模式和小端模式互相转换

*备  注：
******************************************************/
static long PrvtPro_BSEndianReverse(long value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
		   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

/******************************************************
*函数名：PrvtProt_FieldPadding

*形  参：

*返回值：

*描  述：字段填充

*备  注：
******************************************************/
//static int PrvtProt_FieldPadding(PrvtProt_task_t *task)
//{

//}
