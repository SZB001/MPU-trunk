/******************************************************
文件名：PP_doorLockCtrl.h

描述：	车门锁控制

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_DOORLOCK_CTRL_H
#define		_PP_DOORLOCK_CTRL_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_DOORLOCKCTRL_IDLE   		0
#define PP_DOORLOCKCTRL_REQSTART  	1
#define PP_DOORLOCKCTRL_RESPWAIT   	2
#define PP_DOORLOCKCTRL_END    		3
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
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	long expTime;
	char style;//方式：tsp-1；2-蓝牙
}__attribute__((packed))  PP_rmtdoorCtrlSt_t; /*remote control结构体*/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_doorLockCtrl_init(void);
extern int 	PP_doorLockCtrl_mainfunction(void *task);
extern void SetPP_doorLockCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);
extern void PP_doorLockCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);
extern  int PP_doorLockCtrl_start(void);
extern void PP_doorLockCtrl_ClearStatus(void);

extern int PP_doorLockCtrl_end(void);
#endif 
