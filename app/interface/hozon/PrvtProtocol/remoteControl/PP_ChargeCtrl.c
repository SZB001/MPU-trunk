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

#include "../sockproxy/sockproxy_data.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
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
	PP_rmtChargeCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtChargeCtrl_t; /*�ṹ��*/

static PrvtProt_rmtChargeCtrl_t PP_rmtChargeCtrl;
static PrvtProt_pack_t 		PP_rmtChargeCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 		App_rmtChargeCtrl;
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_ChargeCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtChargeCtrl_t *rmtCtrl);
static int PP_ChargeCtrl_BookingStatusResp(PrvtProt_task_t *task,PrvtProt_rmtChargeCtrl_t *rmtACCtrl);
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
	int res;
	static uint32_t stitch = 0;
	static char result = 0;
	if(PP_rmtChargeCtrl.state.req == 1)
	{
		stitch++;
		PP_rmtChargeCtrl.state.req = 0;
		if(PP_rmtChargeCtrl.state.bookingSt == 1)
		{
			PP_rmtChargeCtrl.state.executSt = 0;
		}
		else
		{
			PP_rmtChargeCtrl.state.executSt = 1;
		}

		res = PP_ChargeCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtChargeCtrl);
		if(res < 0)//������ʧ��
		{
			log_e(LOG_HOZON, "socket send error, reset protocol");
			sockproxy_socketclose();//by liujian 20190514
		}
		else if(res > 0)
		{
			log_e(LOG_HOZON, "socket send ok, reset");
			result = 1;
			PP_rmtChargeCtrl.state.period = tm_get_time();
		}
		else
		{}
	}

	if(1 == result)
	{
		if((tm_get_time() - PP_rmtChargeCtrl.state.period) > 3000)
		{
			result = 0;
			if(PP_rmtChargeCtrl.state.bookingSt == 1)
			{
				(void)PP_ChargeCtrl_BookingStatusResp((PrvtProt_task_t *)task,&PP_rmtChargeCtrl);
			}
			else
			{
				if(stitch%2)//����ִ�гɹ�
				{
					PP_rmtChargeCtrl.state.executSt = 2;
					(void)PP_ChargeCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtChargeCtrl);
				}
				else//����ִ��ʧ��
				{
					PP_rmtChargeCtrl.state.executSt = 3;
					(void)PP_ChargeCtrl_StatusResp((PrvtProt_task_t *)task,&PP_rmtChargeCtrl);
				}
			}
		}
	}


	return res;
}


