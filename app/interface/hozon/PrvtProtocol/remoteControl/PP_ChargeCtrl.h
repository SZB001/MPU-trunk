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
	long reqType;
	long rvcReqCode;
	uint8_t bookingSt;//�Ƿ�ԤԼ
	uint8_t executSt;//ִ��״̬
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PP_rmtChargeCtrlSt_t; /*remote control�ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_ChargeCtrl_init(void);
extern int 	PP_ChargeCtrl_mainfunction(void *task);
extern void SetPP_ChargeCtrl_Request(void *appdatarmtCtrl,void *disptrBody);
extern void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);

#endif 
