
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
#include "ble.h"

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
static int startengine_success_flag = 0;  //0默认状态，1上电成功，2，下电成功，3操作失败
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
	PP_rmtengineCtrl.pack.DisBody.eventId = 0;
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
			if(PP_rmtCtrl_cfg_RmtStartSt() == 0)   //上电走此流程
			{
				if((PP_rmtengineCtrl.state.req == 1)|| (PP_get_seat_requestpower_flag() == 1 )||(PP_get_ac_requestpower_flag() == 1))
				{
					log_o(LOG_HOZON,"request power on!!\n");
					if((PP_rmtCtrl_cfg_vehicleSOC()>15) && (PP_rmtCtrl_cfg_vehicleState() == 0))
					{	
						start_engine_stage = PP_STARTENGINE_REQSTART;
						enginecation = PP_POWERON;
						startengine_success_flag = 0;
					}
					else
					{
						start_engine_stage = PP_STARTENGINE_END;
						startengine_success_flag = 3;  //不满足条件，失败标志位置起来
					}
					if(PP_rmtengineCtrl.state.style == RMTCTRL_TSP)//tsp
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;  
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType =PP_rmtengineCtrl.state.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtengineCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					}
				}
			}
		
			if((PP_rmtengineCtrl.state.req == 1)&&(enginecation == PP_POWEROFF)) //下电流程
			{
				if(PP_rmtCtrl_cfg_RmtStartSt() == 2)
				{
					start_engine_stage = PP_STARTENGINE_REQSTART;
					startengine_success_flag = 0;
				}
		
				else
				{
					start_engine_stage = PP_STARTENGINE_END;
					startengine_success_flag = 3;  //不满足条件，失败标志位置起来
				}
				if(PP_rmtengineCtrl.state.style == RMTCTRL_TSP)//tsp
				{
					PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
					rmtCtrl_Stpara.rvcReqStatus = 1;  
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.reqType =PP_rmtengineCtrl.state.reqType;
					rmtCtrl_Stpara.eventid = PP_rmtengineCtrl.pack.DisBody.eventId;
					rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
					res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
				}
			}
			PP_rmtengineCtrl.state.req = 0;
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
			if((tm_get_time() - PP_Respwaittime) > 200)
			{
				if((tm_get_time() - PP_Respwaittime) < 2000)
				{
					if(enginecation == PP_POWERON) //上高压电应答
					{
						if(PP_rmtCtrl_cfg_RmtStartSt() == 2)  // 2s后在远程启动状态
						{
							PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0);
							PP_Engine_time = tm_get_time();       //记录上高压电成功的时间
							startengine_success_flag = 1;  //上电成功
							PP_set_seat_requestpower_flag();
							PP_set_ac_requestpower_flag();
							log_o(LOG_HOZON,"Success on high voltage\n");
							start_engine_stage = PP_STARTENGINE_END;
						}
					}
					else   //下高压电应答
					{
						if(PP_rmtCtrl_cfg_RmtStartSt() == 0) 
						{
							PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0); //将下高压电报文清零
							PP_can_send_data(PP_CAN_SEATHEAT,0,CAN_SEATHEATMAIN);//下电将座椅加热报文清掉
							PP_can_send_data(PP_CAN_SEATHEAT,0,CAN_SEATHEATPASS);//下电将座椅加热报文清掉
							PP_can_send_data(PP_CAN_ACCTRL,CAN_CLOSEACC,0);//下电将空调开启报文关闭
							PP_set_seat_requestpower_flag(); //清除下电标志
							PP_set_ac_requestpower_flag();   //清除下电标志
							startengine_success_flag = 2;   //下电成功
							log_o(LOG_HOZON,"Successful under high voltage\n");
							start_engine_stage = PP_STARTENGINE_END;
						}
					}
				}
				else   //BDM 应答超时
				{
					PP_can_send_data(PP_CAN_ENGINE,CAN_ENGINECLEAN,0);  
					PP_set_seat_requestpower_flag();  
					PP_seatheating_ClearStatus();
					PP_set_ac_requestpower_flag();
					PP_ACCtrl_ClearStatus();
					startengine_success_flag = 3;   //操作失败
					start_engine_stage = PP_STARTENGINE_END;
				}
			}	
		}
		break;
		case PP_STARTENGINE_END:
		{
			if(PP_rmtengineCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				if(enginecation == PP_POWERON )
				{
				rmtCtrl_Stpara.reqType = PP_RMTCTRL_POWERON ;
				}
				else
				{
					rmtCtrl_Stpara.reqType = PP_RMTCTRL_POWEROFF ;
				}
				
				rmtCtrl_Stpara.eventid = PP_rmtengineCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				if((1 == startengine_success_flag)||(2 == startengine_success_flag))
				{
					rmtCtrl_Stpara.rvcReqStatus = 2; 
					rmtCtrl_Stpara.rvcFailureType = 0;
					//log_o(LOG_HOZON,"success");
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			if( PP_rmtengineCtrl.state.style == RMTCTRL_BLUETOOTH)
			{
				TCOM_MSG_HEADER msghdr;
				PrvtProt_respbt_t respbt;
				respbt.msg_type = BT_POWER_CONTROL_RESP;
				respbt.cmd = enginecation;
				if(1 == startengine_success_flag)
				{
					respbt.result = BT_SUCCESS;  //ִ执行成功
					respbt.failtype = 0;
					
				}
				else
				{
					respbt.result = BT_FAIL;  //ִ执行失败
					respbt.failtype = 0;
				}
				msghdr.sender    = MPU_MID_HOZON_PP;
				msghdr.receiver  = MPU_MID_BLE;
				msghdr.msgid     = BLE_MSG_CONTROL;
				msghdr.msglen    = sizeof(PrvtProt_respbt_t);
				tcom_send_msg(&msghdr, &respbt);
			}
			start_engine_stage = PP_STARTENGINE_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}

uint8_t PP_get_powerst()
{
	return startengine_success_flag;
	
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
		return 1;
	}
	else
	{
		return 0;
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
				log_o(LOG_HOZON, "TSP request power on\n");
			}
			else
			{
				enginecation = PP_POWERON; //下高压电
				log_o(LOG_HOZON,"TSP request to shut down the engine\n");
			}
			PP_rmtengineCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtengineCtrl.state.style = RMTCTRL_TSP;
		}
		break;
		case RMTCTRL_BLUETOOTH:	
		{
			 unsigned char cmd = *(unsigned char *)appdatarmtCtrl;
			 if(cmd == 1 )//上高压电
			 {
			 	enginecation = PP_POWERON;
			 }
			 PP_rmtengineCtrl.state.req = 1;
			 PP_rmtengineCtrl.state.style = RMTCTRL_BLUETOOTH;
		}
		break;
#if 0
		case RMTCTRL_TBOX:
		{
			long *cmd = (long *)appdatarmtCtrl;
			PP_rmtengineCtrl.state.reqType = *cmd;
			//判断是否有座椅加热和空调开启时
			if((PP_rmtengineCtrl.state.reqType == PP_RMTCTRL_MAINHEATOPEN)||\
				(PP_rmtengineCtrl.state.reqType == PP_RMTCTRL_ACOPEN))
			{
				enginecation = PP_POWERON;  //上高压电
			}
			else
			{
				enginecation = PP_POWERON; //下高压电
			}
			PP_rmtengineCtrl.state.req = 1;
			PP_rmtengineCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
			PP_rmtengineCtrl.state.style = RMTCTRL_TBOX;
		}		
		break;
#endif
		default:
		break;
	}
}

