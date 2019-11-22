
#ifndef		_PP_AC_CTRL_H
#define		_PP_AC_CTRL_H


#define PP_ACCTRL_IDLE   		0
#define PP_ACCTRL_REQSTART  	1
#define PP_ACCTRL_RESPWAIT   	2
#define PP_ACCTRL_END    		3
#define ACC_APPOINT_NUM 10

/******enum definitions******/
typedef struct
{
	uint8_t req;
	uint8_t accmd;
	uint8_t bookingSt;//是否预约
	uint8_t executSt;//执行状态
	uint8_t CtrlSt;
	char 	style;//方式：tsp-1；2-蓝牙；3-HU；4-tbox
	uint64_t period;
	uint8_t  waitSt;
	uint64_t waittime;
	long	expTime;
	uint8_t  dataUpdata;
	uint8_t failtype;
}__attribute__((packed))  PP_rmtACCtrlSt_t; /*remote control�ṹ��*/

typedef struct
{
	//tsp
	long 	reqType;
	long 	rvcReqCode;
	long 	bookingId;
}__attribute__((packed))  PP_rmtACCtrlPara_t; /*结构体*/


typedef struct
{
	//预约记录
	uint8_t  validFlg;
	uint32_t id;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  period;
	uint8_t eventId;
	uint8_t	bookupdataflag;
}__attribute__((packed))  PP_rmtAC_AppointBook_t; /*结构体*/

typedef struct
{
	//预约周期
	uint8_t  week;
	uint8_t  mask;
} PP_rmtAc_Appointperiod_t; /*结构体*/

extern void PP_ACCtrl_init(void);
extern int 	PP_ACCtrl_mainfunction(void *task);

extern uint8_t PP_ACCtrl_start(void);
extern uint8_t PP_ACCtrl_end(void);
extern int SetPP_ACCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);
extern void ClearPP_ACCtrl_Request(void);
extern void PP_ACCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);
extern void PP_AcCtrl_acStMonitor(void *task);
extern unsigned char GetPP_ACtrl_Sleep(void);
extern int PP_ACCtrl_waketime(void);
extern uint8_t PP_ACCtrl_cmdoff(void);
#endif 
