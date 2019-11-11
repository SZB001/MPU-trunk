
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
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
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "gb32960_api.h"
#include "cfg_api.h"


#include "../PrvtProt_SigParse.h"

#include "PP_rmtCtrl.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "PP_SendWakeUptime.h"

#include "PP_ACCtrl.h"

#define PP_OPEN_ACC      1
#define PP_CLOSE_ACC     2
#define PP_SETH_ACC      3
#define PP_APPOINTACC    4
#define PP_CANCELAPPOINT 5



typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtACCtrl_pack_t; /**/

typedef struct
{
	PP_rmtACCtrl_pack_t 	pack;
	PP_rmtACCtrlPara_t      CtrlPara;
	PP_rmtACCtrlSt_t		state;
	uint8_t fail;//
	uint8_t failtype;//
}__attribute__((packed))  PrvtProt_rmtACCtrl_t; /*结构体*/


static PrvtProt_rmtACCtrl_t PP_rmtACCtrl;
static uint8_t acc_requestpower_flag = 0;
PP_rmtAC_AppointBook_t  PP_rmtac_AppointBook[ACC_APPOINT_NUM] ;
static PP_rmtAc_Appointperiod_t PP_rmtAc_Appointperiod[7] =
{
	{0,0x01},//星期7
	{1,0x40}, //星期1
	{2,0x20},//星期2
	{3,0x10},//星期3
	{4,0x08},//星期4
	{5,0x04},//星期5
	{6,0x02},//星期6
};
static uint8_t PP_ACtrl_Sleepflag = 0;
static uint8_t temp = 16;
/**************增加空调部分shell命令******************/	
typedef struct {
	uint32_t hour;
	uint32_t min;
	uint32_t id;
	uint16_t cmd;
	uint8_t effectivestate;
}ACCAppointSt;
int AC_Shell_setctrl(int argc, const char **argv)
{
	ACCAppointSt acctrl;
	uint32_t cmd;
    if (argc != 4)
    {
		shellprintf(" usage:  shell_cli.bin hozon_actr cmd id hour min\n");
        return -1;
    }
	sscanf(argv[0], "%u", &cmd);
	sscanf(argv[1], "%u", &acctrl.id);
	sscanf(argv[2], "%u", &acctrl.hour);
	sscanf(argv[3], "%u", &acctrl.min);
	acctrl.cmd = cmd;
	acctrl.effectivestate = 1;
	SetPP_ACCtrl_Request(RMTCTRL_SHELL,(void *)&acctrl,NULL);
	return 0;
}
int AC_Shell_showctrl(int argc, const char **argv)
{
	int i;
	
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
		
			log_o(LOG_HOZON, "PP_rmtac_AppointBook[%d].id = %d",i,PP_rmtac_AppointBook[i].id);
			log_o(LOG_HOZON, "PP_rmtac_AppointBook[%d].hour = %d",i,PP_rmtac_AppointBook[i].hour);
			log_o(LOG_HOZON, "PP_rmtac_AppointBook[%d].min = %d",i,PP_rmtac_AppointBook[i].min);
			log_o(LOG_HOZON, "PP_rmtac_AppointBook[%d].period = %d\n",i,PP_rmtac_AppointBook[i].period);
		}
	} 
	return 0;
}