void PP_startengine_ClearStatus(void)
{
	PP_rmtengineCtrl.state.req = 0;
	PP_set_seat_requestpower_flag();  
	PP_set_ac_requestpower_flag();
}
/*****************************************************************************************
函数名：PP_rmtCtrl_checkenginetime
功能    ：高压上下电检测
上电条件：空调开启请求上电、者座椅加热开启请求上电
下电条件：上电计时15分钟、空调关闭请求下电(座椅加热未开启)、座椅加热关闭请求下电(空调未开启)
*******************************************************************************************/
void PP_rmtCtrl_checkenginetime(void)
{

	if(PP_rmtCtrl_cfg_RmtStartSt() == 0)    //在没有上电的情况下
	{
		if(PP_get_seat_requestpower_flag() == 2)
		{
			PP_set_seat_requestpower_flag();  //当已经下电了，又有请求下电的，清除标志
		}
		if(PP_get_ac_requestpower_flag() == 2)
		{
			PP_set_ac_requestpower_flag();    //当已经下电了，又有请求下电的，清除标志
		}
		if((PP_get_seat_requestpower_flag() == 1 )||(PP_get_ac_requestpower_flag() == 1))
		{
			log_o(LOG_HOZON, "Have seat heating or air conditioning turned on, request power on\n");
			enginecation = PP_POWERON;  
			PP_rmtengineCtrl.state.req = 1;
		}
	}
	if(PP_rmtCtrl_cfg_RmtStartSt() == 2)   //在已经上高压电的情况下 
	{
		if(PP_get_seat_requestpower_flag() == 1 )
		{
			PP_set_seat_requestpower_flag();  //当已经上电了，又有请求上电的，清除标志
		}
		if(PP_get_ac_requestpower_flag() == 1)
		{
			PP_set_ac_requestpower_flag(); //当已经上电了，又有请求上电的，清除标志
		}
		if(tm_get_time() - PP_Engine_time > 15 * 60 * 1000) //15分钟到请求下电
		{
			enginecation = PP_POWEROFF;
			PP_rmtengineCtrl.state.req = 1;
			log_o(LOG_HOZON,"15 minutes have arrived, request to shut down the engine\n");
		}
		if(PP_get_ac_requestpower_flag() == 2)  //空调关闭请求下电
		{
			//座椅加热也未开启，下电
			if((PP_rmtCtrl_cfg_HeatingSt(0) == 0)&&(PP_rmtCtrl_cfg_HeatingSt(1) == 0))
			{
				log_o(LOG_HOZON,"Air conditioning is off, request to power off\n");
				enginecation = PP_POWEROFF;
				PP_rmtengineCtrl.state.req = 1;
			}
		}
		if(PP_get_seat_requestpower_flag() == 2)  //座椅加热关闭请求下电
		{
			if(PP_rmtCtrl_cfg_ACOnOffSt() == 0)   //空调也未开启，下电
			{
				log_o(LOG_HOZON,"The seat is heated off and the request is powered off\n");
				enginecation = PP_POWEROFF;
				PP_rmtengineCtrl.state.req = 1;
		
			}
		}
	}
}

