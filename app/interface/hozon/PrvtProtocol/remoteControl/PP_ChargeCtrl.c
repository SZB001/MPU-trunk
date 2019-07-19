/******************************************************
�ļ�����	PP_ACCtrl.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
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
#include "cfg_api.h"
#include "../../support/protocol.h"
#include "PPrmtCtrl_cfg.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "tbox_ivi_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "PP_canSend.h"
#include "PP_ChargeCtrl.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtChargeCtrl_pack_t; /**/

typedef struct
{
	PP_rmtChargeCtrl_pack_t 	pack;
	PP_rmtChargeCtrlPara_t		CtrlPara;
	PP_rmtChargeCtrlSt_t		state;
	uint8_t fail;//执行失败状态：1-失败
	uint8_t failtype;//失败类型
}__attribute__((packed))  PrvtProt_rmtChargeCtrl_t; /*�ṹ��*/

static PrvtProt_rmtChargeCtrl_t PP_rmtChargeCtrl;
//static PrvtProt_pack_t 			PP_rmtChargeCtrl_Pack;
//static PrvtProt_App_rmtCtrl_t 	App_rmtChargeCtrl;
static PP_rmtCharge_AppointBook_t		PP_rmtCharge_AppointBook;

static PP_rmtCharge_Appointperiod_t PP_rmtCharge_Appointperiod[7] =
{
	{0,0x01},//星期7
	{1,0x40}, //星期1
	{2,0x20},//星期2
	{3,0x10},//星期3
	{4,0x08},//星期4
	{5,0x04},//星期5
	{6,0x02},//星期6
};

static uint8_t PP_ChargeCtrl_Sleepflag = 0;
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_ChargeCtrl_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_ChargeCtrl_init(void)
{
	unsigned int len;
	int res;
	memset(&PP_rmtChargeCtrl,0,sizeof(PrvtProt_rmtChargeCtrl_t));
	memcpy(PP_rmtChargeCtrl.pack.Header.sign,"**",2);
	PP_rmtChargeCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtChargeCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtChargeCtrl.pack.Header.opera = 0x02;
	PP_rmtChargeCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtChargeCtrl.pack.DisBody.aID,"110",3);
	PP_rmtChargeCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtChargeCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtChargeCtrl.pack.DisBody.testFlag = 1;

	//读取记录
	len = 32;
	res = cfg_get_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,&len);
	if((res==0) && (PP_rmtCharge_AppointBook.validFlg == 1))
	{
		log_e(LOG_HOZON,"There are currently reservation records\n");
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d\n",PP_rmtCharge_AppointBook.id);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
	}
}

