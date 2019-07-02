/******************************************************
鏂囦欢鍚嶏細	PP_StartEngine.c

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
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "gb32960_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "../../../gb32960/gb32960.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "../PrvtProt_SigParse.h"

#include "PP_SeatHeating.h"


typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtseatheating_pack_t; /**/

typedef struct
{
	PP_rmtseatheating_pack_t 	pack;
	PP_rmtseatheatingSt_t		state;
}__attribute__((packed))  PrvtProt_rmtseatheating_t; /*缁撴瀯浣�*/

static PrvtProt_rmtseatheating_t PP_rmtseatheatCtrl;
static int start_seatheat_stage = PP_SEATHEATING_IDLE;
static unsigned long long PP_Respwaittime = 0;
static int seatheat_success_flag = 0;

void PP_seatheating_init(void)
{
	memset(&PP_rmtseatheatCtrl,0,sizeof(PrvtProt_rmtseatheating_t));
	memcpy(PP_rmtseatheatCtrl.pack.Header.sign,"**",2);
	PP_rmtseatheatCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtseatheatCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtseatheatCtrl.pack.Header.opera = 0x02;
	PP_rmtseatheatCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtseatheatCtrl.pack.DisBody.aID,"110",3);
	PP_rmtseatheatCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtseatheatCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtseatheatCtrl.pack.DisBody.testFlag = 1;
}


