/******************************************************
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
	int         start_seatheat_stage ;
	unsigned long long PP_Respwaittime;
	int         seatheat_success_flag;
	int seatheat_type ;
	int8_t level;
	
}__attribute__((packed))  PrvtProt_rmtseatheating_t;

static PrvtProt_rmtseatheating_t PP_rmtseatheatCtrl[PP_seatheating_max];
static uint8_t seat_requestpower_flag;   //0默认，1请求上电，2请求下电
void PP_seatheating_init(void)
{
	int i;
	for(i=0 ;i<PP_seatheating_max ;i++)
	{
		memset(&PP_rmtseatheatCtrl[i],0,sizeof(PrvtProt_rmtseatheating_t));
		memcpy(PP_rmtseatheatCtrl[i].pack.Header.sign,"**",2);
		PP_rmtseatheatCtrl[i].pack.Header.ver.Byte = 0x30;
		PP_rmtseatheatCtrl[i].pack.Header.commtype.Byte = 0xe1;
		PP_rmtseatheatCtrl[i].pack.Header.opera = 0x02;
		PP_rmtseatheatCtrl[i].pack.Header.tboxid = 27;
		memcpy(PP_rmtseatheatCtrl[i].pack.DisBody.aID,"110",3);
		PP_rmtseatheatCtrl[i].pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
		PP_rmtseatheatCtrl[i].pack.DisBody.appDataProVer = 256;
		PP_rmtseatheatCtrl[i].pack.DisBody.testFlag = 1;
		PP_rmtseatheatCtrl[i].PP_Respwaittime = 0;
		PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
		PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_IDLE;
		PP_rmtseatheatCtrl[i].state.req = 0;
	}
}


int PP_seatheating_mainfunction(void *task)
{
	int res = 0;
	int i ;
	for(i=0;i<PP_seatheating_max;i++)
	{
		switch(PP_rmtseatheatCtrl[i].start_seatheat_stage)
		{
			case PP_SEATHEATING_IDLE:
			{
				if(PP_rmtseatheatCtrl[i].state.req == 1)	
				{
					if((PP_rmtCtrl_cfg_vehicleSOC()>15) && (PP_rmtCtrl_cfg_vehicleState() == 0))
					{
						PP_rmtseatheatCtrl[i].state.req = 0;
						PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
						
						if(PP_rmtseatheatCtrl[i].state.style == RMTCTRL_TSP)//tsp平台
						{
							PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
							rmtCtrl_Stpara.rvcReqStatus = 1;  
							rmtCtrl_Stpara.rvcFailureType = 0;
							rmtCtrl_Stpara.reqType =PP_rmtseatheatCtrl[i].state.reqType;
							rmtCtrl_Stpara.eventid = PP_rmtseatheatCtrl[i].pack.DisBody.eventId;
							rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
							res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
						}
						else
						{
						}
					}
					else
					{
						PP_rmtseatheatCtrl[i].state.req = 0;
						PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
						PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END;
						
					}
					PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_REQSTART;
				}
				
			}
			break;
			case PP_SEATHEATING_REQSTART:        
			{
				if(PP_get_powerst() == 1)//上高压电成功标志
				{
					if(i == 0)
					{
						PP_can_send_data(PP_CAN_SEATHEAT,PP_rmtseatheatCtrl[i].level,CAN_SEATHEATMAIN);
					}
					else
					{
						PP_can_send_data(PP_CAN_SEATHEAT,PP_rmtseatheatCtrl[i].level,CAN_SEATHEATPASS);
					}
				    PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_RESPWAIT;
				    PP_rmtseatheatCtrl[i].PP_Respwaittime = tm_get_time();
				}
				else if(PP_get_powerst() == 3) //高压电操作失败
				{
					PP_seatheating_ClearStatus();//清除座椅请求
					PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END;
					PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
				}
				else
				{}
			}
			break;
			case PP_SEATHEATING_RESPWAIT:  //  等待BDM应答
		    {
			    if(PP_rmtseatheatCtrl[i].state.reqType == PP_RMTCTRL_MAINHEATOPEN)  //开启应答
			    {
			        if((tm_get_time() - PP_rmtseatheatCtrl[i].PP_Respwaittime) < 2000)
			        {
			            if(PP_rmtCtrl_cfg_HeatingSt(i) == PP_rmtseatheatCtrl[i].level)
			            {
			      		   
			               PP_rmtseatheatCtrl[i].seatheat_success_flag = 1;
			               PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END ;
						   log_o(LOG_HOZON,"Open seat heating successfully\n");
			            }
			        }
			         else //超时
			        {
			           	PP_can_send_data(PP_CAN_SEATHEAT,CAN_CLOSESEATHEAT,CAN_SEATHEATMAIN);
						seat_requestpower_flag = 2;  //座椅加热开启失败请求下电
			            PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
			            PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END ;
			        }
			    }
			    else  //关闭应答
			    {
			         if((tm_get_time() - PP_rmtseatheatCtrl[i].PP_Respwaittime) < 2000)
			         {
						 if(PP_rmtCtrl_cfg_HeatingSt(i) == 0) 
			             {
			             
			                 PP_rmtseatheatCtrl[i].seatheat_success_flag = 1;
							 seat_requestpower_flag = 2;  //座椅加热关闭成功请求下电
			                 PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END ;
							 log_o(LOG_HOZON,"Close seat heating successfully\n");
			             }
			        }
			        else
			        {
			             PP_rmtseatheatCtrl[i].seatheat_success_flag = 0;
			             PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_END ;
			         }
			    }

			}
			break;
			case PP_SEATHEATING_END :
			{
				PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
				if(PP_rmtseatheatCtrl[i].state.style == RMTCTRL_TSP)//tsp
				{
					rmtCtrl_Stpara.reqType =PP_rmtseatheatCtrl[i].state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtseatheatCtrl[i].pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					if(1 == PP_rmtseatheatCtrl[i].seatheat_success_flag)
					{
						rmtCtrl_Stpara.rvcReqStatus = 2; 
						rmtCtrl_Stpara.rvcFailureType = 0;
					}
					else
					{
						rmtCtrl_Stpara.rvcReqStatus = 3;  
						rmtCtrl_Stpara.rvcFailureType = 0xff;
					}
					res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					PP_rmtseatheatCtrl[i].start_seatheat_stage = PP_SEATHEATING_IDLE;
				}
				else
				{

				}
			}
			break;
			default:
			break;
		}
	}
	return res;
}


uint8_t PP_seatheating_start(void) 
{
	if((PP_rmtseatheatCtrl[0].state.req == 1)||(PP_rmtseatheatCtrl[1].state.req == 1))
	{
		//log_o(LOG_HOZON,"seatheat start\n");
		return 1;
		
	}
	else
	{
		return 0;
	}
}


uint8_t PP_seatheating_end(void)
{
	if((PP_rmtseatheatCtrl[0].start_seatheat_stage == PP_SEATHEATING_IDLE) && \
			(PP_rmtseatheatCtrl[0].state.req == 0) &&  \
			(PP_rmtseatheatCtrl[1].start_seatheat_stage == PP_SEATHEATING_IDLE) &&
			(PP_rmtseatheatCtrl[1].state.req == 0))
	{
		return 1;
	}
	else
	{
		//log_o(LOG_HOZON,"seat");
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
			if((appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_MAINHEATOPEN) || \
				(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_MAINHEATCLOSE))
			{
				PP_rmtseatheatCtrl[PP_seatheating_drviver].state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				PP_rmtseatheatCtrl[PP_seatheating_drviver].state.req = 1;
				PP_rmtseatheatCtrl[PP_seatheating_drviver].pack.DisBody.eventId = disptrBody_ptr->eventId;
				PP_rmtseatheatCtrl[PP_seatheating_drviver].state.style = RMTCTRL_TSP;
				PP_rmtseatheatCtrl[PP_seatheating_drviver].level = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0];
			}
			else
			{
				PP_rmtseatheatCtrl[PP_seatheating_passenger].state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				PP_rmtseatheatCtrl[PP_seatheating_passenger].state.req = 1;
				PP_rmtseatheatCtrl[PP_seatheating_passenger].pack.DisBody.eventId = disptrBody_ptr->eventId;
				PP_rmtseatheatCtrl[PP_seatheating_passenger].state.style = RMTCTRL_TSP;
				PP_rmtseatheatCtrl[PP_seatheating_passenger].level = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0];
			}
			if((appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_MAINHEATOPEN)||\
				(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_PASSENGERHEATOPEN))
			{
				seat_requestpower_flag = 1;  //请求上电
			}
		}
		break;
		default:
		break;
	}
}

void PP_seatheating_ClearStatus(void)
{
	PP_rmtseatheatCtrl[0].state.req = 0;
	
	PP_rmtseatheatCtrl[1].state.req = 0;
	
}
int PP_get_seat_requestpower_flag()
{
	return seat_requestpower_flag;
}
void PP_set_seat_requestpower_flag()
{
	seat_requestpower_flag = 0;
}

/************************shell命令测试使用**************************/
void PP_seatheating_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	
	PP_rmtseatheatCtrl[1].state.reqType = (long)reqType;
	PP_rmtseatheatCtrl[1].state.req = 1;
	PP_rmtseatheatCtrl[1].state.style = RMTCTRL_TSP;
	log_o(LOG_HOZON,"request seatheat \n");
	if(PP_rmtseatheatCtrl[1].state.reqType == PP_RMTCTRL_MAINHEATOPEN)
	{
		seat_requestpower_flag = 1;  //请求上电
	}
}
/************************shell命令测试使用**************************/