/******************************************************
*��������PP_ChargeCtrl_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_ChargeCtrl_mainfunction(void *task)
{
	switch(PP_rmtChargeCtrl.state.CtrlSt)
	{
		case PP_CHARGECTRL_IDLE:
		{
			if(PP_rmtChargeCtrl.state.req == 1)
			{
				if(PP_rmtChargeCtrl.state.chargecmd == PP_CHARGECTRL_OPEN)//请求充电开启
				{
					if((PP_rmtCtrl_cfg_chargeGunCnctSt() == 1) && \
							(PP_rmtCtrl_cfg_readyLightSt() == 0))//充电枪连接 && 车辆非运动模式״̬
					{
						log_o(LOG_HOZON,"start charge ctrl\n");
						if((PP_rmtChargeCtrl.state.style == RMTCTRL_TSP) || \
								(PP_rmtChargeCtrl.state.style == RMTCTRL_HU))
						{
							//log_o(LOG_HOZON,"tsp platform\n");
							PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
							rmtCtrl_Stpara.rvcReqStatus = 1;//��ʼִ��
							rmtCtrl_Stpara.rvcFailureType = 0;
							rmtCtrl_Stpara.reqType = PP_rmtChargeCtrl.CtrlPara.reqType;
							rmtCtrl_Stpara.eventid = PP_rmtChargeCtrl.pack.DisBody.eventId;
							rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
							PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
						}
						else if(PP_rmtChargeCtrl.state.style == RMTCTRL_TBOX)//
						{
							log_o(LOG_HOZON,"tbox platform\n");
						}
						else//
						{
							log_o(LOG_HOZON,"bluetooth platform\n");
						}
						PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_REQSTART;
					}
					else
					{//
						log_i(LOG_HOZON,"The vehicle control condition is not satisfied\n");
						PP_rmtChargeCtrl.fail     = 1;
						if(PP_rmtCtrl_cfg_chargeGunCnctSt() == 1)
						{
							PP_rmtChargeCtrl.failtype = PP_RMTCTRL_CHRGGUNUNCONNT;
						}
						else
						{
							PP_rmtChargeCtrl.failtype = PP_RMTCTRL_READYLIGHTON;
						}
						PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_END;
					}
				}
				else//请求充电关闭
				{
					log_o(LOG_HOZON,"start charge ctrl\n");
					if((PP_rmtChargeCtrl.state.style == RMTCTRL_TSP) || \
							(PP_rmtChargeCtrl.state.style == RMTCTRL_HU))
					{
						//log_o(LOG_HOZON,"tsp platform\n");
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;//��ʼִ��
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType = PP_rmtChargeCtrl.CtrlPara.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtChargeCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
					}
					else if(PP_rmtChargeCtrl.state.style == RMTCTRL_TBOX)//
					{
						log_o(LOG_HOZON,"tbox platform\n");
					}
					else//
					{
						log_o(LOG_HOZON,"bluetooth platform\n");
					}
					PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_REQSTART;
				}
				PP_rmtChargeCtrl.state.chargeSt = 0;
				PP_rmtChargeCtrl.state.req = 0;
			}
		}
		break;
		case PP_CHARGECTRL_REQSTART:
		{
			if(PP_rmtChargeCtrl.state.chargecmd == PP_CHARGECTRL_OPEN)
			{
				log_o(LOG_HOZON,"request start charge\n");
				PP_can_send_data(PP_CAN_CHAGER,CAN_STARTCHAGER,0);
			}
			else
			{
				log_o(LOG_HOZON,"request stop charge\n");
				if(PP_rmtCharge_AppointBook.validFlg == 1)
				{
					PP_can_send_data(PP_CAN_CHAGER,CAN_SETAPPOINT,0);
				}
				PP_can_send_data(PP_CAN_CHAGER,CAN_STOPCHAGER,0);
			}
			PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_RESPWAIT;
			PP_rmtChargeCtrl.state.waittime = tm_get_time();
		}
		break;
		case PP_CHARGECTRL_RESPWAIT:
		{
			if((tm_get_time() - PP_rmtChargeCtrl.state.waittime) < 2000)
			{
				if(PP_rmtChargeCtrl.state.chargecmd == PP_CHARGECTRL_OPEN)
				{
					if(PP_rmtCtrl_cfg_chargeOnOffSt() == 1) //充电开启
					{
						log_o(LOG_HOZON,"open  success\n");
						//PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLEANDOOR,0); //������ű�־λ
						PP_rmtChargeCtrl.state.chargeSt = 1;//�����
						PP_rmtChargeCtrl.fail     = 0;
						PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
					}
				}
				else
				{
					if(PP_rmtCtrl_cfg_chargeOnOffSt() == 2) //充电关闭
					{
						log_o(LOG_HOZON,"close  success\n");
						PP_rmtChargeCtrl.state.chargeSt = 2;//δ���
						//PP_can_send_data(PP_CAN_DOORLOCK,CAN_CLEANDOOR,0); //������ű�־λ
						PP_rmtChargeCtrl.fail     = 0;
						PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
					}
				}
			}
			else//��ʱ
			{
				log_e(LOG_HOZON,"Instruction execution timeout\n");
				PP_can_send_data(PP_CAN_CHAGER,CAN_STOPCHAGER,0);
				PP_rmtChargeCtrl.fail     = 1;
				PP_rmtChargeCtrl.failtype = PP_RMTCTRL_TIMEOUTFAIL;
				PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
			}
		}
		break;
		case PP_CHARGECTRL_END:
		{
			log_o(LOG_HOZON,"exit charge ctrl\n");
			PP_rmtCtrl_Stpara_t rmtCtrl_chargeStpara;
			memset(&rmtCtrl_chargeStpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if((PP_rmtChargeCtrl.state.style   == RMTCTRL_TSP) || \
					(PP_rmtChargeCtrl.state.style == RMTCTRL_HU))
			{
				rmtCtrl_chargeStpara.reqType  = PP_rmtChargeCtrl.CtrlPara.reqType;
				rmtCtrl_chargeStpara.eventid  = PP_rmtChargeCtrl.pack.DisBody.eventId;
				rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;//非预约
				if(0 == PP_rmtChargeCtrl.fail)
				{
					rmtCtrl_chargeStpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;
					rmtCtrl_chargeStpara.rvcFailureType = 0;
				}
				else
				{
					rmtCtrl_chargeStpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFAIL;
					rmtCtrl_chargeStpara.rvcFailureType = PP_rmtChargeCtrl.failtype;
				}
				PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
			}
			else if(PP_rmtChargeCtrl.state.style   == RMTCTRL_TBOX)
			{
				if(PP_rmtChargeCtrl.state.chargecmd == PP_CHARGECTRL_OPEN)
				{
					if(PP_rmtChargeCtrl.state.chargeSt == 1)
					{//按预约开启充电
						rmtCtrl_chargeStpara.rvcReqCode = 0x710;
						PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_ONGOING;
					}
					else
					{
						rmtCtrl_chargeStpara.rvcReqCode = 0x711;
						if(PP_rmtChargeCtrl.failtype == PP_RMTCTRL_CHRGGUNUNCONNT)
						{
							PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_UNCONNT;
						}
						else if(PP_rmtChargeCtrl.failtype == PP_RMTCTRL_READYLIGHTON)
						{
							PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_SPORT;
						}
						else
						{}
					}
				}
				else
				{
					if(PP_rmtChargeCtrl.state.chargeSt == 2)
					{
						if(PP_rmtChargeCtrl.state.appointchargeSt == PP_APPOINTCHARGE_SUCCESS)
						{
							rmtCtrl_chargeStpara.rvcReqCode = 0x700;
						}
						else
						{
							rmtCtrl_chargeStpara.rvcReqCode = 0x701;
						}
					}
				}

				//inform TSP
				rmtCtrl_chargeStpara.bookingId  = PP_rmtChargeCtrl.CtrlPara.bookingId;
				rmtCtrl_chargeStpara.eventid  = PP_rmtChargeCtrl.pack.DisBody.eventId;
				rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCBOOKINGRESP;//预约
				PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
			}
			PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_ChargeCtrl_chargeStMonitor

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
void PP_ChargeCtrl_chargeStMonitor(void *task)
{
	PP_rmtCtrl_Stpara_t rmtCtrl_chargeStpara;
	static uint8_t appointPerformFlg = 0;
	static uint64_t delaytime;
/*
 *	检查预约充电
 * */
	if(PP_rmtCharge_AppointBook.validFlg == 1)
	{
		char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
		time_t timep;
		struct tm *localdatetime;

		time(&timep);
		localdatetime = localtime(&timep);

		if(PP_rmtCharge_AppointBook.period & 0x80)
		{//重复预约
			if(PP_rmtCharge_Appointperiod[localdatetime->tm_wday].mask & PP_rmtCharge_AppointBook.period)
			{
				if((localdatetime->tm_hour == PP_rmtCharge_AppointBook.hour) && \
									(localdatetime->tm_min == PP_rmtCharge_AppointBook.min))
				{
					if(appointPerformFlg == 0)
					{
						log_i(LOG_HOZON,"%d-%d-%d ",(1900+localdatetime->tm_year), \
								(1 +localdatetime->tm_mon), localdatetime->tm_mday);
						log_i(LOG_HOZON,"%s %d:%d:%d\n", wday[localdatetime->tm_wday], \
								localdatetime->tm_hour, localdatetime->tm_min, localdatetime->tm_sec);
						appointPerformFlg = 1;
						PP_rmtChargeCtrl.state.appointcharge = 1;
						PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_READY;
						PP_rmtChargeCtrl.state.appointchargeTimer = tm_get_time();
						uint8_t cmd = PP_CHARGECTRL_OPEN;
						SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
						log_i(LOG_HOZON,"Appointment time is up. Execute the charge appointment!\n");
					}
					delaytime = tm_get_time();
				}
				else
				{
					if((tm_get_time() - delaytime) > 3000)//延迟3s滤波
					{
						appointPerformFlg = 0;
					}

					if(1 == PP_rmtChargeCtrl.state.appointcharge)
					{
						if((tm_get_time() - PP_rmtChargeCtrl.state.appointchargeTimer) <= PP_CHARGECTRL_APPOINTHOLDTIME)
						{
							if((PP_rmtChargeCtrl.state.appointchargeSt == PP_APPOINTCHARGE_UNCONNT)	  || \
								(PP_rmtChargeCtrl.state.appointchargeSt == PP_APPOINTCHARGE_SPORT))
							{
								if((PP_rmtCtrl_cfg_chargeGunCnctSt() == 1) && \
														(PP_rmtCtrl_cfg_readyLightSt() == 0))//充电枪连接 && 车辆非运动模式
								{
									log_i(LOG_HOZON,"Appointment time is up. Execute the charge appointment!!\n");
									PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_READY;
									uint8_t cmd = PP_CHARGECTRL_OPEN;
									SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
								}
							}
						}
						else
						{
							PP_rmtChargeCtrl.state.appointcharge = 0;
							PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_IDLE;
						}
					}
				}
			}
		}
	}

