
/******************************************************
文件名：PP_StartForbid.h

描述：	车门锁控制

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_STARTFORBID_H
#define		_PP_STARTFORBID_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_STARTFORBID_IDLE   		0
#define PP_STARTFORBID_REQSTART  	1
#define PP_STARTFORBID_RESPWAIT   	2
#define PP_STARTFORBID_END    		3
/***********宏函数***********/



typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;
}__attribute__((packed))  PP_rmtstartforbidSt_t;


extern void PP_startforbid_init(void);


extern int PP_startforbid_mainfunction(void *task);


extern uint8_t PP_startforbid_start(void) ;


extern uint8_t PP_startforbid_end(void);


extern void SetPP_startforbid_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);

extern void ClearPP_startforbid_Request(void);

extern void PP_startforbid_SetCtrlReq(unsigned char req,uint16_t reqType);


#endif






