/******************************************************
�ļ�����	PrvtProt_rmtCtrl.c

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
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_queue.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt_cfg.h"
#include "../PrvtProt.h"
#include "PP_doorLockCtrl.h"
#include "PP_ACCtrl.h"
#include "PP_ChargeCtrl.h"
#include "PP_rmtCtrl_data.h"
#include "PP_rmtCtrl.h"

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
}__attribute__((packed))  PrvtProt_rmtCtrl_pack_t; /**/

typedef struct
{
	PrvtProt_rmtCtrl_pack_t pack;
	PrvtProt_rmtCtrlSt_t	state[RMTCTRL_OBJ_MAX];
}__attribute__((packed))  PrvtProt_rmtCtrl_t; /*�ṹ��*/

static PrvtProt_rmtCtrl_t PP_rmtCtrl;
static PrvtProt_pack_t 		PP_rmtCtrl_Pack;

static PrvtProt_RmtCtrlFunc_t PP_RmtCtrlFunc[RMTCTRL_OBJ_MAX] =
{
	{RMTCTRL_DOORLOCK,PP_doorLockCtrl_init,	PP_doorLockCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_AC,	  PP_ACCtrl_init, 		PP_ACCtrl_mainfunction},
	{RMTCTRL_CHARGE,  PP_ChargeCtrl_init,	PP_ChargeCtrl_mainfunction},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL},
	{RMTCTRL_PANORSUNROOF,NULL,	NULL}
};


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task);
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
//static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task);
static int PP_rmtCtrl_do_report(PrvtProt_task_t *task);
static int PP_rmtCtrl_StatusResp(long bookingId,unsigned int reqtype);
static void PP_rmtCtrl_send_cb(void * para);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_rmtCtrl_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_rmtCtrl_init(void)
{
	int i;
	memset(&PP_rmtCtrl,0,sizeof(PrvtProt_rmtCtrl_t));

	for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
	{
		PP_rmtCtrl.state[i].reqType = PP_RMTCTRL_UNKNOW;
		if(PP_RmtCtrlFunc[i].Init != NULL)
		{
			PP_RmtCtrlFunc[i].Init();
		}
	}

	PP_rmtCtrl_data_init();
}