/*
 * 	检查充电完成状况״̬
 *  */
	if(PP_rmtChargeCtrl.state.appointchargeSt == PP_APPOINTCHARGE_ONGOING)
	{
		if((PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt()) || \
				(PP_RMTCTRL_CFG_CHARGEFAIL == PP_rmtCtrl_cfg_chargeSt()))
		{
			if(PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt())//charge finish
			{
				PP_rmtChargeCtrl.state.appointcharge = 0;
				PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_SUCCESS;
			}
			else
			{
				PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_ABNRSHUTDOWN;
			}

			//关闭充电
			uint8_t cmd = PP_CHARGECTRL_CLOSE;
			SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
		}
		else
		{}
	}
	else
	{
		if(PP_RMTCTRL_CFG_CHARGEING == PP_rmtCtrl_cfg_chargeSt())//��⵽�����
		{
			PP_rmtChargeCtrl.state.appointchargeSt = PP_APPOINTCHARGE_ONGOING;
		}
	}

/*
 * 检查睡眠条件
 * */
	if((PP_rmtChargeCtrl.state.appointchargeSt != PP_APPOINTCHARGE_ONGOING) && \
	   (PP_rmtChargeCtrl.state.req != 1) && (PP_rmtChargeCtrl.state.CtrlSt == PP_CHARGECTRL_IDLE) && \
			   (PP_rmtCharge_AppointBook.bookupdataflag != 2))
	{
		PP_ChargeCtrl_Sleepflag = 1;
	}
	else
	{
		PP_ChargeCtrl_Sleepflag = 0;
	}


