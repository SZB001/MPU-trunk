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
	PP_XCALL_REQ = 0,//xcall request
    PP_XCALL_RESP,//xcall response
	PP_APP_MID_MAX
} PP_APP_MID_TYPE;//应用类型
/*****struct definitions*****/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,long *msgDataLen, \
							  void *disptrBody, void *appchoice);
extern void PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
								  void *MsgData,int isdecodeAppdata);
#endif 
