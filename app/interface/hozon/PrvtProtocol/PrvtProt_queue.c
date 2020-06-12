/******************************************************
�ļ�����	
������
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
#include "PrvtProt_queue.h"

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static PPObj_t PPObj[PP_MAX];
static PPBigBufObj_t PPBigBufObj[PP_BIGBUF_MAX];
static pthread_mutex_t qwrmtx = 	PTHREAD_MUTEX_INITIALIZER;
/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
static void ClrPP_queue(void);

/******************************************************
description�� function code
******************************************************/

/******************************************************
*��������PP_queue_Init
*��  �Σ�
*����ֵ��
*��  ����
*��  ע��
******************************************************/
void PP_queue_Init(void)
{
	pthread_mutex_lock(&qwrmtx);
	ClrPP_queue();
	pthread_mutex_unlock(&qwrmtx);
}


/******************************************************
*��������WrPP_queue
*��  �Σ�
*����ֵ��
*��  ����д���ݵ����ݶ���
*��  ע��
******************************************************/
int WrPP_queue(unsigned char  obj,unsigned char* data,int len)
{
	int Lng;
	
	if(len > PP_DATA_LNG) return -1;
	
	pthread_mutex_lock(&qwrmtx);
	if(1 == PPObj[obj].PPCache[PPObj[obj].HeadLabel].NonEmptyFlg)
	{
		pthread_mutex_unlock(&qwrmtx);
		return -1;
	}

	for(Lng = 0U;Lng< len;Lng++)
	{
		PPObj[obj].PPCache[PPObj[obj].HeadLabel].data[Lng] = data[Lng];
	}
	PPObj[obj].PPCache[PPObj[obj].HeadLabel].len = len;
	PPObj[obj].PPCache[PPObj[obj].HeadLabel].NonEmptyFlg = 1U;
	PPObj[obj].HeadLabel++;
	if(PP_QUEUE_LNG == PPObj[obj].HeadLabel)
	{
		PPObj[obj].HeadLabel = 0U;
	}

	pthread_mutex_unlock(&qwrmtx);
	
	return 0;
}

/******************************************************
*WrPP_BigBuf_queue
*��  �Σ�
*����ֵ��
*��  ����д���ݵ����ݶ���
*��  ע��
******************************************************/
int WrPP_BigBuf_queue(unsigned char  obj,unsigned char* data,int len)
{
	int Lng;
	
	if(len > PP_DATA_BIGBUF_LNG) return -1;

	pthread_mutex_lock(&qwrmtx);
	if(1 == PPBigBufObj[obj].PPCache[PPBigBufObj[obj].HeadLabel].NonEmptyFlg)
	{
		pthread_mutex_unlock(&qwrmtx);
		return -1;
	}

	for(Lng = 0U;Lng< len;Lng++)
	{
		PPBigBufObj[obj].PPCache[PPBigBufObj[obj].HeadLabel].data[Lng] = data[Lng];
	}
	PPBigBufObj[obj].PPCache[PPBigBufObj[obj].HeadLabel].len = len;
	PPBigBufObj[obj].PPCache[PPBigBufObj[obj].HeadLabel].NonEmptyFlg = 1U;
	PPBigBufObj[obj].HeadLabel++;
	if(PP_QUEUE_BIGBUF_LNG == PPBigBufObj[obj].HeadLabel)
	{
		PPBigBufObj[obj].HeadLabel = 0U;
	}
	pthread_mutex_unlock(&qwrmtx);
	return 0;
}


/******************************************************
*������:RdPP_queue
*��  �Σ�
*����ֵ��
*��  ������ȡ����
*��  ע��
******************************************************/
int RdPP_queue(unsigned char  obj,unsigned char* data,int len)
{	
	int Lng;

	if(PPObj[obj].PPCache[PPObj[obj].TialLabel].len > len)
	{
		return -1;//
	}

	pthread_mutex_lock(&qwrmtx);
	if(0U == PPObj[obj].PPCache[PPObj[obj].TialLabel].NonEmptyFlg)
	{
		pthread_mutex_unlock(&qwrmtx);
		return 0;//
	}

	for(Lng = 0U;Lng < PPObj[obj].PPCache[PPObj[obj].TialLabel].len;Lng++)
	{
		data[Lng] =PPObj[obj].PPCache[PPObj[obj].TialLabel].data[Lng];
	}
	PPObj[obj].PPCache[PPObj[obj].TialLabel].NonEmptyFlg = 0U;
		
	PPObj[obj].TialLabel++;
	if(PP_QUEUE_LNG == PPObj[obj].TialLabel)
	{
		PPObj[obj].TialLabel = 0U;
	}
	pthread_mutex_unlock(&qwrmtx);
	return Lng;
}

/******************************************************
*������:RdPP_BigBuf_queue
*��  �Σ�
*����ֵ��
*��  ������ȡ����
*��  ע��
******************************************************/
int RdPP_BigBuf_queue(unsigned char  obj,unsigned char* data,int len)
{	
	int Lng;

	if(PPBigBufObj[obj].PPCache[PPBigBufObj[obj].TialLabel].len > len)
	{
		return -1;//
	}

	pthread_mutex_lock(&qwrmtx);
	if(0U == PPBigBufObj[obj].PPCache[PPBigBufObj[obj].TialLabel].NonEmptyFlg)
	{
		pthread_mutex_unlock(&qwrmtx);
		return 0;//
	}

	for(Lng = 0U;Lng < PPBigBufObj[obj].PPCache[PPBigBufObj[obj].TialLabel].len;Lng++)
	{
		data[Lng] =PPBigBufObj[obj].PPCache[PPBigBufObj[obj].TialLabel].data[Lng];
	}
	PPBigBufObj[obj].PPCache[PPBigBufObj[obj].TialLabel].NonEmptyFlg = 0U;
		
	PPBigBufObj[obj].TialLabel++;
	if(PP_QUEUE_BIGBUF_LNG == PPBigBufObj[obj].TialLabel)
	{
		PPBigBufObj[obj].TialLabel = 0U;
	}
	pthread_mutex_unlock(&qwrmtx);
	return Lng;
}

/******************************************************
*��������ClrUnlockLogCache_Queue
*��  �Σ�
*����ֵ��
*��  ���������ݶ���
*��  ע��
******************************************************/
static void ClrPP_queue(void)
{
	unsigned char  i,j;

	for(i = 0U;i < PP_MAX;i++)
	{
		for(j = 0U;j < PP_QUEUE_LNG;j++)
		{
			PPObj[i].PPCache[j].NonEmptyFlg = 0U;
		}
		PPObj[i].HeadLabel = 0U;
		PPObj[i].TialLabel = 0U;
	}

	for(i = 0U;i < PP_BIGBUF_MAX;i++)
	{
		for(j = 0U;j < PP_QUEUE_BIGBUF_LNG;j++)
		{
			PPBigBufObj[i].PPCache[j].NonEmptyFlg = 0U;
		}
		PPBigBufObj[i].HeadLabel = 0U;
		PPBigBufObj[i].TialLabel = 0U;
	}
}
