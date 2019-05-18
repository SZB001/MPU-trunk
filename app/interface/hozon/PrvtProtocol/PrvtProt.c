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
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt_callCenter.h"
#include "PrvtProt_xcall.h"
#include "PrvtProt_remoteConfig.h"
#include "PrvtProt.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static PrvtProt_heartbeat_t PP_heartbeat;
static PrvtProt_task_t 	pp_task;
//static PrvtProt_pack_t 	PP_RxPack[PP_APP_MAX];
static PrvtProt_pack_Header_t 	PP_PackHeader_HB;
#if 0
static PrvtProt_pack_Header_t 	PP_PackHeader_HB =
{/* sign  version  nonce	commtype	safetype	opera	msglen	tboxid*/
	"**",	0x30,	0,		0x70,		0,			0x01,	18,		  0	  //heart beat
};
#endif

typedef struct
{
	PrvtProt_App_Xcall_t Xcall;//xcall
	PrvtProt_App_rmtCfg_t rmtCfg;//remote config
}PrvtProt_appData_t;

/* message data struct */
typedef struct
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t 		appData;
}PrvtProt_msgData_t;

#if 0
 PrvtProt_appData_t 		PP_appData =
{
	/*xcallType engineSt totalOdoMr	gps{gpsSt latitude longitude altitude heading gpsSpeed hdop}   srsSt  updataTime	battSOCEx*/
	{	0,		0xff,	 0,		       {0,    0,       0,       0,        0,       0,       0  },	1,		0,			 0  },

	{/*  mcuSw   mpuSw     vehicleVin         iccID                  btMacAddr    configSw     cfgVersion */
		{"00000","00000","00000000000000000","00000000000000000000","000000000000","00000","00000000000000000000000000000000" \
				,5,5,17,20,12,5,32,NULL,NULL,NULL,NULL,NULL,NULL,NULL},\

	/*  needUpdate cfgVersion 					*/
		{0,        "00000000000000000000000000000000" ,32,NULL },\

	/*     cfgVersion 					*/
		{"00000000000000000000000000000000" ,32,NULL },\

	/*	result  token                               userID   */
		{0,      "00000000000000000000000000000000","00000000000000000000000000000000", \
	/*   tspAddr                            tspUser             tspPass            tspIP        */
		"00000000000000000000000000000000","0000000000000000","0000000000000000", "000000000000000", \
	/*    tspSms                           tspPort   apn2addr 	                        apn2User			apn2Pass */
		"00000000000000000000000000000000","000000", "00000000000000000000000000000000", "0000000000000000","000000", \
	/*	actived  rcEnabled  svtEnabled vsEnabled  iCallEnabled  bCallEnabled*/
		0,       0,          0,         0,          0,           0, \
	/*  eCallEnabled   dcEnabled   dtcEnabled   journeysEnabled  onlineInfEnabled rChargeEnabled   btKeyEntryEnabled     */
		0,				0,			0,				0,				0,				0,				0,\
	/*  ecallNO   			bcallNO   		   CCNO */
		"0000000000000000","0000000000000000","0000000000000000",32,32,32,16,16,15,32,6,32,16,6,16,16,16, \
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,0},

		/*configSuccess  mcuSw   mpuSw   configSw     cfgVersion */
		{ 0,            "00000","00000","00000","00000000000000000000000000000000" \
						,5,5,5,32,NULL,NULL,NULL,NULL},\
		/* configAccepted*/
			{0}
	}
};
#endif
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

