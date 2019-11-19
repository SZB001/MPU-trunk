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
#include "ble.h"

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
#include "../PrvtProt_SigParse.h"
#include "PPrmtCtrl_cfg.h"

#include "PP_searchvehicle.h"

#define PP_SEARCH 2

typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtsearchvehicle_pack_t; /**/

typedef struct
{
	PP_rmtsearchvehicle_pack_t 	pack;
	PP_rmtsearchvehicleSt_t		state;
}__attribute__((packed))  PrvtProt_rmtsearchvehicle_t; /*结构体*/


static PrvtProt_rmtsearchvehicle_t PP_rmtsearchvehicle;
static int search_vehicle_stage = PP_SEARCHVEHICLE_IDLE;
static unsigned long long PP_Respwaittime = 0;
static int serachvehicle_success_flag = 0;
static int search_type = 0;
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
			if(PP_rmtsearchvehicle.state.req == 1)
			{
				if((PP_rmtCtrl_cfg_vehicleState() == 0)||(PP_rmtCtrl_gettestflag()))
				{
					
					serachvehicle_success_flag = 0;
					search_vehicle_stage = PP_SEARCHVEHICLE_REQSTART;
					if(PP_rmtsearchvehicle.state.style == RMTCTRL_TSP)//tsp
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;  //开始执行
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType =PP_rmtsearchvehicle.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtsearchvehicle.pack.DisBody.eventId;
						rmtCtrl_Stpara.expTime =  PP_rmtsearchvehicle.state.expTime;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					}
					else//蓝牙
					{

					}
					
				}
				else
				{
					log_o(LOG_HOZON," low power or power state on");
					PP_rmtsearchvehicle.state.req = 0;
					serachvehicle_success_flag = 0;
					search_vehicle_stage = PP_SEARCHVEHICLE_END;
				}
				PP_rmtsearchvehicle.state.req = 0;
			}	
		}
		break;
		case PP_SEARCHVEHICLE_REQSTART:
		{
			if(search_type == PP_SEARCH) //寻车
			{
				PP_can_send_data(PP_CAN_SEARCH,CAN_SEARCHVEHICLE,0);
			}
			search_vehicle_stage = PP_SEARCHVEHICLE_RESPWAIT;
			PP_Respwaittime = tm_get_time();
		}
		break;
		case PP_SEARCHVEHICLE_RESPWAIT://执行等待车控响应
		{
			if(search_type == PP_SEARCH) //寻车
			{
				if((tm_get_time() - PP_Respwaittime) > 200)
				{
					if((tm_get_time() - PP_Respwaittime) < 2000)
					{
						if(PP_rmtCtrl_cfg_findcarSt() == 1) //
						{
							log_i(LOG_HOZON,"search vehicle success!!!!!\n");
							PP_can_send_data(PP_CAN_SEARCH,CAN_CLEANSEARCH,0);
							serachvehicle_success_flag = 1;
							search_vehicle_stage = PP_SEARCHVEHICLE_END;
						}
					}
					else//响应超时
					{
						PP_can_send_data(PP_CAN_SEARCH,CAN_CLEANSEARCH,0);
						serachvehicle_success_flag = 0;
						search_vehicle_stage = PP_SEARCHVEHICLE_END;
					}
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
				rmtCtrl_Stpara.expTime =  PP_rmtsearchvehicle.state.expTime;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if(1 == serachvehicle_success_flag)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  //执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  //执行失败
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
				
			}
			else//蓝牙
			{
				PP_rmtCtrl_inform_tb(BT_REMOTE_FIND_CAR_RESP,search_type,serachvehicle_success_flag);
				#if 0
				TCOM_MSG_HEADER msghdr;
				PrvtProt_respbt_t respbt;
				respbt.msg_type = BT_REMOTE_FIND_CAR_RESP;
				respbt.cmd = search_type;
				if(1 == serachvehicle_success_flag)
				{
					respbt.cmd_state.execution_result= search_type;  //ִ执行成功
					respbt.failtype = 0;
					
				}
				else
				{
					respbt.cmd_state.execution_result = BT_FAIL;  //ִ执行失败
					respbt.failtype = 0;
				}
				msghdr.sender    = MPU_MID_HOZON_PP;
				msghdr.receiver  = MPU_MID_BLE;
				msghdr.msgid     = BLE_MSG_CONTROL;
				msghdr.msglen    = sizeof(PrvtProt_respbt_t);
				log_o(LOG_HOZON,"send BT success");
				tcom_send_msg(&msghdr, &respbt);
				#endif
			}
			search_vehicle_stage = PP_SEARCHVEHICLE_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}

uint8_t PP_searchvehicle_start(void) 
{
	if((PP_rmtsearchvehicle.state.req == 1)&&(GetPP_rmtCtrl_fotaUpgrade() == 0))
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
		//log_o(LOG_HOZON,"SEARCH");
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

			log_i(LOG_HOZON, "remote searchvehicle control req");
			PP_rmtsearchvehicle.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtsearchvehicle.state.req = 1;
			PP_rmtsearchvehicle.state.expTime = disptrBody_ptr->expTime;
			if(PP_rmtsearchvehicle.state.reqType == PP_RMTCTRL_RMTSRCHVEHICLEOPEN)
			{
				search_type = PP_SEARCH;
			}
			PP_rmtsearchvehicle.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtsearchvehicle.state.style = RMTCTRL_TSP;
		}
		break;
		case RMTCTRL_BLUETOOTH:
		{
			log_o(LOG_HOZON,"BT REQuest");
			 unsigned char cmd = *(unsigned char *)appdatarmtCtrl;
			 if(cmd == 1)
			 {
			 	search_type = PP_SEARCH;
			 }
			 PP_rmtsearchvehicle.state.req = 1;
			 PP_rmtsearchvehicle.state.style = RMTCTRL_BLUETOOTH;
		}
		break;
		default:
		break;
	}
}

void PP_searchvehicle_ClearStatus(void)
{
	PP_rmtsearchvehicle.state.req = 0;
}
/************************shell命令测试使用**************************/

void PP_searchvehicle_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtsearchvehicle.state.reqType = (long)reqType;
	PP_rmtsearchvehicle.state.req = 1;
	if(PP_rmtsearchvehicle.state.reqType == PP_RMTCTRL_RMTSRCHVEHICLEOPEN)
	{
		search_type = PP_SEARCH;
	}
	PP_rmtsearchvehicle.state.style = RMTCTRL_TSP;
}
/************************shell命令测试使用**************************/