/*
 * 检查上电/掉电
 * */
	uint8_t PowerOffSt;
	PowerOffSt = gb32960_PowerOffSt();
	if((PowerOffSt == 1) || (PP_ChargeCtrl_Sleepflag))//掉电 || 睡眠
	{
		/*
		 * 检查是否有数据更新，注意若在此前出现异常复位，会导致记录丢失
		* */
		if(PP_rmtChargeCtrl.state.dataUpdata == 1)
		{
			if(PP_rmtCharge_AppointBook.bookupdataflag == 1)
			{
				PP_rmtCharge_AppointBook.bookupdataflag = 2;
			}

			//保存记录
			log_o(LOG_HOZON,"save charge para when power off\n");
			(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,32);
			PP_rmtChargeCtrl.state.dataUpdata = 0;
		}
	}
	else if(PowerOffSt == 0)
	{
		/*
		 * 上电检查是否预约记录同步到tsp
		 * */
			if(PP_rmtCharge_AppointBook.bookupdataflag ==2)
			{
				//inform TSP the Reservation instruction issued status
				rmtCtrl_chargeStpara.rvcReqType 	= PP_rmtCharge_AppointBook.rvcReqType;
				rmtCtrl_chargeStpara.huBookingTime 	= PP_rmtCharge_AppointBook.huBookingTime;
				rmtCtrl_chargeStpara.rvcReqHours  	= PP_rmtCharge_AppointBook.hour;
				rmtCtrl_chargeStpara.rvcReqMin		= PP_rmtCharge_AppointBook.min;
				rmtCtrl_chargeStpara.rvcReqEq		= PP_rmtCharge_AppointBook.targetSOC	/* OPTIONAL */;
				rmtCtrl_chargeStpara.rvcReqCycle	= PP_rmtCharge_AppointBook.period	/* OPTIONAL */;
				rmtCtrl_chargeStpara.HUbookingId	= PP_rmtCharge_AppointBook.id;
				rmtCtrl_chargeStpara.eventid 		= PP_rmtCharge_AppointBook.eventId;
				rmtCtrl_chargeStpara.Resptype 		= PP_RMTCTRL_HUBOOKINGRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
				PP_rmtCharge_AppointBook.bookupdataflag = 1;
				PP_rmtChargeCtrl.state.dataUpdata = 1;
			}
	}
	else
	{}
}


