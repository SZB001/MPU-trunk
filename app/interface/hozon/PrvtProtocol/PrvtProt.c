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

#include <sys/prctl.h>
//#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
//#include "gb32960.h"
#include "PrvtProt.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
//static PrvtProt_heartbeat_t heartbeat;
static PrvtProt_task_t pp_task;
static PrvtProt_pack_t PP_RxPack;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void *PrvtProt_main(void);
static void PrvtPro_rcvMsgCallback(char* Msg,int len);
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task);
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* Msg,int len);
static int PrvtPro_do_wait(PrvtProt_task_t *task);
static uint32_t PrvtPro_BSEndianReverse(uint32_t value);
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
    uint32_t reginf = 0, cfglen;
	char startchar[2U] = {0x2A,0x2A};
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			pp_task.heartbeat.ackFlag = 0;
			pp_task.heartbeat.state = 0;//
			pp_task.heartbeat.timer = tm_get_time();
			pp_task.waitSt = PP_IDLE;
			pp_task.waittime = 0;
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			gb32960_ServiceRxConfig(0,startchar,PrvtPro_rcvMsgCallback);
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

    return 0;
}


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
		res = 	PrvtPro_do_wait(&pp_task) || 
				PrvtProt_do_heartbeat(&pp_task);
    }

    return NULL;
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

	if((tm_get_time() - task->heartbeat.timer) > PP_HEART_BEAT_TIME)
	{
		pack_Header.sign[0] = 0x2A;
		pack_Header.sign[1] = 0x2A;
		pack_Header.ver.Byte = 0x30;
		*((uint32_t*)pack_Header.nonce)  = PrvtPro_BSEndianReverse((uint32_t)0);
		pack_Header.commtype.Byte = 0x70;
		pack_Header.safetype.Byte = 0x00;
		pack_Header.opera = 0x01;
		*((uint32_t*)pack_Header.msglen) = PrvtPro_BSEndianReverse((uint32_t)18);
		*((uint32_t*)pack_Header.tboxid) = PrvtPro_BSEndianReverse((uint32_t)204);
		
		if(gb32960_ServiceMsgSend("HOZON privatal protocol",pack_Header.sign, 18) > 0)//发送成功
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
*函数名：PrvtPro_rcvMsgCallback

*形  参：void

*返回值：void

*描  述：接收数据回调函数

*备  注：
******************************************************/
static void PrvtPro_rcvMsgCallback(char* Msg,int len)
{
	int rlen = 0;
	char rcvstep = 0U;
	char *ptr;
	
	protocol_dump(LOG_HOZON, "PRVT_PROT", Msg, len, 0);
	if((Msg[0] != 0x2A) || (Msg[1] != 0x2A) || \
			(len < 18))//判断数据帧头有误或者数据长度不对
	{
		return;
	}
	
	if(len > (18 + PP_MSG_DATA_LEN))//接收数据长度超出缓存buffer长度
	{
		return;
	}
	
	ptr = (char*)PP_RxPack.packHeader.sign;
	for(rlen =0;rlen < len;rlen++)
    {
		ptr[rlen] = Msg[rlen];
    }
	
	PrvtPro_RxMsgHandle(&pp_task,&PP_RxPack,rlen);
}

/******************************************************
*函数名：PrvtPro_rcvMsgCallback

*形  参：void

*返回值：void

*描  述：接收数据处理

*备  注：
******************************************************/
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* Msg,int len)
{
	switch(Msg->packHeader.opera)
	{
		case 1://接收到心跳包
		{
			log_e(LOG_HOZON, "心跳正常!");
			task->heartbeat.state = 1;//正常心跳
			task->waitSt = 0;
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
            log_e(LOG_GB32960, "heartbeat time out");
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
