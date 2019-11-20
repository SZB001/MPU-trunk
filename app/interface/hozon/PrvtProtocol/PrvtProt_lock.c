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
static pthread_mutex_t od_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t odc_mtx = PTHREAD_MUTEX_INITIALIZER;
static PrvtProt_lock_t PP_od_lock;
static PrvtProt_lock_t PP_odc_lock;

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
	memset(&PP_od_lock,0,sizeof(PrvtProt_lock_t));
	memset(&PP_odc_lock,0,sizeof(PrvtProt_lock_t));
}

/******************************************************
*setPP_lock_otadiagmtxlock

*��  �Σ�

*����ֵ��

*设置ota/诊断互斥锁

*��  ע��
******************************************************/
unsigned char setPP_lock_otadiagmtxlock(unsigned char obj)
{
	unsigned char ret = 0;
	pthread_mutex_lock(&od_mtx);

	if(0 == PP_od_lock.flag)
	{
		PP_od_lock.flag = 1;
		PP_od_lock.obj  = obj;
		ret = 1;
	}

	pthread_mutex_unlock(&od_mtx);

	return ret;
}


/******************************************************
*clearPP_lock_otadiagmtxlock

*��  �Σ�

*����ֵ��

*清ota/诊断互斥锁

*
******************************************************/
void clearPP_lock_otadiagmtxlock(unsigned char obj)
{
	pthread_mutex_lock(&od_mtx);

	if((1 == PP_od_lock.flag) && \
		(PP_od_lock.obj  == obj))
	{
		PP_od_lock.flag = 0;
		PP_od_lock.obj  = 0;
	}

	pthread_mutex_unlock(&od_mtx);	
}

/******************************************************
*setPP_lock_diagrmtctrlotamtxlock

*��  �Σ�

*����ֵ��

*设置ota/诊断、远程控制互斥锁

*��  ע��
******************************************************/
unsigned char setPP_lock_diagrmtctrlotamtxlock(unsigned char obj)
{
	unsigned char ret = 0;
	pthread_mutex_lock(&odc_mtx);

	if(0 == PP_odc_lock.flag)
	{
		PP_odc_lock.flag = 1;
		PP_odc_lock.obj  = obj;
		ret = 1;
	}

	pthread_mutex_unlock(&odc_mtx);

	return ret;
}

/******************************************************
*clrPP_lock_diagrmtctrlotamtxlock

*��  �Σ�

*����ֵ��

*清ota/诊断、远程控制互斥锁

*
******************************************************/
void clrPP_lock_diagrmtctrlotamtxlock(unsigned char obj)
{
	pthread_mutex_lock(&odc_mtx);

	if((1 == PP_odc_lock.flag) && \
		(PP_odc_lock.obj  == obj))
	{
		PP_odc_lock.flag = 0;
		PP_odc_lock.obj  = 0;
	}

	pthread_mutex_unlock(&odc_mtx);	
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
	pthread_mutex_lock(&od_mtx);
	pthread_mutex_lock(&odc_mtx);

	log_o(LOG_HOZON, "PP_od_lock.flag = %d\n",PP_od_lock.flag);
	log_o(LOG_HOZON, "PP_od_lock.obj = %d\n",PP_od_lock.obj);
	log_o(LOG_HOZON, "PP_odc_lock.flag = %d\n",PP_odc_lock.flag);
	log_o(LOG_HOZON, "PP_odc_lock.obj = %d\n",PP_odc_lock.obj);

	pthread_mutex_unlock(&odc_mtx);
	pthread_mutex_unlock(&od_mtx);
}