
#ifndef		_PP_AC_CTRL_H
#define		_PP_AC_CTRL_H


#define PP_ACCTRL_IDLE   		0
#define PP_ACCTRL_REQSTART  	1
#define PP_ACCTRL_RESPWAIT   	2
#define PP_ACCTRL_END    		3

/******enum definitions******/
typedef struct
{
	uint8_t req;
	long reqType;
	long rvcReqCode;
	uint8_t bookingSt;
	uint8_t executSt;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;
}__attribute__((packed))  PP_rmtACCtrlSt_t; /*remote control½á¹¹Ìå*/


extern void PP_ACCtrl_init(void);
extern int 	PP_ACCtrl_mainfunction(void *task);

extern uint8_t PP_ACCtrl_start(void);
extern uint8_t PP_ACCtrl_end(void);
extern void SetPP_ACCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);
extern void ClearPP_ACCtrl_Request(void);
extern void PP_ACCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);




#endif 
