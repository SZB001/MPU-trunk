/******************************************************
文件名：	PP_doorLockCtrl.c

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
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "../../../gb32960/gb32960.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "PP_doorLockCtrl.h"

static int doorLock_success_flag = 0;
/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtdoorCtrl_pack_t; /**/

typedef struct
{
	PP_rmtdoorCtrl_pack_t 	pack;
	PP_rmtdoorCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtdoorCtrl_t; /*结构体*/

static PrvtProt_rmtdoorCtrl_t PP_rmtdoorCtrl;
static int door_lock_stage = PP_DOORLOCKCTRL_IDLE;
static unsigned long long PP_Respwaittime = 0;
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_rmtCtrl_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_doorLockCtrl_init(void)
{
	memset(&PP_rmtdoorCtrl,0,sizeof(PrvtProt_rmtdoorCtrl_t));
	memcpy(PP_rmtdoorCtrl.pack.Header.sign,"**",2);
	PP_rmtdoorCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtdoorCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtdoorCtrl.pack.Header.opera = 0x02;
	PP_rmtdoorCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtdoorCtrl.pack.DisBody.aID,"110",3);
	PP_rmtdoorCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtdoorCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtdoorCtrl.pack.DisBody.testFlag = 1;
}

/******************************************************
*函数名：PP_doorLockCtrl_mainfunction

*形  参：void

*返回值：void

*描  述：主任务函数

*备  注：
******************************************************/
int PP_doorLockCtrl_mainfunction(void *task)
{
	int res = 0;
	switch(door_lock_stage)
	{
		case PP_DOORLOCKCTRL_IDLE:
		{
			if((PP_rmtdoorCtrl.state.req == 1)&&(gb_data_vehicleSOC() > 15))   //判断请求是不是
			{
				PP_rmtdoorCtrl.state.req = 0;
				doorLock_success_flag = 0;
				door_lock_stage = PP_DOORLOCKCTRL_REQSTART;
				if(PP_rmtdoorCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //开始执行
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtdoorCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtdoorCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//蓝牙
				{

				}
			}
			else
			{
				PP_rmtdoorCtrl.state.req = 0;
				doorLock_success_flag = 0;
				door_lock_stage = PP_DOORLOCKCTRL_END;
				
			}
		}
		break;
		case PP_DOORLOCKCTRL_REQSTART:
		{
			if(PP_rmtdoorCtrl.state.reqType == 0) //解锁
			{
				PP_canSend_setbit(CAN_ID_440,17,2,2,NULL);  //发解锁报文
			}
			else
			{
				PP_canSend_setbit(CAN_ID_440,17,2,1,NULL); //发上锁报文
			}

			door_lock_stage = PP_DOORLOCKCTRL_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_DOORLOCKCTRL_RESPWAIT://执行等待车控响应
		{
			if(PP_rmtdoorCtrl.state.reqType == 0) //解锁
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_doorlockSt() == 0) //门锁状态为0，解锁程成功
					{
						PP_canSend_resetbit(CAN_ID_440,17,2);
						doorLock_success_flag = 1;
						door_lock_stage = PP_DOORLOCKCTRL_END;
					}
				}
				else//响应超时
				{
					PP_canSend_resetbit(CAN_ID_440,17,2);
					doorLock_success_flag = 0;
					door_lock_stage = PP_DOORLOCKCTRL_END;
				}
			}
			else//上锁
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_doorlockSt() == 1) //门锁状态为0，解锁程成功
					{
						PP_canSend_resetbit(CAN_ID_440,17,2);
						doorLock_success_flag = 1;
						door_lock_stage = PP_DOORLOCKCTRL_END;
					}
				}
				else//响应超时
				{
					PP_canSend_resetbit(CAN_ID_440,17,2);
					doorLock_success_flag = 0;
					door_lock_stage = PP_DOORLOCKCTRL_END;
				}
			}
		}
		break;
		case PP_DOORLOCKCTRL_END:
		{
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtdoorCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtdoorCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtdoorCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == doorLock_success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  //执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				door_lock_stage = PP_DOORLOCKCTRL_IDLE;
			}
			else//蓝牙
			{

			}
		}
		break;
		default:
		break;
	}
	return res;
}

/******************************************************
*函数名：SetPP_doorLockCtrl_Request

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
void SetPP_doorLockCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote door lock control req");
			PP_rmtdoorCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtdoorCtrl.state.req = 1;
			PP_rmtdoorCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtdoorCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}
}

int PP_doorLockCtrl_start(void)
{
	if(PP_rmtdoorCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int PP_doorLockCtrl_end(void)
{
	if((door_lock_stage == PP_DOORLOCKCTRL_IDLE) && \
			(PP_rmtdoorCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/******************************************************
*函数名：PP_doorLockCtrl_SetCtrlReq

*形  参：

*返回值：

*描  述：设置ecall 请求

*备  注：
******************************************************/
void PP_doorLockCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtdoorCtrl.state.reqType = (long)reqType;
	PP_rmtdoorCtrl.state.req = 1;
}

