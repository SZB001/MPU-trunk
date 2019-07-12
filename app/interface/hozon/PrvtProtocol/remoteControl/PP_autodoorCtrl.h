#ifndef		_PP_AUTODOORCTRL_H
#define		_PP_AUTODOORCTRL_H

#define PP_AUTODOORCTRL_IDLE   		0
#define PP_AUTODOORCTR_REQSTART  	1
#define PP_AUTODOORCTR_RESPWAIT   	2
#define PP_AUTODOORCTR_END    		3


typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;
}__attribute__((packed))  PP_rmtautodoorCtrlSt_t;

extern void PP_autodoorCtrl_init(void);

extern int PP_autodoorCtrl_mainfunction(void *task);

extern uint8_t PP_autodoorCtrl_start(void) ;

extern uint8_t PP_autodoorCtrl_end(void);

extern void SetPP_autodoorCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);

extern void PP_autodoorCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);

extern void PP_autodoorCtrl_ClearStatus(void);

#endif


