/******************************************************
文件名：	PP_autodoorCtrl.c

描述：	企业私有协议（浙江合众）
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/

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
#include "tcom_api.h"
#include "ble.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "../../../gb32960/gb32960.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "../PrvtProt_SigParse.h"
#include "../PrvtProt_lock.h"

#include "PP_doorLockCtrl.h"


/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/

#define PP_DOORLOCK_CLOSE 1
#define PP_DOORLOCK_OPEN  2

typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtdoorCtrl_pack_t; /**/

typedef struct
{
	PP_rmtdoorCtrl_pack_t 	pack;
	PP_rmtdoorCtrlSt_t		state;
	uint8_t                 success_flag;
}__attribute__((packed))  PrvtProt_rmtdoorCtrl_t; 

static PrvtProt_rmtdoorCtrl_t PP_rmtdoorCtrl;
static unsigned long long PP_Respwaittime = 0;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/

/******************************************************
函数名PP_rmtCtrl_init

input：void

output：void

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
函数名 ：PP_doorLockCtrl_mainfunction

input  ：void

output ：void

******************************************************/

int PP_doorLockCtrl_mainfunction(void *task)
{
	int res = 0;
	switch(PP_rmtdoorCtrl.state.CtrlSt)
	{
		case PP_DOORLOCKCTRL_IDLE:
		{	
			if(PP_rmtdoorCtrl.state.req == 1)	
			{
				if(PP_rmtCtrl_cfg_vehicleState() == 0)  //电源状态OFF
				{	
					PP_rmtdoorCtrl.success_flag = 0;
					PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_REQSTART;
					if(PP_rmtdoorCtrl.state.style == RMTCTRL_TSP)
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;         //正在执行
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.expTime = PP_rmtdoorCtrl.state.expTime;
						rmtCtrl_Stpara.reqType =PP_rmtdoorCtrl.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtdoorCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					}
					else// 蓝牙
					{
						log_i(LOG_HOZON,"bluetooth platform");
					}
				}
				else  //不满足门控条件
				{
					log_i(LOG_HOZON," Vehicle status is on.........!");
					PP_rmtdoorCtrl.state.failtype = PP_RMTCTRL_ACCNOOFF;
					PP_rmtdoorCtrl.success_flag = 0;
					PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_END;
				}
				PP_rmtdoorCtrl.state.req = 0;
			}
		}
		break;
		
		case PP_DOORLOCKCTRL_REQSTART:  //下发门控报文
		{
			if(PP_rmtdoorCtrl.state.doorcmd == PP_DOORLOCK_OPEN ) //打开车门
			{
				PP_can_send_data(PP_CAN_DOORLOCK,CAN_OPENDOOR,0);
			}
			else       //锁住车门
			{
				PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLOSEDOOR,0); 
			}
			PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
	
		case PP_DOORLOCKCTRL_RESPWAIT://ִ等待BDM应答
		{
			if((tm_get_time() - PP_Respwaittime) > 200) //延时200毫秒在去检测结果
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
			    {
			     	if(PP_rmtdoorCtrl.state.doorcmd == PP_DOORLOCK_OPEN) // 打开车门结果
			    	 {
			      		if(PP_rmtCtrl_cfg_doorlockSt() == 0) //  打开车门成功
			     		{
			       			log_o(LOG_HOZON,"open door success");
			       			PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLEANDOOR,0); //清除开门标志位
			       			PP_rmtdoorCtrl.success_flag = 1;
			       			PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_END;
			      		}
			     	}
			    	else
			     	{
			      		if(PP_rmtCtrl_cfg_doorlockSt() == 1) //锁门成功
			      		{
			      			 log_o(LOG_HOZON,"lock door success");
			       			 PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLEANDOOR,0); ////清除锁门标志位
			       			 PP_rmtdoorCtrl.success_flag = 1;
			       			 PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_END;
			      		}
			     	}
			    }
			    else//BDM超时
			    {
				     log_o(LOG_HOZON,"BDM response timed out");
					 PP_rmtdoorCtrl.state.failtype = PP_RMTCTRL_TIMEOUTFAIL;
				     PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLEANDOOR,0);
				     PP_rmtdoorCtrl.success_flag = 0;
				     PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_END;
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
				rmtCtrl_Stpara.rvcFailureType =  PP_rmtdoorCtrl.state.failtype;
				rmtCtrl_Stpara.reqType =PP_rmtdoorCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtdoorCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				rmtCtrl_Stpara.expTime = PP_rmtdoorCtrl.state.expTime;
				if(1 == PP_rmtdoorCtrl.success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  //ִ执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  //ִ执行失败
				}
				res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			else//蓝牙
			{
				PP_rmtCtrl_inform_tb(BT_VEhICLE_DOOR_RESP,PP_rmtdoorCtrl.state.doorcmd,PP_rmtdoorCtrl.success_flag);
			}
			clearPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_DOORLOCK);//释放锁
			PP_rmtdoorCtrl.state.CtrlSt = PP_DOORLOCKCTRL_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}


/******************************************************
函数名 SetPP_doorLockCtrl_Request   设置门控请求

input  ：void

output ：void

******************************************************/

int SetPP_doorLockCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	int mtxlockst = 0;
	mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_DOORLOCK);
	if(PP_LOCK_OK == mtxlockst)
	{
		switch(ctrlstyle)
		{
			case RMTCTRL_TSP:
			{
				PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
				PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
				
				PP_rmtdoorCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				PP_rmtdoorCtrl.state.req = 1;
				PP_rmtdoorCtrl.state.expTime = disptrBody_ptr->expTime;
				if(PP_rmtdoorCtrl.state.reqType == PP_RMTCTRL_DOORLOCKOPEN)
				{
					PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_OPEN;
					log_i(LOG_HOZON,"TSP remote unlock control");
				}
				else
				{
					PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_CLOSE;
					log_i(LOG_HOZON,"TSP remote lock control");
				}
				PP_rmtdoorCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
				PP_rmtdoorCtrl.state.style = RMTCTRL_TSP;
			}
			break;
			case RMTCTRL_BLUETOOTH:
			{
				 unsigned char cmd = *(unsigned char *)appdatarmtCtrl;
				 if(cmd == 1 )//蓝牙锁门
				 {
				 	PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_CLOSE;
					log_i(LOG_HOZON,"bluetooth lock control");
				 }
				 else if (cmd == 2) //蓝牙开门
				 {
				 	PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_OPEN;
					log_i(LOG_HOZON,"bluetooth unlock control");
				 }
				 PP_rmtdoorCtrl.state.req = 1;
				 PP_rmtdoorCtrl.state.style = RMTCTRL_BLUETOOTH;
				 
			}
			break;
			default:
			break;
		}
	}
	return mtxlockst;
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
	if((PP_rmtdoorCtrl.state.CtrlSt == PP_DOORLOCKCTRL_IDLE) && \
			(PP_rmtdoorCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void PP_doorLockCtrl_ClearStatus(void)
{
	clearPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_DOORLOCK);//释放锁
	PP_rmtdoorCtrl.state.req = 0;
}
/************************shell命令测试使用**************************/
void PP_doorLockCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtdoorCtrl.state.reqType = (long)reqType;
	if(PP_rmtdoorCtrl.state.reqType == PP_RMTCTRL_DOORLOCKOPEN)
	{
		PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_OPEN;
	}
	else
	{
		PP_rmtdoorCtrl.state.doorcmd = PP_DOORLOCK_CLOSE;
	}
	PP_rmtdoorCtrl.state.req = 1;
	PP_rmtdoorCtrl.state.style = RMTCTRL_TSP;
}
/************************shell命令测试使用**************************/

