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
static pthread_mutex_t odc_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint64_t PP_odc_lockflag;
const PrvtProt_lock_mask_t	PP_lock_mask[PP_LOCK_OBJ_MAX] =
{ 
	{PP_LOCK_DIAG_TSPDIAG,PP_LOCK_MASK_TSPDIAG},
	{PP_LOCK_DIAG_ACTIVE,PP_LOCK_MASK_ACTIVE},
    {PP_LOCK_DIAG_CLEAN,PP_LOCK_MASK_CLEAN},
	{PP_LOCK_OTA_READECUVER,PP_LOCK_MASK_READECUVER},
	{PP_LOCK_OTA_FOTAUPDATE,PP_LOCK_MASK_FOTAUPDATE},
	{PP_LOCK_VEHICTRL_AC,PP_LOCK_MASK_AC},
	{PP_LOCK_VEHICTRL_AUTODOOR,PP_LOCK_MASK_AUTODOOR},
	{PP_LOCK_VEHICTRL_BLEKEYSTART,PP_LOCK_MASK_BLEKEYSTART},
	{PP_LOCK_VEHICTRL_CAMERA,PP_LOCK_MASK_CAMERA},
	{PP_LOCK_VEHICTRL_CHRG,PP_LOCK_MASK_CHRG},
	{PP_LOCK_VEHICTRL_DOORLOCK,PP_LOCK_MASK_DOORLOCK},
	{PP_LOCK_VEHICTRL_SEARCHVEHI,PP_LOCK_MASK_SEARCHVEHI},
	{PP_LOCK_VEHICTRL_ENGINE,PP_LOCK_MASK_ENGINE},
	{PP_LOCK_VEHICTRL_FORBIDSTART,PP_LOCK_MASK_FORBIDSTART},
	{PP_LOCK_VEHICTRL_SUNROOF,PP_LOCK_MASK_SUNROOF},
	{PP_LOCK_VEHICTRL_RMTSTART,PP_LOCK_MASK_RMTSTART}
};

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
	PP_odc_lockflag = 0;
}

/******************************************************
*setPP_lock_odcmtxlock

*��  �Σ�

*����ֵ��

*设置ota/诊断、远程控制互斥锁

*��  ע��
******************************************************/
int setPP_lock_odcmtxlock(unsigned char obj)
{
	int ret = PP_LOCK_OK;
	pthread_mutex_lock(&odc_mtx);

	if(0 == PP_odc_lockflag)
	{
		PP_odc_lockflag |= PP_lock_mask[obj].mask;
	}
	else
	{
		switch(obj)
		{
			case PP_LOCK_DIAG_TSPDIAG:
			{
				if((0 == (PP_odc_lockflag & PP_LOCK_MASK_OTA_ALL)) && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_ACTIVE))  && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_CLEAN)))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}
				else
				{
					if(PP_odc_lockflag & PP_LOCK_MASK_READECUVER)
					{
						ret = PP_LOCK_ERR_FOTAREADVER;
					}
					else if(PP_odc_lockflag & PP_LOCK_MASK_FOTAUPDATE)
					{
						ret = PP_LOCK_ERR_FOTAUPDATE;
					}
					else
					{
						ret = PP_LOCK_ERR_DIAGING;
					}
				}
			}
			break;
			case PP_LOCK_DIAG_ACTIVE:
			{
				if((0 == (PP_odc_lockflag & PP_LOCK_MASK_OTA_ALL)) && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_TSPDIAG)) && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_CLEAN)))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}	
				else
				{
					if(PP_odc_lockflag & PP_LOCK_MASK_READECUVER)
					{
						ret = PP_LOCK_ERR_FOTAREADVER;
					}
					else if(PP_odc_lockflag & PP_LOCK_MASK_FOTAUPDATE)
					{
						ret = PP_LOCK_ERR_FOTAUPDATE;
					}
					else
					{
						ret = PP_LOCK_ERR_DIAGING;
					}
				}
			}
			break;
			case PP_LOCK_DIAG_CLEAN:
			{
				if((0 == (PP_odc_lockflag & PP_LOCK_MASK_OTA_ALL)) && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_TSPDIAG)) && \
				   (0 == (PP_odc_lockflag & PP_LOCK_MASK_ACTIVE)))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}
				else
				{
					if(PP_odc_lockflag & PP_LOCK_OTA_READECUVER)
					{
						ret = PP_LOCK_ERR_FOTAREADVER;
					}
					else if(PP_odc_lockflag & PP_LOCK_MASK_FOTAUPDATE)
					{
						ret = PP_LOCK_ERR_FOTAUPDATE;
					}
					else
					{
						ret = PP_LOCK_ERR_DIAGING;
					}
				}
			}
			break;
			case PP_LOCK_OTA_READECUVER:
			{
				if(0 == (PP_odc_lockflag & PP_LOCK_MASK_DIAG_ALL))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}
				else
				{
					ret = PP_LOCK_ERR_DIAGING;
				}
			}
			break;
			case PP_LOCK_OTA_FOTAUPDATE:
			{
				if((0 == (PP_odc_lockflag & PP_LOCK_MASK_DIAG_ALL)) && \
					(0 == (PP_odc_lockflag & PP_LOCK_MASK_CTRL_ALL)))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}
				else
				{
					if(PP_odc_lockflag & PP_LOCK_MASK_DIAG_ALL)
					{
						ret = PP_LOCK_ERR_DIAGING;
					}
					else
					{
						ret = PP_LOCK_ERR_VEHICTRLING;
					}
				}
			}
			break;
			case PP_LOCK_VEHICTRL_AC:
			case PP_LOCK_VEHICTRL_AUTODOOR:
			case PP_LOCK_VEHICTRL_BLEKEYSTART:
			case PP_LOCK_VEHICTRL_CAMERA:
			case PP_LOCK_VEHICTRL_CHRG:
			case PP_LOCK_VEHICTRL_DOORLOCK:
			case PP_LOCK_VEHICTRL_SEARCHVEHI:
			case PP_LOCK_VEHICTRL_ENGINE:
			case PP_LOCK_VEHICTRL_FORBIDSTART:
			case PP_LOCK_VEHICTRL_SUNROOF:
			case PP_LOCK_VEHICTRL_SEAT:
			{
				if(0 == (PP_odc_lockflag & PP_LOCK_MASK_FOTAUPDATE))
				{
					PP_odc_lockflag |= PP_lock_mask[obj].mask;
				}
				else
				{
					ret = PP_LOCK_ERR_FOTAUPDATE;
				}
			}
			break;
			case PP_LOCK_VEHICTRL_RMTSTART:
			{
				PP_odc_lockflag |= PP_lock_mask[obj].mask;
			}
			break;
			default:
			break;
		}
	}
	pthread_mutex_unlock(&odc_mtx);

	return ret;
}

/******************************************************
*clearrPP_lock_odcmtxlock

*��  �Σ�

*����ֵ��

*清ota/诊断、远程控制互斥锁

*
******************************************************/
void clearPP_lock_odcmtxlock(unsigned char obj)
{
	pthread_mutex_lock(&odc_mtx);

	PP_odc_lockflag &= (~PP_lock_mask[obj].mask);

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
	pthread_mutex_lock(&odc_mtx);

	log_o(LOG_HOZON, "PP_odc_lockflag = %d\n",PP_odc_lockflag);

	pthread_mutex_unlock(&odc_mtx);
}