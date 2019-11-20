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
#include "ble.h"
#include "dev_api.h"
#include "log.h"
#include "list.h"
#include "cfg_api.h"
#include "../../support/protocol.h"
#include "PPrmtCtrl_cfg.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "tbox_ivi_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "PP_canSend.h"
#include "PP_SendWakeUptime.h"
#include "PP_ChargeCtrl.h"

extern ivi_client ivi_clients[MAX_IVI_NUM];
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
	uint8_t chargeOnOffFlag;
	uint8_t fail;//执行失败状态：1-失败
	uint8_t failtype;//失败类型
	uint8_t appointCharging;
	uint8_t chargeFinishCloseFlag;//关闭充电类型
}__attribute__((packed))  PrvtProt_rmtChargeCtrl_t; /*�ṹ��*/

static PrvtProt_rmtChargeCtrl_t PP_rmtChargeCtrl;
//static PrvtProt_pack_t 			PP_rmtChargeCtrl_Pack;
//static PrvtProt_App_rmtCtrl_t 	App_rmtChargeCtrl;
PP_rmtCharge_AppointBook_t		PP_rmtCharge_AppointBook;

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
extern void pm_ring_wakeup(void);
extern void ivi_chagerappointment_request_send( int fd,ivi_chargeAppointSt tspchager);
static void PP_ChargeCtrl_chargeStMonitor(void);
static int PP_ChargeCtrl_startHandle(PrvtProt_rmtChargeCtrl_t* pp_rmtCharge);
static void PP_ChargeCtrl_EndHandle(PrvtProt_rmtChargeCtrl_t* pp_rmtCharge);

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
	PP_rmtChargeCtrl.state.expTime = -1;

	//读取记录
	len = 32;
	res = cfg_get_user_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,&len);
	if((res==0) && (PP_rmtCharge_AppointBook.validFlg == 1))
	{
		log_e(LOG_HOZON,"There are currently reservation records\n");
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %u\n",PP_rmtCharge_AppointBook.id);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
		log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
		PP_can_send_data(PP_CAN_CHAGER,CAN_SETAPPOINT,0);//有效充电预约，将预约充电使能位置起
		//log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.appointChargeFlag = %d\n",PP_rmtCharge_AppointBook.appointChargeFlag);
		//log_e(LOG_HOZON, "PP_rmtCharge_AppointBook.appointStartTime = %d\n",PP_rmtCharge_AppointBook.appointStartTime);
		PP_rmtChargeCtrl.state.bookSyncflag = 1;
	}

	PP_rmtChargeCtrl.state.appointchargeCheckDelayTime = tm_get_time();
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

	PP_ChargeCtrl_chargeStMonitor();//监测充电状态

	switch(PP_rmtChargeCtrl.state.CtrlSt)
	{
		case PP_CHARGECTRL_IDLE:
		{
			if(PP_rmtChargeCtrl.state.req == 1)
			{
				PP_rmtChargeCtrl.fail      = 0;
				PP_rmtChargeCtrl.failtype  = PP_RMTCTRL_NORMAL;
				PP_rmtChargeCtrl.chargeOnOffFlag = 0;

				pm_ring_wakeup();   //ring脚唤醒MCU
				PP_can_mcu_awaken();//唤醒

				if(0 == PP_ChargeCtrl_startHandle(&PP_rmtChargeCtrl))
				{
					PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_REQSTART;
				}
				else
				{
					PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_END;
				}
				
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
						log_o(LOG_HOZON,"start charge success\n");
						PP_can_send_data(PP_CAN_CHAGER,CAN_CLEANCHARGE,0); //
						PP_rmtChargeCtrl.chargeOnOffFlag = 1;//
						PP_rmtChargeCtrl.fail     = 0;
						PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_ONGOING;
						PP_rmtChargeCtrl.chargeFinishCloseFlag = 0;
						PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
					}
				}
				else
				{
					if(PP_rmtCtrl_cfg_chargeOnOffSt() == 0) //充电关闭
					{
						log_o(LOG_HOZON,"close charge success\n");
						PP_rmtChargeCtrl.chargeOnOffFlag = 2;//
						PP_rmtChargeCtrl.appointCharging = 0;
						PP_can_send_data(PP_CAN_CHAGER,CAN_CLEANCHARGE,0); //
						PP_rmtChargeCtrl.fail     = 0;
						PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_IDLE;
						PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
					}
				}
			}
			else//��ʱ
			{
				log_e(LOG_HOZON,"Instruction execution timeout\n");
				PP_rmtChargeCtrl.appointCharging = 0;
				PP_can_send_data(PP_CAN_CHAGER,CAN_CLEANCHARGE,0);
				PP_rmtChargeCtrl.fail     = 1;
				PP_rmtChargeCtrl.failtype = PP_RMTCTRL_TIMEOUTFAIL;
				PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_IDLE;
				PP_rmtChargeCtrl.state.CtrlSt = PP_CHARGECTRL_END;
			}
		}
		break;
		case PP_CHARGECTRL_END:
		{
			log_o(LOG_HOZON,"charge control end\n");
			PP_ChargeCtrl_EndHandle(&PP_rmtChargeCtrl);
			PP_can_mcu_sleep();//清除虚拟on线
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
static void PP_ChargeCtrl_chargeStMonitor(void)
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

		RTCTIME localdatetime;
    	tm_get_abstime(&localdatetime);
		
		if(PP_rmtCharge_AppointBook.period & 0x80)
		{//重复预约
			if(PP_rmtCharge_Appointperiod[localdatetime.week].mask & PP_rmtCharge_AppointBook.period)
			{
				if((localdatetime.hour == PP_rmtCharge_AppointBook.hour) && \
									(localdatetime.min == PP_rmtCharge_AppointBook.min))
				{
					if((PP_rmtChargeCtrl.state.chargeSt != PP_CHARGESTATE_ONGOING) && \
						(appointPerformFlg == 0))
					{
						log_i(LOG_HOZON,"%d-%d-%d ",(localdatetime.year - 2000), \
								(localdatetime.mon), localdatetime.mday);
						log_i(LOG_HOZON,"%s %d:%d:%d\n", wday[localdatetime.week], \
								localdatetime.hour, localdatetime.min, localdatetime.sec);
						appointPerformFlg = 1;
						PP_rmtChargeCtrl.state.appointcharge = 1;
						PP_rmtChargeCtrl.state.appointchargeTime = PrvtPro_getTimestamp();
						PP_rmtChargeCtrl.state.appointchargeCheckDelayTime = tm_get_time();
						PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_READY;
						uint8_t cmd = PP_CHARGECTRL_OPEN;
						PP_can_send_data(PP_CAN_CHAGER,CAN_CANCELAPPOINT,0);//预约充电使能去掉
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
				}
			}
		}
	}

/* 
 * 	预约充电触发后，若执行条件不满足导致执行失败，则持续检测6h。
 *  6h内检测到执行预约充电条件满足，则执行预约充电；
 *  持续检测时间超出6h，则不再执行。
 *  */
	long currTimestamp;
	if((tm_get_time() - PP_rmtChargeCtrl.state.appointchargeCheckDelayTime) > 5000)
	{
		if((1 == PP_rmtChargeCtrl.state.appointcharge) && \
			(PP_RMTCTRL_CFG_CHARGEING != PP_rmtCtrl_cfg_chargeSt()))
		{
			currTimestamp = PrvtPro_getTimestamp();
			if(currTimestamp <= (PP_rmtChargeCtrl.state.appointchargeTime + PP_CHARGECTRL_APPOINTHOLDTIME))
			{
				if((PP_rmtChargeCtrl.failtype == PP_RMTCTRL_CHRGGUNUNCONNT)	  || \
					(PP_rmtChargeCtrl.failtype == PP_RMTCTRL_READYLIGHTON)	  || \
					(PP_rmtChargeCtrl.failtype == PP_RMTCTRL_FOTA_UPGRADE))
				{
					if((PP_rmtCtrl_cfg_chargeGunCnctSt() == 1) && \
						(PP_rmtCtrl_cfg_readyLightSt() == 0)   && \
						(0 == GetPP_rmtCtrl_fotaUpgrade()))//充电枪连接 && 车辆非运动模式 && 非fota升级中
					{
						log_i(LOG_HOZON,"Appointment time is up. Execute the charge appointment!!\n");
						PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_READY;
						uint8_t cmd = PP_CHARGECTRL_OPEN;
						SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
					}
				}
			}
			else
			{
				PP_can_send_data(PP_CAN_CHAGER,CAN_SETAPPOINT,0);//预约充电使能置起
				PP_rmtChargeCtrl.state.appointcharge = 0;
			}
		}
	}

/*
 * 	检查充电完成状况״̬
 *  */
	if(PP_rmtChargeCtrl.state.chargeSt == PP_CHARGESTATE_ONGOING)
	{
		PP_rmtChargeCtrl.state.appointcharge = 0;
		
		if(PP_rmtChargeCtrl.appointCharging == 1)//预约充电中
		{
			if(gb_data_vehicleSOC() >= PP_rmtCharge_AppointBook.targetSOC)//预约充电完成判断
			{
				log_i(LOG_HOZON,"The appointment charge has reached the reserved target battery\n");
				PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_FINISH;
				PP_rmtChargeCtrl.chargeFinishCloseFlag = 1;
				uint8_t cmd = PP_CHARGECTRL_CLOSE;
				SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
			}
			else if(PP_RMTCTRL_CFG_CHARGEFAIL == PP_rmtCtrl_cfg_chargeSt())
			{
				log_i(LOG_HOZON,"The appointment charge fail,close charge\n");
				PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_ABNRSHUTDOWN;
				PP_rmtChargeCtrl.chargeFinishCloseFlag = 0;
				uint8_t cmd = PP_CHARGECTRL_CLOSE;
				SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
			}
			else
			{}
		}
		else
		{
			if((PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt()) || \
					(PP_RMTCTRL_CFG_CHARGEFAIL == PP_rmtCtrl_cfg_chargeSt()))
			{
				if(PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt())//charge finish
				{
					PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_FINISH;
				}
				else
				{
					PP_rmtChargeCtrl.state.chargeSt = PP_CHARGESTATE_ABNRSHUTDOWN;
				}

				//关闭充电
				uint8_t cmd = PP_CHARGECTRL_CLOSE;
				SetPP_ChargeCtrl_Request(RMTCTRL_TBOX,&cmd,NULL);
			}
		}
	}

/*
 * 检查睡眠条件
 * */
	if((PP_rmtChargeCtrl.state.chargeSt != PP_CHARGESTATE_ONGOING) && \
	   (PP_rmtChargeCtrl.state.req != 1) && (PP_rmtChargeCtrl.state.CtrlSt == PP_CHARGECTRL_IDLE))
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
	if(dev_get_KL15_signal())
	{
		/*
		 * IGN ON上电检查是否预约记录同步到tsp
		 * */
		if((1 == sockproxy_socketState()) && (PP_rmtChargeCtrl.state.bookSyncflag == 1))
		{
			if((PP_rmtCharge_AppointBook.appointType == RMTCTRL_HU)&&(PP_rmtCharge_AppointBook.informtspflag == 1))
			{
				log_i(LOG_HOZON,"Synchronize reservation records to TSP\n");
				rmtCtrl_chargeStpara.rvcReqType 	= PP_rmtCharge_AppointBook.rvcReqType;
				rmtCtrl_chargeStpara.huBookingTime 	= PP_rmtCharge_AppointBook.huBookingTime;
				rmtCtrl_chargeStpara.rvcReqHours  	= PP_rmtCharge_AppointBook.hour;
				rmtCtrl_chargeStpara.rvcReqMin		= PP_rmtCharge_AppointBook.min;
				rmtCtrl_chargeStpara.rvcReqEq		= PP_rmtCharge_AppointBook.targetSOC	/* OPTIONAL */;
				rmtCtrl_chargeStpara.rvcReqCycle	= PP_rmtCharge_AppointBook.period	/* OPTIONAL */;
				rmtCtrl_chargeStpara.HUbookingId	= PP_rmtCharge_AppointBook.id;
				rmtCtrl_chargeStpara.eventid 		= 0;
				rmtCtrl_chargeStpara.Resptype 		= PP_RMTCTRL_HUBOOKINGRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
				//PP_rmtCharge_AppointBook.informtspflag = 0;
				PP_rmtChargeCtrl.state.bookSyncflag = 2;
			}
		}

		/*
		 * 检查是否有数据更新
		 * */
		if(PP_rmtChargeCtrl.state.dataUpdata == 1)
		{
			//保存记录
			log_i(LOG_HOZON,"save charge para when power off\n");
			(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,32);
			PP_rmtChargeCtrl.state.dataUpdata = 0;
		}
	}
	else
	{
		PP_rmtChargeCtrl.state.bookSyncflag = 1;
	}
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
			PP_rmtChargeCtrl.state.expTime =  disptrBody_ptr->expTime;
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
					log_e(LOG_HOZON, "remote charge control req is excuting\n");
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
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %u\n",PP_rmtCharge_AppointBook.id);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.eventId = %d\n",PP_rmtCharge_AppointBook.eventId);

				PP_rmtChargeCtrl.state.dataUpdata = 1;

				//inform HU appointment status
				ivi_chargeSt.id = PP_rmtCharge_AppointBook.id;
				ivi_chargeSt.hour = PP_rmtCharge_AppointBook.hour;
				ivi_chargeSt.min = PP_rmtCharge_AppointBook.min;
				ivi_chargeSt.timestamp = PrvtPro_getTimestamp();
				ivi_chargeSt.targetpower = PP_rmtCharge_AppointBook.targetSOC;
				ivi_chargeSt.effectivestate = 1;
				ivi_chagerappointment_request_send(ivi_clients[0].fd,ivi_chargeSt);

				//inform TSP the Reservation instruction issued status
				//PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				rmtCtrl_Stpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;//ִ�����
				rmtCtrl_Stpara.rvcFailureType = 0;
				rmtCtrl_Stpara.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
				rmtCtrl_Stpara.eventid = disptrBody_ptr->eventId;
				rmtCtrl_Stpara.expTime = disptrBody_ptr->expTime;
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
				rmtCtrl_Stpara.expTime = disptrBody_ptr->expTime;
				//inform HU appointment status
				ivi_chargeSt.id = PP_rmtCharge_AppointBook.id;
				ivi_chargeSt.hour = PP_rmtCharge_AppointBook.hour;
				ivi_chargeSt.min = PP_rmtCharge_AppointBook.min;
				ivi_chargeSt.timestamp = PrvtPro_getTimestamp();
				ivi_chargeSt.targetpower = PP_rmtCharge_AppointBook.targetSOC;
				ivi_chargeSt.effectivestate = 0;
				ivi_chagerappointment_request_send(ivi_clients[0].fd,ivi_chargeSt);
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
			 unsigned char cmd = *(unsigned char *)appdatarmtCtrl;
			 if(cmd == 1 )//停止充电
			 {
			 	PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_CLOSE;
			 }
			 else if (cmd == 2) //开始充电
			 {
			 	PP_rmtChargeCtrl.state.chargecmd = PP_CHARGECTRL_OPEN;
			 }
			 else
			 {
			 }
			 PP_rmtChargeCtrl.state.req = 1;
			 PP_rmtChargeCtrl.state.style = RMTCTRL_BLUETOOTH;
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
					PP_rmtChargeCtrl.pack.DisBody.eventId = 0;//HU立即充电eventid = 0
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
					log_e(LOG_HOZON, "remote charge control req is excuting\n");
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
				if(ivi_chargeAppointSt_ptr->effectivestate == 1)
				{
					PP_rmtCharge_AppointBook.period = 0xff;
				}
				else
				{
					PP_rmtCharge_AppointBook.period = 0;
				}
				PP_rmtCharge_AppointBook.huBookingTime = ivi_chargeAppointSt_ptr->timestamp;
				PP_rmtCharge_AppointBook.eventId = 0;
				PP_rmtCharge_AppointBook.validFlg  = 1;
				PP_rmtCharge_AppointBook.informtspflag = 1; //需要通知TSP
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d\n",PP_rmtCharge_AppointBook.id);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.eventId = %d\n",PP_rmtCharge_AppointBook.eventId);

				//保存记录
				PP_rmtChargeCtrl.state.dataUpdata = 1;

				PP_rmtChargeCtrl.state.bookSyncflag = 1; //保存

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
				PP_rmtCharge_AppointBook.informtspflag = 1; //需要通知TSP
				//保存记录
				PP_rmtChargeCtrl.state.dataUpdata = 1;

				PP_rmtChargeCtrl.state.bookSyncflag = 1;
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
			//PP_rmtChargeCtrl.pack.DisBody.eventId = PP_rmtCharge_AppointBook.eventId;
			PP_rmtChargeCtrl.pack.DisBody.eventId = 0;
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
	if(PP_rmtCharge_AppointBook.hour > 23)
	{
		return 23;
	}

	return PP_rmtCharge_AppointBook.hour;
}

