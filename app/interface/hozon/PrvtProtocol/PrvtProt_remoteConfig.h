/******************************************************
�ļ�����	PrvtProt_remoteConfig.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_REMOTE_CFG_H
#define		_PRVTPROT_REMOTE_CFG_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define RMTCFG_DELAY_TIME 		5500//
//#define PP_XCALL_ACK_WAIT 	0x01//Ӧ��ɹ�
//#define PP_XCALL_ACK_SUCCESS 	0x02//Ӧ��ɹ�

/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/

typedef enum
{
	PP_RMTCFG_CFG_IDLE = 0,//
	PP_CHECK_CFG_REQ,//���Զ������
	PP_CHECK_CFG_RESP,//
	PP_GET_CFG_REQ,//
	PP_GET_CFG_RESP,//
	PP_RMTCFG_CFG_END,//
} PP_RMTCFG_STATE_MACHINE;//״̬��

typedef enum
{
	PP_RMTCFG_WAIT_IDLE = 0,//
	PP_RMTCFG_CHECK_WAIT_RESP,//
	PP_RMTCFG_GET_WAIT_RESP,//
} PP_RMTCFG_WAIT_STATE;//�ȴ�״̬

/*****struct definitions*****/
typedef struct
{
	uint8_t req;
	uint8_t CfgSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_rmtCfgSt_t; /*remote config�ṹ��*/


/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_rmtCfg_init(void);
extern int PP_rmtCfg_mainfunction(void *task);
extern void PP_rmtCfg_SetCfgReq(unsigned char req);


#endif 