/************负责高压上下电管理**************/
/*************shell命令测试使用***************/
void PP_powermanagement_request(long cmd)
{
	switch(cmd)
	{
		case PP_RMTCTRL_MAINHEATOPEN:
		case PP_RMTCTRL_PASSENGERHEATOPEN:
		case PP_RMTCTRL_ACOPEN:
		{
			enginecation = PP_POWERON;  //上高压电
			PP_rmtengineCtrl.state.req = 1;
			//PP_rmtengineCtrl.state.style = RMTCTRL_TBOX;  //上高压电不区分平台
		}
		break;
		case PP_RMTCTRL_ACCLOSE:
		{
			if((PP_rmtCtrl_cfg_HeatingSt(0) == 0) && (PP_rmtCtrl_cfg_HeatingSt(1) == 0)) //座椅加热也已关闭
			{
				enginecation = PP_POWEROFF;  //下高压电
				PP_rmtengineCtrl.state.req = 1;	
			}
		}
		break;
		case PP_RMTCTRL_MAINHEATCLOSE:
		{
			if((PP_rmtCtrl_cfg_HeatingSt(1) == 0) && (PP_rmtCtrl_cfg_ACOnOffSt()  == 0)) //副驾关闭且空调关闭
			{
				enginecation = PP_POWEROFF;  //下高压电
				PP_rmtengineCtrl.state.req = 1;	
			}
		}
		break;
		case PP_RMTCTRL_PASSENGERHEATCLOSE:
		{
			if((PP_rmtCtrl_cfg_HeatingSt(0) == 0) && (PP_rmtCtrl_cfg_ACOnOffSt()  == 0)) //主驾关闭且空调关闭
			{
				enginecation = PP_POWEROFF;  //下高压电
				PP_rmtengineCtrl.state.req = 1;	
			}
		}
		break;	
		default:
		break;
	}


}




/************************shell命令测试使用**************************/
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
	PP_rmtengineCtrl.state.style = RMTCTRL_TSP;
}
/************************shell命令测试使用**************************/