//static int PrvtPro_xcallCCreq(PrvtProt_task_t *task);
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
			//pp_task.heartbeat.ackFlag = 0;
			PP_heartbeat.state = 1;//
			PP_heartbeat.period = PP_HEART_BEAT_TIME;//
			PP_heartbeat.timer = tm_get_time();
			PP_heartbeat.waitSt = 0;
			PP_heartbeat.waittime = 0;
			pp_task.suspend = 0;
			pp_task.nonce = 0;/* TCP会话ID 由TSP平台产生 */
			pp_task.version = 0x30;/* 大/小版本(由TSP平台定义)*/
			pp_task.tboxid = 27;/* 平台通过tboxID与tboxSN映射 */

			memset(&PP_PackHeader_HB,0 , sizeof(PrvtProt_pack_Header_t));
			memcpy(PP_PackHeader_HB.sign,"**",2);
			PP_PackHeader_HB.ver.Byte = 0x30;
			PP_PackHeader_HB.commtype.Byte = 0x70;
			PP_PackHeader_HB.opera = 0x01;
			PP_PackHeader_HB.msglen = 18;
			PP_PackHeader_HB.tboxid = 27;
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			PrvtProt_CC_init();
			PrvtProt_shell_init();
			PP_xcall_init();
			PP_rmtCfg_init();
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
				PrvtProt_do_heartbeat(&pp_task);

		(void)PP_xcall_mainfunction(&pp_task);//xcall

		PrvtProt_CC_mainfunction();/* CC request:拨打救援电话*/

		PP_rmtCfg_mainfunction(&pp_task);
    }
	(void)res;
    return NULL;
}
#endif

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
	PrvtProt_pack_t 	RxPack;
	
	memset(&RxPack,0 , sizeof(PrvtProt_pack_t));
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
	PrvtPro_makeUpPack(&RxPack,rcvbuf,rlen);
	//protocol_dump(LOG_HOZON, "PRVT_PROT after makeUpPack", PP_RxPack.Header.sign, rlen, 0);
	PrvtPro_RxMsgHandle(task,&RxPack,rlen);

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
	int rlen = 0;
	int size = len;
	uint8_t rcvstep = 0;
	RxPack->Header.sign[0] = input[rlen++];
	RxPack->Header.sign[1] = input[rlen++];
	size = size-2;
	while(size--)
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
				RxPack->Header.nonce = (RxPack->Header.nonce << 8) + input[rlen++];
				if(7 == rlen)
				{
					rcvstep = 2;
				}
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
				RxPack->Header.msglen = (RxPack->Header.msglen << 8) + input[rlen++];
				if(14 == rlen)
				{
					rcvstep = 6;
				}
			}
			break;
			case 6://tboxid
			{
				RxPack->Header.tboxid = (RxPack->Header.tboxid << 8) + input[rlen++];
				if(18 == rlen)
				{
					rcvstep = 7;
				}
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
	int aid;
	switch(rxPack->Header.opera)
	{
		case PP_NATIONALSTANDARD_TYPE:
		{
			
		}
		break;
		case PP_HEARTBEAT_TYPE://接收到心跳包
		{
			log_i(LOG_HOZON, "heart beat is ok");
			PP_heartbeat.state = 1;//正常心跳
			PP_heartbeat.waitSt = 0;
		}
		break;
		case PP_NGTP_TYPE://ngtp
		{//解码出数据，写入对应的队列
			PrvtProt_DisptrBody_t MsgDataBody;
			PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,NULL);
			aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
					  (MsgDataBody.aID[2] - 0x30);
			switch(aid)
			{
				case PP_AID_XCALL://XCALL
				{
					WrPP_queue(PP_XCALL,rxPack->Header.sign,len);
				}
				break;
				case PP_AID_RMTCFG://remote config
				{
					WrPP_queue(PP_REMOTE_CFG,rxPack->Header.sign,len);
				}
				default:
				{
					log_e(LOG_HOZON, "rcv unknow ngtp package\r\n");
				}
				break;
			}
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
    if (!PP_heartbeat.waitSt)//没有事件等待应答
    {
        return 0;
    }

    if((tm_get_time() - PP_heartbeat.waittime) > PP_HB_WAIT_TIMEOUT)
    {
        if (PP_heartbeat.waitSt == 1)
        {
        	PP_heartbeat.waitSt = 0;
        	PP_heartbeat.state = 0;//心跳不正常
            log_e(LOG_HOZON, "heartbeat time out");
        }
        else
        {}
    }
	 return -1;
}

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
	int res;
	if((tm_get_time() - PP_heartbeat.timer) > (PP_heartbeat.period*1000))
	{
		PP_PackHeader_HB.ver.Byte = task->version;
		PP_PackHeader_HB.nonce  = PrvtPro_BSEndianReverse(task->nonce);
		PP_PackHeader_HB.msglen = PrvtPro_BSEndianReverse((long)18);
		PP_PackHeader_HB.tboxid = PrvtPro_BSEndianReverse(task->tboxid);
		memcpy(&pack_Header, &PP_PackHeader_HB, sizeof(PrvtProt_pack_Header_t));

		res = sockproxy_MsgSend(pack_Header.sign, 18,NULL);
		if(res > 0)//发送成功
		{
			protocol_dump(LOG_HOZON, "PRVT_PROT", pack_Header.sign, 18, 1);
			PP_heartbeat.waitSt = 1;
			//task->heartbeat.ackFlag = PP_ACK_WAIT;
			PP_heartbeat.waittime = tm_get_time();
			PP_heartbeat.timer = tm_get_time();
		}
		else if(res < 0)
		{
			log_e(LOG_HOZON, "send heartbeat frame fail,close socket\r\n");
			//task->heartbeat.timer = tm_get_time();
			PP_heartbeat.waitSt = 0;
			sockproxy_socketclose();//by liujian 20190510
		}
		else
		{}
		return -1;
	}
	return 0;
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
	PP_heartbeat.period = period;
}

/******************************************************
*函数名：PrvtPro_getTimestamp

*形  参：

*返回值：

*描  述：获取时间戳

*备  注：
******************************************************/
long PrvtPro_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return (long)(timestamp.tv_sec);
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
*函数名：PrvtPro_BSEndianReverse

*形  参：void

*返回值：void

*描  述：大端模式和小端模式互相转换

*备  注：
******************************************************/
long PrvtPro_BSEndianReverse(long value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
		   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

