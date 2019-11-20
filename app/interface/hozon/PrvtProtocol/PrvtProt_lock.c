/******************************************************
�ļ�����	PrvtProt_lock.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include "init.h"
#include "log.h"
#include <string.h>
#include <pthread.h>
#include "PrvtProt_lock.h"

/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/


/*Static variable definitions*/
static pthread_mutex_t diagmtx = PTHREAD_MUTEX_INITIALIZER;
static PrvtProt_lock_t PP_diaglock;

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static function declaration
*******************************************************/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPP_lock_parameter

*��  �Σ�

*����ֵ��

*初始化

*��  ע��
******************************************************/
void InitPP_lock_parameter(void)
{
	memset(&PP_diaglock,0,sizeof(PrvtProt_lock_t));
}

/******************************************************
*setPP_lock_otadiagmtxlock

*��  �Σ�

*����ֵ��

*设置诊断互斥锁

*��  ע��
******************************************************/
unsigned char setPP_lock_otadiagmtxlock(unsigned char obj)
{
	unsigned char ret = 0;
	pthread_mutex_lock(&diagmtx);

	if(0 == PP_diaglock.flag)
	{
		PP_diaglock.flag = 1;
		PP_diaglock.obj  = obj;
		ret = 1;
	}

	pthread_mutex_unlock(&diagmtx);

	return ret;
}


/******************************************************
*claerPP_lock_otadiagmtxlock

*��  �Σ�

*����ֵ��

*清诊断互斥锁

*
******************************************************/
void claerPP_lock_otadiagmtxlock(unsigned char obj)
{
	pthread_mutex_lock(&diagmtx);

	if((1 == PP_diaglock.flag) && \
		(PP_diaglock.obj  == obj))
	{
		PP_diaglock.flag = 0;
		PP_diaglock.obj  = 0;
	}

	pthread_mutex_unlock(&diagmtx);	
}

/******************************************************
*showPP_lock_mutexlockstatus

*��  �Σ�

*����ֵ��

*显示互斥锁状态

*
******************************************************/
void showPP_lock_mutexlockstatus(void)
{
	pthread_mutex_lock(&diagmtx);

	log_o(LOG_HOZON, "PP_diaglock.flag = %d\n",PP_diaglock.flag);
	log_o(LOG_HOZON, "PP_diaglock.obj = %d\n",PP_diaglock.obj);

	pthread_mutex_unlock(&diagmtx);
}