/******************************************************
文件名：PP_StartEngine.h

描述：	车门锁控制

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_STARTENGINE_H
#define		_PP_STARTENGINE_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_STARTENGINE_IDLE   		0
#define PP_STARTENGINE_REQSTART  	1
#define PP_STARTENGINE_RESPWAIT   	2
#define PP_STARTENGINE_END    		3
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
}__attribute__((packed))  PP_rmtstartengineSt_t;


extern void PP_startengine_init(void);


extern int PP_startengine_mainfunction(void *task);


extern uint8_t PP_startengine_start(void) ;


extern uint8_t PP_startengine_end(void);


extern void SetPP_startengine_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);


extern void PP_startengine_SetCtrlReq(unsigned char req,uint16_t reqType);
extern  void ClearPP_startengine_Request(void);

extern void PP_rmtCtrl_checkenginetime(void);


#endif




