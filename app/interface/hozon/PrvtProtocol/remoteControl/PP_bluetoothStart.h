#ifndef		_PP_BLUETOOTHSTART_H
#define		_PP_BLUETOOTHSTART_H

#define PP_DOORLOCKCTRL_IDLE   		0
#define PP_DOORLOCKCTRL_REQSTART  	1
#define PP_DOORLOCKCTRL_RESPWAIT   	2
#define PP_DOORLOCKCTRL_END    		3

typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;//·½Ê½£ºtsp-1£»2-À¶ÑÀ
}__attribute__((packed))  PP_bluetoothStart_t; 
#endif 

