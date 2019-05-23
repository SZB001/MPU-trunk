/******************************************************
�ļ�����PP_ACCtrl.h

������	����������

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_AC_CTRL_H
#define		_PP_AC_CTRL_H
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
}__attribute__((packed))  PP_rmtACCtrlSt_t; /*remote control�ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_ACCtrl_init(void);
extern int 	PP_ACCtrl_mainfunction(void *task);
extern void SetPP_ACCtrl_Request(void *appdatarmtCtrl,void *disptrBody);
extern void PP_ACCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);

#endif 
