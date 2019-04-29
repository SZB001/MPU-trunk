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
#define PP_AID_ECALL 	170//ecall

//MID类型
#define PP_MID_ECALL_REQ 	1//ecall request
#define PP_MID_ECALL_RESP 	2//ecall response

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
	PP_ECALL_REQ = 1,//ecall request
    PP_ECALL_RESP//ecall response
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
	uint8_t  parents;
	long xcallType;
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
extern int PrvtPro_msgPackage(uint8_t type,uint8_t *msgData,long *msgDataLen, \
							  PrvtProt_DisptrBody_t *DisptrBody, PrvtProt_appData_t *Appchoice);
extern void PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
								  PrvtProt_msgData_t *msgData);
#endif 
