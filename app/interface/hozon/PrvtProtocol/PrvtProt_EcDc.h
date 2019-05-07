/******************************************************
文件名：	PrvtProt_EcDc.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/4/29		V1.0			liujian
*******************************************************/
#ifndef		_PRVTPROT_ECDC_H
#define		_PRVTPROT_ECDC_H
/*******************************************************
description： include the header file
*******************************************************/


/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_ECDC_DATA_LEN 	512//长度

#define PP_ENCODE_DISBODY 	0x01//编码dispatcher header
#define PP_ENCODE_APPDATA 	0x02//编码app data

//AID类型
#define PP_AID_ECALL 	170//Xcall

//MID类型
#define PP_MID_ECALL_REQ 	1//Xcall request
#define PP_MID_ECALL_RESP 	2//Xcall response

/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_APP_HEARTBEAT = 0,//心跳
	PP_ECALL_REQ,//ecall request
    PP_ECALL_RESP,//ecall response
	PP_APP_MAX
} PP_APP_TYPE;//应用类型

/*****struct definitions*****/
/* Dispatcher Body struct */
typedef struct 
{
	uint8_t	 	aID[3];
	uint8_t	 	mID;
	long	eventTime;
	long	eventId	/* OPTIONAL */;
	long	ulMsgCnt	/* OPTIONAL */;
	long	dlMsgCnt	/* OPTIONAL */;
	long	msgCntAcked	/* OPTIONAL */;
	int		ackReq	/* OPTIONAL */;
	long	appDataLen	/* OPTIONAL */;
	long	appDataEncode	/* OPTIONAL */;
	long	appDataProVer	/* OPTIONAL */;
	long	testFlag	/* OPTIONAL */;
	long	result	/* OPTIONAL */;
}PrvtProt_DisptrBody_t;

/* application data struct */
typedef struct 
{
	int  gpsSt;//gps状态 0-无效；1-有效
	long gpsTimestamp;//gps时间戳
	long latitude;//纬度 x 1000000,当GPS信号无效时，值为0
	long longitude;//经度 x 1000000,当GPS信号无效时，值为0
	long altitude;//高度（m）
	long heading;//车头方向角度，0为正北方向
	long gpsSpeed;//速度 x 10，单位km/h
	long hdop;//水平精度因子 x 10
}PrvtProt_Rvsposition_t;

/* application data struct */
typedef struct 
{
	long xcallType;//类型  1-道路救援   2-紧急救援（ecall）  3-400电话进线
	long engineSt;//启动状态；1-熄火；2-启动
	long totalOdoMr;//里程有效范围：0 - 1000000（km）
	PrvtProt_Rvsposition_t gpsPos;//车辆救援位置
	long srsSt;//安全气囊状态 1- 正常；2 - 弹出
	long updataTime;//数据时间戳
	long battSOCEx;//车辆电池剩余电量：0-10000（0%-100%）
}PrvtProt_EcDc_Xcall_t;

typedef struct
{
	PrvtProt_EcDc_Xcall_t Xcall;//xcall
}PrvtProt_appData_t;

/* message data struct */
typedef struct 
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t appData;
}PrvtProt_msgData_t;

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,long *msgDataLen, \
							  PrvtProt_DisptrBody_t *DisptrBody, PrvtProt_appData_t *Appchoice);
extern void PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
								  PrvtProt_msgData_t *msgData);
#endif 