/******************************************************
*��������PP_ChargeCtrl_StatusResp

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static int PP_ChargeCtrl_StatusResp(PrvtProt_task_t *task,PrvtProt_rmtChargeCtrl_t *rmtACCtrl)
{
	int msgdatalen;
	int res;
	/*header*/
	rmtACCtrl->pack.Header.ver.Byte = task->version;
	rmtACCtrl->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtACCtrl->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtChargeCtrl_Pack, &rmtACCtrl->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtACCtrl->pack.DisBody.mID = PP_MID_RMTCTRL_RESP;
	//rmtdoorCtrl->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtACCtrl->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtACCtrl->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	/*appdata*/
	PrvtProtcfg_gpsData_t gpsDt;
	App_rmtChargeCtrl.CtrlResp.rvcReqType = PP_rmtChargeCtrl.state.reqType;
	App_rmtChargeCtrl.CtrlResp.rvcReqStatus = PP_rmtChargeCtrl.state.executSt;
	App_rmtChargeCtrl.CtrlResp.rvcFailureType = 0;
	App_rmtChargeCtrl.CtrlResp.gpsPos.gpsSt = PrvtProtCfg_gpsStatus();//gps״̬ 0-��Ч��1-��Ч;
	App_rmtChargeCtrl.CtrlResp.gpsPos.gpsTimestamp = PrvtPro_getTimestamp();//gpsʱ���:ϵͳʱ��(ͨ��gpsУʱ)

	PrvtProtCfg_gpsData(&gpsDt);

	if(App_rmtChargeCtrl.CtrlResp.gpsPos.gpsSt == 1)
	{
		if(gpsDt.is_north)
		{
			App_rmtChargeCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000);//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			App_rmtChargeCtrl.CtrlResp.gpsPos.latitude = (long)(gpsDt.latitude*10000*(-1));//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}

		if(gpsDt.is_east)
		{
			App_rmtChargeCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000);//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		else
		{
			App_rmtChargeCtrl.CtrlResp.gpsPos.longitude = (long)(gpsDt.longitude*10000*(-1));//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
		}
		log_i(LOG_HOZON, "PP_appData.latitude = %lf",App_rmtChargeCtrl.CtrlResp.gpsPos.latitude);
		log_i(LOG_HOZON, "PP_appData.longitude = %lf",App_rmtChargeCtrl.CtrlResp.gpsPos.longitude);
	}
	else
	{
		App_rmtChargeCtrl.CtrlResp.gpsPos.latitude  = 0;
		App_rmtChargeCtrl.CtrlResp.gpsPos.longitude = 0;
	}
	App_rmtChargeCtrl.CtrlResp.gpsPos.altitude = (long)(gpsDt.height);//�߶ȣ�m��
	if(App_rmtChargeCtrl.CtrlResp.gpsPos.altitude > 10000)
	{
		App_rmtChargeCtrl.CtrlResp.gpsPos.altitude = 10000;
	}
	App_rmtChargeCtrl.CtrlResp.gpsPos.heading = (long)(gpsDt.direction);//��ͷ����Ƕȣ�0Ϊ��������
	App_rmtChargeCtrl.CtrlResp.gpsPos.gpsSpeed = (long)(gpsDt.kms*10);//�ٶ� x 10����λkm/h
	App_rmtChargeCtrl.CtrlResp.gpsPos.hdop = (long)(gpsDt.hdop*10);//ˮƽ�������� x 10
	if(App_rmtChargeCtrl.CtrlResp.gpsPos.hdop > 1000)
	{
		App_rmtChargeCtrl.CtrlResp.gpsPos.hdop = 1000;
	}

	App_rmtChargeCtrl.CtrlResp.basicSt.driverDoor = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.driverLock = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.passengerDoor = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.passengerLock = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearLeftDoor = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearLeftLock = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearRightDoor = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearRightLock = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.bootStatus = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.bootStatusLock = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.driverWindow = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.passengerWindow = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearLeftWindow = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearRightWinow = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.sunroofStatus = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.engineStatus = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.accStatus = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.accTemp = 18	/* OPTIONAL */;//18-36
	App_rmtChargeCtrl.CtrlResp.basicSt.accMode = 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.accBlowVolume	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.innerTemp = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.outTemp = 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.sideLightStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.dippedBeamStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.mainBeamStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.hazardLightStus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.frtRightTyrePre	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.frtRightTyreTemp	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.frontLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.frontLeftTyreTemp= 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearRightTyrePre= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearRightTyreTemp= 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearLeftTyrePre	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.rearLeftTyreTemp	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.batterySOCExact= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.chargeRemainTim	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.availableOdomtr= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.engineRunningTime	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.bookingChargeSt= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.bookingChargeHour= 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.bookingChargeMin	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.chargeMode	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.chargeStatus	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.powerMode	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.speed= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.totalOdometer= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.batteryVoltage= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.batteryCurrent= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.batterySOCPrc= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.dcStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.gearPosition= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.insulationRstance= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.acceleratePedalprc= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.deceleratePedalprc= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.canBusActive= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.bonnetStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.lockStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.gsmStatus= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.wheelTyreMotrSt= 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.vehicleAlarmSt= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.currentJourneyID= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.journeyOdom= 1;
	App_rmtChargeCtrl.CtrlResp.basicSt.frtLeftSeatHeatLel= 1	/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.frtRightSeatHeatLel	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.airCleanerSt	= 1/* OPTIONAL */;
	App_rmtChargeCtrl.CtrlResp.basicSt.srsStatus = 1;

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_RESP,PP_rmtChargeCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtACCtrl->pack.DisBody,&App_rmtChargeCtrl))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtChargeCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtChargeCtrl_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_control_response", PP_rmtChargeCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}