void remote_acc_shell_init(void)
{
	shell_cmd_register("hozon_actrl_setappoint", AC_Shell_setctrl, "TSP AC CTRL");
	shell_cmd_register("hozon_actrl_showappoint", AC_Shell_showctrl, "TSP AC CTRL");
}
/**********************空调shell******************************/
void PP_ACCtrl_init(void)
{
	int res;
	int i;
	uint32_t len;
	remote_acc_shell_init();
	memset(&PP_rmtACCtrl,0,sizeof(PrvtProt_rmtACCtrl_t));
	memset(&PP_rmtac_AppointBook,0,10*sizeof(PP_rmtAC_AppointBook_t));
	memcpy(PP_rmtACCtrl.pack.Header.sign,"**",2);
	PP_rmtACCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtACCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtACCtrl.pack.Header.opera = 0x02;
	PP_rmtACCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtACCtrl.pack.DisBody.aID,"110",3);
	PP_rmtACCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtACCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtACCtrl.pack.DisBody.testFlag = 1;
	PP_rmtACCtrl.state.expTime = -1;
	len = ACC_APPOINT_NUM*sizeof(PP_rmtAC_AppointBook_t);
	res = cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTACAPPOINT,&PP_rmtac_AppointBook,&len);  //从ROM中读出空调预约记录
	if(res==0) 
	{
		for(i=0;i<ACC_APPOINT_NUM;i++)
		{
			if(PP_rmtac_AppointBook[i].validFlg == 1)  //将有效的预约记录打印出来
			{
				log_e(LOG_HOZON,"There are currently reservation records\n");
				log_e(LOG_HOZON, "PP_rmtac_AppointBook[%d].id = %d\n",i,PP_rmtac_AppointBook[i].id);
				log_e(LOG_HOZON, "PP_rmtac_AppointBook[%d].hour = %d\n",i,PP_rmtac_AppointBook[i].hour);
				log_e(LOG_HOZON, "PP_rmtac_AppointBook[%d].min = %d\n",i,PP_rmtac_AppointBook[i].min);
				log_e(LOG_HOZON, "PP_rmtac_AppointBook[%d].period = %d\n",i,PP_rmtac_AppointBook[i].period);
			}
		}
	}
	
}