/******************************************************
*��������SetPP_ChargeCtrl_Request

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
void SetPP_ChargeCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	uint32_t appointId = 0;
	ivi_chargeAppointSt		ivi_chargeSt;
	PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;

	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;

			log_i(LOG_HOZON, "tsp remote charge control req\n");

			PP_rmtChargeCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			if((appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_COMAND_STARTCHARGE) || \
					(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_COMAND_STOPCHARGE))
			{
				if((PP_CHARGECTRL_IDLE == PP_rmtChargeCtrl.state.CtrlSt) && \
						(PP_rmtChargeCtrl.state.req == 0))
				{
					PP_rmtChargeCtrl.state.req = 1;
					PP_rmtChargeCtrl.CtrlPara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
					if(PP_rmtChargeCtrl.CtrlPara.reqType == PP_COMAND_STARTCHARGE)
					{
						PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_OPEN;
					}
					else
					{
						PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_CLOSE;
					}
					PP_rmtChargeCtrl.state.style   = RMTCTRL_TSP;
				}
				else
				{
					log_i(LOG_HOZON, "remote charge control req is excuting\n");
				}
			}
			else if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_COMAND_APPOINTCHARGE)
			{//预约
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];
				PP_rmtCharge_AppointBook.appointType = RMTCTRL_TSP;
				PP_rmtCharge_AppointBook.id = appointId;
				PP_rmtCharge_AppointBook.hour = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[4];
				PP_rmtCharge_AppointBook.min = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[5];
				PP_rmtCharge_AppointBook.targetSOC = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[6];
				PP_rmtCharge_AppointBook.period = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[7];
				PP_rmtCharge_AppointBook.eventId = disptrBody_ptr->eventId;
				PP_rmtCharge_AppointBook.validFlg  = 1;
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d\n",PP_rmtCharge_AppointBook.id);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.eventId = %d\n",PP_rmtCharge_AppointBook.eventId);

				PP_rmtChargeCtrl.state.dataUpdata = 1;
				//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,16);

				//inform HU appointment status
				ivi_chargeSt.id = PP_rmtCharge_AppointBook.id;
				ivi_chargeSt.hour = PP_rmtCharge_AppointBook.hour;
				ivi_chargeSt.min = PP_rmtCharge_AppointBook.min;
				ivi_chargeSt.timestamp = PrvtPro_getTimestamp();
				ivi_chargeSt.targetpower = PP_rmtCharge_AppointBook.targetSOC;
				ivi_chargeSt.effectivestate = 1;
				tbox_ivi_set_tspchager_InformHU(&ivi_chargeSt);

				//inform TSP the Reservation instruction issued status
				//PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//ִ�����
				rmtCtrl_Stpara.rvcFailureType = 0;
				rmtCtrl_Stpara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				rmtCtrl_Stpara.eventid = disptrBody_ptr->eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);

				PP_can_send_data(PP_CAN_CHAGER,CAN_SETAPPOINT,0);
			}
			else if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_COMAND_CANCELAPPOINTCHARGE)
			{//取消预约
				log_i(LOG_HOZON, "TSP cancel appointment\n");
				rmtCtrl_Stpara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				rmtCtrl_Stpara.eventid = disptrBody_ptr->eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];

				PP_rmtCharge_AppointBook.appointType = RMTCTRL_TSP;
				PP_rmtCharge_AppointBook.validFlg  = 0;
				rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//ִ�����
				rmtCtrl_Stpara.rvcFailureType = 0;

				//inform HU appointment status
				ivi_chargeSt.id = PP_rmtCharge_AppointBook.id;
				ivi_chargeSt.hour = PP_rmtCharge_AppointBook.hour;
				ivi_chargeSt.min = PP_rmtCharge_AppointBook.min;
				ivi_chargeSt.timestamp = PrvtPro_getTimestamp();
				ivi_chargeSt.targetpower = PP_rmtCharge_AppointBook.targetSOC;
				ivi_chargeSt.effectivestate = 0;
				tbox_ivi_set_tspchager_InformHU(&ivi_chargeSt);
				PP_rmtChargeCtrl.state.dataUpdata = 1;

				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);

				PP_can_send_data(PP_CAN_CHAGER,CAN_CANCELAPPOINT,0);
			}
			else
			{}
		}
		break;
		case RMTCTRL_BLUETOOTH:
		{

		}
		break;
		case RMTCTRL_HU:
		{
			ivi_chargeAppointSt *ivi_chargeAppointSt_ptr = (ivi_chargeAppointSt*)appdatarmtCtrl;

			if((ivi_chargeAppointSt_ptr->cmd == PP_COMAND_STARTCHARGE) || \
					(ivi_chargeAppointSt_ptr->cmd == PP_COMAND_STOPCHARGE))
			{
				if((PP_CHARGECTRL_IDLE == PP_rmtChargeCtrl.state.CtrlSt) && \
						(PP_rmtChargeCtrl.state.req == 0))
				{
					PP_rmtChargeCtrl.state.req = 1;
					PP_rmtChargeCtrl.pack.DisBody.eventId = 0;
					if(ivi_chargeAppointSt_ptr->cmd == PP_COMAND_STARTCHARGE)
					{
						log_i(LOG_HOZON, "HU charge open request\n");
						PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_OPEN;
						PP_rmtChargeCtrl.CtrlPara.reqType = PP_COMAND_STARTCHARGE;
					}
					else
					{
						log_i(LOG_HOZON, "HU charge close request\n");
						PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_CLOSE;
						PP_rmtChargeCtrl.CtrlPara.reqType = PP_COMAND_STOPCHARGE;
					}
					PP_rmtChargeCtrl.state.style   = RMTCTRL_HU;
				}
				else
				{
					log_i(LOG_HOZON, "remote charge control req is excuting\n");
				}
			}
			else if(ivi_chargeAppointSt_ptr->cmd == PP_COMAND_APPOINTCHARGE)
			{//预约充电
				PP_rmtCharge_AppointBook.appointType = RMTCTRL_HU;
				PP_rmtCharge_AppointBook.rvcReqType = PP_COMAND_APPOINTCHARGE;
				PP_rmtCharge_AppointBook.id = ivi_chargeAppointSt_ptr->id;
				PP_rmtCharge_AppointBook.hour = ivi_chargeAppointSt_ptr->hour;
				PP_rmtCharge_AppointBook.min = ivi_chargeAppointSt_ptr->min;
				PP_rmtCharge_AppointBook.targetSOC = ivi_chargeAppointSt_ptr->targetpower;
				PP_rmtCharge_AppointBook.period = 0xff;
				PP_rmtCharge_AppointBook.huBookingTime = ivi_chargeAppointSt_ptr->timestamp;

				PP_rmtCharge_AppointBook.eventId = 0;
				PP_rmtCharge_AppointBook.validFlg  = 1;
				PP_rmtCharge_AppointBook.bookupdataflag = 1;
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d\n",PP_rmtCharge_AppointBook.id);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.eventId = %d\n",PP_rmtCharge_AppointBook.eventId);

				//保存记录
				PP_rmtChargeCtrl.state.dataUpdata = 1;
				//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,16);

				//inform TSP the Reservation instruction issued status
				rmtCtrl_Stpara.rvcReqType 		= PP_rmtCharge_AppointBook.rvcReqType;
				rmtCtrl_Stpara.huBookingTime 	= PP_rmtCharge_AppointBook.huBookingTime;
				rmtCtrl_Stpara.rvcReqHours  	= PP_rmtCharge_AppointBook.hour;
				rmtCtrl_Stpara.rvcReqMin		= PP_rmtCharge_AppointBook.min;
				rmtCtrl_Stpara.rvcReqEq			= PP_rmtCharge_AppointBook.targetSOC	/* OPTIONAL */;
				rmtCtrl_Stpara.rvcReqCycle		= PP_rmtCharge_AppointBook.period	/* OPTIONAL */;
				rmtCtrl_Stpara.HUbookingId		= PP_rmtCharge_AppointBook.id;
				rmtCtrl_Stpara.eventid 			= PP_rmtCharge_AppointBook.eventId;
				rmtCtrl_Stpara.Resptype 		= PP_RMTCTRL_HUBOOKINGRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);

				PP_can_send_data(PP_CAN_CHAGER,CAN_SETAPPOINT,0);
			}
			else if(ivi_chargeAppointSt_ptr->cmd == PP_COMAND_CANCELAPPOINTCHARGE)
			{//取消预约充电
				log_i(LOG_HOZON, "HU cancel appointment\n");

				PP_rmtCharge_AppointBook.appointType = RMTCTRL_HU;
				PP_rmtCharge_AppointBook.validFlg = 0;

				PP_rmtCharge_AppointBook.rvcReqType = PP_COMAND_CANCELAPPOINTCHARGE;
				PP_rmtCharge_AppointBook.id = ivi_chargeAppointSt_ptr->id;
				PP_rmtCharge_AppointBook.hour = ivi_chargeAppointSt_ptr->hour;
				PP_rmtCharge_AppointBook.min = ivi_chargeAppointSt_ptr->min;
				PP_rmtCharge_AppointBook.targetSOC = ivi_chargeAppointSt_ptr->targetpower;
				PP_rmtCharge_AppointBook.period = 0xff;
				PP_rmtCharge_AppointBook.huBookingTime = ivi_chargeAppointSt_ptr->timestamp;

				PP_rmtCharge_AppointBook.bookupdataflag = 1;

				//保存记录
				PP_rmtChargeCtrl.state.dataUpdata = 1;
				//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,16);

				//inform TSP the Reservation instruction issued status
				rmtCtrl_Stpara.rvcReqType 		= PP_COMAND_CANCELAPPOINTCHARGE;
				rmtCtrl_Stpara.huBookingTime 	= PP_rmtCharge_AppointBook.huBookingTime;
				rmtCtrl_Stpara.rvcReqHours  	= PP_rmtCharge_AppointBook.hour;
				rmtCtrl_Stpara.rvcReqMin		= PP_rmtCharge_AppointBook.min;
				rmtCtrl_Stpara.rvcReqEq			= PP_rmtCharge_AppointBook.targetSOC	/* OPTIONAL */;
				rmtCtrl_Stpara.rvcReqCycle		= PP_rmtCharge_AppointBook.period	/* OPTIONAL */;
				rmtCtrl_Stpara.HUbookingId		= PP_rmtCharge_AppointBook.id;
				rmtCtrl_Stpara.eventid 			= PP_rmtCharge_AppointBook.eventId;
				rmtCtrl_Stpara.Resptype 		= PP_RMTCTRL_HUBOOKINGRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);

				PP_can_send_data(PP_CAN_CHAGER,CAN_CANCELAPPOINT,0);
			}
			else
			{}
		}
		break;
		case RMTCTRL_TBOX:
		{
			uint8_t *cmd = (uint8_t*)appdatarmtCtrl;
			PP_rmtChargeCtrl.state.req = 1;
			PP_rmtChargeCtrl.CtrlPara.bookingId = PP_rmtCharge_AppointBook.id;
			PP_rmtChargeCtrl.pack.DisBody.eventId = PP_rmtCharge_AppointBook.eventId;
			PP_rmtChargeCtrl.state.chargecmd = *cmd;
			PP_rmtChargeCtrl.state.style   = RMTCTRL_TBOX;
		}
		break;
		default:
		break;
	}
}

