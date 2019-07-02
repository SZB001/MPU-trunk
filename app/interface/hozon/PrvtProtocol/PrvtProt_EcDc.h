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
#define PP_ECDC_DATA_LEN 	1024//长度

#define PP_ENCODE_DISBODY 	0x01//编码dispatcher header
#define PP_ENCODE_APPDATA 	0x02//编码app data


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
	/*XCALL*/
	ECDC_XCALL_REQ = 0,//xcall request
    ECDC_XCALL_RESP,//xcall response

	/*remote config*/
	ECDC_RMTCFG_CHECK_REQ,//check remote config req
	ECDC_RMTCFG_GET_REQ,//get remote config req
	ECDC_RMTCFG_END_REQ,//remote config req end
	ECDC_RMTCFG_READ_REQ,//remote config read req
	ECDC_RMTCFG_CONN_RESP,//remote config conn resp
	ECDC_RMTCFG_READ_RESP,//remote config read req
	ECDC_RMTCTRL_RESP,//remote control resp
	ECDC_RMTCTRL_BOOKINGRESP,//remote control booking resp
	ECDC_RMTCTRL_HUBOOKINGRESP,//remote control HU booking resp
	ECDC_RMTVS_RESP,//remote check vehi status response

	/*remote diag*/
	ECDC_RMTDIAG_RESP,//remote diag status response
	ECDC_RMTDIAG_STATUS,//remote diag status
	ECDC_RMTDIAG_IMAGEACQRESP,//remote diag image acq response
	ECDC_APP_MID_MAX
} ECDC_APP_MID_TYPE;//应用类型
/*****struct definitions*****/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,int *msgDataLen, \
							  void *disptrBody, void *appchoice);
extern int PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
										void *DisBody,void *appData);
#endif 