int PP_ACCtrl_mainfunction(void *task)
{
	int res = 0;
	switch(PP_rmtACCtrl.state.CtrlSt)
	{
		case PP_ACCTRL_IDLE:
		{
			if(PP_rmtACCtrl.state.req == 1) 	
			{
				if(PP_get_powerst() == 1)//上高压电成功标志
				{
					if(PP_rmtACCtrl.state.style == RMTCTRL_TSP)   //tsp平台
					{
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;    //开始执行
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType =PP_rmtACCtrl.CtrlPara.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtACCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						res = PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
						PP_rmtACCtrl.state.req = 0;
					}
					else if(PP_rmtACCtrl.state.style == RMTCTRL_TBOX)//tbox
					{
						//log_o(LOG_HOZON,"tbox platform\n");
						PP_rmtACCtrl.state.req = 0;	
					}
					else//蓝牙
					{
						PP_rmtACCtrl.state.req = 0;
					}
					PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_REQSTART;
				}
				else if(PP_get_powerst() == 3) //上高压电操作失败
				{
					log_i(LOG_HOZON,"Power failure failed to end the air conditioning control");
					PP_rmtACCtrl.fail = 0;
					PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
					PP_rmtACCtrl.failtype = PP_RMTCTRL_UPPOWERFAIL;
					PP_rmtACCtrl.state.req = 0;
				}
				else
				{
					if(PP_rmtACCtrl.state.accmd == PP_CLOSE_ACC)

					{
						log_i(LOG_HOZON,"Power failure ");
						PP_rmtACCtrl.state.req = 0;
						PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
					}	
					
				}
				//PP_rmtACCtrl.state.req = 0;
			}
		}
		break;
		case PP_ACCTRL_REQSTART:     //下发报文
		{
			if(PP_rmtACCtrl.state.accmd == PP_OPEN_ACC)    //打开空调
			{
				log_o(LOG_HOZON,"request start ac\n");
				PP_can_send_data(PP_CAN_ACCTRL,CAN_OPENACC,0);
				PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_RESPWAIT;
				PP_rmtACCtrl.state.waittime = tm_get_time();	
			}
			else if(PP_rmtACCtrl.state.accmd == PP_CLOSE_ACC)  //关闭空调
			{
				log_o(LOG_HOZON,"request stop ac\n");     
				PP_can_send_data(PP_CAN_ACCTRL,CAN_CLOSEACC,0);
				PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_RESPWAIT;
				PP_rmtACCtrl.state.waittime = tm_get_time();	
			}
			else if(PP_rmtACCtrl.state.accmd == PP_SETH_ACC)   //设置空调温度
			{   
				log_o(LOG_HOZON,"Set the air conditioning temperature\n"); 
				PP_can_send_data(PP_CAN_ACCTRL,CAN_SETACCTEP,(temp-16)*2); 
				PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_RESPWAIT;
				PP_rmtACCtrl.state.waittime = tm_get_time();	
			}
			else
			{
			}		
		}
		break;
		case PP_ACCTRL_RESPWAIT:
		{
			if((tm_get_time() - PP_rmtACCtrl.state.waittime) > 200)
			{
				if(PP_rmtACCtrl.state.accmd == PP_OPEN_ACC)    //打开空调结果
				{
					if((tm_get_time() - PP_rmtACCtrl.state.waittime) < 2000)
					{
						if(PP_rmtCtrl_cfg_ACOnOffSt() == 1)    //打开成功
						{
							log_o(LOG_HOZON,"open air success\n");
							PP_rmtACCtrl.fail     = 0;
							PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
						}
					}
					else//超时
					{
						log_e(LOG_HOZON,"Instruction execution timeout\n");
						PP_can_send_data(PP_CAN_ACCTRL,CAN_OPNEACCFIAL,0);  
						acc_requestpower_flag = 2; //空调开启失败请求下电
						PP_rmtACCtrl.fail     = 1;
						PP_rmtACCtrl.failtype = PP_RMTCTRL_TIMEOUTFAIL;
						PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
					}
				}
				else if(PP_rmtACCtrl.state.accmd == PP_CLOSE_ACC)
				{
					if((tm_get_time() - PP_rmtACCtrl.state.waittime) < 2000)
					{
						//log_o(LOG_HOZON,"PP_rmtCtrl_cfg_ACOnOffSt() = %d",PP_rmtCtrl_cfg_ACOnOffSt());
						if(PP_rmtCtrl_cfg_ACOnOffSt() == 0)   // 关闭成功
						{
							log_o(LOG_HOZON,"close air success\n");
							PP_can_send_data(PP_CAN_ACCTRL,CAN_ACCMD_INVAILD,0);
							PP_can_send_data(PP_CAN_ACCTRL,CAN_CLOSEACCCLEAN,0);
							PP_rmtACCtrl.fail     = 0;
							acc_requestpower_flag = 2; //空调关闭请求下电
							PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
						}
					}	
					else
					{
						log_e(LOG_HOZON,"Instruction execution timeout\n");
						//PP_can_send_data(PP_CAN_ACCTRL,CAN_CLOSEACC,0);  
						PP_can_send_data(PP_CAN_ACCTRL,CAN_CLOSEACCCLEAN,0);
						acc_requestpower_flag = 2; //空调开启失败请求下电
						PP_rmtACCtrl.fail     = 1;
						PP_rmtACCtrl.failtype = PP_RMTCTRL_TIMEOUTFAIL;
						PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
					}
				}
				else if(PP_rmtACCtrl.state.accmd == PP_SETH_ACC)// 设定温度
				{	
					if((tm_get_time() - PP_rmtACCtrl.state.waittime) < 2000)
					{
						//log_o(LOG_HOZON,"PP_rmtCtrl_cfg_LHTemp() = %d",PP_rmtCtrl_cfg_LHTemp());
						if(PP_rmtCtrl_cfg_LHTemp() == temp)
						{
							log_o(LOG_HOZON,"Set the air conditioner temperature = %d successfully \n",temp);
							PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
						}
						//log_o(LOG_HOZON,"PP_rmtCtrl_cfg_LHTemp() = %d",PP_rmtCtrl_cfg_LHTemp());
					}
					else
					{
						log_e(LOG_HOZON,"Instruction execution timeout\n");
						PP_rmtACCtrl.fail     = 1;
						PP_rmtACCtrl.failtype = PP_RMTCTRL_TIMEOUTFAIL;
						PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_END;
					}
				}
				
			}
		}
		break;
		case PP_ACCTRL_END:
		{
			
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			memset(&rmtCtrl_Stpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(PP_rmtACCtrl.state.style == RMTCTRL_TSP)//tsp
			{
				//log_o(LOG_HOZON, "PP_ACCTRL_END");
				rmtCtrl_Stpara.reqType =PP_rmtACCtrl.CtrlPara.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtACCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;//
				if(0 == PP_rmtACCtrl.fail)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqStatus = 3;  
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
				
			}
			else if(PP_rmtACCtrl.state.style == RMTCTRL_TBOX) //TBOX
			{
				//log_o(LOG_HOZON, "PP_ACCTRL_END");
				rmtCtrl_Stpara.reqType =PP_rmtACCtrl.CtrlPara.reqType;
				rmtCtrl_Stpara.eventid = PP_rmtACCtrl.pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCBOOKINGRESP;//
				if(0 == PP_rmtACCtrl.fail)
				{
					rmtCtrl_Stpara.rvcReqStatus = 2;  
					rmtCtrl_Stpara.rvcFailureType = 0;
					rmtCtrl_Stpara.rvcReqCode = 0x0610;
				}
				else
				{
					rmtCtrl_Stpara.rvcReqCode = 0x0611;
					rmtCtrl_Stpara.rvcReqStatus = 3;  
					rmtCtrl_Stpara.rvcFailureType = 0xff;
				}
				rmtCtrl_Stpara.bookingId  = PP_rmtACCtrl.CtrlPara.bookingId;
				rmtCtrl_Stpara.eventid  = PP_rmtACCtrl.pack.DisBody.eventId;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			PP_rmtACCtrl.state.CtrlSt = PP_ACCTRL_IDLE;
		}
		break;
		default:
		break;
	}
	return res;
}
uint8_t PP_ACCtrl_start(void)  
{
	if((PP_rmtACCtrl.state.req == 1)&&(GetPP_rmtCtrl_fotaUpgrade() == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
uint8_t PP_ACCtrl_end(void)
{


	if((PP_rmtACCtrl.state.CtrlSt == PP_ACCTRL_IDLE) && \
			(PP_rmtACCtrl.state.req == 0))
	{
		return 1;
	}
	else
	{
		//log_o(LOG_HOZON,"ACC");
		return 0;
	}
}
void PP_ACCtrl_Delete_Appoint(int index)
{
	
}
int PP_ACC_Appoint_invalid()
{	
	int i;
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 0)
		{
			return i;
		}
	}
	return -1;
}
int PP_ACC_Id_invalid(uint32_t id)
{	
	int i;
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
			if(PP_rmtac_AppointBook[i].id == id)
			{
				return 1;  //返回表示ID重复
			}
		}
	}
	return 0;
}

void SetPP_ACCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	uint32_t appointId = 0;
	PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
			PP_rmtACCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtACCtrl.state.expTime = disptrBody_ptr->expTime;
			if((appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_ACOPEN) || (appdatarmtCtrl_ptr->CtrlReq.rvcReqType== PP_RMTCTRL_ACCLOSE)\
				||(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_SETTEMP))
			{
				if((PP_ACCTRL_IDLE == PP_rmtACCtrl.state.CtrlSt) && \
						(PP_rmtACCtrl.state.req == 0))      //空闲
				{
					PP_rmtACCtrl.state.req = 1;
					PP_rmtACCtrl.state.bookingSt = 0;      //非预约
					PP_rmtACCtrl.CtrlPara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
					if(PP_rmtACCtrl.CtrlPara.reqType == PP_RMTCTRL_ACOPEN)
					{
						PP_rmtACCtrl.state.accmd = PP_OPEN_ACC;
					}
					else if(PP_rmtACCtrl.CtrlPara.reqType == PP_RMTCTRL_ACCLOSE)
					{
						PP_rmtACCtrl.state.accmd = PP_CLOSE_ACC;
						
					}
					else   //设置温度
					{
						PP_rmtACCtrl.state.accmd = PP_SETH_ACC;
						temp = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] ;
						
					}
					PP_rmtACCtrl.state.style   = RMTCTRL_TSP;
					if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_ACOPEN)
					{
						acc_requestpower_flag = 1;  //请求上电
					}
				}
				else
				{
					log_i(LOG_HOZON, "remote ac control req is excuting\n");
				}
			}
			else if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_ACAPPOINTOPEN)
			{
				int index;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];
				if(PP_ACC_Id_invalid(appointId) == 1) // id重复
				{
					rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFAIL;//失败
					rmtCtrl_Stpara.rvcFailureType = 0x08;     //id重复
					log_o(LOG_HOZON,"Reservation ID repeat\n");
				}
				else
				{
					index = PP_ACC_Appoint_invalid();
					if(index == -1)
					{
						log_o(LOG_HOZON,"Air conditioning reservation more than 10");
					}
					else
					{
						PP_rmtac_AppointBook[index].id = appointId;
						PP_rmtac_AppointBook[index].hour = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[4];
						PP_rmtac_AppointBook[index].min = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[5];
						PP_rmtac_AppointBook[index].period = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[6];
						PP_rmtac_AppointBook[index].eventId = disptrBody_ptr->eventId;
						PP_rmtac_AppointBook[index].validFlg  = 1;	
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].id = %d\n",index,PP_rmtac_AppointBook[index].id);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].hour = %d\n",index,PP_rmtac_AppointBook[index].hour);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].min = %d\n",index,PP_rmtac_AppointBook[index].min);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].period = %d\n",index,PP_rmtac_AppointBook[index].period);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].eventId = %d\n",index,PP_rmtac_AppointBook[index].eventId);
					}
					PP_rmtACCtrl.state.dataUpdata = 1;
					//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTACAPPOINT,&PP_rmtac_AppointBook,ACC_APPOINT_NUM*sizeof(PP_rmtAC_AppointBook_t));
					rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				rmtCtrl_Stpara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				rmtCtrl_Stpara.eventid = disptrBody_ptr->eventId;
				rmtCtrl_Stpara.expTime = disptrBody_ptr->expTime;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			else if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_RMTCTRL_ACCANCELAPPOINT)
			{
				int i;
				int cancel_flag = 0;
				appointId = 0;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];
				for(i=0;i<ACC_APPOINT_NUM;i++)
				{
					if(PP_rmtac_AppointBook[i].validFlg  == 1)
					{
						if(PP_rmtac_AppointBook[i].id == appointId)  //
						{
							PP_rmtac_AppointBook[i].validFlg  = 0;
							log_i(LOG_HOZON, "cancel appointment success , ID = %d\n",PP_rmtac_AppointBook[i].id);
							PP_rmtACCtrl.state.dataUpdata = 1;
							rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//执行完成
							rmtCtrl_Stpara.rvcFailureType = 0;
							rmtCtrl_Stpara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
							rmtCtrl_Stpara.eventid = disptrBody_ptr->eventId;
							rmtCtrl_Stpara.expTime = disptrBody_ptr->expTime;
							rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
							cancel_flag = 1;
							PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
						}
					}
				}
				if(cancel_flag == 0)
				{
					log_o(LOG_HOZON,"No such ID = %d",appointId);
					log_e(LOG_HOZON, "appointment id error,exit cancel appointment\n");
				}
			}
			else
			{
			
			}
		}
		break;
		case RMTCTRL_TBOX:
		{
			int i = *(int *)appdatarmtCtrl;
			PP_rmtACCtrl.state.req = 1;
			PP_rmtACCtrl.state.bookingSt = 1;//预约
			PP_rmtACCtrl.CtrlPara.bookingId = PP_rmtac_AppointBook[i].id; //ID
			PP_rmtACCtrl.pack.DisBody.eventId = PP_rmtac_AppointBook[i].eventId; //eventid
			PP_rmtACCtrl.state.accmd = PP_OPEN_ACC;
			PP_rmtACCtrl.state.style   = RMTCTRL_TBOX;
			acc_requestpower_flag = 1;  //预约开空调时间到，请求上高压电
		}
		break;
		case RMTCTRL_BLUETOOTH:
		{
			
		
		}
		break;
		case RMTCTRL_SHELL:
		{
			ACCAppointSt *shell_actrl = (ACCAppointSt*)appdatarmtCtrl;
			if((shell_actrl->cmd == PP_RMTCTRL_ACOPEN) || (shell_actrl->cmd== PP_RMTCTRL_ACCLOSE)\
				||(shell_actrl->cmd == PP_RMTCTRL_SETTEMP))
			{
				PP_rmtACCtrl.state.style = RMTCTRL_TSP;
				if((PP_ACCTRL_IDLE == PP_rmtACCtrl.state.CtrlSt) && \
						(PP_rmtACCtrl.state.req == 0))      //空闲
				{
					PP_rmtACCtrl.state.req = 1;
					PP_rmtACCtrl.state.bookingSt = 0;      //非预约
					PP_rmtACCtrl.CtrlPara.reqType = shell_actrl->cmd;
					if(shell_actrl->cmd == PP_RMTCTRL_ACOPEN)
					{
						PP_rmtACCtrl.state.accmd = PP_OPEN_ACC;
						log_o(LOG_HOZON,"PP_OPEN_ACC");
						acc_requestpower_flag = 1;  //预约开空调时间到，请求上高压电
					}
					else if(shell_actrl->cmd == PP_RMTCTRL_ACCLOSE)
					{
						PP_rmtACCtrl.state.accmd = PP_CLOSE_ACC;
					}
					else   //设置温度
					{
						PP_rmtACCtrl.state.accmd = PP_SETH_ACC;
						temp = shell_actrl->id;
						
					}
					PP_rmtACCtrl.state.style   = RMTCTRL_TBOX;
				}
				else
				{
					log_i(LOG_HOZON, "remote ac control req is excuting\n");
				}
			}
			else if(shell_actrl->cmd == PP_RMTCTRL_ACAPPOINTOPEN)
			{
				int index;
				if(PP_ACC_Id_invalid(shell_actrl->id) == 1) // id重复
				{
					rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFAIL;//失败
					rmtCtrl_Stpara.rvcFailureType = 0x08;     //id重复
					log_o(LOG_HOZON,"Reservation ID repeat\n");
				}
				else
				{
					index = PP_ACC_Appoint_invalid();
					if(index == -1)
					{
						log_o(LOG_HOZON,"Air conditioning reservation more than 10");
					}
					else
					{
						PP_rmtac_AppointBook[index].id = shell_actrl->id;
						PP_rmtac_AppointBook[index].hour = shell_actrl->hour;
						PP_rmtac_AppointBook[index].min = shell_actrl->min;
						PP_rmtac_AppointBook[index].period = 0xff;
						PP_rmtac_AppointBook[index].eventId = 0;
						PP_rmtac_AppointBook[index].validFlg  = 1;	
						
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].id = %d\n",index,PP_rmtac_AppointBook[index].id);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].hour = %d\n",index,PP_rmtac_AppointBook[index].hour);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].min = %d\n",index,PP_rmtac_AppointBook[index].min);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].period = %d\n",index,PP_rmtac_AppointBook[index].period);
						log_i(LOG_HOZON, "PP_rmtac_AppointBook[%d].eventId = %d\n",index,PP_rmtac_AppointBook[index].eventId);
					}
					PP_rmtACCtrl.state.dataUpdata = 1;
					//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTACAPPOINT,&PP_rmtac_AppointBook,ACC_APPOINT_NUM*sizeof(PP_rmtAC_AppointBook_t));
					rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//执行完成
					rmtCtrl_Stpara.rvcFailureType = 0;
				}
				rmtCtrl_Stpara.reqType = shell_actrl->cmd;
				rmtCtrl_Stpara.eventid = 00;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			else if (shell_actrl->cmd == PP_RMTCTRL_ACCANCELAPPOINT)
			{
				int i;
				int cancel_flag = 0;
				for(i=0;i<ACC_APPOINT_NUM;i++)
				{
					if(PP_rmtac_AppointBook[i].validFlg == 1)
					{
						if(PP_rmtac_AppointBook[i].id == shell_actrl->id )
						{
							PP_rmtac_AppointBook[i].validFlg = 0 ;
							PP_rmtACCtrl.state.dataUpdata = 1;
							rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//执行完成
							rmtCtrl_Stpara.rvcFailureType = 0;
							rmtCtrl_Stpara.rvcReqType = shell_actrl->cmd;
							rmtCtrl_Stpara.eventid = 00;
							rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
							cancel_flag = 1;
							PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
							log_o(LOG_HOZON,"Cancel the air conditioning appointment successfully");
							break;
						}
					}	
				}	
				if(cancel_flag == 0)
				{
					log_o(LOG_HOZON,"No such ID :%d",shell_actrl->id);
					log_e(LOG_HOZON, "appointment id error,exit cancel appointment\n");
				}
				

			}


		}
		default:
		break;
	}
}
/******************************************************
*函数名：PP_AcCtrl_chargeStMonitor

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/


void PP_AcCtrl_acStMonitor(void *task)
{
	int i;
	static uint8_t appointPerformFlg = 0;
	//PP_rmtCtrl_Stpara_t rmtCtrl_acStpara;
	static uint64_t delaytime;
	
	/*Check appointment to turn on air conditioner*/
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
			char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
			
			RTCTIME localdatetime;
			
			tm_get_abstime(&localdatetime);
			
			static uint64_t oldsec;
			if(oldsec != localdatetime.sec)
			{
				oldsec = localdatetime.sec;
				log_i(LOG_HOZON,"%s %d:%d:%d\n",wday[localdatetime.week],localdatetime.hour,localdatetime.min,localdatetime.sec);
			}
			
			if(PP_rmtac_AppointBook[i].period & 0x80)//重复
			{
				if(PP_rmtAc_Appointperiod[localdatetime.week].mask & PP_rmtac_AppointBook[i].period)
				{
					if((localdatetime.hour == PP_rmtac_AppointBook[i].hour) && \
										(localdatetime.min == PP_rmtac_AppointBook[i].min))
					{
	
						if(appointPerformFlg == 0)
						{
							log_i(LOG_HOZON,"%d-%d-%d ",(localdatetime.year - 2000), \
								(localdatetime.mon), localdatetime.mday);
							log_i(LOG_HOZON,"%s %d:%d:%d\n", wday[localdatetime.week], \
								localdatetime.hour, localdatetime.min, localdatetime.sec);
							log_i(LOG_HOZON,"Air conditioning reservation time is up, turn on the air conditioner");
							appointPerformFlg = 1;
							SetPP_ACCtrl_Request(RMTCTRL_TBOX,(void *)&i,NULL);	
						}
						delaytime = tm_get_time();
					}
					else
					{	if((tm_get_time() - delaytime) > 3000)
						{
							appointPerformFlg = 0;
						}
					}
				}
			}
			else
			{ //不重复
				if((localdatetime.hour == PP_rmtac_AppointBook[i].hour) && \
						(localdatetime.min == PP_rmtac_AppointBook[i].min))
				{
					SetPP_ACCtrl_Request(RMTCTRL_TBOX,(void *)&i,NULL);
					PP_rmtACCtrl.state.dataUpdata = 1;
					PP_rmtac_AppointBook[i].validFlg = 0;
					//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTACAPPOINT,&PP_rmtac_AppointBook,10*sizeof(PP_rmtAC_AppointBook_t));
				}
			}
		}
	
		/*
		 * 检查睡眠条件
		 * */
		if((PP_rmtCtrl_cfg_ACOnOffSt() != 1)&&(PP_rmtACCtrl.state.req != 1) && \
				(PP_rmtACCtrl.state.CtrlSt == PP_ACCTRL_IDLE))
		{
			PP_ACtrl_Sleepflag = 1;
		}
		else
		{
			PP_ACtrl_Sleepflag = 0;
		}
		/* * 检查掉电  * */

	 	uint8_t powerOffSt;
		powerOffSt = gb32960_PowerOffSt();
		if((powerOffSt == 1)|| PP_ACtrl_Sleepflag)  //掉电
		{
			if(PP_rmtACCtrl.state.dataUpdata == 1)
			{
				if(PP_rmtac_AppointBook[i].bookupdataflag == 1)
				{
					PP_rmtac_AppointBook[i].bookupdataflag = 2;
				}

				//保存预约记录
				log_o(LOG_HOZON,"save ac para when power off\n");
				(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTACAPPOINT,&PP_rmtac_AppointBook,10*sizeof(PP_rmtAC_AppointBook_t));
				PP_rmtACCtrl.state.dataUpdata = 0;
			}
		}
		else
		{}
	}

}
int PP_get_ac_requestpower_flag()
{
	return acc_requestpower_flag ;
}
void PP_set_ac_requestpower_flag()
{
	acc_requestpower_flag = 0;
}

