
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
#include "gb32960_api.h"
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
}__attribute__((packed))  PrvtProt_rmtstartforbid_t; /*结构体*/

static PrvtProt_rmtstartforbid_t PP_rmtstartforbid;
static int start_forbid_stage = PP_STARTFORBID_IDLE;
static unsigned long long PP_Respwaittime = 0;
static int startforbid_success_flag = 0;

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
	switch(start_forbid_stage)
	{
		case PP_STARTFORBID_IDLE:
		{
			if(PP_rmtstartforbid.state.req == 1)
			{
				if((PP_rmtCtrl_cfg_vehicleSOC()>15) && (PP_rmtCtrl_cfg_vehicleState() == 0))
				{
					PP_rmtstartforbid.state.req = 0;
					startforbid_success_flag = 0;
					start_forbid_stage = PP_STARTFORBID_REQSTART;
					if(PP_rmtstartforbid.state.style == RMTCTRL_TSP)//tsp
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;  
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType =PP_rmtstartforbid.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtstartforbid.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
					}
					else
					{

					}
				}
				else
				{
					PP_rmtstartforbid.state.req = 0;
					startforbid_success_flag = 0;
					start_forbid_stage = PP_STARTFORBID_END;
				}
			}
		}
		break;
		case PP_STARTFORBID_REQSTART:
		{
			if(PP_rmtstartforbid.state.reqType == 1) 
			{
				//PP_canSend_setbit(CAN_ID_440,31,2,1,NULL);  
			}
			else if(PP_rmtstartforbid.state.reqType == 2) 
			{
				//PP_canSend_setbit(CAN_ID_440,31,2,2,NULL); 
			}
			else
			{
			}
			start_forbid_stage = PP_STARTFORBID_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_STARTFORBID_RESPWAIT:
		{
			if(PP_rmtstartforbid.state.reqType == 1) 
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_doorlockSt() == 0) 
					{
						//PP_canSend_resetbit(CAN_ID_440,31,2);
						startforbid_success_flag = 1;
						start_forbid_stage = PP_STARTFORBID_END;
					}
				}
				else
				{
					//PP_canSend_resetbit(CAN_ID_440,31,2);
					startforbid_success_flag = 0;
					start_forbid_stage = PP_STARTFORBID_END;
				}
			}
			else if(PP_rmtstartforbid.state.reqType == 2)
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(gb_data_doorlockSt() == 1) 
					{
						//PP_canSend_resetbit(CAN_ID_440,31,2);
						startforbid_success_flag = 1;
						start_forbid_stage = PP_STARTFORBID_END;
					}
				}
				else
				{
					//PP_canSend_resetbit(CAN_ID_440,31,2);
					startforbid_success_flag = 0;
					start_forbid_stage = PP_STARTFORBID_END;
				}
			}
			else
			{

			}
		}
		break;
		case PP_STARTFORBID_END:
		{
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtstartforbid.state.style == RMTCTRL_TSP)//tsp
			{
				rmtCtrl_Stpara.reqType =PP_rmtstartforbid.state.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtstartforbid.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == startforbid_success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
				start_forbid_stage = PP_STARTFORBID_IDLE;
			}
			else
			{

			}
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
	if((start_forbid_stage == PP_STARTFORBID_IDLE) && \
			(PP_rmtstartforbid.state.req == 0))
	{
		return 0;
	}
	else
	{	
		return 1;
	}
}


void SetPP_startforbid_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "remote door lock control req");
			PP_rmtstartforbid.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtstartforbid.state.req = 1;
			PP_rmtstartforbid.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtstartforbid.state.style = RMTCTRL_TSP;
		}
		break;
		default:
		break;
	}

}

void ClearPP_startforbid_Request(void)
{
	PP_rmtstartforbid.state.req = 0;
}

void PP_startforbid_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtstartforbid.state.reqType = (long)reqType;
	PP_rmtstartforbid.state.req = 1;

}