/******************************************************
*��������PP_rmtCtrl_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_rmtCtrl_mainfunction(void *task)
{
	int res;
	int i;
	res = 		PP_rmtCtrl_do_checksock((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_rcvMsg((PrvtProt_task_t*)task) ||
				PP_rmtCtrl_do_report((PrvtProt_task_t*)task) ;

	for(i = 0;i < RMTCTRL_OBJ_MAX;i++)
	{
		if(PP_RmtCtrlFunc[i].mainFunc != NULL)
		{
			PP_RmtCtrlFunc[i].mainFunc((PrvtProt_task_t*)task);
		}
	}

	return res;
}

/******************************************************
*��������PP_rmtCtrl_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_checksock(PrvtProt_task_t *task)
{
	if(1 == sockproxy_socketState())//socket open
	{

		return 0;
	}

	//�ͷ���Դ

	return -1;
}

/******************************************************
*��������PP_rmtCtrl_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_REMOTE_CTRL,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive rmt ctrl message");
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//�ж�����֡ͷ����������ݳ��Ȳ���
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//�������ݳ��ȳ�������buffer����
	{
		return 0;
	}
	PP_rmtCtrl_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_rmtCtrl_do_report

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_report(PrvtProt_task_t *task)
{
	int res;
	PrvtProt_RmtCtrlSend_t *rpt;

    if ((rpt = PP_rmtCtrl_data_get_pack()) != NULL)
    {
        log_i(LOG_HOZON, "start to send report to server");
        res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, NULL);
        protocol_dump(LOG_HOZON, "send control data to tsp", rpt->msgdata, rpt->msglen, 1);

        if (res < 0)
        {
            log_e(LOG_HOZON, "socket send error, reset protocol");
            PP_rmtCtrl_data_put_back(rpt);
            sockproxy_socketclose();//by liujian 20190510
        }
        else if (res == 0)
        {
            log_e(LOG_HOZON, "unack list is full, send is canceled");
            PP_rmtCtrl_data_put_back(rpt);
        }
        else
        {
        	if(rpt->SendInform_cb != NULL)
        	{
        		rpt->SendInform_cb(rpt->Inform_cb_para);
        	}
        }
    }

    return 0;
}
/******************************************************
*��������PP_rmtCtrl_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_rmtCtrl_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{
	int aid;
	if(PP_NGTP_TYPE != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	PrvtProt_DisptrBody_t MsgDataBody;
	PrvtProt_App_rmtCtrl_t Appdata;
	PrvtPro_decodeMsgData(rxPack->msgdata,(len - 18),&MsgDataBody,&Appdata);
	aid = (MsgDataBody.aID[0] - 0x30)*100 +  (MsgDataBody.aID[1] - 0x30)*10 + \
			  (MsgDataBody.aID[2] - 0x30);
	if(PP_AID_RMTCTRL != aid)
	{
		log_e(LOG_HOZON, "aid unmatch");
		return;
	}

	if(PP_MID_RMTCTRL_REQ == MsgDataBody.mID)//�յ�����
	{
		switch((uint8_t)(Appdata.CtrlReq.rvcReqType >> 8))
		{
			case PP_RMTCTRL_DOORLOCK://���Ƴ�����
			{
				SetPP_doorLockCtrl_Request(&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_PNRSUNROOF://����ȫ���촰
			{
				log_i(LOG_HOZON, "remote PANORSUNROOF control req");
			}
			break;
			case PP_RMTCTRL_AUTODOOR://�����Զ���
			{
				log_i(LOG_HOZON, "remote AUTODOOR control req");
			}
			break;
			case PP_RMTCTRL_RMTSRCHVEHICLE://Զ����������
			{
				log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
			}
			break;
			case PP_RMTCTRL_DETECTCAMERA://��ʻԱ�������ͷ
			{
				log_i(LOG_HOZON, "remote DETECTCAMERA control req");
			}
			break;
			case PP_RMTCTRL_DATARECORDER://�г���¼��
			{
				log_i(LOG_HOZON, "remote DATARECORDER control req");
			}
			break;
			case PP_RMTCTRL_AC://�յ�
			{
				SetPP_ACCtrl_Request(&Appdata,&MsgDataBody);
			}
			break;
			case PP_RMTCTRL_CHARGE://���
			{
				SetPP_ChargeCtrl_Request(&Appdata,&MsgDataBody);
				log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
			}
			break;
			case PP_RMTCTRL_HIGHTENSIONCTRL://�ߵ�ѹ����
			{
				log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
			}
			break;
			case PP_RMTCTRL_ENGINECTRL://����������
			{
				log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
			}
			break;
			default:
			break;
		}
	}
}

#if 0
/******************************************************
*��������PP_rmtCtrl_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_rmtCtrl_do_wait(PrvtProt_task_t *task)
{

	return 0;
}
#endif

/******************************************************
*��������PP_rmtCtrl_SetCtrlReq

*��  �Σ�

*����ֵ��

*��  ��������ecall ����

*��  ע��
******************************************************/
void PP_rmtCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	switch((uint8_t)(reqType >> 8))
	{
		case PP_RMTCTRL_DOORLOCK://���Ƴ�����
		{
			PP_doorLockCtrl_SetCtrlReq(req,reqType);
		}
		break;
		case PP_RMTCTRL_PNRSUNROOF://����ȫ���촰
		{
			log_i(LOG_HOZON, "remote PANORSUNROOF control req");
		}
		break;
		case PP_RMTCTRL_AUTODOOR://�����Զ���
		{
			log_i(LOG_HOZON, "remote AUTODOOR control req");
		}
		break;
		case PP_RMTCTRL_RMTSRCHVEHICLE://Զ����������
		{
			log_i(LOG_HOZON, "remote RMTSRCHVEHICLE control req");
		}
		break;
		case PP_RMTCTRL_DETECTCAMERA://��ʻԱ�������ͷ
		{
			log_i(LOG_HOZON, "remote DETECTCAMERA control req");
		}
		break;
		case PP_RMTCTRL_DATARECORDER://�г���¼��
		{
			//PP_doorLockCtrl_StatusResp(reqType);
			log_i(LOG_HOZON, "remote DATARECORDER control req");
		}
		break;
		case PP_RMTCTRL_AC://�յ�
		{
			PP_rmtCtrl_StatusResp(1,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_AC control req");
		}
		break;
		case PP_RMTCTRL_CHARGE://���
		{
			PP_rmtCtrl_StatusResp(2,reqType);
			log_i(LOG_HOZON, "remote RMTCTRL_CHARGE control req");
		}
		break;
		case PP_RMTCTRL_HIGHTENSIONCTRL://�ߵ�ѹ����
		{
			log_i(LOG_HOZON, "remote RMTCTRL_HIGHTENSIONCTRL control req");
		}
		break;
		case PP_RMTCTRL_ENGINECTRL://����������
		{
			log_i(LOG_HOZON, "remote RMTCTRL_ENGINECTRL control req");
		}
		break;
		default:
		break;
	}
}

