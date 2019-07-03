

/******************************************************
鏂囦欢鍚嶏細PP_SeatHeating.h

鎻忚堪锛�	杞﹂棬閿佹帶鍒�

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_SEATHEATING_H
#define		_PP_SEATHEATING_H
/*******************************************************
description锛� include the header file
*******************************************************/

/*******************************************************
description锛� macro definitions
*******************************************************/
/**********瀹忓紑鍏冲畾涔�*********/

/**********瀹忓父閲忓畾涔�*********/
#define PP_SEATHEATING_IDLE   		0
#define PP_SEATHEATING_REQSTART  	1
#define PP_SEATHEATING_RESPWAIT   	2
#define PP_SEATHEATING_END    		3
/***********瀹忓嚱鏁�***********/



typedef struct
{
	uint8_t req;
	long reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
	char style;
}__attribute__((packed))  PP_rmtseatheatingSt_t;


extern void PP_seatheating_init(void);


extern int PP_seatheating_mainfunction(void *task);


extern uint8_t PP_seatheating_start(void) ;


extern uint8_t PP_seatheating_end(void);


extern void SetPP_seatheating_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);


extern void PP_seatheating_SetCtrlReq(unsigned char req,uint16_t reqType);

extern void ClearPP_seatheating_Request(void);


#endif

