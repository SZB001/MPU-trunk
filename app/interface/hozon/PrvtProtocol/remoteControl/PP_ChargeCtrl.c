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
	uint8_t fail;//����ִ��ʧ�ܱ�־��0--�ɹ���1-ʧ��
	uint8_t failtype;//ʧ������
}__attribute__((packed))  PrvtProt_rmtChargeCtrl_t; /*�ṹ��*/

static PrvtProt_rmtChargeCtrl_t PP_rmtChargeCtrl;
static PrvtProt_pack_t 			PP_rmtChargeCtrl_Pack;
static PrvtProt_App_rmtCtrl_t 	App_rmtChargeCtrl;
static PP_rmtCharge_AppointBook_t		PP_rmtCharge_AppointBook;
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

	//��ȡ����
	len = 12;
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
				if((PP_rmtCtrl_cfg_chargeGunCnctSt() == 1) && \
						(PP_rmtCtrl_cfg_readyLightSt() == 0))//���ǹ���� && ��������ʻ״̬
				{
					if(PP_rmtChargeCtrl.state.style == RMTCTRL_TSP)//tsp ƽ̨
					{
						log_o(LOG_HOZON,"start charge ctrl\n");
						PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
						rmtCtrl_Stpara.rvcReqStatus = 1;//��ʼִ��
						rmtCtrl_Stpara.rvcFailureType = 0;
						rmtCtrl_Stpara.reqType = PP_rmtChargeCtrl.CtrlPara.reqType;
						rmtCtrl_Stpara.eventid = PP_rmtChargeCtrl.pack.DisBody.eventId;
						rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
						PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_Stpara);
					}
					else// ����
					{
						log_o(LOG_HOZON,"bluetooth platform");
					}
					PP_rmtChargeCtrl.state.CtrlSt   = PP_CHARGECTRL_REQSTART;
				}
				else
				{//
					log_i(LOG_HOZON,"The vehicle control condition is not satisfied\n");
					PP_rmtChargeCtrl.fail     = 1;
					PP_rmtChargeCtrl.failtype = PP_RMTCTRL_CHRGGUNUNCONNT;
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
					if(PP_rmtCtrl_cfg_chargeOnOffSt() == 1) //�����ɹ�
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
					if(PP_rmtCtrl_cfg_chargeOnOffSt() == 2) //�رճɹ�
					{
						log_o(LOG_HOZON,"close  success\n");
						PP_rmtChargeCtrl.state.chargeSt = 0;//δ���
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
			if(PP_rmtChargeCtrl.state.style   == RMTCTRL_TSP)
			{
				PP_rmtCtrl_Stpara_t rmtCtrl_chargeStpara;
				memset(&rmtCtrl_chargeStpara,0,sizeof(PP_rmtCtrl_Stpara_t));

				rmtCtrl_chargeStpara.reqType  = PP_rmtChargeCtrl.CtrlPara.reqType;
				rmtCtrl_chargeStpara.eventid  = PP_rmtChargeCtrl.pack.DisBody.eventId;
				rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;//��ԤԼ
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
				PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_chargeStpara);
			}
			else if(PP_rmtChargeCtrl.state.style   == RMTCTRL_TBOX)//��ʾԤԼִ�н��
			{

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

/*
 *	���ԤԼ���
 * */


/*
 * 	�������У���������״̬
 *  */
	if(1 == PP_rmtChargeCtrl.state.chargeSt)
	{
		if((PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt()) || \
				(PP_RMTCTRL_CFG_CHARGEFAIL == PP_rmtCtrl_cfg_chargeSt()))
		{
			if(PP_RMTCTRL_CFG_CHARGEFINISH == PP_rmtCtrl_cfg_chargeSt())//������
			{
				rmtCtrl_chargeStpara.rvcReqCode = 0x700;
			}
			else
			{
				rmtCtrl_chargeStpara.rvcReqCode = 0x701;
			}
			//�ϱ��������TSP
			memset(&rmtCtrl_chargeStpara,0,sizeof(PP_rmtCtrl_Stpara_t));
			if(1 == PP_rmtChargeCtrl.state.bookingSt)
			{
				rmtCtrl_chargeStpara.bookingId  = PP_rmtChargeCtrl.CtrlPara.bookingId;
			}
			else
			{
				rmtCtrl_chargeStpara.bookingId = 0;
			}
			rmtCtrl_chargeStpara.eventid  = PP_rmtChargeCtrl.pack.DisBody.eventId;
			rmtCtrl_chargeStpara.Resptype = PP_RMTCTRL_RVCBOOKINGRESP;//ԤԼ
			PP_rmtCtrl_StInformTsp((PrvtProt_task_t *)task,&rmtCtrl_chargeStpara);
		}
		else
		{}
	}
	else
	{
		if(PP_RMTCTRL_CFG_CHARGEING == PP_rmtCtrl_cfg_chargeSt())//��⵽�����
		{
			PP_rmtChargeCtrl.state.chargeSt = 1;
		}
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
					PP_rmtChargeCtrl.state.bookingSt = 0;//��ԤԼ���
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
			{//ԤԼ
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];
				PP_rmtCharge_AppointBook.id = appointId;
				PP_rmtCharge_AppointBook.hour = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[4];
				PP_rmtCharge_AppointBook.min = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[5];
				PP_rmtCharge_AppointBook.targetSOC = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[6];
				PP_rmtCharge_AppointBook.period = appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[7];
				PP_rmtCharge_AppointBook.validFlg  = 1;
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.id = %d\n",PP_rmtCharge_AppointBook.id);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.hour = %d\n",PP_rmtCharge_AppointBook.hour);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.min = %d\n",PP_rmtCharge_AppointBook.min);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.targetSOC = %d\n",PP_rmtCharge_AppointBook.targetSOC);
				log_i(LOG_HOZON, "PP_rmtCharge_AppointBook.period = %d\n",PP_rmtCharge_AppointBook.period);
				//����ԤԼ��¼
				(void)cfg_set_para(CFG_ITEM_HOZON_TSP_RMTAPPOINT,&PP_rmtCharge_AppointBook,12);
			}
			else if(appdatarmtCtrl_ptr->CtrlReq.rvcReqType == PP_COMAND_CANCELAPPOINTCHARGE)
			{//ȡ��ԤԼ
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[0] << 24;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[1] << 16;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[2] << 8;
				appointId |= (uint32_t)appdatarmtCtrl_ptr->CtrlReq.rvcReqParams[3];
				if(PP_rmtCharge_AppointBook.id == appointId)
				{
					PP_rmtCharge_AppointBook.validFlg  = 0;
					log_i(LOG_HOZON, "cancel appointment\n");
				}
				else
				{
					log_e(LOG_HOZON, "appointment id error,exit cancel appointment\n");
				}
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

		}
		break;
		default:
		break;
	}
}

void ClearPP_ChargeCtrl_Request(void )
{
	PP_rmtChargeCtrl.state.req = 0;
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