/******************************************************
*��������PP_ChargeCtrl_BookingStatusResp

*��  �Σ�

*����ֵ��

*��  ����remote control booking status response

*��  ע��
******************************************************/
static int PP_ChargeCtrl_BookingStatusResp(PrvtProt_task_t *task,PrvtProt_rmtChargeCtrl_t *rmtChargeCtrl)
{
	int msgdatalen;
	int res;

	/*header*/
	rmtChargeCtrl->pack.Header.ver.Byte = task->version;
	rmtChargeCtrl->pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	rmtChargeCtrl->pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);
	memcpy(&PP_rmtChargeCtrl_Pack, &rmtChargeCtrl->pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	rmtChargeCtrl->pack.DisBody.mID = PP_MID_RMTCTRL_BOOKINGRESP;
	//rmtdoorCtrl->pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	rmtChargeCtrl->pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtChargeCtrl->pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtChargeCtrl->pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	PrvtProt_App_rmtCtrl_t app_rmtCtrl;
	/*appdata*/
	app_rmtCtrl.CtrlbookingResp.bookingId = 1;
	app_rmtCtrl.CtrlbookingResp.rvcReqCode = rmtChargeCtrl->state.rvcReqCode;
	app_rmtCtrl.CtrlbookingResp.oprTime = PrvtPro_getTimestamp();

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_BOOKINGRESP,PP_rmtChargeCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtChargeCtrl->pack.DisBody,&app_rmtCtrl))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return 0;
	}

	PP_rmtChargeCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	res = sockproxy_MsgSend(PP_rmtChargeCtrl_Pack.Header.sign,18 + msgdatalen,NULL);

	protocol_dump(LOG_HOZON, "get_remote_control_booking_response", PP_rmtChargeCtrl_Pack.Header.sign, \
					18 + msgdatalen,1);
	return res;
}



/******************************************************
*��������SetPP_ChargeCtrl_Request

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
void SetPP_ChargeCtrl_Request(void *appdatarmtCtrl,void *disptrBody)
{
	PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
	PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
	static char rvcReqCode = 0;
	log_i(LOG_HOZON, "remote AC control req");
	PP_rmtChargeCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
	PP_rmtChargeCtrl.state.req = 1;
	PP_rmtChargeCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
	PP_rmtChargeCtrl.state.bookingSt = 0;
	if(appdatarmtCtrl_ptr->CtrlReq.rvcReqParamslen > 0)//��ԤԼ
	{
		PP_rmtChargeCtrl.state.bookingSt = 1;
		switch(rvcReqCode)
		{
			case 0:
			{
				PP_rmtChargeCtrl.state.rvcReqCode = 0x0710;
			}
			break;
			case 1:
			{
				PP_rmtChargeCtrl.state.rvcReqCode = 0x0711;
			}
			break;
			case 2:
			{
				PP_rmtChargeCtrl.state.rvcReqCode = 0x0700;
			}
			break;
			case 3:
			{
				PP_rmtChargeCtrl.state.rvcReqCode = 0x0701;
			}
			break;
			default:
			{
				rvcReqCode = 0;
				PP_rmtChargeCtrl.state.rvcReqCode = 0x0710;
			}
			break;
		}
		rvcReqCode++;
	}
}

/******************************************************
*��������PP_ChargeCtrl_SetCtrlReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtChargeCtrl.state.reqType = (long)reqType;
	PP_rmtChargeCtrl.state.req = 1;
}

