/******************************************************
�ļ�����PP_ChargeCtrl.h

������	����������

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_CHARGE_CTRL_H
#define		_PP_CHARGE_CTRL_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_CHARGECTRL_APPOINTHOLDTIME   (6*60*60*1000)

#define PP_CHARGECTRL_IDLE   		0
#define PP_CHARGECTRL_REQSTART  	1
#define PP_CHARGECTRL_RESPWAIT   	2
#define PP_CHARGECTRL_END    		3

#define PP_COMAND_STARTCHARGE   	0x700
#define PP_COMAND_STOPCHARGE  		0x701
#define PP_COMAND_APPOINTCHARGE   	0x702
#define PP_COMAND_CANCELAPPOINTCHARGE    	0x703

#define PP_CHARGECTRL_OPEN   		2
#define PP_CHARGECTRL_CLOSE  		1


#define PP_CHARGESTATE_IDLE   	0
#define PP_CHARGESTATE_READY	1
#define PP_CHARGESTATE_ONGOING  2
#define PP_CHARGESTATE_FINISH  	3
//#define PP_CHARGESTATE_UNCONNT  	4//充电枪未连接
//#define PP_CHARGESTATE_SPORT		5//运动模式
#define PP_CHARGESTATE_ABNRSHUTDOWN	6//异常关闭
//#define PP_CHARGESTATE_FOTA_UPGRADE	7//fota升级

/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef struct
{
	uint8_t req;
	long	expTime	;
	uint8_t chargecmd;
	uint8_t bookingSt;//�Ƿ�ԤԼ
	uint8_t executSt;//ִ��״̬
	uint8_t CtrlSt;
	char 	style;//��ʽ��tsp-1��2-������3-HU��4-tbox
	uint64_t period;
	uint8_t  waitSt;
	uint64_t waittime;
	uint8_t  chargeSt;//���״̬��1-����У�0-δ���
	uint8_t	bookSyncflag;//
	uint8_t  dataUpdata;

	uint8_t appointcharge;
	//uint8_t appointchargeSt;
	uint64_t appointchargeTime;
	uint64_t appointchargeCheckDelayTime;
}__attribute__((packed))  PP_rmtChargeCtrlSt_t; /*remote control*/


typedef struct
{
	//tsp
	long 	reqType;
	long 	rvcReqCode;
	long 	bookingId;
}__attribute__((packed))  PP_rmtChargeCtrlPara_t; /*�ṹ��*/


typedef struct
{
	//ԤԼ��¼
	uint8_t  appointType;//ԤԼ��¼���ͣ�1-tsp;3-HU
	uint8_t  validFlg;
	uint32_t id;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  targetSOC;
	uint8_t  period;
	uint32_t eventId;

	uint16_t rvcReqType;
	uint32_t huBookingTime;
	uint32_t HUbookingId;

	uint8_t	informtspflag;// 通知平台是否成功标志位
	//uint8_t appointChargeFlag;
	//uint32_t appointStartTime;
}__attribute__((packed))  PP_rmtCharge_AppointBook_t; /*�ṹ��*/

typedef struct
{
	//ԤԼ����
	uint8_t  week;
	uint8_t  mask;
} PP_rmtCharge_Appointperiod_t; /*�ṹ��*/
/******union definitions*****/

typedef enum
{
	CHARGE_SYNC_START = 0,
	CHARGE_SYNC_END ,
}PP_CHARGE_SYNC;//控制方式

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_ChargeCtrl_init(void);
extern int 	PP_ChargeCtrl_mainfunction(void *task);
extern int SetPP_ChargeCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody);

extern void PP_ChargeCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);
extern int PP_ChargeCtrl_start(void);
extern int PP_ChargeCtrl_end(void);
extern void PP_ChargeCtrl_ClearStatus(void);
extern void PP_ChargeCtrl_send_cb(void);
//extern void SetPP_ChargeCtrl_Awaken(void);
extern unsigned char GetPP_ChargeCtrl_Sleep(void);
extern void SetPP_ChargeCtrl_appointPara(void);
extern int PP_ChargeCtrl_waketime(void);
extern void PP_ChargeCtrl_HUBookingBackResp(void* HUbookingBackResp);
extern void PP_ChargeCtrl_informTsp(void);
#endif 
