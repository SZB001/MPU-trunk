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
}__attribute__((packed))  PrvtProt_rmtautodoorCtrl_t; /*缁撴瀯浣�*/

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
			
			if(PP_rmtautodoorCtrl.state.req == 1)   //鍒ゆ柇璇锋眰鏄笉鏄�
			{
				PP_rmtautodoorCtrl.state.req = 0;
				autodoor_success_flag = 0;
				auto_door_stage = PP_AUTODOORCTR_REQSTART;
				if(PP_rmtautodoorCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //寮�濮嬫墽琛�
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtautodoorCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtautodoorCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//钃濈墮
				{

				}
			}
		}
		break;
		case PP_AUTODOORCTR_REQSTART:
		{
			if(PP_rmtautodoorCtrl.state.reqType == PP_RMTCTRL_AUTODOOROPEN) //鎵撳紑灏鹃棬
			{
				PP_canSend_setbit(19,2,1);  //鍙戞墦寮�灏鹃棬鎶ユ枃
			}
			else            //鍏抽棴灏鹃棬
			{
				PP_canSend_setbit(19,2,2); //鍙戝叧闂熬闂ㄦ姤鏂�
			}

			auto_door_stage = PP_AUTODOORCTR_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_AUTODOORCTR_RESPWAIT://鎵ц绛夊緟杞︽帶鍝嶅簲
		{
			if(PP_rmtautodoorCtrl.state.reqType == PP_RMTCTRL_AUTODOOROPEN) // 绛夊緟鎵撳紑灏鹃棬缁撴灉
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_reardoorSt() == 2) //灏鹃棬鐘舵��2锛屽熬闂ㄥ紑鍚垚鍔�
					{
						PP_canSend_resetbit(19,2);
						autodoor_success_flag = 1;
						auto_door_stage = PP_AUTODOORCTR_END;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(19,2);
					autodoor_success_flag = 0;
					auto_door_stage = PP_AUTODOORCTR_END;
				}
			}
			else//绛夊緟灏鹃棬鍏抽棴缁撴灉
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_reardoorSt() == 1) //灏鹃棬鐘舵��1锛屽熬闂ㄥ叧闂垚鍔�
					{
						PP_canSend_resetbit(19,2);
						autodoor_success_flag = 1;
						auto_door_stage = PP_AUTODOORCTR_END;
					}
				}
				else//鍝嶅簲瓒呮椂
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
					rmtCtrl_Stpara.rvcReqStatus = 2;  //鎵ц瀹屾垚
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  //鎵ц澶辫触
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				auto_door_stage = PP_AUTODOORCTRL_IDLE;
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