/******************************************************
*��������PP_rmtCtrl_StatusResp

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static int PP_rmtCtrl_StatusResp(long bookingId,unsigned int reqtype)
{
	int msgdatalen;

	PrvtProt_rmtCtrl_pack_t rmtCtrl_pack;
	memset(&rmtCtrl_pack.DisBody,0,sizeof(PrvtProt_rmtCtrl_pack_t));
	/*header*/
	memcpy(rmtCtrl_pack.Header.sign,"**",2);
	rmtCtrl_pack.Header.ver.Byte = 0x30;
	rmtCtrl_pack.Header.commtype.Byte = 0xe1;
	rmtCtrl_pack.Header.opera = 0x02;
	rmtCtrl_pack.Header.nonce  = PrvtPro_BSEndianReverse(0);
	rmtCtrl_pack.Header.tboxid = PrvtPro_BSEndianReverse(27);
	memcpy(&PP_rmtCtrl_Pack, &rmtCtrl_pack.Header, sizeof(PrvtProt_pack_Header_t));
	/*body*/
	memcpy(rmtCtrl_pack.DisBody.aID,"110",3);
	rmtCtrl_pack.DisBody.mID = PP_MID_RMTCTRL_BOOKINGRESP;
	rmtCtrl_pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_BOOKINGRESP;
	rmtCtrl_pack.DisBody.eventTime = PrvtPro_getTimestamp();
	rmtCtrl_pack.DisBody.expTime   = PrvtPro_getTimestamp();
	rmtCtrl_pack.DisBody.appDataProVer = 256;
	rmtCtrl_pack.DisBody.testFlag = 1;
	rmtCtrl_pack.DisBody.ulMsgCnt++;	/* OPTIONAL */

	PrvtProt_App_rmtCtrl_t app_rmtCtrl;
	/*appdata*/
	app_rmtCtrl.CtrlbookingResp.bookingId = bookingId;
	app_rmtCtrl.CtrlbookingResp.rvcReqCode = (long)reqtype;
	app_rmtCtrl.CtrlbookingResp.oprTime = PrvtPro_getTimestamp();

	if(0 != PrvtPro_msgPackageEncoding(ECDC_RMTCTRL_BOOKINGRESP,PP_rmtCtrl_Pack.msgdata,&msgdatalen,\
									   &rmtCtrl_pack.DisBody,&app_rmtCtrl))//���ݱ������Ƿ����
	{
		log_e(LOG_HOZON, "uper error");
		return -1;
	}

	PP_rmtCtrl_Pack.Header.msglen = PrvtPro_BSEndianReverse((long)(18 + msgdatalen));
	//res = sockproxy_MsgSend(PP_rmtCtrl_Pack.Header.sign,18 + msgdatalen,NULL);
	PP_rmtCtrl_data_write(PP_rmtCtrl_Pack.Header.sign,18 + msgdatalen,PP_rmtCtrl_send_cb,NULL);
	return 0;
}

/******************************************************
*��������PP_rmtCtrl_send_cb

*��  �Σ�

*����ֵ��

*��  ����remote control status response

*��  ע��
******************************************************/
static void PP_rmtCtrl_send_cb(void * para)
{
	log_e(LOG_HOZON, "send ok");


}