int PP_seatheating_mainfunction(void *task)
{
	int res = 0;
	switch(start_seatheat_stage)
	{
		case PP_SEATHEATING_IDLE:
		{
			if((PP_rmtseatheatCtrl.state.req == 1)&&(gb_data_vehicleSOC() > 15))   //鍒ゆ柇璇锋眰鏄笉鏄�
			{
				PP_rmtseatheatCtrl.state.req = 0;
				seatheat_success_flag = 0;
				start_seatheat_stage = PP_SEATHEATING_REQSTART;
				if(PP_rmtseatheatCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  //寮�濮嬫墽琛�
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtseatheatCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtseatheatCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				}
				else//钃濈墮
				{

				}
			}
			else
			{
				PP_rmtseatheatCtrl.state.req = 0;
				seatheat_success_flag = 0;
				start_seatheat_stage = PP_SEATHEATING_END ;
				
			}
		}
		break;
		case PP_SEATHEATING_REQSTART:
		{
			if(PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_MAINHEATOPEN) //涓诲骇妞呭姞鐑�
			{
				if(PP_rmtseatheatCtrl.state.CtrlSt == 1) //涓诲骇妞呭姞鐑璴ow妗�
				{
					PP_canSend_setbit(CAN_ID_440,28,2,1,NULL);  
				}
				else if(PP_rmtseatheatCtrl.state.CtrlSt == 2) //涓诲骇妞呭姞鐑璵id妗�
				{
					PP_canSend_setbit(CAN_ID_440,28,2,2,NULL); 
				}
				else 
				{
					PP_canSend_setbit(CAN_ID_440,28,2,3,NULL); 
				}
				
			}
			else if(PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_MAINHEATCLOSE) //涓诲骇妞呭姞鐑叧闂�
			{
				PP_canSend_setbit(CAN_ID_440,28,2,0,NULL); 
			}
			else if(PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_PASSENGERHEATOPEN) //鍓骇妞呭姞鐑�
			{
				if(PP_rmtseatheatCtrl.state.CtrlSt == 1) //涓诲骇妞呭姞鐑璴ow妗�
				{
					PP_canSend_setbit(CAN_ID_440,30,2,1,NULL);  
				}
				else if(PP_rmtseatheatCtrl.state.CtrlSt == 2) //涓诲骇妞呭姞鐑璵id妗�
				{
					PP_canSend_setbit(CAN_ID_440,30,2,2,NULL); 
				}
				else 
				{
					PP_canSend_setbit(CAN_ID_440,30,2,3,NULL); 
				}
			}
			else                            //鍓骇妞呭姞鐑叧闂�
			{
				PP_canSend_setbit(CAN_ID_440,30,2,0,NULL);   
			}
			start_seatheat_stage = PP_SEATHEATING_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_SEATHEATING_RESPWAIT://鎵ц绛夊緟杞︽帶鍝嶅簲
		{
			if(PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_MAINHEATOPEN) //涓诲骇妞呭姞鐑紑鍚粨鏋�
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if((PrvtProt_SignParse_DrivHeatingSt() == 1)|| \
						(PrvtProt_SignParse_DrivHeatingSt() == 2)|| \
						(PrvtProt_SignParse_DrivHeatingSt() == 3));
					{
						//PP_canSend_resetbit(28,2);   //搴ф鍔犵儹娓呬笉娓呭師鏉ョ殑can鎶ユ枃浣�
						seatheat_success_flag = 1;
						start_seatheat_stage = PP_SEATHEATING_END ;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(CAN_ID_440,28,2); 
					seatheat_success_flag = 0;
					start_seatheat_stage = PP_SEATHEATING_END ;
				}
			}
			else if (PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_MAINHEATCLOSE) //涓诲骇妞呭姞鐑叧闂粨鏋�
			{
				if((tm_get_time() - PP_Respwaittime) < 2000) 
				{
					if(PrvtProt_SignParse_DrivHeatingSt() == 0) //
					{
						PP_canSend_resetbit(CAN_ID_440,28,2);  
						seatheat_success_flag = 1;
						start_seatheat_stage = PP_SEATHEATING_END ;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(CAN_ID_440,28,2);
					seatheat_success_flag = 0;
					start_seatheat_stage = PP_SEATHEATING_END ;
				}
			}
			else if(PP_rmtseatheatCtrl.state.reqType == PP_RMTCTRL_PASSENGERHEATOPEN) //鍓骇妞呭姞鐑紑鍚粨鏋�
			{
				if((tm_get_time() - PP_Respwaittime) < 2000) 
				{
					if((PrvtProt_SignParse_DrivHeatingSt() == 1)|| \
						(PrvtProt_SignParse_DrivHeatingSt() == 2)|| \
						(PrvtProt_SignParse_DrivHeatingSt() == 3));
					{
						//PP_canSend_resetbit(30,2);
						seatheat_success_flag = 1;
						start_seatheat_stage = PP_SEATHEATING_END ;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(CAN_ID_440,30,2);
					seatheat_success_flag = 0;
					start_seatheat_stage = PP_SEATHEATING_END ;
				}
			}
			else  //鍓骇妞呭姞鐑叧闂粨鏋�
			{
				if((tm_get_time() - PP_Respwaittime) < 2000) 
				{
					if(PrvtProt_SignParse_DrivHeatingSt() == 0) //
					{
						PP_canSend_resetbit(CAN_ID_440,30,2);
						seatheat_success_flag = 1;
						start_seatheat_stage = PP_SEATHEATING_END ;
					}
				}
				else//鍝嶅簲瓒呮椂
				{
					PP_canSend_resetbit(CAN_ID_440,30,2);
					seatheat_success_flag = 0;
					start_seatheat_stage = PP_SEATHEATING_END ;
				}
			}
		}
		break;
		case PP_SEATHEATING_END :
		{
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtseatheatCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtseatheatCtrl.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtseatheatCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == seatheat_success_flag)
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
				start_seatheat_stage = PP_SEATHEATING_IDLE;
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


uint8_t PP_seatheating_start(void) 
{
	if(PP_rmtseatheatCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


uint8_t PP_seatheating_end(void)
{
	if((start_seatheat_stage == PP_SEATHEATING_IDLE) && \
			(PP_rmtseatheatCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void SetPP_seatheating_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote door lock control req");
			PP_rmtseatheatCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtseatheatCtrl.state.CtrlSt = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0]; //搴ф鍔犵儹鐨勬。浣�
			PP_rmtseatheatCtrl.state.req = 1;
			PP_rmtseatheatCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtseatheatCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}
}

void PP_seatheating_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtseatheatCtrl.state.reqType = (long)reqType;
	PP_rmtseatheatCtrl.state.req = 1;
}