/*
 * 定时充电分钟
 */
unsigned char GetPP_ChargeCtrl_appointMin(void)
{
	if(PP_rmtCharge_AppointBook.min > 59)
	{
		return 59;
	}

	return PP_rmtCharge_AppointBook.min;
}


/*
 * 保存数据
 */
void SetPP_ChargeCtrl_appointPara(void)
{
	if(PP_rmtChargeCtrl.state.dataUpdata == 1)
	{
		//保存记录
		log_o(LOG_HOZON,"save charge para\n");
		(void)cfg_set_user_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,32);
		PP_rmtChargeCtrl.state.dataUpdata = 0;
	}
	else
	{
		log_o(LOG_HOZON,"no charge para need to save\n");
	}
}

/*
 * 充电控制start处理
 */
static int PP_ChargeCtrl_startHandle(PrvtProt_rmtChargeCtrl_t* pp_rmtCharge)
{
	int res = 0;

	if(pp_rmtCharge->state.chargecmd == PP_CHARGECTRL_OPEN)//请求充电开启
	{
		if((PP_rmtCtrl_cfg_chargeGunCnctSt() == 1) && \
				(PP_rmtCtrl_cfg_readyLightSt() == 0))//充电枪连接 && 车辆非运动模式״̬
		{
			log_o(LOG_HOZON,"start charge ctrl\n");
			if((pp_rmtCharge->state.style == RMTCTRL_TSP) || \
					(pp_rmtCharge->state.style == RMTCTRL_HU))
			{
				log_o(LOG_HOZON,"TSP or HU platform\n");
				PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
				rmtCtrl_Stpara.rvcReqStatus = 1;
				rmtCtrl_Stpara.rvcFailureType = 0;
				rmtCtrl_Stpara.expTime = pp_rmtCharge->state.expTime;
				rmtCtrl_Stpara.reqType = pp_rmtCharge->CtrlPara.reqType;
				rmtCtrl_Stpara.eventid = pp_rmtCharge->pack.DisBody.eventId;
				rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
				PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
			}
			else if(pp_rmtCharge->state.style == RMTCTRL_TBOX)//
			{
				log_o(LOG_HOZON,"tbox platform\n");
			}
			else
			{
				log_o(LOG_HOZON,"bluetooth platform\n");
			}
		}
		else
		{
			log_i(LOG_HOZON,"The vehicle control condition is not satisfied\n");
			pp_rmtCharge->fail     = 1;
			if(PP_rmtCtrl_cfg_chargeGunCnctSt() == 1)
			{
				pp_rmtCharge->failtype = PP_RMTCTRL_CHRGGUNUNCONNT;
			}
			else
			{
				pp_rmtCharge->failtype = PP_RMTCTRL_READYLIGHTON;
			}
			res = -1;
		}
	}
	else//请求充电关闭
	{
		log_o(LOG_HOZON,"start charge ctrl\n");
		if((pp_rmtCharge->state.style == RMTCTRL_TSP) || \
				(pp_rmtCharge->state.style == RMTCTRL_HU))
		{
			log_o(LOG_HOZON,"TSP or HU platform\n");
			PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
			rmtCtrl_Stpara.rvcReqStatus = 1;//��ʼִ��
			rmtCtrl_Stpara.rvcFailureType = 0;
			rmtCtrl_Stpara.reqType = pp_rmtCharge->CtrlPara.reqType;
			rmtCtrl_Stpara.expTime = pp_rmtCharge->state.expTime;
			rmtCtrl_Stpara.eventid = pp_rmtCharge->pack.DisBody.eventId;
			rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
			PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
		}
		else if(pp_rmtCharge->state.style == RMTCTRL_TBOX)//
		{
			log_o(LOG_HOZON,"tbox platform\n");
		}
		else
		{
			log_o(LOG_HOZON,"bluetooth platform\n");
		}
	}

	return res;	
}


