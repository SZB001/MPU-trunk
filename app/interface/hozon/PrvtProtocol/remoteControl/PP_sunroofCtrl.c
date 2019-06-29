/******************************************************
鏂囦欢鍚嶏細	PP_autodoorCtrl.c

鎻忚堪锛�	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級	
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
#include "PP_sunroofCtrl.h"



typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtsunroofCtrl_pack_t; /**/

typedef struct
{
	PP_rmtsunroofCtrl_pack_t 	pack;
	PP_rmtsunroofCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtsunroofCtrl_t; /*缁撴瀯浣�*/

static PrvtProt_rmtsunroofCtrl_t PP_rmtsunroofCtrl;
static int sunroof_ctrl_stage = PP_SUNROOFCTRL_IDLE;
static int sunroof_success_flag = 0;
static unsigned long long PP_Respwaittime = 0;


void PP_sunroofctrl_init(void)
{
	memset(&PP_rmtsunroofCtrl,0,sizeof(PrvtProt_rmtsunroofCtrl_t));
	memcpy(PP_rmtsunroofCtrl.pack.Header.sign,"**",2);
	PP_rmtsunroofCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtsunroofCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtsunroofCtrl.pack.Header.opera = 0x02;
	PP_rmtsunroofCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtsunroofCtrl.pack.DisBody.aID,"110",3);
	PP_rmtsunroofCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtsunroofCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtsunroofCtrl.pack.DisBody.testFlag = 1;


}


int PP_sunroofctrl_mainfunction(void *task)
{
	int res = 0;
	switch(sunroof_ctrl_stage)
	{
		case PP_SUNROOFCTRL_IDLE:
		{
			
			if(PP_rmtsunroofCtrl.state.req == 1)	//鍒ゆ柇璇锋眰鏄笉鏄�
			{
				PP_rmtsunroofCtrl.state.req = 0;
				sunroof_success_flag = 0;
				sunroof_ctrl_stage = PP_SUNROOFCTRL_REQSTART;
				if(PP_rmtsunroofCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //寮�濮嬫墽琛�
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtsunroofCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtsunroofCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//钃濈墮
				{

				}
			}
		}
		break;
		case PP_SUNROOFCTRL_REQSTART:
		{
			if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFOPEN) //澶╃獥鎵撳紑
			{
				PP_canSend_setbit(47,3,1);	
			}
			else if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFCLOSE)//澶╃獥鍏抽棴
			{
				PP_canSend_setbit(47,3,2);
			}
			else if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFUPWARP)//澶╃獥缈樿捣
			{
				PP_canSend_setbit(47,3,3);
			}
			else  //澶╃獥鍋滄
			{
				PP_canSend_setbit(47,3,4);
			}
			sunroof_ctrl_stage = PP_SUNROOFCTRL_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_SUNROOFCTRL_RESPWAIT://鎵ц绛夊緟杞︽帶鍝嶅簲
		{
			if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFOPEN) //澶╃獥鎵撳紑缁撴灉
			{
				if((tm_get_time() - PP_Respwaittime) < 18000)
				{
					if(gb_data_reardoorSt() == 4) //鐘舵�佷负4锛屽ぉ绐楁墦寮�ok
					{
						PP_canSend_resetbit(47,3);
						sunroof_success_flag = 1;
						sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
					}
				}
				else//
				{
					PP_canSend_resetbit(47,3);
					sunroof_success_flag = 0;
					sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
				}
			}
			else if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFCLOSE)//澶╃獥鍏抽棴缁撴灉
			{
				if((tm_get_time() - PP_Respwaittime) < 18000)
				{
					if(gb_data_reardoorSt() == 2) //
					{
						PP_canSend_resetbit(47,3);
						sunroof_success_flag = 1;
						sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(47,3);
					sunroof_success_flag = 0;
					sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
				}
			}
			else if(PP_rmtsunroofCtrl.state.reqType == PP_RMTCTRL_PNRSUNROOFUPWARP)//澶╃獥缈樿捣缁撴灉
			{
				if((tm_get_time() - PP_Respwaittime) < 18000)
				{
					if(gb_data_reardoorSt() == 0) //
					{
						PP_canSend_resetbit(47,3);
						sunroof_success_flag = 1;
						sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(47,3);
					sunroof_success_flag = 0;
					sunroof_ctrl_stage = PP_SUNROOFCTRL_END;
				}
			}
			else  //澶╃獥鍋滄缁撴灉
			{
			
			}
		}
		break;
		case PP_SUNROOFCTRL_END:
		{
			
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtsunroofCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtsunroofCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtsunroofCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == sunroof_success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  //鎵ц瀹屾垚
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  //鎵ц澶辫触
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				sunroof_ctrl_stage = PP_SUNROOFCTRL_IDLE;
			}
			else//钃濈墮
			{

			}
		}
		break;
		default:
		break;
	}
	return res;

}


uint8_t PP_sunroofctrl_start(void) 
{

	if(PP_rmtsunroofCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


uint8_t PP_sunroofctrl_end(void)
{

	if((sunroof_ctrl_stage == PP_SUNROOFCTRL_IDLE) && \
			(PP_rmtsunroofCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void SetPP_sunroofctrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote door lock control req");
			PP_rmtsunroofCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtsunroofCtrl.state.req = 1;
			PP_rmtsunroofCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtsunroofCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}

}


void PP_sunroofctrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtsunroofCtrl.state.reqType = (long)reqType;
	PP_rmtsunroofCtrl.state.req = 1;

}


