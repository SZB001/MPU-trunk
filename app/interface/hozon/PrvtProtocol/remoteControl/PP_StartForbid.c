
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
#include <udef_cfg_api.h>
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
#include "hozon_SP_api.h"
#include "gb32960_api.h"
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
#include "PPrmtCtrl_cfg.h"
#include "../PrvtProt_lock.h"
#include "PP_StartForbid.h"

typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtstartforbid_pack_t; /**/

typedef struct
{
	PP_rmtstartforbid_pack_t 	pack;
	PP_rmtstartforbidSt_t		state;
	uint8_t            success_flag;
}__attribute__((packed))  PrvtProt_rmtstartforbid_t; /*结构体*/

static PrvtProt_rmtstartforbid_t PP_rmtstartforbid;
static unsigned long long PP_Respwaittime = 0;

void PP_startforbid_init(void)
{
	memset(&PP_rmtstartforbid,0,sizeof(PrvtProt_rmtstartforbid_t));
	memcpy(PP_rmtstartforbid.pack.Header.sign,"**",2);
	PP_rmtstartforbid.pack.Header.ver.Byte = 0x30;
	PP_rmtstartforbid.pack.Header.commtype.Byte = 0xe1;
	PP_rmtstartforbid.pack.Header.opera = 0x02;
	PP_rmtstartforbid.pack.Header.tboxid = 27;
	memcpy(PP_rmtstartforbid.pack.DisBody.aID,"110",3);
	PP_rmtstartforbid.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtstartforbid.pack.DisBody.appDataProVer = 256;
	PP_rmtstartforbid.pack.DisBody.testFlag = 1;
	PP_rmtstartforbid.state.req = 0;
}

int PP_startforbid_mainfunction(void *task)
{
	int res = 0;
	switch(PP_rmtstartforbid.state.CtrlSt)
	{
		case PP_STARTFORBID_IDLE:
		{
			if(PP_rmtstartforbid.state.req == 1)
			{
				if(PP_rmtCtrl_cfg_vehicleState() == 0)
				{
					PP_rmtstartforbid.success_flag = 0;
					PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_REQSTART;
					if(PP_rmtstartforbid.state.style == RMTCTRL_TSP)//tsp
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;  
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType =PP_rmtstartforbid.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtstartforbid.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					}
				}
				else
				{
					log_o(LOG_HOZON," Vehicle status is on.........!,Waiting for condition");
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,(void *)(&PP_rmtstartforbid.state.cmd),1);
					PP_rmtstartforbid.success_flag = 0;
					PP_rmtstartforbid.state.failtype = PP_RMTCTRL_ACCNOOFF;
					PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_END;
				}
				PP_rmtstartforbid.state.req = 0;
			}
		}
		break;
		case PP_STARTFORBID_REQSTART:
		{
			if(PP_rmtstartforbid.state.cmd == PP_STARTFORBID_OPEN)  //禁止启动
			{
				PP_can_send_data(PP_CAN_FORBID,CAN_STARTFORBID,0) ;
			}
			else if(PP_rmtstartforbid.state.cmd == PP_STARTFORBID_CLOSE)  
			{
				PP_can_send_data(PP_CAN_FORBID,CAN_NOFORBID,0) ;
			}
			PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_STARTFORBID_RESPWAIT:
		{
			if(tm_get_time() - PP_Respwaittime > 200)
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(PP_rmtstartforbid.state.cmd == PP_STARTFORBID_OPEN) 
					{
						if(PP_rmtCtrl_cfg_cancelEngiSt() == 1) 
						{
							log_o(LOG_HOZON,"forbidstart");
							PP_can_send_data(PP_CAN_FORBID,CAN_FORBIDCLEAN,0) ;
							PP_rmtstartforbid.success_flag = 1;
							PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_END;
						}
					}
					else if(PP_rmtstartforbid.state.cmd == PP_STARTFORBID_CLOSE)
					{
						if(PP_rmtCtrl_cfg_cancelEngiSt() == 0) 
						{
							log_o(LOG_HOZON,"cancel forbidstart");
							PP_can_send_data(PP_CAN_FORBID,CAN_FORBIDCLEAN,0) ;
							PP_rmtstartforbid.success_flag = 1;
							PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_END;
						}
					}
				}
				else
				{
					log_o(LOG_HOZON,"BDM response timed out");
					PP_rmtstartforbid.state.failtype = PP_RMTCTRL_TIMEOUTFAIL;
					PP_can_send_data(PP_CAN_FORBID,CAN_FORBIDCLEAN,0) ;
					PP_rmtstartforbid.success_flag = 0;
					PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_END;
				}
			}
		}
		break;
		case PP_STARTFORBID_END:
		{
			uint8_t cmd = 0;
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtstartforbid.state.style == RMTCTRL_TSP)
			{
				rmtCtrl_Stpara.reqType =PP_rmtstartforbid.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtstartforbid.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				rmtCtrl_Stpara.rvcFailureType = PP_rmtstartforbid.state.failtype;
				if(1 == PP_rmtstartforbid.success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  
					rmtCtrl_Stpara.rvcFailureType = 0;
					(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,(void *)(&cmd),1);
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  
				}
				res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			clearPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_FORBIDSTART);//释放锁
			PP_rmtstartforbid.state.CtrlSt = PP_STARTFORBID_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}

