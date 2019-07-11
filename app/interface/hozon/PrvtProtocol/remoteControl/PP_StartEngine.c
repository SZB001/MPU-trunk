/******************************************************
鏂囦欢鍚嶏細	PP_StartEngine.c

鎻忚堪锛�?	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description锛� include the header file
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
#include "../PrvtProt_SigParse.h"
#include "PPrmtCtrl_cfg.h"

#include "PP_StartEngine.h"

#define PP_POWERON  1
#define PP_POWEROFF 0


typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtstartengine_pack_t; /**/

typedef struct
{
	PP_rmtstartengine_pack_t 	pack;
	PP_rmtstartengineSt_t		state;
}__attribute__((packed))  PrvtProt_rmtstartengine_t; /*缁撴瀯浣�?*/

static PrvtProt_rmtstartengine_t PP_rmtengineCtrl;
static int start_engine_stage = PP_STARTENGINE_IDLE;
static unsigned long long PP_Respwaittime = 0;
static int startengine_success_flag = 0;
static unsigned long long PP_Engine_time = 0;
static int enginecation = 0;
void PP_startengine_init(void)
{
	memset(&PP_rmtengineCtrl,0,sizeof(PrvtProt_rmtstartengine_t));
	memcpy(PP_rmtengineCtrl.pack.Header.sign,"**",2);
	PP_rmtengineCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtengineCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtengineCtrl.pack.Header.opera = 0x02;
	PP_rmtengineCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtengineCtrl.pack.DisBody.aID,"110",3);
	PP_rmtengineCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtengineCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtengineCtrl.pack.DisBody.testFlag = 1;

}
int PP_startengine_mainfunction(void *task)
{

	int res = 0;
	switch(start_engine_stage)
	{
		case PP_STARTENGINE_IDLE:
		{
			if(PP_rmtengineCtrl.state.req == 1)	
			{
				if((PP_rmtCtrl_cfg_vehicleSOC()>15) && (PP_rmtCtrl_cfg_vehicleState() == 0))
				{
					startengine_success_flag = 0;
					start_engine_stage = PP_STARTENGINE_REQSTART;
					if(PP_rmtengineCtrl.state.style == RMTCTRL_TSP)//tsp 平台
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;            //开始执行
						rmtCtrl_Stpara.rvcFailureType = 0;     
						rmtCtrl_Stpara.reqType =PP_rmtengineCtrl.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtengineCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
					}
					else      //蓝牙
					{

					}
				}
				else
				{
					PP_rmtengineCtrl.state.req = 0;	
					startengine_success_flag = 0;
					start_engine_stage = PP_STARTENGINE_END;
				}
				PP_rmtengineCtrl.state.req = 0;
			}
		}
		break;
		case PP_STARTENGINE_REQSTART:
		{
			if(enginecation == PP_POWERON) //发上高压电报文
			{
				
				PP_can_send_data(PP_CAN_ENGINE,CAN_STARTENGINE,0);
			}
			else     //发下高压电报文
			{
				if(PP_rmtCtrl_cfg_RmtStartSt() == 2)  //判断是否在远程启动模式下
				{
					
					PP_can_send_data(PP_CAN_ENGINE,CAN_CLOSEENGINE,0);
				}	
			}
			start_engine_stage = PP_STARTENGINE_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_STARTENGINE_RESPWAIT://等待BDM应答
		{
			if(enginecation == PP_POWERON) //上高压电应答
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(PP_rmtCtrl_cfg_RmtStartSt() == 2)  // 2s后在远程启动状态
					{
						PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0);
						//PP_canSend_resetbit(CAN_ID_440,0,1);  //将上高压电报文清零
						PP_Engine_time = tm_get_time();       //记录上高压电成功的时间
						startengine_success_flag = 1;
						start_engine_stage = PP_STARTENGINE_END;
					}
				}
				else   //BDM应答超时
				{
					PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0);  //将上高压电报文清零
					startengine_success_flag = 0;
					start_engine_stage = PP_STARTENGINE_END;
				}
			}
			else   //下高压电应答
			{
				if((tm_get_time() - PP_Respwaittime) < 2000) 
				{
					if(PP_rmtCtrl_cfg_RmtStartSt() == 0) 
					{
						PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0); //将下高压电报文清零
						startengine_success_flag = 1;
						start_engine_stage = PP_STARTENGINE_END;
					}
				}
				else   //BDM 应答超时
				{
					PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0);  //将下高压电报文清零
					startengine_success_flag = 0;
					start_engine_stage = PP_STARTENGINE_END;
				}
			}
		}
		break;
		case PP_STARTENGINE_END:
		{
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtengineCtrl.state.style == RMTCTRL_TSP)//tsp 平台
			{
				rmtCtrl_Stpara.reqType =PP_rmtengineCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtengineCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == startengine_success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  //给平台回复执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;   //给平台回复执行完成
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				
			}
			else  //蓝牙
			{

			}
			start_engine_stage = PP_STARTENGINE_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}


uint8_t PP_startengine_start(void) 
{
	if(PP_rmtengineCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


uint8_t PP_startengine_end(void)
{
	if((start_engine_stage == PP_STARTENGINE_IDLE) && \
			(PP_rmtengineCtrl.state.req == 0))
	{
		return 0;
	}
	else
	{
		log_o(LOG_HOZON,"engine");
		return 1;
	}
}
void SetPP_startengine_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
			PP_rmtengineCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtengineCtrl.state.req = 1;
			if(PP_rmtengineCtrl.state.reqType == PP_RMTCTRL_POWERON)
			{
				enginecation = PP_POWERON;  //上高压电
			}
			else
			{
				enginecation = PP_POWERON; //下高压电
			}
			PP_rmtengineCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtengineCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		case RMTCTRL_BLUETOOTH:	
		break;
		
		case RMTCTRL_TBOX:
		{
//			PP_rmtengineCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			if(PP_rmtengineCtrl.state.reqType == PP_RMTCTRL_POWERON)
			{
				enginecation = PP_POWERON;  //上高压电
			}
			else
			{
				enginecation = PP_POWERON; //下高压电
			}
//			PP_rmtengineCtrl.state.req = 1;
//			PP_rmtengineCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
//			PP_rmtengineCtrl.state.style = RMTCTRL_TBOX;
		}		
		break;
		
		default:
		break;
	}
}


void ClearPP_startengine_Request(void)
{
	PP_rmtengineCtrl.state.req = 0;
}

void PP_startengine_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtengineCtrl.state.reqType = (long)reqType;
	PP_rmtengineCtrl.state.req = 1;
	if(PP_rmtengineCtrl.state.reqType == PP_RMTCTRL_POWERON)
	{
		enginecation = 1;  //上高压电
	}
	else
	{
		enginecation = 0; //下高压电
	}
}

void PP_rmtCtrl_checkenginetime(void)
{
	if((tm_get_time() - PP_Engine_time > 15 * 60 *1000) \
		&& (PP_rmtCtrl_cfg_RmtStartSt() == 2))
	{
		PP_rmtengineCtrl.state.reqType = 0x0801;
		PP_rmtengineCtrl.state.req = 1;
	}
}

