/******************************************************
�ļ�����	PrvtProt.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
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
#include "init.h"
#include "log.h"
#include "list.h"
//#include "gb32960.h"
#include "PrvtProt.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
//static PrvtProt_heartbeat_t heartbeat;
static PrvtProt_task_t pp_task;
static PrvtProt_pack_t PP_RxPack;
/*******************************************************
description�� function declaration
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
static int PP_shell_setCtrlParameter(int argc, const char **argv);
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
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
		}
        break;
        case INIT_PHASE_RESTORE:
        break;
        case INIT_PHASE_OUTSIDE:
		{
			 ret |= shell_cmd_register("HOZON_PP_SET", PP_shell_setCtrlParameter, "set HOZON PrvtProt control parameter");
		}
        break;
    }

    return ret;
}


/******************************************************
*��������PrvtProt_run

*��  �Σ�void

*����ֵ��void

*��  �������������߳�

*��  ע��
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
*��������PrvtProt_main

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
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
		
		res = 	PrvtPro_do_checksock(&pp_task) ||
				PrvtPro_do_rcvMsg(&pp_task) ||
				PrvtPro_do_wait(&pp_task) || 
				PrvtProt_do_heartbeat(&pp_task);
    }

    return NULL;
}
#endif

/******************************************************
*��������PrvtProt_do_heartbeat

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static int PrvtProt_do_heartbeat(PrvtProt_task_t *task)
{
	PrvtProt_pack_Header_t pack_Header;

	if((tm_get_time() - task->heartbeat.timer) > (task->heartbeat.period*1000))
	{
		pack_Header.sign[0] = 0x2A;
		pack_Header.sign[1] = 0x2A;
		pack_Header.ver.Byte = 0x30;
		pack_Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)0);
		pack_Header.commtype.Byte = 0x70;
		pack_Header.safetype.Byte = 0x00;
		pack_Header.opera = 0x01;
		pack_Header.msglen = PrvtPro_BSEndianReverse((uint32_t)18);
		pack_Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)204);
		
		if(sockproxy_MsgSend(pack_Header.sign, 18,NULL) > 0)//���ͳɹ�
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
*��������PrvtPro_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
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
*��������PrvtPro_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
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
			(rlen < 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
	{
		return 0;
	}
	PrvtPro_makeUpPack(&PP_RxPack,rcvbuf,rlen);
	protocol_dump(LOG_HOZON, "PRVT_PROT", PP_RxPack.packHeader.sign, 18, 0);
	PrvtPro_RxMsgHandle(task,&PP_RxPack,rlen);

	return 0;
}

/******************************************************
*��������PrvtPro_makeUpPack

*��  �Σ�void

*����ֵ��void

*��  �����������ݽ��

*��  ע��
******************************************************/
static void PrvtPro_makeUpPack(PrvtProt_pack_t *RxPack,uint8_t* input,int len)
{
	int rlen = 0;
	uint8_t rcvstep = 0;

	RxPack->packHeader.sign[0] = input[rlen++];
	RxPack->packHeader.sign[1] = input[rlen++];
	len = len-2;
	while(len--)
	{
		switch(rcvstep)
		{
			case 0://���հ汾��
			{
				RxPack->packHeader.ver.Byte = input[rlen++];
				rcvstep = 1;
			}
			break;
			case 1://����tcp�Ựid
			{
				RxPack->packHeader.nonce = PrvtPro_BSEndianReverse(*((uint32_t*)(&input[rlen])));
				rlen += 4;
				rcvstep = 2;
			}
			break;	
			case 2://���롢���ӵȷ�ʽ
			{
				RxPack->packHeader.commtype.Byte = input[rlen++];
				rcvstep = 3;
			}
			break;	
			case 3://���ܡ�ǩ����ʽ
			{
				RxPack->packHeader.safetype.Byte = input[rlen++];
				rcvstep = 4;
			}
			break;
			case 4://��������
			{
				RxPack->packHeader.opera = input[rlen++];
				rcvstep = 5;
			}
			break;
			case 5://���ĳ���
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
				RxPack->msgdata[rlen-18] = input[rlen++];
			}
			break;
			default:
			break;
		}
	}
}

/******************************************************
*��������PrvtPro_rcvMsgCallback

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PrvtPro_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	switch(rxPack->packHeader.opera)
	{
		case 1://���յ�������
		{
			log_i(LOG_HOZON, "��������!");
			task->heartbeat.state = 1;//��������
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
*��������PrvtPro_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PrvtPro_do_wait(PrvtProt_task_t *task)
{
    if (!task->waitSt)//û���¼��ȴ�Ӧ��
    {
        return 0;
    }

    if((tm_get_time() - task->waittime) > PP_WAIT_TIMEOUT)
    {
        if (task->waitSt == PP_HEARTBEAT)
        {
            task->waitSt = PP_IDLE;
			task->heartbeat.state = 0;//����������
            log_e(LOG_HOZON, "heartbeat time out");
        }
        else
        {
			
		}
    }
	 return -1;
}

/******************************************************
*��������PrvtPro_BSEndianReverse

*��  �Σ�void

*����ֵ��void

*��  �������ģʽ��С��ģʽ����ת��

*��  ע��
******************************************************/
static uint32_t PrvtPro_BSEndianReverse(uint32_t value)
{
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | \
			(value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

/******************************************************
*��������PP_shell_setCtrlParamter

*��  �Σ�
argv[0] - �������� ����λ:��
argv[1] - ���Ƿ���ͣ


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setCtrlParameter(int argc, const char **argv)
{
	char period;
    if (argc != 2)
    {
        shellprintf(" usage: HOZON_PP_SET <heartbeat period> <suspend>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%hu", &period);
	if(period == 0)
	{
		 shellprintf(" usage: heartbeat period invalid\r\n");
		 return -1;
	}	
	pp_task.heartbeat.period = period;
	
	sscanf(argv[1], "%hu", &pp_task.suspend);
    sleep(1);

    return 0;
}
