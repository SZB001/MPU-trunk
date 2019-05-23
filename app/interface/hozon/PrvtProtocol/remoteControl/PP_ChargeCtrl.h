/******************************************************
文件名：PP_ChargeCtrl.h

描述：	车门锁控制

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_CHARGE_CTRL_H
#define		_PP_CHARGE_CTRL_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/


/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef struct
{
	uint8_t req;
	long reqType;
	long rvcReqCode;
	uint8_t bookingSt;//是否预约
	uint8_t executSt;//执行状态
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PP_rmtChargeCtrlSt_t; /*remote control结构体*/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_ChargeCtrl_init(void);
extern int 	PP_ChargeCtrl_mainfunction(void *task);
extern void SetPP_ChargeCtrl_Request(void *appdatarmtCtrl,void *disptrBody);
extern void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);

#endif 
