/******************************************************
�ļ�����	
������	����tsp�Խ�socket��·�Ľ������Ͽ�����/�����ݴ���	
Data			Vasion			author
2019/4/17		V1.0			liujian
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
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>
#include "dir.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "log.h"
#include "sockproxy_rxdata.h"

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static pthread_mutex_t wrmtx = 	PTHREAD_MUTEX_INITIALIZER;
static sockProxyObj_t SPObj[SP_MAX];

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
static void ClrSockproxyData_Queue(void);

/******************************************************
description�� function code
******************************************************/

/******************************************************
*��������SockproxyData_Init
*��  �Σ�
*����ֵ��
*��  ����
*��  ע��
******************************************************/
void SockproxyData_Init(void)
{
	pthread_mutex_lock(&wrmtx);
	ClrSockproxyData_Queue();
	pthread_mutex_unlock(&wrmtx);
}


/******************************************************
*��������WrSockproxyData_Queue
*��  �Σ�
*����ֵ��
*��  ����д���ݵ����ݶ���
*��  ע��
******************************************************/
int WrSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len)
{
	int Lng;
	
	if(len > SP_DATA_LNG)
	{
		log_e(LOG_SOCK_PROXY, "rcv data is too long\n");
		return -1;
	}
	
	pthread_mutex_lock(&wrmtx);

	if(1 == SPObj[obj].SPCache[SPObj[obj].HeadLabel].NonEmptyFlg)
	{
		pthread_mutex_unlock(&wrmtx);
		log_e(LOG_SOCK_PROXY, "rcv buff is full\n");
		return -1;
	}

	for(Lng = 0U;Lng< len;Lng++)
	{
		SPObj[obj].SPCache[SPObj[obj].HeadLabel].data[Lng] = data[Lng];
	}
	SPObj[obj].SPCache[SPObj[obj].HeadLabel].len = len;
	SPObj[obj].SPCache[SPObj[obj].HeadLabel].NonEmptyFlg = 1U;/*�÷ǿձ�־*/
	SPObj[obj].HeadLabel++;
	if(SP_QUEUE_LNG == SPObj[obj].HeadLabel)
	{
		SPObj[obj].HeadLabel = 0U;
	}
	
	pthread_mutex_unlock(&wrmtx);
	return 0;
}


/******************************************************
*��������RdSockproxyData_Queue
*��  �Σ�
*����ֵ��
*��  ������ȡ����
*��  ע��
******************************************************/
int RdSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len)
{	
	int Lng;

	if(SPObj[obj].SPCache[SPObj[obj].TialLabel].len > len) 
	{
		log_e(LOG_SOCK_PROXY, "read buff is small\n");
		return -1;
	}

	pthread_mutex_lock(&wrmtx);
	if(0U == SPObj[obj].SPCache[SPObj[obj].TialLabel].NonEmptyFlg) 
	{
		pthread_mutex_unlock(&wrmtx);
		return 0;
	}

	for(Lng = 0U;Lng < SPObj[obj].SPCache[SPObj[obj].TialLabel].len;Lng++)
	{
		data[Lng] =SPObj[obj].SPCache[SPObj[obj].TialLabel].data[Lng];
	}
	SPObj[obj].SPCache[SPObj[obj].TialLabel].NonEmptyFlg = 0U;	/*��ǿձ�־*/
		
	SPObj[obj].TialLabel++;
	if(SP_QUEUE_LNG == SPObj[obj].TialLabel)
	{
		SPObj[obj].TialLabel = 0U;
	}
	pthread_mutex_unlock(&wrmtx);
	return Lng;
}

/******************************************************
*��������ClrUnlockLogCache_Queue
*��  �Σ�
*����ֵ��
*��  ���������ݶ���
*��  ע��
******************************************************/
static void ClrSockproxyData_Queue(void) 
{
	unsigned char  i,j;

	for(i = 0U;i < SP_MAX;i++)
	{
		for(j = 0U;j < SP_QUEUE_LNG;j++)
		{
			SPObj[i].SPCache[j].NonEmptyFlg = 0U;
		}
		SPObj[i].HeadLabel = 0U;
		SPObj[i].TialLabel = 0U;
	}
}
