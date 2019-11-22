#ifndef		_PP_SUNROOFCTRL_H
#define		_PP_SUNROOFCTRL_H

#define PP_SUNROOFCTRL_IDLE   		0
#define PP_SUNROOFCTRL_REQSTART    	1
#define PP_SUNROOFCTRL_RESPWAIT   	2
#define PP_SUNROOFCTRL_END    		3

typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	long expTime;
	uint8_t failtype;
	char style;
}__attribute__((packed))  PP_rmtsunroofCtrlSt_t;

extern void PP_sunroofctrl_init(void);

extern int PP_sunroofctrl_mainfunction(void *task);

extern uint8_t PP_sunroofctrl_start(void) ;

extern uint8_t PP_sunroofctrl_end(void);

extern int SetPP_sunroofctrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);

extern void PP_sunroofctrl_ClearStatus(void);

extern void PP_sunroofctrl_SetCtrlReq(unsigned char req,uint16_t reqType);


#endif





