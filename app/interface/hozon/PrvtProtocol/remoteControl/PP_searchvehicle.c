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
#include "PP_searchvehicle.h"



typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtsearchvehicle_pack_t; /**/

typedef struct
{
	PP_rmtsearchvehicle_pack_t 	pack;
	PP_rmtsearchvehicleSt_t		state;
}__attribute__((packed))  PrvtProt_rmtsearchvehicle_t; /*缁撴瀯浣�*/


static PrvtProt_rmtsearchvehicle_t PP_rmtsearchvehicle;
static int search_vehicle_stage = PP_SEARCHVEHICLE_IDLE;
static unsigned long long PP_Respwaittime = 0;
static int serachvehicle_success_flag = 0;

void PP_searchvehicle_init(void)
{
	memset(&PP_rmtsearchvehicle,0,sizeof(PrvtProt_rmtsearchvehicle_t));
	memcpy(PP_rmtsearchvehicle.pack.Header.sign,"**",2);
	PP_rmtsearchvehicle.pack.Header.ver.Byte = 0x30;
	PP_rmtsearchvehicle.pack.Header.commtype.Byte = 0xe1;
	PP_rmtsearchvehicle.pack.Header.opera = 0x02;
	PP_rmtsearchvehicle.pack.Header.tboxid = 27;
	memcpy(PP_rmtsearchvehicle.pack.DisBody.aID,"110",3);
	PP_rmtsearchvehicle.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtsearchvehicle.pack.DisBody.appDataProVer = 256;
	PP_rmtsearchvehicle.pack.DisBody.testFlag = 1;

}

int PP_searchvehicle_mainfunction(void *task)
{
	int res = 0;
	switch(search_vehicle_stage)
	{
		case PP_SEARCHVEHICLE_IDLE:
		{
			if(PP_rmtsearchvehicle.state.req == 1)   //鍒ゆ柇璇锋眰鏄笉鏄�
			{
				PP_rmtsearchvehicle.state.req = 0;
				serachvehicle_success_flag = 0;
				search_vehicle_stage = PP_SEARCHVEHICLE_REQSTART;
				if(PP_rmtsearchvehicle.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //寮�濮嬫墽琛�
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtsearchvehicle.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtsearchvehicle.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//钃濈墮
				{

				}
			}
		}
		break;
		case PP_SEARCHVEHICLE_REQSTART:
		{
			if(PP_rmtsearchvehicle.state.reqType == PP_RMTCTRL_RMTSRCHVEHICLEOPEN) //瀵昏溅
			{
				PP_canSend_setbit(17,2,3);  //瀵昏溅鎶ユ枃
			}
			search_vehicle_stage = PP_SEARCHVEHICLE_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_SEARCHVEHICLE_RESPWAIT://鎵ц绛夊緟杞︽帶鍝嶅簲
		{
			if(PP_rmtsearchvehicle.state.reqType == PP_RMTCTRL_RMTSRCHVEHICLEOPEN) //瀵昏溅
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_doorlockSt() == 0) //闂ㄩ攣鐘舵�佷负0锛岃В閿佺▼鎴愬姛
					{
						PP_canSend_resetbit(17,2);
						serachvehicle_success_flag = 1;
						search_vehicle_stage = PP_SEARCHVEHICLE_END;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(17,2);
					serachvehicle_success_flag = 0;
					search_vehicle_stage = PP_SEARCHVEHICLE_END;
				}
			}
		}
		break;
		case PP_SEARCHVEHICLE_END:
		{
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtsearchvehicle.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtsearchvehicle.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtsearchvehicle.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == serachvehicle_success_flag)
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
				search_vehicle_stage = PP_SEARCHVEHICLE_IDLE;
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

uint8_t PP_searchvehicle_start(void) 
{
	if(PP_rmtsearchvehicle.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t PP_searchvehicle_end(void)
{
	if((search_vehicle_stage == PP_SEARCHVEHICLE_IDLE) && \
			(PP_rmtsearchvehicle.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void SetPP_searchvehicle_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote door lock control req");
			PP_rmtsearchvehicle.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtsearchvehicle.state.req = 1;
			PP_rmtsearchvehicle.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtsearchvehicle.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}
}

void PP_searchvehicle_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtsearchvehicle.state.reqType = (long)reqType;
	PP_rmtsearchvehicle.state.req = 1;
}






