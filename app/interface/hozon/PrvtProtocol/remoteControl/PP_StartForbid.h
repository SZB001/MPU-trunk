
/******************************************************
鏂囦欢鍚嶏細PP_StartForbid.h

鎻忚堪锛�	杞﹂棬閿佹帶鍒�

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_STARTFORBID_H
#define		_PP_STARTFORBID_H
/*******************************************************
description锛� include the header file
*******************************************************/

/*******************************************************
description锛� macro definitions
*******************************************************/
/**********瀹忓紑鍏冲畾涔�*********/

/**********瀹忓父閲忓畾涔�*********/
#define PP_STARTFORBID_IDLE   		0
#define PP_STARTFORBID_REQSTART  	1
#define PP_STARTFORBID_RESPWAIT   	2
#define PP_STARTFORBID_END    		3
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
}__attribute__((packed))  PP_rmtstartforbidSt_t;


extern void PP_startforbid_init(void);


extern int PP_startforbid_mainfunction(void *task);


extern uint8_t PP_startforbid_start(void) ;


extern uint8_t PP_startforbid_end(void);


extern void SetPP_startforbid_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);

extern void PP_startforbid_ClearStatus(void);

extern void PP_startforbid_SetCtrlReq(unsigned char req,uint16_t reqType);


#endif