uint8_t PP_startforbid_start(void) 
{
	if(PP_rmtstartforbid.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t PP_startforbid_end(void)
{
	if((PP_rmtstartforbid.state.CtrlSt == PP_STARTFORBID_IDLE) && \
			(PP_rmtstartforbid.state.req == 0))
	{
		return 1;
	}
	else
	{	
		return 0;
	}
}

int SetPP_startforbid_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	int mtxlockst = 0;
	mtxlockst = setPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_FORBIDSTART);
	if(PP_LOCK_OK == mtxlockst)
	{
		switch(ctrlstyle)
		{
			case RMTCTRL_TSP:
			{
				PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
				PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
				PP_rmtstartforbid.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				PP_rmtstartforbid.state.req = 1;
				if(PP_rmtstartforbid.state.reqType == PP_RMTCTRL_BANSTART)
				{
					PP_rmtstartforbid.state.cmd = PP_STARTFORBID_OPEN;	 //禁止启动
					log_i(LOG_HOZON,"TSP remote forbid start control");
				}
				else
				{
					PP_rmtstartforbid.state.cmd = PP_STARTFORBID_CLOSE;	 //取消禁止启动
					log_i(LOG_HOZON,"TSP remote allow start control");
				}
				PP_rmtstartforbid.pack.DisBody.eventId = disptrBody_ptr->eventId;
				PP_rmtstartforbid.state.style = RMTCTRL_TSP;
			}
			break;
			default:
			break;
		}
	}
	return mtxlockst;
}
void PP_startforbid_acStMonitor(void *task)
{
	int res;
	uint32_t len = 1;
	uint8_t cmd;	
	//满足远程控制的条件，检查是否禁止启动或者取消禁止启动
	if(PP_rmtCtrl_cfg_vehicleState() == 0)
	{
		res = cfg_get_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,(void *)(&cmd),&len);
		if(res == 0)
		{
			if(cmd == PP_STARTFORBID_OPEN)
			{
				PP_rmtstartforbid.state.req = 1;
				PP_rmtstartforbid.state.cmd = PP_STARTFORBID_OPEN;
				log_o(LOG_HOZON,"Satisfy the condition, the execution is prohibited");
			}
			else if (cmd == PP_STARTFORBID_CLOSE)
			{
				log_o(LOG_HOZON,"PP_STARTFORBID_CLOSE");
				PP_rmtstartforbid.state.req = 1;
				PP_rmtstartforbid.state.cmd = PP_STARTFORBID_CLOSE;
				log_o(LOG_HOZON,"Satisfy the condition, execute the cancellation prohibition start");
			}
		}
	}	
}

void PP_startforbid_ClearStatus(void)
{
	clearPP_lock_odcmtxlock(PP_LOCK_VEHICTRL_FORBIDSTART);//释放锁
	PP_rmtstartforbid.state.req = 0;
}

/************************shell命令测试使用**************************/

void PP_startforbid_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	//uint8_t cmd = 0;
	#if 1
	PP_rmtstartforbid.state.reqType = (long)reqType;

	PP_rmtstartforbid.state.req = 1;
	if(PP_rmtstartforbid.state.reqType == PP_RMTCTRL_BANSTART)
	{
		PP_rmtstartforbid.state.cmd = PP_STARTFORBID_OPEN;	 //禁止启动
	}
	else
	{
		PP_rmtstartforbid.state.cmd = PP_STARTFORBID_CLOSE;	 
	}
	PP_rmtstartforbid.state.style = RMTCTRL_TSP;
	#endif

	//(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,(void *)(&cmd),1);
					
}

/************************shell命令测试使用**************************/