/******************************************************
*��������PP_ChargeCtrl_SetCtrlReq

*��  �Σ�

*����ֵ��

*��  �������� ����

*��  ע��
******************************************************/
void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtChargeCtrl.CtrlPara.reqType = (long)reqType;
	PP_rmtChargeCtrl.state.req = 1;
}

void PP_ChargeCtrl_send_cb(void)
{
	log_i(LOG_HOZON, "HU appointment status inform to tsp success!\n");
	PP_rmtCharge_AppointBook.bookupdataflag = 3;
	PP_rmtChargeCtrl.state.dataUpdata = 1;
}

int PP_ChargeCtrl_start(void)
{
	if(PP_rmtChargeCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int PP_ChargeCtrl_end(void)
{
	if((PP_rmtChargeCtrl.state.req == 0) && \
			(PP_rmtChargeCtrl.state.CtrlSt == PP_CHARGECTRL_IDLE))
	{
		return 1;

	}
	else
	{
		return 0;

	}
}

void PP_ChargeCtrl_ClearStatus(void)
{
	PP_rmtChargeCtrl.state.req = 0;
}

#if 0
void SetPP_ChargeCtrl_Awaken(void)
{
	PP_ChargeCtrl_Sleepflag = 0;
}
#endif
unsigned char GetPP_ChargeCtrl_Sleep(void)
{
	return PP_ChargeCtrl_Sleepflag;
}

/*
 * 定时充电状态
 */
unsigned char GetPP_ChargeCtrl_appointSt(void)
{
	return PP_rmtCharge_AppointBook.validFlg;
}

/*
 * 定时充电小时
 */
unsigned char GetPP_ChargeCtrl_appointHour(void)
{
	return PP_rmtCharge_AppointBook.hour;
}

/*
 * 定时充电分钟
 */
unsigned char GetPP_ChargeCtrl_appointMin(void)
{
	return PP_rmtCharge_AppointBook.min;
}
