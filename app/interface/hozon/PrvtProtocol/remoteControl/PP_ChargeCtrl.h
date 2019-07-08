/******************************************************
�ļ�����PP_ChargeCtrl.h

������	����������

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_CHARGE_CTRL_H
#define		_PP_CHARGE_CTRL_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_CHARGECTRL_IDLE   		0
#define PP_CHARGECTRL_REQSTART  	1
#define PP_CHARGECTRL_RESPWAIT   	2
#define PP_CHARGECTRL_END    		3

#define PP_COMAND_STARTCHARGE   	0x700
#define PP_COMAND_STOPCHARGE  		0x701
#define PP_COMAND_APPOINTCHARGE   	0x702
#define PP_COMAND_CANCELAPPOINTCHARGE    	0x703

#define PP_CHARGECTRL_OPEN   		1
#define PP_CHARGECTRL_CLOSE  		0
/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef struct
{
	uint8_t req;
	uint8_t chargecmd;
	uint8_t bookingSt;//�Ƿ�ԤԼ
	uint8_t executSt;//ִ��״̬
	uint8_t CtrlSt;
	char 	style;//��ʽ��tsp-1��2-������3-HU��4-tbox
	uint64_t period;
	uint8_t  waitSt;
	uint64_t waittime;
	uint8_t  chargeSt;//���״̬��1-����У�0-δ���
}__attribute__((packed))  PP_rmtChargeCtrlSt_t; /*remote control�ṹ��*/


typedef struct
{
	//tsp
	long 	reqType;
	long 	rvcReqCode;
	long 	bookingId;
}__attribute__((packed))  PP_rmtChargeCtrlPara_t; /*�ṹ��*/


typedef struct
{
	//ԤԼ��¼
	uint8_t  validFlg;
	uint32_t id;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  targetSOC;
	uint8_t  period;
}__attribute__((packed))  PP_rmtCharge_AppointBook_t; /*�ṹ��*/
/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_ChargeCtrl_init(void);
extern int 	PP_ChargeCtrl_mainfunction(void *task);
extern void SetPP_ChargeCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);
extern void ClearPP_ChargeCtrl_Request(void );

extern void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);
extern void PP_ChargeCtrl_chargeStMonitor(void *task);
#endif 
