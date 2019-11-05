#ifndef		_PP_SEARCHVEHICLE_H
#define		_PP_SEARCHVEHICLE_H

#define PP_SEARCHVEHICLE_IDLE   		0
#define PP_SEARCHVEHICLE_REQSTART    	1
#define PP_SEARCHVEHICLE_RESPWAIT   	2
#define PP_SEARCHVEHICLE_END    		3

typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	long expTime;
	char style;
}__attribute__((packed))  PP_rmtsearchvehicleSt_t;


extern void PP_searchvehicle_init(void);


extern int PP_searchvehicle_mainfunction(void *task);


extern uint8_t PP_searchvehicle_start(void) ;


extern uint8_t PP_searchvehicle_end(void);


extern void SetPP_searchvehicle_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);


extern void PP_searchvehicle_SetCtrlReq(unsigned char req,uint16_t reqType);

extern  void PP_searchvehicle_ClearStatus(void);



#endif