/*
 * 充电控制end处理
 */
static void PP_ChargeCtrl_EndHandle(PrvtProt_rmtChargeCtrl_t* pp_rmtCharge)
{
	PP_rmtCtrl_Stpara_t rmtCtrl_chargeStpara;
	memset(&rmtCtrl_chargeStpara,0,sizeof(PP_rmtCtrl_Stpara_t));

	switch(pp_rmtCharge->state.style)
	{
		case RMTCTRL_TSP:
		case RMTCTRL_HU:
		{
			rmtCtrl_chargeStpara.reqType  = pp_rmtCharge->CtrlPara.reqType;
			rmtCtrl_chargeStpara.eventid  = pp_rmtCharge->pack.DisBody.eventId;
			rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;//非预约
			if(0 == pp_rmtCharge->fail)
			{
				rmtCtrl_chargeStpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFINISH;
				rmtCtrl_chargeStpara.rvcFailureType = 0;
			}
			else
			{
				rmtCtrl_chargeStpara.rvcReqStatus = PP_RMTCTRL_EXECUTEDFAIL;
				rmtCtrl_chargeStpara.rvcFailureType = pp_rmtCharge->failtype;
			}
			PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
		}
		break;
		case RMTCTRL_TBOX:
		{
			if(pp_rmtCharge->state.chargecmd == PP_CHARGECTRL_OPEN)
			{
				if(pp_rmtCharge->chargeOnOffFlag == 1)
				{//按预约开启充电
					rmtCtrl_chargeStpara.rvcReqCode = 0x710;
					pp_rmtCharge->appointCharging = 1;
				}
				else
				{
					rmtCtrl_chargeStpara.rvcReqCode = 0x711;
				}
			}
			else
			{
				if(pp_rmtCharge->chargeOnOffFlag == 2)
				{
					if(PP_rmtChargeCtrl.chargeFinishCloseFlag == 1)
					{
						rmtCtrl_chargeStpara.rvcReqCode = 0x700;
					}
					else
					{
						rmtCtrl_chargeStpara.rvcReqCode = 0x701;
					}
				}
			}

			PP_rmtChargeCtrl.chargeFinishCloseFlag = 0;
			//inform TSP
			rmtCtrl_chargeStpara.bookingId  = pp_rmtCharge->CtrlPara.bookingId;
			rmtCtrl_chargeStpara.eventid  = pp_rmtCharge->pack.DisBody.eventId;
			rmtCtrl_chargeStpara.expTime = pp_rmtCharge->state.expTime;
			rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCBOOKINGRESP;//预约
			PP_rmtCtrl_StInformTsp(&rmtCtrl_chargeStpara);
		}
		break;
		case RMTCTRL_BLUETOOTH:
		{
			TCOM_MSG_HEADER msghdr;
			PrvtProt_respbt_t respbt;
			respbt.msg_type = BT_CHARGE_RESP;
			respbt.cmd = pp_rmtCharge->state.chargecmd;
			if(0 == pp_rmtCharge->fail)
			{
				if(pp_rmtCharge->state.chargecmd == PP_CHARGECTRL_OPEN)
				{
					respbt.cmd_state.execution_result = 2;  //ִ执行成功
				}
				if(pp_rmtCharge->state.chargecmd == PP_CHARGECTRL_CLOSE)
				{
					respbt.cmd_state.execution_result = 1;  //ִ执行成功
				}
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
			tcom_send_msg(&msghdr, &respbt);			
		}
		break;
		default:
		break;
	}
}

/*
*	预约同步响应回调
*/
void PP_ChargeCtrl_HUBookingBackResp(void* HUbookingBackResp)
{
	App_rmtCtrlHUbookingBackResp_t* HUbookingBackResp_ptr = (App_rmtCtrlHUbookingBackResp_t*)HUbookingBackResp;
	log_i(LOG_HOZON, "HUbookingBackResp.rvcReqType = %d",HUbookingBackResp_ptr->rvcReqType);
	log_i(LOG_HOZON, "HUbookingBackResp.rvcResult = %d",HUbookingBackResp_ptr->rvcResult);
	log_i(LOG_HOZON, "HUbookingBackResp.bookingId = %d",HUbookingBackResp_ptr->bookingId);
	PP_rmtCharge_AppointBook.informtspflag = 0;
}

/*
*	显示充电参数
*/
void PP_ChargeCtrl_show(void)
{
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.appointType = %d",PP_rmtCharge_AppointBook.appointType);	
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.validFlg = %d",PP_rmtCharge_AppointBook.validFlg);	
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d",PP_rmtCharge_AppointBook.id);
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d",PP_rmtCharge_AppointBook.hour);
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d",PP_rmtCharge_AppointBook.min);
	log_o(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
}

int PP_ChargeCtrl_waketime(void)
{
	waketime ch_time;
	int low_time = 0;   
	int temp_time = 0;
	if(PP_rmtCharge_AppointBook.validFlg == 1)
	{
		ch_time.hour = PP_rmtCharge_AppointBook.hour;
		ch_time.min = PP_rmtCharge_AppointBook.min;
		ch_time.period = PP_rmtCharge_AppointBook.period;
		temp_time = PP_waketime_to_min(&ch_time);
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
	
	return low_time;
}