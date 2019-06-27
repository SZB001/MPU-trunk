/******************************************************
文件名：	PP_autodoorCtrl.c

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
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "../../../gb32960/gb32960.h"
#include "PP_canSend.h"
#include "PP_autodoorCtrl.h"



typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtautodoorCtrl_pack_t; /**/

typedef struct
{
	PP_rmtautodoorCtrl_pack_t 	pack;
	PP_rmtautodoorCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtautodoorCtrl_t; /*结构体*/

static PrvtProt_rmtautodoorCtrl_t PP_rmtautodoorCtrl;
static int auto_door_stage = PP_AUTODOORCTRL_IDLE;
static int autodoor_success_flag = 0;
static unsigned long long PP_Respwaittime = 0;



void PP_autodoorCtrl_init(void)
{
	memset(&PP_rmtautodoorCtrl,0,sizeof(PrvtProt_rmtautodoorCtrl_t));
	memcpy(PP_rmtautodoorCtrl.pack.Header.sign,"**",2);
	PP_rmtautodoorCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtautodoorCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtautodoorCtrl.pack.Header.opera = 0x02;
	PP_rmtautodoorCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtautodoorCtrl.pack.DisBody.aID,"110",3);
	PP_rmtautodoorCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtautodoorCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtautodoorCtrl.pack.DisBody.testFlag = 1;

}

int PP_autodoorCtrl_mainfunction(void *task)
{
	int res = 0;
	switch(auto_door_stage)
	{
		case PP_AUTODOORCTRL_IDLE:
		{
			
			if(PP_rmtautodoorCtrl.state.req == 1)   //判断请求是不是
			{
				PP_rmtautodoorCtrl.state.req = 0;
				autodoor_success_flag = 0;
				auto_door_stage = PP_AUTODOORCTR_REQSTART;
				if(PP_rmtautodoorCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //开始执行
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtautodoorCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtautodoorCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//蓝牙
				{

				}
			}
		}
		break;
		case PP_AUTODOORCTR_REQSTART:
		{
			if(PP_rmtautodoorCtrl.state.reqType == PP_RMTCTRL_AUTODOOROPEN) //打开尾门
			{
				PP_canSend_setbit(19,2,1);  //发打开尾门报文
			}
			else            //关闭尾门
			{
				PP_canSend_setbit(19,2,2); //发关闭尾门报文
			}

			auto_door_stage = PP_AUTODOORCTR_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_AUTODOORCTR_RESPWAIT://执行等待车控响应
		{
			if(PP_rmtautodoorCtrl.state.reqType == PP_RMTCTRL_AUTODOOROPEN) // 等待打开尾门结果
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_reardoorSt() == 2) //尾门状态2，尾门开启成功
					{
						PP_canSend_resetbit(19,2);
						autodoor_success_flag = 1;
						auto_door_stage = PP_AUTODOORCTR_END;
					}
				}
				else//响应超时
				{
					PP_canSend_resetbit(19,2);
					autodoor_success_flag = 0;
					auto_door_stage = PP_AUTODOORCTR_END;
				}
			}
			else//等待尾门关闭结果
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_reardoorSt() == 1) //尾门状态1，尾门关闭成功
					{
						PP_canSend_resetbit(19,2);
						autodoor_success_flag = 1;
						auto_door_stage = PP_AUTODOORCTR_END;
					}
				}
				else//响应超时
				{
					PP_canSend_resetbit(19,2);
					autodoor_success_flag = 0;
					auto_door_stage = PP_AUTODOORCTR_END;
				}
			}
		}
		break;
		case PP_AUTODOORCTR_END:
		{
			
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtautodoorCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtautodoorCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtautodoorCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == autodoor_success_flag)
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
				auto_door_stage = PP_AUTODOORCTRL_IDLE;
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

uint8_t PP_autodoorCtrl_start(void)  
{
	if(PP_rmtautodoorCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t PP_autodoorCtrl_end(void)
{
	if((auto_door_stage == PP_AUTODOORCTRL_IDLE) && \
			(PP_rmtautodoorCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
	return 0;
}

void SetPP_autodoorCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote auto door control req");
			PP_rmtautodoorCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtautodoorCtrl.state.req = 1;
			PP_rmtautodoorCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtautodoorCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}
}

void PP_autodoorCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtautodoorCtrl.state.reqType = (long)reqType;
	PP_rmtautodoorCtrl.state.req = 1;
}