void PP_ACCtrl_ClearStatus(void)
{
	PP_rmtACCtrl.state.req = 0;
}

void PP_ACCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtACCtrl.CtrlPara.reqType = (long)reqType;
	PP_rmtACCtrl.state.req = 1;
}

unsigned char GetPP_ACtrl_Sleep(void)
{
	return PP_ACtrl_Sleepflag;
}

int PP_ACCtrl_waketime(void)
{
	int i;
	waketime ac_time;
	int low_time = 0;   
	int temp_time = 0;
	for(i=0;i<ACC_APPOINT_NUM;i++)
	{
		if(PP_rmtac_AppointBook[i].validFlg == 1)
		{
			ac_time.hour = PP_rmtac_AppointBook[i].hour;
			ac_time.min = PP_rmtac_AppointBook[i].min;
			ac_time.period = PP_rmtac_AppointBook[i].period;
			temp_time = PP_waketime_to_min(&ac_time);
			if(low_time == 0)
			{
				low_time = temp_time;
			}
			else
			{
				if(low_time > temp_time)
				{
					low_time = temp_time;
				}
			}
		}
	}
	return low_time;
}

uint8_t PP_ACCtrl_cmdoff(void)
{
	if(PP_rmtCtrl_cfg_ACOnOffSt() == 1)
	{
		PP_rmtACCtrl.state.accmd = PP_CLOSE_ACC;
		PP_rmtACCtrl.state.req = 1;
	}
	else
	{
		return 1;
	}
	return 0;
}