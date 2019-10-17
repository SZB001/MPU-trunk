#ifndef		_PP_BLUETOOTHSTART_H
#define		_PP_BLUETOOTHSTART_H

#define PP_BLUETOOTHSTART_IDLE   		0
#define PP_BLUETOOTHSTART_REQSTART  	1
#define PP_BLUETOOTHSTART_RESPWAIT   	2
#define PP_BLUETOOTHSTARTL_END    		3

typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t cmd;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;//·½Ê½£ºtsp-1£»2-À¶ÑÀ
}__attribute__((packed))  PP_bluetoothStart_t; 
extern void PP_bluetoothstart_init(void);
extern int PP_bluetoothstart_mainfunction(void *task);
extern void SetPP_bluetoothstart_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);
extern int PP_bluetoothstart_start(void);
extern int PP_bluetoothstart_end(void);
extern void PP_bluetoothstart_ClearStatus(void);
extern void PP_bluetoothstart_SetCtrlReq(unsigned char req,uint16_t reqType);
#endif 

